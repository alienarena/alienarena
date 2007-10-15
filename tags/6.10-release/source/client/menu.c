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
#include <ctype.h>
#ifdef _WINDOWS
#include <winsock.h>
#endif

#ifdef __unix__
#include <sys/time.h>
#endif

#ifdef _WIN32
#include <io.h>
#endif

#include "client.h"
#include "../client/qmenu.h"

static int	m_main_cursor;

extern void RS_LoadScript(char *script);
extern void RS_LoadSpecialScripts(void);
extern cvar_t *scriptsloaded;
extern char map_music[128];
extern cvar_t *background_music;
extern cvar_t *dedicated;

#define NUM_CURSOR_FRAMES 15

//menu mouse
#define MOUSEBUTTON1 0
#define MOUSEBUTTON2 1

static char *menu_in_sound		= "misc/menu1.wav";
static char *menu_move_sound	= "misc/menu2.wav";
static char *menu_out_sound		= "misc/menu3.wav";
static char *menu_background		= "misc/menuback.wav";
int svridx;
int playeridx;
int hover_time;

void M_Menu_Main_f (void);
	void M_Menu_Game_f (void);
		void M_Menu_PlayerConfig_f (void);
		void M_Menu_Credits_f( void );
	void M_Menu_JoinServer_f (void);
			void M_Menu_AddressBook_f( void );
	void M_Menu_StartServer_f (void);
			void M_Menu_DMOptions_f (void);
	void M_Menu_Video_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
	void M_Menu_Quit_f (void);

	void M_Menu_Credits( void );

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound

void	(*m_drawfunc) (void);
const char *(*m_keyfunc) (int key);

//=============================================================================
/* Support Routines */

#define	MAX_MENU_DEPTH	8


typedef struct
{
	void	(*draw) (void);
	const char *(*key) (int k);
} menulayer_t;

menulayer_t	m_layers[MAX_MENU_DEPTH];
int		m_menudepth;

static void M_Banner( char *name )
{
	int w, h;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	Draw_GetPicSize (&w, &h, name );

	w*=scale;
	h*=scale;

	Draw_StretchPic( viddef.width / 2 - (w / 2), viddef.height / 2 - 250*scale, w, h, name );

}
static void M_MapPic( char *name )
{
	int w, h;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	w = h = 128*scale;
	Draw_StretchPic (viddef.width / 2 - w - 4*scale, viddef.height / 2 + 112*scale, w, h, name);
}
static void M_CrosshairPic( char *name )
{
	int w, h;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	w = h = 64*scale;
	Draw_StretchPic (viddef.width / 2 - w/2 - 110*scale, viddef.height / 2 + 100*scale, w, h, name);
}
static void M_Background( char *name)
{
	Draw_StretchPic(0, 0, viddef.width, viddef.height, name);
}
static void M_ArrowPics()
{
	int w, h;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	Draw_GetPicSize (&w, &h, "uparrow" );
	Draw_GetPicSize (&w, &h, "dnarrow" );

	//for the server list
	Draw_StretchPic (viddef.width / 2 - w/2 - 176*scale, viddef.height / 2 - 16*scale, 32*scale, 32*scale, "uparrow");
	Draw_StretchPic (viddef.width / 2 - w/2 - 176*scale, viddef.height / 2 + 20*scale, 32*scale, 32*scale, "dnarrow");

	//for the player list
	Draw_StretchPic (viddef.width / 2 - w/2 - 46*scale, viddef.height / 2 + 134*scale, 32*scale, 32*scale, "uparrow");
	Draw_StretchPic (viddef.width / 2 - w/2 - 46*scale, viddef.height / 2 + 170*scale, 32*scale, 32*scale, "dnarrow");
}

// Knightmare- added Psychospaz's mouse support
void refreshCursorButtons(void)
{
	cursor.buttonused[MOUSEBUTTON2] = true;
	cursor.buttonclicks[MOUSEBUTTON2] = 0;
	cursor.buttonused[MOUSEBUTTON1] = true;
	cursor.buttonclicks[MOUSEBUTTON1] = 0;
}

void M_PushMenu ( void (*draw) (void), const char *(*key) (int k) )
{
	int		i;

	if (Cvar_VariableValue ("maxclients") == 1
		&& Com_ServerState ())
		Cvar_Set ("paused", "1");

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	for (i=0 ; i<m_menudepth ; i++)
		if (m_layers[i].draw == draw &&
			m_layers[i].key == key)
		{
			m_menudepth = i;
		}

	if (i == m_menudepth)
	{
		if (m_menudepth >= MAX_MENU_DEPTH)
			Com_Error (ERR_FATAL, "M_PushMenu: MAX_MENU_DEPTH");
		m_layers[m_menudepth].draw = m_drawfunc;
		m_layers[m_menudepth].key = m_keyfunc;
		m_menudepth++;
	}

	m_drawfunc = draw;
	m_keyfunc = key;

	m_entersound = true;

	// Knightmare- added Psychospaz's mouse support
	refreshCursorLink();
	refreshCursorButtons();

	cls.key_dest = key_menu;
}

void M_ForceMenuOff (void)
{

	// Knightmare- added Psychospaz's mouse support
	refreshCursorLink();

	m_drawfunc = 0;
	m_keyfunc = 0;
	cls.key_dest = key_game;
	m_menudepth = 0;
	Key_ClearStates ();
	Cvar_Set ("paused", "0");

	//-JD kill the music when leaving the menu of course
	S_StopAllSounds();
	background_music = Cvar_Get ("background_music", "1", CVAR_ARCHIVE);
	if(background_music->value)
		S_StartMusic(map_music);
}

void M_PopMenu (void)
{
	S_StartLocalSound( menu_out_sound );
	if (m_menudepth < 1)
		Com_Error (ERR_FATAL, "M_PopMenu: depth < 1");
	m_menudepth--;

	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;

	// Knightmare- added Psychospaz's mouse support
	refreshCursorLink();
	refreshCursorButtons();

	if (!m_menudepth)
		M_ForceMenuOff ();
}


const char *Default_MenuKey( menuframework_s *m, int key )
{
	const char *sound = NULL;
	menucommon_s *item;

	if ( m )
	{
		if ( ( item = Menu_ItemAtCursor( m ) ) != 0 )
		{
			if ( item->type == MTYPE_FIELD )
			{
				if ( Field_Key( ( menufield_s * ) item, key ) )
					return NULL;
			}
		}
	}

	switch ( key )
	{
	case K_ESCAPE:
		M_PopMenu();
		return menu_out_sound;
	case K_KP_UPARROW:
	case K_UPARROW:
		if ( m )
		{
			m->cursor--;

			// Knightmare- added Psychospaz's mouse support
			refreshCursorLink();

			Menu_AdjustCursor( m, -1 );
			sound = menu_move_sound;
		}
		break;
	case K_TAB:
		if ( m )
		{
			m->cursor++;

			// Knightmare- added Psychospaz's mouse support
			refreshCursorLink();

			Menu_AdjustCursor( m, 1 );
			sound = menu_move_sound;
		}
		break;
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if ( m )
		{
			m->cursor++;
			Menu_AdjustCursor( m, 1 );
			sound = menu_move_sound;
		}
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		if ( m )
		{
			Menu_SlideItem( m, -1 );
			sound = menu_move_sound;
		}
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		if ( m )
		{
			Menu_SlideItem( m, 1 );
			sound = menu_move_sound;
		}
		break;

/*	case K_MOUSE1:
	case K_MOUSE2:
	case K_MOUSE3: */
	case K_JOY1:
	case K_JOY2:
	case K_JOY3:
	case K_JOY4:
	case K_AUX1:
	case K_AUX2:
	case K_AUX3:
	case K_AUX4:
	case K_AUX5:
	case K_AUX6:
	case K_AUX7:
	case K_AUX8:
	case K_AUX9:
	case K_AUX10:
	case K_AUX11:
	case K_AUX12:
	case K_AUX13:
	case K_AUX14:
	case K_AUX15:
	case K_AUX16:
	case K_AUX17:
	case K_AUX18:
	case K_AUX19:
	case K_AUX20:
	case K_AUX21:
	case K_AUX22:
	case K_AUX23:
	case K_AUX24:
	case K_AUX25:
	case K_AUX26:
	case K_AUX27:
	case K_AUX28:
	case K_AUX29:
	case K_AUX30:
	case K_AUX31:
	case K_AUX32:

	case K_KP_ENTER:
	case K_ENTER:
		if ( m )
			Menu_SelectItem( m );
		sound = menu_move_sound;
		break;
	}

	return sound;
}

//=============================================================================

/*
================
M_DrawCharacter

Draws one solid graphics character
cx and cy are in 320*240 coordinates, and will be centered on
higher res screens.
================
*/
void M_DrawCharacter (int cx, int cy, int num)
{
	int		charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;

	Draw_ScaledChar ( cx + viddef.width/3 - 3*charscale, cy + viddef.height/3 - 3*charscale, num, charscale);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str)+128);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, *str);
		str++;
		cx += 8;
	}
}

void M_DrawPic (int x, int y, char *pic)
{
	Draw_Pic (x + ((viddef.width - 320)>>1), y + ((viddef.height - 240)>>1), pic);
}

void M_DrawTextBox (int x, int y, int width, int lines)
{
	int		cx, cy;
	int		n;
	int		charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;

	// draw left side
	cx = x;
	cy = y;
	M_DrawCharacter (cx, cy, 1);
	for (n = 0; n < lines; n++)
	{
		cy += charscale;
		M_DrawCharacter (cx, cy, 4);
	}
	M_DrawCharacter (cx, cy+charscale, 7);

	// draw middle
	cx += charscale;
	while (width > 0)
	{
		cy = y;
		M_DrawCharacter (cx, cy, 2);
		for (n = 0; n < lines; n++)
		{
			cy += charscale;
			M_DrawCharacter (cx, cy, 5);
		}
		M_DrawCharacter (cx, cy+charscale, 8);
		width -= 1;
		cx += charscale;
	}

	// draw right side
	cy = y;
	M_DrawCharacter (cx, cy, 3);
	for (n = 0; n < lines; n++)
	{
		cy += charscale;
		M_DrawCharacter (cx, cy, 6);
	}
	M_DrawCharacter (cx, cy+charscale, 9);
}


/*
=======================================================================

MAIN MENU

=======================================================================
*/
#define	MAIN_ITEMS	6

char *main_names[] =
{
	"m_main_game",
	"m_main_join",
	"m_main_host",
	"m_main_options",
	"m_main_video",
	"m_main_quit",
	0
};

void findMenuCoords (int *xoffset, int *ystart, int *totalheight, int *widest)
{
	int w, h, i;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	*totalheight = 0;
	*widest = -1;

	for ( i = 0; main_names[i] != 0; i++ )
	{
		Draw_GetPicSize( &w, &h, main_names[i] );

		if ( w*scale > *widest )
			*widest = w*scale;
		*totalheight += ( h*scale + 24*scale);
	}

	*ystart = ( viddef.height / 2 - 25*scale );
	*xoffset = ( viddef.width - *widest + 150*scale) / 2;

}

void M_Main_Draw (void)
{
	int i;
	int ystart;
	int	xoffset;
	int widest = -1;
	int totalheight = 0;
	char litname[80];
	float scale;
	float widscale;
	int w, h;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	widscale = (float)(viddef.width)/800;
	if(widscale<1)
		widscale = 1;

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);

	ystart = ( viddef.height / 2 - 25*scale );
	xoffset = ( viddef.width - widest - 25*scale) / 2;

	//draw the background pics first -

	M_Background( "m_main"); //draw black background first

	for ( i = 0; main_names[i] != 0; i++ )
	{
		if ( i != m_main_cursor ){
			Draw_GetPicSize( &w, &h, main_names[i] );
			Draw_StretchPic( xoffset + 100*scale + (20*i*scale), (int)(ystart + i * 32.5*scale + 13*scale), w*scale, h*scale, main_names[i] );

		}
	}
	strcpy( litname, main_names[m_main_cursor] );
	strcat( litname, "_sel" );
	Draw_GetPicSize( &w, &h, litname );
	//yuk
	if(!strcmp(litname, "m_main_game_sel"))
		i = 0;
	else if(!strcmp(litname, "m_main_join_sel"))
		i = 1;
	else if(!strcmp(litname, "m_main_host_sel"))
		i = 2;
	else if(!strcmp(litname, "m_main_options_sel"))
		i = 3;
	else if(!strcmp(litname, "m_main_video_sel"))
		i = 4;
	else if(!strcmp(litname, "m_main_quit_sel"))
		i = 5;
	Draw_StretchPic( xoffset + 100*scale + (20*i*scale), (int)(ystart + m_main_cursor * 32.5*scale + 13*scale), w*scale, h*scale, litname );

	//draw web link
	Menu_DrawString( viddef.width / 2 - 140*widscale, 0 + 10*scale, "Copyright 2007 COR Entertainment LLC" );
	Menu_DrawString( viddef.width / 2 - 160*widscale, 0 + 30*scale, "AA2K7 Website @ http://red.planetarena.org" );

}

typedef struct
{
	int	min[2];
	int max[2];

	void (*OpenMenu)(void);
} mainmenuobject_t;

void addButton (mainmenuobject_t *thisObj, int index, int x, int y)
{
	float ratio;
	float scale;
	int w, h;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	Draw_GetPicSize( &w, &h, main_names[index]);

	if (w)
	{
		ratio = 32.0/(float)h;
		h = 32;
		w *= ratio;
	}

	thisObj->min[0] = x; thisObj->max[0] = x + (w*scale);
	thisObj->min[1] = y; thisObj->max[1] = y + (h*scale);

	switch (index)
	{
	case 0:
		thisObj->OpenMenu = M_Menu_Game_f;
	case 1:
		thisObj->OpenMenu = M_Menu_JoinServer_f;
	case 2:
		thisObj->OpenMenu = M_Menu_StartServer_f;
	case 3:
		thisObj->OpenMenu = M_Menu_Options_f;
	case 4:
		thisObj->OpenMenu = M_Menu_Video_f;
	case 5:
		thisObj->OpenMenu = M_Menu_Quit_f;
	}
}

void openMenuFromMain (void)
{
	switch (m_main_cursor)
	{
		case 0:
			M_Menu_Game_f ();
			break;

		case 1:
			M_Menu_JoinServer_f();
			break;

		case 2:
			M_Menu_StartServer_f();
			break;

		case 3:
			M_Menu_Options_f ();
			break;

		case 4:
			M_Menu_Video_f ();
			break;

		case 5:
			M_Menu_Quit_f ();
			break;
	}
}

int MainMenuMouseHover;
void CheckMainMenuMouse (void)
{
	int ystart;
	int	xoffset;
	int widest;
	int totalheight;
	int i, oldhover;
	char *sound = NULL;
	mainmenuobject_t buttons[MAIN_ITEMS];
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	oldhover = MainMenuMouseHover;
	MainMenuMouseHover = 0;

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);
	for ( i = 0; main_names[i] != 0; i++ )
		addButton (&buttons[i], i, xoffset, ystart + (i * 32*scale + 24*scale));

	//Exit with double click 2nd mouse button
	if (!cursor.buttonused[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2]==2)
	{
		M_PopMenu();
		sound = menu_out_sound;
		cursor.buttonused[MOUSEBUTTON2] = true;
		cursor.buttonclicks[MOUSEBUTTON2] = 0;
	}

	for (i=MAIN_ITEMS-1;i>=0;i--)
	{
		if (cursor.x>=buttons[i].min[0] && cursor.x<=buttons[i].max[0] &&
			cursor.y>=buttons[i].min[1] && cursor.y<=buttons[i].max[1])
		{
			if (cursor.mouseaction)
				m_main_cursor = i;

			MainMenuMouseHover = 1 + i;

			if (oldhover == MainMenuMouseHover && MainMenuMouseHover-1 == m_main_cursor &&
				!cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1]==1)
			{
				openMenuFromMain();
				sound = menu_move_sound;
				cursor.buttonused[MOUSEBUTTON1] = true;
				cursor.buttonclicks[MOUSEBUTTON1] = 0;
			}
			break;
		}
	}

	if (!MainMenuMouseHover)
	{
		cursor.buttonused[MOUSEBUTTON1] = false;
		cursor.buttonclicks[MOUSEBUTTON1] = 0;
		cursor.buttontime[MOUSEBUTTON1] = 0;
		hover_time = 0;
	}

	if (oldhover == MainMenuMouseHover && MainMenuMouseHover-1 == m_main_cursor &&
		!cursor.buttonused[MOUSEBUTTON1] && hover_time == 0) {
		sound = menu_move_sound;
		hover_time = 1;
	}

	cursor.mouseaction = false;

	if ( sound )
		S_StartLocalSound( sound );
}

const char *M_Main_Key (int key)
{
	const char *sound = menu_move_sound;

	switch (key)
	{
	case K_ESCAPE:
		M_PopMenu ();
		break;

	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		return sound;

	case K_KP_UPARROW:
	case K_UPARROW:
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		return sound;

	case K_KP_ENTER:
	case K_ENTER:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case 0:
			M_Menu_Game_f ();
			break;

		case 1:
			M_Menu_JoinServer_f();
			break;

		case 2:
			M_Menu_StartServer_f();
			break;

		case 3:
			M_Menu_Options_f ();
			break;

		case 4:
			M_Menu_Video_f ();
			break;

		case 5:
			M_Menu_Quit_f ();
			break;
		}
	}

	return NULL;
}


void M_Menu_Main_f (void)
{
	M_PushMenu (M_Main_Draw, M_Main_Key);
}

/*
=======================================================================

KEYS MENU

=======================================================================
*/
char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"+attack2",        "alt attack"},
{"weapnext", 		"next weapon"},
{"weapprev", 		"previous weapon"},
{"+forward", 		"walk forward"},
{"+back", 			"backpedal"},
{"+left", 			"turn left"},
{"+right", 			"turn right"},
{"+speed", 			"run"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+strafe", 		"sidestep"},
{"+lookup", 		"look up"},
{"+lookdown", 		"look down"},
{"centerview", 		"center view"},
{"+mlook", 			"mouse look"},
{"+klook", 			"keyboard look"},
{"+moveup",			"up / jump"},
{"+movedown",		"down / crouch"},

{"inven",			"inventory"},
{"invuse",			"use item"},
{"invdrop",			"drop item"},
{"invprev",			"prev item"},
{"invnext",			"next item"},

{"use Alien Disruptor",	"alien disruptor" },
{"use Pulse Rifle",		"chaingun" },
{"use Flame Thrower",	"flame thrower" },
{"use Rocket Launcher",	"rocket launcher" },
{"use Alien Smartgun",	"alien smartgun" },
{"use Disruptor",		"alien beamgun" },
{"use Alien Vaporizer", "alien vaporizer" },
{"use Violator", "the violator" },
{"score",				"show scores" },
{"flashlight",			"flashlight" },
{"use grapple",			"grapple hook"},

{ 0, 0 }
};

int				keys_cursor;
static int		bind_grab;

static menuframework_s	s_keys_menu;
static menuaction_s		s_keys_attack_action;
static menuaction_s		s_keys_attack2_action;
static menuaction_s		s_keys_change_weapon_action;
static menuaction_s		s_keys_walk_forward_action;
static menuaction_s		s_keys_backpedal_action;
static menuaction_s		s_keys_turn_left_action;
static menuaction_s		s_keys_turn_right_action;
static menuaction_s		s_keys_run_action;
static menuaction_s		s_keys_step_left_action;
static menuaction_s		s_keys_step_right_action;
static menuaction_s		s_keys_sidestep_action;
static menuaction_s		s_keys_look_up_action;
static menuaction_s		s_keys_look_down_action;
static menuaction_s		s_keys_center_view_action;
static menuaction_s		s_keys_mouse_look_action;
static menuaction_s		s_keys_keyboard_look_action;
static menuaction_s		s_keys_move_up_action;
static menuaction_s		s_keys_move_down_action;
static menuaction_s		s_keys_inventory_action;
static menuaction_s		s_keys_inv_use_action;
static menuaction_s		s_keys_inv_drop_action;
static menuaction_s		s_keys_inv_prev_action;
static menuaction_s		s_keys_inv_next_action;
static menuaction_s		s_keys_alien_disruptor_action;
static menuaction_s		s_keys_chain_pistol_action;
static menuaction_s		s_keys_flame_thrower_action;
static menuaction_s		s_keys_rocket_launcher_action;
static menuaction_s		s_keys_alien_smartgun_action;
static menuaction_s		s_keys_alien_beamgun_action;
static menuaction_s		s_keys_alien_vaporizer_action;
static menuaction_s		s_keys_show_scores_action;
static menuaction_s		s_keys_flashlight_action;
static menuaction_s		s_keys_grapple_action;
static menuaction_s		s_keys_violator_action;
static menuaction_s		s_keys_filler_action;

static void M_UnbindCommand (char *command)
{
	int		j;
	int		l;
	char	*b;

	l = strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}

static void M_FindKeysForCommand (char *command, int *twokeys)
{
	int		count;
	int		j;
	int		l;
	char	*b;

	twokeys[0] = twokeys[1] = -1;
	l = strlen(command);
	count = 0;

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

static void DrawKeyBindingFunc( void *self )
{
	int keys[2];
	menuaction_s *a = ( menuaction_s * ) self;

	M_FindKeysForCommand( bindnames[a->generic.localdata[0]][0], keys);

	if (keys[0] == -1)
	{
		Menu_DrawString( a->generic.x + a->generic.parent->x + 16, a->generic.y + a->generic.parent->y, "???" );
	}
	else
	{
		int x;
		const char *name;

		name = Key_KeynumToString (keys[0]);

		Menu_DrawString( a->generic.x + a->generic.parent->x + 16, a->generic.y + a->generic.parent->y, name );

		x = strlen(name) * 8;

		if (keys[1] != -1)
		{
			Menu_DrawString( a->generic.x + a->generic.parent->x + 24 + x, a->generic.y + a->generic.parent->y, "or" );
			Menu_DrawString( a->generic.x + a->generic.parent->x + 48 + x, a->generic.y + a->generic.parent->y, Key_KeynumToString (keys[1]) );
		}
	}
}

static void KeyBindingFunc( void *self )
{
	menuaction_s *a = ( menuaction_s * ) self;
	int keys[2];

	M_FindKeysForCommand( bindnames[a->generic.localdata[0]][0], keys );

	if (keys[1] != -1)
		M_UnbindCommand( bindnames[a->generic.localdata[0]][0]);

	bind_grab = true;

	Menu_SetStatusBar( &s_keys_menu, "press a key or button for this action" );
}

static void Keys_MenuInit( void )
{
	int y;
	int i = 0;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	y = 80*scale;

	s_keys_menu.x = viddef.width * 0.50;

	s_keys_attack_action.generic.type	= MTYPE_ACTION;
	s_keys_attack_action.generic.x		= 0;
	s_keys_attack_action.generic.y		= y;
	s_keys_attack_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_attack_action.generic.localdata[0] = i;
	s_keys_attack_action.generic.name	= bindnames[s_keys_attack_action.generic.localdata[0]][1];

	s_keys_attack2_action.generic.type	= MTYPE_ACTION;
	s_keys_attack2_action.generic.x		= 0;
	s_keys_attack2_action.generic.y		= y += 9*scale;
	s_keys_attack2_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_attack2_action.generic.localdata[0] = ++i;
	s_keys_attack2_action.generic.name	= bindnames[s_keys_attack2_action.generic.localdata[0]][1];

	s_keys_change_weapon_action.generic.type	= MTYPE_ACTION;
	s_keys_change_weapon_action.generic.x		= 0;
	s_keys_change_weapon_action.generic.y		= y += 9*scale;
	s_keys_change_weapon_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_change_weapon_action.generic.localdata[0] = ++i;
	s_keys_change_weapon_action.generic.name	= bindnames[s_keys_change_weapon_action.generic.localdata[0]][1];

	s_keys_walk_forward_action.generic.type	= MTYPE_ACTION;
	s_keys_walk_forward_action.generic.x		= 0;
	s_keys_walk_forward_action.generic.y		= y += 9*scale;
	s_keys_walk_forward_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_walk_forward_action.generic.localdata[0] = ++i;
	s_keys_walk_forward_action.generic.name	= bindnames[s_keys_walk_forward_action.generic.localdata[0]][1];

	s_keys_backpedal_action.generic.type	= MTYPE_ACTION;
	s_keys_backpedal_action.generic.x		= 0;
	s_keys_backpedal_action.generic.y		= y += 9*scale;
	s_keys_backpedal_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_backpedal_action.generic.localdata[0] = ++i;
	s_keys_backpedal_action.generic.name	= bindnames[s_keys_backpedal_action.generic.localdata[0]][1];

	s_keys_turn_left_action.generic.type	= MTYPE_ACTION;
	s_keys_turn_left_action.generic.x		= 0;
	s_keys_turn_left_action.generic.y		= y += 9*scale;
	s_keys_turn_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_turn_left_action.generic.localdata[0] = ++i;
	s_keys_turn_left_action.generic.name	= bindnames[s_keys_turn_left_action.generic.localdata[0]][1];

	s_keys_turn_right_action.generic.type	= MTYPE_ACTION;
	s_keys_turn_right_action.generic.x		= 0;
	s_keys_turn_right_action.generic.y		= y += 9*scale;
	s_keys_turn_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_turn_right_action.generic.localdata[0] = ++i;
	s_keys_turn_right_action.generic.name	= bindnames[s_keys_turn_right_action.generic.localdata[0]][1];

	s_keys_run_action.generic.type	= MTYPE_ACTION;
	s_keys_run_action.generic.x		= 0;
	s_keys_run_action.generic.y		= y += 9*scale;
	s_keys_run_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_run_action.generic.localdata[0] = ++i;
	s_keys_run_action.generic.name	= bindnames[s_keys_run_action.generic.localdata[0]][1];

	s_keys_step_left_action.generic.type	= MTYPE_ACTION;
	s_keys_step_left_action.generic.x		= 0;
	s_keys_step_left_action.generic.y		= y += 9*scale;
	s_keys_step_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_left_action.generic.localdata[0] = ++i;
	s_keys_step_left_action.generic.name	= bindnames[s_keys_step_left_action.generic.localdata[0]][1];

	s_keys_step_right_action.generic.type	= MTYPE_ACTION;
	s_keys_step_right_action.generic.x		= 0;
	s_keys_step_right_action.generic.y		= y += 9*scale;
	s_keys_step_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_right_action.generic.localdata[0] = ++i;
	s_keys_step_right_action.generic.name	= bindnames[s_keys_step_right_action.generic.localdata[0]][1];

	s_keys_sidestep_action.generic.type	= MTYPE_ACTION;
	s_keys_sidestep_action.generic.x		= 0;
	s_keys_sidestep_action.generic.y		= y += 9*scale;
	s_keys_sidestep_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_sidestep_action.generic.localdata[0] = ++i;
	s_keys_sidestep_action.generic.name	= bindnames[s_keys_sidestep_action.generic.localdata[0]][1];

	s_keys_look_up_action.generic.type	= MTYPE_ACTION;
	s_keys_look_up_action.generic.x		= 0;
	s_keys_look_up_action.generic.y		= y += 9*scale;
	s_keys_look_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_look_up_action.generic.localdata[0] = ++i;
	s_keys_look_up_action.generic.name	= bindnames[s_keys_look_up_action.generic.localdata[0]][1];

	s_keys_look_down_action.generic.type	= MTYPE_ACTION;
	s_keys_look_down_action.generic.x		= 0;
	s_keys_look_down_action.generic.y		= y += 9*scale;
	s_keys_look_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_look_down_action.generic.localdata[0] = ++i;
	s_keys_look_down_action.generic.name	= bindnames[s_keys_look_down_action.generic.localdata[0]][1];

	s_keys_center_view_action.generic.type	= MTYPE_ACTION;
	s_keys_center_view_action.generic.x		= 0;
	s_keys_center_view_action.generic.y		= y += 9*scale;
	s_keys_center_view_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_center_view_action.generic.localdata[0] = ++i;
	s_keys_center_view_action.generic.name	= bindnames[s_keys_center_view_action.generic.localdata[0]][1];

	s_keys_mouse_look_action.generic.type	= MTYPE_ACTION;
	s_keys_mouse_look_action.generic.x		= 0;
	s_keys_mouse_look_action.generic.y		= y += 9*scale;
	s_keys_mouse_look_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_mouse_look_action.generic.localdata[0] = ++i;
	s_keys_mouse_look_action.generic.name	= bindnames[s_keys_mouse_look_action.generic.localdata[0]][1];

	s_keys_keyboard_look_action.generic.type	= MTYPE_ACTION;
	s_keys_keyboard_look_action.generic.x		= 0;
	s_keys_keyboard_look_action.generic.y		= y += 9*scale;
	s_keys_keyboard_look_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_keyboard_look_action.generic.localdata[0] = ++i;
	s_keys_keyboard_look_action.generic.name	= bindnames[s_keys_keyboard_look_action.generic.localdata[0]][1];

	s_keys_move_up_action.generic.type	= MTYPE_ACTION;
	s_keys_move_up_action.generic.x		= 0;
	s_keys_move_up_action.generic.y		= y += 9*scale;
	s_keys_move_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_up_action.generic.localdata[0] = ++i;
	s_keys_move_up_action.generic.name	= bindnames[s_keys_move_up_action.generic.localdata[0]][1];

	s_keys_move_down_action.generic.type	= MTYPE_ACTION;
	s_keys_move_down_action.generic.x		= 0;
	s_keys_move_down_action.generic.y		= y += 9*scale;
	s_keys_move_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_down_action.generic.localdata[0] = ++i;
	s_keys_move_down_action.generic.name	= bindnames[s_keys_move_down_action.generic.localdata[0]][1];

	s_keys_inventory_action.generic.type	= MTYPE_ACTION;
	s_keys_inventory_action.generic.x		= 0;
	s_keys_inventory_action.generic.y		= y += 9*scale;
	s_keys_inventory_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inventory_action.generic.localdata[0] = ++i;
	s_keys_inventory_action.generic.name	= bindnames[s_keys_inventory_action.generic.localdata[0]][1];

	s_keys_inv_use_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_use_action.generic.x		= 0;
	s_keys_inv_use_action.generic.y		= y += 9*scale;
	s_keys_inv_use_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_use_action.generic.localdata[0] = ++i;
	s_keys_inv_use_action.generic.name	= bindnames[s_keys_inv_use_action.generic.localdata[0]][1];

	s_keys_inv_drop_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_drop_action.generic.x		= 0;
	s_keys_inv_drop_action.generic.y		= y += 9*scale;
	s_keys_inv_drop_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_drop_action.generic.localdata[0] = ++i;
	s_keys_inv_drop_action.generic.name	= bindnames[s_keys_inv_drop_action.generic.localdata[0]][1];

	s_keys_inv_prev_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_prev_action.generic.x		= 0;
	s_keys_inv_prev_action.generic.y		= y += 9*scale;
	s_keys_inv_prev_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_prev_action.generic.localdata[0] = ++i;
	s_keys_inv_prev_action.generic.name	= bindnames[s_keys_inv_prev_action.generic.localdata[0]][1];

	s_keys_inv_next_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_next_action.generic.x		= 0;
	s_keys_inv_next_action.generic.y		= y += 9*scale;
	s_keys_inv_next_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_next_action.generic.localdata[0] = ++i;
	s_keys_inv_next_action.generic.name	= bindnames[s_keys_inv_next_action.generic.localdata[0]][1];

	s_keys_alien_disruptor_action.generic.type	= MTYPE_ACTION;
	s_keys_alien_disruptor_action.generic.x		= 0;
	s_keys_alien_disruptor_action.generic.y		= y += 9*scale;
	s_keys_alien_disruptor_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_alien_disruptor_action.generic.localdata[0] = ++i;
	s_keys_alien_disruptor_action.generic.name	= bindnames[s_keys_alien_disruptor_action.generic.localdata[0]][1];

	s_keys_chain_pistol_action.generic.type	= MTYPE_ACTION;
	s_keys_chain_pistol_action.generic.x		= 0;
	s_keys_chain_pistol_action.generic.y		= y += 9*scale;
	s_keys_chain_pistol_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_chain_pistol_action.generic.localdata[0] = ++i;
	s_keys_chain_pistol_action.generic.name	= bindnames[s_keys_chain_pistol_action.generic.localdata[0]][1];

	s_keys_flame_thrower_action.generic.type	= MTYPE_ACTION;
	s_keys_flame_thrower_action.generic.x		= 0;
	s_keys_flame_thrower_action.generic.y		= y += 9*scale;
	s_keys_flame_thrower_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_flame_thrower_action.generic.localdata[0] = ++i;
	s_keys_flame_thrower_action.generic.name	= bindnames[s_keys_flame_thrower_action.generic.localdata[0]][1];

	s_keys_rocket_launcher_action.generic.type	= MTYPE_ACTION;
	s_keys_rocket_launcher_action.generic.x		= 0;
	s_keys_rocket_launcher_action.generic.y		= y += 9*scale;
	s_keys_rocket_launcher_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_rocket_launcher_action.generic.localdata[0] = ++i;
	s_keys_rocket_launcher_action.generic.name	= bindnames[s_keys_rocket_launcher_action.generic.localdata[0]][1];

	s_keys_alien_smartgun_action.generic.type	= MTYPE_ACTION;
	s_keys_alien_smartgun_action.generic.x		= 0;
	s_keys_alien_smartgun_action.generic.y		= y += 9*scale;
	s_keys_alien_smartgun_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_alien_smartgun_action.generic.localdata[0] = ++i;
	s_keys_alien_smartgun_action.generic.name	= bindnames[s_keys_alien_smartgun_action.generic.localdata[0]][1];

	s_keys_alien_beamgun_action.generic.type	= MTYPE_ACTION;
	s_keys_alien_beamgun_action.generic.x		= 0;
	s_keys_alien_beamgun_action.generic.y		= y += 9*scale;
	s_keys_alien_beamgun_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_alien_beamgun_action.generic.localdata[0] = ++i;
	s_keys_alien_beamgun_action.generic.name	= bindnames[s_keys_alien_beamgun_action.generic.localdata[0]][1];

	s_keys_alien_vaporizer_action.generic.type	= MTYPE_ACTION;
	s_keys_alien_vaporizer_action.generic.x		= 0;
	s_keys_alien_vaporizer_action.generic.y		= y += 9*scale;
	s_keys_alien_vaporizer_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_alien_vaporizer_action.generic.localdata[0] = ++i;
	s_keys_alien_vaporizer_action.generic.name	= bindnames[s_keys_alien_vaporizer_action.generic.localdata[0]][1];

	s_keys_violator_action.generic.type	= MTYPE_ACTION;
	s_keys_violator_action.generic.x		= 0;
	s_keys_violator_action.generic.y		= y += 9*scale;
	s_keys_violator_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_violator_action.generic.localdata[0] = ++i;
	s_keys_violator_action.generic.name	= bindnames[s_keys_violator_action.generic.localdata[0]][1];

	s_keys_show_scores_action.generic.type	= MTYPE_ACTION;
	s_keys_show_scores_action.generic.x		= 0;
	s_keys_show_scores_action.generic.y		= y += 9*scale;
	s_keys_show_scores_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_show_scores_action.generic.localdata[0] = ++i;
	s_keys_show_scores_action.generic.name	= bindnames[s_keys_show_scores_action.generic.localdata[0]][1];

	s_keys_flashlight_action.generic.type	= MTYPE_ACTION;
	s_keys_flashlight_action.generic.x		= 0;
	s_keys_flashlight_action.generic.y		= y += 9*scale;
	s_keys_flashlight_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_flashlight_action.generic.localdata[0] = ++i;
	s_keys_flashlight_action.generic.name	= bindnames[s_keys_flashlight_action.generic.localdata[0]][1];

	s_keys_grapple_action.generic.type	= MTYPE_ACTION;
	s_keys_grapple_action.generic.x		= 0;
	s_keys_grapple_action.generic.y		= y += 9*scale;
	s_keys_grapple_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_grapple_action.generic.localdata[0] = ++i;
	s_keys_grapple_action.generic.name	= bindnames[s_keys_grapple_action.generic.localdata[0]][1];

	s_keys_filler_action.generic.type	= MTYPE_ACTION;
	s_keys_filler_action.generic.x		= 0;
	s_keys_filler_action.generic.y		= y += 9*scale;
	s_keys_filler_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_filler_action.generic.localdata[0] = ++i;
	s_keys_filler_action.generic.name	= bindnames[s_keys_filler_action.generic.localdata[0]][1];

	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_attack_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_attack2_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_change_weapon_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_walk_forward_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_backpedal_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_turn_left_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_turn_right_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_run_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_step_left_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_step_right_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_sidestep_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_look_up_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_look_down_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_center_view_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_mouse_look_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_keyboard_look_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_move_up_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_move_down_action );

	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_inventory_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_inv_use_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_inv_drop_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_inv_prev_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_inv_next_action );

	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_alien_disruptor_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_chain_pistol_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_flame_thrower_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_rocket_launcher_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_alien_smartgun_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_alien_beamgun_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_alien_vaporizer_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_violator_action );

	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_show_scores_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_flashlight_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_grapple_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_filler_action ); //needed so last item will show

	Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
	Menu_Center( &s_keys_menu );
}

static void Keys_MenuDraw (void)
{
	M_Background( "m_controls_back"); //draw black background first
	M_Banner( "m_controls" );
	Menu_AdjustCursor( &s_keys_menu, 1 );
	Menu_Draw( &s_keys_menu );
}

static const char *Keys_MenuKey( int key )
{
	menuaction_s *item = ( menuaction_s * ) Menu_ItemAtCursor( &s_keys_menu );

	//menu mouse was (bind_grab)

	if ( bind_grab && !(cursor.buttonused[MOUSEBUTTON1]&&key==K_MOUSE1))
	{
		if ( key != K_ESCAPE && key != '`' )
		{
			char cmd[1024];

			Com_sprintf (cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString(key), bindnames[item->generic.localdata[0]][0]);
			Cbuf_InsertText (cmd);
		}

		// Knightmare- added Psychospaz's mouse support
		//dont let selecting with mouse buttons screw everything up
		refreshCursorButtons();
		if (key==K_MOUSE1)
			cursor.buttonclicks[MOUSEBUTTON1] = -1;

		Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
		bind_grab = false;
		return menu_out_sound;
	}

	switch ( key )
	{
	case K_KP_ENTER:
	case K_ENTER:
	case K_MOUSE1:
		KeyBindingFunc( item );
		return menu_in_sound;
	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
	case K_KP_DEL:
		M_UnbindCommand( bindnames[item->generic.localdata[0]][0] );
		return menu_out_sound;
	default:
		return Default_MenuKey( &s_keys_menu, key );
	}
}

void M_Menu_Keys_f (void)
{
	Keys_MenuInit();
	M_PushMenu( Keys_MenuDraw, Keys_MenuKey );
}


/*
=======================================================================

CONTROLS MENU

=======================================================================
*/
static cvar_t *win_noalttab;
extern cvar_t *in_joystick;
extern cvar_t *cl_showPlayerNames;
extern cvar_t *cl_healthaura;
extern cvar_t *cl_noblood;
extern cvar_t *cl_noskins;
extern cvar_t *gl_shadows;
extern cvar_t *gl_dynamic;
extern cvar_t *gl_rtlights;
extern cvar_t *r_shaders;
extern cvar_t *r_minimap;
extern cvar_t *r_minimap_style;

static menuframework_s	s_options_menu;
static menuaction_s		s_player_setup_action;
static menuaction_s		s_options_defaults_action;
static menuaction_s		s_options_customize_options_action;
static menuslider_s		s_options_sensitivity_slider;
static menulist_s		s_options_freelook_box;
static menulist_s		s_options_noalttab_box;
static menulist_s		s_options_alwaysrun_box;
static menulist_s		s_options_invertmouse_box;
static menulist_s		s_options_font_box;
static menulist_s		s_options_crosshair_box;
static menulist_s		s_options_hud_box;
static menuslider_s		s_options_sfxvolume_slider;
static menuslider_s		s_options_bgvolume_slider;
static menulist_s		s_options_joystick_box;
static menulist_s		s_options_quality_list;
static menulist_s		s_options_compatibility_list;
static menulist_s		s_options_console_action;
static menulist_s		s_options_bgmusic_box;
static menulist_s		s_options_target_box;
static menulist_s		s_options_healthaura_box;
static menulist_s		s_options_noblood_box;
static menulist_s		s_options_noskins_box;
static menulist_s		s_options_shaders_box;
static menulist_s		s_options_shadows_box;
static menulist_s		s_options_dynamic_box;
static menulist_s		s_options_rtlights_box;
static menulist_s		s_options_minimap_box;

static void PlayerSetupFunc( void *unused )
{
	M_Menu_PlayerConfig_f();
}
static void TargetFunc( void *unused )
{
	Cvar_SetValue( "cl_showplayernames", s_options_target_box.curvalue);
}
static void HealthauraFunc( void *unused )
{
	Cvar_SetValue( "cl_healthaura", s_options_healthaura_box.curvalue);
}
static void NoBloodFunc( void *unused )
{
	Cvar_SetValue( "cl_noblood", s_options_noblood_box.curvalue);
}
static void NoskinsFunc( void *unused )
{
	Cvar_SetValue( "cl_noskins", s_options_noskins_box.curvalue);
}
static void ShadersFunc( void *unused )
{
	Cvar_SetValue( "r_shaders", s_options_shaders_box.curvalue);
	if(!s_options_shaders_box.curvalue)
		Cvar_SetValue("gl_normalmaps", 0); //because normal maps are controlled by shaders
}
static void ShadowsFunc( void *unused )
{
	Cvar_SetValue( "gl_shadows", s_options_shadows_box.curvalue);
}
static void DynamicFunc( void *unused )
{
	Cvar_SetValue( "gl_dynamic", s_options_dynamic_box.curvalue);
}
static void RTlightsFunc( void *unused )
{
	Cvar_SetValue( "gl_rtlights", s_options_rtlights_box.curvalue);
}

static void JoystickFunc( void *unused )
{
	Cvar_SetValue( "in_joystick", s_options_joystick_box.curvalue );
}

static void CustomizeControlsFunc( void *unused )
{
	M_Menu_Keys_f();
}

static void AlwaysRunFunc( void *unused )
{
	Cvar_SetValue( "cl_run", s_options_alwaysrun_box.curvalue );
}

static void FreeLookFunc( void *unused )
{
	Cvar_SetValue( "freelook", s_options_freelook_box.curvalue );
}

static void MouseSpeedFunc( void *unused )
{
	Cvar_SetValue( "sensitivity", s_options_sensitivity_slider.curvalue / 2.0F );
}

static void NoAltTabFunc( void *unused )
{
	Cvar_SetValue( "win_noalttab", s_options_noalttab_box.curvalue );
}

static void MinimapFunc( void *unused )
{
	if(s_options_minimap_box.curvalue) {
		Cvar_SetValue("r_minimap", 1);
		if(s_options_minimap_box.curvalue == 2)
			Cvar_SetValue("r_minimap_style", 0);
		else
			Cvar_SetValue("r_minimap_style", s_options_minimap_box.curvalue);
	}
	else
		Cvar_SetValue("r_minimap", s_options_minimap_box.curvalue);
}

static float ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

extern cvar_t *con_font;
#define MAX_FONTS 32
char **font_names;
int	numfonts;

static void FontFunc( void *unused )
{
	Cvar_Set( "con_font", font_names[s_options_font_box.curvalue] );
}

void SetFontCursor (void)
{
	int i;
	s_options_font_box.curvalue = 0;

	if (!con_font)
		con_font = Cvar_Get ("con_font", "default", CVAR_ARCHIVE);

	if (numfonts>1)
		for (i=0; font_names[i]; i++)
		{
			if (!Q_strcasecmp(con_font->string, font_names[i]))
			{
				s_options_font_box.curvalue = i;
				return;
			}
		}
}

qboolean fontInList (char *check, int num, char **list)
{
	int i;
	for (i=0;i<num;i++)
		if (!Q_strcasecmp(check, list[i]))
			return true;
	return false;
}

void insertFont (char ** list, char *insert, int len )
{
	int i, j;

	//i=1 so default stays first!
	for (i=1;i<len; i++)
	{
		if (!list[i])
			break;

		if (strcmp( list[i], insert ))
		{
			for (j=len; j>i ;j--)
				list[j] = list[j-1];

			list[i] = strdup(insert);

			return;
		}
	}

	list[len] = strdup(insert);
}

char **SetFontNames (void)
{
	char *curFont;
	char **list = 0, *p;
	char findname[1024];
	int nfonts = 0, nfontnames;
	char **fontfiles;
	char *path = NULL;
	int i;
	extern char **FS_ListFiles( char *, int *, unsigned, unsigned );

	list = malloc( sizeof( char * ) * MAX_FONTS );
	memset( list, 0, sizeof( char * ) * MAX_FONTS );

	list[0] = strdup("default");

	nfontnames = 1;

	path = FS_NextPath( path );
	while (path)
	{
		Com_sprintf( findname, sizeof(findname), "%s/fonts/*.tga", path );
		fontfiles = FS_ListFiles( findname, &nfonts, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		for (i=0;i<nfonts && nfontnames<MAX_FONTS;i++)
		{
			int num;

			if (!fontfiles || !fontfiles[i])
				continue;

			p = strstr(fontfiles[i], "/fonts/"); p++;
			p = strstr(p, "/"); p++;

			if (	!strstr(p, ".tga")
				&&	!strstr(p, ".pcx")
				)
				continue;

			num = strlen(p)-4;
			p[num] = 0;

			curFont = p;

			if (!fontInList(curFont, nfontnames, list))
			{
				insertFont(list, strdup(curFont),nfontnames);
				nfontnames++;
			}

			//set back so whole string get deleted.
			p[num] = '.';
		}
		path = FS_NextPath( path );
	}

	if (fontfiles)
		free( fontfiles );

	numfonts = nfontnames;

	return list;
}

extern cvar_t *crosshair;
#define MAX_CROSSHAIRS 256
char **crosshair_names;
int	numcrosshairs;

static void CrosshairFunc( void *unused )
{
	char cHair[MAX_OSPATH];

	if(s_options_crosshair_box.curvalue > 3)
		sprintf(cHair, "crosshairs/%s", crosshair_names[s_options_crosshair_box.curvalue]);
	else
		strcpy(cHair, crosshair_names[s_options_crosshair_box.curvalue]);

	Cvar_Set( "crosshair", cHair );
}

void SetCrosshairCursor (void)
{
	int i;
	char cHaircomp[MAX_OSPATH];

	s_options_crosshair_box.curvalue = 1;

	if (!crosshair)
		crosshair = Cvar_Get ("crosshair", "ch1", CVAR_ARCHIVE);

	if (numcrosshairs>1)
		for (i=0; crosshair_names[i]; i++)
		{
			sprintf(cHaircomp, "crosshairs/%s", crosshair_names[i]);
			if (!Q_strcasecmp(crosshair->string, cHaircomp))
			{
				s_options_crosshair_box.curvalue = i;
				return;
			}
		}
}

void insertCrosshair (char ** list, char *insert, int len )
{
	int i, j;

	for (i=4;i<len; i++)
	{
		if (!list[i])
			break;

		if (strcmp( list[i], insert ))
		{
			for (j=len; j>i ;j--)
				list[j] = list[j-1];

			list[i] = strdup(insert);

			return;
		}
	}

	list[len] = strdup(insert);
}

char **SetCrosshairNames (void)
{
	char *curCrosshair;
	char **list = 0, *p;
	char findname[1024];
	int ncrosshairs = 0, ncrosshairnames;
	char **crosshairfiles;
	char *path = NULL;
	int i;
	extern char **FS_ListFiles( char *, int *, unsigned, unsigned );

	list = malloc( sizeof( char * ) * MAX_CROSSHAIRS );
	memset( list, 0, sizeof( char * ) * MAX_CROSSHAIRS );

	ncrosshairnames = 4;

	list[0] = strdup("none"); //the old crosshairs
	list[1] = strdup("ch1");
	list[2] = strdup("ch2");
	list[3] = strdup("ch3");

	path = FS_NextPath( path );
	while (path)
	{
		Com_sprintf( findname, sizeof(findname), "%s/pics/crosshairs/*.tga", path );
		crosshairfiles = FS_ListFiles( findname, &ncrosshairs, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		for (i=0;i<ncrosshairs && ncrosshairnames<MAX_CROSSHAIRS;i++)
		{
			int num;

			if (!crosshairfiles || !crosshairfiles[i])
				continue;

			p = strstr(crosshairfiles[i], "/crosshairs/"); p++;
			p = strstr(p, "/"); p++;

			if (	!strstr(p, ".tga")
				&&	!strstr(p, ".pcx")
				)
				continue;

			num = strlen(p)-4;
			p[num] = 0;

			curCrosshair = p;

			if (!fontInList(curCrosshair, ncrosshairnames, list))
			{
				insertCrosshair(list, strdup(curCrosshair),ncrosshairnames);
				ncrosshairnames++;
			}

			//set back so whole string get deleted.
			p[num] = '.';
		}
		path = FS_NextPath( path );
	}

	if (crosshairfiles)
		free( crosshairfiles );

	numcrosshairs = ncrosshairnames;

	return list;
}

extern cvar_t *cl_hudimage1;
extern cvar_t *cl_hudimage2;
#define MAX_HUDS 256
char **hud_names;
int	numhuds;

static void HudFunc( void *unused )
{
	char hud1[MAX_OSPATH] = "pics/i_health.tga";
	char hud2[MAX_OSPATH] = "pics/i_score.tga";

	if(s_options_hud_box.curvalue > 0) {
		sprintf(hud1, "pics/huds/%s1", hud_names[s_options_hud_box.curvalue]);
		sprintf(hud2, "pics/huds/%s2", hud_names[s_options_hud_box.curvalue]);
	}

	//set the cvars, both of them
	Cvar_Set( "cl_hudimage1", hud1 );
	Cvar_Set( "cl_hudimage2", hud2 );
}

void SetHudCursor (void)
{
	int i;
	char hudset[MAX_OSPATH] = "default";
	char hudcomp[MAX_OSPATH];

	s_options_hud_box.curvalue = 0;

	if (!cl_hudimage1)
		cl_hudimage1 = Cvar_Get ("cl_hudimage1", "pics/i_health.tga", CVAR_ARCHIVE);
	else {
		strcpy(hudset, cl_hudimage1->string);
		hudset[strlen(hudset) - 1] = 0;
	}

	if (numhuds>0)
		for (i=1; hud_names[i]; i++)
		{
			sprintf(hudcomp, "pics/huds/%s", hud_names[i]);
			if (!Q_strcasecmp(hudset, hudcomp))
			{
				s_options_hud_box.curvalue = i;
				return;
			}
		}
}

void insertHud (char ** list, char *insert, int len )
{
	int i, j;

	for (i=1;i<len; i++)
	{
		if (!list[i])
			break;

		if (strcmp( list[i], insert ))
		{
			for (j=len; j>i ;j--)
				list[j] = list[j-1];

			list[i] = strdup(insert);

			return;
		}
	}

	list[len] = strdup(insert);

}

char **SetHudNames (void)
{
	char *curHud;
	char **list = 0, *p;
	char findname[1024];
	int nhuds = 0, nhudnames;
	char **hudfiles;
	char *path = NULL;
	int i;
	extern char **FS_ListFiles( char *, int *, unsigned, unsigned );

	list = malloc( sizeof( char * ) * MAX_HUDS );
	memset( list, 0, sizeof( char * ) * MAX_HUDS );

	nhudnames = 1;

	list[0] = strdup("default"); //the default hud

	path = FS_NextPath( path );
	while (path)
	{
		Com_sprintf( findname, sizeof(findname), "%s/pics/huds/*.tga", path );
		hudfiles = FS_ListFiles( findname, &nhuds, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		for (i=0;i<nhuds && nhudnames<MAX_HUDS;i++)
		{
			int num;

			if (!hudfiles || !hudfiles[i])
				continue;

			p = strstr(hudfiles[i], "/huds/"); p++;
			p = strstr(p, "/"); p++;

			if (	!strstr(p, ".tga")
				&&	!strstr(p, ".pcx")
				)
				continue;

			num = strlen(p)-5;
			p[num] = 0;

			curHud = p;

			if (!fontInList(curHud, nhudnames, list))
			{
				insertHud(list, strdup(curHud),nhudnames);
				nhudnames++;
			}

			//set back so whole string get deleted.
			p[num] = '.';
		}
		path = FS_NextPath( path );
	}

	if (hudfiles)
		free( hudfiles );

	numhuds = nhudnames;

	return list;
}

static void ControlsSetMenuItemValues( void )
{

	s_options_sfxvolume_slider.curvalue		= Cvar_VariableValue( "s_volume" ) * 10;
	s_options_bgvolume_slider.curvalue		= Cvar_VariableValue( "background_music_vol" ) * 10;
	s_options_bgmusic_box.curvalue			= Cvar_VariableValue("background_music");

	//Read what was set for s_khz and set the curvalue accordingly
	if (Cvar_VariableValue("s_khz") == 48)
		s_options_quality_list.curvalue = 3; //high
	else if (Cvar_VariableValue("s_khz") == 44)
		s_options_quality_list.curvalue = 2; //medium
	else if (Cvar_VariableValue("s_khz") == 22)
		s_options_quality_list.curvalue = 1; //low
	else if (Cvar_VariableValue("s_khz") == 11)
		s_options_quality_list.curvalue = 0; //basic

	s_options_sensitivity_slider.curvalue	= ( sensitivity->value ) * 2;

	SetFontCursor();
	SetCrosshairCursor();
	SetHudCursor();

	Cvar_SetValue( "cl_run", ClampCvar( 0, 1, cl_run->value ) );
	s_options_alwaysrun_box.curvalue		= cl_run->value;

	s_options_invertmouse_box.curvalue		= m_pitch->value < 0;

	Cvar_SetValue( "freelook", ClampCvar( 0, 1, freelook->value ) );
	s_options_freelook_box.curvalue			= freelook->value;

	Cvar_SetValue( "in_joystick", ClampCvar( 0, 1, in_joystick->value ) );
	s_options_joystick_box.curvalue		= in_joystick->value;

	s_options_noalttab_box.curvalue			= win_noalttab->value;

	Cvar_SetValue("cl_showplayernames", ClampCvar(0, 1, cl_showPlayerNames->value ) );
	s_options_target_box.curvalue		= cl_showPlayerNames->value;

	Cvar_SetValue("cl_noskins", ClampCvar(0, 1, cl_noskins->value ) );
	s_options_noskins_box.curvalue		= cl_noskins->value;

	Cvar_SetValue("r_shaders", ClampCvar(0, 1, r_shaders->value ) );
	s_options_shaders_box.curvalue		= r_shaders->value;

	Cvar_SetValue("gl_shadows", ClampCvar(0, 2, gl_shadows->value ) );
	s_options_shadows_box.curvalue		= gl_shadows->value;

	Cvar_SetValue("gl_dynamic", ClampCvar(0, 2, gl_dynamic->value ) );
	s_options_dynamic_box.curvalue		= gl_dynamic->value;

	Cvar_SetValue("gl_rtlights", ClampCvar(0, 1, gl_rtlights->value ) );
	s_options_rtlights_box.curvalue		= gl_rtlights->value;

	Cvar_SetValue("cl_healthaura", ClampCvar(0, 1, cl_healthaura->value ) );
	s_options_healthaura_box.curvalue		= cl_healthaura->value;

	Cvar_SetValue("cl_noblood", ClampCvar(0, 1, cl_noblood->value ) );
	s_options_noblood_box.curvalue		= cl_noblood->value;

	Cvar_SetValue("r_minimap_style", ClampCvar(0, 1, r_minimap_style->value));
	Cvar_SetValue("r_minimap", ClampCvar(0, 1, r_minimap->value));
	if(r_minimap_style->value == 0) {
		s_options_minimap_box.curvalue = 2;
	}
	else if(r_minimap->value)
		s_options_minimap_box.curvalue = 1;
	else
		s_options_minimap_box.curvalue = 0;

}

static void ControlsResetDefaultsFunc( void *unused )
{
	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_Execute();

	ControlsSetMenuItemValues();
}

//JD - the next three functions were completely screwed up out of the box by id...so they
//are fixed now.
static void InvertMouseFunc( void *unused )
{
	if(s_options_invertmouse_box.curvalue && m_pitch->value > 0)
		Cvar_SetValue( "m_pitch", -m_pitch->value );
	else if(m_pitch->value < 0)
		Cvar_SetValue( "m_pitch", -m_pitch->value );
}

static void UpdateVolumeFunc( void *unused )
{
	Cvar_SetValue( "s_volume", s_options_sfxvolume_slider.curvalue / 10 );
}

static void UpdateBGVolumeFunc( void *unused )
{
	Cvar_SetValue( "background_music_vol", s_options_bgvolume_slider.curvalue / 10 );
}

static void UpdateBGMusicFunc( void *unused )
{
	Cvar_SetValue( "background_music", s_options_bgmusic_box.curvalue );
}
static void ConsoleFunc( void *unused )
{
	/*
	** the proper way to do this is probably to have ToggleConsole_f accept a parameter
	*/
	extern void Key_ClearTyping( void );

// 	if ( cl.attractloop )
// 	{
// 		Cbuf_AddText ("killserver\n");
// 		return;
// 	}

	Key_ClearTyping ();
	Con_ClearNotify ();

	M_ForceMenuOff ();
	cls.key_dest = key_console;
}

static void UpdateSoundQualityFunc( void *unused )
{
	if ( s_options_quality_list.curvalue == 3 ) //high
	{
		Cvar_SetValue( "s_khz", 48 );
		Cvar_SetValue( "s_loadas8bit", false );
	}
	else if ( s_options_quality_list.curvalue == 2 ) //medium
	{
		Cvar_SetValue( "s_khz", 44 );
		Cvar_SetValue( "s_loadas8bit", false );
	}
	else if ( s_options_quality_list.curvalue == 1 ) //low
	{
		Cvar_SetValue( "s_khz", 22 );
		Cvar_SetValue( "s_loadas8bit", false );
	}
	else if ( s_options_quality_list.curvalue == 0 ) //basic
	{
		Cvar_SetValue( "s_khz", 11 );
		Cvar_SetValue( "s_loadas8bit", true );
	}

	Cvar_SetValue( "s_primary", s_options_compatibility_list.curvalue );

	// the text box won't show up unless we do a buffer swap
	R_EndFrame();

	CL_Snd_Restart_f();
}

void Options_MenuInit( void )
{
	static const char *background_music_items[] =
	{
		"disabled",
		"enabled",
		0
	};

	//here we will assume s_options_quality_list.curvalue 0 through 3
	static const char *quality_items[] =
	{
		"11 khz",
		"22 khz",
		"44 khz",
		"48 hkz",
		0
	};

	static const char *compatibility_items[] =
	{
		"max compatibility", "max performance", 0
	};

	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	static const char *onoff_names[] =
	{
		"off",
		"on",
		0
	};

	static const char *shadow_names[] =
	{
		"off",
		"dynamic",
		"dynamic+world",
		0
	};
	static const char *rtlights_names[] =
	{
		"off",
		"on",
		0
	};
	static const char *minimap_names[] =
	{
		"off",
		"static",
		"rotating",
		0
	};

	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	win_noalttab = Cvar_Get( "win_noalttab", "0", CVAR_ARCHIVE );

	/*
	** configure controls menu and menu items
	*/
	s_options_menu.x = viddef.width / 2;
	s_options_menu.y = viddef.height / 2 - 100*scale;
	s_options_menu.nitems = 0;

	s_player_setup_action.generic.type	= MTYPE_ACTION;
	s_player_setup_action.generic.x		= 0;
	s_player_setup_action.generic.y		= 0;
	s_player_setup_action.generic.name	= "player setup";
	s_player_setup_action.generic.callback = PlayerSetupFunc;

	s_options_customize_options_action.generic.type	= MTYPE_ACTION;
	s_options_customize_options_action.generic.x		= 0;
	s_options_customize_options_action.generic.y		= 10*scale;
	s_options_customize_options_action.generic.name	= "customize controls";
	s_options_customize_options_action.generic.callback = CustomizeControlsFunc;

	s_options_shaders_box.generic.type = MTYPE_SPINCONTROL;
	s_options_shaders_box.generic.x	= 0;
	s_options_shaders_box.generic.y	= 30*scale;
	s_options_shaders_box.generic.name	= "shaders";
	s_options_shaders_box.generic.callback = ShadersFunc;
	s_options_shaders_box.itemnames = onoff_names;

	s_options_shadows_box.generic.type = MTYPE_SPINCONTROL;
	s_options_shadows_box.generic.x	= 0;
	s_options_shadows_box.generic.y	= 40*scale;
	s_options_shadows_box.generic.name	= "shadows";
	s_options_shadows_box.generic.callback = ShadowsFunc;
	s_options_shadows_box.itemnames = shadow_names;

	s_options_dynamic_box.generic.type = MTYPE_SPINCONTROL;
	s_options_dynamic_box.generic.x	= 0;
	s_options_dynamic_box.generic.y	= 50*scale;
	s_options_dynamic_box.generic.name	= "dynamic lights";
	s_options_dynamic_box.generic.callback = DynamicFunc;
	s_options_dynamic_box.itemnames = onoff_names;

	s_options_rtlights_box.generic.type = MTYPE_SPINCONTROL;
	s_options_rtlights_box.generic.x	= 0;
	s_options_rtlights_box.generic.y	= 60*scale;
	s_options_rtlights_box.generic.name	= "real time lighting";
	s_options_rtlights_box.generic.callback = RTlightsFunc;
	s_options_rtlights_box.itemnames = rtlights_names;

	s_options_target_box.generic.type = MTYPE_SPINCONTROL;
	s_options_target_box.generic.x	= 0;
	s_options_target_box.generic.y	= 70*scale;
	s_options_target_box.generic.name	= "identify target";
	s_options_target_box.generic.callback = TargetFunc;
	s_options_target_box.itemnames = yesno_names;

	s_options_healthaura_box.generic.type = MTYPE_SPINCONTROL;
	s_options_healthaura_box.generic.x	= 0;
	s_options_healthaura_box.generic.y	= 80*scale;
	s_options_healthaura_box.generic.name	= "health auras";
	s_options_healthaura_box.generic.callback = HealthauraFunc;
	s_options_healthaura_box.itemnames = onoff_names;

	s_options_noblood_box.generic.type = MTYPE_SPINCONTROL;
	s_options_noblood_box.generic.x	= 0;
	s_options_noblood_box.generic.y	= 90*scale;
	s_options_noblood_box.generic.name	= "No Blood";
	s_options_noblood_box.generic.callback = NoBloodFunc;
	s_options_noblood_box.itemnames = onoff_names;

	s_options_noskins_box.generic.type = MTYPE_SPINCONTROL;
	s_options_noskins_box.generic.x	= 0;
	s_options_noskins_box.generic.y	= 100*scale;
	s_options_noskins_box.generic.name	= "force martian models";
	s_options_noskins_box.generic.callback = NoskinsFunc;
	s_options_noskins_box.itemnames = onoff_names;

	s_options_sfxvolume_slider.generic.type	= MTYPE_SLIDER;
	s_options_sfxvolume_slider.generic.x	= 0;
	s_options_sfxvolume_slider.generic.y	= 110*scale;
	s_options_sfxvolume_slider.generic.name	= "global volume";
	s_options_sfxvolume_slider.generic.callback	= UpdateVolumeFunc;
	s_options_sfxvolume_slider.minvalue		= 0;
	s_options_sfxvolume_slider.maxvalue		= 10;
	s_options_sfxvolume_slider.curvalue		= Cvar_VariableValue( "s_volume" ) * 10;

	s_options_bgvolume_slider.generic.type	= MTYPE_SLIDER;
	s_options_bgvolume_slider.generic.x	= 0;
	s_options_bgvolume_slider.generic.y	= 120*scale;
	s_options_bgvolume_slider.generic.name	= "music volume";
	s_options_bgvolume_slider.generic.callback	= UpdateBGVolumeFunc;
	s_options_bgvolume_slider.minvalue		= 0;
	s_options_bgvolume_slider.maxvalue		= 20;
	s_options_bgvolume_slider.curvalue		= Cvar_VariableValue( "background_music_vol" ) * 10;

	s_options_bgmusic_box.generic.type	= MTYPE_SPINCONTROL;
	s_options_bgmusic_box.generic.x		= 0;
	s_options_bgmusic_box.generic.y		= 130*scale;
	s_options_bgmusic_box.generic.name	= "Background music";
	s_options_bgmusic_box.generic.callback	= UpdateBGMusicFunc;
	s_options_bgmusic_box.itemnames		= background_music_items;
	s_options_bgmusic_box.curvalue 		= Cvar_VariableValue("background_music");

	s_options_quality_list.generic.type	= MTYPE_SPINCONTROL;
	s_options_quality_list.generic.x		= 0;
	s_options_quality_list.generic.y		= 140*scale;
	s_options_quality_list.generic.name		= "sampling rate";
	s_options_quality_list.generic.callback = UpdateSoundQualityFunc;
	s_options_quality_list.itemnames		= quality_items;

	s_options_compatibility_list.generic.type	= MTYPE_SPINCONTROL;
	s_options_compatibility_list.generic.x		= 0;
	s_options_compatibility_list.generic.y		= 150*scale;
	s_options_compatibility_list.generic.name	= "sound compatibility";
	s_options_compatibility_list.generic.callback = UpdateSoundQualityFunc;
	s_options_compatibility_list.itemnames		= compatibility_items;
	s_options_compatibility_list.curvalue		= Cvar_VariableValue( "s_primary" );

	s_options_sensitivity_slider.generic.type	= MTYPE_SLIDER;
	s_options_sensitivity_slider.generic.x		= 0;
	s_options_sensitivity_slider.generic.y		= 170*scale;
	s_options_sensitivity_slider.generic.name	= "mouse speed";
	s_options_sensitivity_slider.generic.callback = MouseSpeedFunc;
	s_options_sensitivity_slider.minvalue		= 2;
	s_options_sensitivity_slider.maxvalue		= 22;

	s_options_alwaysrun_box.generic.type = MTYPE_SPINCONTROL;
	s_options_alwaysrun_box.generic.x	= 0;
	s_options_alwaysrun_box.generic.y	= 180*scale;
	s_options_alwaysrun_box.generic.name	= "always run";
	s_options_alwaysrun_box.generic.callback = AlwaysRunFunc;
	s_options_alwaysrun_box.itemnames = yesno_names;

	s_options_invertmouse_box.generic.type = MTYPE_SPINCONTROL;
	s_options_invertmouse_box.generic.x	= 0;
	s_options_invertmouse_box.generic.y	= 190*scale;
	s_options_invertmouse_box.generic.name	= "invert mouse";
	s_options_invertmouse_box.generic.callback = InvertMouseFunc;
	s_options_invertmouse_box.itemnames = yesno_names;

	font_names = SetFontNames ();
	s_options_font_box.generic.type = MTYPE_SPINCONTROL;
	s_options_font_box.generic.x	= 0;
	s_options_font_box.generic.y	= 210*scale;
	s_options_font_box.generic.name	= "font";
	s_options_font_box.generic.callback = FontFunc;
	s_options_font_box.itemnames = font_names;
	s_options_font_box.generic.statusbar	= "select your font";

	crosshair_names = SetCrosshairNames ();
	s_options_crosshair_box.generic.type = MTYPE_SPINCONTROL;
	s_options_crosshair_box.generic.x	= 0;
	s_options_crosshair_box.generic.y	= 230*scale;
	s_options_crosshair_box.generic.name	= "crosshair";
	s_options_crosshair_box.generic.callback = CrosshairFunc;
	s_options_crosshair_box.itemnames = crosshair_names;
	s_options_crosshair_box.generic.statusbar	= "select your crosshair";

	hud_names = SetHudNames ();
	s_options_hud_box.generic.type = MTYPE_SPINCONTROL;
	s_options_hud_box.generic.x	= 0;
	s_options_hud_box.generic.y	= 240*scale;
	s_options_hud_box.generic.name	= "hud";
	s_options_hud_box.generic.callback = HudFunc;
	s_options_hud_box.itemnames = hud_names;
	s_options_hud_box.generic.statusbar	= "select your hud style";

	s_options_minimap_box.generic.type = MTYPE_SPINCONTROL;
	s_options_minimap_box.generic.x		= 0;
	s_options_minimap_box.generic.y		= 250*scale;
	s_options_minimap_box.generic.name  = "minimap";
	s_options_minimap_box.generic.callback = MinimapFunc;
	s_options_minimap_box.itemnames = minimap_names;

	s_options_joystick_box.generic.type = MTYPE_SPINCONTROL;
	s_options_joystick_box.generic.x	= 0;
	s_options_joystick_box.generic.y	= 260*scale;
	s_options_joystick_box.generic.name	= "use joystick";
	s_options_joystick_box.generic.callback = JoystickFunc;
	s_options_joystick_box.itemnames = yesno_names;

	s_options_defaults_action.generic.type	= MTYPE_ACTION;
	s_options_defaults_action.generic.x		= 0;
	s_options_defaults_action.generic.y		= 280*scale;
	s_options_defaults_action.generic.name	= "reset defaults";
	s_options_defaults_action.generic.callback = ControlsResetDefaultsFunc;

	s_options_console_action.generic.type	= MTYPE_ACTION;
	s_options_console_action.generic.x		= 0;
	s_options_console_action.generic.y		= 290*scale;
	s_options_console_action.generic.name	= "go to console";
	s_options_console_action.generic.callback = ConsoleFunc;

	ControlsSetMenuItemValues();

	Menu_AddItem( &s_options_menu, ( void * ) &s_player_setup_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_customize_options_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_shaders_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_shadows_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_dynamic_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_rtlights_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_target_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_healthaura_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_noblood_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_noskins_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_sfxvolume_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_bgvolume_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_bgmusic_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_quality_list );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_compatibility_list );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_sensitivity_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_alwaysrun_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_invertmouse_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_font_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_crosshair_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_hud_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_minimap_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_joystick_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_defaults_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_console_action );
}

void Options_MenuDraw (void)
{
	char path[MAX_QPATH];

	M_Background( "m_options_back"); //draw black background first
	M_Banner( "m_options" );
	if(strcmp(crosshair->string, "none")) {
		sprintf(path, "/pics/%s", crosshair->string);
		M_CrosshairPic(path);
	}
	Menu_AdjustCursor( &s_options_menu, 1 );
	Menu_Draw( &s_options_menu );
}

const char *Options_MenuKey( int key )
{
	return Default_MenuKey( &s_options_menu, key );
}

void M_Menu_Options_f (void)
{
	Options_MenuInit();
	M_PushMenu ( Options_MenuDraw, Options_MenuKey );
}

/*
=======================================================================

VIDEO MENU

=======================================================================
*/

void M_Menu_Video_f (void)
{
	VID_MenuInit();
	M_PushMenu( VID_MenuDraw, VID_MenuKey );
}

/*
=============================================================================

END GAME MENU

=============================================================================
*/
static int credits_start_time;
static const char **credits;
static char *creditsIndex[256];
static char *creditsBuffer;
static const char *idcredits[] =
{
	"+Alien Arena by COR Entertainment",
	"",
	"+PROGRAMMING",
	"John Diamond",
	"Victor Luchits",
	"Jan Rafaj",
	"Shane Bayer",
	"Tony Jackson",
	"Stephan Stahl",
	"Kyle Hunter",
	"Andres Mejia",
	"Quakesrc.org",
	"",
	"+ART",
	"John Diamond",
	"Shawn Keeth",
	"Enki",
	"",
	"+FONT",
	"John Diamond", 
	"the-interceptor from http://www.quakeworld.nu/",
	"",
	"+LOGO",
	"Adam -servercleaner- Szalai",
	"",
	"+LEVEL DESIGN",
	"John Diamond",
	"",
	"+SOUND EFFECTS AND MUSIC",
	"Music/FX Composed and Produced by",
	"John Diamond, Whitelipper",
	"Arteria Games, Wooden Productions",
	"and Soundrangers.com",
	"",
	"+CROSSHAIRS AND HUDS",
	"Astralsin",
	"Dangeresque",
	"Phenax",
	"Roadrage",
	"Forsaken",
	"Capt Crazy",
	"Torok -Intimidator- Ivan",
	"Stratocaster",
	"ChexGuy",
	"Crayon",
	"",
	"+LINUX PORT",
	"Shane Bayer",
	"",
	"+FREEBSD PORT",
	"Ale",
	"",
	"+GENTOO PORTAGE",
	"Paul Bredbury",
	"",
	"+DEBIAN PACKAGE",
	"Andres Mejia",
	"",
	"+LANGUAGE TRANSLATIONS",
	"Ken Deguisse",
	"",
	"+STORYLINE", 
	"Sinnocent", 
	"",
	"+SPECIAL THANKS",
	"The Alien Arena Community",
	"and everyone else who",
	"has been loyal to the",
	"game.",
	"",
	"",
	"+ALIEN ARENA - THE STORY",
	"",
	"Alien Arena : Many are called, only one will reign supreme",
	"",
	"Eternal war ravaged the vastness of infinite space.",
	"For as far back into the ages as the memories of the",
	"galaxies oldest races could reach, it had been this way.",
	"Planet at war with planet, conflicts ending with a",
	"burned cinder orbiting a dead sun and countless billions",
	"sent screaming into oblivion. War, endless and ",
	"eternal, embracing all the peoples of the cosmos.",
	"Scientific triumphs heralded the creation of ever more",
	"deadly weapons until the very fabric of the universe itself was threatened.",
	"",
	"Then came the call.",
	"",
	"Some said it was sent by an elder race, legendary beings",
	"of terrifying power who had existed since the",
	"birth of the stars and who now made their home beyond",
	"the fringes of known creation, others whispered",
	"fearfully and looked to the skies for the coming of their",
	"gods. Perhaps it didn't matter who had sent the",
	"call, for all the people of the stars could at least agree",
	"that the call was there.",
	"",
	"The wars were to end - or they would be ended. In a",
	"demonstration of power whoever had sent the call",
	"snuffed out the homeworld of the XXXX, the greatest",
	"empire of all the stars, in a heartbeat. One moment it",
	"was there, the next it was dust carried on the solar winds.",
	"All races had no choice but to heed the call.",
	"",
	"For most the call was a distant tug, a whispered warning",
	"that the wars were over, but for the greatest",
	"hero of each people it was more, it was a fire raging",
	"through their blood, a call to a new war, to the battle",
	"to end all battles. That fire burns in your blood, compelling",
	"you, the greatest warrior of your people, to fight",
	"in a distant and unknown arena, your honor and the",
	"future of your race at stake.",
	"",
	"Across the stars you traveled in search of this arena where",
	"the mightiest of the mighty would do battle,",
	"where you would stand face to face with your enemies",
	"in a duel to the death. Strange new weapons",
	"awaited you, weapons which you would have to master",
	"if you were to survive and emerge victorious from",
	"the complex and deadly arenas in which you were summoned to fight.",
	"",
	"The call to battle beats through your heart and soul",
	"like the drums of war. Will you be the one to rise",
	"through the ranks and conquer all others, the one who",
	"stands proud as the undefeated champion of the",
	"Alien Arena?",
	"",
	"Alien Arena (C)2007 COR Entertainment, LLC",
	"All Rights Reserved.",
	0
};

void M_Credits_MenuDraw( void )
{
	int i, y;

	/*
	** draw the credits
	*/
	for ( i = 0, y = viddef.height - ( ( cls.realtime - credits_start_time ) / 40.0F ); credits[i]; y += 10, i++ )
	{
		int j, stringoffset = 0;
		int bold = false;

		if ( y <= -8 )
			continue;

		if ( credits[i][0] == '+' )
		{
			bold = true;
			stringoffset = 1;
		}
		else
		{
			bold = false;
			stringoffset = 0;
		}

		for ( j = 0; credits[i][j+stringoffset]; j++ )
		{
			int x;

			x = ( viddef.width - strlen( credits[i] ) * 8 - stringoffset * 8 ) / 2 + ( j + stringoffset ) * 8;

			if ( bold )
				Draw_Char( x, y, credits[i][j+stringoffset] + 128 );
			else
				Draw_Char( x, y, credits[i][j+stringoffset] );
		}
	}

	if ( y < 0 )
		credits_start_time = cls.realtime;
}

const char *M_Credits_Key( int key )
{
	switch (key)
	{
	case K_ESCAPE:
		if (creditsBuffer)
			FS_FreeFile (creditsBuffer);
		M_PopMenu ();
		break;
	}

	return menu_out_sound;

}

extern int Developer_searchpath (int who);

void M_Menu_Credits_f( void )
{

	creditsBuffer = NULL;
	credits = idcredits;
	credits_start_time = cls.realtime;

	M_PushMenu( M_Credits_MenuDraw, M_Credits_Key);
}

/*
=============================================================================

GAME MENU

=============================================================================
*/

static int		m_game_cursor;

static menuframework_s	s_game_menu;
static menuaction_s		s_game_title;
static menuaction_s		s_easy_game_action;
static menuaction_s		s_medium_game_action;
static menuaction_s		s_hard_game_action;
static menuaction_s		s_credits_action;
static menuseparator_s	s_blankline;

static void StartGame( void )
{
	// disable updates and start the cinematic going
	cl.servercount = -1;
	M_ForceMenuOff ();
	Cvar_SetValue( "deathmatch", 1 );
	Cvar_SetValue( "ctf", 0 );

	Cvar_SetValue( "gamerules", 0 );		//PGM

	Cbuf_AddText ("loading ; killserver ; wait ; newgame\n");
	cls.key_dest = key_game;
}

static void EasyGameFunc( void *data )
{
	Cvar_ForceSet( "skill", "0" );
	StartGame();
}

static void MediumGameFunc( void *data )
{
	Cvar_ForceSet( "skill", "1" );
	StartGame();
}

static void HardGameFunc( void *data )
{
	Cvar_ForceSet( "skill", "2" );
	StartGame();
}

static void CreditsFunc( void *unused )
{
	M_Menu_Credits_f();
}

void Game_MenuInit( void )
{
	static const char *difficulty_names[] =
	{
		"easy",
		"medium",
		"hard",
		0
	};
	float scale;;
	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	s_game_menu.x = viddef.width * 0.50;
	s_game_menu.nitems = 0;

	s_game_title.generic.type	= MTYPE_SEPARATOR;
	s_game_title.generic.x		= 52;
	s_game_title.generic.y		= 40*scale;
	s_game_title.generic.name	= "Instant Action!";

	s_easy_game_action.generic.type	= MTYPE_ACTION;
	s_easy_game_action.generic.x		= 24;
	s_easy_game_action.generic.y		= 50*scale;
	s_easy_game_action.generic.cursor_offset = -16;
	s_easy_game_action.generic.name	= "easy";
	s_easy_game_action.generic.callback = EasyGameFunc;

	s_medium_game_action.generic.type	= MTYPE_ACTION;
	s_medium_game_action.generic.x		= 24;
	s_medium_game_action.generic.y		= 60*scale;
	s_medium_game_action.generic.cursor_offset = -16;
	s_medium_game_action.generic.name	= "medium";
	s_medium_game_action.generic.callback = MediumGameFunc;

	s_hard_game_action.generic.type	= MTYPE_ACTION;
	s_hard_game_action.generic.x		= 24;
	s_hard_game_action.generic.y		= 70*scale;
	s_hard_game_action.generic.cursor_offset = -16;
	s_hard_game_action.generic.name	= "hard";
	s_hard_game_action.generic.callback = HardGameFunc;

	s_blankline.generic.type = MTYPE_SEPARATOR;

	s_credits_action.generic.type	= MTYPE_ACTION;
	s_credits_action.generic.x		= 24;
	s_credits_action.generic.y		= 90*scale;
	s_credits_action.generic.cursor_offset = -16;
	s_credits_action.generic.name	= "credits";
	s_credits_action.generic.callback = CreditsFunc;

	Menu_AddItem( &s_game_menu, ( void * ) &s_game_title );
	Menu_AddItem( &s_game_menu, ( void * ) &s_easy_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_medium_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_hard_game_action );
	Menu_AddItem( &s_game_menu, ( void * ) &s_blankline );
	Menu_AddItem( &s_game_menu, ( void * ) &s_blankline );
	Menu_AddItem( &s_game_menu, ( void * ) &s_credits_action );

	Menu_Center( &s_game_menu );
}

void Game_MenuDraw( void )
{
	M_Background( "m_player_back"); //draw black background first
	M_Banner( "m_single" );
	Menu_AdjustCursor( &s_game_menu, 1 );
	Menu_Draw( &s_game_menu );
}

const char *Game_MenuKey( int key )
{
	return Default_MenuKey( &s_game_menu, key );
}

void M_Menu_Game_f (void)
{
	Game_MenuInit();
	M_PushMenu( Game_MenuDraw, Game_MenuKey );
	m_game_cursor = 1;
}

/*
=============================================================================

JOIN SERVER MENU

=============================================================================
*/
#define MAX_LOCAL_SERVERS 128

static menuframework_s	s_joinserver_menu;
static menuaction_s		s_joinserver_search_action;
static menuaction_s		s_joinserver_address_book_action;
static menulist_s		s_joinserver_filterempty_action;
static menuaction_s		s_joinserver_server_actions[MAX_LOCAL_SERVERS];
static menuaction_s		s_joinserver_server_info[32];
static menuaction_s		s_joinserver_server_data[5];
static menuaction_s		s_joinserver_moveup;
static menuaction_s		s_joinserver_movedown;
static menuaction_s		s_playerlist_moveup;
static menuaction_s		s_playerlist_movedown;

int		m_num_servers;
int		m_show_empty;

#define	NO_SERVER_STRING	"<no server>"

static char local_server_info[256][256];
static char local_server_data[5][64];
unsigned int starttime;

char *GetLine (char **contents, int *len)
{
	int num;
	int i;
	char line[2048];
	char *ret;

	num = 0;
	line[0] = '\0';

	if (*len <= 0)
		return NULL;

	for (i = 0; i < *len; i++) {
		if ((*contents)[i] == '\n') {
			*contents += (num + 1);
			*len -= (num + 1);
			line[num] = '\0';
			ret = (char *)malloc (sizeof(line));
			strcpy (ret, line);
			return ret;
		} else {
			line[num] = (*contents)[i];
			num++;
		}
	}

	ret = (char *)malloc (sizeof(line));
	strcpy (ret, line);
	return ret;
}

typedef struct _SERVERDATA {

	char szHostName[64];
	char szMapName[64];
	char szAdmin[64];
	char szVersion[64];
	char szWebsite[64];
	char maxClients[32];
	char fraglimit[32];
	char timelimit[32];
	char playerInfo[64][80];
	int players;
	int ping;
	netadr_t local_server_netadr;
	char serverInfo[256];

} SERVERDATA;

SERVERDATA mservers[MAX_LOCAL_SERVERS];

void M_AddToServerList (netadr_t adr, char *status_string)
{
	char *rLine;
	char *token;
	char lasttoken[256];
	char seps[]   = "\\";
	int players = 0;
	int result;
	char playername[16];
	int score, ping, i;

	//if by some chance this gets called without the menu being up, return
	if(cls.key_dest != key_menu)
		return;

	if (m_num_servers == MAX_LOCAL_SERVERS)
		return;

	mservers[m_num_servers].local_server_netadr = adr;
	mservers[m_num_servers].ping = Sys_Milliseconds() - starttime;

	//parse it

	result = strlen(status_string);

	//server info - we may revisit this
	rLine = GetLine (&status_string, &result);

	//set the displayed default data first
	Com_sprintf(mservers[m_num_servers].szAdmin, sizeof(mservers[m_num_servers].szAdmin), "Admin:");
	Com_sprintf(mservers[m_num_servers].szWebsite, sizeof(mservers[m_num_servers].szWebsite), "Website:");
	Com_sprintf(mservers[m_num_servers].fraglimit, sizeof(mservers[m_num_servers].fraglimit), "Fraglimit:");
	Com_sprintf(mservers[m_num_servers].timelimit, sizeof(mservers[m_num_servers].timelimit), "Timelimit:");
	Com_sprintf(mservers[m_num_servers].szVersion, sizeof(mservers[m_num_servers].szVersion), "Version:");

	/* Establish string and get the first token: */
	token = strtok( rLine, seps );
	while( token != NULL ) {
		/* While there are tokens in "string" */
		if (!_stricmp (lasttoken, "admin"))
			Com_sprintf(mservers[m_num_servers].szAdmin, sizeof(mservers[m_num_servers].szAdmin), "Admin: %s", token);
		else if (!_stricmp (lasttoken, "website"))
			Com_sprintf(mservers[m_num_servers].szWebsite, sizeof(mservers[m_num_servers].szWebsite), "%s", token);
		else if (!_stricmp (lasttoken, "fraglimit"))
			Com_sprintf(mservers[m_num_servers].fraglimit, sizeof(mservers[m_num_servers].fraglimit), "Fraglimit: %s", token);
		else if (!_stricmp (lasttoken, "timelimit"))
			Com_sprintf(mservers[m_num_servers].timelimit, sizeof(mservers[m_num_servers].timelimit), "Timelimit: %s", token);
		else if (!_stricmp (lasttoken, "version"))
			Com_sprintf(mservers[m_num_servers].szVersion, sizeof(mservers[m_num_servers].szVersion), "%s", token);
		else if (!_stricmp (lasttoken, "mapname"))
			Com_sprintf(mservers[m_num_servers].szMapName, sizeof(mservers[m_num_servers].szMapName), "%s", token);
		else if (!_stricmp (lasttoken, "hostname"))
			Com_sprintf(mservers[m_num_servers].szHostName, sizeof(mservers[m_num_servers].szHostName), "%s", token);
		else if (!_stricmp (lasttoken, "maxclients"))
			Com_sprintf(mservers[m_num_servers].maxClients, sizeof(mservers[m_num_servers].maxClients), "%s", token);

		/* Get next token: */
		strcpy (lasttoken, token);
		token = strtok( NULL, seps );
	}

	free (rLine);

	//playerinfo
	strcpy (seps, " ");
	while ((rLine = GetLine (&status_string, &result)) && players < 32) {
		/* Establish string and get the first token: */
		token = strtok( rLine, seps);
		score = atoi(token);

		token = strtok( NULL, seps);
		ping = atoi(token);

		token = strtok( NULL, "\"");

		if (token)
			strncpy (playername, token, sizeof(playername)-1);
		else
			playername[0] = '\0';

		free (rLine);

		Com_sprintf(mservers[m_num_servers].playerInfo[players], sizeof(mservers[m_num_servers].playerInfo[players]), "%s    %4i    %4i\n", playername, score, ping);

		players++;
	}

	if(!m_show_empty)
		if(!players)
			return;

	mservers[m_num_servers].players = players;

	//build the string for the server (hostname - address - mapname - players/maxClients)
	//pad the strings - gotta do this for both maps and hostname
	for(i=0; i<20; i++) {
		if(!mservers[m_num_servers].szHostName[i])
			mservers[m_num_servers].szHostName[i] = 32;
	}
	mservers[m_num_servers].szHostName[20] = 0;
	for(i=0; i<12; i++) {
		if(!mservers[m_num_servers].szMapName[i])
			mservers[m_num_servers].szMapName[i] = 32;
	}
	mservers[m_num_servers].szMapName[12] = 0;
	Com_sprintf(mservers[m_num_servers].serverInfo, sizeof(mservers[m_num_servers].serverInfo), "%s  %12s  %2i/%2s  %4i", mservers[m_num_servers].szHostName,
		mservers[m_num_servers].szMapName, players, mservers[m_num_servers].maxClients, mservers[m_num_servers].ping);

	Con_Clear_f();
	m_num_servers++;
}
void MoveUp ( void *self)
{
	svridx--;
	if(svridx < 0)
		svridx = 0;
}
void MoveDown ( void *self)
{
	svridx++;
	if(svridx > 112)
		svridx = 112;
}
void MoveUp_plist ( void *self)
{
	playeridx--;
	if(playeridx < 0)
		playeridx = 0;
}
void MoveDown_plist ( void *self)
{
	playeridx++;
	if(playeridx > 24)
		playeridx = 24;
}

//join on double click, return info on single click
void JoinServerFunc( void *self )
{
	char	buffer[128];
	int		index;
	int     i;

	index = ( menuaction_s * ) self - s_joinserver_server_actions;

	if ( Q_stricmp( mservers[index+svridx].szHostName, NO_SERVER_STRING ) == 0 )
		return;

	if (index >= m_num_servers)
		return;

	if(cursor.buttonclicks[MOUSEBUTTON1] != 2)
	{

		//initialize
		for (i=0 ; i<32 ; i++)
			local_server_info[i][0] = '\0';

	    //set strings for output
		mservers[index+svridx].szAdmin[24] = 0; //trim some potential box offenders
		mservers[index+svridx].szWebsite[24] = 0;
		mservers[index+svridx].szVersion[24] = 0;
		Com_sprintf(local_server_data[0], sizeof(local_server_data[0]), mservers[index+svridx].szAdmin);
		Com_sprintf(local_server_data[1], sizeof(local_server_data[1]), mservers[index+svridx].szWebsite);
		Com_sprintf(local_server_data[2], sizeof(local_server_data[2]), mservers[index+svridx].fraglimit);
		Com_sprintf(local_server_data[3], sizeof(local_server_data[3]), mservers[index+svridx].timelimit);
		Com_sprintf(local_server_data[4], sizeof(local_server_data[4]), mservers[index+svridx].szVersion);

		//players
		for(i=0; i<mservers[index+svridx].players; i++)
			Com_sprintf(local_server_info[i], sizeof(local_server_info[i]), mservers[index+svridx].playerInfo[i]);

		return;
	}

	Com_sprintf (buffer, sizeof(buffer), "connect %s\n", NET_AdrToString (mservers[index+svridx].local_server_netadr));
	Cbuf_AddText (buffer);
	M_ForceMenuOff ();
}

void AddressBookFunc( void *self )
{
	M_Menu_AddressBook_f();
}

void NullCursorDraw( void *self )
{
}

void SearchLocalGames( void )
{
	int		i;

	svridx = 0;
	playeridx = 0;
	m_num_servers = 0;
	for (i=0 ; i<MAX_LOCAL_SERVERS ; i++)
		strcpy (mservers[i].serverInfo, NO_SERVER_STRING);

	// the text box won't show up unless we do a buffer swap
	R_EndFrame();

	// send out info packets
	CL_PingServers_f();

#ifdef __unix__
	sleep(1);
#else
	Sleep(1000); //time to recieve packets
#endif

	Con_Clear_f();
}

void SearchLocalGamesFunc( void *self )
{
	SearchLocalGames();
}

static void FilterEmptyFunc( void *unused )
{
	m_show_empty = s_joinserver_filterempty_action.curvalue;
}
void JoinServer_MenuInit( void )
{
	int i;
	float scale;
	
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	m_show_empty = true;

	s_joinserver_menu.x = viddef.width * 0.50 - 120*scale;
	s_joinserver_menu.nitems = 0;

	s_joinserver_address_book_action.generic.type	= MTYPE_ACTION;
	s_joinserver_address_book_action.generic.name	= "address book";
	s_joinserver_address_book_action.generic.x		= -56*scale;
	s_joinserver_address_book_action.generic.y		= 105*scale;
	s_joinserver_address_book_action.generic.cursor_offset = -16*scale;
	s_joinserver_address_book_action.generic.callback = AddressBookFunc;

	s_joinserver_search_action.generic.type = MTYPE_ACTION;
	s_joinserver_search_action.generic.name	= "refresh list";
	s_joinserver_search_action.generic.x	= -56*scale;
	s_joinserver_search_action.generic.y	= 115*scale;
	s_joinserver_search_action.generic.cursor_offset = -16*scale;
	s_joinserver_search_action.generic.callback = SearchLocalGamesFunc;
	s_joinserver_search_action.generic.statusbar = "search for servers";

	s_joinserver_filterempty_action.generic.type = MTYPE_SPINCONTROL;
	s_joinserver_filterempty_action.generic.name	= "show empty";
	s_joinserver_filterempty_action.itemnames = yesno_names;
	s_joinserver_filterempty_action.generic.x	= 80*scale;
	s_joinserver_filterempty_action.generic.y	= 40*scale;
	s_joinserver_filterempty_action.generic.cursor_offset = -16*scale;
	s_joinserver_filterempty_action.curvalue = m_show_empty;
	s_joinserver_filterempty_action.generic.callback = FilterEmptyFunc;
	
	s_joinserver_moveup.generic.type	= MTYPE_ACTION;
	s_joinserver_moveup.generic.name	= "       ";
	s_joinserver_moveup.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_moveup.generic.x		= -40*scale;
	s_joinserver_moveup.generic.y		= 190*scale;
	s_joinserver_moveup.generic.cursor_offset = -16*scale;
	s_joinserver_moveup.generic.callback = MoveUp;

	s_joinserver_movedown.generic.type	= MTYPE_ACTION;
	s_joinserver_movedown.generic.name	= "       ";
	s_joinserver_movedown.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_movedown.generic.x		= -40*scale;
	s_joinserver_movedown.generic.y		= 200*scale;
	s_joinserver_movedown.generic.cursor_offset = -16*scale;
	s_joinserver_movedown.generic.callback = MoveDown;

	s_playerlist_moveup.generic.type	= MTYPE_ACTION;
	s_playerlist_moveup.generic.name	= " ";
	s_playerlist_moveup.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerlist_moveup.generic.x		= 95*scale;
	s_playerlist_moveup.generic.y		= 340*scale;
	s_playerlist_moveup.generic.cursor_offset = -16*scale;
	s_playerlist_moveup.generic.callback = MoveUp_plist;

	s_playerlist_movedown.generic.type	= MTYPE_ACTION;
	s_playerlist_movedown.generic.name	= " ";
	s_playerlist_movedown.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerlist_movedown.generic.x		= 95*scale;
	s_playerlist_movedown.generic.y		= 350*scale;
	s_playerlist_movedown.generic.cursor_offset = -16*scale;
	s_playerlist_movedown.generic.callback = MoveDown_plist;

	Menu_AddItem( &s_joinserver_menu, &s_joinserver_address_book_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_search_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_filterempty_action );

	for ( i = 0; i < 16; i++ )
		Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_actions[i] );

	for ( i = 0; i < 8; i++ ) //same here
		Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_info[i] );

	for ( i = 0; i < 5; i++ )
		Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_data[i] );

	//add items to move the index
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_moveup );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_movedown );
	Menu_AddItem( &s_joinserver_menu, &s_playerlist_moveup );
	Menu_AddItem( &s_joinserver_menu, &s_playerlist_movedown );

	Menu_Center( &s_joinserver_menu );

	SearchLocalGames();
}

void JoinServer_MenuDraw(void)
{
	int i;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	M_Background( "m_options_back"); //draw black background first
	M_Banner( "m_joinserver" );


	for ( i = 0; i < 16; i++ )
	{
		s_joinserver_server_actions[i].generic.type	= MTYPE_ACTION;
		s_joinserver_server_actions[i].generic.name	= mservers[i+svridx].serverInfo;
		s_joinserver_server_actions[i].generic.x		= 372*scale;
		s_joinserver_server_actions[i].generic.y		= 90*scale + i*10*scale;
		s_joinserver_server_actions[i].generic.cursor_offset = -16*scale;
		s_joinserver_server_actions[i].generic.callback = JoinServerFunc;
		s_joinserver_server_actions[i].generic.statusbar = "press ENTER or DBL CLICK to connect";
	}

	//draw the server info here

	for ( i = 0; i < 8; i++)
	{
		s_joinserver_server_info[i].generic.type	= MTYPE_SEPARATOR;
		s_joinserver_server_info[i].generic.name	= local_server_info[i+playeridx];
		s_joinserver_server_info[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_joinserver_server_info[i].generic.x		= 350*scale;
		s_joinserver_server_info[i].generic.y		= 305*scale + i*10*scale;
	}

	for ( i = 0; i < 5; i++)
	{
		s_joinserver_server_data[i].generic.type	= MTYPE_SEPARATOR;
		s_joinserver_server_data[i].generic.name	= local_server_data[i];
		s_joinserver_server_data[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_joinserver_server_data[i].generic.x		= 30*scale;
		s_joinserver_server_data[i].generic.y		= 335*scale + i*10*scale;
	}
	M_ArrowPics();
	Menu_Draw( &s_joinserver_menu );
}

const char *JoinServer_MenuKey( int key )
{
	if ( key == K_ENTER )
	{
		cursor.buttonclicks[MOUSEBUTTON1] = 2;//so we can still join without a mouse
	}
	return Default_MenuKey( &s_joinserver_menu, key );
}

void M_Menu_JoinServer_f (void)
{
	JoinServer_MenuInit();
	M_PushMenu( JoinServer_MenuDraw, JoinServer_MenuKey );
}

/*
=============================================================================

MUTATORS MENU

=============================================================================
*/
static menuframework_s s_mutators_menu;
static menulist_s s_instagib_list;
static menulist_s s_rocketarena_list;
static menulist_s s_excessive_list;
static menulist_s s_vampire_list;
static menulist_s s_regen_list;
static menulist_s s_quickweaps_list;
static menulist_s s_anticamp_list;
static menufield_s s_camptime;
static menulist_s s_speed_list;
static menulist_s s_joust_list;
static menulist_s s_lowgrav_list;
static menulist_s s_classbased_list;

void InstagibFunc(void *self) {

	if(s_instagib_list.curvalue) {
		Cvar_SetValue ("instagib", 1);
		Cvar_SetValue ("rocket_arena", 0);
		s_rocketarena_list.curvalue = 0;
		Cvar_SetValue ("excessive", 0);
		s_excessive_list.curvalue = 0;
		Cvar_SetValue("classbased", 0);
		s_classbased_list.curvalue = 0;
	}
	else
		Cvar_SetValue ("instagib", 0);
}
void RocketFunc(void *self) {

	if(s_rocketarena_list.curvalue) {
		Cvar_SetValue ("rocket_arena", 1);
		Cvar_SetValue ("instagib", 0);
		s_instagib_list.curvalue = 0;
		Cvar_SetValue ("excessvie", 0);
		s_excessive_list.curvalue = 0;
		Cvar_SetValue("classbased", 0);
		s_classbased_list.curvalue = 0;
	}
	else
		Cvar_SetValue ("rocket_arena", 0);
}
void ExcessiveFunc(void *self) {

	if(s_excessive_list.curvalue) {
		Cvar_SetValue("excessive", 1);
		Cvar_SetValue("instagib", 0);
		s_instagib_list.curvalue = 0;
		Cvar_SetValue("rocket_arena", 0);
		s_rocketarena_list.curvalue = 0;
		Cvar_SetValue("classbased", 0);
		s_classbased_list.curvalue = 0;
	}
	else
		Cvar_SetValue("excessive", 0);
}
void ClassbasedFunc(void *self) {

	if(s_classbased_list.curvalue) {
		Cvar_SetValue("classbased", 1);
		s_excessive_list.curvalue = 0;
		Cvar_SetValue("excessive", 0);
		Cvar_SetValue("instagib", 0);
		s_instagib_list.curvalue = 0;
		Cvar_SetValue("rocket_arena", 0);
		s_rocketarena_list.curvalue = 0;
	}
	else
		Cvar_SetValue("classbased", 0);
}
void MutatorsFunc(void *self) {

	Cvar_SetValue("vampire", s_vampire_list.curvalue);

	Cvar_SetValue("regeneration", s_regen_list.curvalue);

	Cvar_SetValue("quickweap", s_quickweaps_list.curvalue);

	Cvar_SetValue("anticamp", s_anticamp_list.curvalue);

	Cvar_SetValue("sv_joustmode", s_joust_list.curvalue);

	Cvar_SetValue("low_grav", s_lowgrav_list.curvalue);

	Cvar_SetValue("playerspeed", s_speed_list.curvalue);

}

void SetMutatorsFunc( void *self) {
	//set the menu according to current cvar settings
	s_instagib_list.curvalue = Cvar_VariableValue("instagib");
	s_rocketarena_list.curvalue = Cvar_VariableValue("rocket_arena");
	s_excessive_list.curvalue = Cvar_VariableValue("excessive");
	s_vampire_list.curvalue = Cvar_VariableValue("vampire");
	s_regen_list.curvalue = Cvar_VariableValue("regeneration");
	s_quickweaps_list.curvalue = Cvar_VariableValue("quickweap");
	s_lowgrav_list.curvalue = Cvar_VariableValue("low_grav");
	s_anticamp_list.curvalue = Cvar_VariableValue("anticamp");
	strcpy( s_camptime.buffer, Cvar_VariableString("camptime") );
	s_joust_list.curvalue = Cvar_VariableValue("sv_joustmode");
	s_speed_list.curvalue = Cvar_VariableValue("playerspeed");
	s_classbased_list.curvalue = Cvar_VariableValue("classbased");
}
void Mutators_MenuInit( void )
{
	int offset;


	static const char *yn[] =
	{
		"no",
		"yes",
		0
	};
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	/*
	** initialize the menu stuff
	*/
	s_mutators_menu.x = viddef.width * 0.50;
	s_mutators_menu.nitems = 0;
	offset = viddef.height/2 - 40*scale;

	s_instagib_list.generic.type = MTYPE_SPINCONTROL;
	s_instagib_list.generic.x	= -8;
	s_instagib_list.generic.y	= 0 + offset;
	s_instagib_list.generic.name	= "instagib";
	s_instagib_list.generic.callback = InstagibFunc;
	s_instagib_list.itemnames = yn;
	s_instagib_list.curvalue = 0;

	s_rocketarena_list.generic.type = MTYPE_SPINCONTROL;
	s_rocketarena_list.generic.x	= -8;
	s_rocketarena_list.generic.y	= 10*scale + offset;
	s_rocketarena_list.generic.name	= "rocket arena";
	s_rocketarena_list.generic.callback = RocketFunc;
	s_rocketarena_list.itemnames = yn;
	s_rocketarena_list.curvalue = 0;

	s_excessive_list.generic.type = MTYPE_SPINCONTROL;
	s_excessive_list.generic.x	= -8;
	s_excessive_list.generic.y	= 20*scale + offset;
	s_excessive_list.generic.name	= "excessive";
	s_excessive_list.generic.callback = ExcessiveFunc;
	s_excessive_list.itemnames = yn;
	s_excessive_list.curvalue = 0;

	s_vampire_list.generic.type = MTYPE_SPINCONTROL;
	s_vampire_list.generic.x	= -8;
	s_vampire_list.generic.y	= 30*scale + offset;
	s_vampire_list.generic.name	= "vampire";
	s_vampire_list.generic.callback = MutatorsFunc;
	s_vampire_list.itemnames = yn;
	s_vampire_list.curvalue = 0;

	s_regen_list.generic.type = MTYPE_SPINCONTROL;
	s_regen_list.generic.x	= -8;
	s_regen_list.generic.y	= 40*scale + offset;
	s_regen_list.generic.name	= "regen";
	s_regen_list.generic.callback = MutatorsFunc;
	s_regen_list.itemnames = yn;
	s_regen_list.curvalue = 0;

	s_quickweaps_list.generic.type = MTYPE_SPINCONTROL;
	s_quickweaps_list.generic.x	= -8;
	s_quickweaps_list.generic.y	= 50*scale + offset;
	s_quickweaps_list.generic.name	= "quick weapons";
	s_quickweaps_list.generic.callback = MutatorsFunc;
	s_quickweaps_list.itemnames = yn;
	s_quickweaps_list.curvalue = 0;

	s_anticamp_list.generic.type = MTYPE_SPINCONTROL;
	s_anticamp_list.generic.x	= -8;
	s_anticamp_list.generic.y	= 60*scale + offset;
	s_anticamp_list.generic.name	= "anticamp";
	s_anticamp_list.generic.callback = MutatorsFunc;
	s_anticamp_list.itemnames = yn;
	s_anticamp_list.curvalue = 0;

	s_camptime.generic.type = MTYPE_FIELD;
	s_camptime.generic.name = "camp time ";
	s_camptime.generic.flags = QMF_NUMBERSONLY;
	s_camptime.generic.x	= 0;
	s_camptime.generic.y	= 76*scale + offset;
	s_camptime.length = 3;
	s_camptime.visible_length = 3;
	strcpy( s_camptime.buffer, Cvar_VariableString("camptime") );

	s_speed_list.generic.type = MTYPE_SPINCONTROL;
	s_speed_list.generic.x	= -8;
	s_speed_list.generic.y	= 92*scale + offset;
	s_speed_list.generic.name	= "speed";
	s_speed_list.generic.callback = MutatorsFunc;
	s_speed_list.itemnames = yn;
	s_speed_list.curvalue = 0;

	s_lowgrav_list.generic.type = MTYPE_SPINCONTROL;
	s_lowgrav_list.generic.x	= -8;
	s_lowgrav_list.generic.y	= 102*scale + offset;
	s_lowgrav_list.generic.name	= "low gravity";
	s_lowgrav_list.generic.callback = MutatorsFunc;
	s_lowgrav_list.itemnames = yn;
	s_lowgrav_list.curvalue = 0;

	s_joust_list.generic.type = MTYPE_SPINCONTROL;
	s_joust_list.generic.x	= -8;
	s_joust_list.generic.y	= 112*scale + offset;
	s_joust_list.generic.name	= "jousting";
	s_joust_list.generic.callback = MutatorsFunc;
	s_joust_list.itemnames = yn;
	s_joust_list.curvalue = 0;

	s_classbased_list.generic.type = MTYPE_SPINCONTROL;
	s_classbased_list.generic.x	= -8;
	s_classbased_list.generic.y	= 122*scale + offset;
	s_classbased_list.generic.name	= "classbased";
	s_classbased_list.generic.callback = ClassbasedFunc;
	s_classbased_list.itemnames = yn;
	s_classbased_list.curvalue = 0;

	Menu_AddItem( &s_mutators_menu, &s_instagib_list );
	Menu_AddItem( &s_mutators_menu, &s_rocketarena_list );
	Menu_AddItem( &s_mutators_menu, &s_excessive_list );
	Menu_AddItem( &s_mutators_menu, &s_vampire_list );
	Menu_AddItem( &s_mutators_menu, &s_regen_list );
	Menu_AddItem( &s_mutators_menu, &s_quickweaps_list );
	Menu_AddItem( &s_mutators_menu, &s_anticamp_list );
	Menu_AddItem( &s_mutators_menu, &s_camptime );
	Menu_AddItem( &s_mutators_menu, &s_speed_list );
	Menu_AddItem( &s_mutators_menu, &s_lowgrav_list );
	Menu_AddItem( &s_mutators_menu, &s_joust_list );
	Menu_AddItem( &s_mutators_menu, &s_classbased_list );

	// call this now to set proper inital state
	SetMutatorsFunc ( NULL );
}
void Mutators_MenuDraw(void)
{
	M_Background( "m_options_back"); //draw black background first
	M_Banner( "m_mutators" );

	Menu_Draw( &s_mutators_menu );
}
const char *Mutators_MenuKey( int key )
{

	Cvar_SetValue("camptime", atoi(s_camptime.buffer));

	if ( key == K_ENTER )
	{
		cursor.buttonclicks[MOUSEBUTTON1] = 2;
	}
	return Default_MenuKey( &s_mutators_menu, key );
}
void M_Menu_Mutators_f (void)
{
	Mutators_MenuInit();
	M_PushMenu( Mutators_MenuDraw, Mutators_MenuKey );
}
/*
=============================================================================

ADD BOTS MENU

=============================================================================
*/
static menuframework_s	s_addbots_menu;
static menuaction_s		s_addbots_bot_action[16];
static menulist_s	s_startmap_list;
static menulist_s	s_rules_box;
static menulist_s   s_bots_bot_action[8];
static char **mapnames;
int totalbots;

struct botdata {
	char name[32];
	char model[64];
	char userinfo[MAX_INFO_STRING];
} bots[16];

struct botinfo {
	char name[32];
	char userinfo[MAX_INFO_STRING];
} bot[8];

int slot;

void LoadBotInfo() {
	FILE *pIn;
	int i, count;
	char *info;
	char *skin;

	if((pIn = fopen("botinfo/allbots.tmp", "rb" )) == NULL)
		return; // bail

	fread(&count,sizeof (int),1,pIn);
	if(count>16)
		count = 16;

	for(i=0;i<count;i++)
	{

		fread(bots[i].userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn);

		info = Info_ValueForKey (bots[i].userinfo, "name");
		skin = Info_ValueForKey (bots[i].userinfo, "skin");
		strcpy(bots[i].name, info);
		sprintf(bots[i].model, "bots/%s_i", skin);
	}
	totalbots = count;
    fclose(pIn);
}

void AddbotFunc(void *self) {
	int i, count;
	char startmap[128];
	char bot_filename[128];
	FILE *pOut;
	menulist_s *f = ( menulist_s * ) self;

	//get the name and copy that config string into the proper slot name
	for(i = 0; i < totalbots; i++) {
		if(!strcmp(f->generic.name, bots[i].name)) { //this is our selected bot
			strcpy(bot[slot].name, bots[i].name);
			strcpy(bot[slot].userinfo, bots[i].userinfo);
			s_bots_bot_action[slot].generic.name = bots[i].name;
		}
	}

	//save off bot file
	count = 8;
	for(i = 0; i < 8; i++) {
		if(!strcmp(bot[i].name, "...empty slot"))
			count--;
	}
	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	for(i = 0; i < strlen(startmap); i++)
		startmap[i] = tolower(startmap[i]);

	if(s_rules_box.curvalue == 1 || s_rules_box.curvalue == 4 || s_rules_box.curvalue == 5)
		strcpy(bot_filename, "botinfo/team.tmp");
	else
		sprintf(bot_filename, "botinfo/%s.tmp", startmap);
	if((pOut = fopen(bot_filename, "wb" )) == NULL)
		return; // bail

	fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	for (i = 7; i > -1; i--) {
		if(strcmp(bot[i].name, "...empty slot"))
			fwrite(bot[i].userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut);
	}

    fclose(pOut);

	//kick back to previous menu
	M_PopMenu();

}
void Addbots_MenuInit( void )
{
	int i;
	int y;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	totalbots = 0;

	LoadBotInfo();

	s_addbots_menu.x = viddef.width * 0.50 - 50*scale;
	s_addbots_menu.nitems = 0;
	y = viddef.height/2 - 140*scale;

	for(i = 0; i < totalbots; i++) {
		s_addbots_bot_action[i].generic.type	= MTYPE_ACTION;
		s_addbots_bot_action[i].generic.name	= bots[i].name;
		s_addbots_bot_action[i].generic.x		= 64;
		s_addbots_bot_action[i].generic.y		= y+=20*scale;
		s_addbots_bot_action[i].generic.cursor_offset = -16*scale;
		s_addbots_bot_action[i].generic.callback = AddbotFunc;

		Menu_AddItem( &s_addbots_menu, &s_addbots_bot_action[i] );
	}


}

void Addbots_MenuDraw(void)
{
	int i;
	int y;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	M_Background( "m_player_back"); //draw black background first
	M_Banner( "m_bots" );

	y = viddef.height/2 - 122*scale;

	//draw the pics for the bots here
	for(i = 0; i < totalbots; i++) {
		Draw_StretchPic (viddef.width / 2 + 16*scale, y, 16*scale, 16*scale, bots[i].model);
		y+=20*scale;
	}
	Menu_Draw( &s_addbots_menu );
}
const char *Addbots_MenuKey( int key )
{
	if ( key == K_ENTER )
	{
		cursor.buttonclicks[MOUSEBUTTON1] = 2;
	}
	return Default_MenuKey( &s_addbots_menu, key );
}
/*
=============================================================================

START SERVER MENU

=============================================================================
*/
#define MAX_MAPS 256

static menuframework_s s_startserver_menu;
static int	  nummaps;

static menuaction_s	s_startserver_start_action;
static menuaction_s	s_startserver_dmoptions_action;
static menufield_s	s_timelimit_field;
static menufield_s	s_fraglimit_field;
static menufield_s	s_maxclients_field;
static menufield_s	s_hostname_field;
static menufield_s  s_mutators_action;
static menulist_s   s_grapple_box;
static menulist_s   s_public_box;
static menulist_s	s_dedicated_box;
static menulist_s   s_skill_box;
static menulist_s   s_startserver_map_data[5];

void DMOptionsFunc( void *self )
{
	M_Menu_DMOptions_f();
}

void MutatorFunc( void *self )
{
	M_Menu_Mutators_f();
}
int Menu_FindFile (char *filename, FILE **file)
{
	*file = fopen (filename, "rb");
	if (!*file) {
		*file = NULL;
		return -1;
	}
	else
		return 1;

}

void MapInfoFunc( void *self ) {

	FILE *map_file;
	FILE *desc_file;
	char line[500];
	char *pLine;
	char *rLine;
	int result;
	int i;
	char seps[]   = "//";
	char *token;
	char startmap[128];
	char path[1024];
	int offset = 65;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	offset*=scale;

	//get a map description if it is there

	if(mapnames)
		strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	else
		strcpy( startmap, "missing");

	sprintf(path, "%s/levelshots/%s.txt", FS_Gamedir(), startmap);
	Menu_FindFile(path, &desc_file);
	if(desc_file)
		fclose(desc_file);
	else 
		sprintf(path, "%s/levelshots/%s.txt", BASEDIRNAME, startmap);

	if ((map_file = fopen(path, "rb")) != NULL)
	{
		if(fgets(line, 500, map_file))
		{
			pLine = line;

			result = strlen(line);

			rLine = GetLine (&pLine, &result);

			/* Establish string and get the first token: */
			token = strtok( rLine, seps );
			i = 0;
			while( token != NULL && i < 5) {

				/* Get next token: */
				token = strtok( NULL, seps );
				/* While there are tokens in "string" */
				s_startserver_map_data[i].generic.type	= MTYPE_SEPARATOR;
				s_startserver_map_data[i].generic.name	= token;
				s_startserver_map_data[i].generic.flags	= QMF_LEFT_JUSTIFY;
				s_startserver_map_data[i].generic.x		= 180*scale;
				s_startserver_map_data[i].generic.y		= 241*scale + offset + i*10*scale;

				i++;
			}

		}

		fclose(map_file);

	}
	else
	{
		for (i = 0; i < 5; i++ )
		{
			s_startserver_map_data[i].generic.type	= MTYPE_SEPARATOR;
			s_startserver_map_data[i].generic.name	= "no data";
			s_startserver_map_data[i].generic.flags	= QMF_LEFT_JUSTIFY;
			s_startserver_map_data[i].generic.x		= 180*scale;
			s_startserver_map_data[i].generic.y		= 241*scale + offset + i*10*scale;
		}
	}
}

void RulesChangeFunc ( void *self ) //this has been expanded to rebuild map list
{
	char *buffer;
	char  mapsname[1024];
	char *s;
	int length;
	int i, k;
	FILE *fp;
	char  shortname[MAX_TOKEN_CHARS];
    char  longname[MAX_TOKEN_CHARS];
	char  scratch[200];
	char *curMap;
	int nmaps = 0;
	int totalmaps;
	char **mapfiles;
	char *path = NULL;
	static char **bspnames;
	extern char **FS_ListFiles( char *, int *, unsigned, unsigned );
	int		j, l;

	s_maxclients_field.generic.statusbar = NULL;
	s_startserver_dmoptions_action.generic.statusbar = NULL;

	//clear out list first

	mapnames = 0;
	nummaps = 0;

	/*
	** reload the list of map names, based on rules
	*/
	Com_sprintf( mapsname, sizeof( mapsname ), "%s/maps.lst", FS_Gamedir() );
	if ( ( fp = fopen( mapsname, "rb" ) ) == 0 )
	{
		if ( ( length = FS_LoadFile( "maps.lst", ( void ** ) &buffer ) ) == -1 )
			Com_Error( ERR_DROP, "couldn't find maps.lst\n" );
	}
	else
	{
#ifdef _WIN32
		length = filelength( fileno( fp  ) );
#else
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
#endif
		buffer = malloc( length );
		fread( buffer, length, 1, fp );
	}

	s = buffer;

	i = 0;
	while ( i < length )
	{
		if ( s[i] == '\r' )
			nummaps++;
		i++;
	}
	totalmaps = nummaps;

	if ( nummaps == 0 )
		Com_Error( ERR_DROP, "no maps in maps.lst\n" );

	mapnames = malloc( sizeof( char * ) * ( MAX_MAPS + 2 ) );  //was + 1, but caused memory errors
	memset( mapnames, 0, sizeof( char * ) * ( MAX_MAPS + 2 ) );

	bspnames = malloc( sizeof( char * ) * ( MAX_MAPS + 2 ) );  //was + 1, but caused memory errors
	memset( bspnames, 0, sizeof( char * ) * ( MAX_MAPS + 2 ) );

	s = buffer;

	k = 0;
	for ( i = 0; i < nummaps; i++ )
	{

		strcpy( shortname, COM_Parse( &s ) );
		l = strlen(shortname);

		for (j=0 ; j<l ; j++)
			shortname[j] = tolower(shortname[j]);

		//keep a list of the shortnames for later comparison to bsp files
		bspnames[i] = malloc( strlen( shortname ) + 1 );
		strcpy(bspnames[i], shortname);

		strcpy( longname, COM_Parse( &s ) );
		Com_sprintf( scratch, sizeof( scratch ), "%s\n%s", longname, shortname );

		if (s_rules_box.curvalue == 0 || s_rules_box.curvalue == 6) {
			if((shortname[0] == 'd' && shortname[1] == 'm') || (shortname[0] == 't' && shortname[1] == 'o')) {
				mapnames[k] = malloc( strlen( scratch ) + 1 );
				strcpy( mapnames[k], scratch );
				k++;
			}
		}
		else if (s_rules_box.curvalue == 1) {
			if(shortname[0] == 'c' && shortname[1] == 't' && shortname[2] == 'f') {
				mapnames[k] = malloc( strlen( scratch ) + 1 );
				strcpy( mapnames[k], scratch );
				k++;
			}
		}
		else if (s_rules_box.curvalue == 2) {
			if(shortname[0] == 'a' && shortname[1] == 'o' && shortname[2] == 'a') {
				mapnames[k] = malloc( strlen( scratch ) + 1 );
				strcpy( mapnames[k], scratch );
				k++;
			}
		}
		else if (s_rules_box.curvalue == 3) {
			if(shortname[0] == 'd' && shortname[1] == 'b') {
				mapnames[k] = malloc( strlen( scratch ) + 1 );
				strcpy( mapnames[k], scratch );
				k++;
			}
		}
		else if (s_rules_box.curvalue == 4) {
			if(shortname[0] == 't' && shortname[1] == 'c' && shortname[2] == 'a') {
				mapnames[k] = malloc( strlen( scratch ) + 1 );
				strcpy( mapnames[k], scratch );
				k++;
			}
		}
		else if (s_rules_box.curvalue == 5) {
			if(shortname[0] == 'c' && shortname[1] == 'p') {
				mapnames[k] = malloc( strlen( scratch ) + 1 );
				strcpy( mapnames[k], scratch );
				k++;
			}
		}

	}

	if ( fp != 0 )
	{
		fp = 0;
		free( buffer );
	}
	else
	{
		FS_FreeFile( buffer );
	}

	//now, check the folders and add the maps not in the list yet
	path = FS_NextPath( path );
	while (path)
	{
		Com_sprintf( mapsname, sizeof(mapsname), "%s/maps/*.bsp", path );
		mapfiles = FS_ListFiles( mapsname, &nmaps, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		for (i=0;i<nmaps && totalmaps<MAX_MAPS;i++)
		{
			int num;

			if (!mapfiles || !mapfiles[i])
				continue;

			s = strstr(mapfiles[i], "/maps/"); s++;
			s = strstr(s, "/"); s++;

			if (!strstr(s, ".bsp"))
				continue;

			num = strlen(s)-4;
			s[num] = 0;

			curMap = s;

			l = strlen(curMap);

			for (j=0 ; j<l ; j++)
				curMap[j] = tolower(curMap[j]);

			Com_sprintf( scratch, sizeof( scratch ), "%s\n%s", "Custom Map", curMap );

			//check game type, and if not already in maps.lst, add it
			l = 0;
			for(j = 0; j < nummaps; j++) {
				l = Q_strcasecmp(curMap, bspnames[j]);
				if(!l)
					break; //already there, don't bother adding
			}
			if(l) { //didn't find it in our list
				if (s_rules_box.curvalue == 0) {
					if((curMap[0] == 'd' && curMap[1] == 'm') || (curMap[0] == 't' && curMap[1] == 'o')) {
						mapnames[k] = malloc( strlen( scratch ) + 1 );
						strcpy( mapnames[k], scratch );
						k++;
						totalmaps++;
					}
				}
				else if (s_rules_box.curvalue == 1) {
					if(curMap[0] == 'c' && curMap[1] == 't' && curMap[2] == 'f') {
						mapnames[k] = malloc( strlen( scratch ) + 1 );
						strcpy( mapnames[k], scratch );
						k++;
						totalmaps++;
					}
				}
				else if (s_rules_box.curvalue == 2) {
					if(curMap[0] == 'a' && curMap[1] == 'o' && curMap[2] == 'a') {
						mapnames[k] = malloc( strlen( scratch ) + 1 );
						strcpy( mapnames[k], scratch );
						k++;
						totalmaps++;
					}
				}
				else if (s_rules_box.curvalue == 3) {
					if(curMap[0] == 'd' && curMap[1] == 'b') {
						mapnames[k] = malloc( strlen( scratch ) + 1 );
						strcpy( mapnames[k], scratch );
						k++;
						totalmaps++;
					}
				}
				else if (s_rules_box.curvalue == 4) {
					if(curMap[0] == 't' && curMap[1] == 'c' && curMap[2] == 'a') {
						mapnames[k] = malloc( strlen( scratch ) + 1 );
						strcpy( mapnames[k], scratch );
						k++;
						totalmaps++;
					}
				}
				else if (s_rules_box.curvalue == 5) {
					if(curMap[0] == 'c' && curMap[1] == 'p') {
						mapnames[k] = malloc( strlen( scratch ) + 1 );
						strcpy( mapnames[k], scratch );
						k++;
						totalmaps++;
					}
				}

			}
			//set back so whole string get deleted.
			s[num] = '.';
		}
		path = FS_NextPath( path );
	}

	if (mapfiles)
		free( mapfiles );

	for(i = k; i<=nummaps; i++) {
		free(mapnames[i]);
		mapnames[i] = 0;
	}

	s_startmap_list.generic.name	= "initial map";
	s_startmap_list.itemnames = mapnames;
	s_startmap_list.curvalue = 0;

	//set map info
	MapInfoFunc(NULL);
}

void StartServerActionFunc( void *self )
{
	char	startmap[128];
	int		timelimit;
	int		fraglimit;
	int		maxclients;
	char	*spot;

	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );

	maxclients  = atoi( s_maxclients_field.buffer );
	timelimit	= atoi( s_timelimit_field.buffer );
	fraglimit	= atoi( s_fraglimit_field.buffer );

	Cvar_SetValue( "maxclients", ClampCvar( 0, maxclients, maxclients ) );
	Cvar_SetValue ("timelimit", ClampCvar( 0, timelimit, timelimit ) );
	Cvar_SetValue ("fraglimit", ClampCvar( 0, fraglimit, fraglimit ) );
	Cvar_Set("hostname", s_hostname_field.buffer );
	Cvar_SetValue("sv_public", s_public_box.curvalue );
	if(s_dedicated_box.curvalue) {
		Cvar_ForceSet("dedicated", "1");
		Cvar_Set("sv_maplist", startmap);
	}
	Cvar_SetValue( "skill", s_skill_box.curvalue );
	Cvar_SetValue( "grapple", s_grapple_box.curvalue);

//PGM
	if(s_rules_box.curvalue == 0)
	{
		Cvar_SetValue ("deathmatch", 1 );
		Cvar_SetValue ("ctf", 0);
		Cvar_SetValue ("tca", 0);
		Cvar_SetValue ("cp", 0);
		Cvar_SetValue ("g_duel", 0);
		Cvar_SetValue ("gamerules", s_rules_box.curvalue );
	}
	else if(s_rules_box.curvalue == 1)
	{
		Cvar_SetValue ("deathmatch", 1 );	// deathmatch is always true for ctf, right?
		Cvar_SetValue ("ctf", 1 ); //set both dm and ctf
		Cvar_SetValue ("tca", 0);
		Cvar_SetValue ("cp", 0);
		Cvar_SetValue ("g_duel", 0);
		Cvar_SetValue ("gamerules", s_rules_box.curvalue );
	}
	else if(s_rules_box.curvalue == 2) //aoa mode
	{
		Cvar_SetValue ("deathmatch", 1 );	// deathmatch is always true for aoa.
		Cvar_SetValue ("ctf", 0 );
		Cvar_SetValue ("tca", 0);
		Cvar_SetValue ("cp", 0);
		Cvar_SetValue ("g_duel", 0);
		Cvar_SetValue ("gamerules", s_rules_box.curvalue );
	}
	else if(s_rules_box.curvalue == 3) //deathball mode
	{
		Cvar_SetValue ("deathmatch", 1 );	// deathmatch is always true for deathball.
		Cvar_SetValue ("ctf", 0 );
		Cvar_SetValue ("tca", 0);
		Cvar_SetValue ("cp", 0);
		Cvar_SetValue ("g_duel", 0);
		Cvar_SetValue ("gamerules", s_rules_box.curvalue );
	}
	else if(s_rules_box.curvalue == 4) //tca mode
	{
		Cvar_SetValue ("deathmatch", 1 );	// deathmatch is always true for tca.
		Cvar_SetValue ("ctf", 0 );
		Cvar_SetValue ("tca", 1);
		Cvar_SetValue ("cp", 0);
		Cvar_SetValue ("g_duel", 0);
		Cvar_SetValue ("gamerules", s_rules_box.curvalue );
	}
	else if(s_rules_box.curvalue == 5) //cattleprod mode
	{
		Cvar_SetValue ("deathmatch", 1 );	// deathmatch is always true for cp.
		Cvar_SetValue ("ctf", 0 );
		Cvar_SetValue ("tca", 0);
		Cvar_SetValue ("cp", 1);
		Cvar_SetValue ("g_duel", 0);
		Cvar_SetValue ("gamerules", s_rules_box.curvalue );
	}
	else if(s_rules_box.curvalue == 6) //duel mode
	{
		Cvar_SetValue ("deathmatch", 1 );	// deathmatch is always true for cp.
		Cvar_SetValue ("ctf", 0 );
		Cvar_SetValue ("tca", 0);
		Cvar_SetValue ("cp", 0);
		Cvar_SetValue ("g_duel", 1);
		Cvar_SetValue ("gamerules", s_rules_box.curvalue );
	}

	spot = NULL;
	if (s_rules_box.curvalue == 0)		// PGM
	{
 		if(Q_stricmp(startmap, "dm-dynamo") == 0)
  			spot = "start";

	}
	if (s_rules_box.curvalue == 1)		// PGM
	{
 		if(Q_stricmp(startmap, "ctf-chromium") == 0)
  			spot = "start";

	}
	if (s_rules_box.curvalue == 2)
	{
		if(Q_stricmp(startmap, "aoa-morpheus") == 0)
			spot = "start";
	}
	if (s_rules_box.curvalue == 3)
	{
		if(Q_stricmp(startmap, "db-chromium") == 0)
			spot = "start";
	}
	if (s_rules_box.curvalue == 4)
	{
		if(Q_stricmp(startmap, "tca-europa") == 0)
			spot = "start";
	}
	if (s_rules_box.curvalue == 5)
	{
		if(Q_stricmp(startmap, "cp-grindery") == 0)
			spot = "start";
	}
	if (spot)
	{
		if (Com_ServerState())
			Cbuf_AddText ("disconnect\n");
		Cbuf_AddText (va("startmap %s\n", startmap));
	}
	else
	{
		Cbuf_AddText (va("startmap %s\n", startmap));
	}

	M_ForceMenuOff ();

}

void StartServer_MenuInit( void )
{
	int i;
	int offset;

	static const char *dm_coop_names[] =
	{
		"deathmatch",
		"ctf",
		"all out assault",
		"deathball",
		"team core assault",
		"cattle prod",
		"duel",
		0
	};

	static const char *public_yn[] =
	{
		"no",
		"yes",
		0
	};

	static const char *skill[] =
	{
		"easy",
		"medium",
		"hard",
		0
	};
	static const char *grapple[] =
	{
		"off",
		"on",
		0
	};
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	//moved the map stuff to where rules are set, so we can filter the list

	/*
	** initialize the menu stuff
	*/
	s_startserver_menu.x = viddef.width * 0.50;
	s_startserver_menu.nitems = 0;
	offset = 65*scale;

	s_startmap_list.generic.type = MTYPE_SPINCONTROL;
	s_startmap_list.generic.x	= -8;
	s_startmap_list.generic.y	= 0 + offset;
	s_startmap_list.generic.name	= "initial map";
	s_startmap_list.itemnames = mapnames;
	s_startmap_list.generic.callback = MapInfoFunc;

	s_rules_box.generic.type = MTYPE_SPINCONTROL;
	s_rules_box.generic.x	= -8;
	s_rules_box.generic.y	= 20*scale + offset;
	s_rules_box.generic.name	= "rules";
	s_rules_box.itemnames = dm_coop_names;
	s_rules_box.curvalue = 0;
	s_rules_box.generic.callback = RulesChangeFunc;

	s_mutators_action.generic.type = MTYPE_ACTION;
	s_mutators_action.generic.x	= 112;
	s_mutators_action.generic.y	= 36*scale + offset;
	s_mutators_action.generic.cursor_offset = -8;
	s_mutators_action.generic.statusbar = NULL;
	s_mutators_action.generic.name	= " mutators";
	s_mutators_action.generic.callback = MutatorFunc;

	s_grapple_box.generic.type = MTYPE_SPINCONTROL;
	s_grapple_box.generic.x	= -8;
	s_grapple_box.generic.y	= 52*scale + offset;
	s_grapple_box.generic.name	= "grapple hook";
	s_grapple_box.itemnames = grapple;
	s_grapple_box.curvalue = 0;

	s_timelimit_field.generic.type = MTYPE_FIELD;
	s_timelimit_field.generic.name = "time limit ";
	s_timelimit_field.generic.flags = QMF_NUMBERSONLY;
	s_timelimit_field.generic.x	= 0;
	s_timelimit_field.generic.y	= 68*scale + offset;
	s_timelimit_field.generic.statusbar = "0 = no limit";
	s_timelimit_field.length = 3;
	s_timelimit_field.visible_length = 3;
	strcpy( s_timelimit_field.buffer, Cvar_VariableString("timelimit") );

	s_fraglimit_field.generic.type = MTYPE_FIELD;
	s_fraglimit_field.generic.name = "frag limit ";
	s_fraglimit_field.generic.flags = QMF_NUMBERSONLY;
	s_fraglimit_field.generic.x	= 0;
	s_fraglimit_field.generic.y	= 86*scale + offset;
	s_fraglimit_field.generic.statusbar = "0 = no limit";
	s_fraglimit_field.length = 3;
	s_fraglimit_field.visible_length = 3;
	strcpy( s_fraglimit_field.buffer, Cvar_VariableString("fraglimit") );

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is.
	** Clamping will be done when the server is actually started.
	*/
	s_maxclients_field.generic.type = MTYPE_FIELD;
	s_maxclients_field.generic.name = "max players ";
	s_maxclients_field.generic.flags = QMF_NUMBERSONLY;
	s_maxclients_field.generic.x	= 0;
	s_maxclients_field.generic.y	= 104*scale + offset;
	s_maxclients_field.generic.statusbar = NULL;
	s_maxclients_field.length = 3;
	s_maxclients_field.visible_length = 3;
	if ( Cvar_VariableValue( "maxclients" ) == 1 )
		strcpy( s_maxclients_field.buffer, "8" );
	else
		strcpy( s_maxclients_field.buffer, Cvar_VariableString("maxclients") );

	s_hostname_field.generic.type = MTYPE_FIELD;
	s_hostname_field.generic.name = "hostname ";
	s_hostname_field.generic.flags = 0;
	s_hostname_field.generic.x	= 0;
	s_hostname_field.generic.y	= 122*scale + offset;
	s_hostname_field.generic.statusbar = NULL;
	s_hostname_field.length = 12;
	s_hostname_field.visible_length = 12;
	strcpy( s_hostname_field.buffer, Cvar_VariableString("hostname") );

	s_public_box.generic.type = MTYPE_SPINCONTROL;
	s_public_box.generic.x	= -8;
	s_public_box.generic.y	= 140*scale + offset;
	s_public_box.generic.name = "public server";
	s_public_box.itemnames = public_yn;
	s_public_box.curvalue = 1;

	s_dedicated_box.generic.type = MTYPE_SPINCONTROL;
	s_dedicated_box.generic.x	= -8;
	s_dedicated_box.generic.y	= 150*scale + offset;
	s_dedicated_box.generic.name = "dedicated server";
	s_dedicated_box.itemnames = public_yn;
	s_dedicated_box.curvalue = 0;

	s_skill_box.generic.type = MTYPE_SPINCONTROL;
	s_skill_box.generic.x	= -8;
	s_skill_box.generic.y	= 160*scale + offset;
	s_skill_box.generic.name	= "skill level";
	s_skill_box.itemnames = skill;
	s_skill_box.curvalue = 1;

	s_startserver_dmoptions_action.generic.type = MTYPE_ACTION;
	s_startserver_dmoptions_action.generic.name	= " deathmatch and bot flags";
	s_startserver_dmoptions_action.generic.x	= 212*scale;
	s_startserver_dmoptions_action.generic.y	= 182*scale + offset;
	s_startserver_dmoptions_action.generic.cursor_offset = -8;
	s_startserver_dmoptions_action.generic.statusbar = NULL;
	s_startserver_dmoptions_action.generic.callback = DMOptionsFunc;

	s_startserver_start_action.generic.type = MTYPE_ACTION;
	s_startserver_start_action.generic.name	= " begin";
	s_startserver_start_action.generic.x	= 64*scale;
	s_startserver_start_action.generic.y	= 200*scale + offset;
	s_startserver_start_action.generic.cursor_offset = -8;
	s_startserver_start_action.generic.callback = StartServerActionFunc;

	for ( i = 0; i < 5; i++) { //initialize it
		s_startserver_map_data[i].generic.type	= MTYPE_SEPARATOR;
		s_startserver_map_data[i].generic.name	= "no data";
		s_startserver_map_data[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_startserver_map_data[i].generic.x		= 180*scale;
		s_startserver_map_data[i].generic.y		= 241*scale + offset + i*10*scale;
	}

	Menu_AddItem( &s_startserver_menu, &s_startmap_list );
	Menu_AddItem( &s_startserver_menu, &s_rules_box );
	Menu_AddItem( &s_startserver_menu, &s_mutators_action );
	Menu_AddItem( &s_startserver_menu, &s_grapple_box );
	Menu_AddItem( &s_startserver_menu, &s_timelimit_field );
	Menu_AddItem( &s_startserver_menu, &s_fraglimit_field );
	Menu_AddItem( &s_startserver_menu, &s_maxclients_field );
	Menu_AddItem( &s_startserver_menu, &s_hostname_field );
	Menu_AddItem( &s_startserver_menu, &s_public_box );
	Menu_AddItem( &s_startserver_menu, &s_dedicated_box );
	Menu_AddItem( &s_startserver_menu, &s_skill_box );
	Menu_AddItem( &s_startserver_menu, &s_startserver_dmoptions_action );
	Menu_AddItem( &s_startserver_menu, &s_startserver_start_action );
	for ( i = 0; i < 5; i++ )
		Menu_AddItem( &s_startserver_menu, &s_startserver_map_data[i] );
	Menu_Center( &s_startserver_menu );

	// call this now to set proper inital state
	RulesChangeFunc ( NULL );
	MapInfoFunc(NULL);
}

void StartServer_MenuDraw(void)
{
	char startmap[128];
	char path[1024];
	int offset = 65;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	offset*=scale;

	M_Background( "m_startserver_back"); //draw black background first
	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	sprintf(path, "/levelshots/%s", startmap);
	M_Banner( "m_startserver" );
	M_MapPic(path);

	Menu_Draw( &s_startserver_menu );
}

const char *StartServer_MenuKey( int key )
{
	if ( key == K_ESCAPE )
	{
		if ( mapnames )
		{
			int i;

			for ( i = 0; i < nummaps; i++ )
				free( mapnames[i] );
			free( mapnames );
		}
		mapnames = 0;
		nummaps = 0;
	}

	return Default_MenuKey( &s_startserver_menu, key );
}

void M_Menu_StartServer_f (void)
{
	StartServer_MenuInit();
	M_PushMenu( StartServer_MenuDraw, StartServer_MenuKey );
}

/*
=============================================================================

DMOPTIONS BOOK MENU

=============================================================================
*/
static char dmoptions_statusbar[128];

static menuframework_s s_dmoptions_menu;

static menulist_s	s_friendlyfire_box;
static menulist_s	s_botchat_box;
static menulist_s	s_bot_fuzzyaim_box;
static menulist_s	s_bot_auto_save_nodes_box;
static menulist_s	s_bots_box;
static menulist_s	s_bot_levelad_box;
static menulist_s	s_falls_box;
static menulist_s	s_weapons_stay_box;
static menulist_s	s_instant_powerups_box;
static menulist_s	s_powerups_box;
static menulist_s	s_health_box;
static menulist_s	s_spawn_farthest_box;
static menulist_s	s_teamplay_box;
static menulist_s	s_samelevel_box;
static menulist_s	s_force_respawn_box;
static menulist_s	s_armor_box;
static menulist_s	s_allow_exit_box;
static menulist_s	s_infinite_ammo_box;
static menulist_s	s_fixed_fov_box;
static menulist_s	s_quad_drop_box;

void M_Menu_Addbots_f (void)
{
	Addbots_MenuInit();
	M_PushMenu( Addbots_MenuDraw, Addbots_MenuKey );
}

void BotAction( void *self )
{
	FILE *pOut;
	int i, count;
	char startmap[128];
	char bot_filename[128];
	menulist_s *f = ( menulist_s * ) self;

	slot = f->curvalue;

	count = 8;

	if(!strcmp(f->generic.name, "...empty slot")) {
		//open the bot menu
		M_Menu_Addbots_f();
		for(i = 0; i < 8; i++) {
			if(!strcmp(s_bots_bot_action[i].generic.name, "...empty slot")) {
				//clear it, it's slot is empty
				strcpy(bot[i].name, "...empty slot");
				bot[i].userinfo[0] = 0;
				count--;
			}
		}
	}
	else {
		f->generic.name = "...empty slot";
		//clear the bot out of the struct...hmmm...kinda hokey, but - need to know which slot
		for(i = 0; i < 8; i++) {
			if(!strcmp(s_bots_bot_action[i].generic.name, "...empty slot")) {
				//clear it, it's slot is empty
				strcpy(bot[i].name, "...empty slot");
				bot[i].userinfo[0] = 0;
				count--;
			}
		}
	}

	//write out bot file

	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	for(i = 0; i < strlen(startmap); i++)
		startmap[i] = tolower(startmap[i]);
	if(s_rules_box.curvalue == 1 || s_rules_box.curvalue == 4 || s_rules_box.curvalue == 5)
		strcpy(bot_filename, "botinfo/team.tmp");
	else
		sprintf(bot_filename, "botinfo/%s.tmp", startmap);

	if((pOut = fopen(bot_filename, "wb" )) == NULL)
		return; // bail

	fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	for (i = 7; i > -1; i--) {
		if(strcmp(bot[i].name, "...empty slot"))
			fwrite(bot[i].userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut);
	}

    fclose(pOut);

	return;
}

static void DMFlagCallback( void *self )
{
	menulist_s *f = ( menulist_s * ) self;
	int flags;
	int bit = 0;

	flags = Cvar_VariableValue( "dmflags" );

	if ( f == &s_bots_box )
	{
		if ( f->curvalue )
			flags &= ~DF_BOTS;
		else
			flags |= DF_BOTS;
		goto setvalue;
	}
	if ( f == &s_bot_auto_save_nodes_box )
	{
		if ( f->curvalue )
			flags &= ~DF_BOT_AUTOSAVENODES;
		else
			flags |= DF_BOT_AUTOSAVENODES;
		goto setvalue;
	}
	if ( f == &s_bot_fuzzyaim_box )
	{
		if ( f->curvalue )
			flags &= ~DF_BOT_FUZZYAIM;
		else
			flags |= DF_BOT_FUZZYAIM;
		goto setvalue;
	}
	if ( f == &s_botchat_box )
	{
		if ( f->curvalue )
			flags &= ~DF_BOTCHAT;
		else
			flags |= DF_BOTCHAT;
		goto setvalue;
	}
	if ( f == &s_bot_levelad_box )
	{
		if ( f->curvalue )
			flags &= ~DF_BOT_LEVELAD;
		else
			flags |= DF_BOT_LEVELAD;
		goto setvalue;
	}
	if ( f == &s_friendlyfire_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_FRIENDLY_FIRE;
		else
			flags |= DF_NO_FRIENDLY_FIRE;
		goto setvalue;
	}
	else if ( f == &s_falls_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_FALLING;
		else
			flags |= DF_NO_FALLING;
		goto setvalue;
	}
	else if ( f == &s_weapons_stay_box )
	{
		bit = DF_WEAPONS_STAY;
	}
	else if ( f == &s_instant_powerups_box )
	{
		bit = DF_INSTANT_ITEMS;
	}
	else if ( f == &s_allow_exit_box )
	{
		bit = DF_ALLOW_EXIT;
	}
	else if ( f == &s_powerups_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_ITEMS;
		else
			flags |= DF_NO_ITEMS;
		goto setvalue;
	}
	else if ( f == &s_health_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_HEALTH;
		else
			flags |= DF_NO_HEALTH;
		goto setvalue;
	}
	else if ( f == &s_spawn_farthest_box )
	{
		bit = DF_SPAWN_FARTHEST;
	}
	else if ( f == &s_teamplay_box )
	{
		if ( f->curvalue == 1 )
		{
			flags |=  DF_SKINTEAMS;
		}
		else
		{
			flags &= ~( DF_SKINTEAMS );
		}

		goto setvalue;
	}
	else if ( f == &s_samelevel_box )
	{
		bit = DF_SAME_LEVEL;
	}
	else if ( f == &s_force_respawn_box )
	{
		bit = DF_FORCE_RESPAWN;
	}
	else if ( f == &s_armor_box )
	{
		if ( f->curvalue )
			flags &= ~DF_NO_ARMOR;
		else
			flags |= DF_NO_ARMOR;
		goto setvalue;
	}
	else if ( f == &s_infinite_ammo_box )
	{
		bit = DF_INFINITE_AMMO;
	}
	else if ( f == &s_fixed_fov_box )
	{
		bit = DF_FIXED_FOV;
	}
	else if ( f == &s_quad_drop_box )
	{
		bit = DF_QUAD_DROP;
	}

	if ( f )
	{
		if ( f->curvalue == 0 )
			flags &= ~bit;
		else
			flags |= bit;
	}

setvalue:
	Cvar_SetValue ("dmflags", flags);

	Com_sprintf( dmoptions_statusbar, sizeof( dmoptions_statusbar ), "dmflags = %d", flags );

}
void Read_Bot_Info()
{
	FILE *pIn;
	char bot_filename[128];
	int i, count;
	char *info;
	char startmap[128];

	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	if(s_rules_box.curvalue == 1 || s_rules_box.curvalue == 4 || s_rules_box.curvalue == 5)
		strcpy(bot_filename, "botinfo/team.tmp");
	else
		sprintf(bot_filename, "botinfo/%s.tmp", startmap);

	if((pIn = fopen(bot_filename, "rb" )) == NULL)
		return; // bail

	fread(&count,sizeof (int),1,pIn);
	if(count>8)
		count = 8;

	for(i=0;i<count;i++)
	{

		fread(bot[i].userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn);

		info = Info_ValueForKey (bot[i].userinfo, "name");
		strcpy(bot[i].name, info);
	}

    fclose(pIn);
}
void DMOptions_MenuInit( void )
{
	int i;

	static const char *yes_no_names[] =
	{
		"no", "yes", 0
	};
	static const char *teamplay_names[] =
	{
		"disabled", "enabled", 0
	};
	int dmflags = Cvar_VariableValue( "dmflags" );
	int y = 70;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	for(i = 0; i < 8; i++)
		strcpy(bot[i].name, "...empty slot");

	Read_Bot_Info();

	s_dmoptions_menu.x = viddef.width * 0.50;
	s_dmoptions_menu.nitems = 0;

	s_falls_box.generic.type = MTYPE_SPINCONTROL;
	s_falls_box.generic.x	= 0;
	s_falls_box.generic.y	= y;
	s_falls_box.generic.name	= "falling damage";
	s_falls_box.generic.callback = DMFlagCallback;
	s_falls_box.itemnames = yes_no_names;
	s_falls_box.curvalue = ( dmflags & DF_NO_FALLING ) == 0;

	s_weapons_stay_box.generic.type = MTYPE_SPINCONTROL;
	s_weapons_stay_box.generic.x	= 0;
	s_weapons_stay_box.generic.y	= y += 10*scale;
	s_weapons_stay_box.generic.name	= "weapons stay";
	s_weapons_stay_box.generic.callback = DMFlagCallback;
	s_weapons_stay_box.itemnames = yes_no_names;
	s_weapons_stay_box.curvalue = ( dmflags & DF_WEAPONS_STAY ) != 0;

	s_instant_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_instant_powerups_box.generic.x	= 0;
	s_instant_powerups_box.generic.y	= y += 10*scale;
	s_instant_powerups_box.generic.name	= "instant powerups";
	s_instant_powerups_box.generic.callback = DMFlagCallback;
	s_instant_powerups_box.itemnames = yes_no_names;
	s_instant_powerups_box.curvalue = ( dmflags & DF_INSTANT_ITEMS ) != 0;

	s_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_powerups_box.generic.x	= 0;
	s_powerups_box.generic.y	= y += 10*scale;
	s_powerups_box.generic.name	= "allow powerups";
	s_powerups_box.generic.callback = DMFlagCallback;
	s_powerups_box.itemnames = yes_no_names;
	s_powerups_box.curvalue = ( dmflags & DF_NO_ITEMS ) == 0;

	s_health_box.generic.type = MTYPE_SPINCONTROL;
	s_health_box.generic.x	= 0;
	s_health_box.generic.y	= y += 10*scale;
	s_health_box.generic.callback = DMFlagCallback;
	s_health_box.generic.name	= "allow health";
	s_health_box.itemnames = yes_no_names;
	s_health_box.curvalue = ( dmflags & DF_NO_HEALTH ) == 0;

	s_armor_box.generic.type = MTYPE_SPINCONTROL;
	s_armor_box.generic.x	= 0;
	s_armor_box.generic.y	= y += 10*scale;
	s_armor_box.generic.name	= "allow armor";
	s_armor_box.generic.callback = DMFlagCallback;
	s_armor_box.itemnames = yes_no_names;
	s_armor_box.curvalue = ( dmflags & DF_NO_ARMOR ) == 0;

	s_spawn_farthest_box.generic.type = MTYPE_SPINCONTROL;
	s_spawn_farthest_box.generic.x	= 0;
	s_spawn_farthest_box.generic.y	= y += 10*scale;
	s_spawn_farthest_box.generic.name	= "spawn farthest";
	s_spawn_farthest_box.generic.callback = DMFlagCallback;
	s_spawn_farthest_box.itemnames = yes_no_names;
	s_spawn_farthest_box.curvalue = ( dmflags & DF_SPAWN_FARTHEST ) != 0;

	s_samelevel_box.generic.type = MTYPE_SPINCONTROL;
	s_samelevel_box.generic.x	= 0;
	s_samelevel_box.generic.y	= y += 10*scale;
	s_samelevel_box.generic.name	= "same map";
	s_samelevel_box.generic.callback = DMFlagCallback;
	s_samelevel_box.itemnames = yes_no_names;
	s_samelevel_box.curvalue = ( dmflags & DF_SAME_LEVEL ) != 0;

	s_force_respawn_box.generic.type = MTYPE_SPINCONTROL;
	s_force_respawn_box.generic.x	= 0;
	s_force_respawn_box.generic.y	= y += 10*scale;
	s_force_respawn_box.generic.name	= "force respawn";
	s_force_respawn_box.generic.callback = DMFlagCallback;
	s_force_respawn_box.itemnames = yes_no_names;
	s_force_respawn_box.curvalue = ( dmflags & DF_FORCE_RESPAWN ) != 0;

	s_teamplay_box.generic.type = MTYPE_SPINCONTROL;
	s_teamplay_box.generic.x	= 0;
	s_teamplay_box.generic.y	= y += 10*scale;
	s_teamplay_box.generic.name	= "teamplay";
	s_teamplay_box.generic.callback = DMFlagCallback;
	s_teamplay_box.itemnames = teamplay_names;

	s_allow_exit_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_exit_box.generic.x	= 0;
	s_allow_exit_box.generic.y	= y += 10*scale;
	s_allow_exit_box.generic.name	= "allow exit";
	s_allow_exit_box.generic.callback = DMFlagCallback;
	s_allow_exit_box.itemnames = yes_no_names;
	s_allow_exit_box.curvalue = ( dmflags & DF_ALLOW_EXIT ) != 0;

	s_infinite_ammo_box.generic.type = MTYPE_SPINCONTROL;
	s_infinite_ammo_box.generic.x	= 0;
	s_infinite_ammo_box.generic.y	= y += 10*scale;
	s_infinite_ammo_box.generic.name	= "infinite ammo";
	s_infinite_ammo_box.generic.callback = DMFlagCallback;
	s_infinite_ammo_box.itemnames = yes_no_names;
	s_infinite_ammo_box.curvalue = ( dmflags & DF_INFINITE_AMMO ) != 0;

	s_fixed_fov_box.generic.type = MTYPE_SPINCONTROL;
	s_fixed_fov_box.generic.x	= 0;
	s_fixed_fov_box.generic.y	= y += 10*scale;
	s_fixed_fov_box.generic.name	= "fixed FOV";
	s_fixed_fov_box.generic.callback = DMFlagCallback;
	s_fixed_fov_box.itemnames = yes_no_names;
	s_fixed_fov_box.curvalue = ( dmflags & DF_FIXED_FOV ) != 0;

	s_quad_drop_box.generic.type = MTYPE_SPINCONTROL;
	s_quad_drop_box.generic.x	= 0;
	s_quad_drop_box.generic.y	= y += 10*scale;
	s_quad_drop_box.generic.name	= "quad drop";
	s_quad_drop_box.generic.callback = DMFlagCallback;
	s_quad_drop_box.itemnames = yes_no_names;
	s_quad_drop_box.curvalue = ( dmflags & DF_QUAD_DROP ) != 0;

	s_friendlyfire_box.generic.type = MTYPE_SPINCONTROL;
	s_friendlyfire_box.generic.x	= 0;
	s_friendlyfire_box.generic.y	= y += 10*scale;
	s_friendlyfire_box.generic.name	= "friendly fire";
	s_friendlyfire_box.generic.callback = DMFlagCallback;
	s_friendlyfire_box.itemnames = yes_no_names;
	s_friendlyfire_box.curvalue = ( dmflags & DF_NO_FRIENDLY_FIRE ) == 0;

	s_botchat_box.generic.type = MTYPE_SPINCONTROL;
	s_botchat_box.generic.x	= 0;
	s_botchat_box.generic.y	= y += 10*scale;
	s_botchat_box.generic.name	= "bot chat";
	s_botchat_box.generic.callback = DMFlagCallback;
	s_botchat_box.itemnames = yes_no_names;
	s_botchat_box.curvalue = ( dmflags & DF_BOTCHAT ) == 0;

	s_bot_fuzzyaim_box.generic.type = MTYPE_SPINCONTROL;
	s_bot_fuzzyaim_box.generic.x	= 0;
	s_bot_fuzzyaim_box.generic.y	= y += 10*scale;
	s_bot_fuzzyaim_box.generic.name	= "bot fuzzy aim";
	s_bot_fuzzyaim_box.generic.callback = DMFlagCallback;
	s_bot_fuzzyaim_box.itemnames = yes_no_names;
	s_bot_fuzzyaim_box.curvalue = ( dmflags & DF_BOT_FUZZYAIM ) == 0;

	s_bot_auto_save_nodes_box.generic.type = MTYPE_SPINCONTROL;
	s_bot_auto_save_nodes_box.generic.x	= 0;
	s_bot_auto_save_nodes_box.generic.y	= y += 10*scale;
	s_bot_auto_save_nodes_box.generic.name	= "auto node save";
	s_bot_auto_save_nodes_box.generic.callback = DMFlagCallback;
	s_bot_auto_save_nodes_box.itemnames = yes_no_names;
	s_bot_auto_save_nodes_box.curvalue = ( dmflags & DF_BOT_AUTOSAVENODES ) == 1;

	s_bot_levelad_box.generic.type = MTYPE_SPINCONTROL;
	s_bot_levelad_box.generic.x	= 0;
	s_bot_levelad_box.generic.y	= y += 10*scale;
	s_bot_levelad_box.generic.name	= "repeat level if bot wins";
	s_bot_levelad_box.generic.callback = DMFlagCallback;
	s_bot_levelad_box.itemnames = yes_no_names;
	s_bot_levelad_box.curvalue = ( dmflags & DF_BOT_LEVELAD ) == 0;

	s_bots_box.generic.type = MTYPE_SPINCONTROL;
	s_bots_box.generic.x	= 0;
	s_bots_box.generic.y	= y += 10*scale;
	s_bots_box.generic.name	= "bots in game";
	s_bots_box.generic.callback = DMFlagCallback;
	s_bots_box.itemnames = yes_no_names;
	s_bots_box.curvalue = ( dmflags & DF_BOTS ) == 0;

	for (i = 0; i < 8; i++) {
		s_bots_bot_action[i].generic.type = MTYPE_ACTION;
		s_bots_bot_action[i].generic.name = bot[i].name;
		s_bots_bot_action[i].generic.x = 0;
		s_bots_bot_action[i].generic.y = y+10*scale*(i+2);
		s_bots_bot_action[i].generic.cursor_offset = -8;
		s_bots_bot_action[i].generic.callback = BotAction;
		s_bots_bot_action[i].curvalue = i;
	}
	//============

	Menu_AddItem( &s_dmoptions_menu, &s_falls_box );
	Menu_AddItem( &s_dmoptions_menu, &s_weapons_stay_box );
	Menu_AddItem( &s_dmoptions_menu, &s_instant_powerups_box );
	Menu_AddItem( &s_dmoptions_menu, &s_powerups_box );
	Menu_AddItem( &s_dmoptions_menu, &s_health_box );
	Menu_AddItem( &s_dmoptions_menu, &s_armor_box );
	Menu_AddItem( &s_dmoptions_menu, &s_spawn_farthest_box );
	Menu_AddItem( &s_dmoptions_menu, &s_samelevel_box );
	Menu_AddItem( &s_dmoptions_menu, &s_force_respawn_box );
	Menu_AddItem( &s_dmoptions_menu, &s_teamplay_box );
	Menu_AddItem( &s_dmoptions_menu, &s_allow_exit_box );
	Menu_AddItem( &s_dmoptions_menu, &s_infinite_ammo_box );
	Menu_AddItem( &s_dmoptions_menu, &s_fixed_fov_box );
	Menu_AddItem( &s_dmoptions_menu, &s_quad_drop_box );
	Menu_AddItem( &s_dmoptions_menu, &s_friendlyfire_box );
	Menu_AddItem( &s_dmoptions_menu, &s_botchat_box );
	Menu_AddItem( &s_dmoptions_menu, &s_bot_fuzzyaim_box );
	Menu_AddItem( &s_dmoptions_menu, &s_bot_auto_save_nodes_box );
	Menu_AddItem( &s_dmoptions_menu, &s_bot_levelad_box );
	Menu_AddItem( &s_dmoptions_menu, &s_bots_box );
	for(i = 0; i < 8; i++)
		Menu_AddItem( &s_dmoptions_menu, &s_bots_bot_action[i]);

	//=======

	Menu_Center( &s_dmoptions_menu );

	// set the original dmflags statusbar
	DMFlagCallback( 0 );
	Menu_SetStatusBar( &s_dmoptions_menu, dmoptions_statusbar );
}

void DMOptions_MenuDraw(void)
{
	M_Background( "m_controls_back"); //draw black background first
	M_Banner( "m_dmoptions" );

	Menu_Draw( &s_dmoptions_menu );
}

const char *DMOptions_MenuKey( int key )
{
	return Default_MenuKey( &s_dmoptions_menu, key );
}

void M_Menu_DMOptions_f (void)
{
	DMOptions_MenuInit();
	M_PushMenu( DMOptions_MenuDraw, DMOptions_MenuKey );
}

/*
=============================================================================

DOWNLOADOPTIONS BOOK MENU

=============================================================================
*/
static menuframework_s s_downloadoptions_menu;

static menuseparator_s	s_download_title;
static menulist_s	s_allow_download_box;
static menulist_s	s_allow_download_maps_box;
static menulist_s	s_allow_download_models_box;
static menulist_s	s_allow_download_players_box;
static menulist_s	s_allow_download_sounds_box;

static void DownloadCallback( void *self )
{
	menulist_s *f = ( menulist_s * ) self;

	if (f == &s_allow_download_box)
	{
		Cvar_SetValue("allow_download", f->curvalue);
	}

	else if (f == &s_allow_download_maps_box)
	{
		Cvar_SetValue("allow_download_maps", f->curvalue);
	}

	else if (f == &s_allow_download_models_box)
	{
		Cvar_SetValue("allow_download_models", f->curvalue);
	}

	else if (f == &s_allow_download_players_box)
	{
		Cvar_SetValue("allow_download_players", f->curvalue);
	}

	else if (f == &s_allow_download_sounds_box)
	{
		Cvar_SetValue("allow_download_sounds", f->curvalue);
	}
}

/*
=============================================================================

ADDRESS BOOK MENU

=============================================================================
*/
#define NUM_ADDRESSBOOK_ENTRIES 9

static menuframework_s	s_addressbook_menu;
static menufield_s		s_addressbook_fields[NUM_ADDRESSBOOK_ENTRIES];

void AddressBook_MenuInit( void )
{
	int i;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	s_addressbook_menu.x = viddef.width / 2 - 142*scale;
	s_addressbook_menu.y = viddef.height / 2 - 58*scale;
	s_addressbook_menu.nitems = 0;

	for ( i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++ )
	{
		cvar_t *adr;
		char buffer[20];

		Com_sprintf( buffer, sizeof( buffer ), "adr%d", i );

		adr = Cvar_Get( buffer, "", CVAR_ARCHIVE );

		s_addressbook_fields[i].generic.type = MTYPE_FIELD;
		s_addressbook_fields[i].generic.name = 0;
		s_addressbook_fields[i].generic.callback = 0;
		s_addressbook_fields[i].generic.x		= 0;
		s_addressbook_fields[i].generic.y		= i * 18*scale + 0;
		s_addressbook_fields[i].generic.localdata[0] = i;
		s_addressbook_fields[i].cursor			= 0;
		s_addressbook_fields[i].length			= 60*scale;
		s_addressbook_fields[i].visible_length	= 30*scale;

		strcpy( s_addressbook_fields[i].buffer, adr->string );

		//test(this is where we will read in a bunch of servers from our website
		//strcpy( s_addressbook_fields[4].buffer, "27.0.0.1");

		Menu_AddItem( &s_addressbook_menu, &s_addressbook_fields[i] );
	}
}

const char *AddressBook_MenuKey( int key )
{
	if ( key == K_ESCAPE )
	{
		int index;
		char buffer[20];

		for ( index = 0; index < NUM_ADDRESSBOOK_ENTRIES; index++ )
		{
			Com_sprintf( buffer, sizeof( buffer ), "adr%d", index );
			Cvar_Set( buffer, s_addressbook_fields[index].buffer );
		}
	}
	return Default_MenuKey( &s_addressbook_menu, key );
}

void AddressBook_MenuDraw(void)
{
	M_Background( "conback"); //draw black background first
	M_Banner( "m_banner_main" );
	Menu_Draw( &s_addressbook_menu );
}

void M_Menu_AddressBook_f(void)
{
	AddressBook_MenuInit();
	M_PushMenu( AddressBook_MenuDraw, AddressBook_MenuKey );
}

/*
=============================================================================

PLAYER CONFIG MENU

=============================================================================
*/
static menuframework_s	s_player_config_menu;
static menufield_s		s_player_name_field;
static menulist_s		s_player_model_box;
static menulist_s		s_player_skin_box;
static menulist_s		s_player_handedness_box;
static menulist_s		s_player_rate_box;
static menuseparator_s	s_player_skin_title;
static menuseparator_s	s_player_model_title;
static menuseparator_s	s_player_hand_title;
static menuseparator_s	s_player_rate_title;

#define MAX_DISPLAYNAME 16
#define MAX_PLAYERMODELS 1024

typedef struct
{
	int		nskins;
	char	**skindisplaynames;
	char	displayname[MAX_DISPLAYNAME];
	char	directory[MAX_QPATH];
} playermodelinfo_s;

static playermodelinfo_s s_pmi[MAX_PLAYERMODELS];
static char *s_pmnames[MAX_PLAYERMODELS];
static int s_numplayermodels;

static int rate_tbl[] = { 2500, 3200, 5000, 10000, 25000, 0 };
static const char *rate_names[] = { "28.8 Modem", "33.6 Modem", "Single ISDN",
	"Dual ISDN/Cable", "T1/LAN", "User defined", 0 };

static void HandednessCallback( void *unused )
{
	Cvar_SetValue( "hand", s_player_handedness_box.curvalue );
}

static void RateCallback( void *unused )
{
	if (s_player_rate_box.curvalue != sizeof(rate_tbl) / sizeof(*rate_tbl) - 1)
		Cvar_SetValue( "rate", rate_tbl[s_player_rate_box.curvalue] );
}

static void ModelCallback( void *unused )
{
	s_player_skin_box.itemnames = s_pmi[s_player_model_box.curvalue].skindisplaynames;
	s_player_skin_box.curvalue = 0;
}

static void FreeFileList( char **list, int n )
{
	int i;

	for ( i = 0; i < n; i++ )
	{
		if ( list[i] )
		{
			free( list[i] );
			list[i] = 0;
		}
	}
	free( list );
}

static qboolean IconOfSkinExists( char *skin, char **pcxfiles, int npcxfiles )
{
	int i;
	char scratch[1024];

	strcpy( scratch, skin );
	*strrchr( scratch, '.' ) = 0;
	strcat( scratch, "_i.tga" );

	for ( i = 0; i < npcxfiles; i++ )
	{
		if ( strcmp( pcxfiles[i], scratch ) == 0 )
			return true;
	}

	return false;
}

static void PlayerConfig_ScanDirectories( void )
{
	char findname[1024];
	char scratch[1024];
	int ndirs = 0, npms = 0;
	char **dirnames;
	int i;

	extern char **FS_ListFiles( char *, int *, unsigned, unsigned );

	s_numplayermodels = 0;

	//get dirs from gamedir first.

	Com_sprintf( findname, sizeof(findname), "%s/players/*.*", FS_Gamedir() );

	dirnames = FS_ListFiles( findname, &ndirs, SFF_SUBDIR, 0 );
	
	if ( dirnames ) {
		
		/*
		** go through the subdirectories
		*/
		npms = ndirs;
		if ( npms > MAX_PLAYERMODELS )
			npms = MAX_PLAYERMODELS;

		for ( i = 0; i < npms; i++ )
		{
			int k, s;
			char *a, *b, *c;
			char **pcxnames;
			char **skinnames;
			int npcxfiles;
			int nskins = 0;

			if ( dirnames[i] == 0 )
				continue;

			// verify the existence of tris.md2
			strcpy( scratch, dirnames[i] );
			strcat( scratch, "/tris.md2" );
			if ( !Sys_FindFirst( scratch, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM ) )
			{
				free( dirnames[i] );
				dirnames[i] = 0;
				Sys_FindClose();
				continue;
			}
			Sys_FindClose();

			// verify the existence of at least one skin
			strcpy( scratch, dirnames[i] );
			strcat( scratch, "/*.tga" );
			pcxnames = FS_ListFiles( scratch, &npcxfiles, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

			if ( !pcxnames )
			{
				free( dirnames[i] );
				dirnames[i] = 0;
				continue;
			}

			// count valid skins, which consist of a skin with a matching "_i" icon
			for ( k = 0; k < npcxfiles-1; k++ )
			{
				if ( !strstr( pcxnames[k], "_i.tga" ) )
				{
					if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles - 1 ) )
					{
						nskins++;
					}
				}
			}
			if ( !nskins )
				continue;

			skinnames = malloc( sizeof( char * ) * ( nskins + 1 ) );
			memset( skinnames, 0, sizeof( char * ) * ( nskins + 1 ) );

			// copy the valid skins
			for ( s = 0, k = 0; k < npcxfiles-1; k++ )
			{
				char *a, *b, *c;

				if ( !strstr( pcxnames[k], "_i.tga" ) )
				{
					if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles - 1 ) )
					{
						a = strrchr( pcxnames[k], '/' );
						b = strrchr( pcxnames[k], '\\' );

						if ( a > b )
							c = a;
						else
							c = b;

						strcpy( scratch, c + 1 );

						if ( strrchr( scratch, '.' ) )
							*strrchr( scratch, '.' ) = 0;

						skinnames[s] = strdup( scratch );
						s++;
					}
				}
			}

			// at this point we have a valid player model
			s_pmi[s_numplayermodels].nskins = nskins;
			s_pmi[s_numplayermodels].skindisplaynames = skinnames;

			// make short name for the model
			a = strrchr( dirnames[i], '/' );
			b = strrchr( dirnames[i], '\\' );

			if ( a > b )
				c = a;
			else
				c = b;

			strncpy( s_pmi[s_numplayermodels].displayname, c + 1, MAX_DISPLAYNAME-1 );
			strcpy( s_pmi[s_numplayermodels].directory, c + 1 );

			FreeFileList( pcxnames, npcxfiles );

			s_numplayermodels++;
		}
		if ( dirnames )
			FreeFileList( dirnames, ndirs );
	}

	/*
	** get a list of directories from basedir
	*/

	Com_sprintf( findname, sizeof(findname), "%s/players/*.*", BASEDIRNAME);

	 dirnames = FS_ListFiles( findname, &ndirs, SFF_SUBDIR, 0 );

	if ( !dirnames )
		return;

	/*
	** go through the subdirectories
	*/
	npms = ndirs;
	if ( npms > MAX_PLAYERMODELS )
		npms = MAX_PLAYERMODELS;

	for ( i = 0; i < npms; i++ )
	{
		int k, s;
		char *a, *b, *c;
		char **pcxnames;
		char **skinnames;
		int npcxfiles;
		int nskins = 0;

		if ( dirnames[i] == 0 )
			continue;

		// verify the existence of tris.md2
		strcpy( scratch, dirnames[i] );
		strcat( scratch, "/tris.md2" );
		if ( !Sys_FindFirst( scratch, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM ) )
		{
			free( dirnames[i] );
			dirnames[i] = 0;
			Sys_FindClose();
			continue;
		}
		Sys_FindClose();

		// verify the existence of at least one skin
		strcpy( scratch, dirnames[i] );
		strcat( scratch, "/*.tga" );
		pcxnames = FS_ListFiles( scratch, &npcxfiles, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		if ( !pcxnames )
		{
			free( dirnames[i] );
			dirnames[i] = 0;
			continue;
		}

		// count valid skins, which consist of a skin with a matching "_i" icon
		for ( k = 0; k < npcxfiles-1; k++ )
		{
			if ( !strstr( pcxnames[k], "_i.tga" ) )
			{
				if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles - 1 ) )
				{
					nskins++;
				}
			}
		}
		if ( !nskins )
			continue;

		skinnames = malloc( sizeof( char * ) * ( nskins + 1 ) );
		memset( skinnames, 0, sizeof( char * ) * ( nskins + 1 ) );

		// copy the valid skins
		for ( s = 0, k = 0; k < npcxfiles-1; k++ )
		{
			char *a, *b, *c;

			if ( !strstr( pcxnames[k], "_i.tga" ) )
			{
				if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles - 1 ) )
				{
					a = strrchr( pcxnames[k], '/' );
					b = strrchr( pcxnames[k], '\\' );

					if ( a > b )
						c = a;
					else
						c = b;

					strcpy( scratch, c + 1 );

					if ( strrchr( scratch, '.' ) )
						*strrchr( scratch, '.' ) = 0;

					skinnames[s] = strdup( scratch );
					s++;
				}
			}
		}

		// at this point we have a valid player model
		s_pmi[s_numplayermodels].nskins = nskins;
		s_pmi[s_numplayermodels].skindisplaynames = skinnames;

		// make short name for the model
		a = strrchr( dirnames[i], '/' );
		b = strrchr( dirnames[i], '\\' );

		if ( a > b )
			c = a;
		else
			c = b;

		strncpy( s_pmi[s_numplayermodels].displayname, c + 1, MAX_DISPLAYNAME-1 );
		strcpy( s_pmi[s_numplayermodels].directory, c + 1 );

		FreeFileList( pcxnames, npcxfiles );

		s_numplayermodels++;
	}
	if ( dirnames )
		FreeFileList( dirnames, ndirs );
}

static int pmicmpfnc( const void *_a, const void *_b )
{
	const playermodelinfo_s *a = ( const playermodelinfo_s * ) _a;
	const playermodelinfo_s *b = ( const playermodelinfo_s * ) _b;

	/*
	** sort by male, female, then alphabetical
	*/
	if ( strcmp( a->directory, "male" ) == 0 )
		return -1;
	else if ( strcmp( b->directory, "male" ) == 0 )
		return 1;

	if ( strcmp( a->directory, "female" ) == 0 )
		return -1;
	else if ( strcmp( b->directory, "female" ) == 0 )
		return 1;

	return strcmp( a->directory, b->directory );
}


qboolean PlayerConfig_MenuInit( void )
{
	extern cvar_t *name;
	extern cvar_t *team;
	extern cvar_t *skin;
	char currentdirectory[1024];
	char currentskin[1024];
	int i = 0;
	float scale;
	int currentdirectoryindex = 0;
	int currentskinindex = 0;
	static const char *yes_no_names[] =
	{
		"no", "yes", 0
	};
	cvar_t *hand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );

	static const char *handedness[] = { "right", "left", "center", 0 };

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	PlayerConfig_ScanDirectories();

	if (s_numplayermodels == 0)
		return false;

	if ( hand->value < 0 || hand->value > 2 )
		Cvar_SetValue( "hand", 0 );

	strcpy( currentdirectory, skin->string );

	if ( strchr( currentdirectory, '/' ) )
	{
		strcpy( currentskin, strchr( currentdirectory, '/' ) + 1 );
		*strchr( currentdirectory, '/' ) = 0;
	}
	else if ( strchr( currentdirectory, '\\' ) )
	{
		strcpy( currentskin, strchr( currentdirectory, '\\' ) + 1 );
		*strchr( currentdirectory, '\\' ) = 0;
	}
	else
	{
		strcpy( currentdirectory, "male" );
		strcpy( currentskin, "grunt" );
	}

	qsort( s_pmi, s_numplayermodels, sizeof( s_pmi[0] ), pmicmpfnc );

	memset( s_pmnames, 0, sizeof( s_pmnames ) );
	for ( i = 0; i < s_numplayermodels; i++ )
	{
		s_pmnames[i] = s_pmi[i].displayname;
		if ( Q_stricmp( s_pmi[i].directory, currentdirectory ) == 0 )
		{
			int j;

			currentdirectoryindex = i;

			for ( j = 0; j < s_pmi[i].nskins; j++ )
			{
				if ( Q_stricmp( s_pmi[i].skindisplaynames[j], currentskin ) == 0 )
				{
					currentskinindex = j;
					break;
				}
			}
		}
	}

	s_player_config_menu.x = viddef.width / 2 - 95*scale;
	s_player_config_menu.y = viddef.height / 2 - 97*scale;
	s_player_config_menu.nitems = 0;

	s_player_name_field.generic.type = MTYPE_FIELD;
	s_player_name_field.generic.name = "name";
	s_player_name_field.generic.callback = 0;
	s_player_name_field.generic.x		= -32;
	s_player_name_field.generic.y		= 0;
	s_player_name_field.length	= 20;
	s_player_name_field.visible_length = 20;
	strcpy( s_player_name_field.buffer, name->string );
	s_player_name_field.cursor = strlen( name->string );


	s_player_model_box.generic.type = MTYPE_SPINCONTROL;
	s_player_model_box.generic.name = "model";
	s_player_model_box.generic.x	= -32;
	s_player_model_box.generic.y	= 70*scale;
	s_player_model_box.generic.callback = ModelCallback;
	s_player_model_box.generic.cursor_offset = -56;
	s_player_model_box.curvalue = currentdirectoryindex;
	s_player_model_box.itemnames = s_pmnames;

	s_player_skin_box.generic.type = MTYPE_SPINCONTROL;
	s_player_skin_box.generic.name = "skin";
	s_player_skin_box.generic.x	= -32;
	s_player_skin_box.generic.y	= 94*scale;
	s_player_skin_box.generic.callback = 0;
	s_player_skin_box.generic.cursor_offset = -56;
	s_player_skin_box.curvalue = currentskinindex;
	s_player_skin_box.itemnames = s_pmi[currentdirectoryindex].skindisplaynames;

	s_player_handedness_box.generic.type = MTYPE_SPINCONTROL;
	s_player_handedness_box.generic.name = "handedness";
	s_player_handedness_box.generic.x	= -32;
	s_player_handedness_box.generic.y	= 118*scale;
	s_player_handedness_box.generic.cursor_offset = -56;
	s_player_handedness_box.generic.callback = HandednessCallback;
	s_player_handedness_box.curvalue = Cvar_VariableValue( "hand" );
	s_player_handedness_box.itemnames = handedness;

	for (i = 0; i < sizeof(rate_tbl) / sizeof(*rate_tbl) - 1; i++)
		if (Cvar_VariableValue("rate") == rate_tbl[i])
			break;

	s_player_rate_box.generic.type = MTYPE_SPINCONTROL;
	s_player_rate_box.generic.x	= -32;
	s_player_rate_box.generic.y	= 142*scale;
	s_player_rate_box.generic.name	= "connect speed";
	s_player_rate_box.generic.callback = RateCallback;
	s_player_rate_box.curvalue = i;
	s_player_rate_box.itemnames = rate_names;

	s_allow_download_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_box.generic.x	= 72;
	s_allow_download_box.generic.y	= 186*scale;
	s_allow_download_box.generic.name	= "allow downloading";
	s_allow_download_box.generic.callback = DownloadCallback;
	s_allow_download_box.itemnames = yes_no_names;
	s_allow_download_box.curvalue = (Cvar_VariableValue("allow_download") != 0);

	s_allow_download_maps_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_maps_box.generic.x	= 72;
	s_allow_download_maps_box.generic.y	= 196*scale;
	s_allow_download_maps_box.generic.name	= "maps";
	s_allow_download_maps_box.generic.callback = DownloadCallback;
	s_allow_download_maps_box.itemnames = yes_no_names;
	s_allow_download_maps_box.curvalue = (Cvar_VariableValue("allow_download_maps") != 0);

	s_allow_download_players_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_players_box.generic.x	= 72;
	s_allow_download_players_box.generic.y	= 206*scale;
	s_allow_download_players_box.generic.name	= "player models/skins";
	s_allow_download_players_box.generic.callback = DownloadCallback;
	s_allow_download_players_box.itemnames = yes_no_names;
	s_allow_download_players_box.curvalue = (Cvar_VariableValue("allow_download_players") != 0);

	s_allow_download_models_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_models_box.generic.x	= 72;
	s_allow_download_models_box.generic.y	= 216*scale;
	s_allow_download_models_box.generic.name	= "models";
	s_allow_download_models_box.generic.callback = DownloadCallback;
	s_allow_download_models_box.itemnames = yes_no_names;
	s_allow_download_models_box.curvalue = (Cvar_VariableValue("allow_download_models") != 0);

	s_allow_download_sounds_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_sounds_box.generic.x	= 72;
	s_allow_download_sounds_box.generic.y	= 226*scale;
	s_allow_download_sounds_box.generic.name	= "sounds";
	s_allow_download_sounds_box.generic.callback = DownloadCallback;
	s_allow_download_sounds_box.itemnames = yes_no_names;
	s_allow_download_sounds_box.curvalue = (Cvar_VariableValue("allow_download_sounds") != 0);

	Menu_AddItem( &s_player_config_menu, &s_allow_download_box );
	Menu_AddItem( &s_player_config_menu, &s_allow_download_maps_box );
	Menu_AddItem( &s_player_config_menu, &s_allow_download_players_box );
	Menu_AddItem( &s_player_config_menu, &s_allow_download_models_box );
	Menu_AddItem( &s_player_config_menu, &s_allow_download_sounds_box );

	Menu_AddItem( &s_player_config_menu, &s_player_name_field );
	Menu_AddItem( &s_player_config_menu, &s_player_model_box );
	if ( s_player_skin_box.itemnames )
	{
		Menu_AddItem( &s_player_config_menu, &s_player_skin_box );
	}
	Menu_AddItem( &s_player_config_menu, &s_player_handedness_box );
	Menu_AddItem( &s_player_config_menu, &s_player_rate_box );

	//add in shader support for player models, if the player goes into the menu before entering a
	//level, that way we see the shaders.  We only want to do this if they are NOT loaded yet.
	scriptsloaded = Cvar_Get("scriptsloaded", "0", 0);
	if(!scriptsloaded->value) {
		Cvar_SetValue("scriptsloaded", 1);
		RS_LoadScript("scripts/models.rscript");
		RS_LoadScript("scripts/caustics.rscript");
		RS_LoadSpecialScripts();
	}

	return true;
}

void PlayerConfig_MenuDraw( void )
{
	extern float CalcFov( float fov_x, float w, float h );
	refdef_t refdef;
	char scratch[MAX_QPATH];
	FILE *modelfile;
	int helmet = false;
	int rack = false;
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	M_Background( "m_player_back"); //draw black background first
	M_Banner( "m_player" );

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.width = viddef.width;
	refdef.height = viddef.height;
	refdef.x = 0;
	refdef.y = 0;
	refdef.fov_x = 90;
	refdef.fov_y = CalcFov( refdef.fov_x, refdef.width, refdef.height );
	refdef.time = cls.realtime*0.001;

	if ( s_pmi[s_player_model_box.curvalue].skindisplaynames )
	{
		static int mframe;
		static int yaw;
		int maxframe = 29;
		entity_t entity[3];

		memset( &entity, 0, sizeof( entity ) );

		mframe += 1;
		if ( mframe > 390 )
			mframe =0;

		yaw += .5;
		if (++yaw > 360)
			yaw = 0;

		Com_sprintf( scratch, sizeof( scratch ), "players/%s/tris.md2", s_pmi[s_player_model_box.curvalue].directory );
		entity[0].model = R_RegisterModel( scratch );
		Com_sprintf( scratch, sizeof( scratch ), "players/%s/%s.tga", s_pmi[s_player_model_box.curvalue].directory, s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );
		entity[0].skin = R_RegisterSkin( scratch );

		Com_sprintf( scratch, sizeof( scratch ), "players/%s/weapon.md2", s_pmi[s_player_model_box.curvalue].directory );
		entity[1].model = R_RegisterModel( scratch );
		Com_sprintf( scratch, sizeof( scratch ), "players/%s/weapon.tga", s_pmi[s_player_model_box.curvalue].directory );
		entity[1].skin = R_RegisterSkin( scratch );

		//if a helmet or other special device
		Com_sprintf( scratch, sizeof( scratch ), "%s/players/%s/helmet.md2", FS_Gamedir(), s_pmi[s_player_model_box.curvalue].directory );
		Menu_FindFile (scratch, &modelfile);
		if(modelfile) {
			helmet = true;
			Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.md2", s_pmi[s_player_model_box.curvalue].directory );
			entity[2].model = R_RegisterModel( scratch );
			Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.tga", s_pmi[s_player_model_box.curvalue].directory );
			entity[2].skin = R_RegisterSkin( scratch );
			fclose(modelfile);
		}
		else {
			Com_sprintf( scratch, sizeof( scratch ), "%s/players/%s/helmet.md2", BASEDIRNAME, s_pmi[s_player_model_box.curvalue].directory );
			Menu_FindFile (scratch, &modelfile);
			if(modelfile) {
				helmet = true;
				Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.md2", s_pmi[s_player_model_box.curvalue].directory );
				entity[2].model = R_RegisterModel( scratch );
				Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.tga", s_pmi[s_player_model_box.curvalue].directory );
				entity[2].skin = R_RegisterSkin( scratch );
				fclose(modelfile);
			}
		}
		//don't bother with gamedirs, this is a special case. Damn brainlets.
		Com_sprintf( scratch, sizeof( scratch ), "%s/players/%s/gunrack.md2", BASEDIRNAME, s_pmi[s_player_model_box.curvalue].directory );
		Menu_FindFile (scratch, &modelfile);
		if(modelfile) {
			rack = true;
			Com_sprintf( scratch, sizeof( scratch ), "players/%s/gunrack.md2", s_pmi[s_player_model_box.curvalue].directory );
			entity[2].model = R_RegisterModel( scratch );
			Com_sprintf( scratch, sizeof( scratch ), "players/%s/gunrack.tga", s_pmi[s_player_model_box.curvalue].directory );
			entity[2].skin = R_RegisterSkin( scratch );
			fclose(modelfile);
		}
		
		entity[0].flags = RF_FULLBRIGHT;
		entity[0].origin[0] = 80;
		entity[0].origin[1] = -25;
		entity[0].origin[2] = 0;

		entity[1].flags = RF_FULLBRIGHT;
		entity[1].origin[0] = 80;
		entity[1].origin[1] = -25;
		entity[1].origin[2] = 0;

		if(helmet)
			entity[2].flags = RF_FULLBRIGHT | RF_TRANSLUCENT;
		else
			entity[2].flags = RF_FULLBRIGHT;
		entity[2].origin[0] = 80;
		entity[2].origin[1] = -25;
		entity[2].origin[2] = 0;
		if(helmet)
			entity[2].alpha = 0.4;

		VectorCopy( entity[0].origin, entity[0].oldorigin );

		VectorCopy( entity[1].origin, entity[1].oldorigin );

		VectorCopy( entity[2].origin, entity[2].oldorigin );

		entity[0].frame = mframe/10;
		entity[0].oldframe = 0;
		entity[0].backlerp = 0.0;
		entity[0].angles[1] = yaw;

		entity[1].frame = mframe/10;
		entity[1].oldframe = 0;
		entity[1].backlerp = 0.0;
		entity[1].angles[1] = yaw;

		entity[2].frame = mframe/10;
		entity[2].oldframe = 0;
		entity[2].backlerp = 0.0;
		entity[2].angles[1] = yaw;

		refdef.areabits = 0;
		if((helmet) || (rack))
			refdef.num_entities = 3;
		else
			refdef.num_entities = 2;

		refdef.entities = entity;
		refdef.lightstyles = 0;
		refdef.rdflags = RDF_NOWORLDMODEL;

		Menu_Draw( &s_player_config_menu );

		refdef.height += 4;

		R_RenderFrame( &refdef );

		Com_sprintf( scratch, sizeof( scratch ), "/players/%s/%s_i.tga",
			s_pmi[s_player_model_box.curvalue].directory,
			s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );

		refdef.y = viddef.height / 2 - 70*scale;
		Draw_StretchPic( s_player_config_menu.x - 88, refdef.y, 32*scale, 32*scale, scratch );
	}
}
void PConfigAccept (void)
{
	int i;
	char scratch[1024];

	Cvar_Set( "name", s_player_name_field.buffer );

	Com_sprintf( scratch, sizeof( scratch ), "%s/%s",
		s_pmi[s_player_model_box.curvalue].directory,
		s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );

	Cvar_Set( "skin", scratch );

	for ( i = 0; i < s_numplayermodels; i++ )
	{
		int j;

		for ( j = 0; j < s_pmi[i].nskins; j++ )
		{
			if ( s_pmi[i].skindisplaynames[j] )
				free( s_pmi[i].skindisplaynames[j] );
			s_pmi[i].skindisplaynames[j] = 0;
		}
		free( s_pmi[i].skindisplaynames );
		s_pmi[i].skindisplaynames = 0;
		s_pmi[i].nskins = 0;
	}
}
const char *PlayerConfig_MenuKey (int key)
{

	if ( key == K_ESCAPE )
		PConfigAccept();

	return Default_MenuKey( &s_player_config_menu, key );
}


void M_Menu_PlayerConfig_f (void)
{
	if (!PlayerConfig_MenuInit())
	{
		Menu_SetStatusBar( &s_options_menu, "No valid player models found" );
		return;
	}
	Menu_SetStatusBar( &s_options_menu, NULL );
	M_PushMenu( PlayerConfig_MenuDraw, PlayerConfig_MenuKey );
}


/*
=======================================================================

GALLERY MENU

=======================================================================
*/
#if 0
void M_Menu_Gallery_f( void )
{
	extern void Gallery_MenuDraw( void );
	extern const char *Gallery_MenuKey( int key );

	M_PushMenu( Gallery_MenuDraw, Gallery_MenuKey );
}
#endif

/*
=======================================================================

QUIT MENU

=======================================================================
*/

static menuframework_s	s_quit_menu;
static menuseparator_s	s_quit_question;
static menuaction_s		s_quit_yes_action;
static menuaction_s		s_quit_no_action;

void M_Quit_Draw( void )
{
	M_Background( "conback"); //draw black background first
	M_Banner( "m_quit" );

	Menu_AdjustCursor( &s_quit_menu, 1 );

	Menu_Draw( &s_quit_menu );
}

const char *M_Quit_MenuKey( int key )
{
	return Default_MenuKey( &s_quit_menu, key );
}

void quitActionNo (void *blah)
{
	M_PopMenu();
}
void quitActionYes (void *blah)
{
	CL_Quit_f();
}

void Quit_MenuInit (void)
{
	float scale;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	s_quit_menu.x = viddef.width * 0.50;
	s_quit_menu.y = viddef.height * 0.50;
	s_quit_menu.nitems = 0;

	s_quit_question.generic.type	= MTYPE_SEPARATOR;
	s_quit_question.generic.name	= "Are you sure?";
	s_quit_question.generic.x	= 32;
	s_quit_question.generic.y	= scale*40;

	s_quit_yes_action.generic.type	= MTYPE_ACTION;
	s_quit_yes_action.generic.x		= 8;
	s_quit_yes_action.generic.y		= scale*60;
	s_quit_yes_action.generic.name	= "  yes";
	s_quit_yes_action.generic.callback = quitActionYes;

	s_quit_no_action.generic.type	= MTYPE_ACTION;
	s_quit_no_action.generic.x		= 8;
	s_quit_no_action.generic.y		= scale*70;
	s_quit_no_action.generic.name	= "  no";
	s_quit_no_action.generic.callback = quitActionNo;

	Menu_AddItem( &s_quit_menu, ( void * ) &s_quit_question );
	Menu_AddItem( &s_quit_menu, ( void * ) &s_quit_yes_action );
	Menu_AddItem( &s_quit_menu, ( void * ) &s_quit_no_action );

	Menu_SetStatusBar( &s_quit_menu, NULL );

	Menu_Center( &s_quit_menu );
}

void M_Menu_Quit_f (void)
{
	Quit_MenuInit();
	M_PushMenu (M_Quit_Draw, M_Quit_MenuKey);
}


//=============================================================================
/* Menu Subsystem */


/*
=================
M_Init
=================
*/
void M_Init (void)
{
	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_game", M_Menu_Game_f);
		Cmd_AddCommand ("menu_joinserver", M_Menu_JoinServer_f);
			Cmd_AddCommand ("menu_addressbook", M_Menu_AddressBook_f);
		Cmd_AddCommand ("menu_startserver", M_Menu_StartServer_f);
			Cmd_AddCommand ("menu_dmoptions", M_Menu_DMOptions_f);
		Cmd_AddCommand ("menu_playerconfig", M_Menu_PlayerConfig_f);
		Cmd_AddCommand ("menu_credits", M_Menu_Credits_f );
	Cmd_AddCommand ("menu_video", M_Menu_Video_f);
	Cmd_AddCommand ("menu_options", M_Menu_Options_f);
		Cmd_AddCommand ("menu_keys", M_Menu_Keys_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
}


/*
=================================
Menu Mouse Cursor - psychospaz
=================================
*/

void refreshCursorMenu (void)
{
	cursor.menu = NULL;
}
void refreshCursorLink (void)
{
	cursor.menuitem = NULL;
}

int Slider_CursorPositionX ( menuslider_s *s )
{
	float range;

	range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );

	if ( range < 0)
		range = 0;
	if ( range > 1)
		range = 1;

	return ( int )( (MENU_FONT_SIZE) + RCOLUMN_OFFSET + (SLIDER_RANGE)*(MENU_FONT_SIZE) * range );
}

int newSliderValueForX (int x, menuslider_s *s)
{
	float newValue;
	int newValueInt;
	int pos = x - (MENU_FONT_SIZE + RCOLUMN_OFFSET + s->generic.x) - s->generic.parent->x;

	newValue = ((float)pos)/((SLIDER_RANGE-1)*(MENU_FONT_SIZE));
	newValueInt = s->minvalue + newValue * (float)( s->maxvalue - s->minvalue );

	return newValueInt;
}

void Slider_CheckSlide( menuslider_s *s )
{
	if ( s->curvalue > s->maxvalue )
		s->curvalue = s->maxvalue;
	else if ( s->curvalue < s->minvalue )
		s->curvalue = s->minvalue;

	if ( s->generic.callback )
		s->generic.callback( s );
}

void Menu_DragSlideItem (menuframework_s *menu, void *menuitem)
{
	menucommon_s *item = ( menucommon_s * ) menuitem;
	menuslider_s *slider = ( menuslider_s * ) menuitem;

	slider->curvalue = newSliderValueForX(cursor.x, slider);
	Slider_CheckSlide ( slider );
}

void Menu_ClickSlideItem (menuframework_s *menu, void *menuitem)
{
	int min, max;
	menucommon_s *item = ( menucommon_s * ) menuitem;
	menuslider_s *slider = ( menuslider_s * ) menuitem;

	min = menu->x + (item->x + Slider_CursorPositionX(slider) - 4);
	max = menu->x + (item->x + Slider_CursorPositionX(slider) + 4);

	if (cursor.x < min)
		Menu_SlideItem( menu, -1 );
	if (cursor.x > max)
		Menu_SlideItem( menu, 1 );
}

void M_Think_MouseCursor (void)
{
	char * sound = NULL;
	menuframework_s *m = (menuframework_s *)cursor.menu;

	if (m_drawfunc == M_Main_Draw) //have to hack for main menu :p
	{
		CheckMainMenuMouse();
		return;
	}
	if (m_drawfunc == M_Credits_MenuDraw) //have to hack for credits :p
	{
		if (cursor.buttonclicks[MOUSEBUTTON2])
		{
			cursor.buttonused[MOUSEBUTTON2] = true;
			cursor.buttonclicks[MOUSEBUTTON2] = 0;
			cursor.buttonused[MOUSEBUTTON1] = true;
			cursor.buttonclicks[MOUSEBUTTON1] = 0;
			S_StartLocalSound( menu_out_sound );
			if (creditsBuffer)
				FS_FreeFile (creditsBuffer);
			M_PopMenu();
			return;
		}
	}

	if (!m)
		return;

	//Exit with double click 2nd mouse button

	if (cursor.menuitem)
	{
		//MOUSE1
		if (cursor.buttondown[MOUSEBUTTON1])
		{
			if (cursor.menuitemtype == MENUITEM_SLIDER)
			{
				Menu_DragSlideItem(m, cursor.menuitem);
			}
			else if (!cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1])
			{
				if (cursor.menuitemtype == MENUITEM_ROTATE)
				{
					Menu_SlideItem( m, 1 );

					sound = menu_move_sound;
					cursor.buttonused[MOUSEBUTTON1] = true;
				}
				else
				{
					cursor.buttonused[MOUSEBUTTON1] = true;
					Menu_MouseSelectItem( cursor.menuitem );
					sound = menu_move_sound;
				}
			}
		}
		//MOUSE2
		if (cursor.buttondown[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2])
		{
			if (cursor.menuitemtype == MENUITEM_SLIDER && !cursor.buttonused[MOUSEBUTTON2])
			{
				Menu_ClickSlideItem(m, cursor.menuitem);
				sound = menu_move_sound;
				cursor.buttonused[MOUSEBUTTON2] = true;
			}
			else if (!cursor.buttonused[MOUSEBUTTON2])
			{
				if (cursor.menuitemtype == MENUITEM_ROTATE)
				{
					Menu_SlideItem( m, -1 );

					sound = menu_move_sound;
					cursor.buttonused[MOUSEBUTTON2] = true;
				}
			}
		}

		if(hover_time == 0) {
			sound = menu_move_sound;
			hover_time = 1;
		}

	}
	else if (!cursor.buttonused[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2]==2 && cursor.buttondown[MOUSEBUTTON2])
	{
		if (m_drawfunc==PlayerConfig_MenuDraw)
			PConfigAccept();

		if (m_drawfunc==Options_MenuDraw)
		{
			Cvar_SetValue( "options_menu", 0 );
			refreshCursorLink();
			M_PopMenu();
		}
		else
			M_PopMenu();

		sound = menu_out_sound;
		cursor.buttonused[MOUSEBUTTON2] = true;
		cursor.buttonclicks[MOUSEBUTTON2] = 0;
		cursor.buttonused[MOUSEBUTTON1] = true;
		cursor.buttonclicks[MOUSEBUTTON1] = 0;
	}

	else
		hover_time = 0;

	if ( sound )
		S_StartLocalSound( sound );
}

void M_Draw_Cursor (void)
{
	int w,h;

	//get sizing vars
	Draw_GetPicSize( &w, &h, "m_mouse_cursor" );
	Draw_StretchPic (cursor.x-w/2, cursor.y-h/2, w, h, "m_mouse_cursor");
}

/*
=================
M_Draw
=================
*/
void M_Draw (void)
{
	if (cls.key_dest != key_menu)
		return;

	// repaint everything next frame
	SCR_DirtyScreen ();

	// dim everything behind it down
	if (cl.cinematictime > 0)
		Draw_Fill (0,0,viddef.width, viddef.height, 0);
	else
		Draw_FadeScreen ();

	// Knigthmare- added Psychospaz's mouse support
	refreshCursorMenu();

	m_drawfunc ();

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		S_StartLocalSound( menu_in_sound );
		m_entersound = false;
	}

	// Knigthmare- added Psychospaz's mouse support
	//menu cursor for mouse usage :)
	M_Draw_Cursor();

}


/*
=================
M_Keydown
=================
*/
void M_Keydown (int key)
{
	const char *s;

	if (m_keyfunc)
		if ( ( s = m_keyfunc( key ) ) != 0 )
			S_StartLocalSound( ( char * ) s );
}


