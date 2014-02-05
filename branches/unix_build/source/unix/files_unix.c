/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2013 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

A copy of the  GNU General Public License is included;
see the file, COPYING.

*/

/**
 * @file files_unix.c
 *
 * @brief CRX Engine system dependent file system for Unix/GNU Linux
 *
 * Replaces qcommon/files.c
 */

/* -jjb-
 * to be re-integrated with Windows
 * 2014-01:
 * -- botinfo/ special-case separate top-level eliminated. now a subdir
 *     of data1 and game
 * -- basename of user writeable path is not configurable.
 *     now is always "cor_games"
 * -- use XDG conventions if defined
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined HAVE_ZLIB
# include <zlib.h>
#endif

#include "qcommon/qcommon.h"


/* ---- Full path for shared game resources ----*/

#if !defined COR_DATADIR
ERROR
#endif

/* ---- Path basenames for game resources ---*/

#define BASE_GAMEDATA "data1"
#define USER_GAMEDATA "cor_games"

#if defined TACTICAL
# define GAME_GAMEDATA "tactical"
#else
# define GAME_GAMEDATA "arena"
#endif

/*----------------------------------------*/

#define SFF_INPACK 0x20 /* For FS_ListFilesInFS(). */

char   fs_gamedir[MAX_OSPATH];
cvar_t *fs_gamedirvar;

#define GAME_SEARCH_SLOTS  8
char fs_gamesearch[GAME_SEARCH_SLOTS][MAX_OSPATH];
int  fs_slots_used;

const char* xdg_data_home_default = ".local/share";

#if 0
/* technically, should use this for shared data
 * but right now using -DCOR_DATADIR instead
 */
const char* xdg_data_dir_default = 	"/usr/local/share:/usr/share";
#endif


/* helper function */
static inline bool
is_absolute_path( const char* path )
{
	return *path == '/';
}

/* helper function */
static bool
is_valid_path( const char* path )
{
	return true;
}


/**
 * Initialize the pathnames for the shared data directory and
 *  the user home data directory.
 */
static void
FS_init_paths( void )
{
	int  i;
	char fs_bindir[MAX_OSPATH];
	char fs_datadir[MAX_OSPATH];
	char fs_homedir[MAX_OSPATH];
	char *cwdstr;
	char *homestr;
    char *xdg_data_home;
	char base_gamedata[MAX_OSPATH];
	char game_gamedata[MAX_OSPATH];
	char user_base_gamedata[MAX_OSPATH];
	char user_game_gamedata[MAX_OSPATH];

	memset( fs_bindir, 0, sizeof( fs_bindir ));
	memset( fs_datadir, 0, sizeof( fs_datadir ));
	memset( fs_homedir, 0, sizeof( fs_homedir ));
	memset( base_gamedata, 0, sizeof( base_gamedata ));
	memset( game_gamedata, 0, sizeof( game_gamedata ));
	memset( user_base_gamedata, 0, sizeof( user_base_gamedata ));
	memset( user_game_gamedata, 0, sizeof( user_game_gamedata ));
	for ( i = 0; i < GAME_SEARCH_SLOTS; i++ )
		memset( fs_gamesearch[i], 0, sizeof( fs_gamesearch[0]) );

	cwdstr = getcwd( fs_bindir, sizeof( fs_bindir ));
	if ( cwdstr == NULL )
		Sys_Error( strerror(errno) );
	
	xdg_data_home = getenv( "XDG_DATA_HOME" );
	if ( xdg_data_home == NULL || !is_absolute_path( xdg_data_home ) )
		xdg_data_home = xdg_data_home_default;

	homestr = getenv( "HOME" );
	if ( homestr != NULL )
		/* absolute path to user-writeable data */
		Com_sprintf( fs_homedir, sizeof( fs_homedir ), 
					 "%s/%s/%s", homestr, xdg_data_home, USER_GAMEDATA );
	else
		Sys_Error( "\nCould not initialize HOME subdirectory\n" );
	

	/* absolute path where shared data files are installed */
	if ( strnlen( COR_DATADIR, sizeof(fs_datadir) ) >= sizeof( fs_datadir ) )
	{
		Com_Printf( "\nCOR_DATADIR is: %s\n", COR_DATADIR );
		Sys_Error( "\nCOR_DATADIR path name is too long\n" );
	}
	Q_strncpyz2( fs_datadir, COR_DATADIR, sizeof( fs_datadir ));

	/* absolute path where official game resource files are installed */
	Com_sprintf( base_gamedata, sizeof( base_gamedata ), "%s/%s",
				 fs_datadir, BASE_GAMEDATA );

	/* absolute paths where official game-dependent data are stored.
	 * If the game name is not set in the first 'Early' commands
	 *  then it will be 'arena'
	 * (Note: there seems to have been a command line arg '-game'
	 *  to set the game name early. TODO: add that?  */
	fs_gamedirvar = Cvar_Get( "gamedir", "", CVAR_LATCH | CVAR_SERVERINFO );
	if ( *fs_gamedirvar->string && fs_gamedirvar->string[0] )
	{
		/* gamedir cvar specified */
		Com_sprintf( game_gamedata, sizeof( game_gamedata ),
					 "%s/%s", fs_datadir, fs_gamedirvar->string );
		Com_sprintf( user_game_gamedata, sizeof( user_game_gamedata ),
					 "%s/%s", fs_homedir, fs_gamedirvar->string );
	}
	else
	{
		/* gamedir cvar not specified, use GAME_GAMEDATA (arena or tactical) */
		fs_gamedirvar = Cvar_ForceSet( "gamedir", GAME_GAMEDATA );
		Com_sprintf( game_gamedata, sizeof( game_gamedata ),
					 "%s/%s", fs_datadir, GAME_GAMEDATA );
		Com_sprintf( user_game_gamedata, sizeof(user_game_gamedata),
					 "%s/%s", fs_homedir, GAME_GAMEDATA );
	}
	/* absolute path to BASE_GAMEDATA (data1) in user home */
	Com_sprintf( user_base_gamedata, sizeof(user_base_gamedata),
				 "%s/%s", fs_homedir, BASE_GAMEDATA );

	/* 
	 * Create absolute search paths
	 * As of 2014-02 version:
	 * In the user home directory there are 2: data1 and arena -xor- tactical
	 * In the shared directory there are 2: data1 and arena -xor- tactical
	 * Preference order:
	 *   home game, shared game, home data1, shared data1
	 */

	i = 0;
	Q_strncpyz2(fs_gamesearch[i++],user_game_gamedata,sizeof(fs_gamesearch[0]));
	Q_strncpyz2( fs_gamesearch[i++], game_gamedata, sizeof(fs_gamesearch[0]) );
	Q_strncpyz2(fs_gamesearch[i++],user_base_gamedata,sizeof(fs_gamesearch[0]));
	Q_strncpyz2( fs_gamesearch[i++], base_gamedata, sizeof(fs_gamesearch[0]) );
	fs_slots_used = i;

	/* the 'game' directory is the same as the first search slot */
	Q_strncpyz2( fs_gamedir, fs_gamesearch[0], sizeof( fs_gamedir ));

	// -jjb- TODO: test accessibility, user_game_gamedata should be writeable 

} /* FS_init_paths */

/**
 *
 * @param f -
 * @return -
 */
int
FS_filelength( FILE *f )
{
	int length = -1;

#if defined HAVE_FILELENGTH
	length = filelength( fileno( f ));
#elif defined HAVE_FSTAT
	struct stat statbfr;
	int         result;
	result = fstat( fileno( f ), &statbfr );
	if ( result != -1 )
	{
		length = (int)statbfr.st_size;
	}
#else
	long int    pos;
	pos = ftell( f );
	fseek( f, 0L, SEEK_END );
	length = ftell( f );
	fseek( f, pos, SEEK_SET );
#endif /* if defined HAVE_FILELENGTH */

	return length;

} /* FS_filelength */

/**
 *
 * @param path -
 */
void
FS_CreatePath ( char *path )
{
	char *ofs;

	for ( ofs = path + 1; *ofs; ofs++ )
	{
		if ( *ofs == '/' )
		{
			*ofs = 0;
			Sys_Mkdir( path );
			*ofs = '/';
		}
	}

} /* FS_CreatePath */

/**
 *
 * @param search_path -
 * @return -
 */
static qboolean
FS_CheckFile( const char *search_path )
{
#if defined HAVE_STAT

	struct stat statbfr;
	int result;

	result = stat( search_path, &statbfr );

	return result != -1 && S_ISREG( statbfr.st_mode );

#else

	FILE *pfile;

	pfile = fopen( search_path, "rb" );
	if ( !pfile )
		return false;

	fclose( pfile );

	return true;

#endif /* if defined HAVE_STAT */
} /* FS_CheckFile */


/**
 *
 * @param full_path -
 * @param pathsize -
 * @param relative_path -
 * @return -
 */
qboolean
FS_FullPath( char *full_path, size_t pathsize, const char *relative_path )
{
	char     search_path[MAX_OSPATH];
	qboolean found = false;
	int      i;

	*full_path = 0;
	if ( strlen( relative_path ) >= MAX_QPATH )
	{
		Com_DPrintf( "FS_FullPath: relative path size error: %s\n",
					 relative_path );
		return false;
	}

	for ( i = 0 ; !found && i < fs_slots_used ; ++i )
	{
		Com_sprintf( search_path, sizeof(search_path), "%s/%s",
					 fs_gamesearch[i], relative_path );
		found = FS_CheckFile( search_path );
	}

	if ( found )
	{
		if ( strlen( search_path ) < pathsize )
		{
			Q_strncpyz2( full_path, search_path, pathsize );
			if ( developer && developer->integer == 2 )
				Com_DPrintf( "FS_FullPath: found : %s\n", full_path );
		}
		else
		{
			Com_DPrintf( "FS_FullPath: pathsize error: %s\n", search_path );
			found = false;
		}
	}
	else if ( developer && developer->integer == 2 ) // -jjb- improve this
	{
		Com_DPrintf( "FS_FullPath: not found : %s\n", relative_path );
	}

	return found;

} /* FS_FullPath */

/**
 *
 * @param full_path -
 * @param pathsize -
 * @param relative_path -
 */
void
FS_FullWritePath( char *full_path, size_t pathsize, const char *relative_path )
{
	if ( strlen( relative_path ) >= MAX_QPATH )
	{
		Com_DPrintf( "FS_FullPath: relative path size error: %s\n",
					 relative_path );
		*full_path = 0;
		return;
	}
	Com_sprintf( full_path, pathsize, "%s/%s", fs_gamedir, relative_path );
	FS_CreatePath( full_path );

} /* FS_FullWritePath */

/**
 *
 * @param f -
 */
void
FS_FCloseFile( FILE *f )
{
	fclose( f );
} /* FS_FCloseFile */

/**
 *
 * @param filename -
 * @param file -
 * @return -
 */
int
FS_FOpenFile( char *filename, FILE **file )
{
	char     netpath[MAX_OSPATH];
	qboolean found;
	int      length = -1;

	found = FS_FullPath( netpath, sizeof( netpath ), filename );
	if ( found )
	{
		*file = fopen( netpath, "rb" );
		if ( !( *file ))
		{
			Com_DPrintf( "FS_FOpenFile: failed file open: %s:\n", netpath );
		}
		else
		{
			length = FS_filelength( *file );
			if ( length < 0 )
			{
				FS_FCloseFile( *file );
				*file = 0;
			}
		}
	}
	else
	{
		*file = 0;
	}

	return length;

} /* FS_FOpenFile */

// -jjb- why the limit??? something to do with ZMalloc? 
#define MAX_READ  0x10000

/**
 *
 * @param buffer -
 * @param len -
 * @param f -
 */
void
FS_Read( void *buffer, int len, FILE *f )
{
	int  block, remaining;
	int  read;
	byte *buf;

	buf       = (byte*)buffer;
	remaining = len;
	while ( remaining )
	{
		block = remaining;
		if ( block > MAX_READ )
			block = MAX_READ;
		read = fread( buf, 1, block, f );
		// -jjb- fix this oddity, maybe return -1 on errors
		if ( read == 0 )
		{
			if ( feof( f ))
				Com_Error( ERR_FATAL, "FS_Read: premature end-of-file" );
			if ( ferror( f ))
				Com_Error( ERR_FATAL, "FS_Read: file read error" );
			Com_Error( ERR_FATAL, "FS_Read: 0 bytes read" );
		}
		if ( read == -1 )
			Com_Error( ERR_FATAL, "FS_Read: -1 bytes read" );
		remaining -= read;
		buf       += read;
	}

} /* FS_Read */

/**
 *
 * @param path -
 * @return -
 */
char*
FS_TolowerPath( char *path )
{
	int i = 0;
	static char buf[MAX_OSPATH];

	do
	{
		buf[i] = tolower( path[i] );
	}
	while ( path[i++] );

	return buf;

} /* FS_TolowerPath */

/**
 *
 * @param path -
 * @param buffer -
 * @return -
 */
int
FS_LoadFile( char *path, void **buffer )
{
	FILE *h;
	byte *buf = NULL;
	int  len;
	char lc_path[MAX_OSPATH];

	len = FS_FOpenFile( path, &h );
	if ( !h )
	{
		Q_strncpyz2( lc_path, FS_TolowerPath( path ), sizeof( lc_path ));
		if ( strcmp( path, lc_path ))
		{
			len = FS_FOpenFile( lc_path, &h );
		}
	}
	if ( !h )
	{
		if ( buffer )
			*buffer = NULL;
		return -1;
	}
	if ( !buffer )
	{
		FS_FCloseFile( h );
		return len;
	}
	buf      = Z_Malloc( len + 1 );
	buf[len] = 0;
	*buffer  = buf;
	FS_Read( buf, len, h );
	FS_FCloseFile( h );

	return len;

} /* FS_LoadFile */

/**
 *
 * @param path -
 * @param buffer -
 * @param statbuffer -
 * @param statbuffer_len -
 * @return -
 */
int
FS_LoadFile_TryStatic( char *path, void **buffer, void *statbuffer,
					   size_t statbuffer_len )
{
	FILE *h;
	byte *buf = NULL;
	int  len;
	char lc_path[MAX_OSPATH];

	len = FS_FOpenFile( path, &h );
	if ( !h )
	{
		Q_strncpyz2( lc_path, FS_TolowerPath( path ), sizeof( lc_path ));
		if ( strcmp( path, lc_path ))
			len = FS_FOpenFile( lc_path, &h );
	}
	if ( !h )
	{
		if ( buffer )
			*buffer = NULL;
		return -1;
	}
	if ( buffer == NULL || statbuffer == NULL || statbuffer_len == 0 )
	{
		FS_FCloseFile( h );
		return len;
	}
	if ( statbuffer_len > len )
	{
		memset( statbuffer, 0, statbuffer_len );
		buf = statbuffer;
	}
	else
	{
		buf = Z_Malloc( len + 1 );
	}
	buf[len] = 0;
	*buffer  = buf;
	FS_Read( buf, len, h );
	FS_FCloseFile( h );

	return len;

} /* FS_LoadFile_TryStatic */

/**
 *
 * @param buffer -
 */
void
FS_FreeFile( void *buffer )
{

	Z_Free( buffer );

} /* FS_FreeFile */

/**
 *
 * @param path -
 * @return -
 */
qboolean
FS_FileExists( char *path )
{
	char fullpath[MAX_OSPATH];

	return FS_FullPath( fullpath, sizeof( fullpath ), path );

} /* FS_FileExists */

/**
 *
 * @return -
 */
char*
FS_Gamedir ( void )
{

	return fs_gamedir;

} /* FS_Gamedir */

/**
 *
 *
 */
void
FS_ExecAutoexec( void )
{
	char name [MAX_OSPATH];
	int  i;

	for ( i = 0; fs_gamesearch[i][0]; i++ )
	{
		Com_sprintf( name, sizeof( name ), "%s/autoexec.cfg", fs_gamesearch[i] );
		if ( Sys_FindFirst( name, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM ))
		{
			Cbuf_AddText( "exec autoexec.cfg\n" );
			Sys_FindClose();
			break;
		}
		Sys_FindClose();
	}

} /* FS_ExecAutoexec */

/**
 *
 * @param dir -
 */
void
FS_SetGamedir( char *dir )
{

	if ( strstr( dir, ".." ) || strstr( dir, "/" )
		 || strstr( dir, "\\" ) || strstr( dir, ":" ))
	{
		Sys_Warn( "\nGame directory should be one word, not a path\n" );
		return;
	}

#if !defined DEDICATED_ONLY
	Cbuf_AddText( "vid_restart\nsnd_restart\n" );
#endif

	// -jjb- check this, requesting data1 should do arena?
	if ( !strcmp( dir, BASE_GAMEDATA ) || ( *dir == 0 ))
		Cvar_FullSet( "gamedir", "", CVAR_SERVERINFO | CVAR_NOSET );
	else
		Cvar_FullSet( "gamedir", dir, CVAR_SERVERINFO | CVAR_NOSET );

	FS_init_paths();

} /* FS_SetGamedir */

/*----------------------------------------------------- Directory Listing ----*/

// -jjb- research dirent stuff

/**
 *
 * @param list -
 * @param n -
 */
void
FS_FreeFileList ( char **list, int n )
{
	int i;

	for ( i = 0; i < n; i++ )
	{
		free( list[i] );
		list[i] = 0;
	}

	free( list );

} /* FS_FreeFileList */

/**
 *
 * @param findname -
 * @param numfiles - output, number of files in the list
 * @param musthave -
 * @param canthave -
 * @return -
 */
char**
FS_ListFiles( char *findname, int *numfiles,
			  unsigned musthave, unsigned canthave )
{
	char *namestr;
	int  nfiles = 0;
	char **list = NULL;

	namestr = Sys_FindFirst( findname, musthave, canthave );
	while ( namestr )
	{
		if ( namestr[ strlen( namestr ) - 1 ] != '.' )
			nfiles++;
		namestr = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose();
	if ( !nfiles )
		return NULL;

	*numfiles = nfiles;
	nfiles++;

// -jjb- calloc need HAVE?
	list = calloc( nfiles, sizeof(char*) );
	if ( list == NULL )
		return NULL;
	/* memset( list, 0, sizeof( char* ) * nfiles );*/

	namestr = Sys_FindFirst( findname, musthave, canthave );
	nfiles  = 0;
	while ( namestr )
	{
		if ( namestr[ strlen( namestr ) - 1] != '.' )
		{
			list[nfiles] = strdup( namestr ); // -jjb- need HAVE?
			if ( list[nfiles] == NULL )
			{
				if ( nfiles )
				{
					FS_FreeFileList( list, nfiles );
					return NULL;
				}
			}
			nfiles++;
		}
		namestr = Sys_FindNext( musthave, canthave );
	}
	Sys_FindClose();

	return list;

} /* FS_ListFiles */

/**
 *
 * @param findname -
 * @param numfiles -
 * @param musthave -
 * @param canthave -
 * @return -
 */
char**
FS_ListFilesInFS( char *findname, int *numfiles,
				  unsigned musthave, unsigned canthave )
{
	int  i, j;
	int  nfiles;
	int  tmpnfiles;
	char **tmplist;
	char **list;
	char **new_list;
	char path[MAX_OSPATH];
	int  s;

	nfiles = 0;
	list   = malloc( sizeof( char* ));
	if ( list == NULL )
		return NULL;
	for ( s = 0; fs_gamesearch[s][0]; s++ )
	{
		Com_sprintf( path, sizeof(path), "%s/%s", fs_gamesearch[s], findname );
		tmplist = FS_ListFiles( path, &tmpnfiles, musthave, canthave );
		if ( tmplist != NULL )
		{
			nfiles  += tmpnfiles;
			new_list = realloc( list, nfiles * sizeof( char* ));
			if ( new_list == NULL )
			{
				FS_FreeFileList( tmplist, tmpnfiles );
				// -jjb- Com_Printf( "WARN: SYSTEM MEMORY EXHAUSTION!\n" );
				break;
			}
			list = new_list;
			for ( i = 0, j = nfiles - tmpnfiles; i < tmpnfiles; i++, j++ )
			{
				list[j] = strdup( tmplist[i] + strlen( fs_gamesearch[s] ) + 1 );
			}

			FS_FreeFileList( tmplist, tmpnfiles );
		}
	}

	tmpnfiles = 0;
	for ( i = 0; i < nfiles; i++ )
	{
		if ( list[i] == NULL )
			continue;
		for ( j = i + 1; j < nfiles; j++ )
		{
			if ( list[j] != NULL &&
				 strcmp( list[i], list[j] ) == 0 ) // -jjb-
			{
				free( list[j] );
				list[j] = NULL;
				tmpnfiles++;  // -jjb- ???
			}
		}
	}
	if ( tmpnfiles > 0 )
	{
		nfiles -= tmpnfiles;
		tmplist = malloc( nfiles * sizeof( char* ));
		for ( i = 0, j = 0; i < nfiles + tmpnfiles; i++ )
		{
			if ( list[i] != NULL )
				tmplist[j++] = list[i];
		}

		free( list );
		list = tmplist;
	}
	if ( nfiles > 0 )
	{
		nfiles++;
		new_list = realloc( list, nfiles * sizeof( char* ));
		if ( new_list == NULL )
		{
			// -jjb- Com_Printf( "WARN: SYSTEM MEMORY EXHAUSTION!\n" );
			nfiles--;
		}
		else
		{
			list = new_list;
		}
		list[nfiles - 1] = NULL;
	}
	else
	{
		free( list );
		list = NULL;
	}
	nfiles--; // -jjb- ???
	*numfiles = nfiles;

	return list;

} /* FS_ListFilesInFS */

/**
 *
 *
 */
void
FS_Dir_f( void )
{
	char *path = NULL;
	char findname[1024];
	char wildcard[1024] = "*.*";
	char **dirnames;
	int  ndirs;

	if ( Cmd_Argc() != 1 )
	{
		strcpy( wildcard, Cmd_Argv( 1 ));
	}
	while (( path = FS_NextPath( path )) != NULL )
	{
		char *tmp = findname;
		Com_sprintf( findname, sizeof( findname ), "%s/%s", path, wildcard );
		if ( strstr( findname, ".." ))
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
		if (( dirnames = FS_ListFiles( findname, &ndirs, 0, 0 )) != 0 )
		{
			int i;
			for ( i = 0; i < ndirs; i++ )
			{
				if ( strrchr( dirnames[i], '/' ))
					Com_Printf( "%s\n", strrchr( dirnames[i], '/' ) + 1 );
				else
					Com_Printf( "%s\n", dirnames[i] );
				free( dirnames[i] );
			}

			free( dirnames );
		}
		Com_Printf( "\n" );
	}
} /* FS_Dir_f */

/**
 * Target of "path" console command
 *
 */
void
FS_Path_f( void )
{
	int i;

	Com_Printf( "Game data search path:\n" );
	for ( i = 0; fs_gamesearch[i][0]; i++ )
		Com_Printf( "%s/\n", fs_gamesearch[i] );

} /* FS_Path_f */

/**
 *
 * @param prevpath -
 * @return -
 */
char*
FS_NextPath ( char *prevpath )
{
	char *nextpath;
	int  i = 0;

	if ( prevpath == NULL )
	{
		nextpath = fs_gamesearch[0];
	}
	else
	{
		nextpath = NULL;
		while ( prevpath != fs_gamesearch[i++] )
		{
			if ( i >= GAME_SEARCH_SLOTS )
			{
				Sys_Error( "Program Error in FS_NextPath()" );
			}
		}
		if ( fs_gamesearch[i][0] )
		{
			nextpath = fs_gamesearch[i];
		}
	}

	return nextpath;

} /* FS_NextPath */

/**
 *
 */
void
FS_InitFilesystem ( void )
{

	char dummy_path[MAX_OSPATH];

	Cmd_AddCommand( "path", FS_Path_f );
	Cmd_AddCommand( "dir", FS_Dir_f );

	FS_init_paths();

#if !defined DEDICATED_ONLY
	Com_Printf( "Configuration and Download Directory: %s\n", fs_gamedir );
	// -jjb- maybe stuff a "path" command instead
#endif
	Com_sprintf( dummy_path, sizeof( dummy_path ), "%s/dummy", fs_gamedir );
	FS_CreatePath( dummy_path ); // -jjb- ???

#if defined HAVE_ZLIB && !defined DEDICATED_ONLY
	Com_Printf( "using zlib version %s\n", zlibVersion());
#endif

} /* FS_InitFilesystem */
