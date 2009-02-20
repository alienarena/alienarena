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

#include "client.h"

console_t	con;

cvar_t		*con_notifytime;


#define		MAXCMDLINE	256

vec4_t	color_table[8] =
{
	{0.0, 0.0, 0.0, 1.0},
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{0.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
};

void DrawString (int x, int y, char *s)
{
	int charscale;
	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;
	while (*s)
	{
		Draw_ScaledChar (x, y, *s, charscale);
		x+=charscale;
		s++;
	}
}
/*
=============
Draw_ColorString
=============
*/
void Draw_ColorString ( int x, int y, char *str, float scale)
{
	int		num;
	vec4_t	scolor;
	int		charscale;

	charscale = (float)(viddef.height)*8*scale/600;
	if(charscale < 8)
		charscale = 8;

	scolor[0] = 0;
	scolor[1] = 1;
	scolor[2] = 0;
	scolor[3] = 1;

	while (*str) {
		if ( Q_IsColorString( str ) ) {
			VectorCopy ( color_table[ColorIndex(str[1])], scolor );
			str += 2;
			continue;
		}
		
		Draw_ScaledColorChar (x, y, *str, scolor, charscale); //this is only ever used for names.
		
		num = *str++;
		num &= 255;
			
		if ( (num&127) == 32 ) { //spaces reset colors
			scolor[0] = 0;
			scolor[1] = 1;
			scolor[2] = 0;
			scolor[3] = 1;
		}

		x += charscale;
	}
}


void DrawAltString (int x, int y, char *s)
{
	float	charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;

	while (*s)
	{
		Draw_ScaledChar (x, y, *s ^ 0x80, charscale);
		x+=charscale;
		s++;
	}
}


void Key_ClearTyping (void)
{
	key_lines[edit_line][1] = 0;	// clear any typing
	key_linelen = key_linepos = 1;
}

/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void)
{
	SCR_EndLoadingPlaque ();	// get rid of loading plaque

	Key_ClearTyping ();
	Con_ClearNotify ();

	if (cls.key_dest == key_console)
	{
		M_ForceMenuOff ();
		Cvar_Set ("paused", "0");
	}
	else
	{
		M_ForceMenuOff ();
		cls.key_dest = key_console;	

		if (Cvar_VariableValue ("maxclients") == 1 
			&& Com_ServerState ())
			Cvar_Set ("paused", "1");
	}
}

/*
================
Con_ToggleChat_f
================
*/
void Con_ToggleChat_f (void)
{
	Key_ClearTyping ();

	if (cls.key_dest == key_console)
	{
		if (cls.state == ca_active)
		{
			M_ForceMenuOff ();
			cls.key_dest = key_game;
		}
	}
	else
		cls.key_dest = key_console;
	
	Con_ClearNotify ();
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void)
{
	memset (con.text, ' ', CON_TEXTSIZE);
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int		l, x;
	char	*line;
	FILE	*f;
	char	buffer[1024];
	char	name[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Com_sprintf (name, sizeof(name), "%s/%s.txt", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("Dumped console text to %s.\n", name);
	FS_CreatePath (name);
	f = fopen (name, "w");
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if (line[x] != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		strncpy (buffer, line, con.linewidth);
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		for (x=0; buffer[x]; x++)
			buffer[x] &= 0x7f;

		fprintf (f, "%s\n", buffer);
	}

	fclose (f);
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify (void)
{
	int		i;
	
	for (i=0 ; i<NUM_CON_TIMES ; i++)
		con.times[i] = 0;
}

						
/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void)
{
	chat_team = false;
	cls.key_dest = key_message;
	Cbuf_AddText("chatbubble\n"); 
	Cbuf_Execute ();
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void)
{
	chat_team = true;
	cls.key_dest = key_message;
	Cbuf_AddText("chatbubble\n"); 
	Cbuf_Execute ();
}

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars, charscale;
	char	tbuf[CON_TEXTSIZE];

	// find out the character's scale and use it to compute the width
	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;
	width = (viddef.width / charscale) - 2;

	if (width == con.linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = 38;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		memset (con.text, ' ', CON_TEXTSIZE);
	}
	else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		memcpy (tbuf, con.text, CON_TEXTSIZE);
		memset (con.text, ' ', CON_TEXTSIZE);

		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init (void)
{
	con.linewidth = -1;

	Con_CheckResize ();
	
	Com_Printf ("Console initialized.\n");

//
// register our commands
//
	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("togglechat", Con_ToggleChat_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_Dump_f);
	con.initialized = true;
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (void)
{
	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	memset (&con.text[(con.current%con.totallines)*con.linewidth]
	, ' ', con.linewidth);
}

/*
================
Con_Print

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void Con_Print (char *txt)
{
	int		y;
	int		c, l;
	static int	cr;
	int		mask;
	qboolean mask_skip;
	
	if (!con.initialized)
		return;

	if (txt[0] == 1 || txt[0] == 2)
	{
		mask = 128;		// go to colored text
		txt++;
	}
	else
		mask = 0;

	mask_skip = false;

	while ( (c = *txt) )
	{
	// count word length
		for (l=0 ; l< con.linewidth ; l++)
			if ( txt[l] <= ' ')
				break;

	// word wrap
		if (l != con.linewidth && (con.x + l > con.linewidth) )
			con.x = 0;
	
		txt++;

		if (cr)
		{
			con.current--;
			cr = false;
		}

		
		if (!con.x)
		{
			Con_Linefeed ();
		// mark time for transparent overlay
			if (con.current >= 0)
				con.times[con.current % NUM_CON_TIMES] = cls.realtime;
		}

		switch (c)
		{
		case '\n':
			con.x = 0;
			break;

		case '\r':
			con.x = 0;
			cr = 1;
			break;

		default:	// display character and advance
			y = con.current % con.totallines;
			if(c == '^' || mask_skip) {
				con.text[y*con.linewidth+con.x] = c; //don't use masks in strings with color esc
				mask_skip = true;
			}
			else
				con.text[y*con.linewidth+con.x] = c | mask | con.ormask; 
			con.x++;
			if (con.x >= con.linewidth)
				con.x = 0;
			break;
		}
		
	}
}


/*
==============
Con_CenteredPrint
==============
*/
void Con_CenteredPrint (char *text)
{
	int		l;
	char	buffer[1024];

	l = strlen(text);
	l = (con.linewidth-l)/2;
	if (l < 0)
		l = 0;
	memset (buffer, ' ', l);
	strcpy (buffer+l, text);
	strcat (buffer, "\n");
	Con_Print (buffer);
}

/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

The input line scrolls horizontally if typing goes beyond the right edge
================
*/
void Con_DrawInput (void)
{
	int		i;
	char		text[2048];
	char		*output;
	int		charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;

	if (cls.key_dest == key_menu)
		return;
	if (cls.key_dest != key_console && cls.state == ca_active)
		return;		// don't draw anything (always draw if not active)

// copy the text into the temporary buffer and add the cursor frame
	memcpy (text, key_lines[edit_line], key_linepos);
	text[key_linepos] = 10+((int)(cls.realtime>>8)&1);
	if ( key_linelen > key_linepos )
		memcpy (text + key_linepos + 1, key_lines[edit_line] + key_linepos, key_linelen - key_linepos);

// fill out remainder with spaces
	for (i = key_linelen + 1 ; i < con.linewidth ; i++)
		text[i] = ' ';
	text[i] = 0;

// prestep if horizontally scrolling
	output = text;
	if (key_linepos >= con.linewidth)
		output += 1 + key_linepos - con.linewidth;


// draw it
	Draw_ColorString ( charscale, (int)(con.vislines - 2.75*charscale), output, 1);
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	char	*text;
	int		i;
	int		time;
	char	*s;
	int		skip;
	int num;
	int spacer;
	vec4_t	scolor;
	int		charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;
	
	scolor[0] = 1;
	scolor[1] = 1;
	scolor[2] = 1;
	scolor[3] = 1;

	v = 0;
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
	{
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;
		
	    x = 0;
		spacer = 0;
		while (*text && x < con.linewidth-spacer) {
			if ( Q_IsColorString( text ) ) {
				VectorCopy ( color_table[ColorIndex(text[1])], scolor );
				text += 2;
				spacer +=2;
				continue;
			}
			
			Draw_ScaledColorChar ((x+1)*charscale, v, *text, scolor, charscale);
				
			num = *text++;
			num &= 255;	

			if ( (num&127) == 32 ) { //spaces reset colors
				scolor[0] = 1;
				scolor[1] = 1;
				scolor[2] = 1;
				scolor[3] = 1;
			}	
			
			x++;
			
		}
	
		v += charscale;
	}


	if (cls.key_dest == key_message)
	{
		
		if (chat_team)
		{
			DrawString (charscale, v, "say_team:");
			skip = 11;
		}
		else
		{
			DrawString (charscale, v, "say:");
			skip = 5;
		}

		s = chat_buffer;
		if (chat_bufferlen > (viddef.width/charscale)-(skip+1))
			s += chat_bufferlen - ((viddef.width/charscale)-(skip+1));
		
		x = 0;
		while (*s) {
			if ( Q_IsColorString( s ) ) {
				VectorCopy ( color_table[ColorIndex(s[1])], scolor );
				s += 2;
				continue;
			}
			
			Draw_ScaledColorChar ( (x+skip)*charscale, v, *s, scolor, charscale);
			
			num = *s++;
			num &= 255;	

			if ( (num&127) == 32 ) { //spaces reset colors
				scolor[0] = 1;
				scolor[1] = 1;
				scolor[2] = 1;
				scolor[3] = 1;
			}	
								
			x++;
			
		}
		Draw_ScaledChar ( (x+skip)*charscale, v, 10+((cls.realtime>>charscale)&1), charscale);
		v += charscale;
	}
	
	if (v)
	{
		SCR_AddDirtyPoint (0,0);
		SCR_AddDirtyPoint (viddef.width-1, v);
	}
}

/*
================
Con_DrawConsole

Draws the console with the solid background
================
*/
void Con_DrawConsole (float frac)
{
	int				i, x, y;
	int				rows;
	char			*text, dl[MAX_STRING_CHARS], output[1024];
	int				row;
	int				lines;
	char			version[64];
	int kb;
	vec4_t	scolor;
	int		charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;

	scolor[0] = 1;
	scolor[1] = 1;
	scolor[2] = 1;
	scolor[3] = 1;

	lines = viddef.height * frac;
	if (lines <= 0)
		return;

	if (lines > viddef.height)
		lines = viddef.height;

// draw the background
	Draw_StretchPic (0, lines-viddef.height, viddef.width, viddef.height, "conback");
	SCR_AddDirtyPoint (0,0);
	SCR_AddDirtyPoint (viddef.width-1,lines-1);
 
	Com_sprintf (version, sizeof(version), "v%4.2f", VERSION);
	DrawString(viddef.width-charscale*(strlen(version)+1), lines-charscale-1, version);

// draw the text
	con.vislines = lines;
	
	rows = (lines-22)/charscale;		// rows of text to draw

	y = lines - 3.75*charscale;


// draw from the bottom up
	if (con.display != con.current)
	{
	// draw arrows to show the buffer is backscrolled
		for (x=0 ; x<con.linewidth ; x+=charscale*2)
			Draw_ScaledChar ( (x+1)*charscale, y, '^', charscale);
	
		y -= charscale;
		rows--;
	}
	
	row = con.display;
	for (i=0 ; i<rows ; i++, y-=charscale, row--)
	{
		if (row < 0)
			break;
		if (con.current - row >= con.totallines)
			break;		// past scrollback wrap point
			
		text = con.text + (row % con.totallines)*con.linewidth;
	
		Com_sprintf (output, sizeof(output), "");
		for (x=0 ; x<con.linewidth ; x++)
			Com_sprintf (output, sizeof(output), "%s%c", output, text[x]);

		Draw_ColorString ( 4, y, output, 1);
	}

//ZOID

	if(cls.download && (kb = (int)ftell(cls.download) / 1024)){  // draw progress

		Com_sprintf(dl, sizeof(dl), "%s [%s] %dKB ", cls.downloadname,
				(cls.downloadhttp ? "HTTP" : "UDP"), kb);
		
		for(i = 0; i < strlen(dl); i++)
			Draw_ScaledChar(i * charscale, con.vislines - charscale, dl[i], charscale);
	}
//ZOID

// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();
}


