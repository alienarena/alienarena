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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

/*

  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  full screen image for quit and victory

  end of unit intermissions

  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"
#include "qmenu.h"

float		scr_con_current;	// aproaches scr_conlines at scr_conspeed
float		scr_conlines;		// 0.0 to 1.0 lines of console to display

qboolean	scr_initialized;		// ready to draw

int			scr_draw_loading;

vrect_t		scr_vrect;		// position of render window on screen

cvar_t		*scr_conspeed;
cvar_t		*scr_centertime;
cvar_t		*scr_showturtle;
cvar_t		*scr_showpause;
cvar_t		*scr_printspeed;

cvar_t		*scr_netgraph;
cvar_t		*scr_timegraph;
cvar_t		*scr_debuggraph;
cvar_t		*scr_graphheight;
cvar_t		*scr_graphscale;
cvar_t		*scr_graphshift;

cvar_t		*scr_consize;

cvar_t		*cl_drawfps;
cvar_t		*cl_drawtimer;

cvar_t		*cl_drawtime;

typedef struct
{
	int		x1, y1, x2, y2;
} dirty_t;

dirty_t		scr_dirty, scr_old_dirty[2];

char		crosshair_pic[MAX_QPATH];
int			crosshair_width, crosshair_height;

void SCR_TimeRefresh_f (void);
void SCR_Loading_f (void);

extern void R_VCFreeFrame(void);

/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

/*
==============
CL_AddNetgraph

A new packet was just parsed
==============
*/
void CL_AddNetgraph (void)
{
	int		i;
	int		in;
	int		ping;

	// if using the debuggraph for something else, don't
	// add the net lines
	if (scr_debuggraph->value || scr_timegraph->value)
		return;

	for (i=0 ; i<cls.netchan.dropped ; i++)
		SCR_DebugGraph (30, 0x40);

	for (i=0 ; i<cl.surpressCount ; i++)
		SCR_DebugGraph (30, 0xdf);

	// see what the latency was on this packet
	in = cls.netchan.incoming_acknowledged & (CMD_BACKUP-1);
	ping = cls.realtime - cl.cmd_time[in];
	ping /= 30;
	if (ping > 30)
		ping = 30;
	SCR_DebugGraph (ping, 0xd0);
}


typedef struct
{
	float	value;
	int		color;
} graphsamp_t;

static	int			current;
static	graphsamp_t	values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color)
{
	values[current&1023].value = value;
	values[current&1023].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void)
{
	int		a, x, y, w, i, h;
	float	v;
	int		color;

	//
	// draw the graph
	//
	w = scr_vrect.width;

	x = scr_vrect.x;
	y = scr_vrect.y+scr_vrect.height;
	Draw_Fill (x, y-scr_graphheight->value,
		w, scr_graphheight->value, 8);

	for (a=0 ; a<w ; a++)
	{
		i = (current-1-a+1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v*scr_graphscale->value + scr_graphshift->value;

		if (v < 0)
			v += scr_graphheight->value * (1+(int)(-v/scr_graphheight->value));
		h = (int)v % (int)scr_graphheight->value;
		Draw_Fill (x+w-1-a, y - h, 1,	h, color);
	}
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_center;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	char	*s;

	if(!cl_centerprint->value)
		return;

	strncpy (scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = cl.time;

	// count the number of lines for centering
	scr_center_lines = 1;
	s = str;
	while (*s)
	{
		if (*s == '\n')
			scr_center_lines++;
		s++;
	}
}


void SCR_DrawCenterString (void)
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;
	int		charscale;

// the finale prints the characters one at a time
	remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	charscale = (float)(viddef.height)*8/400;; //make these a litter larger
	if(charscale < 8)
		charscale = 8;

	if (scr_center_lines <= 4)
		y = viddef.height*0.35;
	else
		y = 48;

	do
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (viddef.width - l*charscale)/2;
		SCR_AddDirtyPoint (x, y);
		for (j=0 ; j<l ; j++, x+=charscale)
		{
			Draw_ScaledChar (x, y, start[j], charscale, false);
			if (!remaining--)
				return;
		}
		SCR_AddDirtyPoint (x, y+charscale);

		y += charscale;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	scr_centertime_off -= cls.frametime;

	if (scr_centertime_off <= 0)
		return;

	SCR_DrawCenterString ();
}

/*
===============================================================================

IRC PRINTING

  we'll have 5 lines stored.
  once full, start bumping lines, as in copying line 1 into line 0, and so forth

===============================================================================
*/

char		scr_IRCstring[5][1024];
float		scr_IRCtime_start;	// for slow victory printing
float		scr_IRCtime_off;
int			scr_IRC_lines = -1;

/*
==============
SCR_IRCPrint

Called for IRC messages
==============
*/

void IRC_BumpMsg(void)
{
	int i;

	for(i = 0; i < scr_IRC_lines; i++)
		strcpy(scr_IRCstring[i], scr_IRCstring[i+1]);
}

void SCR_IRCPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];

	va_start (argptr,fmt);
	vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	scr_IRCtime_off = 15;
	scr_IRCtime_start = cl.time;

	// count the number of lines
	if(scr_IRC_lines < 4)
		scr_IRC_lines += 1;
	else
		IRC_BumpMsg();

	strncpy (scr_IRCstring[scr_IRC_lines], msg, sizeof(scr_IRCstring[scr_IRC_lines])-1);
}

extern vec4_t	Color_Table[8];

void SCR_DrawColorString ( int x, int y, const char *str )
{
	int		num;
	vec4_t	scolor;
	int	charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;

	scolor[0] = 0;
	scolor[1] = 1;
	scolor[2] = 0;
	scolor[3] = 1;

	while (*str) {
		if ( Q_IsColorString( str ) ) {
			VectorCopy ( Color_Table[ColorIndex(str[1])], scolor );
			str += 2;
			continue;
		}

		Draw_ScaledColorChar (x, y, *str, scolor, charscale, false); //this is only ever used for names.

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
void SCR_DrawIRCString (int lineNum)
{
	int		x, y;
	int		charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;

	y = charscale*5 + lineNum*charscale;
	x = 0;

	SCR_AddDirtyPoint (x, y);
	SCR_DrawColorString (x, y, scr_IRCstring[lineNum]);
	SCR_AddDirtyPoint (x, y+charscale);

}

void SCR_CheckDrawIRCString (void)
{
	int i;

	scr_IRCtime_off -= cls.frametime;

	if (scr_IRCtime_off <= 0)
		return;

	for(i = 0; i <= scr_IRC_lines; i++)
		SCR_DrawIRCString (i);
}

//=============================================================================

/*
=================
SCR_CalcVrect

Sets scr_vrect, the coordinates of the rendered window
=================
*/
static void SCR_CalcVrect (void)
{

	scr_vrect.width = viddef.width;
	scr_vrect.width &= ~7;

	scr_vrect.height = viddef.height;
	scr_vrect.height &= ~1;

	scr_vrect.x = (viddef.width - scr_vrect.width)/2;
	scr_vrect.y = (viddef.height - scr_vrect.height)/2;
}

/*
=================
SCR_Sky_f

Set a specific sky and rotation speed
=================
*/
void SCR_Sky_f (void)
{
	float	rotate;
	vec3_t	axis;

	if (Cmd_Argc() < 2)
	{
		Com_Printf ("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}
	if (Cmd_Argc() > 2)
		rotate = atof(Cmd_Argv(2));
	else
		rotate = 0;
	if (Cmd_Argc() == 6)
	{
		axis[0] = atof(Cmd_Argv(3));
		axis[1] = atof(Cmd_Argv(4));
		axis[2] = atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	R_SetSky (Cmd_Argv(1), rotate, axis);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	scr_conspeed = Cvar_Get ("scr_conspeed", "3", CVAR_ARCHIVE);
	scr_consize = Cvar_Get ("scr_consize", "0.5", CVAR_ARCHIVE);
	scr_showturtle = Cvar_Get ("scr_showturtle", "0", 0);
	scr_showpause = Cvar_Get ("scr_showpause", "1", 0);
	scr_centertime = Cvar_Get ("scr_centertime", "2.5", CVAR_ARCHIVE);
	scr_printspeed = Cvar_Get ("scr_printspeed", "8", CVAR_ARCHIVE);
	scr_netgraph = Cvar_Get ("netgraph", "0", 0);
	scr_timegraph = Cvar_Get ("timegraph", "0", 0);
	scr_debuggraph = Cvar_Get ("debuggraph", "0", 0);
	scr_graphheight = Cvar_Get ("graphheight", "32", 0);
	scr_graphscale = Cvar_Get ("graphscale", "1", 0);
	scr_graphshift = Cvar_Get ("graphshift", "0", 0);

	cl_drawfps = Cvar_Get ("cl_drawfps", "0", CVAR_ARCHIVE);
	cl_drawtimer = Cvar_Get("cl_drawtimer", "0", CVAR_ARCHIVE);

//
// register our commands
//
	Cmd_AddCommand ("timerefresh",SCR_TimeRefresh_f);
	Cmd_AddCommand ("loading",SCR_Loading_f);
	Cmd_AddCommand ("sky",SCR_Sky_f);

	scr_initialized = true;
}


/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged
		< CMD_BACKUP-1)
		return;

	Draw_Pic (scr_vrect.x+64, scr_vrect.y, "net");
}

void SCR_DrawAlertMessagePicture (char *name, qboolean center)
{
	float ratio;
	int scale;
	int w, h;

	scale = viddef.width/MENU_STATIC_WIDTH;

	Draw_GetPicSize (&w, &h, name );
	if (w)
	{
		ratio = 35.0/(float)h;
		h = 35;
		w *= ratio;
	}
	else
		return;

	if(center)
		Draw_StretchPic (
			viddef.width / 2 - w*scale/2,
			viddef.height/ 2 - h*scale/2,
			scale*w, scale*h, name);
	else
		Draw_StretchPic (
			viddef.width / 2 - w*scale/2,
			scale * 100,
			scale*w, scale*h, name);
}

void SCR_DrawPause (void)
{

	if (!scr_showpause->value)		// turn off for screenshots
		return;

	if (!cl_paused->value)
		return;

	if (cls.key_dest == key_menu)
		return;

	SCR_DrawAlertMessagePicture("pause", true);
}

/*
==============
SCR_DrawLoading
==============
*/


void SCR_DrawRotatingIcon( void )
{
	extern float CalcFov( float fov_x, float w, float h );
	refdef_t refdef;
	static float yaw;
	entity_t entity;

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.width = viddef.width;
	refdef.height = viddef.height;
	refdef.x = 0;
	refdef.y = 0;
	refdef.fov_x = 90;
	refdef.fov_y = CalcFov( refdef.fov_x, refdef.width, refdef.height );
	refdef.time = cls.realtime*0.001;

	memset( &entity, 0, sizeof( entity ) );

	yaw += cls.frametime*50;
	if (yaw > 360)
		yaw = 0;

	entity.model = R_RegisterModel( "models/objects/icon/tris.md2" );

	entity.flags = RF_FULLBRIGHT | RF_MENUMODEL;
	entity.origin[0] = 80;
	entity.origin[1] = 30;
	entity.origin[2] = -5;

	VectorCopy( entity.origin, entity.oldorigin );

	entity.frame = 0;
	entity.oldframe = 0;
	entity.backlerp = 0.0;
	entity.angles[1] = (int)yaw;

	refdef.areabits = 0;
	refdef.num_entities = 1;

	refdef.entities = &entity;
	refdef.lightstyles = 0;
	refdef.rdflags = RDF_NOWORLDMODEL;

	refdef.height += 4;

	R_RenderFrame( &refdef );

	free(entity.model);
}

void SCR_DrawLoadingBar (int percent, int scale)
{
	float hudscale;

	hudscale = (float)(viddef.height)/600;
	if(hudscale < 1)
		hudscale = 1;

	if (R_RegisterPic("bar_background") && R_RegisterPic("bar_loading"))
	{
		Draw_StretchPic (
			viddef.width/2-scale*15 + 1*hudscale,viddef.height/2 + scale*5+1*hudscale,
			scale*30-2*hudscale, scale*10-2*hudscale, "bar_background");
		Draw_StretchPic (
			viddef.width/2-scale*15 + 1*hudscale,viddef.height/2 + scale*5+8*hudscale,
			(scale*30-2*hudscale)*percent/100, scale*2-2*hudscale, "bar_loading");
	}
	else
	{
		Draw_Fill (
			viddef.width/2-scale*15 + 1,viddef.height/2 + scale*5+1,
			scale*30-2, scale*2-2, 3);
		Draw_Fill (
			viddef.width/2-scale*15 + 1,viddef.height/2 + scale*5+1,
			(scale*30-2)*percent/100, scale*2-2, 7);
	}
}
extern void Menu_DrawString( int x, int y, const char *string);
int stringLen (char *string)
{
	return strlen(string);
}
void SCR_DrawLoading (void)
{
	char	mapfile[32];
	qboolean isMap = false;
	int		font_size, kb;
	float	hudscale;

	if (!scr_draw_loading)
		return;
	scr_draw_loading = false;

	font_size = (float)(viddef.height)*8/600;
	if(font_size < 8)
		font_size = 8;

	hudscale = (float)(viddef.height)/600;
	if(hudscale < 1)
		hudscale = 1;

	//loading a map...
	if (loadingMessage && cl.configstrings[CS_MODELS+1][0])
	{
		strcpy (mapfile, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
		mapfile[strlen(mapfile)-4] = 0;		// cut off ".bsp"

		if(map_pic_loaded)
			Draw_StretchPic (0, 0, viddef.width, viddef.height, va("/levelshots/%s.pcx", mapfile));
		else
			Draw_Fill (0, 0, viddef.width, viddef.height, 0);

		isMap = true;

	}
	else
		Draw_Fill (0, 0, viddef.width, viddef.height, 0);

#if 0
	// no m_background pic, but a pic here over-writes the levelshot
	Draw_StretchPic (0, 0, viddef.width, viddef.height, "m_background");
#endif

	//loading message stuff...
	if (isMap)
	{
		char *mapmsg;

		mapmsg = va("Loading Map [%s]", mapfile);
		DrawString(
			viddef.width/2 - font_size/2*stringLen(mapmsg),
			viddef.height/2 - font_size*5,
			mapmsg);

		mapmsg = va("[%s]", cl.configstrings[CS_NAME]);
		DrawString(
			viddef.width/2 - font_size/2*stringLen(mapmsg),
			viddef.height/2 - font_size*4,
			mapmsg);

		DrawString(
			viddef.width/2 - font_size*15,
			viddef.height/2 - font_size*1,
			loadingMessages[0]);
		DrawString(
			viddef.width/2 - font_size*15,
			viddef.height/2 - font_size*0,
			loadingMessages[1]);
		DrawString(
			viddef.width/2 - font_size*15,
			viddef.height/2 + font_size*1,
			loadingMessages[2]);
		DrawString(
			viddef.width/2 - font_size*15,
			viddef.height/2 + font_size*2,
			loadingMessages[3]);
		DrawString(
			viddef.width/2 - font_size*15,
			viddef.height/2 + font_size*3,
			loadingMessages[4]);

		//check for instance of icons we would like to show in loading process, ala q3
		if(rocketlauncher) {
			Draw_ScaledPic((int)(viddef.width/3), (int)(viddef.height/3.2), hudscale, "w_rlauncher");
			if(!rocketlauncher_drawn){
				rocketlauncher_drawn = 40*hudscale;
			}
		}
		if(chaingun) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn, (int)(viddef.height/3.2), hudscale, "w_sshotgun");
			if(!chaingun_drawn) {
				chaingun_drawn = 40*hudscale;
			}
		}
		if(smartgun) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn, (int)(viddef.height/3.2), hudscale, "w_shotgun");
			if(!smartgun_drawn) {
				smartgun_drawn = 40*hudscale;
			}
		}
		if(beamgun) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn, (int)(viddef.height/3.2), hudscale, "w_railgun");
			if(!beamgun_drawn) {
				beamgun_drawn = 40*hudscale;
			}
		}
		if(flamethrower) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn
				, (int)(viddef.height/3.2), hudscale, "w_chaingun");
			if(!flamethrower_drawn) {
				flamethrower_drawn = 40*hudscale;
			}
		}
		if(disruptor) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn
				, (int)(viddef.height/3.2), hudscale, "w_hyperblaster");
			if(!disruptor_drawn) {
				disruptor_drawn = 40*hudscale;
			}
		}
		if(vaporizer) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn
				, (int)(viddef.height/3.2), hudscale, "w_bfg");
			if(!vaporizer_drawn) {
				vaporizer_drawn = 40*hudscale;
			}
		}
		if(quad) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn+vaporizer_drawn
				, (int)(viddef.height/3.2), hudscale, "p_quad");
			if(!quad_drawn) {
				quad_drawn = 40*hudscale;
			}
		}
		if(haste) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn+vaporizer_drawn+quad_drawn
				, (int)(viddef.height/3.2), hudscale, "p_haste");
			if(!haste_drawn) {
				haste_drawn = 40*hudscale;
			}
		}
		if(sproing) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn+vaporizer_drawn+quad_drawn+haste_drawn
				, (int)(viddef.height/3.2), hudscale, "p_sproing");
			if(!sproing_drawn) {
				sproing_drawn = 40*hudscale;
			}
		}
		if(inv) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn+vaporizer_drawn+quad_drawn+haste_drawn+sproing_drawn
				, (int)(viddef.height/3.2), hudscale, "p_invulnerability");
			if(!inv_drawn) {
				inv_drawn = 40*hudscale;
			}
		}
		if(adren) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn+vaporizer_drawn+quad_drawn+haste_drawn+sproing_drawn+inv_drawn
				, (int)(viddef.height/3.2), hudscale, "p_adrenaline");
		}
	}
	else
	{
		char *msg = va("Awaiting Connection...");

		//draw centered
		DrawString(
				viddef.width/2 - font_size/2*stringLen(msg),
				viddef.height/2 - font_size/2,
				msg);

	}

	// Add Download info stuff...
	if (cls.download && (kb = (int)ftell(cls.download)))
	{
		if ( cls.key_dest != key_console ) //drop the console because of CURL's readout
		{
			Con_ToggleConsole_f();
		}
	}
	else if (isMap) //loading bar...
	{
		//to do - fix
		//SCR_DrawRotatingIcon();

		SCR_DrawLoadingBar(loadingPercent, font_size);

		DrawString(
			viddef.width/2 - font_size*3,
			viddef.height/2 + (int)(font_size*6.3),
			va("%3d%%", (int)loadingPercent));
	}
}

//=============================================================================

/*
==================
SCR_RunConsole

Scroll it up or down
==================
*/
void SCR_RunConsole (void)
{
// decide on the height of the console
	if (cls.key_dest == key_console)
		scr_conlines = scr_consize->value;
	else
		scr_conlines = 0;				// none visible

	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed->value*cls.frametime;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed->value*cls.frametime;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

}

/*
==================
SCR_DrawConsole
==================
*/
float sendBubbleNow;
void SCR_DrawConsole (void)
{
	Con_CheckResize ();

	if (cls.state == ca_disconnected || cls.state == ca_connecting)
	{	// forced full screen console
		Con_DrawConsole (scr_consize->value);
		return;
	}

	if (cls.state != ca_active || !cl.refresh_prepped)
	{	// connected, but can't render
		Con_DrawConsole (scr_consize->value);
		Draw_Fill (0, viddef.height/2, viddef.width, viddef.height/2, 0);
		return;
	}

	if (scr_con_current)
	{
		Con_DrawConsole (scr_con_current);
	}
	else
	{
		if (cls.key_dest == key_game || cls.key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}

	//draw chat bubble
	if(cls.key_dest == key_message || scr_con_current) {
		//only send this every couple of seconds no need to flood
		sendBubbleNow += cls.frametime;
		if(sendBubbleNow >= 1) {
			Cbuf_AddText("chatbubble\n");
			Cbuf_Execute ();
			sendBubbleNow = 0;
		}
	}
}

//=============================================================================

/*
================
SCR_BeginLoadingPlaque
================
*/
qboolean needLoadingPlaque (void)
{
	if (!cls.disable_screen || !scr_draw_loading)
		return true;
	return false;
}
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds ();
	cl.sound_prepped = false;		// don't play ambients

	scr_draw_loading = 1;

	SCR_UpdateScreen ();
	cls.disable_screen = Sys_Milliseconds ();
	cls.disable_servercount = cl.servercount;

}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void)
{
	cls.disable_screen = 0;
	scr_draw_loading = 0;
	Con_ClearNotify ();

	Cvar_Set ("paused", "0");
}

/*
================
SCR_Loading_f
================
*/
void SCR_Loading_f (void)
{
	SCR_BeginLoadingPlaque ();
}

int entitycmpfnc( const entity_t *a, const entity_t *b )
{
	/*
	** all other models are sorted by model then skin
	*/
	if ( a->model == b->model )
	{
		return ( ( ptrdiff_t ) a->skin - ( ptrdiff_t ) b->skin );
	}
	else
	{
		return ( ( ptrdiff_t ) a->model - ( ptrdiff_t ) b->model );
	}
}


/*
================
SCR_TimeRefresh_f
================
*/
void SCR_TimeRefresh_f (void)
{
	int		i;
	int		start, stop;
	float	time;

	if ( cls.state != ca_active )
		return;

	start = Sys_Milliseconds ();

	if (Cmd_Argc() == 2)
	{	// run without page flipping
		R_BeginFrame( 0 );
		for (i=0 ; i<128 ; i++)
		{
			cl.refdef.viewangles[1] = i/128.0*360.0;
			R_RenderFrame (&cl.refdef);
		}
		R_EndFrame();
	}
	else
	{
		for (i=0 ; i<128 ; i++)
		{
			cl.refdef.viewangles[1] = i/128.0*360.0;

			R_BeginFrame( 0 );
			R_RenderFrame (&cl.refdef);
			R_EndFrame();
		}
	}

	stop = Sys_Milliseconds ();
	time = (stop-start)/1000.0;
	Com_Printf ("%f seconds (%f fps)\n", time, 128/time);
}

/*
=================
SCR_AddDirtyPoint
=================
*/
void SCR_AddDirtyPoint (int x, int y)
{
	if (x < scr_dirty.x1)
		scr_dirty.x1 = x;
	if (x > scr_dirty.x2)
		scr_dirty.x2 = x;
	if (y < scr_dirty.y1)
		scr_dirty.y1 = y;
	if (y > scr_dirty.y2)
		scr_dirty.y2 = y;
}

void SCR_DirtyScreen (void)
{
	SCR_AddDirtyPoint (0, 0);
	SCR_AddDirtyPoint (viddef.width-1, viddef.height-1);
}

//===============================================================


#define STAT_MINUS		10	// num frame for '-' stats digit
char		*sb_nums[2][11] =
{
	{"num_0", "num_1", "num_2", "num_3", "num_4", "num_5",
	"num_6", "num_7", "num_8", "num_9", "num_minus"},
	{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5",
	"anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
};

#define	ICON_WIDTH	24
#define	ICON_HEIGHT	24
#define	CHAR_WIDTH	16
#define	ICON_SPACE	8



/*
================
SizeHUDString

Allow embedded \n in the string
================
*/
void SizeHUDString (char *string, int *w, int *h)
{
	int		lines, width, current;
	int		charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;

	lines = 1;
	width = 0;

	current = 0;
	while (*string)
	{
		if (*string == '\n')
		{
			lines++;
			current = 0;
		}
		else
		{
			current++;
			if (current > width)
				width = current;
		}
		string++;
	}

	*w = width * charscale;
	*h = lines * charscale;
}

void DrawHUDString (char *string, int x, int y, int centerwidth, int xor)
{
	int		margin;
	char	line[1024];
	int		width;
	int		i;
	int		charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;

	margin = x;

	while (*string)
	{
		// scan out one line of text from the string
		width = 0;
		while (*string && *string != '\n')
			line[width++] = *string++;
		line[width] = 0;

		if (centerwidth)
			x = margin + (centerwidth - width*charscale)/2;
		else
			x = margin;
		for (i=0 ; i<width ; i++)
		{
			Draw_Char (x, y, line[i]^xor);
			x += charscale;
		}
		if (*string)
		{
			string++;	// skip the \n
			x = margin;
			y += charscale;
		}
	}
}


/*
==============
SCR_DrawField
==============
*/
void SCR_DrawField (int x, int y, int color, int width, int value, float scale)
{
	char	num[16], *ptr;
	int		l;
	int		frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	SCR_AddDirtyPoint (x, y);
	SCR_AddDirtyPoint (x+width*CHAR_WIDTH+2, y+23);

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l)*scale;

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Draw_ScaledPic (x,y,scale/1.8,sb_nums[color][frame]);
		x += CHAR_WIDTH*scale;
		ptr++;
		l--;
	}
}


/*
===============
SCR_TouchPics

Allows rendering code to cache all needed sbar graphics
===============
*/
void SCR_TouchPics (void)
{
	int		i, j;

	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<11 ; j++)
			R_RegisterPic (sb_nums[i][j]);

	if (strcmp(crosshair->string, "none"))
	{

		Com_sprintf (crosshair_pic, sizeof(crosshair_pic), "%s", crosshair->string);
		Draw_GetPicSize (&crosshair_width, &crosshair_height, crosshair_pic);
		if (!crosshair_width)
			crosshair_pic[0] = 0;
	}
}

/*
================
SCR_ExecuteLayoutString

================
*/
void SCR_ExecuteLayoutString (char *s)
{
	int		x, y, ny=0; //ny is coordinates used for new sb layout client tags only
	int		value;
	char	*token;
	int		width;
	int		index;
	clientinfo_t	*ci;
	int		charscale;
	float	scale;
	qboolean newSBlayout = false;

	if (cls.state != ca_active || !cl.refresh_prepped)
		return;

	if (!s[0])
		return;

	x = 0;
	y = 0;
	width = 3;
	scale = (float)(viddef.height)/600;

	if(scale < 1)
		scale = 1;

	charscale = (float)(viddef.height)*8/600;

	if(charscale < 8)
		charscale = 8;

	while (s)
	{
		token = COM_Parse (&s);
		if (!strcmp(token, "xl"))
		{
			token = COM_Parse (&s);
			x = atoi(token)*scale;
			continue;
		}
		if (!strcmp(token, "xr"))
		{
			token = COM_Parse (&s);
			x = viddef.width + atoi(token)*scale;
			continue;
		}
		if (!strcmp(token, "xv"))
		{
			token = COM_Parse (&s);
			x = viddef.width/2 - 160*scale + atoi(token)*scale;
			continue;
		}

		if (!strcmp(token, "yt"))
		{
			token = COM_Parse (&s);
			y = atoi(token)*scale;
			continue;
		}
		if (!strcmp(token, "yb"))
		{
			token = COM_Parse (&s);
			y = viddef.height + atoi(token)*scale;
			continue;
		}
		if (!strcmp(token, "yv"))
		{
			token = COM_Parse (&s);
			y = viddef.height/2 - 100*scale + atoi(token)*scale;
			ny = viddef.height/2 - 100*scale + atoi(token)*scale/2;
			continue;
		}

		if (!strcmp(token, "pic"))
		{	// draw a pic from a stat number
			if(strcmp(cl_hudimage1->string, "none")) {
				token = COM_Parse (&s);
				value = cl.frame.playerstate.stats[atoi(token)];
				if (value >= MAX_IMAGES)
					Com_Error (ERR_DROP, "Pic >= MAX_IMAGES");
				if( cl.configstrings[CS_IMAGES+value][0] )
				{
					SCR_AddDirtyPoint (x, y);
					SCR_AddDirtyPoint (x+23*scale, y+23*scale);
					Draw_ScaledPic (x, y, scale, cl.configstrings[CS_IMAGES+value]);
				}
			}

			continue;
		}

		if (!strcmp(token, "newsb"))
		{
			//print header here
			x = viddef.width/2 - 160*scale;
			y = viddef.height/2 - 64*scale;

			DrawString (x+2*scale, y, va("Player"));
			DrawString (x+176*scale, y, va("Score"));
			DrawString (x+232*scale, y, va("Ping"));
			DrawString (x+280*scale, y, va("time"));

			newSBlayout = true;
			continue;
		}

		if (!strcmp(token, "newctfsb"))
		{
			//print header here
			x = viddef.width/2 - 160*scale;
			y = viddef.height/2 - 72*scale;

			//team 1
			DrawString (x-82*scale, y, va("Player"));
			DrawString (x+76*scale, y, va("Score"));
			DrawString (x+124*scale, y, va("Ping"));

			x += 190*scale;

			//team 2
			DrawString (x-16*scale, y, va("Player"));
			DrawString (x+142*scale, y, va("Score"));
			DrawString (x+190*scale, y, va("Ping"));

			newSBlayout = true;
			continue;
		}

		if (!strcmp(token, "client"))
		{	// draw a deathmatch client block
			int		score, ping, time;

			token = COM_Parse (&s);
			x = viddef.width/2 - 160*scale + atoi(token)*scale;
			token = COM_Parse (&s);
			if(newSBlayout)
				y = viddef.height/2 - 100*scale + atoi(token)*scale/2;
			else
				y = viddef.height/2 - 100*scale + atoi(token)*scale;
			SCR_AddDirtyPoint (x, y);
			SCR_AddDirtyPoint (x+159*scale, y+31*scale);

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);

			token = COM_Parse (&s);
			time = atoi(token);

			if(newSBlayout) { //new scoreboard layout

				Draw_ColorString (x+2*scale, y+34*scale, ci->name, 1.2);
				DrawAltString (x+176*scale, y+34*scale, va("%i", score));
				DrawString (x+232*scale, y+34*scale, va("%i", ping));
				DrawString (x+280*scale, y+34*scale, va("%i", time));
			}
			else
			{
				Draw_ColorString (x+32*scale, y, ci->name, 1);
				DrawString (x+32*scale, y+8*scale,  "Score: ");
				DrawAltString (x+32*scale+7*charscale, y+8*scale,  va("%i", score));
				DrawString (x+32*scale, y+16*scale, va("Ping:  %i", ping));
				DrawString (x+32*scale, y+24*scale, va("Time:  %i", time));

				if (!ci->icon)
					ci = &cl.baseclientinfo;
				Draw_ScaledPic (x, y, scale/2, ci->iconname);
			}

			continue;
		}
		if (!strcmp(token, "queued"))
		{	// draw a deathmatch client block
			int		score, ping, time;

			token = COM_Parse (&s);
			x = viddef.width/2 - 160*scale + atoi(token)*scale;
			token = COM_Parse (&s);
			if(newSBlayout)
				y = viddef.height/2 - 100*scale + atoi(token)*scale/2;
			else
				y = viddef.height/2 - 100*scale + atoi(token)*scale;
			SCR_AddDirtyPoint (x, y);
			SCR_AddDirtyPoint (x+159*scale, y+31*scale);

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);

			token = COM_Parse (&s);
			time = atoi(token);

			token = COM_Parse (&s);

			if(newSBlayout) { //new scoreboard layout

				Draw_ColorString (x+2*scale, y+34*scale, ci->name, 1.2);
				DrawAltString (x+176*scale, y+34*scale, va("%i", score));
				DrawString (x+232*scale, y+34*scale, va("%i", ping));
				Draw_ColorString (x+280*scale, y+34*scale, va("^1%i", time), 1.2);

			}
			else {

				Draw_ColorString (x+32*scale, y, ci->name, 1);
				DrawString (x+32*scale, y+8*scale,  "Score: ");
				DrawAltString (x+32*scale+7*charscale, y+8*scale,  va("%i", score));
				DrawString (x+32*scale, y+16*scale, va("Ping:  %i", ping));
				Draw_ColorString (x+32*scale, y+24*scale, va("^1Queue:  ^1%i", time), 1);

				if (!ci->icon)
					ci = &cl.baseclientinfo;
				Draw_ScaledPic (x, y, scale/2, ci->iconname);
			}

			continue;
		}
		if (!strcmp(token, "ctf"))
		{	// draw a ctf client block
			int	score, ping;
			char	block[80];
			int		team;

			token = COM_Parse (&s);
			x = viddef.width/2 - 160*scale + atoi(token)*scale;
			if(atoi(token) < 0)
				team = 0;
			else
				team = 1;
			token = COM_Parse (&s);
			y = viddef.height/2 - 100*scale + atoi(token)*scale;
			SCR_AddDirtyPoint (x, y);
			SCR_AddDirtyPoint (x+159*scale, y+31*scale);

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);
			if (ping > 999)
				ping = 999;

			if(newSBlayout) {
				Draw_ColorString (x+16, y, ci->name, 1.2);
				DrawAltString (x+172*scale, y+1*scale, va("%i", score));
				DrawString (x+220*scale, y+1*scale, va("%i", ping));

				if(team) { //draw pic on left side(done client side to save packetsize
					Draw_ScaledPic (x, y-2*scale, scale, "/pics/blueplayerbox");
				}
				else
					Draw_ScaledPic(x, y-2*scale, scale, "/pics/redplayerbox");
			}
			else {
				sprintf(block, "%3d %3d %s", score, ping, ci->name);
				Draw_ColorString (x, y, block, 1);
			}

			continue;
		}

		if (!strcmp(token, "picn"))
		{	// draw a pic from a name
			token = COM_Parse (&s);
			if(newSBlayout && !strcmp(token, "playerbox")) { //cannot simply fill y = ny here
				SCR_AddDirtyPoint (x, ny+32*scale);
				SCR_AddDirtyPoint (x+23*scale, ny+55*scale);
				Draw_ScaledPic (x, ny+32*scale, scale, token);
			}
			else {
				SCR_AddDirtyPoint (x, y);
				SCR_AddDirtyPoint (x+23*scale, y+23*scale);
				Draw_ScaledPic (x, y, scale, token);
			}
			continue;
		}

		if (!strcmp(token, "num"))
		{	// draw a number
			if(strcmp(cl_hudimage1->string, "none")) {
				token = COM_Parse (&s);
				width = atoi(token);
				token = COM_Parse (&s);
				value = cl.frame.playerstate.stats[atoi(token)];
				SCR_DrawField (x, y, 0, width, value, scale);
			}

			continue;
		}

		if (!strcmp(token, "hnum"))
		{	// health number
			if(strcmp(cl_hudimage1->string, "none")) {
				int		color;

				width = 3;
				value = cl.frame.playerstate.stats[STAT_HEALTH];
				if (value > 25)
					color = 0;	// green
				else if (value > 0)
					color = (cl.frame.serverframe>>2) & 1;		// flash
				else
					color = 1;

				SCR_DrawField (x, y, color, width, value, scale);
			}

			//draw the zoom scope pic if we are using the zoom alt-fire of disruptor
			if (cl.frame.playerstate.stats[STAT_ZOOMED])
			{
				char zoompic[32];
				sprintf(zoompic, "zoomscope%i", cl.frame.playerstate.stats[STAT_ZOOMED]);
				Draw_StretchPic (0, 0, viddef.width, viddef.height, zoompic);
			}

			continue;
		}

		if (!strcmp(token, "anum"))
		{	// ammo number
			if(strcmp(cl_hudimage1->string, "none")) {
				int		color;

				width = 3;
				value = cl.frame.playerstate.stats[STAT_AMMO];
				if (value > 5)
					color = 0;	// green
				else if (value >= 0)
					color = (cl.frame.serverframe>>2) & 1;		// flash
				else
					continue;	// negative number = don't show

				SCR_DrawField (x, y, color, width, value, scale);
			}

			continue;
		}

		if (!strcmp(token, "rnum"))
		{	// armor number
			if(strcmp(cl_hudimage1->string, "none")) {
				int		color;

				width = 3;
				value = cl.frame.playerstate.stats[STAT_ARMOR];
				if (value < 1)
					continue;

				color = 0;	// green

				SCR_DrawField (x, y, color, width, value, scale);
			}

			continue;
		}


		if (!strcmp(token, "stat_string"))
		{
			token = COM_Parse (&s);
			index = atoi(token);
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error (ERR_DROP, "Bad stat_string index");
			index = cl.frame.playerstate.stats[index];
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error (ERR_DROP, "Bad stat_string index");
			DrawString (x, y, cl.configstrings[index]);
			continue;
		}

		if (!strcmp(token, "cstring"))
		{
			token = COM_Parse (&s);
			DrawHUDString (token, x, y, 320, 0);
			continue;
		}

		if (!strcmp(token, "string"))
		{
			token = COM_Parse (&s);
			//this line is an Alien Arena specific hack of sorts, remove if needed
			if(!strcmp(token, "Vote"))
				Draw_ScaledPic (viddef.width/2 - 85*scale, y-8*scale, scale, "votebox");
			DrawString (x, y, token);
			continue;
		}

		if (!strcmp(token, "cstring2"))
		{
			token = COM_Parse (&s);
			DrawHUDString (token, x, y, 320,0x80);
			continue;
		}

		if (!strcmp(token, "string2"))
		{
			token = COM_Parse (&s);
			DrawString (x, y, token);
			continue;
		}

		if (!strcmp(token, "if"))
		{	// draw a number
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			if (!value)
			{	// skip to endif
				while (s && strcmp(token, "endif") )
				{
					token = COM_Parse (&s);
				}
			}

			continue;
		}
	}
}


/*
================
SCR_DrawStats

The status bar is a small layout program that
is based on the stats array
================
*/
void SCR_DrawStats (void)
{
	SCR_ExecuteLayoutString (cl.configstrings[CS_STATUSBAR]);
}


/*
================
SCR_DrawLayout

================
*/
#define	STAT_LAYOUTS		13

void SCR_DrawLayout (void)
{
	if (!cl.frame.playerstate.stats[STAT_LAYOUTS])
		return;
	SCR_ExecuteLayoutString (cl.layout);
}

/*
=================
SCR_DrawPlayerIcon
=================
*/
char		scr_playericon[MAX_OSPATH];
char		scr_playername[32];
float		scr_playericonalpha;
void SCR_DrawPlayerIcon(void) {

	int w, h;
	float scale, iconPos;

	if (cls.key_dest == key_menu || cls.key_dest == key_console)
		return;

	if(scr_playericonalpha <= 0.0)
		return;

	scr_playericonalpha -= cls.frametime; //fade map pic in

	if(scr_playericonalpha < 1.0)
		iconPos = scr_playericonalpha;
	else
		iconPos = 1.0;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	w = h = 64; //icon size, will change to be larger

	w*=scale;
	h*=scale;

	Draw_AlphaStretchPlayerIcon( -w+(w*iconPos), viddef.height/2 + h/2, w, h, scr_playericon, scr_playericonalpha);
	SCR_DrawColorString ( -w+(w*iconPos), viddef.height/2 + h + 32*scale, scr_playername);
}

/*
================

SCR_showTimer

================
*/
char		temptime[32];
int		timecounter;
int		seconds, minutes;
void SCR_showTimer(void)
{
	float scale;

	if (cls.key_dest == key_menu || cls.key_dest == key_console)
		return;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	if ((cls.realtime + 2000) < timecounter)
		timecounter = cl.time + 1000;

	if (cls.realtime > timecounter)
	{
		seconds += 1;
		if(seconds >= 60) {
			minutes++;
			seconds = 0;
		}
		Com_sprintf(temptime, sizeof(temptime),"%i:%2i", minutes, seconds);
		timecounter = cls.realtime + 1000;
	}

	Draw_StretchPic (viddef.width - 104*scale, viddef.height -42*scale, 32*scale, 32*scale, "timer");

	DrawString(viddef.width - 64*scale, viddef.height - 32*scale, temptime);
}

/*
================

SCR_showFPS

================
*/

int		fpscounter;
char		temp[32];

void SCR_showFPS(void)
{
	float scale;
	static qboolean restart = true;
	static float update_trigger = 0.0f;
	static float framecounter = 0.0f;
	static int start_msec;
	int end_msec;
	float time_sec;
	float framerate;

	if (cls.key_dest == key_menu || cls.key_dest == key_console)
	{
		restart = true;
		return;
	}

	if( restart )
	{
		start_msec = Sys_Milliseconds();
		framecounter = 0.0f;
		update_trigger = 10.0f; // initial delay to update
		temp[0] = 0; // blank the text buffer
		restart = false;
	}
	else
	{
		framecounter += 1.0f;
		if( framecounter >= update_trigger )
		{
			// calculate frames-per-second
			end_msec = Sys_Milliseconds();
			time_sec = ((float)(end_msec - start_msec)) / 1000.0f;
			framerate = framecounter / time_sec ;

			// update text buffer for display
			if ( cl_drawfps->integer == 2 )
			{ // for developers
				Com_sprintf( temp, sizeof(temp), "%3.1ffps", framerate );
			}
			else
			{
				Com_sprintf( temp, sizeof(temp), "%3.0ffps", framerate );
			}

			// setup for next update
			framecounter = 0.0f;
			start_msec = end_msec;
			update_trigger = framerate / 2.0 ; // for .5 sec update interval
		}
	}

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	DrawString(viddef.width - 64*scale, viddef.height - 24*scale, temp);

}

/*
 * SCR_ShowRSpeeds()
 *   display the cvar r_speeds activated performance counters
 */
void SCR_showRSpeeds( void )
{
	char prtstring[32];
 	float scale;
 	extern int c_brush_polys;
 	extern int c_alias_polys;

 	if (cls.key_dest == key_menu || cls.key_dest == key_console)
 		return;

 	scale = (float)(viddef.height)/600;
 	if(scale < 1)
 		scale = 1;

 	Com_sprintf( prtstring, sizeof( prtstring ),
 		"%5.5i wpoly %5.5i epoly",  c_brush_polys, c_alias_polys );
 	DrawString(	viddef.width/2, viddef.height - 48*scale,	prtstring );
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen (void)
{
	int numframes;
	int i;
	float separation[2] = { 0, 0 };

	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if (cls.disable_screen)
	{
		if (Sys_Milliseconds() - cls.disable_screen > 120000)
		{
			cls.disable_screen = 0;
			Com_Printf ("Loading plaque timed out.\n");
			return;
		}
		scr_draw_loading = 2;
	}

	if (!scr_initialized || !con.initialized)
		return;				// not initialized yet

	/*
	** range check cl_camera_separation so we don't inadvertently fry someone's
	** brain
	*/
	if ( cl_stereo_separation->value > 1.0 )
		Cvar_SetValue( "cl_stereo_separation", 1.0 );
	else if ( cl_stereo_separation->value < 0 )
		Cvar_SetValue( "cl_stereo_separation", 0.0 );

	if ( cl_stereo->value )
	{
		numframes = 2;
		separation[0] = -cl_stereo_separation->value / 2;
		separation[1] =  cl_stereo_separation->value / 2;
	}
	else
	{
		separation[0] = 0;
		separation[1] = 0;
		numframes = 1;
	}

	for ( i = 0; i < numframes; i++ )
	{
		R_BeginFrame( separation[i] );

		if (scr_draw_loading == 2)
		{	//  loading plaque over black screen

			SCR_DrawLoading ();

			if (cls.disable_screen)
				scr_draw_loading = 2;

			//NO CONSOLE!!!
			continue;
		}
		else
		{
			// do 3D refresh drawing, and then update the screen
			SCR_CalcVrect ();

//			need_free_vbo = true;
			V_RenderView ( separation[i] );

			SCR_DrawStats ();
			if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 1)
				SCR_DrawLayout ();
			if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 2)
				CL_DrawInventory ();

			SCR_DrawNet ();
			SCR_CheckDrawCenterString ();
			SCR_CheckDrawIRCString();

			if (scr_timegraph->value)
				SCR_DebugGraph (cls.frametime*300, 0);

			if (scr_debuggraph->value || scr_timegraph->value || scr_netgraph->value)
				SCR_DrawDebugGraph ();

			SCR_DrawPause ();

			SCR_DrawConsole ();

//			if (need_free_vbo)
//				R_VCFreeFrame();

			M_Draw ();

			SCR_DrawLoading ();

			if(cl_drawfps->value)
			{
				SCR_showFPS();
			}

			if(cl_drawtimer->value)
			{
				SCR_showTimer();
			}
			{
				extern cvar_t* r_speeds;
				if( r_speeds->value == 2 )
					SCR_showRSpeeds();
			}
			SCR_DrawPlayerIcon();
		}
	}
	R_EndFrame();
}
