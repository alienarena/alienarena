/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2005-2011 COR Entertainment, LLC.

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

extern int CL_GetPingStartTime(netadr_t adr);

extern void RS_LoadScript(char *script);
extern void RS_LoadSpecialScripts(void);
extern void RS_ScanPathForScripts(void);
extern cvar_t *scriptsloaded;
#if defined WIN32_VARIANT
extern char map_music[MAX_PATH];
#else
extern char map_music[MAX_OSPATH];
#endif
extern cvar_t *background_music;
extern cvar_t *background_music_vol;
extern cvar_t *fov;
extern cvar_t *stats_password;

static char *menu_in_sound		= "misc/menu1.wav";
static char *menu_move_sound	= "misc/menu2.wav";
static char *menu_out_sound		= "misc/menu3.wav";
static int pNameUnique;

void M_Menu_Main_f (void);
	void M_Menu_PlayerConfig_f (void);
	void M_Menu_Game_f (void);
		void M_Menu_Credits_f( void );
	void M_Menu_JoinServer_f (void);
			void M_Menu_AddressBook_f( void );
			void M_Menu_PlayerRanking_f( void );
			void M_Menu_Tactical_f( void );
	void M_Menu_StartServer_f (void);
			void M_Menu_BotOptions_f (void);
	void M_Menu_IRC_f (void);
	void M_Menu_Video_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
	void M_Menu_Quit_f (void);

	void M_Menu_Credits( void );

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound

void	(*m_drawfunc) (void);
const char *(*m_keyfunc) (menuframework_s *menu, int key);

static size_t szr; // just for unused result warnings


// common callbacks

static void StrFieldCallback( void *_self )
{
	menufield_s *self = (menufield_s *)_self;
	Cvar_Set( self->generic.localstrings[0], self->buffer);
}

static void IntFieldCallback( void *_self )
{
	menufield_s *self = (menufield_s *)_self;
	Cvar_SetValue( self->generic.localstrings[0], atoi(self->buffer));
}

static menuvec2_t PicSizeFunc (void *_self, FNT_font_t font)
{
	menuvec2_t ret;
	menuitem_s *self = (menuitem_s *)_self;
	ret.x = self->generic.localints[0]*font->size;
	ret.y = self->generic.localints[1]*font->size;
	ret.x += self->generic.localints[2];
	return ret;
}

// most useful if this element will always draw the same pic
static void PicDrawFunc (void *_self, FNT_font_t font)
{
	int x, y;
	menuitem_s *self = (menuitem_s *)_self;
	
	x = Item_GetX (*self) + self->generic.localints[2];
	y = Item_GetY (*self);
	
	Draw_StretchPic (x, y, font->size*self->generic.localints[0], font->size*self->generic.localints[1], self->generic.localstrings[0]);
}


// name lists: list of strings terminated by 0

static const char *onoff_names[] =
{
	"",
	"menu/on",
	0
};

// when you want 0 to be on
static const char *offon_names[] =
{
	"menu/on",
	"",
	0
};

static menuvec2_t IconSpinSizeFunc (void *_self, FNT_font_t font)
{
	menuvec2_t ret;
	menulist_s *self = (menulist_s *)_self;
	
	ret.x = ret.y = font->size;
	ret.x += RCOLUMN_OFFSET;
	if ((self->generic.flags & QMF_LEFT_JUSTIFY)) 
		ret.x += Menu_PredictSize (self->generic.name);
	return ret;
}

static void IconSpinDrawFunc (void *_self, FNT_font_t font)
{
	int x, y;
	menulist_s *self = (menulist_s *)_self;
	
	x = Item_GetX (*self)+RCOLUMN_OFFSET;
	y = Item_GetY (*self);
	if ((self->generic.flags & QMF_LEFT_JUSTIFY))
		x += Menu_PredictSize (self->generic.name);
	Draw_StretchPic (x, y, font->size, font->size, "menu/icon_border");
	if (strlen(self->itemnames[self->curvalue]) > 0)
		Draw_StretchPic (x, y, font->size, font->size, self->itemnames[self->curvalue]);
}

#define setup_tickbox(spinctrl) \
{ \
	(spinctrl).generic.type = MTYPE_SPINCONTROL; \
	(spinctrl).generic.itemsizecallback = IconSpinSizeFunc; \
	(spinctrl).generic.itemdraw = IconSpinDrawFunc; \
	(spinctrl).itemnames = onoff_names; \
	(spinctrl).generic.flags |= QMF_ALLOW_WRAP; \
	(spinctrl).curvalue = 0; \
}

static void RadioSpinDrawFunc (void *_self, FNT_font_t font)
{
	int x, y;
	menulist_s *self = (menulist_s *)_self;
	
	x = Item_GetX (*self)+RCOLUMN_OFFSET;
	y = Item_GetY (*self);
	if ((self->generic.flags & QMF_LEFT_JUSTIFY))
		x += Menu_PredictSize (self->generic.name);
	Draw_StretchPic (x, y, font->size, font->size, "menu/radio_border");
	if (strlen(self->itemnames[self->curvalue]) > 0)
		Draw_StretchPic (x, y, font->size, font->size, self->itemnames[self->curvalue]);
}

#define setup_radiobutton(spinctrl) \
{ \
	(spinctrl).generic.type = MTYPE_SPINCONTROL; \
	(spinctrl).generic.itemsizecallback = IconSpinSizeFunc; \
	(spinctrl).generic.itemdraw = RadioSpinDrawFunc; \
	(spinctrl).itemnames = onoff_names; \
	(spinctrl).generic.flags |= QMF_ALLOW_WRAP; \
	(spinctrl).curvalue = 0; \
}

#define setup_window(parent,window,title) \
{ \
	(window).generic.type = MTYPE_SUBMENU; \
	(window).navagable = true; \
	(window).nitems = 0; \
	(window).bordertitle = title; \
	(window).bordertexture = "menu/m_"; \
	Menu_AddItem (&(parent), &(window)); \
}

// if you just want to add some text to a menu and never need to refer to it
// again (don't use inside a loop!)
#define add_text(menu,text,itflags) \
{\
	static menutxt_s it; \
	it.generic.type = MTYPE_TEXT; \
	it.generic.flags = (itflags); \
	it.generic.name = (text); \
	Menu_AddItem(&(menu), &(it)); \
}

// if you just want to add an action to a menu and never need to refer to it
// again (don't use inside a loop!)
#define add_action(menu,itname,itcallback) \
{\
	static menuaction_s it; \
	it.generic.type = MTYPE_ACTION; \
	it.generic.name = (itname); \
	it.generic.callback = (itcallback); \
	Menu_AddItem(&(menu), &(it)); \
}


// Should be useful for most menus
#define screen_boilerplate(name,struct) \
static void name ## _MenuDraw (void) \
{ \
	Menu_Draw( &struct ); \
} \
\
void M_Menu_ ## name ## _f (void) \
{ \
	name ## _MenuInit(); \
	M_PushMenu ( name ## _MenuDraw, Default_MenuKey, &struct); \
}


//=============================================================================
/* Support Routines */

#define	MAX_MENU_DEPTH	8


typedef struct
{
	void	(*draw) (void);
	const char *(*key) (menuframework_s *screen, int k);
	menuframework_s *screen;
	int		start_x, end_x;
	int		offset;
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

	Draw_AlphaStretchPic( global_menu_xoffset + viddef.width / 2 - (w / 2), viddef.height / 2 - 260*scale, w, h, name, alpha );

}

static void CrosshairPicDrawFunc (void *_self, FNT_font_t font)
{
	int x, y;
	menuitem_s *self = (menuitem_s *)_self;
	x = Item_GetX (*self) + RCOLUMN_OFFSET;
	y = Item_GetY (*self);
	
	Draw_StretchPic (x, y, font->size*5, font->size*5, crosshair->string);
}


void refreshCursorButtons(void)
{
	cursor.buttonused[MOUSEBUTTON2] = true;
	cursor.buttonclicks[MOUSEBUTTON2] = 0;
	cursor.buttonused[MOUSEBUTTON1] = true;
	cursor.buttonclicks[MOUSEBUTTON1] = 0;
}

void M_PushMenu ( void (*draw) (void), const char *(*key) (menuframework_s *screen, int k), menuframework_s *screen)
{
	int		i;
	
	if (Cvar_VariableValue ("maxclients") == 1
		&& Com_ServerState ())
		Cvar_Set ("paused", "1");

	// if this menu is already present, drop back to that level
	// to avoid stacking menus by hotkeys
	for (i=0 ; i<m_menudepth ; i++)
		if (m_layers[i].draw == draw &&
			m_layers[i].key == key && 
			m_layers[i].screen == screen)
		{
			m_menudepth = i;
		}

	if (i == m_menudepth)
	{
		if (m_menudepth >= MAX_MENU_DEPTH)
			Com_Error (ERR_FATAL, "M_PushMenu: MAX_MENU_DEPTH");
		m_menudepth++;
		m_layers[m_menudepth-1].offset = global_menu_xoffset;
		m_layers[m_menudepth].screen = screen;
		m_layers[m_menudepth].draw = draw;
		m_layers[m_menudepth].key = key;
	}

	m_drawfunc = draw;
	m_keyfunc = key;

	m_entersound = true;

	refreshCursorLink();
	refreshCursorButtons();

	cls.key_dest = key_menu;
	
	if (m_menudepth > 1)
	{
		int prev_natural, width;
		width = Menu_TrueWidth(*screen);
		m_layers[m_menudepth].start_x = m_layers[m_menudepth-1].end_x;
		m_layers[m_menudepth].end_x = m_layers[m_menudepth].start_x + width;
		prev_natural = Menu_NaturalWidth(*m_layers[m_menudepth-1].screen);
		if (width < viddef.width-prev_natural)
			width = viddef.width-prev_natural;
		global_menu_xoffset_target = width; //setup animation
	}
	else
	{
		int width = Menu_TrueWidth(*screen);
		m_layers[m_menudepth].start_x = 0;
		m_layers[m_menudepth].end_x = m_layers[m_menudepth].start_x + width;
	}
}

void M_ForceMenuOff (void)
{
	refreshCursorLink();

	m_drawfunc = NULL;
	m_keyfunc = NULL;
	cls.key_dest = key_game;
	m_menudepth = 0;
	Key_ClearStates ();
	Cvar_Set ("paused", "0");

	//-JD kill the music when leaving the menu of course
	S_StopAllSounds();
	background_music = Cvar_Get ("background_music", "1", CVAR_ARCHIVE);
	S_StartMapMusic();
	global_menu_xoffset_progress = global_menu_xoffset_target = 0;
}

void M_PopMenu (void)
{
	S_StartLocalSound( menu_out_sound );
	if (m_menudepth < 1)
		Com_Error (ERR_FATAL, "M_PopMenu: depth < 1");
	m_menudepth--;
	
	m_drawfunc = m_layers[m_menudepth].draw;
	m_keyfunc = m_layers[m_menudepth].key;

	refreshCursorLink();
	refreshCursorButtons();

	if (m_menudepth == 0)
	{
		M_ForceMenuOff ();
	}
	else
	{
		global_menu_xoffset_target = m_layers[m_menudepth].offset-global_menu_xoffset-viddef.width;
		if (m_menudepth == 1)
			global_menu_xoffset_target += Menu_NaturalWidth(*m_layers[m_menudepth].screen);
/*		global_menu_xoffset_target = -m_layers[m_menudepth+1].width;*/
/*		if (m_menudepth == 1)*/
/*			global_menu_xoffset_target = m_layers[m_menudepth].natural_width-viddef.width;*/
	}
}


const char *Default_MenuKey (menuframework_s *m, int key)
{
	const char *sound = NULL;
	menucommon_s *item;

	if ( !m )
		return NULL;
	
	if ( ( item = Menu_ItemAtCursor( m ) ) != 0 )
	{
		if ( item->type == MTYPE_FIELD && Field_Key( ( menufield_s * ) item, key ) )
				return NULL;
		if ( item->type == MTYPE_SUBMENU )
		{
			menuframework_s *sm = (menuframework_s *) item;
			if (sm->navagable && Menu_ContainsCursor (*sm))
			{
				return Default_MenuKey (sm, key);
			}
		}
	}

	switch ( key )
	{
	case K_ESCAPE:
		M_PopMenu();
		return menu_out_sound;
	case K_MWHEELUP:
	case K_KP_UPARROW:
	case K_UPARROW:
		m->cursor--;
		refreshCursorLink();
		Menu_AdjustCursor( m, -1 );
		break;
	case K_TAB:
		m->cursor++;
		refreshCursorLink();
		Menu_AdjustCursor( m, 1 );
		break;
	case K_MWHEELDOWN:
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		m->cursor++;
		Menu_AdjustCursor( m, 1 );
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		Menu_SlideItem( m, -1 );
		sound = menu_move_sound;
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		Menu_SlideItem( m, 1 );
		sound = menu_move_sound;
		break;
	case K_KP_ENTER:
	case K_ENTER:
		Menu_SelectItem( m );
		sound = menu_move_sound;
		break;
	}

	return sound;
}

/*
=======================================================================

MAIN MENU

=======================================================================
*/

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
	"m_main_credits",
};
#define MAIN_ITEMS (sizeof(main_names)/sizeof(main_names[0]))

void (*main_open_funcs[MAIN_ITEMS])(void) = 
{
	&M_Menu_PlayerConfig_f,
	&M_Menu_Game_f,
	&M_Menu_JoinServer_f,
	&M_Menu_StartServer_f,
	&M_Menu_IRC_f,
	&M_Menu_Options_f,
	&M_Menu_Video_f,
	&M_Menu_Quit_f,
	&M_Menu_Credits_f
};

void findMenuCoords (int *xoffset, int *ystart, int *totalheight, int *widest)
{
	int w, h, i;
	float scale;

	scale = (float)(viddef.height)/600;

	*totalheight = 0;
	*widest = -1;

	for ( i = 0; i < MAIN_ITEMS; i++ )
	{
		Draw_GetPicSize( &w, &h, main_names[i] );

		if ( w*scale > *widest )
			*widest = w*scale;
		*totalheight += ( h*scale + 24*scale);
	}

	*ystart = ( viddef.height / 2 - 60*scale );
	*xoffset = ( viddef.width - *widest + 250*scale) / 2;
}

void M_Main_Draw (void)
{
	int i;
	int ystart, xstart, xend;
	int	xoffset;
	int widest = -1;
	int totalheight = 0;
	char litname[80];
	float scale, hscale, hscaleoffs;
	float widscale;
	int w, h;
	char montagepicname[16];
	char backgroundpic[16];
	char *version_warning;
	
	static float mainalpha;
	static int montagepic = 1;

	scale = ((float)(viddef.height))/600.0;

	widscale = ((float)(viddef.width))/1024.0;

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);

	ystart = ( viddef.height / 2 - 60*scale );
	xoffset = ( viddef.width - widest - 25*widscale) / 2 + global_menu_xoffset;
	
	// When animating a transition away from the main menu, the background 
	// slides away at double speed, disappearing and leaving just the menu 
	// items themselves. Hence some things use global_menu_xoffset*1.25.
	
	Draw_StretchPic(global_menu_xoffset*1.25, 0, viddef.width, viddef.height, "m_main");

	//draw the montage pics
	mainalpha += cls.frametime; //fade image in
	if(mainalpha > 4) 
	{		
		//switch pics at this point
		mainalpha = 0.1;
		montagepic++;
		if(montagepic > 5) 
		{
			montagepic = 1;
		}
	}
	sprintf(backgroundpic, "m_main_mont%i", (montagepic==1)?5:montagepic-1);
	sprintf(montagepicname, "m_main_mont%i", montagepic);
	Draw_StretchPic (global_menu_xoffset*1.25, 0, viddef.width, viddef.height, backgroundpic);
	Draw_AlphaStretchPic (global_menu_xoffset*1.25, 0, viddef.width, viddef.height, montagepicname, mainalpha);


	/* check for more recent program version */
	version_warning = VersionUpdateNotice();	
	if ( version_warning != NULL )
	{
		Menu_DrawString (global_menu_xoffset, 5*scale, version_warning);
	}
	
	//draw the main menu buttons
	for ( i = 0; i < MAIN_ITEMS; i++ )
	{
		strcpy( litname, main_names[i] );
		if (i == m_main_cursor)
			strcat( litname, "_sel");
		Draw_GetPicSize( &w, &h, litname );
		xstart = xoffset + 100*widscale + (20*i*widscale);
		if (xstart < 0)
			xstart += min(-xstart, (8-i)*20*widscale);
		xend = xstart+w*widscale;
		hscale = 1;
		if (xstart < 0)
		{
			if (xend < 150*widscale)
				xend = min (viddef.width+global_menu_xoffset, 150*widscale);
			xstart = 0;
			if (xend < 50*widscale)
				return;
		}
		hscale = (float)(xend-xstart)/(float)(w*widscale);
		hscaleoffs = (float)h*scale-(float)h*hscale*scale;
		Draw_StretchPic( xstart, (int)(ystart + i * 32.5*scale + 13*scale + hscaleoffs), xend-xstart, h*hscale*scale, litname );
	}
}

void CheckMainMenuMouse (void)
{
	int ystart;
	int	xoffset;
	int widest;
	int totalheight;
	int i, oldhover;
	float scale;
	static int MainMenuMouseHover;

	scale = (float)(viddef.height)/600;

	oldhover = MainMenuMouseHover;
	MainMenuMouseHover = 0;

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);

	i = (cursor.y - ystart - 24*scale)/(32*scale);
	if (i < 0 || i >= MAIN_ITEMS)
	{
		if (cursor.buttonclicks[MOUSEBUTTON1]==1)
		{
			cursor.buttonused[MOUSEBUTTON1] = true;
			cursor.buttonclicks[MOUSEBUTTON1] = 0;
		}
		return;
	}

	if (cursor.mouseaction)
		m_main_cursor = i;

	MainMenuMouseHover = 1 + i;

	if (oldhover == MainMenuMouseHover && MainMenuMouseHover-1 == m_main_cursor &&
		!cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1]==1)
	{
		main_open_funcs[m_main_cursor]();
		S_StartLocalSound( menu_move_sound );
		cursor.buttonused[MOUSEBUTTON1] = true;
		cursor.buttonclicks[MOUSEBUTTON1] = 0;
	}
}

const char *M_Main_Key (menuframework_s *dummy, int key)
{
	switch (key)
	{
	case K_ESCAPE:
		M_PopMenu ();
		break;

	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_KP_UPARROW:
	case K_UPARROW:
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_KP_ENTER:
	case K_ENTER:
		m_entersound = true;
		main_open_funcs[m_main_cursor]();
		break;
	}

	return NULL;
}


void M_Menu_Main_f (void)
{
	float widscale;
	static menuframework_s dummy;
	
	widscale = (float)(viddef.width)/1024.0;
	CHASELINK(dummy.rwidth) = viddef.width;
	dummy.natural_width = 150*widscale;
	
	S_StartMenuMusic();
	global_menu_xoffset_progress = global_menu_xoffset_target = 0;
	M_PushMenu (M_Main_Draw, M_Main_Key, &dummy);
}

/*
=======================================================================

KEYS MENU

=======================================================================
*/
char *bindnames[][2] =
{
{"+attack", 		"attack"},
{"+attack2",		"alt attack"},
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
{"vtaunt 0",			"voice taunt auto"}
};

#define num_bindable_actions (sizeof(bindnames)/sizeof(bindnames[0]))

int				keys_cursor;
static int		bind_grab;

static menuframework_s	s_keys_screen;
static menuframework_s	s_keys_menu;
static menuaction_s		s_keys_actions[num_bindable_actions];

static void M_UnbindCommand (const char *command)
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

static void M_FindKeysForCommand (const char *command, int *twokeys)
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

static void M_KeyBindingDisplayStr (const char *command, size_t bufsize, char *buf)
{
	int keys[2];
	
	M_FindKeysForCommand (command, keys);
	
	if (keys[0] == -1)
	{
		Com_sprintf (buf, bufsize, "???");
	}
	else
	{
		// Key_KeynumToString reuses the same buffer for output sometimes
		int len;
		Com_sprintf (buf, bufsize, "%s", Key_KeynumToString (keys[0]));
		len = strlen(buf);
		if (keys[1] != -1)
			Com_sprintf (buf+len, bufsize-len, "  or  %s", Key_KeynumToString (keys[1]));
	}
}

static menuvec2_t KeySizeFunc (void *_self, FNT_font_t font)
{
	menuvec2_t ret;
	char buf[1024];
	menuaction_s *self = ( menuaction_s * ) _self;
	
	M_KeyBindingDisplayStr (self->generic.localstrings[0], sizeof(buf), buf);
	
	ret.y = font->height;
	ret.x = RCOLUMN_OFFSET + Menu_PredictSize (buf);
	
	return ret;
}

static void DrawKeyBindingFunc( void *_self, FNT_font_t font )
{
	char buf[1024];
	menuaction_s *self = ( menuaction_s * ) _self;

	M_KeyBindingDisplayStr (self->generic.localstrings[0], sizeof(buf), buf);
	
	Menu_DrawString (Item_GetX (*self) + RCOLUMN_OFFSET, Item_GetY (*self), buf);
}

static void KeyBindingFunc( void *_self )
{
	menuaction_s *self = ( menuaction_s * ) _self;
	int keys[2];

	M_FindKeysForCommand( self->generic.localstrings[0], keys );

	if (keys[1] != -1)
		M_UnbindCommand( self->generic.localstrings[0]);

	bind_grab = true;

	Menu_SetStatusBar( &s_keys_menu, "press a key or button for this action" );
}

static void Keys_MenuInit( void )
{
	int i = 0;

	s_keys_screen.nitems = 0;

	setup_window (s_keys_screen, s_keys_menu, "CUSTOMIZE CONTROLS");
	
	for (i = 0; i < num_bindable_actions; i++)
	{
		s_keys_actions[i].generic.type				= MTYPE_ACTION;
		s_keys_actions[i].generic.callback			= KeyBindingFunc;
		s_keys_actions[i].generic.itemdraw			= DrawKeyBindingFunc;
		s_keys_actions[i].generic.localstrings[0]	= bindnames[i][0];
		s_keys_actions[i].generic.name				= bindnames[i][1];
		s_keys_actions[i].generic.itemsizecallback	= KeySizeFunc;
		Menu_AddItem( &s_keys_menu, &s_keys_actions[i]);
	}

	Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
	
	s_keys_menu.maxlines = 30;
	
	Menu_AutoArrange (&s_keys_screen);
}

static void Keys_MenuDraw (void)
{
	Menu_Draw( &s_keys_screen );
}

static const char *Keys_MenuKey (menuframework_s *screen, int key)
{
	menuaction_s *item = ( menuaction_s * ) Menu_ItemAtCursor (&s_keys_menu);

	if ( bind_grab && !(cursor.buttonused[MOUSEBUTTON1]&&key==K_MOUSE1))
	{
		if ( key != K_ESCAPE && key != '`' )
		{
			char cmd[1024];

			Com_sprintf (cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString(key), item->generic.localstrings[0]);
			Cbuf_InsertText (cmd);
		}

		//dont let selecting with mouse buttons screw everything up
		refreshCursorButtons();
		if (key==K_MOUSE1)
			cursor.buttonclicks[MOUSEBUTTON1] = -1;

		Menu_SetStatusBar (&s_keys_menu, "enter to change, backspace to clear");
		bind_grab = false;
		
		Menu_AutoArrange (screen);
		
		return menu_out_sound;
	}

	switch ( key )
	{
	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
	case K_KP_DEL:
		M_UnbindCommand( item->generic.localstrings[0] );
		return menu_out_sound;
	default:
		return Default_MenuKey (screen, key);
	}
}

void M_Menu_Keys_f (void)
{
	Keys_MenuInit();
	M_PushMenu( Keys_MenuDraw, Keys_MenuKey, &s_keys_screen);
}


/*
=======================================================================

OPTIONS MENU

=======================================================================
*/
extern cvar_t *r_minimap;
extern cvar_t *r_minimap_style;

static menuframework_s 	s_options_screen;

static menuframework_s	s_options_menu;

static menuframework_s	s_options_header_submenu;

static menuframework_s	s_options_main_submenu;
static menulist_s		s_options_invertmouse_box;

static void CustomizeControlsFunc( void *unused )
{
	M_Menu_Keys_f();
}

static void MinimapFunc( void *item )
{
	menulist_s *self = (menulist_s *)item;
	Cvar_SetValue("r_minimap", self->curvalue != 0);
	if(r_minimap->integer) {
		Cvar_SetValue("r_minimap_style", self->curvalue % 2);
	}
}

static float ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

#define MAX_FONTS 32
char *font_names[MAX_FONTS];
int	numfonts = 0;

qboolean fontInList (char *check, int num, char **list)
{
	int i;
	for (i=0;i<num;i++)
		if (!Q_strcasecmp(check, list[i]))
			return true;
	return false;
}

// One string after another, both in the same memory block, but each with its
// own null terminator. 
char *str_combine (char *in1, char *in2)
{
	size_t outsize;
	char *out;
	
	outsize = strlen(in1)+1+strlen(in2)+1;
	out = malloc (outsize);
	memset (out, 0, outsize);
	strcpy (out, in1);
	strcpy (strchr(out, '\0')+1, in2);
	
	return out;
}

void insertFile (char ** list, char *insert1, char *insert2, int ndefaults, int len )
{
	int i, j;
	char *tmp;
	
	tmp = str_combine (insert1, insert2);

	for (i=ndefaults;i<len; i++)
	{
		if (!list[i])
			break;

		if (strcmp( list[i], insert1 ))
		{
			for (j=len; j>i ;j--)
				list[j] = list[j-1];

			list[i] = tmp;

			return;
		}
	}

	list[len] = tmp;
}

static void AddFontNames( char * path , int * nfontnames , char ** list )
{
	char ** fontfiles;
	int nfonts = 0;
	int i;

	fontfiles = FS_ListFilesInFS( path , &nfonts, 0,
		SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<nfonts && *nfontnames<MAX_FONTS;i++)
	{
		int num;
		char * p;

		p = strstr(fontfiles[i], "fonts/"); p++;
		p = strstr(p, "/"); p++;

		if (!strstr( p , ".ttf" ) )
			continue;

		num = strlen(p)-4;
		p[num] = 0;

		if (!fontInList(p, i, list))
		{
			insertFile (list, p, p, 1, i);
			(*nfontnames)++;
		}
	}

	if (fontfiles)
		FS_FreeFileList(fontfiles, nfonts);
}

void SetFontNames (char **list)
{
	int nfontnames;

	memset( list, 0, sizeof( char * ) * MAX_FONTS );

	nfontnames = 0;
	AddFontNames( "fonts/*.ttf" , &nfontnames , list );

	numfonts = nfontnames;
}

extern cvar_t *crosshair;
#define MAX_CROSSHAIRS 256
char *crosshair_names[MAX_CROSSHAIRS];
int	numcrosshairs = 0;

void SetCrosshairNames (char **list)
{
	char *curCrosshair, *curCrosshairFile;
	char *p;
	int ncrosshairs = 0, ncrosshairnames;
	char **crosshairfiles;
	int i;

	ncrosshairnames = 4;

	list[0] = str_combine("none", "none"); //the old crosshairs
	list[1] = str_combine("ch1", "ch1");
	list[2] = str_combine("ch2", "ch2");
	list[3] = str_combine("ch3", "ch3");

	crosshairfiles = FS_ListFilesInFS( "pics/crosshairs/*.tga",
		&ncrosshairs, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<ncrosshairs && ncrosshairnames<MAX_CROSSHAIRS;i++)
	{
		int num;

		p = strstr(crosshairfiles[i], "/crosshairs/"); p++;
		curCrosshairFile = p;
		
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
			insertFile (list,curCrosshair,curCrosshairFile,4,ncrosshairnames);
			ncrosshairnames++;
		}
	}

	if (crosshairfiles)
		FS_FreeFileList(crosshairfiles, ncrosshairs);

	numcrosshairs = ncrosshairnames;
}

extern cvar_t *cl_hudimage1;
extern cvar_t *cl_hudimage2;
#define MAX_HUDS 256
char *hud_names[MAX_HUDS];
int	numhuds = 0;

static void HudFunc( void *item )
{
	menulist_s *self;
	char hud1[MAX_OSPATH];
	char hud2[MAX_OSPATH];
	
	self = (menulist_s *)item;

	if(self->curvalue == 0) { //none
		sprintf(hud1, "none");
		sprintf(hud2, "none");
	}

	if(self->curvalue == 1) {
		sprintf(hud1, "pics/i_health.tga");
		sprintf(hud2, "pics/i_score.tga");
	}

	if(self->curvalue > 1) {
		sprintf(hud1, "pics/huds/%s1", hud_names[self->curvalue]);
		sprintf(hud2, "pics/huds/%s2", hud_names[self->curvalue]);
	}

	//set the cvars, both of them
	Cvar_Set( "cl_hudimage1", hud1 );
	Cvar_Set( "cl_hudimage2", hud2 );
}

void SetHudNames (char **list)
{
	char *curHud, *curHudFile;
	char *p;
	int nhuds = 0, nhudnames;
	char **hudfiles;
	int i;

	memset( list, 0, sizeof( char * ) * MAX_HUDS );

	nhudnames = 2;

	list[0] = str_combine("none", "none");
	list[1] = str_combine("default", "pics/i_health.tga"); //the default hud

	hudfiles = FS_ListFilesInFS( "pics/huds/*.tga", &nhuds, 0,
		SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<nhuds && nhudnames<MAX_HUDS;i++)
	{
		int num, file_ext;

		p = strstr(hudfiles[i], "/huds/"); p++;
		p = strstr(p, "/"); p++;

		if (	!strstr(p, ".tga")
			&&	!strstr(p, ".pcx")
			)
			continue;

		num = strlen(p)-5;
		 // we'll just assume the side hud is there if the bottom one is--
		 // we're interested in the bottom one specifically, as that's the
		 // value of the cvar.
		if (p[num] == '2')
			continue;
		file_ext = num+1;
		p[file_ext] = 0;
		
		curHudFile = _strdup (hudfiles[i]);
		
		p[num] = 0;

		curHud = p;

		if (!fontInList(curHud, nhudnames, list))
		{
			insertFile (list,curHud,curHudFile,2,nhudnames);
			nhudnames++;
		}

		free (curHudFile);
	}

	if (hudfiles)
		FS_FreeFileList(hudfiles, nhuds);

	numhuds = nhudnames;
}

static void InvertMouseFunc( void *unused )

{
	if(s_options_invertmouse_box.curvalue && m_pitch->value > 0)
		Cvar_SetValue( "m_pitch", -m_pitch->value );
	else if(m_pitch->value < 0)
		Cvar_SetValue( "m_pitch", -m_pitch->value );
}

static void UpdateBGMusicFunc( void *_self )
{
	menulist_s *self = (menulist_s *)_self;
	Cvar_SetValue( "background_music", self->curvalue );
	if ( background_music->value > 0.99f && background_music_vol->value >= 0.1f )
	{
		S_StartMenuMusic();
	}
}

// name lists: list of strings terminated by 0

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
	"^2green",
	"^4blue",
	"^1red",
	"^3yellow",
	"^6purple",
	0
};

// name and value lists: for spin-controls where the cvar value isn't simply
// the integer index of whatever text is displaying in the control. We use
// NULL terminators to separate display names and variable values, so that way
// we can use the existing menu code.

static const char *doppler_effect_items[] =
{
	"off\0000",
	"normal\0001",
	"high\0003",
	"very high\0005",
	0
};

// slider limits:

typedef struct
{
	float slider_min, slider_max, cvar_min, cvar_max;
} sliderlimit_t;

sliderlimit_t volume_limits = 
{
	0.0f, 50.0f, 0.0, 1.0
};

sliderlimit_t mousespeed_limits = 
{
	0.0f, 110.0f, 0.0f, 11.0f
};

typedef struct {
	enum {
		option_slider,
		option_spincontrol,
		option_textcvarspincontrol,
		option_hudspincontrol,
		option_minimapspincontrol,
	} type;
	const char *cvarname;
	const char *displayname;
	const char *tooltip;
	const void *extradata;
} option_name_t;

option_name_t optionnames[] = 
{
	{
		option_spincontrol,
		"cl_precachecustom",
		"precache custom models",
		"Enabling this can result in slow map loading times",
		onoff_names
	},
	{
		option_spincontrol,
		"cl_paindist",
		"pain distortion fx",
		"GLSL must be enabled for this to take effect",
		onoff_names
	},
	{
		option_spincontrol,
		"cl_explosiondist",
		"explosion distortion fx",
		"GLSL must be enabled for this to take effect",
		onoff_names
	},
	{
		option_spincontrol,
		"cl_raindist",
		"rain droplet fx",
		"GLSL must be enabled for this to take effect",
		onoff_names
	},
	{
		option_spincontrol,
		"cl_showplayernames",
		"identify target",
		NULL, 
		playerid_names
	},
	{
		option_spincontrol,
		"r_ragdolls",
		"ragdolls",
		NULL,
		onoff_names
	},
	{
		option_spincontrol,
		"cl_noblood",
		"no blood",
		NULL,
		onoff_names
	},
	{
		option_spincontrol,
		"cl_noskins",
		"force martian models",
		NULL,
		onoff_names
	},
	{
		option_spincontrol,
		"cl_playertaunts",
		"player taunts",
		NULL,
		onoff_names
	},
	{
		option_slider,
		"s_volume",
		"global volume", 
		NULL,
		&volume_limits
	},
	{
		option_slider,
		"background_music_vol",
		"music volume", 
		NULL,
		&volume_limits
	},
	{
		option_spincontrol,
		"background_music",
		"Background music",
		NULL, 
		onoff_names
	},
	{
		option_textcvarspincontrol,
		"s_doppler",
		"doppler sound effect",
		NULL,
		doppler_effect_items
	},
	{
		option_spincontrol,
		"m_accel", 
		"mouse acceleration",
		NULL,
		onoff_names
	},
	{
		option_slider,
		"sensitivity",
		"mouse speed",
		NULL,
		&mousespeed_limits
	},
	{
		option_slider,
		"menu_sensitivity",
		"menu mouse speed",
		NULL,
		&mousespeed_limits
	},
	{
		option_spincontrol,
		"m_smoothing",
		"mouse smoothing",
		NULL,
		onoff_names
	},
	{
		option_spincontrol,
		"cl_run",
		"always run",
		NULL,
		onoff_names
	},
	// Invert mouse here!
	{
		option_textcvarspincontrol,
		"fnt_console",
		"console font",
		"select the font used to display the console",
		font_names
	},
	{
		option_textcvarspincontrol,
		"fnt_game",
		"game font",
		"select the font used in the game",
		font_names
	},
	{
		option_textcvarspincontrol,
		"fnt_menu",
		"menu font",
		"select the font used in the menu",
		font_names
	},
	{
		option_textcvarspincontrol,
		"crosshair",
		"crosshair",
		"select your crosshair",
		crosshair_names 
	},
	{
		option_hudspincontrol,
		"cl_hudimage1", //multiple cvars controlled-- see HudFunc
		"HUD",
		"select your HUD style",
		hud_names //will be set later
	},
	{
		option_spincontrol,
		"cl_disbeamclr",
		"disruptor color",
		"select disruptor beam color",
		color_names
	},
	{
		option_minimapspincontrol,
		NULL, //multiple cvars controlled-- see MinimapFunc
		"Minimap",
		"select your minimap style",
		minimap_names
	},
	{
		option_spincontrol,
		"in_joystick",
		"use joystick",
		NULL,
		onoff_names
	},
	{
		option_spincontrol,
		"cl_drawfps",
		"display framerate",
		NULL,
		onoff_names
	},
	{
		option_spincontrol,
		"cl_drawtimer",
		"display timer",
		NULL,
		onoff_names
	},
	{
		option_spincontrol,
		"cl_simpleitems",
		"simple items",
		"Draw floating icons instead of 3D models for ingame items",
		onoff_names
	},
};

#define num_options (sizeof(optionnames)/sizeof(optionnames[0]))

menuitem_s crosshair_pic_thumbnail;

menumultival_s options[num_options];

static void SpinOptionFunc (void *_self)
{
	menulist_s *self;
	const char *cvarname;
	
	self = (menulist_s *)_self;
	cvarname = self->generic.localstrings[0];
	
	Cvar_SetValue( cvarname, self->curvalue );
}

static void TextVarSpinOptionFunc (void *_self)
{
	menulist_s *self;
	const char *cvarname;
	char *cvarval;
	
	self = (menulist_s *)_self;
	cvarname = self->generic.localstrings[0];
	
	cvarval = strchr(self->itemnames[self->curvalue], '\0')+1;
	Cvar_Set( cvarname, cvarval);
}

static void UpdateDopplerEffectFunc( void *self )
{
	TextVarSpinOptionFunc (self);
	R_EndFrame(); // buffer swap needed to show text box
	S_UpdateDopplerFactor();

}

static void SliderOptionFunc (void *_self)
{
	menuslider_s *self;
	const char *cvarname;
	float cvarval, sliderval, valscale;
	const sliderlimit_t *limit;
	
	self = (menulist_s *)_self;
	cvarname = self->generic.localstrings[0];
	
	limit = (const sliderlimit_t *) self->generic.localptrs[0];
	
	sliderval = self->curvalue;
	
	valscale = 	(limit->cvar_max-limit->cvar_min)/
				(limit->slider_max-limit->slider_min);
	cvarval = limit->cvar_min + valscale*(sliderval-limit->slider_min);
	
	Cvar_SetValue( cvarname, cvarval );
}

void Option_SetMenuItemValue (menumultival_s *item, option_name_t *optionname)
{
	int val, maxval;
	char *vartextval;
	int i;
	float cvarval, sliderval, valscale;
	const sliderlimit_t *limit;
	
	// special types
	switch (optionname->type)
	{
		case option_spincontrol:
			for (maxval = 0; item->itemnames[maxval]; maxval++) 
				continue;
			maxval--;
		
			val = ClampCvar (0, maxval, Cvar_VariableValue (optionname->cvarname));
		
			item->curvalue = val;
			Cvar_SetValue (optionname->cvarname, val);
			return;
		
		case option_hudspincontrol:
		case option_textcvarspincontrol:
			item->curvalue = 0;
			vartextval = Cvar_VariableString (optionname->cvarname);
			
			for (i=0; item->itemnames[i]; i++)
			{
				char *corresponding_cvar_val = strchr(item->itemnames[i], '\0')+1;
				if (!Q_strcasecmp(vartextval, corresponding_cvar_val))
				{
					item->curvalue = i;
					break;
				}
			}
			return;
		
		case option_minimapspincontrol:
			Cvar_SetValue("r_minimap_style", ClampCvar(0, 1, r_minimap_style->value));
			Cvar_SetValue("r_minimap", ClampCvar(0, 1, r_minimap->value));
			if(r_minimap_style->value == 0)
				item->curvalue = 2;
			else
				item->curvalue = r_minimap->value;
			return;
		
		case option_slider:
			limit = (const sliderlimit_t *) optionname->extradata;
		
			cvarval = ClampCvar (	limit->cvar_min, limit->cvar_max,
									Cvar_VariableValue (optionname->cvarname));
			Cvar_SetValue (optionname->cvarname, cvarval);
		
			valscale = 	(limit->slider_max-limit->slider_min)/
						(limit->cvar_max-limit->cvar_min);
			sliderval = limit->slider_min + valscale*(cvarval-limit->cvar_min);
			item->curvalue = sliderval;
	}
}

static void ControlsSetMenuItemValues( void )
{
	int i;
	
	for (i = 0; i < num_options; i++)
	{
		Option_SetMenuItemValue (&options[i], &optionnames[i]);
	}
	
	s_options_invertmouse_box.curvalue		= m_pitch->value < 0;
}

static void ControlsResetDefaultsFunc( void *unused )
{
	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_Execute();

	ControlsSetMenuItemValues();

	CL_Snd_Restart_f();
	S_StartMenuMusic();

}

void Options_MenuInit( void )
{
	int i;
	const sliderlimit_t *cur_slider_limit;

	s_options_screen.nitems = 0;

	setup_window (s_options_screen, s_options_menu, "OPTIONS");
	
	s_options_header_submenu.generic.type = MTYPE_SUBMENU;
	s_options_header_submenu.navagable = true;
	s_options_header_submenu.nitems = 0;
	s_options_header_submenu.horizontal = true;

	add_action(s_options_header_submenu, "customize controls", CustomizeControlsFunc);
	add_action(s_options_header_submenu, "reset defaults", ControlsResetDefaultsFunc);
	
	Menu_AddItem (&s_options_menu, &s_options_header_submenu);
	
	setup_window (s_options_menu, s_options_main_submenu, NULL);
	s_options_main_submenu.bordertexture = "menu/sm_";
	s_options_main_submenu.generic.flags = QMF_SNUG_LEFT;
	s_options_main_submenu.maxlines = 25;
	
	crosshair_pic_thumbnail.generic.type = MTYPE_NOT_INTERACTIVE;
	VectorSet (crosshair_pic_thumbnail.generic.localints, 5, 5, RCOLUMN_OFFSET);
	crosshair_pic_thumbnail.generic.itemsizecallback = PicSizeFunc;
	crosshair_pic_thumbnail.generic.itemdraw = CrosshairPicDrawFunc;
	
	// Do not re-allocate font/crosshair/HUD names each time the menu is
	// displayed - BlackIce
	if ( numfonts == 0 )
		SetFontNames (font_names);
	
	if ( numhuds == 0 )
		SetHudNames (hud_names);
	
	if ( numcrosshairs == 0 )
		SetCrosshairNames (crosshair_names);
	
	for (i = 0; i < num_options; i++)
	{
		options[i].generic.name = optionnames[i].displayname;
		options[i].generic.tooltip = optionnames[i].tooltip;
		options[i].generic.localstrings[0] = optionnames[i].cvarname;
		
		switch (optionnames[i].type)
		{
			case option_spincontrol:
				options[i].generic.type = MTYPE_SPINCONTROL;
				options[i].itemnames = (const char **) optionnames[i].extradata;
				if (!strcmp (optionnames[i].cvarname, "background_music"))
					// FIXME HACK
					options[i].generic.callback = UpdateBGMusicFunc;
				else
					options[i].generic.callback = SpinOptionFunc;
				if (options[i].itemnames == onoff_names)
					setup_tickbox (options[i]);
				break;
			case option_textcvarspincontrol:
				if (optionnames[i].extradata == crosshair_names)
					// found the crosshair, put the preview just above it.
					Menu_AddItem (&s_options_main_submenu, &crosshair_pic_thumbnail);
				options[i].generic.type = MTYPE_SPINCONTROL;
				options[i].itemnames = (const char **) optionnames[i].extradata;
				if (options[i].itemnames == doppler_effect_items)
					// FIXME HACK
					options[i].generic.callback = UpdateDopplerEffectFunc;
				else
					options[i].generic.callback = TextVarSpinOptionFunc;
				break;
			case option_hudspincontrol:
				optionnames[i].extradata = hud_names;
				options[i].generic.type = MTYPE_SPINCONTROL;
				options[i].itemnames = (const char **) optionnames[i].extradata;
				options[i].generic.callback = HudFunc;
				break;
			case option_minimapspincontrol:
				options[i].generic.type = MTYPE_SPINCONTROL;
				options[i].itemnames = (const char **) optionnames[i].extradata;
				options[i].generic.callback = MinimapFunc;
				break;
			case option_slider:
				cur_slider_limit = (const sliderlimit_t *) optionnames[i].extradata;
				options[i].generic.type = MTYPE_SLIDER;
				options[i].minvalue = cur_slider_limit->slider_min;
				options[i].maxvalue = cur_slider_limit->slider_max;
				options[i].generic.callback = SliderOptionFunc;
				options[i].generic.localptrs[0] = cur_slider_limit;
				break;
		}
		
		Option_SetMenuItemValue (&options[i], &optionnames[i]);
		Menu_AddItem( &s_options_main_submenu, &options[i]);
	}

	s_options_invertmouse_box.generic.name	= "invert mouse";
	s_options_invertmouse_box.generic.callback = InvertMouseFunc;
	s_options_invertmouse_box.curvalue		= m_pitch->value < 0;
	setup_tickbox (s_options_invertmouse_box);

	Menu_AddItem( &s_options_main_submenu, &s_options_invertmouse_box );
	
	Menu_AutoArrange (&s_options_screen);
}

screen_boilerplate (Options, s_options_screen)

/*
=======================================================================

VIDEO MENU

=======================================================================
*/

// TODO: just bring the actual menu portions of vid_menu.c into this file.
extern menuframework_s s_opengl_screen;
extern void VID_MenuInit( void );
extern void VID_MenuDraw( void );
void M_Menu_Video_f (void)
{
	VID_MenuInit();
	M_PushMenu( VID_MenuDraw, Default_MenuKey, &s_opengl_screen);
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
	"Max Eliaser",
	"Charles Hudson",
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
	"Franc Cassar",
	"Shawn Keeth",
	"Enki",
	"",
	"+FONTS",
	"John Diamond",
	"The League of Moveable Type",
	"Brian Kent",
	"",
	"+LOGO",
	"Adam -servercleaner- Szalai",
	"Paul -GimpAlien-",
	"",
	"+LEVEL DESIGN",
	"John Diamond",
	"Dennis -xEMPx- Zedlach",
	"Charles Hudson",
	"Torben Fahrnbach",
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
	"Alien Arena (C)2007-2012 COR Entertainment, LLC",
	"All Rights Reserved.",
	0
};

void M_Credits_MenuDraw( void )
{
	int i, y, scale;
	FNT_font_t		font;
	struct FNT_window_s	box;
	
	font = FNT_AutoGet( CL_menuFont );
	scale = font->size / 8.0;
	
	/*
	** draw the credits
	*/
	for ( i = 0, y = viddef.height - ( ( cls.realtime - credits_start_time ) / 40.0F ); credits[i]; y += 12*scale, i++ )
	{
		if ( y <= -12*scale )
			continue;
		
		box.y = y;
		box.x = global_menu_xoffset;
		box.height = 0;
		box.width = viddef.width;

		if ( credits[i][0] == '+' )
		{
			FNT_BoundedPrint (font, credits[i]+1, FNT_CMODE_NONE, FNT_ALIGN_CENTER, &box, FNT_colors[3]);
		}
		else
		{
			FNT_BoundedPrint (font, credits[i], FNT_CMODE_NONE, FNT_ALIGN_CENTER, &box, FNT_colors[7]);
		}
	}

	if ( y < 0 )
		credits_start_time = cls.realtime;
}

const char *M_Credits_Key (menuframework_s *dummy, int key)
{
	if (key == K_ESCAPE)
	{
		if (creditsBuffer)
			FS_FreeFile (creditsBuffer);
		M_PopMenu ();
	}

	return menu_out_sound;

}

void M_Menu_Credits_f( void )
{
	static menuframework_s dummy;
	
	CHASELINK(dummy.rwidth) = viddef.width;
	
	creditsBuffer = NULL;
	credits = idcredits;
	credits_start_time = cls.realtime;

	M_PushMenu( M_Credits_MenuDraw, M_Credits_Key, &dummy);
}

/*
=============================================================================

GAME MENU

=============================================================================
*/

static menuframework_s	s_game_screen;
static menuframework_s	s_game_menu;

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

static void SinglePlayerGameFunc (void *data)
{
	char skill[2];
	skill[1] = '\0';
	skill[0] = ((menuaction_s*)data)->generic.localints[0]+'0';
	Cvar_ForceSet ("skill", skill);
	StartGame ();
}

static const char *singleplayer_skill_level_names[][2] = {
	{"easy",	"You will win"},
	{"medium",	"You might win"},
	{"hard",	"Very challenging"},
	{"ultra",	"Only the best will win"}
};
#define num_singleplayer_skill_levels (sizeof(singleplayer_skill_level_names)/sizeof(singleplayer_skill_level_names[0]))
static menuaction_s		s_singleplayer_game_actions[num_singleplayer_skill_levels];

void Game_MenuInit( void )
{
	int i;
	
	s_game_screen.nitems = 0;
	
	setup_window (s_game_screen, s_game_menu, "SINGLE PLAYER");
	
	s_game_menu.statusbar = "Progress levels against bots";
	
	add_text(s_game_menu, "Instant Action!", 0);
	
	for (i = 0; i < num_singleplayer_skill_levels; i++)
	{
		s_singleplayer_game_actions[i].generic.type = MTYPE_ACTION;
		s_singleplayer_game_actions[i].generic.name = singleplayer_skill_level_names[i][0];
		s_singleplayer_game_actions[i].generic.tooltip = singleplayer_skill_level_names[i][1];
		s_singleplayer_game_actions[i].generic.localints[0] = i;
		s_singleplayer_game_actions[i].generic.callback = SinglePlayerGameFunc;
		Menu_AddItem (&s_game_menu, &s_singleplayer_game_actions[i]);
	}

	Menu_AutoArrange (&s_game_screen);
	Menu_Center (&s_game_screen);
}

screen_boilerplate (Game, s_game_screen)

/*
=============================================================================

IRC MENUS

=============================================================================
*/

char			IRC_key[64];

static menuframework_s	s_irc_screen;

static menuframework_s	s_irc_menu;
static menuaction_s		s_irc_join;
static menulist_s		s_irc_joinatstartup;

static menufield_s		s_irc_server;
static menufield_s		s_irc_channel;
static menufield_s		s_irc_port;
static menulist_s		s_irc_ovnickname;
static menufield_s		s_irc_nickname;
static menufield_s		s_irc_kickrejoin;
static menufield_s		s_irc_reconnectdelay;

static void JoinIRCFunc( void *unused )
{
	if(pNameUnique)
		CL_InitIRC();
}

static void QuitIRCFunc( void *unused )
{
	CL_IRCInitiateShutdown();
}

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

static void IRC_Settings_SubMenuInit( )
{
	setup_tickbox (s_irc_joinatstartup);
	s_irc_joinatstartup.generic.name	= "Autojoin At Startup";
	s_irc_joinatstartup.generic.localstrings[0] = "cl_IRC_connect_at_startup";
	s_irc_joinatstartup.curvalue = cl_IRC_connect_at_startup->integer != 0;
	s_irc_joinatstartup.generic.callback = SpinOptionFunc;
	Menu_AddItem( &s_irc_menu, &s_irc_joinatstartup );

	s_irc_server.generic.type		= MTYPE_FIELD;
	s_irc_server.generic.name		= "Server ";
	s_irc_server.generic.tooltip	= "Address or name of the IRC server";
	s_irc_server.generic.visible_length		= 16;
	s_irc_server.cursor			= strlen( cl_IRC_server->string );
	strcpy( s_irc_server.buffer, Cvar_VariableString("cl_IRC_server") );
	Menu_AddItem( &s_irc_menu, &s_irc_server );

	s_irc_channel.generic.type		= MTYPE_FIELD;
	s_irc_channel.generic.name		= "Channel ";
	s_irc_channel.generic.tooltip	= "Name of the channel to join";
	s_irc_channel.generic.visible_length		= 16;
	s_irc_channel.cursor			= strlen( cl_IRC_channel->string );
	strcpy( s_irc_channel.buffer, Cvar_VariableString("cl_IRC_channel") );
	Menu_AddItem( &s_irc_menu, &s_irc_channel );

	s_irc_port.generic.type			= MTYPE_FIELD;
	s_irc_port.generic.name			= "TCP Port ";
	s_irc_port.generic.tooltip		= "Port to connect to on the server";
	s_irc_port.generic.visible_length		= 6;
	s_irc_port.cursor			= strlen( cl_IRC_port->string );
	strcpy( s_irc_port.buffer, Cvar_VariableString("cl_IRC_port") );
	Menu_AddItem( &s_irc_menu, &s_irc_port );

	s_irc_ovnickname.generic.name		= "Override nick";
	s_irc_ovnickname.generic.tooltip	= "Enable this to override the default, player-based nick";
	setup_tickbox (s_irc_ovnickname);
	s_irc_ovnickname.curvalue		= cl_IRC_override_nickname->value ? 1 : 0;
	Menu_AddItem( &s_irc_menu, &s_irc_ovnickname );

	s_irc_nickname.generic.type		= MTYPE_FIELD;
	s_irc_nickname.generic.name		= "Nick ";
	s_irc_nickname.generic.tooltip	= "Nickname override to use";
	s_irc_nickname.generic.visible_length		= 16;
	s_irc_nickname.cursor			= strlen( cl_IRC_nickname->string );
	strcpy( s_irc_nickname.buffer, Cvar_VariableString("cl_IRC_nickname") );
	Menu_AddItem( &s_irc_menu, &s_irc_nickname );

	s_irc_kickrejoin.generic.type		= MTYPE_FIELD;
	s_irc_kickrejoin.generic.name		= "Autorejoin ";
	s_irc_kickrejoin.generic.tooltip	= "Delay before automatic rejoin after kick (0 to disable)";
	s_irc_kickrejoin.generic.visible_length		= 4;
	s_irc_kickrejoin.cursor			= strlen( cl_IRC_kick_rejoin->string );
	strcpy( s_irc_kickrejoin.buffer, Cvar_VariableString("cl_IRC_kick_rejoin") );
	Menu_AddItem( &s_irc_menu, &s_irc_kickrejoin );

	s_irc_reconnectdelay.generic.type	= MTYPE_FIELD;
	s_irc_reconnectdelay.generic.name	= "Reconnect ";
	s_irc_reconnectdelay.generic.tooltip= "Delay between reconnection attempts (minimum 5)";
	s_irc_reconnectdelay.generic.visible_length	= 4;
	s_irc_reconnectdelay.cursor		= strlen( cl_IRC_reconnect_delay->string );
	strcpy( s_irc_reconnectdelay.buffer, Cvar_VariableString("cl_IRC_reconnect_delay") );
	Menu_AddItem( &s_irc_menu, &s_irc_reconnectdelay );

	add_action (s_irc_menu, "Apply settings", ApplyIRCSettings);

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
	Com_sprintf(IRC_key, sizeof(IRC_key), "(IRC Chat Key is %s)", Key_KeynumToString(twokeys[0]));
}

void IRC_MenuInit( void )
{
	extern cvar_t *name;

	if(!strcmp(name->string, "Player"))
		pNameUnique = false;
	else
		pNameUnique = true;

	if(!cl_IRC_connect_at_startup)
		cl_IRC_connect_at_startup = Cvar_Get("cl_IRC_connect_at_startup", "0", CVAR_ARCHIVE);

	M_FindIRCKey();
	
	s_irc_screen.nitems = 0;

	setup_window (s_irc_screen, s_irc_menu, "IRC CHAT OPTIONS");

	s_irc_join.generic.type	= MTYPE_ACTION;
	s_irc_join.generic.name	= "Join IRC Chat";
	s_irc_join.generic.callback = JoinIRCFunc;
	Menu_AddItem( &s_irc_menu, &s_irc_join );

	IRC_Settings_SubMenuInit ();

	add_text (s_irc_menu, IRC_key, 0);

	Menu_AutoArrange (&s_irc_screen);
}


void IRC_MenuDraw( void )
{
	//warn user that they cannot join until changing default player name
	if(!pNameUnique)
		s_irc_menu.statusbar = "You must create your player name before joining a server!";
	else if(CL_IRCIsConnected())
		s_irc_menu.statusbar = "Connected to IRC server.";
	else if(CL_IRCIsRunning())
		s_irc_menu.statusbar = "Connecting to IRC server...";
	else
		s_irc_menu.statusbar = "Not connected to IRC server.";

	// Update join/quit menu entry
	if ( CL_IRCIsRunning( ) ) {
		s_irc_join.generic.name	= "Quit IRC Chat";
		s_irc_join.generic.callback = QuitIRCFunc;
	} else {
		s_irc_join.generic.name	= "Join IRC Chat";
		s_irc_join.generic.callback = JoinIRCFunc;
	}

	Menu_Draw( &s_irc_screen );
}

void M_Menu_IRC_f (void)
{
	IRC_MenuInit();
	M_PushMenu( IRC_MenuDraw, Default_MenuKey, &s_irc_screen);
}



/*
=============================================================================

JOIN SERVER MENU

=============================================================================
*/
#define MAX_LOCAL_SERVERS 128
#define MAX_SERVER_MODS 16
#define SERVER_LIST_COLUMNS 4

static const char *updown_names[] = {
	"menu/midarrow",
	"menu/dnarrow",
	"menu/uparrow",
	0
};

int		m_num_servers = 0;

static char local_player_info[MAX_PLAYERS][SVDATA_PLAYERINFO][SVDATA_PLAYERINFO_COLSIZE];
static char *local_player_info_ptrs[MAX_PLAYERS*SVDATA_PLAYERINFO];
static char local_server_data[6][64];
static char local_mods_data[16][53]; //53 is measured max tooltip width

//Lists for all stock mutators and game modes, plus some of the more popular
//custom ones. (NOTE: For non-boolean cvars, i.e. those which have values
//other than 0 or 1, you get a string of the form cvar=value. In the future,
//we may do something special to parse these, but since no such cvars are
//actually recognized right now anyway, we currently don't.)

//TODO: Have a menu to explore this list?

//Names. If a cvar isn't recognized, the name of the cvar itself is used.
static char mod_names[] =
	//cannot be wider than this boundary:	|
	"\\ctf"             "\\capture the flag"
	"\\tca"             "\\team core assault"
	"\\cp"              "\\cattle prod"
	"\\instagib"        "\\instagib"
	"\\rocket_arena"    "\\rocket arena"
	"\\low_grav"        "\\low gravity"
	"\\regeneration"    "\\regeneration"
	"\\vampire"         "\\vampire"
	"\\excessive"       "\\excessive"
	"\\grapple"         "\\grappling hook"
	"\\classbased"      "\\class based"
	"\\g_duel"          "\\duel mode"
	"\\quickweap"       "\\quick switch"
	"\\anticamp"        "\\anticamp"
	"\\sv_joustmode"    "\\joust mode"
	"\\playerspeed"     "\\player speed"
	"\\insta_rockets"   "\\insta/rockets"
	"\\chaingun_arena"  "\\chaingun arena"
	"\\instavap"        "\\vaporizer arena"
	"\\vape_arena"      "\\vaporizer arena"
	"\\testcode"        "\\code testing"
	"\\testmap"         "\\map testing"
	"\\dodgedelay=0"    "\\rapid dodging"
	"\\g_tactical"      "\\aa tactical"
	"\\";

//Descriptions. If a cvar isn't recognized, "(no description)" is used.
static char mods_desc[] =
	//cannot be wider than this boundary:									|
	"\\ctf"             "\\capture the enemy team's flag to earn points"
	"\\tca"             "\\destroy the enemy team's spider node to win"
	"\\cp"              "\\herd cows through your team's goal for points"
	"\\instagib"        "\\disruptor only, instant kill, infinite ammo"
	"\\rocket_arena"    "\\rocket launcher only, infinite ammo"
	"\\low_grav"        "\\reduced gravity"
	"\\regeneration"    "\\regain health over time"
	"\\vampire"         "\\regain health by damaging people"
	"\\excessive"       "\\all weapons enhanced, infinite ammo"
	"\\grapple"         "\\spawn with a grappling hook"
	"\\classbased"      "\\different races have different strengths"
	"\\g_duel"          "\\wait in line for your turn to duel"
	"\\quickweap"       "\\switch weapons instantly"
	"\\anticamp"        "\\you are punished for holding still too long"
	"\\sv_joustmode"    "\\you can still jump while in midair"
	"\\playerspeed"     "\\run much faster than normal"
	"\\insta_rockets"   "\\hybrid of instagib and rocket_arena"
	"\\chaingun_arena"  "\\chaingun only, infinite ammo"
	"\\instavap"        "\\vaporizer only, infinite ammo"
	"\\vape_arena"      "\\vaporizer only, infinite ammo"
	"\\testcode"        "\\server is testing experimental code"
	"\\testmap"         "\\server is testing an unfinished map"
	"\\dodgedelay=0"    "\\no minimum time between dodges"
	"\\g_tactical"      "\\humans vs martians, destroy enemy bases"
	"\\";

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
		} 
		line[num] = (*contents)[i];
		num++;
	}

	ret = (char *)malloc (sizeof(line));
	strcpy (ret, line);
	return ret;
}

SERVERDATA mservers[MAX_LOCAL_SERVERS];

PLAYERSTATS thisPlayer;

//TODO: Move this out of the menu section!
void M_ParseServerInfo (netadr_t adr, char *status_string, SERVERDATA *destserver)
{
	char *rLine;
	char *token;
	char skillLevel[24];
	char lasttoken[256];
	char seps[]   = "\\";
	int players = 0;
	int bots = 0;
	int result;
	char playername[PLAYERNAME_SIZE];
	int score, ping, rankTotal, starttime;
	PLAYERSTATS	player;

	destserver->local_server_netadr = adr;
	// starttime now sourced per server.
	starttime = CL_GetPingStartTime(adr);
	if (starttime != 0)
		destserver->ping = Sys_Milliseconds() - starttime;
	else
	{
		// Local LAN?
		destserver->ping = 1;
	}
	if ( destserver->ping < 1 )
		destserver->ping = 1; /* for LAN and address book entries */

	//parse it

	result = strlen(status_string);

	//server info
	rLine = GetLine (&status_string, &result);

	/* Establish string and get the first token: */
	token = strtok( rLine, seps );
	if ( token != NULL )
	{
		Com_sprintf(lasttoken, sizeof(lasttoken), "%s", token);
		token = strtok( NULL, seps );
	}
	
	// HACK for backward compatibility
	memset (destserver->modInfo, 0, sizeof(destserver->modInfo));
	
	/* Loop through the rest of them */
	while( token != NULL ) 
	{
		/* While there are tokens in "string" */
		if (!Q_strcasecmp (lasttoken, "admin"))
			Com_sprintf(destserver->szAdmin, sizeof(destserver->szAdmin), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "website"))
			Com_sprintf(destserver->szWebsite, sizeof(destserver->szWebsite), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "fraglimit"))
			Com_sprintf(destserver->fraglimit, sizeof(destserver->fraglimit), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "timelimit"))
			Com_sprintf(destserver->timelimit, sizeof(destserver->timelimit), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "version"))
			Com_sprintf(destserver->szVersion, sizeof(destserver->szVersion), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "mapname"))
			Com_sprintf(destserver->szMapName, sizeof(destserver->szMapName), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "hostname"))
			Com_sprintf(destserver->szHostName, sizeof(destserver->szHostName), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "maxclients"))
			Com_sprintf(destserver->maxClients, sizeof(destserver->maxClients), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "mods"))
			Com_sprintf(destserver->modInfo, sizeof(destserver->modInfo), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "sv_joustmode"))
			destserver->joust = atoi(token);

		/* Get next token: */
		Com_sprintf(lasttoken, sizeof(lasttoken), "%s", token);
		token = strtok( NULL, seps );
	}
	
	free (rLine);

	//playerinfo
	rankTotal = 0;
	strcpy (seps, " ");
	while ((rLine = GetLine (&status_string, &result)) && players < MAX_PLAYERS) 
	{
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

		Com_sprintf	(	destserver->playerInfo[players][SVDATA_PLAYERINFO_NAME],
						SVDATA_PLAYERINFO_COLSIZE,
						"%s", playername
					);
		Com_sprintf	(	destserver->playerInfo[players][SVDATA_PLAYERINFO_SCORE],
						SVDATA_PLAYERINFO_COLSIZE,
						"%i", score
					);
		Com_sprintf	(	destserver->playerInfo[players][SVDATA_PLAYERINFO_PING],
						SVDATA_PLAYERINFO_COLSIZE,
						"%i", ping
					);
		destserver->playerRankings[players] = player.ranking;

		rankTotal += player.ranking;

		players++;

		if(ping == 0)
			bots++;
	}

	if(players) 
	{
		if(thisPlayer.ranking < (rankTotal/players) - 100)
			strcpy(skillLevel, "Your Skill is ^1Higher");
		else if(thisPlayer.ranking > (rankTotal/players + 100))
			strcpy(skillLevel, "Your Skill is ^4Lower");
		else
			strcpy(skillLevel, "Your Skill is ^3Even");

		Com_sprintf(destserver->skill, sizeof(destserver->skill), "%s", skillLevel);
	}
	else
		Com_sprintf(destserver->skill, sizeof(destserver->skill), "Unknown");

	destserver->players = players;

	//build the string for the server (hostname - address - mapname - players/maxClients)
	if(strlen(destserver->maxClients) > 2)
		strcpy(destserver->maxClients, "??");
	
	Com_sprintf (destserver->szPlayers, sizeof(destserver->szPlayers), "%i(%i)/%s", min(99,players), min(99,bots), destserver->maxClients);
	Com_sprintf (destserver->szPing, sizeof(destserver->szPing), "%i", min(9999,destserver->ping));
}

static menulist_s		s_joinserver_filterempty_action;

void M_AddToServerList (netadr_t adr, char *status_string)
{
	//if by some chance this gets called without the menu being up, return
	if(cls.key_dest != key_menu)
		return;

	if (m_num_servers == MAX_LOCAL_SERVERS)
		return;
	
	M_ParseServerInfo (adr, status_string, &mservers[m_num_servers]);
	
	CON_Clear();
	
	if(s_joinserver_filterempty_action.curvalue)
	{
		// show empty off; filter empty servers
		if(mservers[m_num_servers].players == 0)
			// don't increment m_num_servers; this one will be overwritten
			return; 
	}
	
	m_num_servers++;
}

void M_UpdateConnectedServerInfo (netadr_t adr, char *status_string)
{
	M_ParseServerInfo (adr, status_string, &connectedserver);
	remoteserver_jousting = connectedserver.joust;
}


static menuframework_s	s_serverbrowser_screen;

static menuframework_s	s_joinserver_menu;

static menuframework_s	s_joinserver_header;

static menuframework_s	s_serverlist_submenu;
static menuframework_s	s_serverlist_header;
static menulist_s		s_serverlist_header_columns[SERVER_LIST_COLUMNS];
static menuframework_s	s_serverlist_rows[MAX_LOCAL_SERVERS];
static menutxt_s		s_serverlist_columns[MAX_LOCAL_SERVERS][SERVER_LIST_COLUMNS];

static menuframework_s	s_selectedserver_screen;

static menuframework_s	s_playerlist_submenu;
static menuframework_s	s_playerlist_scrollingmenu;
static menuframework_s	s_playerlist_header;
static menutxt_s		s_playerlist_header_columns[SVDATA_PLAYERINFO];
static menuframework_s	s_joinserver_player_rows[MAX_PLAYERS];
static menutxt_s		s_joinserver_player_columns[MAX_PLAYERS][SVDATA_PLAYERINFO];
char 					ranktxt[MAX_PLAYERS][32];

static menuframework_s	s_modlist_submenu;
static menuaction_s		s_joinserver_mods_data[MAX_SERVER_MODS];
char					modtxt[MAX_SERVER_MODS][48];
char					modnames[MAX_SERVER_MODS][24];

static menuframework_s	s_serverinfo_submenu;
static menuframework_s	s_joinserver_server_data_rows[6];
static menutxt_s		s_joinserver_server_data_columns[6][2];

static int serverindex;
qboolean serveroutdated;

void DeselectServer (void)
{
	serverindex = -1;
	s_serverlist_submenu.force_highlight = false;
	s_serverinfo_submenu.nitems = 0;
	s_playerlist_scrollingmenu.nitems = 0;
	s_modlist_submenu.nitems = 0;
}

void SelectServer ( void *self )
{
	int		i;
	char	modstring[64];
	char	*token;
	int 	index = ( menuframework_s * ) self - s_serverlist_rows;
	
	//set strings for output
	Com_sprintf(local_server_data[0], sizeof(local_server_data[0]), mservers[index].skill);
	Com_sprintf(local_server_data[1], sizeof(local_server_data[1]), mservers[index].szAdmin);
	Com_sprintf(local_server_data[2], sizeof(local_server_data[2]), mservers[index].szWebsite);
	Com_sprintf(local_server_data[3], sizeof(local_server_data[3]), mservers[index].fraglimit);
	Com_sprintf(local_server_data[4], sizeof(local_server_data[4]), mservers[index].timelimit);

	if ((serveroutdated = serverIsOutdated (mservers[index].szVersion)))
		Com_sprintf(local_server_data[5], sizeof(local_server_data[5]), "^1%s", mservers[index].szVersion);
	else
		Com_sprintf(local_server_data[5], sizeof(local_server_data[5]), mservers[index].szVersion);
	s_serverinfo_submenu.nitems = 6;
	s_serverinfo_submenu.yscroll = 0;
	Menu_AutoArrange (&s_serverinfo_submenu);

	//Copy modstring over since strtok will modify it
	Q_strncpyz(modstring, mservers[index].modInfo, sizeof(modstring));
	token = strtok(modstring, "%%");
	for (i=0; i<MAX_SERVER_MODS; i++) {
		if (!token)
			break;
		Com_sprintf(local_mods_data[i], sizeof(local_mods_data[i]), token);
		token = strtok(NULL, "%%");
		
		Com_sprintf (   modtxt[i], sizeof(modtxt[i]),
						Info_ValueForKey(mods_desc, local_mods_data[i])
					);
		if (!strlen(modtxt[i]))
			Com_sprintf (modtxt[i], sizeof(modtxt[i]), "(no description)");
		
		Com_sprintf (   modnames[i], sizeof(modnames[i]),
						Info_ValueForKey(mod_names, local_mods_data[i])
					);
		if (!strlen(modnames[i]))
			Com_sprintf (modnames[i], sizeof(modnames[i]), local_mods_data[i]);
	}
	s_modlist_submenu.nitems = i;
	s_modlist_submenu.yscroll = 0;
	Menu_AutoArrange (&s_modlist_submenu);

	//players
	for(i=0; i<mservers[index].players; i++) {
		int ranking = mservers[index].playerRankings[i];
		if (ranking == 1000)
			Com_sprintf(ranktxt[i], sizeof(ranktxt[i]), "Player is unranked");
		else
			Com_sprintf(ranktxt[i], sizeof(ranktxt[i]), "Player is ranked %i", ranking);
	}
	memcpy (local_player_info, mservers[index].playerInfo, sizeof(mservers[index].playerInfo));
	s_playerlist_scrollingmenu.nitems = mservers[index].players;
	s_playerlist_scrollingmenu.yscroll = 0;
	Menu_AutoArrange (&s_playerlist_submenu);
	
	// used if the player hits enter without his mouse over the server list	
	serverindex = index;
}

//join on double click, return info on single click - to do - might consider putting player info in a tooltip on single click/right click
void JoinServerFunc( void *self )
{
	char	buffer[128];
	int		index;
	int		i;

	cl.tactical = false;

	index = ( menuframework_s * ) self - s_serverlist_rows;

	if(cursor.buttonclicks[MOUSEBUTTON1] != 2 || serverindex != index)
	{
		SelectServer (self);
		return;
	}

	if(!pNameUnique) {
		M_Menu_PlayerConfig_f();
		return;
	}

	remoteserver_runspeed = 300; //default
	for ( i = 0; i < 16; i++)
	{
		if( !strcmp("aa tactical", Info_ValueForKey(mod_names, local_mods_data[i])) )
		{
			remoteserver_runspeed = 200; //for correct prediction
			M_Menu_Tactical_f();
			return;
		}
		else if( !strcmp("excessive", Info_ValueForKey(mod_names, local_mods_data[i])) )
			remoteserver_runspeed = 450;
		else if( !strcmp("playerspeed", Info_ValueForKey(mod_names, local_mods_data[i])) )
			remoteserver_runspeed = 450;
	} //TO DO:  We need to do the speed check on connect instead - meaning the server will need to be pinged and parsed there as well(but only if not done already through the menu).

	Com_sprintf (buffer, sizeof(buffer), "connect %s\n", NET_AdrToString (mservers[index].local_server_netadr));
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

void SearchLocalGames( void )
{
	m_num_servers = 0;
	DeselectServer ();
	s_serverlist_submenu.cursor = 0;
	s_serverlist_submenu.nitems = 0;
	s_serverlist_submenu.yscroll = 0;

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

static qboolean QSortReverse;
static int QSortColumn;

static int SortServerList_Compare (const void *_a, const void *_b)
{
	int ret = 0;
	const menuframework_s *a, *b;
	const char *a_s, *b_s;
	
	a = *(menuframework_s **)_a;
	b = *(menuframework_s **)_b;
	
	a_s = ((menutxt_s *)(a->items[QSortColumn]))->generic.name;
	b_s = ((menutxt_s *)(b->items[QSortColumn]))->generic.name;
	
	if (QSortColumn > 1)
	{
		// do numeric sort for player count and ping
		if (atoi (a_s) > atoi (b_s))
			ret = 1;
		else if (atoi (a_s) < atoi (b_s))
			ret = -1;
	}
	else
		// because strcmp doesn't handle ^colors
		while (*a_s && *b_s)
		{
			if (*a_s == '^')
			{
				a_s++;
			}
			else if (*b_s == '^')
			{
				b_s++;
			}
			else if (tolower(*a_s) > tolower(*b_s))
			{
				ret = 1;
				break;
			}
			else if (tolower(*a_s) < tolower(*b_s))
			{
				ret = -1;
				break;
			}
			a_s++;
			b_s++;
		}
	
	if (QSortReverse)
		return -ret;
	return ret;
}

static void SortServerList_Func ( void *_self )
{
	int column_num, i;
	menulist_s *self = (menulist_s *)_self;
	
	column_num = self-s_serverlist_header_columns;
	
	for (i = 0; i < SERVER_LIST_COLUMNS; i++)
		if (i != column_num)
			s_serverlist_header_columns[i].curvalue = 0;
	
	if (self->curvalue == 0)
	{
		if (column_num == 3)
		{
			self->curvalue = 1;
		}
		else
		{
			s_serverlist_header_columns[3].curvalue = 1;
			SortServerList_Func (&s_serverlist_header_columns[3]);
			return;
		}
	}
	
	QSortColumn = column_num;
	QSortReverse = self->curvalue == 2;
	
	qsort (s_serverlist_submenu.items, s_serverlist_submenu.nitems, sizeof (void*), SortServerList_Compare);
	s_serverlist_submenu.yscroll = 0;
}

void ServerList_SubmenuInit (void)
{
	int i, j;
	
	s_serverlist_submenu.generic.type = MTYPE_SUBMENU;
	s_serverlist_submenu.navagable = true;
	s_serverlist_submenu.nitems = 0;
	s_serverlist_submenu.bordertexture = "menu/sm_";
	
	s_serverlist_header.generic.type = MTYPE_SUBMENU;
	s_serverlist_header.horizontal = true;
	s_serverlist_header.navagable = true;
	s_serverlist_header.nitems = 0;
	
	s_serverlist_header_columns[0].generic.name = "^3Server";
	s_serverlist_header_columns[1].generic.name = "^3Map";
	s_serverlist_header_columns[2].generic.name = "^3Players";
	s_serverlist_header_columns[3].generic.name = "^3Ping";
	
	for (j = 0; j < SERVER_LIST_COLUMNS; j++)
	{
		s_serverlist_header_columns[j].generic.type = MTYPE_SPINCONTROL;
		s_serverlist_header_columns[j].generic.flags = QMF_LEFT_JUSTIFY|QMF_ALLOW_WRAP;
		s_serverlist_header_columns[j].itemnames = updown_names;
		s_serverlist_header_columns[j].generic.itemsizecallback = IconSpinSizeFunc;
		s_serverlist_header_columns[j].generic.itemdraw = IconSpinDrawFunc;
		s_serverlist_header_columns[j].curvalue = 0;
		s_serverlist_header_columns[j].generic.callback = SortServerList_Func;
		Menu_AddItem (&s_serverlist_header, &s_serverlist_header_columns[j]);
	}
	s_serverlist_header_columns[3].curvalue = 1;
	
	Menu_AddItem (&s_joinserver_menu, &s_serverlist_header);
	
	serverindex = -1;
	
	for ( i = 0; i < MAX_LOCAL_SERVERS; i++ )
	{
		s_serverlist_rows[i].generic.type	= MTYPE_SUBMENU;
		s_serverlist_rows[i].generic.callback = JoinServerFunc;
		// undecided on this:
/*		s_serverlist_rows[i].generic.cursorcallback = SelectServer;*/
		
		s_serverlist_rows[i].nitems = 0;
		s_serverlist_rows[i].horizontal = true;
		s_serverlist_rows[i].enable_highlight = true;
		
		s_serverlist_columns[i][0].generic.name = mservers[i].szHostName;
		s_serverlist_columns[i][1].generic.name = mservers[i].szMapName;
		s_serverlist_columns[i][2].generic.name = mservers[i].szPlayers;
		s_serverlist_columns[i][3].generic.name = mservers[i].szPing;
		
		for (j = 0; j < SERVER_LIST_COLUMNS; j++)
		{
			s_serverlist_columns[i][j].generic.type = MTYPE_TEXT;
			s_serverlist_columns[i][j].generic.flags = QMF_LEFT_JUSTIFY;
			LINK(s_serverlist_header_columns[j].generic.x, s_serverlist_columns[i][j].generic.x);
			Menu_AddItem (&s_serverlist_rows[i], &s_serverlist_columns[i][j]);
		}
		
		LINK(s_serverlist_header.lwidth, s_serverlist_rows[i].lwidth);
		LINK(s_serverlist_header.rwidth, s_serverlist_rows[i].rwidth);
		
		Menu_AddItem( &s_serverlist_submenu, &s_serverlist_rows[i] );
	}
	
	Menu_AddItem (&s_joinserver_menu, &s_serverlist_submenu);
	
	s_serverlist_submenu.maxlines = 17;
	
}

void ServerInfo_SubmenuInit (void)
{
	static const char *contents[] = {
		"Skill:",		local_server_data[0],
		"Admin:",		local_server_data[1],
		"Website:",		local_server_data[2],
		"Fraglimit:",	local_server_data[3],
		"Timelimit:",	local_server_data[4],
		"Version:",		local_server_data[5]
	};
	
	static size_t sizes[2] = {sizeof(menutxt_s), sizeof(menutxt_s)};

	setup_window (s_selectedserver_screen, s_serverinfo_submenu, "SERVER");
	
	s_joinserver_server_data_columns[0][0].generic.type		= MTYPE_TEXT;
	s_joinserver_server_data_columns[0][1].generic.type		= MTYPE_TEXT;
	s_joinserver_server_data_columns[0][1].generic.flags	= QMF_LEFT_JUSTIFY;
	
	Menu_MakeTable (&s_serverinfo_submenu, 6, 2, sizes, s_joinserver_server_data_rows, s_joinserver_server_data_rows, s_joinserver_server_data_columns, contents);
	
	s_serverinfo_submenu.nitems = 0;
}

void ModList_SubmenuInit (void)
{
	int i;
	
	setup_window (s_selectedserver_screen, s_modlist_submenu, "GAMEPLAY");
	
	for ( i = 0; i < MAX_SERVER_MODS; i++ )
	{
		s_joinserver_mods_data[i].generic.type	= MTYPE_ACTION;
		s_joinserver_mods_data[i].generic.flags = QMF_LEFT_JUSTIFY;
		s_joinserver_mods_data[i].generic.name	= modnames[i];
		s_joinserver_mods_data[i].generic.tooltip = modtxt[i];
		
		Menu_AddItem( &s_modlist_submenu, &s_joinserver_mods_data[i] );
	}
	
	s_modlist_submenu.maxlines = 5;
	s_modlist_submenu.nitems = 0;
}

void PlayerList_SubmenuInit (void)
{
	int i, j;
	
	static size_t sizes[3] = {sizeof(menutxt_s), sizeof(menutxt_s), sizeof(menutxt_s)};
	
	setup_window (s_selectedserver_screen, s_playerlist_submenu, "PLAYERS");
	
	s_playerlist_scrollingmenu.generic.type = MTYPE_SUBMENU;
	s_playerlist_scrollingmenu.navagable = true;
	s_playerlist_scrollingmenu.nitems = 0;
	
	s_playerlist_header.generic.type = MTYPE_SUBMENU;
	s_playerlist_header.horizontal = true;
	s_playerlist_header.nitems = 0;
	
	s_playerlist_header_columns[SVDATA_PLAYERINFO_NAME].generic.name	= "^7Name";
	s_playerlist_header_columns[SVDATA_PLAYERINFO_SCORE].generic.name	= "^7Score";
	s_playerlist_header_columns[SVDATA_PLAYERINFO_PING].generic.name	= "^7Ping";
	for (i = 0; i < SVDATA_PLAYERINFO; i++)
	{
		s_playerlist_header_columns[i].generic.type			= MTYPE_TEXT;
		if (i > 0)
			s_playerlist_header_columns[i].generic.flags	= QMF_LEFT_JUSTIFY;
		Menu_AddItem (&s_playerlist_header, &s_playerlist_header_columns[i]);
	}
	
	Menu_AddItem (&s_playerlist_submenu, &s_playerlist_header);
	
	for (i = 0; i < MAX_PLAYERS; i++)
		for (j = 0; j < SVDATA_PLAYERINFO; j++)
			local_player_info_ptrs[i*SVDATA_PLAYERINFO+j] = &local_player_info[i][j][0];
	
	Menu_MakeTable	(	&s_playerlist_scrollingmenu,
						MAX_PLAYERS, SVDATA_PLAYERINFO,
						sizes, &s_playerlist_header,
						s_joinserver_player_rows, s_joinserver_player_columns,
						local_player_info_ptrs
					);
	
	for ( i = 0; i < MAX_PLAYERS; i++ )
		s_joinserver_player_rows[i].generic.tooltip = ranktxt[i];
	
	Menu_AddItem (&s_playerlist_submenu, &s_playerlist_scrollingmenu);
	
	s_playerlist_scrollingmenu.maxlines = 7;
	s_playerlist_scrollingmenu.nitems = 0;
}

void ServerListHeader_SubmenuInit (void)
{
	s_joinserver_header.generic.type = MTYPE_SUBMENU;
	s_joinserver_header.nitems = 0;
	s_joinserver_header.horizontal = true;
	s_joinserver_header.navagable = true;
	
	add_action (s_joinserver_header, "address book", AddressBookFunc);
	add_action (s_joinserver_header, "refresh list", SearchLocalGamesFunc);
	add_action (s_joinserver_header, "Rank/Stats", PlayerRankingFunc);

	s_joinserver_filterempty_action.generic.name	= "show empty";
	setup_tickbox (s_joinserver_filterempty_action);
	s_joinserver_filterempty_action.itemnames = offon_names;

	Menu_AddItem( &s_joinserver_header, &s_joinserver_filterempty_action );
	
	Menu_AddItem (&s_joinserver_menu, &s_joinserver_header);
}


void JoinServer_MenuInit( void )
{
	extern cvar_t *name;

	static int gotServers = false;

	if(!gotServers)
	{
		STATS_getStatsDB();
		getLatestGameVersion();
	}
	
	ValidatePlayerName( name->string, (strlen(name->string)+1) );
	Q_strncpyz2( thisPlayer.playername, name->string, sizeof(thisPlayer.playername) );
	thisPlayer.totalfrags = thisPlayer.totaltime = thisPlayer.ranking = 0;
	thisPlayer = getPlayerRanking ( thisPlayer );

	if(!strcmp(name->string, "Player"))
		pNameUnique = false;
	else
		pNameUnique = true;

	s_serverbrowser_screen.nitems = 0;

	setup_window (s_serverbrowser_screen, s_joinserver_menu, "SERVER LIST");
	
	ServerListHeader_SubmenuInit ();
	ServerList_SubmenuInit ();
	
	s_selectedserver_screen.generic.type = MTYPE_SUBMENU;
	s_selectedserver_screen.generic.flags = QMF_SNUG_LEFT;
	s_selectedserver_screen.nitems = 0;
	s_selectedserver_screen.navagable = true;
	s_selectedserver_screen.horizontal = true;
	
	ServerInfo_SubmenuInit ();
	ModList_SubmenuInit ();
	PlayerList_SubmenuInit ();
	
	Menu_AddItem (&s_serverbrowser_screen, &s_selectedserver_screen);

	if(!gotServers)
		SearchLocalGames();
	gotServers = true;
	
	Menu_AutoArrange (&s_serverbrowser_screen);
	
}

void M_LevelShotBackground (const char *mapname)
{
	char path[MAX_QPATH];
	qboolean found = false;
	
	if (mapname != NULL)
	{
		sprintf(path, "/levelshots/%s", mapname);
		if (R_RegisterPic (path))
			found = true;
	}
	
	if (found)
	{
		Draw_Fill (global_menu_xoffset, 0, viddef.width, viddef.height, 15);
		Draw_AlphaStretchPic (global_menu_xoffset, 0, viddef.width, viddef.height, path, 0.90);
	}
}


void JoinServer_MenuDraw(void)
{
	const char *statusbar_text;
	
		//warn user that they cannot join until changing default player name
	if(!pNameUnique) 
		statusbar_text = "You must change your player name from the default before connecting!";
	else if (serverindex == -1)
		statusbar_text = NULL;
	else if (serveroutdated)
		statusbar_text = "Warning: server is ^1outdated!^7 It may have bugs or different gameplay.";
	else
		statusbar_text = "press ENTER or DBL CLICK to connect";

	// FIXME: we really shouldn't have to set it in both places.
	s_serverbrowser_screen.statusbar = statusbar_text;

	s_serverlist_submenu.force_highlight = false;
	if (serverindex != -1 && !Menu_ContainsCursor (s_serverlist_submenu))
	{
		s_serverlist_submenu.force_highlight = true;
		// The order of the items in the menu list on-screen doesn't
		// necessarily correspond to s_serverlist_rows and mservers,
		// due to sorting. So we have to look for the item manually to set
		// the cursor index.
		s_serverlist_submenu.cursor = 0;
		while (s_serverlist_submenu.items[s_serverlist_submenu.cursor] != &s_serverlist_rows[serverindex])
			s_serverlist_submenu.cursor++;
	}
	
	s_serverlist_submenu.nitems = m_num_servers;
	if (serverindex == -1)
		s_serverbrowser_screen.nitems = 1;
	else
		s_serverbrowser_screen.nitems = 2;
	Menu_Draw( &s_serverbrowser_screen );
	
	Menu_AutoArrange (&s_serverbrowser_screen);
}

const char *JoinServer_MenuKey (menuframework_s *screen, int key)
{
	if ( key == K_ENTER && serverindex != -1 )
	{
		cursor.buttonclicks[MOUSEBUTTON1] = 2;//so we can still join without a mouse
		JoinServerFunc (&s_serverlist_rows[serverindex]);
		return "";
	}
	if (serverindex != -1 && !Menu_ContainsCursor (s_joinserver_menu) && cursor.buttonclicks[MOUSEBUTTON1] == 1)
	{
		DeselectServer ();
		return menu_out_sound;
	}
	return Default_MenuKey (screen, key );
}

void M_Menu_JoinServer_f (void)
{
	JoinServer_MenuInit();
	M_PushMenu( JoinServer_MenuDraw, JoinServer_MenuKey, &s_serverbrowser_screen);
}

/*
=============================================================================

MUTATORS MENU

=============================================================================
*/
static menuframework_s s_mutators_screen;
static menuframework_s s_mutators_menu;

// weapon modes are different from regular mutators in that they cannot be
// combined
static const char *weaponModeNames[][2] = 
{
	{"instagib",		"instagib"},
	{"rocket arena",	"rocket_arena"},
	{"insta/rockets",	"insta_rockets"},
	{"excessive",		"excessive"},
	{"class based",		"clasbased"}
};
#define num_weapon_modes (sizeof(weaponModeNames)/sizeof(weaponModeNames[0]))
static menulist_s s_weaponmode_list[num_weapon_modes];

static const char *mutatorNames[][2] = 
{
	{"vampire",			"vampire"},
	{"regen",			"regeneration"},
	{"quick weapons",	"quickweap"},
	{"anticamp",		"anticamp"},
	{"speed",			"playerspeed"},
	{"low gravity",		"low_grav"},
	{"jousting",		"sv_joustmode"},
	{"grapple hook",	"grapple"}
};
#define num_mutators (sizeof(mutatorNames)/sizeof(mutatorNames[0]))
static menulist_s s_mutator_list[num_mutators];
static menufield_s s_camptime;

static char dmflags_display_buffer[128];

static void DMFlagCallback( void *self )
{
	menulist_s *f = ( menulist_s * ) self;
	int flags;
	int bit;
	qboolean invert, enabled;

	flags = Cvar_VariableValue( "dmflags" );
	
	if (f != NULL)
	{
		bit = f->generic.localints[0];
		invert = f->generic.localints[1];
		enabled = f->curvalue != 0;
	
		if (invert != enabled)
			flags |= bit;
		else
			flags &= ~bit;
	}

	Cvar_SetValue ("dmflags", flags);

	Com_sprintf( dmflags_display_buffer, sizeof( dmflags_display_buffer ), "(dmflags = %d)", flags );
}

typedef struct {
	char		*display_name;
	qboolean	invert;
	int			bit;
} DMFlag_control_t;

static const DMFlag_control_t dmflag_control_names[] = {
	{"falling damage",		true,	DF_NO_FALLING},
	{"weapons stay",		false,	DF_WEAPONS_STAY},
	{"instant powerups",	false,	DF_INSTANT_ITEMS},
	{"allow powerups",		true,	DF_NO_ITEMS},
	{"allow health",		true,	DF_NO_HEALTH},
	{"allow armor",			true,	DF_NO_ARMOR},
	{"spawn farthest",		false,	DF_SPAWN_FARTHEST},
	{"same map",			false,	DF_SAME_LEVEL},
	{"force respawn",		false,	DF_FORCE_RESPAWN},
	{"team deathmatch",		false,	DF_SKINTEAMS},
	{"allow exit", 			false,	DF_ALLOW_EXIT},
	{"infinite ammo",		false,	DF_INFINITE_AMMO},
	{"quad drop",			false,	DF_QUAD_DROP},
	{"friendly fire",		true,	DF_NO_FRIENDLY_FIRE},
	{"bot chat",			false,	DF_BOTCHAT},
	{"bot fuzzy aim",		false,	DF_BOT_FUZZYAIM},
	{"auto node save",		false,	DF_BOT_AUTOSAVENODES},
	{"repeat level if "
	 "bot wins",			true,	DF_BOT_LEVELAD},
	{"bots in game",		true,	DF_BOTS}
};
#define num_dmflag_controls (sizeof(dmflag_control_names)/sizeof(dmflag_control_names[0]))

static menuframework_s	s_dmflags_submenu;
static menulist_s		s_dmflag_controls[num_dmflag_controls];

void SetWeaponModeFunc(void *_self)
{
	menulist_s *self;
	int i, value;
	
	self = (menulist_s*)_self;
	
	value = self->curvalue;
	
	if (self->curvalue)
	{
		for (i = 0; i < num_weapon_modes; i++)
		{
			Cvar_SetValue (weaponModeNames[i][1], 0);
			s_weaponmode_list[i].curvalue = 0;
		}
	}
	
	Cvar_SetValue (self->generic.localstrings[0], value);
	self->curvalue = value;
}

void Mutators_MenuInit( void )
{
	int i;
	
	int dmflags = Cvar_VariableValue( "dmflags" );
	
	s_mutators_screen.nitems = 0;
	
	setup_window (s_mutators_screen, s_mutators_menu, "MUTATORS");
	
	for (i = 0; i < num_weapon_modes; i++)
	{
		s_weaponmode_list[i].generic.name = weaponModeNames[i][0];
		s_weaponmode_list[i].generic.callback = SetWeaponModeFunc;
		s_weaponmode_list[i].generic.localstrings[0] = weaponModeNames[i][1];
		s_weaponmode_list[i].curvalue = Cvar_VariableValue (weaponModeNames[i][1]);
		setup_radiobutton (s_weaponmode_list[i]);
		Menu_AddItem (&s_mutators_menu, &s_weaponmode_list[i]);
	}
	
	s_camptime.generic.type = MTYPE_FIELD;
	s_camptime.generic.name = "camp time";
	s_camptime.generic.flags = QMF_NUMBERSONLY;
	s_camptime.generic.localstrings[0] = "camptime";
	s_camptime.length = 3;
	s_camptime.generic.visible_length = 3;
	strcpy( s_camptime.buffer, Cvar_VariableString("camptime") );
	s_camptime.generic.callback = IntFieldCallback;
	
	for (i = 0; i < num_mutators; i++)
	{
		s_mutator_list[i].generic.name = mutatorNames[i][0];
		s_mutator_list[i].generic.callback = SpinOptionFunc;
		s_mutator_list[i].generic.localstrings[0] = weaponModeNames[i][1];
		s_mutator_list[i].curvalue = Cvar_VariableValue (mutatorNames[i][1]);
		setup_tickbox (s_mutator_list[i]);
		Menu_AddItem (&s_mutators_menu, &s_mutator_list[i]);
		
		// camptime goes after anticamp control-- we put this here so we can
		// insert it in the right place in the menu
		if (!strcmp (mutatorNames[i][0], "anticamp"))
			Menu_AddItem( &s_mutators_menu, &s_camptime );
	}
	
	add_text (s_mutators_menu, dmflags_display_buffer, 0);
	
	s_dmflags_submenu.generic.type = MTYPE_SUBMENU;
	s_dmflags_submenu.navagable = true;
	s_dmflags_submenu.bordertexture = "menu/sm_";
	s_dmflags_submenu.generic.flags = QMF_SNUG_LEFT;
	s_dmflags_submenu.nitems = 0;
	s_dmflags_submenu.maxlines = 15;
	for (i = 0; i < num_dmflag_controls; i++)
	{
		s_dmflag_controls[i].generic.name = dmflag_control_names[i].display_name;
		s_dmflag_controls[i].generic.callback = DMFlagCallback;
		setup_tickbox (s_dmflag_controls[i]);
		s_dmflag_controls[i].generic.localints[0] = dmflag_control_names[i].bit;
		s_dmflag_controls[i].generic.localints[1] = dmflag_control_names[i].invert;
		s_dmflag_controls[i].curvalue = (dmflags & dmflag_control_names[i].bit) != 0;
		if (dmflag_control_names[i].invert)
		{
			s_dmflag_controls[i].curvalue = s_dmflag_controls[i].curvalue == 0;
		}
		
		Menu_AddItem (&s_dmflags_submenu, &s_dmflag_controls[i]);
	}
	
	Menu_AddItem (&s_mutators_menu, &s_dmflags_submenu);
	
	// set the original dmflags statusbar
	DMFlagCallback( 0 );
	Menu_AutoArrange (&s_mutators_screen);
}

screen_boilerplate (Mutators, s_mutators_screen)
/*
=============================================================================

ADD BOTS MENU

=============================================================================
*/
static menuframework_s	s_addbots_screen;
static menuframework_s	s_addbots_menu;
static menuframework_s	s_addbots_header;
static menutxt_s		s_addbots_name_label;
static menutxt_s		s_addbots_skill_label;
static menutxt_s		s_addbots_faveweap_label;

int totalbots;

#define MAX_BOTS 16
struct botdata {
	char	name[32];
	char	model[64];
	char	userinfo[MAX_INFO_STRING];
	char	faveweap[64];
	int		skill;
	
	// menu entities
	menuframework_s	row;
	menuaction_s	action;
	char			skill_buf[2];
	menutxt_s		m_skill;
	menutxt_s		m_faveweap;
} bots[MAX_BOTS];

static menulist_s		s_startmap_list;
static menulist_s		s_rules_box;
static menulist_s   	s_bots_bot_action[8];
#define MAX_MAPS 256
static char *mapnames[MAX_MAPS + 2];

struct botinfo {
	char name[32];
	char userinfo[MAX_INFO_STRING];
} bot[8];

int slot;

void LoadBotInfo( void )
{
	FILE *pIn;
	int i, count;
	char *name;
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
	if(count>MAX_BOTS)
		count = MAX_BOTS;

	for(i=0;i<count;i++)
	{
		char *cfg, *s;
		char cfgpath[MAX_QPATH];
		const char *delim = "\r\n";
		
		szr = fread(bots[i].userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn);

		name = Info_ValueForKey (bots[i].userinfo, "name");
		skin = Info_ValueForKey (bots[i].userinfo, "skin");
		strncpy(bots[i].name, name, sizeof(bots[i].name)-1);
		Com_sprintf (bots[i].model, sizeof(bots[i].model), "bots/%s_i", skin);
		
		// defaults for .cfg data
		bots[i].skill = 1; //medium
		strcpy (bots[i].faveweap, "None");
		Com_sprintf (bots[i].skill_buf, sizeof(bots[i].skill_buf), "%d", bots[i].skill);
		
		// load info from config file if possible
		
		Com_sprintf (cfgpath, sizeof(cfgpath), "%s/%s.cfg", BOT_GAMEDATA, name);
		if( FS_LoadFile (cfgpath, &cfg) == -1 )
		{
			Com_DPrintf("LoadBotInfo: failed file open: %s\n", fullpath );
			continue;
		}
		
		if ( (s = strtok( cfg, delim )) != NULL )
			bots[i].skill = atoi( s );
		if ( bots[i].skill < 0 )
			bots[i].skill = 0;
		
		Com_sprintf (bots[i].skill_buf, sizeof(bots[i].skill_buf), "%d", bots[i].skill);
		
		if ( s && ((s = strtok( NULL, delim )) != NULL) )
			strncpy( bots[i].faveweap, s, sizeof(bots[i].faveweap)-1 );
		
		Z_Free (cfg);
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
	menuframework_s *f = ( menuframework_s * ) self;

	//get the name and copy that config string into the proper slot name
	for(i = 0; i < totalbots; i++)
	{
		if (f == &bots[i].row)
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
void AddBots_MenuInit( void )
{
	int i;

	totalbots = 0;

	LoadBotInfo();

	s_addbots_screen.nitems = 0;
	
	setup_window (s_addbots_screen, s_addbots_menu, "CHOOSE A BOT");
	s_addbots_menu.maxlines = 16;
	
	s_addbots_header.generic.type = MTYPE_SUBMENU;
	s_addbots_header.horizontal = true;
	s_addbots_header.nitems = 0;
	
	s_addbots_name_label.generic.type = MTYPE_TEXT;
	s_addbots_name_label.generic.name = "^3bot";
	Menu_AddItem (&s_addbots_header, &s_addbots_name_label);
	
	s_addbots_skill_label.generic.type = MTYPE_TEXT;
	s_addbots_skill_label.generic.name = "^3skill";
	Menu_AddItem (&s_addbots_header, &s_addbots_skill_label);
	
	s_addbots_faveweap_label.generic.type = MTYPE_TEXT;
	s_addbots_faveweap_label.generic.flags = QMF_LEFT_JUSTIFY;
	s_addbots_faveweap_label.generic.name = "^3favorite ^3weapon";
	Menu_AddItem (&s_addbots_header, &s_addbots_faveweap_label);
	
	Menu_AddItem (&s_addbots_menu, &s_addbots_header);

	for(i = 0; i < totalbots; i++) {
		bots[i].row.generic.type = MTYPE_SUBMENU;
		bots[i].row.generic.flags = QMF_SNUG_LEFT;
		bots[i].row.nitems = 0;
		bots[i].row.horizontal = true;
		bots[i].row.enable_highlight = true;
		
		bots[i].row.generic.callback = AddbotFunc;
	
		bots[i].action.generic.type	= MTYPE_ACTION;
		bots[i].action.generic.name	= bots[i].name;
		bots[i].action.generic.localstrings[0] = bots[i].model;
		VectorSet (bots[i].action.generic.localints, 2, 2, RCOLUMN_OFFSET);
		bots[i].action.generic.itemsizecallback = PicSizeFunc;
		bots[i].action.generic.itemdraw = PicDrawFunc;
		LINK(s_addbots_name_label.generic.x, bots[i].action.generic.x);
		Menu_AddItem (&bots[i].row, &bots[i].action);
		
		bots[i].m_skill.generic.type = MTYPE_TEXT;
		bots[i].m_skill.generic.name = bots[i].skill_buf;
		LINK(s_addbots_skill_label.generic.x, bots[i].m_skill.generic.x);
		Menu_AddItem (&bots[i].row, &bots[i].m_skill);
		
		bots[i].m_faveweap.generic.type = MTYPE_TEXT;
		bots[i].m_faveweap.generic.flags = QMF_LEFT_JUSTIFY;
		bots[i].m_faveweap.generic.name = bots[i].faveweap;
		LINK(s_addbots_faveweap_label.generic.x, bots[i].m_faveweap.generic.x);
		Menu_AddItem (&bots[i].row, &bots[i].m_faveweap);

		LINK(s_addbots_header.lwidth, bots[i].row.lwidth);
		LINK(s_addbots_header.rwidth, bots[i].row.rwidth);
		Menu_AddItem( &s_addbots_menu, &bots[i].row );
	}

	Menu_AutoArrange (&s_addbots_screen);

}

/*
=============================================================================

START SERVER MENU

=============================================================================
*/


static menuframework_s s_startserver_screen;
static menuframework_s s_startserver_menu;
static int	  nummaps = 0;

static menufield_s	s_timelimit_field;
static menufield_s	s_fraglimit_field;
static menufield_s	s_maxclients_field;
static menufield_s	s_hostname_field;
static menulist_s	s_antilag_box;
static menulist_s   s_public_box;
static menulist_s	s_dedicated_box;
static menulist_s   s_skill_box;
static menulist_s   s_startserver_map_data[5];

void BotOptionsFunc( void *self )
{
	M_Menu_BotOptions_f();
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

	//get a map description if it is there

	if(mapnames[0])
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
				s_startserver_map_data[i].generic.type	= MTYPE_TEXT;
				s_startserver_map_data[i].generic.name	= token;
				s_startserver_map_data[i].generic.flags	= QMF_LEFT_JUSTIFY;

				i++;
			}

		}

		fclose(desc_file);

	}
	else
	{
		for (i = 0; i < 5; i++ )
		{
			s_startserver_map_data[i].generic.type	= MTYPE_TEXT;
			s_startserver_map_data[i].generic.name	= "no data";
			s_startserver_map_data[i].generic.flags	= QMF_LEFT_JUSTIFY;
		}
	}

}

static const char *game_mode_names[] =
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
#define num_game_modes (sizeof(game_mode_names)/(sizeof(game_mode_names[0]))-1)

//same order as game_mode_names
static const char *map_prefixes[num_game_modes][3] =
{
	{"dm", "tourney", NULL},
	{"ctf", NULL},
	{"aoa", NULL},
	{"db", NULL},
	{"tca", NULL},
	{"cp", NULL},
	{"dm", "tourney", NULL}
};

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

	//clear out list first
	for ( i = 0; i < nummaps; i++ )
		free( mapnames[i] );

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
		fclose( fp );
		free( buffer );
		Com_Error( ERR_DROP, "no maps in maps.lst\n" );
		return; // for showing above is fatal.
	}

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
		
		// Each game mode has one or more map name prefixes. For example, if
		// the game mode is capture the flag, only maps that start with ctf
		// should make it into the mapnames list.
		for (j = 0; map_prefixes[s_rules_box.curvalue][j]; j++)
		{
			const char *curpfx = map_prefixes[s_rules_box.curvalue][j];
			if (!strncmp (curpfx, shortname, strlen(curpfx)))
			{
				// matched an allowable prefix
				mapnames[k] = malloc( strlen( scratch ) + 1 );
				strcpy( mapnames[k], scratch );
				k++;
				break;
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
		
			// FIXME: copy and paste sux0rs
			// Each game mode has one or more map name prefixes. For example, if
			// the game mode is capture the flag, only maps that start with ctf
			// should make it into the mapnames list.
			for (j = 0; map_prefixes[s_rules_box.curvalue][j]; j++)
			{
				const char *curpfx = map_prefixes[s_rules_box.curvalue][j];
				if (!strncmp (curpfx, curMap, strlen(curpfx)))
				{
					// matched an allowable prefix
					mapnames[k] = malloc( strlen( scratch ) + 1 );
					strcpy( mapnames[k], scratch );
					k++;
					totalmaps++;
					break;
				}
			}
		}
	}

	if (mapfiles)
		FS_FreeFileList(mapfiles, nmaps);

	for(i = k; i<=nummaps; i++) 
	{
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
	Cvar_SetValue( "g_antilag", s_antilag_box.curvalue);

	// The deathmatch cvar doesn't specifically indicate a pure frag-to-win
	// game mode. It's actually the "enable multiplayer" cvar.
	// TODO: Does Alien Arena even work with deathmatch set to 0? Might be
	// able to remove it from the game.
	Cvar_SetValue ("deathmatch", 1 );
	Cvar_SetValue ("ctf", 0);
	Cvar_SetValue ("tca", 0);
	Cvar_SetValue ("cp", 0);
	Cvar_SetValue ("g_duel", 0);
	Cvar_SetValue ("gamerules", s_rules_box.curvalue );
	
	switch (s_rules_box.curvalue)
	{
		case 1:
			Cvar_SetValue ("ctf", 1 );
			break;
		case 4:
			Cvar_SetValue ("tca", 1);
			break;
		case 5:
			Cvar_SetValue ("cp", 1);
			break;
		case 6:
			Cvar_SetValue ("g_duel", 1);
			break;
		default:
			break;
	}

	Cbuf_AddText (va("startmap %s\n", startmap));
	
	M_ForceMenuOff ();

}

void StartServer_MenuInit( void )
{
	int i;

	
	static const char *skill[] =
	{
		"easy",
		"medium",
		"hard",
		0
	};
	
	s_startserver_screen.nitems = 0;

	setup_window (s_startserver_screen, s_startserver_menu, "HOST SERVER");

	s_startmap_list.generic.type = MTYPE_SPINCONTROL;
	s_startmap_list.generic.name	= "initial map";
	s_startmap_list.itemnames = (const char **) mapnames;
	s_startmap_list.generic.callback = MapInfoFunc;
	Menu_AddItem( &s_startserver_menu, &s_startmap_list );

	s_rules_box.generic.type = MTYPE_SPINCONTROL;
	s_rules_box.generic.name	= "rules";
	s_rules_box.itemnames = game_mode_names;
	s_rules_box.curvalue = 0;
	s_rules_box.generic.callback = RulesChangeFunc;
	Menu_AddItem( &s_startserver_menu, &s_rules_box );
	
	add_action (s_startserver_menu, "mutators", MutatorFunc);

	s_antilag_box.generic.name	= "antilag";
	setup_tickbox (s_antilag_box);
	s_antilag_box.curvalue = 1;

	s_timelimit_field.generic.type = MTYPE_FIELD;
	s_timelimit_field.generic.name = "time limit";
	s_timelimit_field.generic.flags = QMF_NUMBERSONLY;
	s_timelimit_field.generic.statusbar = "0 = no limit";
	s_timelimit_field.length = 3;
	s_timelimit_field.generic.visible_length = 3;
	strcpy( s_timelimit_field.buffer, Cvar_VariableString("timelimit") );

	s_fraglimit_field.generic.type = MTYPE_FIELD;
	s_fraglimit_field.generic.name = "frag limit";
	s_fraglimit_field.generic.flags = QMF_NUMBERSONLY;
	s_fraglimit_field.generic.statusbar = "0 = no limit";
	s_fraglimit_field.length = 3;
	s_fraglimit_field.generic.visible_length = 3;
	strcpy( s_fraglimit_field.buffer, Cvar_VariableString("fraglimit") );

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is.
	** Clamping will be done when the server is actually started.
	*/
	s_maxclients_field.generic.type = MTYPE_FIELD;
	s_maxclients_field.generic.name = "max players";
	s_maxclients_field.generic.flags = QMF_NUMBERSONLY;
	s_maxclients_field.length = 3;
	s_maxclients_field.generic.visible_length = 3;
	if ( Cvar_VariableValue( "maxclients" ) == 1 )
		strcpy( s_maxclients_field.buffer, "8" );
	else
		strcpy( s_maxclients_field.buffer, Cvar_VariableString("maxclients") );

	s_hostname_field.generic.type = MTYPE_FIELD;
	s_hostname_field.generic.name = "hostname";
	s_hostname_field.generic.flags = 0;
	s_hostname_field.length = 12;
	s_hostname_field.generic.visible_length = 12;
	strcpy( s_hostname_field.buffer, Cvar_VariableString("hostname") );

	s_public_box.generic.name = "public server";
	setup_tickbox (s_public_box);
	s_public_box.curvalue = 1;


#if defined WIN32_VARIANT
	s_dedicated_box.generic.name = "dedicated server";
	setup_tickbox (s_dedicated_box);
#else
	// may or may not need this when disabling dedicated server menu
	s_dedicated_box.generic.type = -1;
	s_dedicated_box.generic.name = NULL;
	s_dedicated_box.curvalue = 0;
#endif

	s_skill_box.generic.type = MTYPE_SPINCONTROL;
	s_skill_box.generic.name	= "skill level";
	s_skill_box.itemnames = skill;
	s_skill_box.curvalue = 1;

	for ( i = 0; i < 5; i++) { //initialize it
		s_startserver_map_data[i].generic.type	= MTYPE_TEXT;
		s_startserver_map_data[i].generic.name	= "no data";
		s_startserver_map_data[i].generic.flags	= QMF_LEFT_JUSTIFY;
	}

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
	
	add_action (s_startserver_menu, "Bot Options", BotOptionsFunc);
	add_action (s_startserver_menu, "begin", StartServerActionFunc);
	
	for ( i = 0; i < 5; i++ )
		Menu_AddItem( &s_startserver_menu, &s_startserver_map_data[i] );
	
	Menu_AutoArrange (&s_startserver_screen);

	// call this now to set proper inital state
	RulesChangeFunc ( NULL );
	MapInfoFunc(NULL);
}

void StartServer_MenuDraw(void)
{
	char startmap[128];
	
	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	M_LevelShotBackground (startmap);

	Menu_AutoArrange (&s_startserver_screen);
	Menu_Draw( &s_startserver_screen );
}

void M_Menu_StartServer_f (void)
{
	StartServer_MenuInit();
	M_PushMenu( StartServer_MenuDraw, Default_MenuKey, &s_startserver_screen);
}

/*
=============================================================================

BOT OPTIONS MENU

=============================================================================
*/

static menuframework_s s_botoptions_screen;
static menuframework_s s_botoptions_menu;

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

void BotAction (void *self);

void BotOptions_MenuInit( void )
{
	int i;

	for(i = 0; i < 8; i++)
		strcpy(bot[i].name, "...empty slot");

	Read_Bot_Info();
	
	s_botoptions_screen.nitems = 0;

	setup_window (s_botoptions_screen, s_botoptions_menu, "BOT OPTIONS");
	
	for (i = 0; i < 8; i++) {
		s_bots_bot_action[i].generic.type = MTYPE_ACTION;
		s_bots_bot_action[i].generic.name = bot[i].name;
		s_bots_bot_action[i].generic.callback = BotAction;
		s_bots_bot_action[i].curvalue = i;
		Menu_AddItem( &s_botoptions_menu, &s_bots_bot_action[i]);
	}

	Menu_AutoArrange (&s_botoptions_screen);
}

screen_boilerplate (BotOptions, s_botoptions_screen);

screen_boilerplate (AddBots, s_addbots_screen);

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
		M_Menu_AddBots_f();
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

/*
=============================================================================

ADDRESS BOOK MENU

=============================================================================
*/
#define NUM_ADDRESSBOOK_ENTRIES 9

static menuframework_s	s_addressbook_menu;
static char				s_addressbook_cvarnames[NUM_ADDRESSBOOK_ENTRIES][20];
static menufield_s		s_addressbook_fields[NUM_ADDRESSBOOK_ENTRIES];

void AddressBook_MenuInit( void )
{
	int i;

	s_addressbook_menu.nitems = 0;

	for ( i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++ )
	{
		cvar_t *adr;

		Com_sprintf( s_addressbook_cvarnames[i], sizeof( s_addressbook_cvarnames[i] ), "adr%d", i );

		adr = Cvar_Get( s_addressbook_cvarnames[i], "", CVAR_ARCHIVE );

		s_addressbook_fields[i].generic.type			= MTYPE_FIELD;
		s_addressbook_fields[i].generic.callback		= StrFieldCallback;
		s_addressbook_fields[i].generic.localstrings[0]	= &s_addressbook_cvarnames[i][0];
		s_addressbook_fields[i].cursor					= strlen (adr->string);
		s_addressbook_fields[i].generic.visible_length	= 15;

		strcpy( s_addressbook_fields[i].buffer, adr->string );

		Menu_AddItem( &s_addressbook_menu, &s_addressbook_fields[i] );
	}
	
	Menu_AutoArrange (&s_addressbook_menu);
	Menu_Center (&s_addressbook_menu);
}

screen_boilerplate (AddressBook, s_addressbook_menu);

/*
=============================================================================

PLAYER RANKING MENU

=============================================================================
*/

static menuframework_s	s_playerranking_screen;
static menuframework_s	s_playerranking_menu;
static menuaction_s		s_playerranking_title;
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
	int i;

	s_playerranking_screen.nitems = 0;

	setup_window (s_playerranking_screen, s_playerranking_menu, "PLAYER RANKINGS");

	Q_strncpyz2( player.playername, name->string, sizeof(player.playername) );

	player.totalfrags = player.totaltime = player.ranking = 0;
	player = getPlayerRanking ( player );

	Com_sprintf(playername, sizeof(playername), "Name: %s", player.playername);
	if(player.ranking > 0)
		Com_sprintf(rank, sizeof(rank), "Rank: ^1%i", player.ranking);
	else
		Com_sprintf(rank, sizeof(rank), "Rank: ^1Unranked");
	if ( player.totaltime > 1.0f )
		Com_sprintf(fragrate, sizeof(fragrate), "Frag Rate: %6.2f", (float)(player.totalfrags)/(player.totaltime - 1.0f) );
	else
		Com_sprintf(fragrate, sizeof(fragrate), "Frag Rate: 0" );
	Com_sprintf(totalfrags, sizeof(totalfrags), "Total Frags: ^1%i", player.totalfrags);
	Com_sprintf(totaltime, sizeof(totaltime), "Total Time: %6.2f", player.totaltime - 1.0f);

	s_playerranking_title.generic.type	= MTYPE_ACTION;
	s_playerranking_title.generic.name	= "Player Ranking and Stats";
	s_playerranking_title.generic.flags	= QMF_LEFT_JUSTIFY;
	Menu_AddItem( &s_playerranking_menu, &s_playerranking_title );
	
	add_text(s_playerranking_menu, playername, QMF_LEFT_JUSTIFY);
	add_text(s_playerranking_menu, rank, QMF_LEFT_JUSTIFY);
	add_text(s_playerranking_menu, fragrate, QMF_LEFT_JUSTIFY);
	add_text(s_playerranking_menu, totalfrags, QMF_LEFT_JUSTIFY);
	add_text(s_playerranking_menu, totaltime, QMF_LEFT_JUSTIFY);

	s_playerranking_ttheader.generic.type	= MTYPE_ACTION;
	s_playerranking_ttheader.generic.name	= "Top Ten Players";
	s_playerranking_ttheader.generic.flags	= QMF_LEFT_JUSTIFY;
	Menu_AddItem (&s_playerranking_menu, &s_playerranking_ttheader);

	for(i = 0; i < 10; i++) {

		topTenPlayers[i].totalfrags = topTenPlayers[i].totaltime = topTenPlayers[i].ranking = 0;
		topTenPlayers[i] = getPlayerByRank ( i+1, topTenPlayers[i] );

		if(i < 9)
			Com_sprintf(topTenList[i], sizeof(topTenList[i]), "Rank: ^1%i %s", topTenPlayers[i].ranking, topTenPlayers[i].playername);
		else
			Com_sprintf(topTenList[i], sizeof(topTenList[i]), "Rank:^1%i %s", topTenPlayers[i].ranking, topTenPlayers[i].playername);

		s_playerranking_topten[i].generic.type	= MTYPE_TEXT;
		s_playerranking_topten[i].generic.name	= topTenList[i];
		s_playerranking_topten[i].generic.flags	= QMF_LEFT_JUSTIFY;

		Menu_AddItem( &s_playerranking_menu, &s_playerranking_topten[i] );
	}
	
	Menu_AutoArrange (&s_playerranking_screen);
}

screen_boilerplate (PlayerRanking, s_playerranking_screen)

/*
=============================================================================

PLAYER CONFIG MENU

=============================================================================
*/

typedef struct 
{
	menucommon_s generic;
	const char *name;
	const char *skin;
	int w, h;
	float mframe, yaw;
} menumodel_s;

static menuvec2_t PlayerModelSizeFunc (void *_self, FNT_font_t font)
{
	menuvec2_t ret;
	menumodel_s *self = (menumodel_s*) _self;
	
	ret.x = (self->w+2)*font->size;
	ret.y = (self->h+2)*font->size;
	
	return ret;
}

static void PlayerModelDrawFunc (void *_self, FNT_font_t font)
{
	refdef_t refdef;
	char scratch[MAX_OSPATH];
	FILE *modelfile;
	int i;
	extern float CalcFov( float fov_x, float w, float h );
	float scale;
	entity_t entity[3];
	menumodel_s *self = (menumodel_s*) _self;
	
	self->mframe += cls.frametime*150;
	if ( self->mframe > 390 )
		self->mframe = 10;
	if ( self->mframe < 10)
		self->mframe = 10;

	self->yaw += cls.frametime*50;
	if (self->yaw > 360)
		self->yaw = 0;

	scale = (float)(viddef.height)/600;
	
	memset( &refdef, 0, sizeof( refdef ) );
	
	refdef.width = self->w*font->size;
	refdef.height = self->h*font->size;
	refdef.x = Item_GetX(*self);
	refdef.y = Item_GetY(*self);
	refdef.x -= refdef.width;
	
	Menu_DrawBox (refdef.x, refdef.y, refdef.width, refdef.height, 1, NULL, "menu/sm_");
	
	refdef.width -= font->size;
	refdef.height -= font->size;
	
	refdef.fov_x = 50;
	refdef.fov_y = CalcFov( refdef.fov_x, refdef.width, refdef.height );
	refdef.time = cls.realtime*0.001;
	
	memset( &entity, 0, sizeof( entity ) );

	Com_sprintf( scratch, sizeof( scratch ), "players/%s/tris.md2", self->name );
	entity[0].model = R_RegisterModel( scratch );
	Com_sprintf( scratch, sizeof( scratch ), "players/%s/%s.jpg", self->name, self->skin );
	entity[0].skin = R_RegisterSkin( scratch );
	entity[0].flags = RF_FULLBRIGHT | RF_MENUMODEL;

	Com_sprintf( scratch, sizeof( scratch ), "players/%s/weapon.md2", self->name );
	entity[1].model = R_RegisterModel( scratch );
	Com_sprintf( scratch, sizeof( scratch ), "players/%s/weapon.tga", self->name );
	entity[1].skin = R_RegisterSkin( scratch );
	entity[1].flags = RF_FULLBRIGHT | RF_MENUMODEL;
	
	refdef.num_entities = 2;

	//if a helmet or other special device
	Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.md2", self->name );
	FS_FOpenFile( scratch, &modelfile );
	if ( modelfile )
	{
		fclose(modelfile);
		
		entity[2].model = R_RegisterModel( scratch );
		Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.tga", self->name );
		entity[2].skin = R_RegisterSkin( scratch );
		entity[2].flags = RF_FULLBRIGHT | RF_TRANSLUCENT | RF_MENUMODEL;
		entity[2].alpha = 0.4;
		
		refdef.num_entities = 3;
	}

	for (i = 0; i < refdef.num_entities; i++)
	{
		float len, len2; 
		
		entity[i].frame = (int)(self->mframe/10);
		entity[i].oldframe = (int)(self->mframe/10) - 1;
		entity[i].backlerp = 1.0;
		entity[i].angles[1] = (int)self->yaw;
		
		VectorSet (entity[i].origin, 70, 0, 0);
		
		len = VectorLength (entity[i].origin);
		entity[i].origin[1] -= (refdef.x+refdef.width/2-viddef.width/2)/(4*scale);
		entity[i].origin[2] -= (refdef.y+refdef.height/2-viddef.height/2)/(4*scale);
		len2 = VectorLength (entity[i].origin);
		
		vectoangles (entity[i].origin, refdef.viewangles);
		
		entity[i].origin[2] -= 5;
		
		VectorScale (entity[i].origin, len/len2, entity[i].origin);
		VectorCopy (entity[i].origin, entity[i].oldorigin);
	}
		
	refdef.areabits = 0;
	refdef.entities = entity;
	refdef.lightstyles = 0;
	refdef.rdflags = RDF_NOWORLDMODEL;
	
	R_RenderFramePlayerSetup( &refdef );
}

static const char *download_option_names[][2] = 
{
	{"allow_download",			"download missing files"},
	{"allow_download_maps",		"maps"},
	{"allow_download_players",	"player models/skins"},
	{"allow_download_models",	"models"},
	{"allow_download_sound",	"sounds"}
};
#define num_download_options (sizeof(download_option_names)/sizeof(download_option_names[0]))
static menulist_s s_download_boxes[num_download_options];

static menuframework_s	s_player_config_screen;

static menuframework_s	s_player_config_menu;
static menufield_s		s_player_name_field;
static menufield_s		s_player_password_field;
static menuframework_s	s_player_skin_submenu;
static menuframework_s	s_player_skin_controls_submenu;
static menulist_s		s_player_model_box;
static menulist_s		s_player_skin_box;
static menuitem_s   	s_player_thumbnail;
static menulist_s		s_player_handedness_box;
static menulist_s		s_player_rate_box;
static menufield_s		s_player_fov_field;

static menumodel_s		s_player_skin_preview;

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
			//try for tris.iqm if no md2
			strcpy( scratch, dirnames[i] );
			strcat( scratch, "/tris.iqm" );
			if (!FS_FileExists(scratch))
			{
				free( dirnames[i] );
				dirnames[i] = 0;
				continue;
			}
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

		FS_FreeFileList( pcxnames, npcxfiles );

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


static void PlayerPicDrawFunc (void *_self, FNT_font_t font)
{
	int x, y;
	char scratch[MAX_QPATH];
	menuitem_s *self = (menuitem_s *)_self;
	x = Item_GetX (*self);
	y = Item_GetY (*self);
	
	Com_sprintf( scratch, sizeof( scratch ), "/players/%s/%s_i.tga",
			s_pmi[s_player_model_box.curvalue].directory,
			s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );
	
	Draw_StretchPic (x, y, font->size*5, font->size*5, scratch);
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
	cvar_t *hand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );

	static const char *handedness[] = { "right", "left", "center", 0 };

	scale = (float)(viddef.height)/600;

	PlayerConfig_ScanDirectories();

	if (s_numplayermodels == 0)
		return false;

	if ( hand->value < 0 || hand->value > 2 )
		Cvar_SetValue( "hand", 0 );

	Q_strncpyz( currentdirectory, Cvar_VariableString ("skin"), sizeof(currentdirectory)-1);

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

	s_player_config_screen.nitems = 0;
	s_player_config_screen.horizontal = true;
	
	setup_window (s_player_config_screen, s_player_config_menu, "PLAYER SETUP");

	s_player_name_field.generic.type = MTYPE_FIELD;
	s_player_name_field.generic.name = "name";
	s_player_name_field.length	= 20;
	s_player_name_field.generic.visible_length = 20;
	Q_strncpyz2( s_player_name_field.buffer, name->string, sizeof(s_player_name_field.buffer) );
	s_player_name_field.cursor = strlen( s_player_name_field.buffer );

	s_player_password_field.generic.type = MTYPE_FIELD;
	s_player_password_field.generic.name = "password";
	s_player_password_field.length	= 20;
	s_player_password_field.generic.visible_length = 20;
	s_player_password_field.generic.statusbar = "COR Entertainment is not responsible for lost or stolen passwords";
	Q_strncpyz2( s_player_password_field.buffer, "********", sizeof(s_player_password_field.buffer) );
	s_player_password_field.cursor = 0;
	
	// Horizontal submenu with two items. The first is a submenu with the
	// model/skin controls. The second is just a thumbnail of the current
	// selection.
	s_player_skin_submenu.generic.type = MTYPE_SUBMENU;
	// Keep the model/skin controls vertically lined up:
	s_player_skin_submenu.generic.flags = QMF_SNUG_LEFT;
	s_player_skin_submenu.navagable = true;
	s_player_skin_submenu.horizontal = true;
	s_player_skin_submenu.nitems = 0;
	
	// Vertical sub-submenu with two items. The first is the model control. 
	// The second is the skin control.
	s_player_skin_controls_submenu.generic.type = MTYPE_SUBMENU;
	s_player_skin_controls_submenu.navagable = true;
	s_player_skin_controls_submenu.nitems = 0;
	// keep the model/skin controls vertically lined up:
	LINK (s_player_config_menu.lwidth, s_player_skin_controls_submenu.lwidth);

	s_player_model_box.generic.type = MTYPE_SPINCONTROL;
	s_player_model_box.generic.name = "model";
	s_player_model_box.generic.callback = ModelCallback;
	s_player_model_box.curvalue = currentdirectoryindex;
	s_player_model_box.itemnames = (const char **) s_pmnames;

	s_player_skin_box.generic.type = MTYPE_SPINCONTROL;
	s_player_skin_box.generic.name = "skin";
	s_player_skin_box.curvalue = currentskinindex;
	s_player_skin_box.itemnames = (const char **) s_pmi[currentdirectoryindex].skindisplaynames;
	
	Menu_AddItem( &s_player_skin_controls_submenu, &s_player_model_box );
	if ( s_player_skin_box.itemnames )
		Menu_AddItem( &s_player_skin_controls_submenu, &s_player_skin_box );
	
	Menu_AddItem (&s_player_skin_submenu, &s_player_skin_controls_submenu);
	
	// TODO: click this to cycle skins
	s_player_thumbnail.generic.type = MTYPE_NOT_INTERACTIVE;
	VectorSet(s_player_thumbnail.generic.localints, 5, 5, 0);
	s_player_thumbnail.generic.itemsizecallback = PicSizeFunc;
	s_player_thumbnail.generic.itemdraw = PlayerPicDrawFunc;
	Menu_AddItem (&s_player_skin_submenu, &s_player_thumbnail);

	s_player_handedness_box.generic.type = MTYPE_SPINCONTROL;
	s_player_handedness_box.generic.name = "handedness";
	s_player_handedness_box.generic.localstrings[0] = "hand";
	s_player_handedness_box.generic.callback = SpinOptionFunc;
	s_player_handedness_box.curvalue = Cvar_VariableValue( "hand" );
	s_player_handedness_box.itemnames = handedness;

	s_player_fov_field.generic.type = MTYPE_FIELD;
	s_player_fov_field.generic.name = "fov";
	s_player_fov_field.generic.localstrings[0] = "fov";
	s_player_fov_field.length	= 6;
	s_player_fov_field.generic.visible_length = 6;
	s_player_fov_field.generic.callback = IntFieldCallback;
	strcpy( s_player_fov_field.buffer, fov->string );
	s_player_fov_field.cursor = strlen( fov->string );

	Menu_AddItem( &s_player_config_menu, &s_player_name_field );
	Menu_AddItem( &s_player_config_menu, &s_player_password_field);
	Menu_AddItem( &s_player_config_menu, &s_player_skin_submenu);
	Menu_AddItem( &s_player_config_menu, &s_player_handedness_box );
	Menu_AddItem( &s_player_config_menu, &s_player_fov_field );
		
	for (i = 0; i < num_download_options; i++)
	{
		s_download_boxes[i].generic.name = download_option_names[i][1];
		s_download_boxes[i].generic.callback = SpinOptionFunc;
		s_download_boxes[i].generic.localstrings[0] = download_option_names[i][0];
		setup_tickbox (s_download_boxes[i]);
		s_download_boxes[i].curvalue = (Cvar_VariableValue (download_option_names[i][0]) != 0);
		Menu_AddItem (&s_player_config_menu, &s_download_boxes[i]);
	}
	
	for (i = 0; i < sizeof(rate_tbl) / sizeof(*rate_tbl) - 1; i++)
		if (Cvar_VariableValue("rate") == rate_tbl[i])
			break;

	s_player_rate_box.generic.type = MTYPE_SPINCONTROL;
	s_player_rate_box.generic.name	= "connection";
	s_player_rate_box.generic.callback = RateCallback;
	s_player_rate_box.curvalue = i;
	s_player_rate_box.itemnames = rate_names;

	Menu_AddItem( &s_player_config_menu, &s_player_rate_box );
	
	s_player_skin_preview.generic.type = MTYPE_NOT_INTERACTIVE;
	s_player_skin_preview.generic.namesizecallback = PlayerModelSizeFunc;
	s_player_skin_preview.generic.namedraw = PlayerModelDrawFunc;
	s_player_skin_preview.h = 36;
	s_player_skin_preview.w = 28;
	
	Menu_AddItem (&s_player_config_screen, &s_player_skin_preview);
	
	Menu_AutoArrange (&s_player_config_screen);

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
	if(!strcmp(s_player_name_field.buffer, "Player"))
		pNameUnique = false;
	else
		pNameUnique = true;

	if(!pNameUnique)
		s_player_config_menu.statusbar = "You must change your player name before joining a server!";

	if ( s_pmi[s_player_model_box.curvalue].skindisplaynames )
	{
		s_player_skin_preview.name = s_pmi[s_player_model_box.curvalue].directory;
		s_player_skin_preview.skin = s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue];
		Menu_Draw( &s_player_config_screen );
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

	//was the password changed?
	if(strcmp("********", s_player_password_field.buffer))
	{
		//if this is a virgin password, don't change, just authenticate
		if(!strcmp(stats_password->string, "password"))
		{
			Cvar_FullSet( "stats_password", s_player_password_field.buffer, CVAR_PROFILE);
			stats_password = Cvar_Get("stats_password", "password", CVAR_PROFILE);
			Cvar_FullSet( "stats_pw_hashed", "0", CVAR_PROFILE);
			currLoginState.validated = false;
			STATS_RequestVerification();
		}
		else
		{
			Cvar_FullSet( "stats_password", s_player_password_field.buffer, CVAR_PROFILE);
			stats_password = Cvar_Get("stats_password", "password", CVAR_PROFILE);
			Cvar_FullSet( "stats_pw_hashed", "0", CVAR_PROFILE);
			STATS_RequestPwChange();
		}
	}

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
const char *PlayerConfig_MenuKey (menuframework_s *screen, int key)
{

	if ( key == K_ESCAPE )
		PConfigAccept();

	return Default_MenuKey (screen, key);
}


void M_Menu_PlayerConfig_f (void)
{
	if (!PlayerConfig_MenuInit())
	{
		Menu_SetStatusBar( &s_options_menu, "No valid player models found" );
		return;
	}
	Menu_SetStatusBar( &s_options_menu, NULL );
	M_PushMenu( PlayerConfig_MenuDraw, PlayerConfig_MenuKey, &s_player_config_screen);
}

/*
=======================================================================

ALIEN ARENA TACTICAL MENU

=======================================================================
*/

static menuframework_s	s_tactical_screen;
static menuaction_s		s_tactical_title_action;

#define num_tactical_teams		2
#define num_tactical_classes	3
static const char *tactical_skin_names[num_tactical_teams][num_tactical_classes][2] =
{
	//ALIEN CLASSES
	{
		{"enforcer",	"martianenforcer"},
		{"warrior",		"martianwarrior"},
		{"overlord",	"martianoverlord"}
	},
	//HUMAN CLASSES
	{
		{"lauren",		"lauren"},
		{"enforcer",	"enforcer"},
		{"commander",	"commander"}
	}
};

static const char *tactical_team_names[num_tactical_teams] =
{
	"ALIENS",
	"HUMANS"
};

static menuframework_s	s_tactical_menus[num_tactical_teams];
static menuframework_s	s_tactical_columns[num_tactical_teams][num_tactical_classes];
static menuaction_s 	s_tactical_skin_actions[num_tactical_teams][num_tactical_classes];
static menumodel_s 		s_tactical_skin_previews[num_tactical_teams][num_tactical_classes];

static void TacticalJoinFunc ( void *item )
{
	menuaction_s *self;
	char buffer[128];
	
	self = (menuaction_s*)item;
	
	cl.tactical = true;
	
	//set skin and model
	Com_sprintf (buffer, sizeof(buffer), "%s/default", self->generic.localstrings[0]);
	Cvar_Set ("skin", buffer);
	
	//join server
	Com_sprintf (buffer, sizeof(buffer), "connect %s\n", NET_AdrToString (mservers[serverindex].local_server_netadr));
	Cbuf_AddText (buffer);
	M_ForceMenuOff ();
}

void Tactical_MenuInit( void )
{
	extern cvar_t *name;
	float scale;
	int i, j;
	
	scale = (float)(viddef.height)/600;
	
	s_tactical_screen.nitems = 0;
	
	for (i = 0; i < num_tactical_teams; i++)
	{
		setup_window (s_tactical_screen, s_tactical_menus[i], tactical_team_names[i]);
		s_tactical_menus[i].horizontal = true;
		
		for (j = 0; j < num_tactical_classes; j++)
		{
			s_tactical_columns[i][j].generic.type = MTYPE_SUBMENU;
			s_tactical_columns[i][j].nitems = 0;
			s_tactical_columns[i][j].navagable = true;
			Menu_AddItem (&s_tactical_menus[i], &s_tactical_columns[i][j]);

			s_tactical_skin_previews[i][j].generic.type = MTYPE_NOT_INTERACTIVE;
			s_tactical_skin_previews[i][j].generic.namesizecallback = PlayerModelSizeFunc;
			s_tactical_skin_previews[i][j].generic.namedraw = PlayerModelDrawFunc;
			s_tactical_skin_previews[i][j].name = tactical_skin_names[i][j][1];
			s_tactical_skin_previews[i][j].skin = "default";
			s_tactical_skin_previews[i][j].h = 14;
			s_tactical_skin_previews[i][j].w = 13;
			Menu_AddItem (&s_tactical_columns[i][j], &s_tactical_skin_previews[i][j]);
			
			s_tactical_skin_actions[i][j].generic.type = MTYPE_ACTION;
			s_tactical_skin_actions[i][j].generic.name = tactical_skin_names[i][j][0];
			s_tactical_skin_actions[i][j].generic.localstrings[0] = tactical_skin_names[i][j][1];
			s_tactical_skin_actions[i][j].generic.callback = TacticalJoinFunc;
			Menu_AddItem (&s_tactical_columns[i][j], &s_tactical_skin_actions[i][j]);
		}
	}

	Menu_AutoArrange (&s_tactical_screen);

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
}

screen_boilerplate (Tactical, s_tactical_screen);


/*
=======================================================================

QUIT MENU

=======================================================================
*/

static menuframework_s	s_quit_screen;
static menuframework_s	s_quit_menu;

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
	s_quit_screen.nitems = 0;
	
	setup_window (s_quit_screen, s_quit_menu, "EXIT ALIEN ARENA");

	add_text (s_quit_menu, "Are you sure?", 0);
	add_action (s_quit_menu, "yes", quitActionYes);
	add_action (s_quit_menu, "no", quitActionNo);
	
	Menu_AutoArrange (&s_quit_screen);
	Menu_Center (&s_quit_screen);

	Menu_SetStatusBar( &s_quit_menu, NULL );
}

screen_boilerplate (Quit, s_quit_screen);

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
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
}


/*
=================================
Menu Mouse Cursor
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
	float		range;
	FNT_font_t	font;
	
	font = FNT_AutoGet( CL_menuFont );

	range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );

	if ( range < 0)
		range = 0;
	if ( range > 1)
		range = 1;

	return ( int )( font->width + RCOLUMN_OFFSET + (SLIDER_RANGE) * font->width * range );
}

int newSliderValueForX (int x, menuslider_s *s)
{
	float 		newValue;
	int 		newValueInt;
	FNT_font_t	font;
	int			pos;
	
	font = FNT_AutoGet( CL_menuFont );
	
	pos = x - (font->width + RCOLUMN_OFFSET + CHASELINK(s->generic.x)) - Menu_GetCtrX(*(s->generic.parent));

	newValue = ((float)pos)/((SLIDER_RANGE-1)*font->width);
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
	menuslider_s *slider = ( menuslider_s * ) menuitem;

	slider->curvalue = newSliderValueForX(cursor.x, slider);
	Slider_CheckSlide ( slider );
}

void Menu_ClickSlideItem (menuframework_s *menu, void *menuitem)
{
	int min, max;
	menuslider_s *slider = ( menuslider_s * ) menuitem;

	min = Item_GetX (*slider) + Slider_CursorPositionX(slider) - 4;
	max = Item_GetX (*slider) + Slider_CursorPositionX(slider) + 4;

	if (cursor.x < min)
		Menu_SlideItem( menu, -1 );
	if (cursor.x > max)
		Menu_SlideItem( menu, 1 );
}


void _M_Think_MouseCursor (void)
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
	
	if (cursor.buttondown[MOUSEBUTTON1] && !cursor.suppress_drag)
	{
		if (cursor.click_menuitem != NULL)
		{
			cursor.menuitem = cursor.click_menuitem;
			cursor.menuitemtype = cursor.click_menuitemtype;
		}
		else if (cursor.menuitem)
		{
			cursor.click_menuitem = cursor.menuitem;
			cursor.click_menuitemtype = cursor.menuitemtype;
		}
	}
	else
		cursor.click_menuitem = NULL;
	
	if (!cursor.buttondown[MOUSEBUTTON1])
		cursor.suppress_drag = false;
	else if (!cursor.menuitem)
		cursor.suppress_drag = true;
	
	if (cursor.suppress_drag || cursor.menuitem == NULL)
		return;
	
	//MOUSE1
	if (cursor.buttondown[MOUSEBUTTON1])
	{
		if (cursor.menuitemtype == MTYPE_SLIDER)
		{
			Menu_DragSlideItem(m, cursor.menuitem);
		}
		else if (!cursor.buttonused[MOUSEBUTTON1])
		{
			if (cursor.menuitemtype == MTYPE_SPINCONTROL)
				Menu_SlideItem( m, 1 );
			else
				Menu_MouseSelectItem( cursor.menuitem );
			
			// rate-limit repeat clicking
			cursor.buttonused[MOUSEBUTTON1] = true;
			sound = menu_move_sound;
		}
	}
	//MOUSE2
	else if (cursor.buttondown[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2] && !cursor.buttonused[MOUSEBUTTON2])
	{
		if (cursor.menuitemtype == MTYPE_SPINCONTROL)
			Menu_SlideItem( m, -1 );
		else if (cursor.menuitemtype == MTYPE_SLIDER)
			Menu_ClickSlideItem(m, cursor.menuitem);
		else
			return;

		// rate-limit repeat clicking
		cursor.buttonused[MOUSEBUTTON2] = true;
		sound = menu_move_sound;
	}

	if ( sound )
		S_StartLocalSound( sound );
}

void M_Draw_Cursor (void)
{
	Draw_Pic (cursor.x, cursor.y, "m_mouse_cursor");
}


// M_Transition - responsible for animating the transitions as menus are added
// and removed from the screen. Actually, this function just performs an 
// interpolation between 0 and target, returning a number that should be used
// next time for progress. you determine roughly how many pixels a menu is
// going to slide, and in which direction. Your target is that number of
// pixels, positive or negative depending on the direction. Call this function
// repeatedly with your target, and it will return a series of pixel offsets 
// that can be used in your animation.
int M_Transition (int progress, int target)
{
	int increment = 0; // always positive
	
	// Determine the movement amount this frame. Make it nice and fast,
	// because while slow might look cool, it's also an inconvenience.
	if (target != progress)
	{
		static float frametime_accum = 0;
		
		// The animation speeds up as it gets further from the starting point
		// and slows down (although half as much) as it approaches the ending
		// point.
		increment = min(	abs(target-progress)/2,
							abs(progress) )*40;
		
		// Clamp the animation speed at a minimum so it won't freeze due to
		// rounding errors or take too long at either end.
		increment = max (increment, abs(target)/10);

		// Scale the animation by frame time so its speed is independent of 
		// framerate. At very high framerates, each individual frame might be
		// too small a time to result in an integer amount of movement. So we
		// just add frames together until we do get some movement.
		frametime_accum += cls.frametime;
		increment *= frametime_accum;

		if (increment > 0)
			frametime_accum = 0;
		else
			return progress; // no movement, better luck next time.
	}
	
	if (target > 0)
	{
		// increasing
		progress += increment;
		progress = min (progress, target); // make sure we don't overshoot
	}
	else if (target < 0)
	{
		// decreasing
		progress -= increment;
		progress = max (progress, target); // make sure we don't overshoot
	}
	
	return progress;
}
	


// M_ThinkRedirect - juggles various tasks for all the menus on screen. 
// FIXME: mushing three different functions into one with an argument to 
// determine which task the function should actually perform is kind of an
// anti-pattern.

enum thinkredirect_action
{
	think_draw,
	think_key,
	think_mouse
};

void M_ThinkRedirect (enum thinkredirect_action action, int key)
{
	int base_offset, shove_offset;
	int depth_max;
	int i;
	const char *s;
	int old_menudepth = m_menudepth;
		
	if (cls.key_dest != key_menu)
		return;
	
	if (action == think_key && key == K_ESCAPE)
	{
		if ((s = m_keyfunc (m_layers[m_menudepth].screen, key)))
			S_StartLocalSound (s);
		return;
	}
	
	if (action == think_draw)
		refreshCursorMenu();
	
	depth_max = m_menudepth;
	base_offset = 0;
	shove_offset = 0;
	for (i = 1; i < depth_max; i++)
	{
		if (m_layers[i+1].end_x-base_offset > viddef.width)
			base_offset = m_layers[i].end_x-Menu_NaturalWidth(*m_layers[i].screen);
	}
	if (global_menu_xoffset_target > global_menu_xoffset_progress)
	{
		if (m_layers[m_menudepth-1].start_x < base_offset)
			base_offset = m_layers[m_menudepth-1].start_x;
		shove_offset = m_layers[m_menudepth].start_x-viddef.width+global_menu_xoffset_progress;
		if (shove_offset > base_offset)
		{
			if (m_layers[m_menudepth].end_x-base_offset > viddef.width)
				base_offset = shove_offset;
			else
				shove_offset = base_offset;
		}
	}
	else if (global_menu_xoffset_target < global_menu_xoffset_progress)
	{
		shove_offset = base_offset;
		if (m_layers[m_menudepth+1].end_x-shove_offset > viddef.width)
			shove_offset = m_layers[m_menudepth].end_x-Menu_NaturalWidth(*m_layers[m_menudepth].screen);
		shove_offset += global_menu_xoffset_progress;
		if (shove_offset > base_offset)
			base_offset = shove_offset;
	}
	else if (global_menu_xoffset_target == global_menu_xoffset_progress)
	{
		shove_offset = m_layers[i].end_x-viddef.width;
		if (shove_offset > base_offset)
			base_offset = shove_offset;
		else
			shove_offset = base_offset;
	}
	
	if (global_menu_xoffset_target < global_menu_xoffset_progress)
		depth_max++;
	for (i = 1; i <= depth_max; i++)
	{
		int next_xoffset;
		
		if (i == depth_max)
			base_offset = shove_offset;
		
		global_menu_xoffset = m_layers[i].start_x-base_offset;
		next_xoffset = m_layers[i].end_x-base_offset;
		
		if (global_menu_xoffset < -viddef.width)
			continue;
		
		m_drawfunc = m_layers[i].draw;
		m_keyfunc = m_layers[i].key;
		
		if (i <= old_menudepth)
			m_menudepth = i;
		
		if (action == think_draw && m_layers[i].draw != NULL)
			m_layers[i].draw();
		else if (cursor.x >= global_menu_xoffset && cursor.x <= next_xoffset && i <= old_menudepth)
		{
			if (action == think_key && m_layers[i].key != NULL && (s = m_layers[i].key (m_layers[i].screen, key)))
				S_StartLocalSound( s );
			else if (action == think_mouse)
				_M_Think_MouseCursor ();
		}
		
		if (m_menudepth != i)
			old_menudepth = m_menudepth;
	}
	
	if (action != think_draw)
		return;
	
	global_menu_xoffset_progress = M_Transition (global_menu_xoffset_progress, global_menu_xoffset_target);
	if (global_menu_xoffset_progress == global_menu_xoffset_target)
		global_menu_xoffset_progress = global_menu_xoffset_target = 0;

	// delay playing the enter sound until after the
	// menu has been drawn, to avoid delay while
	// caching images
	if (m_entersound)
	{
		S_StartLocalSound( menu_in_sound );
		m_entersound = false;
	}

	M_Draw_Cursor();

}

// draw all menus on screen
void M_Draw (void)
{
	// draw a black background unless it will cover the console
	if (m_menudepth >= 1)
		Draw_Fill (0, 0, viddef.width, viddef.height, 0);
	M_ThinkRedirect (think_draw, 0);
}

// send key presses to the appropriate menu
void M_Keydown (int key)
{
	if (global_menu_xoffset_target != 0)
		return;
	
	M_ThinkRedirect (think_key, key);
}

// send mouse movement to the appropriate menu
void M_Think_MouseCursor (void)
{
	M_ThinkRedirect (think_mouse, 0);
} 
