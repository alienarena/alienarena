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
// r_main.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_script.h"
#include "r_ragdoll.h"
#include "r_text.h"


void R_Clear (void);

viddef_t	vid;

int r_viewport[4];

int GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6, GL_TEXTURE7;

model_t		*r_worldmodel;

float			gldepthmin, gldepthmax;
float			r_frametime;

glconfig_t		gl_config;
glstate_t		gl_state;

cvar_t	*gl_normalmaps;
cvar_t	*gl_bspnormalmaps;
cvar_t  *gl_shadowmaps;
cvar_t	*gl_fog;

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int		r_visframecount;	// bumped when going to a new PVS
int		r_framecount;		// used for dlight push checking
int		r_shadowmapcount;	// number of shadowmaps rendered this frame

// performance counters for r_speeds reports
int last_c_brush_polys, c_brush_polys;
int last_c_alias_polys, c_alias_polys;
int c_flares;
int c_grasses;
int c_beams;
extern int c_vbo_batches;
extern int c_visible_lightmaps;
extern int c_visible_textures;

float		v_blend[4];			// final blending color

float		r_farclip, r_farclip_min, r_farclip_bias = 256.0f;

void GL_Strings_f( void );

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;
int		r_origin_leafnum;

float	r_world_matrix[16];
float	r_project_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t  *r_wave; // Water waves

cvar_t	*r_shadowmapscale;

cvar_t	*r_overbrightbits;

cvar_t	*gl_vlights;

cvar_t	*gl_nosubimage;

cvar_t	*gl_log;
cvar_t	*gl_bitdepth;
cvar_t	*gl_drawbuffer;
cvar_t	*gl_driver;
cvar_t	*gl_lightmap;
cvar_t	*gl_mode;
cvar_t	*gl_dynamic;
cvar_t	*gl_modulate;
cvar_t	*gl_nobind;
cvar_t	*gl_picmip;
cvar_t	*gl_skymip;
cvar_t	*gl_showtris;
cvar_t	*gl_showpolys;
cvar_t	*gl_finish;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_polyblend;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_texturealphamode;
cvar_t	*gl_texturesolidmode;
cvar_t	*gl_lockpvs;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
cvar_t  *vid_contrast;
cvar_t	*vid_ref;

cvar_t *r_anisotropic;
cvar_t *r_alphamasked_anisotropic;
cvar_t *r_ext_max_anisotropy;

cvar_t	*r_shaders;
cvar_t	*r_bloom;
cvar_t	*r_lensflare;
cvar_t	*r_lensflare_intens;
cvar_t	*r_drawsun;
cvar_t	*r_lightbeam;
cvar_t	*r_godrays;
cvar_t  *r_godray_intensity;
cvar_t	*r_optimize;

cvar_t	*r_lightmapfiles;

qboolean	map_fog;

cvar_t	*con_font;

cvar_t	*r_minimap_size;
cvar_t	*r_minimap_zoom;
cvar_t	*r_minimap_style;
cvar_t	*r_minimap;

cvar_t	*sys_affinity;
cvar_t	*sys_priority;

cvar_t	*gl_screenshot_type;
cvar_t	*gl_screenshot_jpeg_quality;

//no blood
extern cvar_t *cl_noblood;

//first time running game
cvar_t	*r_firstrun;

//for testing
cvar_t  *r_test;
cvar_t	*r_tracetest;

//ODE initialization error check
int r_odeinit_success; // 0 if dODEInit2() fails, 1 otherwise.

//fog script stuff
struct r_fog
{
	float red;
	float green;
	float blue;
	float start;
	float end;
	float density;
} fog;
unsigned r_weather;
unsigned r_nosun;
float r_sunX;
float r_sunY;
float r_sunZ;

/*
=================
R_ReadFogScript
=================
*/

void R_ReadFogScript( char *config_file )
{
	FILE *fp;
	int length;
	char a_string[128];
	char *buffer;
	char *s;
	size_t result;	

	if((fp = fopen(config_file, "rb" )) == NULL)
	{
		return;
	}

	length = FS_filelength( fp );

	buffer = malloc( length + 1 );
	if ( buffer != NULL )
	{
		buffer[length] = 0;
		result = fread( buffer, length, 1, fp );
		if ( result == 1 )
		{
			s = buffer;

			strcpy( a_string, COM_Parse( &s ) );
			fog.red = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			fog.green = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			fog.blue = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			fog.start = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			fog.end = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			fog.density = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_weather = atoi(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_nosun = atoi(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_sunX = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_sunY = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_sunZ = atof(a_string);

			if(fog.density > 0)
				map_fog = true;

		}
		else
		{
			Com_DPrintf("R_ReadFogScript: read fail: %s\n", config_file);
		}
		free( buffer );
	}
	fclose( fp );
	
	if (gl_fog->integer < 1)
	{
		map_fog = false;
		r_weather = false;
	}

	return;
}

/*
=================
R_ReadMusicScript
=================
*/

//to do - read in secondary music location(for CTF music shift)
void R_ReadMusicScript( char *config_file )
{
	FILE *fp;
	int length;
	char *buffer;
	char *s;
	size_t result;

	if((fp = fopen(config_file, "rb" )) == NULL)
	{
		return;
	}

	length = FS_filelength( fp );

	buffer = malloc( length + 1 );
	if ( buffer != NULL )
	{
		result = fread( buffer, length, 1, fp );
		if ( result == 1 )
		{
			buffer[length] = 0;
			s = buffer;
			strcpy( map_music, COM_Parse( &s ) );
			map_music[length] = 0; //clear any possible garbage
		}
		else
		{
			Com_DPrintf("R_ReadMusicScript: read fail: %s\n", config_file);
		}

		free( buffer );
	}
	fclose( fp );

	return;
}

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;
	cplane_t *p;

	if (r_nocull->integer)
		return false;

	for (i=0,p=frustum ; i<4; i++,p++)
	{
		switch (p->signbits)
		{
		case 0:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 1:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 2:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 3:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 4:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 5:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 6:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 7:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		default:
			return false;
		}
	}

	return false;
}

/*
=================
R_CullOrigin

Returns true if the origin is completely outside the frustom
=================
*/

qboolean R_CullOrigin(vec3_t origin)
{
	int i;

	for (i = 0; i < 4; i++)
		if (BOX_ON_PLANE_SIDE(origin, origin, &frustum[i]) == 2)
			return true;
	return false;
}

qboolean R_CullSphere( const vec3_t centre, const float radius, const int clipflags )
{
	int		i;
	cplane_t *p;

	if (r_nocull->value)
		return false;

	for (i=0,p=frustum ; i<4; i++,p++)
	{
		if ( !(clipflags & (1<<i)) ) {
			continue;
		}

		if ( DotProduct ( centre, p->normal ) - p->dist <= -radius )
			return true;
	}

	return false;
}

// should be able to handle every mesh type
void R_RotateForEntity (entity_t *e)
{
    qglTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    qglRotatef (e->angles[YAW],		0, 0, 1);
    // pitch and roll are handled by IQM_AnimateFrame. 
    if (e->model == NULL || (e->flags & RF_WEAPONMODEL) || e->model->type != mod_iqm)
    {
		qglRotatef (e->angles[PITCH],	0, 1, 0);
		qglRotatef (e->angles[ROLL],	1, 0, 0);
	}
}

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel (void)
{
	vec3_t	shadelight;
	int		i;

	if ( currententity->flags & RF_FULLBRIGHT )
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	else
		R_LightPoint (currententity->origin, shadelight, true);

    qglPushMatrix ();
	R_RotateForEntity (currententity);

	qglDisable (GL_TEXTURE_2D);
	qglColor3fv (shadelight);

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		qglVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	qglEnd ();

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		qglVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	qglEnd ();

	qglColor3f (1,1,1);
	qglPopMatrix ();
	qglEnable (GL_TEXTURE_2D);
}

#include "r_lodcalc.h"

/*
=============
R_DrawEntitiesOnList
=============
*/
extern cvar_t *cl_simpleitems;
void R_DrawEntitiesOnList (void)
{
	int		i;
	rscript_t	*rs = NULL;
	vec3_t	dist;

	if (!r_drawentities->integer)
		return;

	if ( !r_odeinit_success )
	{ // ODE init failed, force ragdolls off
		r_ragdolls = Cvar_ForceSet("r_ragdolls", "0");
	}

	// draw non-transparent first
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// transluscent
		
		if (currententity->model && r_shaders->integer)
		{
			rs=(rscript_t *)currententity->model->script;

			//custom player skin (must be done here)
			if (currententity->skin)
			{
			    rs = currententity->skin->script;
                if(rs)
                    RS_ReadyScript(rs);
            }

			if (rs)
				currententity->script = rs;
			else
				currententity->script = NULL;
		}

		currentmodel = currententity->model;
		
		if (cl_simpleitems->integer && currentmodel && currentmodel->simple_texnum)
			continue;

		//get distance
		VectorSubtract(r_origin, currententity->origin, dist);		
		
		//set lod if available
		if(VectorLength(dist) > LOD_DIST*2.0)
		{
			if(currententity->lod2)
				currentmodel = currententity->lod2;
		}
		else if(VectorLength(dist) > LOD_DIST)
		{
			if(currententity->lod1)
				currentmodel = currententity->lod1;
		}

		if (!currentmodel)
		{
			R_DrawNullModel ();
			continue;
		}
		switch (currentmodel->type)
		{
		    case mod_md2:
		    case mod_iqm:
		        R_Mesh_Draw ();
				break;
			case mod_brush:
				R_DrawBrushModel ();
				break;
			default:
				Com_Error(ERR_DROP, "Bad modeltype");
				break;
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	qglDepthMask (0);		// no z writes
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid

		if (currententity->model && r_shaders->integer)
		{
			rs=(rscript_t *)currententity->model->script;

			//custom player skin (must be done here)
			if (currententity->skin)
			{
                rs = currententity->skin->script;
                if(rs)
                    RS_ReadyScript(rs);
            }

			if (rs)
				currententity->script = rs;
			else
				currententity->script = NULL;
		}

		currentmodel = currententity->model;
		fadeShadow = 1.0;

		if (!currentmodel)
		{
			R_DrawNullModel ();
			continue;
		}
		switch (currentmodel->type)
		{
		    case mod_md2:
		    case mod_iqm:
		        R_Mesh_Draw ();
				break;
			case mod_brush:
				R_DrawBrushModel ();
				break;
			default:
				Com_Error (ERR_DROP, "Bad modeltype");
				break;
		}
	}
	qglDepthMask (1);		// back to writing
}

void R_DrawViewEntitiesOnList (void)
{
	int		i;
	rscript_t	*rs = NULL;

	if (!r_drawentities->integer)
		return;

	if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// draw non-transparent first
	for (i=0 ; i<r_newrefdef.num_viewentities ; i++)
	{
		currententity = &r_newrefdef.viewentities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// transluscent

		if (currententity->model && r_shaders->integer)
		{
			rs=(rscript_t *)currententity->model->script;

			//custom player skin (must be done here)
			if (currententity->skin)
			{
                rs = currententity->skin->script;
                if(rs)
                    RS_ReadyScript(rs);
            }

			if (rs)
				currententity->script = rs;
			else
				currententity->script = NULL;
		}

		currentmodel = currententity->model;

		if (!currentmodel)
		{
			R_DrawNullModel ();
			continue;
		}
		switch (currentmodel->type)
		{
		case mod_md2:
		case mod_iqm:
		    R_Mesh_Draw ();
			break;
		default:
			Com_Error(ERR_DROP, "Bad modeltype");
			break;
		}
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	qglDepthMask (0);		// no z writes
	for (i=0 ; i<r_newrefdef.num_viewentities ; i++)
	{
		currententity = &r_newrefdef.viewentities[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid

		if (currententity->model && r_shaders->integer)
		{
			rs=(rscript_t *)currententity->model->script;

			//custom player skin (must be done here)
			if (currententity->skin)
			{
                rs = currententity->skin->script;
                if(rs)
                    RS_ReadyScript(rs);
            }

			if (rs)
				currententity->script = rs;
			else
				currententity->script = NULL;
		}

		currentmodel = currententity->model;

		if (!currentmodel)
		{
			R_DrawNullModel ();
			continue;
		}
		switch (currentmodel->type)
		{
		case mod_md2:
		case mod_iqm:
		    R_Mesh_Draw ();
			break;
		default:
			Com_Error (ERR_DROP, "Bad modeltype");
			break;
		}
	}
	qglDepthMask (1);		// back to writing
}

void R_DrawTerrain (void)
{
	int		i;
	rscript_t	*rs = NULL;
	vec3_t	dist;

	if (!r_drawworld->integer)
		return;

	for (i=0 ; i<num_terrain_entities ; i++)
	{
		currententity = &terrain_entities[i];
		
		if (currententity->model && r_shaders->integer)
		{
			rs=(rscript_t *)currententity->model->script;

			//custom player skin (must be done here)
			if (currententity->skin)
			{
			    rs = currententity->skin->script;
                if(rs)
                    RS_ReadyScript(rs);
            }

			if (rs)
				currententity->script = rs;
			else
				currententity->script = NULL;
		}

		currentmodel = currententity->model;
		
		//get distance
		VectorSubtract(r_origin, currententity->origin, dist);		
		
		//set lod if available
		if(VectorLength(dist) > LOD_DIST*2.0)
		{
			if(currententity->lod2)
				currentmodel = currententity->lod2;
		}
		else if(VectorLength(dist) > LOD_DIST)
		{
			if(currententity->lod1)
				currentmodel = currententity->lod1;
		}

		if (!currentmodel)
		{
			R_DrawNullModel ();
			continue;
		}
		
		// TODO: maybe we don't actually want to assert this?
		assert (currentmodel->type == mod_terrain);
		
		R_Mesh_Draw ();
	}
	
	// TODO: will these models ever be transparent?
	
	for (i=0 ; i<num_rock_entities ; i++)
	{
		currententity = &rock_entities[i];
		
		if (currententity->model && r_shaders->integer)
		{
			rs=(rscript_t *)currententity->model->script;

			//custom player skin (must be done here)
			if (currententity->skin)
			{
			    rs = currententity->skin->script;
                if(rs)
                    RS_ReadyScript(rs);
            }

			if (rs)
				currententity->script = rs;
			else
				currententity->script = NULL;
		}

		currentmodel = currententity->model;
		
		//get distance
		VectorSubtract(r_origin, currententity->origin, dist);
		
		//cull very distant rocks
		if (VectorLength (dist) > LOD_DIST*8.0)
			continue;
		
		//set lod if available
		if(VectorLength(dist) > LOD_DIST*2.0)
		{
			if(currententity->lod2)
				currentmodel = currententity->lod2;
		}
		else if(VectorLength(dist) > LOD_DIST)
		{
			if(currententity->lod1)
				currentmodel = currententity->lod1;
		}

		if (!currentmodel)
		{
			R_DrawNullModel ();
			continue;
		}
		
		R_Mesh_Draw ();
	}
}

extern int r_drawing_fbeffect;
extern int	r_fbFxType;
extern float r_fbeffectTime;
extern cvar_t *cl_paindist;
/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!gl_polyblend->integer)
		return;
	if (!v_blend[3])
		return;

	if(!r_drawing_fbeffect && cl_paindist->integer) {
		if(v_blend[0] > 2*v_blend[1] && v_blend[0] > 2*v_blend[2]) {
			r_drawing_fbeffect = true;
			r_fbFxType = 2; //FLASH DISTORTION
			r_fbeffectTime = rs_realtime;
		}
	}

	qglDisable (GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);

	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho (0, 1, 1, 0, -99999, 99999);

	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

	qglColor4fv (v_blend);

	qglBegin (GL_TRIANGLES);
	qglVertex2f (-5, -5);
	qglVertex2f (10, -5);
	qglVertex2f (-5, 10);
	qglEnd ();

	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	qglEnable (GL_ALPHA_TEST);

	qglColor4f(1,1,1,1);
}

//=======================================================================

int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_newrefdef.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_newrefdef.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_newrefdef.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_newrefdef.fov_y / 2 ) );

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
void R_SetupFrame (void)
{
	int i;
	mleaf_t *leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.vieworg, r_origin);

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);

// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		r_viewleaf = leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_origin_leafnum = CM_PointLeafnum (r_origin);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			r_viewleaf2 = leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] += 16;
			r_viewleaf2 = leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i=0 ; i<4 ; i++)
		v_blend[i] = r_newrefdef.blend[i];

	c_brush_polys = 0;
	c_alias_polys = 0;

}

void MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	xmin += -( 2 * gl_state.camera_separation ) / zNear;
	xmax += -( 2 * gl_state.camera_separation ) / zNear;

	qglFrustum( xmin, xmax, ymin, ymax, zNear, zFar );

}



/*
=============
R_SetupViewport
=============
*/
void R_SetupViewport (void)
{
	int		x, y, w, h;

	// The viewport info in r_newrefdef is constructed with the upper left 
	// corner as the origin, whereas glViewport treats the lower left corner
	// as the origin. So we have to do some math to fix the y-coordinates.
	
	x = r_newrefdef.x;
	w = r_newrefdef.width;
	y = vid.height - r_newrefdef.y - r_newrefdef.height;
	h = r_newrefdef.height;
	
	qglViewport (x, y, w, h);	// MPO : note this happens every frame interestingly enough
}



/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	
	R_SetupViewport ();

	//
	// set up projection matrix
	//
    screenaspect = (float)r_newrefdef.width/r_newrefdef.height;
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();

	if(r_newrefdef.fov_y < 90)
		MYgluPerspective (r_newrefdef.fov_y,  screenaspect,  4,  128000);
	else
		MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4 * 74 / r_newrefdef.fov_y, 15000); //Phenax

	qglCullFace(GL_FRONT);

	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

    qglRotatef (-90, 1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

    qglRotatef (-r_newrefdef.viewangles[2],  1, 0, 0);
	qglRotatef (-r_newrefdef.viewangles[0],  0, 1, 0);
	qglRotatef (-r_newrefdef.viewangles[1],  0, 0, 1);
	qglTranslatef (-r_newrefdef.vieworg[0],  -r_newrefdef.vieworg[1],  -r_newrefdef.vieworg[2]);

	qglGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);
	qglGetFloatv(GL_PROJECTION_MATRIX, r_project_matrix);
	qglGetIntegerv(GL_VIEWPORT, (int *) r_viewport);

	//
	// set drawing parms
	//

	if (gl_cull->integer)
		qglEnable(GL_CULL_FACE);

	qglDisable(GL_BLEND);

	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear
=============
*/
extern cvar_t *info_spectator;
extern cvar_t *cl_add_blend;
extern qboolean have_stencil;
void R_Clear (void)
{
	qglClearColor(0,0,0,1);
	if (gl_clear->integer)
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else if (!cl_add_blend->integer && info_spectator->integer && (CM_PointContents(r_newrefdef.vieworg, 0) & CONTENTS_SOLID))
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //out of map
	else
		qglClear (GL_DEPTH_BUFFER_BIT);

	gldepthmin = 0;
	gldepthmax = 1;
	qglDepthFunc (GL_LEQUAL);

	qglDepthRange (gldepthmin, gldepthmax);

	 //our shadow system uses a combo of shadmaps and stencil volumes.
    if (have_stencil && gl_shadowmaps->integer) {

        qglClearStencil(0);
        qglClear(GL_STENCIL_BUFFER_BIT);
    }
}

void R_Flash( void )
{
	R_PolyBlend ();
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/

static void R_DrawTerrainTri (const vec_t *verts[3], const vec3_t normal, qboolean does_intersect)
{
	int i;
	vec3_t up;
	
	VectorSet (up, 0, 0, 1);
	
	qglDisable (GL_TEXTURE_2D);
	qglDisable (GL_DEPTH_TEST);
	if (does_intersect)
	{
		qglColor4f (1, 0, 0, 1);
		qglLineWidth (5.0);
	}
	else if (DotProduct (normal, up) < 0)
	{
		qglColor4f (0, 0, 1, 1);
		qglLineWidth (2.0);
	}
	else
	{
		qglColor4f (0, 0, 0, 1);
		qglLineWidth (1.0);
	}
	qglBegin (GL_LINE_LOOP);
	for (i = 0; i < 3; i++)
		qglVertex3fv (verts[i]);
	qglEnd ();
	qglEnable (GL_DEPTH_TEST);
	qglEnable (GL_TEXTURE_2D);
}

extern void CM_TerrainDrawIntersecting (vec3_t start, vec3_t dir, void (*do_draw) (const vec_t *verts[3], const vec3_t normal, qboolean does_intersect));

void R_SetupFog (float distance_boost)
{
	GLfloat colors[4] = {(GLfloat) fog.red, (GLfloat) fog.green, (GLfloat) fog.blue, (GLfloat) 0.1};
	
	if(map_fog)
	{
		qglFogi(GL_FOG_MODE, GL_LINEAR);
		qglFogfv(GL_FOG_COLOR, colors);
		qglFogf(GL_FOG_START, fog.start * distance_boost);
		qglFogf(GL_FOG_END, fog.end * distance_boost);
		qglFogf(GL_FOG_DENSITY, fog.density);
		qglEnable(GL_FOG);
	}
}

void R_RenderView (refdef_t *fd)
{
	vec3_t forward;

	numRadarEnts = 0;

	if (r_norefresh->integer)
		return;

	r_newrefdef = *fd;

	//shadowmaps
	if(gl_shadowmaps->integer) {

		qglEnable(GL_DEPTH_TEST);
		qglClearColor(0,0,0,1.0f);

		qglEnable(GL_CULL_FACE);

		qglHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);

		R_DrawDynamicCaster();

		R_DrawVegetationCaster();

		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);

		//Enabling color write (previously disabled for light POV z-buffer rendering)
		qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		Com_Error (ERR_DROP, "R_RenderView: NULL worldmodel");
	
	// init r_speeds counters
	last_c_brush_polys = c_brush_polys;
	last_c_alias_polys = c_alias_polys;
	c_brush_polys = 0;
	c_alias_polys = 0;
	c_flares = 0;
	c_grasses = 0;
	c_beams = 0;
	c_vbo_batches = 0;

	R_PushDlights ();

	R_SetupFrame ();

	R_SetFrustum ();
	
	R_MarkWorldSurfs ();	// done here so we know if we're in water
	
	if (gl_finish->integer)
		qglFinish ();
	
	// OpenGL calls come after here

	R_SetupGL ();

	R_SetupFog (1);

	R_DrawWorldSurfs ();
	
	R_DrawTerrain ();

	if(r_lensflare->integer)
		R_RenderFlares ();

	R_DrawEntitiesOnList ();
			
	R_DrawVegetationSurface ();

	R_DrawSimpleItems ();

	R_CastShadow();

	R_RenderAllRagdolls(); //move back ahead of r_castshadow when we figure out shadow jitter bug
	
	R_DrawViewEntitiesOnList ();

	R_DrawAlphaSurfaces ();

	if (r_lightbeam->integer)
		R_DrawBeamSurface ();

	R_DrawParticles ();

	if(gl_mirror->integer) 
	{
		int ms = Sys_Milliseconds ();
		if (	ms < r_newrefdef.last_mirrorupdate_time || 
				(ms-r_newrefdef.last_mirrorupdate_time) >= 16)
		{
			GL_SelectTexture (0);
			GL_Bind (r_mirrortexture->texnum);
			qglCopyTexSubImage2D(GL_TEXTURE_2D, 0,
						0, 0, 0, r_mirrortexture->upload_height/2, 
						r_mirrortexture->upload_width, 
						r_mirrortexture->upload_height);
			r_newrefdef.last_mirrorupdate_time = ms;
		}
	}


	R_BloomBlend( &r_newrefdef );//BLOOMS

	R_RenderSun();
	
	AngleVectors (r_newrefdef.viewangles, forward, NULL, NULL);
	if (gl_showpolys->integer)
		CM_TerrainDrawIntersecting (r_origin, forward, R_DrawTerrainTri);
	if (r_tracetest->integer > 0)
	{
		int		i;
		vec3_t	targ;
		VectorMA (r_origin, 8192, forward, targ);
		for (i = 0; i < r_tracetest->integer; i++)
			CM_BoxTrace (r_origin, targ, vec3_origin, vec3_origin, r_worldmodel->firstnode, MASK_OPAQUE);
	}

	R_GLSLPostProcess();

	R_DrawVehicleHUD();

	R_Flash();

	if(map_fog)
		qglDisable(GL_FOG);

	R_DrawRadar();
	
	*fd = r_newrefdef;
}

void	R_SetGL2D (void)
{
	// set 2D virtual screen size
	qglViewport (0,0, vid.width, vid.height);
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho  (0, vid.width, vid.height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity ();
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);
	qglEnable (GL_ALPHA_TEST);
	qglColor4f (1,1,1,1);
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/

void R_RenderFrame (refdef_t *fd)
{
	R_RenderView( fd );

	R_SetGL2D ();
}

void R_RenderFramePlayerSetup( refdef_t *rfdf )
{

	numRadarEnts = 0;

	r_newrefdef = *rfdf;

	R_SetupFrame();
	R_SetFrustum();
	R_SetupGL();
	R_DrawEntitiesOnList();

	R_SetGL2D();

}

void R_Register( void )
{

	con_font = Cvar_Get ("con_font", "default", CVAR_ARCHIVE);

	r_lefthand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE | CVARDOC_INT);
	Cvar_Describe (r_lefthand, "0 means show gun on the right, 1 means show gun on the left, 2 means hide the gun altogether.");
	r_norefresh = Cvar_Get ("r_norefresh", "0", 0);
	r_fullbright = Cvar_Get ("r_fullbright", "0", 0);
	r_drawentities = Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = Cvar_Get ("r_drawworld", "1", 0);
	r_novis = Cvar_Get ("r_novis", "0", 0);
	r_nocull = Cvar_Get ("r_nocull", "0", 0);
	r_lerpmodels = Cvar_Get ("r_lerpmodels", "1", 0);

	r_wave = Cvar_Get ("r_wave", "2", CVAR_ARCHIVE); // Water waves

	gl_nosubimage = Cvar_Get( "gl_nosubimage", "0", 0 );

	gl_modulate = Cvar_Get ("gl_modulate", "2", CVAR_ARCHIVE|CVARDOC_INT );
	Cvar_Describe (gl_modulate, "Brightness setting. Higher means brighter.");
	gl_log = Cvar_Get( "gl_log", "0", 0 );
	gl_bitdepth = Cvar_Get( "gl_bitdepth", "0", 0 );
	gl_mode = Cvar_Get( "gl_mode", "3", CVAR_ARCHIVE );
	gl_lightmap = Cvar_Get ("gl_lightmap", "0", 0);
	gl_nobind = Cvar_Get ("gl_nobind", "0", 0);
	gl_picmip = Cvar_Get ("gl_picmip", "0", CVAR_ARCHIVE|CVARDOC_INT);
	Cvar_Describe (gl_picmip, "Texture detail. 0 means full detail. Each higher setting has 1/4 less detail.");
	gl_skymip = Cvar_Get ("gl_skymip", "0", 0);
	gl_showtris = Cvar_Get ("gl_showtris", "0", 0);
	gl_showpolys = Cvar_Get ("gl_showpolys", "0", CVARDOC_INT);
	Cvar_Describe (gl_showpolys, "Useful tool for mappers. 1 means show world polygon outlines for visible surfaces. 2 means show outlines for all surfaces in the PVS, even if they are hidden. FIXME: currently broken.");
	gl_finish = Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE|CVARDOC_BOOL);
	Cvar_Describe (gl_finish, "Waits for graphics driver to finish drawing each frame before drawing the next one. Hurts performance but may improve smoothness on very low-end machines.");
	gl_clear = Cvar_Get ("gl_clear", "0", CVARDOC_BOOL);
	gl_cull = Cvar_Get ("gl_cull", "1", CVARDOC_BOOL);
	Cvar_Describe (gl_cull, "Avoid rendering anything that's off the edge of the screen. Good for performance, recommend leaving it on.");
	gl_polyblend = Cvar_Get ("gl_polyblend", "1", 0);

// OPENGL_DRIVER defined by in config.h
	gl_driver = Cvar_Get( "gl_driver", OPENGL_DRIVER, 0 );

	gl_texturemode = Cvar_Get( "gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	gl_texturealphamode = Cvar_Get( "gl_texturealphamode", "default", CVAR_ARCHIVE );
	gl_texturesolidmode = Cvar_Get( "gl_texturesolidmode", "default", CVAR_ARCHIVE );
	gl_lockpvs = Cvar_Get( "gl_lockpvs", "0", 0 );

	gl_drawbuffer = Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );
	gl_swapinterval = Cvar_Get( "gl_swapinterval", "1", CVAR_ARCHIVE|CVARDOC_BOOL );
	Cvar_Describe (gl_swapinterval, "Sync to Vblank. Eliminates \"tearing\" effects, but it can hurt framerates.");

	r_shaders = Cvar_Get ("r_shaders", "1", CVAR_ARCHIVE|CVARDOC_BOOL);

	r_overbrightbits = Cvar_Get( "r_overbrightbits", "2", CVAR_ARCHIVE );

	gl_mirror = Cvar_Get("gl_mirror", "1", CVAR_ARCHIVE|CVARDOC_BOOL);

	vid_fullscreen = Cvar_Get( "vid_fullscreen", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	vid_gamma = Cvar_Get( "vid_gamma", "1.0", CVAR_ARCHIVE );
	vid_contrast = Cvar_Get( "vid_contrast", "1.0", CVAR_ARCHIVE);
	//TODO: remove, unless we decide to add GL ES support or something.
	vid_ref = Cvar_Get( "vid_ref", "gl", CVAR_ARCHIVE|CVAR_ROM ); 

	gl_vlights = Cvar_Get("gl_vlights", "1", CVAR_ARCHIVE);

	gl_normalmaps = Cvar_Get("gl_normalmaps", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	gl_bspnormalmaps = Cvar_Get("gl_bspnormalmaps", "0", CVAR_ARCHIVE|CVARDOC_BOOL);
	gl_shadowmaps = Cvar_Get("gl_shadowmaps", "0", CVAR_ARCHIVE|CVARDOC_BOOL);
	gl_fog = Cvar_Get ("gl_fog", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	Cvar_Describe (gl_fog, "Fog and weather effects.");

	r_shadowmapscale = Cvar_Get( "r_shadowmapscale", "1", CVAR_ARCHIVE );
	r_shadowcutoff = Cvar_Get( "r_shadowcutoff", "880", CVAR_ARCHIVE );

	r_lensflare = Cvar_Get( "r_lensflare", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_lensflare_intens = Cvar_Get ("r_lensflare_intens", "3", CVAR_ARCHIVE|CVARDOC_INT);
	r_drawsun =	Cvar_Get("r_drawsun", "2", CVAR_ARCHIVE);
	r_lightbeam = Cvar_Get ("r_lightbeam", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_godrays = Cvar_Get ("r_godrays", "0", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_godray_intensity = Cvar_Get ("r_godray_intensity", "1.0", CVAR_ARCHIVE);
	r_optimize = Cvar_Get ("r_optimize", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	Cvar_Describe (r_optimize, "Skip BSP recursion unless you move. Good for performance, recommend leaving it on.");
	
	r_lightmapfiles = Cvar_Get("r_lightmapfiles", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	Cvar_Describe (r_lightmapfiles, "Enables the loading of .lightmap files, with more detailed light and shadow. Turn this off if video RAM is limited.");

	r_minimap_size = Cvar_Get ("r_minimap_size", "256", CVAR_ARCHIVE );
	r_minimap_zoom = Cvar_Get ("r_minimap_zoom", "1", CVAR_ARCHIVE );
	r_minimap_style = Cvar_Get ("r_minimap_style", "1", CVAR_ARCHIVE );
	r_minimap = Cvar_Get ("r_minimap", "0", CVAR_ARCHIVE|CVARDOC_BOOL );

	r_ragdolls = Cvar_Get ("r_ragdolls", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_ragdoll_debug = Cvar_Get("r_ragdoll_debug", "0", CVAR_ARCHIVE|CVARDOC_BOOL);

	sys_priority = Cvar_Get("sys_priority", "0", CVAR_ARCHIVE);
	sys_affinity = Cvar_Get("sys_affinity", "1", CVAR_ARCHIVE);

	gl_screenshot_type = Cvar_Get("gl_screenshot_type", "jpeg", CVAR_ARCHIVE|CVARDOC_STR);
	gl_screenshot_jpeg_quality = Cvar_Get("gl_screenshot_jpeg_quality", "85", CVAR_ARCHIVE|CVARDOC_INT);

	r_firstrun = Cvar_Get("r_firstrun", "0", CVAR_ARCHIVE|CVARDOC_BOOL); //first time running the game
	Cvar_Describe (r_firstrun, "Set this to 0 if you want the game to auto detect your graphics settings next time you run it.");

	r_test = Cvar_Get("r_test", "0", CVAR_ARCHIVE); //for testing things
	r_tracetest = Cvar_Get("r_tracetest", "0", CVARDOC_INT); // BoxTrace performance test
	
	// FIXME HACK copied over from the video menu code. These are initialized
	// again elsewhere. TODO: work out any complications that may arise from
	// deleting these duplicate initializations.
	Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );
	Cvar_Get( "r_bloom_intensity", "0.5", CVAR_ARCHIVE);
	Cvar_Get( "r_overbrightbits", "2", CVAR_ARCHIVE);
	Cvar_Get( "vid_width", "640", CVAR_ARCHIVE);
	Cvar_Get( "vid_height", "400", CVAR_ARCHIVE);

	Cmd_AddCommand( "imagelist", GL_ImageList_f );
	Cmd_AddCommand( "screenshot", GL_ScreenShot_f );
	Cmd_AddCommand( "modellist", Mod_Modellist_f );
	Cmd_AddCommand( "gl_strings", GL_Strings_f );
}

/*

==================
R_SetMode
==================
*/
qboolean R_SetMode (void)
{
	rserr_t err;
	qboolean fullscreen;

	fullscreen = vid_fullscreen->integer;

	vid_fullscreen->modified = false;
	gl_mode->modified = false;

	if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->integer, fullscreen ) ) == rserr_ok )
	{
		gl_state.prev_mode = gl_mode->integer;
	}
	else
	{
		if ( err == rserr_invalid_fullscreen )
		{
			Cvar_SetValue( "vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			Com_Printf ("ref_gl::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->integer, false ) ) == rserr_ok )
				return true;
		}
		else if ( err == rserr_invalid_mode )
		{
			Cvar_SetValue( "gl_mode", gl_state.prev_mode );
			gl_mode->modified = false;
			Com_Printf ("ref_gl::R_SetMode() - invalid mode\n" );
		}

		// try setting it back to something safe
		if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_state.prev_mode, false ) ) != rserr_ok )
		{
			Com_Printf ("ref_gl::R_SetMode() - could not revert to safe mode\n" );
			return false;
		}
	}
	return true;
}

/*
===============
R_SetCompatibility
===============
*/

void R_SetCompatibility(void)
{
	Cmd_ExecuteString ("exec graphical_presets/compatibility.cfg");
	Cbuf_Execute ();
	Com_Printf("...autodetected MAX COMPATIBILITY game setting\n");
}

/*
===============
R_SetMaxPerformance
===============
*/

void R_SetMaxPerformance( void )
{
	Cmd_ExecuteString ("exec graphical_presets/maxperformance.cfg");
	Cbuf_Execute ();
	Com_Printf("...autodetected MAX PERFORMANCE game setting\n");
}

/*
===============
R_SetPerformance
===============
*/

void R_SetPerformance( void )
{
	Cmd_ExecuteString ("exec graphical_presets/performance.cfg");
	Cbuf_Execute ();
	Com_Printf("...autodetected PERFORMANCE game setting\n");
}

/*
===============
R_SetQuality
===============
*/

void R_SetQuality( void )
{
	Cmd_ExecuteString ("exec graphical_presets/quality.cfg");
	Cbuf_Execute ();
	Com_Printf("...autodetected QUALITY game setting\n");
}

/*
===============
R_SetMaxQuality
===============
*/

void R_SetMaxQuality( void )
{
	Cmd_ExecuteString ("exec graphical_presets/maxquality.cfg");
	Cbuf_Execute ();
	Com_Printf("...autodetected MAX QUALITY game setting\n");
}

#if defined WIN32_VARIANT
double CPUSpeed()
{


	DWORD BufSize = _MAX_PATH;
	DWORD dwMHz = _MAX_PATH;
	HKEY hKey;	// open the key where the proc speed is hidden:

	long lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		0,
		KEY_READ,
		&hKey);

	if(lError != ERROR_SUCCESS)
		return 0;

	// query the key:
	RegQueryValueEx(hKey, "~MHz", NULL, NULL, (LPBYTE) &dwMHz, &BufSize);
	return (double)dwMHz;

}
#endif

/*
===============
R_Init
===============
*/
int R_Init( void *hinstance, void *hWnd )
{
	int		err;
	int		j;
	extern float r_turbsin[256];

	for ( j = 0; j < 256; j++ )
	{
		r_turbsin[j] *= 0.5;
	}
	
	Draw_GetPalette ();

	R_Register();

	// initialize our QGL dynamic bindings
	if ( !QGL_Init( gl_driver->string ) )
	{
		QGL_Shutdown();
		Com_Printf ("ref_gl::R_Init() - could not load \"%s\"\n", gl_driver->string );
		return -1;
	}

	// initialize OS-specific parts of OpenGL
	if ( !GLimp_Init( hinstance, hWnd ) )
	{
		QGL_Shutdown();
		return -1;
	}
	
	// reset GL_BlendFunc state variables.
	gl_state.bFunc1 = gl_state.bFunc2 = -1;

	// set our "safe" modes
	gl_state.prev_mode = 3;

	// create the window and set up the context
	if ( !R_SetMode () )
	{
		QGL_Shutdown();
		Com_Printf ("ref_gl::R_Init() - could not R_SetMode()\n" );
		return -1;
	}

	// Initialise TrueType fonts
	if ( ! FNT_Initialise( ) ) {
		QGL_Shutdown( );
		Com_Printf( "ref_gl::R_Init() - could not initialise text drawing front-end\n" );
		return -1;
	}

	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = (const char*)qglGetString (GL_VENDOR);
	Com_Printf ("GL_VENDOR: %s\n", gl_config.vendor_string );
	gl_config.renderer_string = (const char*)qglGetString (GL_RENDERER);
	Com_Printf ("GL_RENDERER: %s\n", gl_config.renderer_string );
	gl_config.version_string = (const char*)qglGetString (GL_VERSION);
	Com_Printf ("GL_VERSION: %s\n", gl_config.version_string );
	gl_config.extensions_string = (const char*)qglGetString (GL_EXTENSIONS);
	Com_Printf ("GL_EXTENSIONS: %s\n", gl_config.extensions_string );

	/*
	** grab extensions
	*/
	if ( strstr( gl_config.extensions_string, "GL_EXT_compiled_vertex_array" ) ||
		 strstr( gl_config.extensions_string, "GL_SGI_compiled_vertex_array" ) )
	{
		Com_Printf ("...enabling GL_EXT_compiled_vertex_array\n" );
		qglLockArraysEXT = ( void * ) qwglGetProcAddress( "glLockArraysEXT" );
		qglUnlockArraysEXT = ( void * ) qwglGetProcAddress( "glUnlockArraysEXT" );
	}
	else
	{
		Com_Printf ("...GL_EXT_compiled_vertex_array not found\n" );
	}

#if defined WIN32_VARIANT
	if ( strstr( gl_config.extensions_string, "WGL_EXT_swap_control" ) )
	{
		qwglSwapIntervalEXT = ( BOOL (WINAPI *)(int)) qwglGetProcAddress( "wglSwapIntervalEXT" );
		Com_Printf ("...enabling WGL_EXT_swap_control\n" );
	}
	else
	{
		Com_Printf ("...WGL_EXT_swap_control not found\n" );
	}
#endif

	R_InitImageSubsystem();

	R_InitShadowSubsystem();

	R_LoadVBOSubsystem();

#if defined DARWIN_SPECIAL_CASE
	/*
	 * development workaround for Mac OS X / Darwin using X11
	 *  problems seen with 2.1 NVIDIA-1.6.18 when calling
	 *  glCreateProgramObjectARB()
	 * For now, just go with low settings.
	 */
	gl_state.fragment_program = false;
	gl_dynamic = Cvar_Get ("gl_dynamic", "0", CVAR_ARCHIVE);
	Cvar_SetValue("r_firstrun", 1);
	R_SetMaxPerformance();
	Com_Printf("...Development Workaround. Low game settings forced.\n");
#else

	//always do this check for ATI drivers - they are somewhat bugged in regards to shadowmapping and use of shadow2dproj command
	if(!strcmp(gl_config.vendor_string, "ATI Technologies Inc."))
		gl_state.ati = true;

	//load shader programs
	R_LoadGLSLPrograms();

	//if running for the very first time, automatically set video settings
	if(!r_firstrun->integer)
	{
		qboolean ati_nvidia = false;
		double CPUTotalSpeed = 4000.0; //default to this
		double OGLVer = atof(&gl_config.version_string[0]);
		//int OGLSubVer = atoi(&gl_config.version_string[2]);

#if defined WIN32_VARIANT
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);

		Com_Printf("...CPU: %4.2f Cores: %d\n", CPUSpeed(), sysInfo.dwNumberOfProcessors);

		CPUTotalSpeed = sysInfo.dwNumberOfProcessors * CPUSpeed();

#else
		FILE	*fp;
        char	res[128];
		int		cores;
		size_t	szrslt;
		int     irslt;
        fp = popen("/bin/cat /proc/cpuinfo | grep -c '^processor'","r");
        if ( fp == NULL )
        	goto cpuinfo_error;
        szrslt = fread(res, 1, sizeof(res)-1, fp);
        res[szrslt] = 0;
        pclose(fp);
        if ( !szrslt )
        	goto cpuinfo_error;
		cores = atoi( &res[0] );
		fp = popen("/bin/cat /proc/cpuinfo | grep '^cpu MHz'","r");
		if ( fp == NULL )
			goto cpuinfo_error;
        szrslt = fread(res, 1, sizeof(res)-1, fp);  // about 20 bytes/cpu
        res[szrslt] = 0;
		pclose(fp);
		if ( !szrslt )
			goto cpuinfo_error;
		irslt = sscanf( res, "cpu MHz : %lf", &CPUTotalSpeed );
		if ( !irslt )
			goto cpuinfo_error;
		Com_Printf("...CPU: %4.2f Cores: %d\n", CPUTotalSpeed, cores);
	    CPUTotalSpeed *= cores;
	    goto cpuinfo_exit;
cpuinfo_error:
		Com_Printf("...Reading /proc/cpuinfo failed.\n");
cpuinfo_exit:
#endif

		//check to see if we are using ATI or NVIDIA, otherwise, we don't want to
		//deal with high settings on offbrand GPU's like Intel or Unichrome
		if(!strcmp(gl_config.vendor_string, "ATI Technologies Inc.") || !strcmp(gl_config.vendor_string, "NVIDIA Corporation"))
			ati_nvidia = true;

		if(OGLVer < 2.1)
		{
			//weak GPU, set to maximum compatibility
			R_SetCompatibility();
		}
		else if(OGLVer >= 3)
		{
			//GPU is modern, check CPU
			if(CPUTotalSpeed > 3800.0 && ati_nvidia)
				R_SetMaxQuality();
			else
				R_SetQuality();
		}
		else 
		{
			if(CPUTotalSpeed > 3800.0 && ati_nvidia)
				R_SetQuality();
			else
				R_SetPerformance();
		}

		//never run again
		Cvar_SetValue("r_firstrun", 1);
	}
#endif

	GL_SetDefaultState();
	
	R_CheckFBOExtensions ();

	GL_InitImages ();
	Mod_Init ();
	R_InitParticleTexture ();
	Draw_InitLocal ();

	R_GenerateShadowFBO();
	VLight_Init();

	//Initialize ODE
	// ODE assert failures sometimes occur, this may or may not help.
	r_odeinit_success = dInitODE2(0);
	//ODE - clear out any ragdolls;
	if ( r_odeinit_success )
	{
		Com_Printf("...ODE initialized.\n");
		R_ClearAllRagdolls();
		Com_Printf("...Ragdolls initialized.\n");
	}
	else
	{
		Com_Printf("...ODE initialization failed.\n...Ragdolls are disabled.\n");
	}

	scr_playericonalpha = 0.0;

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		Com_Printf ("glGetError() = 0x%x\n", err);
	
	GL_InvalidateTextureState ();

	return 0;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{
	Cmd_RemoveCommand ("modellist");
	Cmd_RemoveCommand ("screenshot");
	Cmd_RemoveCommand ("imagelist");
	Cmd_RemoveCommand ("gl_strings");

	Mod_FreeAll ();
	
	R_VCShutdown();

	FNT_Shutdown( );
	GL_ShutdownImages ();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();

	/*
	** shutdown our QGL subsystem
	*/
	glDeleteObjectARB( g_vertexShader );
	glDeleteObjectARB( g_fragmentShader );
	glDeleteObjectARB( g_programObj );

	QGL_Shutdown();

	//Shutdown ODE
	// ODE might fail assert on close when not intialized
	if ( r_odeinit_success )
	{
		Com_DPrintf("Closing ODE\n");
		dCloseODE();
	}
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame( float camera_separation )
{

	gl_state.camera_separation = camera_separation;

	/*
	** change modes if necessary
	*/
	if ( gl_mode->modified || vid_fullscreen->modified )
	{	// FIXME: only restart if CDS is required
		cvar_t	*ref;

		ref = Cvar_Get ("vid_ref", "gl", 0);
		ref->modified = true;
	}

	if ( gl_log->modified )
	{
		GLimp_EnableLogging( gl_log->integer );
		gl_log->modified = false;
	}

	if ( gl_log->integer )
	{
		GLimp_LogNewFrame();
	}

	GLimp_BeginFrame( camera_separation );

	/*
	** go into 2D mode
	*/
	R_SetGL2D ();

	/*
	** draw buffer stuff
	*/
	if ( gl_drawbuffer->modified )
	{
		gl_drawbuffer->modified = false;

		if ( gl_state.camera_separation == 0 || !gl_state.stereo_enabled )
		{
			if ( Q_strcasecmp( gl_drawbuffer->string, "GL_FRONT" ) == 0 )
				qglDrawBuffer( GL_FRONT );
			else
				qglDrawBuffer( GL_BACK );
		}
	}

	/*
	** texturemode stuff
	*/
	if ( gl_texturemode->modified )
	{
		GL_TextureMode( gl_texturemode->string );
		gl_texturemode->modified = false;
	}

	if ( gl_texturealphamode->modified )
	{
		GL_TextureAlphaMode( gl_texturealphamode->string );
		gl_texturealphamode->modified = false;
	}

	if ( gl_texturesolidmode->modified )
	{
		GL_TextureSolidMode( gl_texturesolidmode->string );
		gl_texturesolidmode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear ();
}

/*
=============
R_AppActivate
=============
*/
void R_AppActivate( qboolean active )
{
	GLimp_AppActivate (active);
}

/*
=============
R_EndFrame
=============
*/
void R_EndFrame (void)
{
	 GLimp_EndFrame ();
}

/*
=============
R_SetPalette
=============
*/
unsigned r_rawpalette[256];

void R_SetPalette ( const unsigned char *palette)
{
	int		i;

	byte *rp = ( byte * ) r_rawpalette;

	if ( palette )
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = palette[i*3+0];
			rp[i*4+1] = palette[i*3+1];
			rp[i*4+2] = palette[i*3+2];
			rp[i*4+3] = 0xff;
		}
	}
	else
	{
		for ( i = 0; i < 256; i++ )
		{
			rp[i*4+0] = d_8to24table[i] & 0xff;
			rp[i*4+1] = ( d_8to24table[i] >> 8 ) & 0xff;
			rp[i*4+2] = ( d_8to24table[i] >> 16 ) & 0xff;
			rp[i*4+3] = 0xff;
		}
	}

	if ( qglClear && qglClearColor)
	{
		// only run this if we haven't uninitialised
		// OpenGL already
		qglClearColor (0,0,0,0);
		qglClear (GL_COLOR_BUFFER_BIT);
		qglClearColor (1,0, 0.5 , 0.5);
	}
}

/*
===============
R_FarClip
===============
*/
float R_FarClip( void )
{
	float farclip, farclip_dist;
	int i;
	vec_t mins[4];
	vec_t maxs[4];
	vec_t dist;

	farclip_dist = DotProduct( r_origin, vpn );

	if(r_farclip_min > 256.0f)
		farclip = farclip_dist + r_farclip_min;
	else
		farclip = farclip_dist + 256.0f;

	if( r_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL) ) {

		for (i = 0; i < 3; i++) {
			mins[i] = r_worldmodel->nodes[0].minmaxs[i];
			maxs[i] = r_worldmodel->nodes[0].minmaxs[3+i];
		}
		dist = (vpn[0] < 0 ? mins[0] : maxs[0]) * vpn[0] +
			(vpn[1] < 0 ? mins[1] : maxs[1]) * vpn[1] +
			(vpn[2] < 0 ? mins[2] : maxs[2]) * vpn[2];
		if( dist > farclip )
			farclip = dist;
	}

	if((farclip - farclip_dist + r_farclip_bias) > r_farclip)
		return ( farclip - farclip_dist + r_farclip_bias);
	else
		return r_farclip;
}

/*
=============
R_SetupProjectionMatrix
=============
*/
void R_SetupProjectionMatrix( refdef_t *rd, mat4x4_t m )
{
	double xMin, xMax, yMin, yMax, zNear, zFar;

	r_farclip = R_FarClip ();

	zNear = 4;
	zFar = r_farclip;

	yMax = zNear * tan( rd->fov_y * M_PI / 360.0 );
	yMin = -yMax;

	xMin = yMin * rd->width / rd->height;
	xMax = yMax * rd->width / rd->height;

	xMin += -( 2 * gl_state.camera_separation ) / zNear;
	xMax += -( 2 * gl_state.camera_separation ) / zNear;

	m[0] = (2.0 * zNear) / (xMax - xMin);
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = (2.0 * zNear) / (yMax - yMin);
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = (xMax + xMin) / (xMax - xMin);
	m[9] = (yMax + yMin) / (yMax - yMin);
	m[10] = -(zFar + zNear) / (zFar - zNear);
	m[11] = -1.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = -(2.0 * zFar * zNear) / (zFar - zNear);
	m[15] = 0.0f;
}
/*
=============
R_SetupModelviewMatrix
=============
*/
void R_SetupModelviewMatrix( refdef_t *rd, mat4x4_t m )
{
	Vector4Set( &m[0], 0, 0, -1, 0 );
	Vector4Set( &m[4], -1, 0, 0, 0 );
	Vector4Set( &m[8], 0, 1, 0, 0 );
	Vector4Set( &m[12], 0, 0, 0, 1 );

	Matrix4_Rotate( m, -rd->viewangles[2], 1, 0, 0 );
	Matrix4_Rotate( m, -rd->viewangles[0], 0, 1, 0 );
	Matrix4_Rotate( m, -rd->viewangles[1], 0, 0, 1 );
	Matrix4_Translate( m, -rd->vieworg[0], -rd->vieworg[1], -rd->vieworg[2] );
}

void R_TransformVectorToScreen( refdef_t *rd, vec3_t in, vec2_t out )
{
   mat4x4_t p, m;
   vec4_t temp, temp2;

   if( !rd || !in || !out )
      return;

   temp[0] = in[0];
   temp[1] = in[1];
   temp[2] = in[2];
   temp[3] = 1.0f;

   R_SetupProjectionMatrix( rd, p );
   R_SetupModelviewMatrix( rd, m );

   Matrix4_Multiply_Vector( m, temp, temp2 );
   Matrix4_Multiply_Vector( p, temp2, temp );

   if( !temp[3] )
      return;
   out[0] = rd->x + (temp[0] / temp[3] + 1.0f) * rd->width * 0.5f;
   out[1] = rd->y + (temp[1] / temp[3] + 1.0f) * rd->height * 0.5f;
}
