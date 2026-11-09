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
// console.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"
#include "ref_gl/qgl.h"


/* The console's main structure */
struct CON_console_s	CON_console;

/* CVar for notifications display time */
cvar_t *		con_notifytime;

// ignore color codes (useful if you're trying to kick/ban someone with lots
// of color codes in their name)
cvar_t			*con_ignorecolorcodes;
#define cmode	(con_ignorecolorcodes != NULL && con_ignorecolorcodes->integer ? FNT_CMODE_NONE : FNT_CMODE_QUAKE_SRS)
cvar_t			*con_color;

// Amount of chat lines (wrapped screen lines, not raw messages) shown by
// the chat overlay
cvar_t			*cl_chatlines;

float consolecolors[ 6 ][ 4 ] =
{
	{0.0, 0.0, 0.0, 0.2},
	{0.5, 0.0, 0.0, 0.2},
	{0.0, 0.5, 0.0, 0.2},
	{0.0, 0.0, 0.5, 0.2},
	{0.0, 0.5, 0.5, 0.2},
	{0.5, 0.0, 0.5, 0.2}
};

float consolecolors_menu[ 6 ][ 4 ] =
{
	{0.0, 0.0, 0.0, 0.85},
	{0.1, 0.0, 0.0, 0.85},
	{0.0, 0.1, 0.0, 0.85},
	{0.0, 0.0, 0.1, 0.85},
	{0.0, 0.1, 0.1, 0.85},
	{0.1, 0.0, 0.1, 0.85}
};

extern viddef_t vid;

/**************************************************************************/
/* VARIOUS COMMANDS                                                       */
/**************************************************************************/

/*
 * Show or hide the console.
 */
void CON_ToggleConsole( void )
{
	// remembers whether the console was opened over the menu, so closing it
	// (tilde or escape) goes back to the menu instead of always dumping the
	// player into the game - and so the menu's own navigation state isn't
	// wiped out from under it by the M_ForceMenuOff() below while it does
	static qboolean con_entered_from_menu = false;

	SCR_EndLoadingPlaque ();	// get rid of loading plaque

	CON_ClearNotify( );

	if (cls.key_dest == key_console)
	{
		if (con_entered_from_menu)
		{
			cls.key_dest = key_menu;
			Key_ClearStates ();
			if (Cvar_VariableValue ("maxclients") == 1
				&& Com_ServerState ())
				Cvar_Set ("paused", "1");
		}
		else
		{
			M_ForceMenuOff ();
			Cvar_Set ("paused", "0");
		}
	}
	else
	{
		con_entered_from_menu = (cls.key_dest == key_menu);
		if (!con_entered_from_menu)
			M_ForceMenuOff ();
		cls.key_dest = key_console;

		if (Cvar_VariableValue ("maxclients") == 1
			&& Com_ServerState ())
			Cvar_Set ("paused", "1");
	}
}


/*
 * Clear the chat area
 */
static void _CON_ToggleChat( void )
{
	Key_ClearTyping( );

	if ( cls.key_dest == key_console ) {
		if ( cls.state == ca_active ) {
			M_ForceMenuOff( );
			cls.key_dest = key_game;
		}
	} else {
		cls.key_dest = key_console;
	}

	CON_ClearNotify( );
}


/*
 * Toggle the general chat input
 */
static void _CON_GeneralChatInput( void )
{
	chat_team = false;
	chat_irc = false;
	cls.key_dest = key_message;
	Cbuf_AddText("chatbubble\n");
	Cbuf_Execute ();
}


/*
 * Toggle the team chat input
 */
static void _CON_TeamChatInput( void )
{
	chat_team = true;
	chat_irc = false;
	cls.key_dest = key_message;
	Cbuf_AddText("chatbubble\n");
	Cbuf_Execute ();
}


/*
 * Toggle the IRC input
 */
static void _CON_IRCInput( void )
{
	chat_team = false;
	chat_irc = true;
	cls.key_dest = key_message;
}



/**************************************************************************/
/* CONSOLE BUFFER ACCESS                                                  */
/**************************************************************************/

/*
 * Find the start of the current line
 *
 * Parameters:
 *	line	the initial line to look from
 *
 * Returns:
 *	the identifier of the current line in the buffer
 */
static int _CON_FindLineStart( int line )
{
	line = ( line + CON_MAX_LINES ) % CON_MAX_LINES;
	while ( CON_console.lCount[ line ] == 0 ) {
		line = ( line - 1 + CON_MAX_LINES ) % CON_MAX_LINES;
	}
	return line;
}


/*
 * Find the start of the next line
 *
 * Parameters:
 *	line	the current location
 *
 * Returns:
 *	the identifier of the next line in the buffer
 */
static int _CON_FindNextLine( int line )
{
	line = ( line + CON_MAX_LINES ) % CON_MAX_LINES;
	if ( CON_console.lCount[ line ] == 0 ) {
		while ( CON_console.lCount[ line ] == 0 ) {
			line = ( line + 1 ) % CON_MAX_LINES;
		}
	} else {
		line = ( line + CON_console.lCount[ line ] ) % CON_MAX_LINES;
	}
	return line;
}


/*
 * Find the start of the previous line
 *
 * Parameters:
 *	line	the current location
 *
 * Returns:
 *	the identifier of the previous line in the buffer
 */
static int _CON_FindPreviousLine( int line )
{
	return _CON_FindLineStart( _CON_FindLineStart( line ) - 1 );
}


/*
 * End the current line and start a new one
 */
static void _CON_NewLine( )
{
	CON_console.times[ CON_console.curTime ] = cls.realtime;
	CON_console.curTime = ( CON_console.curTime + 1 ) % CON_MAX_NOTIFY;

	CON_console.curLine = ( CON_console.curLine + 1 ) % CON_MAX_LINES;
	CON_console.lines += 1 - CON_console.lCount[ CON_console.curLine ];
	CON_console.lCount[ CON_console.curLine ] = 1;

	CON_console.offset = 0;
	memset( CON_console.text[ CON_console.curLine ] , 0 , CON_LINE_LENGTH );

	CON_console.heights[ CON_console.curLine ] = 0;
}


/*
 * Go back to the start of the current line
 */
static void _CON_RestartLine( )
{
	CON_console.curLine = _CON_FindLineStart( CON_console.curLine );
	CON_console.offset = 0;
}


/*
 * Append a character to the console buffer
 *
 * Parameters:
 *	new_char	the character to append
 */
static void _CON_AppendChar( char new_char )
{
	int lStart = _CON_FindLineStart( CON_console.curLine );

	CON_console.text[ CON_console.curLine ][ CON_console.offset ] = new_char;
	CON_console.offset = ( CON_console.offset + 1 ) % CON_LINE_LENGTH;
	CON_console.heights[ lStart ] = 0;

	if ( CON_console.offset == 0 ) {
		// Start a new buffer line that is part of the current logical line
		CON_console.lCount[ lStart ] ++;
		CON_console.curLine = ( CON_console.curLine + 1 ) % CON_MAX_LINES;
		CON_console.lines += 1 - CON_console.lCount[ CON_console.curLine ];
		memset( CON_console.text[ CON_console.curLine ] , 0 , CON_LINE_LENGTH );
	}
}


/*
 * Copy the specified logical line into a buffer
 *
 * If the current buffer line is full, the logical line will be extended to
 * include the next buffer line.
 *
 * Parameters:
 *	buffer		the buffer to copy to
 *	line		the first buffer line of the logical line
 *
 * Notes:
 *	The buffer should have a sufficient size, which can be computed
 *		using lCount[ line ] * CON_LINE_LENGTH.
 */
static void _CON_CopyLine( char * buffer , int line )
{
	int len = CON_console.lCount[ line ];
	int i;
	for ( i = 0 ; i < len ; i ++ ) {
		memcpy( buffer , CON_console.text[ line ] , CON_LINE_LENGTH );
		buffer += CON_LINE_LENGTH;
		line = ( line + 1 ) % CON_MAX_LINES;
	}
}



/**************************************************************************/
/* NOTIFICATIONS                                                          */
/**************************************************************************/

/* Clear all notifications */
void CON_ClearNotify( )
{
	int		i;
	for ( i = 0 ; i < CON_MAX_NOTIFY ; i ++ ) {
		CON_console.times[ i ] = 0;
	}
}


/* Draw the few last lines of the console transparently over the game */
void CON_DrawNotify( )
{
	FNT_font_t		font;
	struct FNT_window_s	box;
	int			line;
	int			bLines;
	int			count;
	int			timeIndex;

	font = FNT_AutoGet( CL_gameFont );
	box.x = 0;
	box.y = 0;

	// Find the first logical line to display
	line = _CON_FindLineStart( CON_console.curLine );
	bLines = CON_console.lines - CON_console.lCount[ line ];
	timeIndex = ( CON_console.curTime - 1 + CON_MAX_NOTIFY ) % CON_MAX_NOTIFY;
	for ( count = 0 ; count < CON_MAX_NOTIFY && bLines > 0 ; count ++ ) {
		int time = CON_console.times[ timeIndex ];
		if ( time == 0 || cls.realtime - time > con_notifytime->value * 1000 ) {
			break;
		}
		timeIndex = ( timeIndex - 1 + CON_MAX_NOTIFY ) % CON_MAX_NOTIFY;

		line = _CON_FindPreviousLine( line );
		bLines -= CON_console.lCount[ line ];
	}

	// No lines to display
	if ( count == 0 ) {
		return;
	}

	// Display lines
	while ( count > 0 ) {
		box.width = viddef.width;
		box.height = 0;
		if ( CON_console.lCount[ line ] == 1 ) {
			FNT_WrappedPrint( font , CON_console.text[ line ] , FNT_CMODE_QUAKE_SRS , FNT_ALIGN_LEFT , 30 , &box , FNT_colors[ 7 ] );
		} else {
			char * buffer = Z_Malloc( CON_LINE_LENGTH * CON_console.lCount[ line ] );
			_CON_CopyLine( buffer , line );
			FNT_WrappedPrint( font , buffer , FNT_CMODE_QUAKE_SRS , FNT_ALIGN_LEFT , 30 , &box , FNT_colors[ 7 ] );
			Z_Free( buffer );
		}
		box.y += box.height;

		line = _CON_FindNextLine( line );
		timeIndex = ( timeIndex + 1 ) % CON_MAX_NOTIFY;
		count --;
	}
}



/**************************************************************************/
/* CHAT OVERLAY                                                           */
/**************************************************************************/

/* Ring buffer storing the lines for one chat history (say/team/IRC) */
typedef struct
{
	char	lines[ CON_CHAT_HISTORY_LINES ][ CON_CHAT_LINE_LENGTH ];
	int	count;		// amount of valid entries, caps at CON_CHAT_HISTORY_LINES
	int	next;		// index at which the next line will be written
	float	lastAddTime;	// cls.realtime of the most recent addition
} chatHistory_t;

static chatHistory_t	chatHistory[ CHAT_HISTORY_COUNT ];


/*
 * Say/team chat lines are always built by the server as "name: message"
 * (optionally with a "[TEAM] " prefix first) - the name comes first, so
 * the *first* ": " in the string is always the end of it. If the player's
 * name ends with a color code and never resets it, that color would
 * otherwise bleed into the whole message with no explicit reset of its
 * own (now that history lines are drawn with persisting color codes, see
 * CON_DrawChatOverlay). Insert an explicit reset right there.
 */
static void _CON_ChatResetColorAfterName( char * buf , size_t bufSize )
{
	char *	sep = strstr( buf , ": " );
	size_t	len;
	size_t	insertAt;

	if ( ! sep ) {
		return;
	}

	insertAt = ( sep - buf ) + 2;	// right after ": "
	len = strlen( buf );
	if ( len + 2 >= bufSize ) {
		return;	// no room to insert without truncating content
	}

	memmove( buf + insertAt + 2 , buf + insertAt , len - insertAt + 1 );	// +1 for the NUL
	buf[ insertAt ] = '^';
	buf[ insertAt + 1 ] = '7';
}


/* Add a line to the specified chat history, stripping any trailing newline */
void CON_ChatHistoryAdd( chatHistoryType_t type , const char * text )
{
	chatHistory_t *	hist;
	char *			dest;
	size_t			len;

	if ( type < 0 || type >= CHAT_HISTORY_COUNT ) {
		return;
	}

	hist = &chatHistory[ type ];
	dest = hist->lines[ hist->next ];
	Q_strncpyz2( dest , text , CON_CHAT_LINE_LENGTH );

	len = strlen( dest );
	while ( len > 0 && ( dest[ len - 1 ] == '\n' || dest[ len - 1 ] == '\r' ) ) {
		dest[ -- len ] = 0;
	}

	hist->next = ( hist->next + 1 ) % CON_CHAT_HISTORY_LINES;
	if ( hist->count < CON_CHAT_HISTORY_LINES ) {
		hist->count ++;
	}
	hist->lastAddTime = cls.realtime;
}


/* Auto-detect say vs team chat (from the server's "[TEAM] " prefix) and store it */
void CON_ChatCapture( const char * text )
{
	chatHistoryType_t	type = ! strncmp( text , "[TEAM] " , 7 ) ? CHAT_HISTORY_TEAM : CHAT_HISTORY_SAY;
	char			fixed[ CON_CHAT_LINE_LENGTH ];

	Q_strncpyz2( fixed , text , sizeof( fixed ) );
	_CON_ChatResetColorAfterName( fixed , sizeof( fixed ) );

	CON_ChatHistoryAdd( type , fixed );
}


/*
 * Fetch a line from a chat history by its distance from the most recently
 * added line (0 = newest).
 *
 * Returns:
 *	the requested line, or NULL if fromNewest is out of range
 */
static const char * _CON_ChatHistoryGet( chatHistory_t * hist , int fromNewest )
{
	int idx;

	if ( fromNewest < 0 || fromNewest >= hist->count ) {
		return NULL;
	}

	idx = ( hist->next - 1 - fromNewest + CON_CHAT_HISTORY_LINES * 2 ) % CON_CHAT_HISTORY_LINES;
	return hist->lines[ idx ];
}


/*
 * Draw the console-styled background panel (tinted fill plus decorative
 * corner/edge tiles) used by the chat overlay. This mirrors the in-game
 * styling used by CON_DrawConsole (same con_color palette and tile
 * graphics), but is kept separate since the chat overlay never needs the
 * "from menu" styling used while disconnected.
 *
 * Parameters:
 *	font	the font whose size drives the tile scaling
 *	height	the height of the panel's content area, in pixels
 *
 * Returns:
 *	the panel's actual visual bottom edge, in pixels - this extends past
 *	"height" because the bottom corner/edge tiles and the tinted fill both
 *	overshoot it; callers that need to avoid overlapping the panel
 *	entirely (not just its content area) should use this instead of
 *	"height" itself.
 */
static int _CON_DrawPanelBackground( FNT_font_t font , int height )
{
	int	_tile_w , _tile_h;
	float	tile_w , tile_h;
	int	color = con_color->integer;
	int	fillBottom , tileBottom;

	Draw_GetPicSize( &_tile_w , &_tile_h , "menu/m_topcorner" );

	tile_w = (float)_tile_w / 64.0 * (float)font->size * 4.0;
	tile_h = (float)_tile_h / 64.0 * (float)font->size * 4.0;

	if ( color < 0 || color > 5 ) {
		color = 0;
	}

	if ( color > 0 ) {
		Draw_Fill_CutCorners( font->size / 3 , 0 , viddef.width - font->size / 1.5 , height + font->size * 1.5 ,
			consolecolors[ color ] , font->size , RCF_BOTTOMLEFT|RCF_BOTTOMRIGHT );
	}

	Draw_AlphaStretchTilingPic( -tile_w/4 , height-tile_h/2 , tile_w , tile_h , "menu/m_bottomcorner" , 1 );
	Draw_AlphaStretchTilingPic( viddef.width+tile_w/4 , height-tile_h/2 , -tile_w , tile_h , "menu/m_bottomcorner" , 1 );

	Draw_AlphaStretchTilingPic( tile_w*0.75 , height-tile_h/2 , viddef.width-tile_w*1.5 , tile_h , "menu/m_bottom" , 1 );

	Draw_AlphaStretchTilingPic( -tile_w/4 , 0 , tile_w , height-tile_h/2 , "menu/m_side" , 1 );
	Draw_AlphaStretchTilingPic( viddef.width+tile_w/4 , 0 , -tile_w , height-tile_h/2 , "menu/m_side" , 1 );
	Draw_AlphaStretchTilingPic( tile_w*0.75 , 0 , viddef.width-tile_w*1.5 , height-tile_h/2 , "menu/m_background" , 1 );

	fillBottom = ( color > 0 ) ? (int)( height + font->size * 1.5 ) : height;
	tileBottom = (int)( height + tile_h / 2 );
	return ( tileBottom > fillBottom ) ? tileBottom : fillBottom;
}


/* Draw the chat overlay: history of the active chat channel plus the input line */
void CON_DrawChatOverlay( void )
{
	FNT_font_t		font;
	struct FNT_window_s	box;
	chatHistoryType_t	type;
	chatHistory_t *		hist;
	const char *		prompt;
	int			maxLines;
	int			textWidth;
	int			includedHeights[ CON_CHAT_HISTORY_LINES ];
	int			includedCount;
	int			totalScreenLines;
	int			textHeight;
	int			inputRowHeight;
	int			panelHeight;
	int			panelVisualBottom;
	int			i , y;

	font = FNT_AutoGet( CL_gameFont );

	if ( chat_team ) {
		type = CHAT_HISTORY_TEAM;
		prompt = "say_team: ";
	} else if ( chat_irc ) {
		type = CHAT_HISTORY_IRC;
		prompt = "say_IRC: ";
	} else {
		type = CHAT_HISTORY_SAY;
		prompt = "say: ";
	}
	hist = &chatHistory[ type ];

	maxLines = cl_chatlines->integer;
	if ( maxLines < 1 ) {
		maxLines = 1;
	}
	if ( maxLines > CON_CHAT_HISTORY_LINES ) {
		maxLines = CON_CHAT_HISTORY_LINES;
	}

	textWidth = viddef.width - font->size * 5;

	// Walk backwards from the most recent message, measuring its wrapped
	// height (in screen lines) off-screen, until the requested amount of
	// screen lines is reached. Always include at least one message.
	includedCount = 0;
	totalScreenLines = 0;
	for ( i = 0 ; i < hist->count ; i ++ ) {
		const char *	text = _CON_ChatHistoryGet( hist , i );
		int		lines;

		box.x = 0;
		box.y = viddef.height;	// draw off-screen, we only want the resulting height
		box.width = textWidth;
		box.height = 0;
		// FNT_CMODE_QUAKE (not _SRS): chat messages are free text where a
		// color code is meant to persist across multiple words - SRS mode
		// resets to the base color at every space, corrupting that.
		// FNT_colors[7] (white): matches the existing plain console
		// notification's default, so uncolored text (e.g. the "[TEAM] "
		// prefix) looks the same regardless of which element shows it.
		FNT_WrappedPrint( font , text , FNT_CMODE_QUAKE , FNT_ALIGN_LEFT , 30 , &box , FNT_colors[ 7 ] );

		lines = box.height / font->height;
		if ( lines < 1 ) {
			lines = 1;
		}

		if ( includedCount > 0 && totalScreenLines + lines > maxLines ) {
			break;
		}

		includedHeights[ includedCount ] = box.height;
		includedCount ++;
		totalScreenLines += lines;

		if ( totalScreenLines >= maxLines ) {
			break;
		}
	}

	textHeight = 0;
	for ( i = 0 ; i < includedCount ; i ++ ) {
		textHeight += includedHeights[ i ];
	}

	inputRowHeight = font->height + font->size / 2;
	panelHeight = textHeight + inputRowHeight;

	panelVisualBottom = _CON_DrawPanelBackground( font , panelHeight );

	// Draw history, oldest (top) to newest (bottom), clipped to the text area
	qglScissor( 0 , viddef.height - textHeight , viddef.width , textHeight );
	qglEnable( GL_SCISSOR_TEST );

	y = 0;
	for ( i = includedCount - 1 ; i >= 0 ; i -- ) {
		box.x = font->size * 2;
		box.y = y;
		box.width = textWidth;
		box.height = 0;
		FNT_WrappedPrint( font , _CON_ChatHistoryGet( hist , i ) , FNT_CMODE_QUAKE , FNT_ALIGN_LEFT , 30 , &box , FNT_colors[ 7 ] );
		y += box.height;
	}

	qglDisable( GL_SCISSOR_TEST );

	// Draw the input prompt and currently typed text
	box.x = font->size * 2;
	box.y = panelHeight - inputRowHeight + font->size / 4;
	box.width = viddef.width;
	box.height = 0;
	FNT_BoundedPrint( font , prompt , FNT_CMODE_NONE , FNT_ALIGN_LEFT , &box , FNT_colors[ 7 ] );

	box.x = font->size * 2 + box.width + 1;
	box.width = viddef.width - box.x - font->size * 2;
	box.height = 0;
	FNT_WrappedPrint( font , chat_buffer , FNT_CMODE_NONE , FNT_ALIGN_LEFT , 30 , &box , FNT_colors[ 7 ] );

	// A message on a channel that isn't currently open would otherwise be
	// completely invisible while chatting - show the latest line from each
	// other channel (if recent) just below the panel, so nothing is missed.
	y = panelVisualBottom + font->size / 4;
	for ( i = 0 ; i < CHAT_HISTORY_COUNT ; i ++ ) {
		chatHistory_t *	other;
		const char *	latest;

		if ( i == type ) {
			continue;
		}

		other = &chatHistory[ i ];
		if ( other->count == 0
				|| cls.realtime - other->lastAddTime > con_notifytime->value * 1000 ) {
			continue;
		}

		latest = _CON_ChatHistoryGet( other , 0 );

		box.x = font->size * 2;
		box.y = y;
		box.width = textWidth;
		box.height = 0;
		FNT_WrappedPrint( font , latest , FNT_CMODE_QUAKE , FNT_ALIGN_LEFT , 30 , &box , FNT_colors[ 7 ] );

		y += ( box.height > font->height ) ? box.height : font->height;
	}
}



/**************************************************************************/
/* CONSOLE ACCESS FUNCTIONS                                               */
/**************************************************************************/

/* Save the console contents out to a file. */
static void _CON_Dump( void )
{
	char	name[MAX_OSPATH];
	int	line , lastLine;
	FILE *	f;

	if ( Cmd_Argc( ) != 2 ) {
		Com_Printf ("usage: condump <filename>\n");
		return;
	}
	Com_sprintf( name , sizeof( name ) , "%s/%s.txt" , FS_Gamedir( ) , Cmd_Argv( 1 ) );

	Com_Printf( "Dumping console text to %s.\n" , name );
	FS_CreatePath( name );
	f = fopen( name, "w" );
	if (!f) {
		Com_Printf( "ERROR: couldn't open %s.\n" , name );
		return;
	}

	lastLine = _CON_FindLineStart( CON_console.curLine );
	line = ( CON_console.curLine + 1 + CON_MAX_LINES - CON_console.lines ) % CON_MAX_LINES;
	while ( line != lastLine ) {
		if ( CON_console.text[ line ][ 0 ] != 0 ) {
			if ( CON_console.lCount[ line ] == 1 ) {
				fprintf( f , "%s\n" , CON_console.text[ line ] );
			} else {
				char * buffer = Z_Malloc( CON_LINE_LENGTH * CON_console.lCount[ line ] );
				_CON_CopyLine( buffer , line );
				fprintf( f , "%s\n" , buffer );
				Z_Free( buffer );
			}
		}
		line = _CON_FindNextLine( line );
	}

	fclose( f );
}

/* Clears the console */
void CON_Clear( void )
{
	memset( &CON_console , 0 , sizeof( CON_console ) );
	CON_console.lines = 1;
	CON_console.lCount[ 0 ] = 1;
	CON_console.curLine = 0;
	CON_console.offset = 0;
	CON_console.initialised = true;
}


/* Initialise the console. */
void CON_Initialise( )
{
	CON_Clear( );
	Com_Printf( "Console initialized.\n" );

	// Register CVars
	con_notifytime = Cvar_Get( "con_notifytime" , "6" , CVARDOC_INT | CVAR_ARCHIVE );
	con_ignorecolorcodes = Cvar_Get( "con_ignorecolorcodes", "0", CVARDOC_BOOL|CVAR_ARCHIVE );

	// 0 = off
	// 1 = red
	// 2 = green
	// 3 = blue
	// 4 = cyan
	// 5 = purple
	con_color = Cvar_Get( "con_color", "0", CVARDOC_INT|CVAR_ARCHIVE );

	cl_chatlines = Cvar_Get( "cl_chatlines" , "10" , CVARDOC_INT|CVAR_ARCHIVE );

	// Register commands
	Cmd_AddCommand( "toggleconsole", CON_ToggleConsole );
	Cmd_AddCommand( "togglechat", _CON_ToggleChat );
	Cmd_AddCommand( "messagemode", _CON_GeneralChatInput );
	Cmd_AddCommand( "messagemode2", _CON_TeamChatInput );
	Cmd_AddCommand( "messagemode3", _CON_IRCInput );
	Cmd_AddCommand( "clear" , CON_Clear );
	Cmd_AddCommand( "condump" , _CON_Dump );
}


/* Add text to the console. */
void CON_Print( const char * text )
{
	char curChar;

	if ( ! CON_console.initialised ) {
		return;
	}

	while ( ( curChar = *text ) != 0 ) {
		if ( curChar == '\r' ) {
			_CON_RestartLine( );
		} else if ( curChar == '\n' ) {
			_CON_NewLine( );
		} else {
			_CON_AppendChar( curChar );
		}
		text ++;
	}
}



/**************************************************************************/
/* CONSOLE DISPLAY FUNCTIONS                                              */
/**************************************************************************/

/*
 * Compute the height of a logical console line and update the console's
 * data structure accordingly.
 *
 * Parameters:
 *	font	the current console font
 *	line	the start of the logical line for which the computation will
 *		be performed
 */
static void _CON_ComputeLineHeight( FNT_font_t font , int line )
{
	if ( CON_console.text[ line ][ 0 ] == 0 ) {
		// Empty lines are a special case, as we don't need to draw them
		CON_console.heights[ line ] = font->size;
	} else {
		// "Print" the line outside of the screen
		struct FNT_window_s box;
		box.x = 0 , box.y = viddef.height;
		box.width = viddef.width - font->size * 5 , box.height = 0;

		if ( CON_console.lCount[ line ] == 1 ) {
			FNT_WrappedPrint( font , CON_console.text[ line ] , cmode ,
				FNT_ALIGN_LEFT , 0 , &box , FNT_colors[ 0 ] );
		} else {
			char * buffer = Z_Malloc( CON_console.lCount[ line ] * CON_LINE_LENGTH );
			_CON_CopyLine( buffer , line );
			FNT_WrappedPrint( font , buffer , cmode ,
				FNT_ALIGN_LEFT , 0 , &box , FNT_colors[ 0 ] );
			Z_Free( buffer );
		}

		if ( box.height == 0 ) {
			CON_console.heights[ line ] = font->size;
		} else {
			CON_console.heights[ line ] = box.height;
		}
	}
}


/*
 * Compute the height of all active lines in the console.
 *
 * Parameters:
 *	font	the console's current font
 *
 * Returns:
 *	the total height of the console
 */
static unsigned int _CON_ComputeHeight( FNT_font_t font )
{
	int			line , lastLine;
	unsigned int		total = 0;

	lastLine = _CON_FindLineStart( CON_console.curLine );
	line = ( CON_console.curLine + 1 + CON_MAX_LINES - CON_console.lines ) % CON_MAX_LINES;
	while ( 1 ) {
		if ( ! CON_console.heights[ line ] ) {
			_CON_ComputeLineHeight( font , line );
		}

		total += CON_console.heights[ line ];
		if ( line == lastLine ) {
			break;
		}
		line = _CON_FindNextLine( line );
	}

	return total;
}


/*
 * Check for font or resolution changes.
 *
 * Parameters:
 *	font	the console's current font
 *
 * Returns:
 *	true if the resolution or fonts have changed, false otherwise
 */
static qboolean _CON_CheckResize( FNT_font_t font )
{
	return ( viddef.width != CON_console.pWidth || font->size != CON_console.pSize
		|| strcmp( font->face->name , CON_console.pFace ) );
}


/*
 * Draw the console's text.
 *
 * Parameters:
 *	font	the current console font
 *	start	the vertical offset relative to the top of the console's
 *		contents at which drawing is to begin
 *	dHeight	the height of the display area
 */
static void _CON_DrawConsoleText(
		FNT_font_t		font ,
		int			start ,
		int			dHeight
	)
{
	struct FNT_window_s	box;
	int			line , lastLine , total;

	qglScissor( 0 , viddef.height - dHeight , viddef.width , dHeight );
	qglEnable( GL_SCISSOR_TEST );

	total = 0;
	lastLine = _CON_FindLineStart( CON_console.curLine );
	line = ( CON_console.curLine + 1 + CON_MAX_LINES - CON_console.lines ) % CON_MAX_LINES;

	do {
		if ( total + CON_console.heights[ line ] >= start && CON_console.text[ line ][ 0 ] ) {
			box.x = font->size*2;
			box.y = total - start;
			box.width = viddef.width - font->size * 5;
			box.height = 0;

			if ( CON_console.lCount[ line ] == 1 ) {
				FNT_WrappedPrint( font , CON_console.text[ line ] , cmode ,
					FNT_ALIGN_LEFT , 0 , &box , FNT_colors[ 2 ] );
			} else {
				char * buffer = Z_Malloc( CON_console.lCount[ line ] * CON_LINE_LENGTH );
				_CON_CopyLine( buffer , line );
				FNT_WrappedPrint( font , buffer , cmode ,
					FNT_ALIGN_LEFT , 0 , &box , FNT_colors[ 2 ] );
				Z_Free( buffer );
			}
		}

		total += CON_console.heights[ line ];
		if ( line == lastLine ) {
			break;
		}
		line = _CON_FindNextLine( line );
	} while ( total - start < dHeight );

	qglDisable( GL_SCISSOR_TEST );
}


/*
 * Draw the scroller on the right of the console.
 *
 * For now this is not interactive, it only exists to indicate where in the
 * console's contents the current view is located.
 *
 * This function uses Draw_Fill to generate the scroller's graphics. It uses
 * the following fill colours: 15 (white, outside of the box), 201 (dark
 * green, inside of the box) and 208 (bright green, selected area).
 *
 * Parameters:
 *	font_size	The size of the font (used as the vertical offset and
 *			the scroller's width)
 *	text_height	Total height of the console's text contents.
 *	display_height	Height of the viewing area.
 */
static void _CON_DrawScroller(
		int	font_size ,
		int	text_height ,
		int	display_height )
{ 
	// FIXME: lots of copy-pasted code here. Doesn't matter, we will 
	// eventually redo the whole console using the menu code.
	
	int hStart = viddef.width - 3 * font_size;
	int vStart = font_size / 2;
	int tHeight = display_height - font_size;
	int bHeight , bStart;
	
	Draw_AlphaStretchTilingPic (hStart, vStart, font_size, font_size, "menu/scroll_border_end", 1);
	Draw_AlphaStretchTilingPic (hStart, vStart+font_size, font_size, tHeight-vStart, "menu/scroll_border", 1);
	Draw_AlphaStretchTilingPic (hStart+font_size, vStart+tHeight+font_size, -font_size, -font_size, "menu/scroll_border_end", 1);
	
	if ( display_height >= text_height )
	{
		// Fill whole bar
		bStart = vStart;
		bHeight = tHeight;
	}
	else
	{
		bHeight = tHeight * display_height / text_height;
		if ( bHeight < font_size ) 
			bHeight = font_size;
		// "Top" offset is height - dHeight, "bottom" offset is 0
		// "Top" bar location is vStart, bottom location is vStart + tHeight - bHeight.
		bStart = vStart + ( tHeight - bHeight )
			* ( CON_console.displayOffset - text_height + display_height )
			/ ( display_height - text_height );
	}
	
	Draw_AlphaStretchTilingPic (hStart, bStart, font_size, font_size, "menu/scroll_cursor_end", 1);
	Draw_AlphaStretchTilingPic (hStart, bStart+font_size, font_size, bHeight-font_size, "menu/scroll_cursor", 1);
	Draw_AlphaStretchTilingPic (hStart+font_size, bStart+bHeight+font_size, -font_size, -font_size, "menu/scroll_cursor_end", 1);
}


/*
 * Draw the console's input line.
 *
 * Parameters:
 *	font	the current console font
 *	inputY	the height at which the input line is located
 */
static void _CON_DrawInputLine(
		FNT_font_t		font ,
		int			inputY )
{
	struct FNT_window_s	box;
	char			text[ MAXCMDLINE + 1 ];
	unsigned int		wToCursor;
	unsigned int		wAfterCursor;
	unsigned int		align;
	char			old;

	if ( cls.key_dest == key_menu || ( cls.key_dest != key_console && cls.state == ca_active ) )
		return;

	// Copy current line
	memcpy( text , key_lines[ edit_line ] , key_linelen );
	text[ key_linelen ] = 0;

	// Determine width of text before the cursor ...
	old = text[ key_linepos ];
	text[ key_linepos ] = 0;
	box.x = 0;
	box.y = viddef.height;
	box.width = box.height = 0;
	FNT_BoundedPrint( font , text , cmode , FNT_ALIGN_LEFT , &box , FNT_colors[ 2 ] );
	wToCursor = box.width;
	text[ key_linepos ] = old;

	// ... and after the cursor
	if ( key_linelen > key_linepos ) {
		box.width = box.height = 0;
		FNT_BoundedPrint( font , text + key_linepos , cmode , FNT_ALIGN_LEFT , &box , FNT_colors[ 2 ] );
		wAfterCursor = box.width;
	} else {
		wAfterCursor = 0;
	}

	box.x = font->size;
	box.y = inputY;
	box.height = 0;
	text[ key_linepos ] = 0;
	if ( wToCursor + font->size * 5 < viddef.width ) {
		// There is enough space for the start of the line
		align = FNT_ALIGN_LEFT;
	} else {
		// Not enough space, print with right alignment
		wToCursor = viddef.width * 0.9;
		align = FNT_ALIGN_RIGHT;
	}
	box.width = wToCursor;
	FNT_BoundedPrint( font , text , cmode , align , &box , FNT_colors[ 2 ] );

	// Draw cursor
	if ( ( (int)( cls.realtime >> 8 ) & 1) != 0 ) {
		Draw_Fill (box.x + box.width + 1 , box.y + 1 , font->size - 2 , font->size - 2 , RGBA(0,1,0,1));
	}

	// Draw whatever is after the cursor
	if ( wAfterCursor ) {
		text[ key_linepos ] = old;
		box.x += box.width + font->size;
		box.width = viddef.width - ( box.width + font->size * 5 );
		box.height = 0;
		FNT_BoundedPrint( font , text + key_linepos , cmode , FNT_ALIGN_LEFT , &box , FNT_colors[ 2 ] );
	}
}


/*
 * Draw the line at the bottom of the console.
 *
 * This line includes the version string and the current download status.
 *
 * Parameters:
 *	y	vertical offset of the bottom of the console
 *
 * Returns:
 *	vertical offset above the line at the bottom of the console
 *
 */
static int _CON_DrawConsoleBottom( int y )
{
	FNT_font_t	font;
	char		buffer[ MAX_STRING_CHARS ];
	int		kb , sz;

	font = FNT_AutoGet( CL_gameFont );
	y -= font->size;
	
	// Draw version string
	sz = sizeof( VERSION );
	FNT_RawPrint( font , VERSION , sz , false ,
		viddef.width - font->size * 5 * sz / 2 , y , FNT_colors[ 7 ] );

	// Draw download status if needed
	if ( ! ( cls.download && ( kb = (int)ftell( cls.download ) / 1024 ) ) ) {
		return y;
	}

	Com_sprintf( buffer , MAX_STRING_CHARS , "Downloading %s [%s] ... %dKB" ,
		cls.downloadname , ( cls.downloadhttp ? "HTTP" : "UDP" ), kb );
	FNT_RawPrint( font , buffer , strlen( buffer ) , false ,
		font->size * 3 , y , FNT_colors[ 3 ] );

	return y;
}


/* Draw the console. */
void CON_DrawConsole( float relSize )
{
	FNT_font_t		font;
	int			dHeight;
	int			height;
	int			start;
	int			fontSize;

	// Don't draw if there's no video
	if ( viddef.width < 1 ) {
		return;
	}

	// Check for changes and act accordingly
	font = FNT_AutoGet( CL_consoleFont );
	if ( _CON_CheckResize( font ) ) {
		CON_console.pWidth = viddef.width;
		CON_console.pSize = font->size;
		strcpy( CON_console.pFace , font->face->name );
		memset( CON_console.heights , 0 , sizeof( CON_console.heights ) );
	}
	fontSize = font->size;

	// Compute display height and draw background
	dHeight = viddef.height * relSize;
	
	// FIXME: lots of copy-pasted code here. Doesn't matter, we will 
	// eventually redo the whole console using the menu code. 
	
	{
		int _tile_w, _tile_h;
		float tile_w, tile_h;
		//qboolean fromMenu = (cls.state == ca_disconnected && cls.key_dest != key_game);
		qboolean fromMenu = (cls.state == ca_disconnected || cls.state == ca_connecting);
		int color = con_color->integer;

//Com_Printf("%d %d\n", cls.state, cls.key_dest);

		// assume all tiles are the same size
		Draw_GetPicSize (&_tile_w, &_tile_h, "menu/m_topcorner" );
	
		tile_w = (float)_tile_w/64.0*(float)font->size*4.0;
		tile_h = (float)_tile_h/64.0*(float)font->size*4.0;

		if (color < 0 || color > 5) {
			color = 0;
		}

		if (color > 0 || fromMenu) 
		{
			Draw_Fill_CutCorners(font->size / 3, 0, viddef.width - font->size / 1.5, dHeight + font->size * 1.5, 
				fromMenu ? consolecolors_menu[color] : consolecolors[color], font->size, RCF_BOTTOMLEFT|RCF_BOTTOMRIGHT);
		}

		Draw_AlphaStretchTilingPic( -tile_w/4, dHeight-tile_h/2, tile_w, tile_h, "menu/m_bottomcorner", 1 );
		Draw_AlphaStretchTilingPic( viddef.width+tile_w/4, dHeight-tile_h/2, -tile_w, tile_h, "menu/m_bottomcorner", 1 );
		
		Draw_AlphaStretchTilingPic( tile_w*0.75, dHeight-tile_h/2, viddef.width-tile_w*1.5, tile_h, "menu/m_bottom", 1 );
		
		Draw_AlphaStretchTilingPic( -tile_w/4, 0, tile_w, dHeight-tile_h/2, "menu/m_side", 1 );
		
		Draw_AlphaStretchTilingPic( viddef.width+tile_w/4, 0, -tile_w, dHeight-tile_h/2, "menu/m_side", 1 );
		Draw_AlphaStretchTilingPic( tile_w*0.75, 0, viddef.width-tile_w*1.5, dHeight-tile_h/2, "menu/m_background", 1 );		
	}

	// Draw version string and download status
	dHeight = _CON_DrawConsoleBottom( dHeight ) - fontSize;

	// Compute heights
	height = _CON_ComputeHeight( font );
	if ( height < dHeight ) {
		CON_console.displayOffset = 0;
	} else if ( CON_console.displayOffset > height - dHeight ) {
		CON_console.displayOffset = height - dHeight;
	}
	start = height - dHeight - CON_console.displayOffset;

	// Draw console contents & input line
	_CON_DrawConsoleText( font , start , dHeight );
	_CON_DrawScroller( font->size , height , dHeight );
	_CON_DrawInputLine( font , dHeight );
}
