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

#include <ctype.h>
#if defined WIN32_VARIANT
#include <winsock.h>
#endif

#if defined UNIX_VARIANT
#include <sys/time.h>
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

#if defined WIN32_VARIANT
#include <io.h>
#endif

#include "client.h"
#include "client/qmenu.h"

#if !defined HAVE__STRDUP
#if defined HAVE_STRDUP
#define _strdup strdup
#endif
#endif

static int	m_main_cursor;

extern void RS_LoadScript(char *script);
extern void RS_LoadSpecialScripts(void);
extern void RS_ScanPathForScripts(void);
extern cvar_t *scriptsloaded;
extern char map_music[128];
extern cvar_t *background_music;
extern cvar_t *background_music_vol;
extern cvar_t *dedicated;
extern cvar_t *cl_drawfps;
extern cvar_t *cl_drawtimer;
extern cvar_t *fov;

static char *menu_in_sound		= "misc/menu1.wav";
static char *menu_move_sound	= "misc/menu2.wav";
static char *menu_out_sound		= "misc/menu3.wav";
// static char *menu_background	= "misc/menuback.wav"; // unused
int svridx;
int playeridx;
int hover_time;
float mappicalpha;
float banneralpha;
float mainalpha;
int montagepic = 1;
int pNameUnique;

void M_Menu_Main_f (void);
	void M_Menu_PlayerConfig_f (void);
	void M_Menu_Game_f (void);
		void M_Menu_Credits_f( void );
	void M_Menu_JoinServer_f (void);
			void M_Menu_AddressBook_f( void );
			void M_Menu_PlayerRanking_f( void );
	void M_Menu_StartServer_f (void);
			void M_Menu_DMOptions_f (void);
	void M_Menu_IRC_f (void);
			void M_Menu_IRCSettings_f (void);
	void M_Menu_Video_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
	void M_Menu_Quit_f (void);

	void M_Menu_Credits( void );

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound

void	(*m_drawfunc) (void);
const char *(*m_keyfunc) (int key);

static size_t szr; // just for unused result warnings

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

static void M_Banner( char *name, float alpha )
{
	int w, h;
	float scale;

	scale = (float)(viddef.height)/600;

	Draw_GetPicSize (&w, &h, name );

	w*=scale;
	h*=scale;

	Draw_AlphaStretchPic( viddef.width / 2 - (w / 2), viddef.height / 2 - 260*scale, w, h, name, alpha );

}
static void M_MapPic( char *name, float alpha )
{
	int w, h;
	float scale;

	scale = (float)(viddef.height)/600;

	w = h = 128*scale;
	Draw_AlphaStretchPic (viddef.width / 2 - w + 240*scale, viddef.height / 2 - 140*scale, w, h, name, alpha);
}
static void M_MontagePic( char *name, float alpha )
{
	Draw_AlphaStretchPic (0, 0, viddef.width, viddef.height, name, alpha);
}
static void M_CrosshairPic( char *name )
{
	int w, h;
	float scale;

	scale = (float)(viddef.height)/600;

	w = h = 64*scale;
	Draw_StretchPic (viddef.width / 2 - w/2 - 120*scale, viddef.height / 2 + 64*scale, w, h, name);
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

	Draw_GetPicSize (&w, &h, "uparrow" );
	Draw_GetPicSize (&w, &h, "dnarrow" );

	//for the server list
	Draw_StretchPic (viddef.width / 2 - w/2 + (int)(382.5*scale), viddef.height / 2 - 205*scale, 32*scale, 32*scale, "uparrow");
	Draw_StretchPic (viddef.width / 2 - w/2 + (int)(382.5*scale), viddef.height / 2 - 53*scale, 32*scale, 32*scale, "dnarrow");

	//for the player list
	Draw_StretchPic (viddef.width / 2 - w/2 + (int)(147.5*scale), viddef.height / 2 + 153*scale, 32*scale, 32*scale, "uparrow");
	Draw_StretchPic (viddef.width / 2 - w/2 + (int)(147.5*scale), viddef.height / 2 + 243*scale, 32*scale, 32*scale, "dnarrow");
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
	S_StartMapMusic();
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
	case K_MOUSE4:
	case K_MOUSE5:
	case K_MOUSE6:
	case K_MOUSE7:
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

	charscale = (float)(viddef.height)*16/600;

	Draw_ScaledChar ( cx + viddef.width/3 - 3*charscale, cy + viddef.height/3 - 3*charscale, num, charscale, true);
}

void M_Print (int cx, int cy, char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str)+128);
		str++;
		cx += 16;
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

	charscale = (float)(viddef.height)*16/600;

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
#define	MAIN_ITEMS	8

char *main_names[] =
{
	"m_main_player",
	"m_main_game",
	"m_main_join",
	"m_main_host",
	"m_main_irc",
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
	char montagepicname[16];
	char backgroundpic[16];

	scale = (float)(viddef.height)/600;

	widscale = (float)(viddef.width)/800;
	if(widscale<1)
		widscale = 1;

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);

	ystart = ( viddef.height / 2 - 25*scale );
	xoffset = ( viddef.width - widest - 25*scale) / 2;

	M_Background("m_main");

	//draw the montage pics
	mainalpha += cls.frametime; //fade image in
	if(mainalpha > 4) { //switch pics at this point
		mainalpha = 0.1;
		montagepic++;
		if(montagepic > 5) {
			montagepic = 1;
		}
	}
	sprintf(backgroundpic, "m_main_mont%i", (montagepic==1)?5:montagepic-1);
	sprintf(montagepicname, "m_main_mont%i", montagepic);
	M_Background(backgroundpic);
	M_MontagePic(montagepicname, mainalpha);

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
	if(!strcmp(litname, "m_main_player_sel"))
		i = 0;
	if(!strcmp(litname, "m_main_game_sel"))
		i = 1;
	else if(!strcmp(litname, "m_main_join_sel"))
		i = 2;
	else if(!strcmp(litname, "m_main_host_sel"))
		i = 3;
	else if(!strcmp(litname, "m_main_irc_sel"))
		i = 4;
	else if(!strcmp(litname, "m_main_options_sel"))
		i = 5;
	else if(!strcmp(litname, "m_main_video_sel"))
		i = 6;
	else if(!strcmp(litname, "m_main_quit_sel"))
		i = 7;
	Draw_StretchPic( xoffset + 100*scale + (20*i*scale), (int)(ystart + m_main_cursor * 32.5*scale + 13*scale), w*scale, h*scale, litname );
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
		thisObj->OpenMenu = M_Menu_PlayerConfig_f;
	case 1:
		thisObj->OpenMenu = M_Menu_Game_f;
	case 2:
		thisObj->OpenMenu = M_Menu_JoinServer_f;
	case 3:
		thisObj->OpenMenu = M_Menu_StartServer_f;
	case 4:
		thisObj->OpenMenu = M_Menu_IRC_f;
	case 5:
		thisObj->OpenMenu = M_Menu_Options_f;
	case 6:
		thisObj->OpenMenu = M_Menu_Video_f;
	case 7:
		thisObj->OpenMenu = M_Menu_Quit_f;
	}
}

void openMenuFromMain (void)
{
	switch (m_main_cursor)
	{
		case 0:
			M_Menu_PlayerConfig_f();
			break;
		case 1:
			M_Menu_Game_f ();
			break;

		case 2:
			M_Menu_JoinServer_f();
			break;

		case 3:
			M_Menu_StartServer_f();
			break;

		case 4:
			M_Menu_IRC_f();
			break;

		case 5:
			M_Menu_Options_f ();
			break;

		case 6:
			M_Menu_Video_f ();
			break;

		case 7:
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
			M_Menu_PlayerConfig_f();
			break;
		case 1:
			M_Menu_Game_f ();
			break;

		case 2:
			M_Menu_JoinServer_f();
			break;

		case 3:
			M_Menu_StartServer_f();
			break;

		case 4:
			M_Menu_IRC_f();
			break;

		case 5:
			M_Menu_Options_f ();
			break;

		case 6:
			M_Menu_Video_f ();
			break;

		case 7:
			M_Menu_Quit_f ();
			break;
		}
	}

	return NULL;
}


void M_Menu_Main_f (void)
{
	S_StartMenuMusic();
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
{"+speed", 			"run"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
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
{"use grapple",			"grapple hook"},
{"use sproing",			"sproing"},
{"use haste",			"haste"},
{"use invisibility",	"invisibility"},

{"vtaunt 1",			"voice taunt #1"},
{"vtaunt 2",			"voice taunt #2"},
{"vtaunt 3",			"voice taunt #3"},
{"vtaunt 4",			"voice taunt #4"},
{"vtaunt 5",			"voice taunt #5"},
{"vtaunt 0",			"voice taunt auto"},

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
static menuaction_s		s_keys_run_action;
static menuaction_s		s_keys_step_left_action;
static menuaction_s		s_keys_step_right_action;
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
static menuaction_s		s_keys_grapple_action;
static menuaction_s		s_keys_violator_action;
static menuaction_s		s_keys_sproing_action;
static menuaction_s		s_keys_haste_action;
static menuaction_s		s_keys_invis_action;
static menuaction_s		s_keys_vtaunt1_action;
static menuaction_s		s_keys_vtaunt2_action;
static menuaction_s		s_keys_vtaunt3_action;
static menuaction_s		s_keys_vtaunt4_action;
static menuaction_s		s_keys_vtaunt5_action;
static menuaction_s		s_keys_vtauntauto_action;
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

	float charscale = (float)(viddef.height)/600*16;

	M_FindKeysForCommand( bindnames[a->generic.localdata[0]][0], keys);

	if (keys[0] == -1)
	{
		Menu_DrawString( a->generic.x + a->generic.parent->x + charscale, a->generic.y + a->generic.parent->y, "???" );
	}
	else
	{
		int x;
		const char *name;

		name = Key_KeynumToString (keys[0]);

		Menu_DrawString( a->generic.x + a->generic.parent->x + charscale, a->generic.y + a->generic.parent->y, name );

		x = strlen(name) * charscale;

		if (keys[1] != -1)
		{
			Menu_DrawString( a->generic.x + a->generic.parent->x + 1*charscale + x, a->generic.y + a->generic.parent->y, "or" );
			Menu_DrawString( a->generic.x + a->generic.parent->x + 3*charscale + x, a->generic.y + a->generic.parent->y, Key_KeynumToString (keys[1]) );
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

	y = 80*scale;

	banneralpha = .1;

	s_keys_menu.x = viddef.width*.50 - 20*scale;

	s_keys_attack_action.generic.type	= MTYPE_ACTION;
	s_keys_attack_action.generic.x		= 0;
	s_keys_attack_action.generic.y		= y;
	s_keys_attack_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_attack_action.generic.localdata[0] = i;
	s_keys_attack_action.generic.name	= bindnames[s_keys_attack_action.generic.localdata[0]][1];

	s_keys_attack2_action.generic.type	= MTYPE_ACTION;
	s_keys_attack2_action.generic.x		= 0;
	s_keys_attack2_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_attack2_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_attack2_action.generic.localdata[0] = ++i;
	s_keys_attack2_action.generic.name	= bindnames[s_keys_attack2_action.generic.localdata[0]][1];

	s_keys_change_weapon_action.generic.type	= MTYPE_ACTION;
	s_keys_change_weapon_action.generic.x		= 0;
	s_keys_change_weapon_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_change_weapon_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_change_weapon_action.generic.localdata[0] = ++i;
	s_keys_change_weapon_action.generic.name	= bindnames[s_keys_change_weapon_action.generic.localdata[0]][1];

	s_keys_walk_forward_action.generic.type	= MTYPE_ACTION;
	s_keys_walk_forward_action.generic.x		= 0;
	s_keys_walk_forward_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_walk_forward_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_walk_forward_action.generic.localdata[0] = ++i;
	s_keys_walk_forward_action.generic.name	= bindnames[s_keys_walk_forward_action.generic.localdata[0]][1];

	s_keys_backpedal_action.generic.type	= MTYPE_ACTION;
	s_keys_backpedal_action.generic.x		= 0;
	s_keys_backpedal_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_backpedal_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_backpedal_action.generic.localdata[0] = ++i;
	s_keys_backpedal_action.generic.name	= bindnames[s_keys_backpedal_action.generic.localdata[0]][1];

	s_keys_run_action.generic.type	= MTYPE_ACTION;
	s_keys_run_action.generic.x		= 0;
	s_keys_run_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_run_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_run_action.generic.localdata[0] = ++i;
	s_keys_run_action.generic.name	= bindnames[s_keys_run_action.generic.localdata[0]][1];

	s_keys_step_left_action.generic.type	= MTYPE_ACTION;
	s_keys_step_left_action.generic.x		= 0;
	s_keys_step_left_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_step_left_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_left_action.generic.localdata[0] = ++i;
	s_keys_step_left_action.generic.name	= bindnames[s_keys_step_left_action.generic.localdata[0]][1];

	s_keys_step_right_action.generic.type	= MTYPE_ACTION;
	s_keys_step_right_action.generic.x		= 0;
	s_keys_step_right_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_step_right_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_step_right_action.generic.localdata[0] = ++i;
	s_keys_step_right_action.generic.name	= bindnames[s_keys_step_right_action.generic.localdata[0]][1];

	s_keys_move_up_action.generic.type	= MTYPE_ACTION;
	s_keys_move_up_action.generic.x		= 0;
	s_keys_move_up_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_move_up_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_up_action.generic.localdata[0] = ++i;
	s_keys_move_up_action.generic.name	= bindnames[s_keys_move_up_action.generic.localdata[0]][1];

	s_keys_move_down_action.generic.type	= MTYPE_ACTION;
	s_keys_move_down_action.generic.x		= 0;
	s_keys_move_down_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_move_down_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_move_down_action.generic.localdata[0] = ++i;
	s_keys_move_down_action.generic.name	= bindnames[s_keys_move_down_action.generic.localdata[0]][1];

	s_keys_inventory_action.generic.type	= MTYPE_ACTION;
	s_keys_inventory_action.generic.x		= 0;
	s_keys_inventory_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_inventory_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inventory_action.generic.localdata[0] = ++i;
	s_keys_inventory_action.generic.name	= bindnames[s_keys_inventory_action.generic.localdata[0]][1];

	s_keys_inv_use_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_use_action.generic.x		= 0;
	s_keys_inv_use_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_inv_use_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_use_action.generic.localdata[0] = ++i;
	s_keys_inv_use_action.generic.name	= bindnames[s_keys_inv_use_action.generic.localdata[0]][1];

	s_keys_inv_drop_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_drop_action.generic.x		= 0;
	s_keys_inv_drop_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_inv_drop_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_drop_action.generic.localdata[0] = ++i;
	s_keys_inv_drop_action.generic.name	= bindnames[s_keys_inv_drop_action.generic.localdata[0]][1];

	s_keys_inv_prev_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_prev_action.generic.x		= 0;
	s_keys_inv_prev_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_inv_prev_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_prev_action.generic.localdata[0] = ++i;
	s_keys_inv_prev_action.generic.name	= bindnames[s_keys_inv_prev_action.generic.localdata[0]][1];

	s_keys_inv_next_action.generic.type	= MTYPE_ACTION;
	s_keys_inv_next_action.generic.x		= 0;
	s_keys_inv_next_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_inv_next_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_inv_next_action.generic.localdata[0] = ++i;
	s_keys_inv_next_action.generic.name	= bindnames[s_keys_inv_next_action.generic.localdata[0]][1];

	s_keys_alien_disruptor_action.generic.type	= MTYPE_ACTION;
	s_keys_alien_disruptor_action.generic.x		= 0;
	s_keys_alien_disruptor_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_alien_disruptor_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_alien_disruptor_action.generic.localdata[0] = ++i;
	s_keys_alien_disruptor_action.generic.name	= bindnames[s_keys_alien_disruptor_action.generic.localdata[0]][1];

	s_keys_chain_pistol_action.generic.type	= MTYPE_ACTION;
	s_keys_chain_pistol_action.generic.x		= 0;
	s_keys_chain_pistol_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_chain_pistol_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_chain_pistol_action.generic.localdata[0] = ++i;
	s_keys_chain_pistol_action.generic.name	= bindnames[s_keys_chain_pistol_action.generic.localdata[0]][1];

	s_keys_flame_thrower_action.generic.type	= MTYPE_ACTION;
	s_keys_flame_thrower_action.generic.x		= 0;
	s_keys_flame_thrower_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_flame_thrower_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_flame_thrower_action.generic.localdata[0] = ++i;
	s_keys_flame_thrower_action.generic.name	= bindnames[s_keys_flame_thrower_action.generic.localdata[0]][1];

	s_keys_rocket_launcher_action.generic.type	= MTYPE_ACTION;
	s_keys_rocket_launcher_action.generic.x		= 0;
	s_keys_rocket_launcher_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_rocket_launcher_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_rocket_launcher_action.generic.localdata[0] = ++i;
	s_keys_rocket_launcher_action.generic.name	= bindnames[s_keys_rocket_launcher_action.generic.localdata[0]][1];

	s_keys_alien_smartgun_action.generic.type	= MTYPE_ACTION;
	s_keys_alien_smartgun_action.generic.x		= 0;
	s_keys_alien_smartgun_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_alien_smartgun_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_alien_smartgun_action.generic.localdata[0] = ++i;
	s_keys_alien_smartgun_action.generic.name	= bindnames[s_keys_alien_smartgun_action.generic.localdata[0]][1];

	s_keys_alien_beamgun_action.generic.type	= MTYPE_ACTION;
	s_keys_alien_beamgun_action.generic.x		= 0;
	s_keys_alien_beamgun_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_alien_beamgun_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_alien_beamgun_action.generic.localdata[0] = ++i;
	s_keys_alien_beamgun_action.generic.name	= bindnames[s_keys_alien_beamgun_action.generic.localdata[0]][1];

	s_keys_alien_vaporizer_action.generic.type	= MTYPE_ACTION;
	s_keys_alien_vaporizer_action.generic.x		= 0;
	s_keys_alien_vaporizer_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_alien_vaporizer_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_alien_vaporizer_action.generic.localdata[0] = ++i;
	s_keys_alien_vaporizer_action.generic.name	= bindnames[s_keys_alien_vaporizer_action.generic.localdata[0]][1];

	s_keys_violator_action.generic.type	= MTYPE_ACTION;
	s_keys_violator_action.generic.x		= 0;
	s_keys_violator_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_violator_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_violator_action.generic.localdata[0] = ++i;
	s_keys_violator_action.generic.name	= bindnames[s_keys_violator_action.generic.localdata[0]][1];

	s_keys_show_scores_action.generic.type	= MTYPE_ACTION;
	s_keys_show_scores_action.generic.x		= 0;
	s_keys_show_scores_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_show_scores_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_show_scores_action.generic.localdata[0] = ++i;
	s_keys_show_scores_action.generic.name	= bindnames[s_keys_show_scores_action.generic.localdata[0]][1];

	s_keys_grapple_action.generic.type	= MTYPE_ACTION;
	s_keys_grapple_action.generic.x		= 0;
	s_keys_grapple_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_grapple_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_grapple_action.generic.localdata[0] = ++i;
	s_keys_grapple_action.generic.name	= bindnames[s_keys_grapple_action.generic.localdata[0]][1];

	s_keys_sproing_action.generic.type	= MTYPE_ACTION;
	s_keys_sproing_action.generic.x		= 0;
	s_keys_sproing_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_sproing_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_sproing_action.generic.localdata[0] = ++i;
	s_keys_sproing_action.generic.name	= bindnames[s_keys_sproing_action.generic.localdata[0]][1];

	s_keys_haste_action.generic.type	= MTYPE_ACTION;
	s_keys_haste_action.generic.x		= 0;
	s_keys_haste_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_haste_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_haste_action.generic.localdata[0] = ++i;
	s_keys_haste_action.generic.name	= bindnames[s_keys_haste_action.generic.localdata[0]][1];

	s_keys_invis_action.generic.type	= MTYPE_ACTION;
	s_keys_invis_action.generic.x		= 0;
	s_keys_invis_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_invis_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_invis_action.generic.localdata[0] = ++i;
	s_keys_invis_action.generic.name	= bindnames[s_keys_invis_action.generic.localdata[0]][1];

	s_keys_vtaunt1_action.generic.type	= MTYPE_ACTION;
	s_keys_vtaunt1_action.generic.x		= 0;
	s_keys_vtaunt1_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_vtaunt1_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_vtaunt1_action.generic.localdata[0] = ++i;
	s_keys_vtaunt1_action.generic.name	= bindnames[s_keys_vtaunt1_action.generic.localdata[0]][1];

	s_keys_vtaunt2_action.generic.type	= MTYPE_ACTION;
	s_keys_vtaunt2_action.generic.x		= 0;
	s_keys_vtaunt2_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_vtaunt2_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_vtaunt2_action.generic.localdata[0] = ++i;
	s_keys_vtaunt2_action.generic.name	= bindnames[s_keys_vtaunt2_action.generic.localdata[0]][1];

	s_keys_vtaunt3_action.generic.type	= MTYPE_ACTION;
	s_keys_vtaunt3_action.generic.x		= 0;
	s_keys_vtaunt3_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_vtaunt3_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_vtaunt3_action.generic.localdata[0] = ++i;
	s_keys_vtaunt3_action.generic.name	= bindnames[s_keys_vtaunt3_action.generic.localdata[0]][1];

	s_keys_vtaunt4_action.generic.type	= MTYPE_ACTION;
	s_keys_vtaunt4_action.generic.x		= 0;
	s_keys_vtaunt4_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_vtaunt4_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_vtaunt4_action.generic.localdata[0] = ++i;
	s_keys_vtaunt4_action.generic.name	= bindnames[s_keys_vtaunt4_action.generic.localdata[0]][1];

	s_keys_vtaunt5_action.generic.type	= MTYPE_ACTION;
	s_keys_vtaunt5_action.generic.x		= 0;
	s_keys_vtaunt5_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_vtaunt5_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_vtaunt5_action.generic.localdata[0] = ++i;
	s_keys_vtaunt5_action.generic.name	= bindnames[s_keys_vtaunt5_action.generic.localdata[0]][1];

	s_keys_vtauntauto_action.generic.type	= MTYPE_ACTION;
	s_keys_vtauntauto_action.generic.x		= 0;
	s_keys_vtauntauto_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_vtauntauto_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_vtauntauto_action.generic.localdata[0] = ++i;
	s_keys_vtauntauto_action.generic.name	= bindnames[s_keys_vtauntauto_action.generic.localdata[0]][1];

	s_keys_filler_action.generic.type	= MTYPE_ACTION;
	s_keys_filler_action.generic.x		= 0;
	s_keys_filler_action.generic.y		= y += FONTSCALE*9*scale;
	s_keys_filler_action.generic.ownerdraw = DrawKeyBindingFunc;
	s_keys_filler_action.generic.localdata[0] = ++i;
	s_keys_filler_action.generic.name	= bindnames[s_keys_filler_action.generic.localdata[0]][1];

	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_attack_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_attack2_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_change_weapon_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_walk_forward_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_backpedal_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_run_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_step_left_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_step_right_action );
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
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_grapple_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_sproing_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_haste_action );
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_invis_action );

	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_vtaunt1_action);
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_vtaunt2_action);
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_vtaunt3_action);
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_vtaunt4_action);
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_vtaunt5_action);
	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_vtauntauto_action);

	Menu_AddItem( &s_keys_menu, ( void * ) &s_keys_filler_action ); //needed so last item will show

	Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
}

static void Keys_MenuDraw (void)
{
	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back"); //draw black background first
	M_Banner( "m_controls", banneralpha);
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

OPTIONS MENU

=======================================================================
*/
static cvar_t *win_noalttab;
extern cvar_t *in_joystick;
extern cvar_t *cl_showPlayerNames;
extern cvar_t *r_ragdolls;
extern cvar_t *cl_noblood;
extern cvar_t *cl_noskins;
extern cvar_t *cl_precachecustom;
extern cvar_t *r_minimap;
extern cvar_t *r_minimap_style;

static menuframework_s	s_options_menu;
static menuaction_s		s_options_defaults_action;
static menuaction_s		s_options_customize_options_action;
static menulist_s		s_options_mouse_accel_box;
static menuslider_s		s_options_sensitivity_slider;
static menuslider_s		s_options_menu_sensitivity_slider;
static menulist_s		s_options_smoothing_box;
static menulist_s		s_options_freelook_box;
static menulist_s		s_options_noalttab_box;
static menulist_s		s_options_alwaysrun_box;
static menulist_s		s_options_invertmouse_box;
static menulist_s		s_options_font_box;
static menulist_s		s_options_crosshair_box;
static menulist_s		s_options_hud_box;
static menulist_s		s_options_discolor_box;
static menuslider_s		s_options_sfxvolume_slider;
static menuslider_s		s_options_bgvolume_slider;
static menulist_s		s_options_joystick_box;
static menulist_s		s_options_doppler_effect_list;
static menulist_s		s_options_console_action;
static menulist_s		s_options_bgmusic_box;
static menulist_s		s_options_target_box;
static menulist_s		s_options_ragdoll_box;
static menulist_s		s_options_noblood_box;
static menulist_s		s_options_noskins_box;
static menulist_s		s_options_taunts_box;
static menulist_s		s_options_precachecustom_box;
static menulist_s		s_options_minimap_box;
static menulist_s		s_options_showfps_box;
static menulist_s		s_options_showtime_box;
static menulist_s		s_options_paindist_box;
static menulist_s		s_options_explosiondist_box;

static void TargetFunc( void *unused )
{
	Cvar_SetValue( "cl_showplayernames", s_options_target_box.curvalue);
}

static void RagDollFunc( void *unused )
{
	Cvar_SetValue( "r_ragdolls", s_options_ragdoll_box.curvalue);
}

static void NoBloodFunc( void *unused )
{
	Cvar_SetValue( "cl_noblood", s_options_noblood_box.curvalue);
}

static void NoskinsFunc( void *unused )
{
	Cvar_SetValue( "cl_noskins", s_options_noskins_box.curvalue);
}

static void TauntsFunc( void *unused )
{
	Cvar_SetValue( "cl_playtaunts", s_options_taunts_box.curvalue);
}

static void PrecacheFunc( void *unused )
{
	Cvar_SetValue( "cl_precachecustom", s_options_precachecustom_box.curvalue);
}

static void PainDistFunc( void *unused )
{
	Cvar_SetValue( "cl_paindist", s_options_paindist_box.curvalue);
}

static void ExplosionDistFunc( void *unused )
{
	Cvar_SetValue( "cl_explosiondist", s_options_explosiondist_box.curvalue);
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

/*
// unused
static void FreeLookFunc( void *unused )
{
	Cvar_SetValue( "freelook", s_options_freelook_box.curvalue );
}
*/

static void DisColorFunc( void *unused )
{
	Cvar_SetValue( "cl_disbeamclr", s_options_discolor_box.curvalue );
}

static void MouseAccelFunc( void *unused )
{
	Cvar_SetValue( "m_accel", s_options_mouse_accel_box.curvalue);
}

static void MouseSpeedFunc( void *unused )
{
	Cvar_SetValue( "sensitivity", s_options_sensitivity_slider.curvalue / 2.0F );
}

static void MenuMouseSpeedFunc( void *unused )
{
	Cvar_SetValue( "menu_sensitivity", s_options_menu_sensitivity_slider.curvalue / 2.0F );
}

static void MouseSmoothingFunc( void *unused )
{
	Cvar_SetValue( "m_smoothing", s_options_smoothing_box.curvalue );
}

#if !defined UNIX_VARIANT
static void NoAltTabFunc( void *unused )
{
	Cvar_SetValue( "win_noalttab", s_options_noalttab_box.curvalue );
}
#endif

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

static void ShowfpsFunc( void *unused )
{
	Cvar_SetValue( "cl_drawfps", s_options_showfps_box.curvalue);
}

static void ShowtimeFunc( void *unused )
{
	Cvar_SetValue( "cl_drawtimer", s_options_showtime_box.curvalue);
}

static float ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

extern cvar_t *con_font;
#define MAX_FONTS 32
char **font_names = NULL;
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

			list[i] = _strdup(insert);

			return;
		}
	}

	list[len] = _strdup(insert);
}

char **SetFontNames (void)
{
	char *curFont;
	char **list = 0, *p;
	int nfonts = 0, nfontnames;
	char **fontfiles;
	int i;

	list = malloc( sizeof( char * ) * MAX_FONTS );
	memset( list, 0, sizeof( char * ) * MAX_FONTS );

	list[0] = _strdup("default");

	nfontnames = 1;

	fontfiles = FS_ListFilesInFS( "fonts/*.tga", &nfonts, 0,
	    SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<nfonts && nfontnames<MAX_FONTS;i++)
	{
		int num;

		p = strstr(fontfiles[i], "fonts/"); p++;
		p = strstr(p, "/"); p++;

		if (!strstr(p, ".tga") && !strstr(p, ".pcx"))
			continue;

		num = strlen(p)-4;
		p[num] = 0;

		curFont = p;

		if (!fontInList(curFont, nfontnames, list))
		{
			insertFont(list, _strdup(curFont),nfontnames);
			nfontnames++;
		}

		//set back so whole string get deleted.
		p[num] = '.';
	}

	if (fontfiles)
		FS_FreeFileList(fontfiles, nfonts);

	numfonts = nfontnames;

	return list;
}

extern cvar_t *crosshair;
#define MAX_CROSSHAIRS 256
char **crosshair_names = NULL;
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

			list[i] = _strdup(insert);

			return;
		}
	}

	list[len] = _strdup(insert);
}

char **SetCrosshairNames (void)
{
	char *curCrosshair;
	char **list = 0, *p;
	int ncrosshairs = 0, ncrosshairnames;
	char **crosshairfiles;
	int i;

	list = malloc( sizeof( char * ) * MAX_CROSSHAIRS );
	memset( list, 0, sizeof( char * ) * MAX_CROSSHAIRS );

	ncrosshairnames = 4;

	list[0] = _strdup("none"); //the old crosshairs
	list[1] = _strdup("ch1");
	list[2] = _strdup("ch2");
	list[3] = _strdup("ch3");

	crosshairfiles = FS_ListFilesInFS( "pics/crosshairs/*.tga",
	    &ncrosshairs, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<ncrosshairs && ncrosshairnames<MAX_CROSSHAIRS;i++)
	{
		int num;

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
			insertCrosshair(list, _strdup(curCrosshair),ncrosshairnames);
			ncrosshairnames++;
		}

		//set back so whole string get deleted.
		p[num] = '.';
	}

	if (crosshairfiles)
		FS_FreeFileList(crosshairfiles, ncrosshairs);

	numcrosshairs = ncrosshairnames;

	return list;
}

extern cvar_t *cl_hudimage1;
extern cvar_t *cl_hudimage2;
#define MAX_HUDS 256
char **hud_names = NULL;
int	numhuds;

static void HudFunc( void *unused )
{
	char hud1[MAX_OSPATH];
	char hud2[MAX_OSPATH];

	if(s_options_hud_box.curvalue == 0) { //none
		sprintf(hud1, "none");
		sprintf(hud2, "none");
	}

	if(s_options_hud_box.curvalue == 1) {
		sprintf(hud1, "pics/i_health.tga");
		sprintf(hud2, "pics/i_score.tga");
	}

	if(s_options_hud_box.curvalue > 1) {
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

	s_options_hud_box.curvalue = 1;

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

	for (i=2;i<len; i++)
	{
		if (!list[i])
			break;

		if (strcmp( list[i], insert ))
		{
			for (j=len; j>i ;j--)
				list[j] = list[j-1];

			list[i] = _strdup(insert);

			return;
		}
	}

	list[len] = _strdup(insert);

}

char **SetHudNames (void)
{
	char *curHud;
	char **list = 0, *p;
	int nhuds = 0, nhudnames;
	char **hudfiles;
	int i;

	list = malloc( sizeof( char * ) * MAX_HUDS );
	memset( list, 0, sizeof( char * ) * MAX_HUDS );

	nhudnames = 2;

	list[0] = _strdup("none");
	list[1] = _strdup("default"); //the default hud

	hudfiles = FS_ListFilesInFS( "pics/huds/*.tga", &nhuds, 0,
	    SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<nhuds && nhudnames<MAX_HUDS;i++)
	{
		int num;

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
			insertHud(list, _strdup(curHud),nhudnames);
			nhudnames++;
		}

		//set back so whole string get deleted.
		p[num] = '.';
	}

	if (hudfiles)
		FS_FreeFileList(hudfiles, nhuds);

	numhuds = nhudnames;

	return list;
}

static void ControlsSetMenuItemValues( void )
{

	s_options_sfxvolume_slider.curvalue		= Cvar_VariableValue( "s_volume" ) * 10;
	s_options_bgvolume_slider.curvalue		= Cvar_VariableValue( "background_music_vol" ) * 10;
	s_options_bgmusic_box.curvalue			= Cvar_VariableValue("background_music");
	s_options_discolor_box.curvalue			= Cvar_VariableValue("cl_disbeamclr");

	if ( Cvar_VariableValue("s_doppler") == 5.0f ) // TODO: constant declarations would be better
		s_options_doppler_effect_list.curvalue = 3;
	else if ( Cvar_VariableValue("s_doppler") == 3.0f )
		s_options_doppler_effect_list.curvalue = 2;
	else if ( Cvar_VariableValue("s_doppler") == 1.0f )
		s_options_doppler_effect_list.curvalue = 1;
	else /* set to 0 for off  (or curvalue is invalid) */
		s_options_doppler_effect_list.curvalue = 0;

	Cvar_SetValue( "m_accel", ClampCvar( 0, 1, m_accel->value ) );
	s_options_mouse_accel_box.curvalue = m_accel->value;
	s_options_sensitivity_slider.curvalue	= ( sensitivity->value ) * 2;
	s_options_menu_sensitivity_slider.curvalue	= ( menu_sensitivity->value ) * 2;

	Cvar_SetValue("m_smoothing", ClampCvar(0, 1, m_smoothing->value ) );
	s_options_smoothing_box.curvalue		= m_smoothing->value;

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

	Cvar_SetValue("cl_showplayernames", ClampCvar(0, 2, cl_showPlayerNames->value ) );
	s_options_target_box.curvalue		= cl_showPlayerNames->value;

	Cvar_SetValue("cl_noskins", ClampCvar(0, 1, cl_noskins->value ) );
	s_options_noskins_box.curvalue		= cl_noskins->value;

	Cvar_SetValue("cl_playtaunts", ClampCvar(0, 1, cl_playtaunts->value ) );
	s_options_taunts_box.curvalue		= cl_playtaunts->value;

	Cvar_SetValue("cl_drawfps", ClampCvar(0, 1, cl_drawfps->value ) );
	s_options_showfps_box.curvalue		= cl_drawfps->value;

	Cvar_SetValue("cl_drawtimer", ClampCvar(0, 1, cl_drawtimer->value ) );
	s_options_showtime_box.curvalue		= cl_drawtimer->value;

	Cvar_SetValue("cl_precachecustom", ClampCvar(0, 2, cl_precachecustom->value ) );
	s_options_precachecustom_box.curvalue		= cl_precachecustom->value;

	Cvar_SetValue("cl_paindist", ClampCvar(0, 1, cl_paindist->value ) );
	s_options_paindist_box.curvalue		= cl_paindist->value;

	Cvar_SetValue("cl_explosiondist", ClampCvar(0, 1, cl_explosiondist->value ) );
	s_options_explosiondist_box.curvalue		= cl_explosiondist->value;

	Cvar_SetValue("r_ragdolls", ClampCvar(0, 1, r_ragdolls->value ) );
	s_options_ragdoll_box.curvalue		= r_ragdolls->value;

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

	CL_Snd_Restart_f();
	S_StartMenuMusic();

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
	if ( background_music->value > 0.99f && background_music_vol->value >= 0.1f )
	{
		S_StartMenuMusic();
	}
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
	CON_ClearNotify ();

	M_ForceMenuOff ();
	cls.key_dest = key_console;
}

static void UpdateDopplerEffectFunc( void *unused )
{
	if ( s_options_doppler_effect_list.curvalue == 3 )
	{
		Cvar_SetValue( "s_doppler", 5.0f ); // very high
	}
	else if ( s_options_doppler_effect_list.curvalue == 2 )
	{
		Cvar_SetValue( "s_doppler", 3.0f ); // high
	}
	else if ( s_options_doppler_effect_list.curvalue == 1 )
	{
		Cvar_SetValue( "s_doppler", 1.0f ); // normal
	}
	else if ( s_options_doppler_effect_list.curvalue == 0 )
	{
		Cvar_SetValue( "s_doppler", 0.0f ); // off
	}

	R_EndFrame(); // buffer swap needed to show text box
	S_UpdateDopplerFactor();

}

void Options_MenuInit( void )
{
	static const char *background_music_items[] =
	{
		"disabled",
		"enabled",
		0
	};

	static const char *doppler_effect_items[] =
	{
		"off",
		"normal",
		"high",
		"very high",
		0
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

	static const char *minimap_names[] =
	{
		"off",
		"static",
		"rotating",
		0
	};
	static const char *playerid_names[] =
	{
		"off",
		"centered",
		"over player",
		0
	};

	static const char *color_names[] =
	{
		"green",
		"blue",
		"red",
		"yellow",
		"purple",
		0
	};

	float scale;

	scale = (float)(viddef.height)/600;

	banneralpha = 0.1;

	win_noalttab = Cvar_Get( "win_noalttab", "0", CVAR_ARCHIVE );

	/*
	** configure controls menu and menu items
	*/
	s_options_menu.x = viddef.width / 2 + 55*scale;
	s_options_menu.y = viddef.height / 2 - 242*scale;
	s_options_menu.nitems = 0;

	s_options_customize_options_action.generic.type	= MTYPE_ACTION;
	s_options_customize_options_action.generic.x		= 0;
	s_options_customize_options_action.generic.y		= FONTSCALE*10*scale;
	s_options_customize_options_action.generic.name	= "customize controls";
	s_options_customize_options_action.generic.callback = CustomizeControlsFunc;

	s_options_precachecustom_box.generic.type = MTYPE_SPINCONTROL;
	s_options_precachecustom_box.generic.x	= 0;
	s_options_precachecustom_box.generic.y	= FONTSCALE*30*scale;
	s_options_precachecustom_box.generic.name	= "precache custom models";
	s_options_precachecustom_box.generic.callback = PrecacheFunc;
	s_options_precachecustom_box.itemnames = onoff_names;
	s_options_precachecustom_box.generic.statusbar = "Enabling this can result in slow map loading times";

	s_options_paindist_box.generic.type = MTYPE_SPINCONTROL;
	s_options_paindist_box.generic.x	= 0;
	s_options_paindist_box.generic.y	= FONTSCALE*40*scale;
	s_options_paindist_box.generic.name	= "pain distortion fx";
	s_options_paindist_box.generic.callback = PainDistFunc;
	s_options_paindist_box.itemnames = onoff_names;
	s_options_paindist_box.generic.statusbar = "GLSL must be enabled for this to take effect";

	s_options_explosiondist_box.generic.type = MTYPE_SPINCONTROL;
	s_options_explosiondist_box.generic.x	= 0;
	s_options_explosiondist_box.generic.y	= FONTSCALE*50*scale;
	s_options_explosiondist_box.generic.name	= "explosion distortion fx";
	s_options_explosiondist_box.generic.callback = ExplosionDistFunc;
	s_options_explosiondist_box.itemnames = onoff_names;
	s_options_explosiondist_box.generic.statusbar = "GLSL must be enabled for this to take effect";

	s_options_target_box.generic.type = MTYPE_SPINCONTROL;
	s_options_target_box.generic.x	= 0;
	s_options_target_box.generic.y	= FONTSCALE*60*scale;
	s_options_target_box.generic.name	= "identify target";
	s_options_target_box.generic.callback = TargetFunc;
	s_options_target_box.itemnames = playerid_names;

	s_options_ragdoll_box.generic.type = MTYPE_SPINCONTROL;
	s_options_ragdoll_box.generic.x	= 0;
	s_options_ragdoll_box.generic.y	= FONTSCALE*70*scale;
	s_options_ragdoll_box.generic.name	= "ragdolls";
	s_options_ragdoll_box.generic.callback = RagDollFunc;
	s_options_ragdoll_box.itemnames = onoff_names;

	s_options_noblood_box.generic.type = MTYPE_SPINCONTROL;
	s_options_noblood_box.generic.x	= 0;
	s_options_noblood_box.generic.y	= FONTSCALE*80*scale;
	s_options_noblood_box.generic.name	= "no blood";
	s_options_noblood_box.generic.callback = NoBloodFunc;
	s_options_noblood_box.itemnames = onoff_names;

	s_options_noskins_box.generic.type = MTYPE_SPINCONTROL;
	s_options_noskins_box.generic.x	= 0;
	s_options_noskins_box.generic.y	= FONTSCALE*90*scale;
	s_options_noskins_box.generic.name	= "force martian models";
	s_options_noskins_box.generic.callback = NoskinsFunc;
	s_options_noskins_box.itemnames = onoff_names;

	s_options_taunts_box.generic.type = MTYPE_SPINCONTROL;
	s_options_taunts_box.generic.x	= 0;
	s_options_taunts_box.generic.y	= FONTSCALE*100*scale;
	s_options_taunts_box.generic.name	= "player taunts";
	s_options_taunts_box.generic.callback = TauntsFunc;
	s_options_taunts_box.itemnames = onoff_names;

	s_options_sfxvolume_slider.generic.type	= MTYPE_SLIDER;
	s_options_sfxvolume_slider.generic.x	= 0;
	s_options_sfxvolume_slider.generic.y	= FONTSCALE*110*scale;
	s_options_sfxvolume_slider.generic.name	= "global volume";
	s_options_sfxvolume_slider.generic.callback	= UpdateVolumeFunc;
	s_options_sfxvolume_slider.minvalue		= 0;
	s_options_sfxvolume_slider.maxvalue		= 10;
	s_options_sfxvolume_slider.curvalue		= Cvar_VariableValue( "s_volume" ) * 10;

	s_options_bgvolume_slider.generic.type	= MTYPE_SLIDER;
	s_options_bgvolume_slider.generic.x	= 0;
	s_options_bgvolume_slider.generic.y	= FONTSCALE*120*scale;
	s_options_bgvolume_slider.generic.name	= "music volume";
	s_options_bgvolume_slider.generic.callback	= UpdateBGVolumeFunc;
	s_options_bgvolume_slider.minvalue		= 0;
	s_options_bgvolume_slider.maxvalue		= 10;
	s_options_bgvolume_slider.curvalue		= Cvar_VariableValue( "background_music_vol" ) * 10;

	s_options_bgmusic_box.generic.type	= MTYPE_SPINCONTROL;
	s_options_bgmusic_box.generic.x		= 0;
	s_options_bgmusic_box.generic.y		= FONTSCALE*130*scale;
	s_options_bgmusic_box.generic.name	= "Background music";
	s_options_bgmusic_box.generic.callback	= UpdateBGMusicFunc;
	s_options_bgmusic_box.itemnames		= background_music_items;
	s_options_bgmusic_box.curvalue 		= Cvar_VariableValue("background_music");

	s_options_doppler_effect_list.generic.type	= MTYPE_SPINCONTROL;
	s_options_doppler_effect_list.generic.x		= 0;
	s_options_doppler_effect_list.generic.y		= FONTSCALE*140*scale;
	s_options_doppler_effect_list.generic.name	= "doppler sound effect";
	s_options_doppler_effect_list.generic.callback = UpdateDopplerEffectFunc;
	s_options_doppler_effect_list.itemnames		= doppler_effect_items;
	s_options_doppler_effect_list.curvalue		= Cvar_VariableValue( "s_doppler" );

	s_options_mouse_accel_box.generic.type	= MTYPE_SPINCONTROL;
	s_options_mouse_accel_box.generic.x		= 0;
	s_options_mouse_accel_box.generic.y		= FONTSCALE*150*scale;
	s_options_mouse_accel_box.generic.name	= "mouse acceleration";
	s_options_mouse_accel_box.generic.callback = MouseAccelFunc;
	s_options_mouse_accel_box.itemnames		= yesno_names;
	s_options_mouse_accel_box.curvalue		= Cvar_VariableValue( "m_accel" );

	s_options_sensitivity_slider.generic.type	= MTYPE_SLIDER;
	s_options_sensitivity_slider.generic.x		= 0;
	s_options_sensitivity_slider.generic.y		= FONTSCALE*160*scale;
	s_options_sensitivity_slider.generic.name	= "mouse speed";
	s_options_sensitivity_slider.generic.callback = MouseSpeedFunc;
	s_options_sensitivity_slider.minvalue		= 2;
	s_options_sensitivity_slider.maxvalue		= 22;

	s_options_menu_sensitivity_slider.generic.type	= MTYPE_SLIDER;
	s_options_menu_sensitivity_slider.generic.x		= 0;
	s_options_menu_sensitivity_slider.generic.y		= FONTSCALE*170*scale;
	s_options_menu_sensitivity_slider.generic.name	= "menu mouse speed";
	s_options_menu_sensitivity_slider.generic.callback = MenuMouseSpeedFunc;
	s_options_menu_sensitivity_slider.minvalue		= 2;
	s_options_menu_sensitivity_slider.maxvalue		= 22;

	s_options_smoothing_box.generic.type = MTYPE_SPINCONTROL;
	s_options_smoothing_box.generic.x	= 0;
	s_options_smoothing_box.generic.y	= FONTSCALE*180*scale;
	s_options_smoothing_box.generic.name	= "mouse smoothing";
	s_options_smoothing_box.generic.callback = MouseSmoothingFunc;
	s_options_smoothing_box.itemnames = yesno_names;

	s_options_alwaysrun_box.generic.type = MTYPE_SPINCONTROL;
	s_options_alwaysrun_box.generic.x	= 0;
	s_options_alwaysrun_box.generic.y	= FONTSCALE*190*scale;
	s_options_alwaysrun_box.generic.name	= "always run";
	s_options_alwaysrun_box.generic.callback = AlwaysRunFunc;
	s_options_alwaysrun_box.itemnames = yesno_names;

	s_options_invertmouse_box.generic.type = MTYPE_SPINCONTROL;
	s_options_invertmouse_box.generic.x	= 0;
	s_options_invertmouse_box.generic.y	= FONTSCALE*200*scale;
	s_options_invertmouse_box.generic.name	= "invert mouse";
	s_options_invertmouse_box.generic.callback = InvertMouseFunc;
	s_options_invertmouse_box.itemnames = yesno_names;

	// Do not re-allocate font/crosshair/HUD names each time the menu is
	// displayed - BlackIce
	if ( font_names == NULL )
		font_names = SetFontNames ();
	s_options_font_box.generic.type = MTYPE_SPINCONTROL;
	s_options_font_box.generic.x	= 0;
	s_options_font_box.generic.y	= FONTSCALE*210*scale;
	s_options_font_box.generic.name	= "font";
	s_options_font_box.generic.callback = FontFunc;
	s_options_font_box.itemnames = (const char **) font_names;
	s_options_font_box.generic.statusbar	= "select your font";

	if ( crosshair_names == NULL )
		crosshair_names = SetCrosshairNames ();
	s_options_crosshair_box.generic.type = MTYPE_SPINCONTROL;
	s_options_crosshair_box.generic.x	= 0;
	s_options_crosshair_box.generic.y	= FONTSCALE*220*scale;
	s_options_crosshair_box.generic.name	= "crosshair";
	s_options_crosshair_box.generic.callback = CrosshairFunc;
	s_options_crosshair_box.itemnames = (const char **) crosshair_names;
	s_options_crosshair_box.generic.statusbar	= "select your crosshair";

	if ( hud_names == NULL )
		hud_names = SetHudNames ();
	s_options_hud_box.generic.type = MTYPE_SPINCONTROL;
	s_options_hud_box.generic.x	= 0;
	s_options_hud_box.generic.y	= FONTSCALE*230*scale;
	s_options_hud_box.generic.name	= "hud";
	s_options_hud_box.generic.callback = HudFunc;
	s_options_hud_box.itemnames = (const char **) hud_names;
	s_options_hud_box.generic.statusbar	= "select your hud style";

	s_options_discolor_box.generic.type = MTYPE_SPINCONTROL;
	s_options_discolor_box.generic.x = 0;
	s_options_discolor_box.generic.y = FONTSCALE*240*scale;
	s_options_discolor_box.generic.name = "disruptor color";
	s_options_discolor_box.generic.callback = DisColorFunc;
	s_options_discolor_box.itemnames = color_names;
	s_options_discolor_box.generic.statusbar = "select disruptor beam color";

	s_options_minimap_box.generic.type = MTYPE_SPINCONTROL;
	s_options_minimap_box.generic.x		= 0;
	s_options_minimap_box.generic.y		= FONTSCALE*250*scale;
	s_options_minimap_box.generic.name  = "minimap";
	s_options_minimap_box.generic.callback = MinimapFunc;
	s_options_minimap_box.itemnames = minimap_names;

	s_options_joystick_box.generic.type = MTYPE_SPINCONTROL;
	s_options_joystick_box.generic.x	= 0;
	s_options_joystick_box.generic.y	= FONTSCALE*260*scale;
	s_options_joystick_box.generic.name	= "use joystick";
	s_options_joystick_box.generic.callback = JoystickFunc;
	s_options_joystick_box.itemnames = yesno_names;

	s_options_showfps_box.generic.type = MTYPE_SPINCONTROL;
	s_options_showfps_box.generic.x	= 0;
	s_options_showfps_box.generic.y	= FONTSCALE*270*scale;
	s_options_showfps_box.generic.name	= "display fps";
	s_options_showfps_box.generic.callback = ShowfpsFunc;
	s_options_showfps_box.itemnames = yesno_names;

	s_options_showtime_box.generic.type = MTYPE_SPINCONTROL;
	s_options_showtime_box.generic.x	= 0;
	s_options_showtime_box.generic.y	= FONTSCALE*280*scale;
	s_options_showtime_box.generic.name	= "display time";
	s_options_showtime_box.generic.callback = ShowtimeFunc;
	s_options_showtime_box.itemnames = yesno_names;

	s_options_defaults_action.generic.type	= MTYPE_ACTION;
	s_options_defaults_action.generic.x		= 0;
	s_options_defaults_action.generic.y		= FONTSCALE*300*scale;
	s_options_defaults_action.generic.name	= "reset defaults";
	s_options_defaults_action.generic.callback = ControlsResetDefaultsFunc;

	s_options_console_action.generic.type	= MTYPE_ACTION;
	s_options_console_action.generic.x		= 0;
	s_options_console_action.generic.y		= FONTSCALE*310*scale;
	s_options_console_action.generic.name	= "go to console";
	s_options_console_action.generic.callback = ConsoleFunc;

	ControlsSetMenuItemValues();

	Menu_AddItem( &s_options_menu, ( void * ) &s_options_customize_options_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_precachecustom_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_paindist_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_explosiondist_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_target_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_ragdoll_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_noblood_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_noskins_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_taunts_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_sfxvolume_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_bgvolume_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_bgmusic_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_doppler_effect_list );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_mouse_accel_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_sensitivity_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_menu_sensitivity_slider );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_smoothing_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_alwaysrun_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_invertmouse_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_font_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_crosshair_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_hud_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_discolor_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_minimap_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_joystick_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_showfps_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_showtime_box );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_defaults_action );
	Menu_AddItem( &s_options_menu, ( void * ) &s_options_console_action );
}

void Options_MenuDraw (void)
{
	char path[MAX_OSPATH];

	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back"); //draw black background first
	M_Banner( "m_options", banneralpha );
	if(strcmp(crosshair->string, "none")) {
		sprintf(path, "/pics/%s", crosshair->string);
		M_CrosshairPic(path);
	}
	//draw disruptor color icon
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
//static char *creditsIndex[256];
static char *creditsBuffer;
static const char *idcredits[] =
{
	"+Alien Arena by COR Entertainment",
	"",
	"+PROGRAMMING",
	"John Diamond",
	"Jim Bower",
	"Emmanuel Benoit",
	"Lee Salzman",
	"Dave Carter",
	"Victor Luchits",
	"Jan Rafaj",
	"Shane Bayer",
	"Tony Jackson",
	"Stephan Stahl",
	"Kyle Hunter",
	"Andres Mejia",
	"",
	"+ART",
	"John Diamond",
	"Dennis -xEMPx- Zedlach",
	"Shawn Keeth",
	"Enki",
	"",
	"+FONTS",
	"John Diamond",
	"the-interceptor from http://www.quakeworld.nu/",
	"",
	"+LOGO",
	"Adam -servercleaner- Szalai",
	"Paul -GimpAlien-",
	"",
	"+LEVEL DESIGN",
	"John Diamond",
	"Dennis -xEMPx- Zedlach",
	"",
	"+SOUND EFFECTS AND MUSIC",
	"Music/FX Composed and Produced by",
	"Paul Joyce, Whitelipper, Divinity",
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
	extern cvar_t *name;
	char pw[64];

	// disable updates
	cl.servercount = -1;
	M_ForceMenuOff ();
	Cvar_SetValue( "deathmatch", 1 );
	Cvar_SetValue( "ctf", 0 );

	//listen servers are passworded
	sprintf(pw, "%s%4.2f", name->string, crand());
	Cvar_Set ("password", pw);

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
/*
	static const char *difficulty_names[] =
	{
		"easy",
		"medium",
		"hard",
		0
	};
*/
	float scale;;
	scale = (float)(viddef.height)/600;

	banneralpha = 0.1;

	s_game_menu.x = viddef.width * 0.50;
	s_game_menu.nitems = 0;

	s_game_title.generic.type	= MTYPE_SEPARATOR;
	s_game_title.generic.x		= FONTSCALE*72*scale;
	s_game_title.generic.y		= FONTSCALE*30*scale;
	s_game_title.generic.name	= "Instant Action!";

	s_easy_game_action.generic.type	= MTYPE_ACTION;
	s_easy_game_action.generic.x		= FONTSCALE*32*scale;
	s_easy_game_action.generic.y		= FONTSCALE*50*scale;
	s_easy_game_action.generic.cursor_offset = -16;
	s_easy_game_action.generic.name	= "easy";
	s_easy_game_action.generic.callback = EasyGameFunc;

	s_medium_game_action.generic.type	= MTYPE_ACTION;
	s_medium_game_action.generic.x		= FONTSCALE*32*scale;
	s_medium_game_action.generic.y		= FONTSCALE*60*scale;
	s_medium_game_action.generic.cursor_offset = -16;
	s_medium_game_action.generic.name	= "medium";
	s_medium_game_action.generic.callback = MediumGameFunc;

	s_hard_game_action.generic.type	= MTYPE_ACTION;
	s_hard_game_action.generic.x		= FONTSCALE*32*scale;
	s_hard_game_action.generic.y		= FONTSCALE*70*scale;
	s_hard_game_action.generic.cursor_offset = -16;
	s_hard_game_action.generic.name	= "hard";
	s_hard_game_action.generic.callback = HardGameFunc;

	s_blankline.generic.type = MTYPE_SEPARATOR;

	s_credits_action.generic.type	= MTYPE_ACTION;
	s_credits_action.generic.x		= FONTSCALE*32*scale;
	s_credits_action.generic.y		= FONTSCALE*90*scale;
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
	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back"); //draw black background first
	M_Banner( "m_single", banneralpha );
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

IRC MENUS

=============================================================================
*/

static int		m_IRC_cursor;
char			IRC_key[64];

static menuframework_s	s_irc_menu;
static menuaction_s		s_irc_title;
static menuaction_s		s_irc_join;
static menulist_s		s_irc_joinatstartup;
static menuaction_s		s_irc_key;
static menuaction_s		s_irc_editsettings;


static void JoinIRCFunc( void *unused )
{
	if(pNameUnique)
		CL_InitIRC();
}

static void QuitIRCFunc( void *unused )
{
	CL_IRCInitiateShutdown();
}

static void AutoIRCFunc( void *unused)
{
	Cvar_SetValue("cl_IRC_connect_at_startup", s_irc_joinatstartup.curvalue);
}

static void M_FindIRCKey ( void )
{
	int		count;
	int		j;
	int		l;
	char	*b;
	int twokeys[2];

	twokeys[0] = twokeys[1] = -1;
	l = strlen("messagemode3");
	count = 0;

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, "messagemode3", l) )
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
	//got our key
	Com_sprintf(IRC_key, sizeof(IRC_key), "IRC Chat Key is %s", Key_KeynumToString(twokeys[0]));
}

static void IRCSettingsFunc( void * self )
{
	M_Menu_IRCSettings_f( );
}

void IRC_MenuInit( void )
{
	float scale;
	extern cvar_t *name;

	static const char *yes_no_names[] =
	{
		"no", "yes", 0
	};

	if(!strcmp(name->string, "Player"))
		pNameUnique = false;
	else
		pNameUnique = true;

	if(!cl_IRC_connect_at_startup)
		cl_IRC_connect_at_startup = Cvar_Get("cl_IRC_connect_at_startup", "0", CVAR_ARCHIVE);

	M_FindIRCKey();

	scale = (float)(viddef.height)/600;

	banneralpha = 0.1;

	s_irc_menu.x = viddef.width * 0.50;
	s_irc_menu.nitems = 0;

	s_irc_title.generic.type	= MTYPE_COLORTXT;
	s_irc_title.generic.x		= -232*scale;
	s_irc_title.generic.y		= FONTSCALE*30*scale;
	s_irc_title.generic.name	= "^3IRC ^1Chat ^1Utilities";

	s_irc_join.generic.type	= MTYPE_ACTION;
	s_irc_join.generic.x		= 128*scale;
	s_irc_join.generic.y		= FONTSCALE*60*scale;
	s_irc_join.generic.name	= "Join IRC Chat";
	s_irc_join.generic.callback = JoinIRCFunc;

	s_irc_joinatstartup.generic.type	= MTYPE_SPINCONTROL;
	s_irc_joinatstartup.generic.x		= 128*scale;
	s_irc_joinatstartup.generic.y		= FONTSCALE*80*scale;
	s_irc_joinatstartup.generic.name	= "Autojoin At Startup";
	s_irc_joinatstartup.itemnames = yes_no_names;
	s_irc_joinatstartup.curvalue = cl_IRC_connect_at_startup->value;
	s_irc_joinatstartup.generic.callback = AutoIRCFunc;

	s_irc_editsettings.generic.type = MTYPE_ACTION;
	s_irc_editsettings.generic.x	= 128*scale;
	s_irc_editsettings.generic.y	= FONTSCALE*100*scale;
	s_irc_editsettings.generic.name	= "IRC settings";
	s_irc_editsettings.generic.callback = IRCSettingsFunc;

	s_irc_key.generic.type	= MTYPE_COLORTXT;
	s_irc_key.generic.x		= -128*scale;
	s_irc_key.generic.y		= FONTSCALE*140*scale;
	s_irc_key.generic.name	= IRC_key;

	Menu_AddItem( &s_irc_menu, ( void * ) &s_irc_title );
	Menu_AddItem( &s_irc_menu, ( void * ) &s_irc_join );
	Menu_AddItem( &s_irc_menu, ( void * ) &s_irc_joinatstartup );
	Menu_AddItem( &s_irc_menu, ( void * ) &s_irc_editsettings );
	Menu_AddItem( &s_irc_menu, ( void * ) &s_irc_key );

	Menu_Center( &s_irc_menu );
}


void IRC_MenuDraw( void )
{

	float scale;

	scale = (float)(viddef.height)/600;

	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back"); //draw black background first
	M_Banner( "m_irc", banneralpha );

	//warn user that they cannot join until changing default player name
	if(!pNameUnique) {
		M_DrawTextBox( -32*scale, (int)(FONTSCALE*-95*scale), 40/scale, 2 );
		M_Print( (int)(FONTSCALE*32*scale), (int)(FONTSCALE*-85*scale),  "You must create your player" );
		M_Print( (int)(FONTSCALE*32*scale), (int)(FONTSCALE*-75*scale),  "name before joining a server!" );
	} else if(CL_IRCIsConnected()) {
		M_DrawTextBox( -28*scale, (int)(FONTSCALE*-95*scale), 36/scale, 1 );
		M_Print( (int)(FONTSCALE*28*scale), (int)(FONTSCALE*-80*scale),  "Connected to IRC server." );
	} else if(CL_IRCIsRunning()) {
		M_DrawTextBox( -28*scale, (int)(FONTSCALE*-95*scale), 36/scale, 1 );
		M_Print( (int)(FONTSCALE*28*scale), (int)(FONTSCALE*-80*scale),  "Connecting to IRC server..." );
	}

	// Update join/quit menu entry
	if ( CL_IRCIsRunning( ) ) {
		s_irc_join.generic.name	= "Quit IRC Chat";
		s_irc_join.generic.callback = QuitIRCFunc;
	} else {
		s_irc_join.generic.name	= "Join IRC Chat";
		s_irc_join.generic.callback = JoinIRCFunc;
	}

	Menu_AdjustCursor( &s_irc_menu, 1 );
	Menu_Draw( &s_irc_menu );
}

const char *IRC_MenuKey( int key )
{
	return Default_MenuKey( &s_irc_menu, key );
}


void M_Menu_IRC_f (void)
{
	IRC_MenuInit();
	M_PushMenu( IRC_MenuDraw, IRC_MenuKey );
	m_IRC_cursor = 1;
}



/*
=============================================================================

IRC SETTINGS MENU

=============================================================================
*/

static menuframework_s		s_irc_settings_menu;
static menuaction_s		s_irc_settings_title;
static menufield_s		s_irc_server;
static menufield_s		s_irc_channel;
static menufield_s		s_irc_port;
static menulist_s		s_irc_ovnickname;
static menufield_s		s_irc_nickname;
static menufield_s		s_irc_kickrejoin;
static menufield_s		s_irc_reconnectdelay;
static menuaction_s		s_irc_settings_apply;


static void ApplyIRCSettings( void * self )
{
	qboolean running = CL_IRCIsRunning( );
	if ( running ) {
		CL_IRCInitiateShutdown( );
		CL_IRCWaitShutdown( );
	}

	Cvar_Set(	"cl_IRC_server" ,		s_irc_server.buffer);
	Cvar_Set(	"cl_IRC_channel" ,		s_irc_channel.buffer);
	Cvar_SetValue(	"cl_IRC_port" , 		atoi( s_irc_port.buffer ) );
	Cvar_SetValue(	"cl_IRC_override_nickname" ,	s_irc_ovnickname.curvalue );
	Cvar_Set(	"cl_IRC_nickname" ,		s_irc_nickname.buffer );
	Cvar_SetValue(	"cl_IRC_kick_rejoin" ,		atoi( s_irc_kickrejoin.buffer ) );
	Cvar_SetValue(	"cl_IRC_reconnect_delay" ,	atoi( s_irc_reconnectdelay.buffer ) );

	if ( running )
		CL_InitIRC( );

	M_PopMenu( );
}


static void IRC_SettingsMenuInit( )
{
	float scale;
	extern cvar_t *name;

	static const char *yes_no_names[] =
	{
		"no", "yes", 0
	};

	scale = (float)(viddef.height)/600;
	banneralpha = 0.1;

	s_irc_settings_menu.x			= viddef.width * 0.50;
	s_irc_settings_menu.nitems		= 0;

	s_irc_settings_title.generic.type	= MTYPE_COLORTXT;
	s_irc_settings_title.generic.x		= -124*scale;
	s_irc_settings_title.generic.y		= FONTSCALE*30*scale;
	s_irc_settings_title.generic.name	= "IRC Chat Settings";

	s_irc_server.generic.type		= MTYPE_FIELD;
	s_irc_server.generic.name		= "Server ";
	s_irc_server.generic.x			= -67*scale;
	s_irc_server.generic.y			= FONTSCALE*48*scale;
	s_irc_server.generic.statusbar		= "Address or name of the IRC server";
	s_irc_server.length			= 32;
	s_irc_server.visible_length		= 16;
	s_irc_server.generic.callback		= 0;
	s_irc_server.cursor			= strlen( cl_IRC_server->string );
	strcpy( s_irc_server.buffer, Cvar_VariableString("cl_IRC_server") );

	s_irc_channel.generic.type		= MTYPE_FIELD;
	s_irc_channel.generic.name		= "Channel ";
	s_irc_channel.generic.x			= -67*scale;
	s_irc_channel.generic.y			= FONTSCALE*64*scale;
	s_irc_channel.generic.statusbar		= "Name of the channel to join";
	s_irc_channel.length 			= 16;
	s_irc_channel.visible_length		= 16;
	s_irc_channel.generic.callback		= 0;
	s_irc_channel.cursor			= strlen( cl_IRC_channel->string );
	strcpy( s_irc_channel.buffer, Cvar_VariableString("cl_IRC_channel") );

	s_irc_port.generic.type			= MTYPE_FIELD;
	s_irc_port.generic.name			= "TCP Port ";
	s_irc_port.generic.x			= -67*scale;
	s_irc_port.generic.y			= FONTSCALE*80*scale;
	s_irc_port.generic.statusbar		= "Port to connect to on the server";
	s_irc_port.length 			= 5;
	s_irc_port.visible_length		= 6;
	s_irc_port.generic.callback		= 0;
	s_irc_port.cursor			= strlen( cl_IRC_port->string );
	strcpy( s_irc_port.buffer, Cvar_VariableString("cl_IRC_port") );

	s_irc_ovnickname.generic.type		= MTYPE_SPINCONTROL;
	s_irc_ovnickname.generic.x		= 90*scale;
	s_irc_ovnickname.generic.y		= FONTSCALE*96*scale;
	//s_irc_ovnickname.generic.cursor_offset	= -24*scale;
	s_irc_ovnickname.generic.name		= "Override nick";
	s_irc_ovnickname.generic.callback	= 0;
	s_irc_ovnickname.generic.statusbar	= "Enable this to override the default, player-based nick";
	s_irc_ovnickname.itemnames		= yes_no_names;
	s_irc_ovnickname.curvalue		= cl_IRC_override_nickname->value ? 1 : 0;

	s_irc_nickname.generic.type		= MTYPE_FIELD;
	s_irc_nickname.generic.name		= "Nick ";
	s_irc_nickname.generic.x		= -67*scale;
	s_irc_nickname.generic.y		= FONTSCALE*112*scale;
	s_irc_nickname.generic.statusbar	= "Nickname override to use";
	s_irc_nickname.length 			= 15;
	s_irc_nickname.visible_length		= 16;
	s_irc_nickname.generic.callback		= 0;
	s_irc_nickname.cursor			= strlen( cl_IRC_nickname->string );
	strcpy( s_irc_nickname.buffer, Cvar_VariableString("cl_IRC_nickname") );

	s_irc_kickrejoin.generic.type		= MTYPE_FIELD;
	s_irc_kickrejoin.generic.name		= "Autorejoin ";
	s_irc_kickrejoin.generic.x		= -67*scale;
	s_irc_kickrejoin.generic.y		= FONTSCALE*128*scale;
	s_irc_kickrejoin.generic.statusbar	= "Delay before automatic rejoin after kick (0 to disable)";
	s_irc_kickrejoin.length 		= 3;
	s_irc_kickrejoin.visible_length		= 4;
	s_irc_kickrejoin.generic.callback	= 0;
	s_irc_kickrejoin.cursor			= strlen( cl_IRC_kick_rejoin->string );
	strcpy( s_irc_kickrejoin.buffer, Cvar_VariableString("cl_IRC_kick_rejoin") );

	s_irc_reconnectdelay.generic.type	= MTYPE_FIELD;
	s_irc_reconnectdelay.generic.name	= "Reconnect ";
	s_irc_reconnectdelay.generic.x		= -67*scale;
	s_irc_reconnectdelay.generic.y		= FONTSCALE*144*scale;
	s_irc_reconnectdelay.generic.statusbar	= "Delay between reconnection attempts (minimum 5)";
	s_irc_reconnectdelay.length 		= 3;
	s_irc_reconnectdelay.visible_length	= 4;
	s_irc_reconnectdelay.generic.callback	= 0;
	s_irc_reconnectdelay.cursor		= strlen( cl_IRC_reconnect_delay->string );
	strcpy( s_irc_reconnectdelay.buffer, Cvar_VariableString("cl_IRC_reconnect_delay") );

	s_irc_settings_apply.generic.type	= MTYPE_ACTION;
	s_irc_settings_apply.generic.x		= 124*scale;
	s_irc_settings_apply.generic.y		= FONTSCALE*170*scale;
	s_irc_settings_apply.generic.cursor_offset = -24 * scale;
	s_irc_settings_apply.generic.name	= "Apply settings";
	s_irc_settings_apply.generic.callback	= ApplyIRCSettings;

	Menu_AddItem( &s_irc_settings_menu, ( void * ) &s_irc_settings_title );
	Menu_AddItem( &s_irc_settings_menu, ( void * ) &s_irc_server );
	Menu_AddItem( &s_irc_settings_menu, ( void * ) &s_irc_channel );
	Menu_AddItem( &s_irc_settings_menu, ( void * ) &s_irc_port );
	Menu_AddItem( &s_irc_settings_menu, ( void * ) &s_irc_ovnickname );
	Menu_AddItem( &s_irc_settings_menu, ( void * ) &s_irc_nickname );
	Menu_AddItem( &s_irc_settings_menu, ( void * ) &s_irc_kickrejoin );
	Menu_AddItem( &s_irc_settings_menu, ( void * ) &s_irc_reconnectdelay );
	Menu_AddItem( &s_irc_settings_menu, ( void * ) &s_irc_settings_apply );

	Menu_Center( &s_irc_settings_menu );
}

void IRC_SettingsMenuDraw( void )
{

	float scale;
	scale = (float)(viddef.height)/600;

	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back");
	M_Banner( "m_irc", banneralpha );

	if(!pNameUnique) {
		M_DrawTextBox( -32*scale, (int)(FONTSCALE*-95*scale), 40/scale, 2 );
		M_Print( (int)(FONTSCALE*32*scale), (int)(FONTSCALE*-85*scale),  "You must create your player" );
		M_Print( (int)(FONTSCALE*32*scale), (int)(FONTSCALE*-75*scale),  "name before joining a server!" );
	} else if(CL_IRCIsConnected()) {
		M_DrawTextBox( -28*scale, (int)(FONTSCALE*-95*scale), 36/scale, 1 );
		M_Print( (int)(FONTSCALE*28*scale), (int)(FONTSCALE*-80*scale),  "Connected to IRC server." );
	} else if(CL_IRCIsRunning()) {
		M_DrawTextBox( -28*scale, (int)(FONTSCALE*-95*scale), 36/scale, 1 );
		M_Print( (int)(FONTSCALE*28*scale), (int)(FONTSCALE*-80*scale),  "Connecting to IRC server..." );
	}

	Menu_AdjustCursor( &s_irc_settings_menu, 1 );
	Menu_Draw( &s_irc_settings_menu );
}

const char *IRC_SettingsMenuKey( int key )
{
	return Default_MenuKey( &s_irc_settings_menu, key );
}


void M_Menu_IRCSettings_f (void)
{
	IRC_SettingsMenuInit( );
	M_PushMenu( IRC_SettingsMenuDraw , IRC_SettingsMenuKey );
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
static menuaction_s		s_joinserver_player_ranking_action;
static menulist_s		s_joinserver_filterempty_action;
static menuaction_s		s_joinserver_server_actions[MAX_LOCAL_SERVERS];
static menuaction_s		s_joinserver_server_info[32];
static menuaction_s		s_joinserver_server_data[6];
static menuaction_s		s_joinserver_moveup;
static menuaction_s		s_joinserver_movedown;
static menuslider_s		s_joinserver_scrollbar;
static menuaction_s		s_playerlist_moveup;
static menuaction_s		s_playerlist_movedown;
static menuslider_s		s_playerlist_scrollbar;

int		m_num_servers;
int		m_show_empty;

#define	NO_SERVER_STRING	"<no server>"

static char local_server_info[256][256];
static char local_server_data[6][64];
static int	local_server_rankings[64];
unsigned int starttime;

int GetColorTokens( char *string)
{
	int count;
	char *pch;

	count = 0;
	pch=string;
	while ( *pch )
	{
		if ( Q_IsColorString( pch ) )
		{
			++count;
			++pch;
		}
		++pch;
	}

	return count;
}

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
	int	 playerRankings[64];
	char skill[32];
	int players;
	int ping;
	netadr_t local_server_netadr;
	char serverInfo[256];

} SERVERDATA;

SERVERDATA mservers[MAX_LOCAL_SERVERS];

PLAYERSTATS thisPlayer;

void M_AddToServerList (netadr_t adr, char *status_string)
{
	char *rLine;
	char *token;
	char skillLevel[24];
	char lasttoken[256];
	char seps[]   = "\\";
	int players = 0;
	int result;
	char playername[PLAYERNAME_SIZE];
	int score, ping, rankTotal, i, x;
	PLAYERSTATS	player;

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
		if (!Q_strcasecmp (lasttoken, "admin"))
			Com_sprintf(mservers[m_num_servers].szAdmin, sizeof(mservers[m_num_servers].szAdmin), "Admin: %s", token);
		else if (!Q_strcasecmp (lasttoken, "website"))
			Com_sprintf(mservers[m_num_servers].szWebsite, sizeof(mservers[m_num_servers].szWebsite), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "fraglimit"))
			Com_sprintf(mservers[m_num_servers].fraglimit, sizeof(mservers[m_num_servers].fraglimit), "Fraglimit: %s", token);
		else if (!Q_strcasecmp (lasttoken, "timelimit"))
			Com_sprintf(mservers[m_num_servers].timelimit, sizeof(mservers[m_num_servers].timelimit), "Timelimit: %s", token);
		else if (!Q_strcasecmp (lasttoken, "version"))
			Com_sprintf(mservers[m_num_servers].szVersion, sizeof(mservers[m_num_servers].szVersion), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "mapname"))
			Com_sprintf(mservers[m_num_servers].szMapName, sizeof(mservers[m_num_servers].szMapName), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "hostname"))
			Com_sprintf(mservers[m_num_servers].szHostName, sizeof(mservers[m_num_servers].szHostName), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "maxclients"))
			Com_sprintf(mservers[m_num_servers].maxClients, sizeof(mservers[m_num_servers].maxClients), "%s", token);

		/* Get next token: */
		Com_sprintf(lasttoken, sizeof(lasttoken), "%s", token);
		token = strtok( NULL, seps );
	}

	free (rLine);

	//playerinfo
	rankTotal = 0;
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

		playername[sizeof(playername)-1] = '\0';

		//get ranking
		Q_strncpyz2( player.playername, playername, sizeof(player.playername));
		player.totalfrags = player.totaltime = player.ranking = 0;
		player = getPlayerRanking ( player );

		// trim playername string
		x = ValidatePlayerName( playername, sizeof(playername) );
		x = 15 - x; // calc space padding from visible glyph count
		assert( x >= 0 );
		if ( x > 0 && ( (x + strlen(playername)) < sizeof(playername) ) )
			strncat( playername, "               ", x );

		Com_sprintf(mservers[m_num_servers].playerInfo[players], sizeof(mservers[m_num_servers].playerInfo[players]),
			"%s    %4i    %4i", playername, score, ping);

		mservers[m_num_servers].playerRankings[players] = player.ranking;

		rankTotal += player.ranking;

		players++;
	}

	if(players) {

		if(thisPlayer.ranking < (rankTotal/players) - 100)
			strcpy(skillLevel, "Your Skill is ^1Higher");
		else if(thisPlayer.ranking > (rankTotal/players + 100))
			strcpy(skillLevel, "Your Skill is ^4Lower");
		else
			strcpy(skillLevel, "Your Skill is ^3Even");

		Com_sprintf(mservers[m_num_servers].skill, sizeof(mservers[m_num_servers].skill), "Skill: %s", skillLevel);
	}
	else
		Com_sprintf(mservers[m_num_servers].skill, sizeof(mservers[m_num_servers].skill), "Skill Level: Unknown");

	if(!m_show_empty)
		if(!players)
			return;

	mservers[m_num_servers].players = players;

	//build the string for the server (hostname - address - mapname - players/maxClients)
	//pad the strings - gotta do this for both maps and hostname
	x = 0;
	for(i=0; i<32; i++) {
		if(!mservers[m_num_servers].szHostName[i])
			mservers[m_num_servers].szHostName[i] = 32;
		else if(mservers[m_num_servers].szHostName[i] == '^' && i < strlen( mservers[m_num_servers].szHostName )-1) {
			if(mservers[m_num_servers].szHostName[i+1] != '^')
				x += 2;
		}
	}
	mservers[m_num_servers].szHostName[20+x] = 0; //fix me this is dangerous
	for(i=0; i<12; i++) {
		if(!mservers[m_num_servers].szMapName[i])
			mservers[m_num_servers].szMapName[i] = 32;
	}
	mservers[m_num_servers].szMapName[12] = 0;
	Com_sprintf(mservers[m_num_servers].serverInfo, sizeof(mservers[m_num_servers].serverInfo), "%s  %12s  %2i/%2s  %4i", mservers[m_num_servers].szHostName,
		mservers[m_num_servers].szMapName, players, mservers[m_num_servers].maxClients, mservers[m_num_servers].ping);

	CON_Clear();
	m_num_servers++;
}
void MoveUp ( void *self)
{
	svridx--;
	if(svridx < 0)
		svridx = 0;
	s_joinserver_scrollbar.curvalue--;
}
void MoveDown ( void *self)
{
	svridx++;
	if(svridx > 112)
		svridx = 112;
	s_joinserver_scrollbar.curvalue++;
}
void JoinScrollMove ( void *self)
{
	svridx = s_joinserver_scrollbar.curvalue;
	if(svridx > 112)
		svridx = 112;
}

void MoveUp_plist ( void *self)
{
	playeridx--;
	if(playeridx < 0)
		playeridx = 0;
	s_playerlist_scrollbar.curvalue--;
}
void MoveDown_plist ( void *self)
{
	playeridx++;
	if(playeridx > 24)
		playeridx = 24;
	s_playerlist_scrollbar.curvalue++;
}
void PlayerScrollMove ( void *self)
{
	playeridx = s_playerlist_scrollbar.curvalue;
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

	playeridx = s_playerlist_scrollbar.curvalue = 0;

	if ( Q_strcasecmp( mservers[index+svridx].szHostName, NO_SERVER_STRING ) == 0 )
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
		Com_sprintf(local_server_data[0], sizeof(local_server_data[0]), mservers[index+svridx].skill);
		Com_sprintf(local_server_data[1], sizeof(local_server_data[1]), mservers[index+svridx].szAdmin);
		Com_sprintf(local_server_data[2], sizeof(local_server_data[2]), mservers[index+svridx].szWebsite);
		Com_sprintf(local_server_data[3], sizeof(local_server_data[3]), mservers[index+svridx].fraglimit);
		Com_sprintf(local_server_data[4], sizeof(local_server_data[4]), mservers[index+svridx].timelimit);
		Com_sprintf(local_server_data[5], sizeof(local_server_data[5]), mservers[index+svridx].szVersion);

		//players
		for(i=0; i<mservers[index+svridx].players; i++) {
			Com_sprintf(local_server_info[i], sizeof(local_server_info[i]), mservers[index+svridx].playerInfo[i]);
			local_server_rankings[i] = mservers[index+svridx].playerRankings[i];
		}

		return;
	}

	if(!pNameUnique) {
		M_Menu_PlayerConfig_f();
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

void PlayerRankingFunc( void *self )
{
	M_Menu_PlayerRanking_f();
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

#if defined UNIX_VARIANT
	sleep(1);
#else
	Sleep(1000); //time to recieve packets
#endif

	CON_Clear();
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
	float scale, offset;
	extern cvar_t *name;

	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	scale = (float)(viddef.height)/600;

	banneralpha = 0.1;

	m_show_empty = true;

	getStatsDB();

	ValidatePlayerName( name->string, (strlen(name->string)+1) );
	Q_strncpyz2( thisPlayer.playername, name->string, sizeof(thisPlayer.playername) );
	thisPlayer.totalfrags = thisPlayer.totaltime = thisPlayer.ranking = 0;
	thisPlayer = getPlayerRanking ( thisPlayer );

	if(!strcmp(name->string, "Player"))
		pNameUnique = false;
	else
		pNameUnique = true;

	s_joinserver_menu.x = viddef.width * 0.50;
	offset = viddef.height/2 + 60*scale;

	s_joinserver_menu.nitems = 0;

	s_joinserver_address_book_action.generic.type	= MTYPE_ACTION;
	s_joinserver_address_book_action.generic.name	= "address book";
	s_joinserver_address_book_action.generic.x		= 290*scale;
	s_joinserver_address_book_action.generic.y		= FONTSCALE*-69*scale+offset;
	s_joinserver_address_book_action.generic.cursor_offset = -16*scale;
	s_joinserver_address_book_action.generic.callback = AddressBookFunc;

	s_joinserver_search_action.generic.type = MTYPE_ACTION;
	s_joinserver_search_action.generic.name	= "refresh list";
	s_joinserver_search_action.generic.x	= -190*scale;
	s_joinserver_search_action.generic.y	= FONTSCALE*-290*scale+offset;
	s_joinserver_search_action.generic.cursor_offset = -16*scale;
	s_joinserver_search_action.generic.callback = SearchLocalGamesFunc;
	s_joinserver_search_action.generic.statusbar = "search for servers";

	s_joinserver_player_ranking_action.generic.type	= MTYPE_ACTION;
	s_joinserver_player_ranking_action.generic.name	= "Rank/Stats";
	s_joinserver_player_ranking_action.generic.x		= 55*scale;
	s_joinserver_player_ranking_action.generic.y		= FONTSCALE*-290*scale+offset;
	s_joinserver_player_ranking_action.generic.cursor_offset = -16*scale;
	s_joinserver_player_ranking_action.generic.callback = PlayerRankingFunc;

	s_joinserver_filterempty_action.generic.type = MTYPE_SPINCONTROL;
	s_joinserver_filterempty_action.generic.name	= "show empty";
	s_joinserver_filterempty_action.itemnames = yesno_names;
	s_joinserver_filterempty_action.generic.x	= 285*scale;
	s_joinserver_filterempty_action.generic.y	= FONTSCALE*-290*scale+offset;
	s_joinserver_filterempty_action.generic.cursor_offset = -16*scale;
	s_joinserver_filterempty_action.curvalue = m_show_empty;
	s_joinserver_filterempty_action.generic.callback = FilterEmptyFunc;

	s_joinserver_moveup.generic.type	= MTYPE_ACTION;
	s_joinserver_moveup.generic.name	= "     ";
	s_joinserver_moveup.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_moveup.generic.x		= 365*scale;
	s_joinserver_moveup.generic.y		= FONTSCALE*-232*scale+offset;
	s_joinserver_moveup.generic.cursor_offset = -16*scale;
	s_joinserver_moveup.generic.callback = MoveUp;

	s_joinserver_movedown.generic.type	= MTYPE_ACTION;
	s_joinserver_movedown.generic.name	= "     ";
	s_joinserver_movedown.generic.flags	= QMF_LEFT_JUSTIFY;
	s_joinserver_movedown.generic.x		= 365*scale;
	s_joinserver_movedown.generic.y		= FONTSCALE*-140*scale+offset;
	s_joinserver_movedown.generic.cursor_offset = -16*scale;
	s_joinserver_movedown.generic.callback = MoveDown;

	s_joinserver_scrollbar.generic.type  = MTYPE_VERTSLIDER;
	s_joinserver_scrollbar.generic.name  = "     ";
	s_joinserver_scrollbar.generic.x	 = 370*scale;
	s_joinserver_scrollbar.generic.y	 = FONTSCALE*-215*scale+offset;
	s_joinserver_scrollbar.minvalue		 = 0;
	s_joinserver_scrollbar.maxvalue		 = 16;
	s_joinserver_scrollbar.size			 = 12;
	s_joinserver_scrollbar.curvalue		 = 0;
	s_joinserver_scrollbar.generic.callback = JoinScrollMove;

	s_playerlist_moveup.generic.type	= MTYPE_ACTION;
	s_playerlist_moveup.generic.name	= "     ";
	s_playerlist_moveup.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerlist_moveup.generic.x		= 131*scale;
	s_playerlist_moveup.generic.y		= FONTSCALE*6*scale+offset;
	s_playerlist_moveup.generic.cursor_offset = -16*scale;
	s_playerlist_moveup.generic.callback = MoveUp_plist;

	s_playerlist_movedown.generic.type	= MTYPE_ACTION;
	s_playerlist_movedown.generic.name	= "     ";
	s_playerlist_movedown.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerlist_movedown.generic.x		= 131*scale;
	s_playerlist_movedown.generic.y		= FONTSCALE*54*scale+offset;
	s_playerlist_movedown.generic.cursor_offset = -16*scale;
	s_playerlist_movedown.generic.callback = MoveDown_plist;

	s_playerlist_scrollbar.generic.type  = MTYPE_VERTSLIDER;
	s_playerlist_scrollbar.generic.name  = " ";
	s_playerlist_scrollbar.generic.x	 = 135*scale;
	s_playerlist_scrollbar.generic.y	 = FONTSCALE*+24*scale+offset;
	s_playerlist_scrollbar.minvalue		 = 0;
	s_playerlist_scrollbar.maxvalue		 = 16;
	s_playerlist_scrollbar.size			 = 4;
	s_playerlist_scrollbar.curvalue		 = 0;
	s_playerlist_scrollbar.generic.callback = PlayerScrollMove;

	Menu_AddItem( &s_joinserver_menu, &s_joinserver_address_book_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_search_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_filterempty_action );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_player_ranking_action );

	for ( i = 0; i < 16; i++ )
		Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_actions[i] );

	for ( i = 0; i < 8; i++ ) //same here
		Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_info[i] );

	for ( i = 0; i < 6; i++ )
		Menu_AddItem( &s_joinserver_menu, &s_joinserver_server_data[i] );

	//add items to move the index
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_moveup );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_movedown );
	Menu_AddItem( &s_joinserver_menu, &s_joinserver_scrollbar );
	Menu_AddItem( &s_joinserver_menu, &s_playerlist_moveup );
	Menu_AddItem( &s_joinserver_menu, &s_playerlist_movedown );
	Menu_AddItem( &s_joinserver_menu, &s_playerlist_scrollbar );

	Menu_Center( &s_joinserver_menu );

	SearchLocalGames();
}

void JoinServer_MenuDraw(void)
{
	int i;
	float scale, offset, xoffset;
	char ranktxt[8][32];

	scale = (float)(viddef.height)/600;

	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back" ); //draw background first
	M_Banner( "m_joinserver", banneralpha );

	//warn user that they cannot join until changing default player name
	if(!pNameUnique) {
		M_DrawTextBox( 68*scale, -50*scale, 28, 2 );
		M_Print( 93*scale, -46*scale,  "You must create your player" );
		M_Print( 93*scale, -36*scale,  "name before joining a server!" );
	}

	offset = viddef.height/2 - 326*scale;
	for ( i = 0; i < 16; i++ )
	{
		s_joinserver_server_actions[i].generic.type	= MTYPE_COLORACTION;
		s_joinserver_server_actions[i].generic.name	= mservers[i+svridx].serverInfo;
		s_joinserver_server_actions[i].generic.x		= scale*-360;
		s_joinserver_server_actions[i].generic.y		= FONTSCALE*-1*scale + FONTSCALE*i*10*scale+offset;
		s_joinserver_server_actions[i].generic.cursor_offset = -16*scale;
		s_joinserver_server_actions[i].generic.callback = JoinServerFunc;
		if(pNameUnique)
			s_joinserver_server_actions[i].generic.statusbar = "press ENTER or DBL CLICK to connect";
		else
			s_joinserver_server_actions[i].generic.statusbar = "you must change your player name from the default before connecting!";
	}

	//draw the server info here

	for ( i = 0; i < 8; i++)
	{
		Com_sprintf(ranktxt[i], sizeof(ranktxt[i]), "Player is ranked %i", local_server_rankings[i+playeridx]);
		s_joinserver_server_info[i].generic.type	= MTYPE_COLORACTION;
		s_joinserver_server_info[i].generic.name	= local_server_info[i+playeridx];
		s_joinserver_server_info[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_joinserver_server_info[i].generic.x		= -360*scale;
		s_joinserver_server_info[i].generic.callback = NULL;
		s_joinserver_server_info[i].generic.y		= FONTSCALE*254*scale + FONTSCALE*i*10*scale+offset;
		s_joinserver_server_info[i].generic.statusbar = ranktxt[i];
	}

	xoffset = GetColorTokens(local_server_data[0]); //only this line
	for ( i = 0; i < 6; i++)
	{
		s_joinserver_server_data[i].generic.type	= MTYPE_COLORTXT;
		s_joinserver_server_data[i].generic.name	= local_server_data[i];
		s_joinserver_server_data[i].generic.flags	= QMF_LEFT_JUSTIFY;
		if(i == 0)
			s_joinserver_server_data[i].generic.x		= (-380-(FONTSCALE*xoffset*20))*scale;
		else
			s_joinserver_server_data[i].generic.x		= -380*scale;
		s_joinserver_server_data[i].generic.y		= FONTSCALE*169*scale + FONTSCALE*i*10*scale+offset;
	}
	s_joinserver_scrollbar.maxvalue = m_num_servers - 16;
	M_ArrowPics();
	Menu_Draw( &s_joinserver_menu );
}

const char *JoinServer_MenuKey( int key )
{
	if ( key == K_ENTER )
	{
		cursor.buttonclicks[MOUSEBUTTON1] = 2;//so we can still join without a mouse
	}
	if ( key == K_MWHEELDOWN )
	{
		svridx++;
		if(svridx > 112)
			svridx = 112;
		s_joinserver_scrollbar.curvalue++;
		if(s_joinserver_scrollbar.curvalue > 16)
			s_joinserver_scrollbar.curvalue = 16;
	}
	if( key == K_MWHEELUP )
	{
			svridx--;
		if(svridx < 0)
			svridx = 0;
		s_joinserver_scrollbar.curvalue--;
		if(s_joinserver_scrollbar.curvalue < 0)
			s_joinserver_scrollbar.curvalue = 0;
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

	banneralpha = 0.1;

	/*
	** initialize the menu stuff
	*/
	s_mutators_menu.x = viddef.width * 0.50 + 50*scale;
	s_mutators_menu.nitems = 0;
	offset = viddef.height/2 - 80*scale;

	s_instagib_list.generic.type = MTYPE_SPINCONTROL;
	s_instagib_list.generic.x	= -8*scale;
	s_instagib_list.generic.y	= 0 + offset;
	s_instagib_list.generic.name	= "instagib";
	s_instagib_list.generic.callback = InstagibFunc;
	s_instagib_list.itemnames = yn;
	s_instagib_list.curvalue = 0;

	s_rocketarena_list.generic.type = MTYPE_SPINCONTROL;
	s_rocketarena_list.generic.x	= -8*scale;
	s_rocketarena_list.generic.y	= FONTSCALE*10*scale + offset;
	s_rocketarena_list.generic.name	= "rocket arena";
	s_rocketarena_list.generic.callback = RocketFunc;
	s_rocketarena_list.itemnames = yn;
	s_rocketarena_list.curvalue = 0;

	s_excessive_list.generic.type = MTYPE_SPINCONTROL;
	s_excessive_list.generic.x	= -8*scale;
	s_excessive_list.generic.y	= FONTSCALE*20*scale + offset;
	s_excessive_list.generic.name	= "excessive";
	s_excessive_list.generic.callback = ExcessiveFunc;
	s_excessive_list.itemnames = yn;
	s_excessive_list.curvalue = 0;

	s_vampire_list.generic.type = MTYPE_SPINCONTROL;
	s_vampire_list.generic.x	= -8*scale;
	s_vampire_list.generic.y	= FONTSCALE*30*scale + offset;
	s_vampire_list.generic.name	= "vampire";
	s_vampire_list.generic.callback = MutatorsFunc;
	s_vampire_list.itemnames = yn;
	s_vampire_list.curvalue = 0;

	s_regen_list.generic.type = MTYPE_SPINCONTROL;
	s_regen_list.generic.x	= -8*scale;
	s_regen_list.generic.y	= FONTSCALE*40*scale + offset;
	s_regen_list.generic.name	= "regen";
	s_regen_list.generic.callback = MutatorsFunc;
	s_regen_list.itemnames = yn;
	s_regen_list.curvalue = 0;

	s_quickweaps_list.generic.type = MTYPE_SPINCONTROL;
	s_quickweaps_list.generic.x	= -8*scale;
	s_quickweaps_list.generic.y	= FONTSCALE*50*scale + offset;
	s_quickweaps_list.generic.name	= "quick weapons";
	s_quickweaps_list.generic.callback = MutatorsFunc;
	s_quickweaps_list.itemnames = yn;
	s_quickweaps_list.curvalue = 0;

	s_anticamp_list.generic.type = MTYPE_SPINCONTROL;
	s_anticamp_list.generic.x	= -8*scale;
	s_anticamp_list.generic.y	= FONTSCALE*60*scale + offset;
	s_anticamp_list.generic.name	= "anticamp";
	s_anticamp_list.generic.callback = MutatorsFunc;
	s_anticamp_list.itemnames = yn;
	s_anticamp_list.curvalue = 0;

	s_camptime.generic.type = MTYPE_FIELD;
	s_camptime.generic.name = "camp time ";
	s_camptime.generic.flags = QMF_NUMBERSONLY;
	s_camptime.generic.x	= 8*scale;
	s_camptime.generic.y	= FONTSCALE*76*scale + offset;
	s_camptime.length = 3;
	s_camptime.visible_length = 3;
	strcpy( s_camptime.buffer, Cvar_VariableString("camptime") );

	s_speed_list.generic.type = MTYPE_SPINCONTROL;
	s_speed_list.generic.x	= -8*scale;
	s_speed_list.generic.y	= FONTSCALE*92*scale + offset;
	s_speed_list.generic.name	= "speed";
	s_speed_list.generic.callback = MutatorsFunc;
	s_speed_list.itemnames = yn;
	s_speed_list.curvalue = 0;

	s_lowgrav_list.generic.type = MTYPE_SPINCONTROL;
	s_lowgrav_list.generic.x	= -8*scale;
	s_lowgrav_list.generic.y	= FONTSCALE*102*scale + offset;
	s_lowgrav_list.generic.name	= "low gravity";
	s_lowgrav_list.generic.callback = MutatorsFunc;
	s_lowgrav_list.itemnames = yn;
	s_lowgrav_list.curvalue = 0;

	s_joust_list.generic.type = MTYPE_SPINCONTROL;
	s_joust_list.generic.x	= -8*scale;
	s_joust_list.generic.y	= FONTSCALE*112*scale + offset;
	s_joust_list.generic.name	= "jousting";
	s_joust_list.generic.callback = MutatorsFunc;
	s_joust_list.itemnames = yn;
	s_joust_list.curvalue = 0;

	s_classbased_list.generic.type = MTYPE_SPINCONTROL;
	s_classbased_list.generic.x	= -8*scale;
	s_classbased_list.generic.y	= FONTSCALE*122*scale + offset;
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
	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back"); //draw black background first
	M_Banner( "m_mutators", banneralpha );

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

void LoadBotInfo( void )
{
	FILE *pIn;
	int i, count;
	char *info;
	char *skin;

	char fullpath[MAX_OSPATH];

	if ( !FS_FullPath( fullpath, sizeof(fullpath), BOT_GAMEDATA"/allbots.tmp" ) )
	{
		Com_DPrintf("LoadBotInfo: %s/allbots.tmp not found\n", BOT_GAMEDATA );
		return;
	}
	if( (pIn = fopen( fullpath, "rb" )) == NULL )
	{
		Com_DPrintf("LoadBotInfo: failed file open: %s\n", fullpath );
		return;
	}

	szr = fread(&count,sizeof (int),1,pIn);
	if(count>16)
		count = 16;

	for(i=0;i<count;i++)
	{
		szr = fread(bots[i].userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn);

		info = Info_ValueForKey (bots[i].userinfo, "name");
		skin = Info_ValueForKey (bots[i].userinfo, "skin");
		strcpy(bots[i].name, info);
		sprintf(bots[i].model, "bots/%s_i", skin);
	}
	totalbots = count;
    fclose(pIn);
}

void AddbotFunc(void *self)
{
	int i, count;
	char startmap[MAX_QPATH];
	char bot_filename[MAX_OSPATH];
	FILE *pOut;
	menulist_s *f = ( menulist_s * ) self;

	//get the name and copy that config string into the proper slot name
	for(i = 0; i < totalbots; i++)
	{
		if(!strcmp(f->generic.name, bots[i].name))
		{ //this is our selected bot
			strcpy(bot[slot].name, bots[i].name);
			strcpy(bot[slot].userinfo, bots[i].userinfo);
			s_bots_bot_action[slot].generic.name = bots[i].name;
		}
	}

	//save off bot file
	count = 8;
	for(i = 0; i < 8; i++)
	{
		if(!strcmp(bot[i].name, "...empty slot"))
			count--;
	}
	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	for(i = 0; i < strlen(startmap); i++)
		startmap[i] = tolower(startmap[i]);

	if(s_rules_box.curvalue == 1 || s_rules_box.curvalue == 4 || s_rules_box.curvalue == 5)
	{ // team game
		FS_FullWritePath( bot_filename, sizeof(bot_filename), BOT_GAMEDATA"/team.tmp" );
	}
	else
	{ // non-team, bots per map
		char relative_path[MAX_QPATH];
		Com_sprintf( relative_path, sizeof(relative_path), BOT_GAMEDATA"/%s.tmp", startmap );
		FS_FullWritePath( bot_filename, sizeof(bot_filename), relative_path );
	}

	if((pOut = fopen(bot_filename, "wb" )) == NULL)
	{
		Com_DPrintf("AddbotFunc: failed fopen for write: %s\n", bot_filename );
		return; // bail
	}

	szr = fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	for (i = 7; i > -1; i--) {
		if(strcmp(bot[i].name, "...empty slot"))
			szr = fwrite(bot[i].userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut);
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

	banneralpha = 0.1;

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

	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back"); //draw black background first
	M_Banner( "m_bots", banneralpha );

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
static menulist_s	s_antilag_box;
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

	// FILE *map_file; // unused
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
	int offset = 5;
	float scale;

	scale = (float)(viddef.height)/600;

	offset*=scale;

	mappicalpha = 0.1;

	//get a map description if it is there

	if(mapnames)
		strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	else
		strcpy( startmap, "missing");

	Com_sprintf(path, sizeof(path), "levelshots/%s.txt", startmap);
	FS_FOpenFile(path, &desc_file);
	if (desc_file) {
		if(fgets(line, 500, desc_file))
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
				s_startserver_map_data[i].generic.x		= 120*scale;
				s_startserver_map_data[i].generic.y		= FONTSCALE*241*scale + offset + FONTSCALE*i*10*scale;

				i++;
			}

		}

		fclose(desc_file);

	}
	else
	{
		for (i = 0; i < 5; i++ )
		{
			s_startserver_map_data[i].generic.type	= MTYPE_SEPARATOR;
			s_startserver_map_data[i].generic.name	= "no data";
			s_startserver_map_data[i].generic.flags	= QMF_LEFT_JUSTIFY;
			s_startserver_map_data[i].generic.x		= 120*scale;
			s_startserver_map_data[i].generic.y		= FONTSCALE*241*scale + offset + FONTSCALE*i*10*scale;
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
	// char *path = NULL; // unused
	static char **bspnames;
	int		j, l;

	s_maxclients_field.generic.statusbar = NULL;
	s_startserver_dmoptions_action.generic.statusbar = NULL;

	//clear out list first

	mapnames = 0;
	nummaps = 0;

	/*
	** reload the list of map names, based on rules
	*/
	// maps.lst normally in "data1/"
	//  need  to add a function to FS_ if that is the only place it is allowed
	if ( !FS_FullPath( mapsname, sizeof( mapsname ), "maps.lst" ) )
	{
			Com_Error( ERR_DROP, "couldn't find maps.lst\n" );
		return; // for show, no maps.lst is fatal error
	}
	if ( ( fp = fopen( mapsname, "rb" ) ) == 0 )
	{
		Com_Error( ERR_DROP, "couldn't open maps.lst\n" );
		return; // for "show". above is fatal error.
	}

	length = FS_filelength( fp );
	buffer = malloc( length + 1 );
	szr = fread( buffer, length, 1, fp );
	buffer[length] = 0;

	i = 0;
	while ( i < length )
	{
		if ( buffer[i] == '\r' )
			nummaps++;
		i++;
	}
	totalmaps = nummaps;

	if ( nummaps == 0 )
	{
		Com_Error( ERR_DROP, "no maps in maps.lst\n" );
		return; // for showing above is fatal.
	}

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
#if defined WIN32_VARIANT
		for (j=0 ; j<l ; j++)
			shortname[j] = tolower(shortname[j]);
#endif
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
	// done with maps.lst
	fclose( fp );
	free( buffer );

	//now, check the folders and add the maps not in the list yet

	mapfiles = FS_ListFilesInFS( "maps/*.bsp", &nmaps, 0,
	    SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<nmaps && totalmaps<MAX_MAPS;i++)
	{
		int num;

		s = strstr( mapfiles[i], "maps/");
		s++;
		s = strstr(s, "/");
		s++;

		if (!strstr(s, ".bsp"))
			continue;

		num = strlen(s)-4;
		s[num] = 0;

		curMap = s;

		l = strlen(curMap);

#if defined WIN32_VARIANT
		for (j=0 ; j<l ; j++)
			curMap[j] = tolower(curMap[j]);
#endif

		Com_sprintf( scratch, sizeof( scratch ), "%s\n%s", "Custom Map", curMap );

		//check game type, and if not already in maps.lst, add it
		l = 0;
		for ( j = 0 ; j < nummaps ; j++ )
		{
			l = Q_strcasecmp(curMap, bspnames[j]);
			if(!l)
				break; //already there, don't bother adding
		}
		if ( l )
		{ //didn't find it in our list
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

	if (mapfiles)
		FS_FreeFileList(mapfiles, nmaps);

	for(i = k; i<=nummaps; i++) {
		free(mapnames[i]);
		mapnames[i] = 0;
	}

	s_startmap_list.generic.name	= "initial map";
	s_startmap_list.itemnames = (const char **)mapnames;
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

// Running a dedicated server from menu does not always work right in Linux, if program is
//  invoked from a gui menu system. Listen server should be ok.
// Removing option from menu for now, since it is possible to start server running in
//  background without realizing it.
	if(s_dedicated_box.curvalue) {
#if defined WIN32_VARIANT
		Cvar_ForceSet("dedicated", "1");
#else
		Cvar_ForceSet("dedicated", "0");
#endif
		Cvar_Set("sv_maplist", startmap);
		Cbuf_AddText ("setmaster master.corservers.com master2.corservers.com\n");
	}
	Cvar_SetValue( "skill", s_skill_box.curvalue );
	Cvar_SetValue( "grapple", s_grapple_box.curvalue);
	Cvar_SetValue( "g_antilag", s_antilag_box.curvalue);

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
 		if(Q_strcasecmp(startmap, "dm-dynamo") == 0)
  			spot = "start";

	}
	if (s_rules_box.curvalue == 1)		// PGM
	{
 		if(Q_strcasecmp(startmap, "ctf-chromium") == 0)
  			spot = "start";

	}
	if (s_rules_box.curvalue == 2)
	{
		if(Q_strcasecmp(startmap, "aoa-morpheus") == 0)
			spot = "start";
	}
	if (s_rules_box.curvalue == 3)
	{
		if(Q_strcasecmp(startmap, "db-chromium") == 0)
			spot = "start";
	}
	if (s_rules_box.curvalue == 4)
	{
		if(Q_strcasecmp(startmap, "tca-europa") == 0)
			spot = "start";
	}
	if (s_rules_box.curvalue == 5)
	{
		if(Q_strcasecmp(startmap, "cp-grindery") == 0)
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
	static const char *offon[] =
	{
		"off",
		"on",
		0
	};
	float scale;

	scale = (float)(viddef.height)/600;

	/*
	** initialize the menu stuff
	*/
	s_startserver_menu.x = viddef.width * 0.50;
	s_startserver_menu.nitems = 0;
	offset = 5*scale;
	mappicalpha = 0.1;
	banneralpha = 0.1;

	s_startmap_list.generic.type = MTYPE_SPINCONTROL;
	s_startmap_list.generic.x	= -8*scale;
	s_startmap_list.generic.y	= 0 + offset;
	s_startmap_list.generic.name	= "initial map";
	s_startmap_list.itemnames = (const char **) mapnames;
	s_startmap_list.generic.callback = MapInfoFunc;

	s_rules_box.generic.type = MTYPE_SPINCONTROL;
	s_rules_box.generic.x	= -8*scale;
	s_rules_box.generic.y	= FONTSCALE*20*scale + offset;
	s_rules_box.generic.name	= "rules";
	s_rules_box.itemnames = dm_coop_names;
	s_rules_box.curvalue = 0;
	s_rules_box.generic.callback = RulesChangeFunc;

	s_mutators_action.generic.type = MTYPE_ACTION;
	s_mutators_action.generic.x	= -8*scale;
	s_mutators_action.generic.y	= FONTSCALE*36*scale + offset;
	s_mutators_action.generic.cursor_offset = -8;
	s_mutators_action.generic.statusbar = NULL;
	s_mutators_action.generic.name	= "mutators";
	s_mutators_action.generic.callback = MutatorFunc;

	s_grapple_box.generic.type = MTYPE_SPINCONTROL;
	s_grapple_box.generic.x	= -8*scale;
	s_grapple_box.generic.y	= FONTSCALE*52*scale + offset;
	s_grapple_box.generic.name	= "grapple hook";
	s_grapple_box.itemnames = offon;
	s_grapple_box.curvalue = 0;

	s_antilag_box.generic.type = MTYPE_SPINCONTROL;
	s_antilag_box.generic.x	= -8*scale;
	s_antilag_box.generic.y	= FONTSCALE*68*scale + offset;
	s_antilag_box.generic.name	= "antilag";
	s_antilag_box.itemnames = offon;
	s_antilag_box.curvalue = 1;

	s_timelimit_field.generic.type = MTYPE_FIELD;
	s_timelimit_field.generic.name = "time limit ";
	s_timelimit_field.generic.flags = QMF_NUMBERSONLY;
	s_timelimit_field.generic.x	= 8*scale;
	s_timelimit_field.generic.y	= FONTSCALE*84*scale + offset;
	s_timelimit_field.generic.statusbar = "0 = no limit";
	s_timelimit_field.length = 3;
	s_timelimit_field.visible_length = 3;
	strcpy( s_timelimit_field.buffer, Cvar_VariableString("timelimit") );

	s_fraglimit_field.generic.type = MTYPE_FIELD;
	s_fraglimit_field.generic.name = "frag limit ";
	s_fraglimit_field.generic.flags = QMF_NUMBERSONLY;
	s_fraglimit_field.generic.x	= 8*scale;
	s_fraglimit_field.generic.y	= FONTSCALE*102*scale + offset;
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
	s_maxclients_field.generic.x	= 8*scale;
	s_maxclients_field.generic.y	= FONTSCALE*120*scale + offset;
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
	s_hostname_field.generic.x	= 8*scale;
	s_hostname_field.generic.y	= FONTSCALE*138*scale + offset;
	s_hostname_field.generic.statusbar = NULL;
	s_hostname_field.length = 12;
	s_hostname_field.visible_length = 12;
	strcpy( s_hostname_field.buffer, Cvar_VariableString("hostname") );

	s_public_box.generic.type = MTYPE_SPINCONTROL;
	s_public_box.generic.x	= -8*scale;
	s_public_box.generic.y	= FONTSCALE*154*scale + offset;
	s_public_box.generic.name = "public server";
	s_public_box.itemnames = public_yn;
	s_public_box.curvalue = 1;


#if defined WIN32_VARIANT
	s_dedicated_box.generic.type = MTYPE_SPINCONTROL;
	s_dedicated_box.generic.x	= -8*scale;
	s_dedicated_box.generic.y	= FONTSCALE*164*scale + offset;
	s_dedicated_box.generic.name = "dedicated server";
	s_dedicated_box.itemnames = public_yn;
	s_dedicated_box.curvalue = 0;
#else
	// may or may not need this when disabling dedicated server menu
	s_dedicated_box.generic.type = -1;
	s_dedicated_box.generic.x	= 0;
	s_dedicated_box.generic.y	= 0;
	s_dedicated_box.generic.name = NULL;
	s_dedicated_box.itemnames = NULL;
	s_dedicated_box.curvalue = 0;
#endif

	s_skill_box.generic.type = MTYPE_SPINCONTROL;
	s_skill_box.generic.x	= -8*scale;
	s_skill_box.generic.y	= FONTSCALE*174*scale + offset;
	s_skill_box.generic.name	= "skill level";
	s_skill_box.itemnames = skill;
	s_skill_box.curvalue = 1;

	s_startserver_dmoptions_action.generic.type = MTYPE_ACTION;
	s_startserver_dmoptions_action.generic.name	= " deathmatch and bot flags";
	s_startserver_dmoptions_action.generic.x	= 212*scale;
	s_startserver_dmoptions_action.generic.y	= FONTSCALE*196*scale + offset;
	s_startserver_dmoptions_action.generic.cursor_offset = -8;
	s_startserver_dmoptions_action.generic.statusbar = NULL;
	s_startserver_dmoptions_action.generic.callback = DMOptionsFunc;

	s_startserver_start_action.generic.type = MTYPE_ACTION;
	s_startserver_start_action.generic.name	= " begin";
	s_startserver_start_action.generic.x	= -8*scale;
	s_startserver_start_action.generic.y	= FONTSCALE*214*scale + offset;
	s_startserver_start_action.generic.cursor_offset = -8;
	s_startserver_start_action.generic.callback = StartServerActionFunc;

	for ( i = 0; i < 5; i++) { //initialize it
		s_startserver_map_data[i].generic.type	= MTYPE_SEPARATOR;
		s_startserver_map_data[i].generic.name	= "no data";
		s_startserver_map_data[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_startserver_map_data[i].generic.x		= 180*scale;
		s_startserver_map_data[i].generic.y		= FONTSCALE*241*scale + offset + i*10*scale;
	}

	Menu_AddItem( &s_startserver_menu, &s_startmap_list );
	Menu_AddItem( &s_startserver_menu, &s_rules_box );
	Menu_AddItem( &s_startserver_menu, &s_mutators_action );
	Menu_AddItem( &s_startserver_menu, &s_grapple_box );
	Menu_AddItem( &s_startserver_menu, &s_antilag_box );
	Menu_AddItem( &s_startserver_menu, &s_timelimit_field );
	Menu_AddItem( &s_startserver_menu, &s_fraglimit_field );
	Menu_AddItem( &s_startserver_menu, &s_maxclients_field );
	Menu_AddItem( &s_startserver_menu, &s_hostname_field );
	Menu_AddItem( &s_startserver_menu, &s_public_box );
#if defined WIN32_VARIANT
	Menu_AddItem( &s_startserver_menu, &s_dedicated_box );
#endif
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

	mappicalpha += cls.frametime; //fade map pic in
	if(mappicalpha > 1)
		mappicalpha = 1;

	offset*=scale;

	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back"); //draw black background first
	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	sprintf(path, "/levelshots/%s", startmap);
	M_Banner( "m_startserver", banneralpha );
	M_MapPic(path, mappicalpha);

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

	char stem[MAX_QPATH];
	char relative_path[MAX_QPATH];
	char bot_filename[MAX_OSPATH];

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
	if(s_rules_box.curvalue == 1 || s_rules_box.curvalue == 4 || s_rules_box.curvalue == 5)
	{ // team game
		strcpy( stem, "team" );
	}
	else
	{ // non-team, bots per map
		strcpy( stem, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
		for(i = 0; i < strlen(stem); i++)
			stem[i] = tolower( stem[i] );
	}
	Com_sprintf( relative_path, sizeof(relative_path), BOT_GAMEDATA"/%s.tmp", stem );
	FS_FullWritePath( bot_filename, sizeof(bot_filename), relative_path );

	if((pOut = fopen(bot_filename, "wb" )) == NULL)
	{
		Com_DPrintf("BotAction: failed fopen for write: %s\n", bot_filename );
		return; // bail
	}

	szr = fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	for (i = 7; i > -1; i--) {
		if(strcmp(bot[i].name, "...empty slot"))
			szr = fwrite(bot[i].userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut);
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
	int i, count;
	char *info;
	char bot_filename[MAX_OSPATH];
	char stem[MAX_QPATH];
	char relative_path[MAX_QPATH];

	if(s_rules_box.curvalue == 1 || s_rules_box.curvalue == 4 || s_rules_box.curvalue == 5)
	{ // team game
		strcpy( stem, "team" );
	}
	else
	{ // non-team, bots per map
		strcpy( stem, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
		for(i = 0; i < strlen(stem); i++)
			stem[i] = tolower( stem[i] );
	}
	Com_sprintf( relative_path, sizeof(relative_path), BOT_GAMEDATA"/%s.tmp", stem );
	if ( !FS_FullPath( bot_filename, sizeof(bot_filename), relative_path ) )
	{
		Com_DPrintf("Read_Bot_Info: %s/%s not found\n", BOT_GAMEDATA, relative_path );
		return;
	}

	if((pIn = fopen(bot_filename, "rb" )) == NULL)
	{
		Com_DPrintf("Read_Bot_Info: failed file open for read: %s", bot_filename );
		return;
	}

	szr = fread(&count,sizeof (int),1,pIn);
	if(count>8)
		count = 8;

	for(i=0;i<count;i++)
	{

		szr = fread(bot[i].userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn);

		info = Info_ValueForKey (bot[i].userinfo, "name");
		strcpy(bot[i].name, info);
	}

    fclose(pIn);
}
void DMOptions_MenuInit( void )
{
	int i, y;

	static const char *yes_no_names[] =
	{
		"no", "yes", 0
	};
	static const char *teamplay_names[] =
	{
		"disabled", "enabled", 0
	};
	int dmflags = Cvar_VariableValue( "dmflags" );

	float	scale;

	scale = (float)(viddef.height)/600;

	banneralpha = 0.1;

	for(i = 0; i < 8; i++)
		strcpy(bot[i].name, "...empty slot");

	Read_Bot_Info();

	y = 20*scale;
	s_dmoptions_menu.x = viddef.width*0.50 + 100*scale;
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
	s_weapons_stay_box.generic.y	= y += FONTSCALE*10*scale;
	s_weapons_stay_box.generic.name	= "weapons stay";
	s_weapons_stay_box.generic.callback = DMFlagCallback;
	s_weapons_stay_box.itemnames = yes_no_names;
	s_weapons_stay_box.curvalue = ( dmflags & DF_WEAPONS_STAY ) != 0;

	s_instant_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_instant_powerups_box.generic.x	= 0;
	s_instant_powerups_box.generic.y	= y += FONTSCALE*10*scale;
	s_instant_powerups_box.generic.name	= "instant powerups";
	s_instant_powerups_box.generic.callback = DMFlagCallback;
	s_instant_powerups_box.itemnames = yes_no_names;
	s_instant_powerups_box.curvalue = ( dmflags & DF_INSTANT_ITEMS ) != 0;

	s_powerups_box.generic.type = MTYPE_SPINCONTROL;
	s_powerups_box.generic.x	= 0;
	s_powerups_box.generic.y	= y += FONTSCALE*10*scale;
	s_powerups_box.generic.name	= "allow powerups";
	s_powerups_box.generic.callback = DMFlagCallback;
	s_powerups_box.itemnames = yes_no_names;
	s_powerups_box.curvalue = ( dmflags & DF_NO_ITEMS ) == 0;

	s_health_box.generic.type = MTYPE_SPINCONTROL;
	s_health_box.generic.x	= 0;
	s_health_box.generic.y	= y += FONTSCALE*10*scale;
	s_health_box.generic.callback = DMFlagCallback;
	s_health_box.generic.name	= "allow health";
	s_health_box.itemnames = yes_no_names;
	s_health_box.curvalue = ( dmflags & DF_NO_HEALTH ) == 0;

	s_armor_box.generic.type = MTYPE_SPINCONTROL;
	s_armor_box.generic.x	= 0;
	s_armor_box.generic.y	= y += FONTSCALE*10*scale;
	s_armor_box.generic.name	= "allow armor";
	s_armor_box.generic.callback = DMFlagCallback;
	s_armor_box.itemnames = yes_no_names;
	s_armor_box.curvalue = ( dmflags & DF_NO_ARMOR ) == 0;

	s_spawn_farthest_box.generic.type = MTYPE_SPINCONTROL;
	s_spawn_farthest_box.generic.x	= 0;
	s_spawn_farthest_box.generic.y	= y += FONTSCALE*10*scale;
	s_spawn_farthest_box.generic.name	= "spawn farthest";
	s_spawn_farthest_box.generic.callback = DMFlagCallback;
	s_spawn_farthest_box.itemnames = yes_no_names;
	s_spawn_farthest_box.curvalue = ( dmflags & DF_SPAWN_FARTHEST ) != 0;

	s_samelevel_box.generic.type = MTYPE_SPINCONTROL;
	s_samelevel_box.generic.x	= 0;
	s_samelevel_box.generic.y	= y += FONTSCALE*10*scale;
	s_samelevel_box.generic.name	= "same map";
	s_samelevel_box.generic.callback = DMFlagCallback;
	s_samelevel_box.itemnames = yes_no_names;
	s_samelevel_box.curvalue = ( dmflags & DF_SAME_LEVEL ) != 0;

	s_force_respawn_box.generic.type = MTYPE_SPINCONTROL;
	s_force_respawn_box.generic.x	= 0;
	s_force_respawn_box.generic.y	= y += FONTSCALE*10*scale;
	s_force_respawn_box.generic.name	= "force respawn";
	s_force_respawn_box.generic.callback = DMFlagCallback;
	s_force_respawn_box.itemnames = yes_no_names;
	s_force_respawn_box.curvalue = ( dmflags & DF_FORCE_RESPAWN ) != 0;

	s_teamplay_box.generic.type = MTYPE_SPINCONTROL;
	s_teamplay_box.generic.x	= 0;
	s_teamplay_box.generic.y	= y += FONTSCALE*10*scale;
	s_teamplay_box.generic.name	= "teamplay";
	s_teamplay_box.generic.callback = DMFlagCallback;
	s_teamplay_box.itemnames = teamplay_names;

	s_allow_exit_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_exit_box.generic.x	= 0;
	s_allow_exit_box.generic.y	= y += FONTSCALE*10*scale;
	s_allow_exit_box.generic.name	= "allow exit";
	s_allow_exit_box.generic.callback = DMFlagCallback;
	s_allow_exit_box.itemnames = yes_no_names;
	s_allow_exit_box.curvalue = ( dmflags & DF_ALLOW_EXIT ) != 0;

	s_infinite_ammo_box.generic.type = MTYPE_SPINCONTROL;
	s_infinite_ammo_box.generic.x	= 0;
	s_infinite_ammo_box.generic.y	= y += FONTSCALE*10*scale;
	s_infinite_ammo_box.generic.name	= "infinite ammo";
	s_infinite_ammo_box.generic.callback = DMFlagCallback;
	s_infinite_ammo_box.itemnames = yes_no_names;
	s_infinite_ammo_box.curvalue = ( dmflags & DF_INFINITE_AMMO ) != 0;

	s_fixed_fov_box.generic.type = MTYPE_SPINCONTROL;
	s_fixed_fov_box.generic.x	= 0;
	s_fixed_fov_box.generic.y	= y += FONTSCALE*10*scale;
	s_fixed_fov_box.generic.name	= "fixed FOV";
	s_fixed_fov_box.generic.callback = DMFlagCallback;
	s_fixed_fov_box.itemnames = yes_no_names;
	s_fixed_fov_box.curvalue = ( dmflags & DF_FIXED_FOV ) != 0;

	s_quad_drop_box.generic.type = MTYPE_SPINCONTROL;
	s_quad_drop_box.generic.x	= 0;
	s_quad_drop_box.generic.y	= y += FONTSCALE*10*scale;
	s_quad_drop_box.generic.name	= "quad drop";
	s_quad_drop_box.generic.callback = DMFlagCallback;
	s_quad_drop_box.itemnames = yes_no_names;
	s_quad_drop_box.curvalue = ( dmflags & DF_QUAD_DROP ) != 0;

	s_friendlyfire_box.generic.type = MTYPE_SPINCONTROL;
	s_friendlyfire_box.generic.x	= 0;
	s_friendlyfire_box.generic.y	= y += FONTSCALE*10*scale;
	s_friendlyfire_box.generic.name	= "friendly fire";
	s_friendlyfire_box.generic.callback = DMFlagCallback;
	s_friendlyfire_box.itemnames = yes_no_names;
	s_friendlyfire_box.curvalue = ( dmflags & DF_NO_FRIENDLY_FIRE ) == 0;

	s_botchat_box.generic.type = MTYPE_SPINCONTROL;
	s_botchat_box.generic.x	= 0;
	s_botchat_box.generic.y	= y += FONTSCALE*10*scale;
	s_botchat_box.generic.name	= "bot chat";
	s_botchat_box.generic.callback = DMFlagCallback;
	s_botchat_box.itemnames = yes_no_names;
	s_botchat_box.curvalue = ( dmflags & DF_BOTCHAT ) == 0;

	s_bot_fuzzyaim_box.generic.type = MTYPE_SPINCONTROL;
	s_bot_fuzzyaim_box.generic.x	= 0;
	s_bot_fuzzyaim_box.generic.y	= y += FONTSCALE*10*scale;
	s_bot_fuzzyaim_box.generic.name	= "bot fuzzy aim";
	s_bot_fuzzyaim_box.generic.callback = DMFlagCallback;
	s_bot_fuzzyaim_box.itemnames = yes_no_names;
	s_bot_fuzzyaim_box.curvalue = ( dmflags & DF_BOT_FUZZYAIM ) == 0;

	s_bot_auto_save_nodes_box.generic.type = MTYPE_SPINCONTROL;
	s_bot_auto_save_nodes_box.generic.x	= 0;
	s_bot_auto_save_nodes_box.generic.y	= y += FONTSCALE*10*scale;
	s_bot_auto_save_nodes_box.generic.name	= "auto node save";
	s_bot_auto_save_nodes_box.generic.callback = DMFlagCallback;
	s_bot_auto_save_nodes_box.itemnames = yes_no_names;
	s_bot_auto_save_nodes_box.curvalue = ( dmflags & DF_BOT_AUTOSAVENODES ) == 1;

	s_bot_levelad_box.generic.type = MTYPE_SPINCONTROL;
	s_bot_levelad_box.generic.x	= 0;
	s_bot_levelad_box.generic.y	= y += FONTSCALE*10*scale;
	s_bot_levelad_box.generic.name	= "repeat level if bot wins";
	s_bot_levelad_box.generic.callback = DMFlagCallback;
	s_bot_levelad_box.itemnames = yes_no_names;
	s_bot_levelad_box.curvalue = ( dmflags & DF_BOT_LEVELAD ) == 0;

	s_bots_box.generic.type = MTYPE_SPINCONTROL;
	s_bots_box.generic.x	= 0;
	s_bots_box.generic.y	= y += FONTSCALE*10*scale;
	s_bots_box.generic.name	= "bots in game";
	s_bots_box.generic.callback = DMFlagCallback;
	s_bots_box.itemnames = yes_no_names;
	s_bots_box.curvalue = ( dmflags & DF_BOTS ) == 0;

	for (i = 0; i < 8; i++) {
		s_bots_bot_action[i].generic.type = MTYPE_ACTION;
		s_bots_bot_action[i].generic.name = bot[i].name;
		s_bots_bot_action[i].generic.x = 0;
		s_bots_bot_action[i].generic.y = y+FONTSCALE*10*scale*(i+2);
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

	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back"); //draw black background first
	M_Banner( "m_dmoptions", banneralpha );

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
// static menuframework_s s_downloadoptions_menu; // unused

// static menuseparator_s	s_download_title; // unused
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

	banneralpha = 0.1;

	s_addressbook_menu.x = viddef.width / 2 - 138*scale;
	s_addressbook_menu.y = viddef.height / 2 - 158*scale;
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
		s_addressbook_fields[i].generic.y		= FONTSCALE*i * 18*scale + 0;
		s_addressbook_fields[i].generic.localdata[0] = i;
		s_addressbook_fields[i].cursor			= 0;
		s_addressbook_fields[i].length			= 15*scale;
		s_addressbook_fields[i].visible_length	= 15*scale;

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
	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "conback"); //draw black background first
	M_Banner( "m_banner_main", banneralpha );
	Menu_Draw( &s_addressbook_menu );
}

void M_Menu_AddressBook_f(void)
{
	AddressBook_MenuInit();
	M_PushMenu( AddressBook_MenuDraw, AddressBook_MenuKey );
}

/*
=============================================================================

PLAYER RANKING MENU

=============================================================================
*/

static menuframework_s	s_playerranking_menu;
static menuaction_s		s_playerranking_title;
static menuaction_s		s_playerranking_name;
static menuaction_s		s_playerranking_rank;
static menuaction_s		s_playerranking_fragrate;
static menuaction_s		s_playerranking_totaltime;
static menuaction_s		s_playerranking_totalfrags;
static menuaction_s		s_playerranking_ttheader;
static menuaction_s		s_playerranking_topten[10];
char rank[32];
char fragrate[32];
char playername[64]; // a print field, not just name
char totaltime[32];
char totalfrags[32];
char topTenList[10][64];

void PlayerRanking_MenuInit( void )
{
	extern cvar_t *name;
	PLAYERSTATS player;
	PLAYERSTATS topTenPlayers[10];
	float scale;
	int offset;
	int i;

	scale = (float)(viddef.height)/600;

	banneralpha = 0.1;

	s_playerranking_menu.x = viddef.width / 2 - 170*scale;
	s_playerranking_menu.y = viddef.height / 2 - 160*scale;
	s_playerranking_menu.nitems = 0;

	Q_strncpyz2( player.playername, name->string, sizeof(player.playername) );

	player.totalfrags = player.totaltime = player.ranking = 0;
	player = getPlayerRanking ( player );

	Com_sprintf(playername, sizeof(playername), "Name: %s", player.playername);
	if(player.ranking > 0)
		Com_sprintf(rank, sizeof(rank), "Rank: ^1%i", player.ranking);
	else
		Com_sprintf(rank, sizeof(rank), "Rank: ^1Unranked");
	Com_sprintf(fragrate, sizeof(fragrate), "Frag Rate: %6.2f", (float)(player.totalfrags/player.totaltime));
	Com_sprintf(totalfrags, sizeof(totalfrags), "Total Frags: ^1%i", player.totalfrags);
	Com_sprintf(totaltime, sizeof(totaltime), "Total Time: %6.2f", player.totaltime);

	s_playerranking_title.generic.type	= MTYPE_ACTION;
	s_playerranking_title.generic.name	= "Player Ranking and Stats";
	s_playerranking_title.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerranking_title.generic.x		= 32*scale;
	s_playerranking_title.generic.y		= 0;

	offset = GetColorTokens(playername);

	s_playerranking_name.generic.type	= MTYPE_COLORTXT;
	s_playerranking_name.generic.name	= playername;
	s_playerranking_name.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerranking_name.generic.x		= -31*offset*scale+16*scale;
	s_playerranking_name.generic.y		= FONTSCALE*20*scale;

	s_playerranking_rank.generic.type	= MTYPE_COLORTXT;
	s_playerranking_rank.generic.name	= rank;
	s_playerranking_rank.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerranking_rank.generic.x		= -16*scale;
	s_playerranking_rank.generic.y		= FONTSCALE*40*scale;

	s_playerranking_fragrate.generic.type	= MTYPE_COLORTXT;
	s_playerranking_fragrate.generic.name	= fragrate;
	s_playerranking_fragrate.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerranking_fragrate.generic.x		= 16*scale;
	s_playerranking_fragrate.generic.y		= FONTSCALE*60*scale;

	s_playerranking_totalfrags.generic.type	= MTYPE_COLORTXT;
	s_playerranking_totalfrags.generic.name	= totalfrags;
	s_playerranking_totalfrags.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerranking_totalfrags.generic.x		= -16*scale;
	s_playerranking_totalfrags.generic.y		= FONTSCALE*80*scale;

	s_playerranking_totaltime.generic.type	= MTYPE_COLORTXT;
	s_playerranking_totaltime.generic.name	= totaltime;
	s_playerranking_totaltime.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerranking_totaltime.generic.x		= 16*scale;
	s_playerranking_totaltime.generic.y		= FONTSCALE*100*scale;

	s_playerranking_ttheader.generic.type	= MTYPE_ACTION;
	s_playerranking_ttheader.generic.name	= "Top Ten Players";
	s_playerranking_ttheader.generic.flags	= QMF_LEFT_JUSTIFY;
	s_playerranking_ttheader.generic.x		= 32*scale;
	s_playerranking_ttheader.generic.y		= FONTSCALE*120*scale;

	Menu_AddItem( &s_playerranking_menu, &s_playerranking_title );
	Menu_AddItem( &s_playerranking_menu, &s_playerranking_name );
	Menu_AddItem( &s_playerranking_menu, &s_playerranking_rank );
	Menu_AddItem( &s_playerranking_menu, &s_playerranking_fragrate );
	Menu_AddItem( &s_playerranking_menu, &s_playerranking_totalfrags );
	Menu_AddItem( &s_playerranking_menu, &s_playerranking_totaltime );
	Menu_AddItem( &s_playerranking_menu, &s_playerranking_ttheader );

	for(i = 0; i < 10; i++) {

		topTenPlayers[i].totalfrags = topTenPlayers[i].totaltime = topTenPlayers[i].ranking = 0;
		topTenPlayers[i] = getPlayerByRank ( i+1, topTenPlayers[i] );

		if(i < 9)
			Com_sprintf(topTenList[i], sizeof(topTenList[i]), "Rank: ^1%i %s", topTenPlayers[i].ranking, topTenPlayers[i].playername);
		else
			Com_sprintf(topTenList[i], sizeof(topTenList[i]), "Rank:^1%i %s", topTenPlayers[i].ranking, topTenPlayers[i].playername);

		offset = GetColorTokens(topTenPlayers[i].playername);

		s_playerranking_topten[i].generic.type	= MTYPE_COLORTXT;
		s_playerranking_topten[i].generic.name	= topTenList[i];
		s_playerranking_topten[i].generic.flags	= QMF_LEFT_JUSTIFY;
		s_playerranking_topten[i].generic.x		= -31*offset*scale - 16*scale;
		s_playerranking_topten[i].generic.y		= FONTSCALE*(140+(i*10))*scale;

		Menu_AddItem( &s_playerranking_menu, &s_playerranking_topten[i] );
	}
}

const char *PlayerRanking_MenuKey( int key )
{
	return Default_MenuKey( &s_playerranking_menu, key );
}

void PlayerRanking_MenuDraw(void)
{
	float scale;

	scale = (float)(viddef.height)/600;

	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back");
	M_Banner( "m_player", banneralpha );

	Menu_Draw( &s_playerranking_menu );
}

void M_Menu_PlayerRanking_f(void)
{
	PlayerRanking_MenuInit();
	M_PushMenu( PlayerRanking_MenuDraw, PlayerRanking_MenuKey );
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
static menufield_s		s_player_fov_field;

#define MAX_DISPLAYNAME 16
#define MAX_PLAYERMODELS 1024

typedef struct
{
	int		nskins;
	char	**skindisplaynames;
	char	displayname[MAX_DISPLAYNAME];
	char	directory[MAX_OSPATH];
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
	s_player_skin_box.itemnames = (const char **) s_pmi[s_player_model_box.curvalue].skindisplaynames;
	s_player_skin_box.curvalue = 0;
}

static void FovCallBack( void *unused )
{
	Cvar_SetValue( "fov", atoi(s_player_fov_field.buffer));
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

	strcpy( scratch, skin );
	*strrchr( scratch, '.' ) = 0;
	strcat( scratch, "_i.jpg" );

	for ( i = 0; i < npcxfiles; i++ )
	{
		if ( strcmp( pcxfiles[i], scratch ) == 0 )
			return true;
	}

	return false;
}

static void PlayerConfig_ScanDirectories( void )
{
	char scratch[1024];
	int ndirs = 0, npms = 0;
	char **dirnames;
	int i;

	s_numplayermodels = 0;

	//get dirs from gamedir first.
	dirnames = FS_ListFilesInFS( "players/*.*", &ndirs, SFF_SUBDIR, 0 );

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
		if (!FS_FileExists(scratch))
		{
			free( dirnames[i] );
			dirnames[i] = 0;
			continue;
		}

		// verify the existence of at least one skin(note, do not mix .tga and .jpeg)
		strcpy( scratch, dirnames[i] );
		strcat( scratch, "/*.jpg" );
		pcxnames = FS_ListFilesInFS( scratch, &npcxfiles, 0,
		    SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		if(!pcxnames) {
			// check for .tga, though this is no longer used for current models
			strcpy( scratch, dirnames[i] );
			strcat( scratch, "/*.tga" );
			pcxnames = FS_ListFilesInFS( scratch, &npcxfiles, 0,
				SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );
		}

		if ( !pcxnames )
		{
			free( dirnames[i] );
			dirnames[i] = 0;
			continue;
		}

		// count valid skins, which consist of a skin with a matching "_i" icon
		for ( k = 0; k < npcxfiles; k++ )
		{
			if ( !strstr( pcxnames[k], "_i.tga" ) || !strstr( pcxnames[k], "_i.jpg" ))
			{
				if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles) )
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
		for ( s = 0, k = 0; k < npcxfiles; k++ )
		{
			char *a, *b, *c;

			if ( !strstr( pcxnames[k], "_i.tga" ) )
			{
				if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles ) )
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

					skinnames[s] = _strdup( scratch );
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
		free( dirnames );
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
	// extern cvar_t *team; // unused
	// extern cvar_t *skin; // unused
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

	banneralpha = 0.1;

	PlayerConfig_ScanDirectories();

	if (s_numplayermodels == 0)
		return false;

	if ( hand->value < 0 || hand->value > 2 )
		Cvar_SetValue( "hand", 0 );

	Q_strncpyz( currentdirectory, Cvar_VariableString ("skin"), sizeof(currentdirectory)-1);
    // Richard Stanway's Q1 code says there is a buffer overflow here.
    // strcpy( currentdirectory, skin->string );

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
		strcpy( currentdirectory, "martianenforcer" );
		strcpy( currentskin, "default" );
	}

	qsort( s_pmi, s_numplayermodels, sizeof( s_pmi[0] ), pmicmpfnc );

	memset( s_pmnames, 0, sizeof( s_pmnames ) );
	for ( i = 0; i < s_numplayermodels; i++ )
	{
		s_pmnames[i] = s_pmi[i].displayname;
		if ( Q_strcasecmp( s_pmi[i].directory, currentdirectory ) == 0 )
		{
			int j;

			currentdirectoryindex = i;

			for ( j = 0; j < s_pmi[i].nskins; j++ )
			{
				if ( Q_strcasecmp( s_pmi[i].skindisplaynames[j], currentskin ) == 0 )
				{
					currentskinindex = j;
					break;
				}
			}
		}
	}

	s_player_config_menu.x = viddef.width / 2 - 120;
	s_player_config_menu.y = viddef.height / 2 - 158*scale;
	s_player_config_menu.nitems = 0;

	s_player_name_field.generic.type = MTYPE_FIELD;
	s_player_name_field.generic.name = "name";
	s_player_name_field.generic.callback = 0;
	s_player_name_field.generic.x		= FONTSCALE*-32;
	s_player_name_field.generic.y		= 0;
	s_player_name_field.length	= 20;
	s_player_name_field.visible_length = 20;
	Q_strncpyz2( s_player_name_field.buffer, name->string, sizeof(s_player_name_field.buffer) );
	s_player_name_field.cursor = strlen( s_player_name_field.buffer );

	s_player_model_box.generic.type = MTYPE_SPINCONTROL;
	s_player_model_box.generic.name = "model";
	s_player_model_box.generic.x	= FONTSCALE*-32;
	s_player_model_box.generic.y	= FONTSCALE*70*scale;
	s_player_model_box.generic.callback = ModelCallback;
	s_player_model_box.generic.cursor_offset = -56;
	s_player_model_box.curvalue = currentdirectoryindex;
	s_player_model_box.itemnames = (const char **) s_pmnames;

	s_player_skin_box.generic.type = MTYPE_SPINCONTROL;
	s_player_skin_box.generic.name = "skin";
	s_player_skin_box.generic.x	= FONTSCALE*-32;
	s_player_skin_box.generic.y	= FONTSCALE*94*scale;
	s_player_skin_box.generic.callback = 0;
	s_player_skin_box.generic.cursor_offset = -56;
	s_player_skin_box.curvalue = currentskinindex;
	s_player_skin_box.itemnames = (const char **) s_pmi[currentdirectoryindex].skindisplaynames;

	s_player_handedness_box.generic.type = MTYPE_SPINCONTROL;
	s_player_handedness_box.generic.name = "handedness";
	s_player_handedness_box.generic.x	= FONTSCALE*-32;
	s_player_handedness_box.generic.y	= FONTSCALE*118*scale;
	s_player_handedness_box.generic.cursor_offset = -56;
	s_player_handedness_box.generic.callback = HandednessCallback;
	s_player_handedness_box.curvalue = Cvar_VariableValue( "hand" );
	s_player_handedness_box.itemnames = handedness;

	s_player_fov_field.generic.type = MTYPE_FIELD;
	s_player_fov_field.generic.name = "fov";
	s_player_fov_field.generic.callback = 0;
	s_player_fov_field.generic.x		= FONTSCALE*-32;
	s_player_fov_field.generic.y		= FONTSCALE*132*scale;
	s_player_fov_field.length	= 6;
	s_player_fov_field.visible_length = 6;
	s_player_fov_field.generic.callback = FovCallBack;
	strcpy( s_player_fov_field.buffer, fov->string );
	s_player_fov_field.cursor = strlen( fov->string );

	for (i = 0; i < sizeof(rate_tbl) / sizeof(*rate_tbl) - 1; i++)
		if (Cvar_VariableValue("rate") == rate_tbl[i])
			break;

	s_player_rate_box.generic.type = MTYPE_SPINCONTROL;
	s_player_rate_box.generic.x	= FONTSCALE*-32;
	s_player_rate_box.generic.y	= FONTSCALE*146*scale;
	s_player_rate_box.generic.name	= "connection";
	s_player_rate_box.generic.callback = RateCallback;
	s_player_rate_box.curvalue = i;
	s_player_rate_box.itemnames = rate_names;

	s_allow_download_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_box.generic.x	= FONTSCALE*72;
	s_allow_download_box.generic.y	= FONTSCALE*186*scale;
	s_allow_download_box.generic.name	= "allow downloading";
	s_allow_download_box.generic.callback = DownloadCallback;
	s_allow_download_box.itemnames = yes_no_names;
	s_allow_download_box.curvalue = (Cvar_VariableValue("allow_download") != 0);

	s_allow_download_maps_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_maps_box.generic.x	= FONTSCALE*72;
	s_allow_download_maps_box.generic.y	= FONTSCALE*196*scale;
	s_allow_download_maps_box.generic.name	= "maps";
	s_allow_download_maps_box.generic.callback = DownloadCallback;
	s_allow_download_maps_box.itemnames = yes_no_names;
	s_allow_download_maps_box.curvalue = (Cvar_VariableValue("allow_download_maps") != 0);

	s_allow_download_players_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_players_box.generic.x	= FONTSCALE*72;
	s_allow_download_players_box.generic.y	= FONTSCALE*206*scale;
	s_allow_download_players_box.generic.name	= "player models/skins";
	s_allow_download_players_box.generic.callback = DownloadCallback;
	s_allow_download_players_box.itemnames = yes_no_names;
	s_allow_download_players_box.curvalue = (Cvar_VariableValue("allow_download_players") != 0);

	s_allow_download_models_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_models_box.generic.x	= FONTSCALE*72;
	s_allow_download_models_box.generic.y	= FONTSCALE*216*scale;
	s_allow_download_models_box.generic.name	= "models";
	s_allow_download_models_box.generic.callback = DownloadCallback;
	s_allow_download_models_box.itemnames = yes_no_names;
	s_allow_download_models_box.curvalue = (Cvar_VariableValue("allow_download_models") != 0);

	s_allow_download_sounds_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_download_sounds_box.generic.x	= FONTSCALE*72;
	s_allow_download_sounds_box.generic.y	= FONTSCALE*226*scale;
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
	Menu_AddItem( &s_player_config_menu, &s_player_fov_field );
	Menu_AddItem( &s_player_config_menu, &s_player_rate_box );

	//add in shader support for player models, if the player goes into the menu before entering a
	//level, that way we see the shaders.  We only want to do this if they are NOT loaded yet.
	scriptsloaded = Cvar_Get("scriptsloaded", "0", 0);
	if(!scriptsloaded->value)
	{
		Cvar_SetValue("scriptsloaded", 1); //this needs to be reset on vid_restart
		RS_ScanPathForScripts();
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
	char scratch[MAX_OSPATH];
	FILE *modelfile;
	int helmet = false;
	float scale;

	scale = (float)(viddef.height)/600;

	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "menu_back"); //draw black background first
	M_Banner( "m_player", banneralpha );

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.width = viddef.width;
	refdef.height = viddef.height;
	refdef.x = 0;
	refdef.y = 0;
	if((float)viddef.width/(float)viddef.height > 1.5)
		refdef.fov_x = 90;
	else
		refdef.fov_x = 75;
	refdef.fov_y = CalcFov( refdef.fov_x, refdef.width, refdef.height );
	refdef.time = cls.realtime*0.001;

	if(!strcmp(s_player_name_field.buffer, "Player"))
		pNameUnique = false;
	else
		pNameUnique = true;

	if(!pNameUnique) {
		M_DrawTextBox( -32*scale, (int)(FONTSCALE*-95*scale), 40/scale, 2 );
		M_Print( (int)(FONTSCALE*32*scale), (int)(FONTSCALE*-85*scale),  "You must change your player" );
		M_Print( (int)(FONTSCALE*32*scale), (int)(FONTSCALE*-75*scale),  "name before joining a server!" );
	}

	if ( s_pmi[s_player_model_box.curvalue].skindisplaynames )
	{
		static float mframe;
		static float yaw;
		entity_t entity[3];

		memset( &entity, 0, sizeof( entity ) );

		mframe += cls.frametime*150;
		if ( mframe > 390 )
			mframe = 10;
		if ( mframe < 10)
			mframe = 10;

		yaw += cls.frametime*50;
		if (yaw > 360)
			yaw = 0;

		Com_sprintf( scratch, sizeof( scratch ), "players/%s/tris.md2", s_pmi[s_player_model_box.curvalue].directory );
		entity[0].model = R_RegisterModel( scratch );
		Com_sprintf( scratch, sizeof( scratch ), "players/%s/%s.jpg", s_pmi[s_player_model_box.curvalue].directory, s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );
		entity[0].skin = R_RegisterSkin( scratch );

		Com_sprintf( scratch, sizeof( scratch ), "players/%s/weapon.md2", s_pmi[s_player_model_box.curvalue].directory );
		entity[1].model = R_RegisterModel( scratch );
		Com_sprintf( scratch, sizeof( scratch ), "players/%s/weapon.tga", s_pmi[s_player_model_box.curvalue].directory );
		entity[1].skin = R_RegisterSkin( scratch );

		//if a helmet or other special device
			Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.md2", s_pmi[s_player_model_box.curvalue].directory );
		FS_FOpenFile( scratch, &modelfile );
		if ( modelfile )
		{
				helmet = true;
				Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.md2", s_pmi[s_player_model_box.curvalue].directory );
				entity[2].model = R_RegisterModel( scratch );
				Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.tga", s_pmi[s_player_model_box.curvalue].directory );
				entity[2].skin = R_RegisterSkin( scratch );
				fclose(modelfile);
		}

		entity[0].flags = RF_FULLBRIGHT | RF_MENUMODEL;
		entity[0].origin[0] = 80;
		entity[0].origin[1] = -30;
		entity[0].origin[2] = -5;

		entity[1].flags = RF_FULLBRIGHT | RF_MENUMODEL;
		entity[1].origin[0] = 80;
		entity[1].origin[1] = -30;
		entity[1].origin[2] = -5;

		if(helmet)
		{
			entity[2].flags = RF_FULLBRIGHT | RF_TRANSLUCENT | RF_MENUMODEL;
			entity[2].origin[0] = 80;
			entity[2].origin[1] = -30;
			entity[2].origin[2] = -5;
			entity[2].alpha = 0.4;
		}

		VectorCopy( entity[0].origin, entity[0].oldorigin );

		VectorCopy( entity[1].origin, entity[1].oldorigin );

		VectorCopy( entity[2].origin, entity[2].oldorigin );

		entity[0].frame = (int)(mframe/10);
		entity[0].oldframe = (int)(mframe/10) - 1;
		entity[0].backlerp = 1.0;
		entity[0].angles[1] = (int)yaw;

		entity[1].frame = (int)(mframe/10);
		entity[1].oldframe = (int)(mframe/10) - 1;
		entity[1].backlerp = 1.0;
		entity[1].angles[1] = (int)yaw;

		entity[2].frame = (int)(mframe/10);
		entity[2].oldframe = (int)(mframe/10) - 1;
		entity[2].backlerp = 1.0;
		entity[2].angles[1] = (int)yaw;

		refdef.areabits = 0;
		if(helmet)
			refdef.num_entities = 3;
		else
			refdef.num_entities = 2;

		refdef.entities = entity;
		refdef.lightstyles = 0;
		refdef.rdflags = RDF_NOWORLDMODEL;

		Menu_Draw( &s_player_config_menu );

		refdef.height += 4;

		R_RenderFramePlayerSetup( &refdef );

		Com_sprintf( scratch, sizeof( scratch ), "/players/%s/%s_i.tga",
			s_pmi[s_player_model_box.curvalue].directory,
			s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );

		refdef.y = viddef.height / 2 - 70*scale;
		Draw_StretchPic( s_player_config_menu.x - 120*scale, refdef.y - 56*scale, 64*scale, 64*scale, scratch );
	}
}
void PConfigAccept (void)
{
	int i;
	char scratch[1024];

	ValidatePlayerName( s_player_name_field.buffer, sizeof(s_player_name_field.buffer) );
	Cvar_Set( "name", s_player_name_field.buffer );

	if(!strcmp(s_player_name_field.buffer, "Player"))
		pNameUnique = false;
	else
		pNameUnique = true;

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
	banneralpha += cls.frametime;
	if (banneralpha > 1)
		banneralpha = 1;

	M_Background( "conback"); //draw black background first
	M_Banner( "m_quit", banneralpha );

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

	banneralpha = 0.1;

	s_quit_menu.x = viddef.width*0.50 + 48*scale;
	s_quit_menu.y = viddef.height*0.50 - 48*scale;
	s_quit_menu.nitems = 0;

	s_quit_question.generic.type	= MTYPE_SEPARATOR;
	s_quit_question.generic.name	= "Are you sure?";
	s_quit_question.generic.x	= 32*scale;
	s_quit_question.generic.y	= FONTSCALE*scale*40;

	s_quit_yes_action.generic.type	= MTYPE_ACTION;
	s_quit_yes_action.generic.x		= -24*scale;
	s_quit_yes_action.generic.y		= FONTSCALE*scale*60;
	s_quit_yes_action.generic.name	= "  yes";
	s_quit_yes_action.generic.callback = quitActionYes;

	s_quit_no_action.generic.type	= MTYPE_ACTION;
	s_quit_no_action.generic.x		= -24*scale;
	s_quit_no_action.generic.y		= FONTSCALE*scale*70;
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

int Slider_CursorPositionY ( menuslider_s *s )
{
	float range;

	range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );

	if ( range < 0)
		range = 0;
	if ( range > 1)
		range = 1;

	return ( int )( (MENU_FONT_SIZE) + (s->size)*(MENU_FONT_SIZE) * range );
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

int newSliderValueForY (int y, menuslider_s *s)
{
	float newValue;
	int newValueInt;
	int pos = y - (MENU_FONT_SIZE + s->generic.y) - s->generic.parent->y;

	newValue = ((float)pos)/((s->size-1)*(MENU_FONT_SIZE));
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
	// menucommon_s *item = ( menucommon_s * ) menuitem; // unused
	menuslider_s *slider = ( menuslider_s * ) menuitem;

	slider->curvalue = newSliderValueForX(cursor.x, slider);
	Slider_CheckSlide ( slider );
}

void Menu_DragVertSlideItem (menuframework_s *menu, void *menuitem)
{
	// menucommon_s *item = ( menucommon_s * ) menuitem; // unused
	menuslider_s *slider = ( menuslider_s * ) menuitem;

	slider->curvalue = newSliderValueForY(cursor.y, slider);
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

void Menu_ClickVertSlideItem (menuframework_s *menu, void *menuitem)
{
	int min, max;
	menucommon_s *item = ( menucommon_s * ) menuitem;
	menuslider_s *slider = ( menuslider_s * ) menuitem;

	min = menu->y + (item->y + Slider_CursorPositionY(slider) - 4);
	max = menu->y + (item->x + Slider_CursorPositionY(slider) + 4);

	if (cursor.y < min)
		Menu_SlideItem( menu, -1 );
	if (cursor.y > max)
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
			else if (cursor.menuitemtype == MENUITEM_VERTSLIDER)
			{
				Menu_DragVertSlideItem(m, cursor.menuitem);
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
			else if (cursor.menuitemtype == MENUITEM_VERTSLIDER && !cursor.buttonused[MOUSEBUTTON2])
			{
				Menu_ClickVertSlideItem(m, cursor.menuitem);
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

	// dim everything behind it down
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


