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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MREMAP
#define _GNU_SOURCE   // -jjb-ac necessary?, mremap() is GNU extension
// shouldn't this be in config.h or built-in ???
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "unix/glob.h"

#include "qcommon/qcommon.h"

/*
 * State of currently open Hunk.
 * Valid between Hunk_Begin() and Hunk_End()
 * -jjb-fix  maybe should update to use mmap2()
 */
byte *hunk_base = NULL;
size_t *phunk_size_store = NULL;
static const size_t hunk_header_size = 32;
byte *user_hunk_base = NULL ;
size_t rsvd_hunk_size;
size_t user_hunk_size;
size_t total_hunk_used_size;


#ifndef HAVE_MREMAP
static size_t round_page( size_t requested_size )
{
	size_t page_size;
	size_t num_pages;

	/* round requested size to next higher page aligned size
	 * assumes requested_size is non-zero, so always yields at least one page
	 * also always yields at least 1 byte more than requested size
	 */
	page_size = (size_t)sysconf( _SC_PAGESIZE );
	num_pages = (requested_size / page_size) + 1;
	page_size *= even_pages;

	return page_size;
}
#endif


void *Hunk_Begin (int maxsize)
{
#if 0
	// -jjb-dbg
	Com_Printf("[Hunk_Begin : 0x%X :]\n", maxsize );
	if ( hunk_base != NULL )
		Com_Printf("Warning: Hunk_Begin possible program error\n");
#endif

	// reserve virtual memory space
	rsvd_hunk_size = (size_t)maxsize + hunk_header_size;
	hunk_base = mmap(0, rsvd_hunk_size, PROT_READ|PROT_WRITE,
			MAP_PRIVATE|MAP_ANON, -1, 0);
	if ( hunk_base == NULL || hunk_base == (byte *)-1)
		Sys_Error("unable to virtual allocate %d bytes", maxsize);

	total_hunk_used_size = hunk_header_size;
	user_hunk_size = 0;
	user_hunk_base = hunk_base + hunk_header_size;

	// store the size reserved for Hunk_Free()
	phunk_size_store = (size_t *)hunk_base;
	*phunk_size_store = rsvd_hunk_size; // the initial reserved size

	return user_hunk_base;
}

void *Hunk_Alloc (int size)
{
	byte *user_hunk_bfr;
	size_t user_hunk_block_size;
	size_t new_user_hunk_size;

	if ( hunk_base == NULL ) {
		Sys_Error("Hunk program error");
	}

	user_hunk_block_size = ((size_t)size + 1) & ~0x01; // size is sometimes odd
	new_user_hunk_size = user_hunk_size + user_hunk_block_size;
	total_hunk_used_size += user_hunk_block_size;

	if ( total_hunk_used_size > rsvd_hunk_size )
		Sys_Error("Hunk_Alloc overflow");

	user_hunk_bfr = user_hunk_base + user_hunk_size; // set base of new block
	user_hunk_size = new_user_hunk_size; // then update user hunk size

#if 0
	// -jjb-dbg
	Com_Printf("[Hunk_Alloc : %u @ %p :]\n",
			user_hunk_block_size, user_hunk_bfr );
#endif

	return (void *)user_hunk_bfr;
}

int Hunk_End (void)
{
	byte *remap_base;
	size_t new_rsvd_hunk_size;

	// shrink reserved size to size used
	new_rsvd_hunk_size = total_hunk_used_size;
	remap_base = mremap( hunk_base, rsvd_hunk_size, new_rsvd_hunk_size, 0 );
	if ( remap_base != hunk_base ) {
		Sys_Error("Hunk_End:  Could not remap virtual block (%d)", errno);
	}

	// "close" this hunk, setting reserved size for Hunk_Free
	rsvd_hunk_size = new_rsvd_hunk_size;
	*phunk_size_store = rsvd_hunk_size;

#if 0
	// -jjb-dbg
	Com_Printf("[Hunk_End : 0x%X @ %p :]\n", rsvd_hunk_size, remap_base );
#endif

	hunk_base = user_hunk_base = NULL;
	phunk_size_store = NULL;

	return user_hunk_size; // user buffer is user_hunk_size @ user_hunk_base
}

void Hunk_Free (void *base)
{
	byte *hunk_base;
	size_t hunk_rsvd_size;

	if ( base != NULL )
	{
		// find hunk base and retreive the hunk reserved size
		hunk_base = base - hunk_header_size;
		hunk_rsvd_size = *((size_t *)hunk_base);
#if 0
		// -jjb-dbg
		Com_Printf("[Hunk_Free : 0x%X @ %p :]\n", hunk_rsvd_size, hunk_base );
#endif
		if ( munmap( hunk_base, hunk_rsvd_size ) )
			Sys_Error("Hunk_Free: munmap failed (%d)", errno);
	}

}

//===============================================================================


/*
================
Sys_Milliseconds
================
*/
int curtime;
int Sys_Milliseconds (void)
{
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;
	int timeofday;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	timeofday = (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;

	return timeofday;
}

void Sys_Mkdir (char *path)
{
    mkdir (path, 0777);
}

//============================================

static	char	findbase[MAX_OSPATH];
static	char	findpath[MAX_OSPATH];
static	char	findpattern[MAX_OSPATH];
static	DIR		*fdir;

static qboolean CompareAttributes(char *path, char *name,
	unsigned musthave, unsigned canthave )
{
	struct stat st;
	char fn[MAX_OSPATH];

// . and .. never match
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return false;

	return true;

	if (stat(fn, &st) == -1)
		return false; // shouldn't happen

	if ( ( st.st_mode & S_IFDIR ) && ( canthave & SFF_SUBDIR ) )
		return false;

	if ( ( musthave & SFF_SUBDIR ) && !( st.st_mode & S_IFDIR ) )
		return false;

	return true;
}

char *Sys_FindFirst (char *path, unsigned musthave, unsigned canhave)
{
	struct dirent *d;
	char *p;

	if (fdir)
		Sys_Error ("Sys_BeginFind without close");

//	COM_FilePath (path, findbase);
	strcpy(findbase, path);

	if ((p = strrchr(findbase, '/')) != NULL) {
		*p = 0;
		strcpy(findpattern, p + 1);
	} else
		strcpy(findpattern, "*");

	if (strcmp(findpattern, "*.*") == 0)
		strcpy(findpattern, "*");

	if ((fdir = opendir(findbase)) == NULL)
		return NULL;
	while ((d = readdir(fdir)) != NULL) {
		if (!*findpattern || glob_match(findpattern, d->d_name)) {
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d->d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

char *Sys_FindNext (unsigned musthave, unsigned canhave)
{
	struct dirent *d;

	if (fdir == NULL)
		return NULL;
	while ((d = readdir(fdir)) != NULL) {
		if (!*findpattern || glob_match(findpattern, d->d_name)) {
//			if (*findpattern)
//				printf("%s matched %s\n", findpattern, d->d_name);
			if (CompareAttributes(findbase, d->d_name, musthave, canhave)) {
				sprintf (findpath, "%s/%s", findbase, d->d_name);
				return findpath;
			}
		}
	}
	return NULL;
}

void Sys_FindClose (void)
{
	if (fdir != NULL)
		closedir(fdir);
	fdir = NULL;
}


//============================================


