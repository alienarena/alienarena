/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2010 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

/*
-------------------------------------------------------------------------------
Alien Arena File System  (August 2010)
 *** with autotools build and filesystem update modifications ***

------ Windows XP, Vista, Windows 7 (MSVC or MinGW builds) ------

  c:\alienarena\
    data1\
      --- game data from release package
    arena\
      --- 3rd party, legacy, and downloaded game data
      --- screenshots, demos
      config.cfg
      autoexec.cfg
      qconsole.log
    botinfo\
    Tools\doc\
    crx.exe

------ Linux, Unix, Darwin ------

 --- Standard Install ---

  $prefix/          /usr/local/
    $bindir/          /usr/local/bin/
      crx
      crx-ded
    $datadir/         /usr/local/share/
      $pkgdatadir       /usr/local/share/alienarena/
        data1/
          --- shared game data from release package
        arena/
          --- shared 3rd party and legacy game data
        botinfo/
          --- shared bot information

  $COR_GAME         ~/.codered
    arena/
      --- downloaded 3rd party, legacy game data
      --- screenshots, demos
      config.cfg
      autoexec.cfg
      qconsole.log
    botinfo/
      --- user custom bot information

 --- Alternate (Traditional) Install ---

  /home/user/games/alienarena/  (for example)
    crx
    crx-ded
    data1/
    arena/
    botinfo/

  $COR_GAME         $(HOME)/.codered
    --- Same as Standard Install

--- Alien Arena ACEBOT File System ---

 - botinfo can be in one place or two places depending on the installation

 -- botinfo files and file types --
  - allbots.tmp   : data for bots included in the Add Bot menu.
  - team.tmp      : data for default set of bots spawned for a team game.
  - <botname>.cfg : config data for bots by name.
  - <mapname>.tmp : data for the set of bots spawned in a map.

--- Other Notes ---

.pak file support removed because:
  1. Alien Arena does not use it.
  2. simplifies filesystem modifications

link command removed because:
  1. it is probably obsolete (hack for DOS to simulate Unix link?)
  2. simplifies filesystem modifications
  3. might be a security issue?

basedir cvar removed because:
  1. it is probably an unnecessary complication.


--- Original Quake File System Comments ---
All of Quake's data access is through a hierchal file system, but the contents
of the file system can be transparently merged from several sources.

The "base directory" is the path to the directory holding the quake.exe and all
game directories.  The sys_* files pass this to host_init in
quakeparms_t->basedir.  This can be overridden with the "-basedir" command line
parm to allow code debugging in a different directory. The base directory is
only used during filesystem initialization.

The "game directory" is the first tree on the search path and directory that all
generated files (savegames, screenshots, demos, config files) will be saved to.
This can be overridden with the "-game" command line parameter. The game
directory can never be changed while quake is executing. This is a precacution
against having a malicious server instruct clients to write files over areas
they shouldn't.

-------------------------------------------------------------------------------
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined HAVE_UNISTD_H
#include <unistd.h>
#elif defined HAVE_DIRECT_H
/* for _getcwd in Windows */
#include <direct.h>
#endif

#if defined HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <errno.h>

#include "qcommon.h"
#include "game/q_shared.h"
#include "unix/glob.h"

#if !defined HAVE__GETCWD && defined HAVE_GETCWD
#define _getcwd getcwd
#endif

char fs_gamedir[MAX_OSPATH];
cvar_t *fs_gamedirvar; // for the "game" cvar, default is "arena"

/* --- Search Paths ---
 *
 * Pathname strings do not include a trailing '/'
 * The last slot is guard, containing an empty string
 * The bot pathname strings do not include the "botinfo" part.
 *
 */
#define GAME_SEARCH_SLOTS 7
char fs_gamesearch[GAME_SEARCH_SLOTS][MAX_OSPATH];

#define BOT_SEARCH_SLOTS 3
char fs_botsearch[BOT_SEARCH_SLOTS][MAX_OSPATH];

/**
 * @brief Initialize paths: data1, arena, botinfo, etc.
 *
 * @note This gets executed after command line "+set" commands are executed
 *       the first time. But, before any other commands from command line
 *       or .cfg files are executed.
 */
static void FS_init_paths( void )
{
	int i;
	char fs_bindir[MAX_OSPATH];
	char fs_datadir[MAX_OSPATH];
	char fs_homedir[MAX_OSPATH];
	char *cwdstr;
	char *homestr;

	char base_gamedata[MAX_OSPATH];
	char game_gamedata[MAX_OSPATH];
	char bot_gamedata[MAX_OSPATH];

#if defined UNIX_VARIANT
	char user_base_gamedata[MAX_OSPATH]; // possibly never used in Alien Arena
	char user_game_gamedata[MAX_OSPATH]; // configuration, logs, downloaded data
	char user_bot_gamedata[MAX_OSPATH]; //  user custom bot data
#endif

	memset(fs_bindir, 0, sizeof(fs_bindir));
	memset(fs_datadir, 0, sizeof(fs_datadir));
	memset(fs_homedir, 0, sizeof(fs_homedir));

	cwdstr = _getcwd( fs_bindir, sizeof(fs_bindir) );
	if ( cwdstr == NULL )
	{
		Sys_Error( "path initialization (getcwd error: %i)", strerror(errno) );
	}

#if defined UNIX_VARIANT
	homestr = getenv( "COR_GAME" );
	if ( homestr != NULL )
	{
		if ( strlen( homestr) >= sizeof(fs_homedir) || !strlen( homestr ) )
		{
			Sys_Error( "path initialization (getenv COR_GAME is %s)", homestr );
		}
		Q_strncpyz2( fs_homedir, homestr, sizeof( fs_homedir) );
	}
	else
	{
		homestr = getenv( "HOME" );
		if ( homestr != NULL )
		{
			Com_sprintf( fs_homedir, sizeof( fs_homedir), "%s/%s",
					homestr, USER_GAMEDATA );
		}
		else
		{
			fs_homedir[0] = 0;
		}
	}
#else
	homestr = NULL;
	fs_homedir[0] = 0;
#endif

#if !defined DATADIR
	// "Traditional" install, expect game data in current directory
	Q_strncpyz2( fs_datadir, fs_bindir, sizeof(fs_datadir) );
#else
	if ( !Q_strncasecmp( fs_bindir, DATADIR, sizeof(fs_bindir)) )
	{ // DATADIR defined, but it is current directory for executable
		// note: this may not need to be a special case
		Q_strncpyz2( fs_datadir, fs_bindir, sizeof(fs_datadir) );
	}
	else
	{ // "Standard" install. game data is not in same directory as executables
		if ( strlen( DATADIR ) >= sizeof( fs_datadir ) )
		{
			Sys_Error("DATADIR size error");
		}
		Q_strncpyz2( fs_datadir, DATADIR, sizeof(fs_datadir) );
	}
#endif

	// set path for "data1"
	memset( base_gamedata, 0, sizeof(base_gamedata) );
	Com_sprintf( base_gamedata, sizeof(base_gamedata), "%s/%s",
			fs_datadir, BASE_GAMEDATA );

	// set path for "arena" or mod
	memset( game_gamedata, 0, sizeof(game_gamedata) );
	fs_gamedirvar = Cvar_Get( "game", "", CVAR_LATCH|CVAR_SERVERINFO);
	if ( *fs_gamedirvar->string  && fs_gamedirvar->string[0] )
	{ // not empty
		if (  Q_strncasecmp( fs_gamedirvar->string, BASE_GAMEDATA, MAX_OSPATH )
				&& Q_strncasecmp( fs_gamedirvar->string, GAME_GAMEDATA, MAX_OSPATH ))
		{ // not "data1" nor "arena".  expect a mod data directory.
			Com_sprintf( game_gamedata, sizeof(game_gamedata), "%s/%s",
					fs_datadir, fs_gamedirvar->string );
		}
		else
		{ // was "data1" or "arena", use "arena"
			Com_sprintf( game_gamedata, sizeof(game_gamedata), "%s/%s",
					fs_datadir, GAME_GAMEDATA );
		}
	}
	else
	{ // was empty, set to "arena"
		fs_gamedirvar = Cvar_ForceSet("gamedir", GAME_GAMEDATA );
		Com_sprintf( game_gamedata, sizeof(game_gamedata), "%s/%s",
				fs_datadir, GAME_GAMEDATA );
	}

	// set path for "botinfo"
	// note: the "botinfo" part of the path, will be in the relative path
	//  so do not include it here
	memset( bot_gamedata, 0, sizeof(bot_gamedata) );
	Com_sprintf( bot_gamedata, sizeof(bot_gamedata), "%s", fs_datadir );

#if defined UNIX_VARIANT
	if ( fs_homedir[0] )
	{ // have a user HOME as expected
		/*
		 * use the ".codered" or $COR_GAME directory,
		 *  even if this is a Traditional install.
		 */
		// set "data1" path for user writeable data
		// Alien Arena maybe never uses this
		memset( user_base_gamedata, 0, sizeof(user_base_gamedata) );
		Com_sprintf( user_base_gamedata, sizeof(user_base_gamedata),
			"%s/%s", fs_homedir, BASE_GAMEDATA );

		// set "arena" path for user writeable data
		memset( user_game_gamedata, 0, sizeof(user_game_gamedata) );
		Com_sprintf( user_game_gamedata, sizeof(user_game_gamedata),
			"%s/%s", fs_homedir, GAME_GAMEDATA );

		// set "botinfo" path for user writeable data
		memset( user_bot_gamedata, 0, sizeof(user_bot_gamedata) );
		Com_sprintf( user_bot_gamedata, sizeof(user_bot_gamedata),
				"%s", fs_homedir );
	}
	else
	{
		// unexpected missing $HOME directory
		Sys_Error("set environment variable HOME (or COR_GAME) to a writeable directory.");
	}
#endif

	//
	// set up search priority sequence
	//
	// 2 or 4 places for game data
	for ( i = 0 ; i < GAME_SEARCH_SLOTS ; i++ )
	{
		memset( fs_gamesearch[i], 0, MAX_OSPATH );
	}
	i = 0;
#if defined UNIX_VARIANT
	if ( user_game_gamedata[0] )
	{ // default: ~/.codered/arena or $COR_GAME/arena
		Q_strncpyz2( fs_gamesearch[i], user_game_gamedata, sizeof(fs_gamesearch[0]));
		i++;
	}
#endif
	if ( game_gamedata[0] )
	{ // default: DATADIR/arena or $CWD/arena
		Q_strncpyz2( fs_gamesearch[i], game_gamedata, sizeof(fs_gamesearch[0]));
		i++;
	}
#if defined UNIX_VARIANT
	if ( user_base_gamedata[0] )
	{ // default: ~/.codered/data1  or $COR_GAME/data1 (may not be needed)
		Q_strncpyz2( fs_gamesearch[i], user_base_gamedata, sizeof(fs_gamesearch[0]));
		i++;
	}
#endif
	if ( base_gamedata[0] )
	{ // default: DATADIR/data1 or $CWD/data1
		Q_strncpyz2( fs_gamesearch[i], base_gamedata, sizeof(fs_gamesearch[0]));
	}

	// one or two places for bot information
	for ( i = 0 ; i < BOT_SEARCH_SLOTS ; i++ )
	{
		memset( fs_botsearch[i], 0, MAX_OSPATH );
	}
	i = 0;
#if defined UNIX_VARIANT
	if ( user_bot_gamedata[0] )
	{
		Q_strncpyz2( fs_botsearch[i], user_bot_gamedata, sizeof(fs_botsearch[0]));
		i++;
	}
#endif
	if ( bot_gamedata[0] )
	{
		Q_strncpyz2( fs_botsearch[i], bot_gamedata, sizeof(fs_botsearch[0]));
	}

	// set up fs_gamedir, location for writing various files
	//  this is the first directory in the search path
	Q_strncpyz2( fs_gamedir, fs_gamesearch[0], sizeof(fs_gamedir) );

}

/*
================
FS_filelength
================
*/
int FS_filelength( FILE *f )
{
	int length = -1;

#if defined HAVE_FILELENGTH

	length = filelength( fileno( f ) );

#elif defined HAVE_FSTAT

	struct stat statbfr;
	int result;

	result = fstat( fileno(f), &statbfr );
	if ( result != -1 )
	{
		length = (int)statbfr.st_size;
	}

#else

	long int pos;

	pos = ftell( f );
	fseek( f, 0L, SEEK_END );
	length = ftell( f );
	fseek( f, pos, SEEK_SET );

#endif

	return length;
}


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void FS_CreatePath (char *path)
{
	char	*ofs;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_Mkdir (path);
			*ofs = '/';
		}
	}
}


/*
===
FS_CheckFile

Given the full path to a file, find out whether or not the file actually
exists.
===
*/
#if defined HAVE_STAT

static qboolean FS_CheckFile( const char * search_path )
{
	struct stat statbfr;
	int result;
	result = stat( search_path, &statbfr );
	return ( result != -1 && S_ISREG(statbfr.st_mode) );
}

#else	// HAVE_STAT

static qboolean FS_CheckFile( const char * search_path )
{
	FILE *pfile;

	pfile = fopen( search_path, "rb");
	if ( ! pfile )
		return false;
	fclose( pfile );
	return true;
}

#endif	// HAVE_STAT


/*
===
FS_FullPath

Given a relative path to a file
  search for the file in the installation-dependent filesystem hierarchy
  if found, return true, with the constructed full path
  otherwise, return false, with a constructed writeable full path

===
 */
qboolean FS_FullPath( char *full_path, size_t pathsize, const char *relative_path )
{
	char search_path[MAX_OSPATH];
	char * to_search;
	qboolean found = false;

	if ( strlen( relative_path ) >= MAX_QPATH )
	{
		Com_DPrintf("FS_FullPath: relative path size error: %s\n", relative_path );
		return false;
	}

	if ( !Q_strncasecmp( relative_path, BOT_GAMEDATA, strlen(BOT_GAMEDATA) ) )
	{ // search in botinfo places
		to_search = &fs_botsearch[0][0];
	}
	else
	{ // search in the usual game data places
		to_search = &fs_gamesearch[0][0];
	}

	while ( to_search[0] && !found )
	{
		Com_sprintf( search_path, sizeof(search_path), "%s/%s",
					to_search, relative_path);
		found = FS_CheckFile( search_path );
		to_search += MAX_OSPATH;
	}

	if ( found )
	{
		if ( strlen( search_path ) < pathsize )
		{
			Q_strncpyz2( full_path, search_path, pathsize );
			if ( developer && developer->integer == 2 )
			{ // tracing for found files, not an error.
				Com_DPrintf("FS_FullPath: found : %s\n", full_path );
			}
		}
		else
		{
			Com_DPrintf("FS_FullPath: full path size error: %s\n", search_path );
			found = false;
		}
	}
	else if ( developer && developer->integer == 2 )
	{ // tracing for not found files, not necessarily an error.
		Com_DPrintf("FS_FullPath: not found : %s\n", relative_path );
	}

	return found;
}


/*
===
 FS_FullWritePath()

  Given a relative path for a file to be written
    Using the game writeable directory
    Create any sub-directories required
    Return the generated full path for file

===
*/
void FS_FullWritePath( char *full_path, size_t pathsize, const char* relative_path)
{
	if ( strlen( relative_path ) >= MAX_QPATH )
	{
		Com_DPrintf("FS_FullPath: relative path size error: %s\n", relative_path );
		*full_path = 0;
		return;
	}

	if ( !Q_strncasecmp( relative_path, BOT_GAMEDATA, strlen(BOT_GAMEDATA) ) )
	{ // a "botinfo/" prefixed relative path
		Com_sprintf( full_path, pathsize, "%s/%s", fs_botsearch[0], relative_path);
	}
	else
	{ // writeable path for game data, same as fs_gamesearch[0]
		Com_sprintf( full_path, pathsize, "%s/%s", fs_gamedir, relative_path);
	}

	FS_CreatePath( full_path );

}

/*
==============
FS_FCloseFile
==============
*/
void FS_FCloseFile (FILE *f)
{
	fclose (f);
}

/*
===========
FS_FOpenFile

 Given relative path, search for a file
  if found, open it and return length
  if not found, return length -1 and NULL file

===========
*/
int FS_FOpenFile (char *filename, FILE **file)
{
	char netpath[MAX_OSPATH];
	qboolean found;
	int length = -1;

	found = FS_FullPath( netpath, sizeof(netpath), filename );
	if ( found )
	{
		*file = fopen( netpath, "rb" );
		if ( !(*file) )
		{
			Com_DPrintf("FS_FOpenFile: failed file open: %s:\n", netpath);
		}
		else
		{
			length = FS_filelength( *file );

			// On error, the file's length will be negative, and we probably
			// can't read from that.
			if ( length < 0 )
			{
				FS_FCloseFile( *file );
				*file = NULL;
			}
		}
	}
	else
	{
		*file = NULL;
	}

	return length;
}

/*
=================
FS_ReadFile

Properly handles partial reads
=================
*/
#define	MAX_READ	0x10000		// read in blocks of 64k
void FS_Read (void *buffer, int len, FILE *f)
{
	int		block, remaining;
	int		read;
	byte	*buf;
	int		tries;

	buf = (byte *)buffer;

	// read in chunks for progress bar
	remaining = len;
	tries = 0;
	while (remaining)
	{
		block = remaining;
		if (block > MAX_READ)
			block = MAX_READ;
		read = fread (buf, 1, block, f);
		if (read == 0)
		{
			if ( feof( f ) )
				Com_Error( ERR_FATAL, "FS_Read: premature end-of-file");
			if ( ferror( f ) )
				Com_Error( ERR_FATAL, "FS_Read: file read error");
			Com_Error (ERR_FATAL, "FS_Read: 0 bytes read");
		}
		if (read == -1)
			Com_Error (ERR_FATAL, "FS_Read: -1 bytes read");

		// do some progress bar thing here...

		remaining -= read;
		buf += read;
	}
}

/*
==================
FS_TolowerPath

Makes the path to the given file lowercase on case-sensitive systems.
Kludge for case-sensitive and case-insensitive systems inteoperability.
Background: Some people may extract game paths/files as uppercase onto their
            HDD (while their system is case-insensitive, so game will work, but
            will case trouble for case-sensitive systems if they are acting
            as servers [propagating their maps etc. in uppercase]). Indeed the
            best approach here would be to make fopen()ing always case-
            insensitive, but due to resulting complexity and fact that
            Linux people will always install the game files with correct
            name casing, this is just fine.
-JR / 20050802 / 1
==================
*/
char *FS_TolowerPath (char *path)
{
	int	i = 0;
	static char buf[MAX_OSPATH]; // path can be const char *, so thats why

	do
		buf[i] = tolower(path[i]);
	while (path[i++]);

	return buf;
}

/*
============
FS_LoadFile

Given relative path
 if buffer arg is NULL, just return the file length
 otherwise,
   Z_malloc a buffer, read the file into it, return pointer to the new buffer
============
*/
int FS_LoadFile (char *path, void **buffer)
{
	FILE	*h;
	byte	*buf = NULL;
	int		len;
	char lc_path[MAX_OSPATH];

	len = FS_FOpenFile (path, &h);

	//-JR
	if (!h)
	{
		Q_strncpyz2( lc_path, FS_TolowerPath( path ), sizeof(lc_path) );
		if ( strcmp( path, lc_path ) )
		{ // lowercase conversion changed something
			len = FS_FOpenFile( lc_path, &h);
		}
	}

	if (!h)
	{
		if (buffer)
			*buffer = NULL;
		return -1;
	}

	if (!buffer)
	{
		FS_FCloseFile (h);
		return len;
	}

	buf = Z_Malloc(len+1);
	buf[len] = 0;
	*buffer = buf;

	FS_Read (buf, len, h);

	FS_FCloseFile (h);

	return len;
}

/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile (void *buffer)
{
	Z_Free (buffer);
}

/*
=========
FS_FileExists

 Given relative path, search for a file
  if found, return true
  if not found, return false

========
 */
qboolean FS_FileExists(char *path)
{
	char fullpath[MAX_OSPATH];

	return ( FS_FullPath( fullpath, sizeof(fullpath), path ) );
}

/*
============
FS_Gamedir

Called to find where to write a file (demos, savegames, etc)
============
*/
char *FS_Gamedir (void)
{

	return fs_gamedir;

}

/*
=============
FS_ExecAutoexec
=============
*/
void FS_ExecAutoexec (void)
{
	char name [MAX_OSPATH];
	int i;

	// search through all the paths for an autoexec.cfg file
	for ( i = 0; fs_gamesearch[i][0] ; i++ )
	{
		Com_sprintf(name, sizeof(name), "%s/autoexec.cfg", fs_gamesearch[i] );

		if (Sys_FindFirst(name, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM))
		{
			Cbuf_AddText ("exec autoexec.cfg\n");
			Sys_FindClose();
			break;
		}
		Sys_FindClose();
	}
}

/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.

================
*/
void FS_SetGamedir (char *dir)
{

	if (strstr(dir, "..") || strstr(dir, "/")
		|| strstr(dir, "\\") || strstr(dir, ":") )
	{
		Com_Printf ("Gamedir should be a single filename, not a path\n");
		return;
	}
	//
	// flush all data, so it will be forced to reload
	//
	if ( dedicated && !dedicated->value )
	{ // set up for client restart
		Cbuf_AddText ("vid_restart\nsnd_restart\n");
	}

	// note: FullSet triggers sending info to client
	if ( !strcmp( dir, BASE_GAMEDATA ) || ( *dir == 0 ) )
	{ // either "data1" or "", set to empty string.
		Cvar_FullSet ("gamedir", "", CVAR_SERVERINFO|CVAR_NOSET);
	}
	else
	{
		Cvar_FullSet ("gamedir", dir, CVAR_SERVERINFO|CVAR_NOSET);
	}

	// re-initialize search paths
	FS_init_paths();

}

/*
** FS_ListFiles
**
** IMPORTANT: does not count the guard in returned "numfiles" anymore, to
** avoid adding/subtracting 1 all the time.
*/
char **FS_ListFiles( char *findname, int *numfiles, unsigned musthave, unsigned canthave )
{
	char *namestr;
	int nfiles = 0;
	char **list = NULL; // pointer to array of pointers

	// --- count the matching files ---
	namestr = Sys_FindFirst( findname, musthave, canthave );
	while ( namestr  )
	{
		if ( namestr[ strlen( namestr )-1 ] != '.' ) // not '..' nor '.'
			nfiles++;
		namestr = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose ();

	if ( !nfiles )
		return NULL;

	*numfiles = nfiles;
	nfiles++; // add space for a guard

	// --- create the file name string array ---
	list = malloc( sizeof( char * ) * nfiles ); // array of pointers
	memset( list, 0, sizeof( char * ) * nfiles );

	namestr = Sys_FindFirst( findname, musthave, canthave );
	nfiles = 0;
	while ( namestr )
	{
		if ( namestr[ strlen(namestr) - 1] != '.' )  // not ".." nor "."
		{
			list[nfiles] = strdup( namestr );  // list[n] <- malloc'd pointer
#if defined WIN32_VARIANT
			_strlwr( list[nfiles] );
#endif
			nfiles++;
		}
		namestr = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose ();

	return list;
}

void FS_FreeFileList (char **list, int n) // jit
{
	int i;

	for (i = 0; i < n; i++)
	{
		free(list[i]);
		list[i] = 0;
	}

	free(list);
}


/*
 * FS_ListFilesInFS
 *
 * Create a list of files that match a criteria.
 *
 * Searchs are relative to the game directory and use all the search paths
 */
char **
FS_ListFilesInFS(char *findname, int *numfiles, unsigned musthave, unsigned canthave)
{
//	searchpath_t	*search;		/* Search path. */
	int		i, j;			/* Loop counters. */
	int		nfiles;			/* Number of files found. */
	int		tmpnfiles;		/* Temp number of files. */
	char		**tmplist;		/* Temporary list of files. */
	char		**list;			/* List of files found. */
	char		path[MAX_OSPATH];	/* Temporary path. */
	int s;

	nfiles = 0;
	list = malloc(sizeof(char *));

	for ( s = 0 ; fs_gamesearch[s][0]; s++ )
	{ // for non-empty search slots
		Com_sprintf(path, sizeof(path), "%s/%s",fs_gamesearch[s], findname);
		tmplist = FS_ListFiles(path, &tmpnfiles, musthave, canthave);
		if (tmplist != NULL)
		{
			nfiles += tmpnfiles;
			list = realloc(list, nfiles * sizeof(char *));
			for (i = 0, j = nfiles - tmpnfiles; i < tmpnfiles; i++, j++)
			{ // copy from full path to relative path
				list[j] = strdup( tmplist[i] + strlen( fs_gamesearch[s] ) + 1  );
			}
			FS_FreeFileList(tmplist, tmpnfiles);
		}
	}

	/* Delete duplicates. */
	tmpnfiles = 0;
	for (i = 0; i < nfiles; i++)
	{
		if (list[i] == NULL)
			continue;
		for (j = i + 1; j < nfiles; j++)
			if (list[j] != NULL &&
			    strcmp(list[i], list[j]) == 0) {
				free(list[j]);
				list[j] = NULL;
				tmpnfiles++;
			}
	}

	if (tmpnfiles > 0) {
		nfiles -= tmpnfiles;
		tmplist = malloc(nfiles * sizeof(char *));
		for (i = 0, j = 0; i < nfiles + tmpnfiles; i++)
			if (list[i] != NULL)
				tmplist[j++] = list[i];
		free(list);
		list = tmplist;
	}

	/* Add a guard. */
	if (nfiles > 0)
	{
		nfiles++;
		list = realloc(list, nfiles * sizeof(char *));
		list[nfiles - 1] = NULL;
	} else
	{
		free(list);
		list = NULL;
	}

	/* IMPORTANT: Don't count the guard when returning nfiles. */
	nfiles--;

	*numfiles = nfiles;

	return (list);
}

/*
** FS_Dir_f
**
** target of "dir" command
*/
void FS_Dir_f( void )
{
	char	*path = NULL;
	char	findname[1024];
	char	wildcard[1024] = "*.*";
	char	**dirnames;
	int		ndirs;

	if ( Cmd_Argc() != 1 )
	{
		strcpy( wildcard, Cmd_Argv( 1 ) );
	}

	while ( ( path = FS_NextPath( path ) ) != NULL )
	{
		char *tmp = findname;

		Com_sprintf( findname, sizeof(findname), "%s/%s", path, wildcard );

		// limit to game directories.
		// FIXME: figure out a way to look in botinfo directories
		if ( strstr( findname, ".." ) )
		{
			continue;
		}

		while ( *tmp != 0 )
		{
			if ( *tmp == '\\' )
				*tmp = '/';
			tmp++;
		}
		Com_Printf( "Directory of %s\n", findname );
		Com_Printf( "----\n" );

		if ( ( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 ) ) != 0 )
		{
			int i;

			for ( i = 0; i < ndirs; i++ )
			{
				if ( strrchr( dirnames[i], '/' ) )
					Com_Printf( "%s\n", strrchr( dirnames[i], '/' ) + 1 );
				else
					Com_Printf( "%s\n", dirnames[i] );

				free( dirnames[i] );
			}
			free( dirnames );
		}
		Com_Printf( "\n" );
	};
}

/*
============
FS_Path_f

 target of "path" command

============
*/
void FS_Path_f (void)
{
	int i;

	Com_Printf ("Game data search path:\n");
	for ( i = 0; fs_gamesearch[i][0] ; i++ )
	{
		Com_Printf( "%s/\n", fs_gamesearch[i] );
	}
	Com_Printf ("Bot data search path:\n");
	for ( i = 0; fs_botsearch[i][0] ; i++ )
	{
		Com_Printf( "%s/%s/\n", fs_botsearch[i], BOT_GAMEDATA );
	}

}

/*
================
FS_NextPath

Allows enumerating all of the directories in the search path

 prevpath to NULL on first call, previously returned char* on subsequent
 returns NULL when done

================
*/
char *FS_NextPath (char *prevpath)
{
	char *nextpath;
	int i = 0;

	if ( prevpath == NULL )
	{ // return the first
		nextpath = fs_gamesearch[0];
	}
	else
	{ // scan the fs_gamesearch elements for an address match with prevpath
		nextpath = NULL;
		while ( prevpath != fs_gamesearch[i++] )
		{
			if ( i >= GAME_SEARCH_SLOTS )
			{
				Sys_Error("Program Error in FS_NextPath()");
			}
		}
		if ( fs_gamesearch[i][0] )
		{ // non-empty slot
			nextpath = fs_gamesearch[i];
		}
	}

	return nextpath;
}

/*
================
FS_InitFilesystem
================
*/
void FS_InitFilesystem (void)
{
#if defined UNIX_VARIANT
	char dummy_path[MAX_OSPATH];
#endif

	Cmd_AddCommand ("path", FS_Path_f);
	Cmd_AddCommand ("dir", FS_Dir_f );

	FS_init_paths();

#if defined UNIX_VARIANT
	Com_Printf("using %s for writing\n", fs_gamedir );
	/*
	 * Create the writeable directory if it does not exist.
	 * Otherwise, a dedicated server may fail to create a logfile.
	 * "dummy" is there just so FS_CreatePath works, no file is created.
	 */
	Com_sprintf( dummy_path, sizeof(dummy_path), "%s/dummy", fs_gamedir );
	FS_CreatePath( dummy_path );
#endif

}
