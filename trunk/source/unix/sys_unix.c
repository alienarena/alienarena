/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <errno.h>

#include <dlfcn.h>

#include "../qcommon/qcommon.h"

#include "../unix/rw_unix.h"

cvar_t *nostdout;

unsigned	sys_frame_time;

uid_t saved_euid;
qboolean stdin_active = true;

// =======================================================================
// General routines
// =======================================================================

void Sys_ConsoleOutput (char *string)
{
	if (nostdout && nostdout->value)
		return;

	fputs(string, stdout);
}

void Sys_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	unsigned char		*p;

	va_start (argptr,fmt);
	vsnprintf (text,1024,fmt,argptr);
	va_end (argptr);

	if (nostdout && nostdout->value)
        return;

	for (p = (unsigned char *)text; *p; p++) {
		*p &= 0x7f;
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
	}
}

void Sys_Quit (void)
{
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
	CL_Shutdown ();
	Qcommon_Shutdown ();
	exit(0);
}

void Sys_Init(void)
{
#if id386
//	Sys_SetFPCW();
#endif
}

//void Sys_Error (const char *error, ...)
void Sys_Error (char *error, ...)
{
	va_list     argptr;
	char        string[1024];

// change stdin to non blocking
	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

	CL_Shutdown ();
	Qcommon_Shutdown ();

	va_start (argptr,error);
	vsnprintf (string,1024,error,argptr);
	va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

	exit (1);

}

void Sys_Warn (char *warning, ...)
{
	va_list     argptr;
	char        string[1024];

	va_start (argptr,warning);
	vsnprintf (string,1024,warning,argptr);
	va_end (argptr);
	fprintf(stderr, "Warning: %s", string);
}

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int	Sys_FileTime (char *path)
{
	struct	stat	buf;

	if (stat (path,&buf) == -1)
		return -1;

	return buf.st_mtime;
}

void floating_point_exception_handler(int whatever)
{
//	Sys_Warn("floating point exception\n");
	signal(SIGFPE, floating_point_exception_handler);
}

char *Sys_ConsoleInput(void)
{
    static char text[256];
    int     len;
	fd_set	fdset;
    struct timeval timeout;

	if (!dedicated || !dedicated->value)
		return NULL;

	if (!stdin_active)
		return NULL;

	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select (1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET(0, &fdset))
		return NULL;

	len = read (0, text, sizeof(text));
	if (len == 0) { // eof!
		stdin_active = false;
		return NULL;
	}

	if (len < 1)
		return NULL;
	text[len-1] = 0;    // rip off the /n and terminate

	return text;
}

/*****************************************************************************/

static void *game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (game_library)
		dlclose (game_library);
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);

	FILE	*fp;
	char	name[MAX_OSPATH];
	char	*path;
	char	*str_p;
	const char *gamename = "game.so";

	setreuid(getuid(), getuid());
	setegid(getgid());

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	Com_Printf("------- Loading %s -------\n", gamename);

	// now run through the search paths
	path = NULL;
	while (1)
	{
		path = FS_NextPath (path);
		if (!path)
			return NULL;		// couldn't find one anywhere
		snprintf (name, MAX_OSPATH, "%s/%s", path, gamename);

		/* skip it if it just doesn't exist */
		fp = fopen(name, "rb");
		if (fp == NULL)
			continue;
		fclose(fp);

		game_library = dlopen (name, RTLD_NOW);
		if (game_library)
		{
			Com_Printf ("LoadLibrary (%s)\n",name);
			break;
		}
		else
		{
			Com_Printf ("LoadLibrary (%s):", name);

			path = dlerror();
			str_p = strchr(path, ':'); // skip the path (already shown)
			if (str_p == NULL)
				str_p = path;
			else
				str_p++;

			Com_Printf ("%s\n", str_p);

			return NULL;
		}
	}

	GetGameAPI = (void *)dlsym (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();
		return NULL;
	}

	return GetGameAPI (parms);
}

/*****************************************************************************/

void Sys_AppActivate (void)
{
}

void HandleEvents(void);

void Sys_SendKeyEvents (void)
{
#ifndef DEDICATED_ONLY
		HandleEvents();
#endif

	// grab frame time
	sys_frame_time = Sys_Milliseconds();
}

/*****************************************************************************/

int main (int argc, char **argv)
{
	int 	time, oldtime, newtime;

	// go back to real user for config loads
	saved_euid = geteuid();
	seteuid(getuid());

	Qcommon_Init(argc, argv);

	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);

	nostdout = Cvar_Get("nostdout", "0", 0);
	if (!nostdout->value) {
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
	}

    oldtime = Sys_Milliseconds ();
    while (1)
    {
// find time spent rendering last frame
		do {
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);
		// curtime setting moved from Sys_Milliseconds()
		//   so it consistent for entire frame 
		curtime = newtime; 
		
        Qcommon_Frame (time);
		oldtime = newtime;
    }

}


