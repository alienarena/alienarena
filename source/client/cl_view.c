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
// cl_view.c -- player rendering positioning

#include "client.h"

//=============
//
// development tools for weapons
//
int			gun_frame;
struct model_s	*gun_model;

int r_numflares;
flare_t r_flares[MAX_FLARES];

int r_numgrasses;
grass_t r_grasses[MAX_GRASSES];

//=============
extern cvar_t *cl_showPlayerNames;
extern cvar_t *name;
extern char map_music[128];
extern cvar_t *background_music;
extern qboolean IsVisible(vec3_t org1,vec3_t org2);

cvar_t		*crosshair;
cvar_t		*cl_testparticles;
cvar_t		*cl_testentities;
cvar_t		*cl_testlights;
cvar_t		*cl_testblend;

cvar_t		*cl_stats;


int			r_numdlights;
dlight_t	r_dlights[MAX_DLIGHTS];

int			r_numentities;
entity_t	r_entities[MAX_ENTITIES];

int			r_numparticles;
particle_t	r_particles[MAX_PARTICLES];

lightstyle_t	r_lightstyles[MAX_LIGHTSTYLES];

char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int num_cl_weaponmodels;

/*
====================
V_ClearScene

Specifies the model that will be used as the world
====================
*/
void V_ClearScene (void)
{
	r_numdlights = 0;
	r_numentities = 0;
	r_numparticles = 0;
}


/*
=====================
V_AddEntity

=====================
*/
void V_AddEntity (entity_t *ent)
{
	if (r_numentities >= MAX_ENTITIES)
		return;
	r_entities[r_numentities++] = *ent;
}


/*
=====================
V_AddParticle

=====================
*/
void V_AddParticle (vec3_t org, vec3_t angle, int color, int type, int texnum, int blenddst, int blendsrc, float alpha, float scale)
{
	particle_t	*p;

	if (r_numparticles >= MAX_PARTICLES)
		return;
	p = &r_particles[r_numparticles++];
	VectorCopy (org, p->origin);
	VectorCopy (angle, p->angle);
	p->color = color;
	p->alpha = alpha;
	p->type = type;
	p->texnum = texnum;
	p->blenddst = blenddst;
	p->blendsrc = blendsrc;
	p->scale = scale;
}

/*
=====================
V_AddLight

=====================
*/
void V_AddLight (vec3_t org, float intensity, float r, float g, float b)
{
	dlight_t	*dl;

	if (r_numdlights >= MAX_DLIGHTS)
		return;
	dl = &r_dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->team = 0;  //added for team lights
}
/*
=====================
V_AddTeamLight

=====================
*/
void V_AddTeamLight (vec3_t org, float intensity, float r, float g, float b, int team)
{
	dlight_t	*dl;

	if (r_numdlights >= MAX_DLIGHTS)
		return;
	dl = &r_dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->team = team;
}

/*
=====================
V_AddLightStyle

=====================
*/
void V_AddLightStyle (int style, float r, float g, float b)
{
	lightstyle_t	*ls;

	if (style < 0 || style > MAX_LIGHTSTYLES)
		Com_Error (ERR_DROP, "Bad light style %i", style);
	ls = &r_lightstyles[style];

	ls->white = r+g+b;
	ls->rgb[0] = r;
	ls->rgb[1] = g;
	ls->rgb[2] = b;
}

/*
================
V_TestParticles

If cl_testparticles is set, create 4096 particles in the view
================
*/
void V_TestParticles (void)
{
	particle_t	*p;
	int			i, j;
	float		d, r, u;

	r_numparticles = MAX_PARTICLES;
	for (i=0 ; i<r_numparticles ; i++)
	{
		d = i*0.25;
		r = 4*((i&7)-3.5);
		u = 4*(((i>>3)&7)-3.5);
		p = &r_particles[i];

		for (j=0 ; j<3 ; j++)
			p->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*d +
			cl.v_right[j]*r + cl.v_up[j]*u;

		p->color = 8;
		p->alpha = cl_testparticles->value;
	}
}

/*
================
V_TestEntities

If cl_testentities is set, create 32 player models
================
*/
void V_TestEntities (void)
{
	int			i, j;
	float		f, r;
	entity_t	*ent;

	r_numentities = 32;
	memset (r_entities, 0, sizeof(r_entities));

	for (i=0 ; i<r_numentities ; i++)
	{
		ent = &r_entities[i];

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++)
			ent->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*f +
			cl.v_right[j]*r;

		ent->model = cl.baseclientinfo.model;
		ent->skin = cl.baseclientinfo.skin;
	}
}

/*
================
V_TestLights

If cl_testlights is set, create 32 lights models
================
*/
void V_TestLights (void)
{
	int			i, j;
	float		f, r;
	dlight_t	*dl;

	r_numdlights = 32;
	memset (r_dlights, 0, sizeof(r_dlights));

	for (i=0 ; i<r_numdlights ; i++)
	{
		dl = &r_dlights[i];

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++)
			dl->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*f +
			cl.v_right[j]*r;
		dl->color[0] = ((i%6)+1) & 1;
		dl->color[1] = (((i%6)+1) & 2)>>1;
		dl->color[2] = (((i%6)+1) & 4)>>2;
		dl->intensity = 200;
	}
}

//===================================================================

/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
qboolean needLoadingPlaque (void);
void CL_PrepRefresh ()
{
	char		mapname[32];
	int			i, max;
	char		name[MAX_QPATH];
	float		rotate;
	vec3_t		axis;
	qboolean	newPlaque = needLoadingPlaque();
	
	loadingPercent = 0;
	rocketlauncher = 0;
	rocketlauncher_drawn = 0;
	smartgun = 0;
	smartgun_drawn = 0;
	disruptor = 0;
	disruptor_drawn = 0;
	flamethrower = 0;
	flamethrower_drawn = 0;
	beamgun = 0;
	beamgun_drawn = 0;
	chaingun = 0;
	chaingun_drawn = 0;
	vaporizer = 0;
	vaporizer_drawn = 0;
	quad = 0;
	quad_drawn = 0;
	haste = 0;
	haste_drawn = 0;
	sproing = 0;
	sproing_drawn = 0;
	inv = 0;
	inv_drawn = 0;
	adren = 0;
	adren_drawn = 0;

	numitemicons = 0;

	if (!cl.configstrings[CS_MODELS+1][0])
		return;		// no map loaded
    
	if (newPlaque)
		SCR_BeginLoadingPlaque();

	loadingMessage = true;
	Com_sprintf (loadingMessages[0], sizeof(loadingMessages[0]), "loading map...");
	Com_sprintf (loadingMessages[1], sizeof(loadingMessages[1]), "loading models...");
	Com_sprintf (loadingMessages[2], sizeof(loadingMessages[2]), "loading pics...");
	Com_sprintf (loadingMessages[3], sizeof(loadingMessages[3]), "loading clients...");

	SCR_AddDirtyPoint (0, 0);
	SCR_AddDirtyPoint (viddef.width-1, viddef.height-1);

	// let the render dll load the map
	strcpy (mapname, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
	mapname[strlen(mapname)-4] = 0;		// cut off ".bsp"

	// register models, pics, and skins

	//this was moved here, to prevent having a pic loaded over and over again, which was
	//totally killing performance after a dozen or so maps.
	map_pic_loaded = (ptrdiff_t)(R_RegisterPic(va("/levelshots/%s.pcx", mapname)));
	
	Com_Printf ("Map: %s\r", mapname); 
	SCR_UpdateScreen ();
	R_BeginRegistration (mapname);
	Com_Printf ("                                     \r");
	Com_sprintf (loadingMessages[0], sizeof(loadingMessages[0]), "loading map...done");
	loadingPercent += 20;
	
	// precache status bar pics
	Com_Printf ("pics\r"); 
	SCR_UpdateScreen ();
	SCR_TouchPics ();
	Com_Printf ("                                     \r");

	num_cl_weaponmodels = 1;
	strcpy(cl_weaponmodels[0], "weapon.md2");

	for (i=1, max=0 ; i<MAX_MODELS && cl.configstrings[CS_MODELS+i][0] ; i++)
		max++;
	for (i=1 ; i<MAX_MODELS && cl.configstrings[CS_MODELS+i][0] ; i++)
	{
		strcpy (name, cl.configstrings[CS_MODELS+i]);
		name[37] = 0;	// never go beyond one line
		if (name[0] != '*')
		{
			Com_Printf ("%s\r", name);

			//only make max of 20 chars long
			Com_sprintf (loadingMessages[1], sizeof(loadingMessages[1]), "loading models...%s", 
				(strlen(name)>20)? &name[strlen(name)-20]: name);

			//check for types
			if(!strcmp(name, "models/weapons/g_rocket/tris.md2"))
				rocketlauncher = 1;
			if(!strcmp(name, "models/weapons/g_glaunch/tris.md2"))
				rocketlauncher = 1;
			if(!strcmp(name, "models/weapons/g_shotg2/tris.md2"))
				chaingun = 1;
			if(!strcmp(name, "models/weapons/g_shotg/tris.md2"))
				smartgun = 1;
			if(!strcmp(name, "models/weapons/g_rail/tris.md2"))
				beamgun = 1;
			if(!strcmp(name, "models/weapons/g_hyperb/tris.md2"))
				disruptor = 1;
			if(!strcmp(name, "models/weapons/g_chain/tris.md2"))
				flamethrower = 1;
			if(!strcmp(name, "models/weapons/g_machn/tris.md2"))
				vaporizer = 1;
			if(!strcmp(name, "models/weapons/g_bfg/tris.md2"))
				vaporizer = 1;
			if(!strcmp(name, "models/items/quaddama/tris.md2"))
				quad = 1;
			if(!strcmp(name, "models/items/haste/tris.md2"))
				haste = 1;
			if(!strcmp(name, "models/items/sproing/tris.md2"))
				sproing = 1;
			if(!strcmp(name, "models/items/adrenaline/tris.md2"))
				adren = 1;
			if(!strcmp(name, "models/items/invulner/tris.md2"))
				inv = 1;
		}

		SCR_UpdateScreen ();
	
		Sys_SendKeyEvents ();	// pump message loop
		if (name[0] == '#')
		{
			// special player weapon model
			if (num_cl_weaponmodels < MAX_CLIENTWEAPONMODELS)
			{
				strncpy(cl_weaponmodels[num_cl_weaponmodels], cl.configstrings[CS_MODELS+i]+1,
					sizeof(cl_weaponmodels[num_cl_weaponmodels]) - 1);
				num_cl_weaponmodels++;
			}
		} 
		else
		{
			cl.model_draw[i] = R_RegisterModel (cl.configstrings[CS_MODELS+i]);
			if (name[0] == '*')
				cl.model_clip[i] = CM_InlineModel (cl.configstrings[CS_MODELS+i]);
			else
				cl.model_clip[i] = NULL;
		}
		if (name[0] != '*')
			Com_Printf ("                                     \r");

		loadingPercent += 60.0f/(float)max;
	}
	Com_sprintf (loadingMessages[1], sizeof(loadingMessages[1]), "loading models...done");
	


	Com_Printf ("images\r", i); 
	SCR_UpdateScreen ();

	for (i=1, max=0 ; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0] ; i++)
		max++;
	for (i=1 ; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0] ; i++)
	{
		cl.image_precache[i] = R_RegisterPic (cl.configstrings[CS_IMAGES+i]);
		Sys_SendKeyEvents ();	// pump message loop		
		loadingPercent += 10.0f/(float)max;
	}
	Com_sprintf (loadingMessages[2], sizeof(loadingMessages[2]), "loading pics...done");

	Com_Printf ("                                     \r");

	//refresh the player model/skin info 
	CL_LoadClientinfo (&cl.baseclientinfo, va("unnamed\\%s\\%s", DEFAULTMODEL, DEFAULTSKIN));

	for (i=1, max=0 ; i<MAX_CLIENTS ; i++)
		if (cl.configstrings[CS_PLAYERSKINS+i][0])
			max++;
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS+i][0])
			continue;

		Com_sprintf (loadingMessages[3], sizeof(loadingMessages[3]), "loading clients...%i", i);
		Com_Printf ("client %i\r", i); 
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		CL_ParseClientinfo (i);
		Com_Printf ("                                     \r");
		loadingPercent += 10.0f/(float)max;
	}
	Com_sprintf (loadingMessages[3], sizeof(loadingMessages[3]), "loading clients...done");

	//hack hack hack - psychospaz
	loadingPercent = 100;

	// set sky textures and speed
	Com_Printf ("sky\r", i); 
	SCR_UpdateScreen ();
	rotate = atof (cl.configstrings[CS_SKYROTATE]);
	sscanf (cl.configstrings[CS_SKYAXIS], "%f %f %f", 
		&axis[0], &axis[1], &axis[2]);
	R_SetSky (cl.configstrings[CS_SKY], rotate, axis);
	Com_Printf ("                                     \r");

	// the renderer can now free unneeded stuff
	R_EndRegistration ();

	// clear any lines of console text
	Con_ClearNotify ();

	SCR_UpdateScreen ();
	cl.refresh_prepped = true;
	cl.force_refdef = true;	// make sure we have a valid refdef

	// start background music
	background_music = Cvar_Get ("background_music", "1", CVAR_ARCHIVE);
	if(background_music->value)
		S_StartMusic(map_music);

	//loadingMessage = false;
	rocketlauncher = 0;
	rocketlauncher_drawn = 0;
	smartgun = 0;
	smartgun_drawn = 0;
	disruptor = 0;
	disruptor_drawn = 0;
	flamethrower = 0;
	flamethrower_drawn = 0;
	beamgun = 0;
	beamgun_drawn = 0;
	chaingun = 0;
	chaingun_drawn = 0;
	vaporizer = 0;
	vaporizer_drawn = 0;
	quad = 0;
	quad_drawn = 0;
	haste = 0;
	haste_drawn = 0;
	sproing = 0;
	sproing_drawn = 0;
	inv = 0;
	inv_drawn = 0;
	adren = 0;
	adren_drawn = 0;
	numitemicons = 0;

	if (newPlaque)
		SCR_EndLoadingPlaque();
	else
		Cvar_Set ("paused", "0");
}

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		Com_Error (ERR_DROP, "Bad fov: %f", fov_x);

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}

//============================================================================

// gun frame debugging functions
void V_Gun_Next_f (void)
{
	gun_frame++;
	Com_Printf ("frame %i\n", gun_frame);
}

void V_Gun_Prev_f (void)
{
	gun_frame--;
	if (gun_frame < 0)
		gun_frame = 0;
	Com_Printf ("frame %i\n", gun_frame);
}

void V_Gun_Model_f (void)
{
	char	name[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		gun_model = NULL;
		return;
	}
	Com_sprintf (name, sizeof(name), "models/%s/tris.md2", Cmd_Argv(1));
	gun_model = R_RegisterModel (name);
}

//============================================================================


/*
=================
SCR_DrawCrosshair
=================
*/
void SCR_DrawCrosshair (void)
{
	if (!strcmp(crosshair->string, "none"))
		return;

	if (crosshair->modified)
	{
		crosshair->modified = false;
		SCR_TouchPics ();
	}

	if (!crosshair_pic[0])
		return;

	Draw_Pic (scr_vrect.x + ((scr_vrect.width - crosshair_width)>>1)
	, scr_vrect.y + ((scr_vrect.height - crosshair_height)>>1), crosshair_pic);
}

qboolean InFront (vec3_t target)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;
	
	AngleVectors (cl.refdef.viewangles, forward, NULL, NULL);
	VectorSubtract (target, cl.refdef.vieworg, vec);
	VectorNormalize (vec);
	dot = DotProduct (vec, forward);
	
	if (dot > 0.3)
		return true;
	return false;
}
//============== 
//SCR_DrawPlayerNamesCenter
// shows player names at center of screen 
//============== 
void SCR_DrawPlayerNamesCenter( void ) 
{ 
   int         i; 
   centity_t   *cent; 
   float      dist, mindist; 
   trace_t   trace;
   vec3_t vecdist; 
   vec3_t temp; 
   vec3_t axis[3]; 
   static vec3_t mins = { -8, -8, -8 }; 
   static vec3_t maxs = { 8, 8, 8 }; 
   int closest;

   if( !cl_showPlayerNames->integer ) 
      return; 

   mindist = 1000;
   closest = 0;
 
   for( i = 0; i < MAX_CLIENTS; i++ ) 
   { 
      
	  cent = cl_entities + i + 1; 
	
	  if( !cent->current.modelindex ) 
		 continue; 
	 
	  if(!strcmp(cl.clientinfo[i].name, name->string))
		  continue;

	  trace = CL_Trace ( cl.refdef.vieworg, mins, maxs, cent->current.origin, -1, MASK_PLAYERSOLID, true, NULL); 
	  if (trace.fraction != 1.0)	
		  continue;

	  VectorSubtract(cent->current.origin, cl.refdef.vieworg, vecdist); 
	  dist = VectorLength(vecdist); 

	  if (dist >= 1000) 
		  continue; 

	  if(dist < mindist && (strlen(cl.clientinfo[i].name) > 1) && InFront(cent->current.origin) ) {
		  mindist = dist;
		  closest = i;
	  }
      
	  VectorSubtract (cent->current.origin, cl.refdef.vieworg, temp); 
	  VectorNormalize (temp); 

	  AngleVectors(cl.refdef.viewangles, axis[0], axis[1], axis[2]); 

	  if (DotProduct (temp, axis[0]) < 0) 
	      continue; 

	  }
	  if(closest)
		  Draw_ColorString( (int)(cl.refdef.width/2 - strlen(cl.clientinfo[closest].name)*3), (int)(cl.refdef.height/1.8), cl.clientinfo[closest].name);
  		
}

//============== 
//SCR_DrawPlayerNames 
// shows player names at their feets. 
//============== 
extern void R_TransformVectorToScreen( refdef_t *rd, vec3_t in, vec2_t out );
void SCR_DrawPlayerNames( void ) 
{ 
   static vec4_t   whiteTransparent = { 1.0f, 1.0f, 1.0f, 0.5f }; 
   int         i; 
   centity_t   *cent; 
   float      dist; 
   trace_t   trace;
   vec2_t screen_pos; 
   vec3_t vecdist; 
   vec3_t temp; 
   vec3_t axis[3]; 
   int y; 
   static vec3_t mins = { -4, -4, -4 }; 
   static vec3_t maxs = { 4, 4, 4 }; 

   if( !cl_showPlayerNames->integer ) 
      return; 
  
   for( i = 0; i < MAX_CLIENTS; i++ ) 
   { 
      
	  cent = cl_entities + i + 1; 
	
	  if( !cent->current.modelindex ) 
		 continue; 
	 
	  if(!strcmp(cl.clientinfo[i].name, name->string))
		  continue;
	
	  trace = CL_Trace ( cl.refdef.vieworg, mins, maxs, cent->current.origin, -1, MASK_PLAYERSOLID, true, NULL); 
	  if (trace.fraction != 1.0)	
		  continue;

	  VectorSubtract(cent->current.origin, cl.refdef.vieworg, vecdist); 
	  dist = VectorLength(vecdist); 

	  if (dist >= 1000 || !InFront(cent->current.origin)) 
		  continue; 
      
	  VectorSubtract (cent->current.origin, cl.refdef.vieworg, temp); 
	  VectorNormalize (temp); 

	  AngleVectors(cl.refdef.viewangles, axis[0], axis[1], axis[2]); 

	  if (DotProduct (temp, axis[0]) < 0) 
	      continue; 

	  R_TransformVectorToScreen(&cl.refdef, cent->current.origin, screen_pos); 
	  y = cl.refdef.height-(int)screen_pos[1]-cl.refdef.height/6; 
	  Draw_ColorString ( (int)screen_pos[0], y, cl.clientinfo[i].name ); 

   } 
}

void SCR_DrawBases (void)
{
	int			i;
	entity_t	*ent;
    vec2_t screen_pos; 
    int y; 

	for (i=0 ; i<cl.refdef.num_entities; i++)
	{
		ent = &r_entities[i];

		if(!ent->team)
			continue;

		if (!InFront(ent->origin)) 
			continue; 
    
		R_TransformVectorToScreen(&cl.refdef, ent->origin, screen_pos); 
		y = cl.refdef.height-(int)screen_pos[1]-cl.refdef.height/6; 
		if(ent->team == 2)
			Draw_ColorString ( (int)screen_pos[0], y, "^4Blue Flag" ); 
		else if(ent->team == 1)
			Draw_ColorString ( (int)screen_pos[0], y, "^1Red Flag" );

	}
}

/*
==================
V_RenderView

==================
*/
void V_RenderView( float stereo_separation )
{
	extern int entitycmpfnc( const entity_t *, const entity_t * );

	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;			// still loading

	if (cl_timedemo->value)
	{
		if (!cl.timedemo_start)
			cl.timedemo_start = Sys_Milliseconds ();
		cl.timedemo_frames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if ( cl.frame.valid && (cl.force_refdef || !cl_paused->value) )
	{
		cl.force_refdef = false;

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities ();

		if (cl_testparticles->value)
			V_TestParticles ();
		if (cl_testentities->value)
			V_TestEntities ();
		if (cl_testlights->value)
			V_TestLights ();
		if (cl_testblend->value)
		{
			cl.refdef.blend[0] = 1;
			cl.refdef.blend[1] = 0.5;
			cl.refdef.blend[2] = 0.25;
			cl.refdef.blend[3] = 0.5;
		}

		// offset vieworg appropriately if we're doing stereo separation
		if ( stereo_separation != 0 )
		{
			vec3_t tmp;

			VectorScale( cl.v_right, stereo_separation, tmp );
			VectorAdd( cl.refdef.vieworg, tmp, cl.refdef.vieworg );
		}

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.vieworg[0] += 1.0/16;
		cl.refdef.vieworg[1] += 1.0/16;
		cl.refdef.vieworg[2] += 1.0/16;

		cl.refdef.x = scr_vrect.x;
		cl.refdef.y = scr_vrect.y;
		cl.refdef.width = scr_vrect.width;
		cl.refdef.height = scr_vrect.height;
		cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
		cl.refdef.time = cl.time*0.001;

		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->value)
			r_numentities = 0;
		if (!cl_add_particles->value)
			r_numparticles = 0;
		if (!cl_add_lights->value)
			r_numdlights = 0;
		if (!cl_add_blend->value)
		{
			VectorClear (cl.refdef.blend);
		}

		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;
		cl.refdef.num_particles = r_numparticles;
		cl.refdef.particles = r_particles;
		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;
		cl.refdef.lightstyles = r_lightstyles;
		cl.refdef.num_flares = r_numflares;
        cl.refdef.flares = r_flares;
		cl.refdef.num_grasses = r_numgrasses;
		cl.refdef.grasses = r_grasses;

		cl.refdef.rdflags = cl.frame.playerstate.rdflags;

		// sort entities for better cache locality
        qsort( cl.refdef.entities, cl.refdef.num_entities, sizeof( cl.refdef.entities[0] ), (int (*)(const void *, const void *))entitycmpfnc );

		V_ClearScene ();
	}

	cl.refdef.rdflags |= RDF_BLOOM;   //BLOOMS

	R_RenderFrame (&cl.refdef);
	if (cl_stats->value)
		Com_Printf ("ent:%i  lt:%i  part:%i\n", r_numentities, r_numdlights, r_numparticles);
	if ( log_stats->value && ( log_stats_file != 0 ) )
		fprintf( log_stats_file, "%i,%i,%i,",r_numentities, r_numdlights, r_numparticles);


	SCR_AddDirtyPoint (scr_vrect.x, scr_vrect.y);
	SCR_AddDirtyPoint (scr_vrect.x+scr_vrect.width-1,
		scr_vrect.y+scr_vrect.height-1);

	SCR_DrawCrosshair ();

	if(cl_showPlayerNames->integer) {
		if(cl_showPlayerNames->integer == 2)
			SCR_DrawPlayerNames();
		else
			SCR_DrawPlayerNamesCenter();

		SCR_DrawBases();
	}
}


/*
=============
V_Viewpos_f
=============
*/
void V_Viewpos_f (void)
{
	Com_Printf ("(%i %i %i) : %i\n", (int)cl.refdef.vieworg[0],
		(int)cl.refdef.vieworg[1], (int)cl.refdef.vieworg[2], 
		(int)cl.refdef.viewangles[YAW]);
}

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("gun_next", V_Gun_Next_f);
	Cmd_AddCommand ("gun_prev", V_Gun_Prev_f);
	Cmd_AddCommand ("gun_model", V_Gun_Model_f);

	Cmd_AddCommand ("viewpos", V_Viewpos_f);

	crosshair = Cvar_Get ("crosshair", "0", CVAR_ARCHIVE);

	cl_testblend = Cvar_Get ("cl_testblend", "0", 0);
	cl_testparticles = Cvar_Get ("cl_testparticles", "0", 0);
	cl_testentities = Cvar_Get ("cl_testentities", "0", 0);
	cl_testlights = Cvar_Get ("cl_testlights", "0", 0);

	cl_stats = Cvar_Get ("cl_stats", "0", 0);
}
