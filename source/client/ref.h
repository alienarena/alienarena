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
#ifndef __REF_H
#define __REF_H

#include "../qcommon/qcommon.h"

#define DIV254BY255 (0.9960784313725490196078431372549f)
#define DIV255 (0.003921568627450980392156862745098f)
#define DIV256 (0.00390625f)
#define DIV512 (0.001953125f)

#define	MAX_DLIGHTS			32
#define MAX_STAINS			32
#define	MAX_ENTITIES		128
#define	MAX_PARTICLES		8192
#define	MAX_LIGHTSTYLES		256

#define MAX_FLARES      512

typedef struct
{
	vec3_t origin;
	vec3_t color;
	int size;
	int style;
} flare_t;

typedef struct
{
	vec3_t origin2;
	vec3_t color2;
	int size2;
	int style2;
} flare2_t;


#define POWERSUIT_SCALE		4.0F

#define SHELL_RED_COLOR		0xF2
#define SHELL_GREEN_COLOR	0xD0
#define SHELL_BLUE_COLOR	0xF3

#define SHELL_RG_COLOR		0xDC
//#define SHELL_RB_COLOR		0x86
#define SHELL_RB_COLOR		0x68
#define SHELL_BG_COLOR		0x78

//ROGUE
#define SHELL_DOUBLE_COLOR	0xDF // 223
#define	SHELL_HALF_DAM_COLOR	0x90
#define SHELL_CYAN_COLOR	0x72
//ROGUE

#define SHELL_WHITE_COLOR	0xD7


#define PARTICLE_NONE				0
#define PARTICLE_BLASTER			1
#define PARTICLE_BLOOD				2
#define PARTICLE_GUNSHOT			3
#define PARTICLE_SMOKE				4
#define PARTICLE_BLASTER_MZFLASH	5
#define PARTICLE_LEADER_FIELD		6
#define PARTICLE_DEATH_FIELD		7
#define PARTICLE_EXPLOSION1			8
#define PARTICLE_EXPLOSION2			9
#define PARTICLE_EXPLOSION3			10
#define PARTICLE_EXPLOSION4			11
#define PARTICLE_EXPLOSION5			12
#define PARTICLE_EXPLOSION6			13
#define PARTICLE_EXPLOSION7			14
#define PARTICLE_FIREBALL			15
#define PARTICLE_BLUE_MZFLASH		16
#define PARTICLE_BUBBLE				17
#define PARTICLE_SHOT				18
#define PARTICLE_HIT				19
#define PARTICLE_SPARK				20
#define PARTICLE_SAY_ICON			21
#define PARTICLE_FLARE				22

#define RDF_BLOOM         4      //BLOOMS

typedef struct entity_s
{
	struct model_s		*model;			// opaque type outside refresh
	float				angles[3];

	/*
	** most recent data
	*/
	float				origin[3];		// also used as RF_BEAM's "from"
	int					frame;			// also used as RF_BEAM's diameter

	/*
	** previous data for lerping
	*/
	float				oldorigin[3];	// also used as RF_BEAM's "to"
	int					oldframe;

	/*
	** misc
	*/
	float	backlerp;				// 0.0 = current, 1.0 = old
	int		skinnum;				// also used as RF_BEAM's palette index

	int		lightstyle;				// for flashing entities
	float	alpha;					// ignore if RF_TRANSLUCENT isn't set

	struct image_s	*skin;			// NULL for inline skin
	int		flags;

	struct rscript_t *script;

} entity_t;

#define ENTITY_FLAGS  68

typedef struct
{
	vec3_t	origin;
	vec3_t	color;
	float	intensity;
	int		team;				//so we can add team lights like UT
} dlight_t;

typedef struct
{
	float	strength;
	vec3_t	direction;
	vec3_t	color;
} m_dlight_t;

typedef enum 
{
	stain_add,
	stain_modulate,
	stain_subtract
} staintype_t;

typedef struct
{
	vec3_t	origin;
	vec3_t	color;
	float	alpha;
	float	intensity;
	staintype_t type;
} dstain_t;

typedef struct
{
	vec3_t	origin;
	vec3_t	color;
	float	alpha;
	float	intensity;
} stain_t;

typedef struct
{
	vec3_t	origin;
	int		color;
	float	alpha;
	int		type;
	int		texnum;
	int		blendsrc;
	int		blenddst;
	float	scale;
} particle_t;

typedef struct
{
	float		rgb[3];			// 0.0 - 2.0
	float		white;			// highest of rgb
} lightstyle_t;

typedef struct
{
	int			x, y, width, height;// in virtual screen coordinates
	float		fov_x, fov_y;
	float		vieworg[3];
	float		viewangles[3];
	float		blend[4];			// rgba 0-1 full screen blend
	float		time;				// time is uesed to auto animate
	int			rdflags;			// RDF_UNDERWATER, etc

	byte		*areabits;			// if not NULL, only areas with set bits will be drawn

	lightstyle_t	*lightstyles;	// [MAX_LIGHTSTYLES]

	int			num_entities;
	entity_t	*entities;

	int			num_stains;
	stain_t		*stains;

	int			num_dlights;
	dlight_t	*dlights;

	int			num_particles;
	particle_t	*particles;

	int          num_flares;
    flare_t      *flares;

} refdef_t;

// Knightmare- added Psychospaz's menu cursor
//cursor - psychospaz
#define MENU_CURSOR_BUTTON_MAX 2

#define MENUITEM_ACTION		1
#define MENUITEM_ROTATE		2
#define MENUITEM_SLIDER		3
#define MENUITEM_TEXT		4

typedef struct
{
	//only 2 buttons for menus
	float		buttontime[MENU_CURSOR_BUTTON_MAX];
	int			buttonclicks[MENU_CURSOR_BUTTON_MAX];
	int			buttonused[MENU_CURSOR_BUTTON_MAX];
	qboolean	buttondown[MENU_CURSOR_BUTTON_MAX];

	qboolean	mouseaction;

	//this is the active item that cursor is on.
	int			menuitemtype;
	void		*menuitem;
	void		*menu;

	//coords
	int		x;
	int		y;

	int		oldx;
	int		oldy;
} cursor_t;

void	Draw_GetPicSize (int *w, int *h, char *name);
void	Draw_Pic (int x, int y, char *name);
void	Draw_StretchPic (int x, int y, int w, int h, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_ColorChar (int x, int y, int num, vec4_t color);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_BeginFrame( float camera_separation );
void	R_SwapBuffers( int );
void	R_SetPalette ( const unsigned char *palette);

struct model_s	*R_RegisterModel (char *name);
struct image_s	*R_RegisterSkin (char *name);
struct image_s	*R_RegisterPic (char *name);

void	R_SetSky (char *name, float rotate, vec3_t axis);

void	R_BeginRegistration (char *map);
void	R_EndRegistration (void);

void	R_RenderFrame (refdef_t *fd);
void	R_EndFrame (void);

int		R_Init( void *hinstance, void *hWnd );
void	R_Shutdown (void);

void	R_AppActivate( qboolean active );

#endif // __REF_H
