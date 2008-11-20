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
#include "r_local.h"
#include "r_refl.h"	// MPO
#include "r_script.h"
#include "vlights.h"

//****************************************************************************
// === jitwater -- fragment program extensions
PFNGLGENPROGRAMSARBPROC             qglGenProgramsARB            = NULL;
PFNGLDELETEPROGRAMSARBPROC          qglDeleteProgramsARB         = NULL;
PFNGLBINDPROGRAMARBPROC             qglBindProgramARB            = NULL;
PFNGLPROGRAMSTRINGARBPROC           qglProgramStringARB          = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC   qglProgramEnvParameter4fARB  = NULL;
PFNGLPROGRAMLOCALPARAMETER4FARBPROC qglProgramLocalParameter4fARB = NULL;
// jitwater ===
//****************************************************************************

//GLSL
PFNGLCREATEPROGRAMOBJECTARBPROC		glCreateProgramObjectARB	= NULL;	
PFNGLDELETEOBJECTARBPROC			glDeleteObjectARB			= NULL;
PFNGLUSEPROGRAMOBJECTARBPROC		glUseProgramObjectARB		= NULL;
PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObjectARB		= NULL;
PFNGLSHADERSOURCEARBPROC			glShaderSourceARB			= NULL;		
PFNGLCOMPILESHADERARBPROC			glCompileShaderARB			= NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC	glGetObjectParameterivARB	= NULL;
PFNGLATTACHOBJECTARBPROC			glAttachObjectARB			= NULL;
PFNGLGETINFOLOGARBPROC				glGetInfoLogARB				= NULL;
PFNGLLINKPROGRAMARBPROC				glLinkProgramARB			= NULL;
PFNGLGETUNIFORMLOCATIONARBPROC		glGetUniformLocationARB		= NULL;
PFNGLUNIFORM3FARBPROC				glUniform3fARB				= NULL;
PFNGLUNIFORM1IARBPROC				glUniform1iARB				= NULL;
PFNGLUNIFORM1FARBPROC				glUniform1fARB				= NULL;
PFNGLUNIFORMMATRIX3FVARBPROC		glUniformMatrix3fvARB		= NULL;

GLhandleARB g_programObj;
GLhandleARB g_vertexShader;
GLhandleARB g_fragmentShader;
GLuint      g_location_testTexture;

GLuint      g_location_eyePos;
GLuint		g_tangentSpaceTransform;
GLuint      g_location_heightTexture;
GLuint		g_location_lmTexture;
GLuint		g_location_normalTexture;
GLuint		g_heightMapID = 0;
GLuint		g_location_fog;
GLuint	    g_location_parallax;
GLuint		g_location_dynamic;
GLuint		g_location_specular;
GLuint	    g_location_surfaceColor;
GLuint	    g_location_lightPosition;
GLuint		g_location_lightColour;
GLuint	    g_location_lightCutoffSquared;
GLuint	    g_location_tangent;

void R_Clear (void);

viddef_t	vid;

int GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3;

model_t		*r_worldmodel;

float			gldepthmin, gldepthmax;

glconfig_t		gl_config;
glstate_t		gl_state;

cvar_t	*gl_normalmaps;
cvar_t	*gl_parallaxmaps;
cvar_t  *gl_specular;
cvar_t	*gl_arb_fragment_program; // jit
cvar_t	*gl_glsl_shaders;

entity_t	*currententity;
model_t	*currentmodel;

cplane_t	frustum[4];

int		r_visframecount;	// bumped when going to a new PVS
int		r_framecount;		// used for dlight push checking

int		c_brush_polys, c_alias_polys, c_flares;

float		v_blend[4];			// final blending color

gparticle_t gparticles[MAX_PARTICLES];
int			num_gparticles;

float		r_farclip, r_farclip_min, r_farclip_bias = 256.0f;

void GL_Strings_f( void );

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level
cvar_t  *r_wave; // Water waves
// Vic - begin

cvar_t	*r_overbrightbits;
cvar_t	*gl_ext_mtexcombine;

// Vic - end

cvar_t	*gl_nosubimage;

cvar_t	*gl_vertex_arrays;

cvar_t	*gl_particle_min_size;
cvar_t	*gl_particle_max_size;
cvar_t	*gl_particle_size;
cvar_t	*gl_particle_att_a;
cvar_t	*gl_particle_att_b;
cvar_t	*gl_particle_att_c;

cvar_t	*gl_ext_swapinterval;
cvar_t	*gl_ext_palettedtexture;
cvar_t	*gl_ext_multitexture;
cvar_t	*gl_ext_pointparameters;
cvar_t	*gl_ext_compiled_vertex_array;

cvar_t	*gl_log;
cvar_t	*gl_bitdepth;
cvar_t	*gl_drawbuffer;
cvar_t	*gl_driver;
cvar_t	*gl_lightmap;
cvar_t	*gl_shadows;
cvar_t	*gl_mode;
cvar_t	*gl_dynamic;
cvar_t	*gl_modulate;
cvar_t	*gl_nobind;
cvar_t	*gl_round_down;
cvar_t	*gl_picmip;
cvar_t	*gl_skymip;
cvar_t	*gl_showtris;
cvar_t	*gl_finish;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_polyblend;
cvar_t	*gl_flashblend;
cvar_t	*gl_playermip;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_texturealphamode;
cvar_t	*gl_texturesolidmode;
cvar_t	*gl_lockpvs;
cvar_t	*gl_rtlights;

cvar_t	*gl_3dlabs_broken;

cvar_t	*vid_fullscreen;
cvar_t	*vid_gamma;
cvar_t  *vid_contrast;
cvar_t	*vid_ref;

cvar_t	*r_shaders;
cvar_t	*r_bloom;
cvar_t	*r_lensflare;
cvar_t	*r_lensflare_intens;

qboolean	map_fog;

cvar_t	*gl_reflection;			// MPO	alpha transparency, 1.0 is full bright
cvar_t	*gl_reflection_debug;	// MPO	for debugging the reflection
cvar_t	*gl_reflection_max;		// MPO  max number of water reflections

cvar_t	*con_font;

cvar_t	*r_minimap_size; // GLOOM radar
cvar_t	*r_minimap_zoom;
cvar_t	*r_minimap_style;
cvar_t	*r_minimap;

cvar_t	*sys_affinity;
cvar_t	*sys_priority;

cvar_t	*gl_screenshot_type;
cvar_t	*gl_screenshot_jpeg_quality;

//no blood
extern cvar_t *cl_noblood;

//fog stuff
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

/*
=================
R_ReadFogScript

The linux "c" version of this function
=================
*/
#ifdef __unix__
void R_ReadFogScript(char config_file[128])
{
	FILE *fp;
	int k, length, rslt;
	char a_string[128];
	char *buffer;
	char *s;

	if((fp = fopen(config_file, "rb" )) == NULL)
	{
		return;
	}
	else
	{

		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		buffer = malloc( length + 1 );
		buffer[length] = 0;
		fread( buffer, length, 1, fp );
	}
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
	if ( fp != 0 )
	{
		fp = 0;
		free( buffer );
	}
	else
	{
		FS_FreeFile( buffer );
	}
	return;
}
#endif

/*
=================
R_ReadMusicScript

The linux "c" version of this function
=================
*/
#ifdef __unix__
void R_ReadMusicScript(char config_file[128])
{
	FILE *fp;
	int k, length, rslt;
	char a_string[128];
	char *buffer;
	char *s;

	if((fp = fopen(config_file, "rb" )) == NULL)
	{
		return;
	}
	else
	{

		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		buffer = malloc( length + 1 );
		fread( buffer, length, 1, fp );
		buffer[length] = 0;
	}
	s = buffer;

	strcpy( map_music, COM_Parse( &s ) );
	map_music[length] = 0; //clear any possible garbage

	if ( fp != 0 )
	{
		fp = 0;
		free( buffer );
	}
	else
	{
		FS_FreeFile( buffer );
	}
	return;
}
#endif

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

	if (r_nocull->value)
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


void R_RotateForEntity (entity_t *e)
{
    qglTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    qglRotatef (e->angles[1],  0, 0, 1);
    qglRotatef (-e->angles[0],  0, 1, 0);
    qglRotatef (-e->angles[2],  1, 0, 0);
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

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;
	rscript_t	*rs = NULL;
	vec3_t	dist;
	char	shortname[MAX_QPATH];

	if (!r_drawentities->value)
		return;

	// draw non-transparent first
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// transluscent

		if (currententity->model && r_shaders->value)
		{
			rs=(rscript_t *)currententity->model->script[currententity->skinnum];

			if (currententity->skin) { //custom player skin (must be done here)
				COM_StripExtension ( currententity->skin->name, shortname );
				rs = RS_FindScript(shortname);	
				if(rs)
					RS_ReadyScript(rs);
			}

			if (rs)
				currententity->script = rs;
			else
				currententity->script = NULL;
		}


		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;

			//get distance, set lod if available
			VectorSubtract(r_origin, currententity->origin, dist);
			if(VectorLength(dist) > 1000) {
				if(currententity->lod2)
					currentmodel = currententity->lod2;
			}
			else if(VectorLength(dist) > 500) {
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
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			default:
				Com_Error(ERR_DROP, "Bad modeltype");
				break;
			}
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

		if (currententity->model && r_shaders->value)
		{
			rs=(rscript_t *)currententity->model->script[currententity->skinnum];
			
			if (currententity->skin) { //custom player skin
				COM_StripExtension ( currententity->skin->name, shortname );
				rs = RS_FindScript(shortname);	
				if(rs)
					RS_ReadyScript(rs);
			}

			if (rs)
				currententity->script = rs;
			else
				currententity->script = NULL;
		}


		if ( currententity->flags & RF_BEAM )
		{
			R_DrawBeam( currententity );
		}
		else
		{
			currentmodel = currententity->model;

			if (!currentmodel)
			{
				R_DrawNullModel ();
				continue;
			}
			switch (currentmodel->type)
			{
			case mod_alias:
				R_DrawAliasModel (currententity);
				break;
			case mod_brush:
				R_DrawBrushModel (currententity);
				break;
			default:
				Com_Error (ERR_DROP, "Bad modeltype");
				break;
			}
		}
	}
	qglDepthMask (1);		// back to writing

}

/*
** GL_DrawParticles
**
*/
static float yawOrRoll;
void GL_DrawParticles( int num_particles, gparticle_t particles[], const unsigned colortable[768])
{
	const gparticle_t *p;
	int				i;
	vec3_t			corner[4], up, right, pup, pright, dir;
	float			scale, oldscale;
	byte			color[4], oldcolor[4];
	int			    oldtype;
	int				texnum, blenddst, blendsrc;
	float			*corner0 = corner[0];
	vec3_t move, delta, v;

	if ( !num_particles )
		return;

	qglDepthMask( GL_FALSE );	 	// no z buffering
	qglEnable( GL_BLEND);
	GL_TexEnv( GL_MODULATE );

	for ( p = particles, i=0, oldtype=-1 ; i < num_particles ; i++,p++)
	{

		if(p->type == PARTICLE_NONE || p->type == PARTICLE_WEATHER){
			if(r_weather == 1)
				texnum = r_raintexture->texnum; //rain
			else
				texnum = r_particletexture->texnum; //snow
			scale = 1;
			*(int *)color = colortable[p->color];
			blendsrc = GL_SRC_ALPHA;
			blenddst = GL_ONE;
		}
		else {
			texnum = p->texnum;
			blendsrc = p->blendsrc;
			blenddst = p->blenddst;
			scale = p->scale;
			*(int *)color = colortable[p->color];
		}

		color[3] = p->alpha*255;

		if (
			p->type != oldtype
			|| color[0] != oldcolor[0] || color[1] != oldcolor[1]
			|| color[2] != oldcolor[2] || color[3] != oldcolor[3] || scale != oldscale)
		{
			if ( oldtype != -1 )
			{
				qglEnd ();
			}
			if ( scale != 1 )
			{
				VectorScale (vup, scale, up);
				VectorScale (vright, scale, right);
			}
			else
			{
				VectorCopy (vup, up);
				VectorCopy (vright, right);
			}

			oldtype = p->type;
			oldscale = scale;
			oldcolor[3] = color[3];
			VectorCopy ( color, oldcolor );

			GL_Bind ( texnum );
			qglBlendFunc ( blendsrc, blenddst );
			if(p->type == PARTICLE_RAISEDDECAL) {
				qglColor4f(1,1,1,1);
			}
			else
				qglColor4ubv( color );
			qglBegin ( GL_QUADS );
		}

		if(p->type == PARTICLE_BEAM) {

			VectorSubtract(p->origin, p->angle, move);
			VectorNormalize(move);

			VectorCopy(move, pup);
			VectorSubtract(r_newrefdef.vieworg, p->angle, delta);
			CrossProduct(pup, delta, pright);
			VectorNormalize(pright);

			VectorScale(pright, 5*scale, pright);
			VectorScale(pup, 5*scale, pup);
		}
		else if(p->type == PARTICLE_DECAL || p->type == PARTICLE_RAISEDDECAL) {
			VectorCopy(p->angle, dir);
			AngleVectors(dir, NULL, right, up);
			VectorScale ( right, p->dist, pright );
			VectorScale ( up, p->dist, pup );
			VectorScale(pright, 5*scale, pright);
			VectorScale(pup, 5*scale, pup);

		}
		else if(p->type == PARTICLE_FLAT) {

			VectorCopy(r_newrefdef.viewangles, dir);

			dir[0] = -90;  // and splash particles horizontal by setting it
			AngleVectors(dir, NULL, right, up);

			if(p->origin[2] > r_newrefdef.vieworg[2]){  // it's above us
				VectorScale(right, 5*scale, pright);
				VectorScale(up, 5*scale, pup);
			}
			else {  // it's below us
				VectorScale(right, 5*scale, pright);
				VectorScale(up, -5*scale, pup);
			}
		}
		else if(p->type == PARTICLE_WEATHER){  // keep it vertical
			VectorCopy(r_newrefdef.viewangles, v);
			v[0] = 0;  // keep weather particles vertical by removing pitch
			AngleVectors(v, NULL, right, up);	
			VectorScale(right, 3*scale, pright);
			VectorScale(up, 3*scale, pup);
		}
		else if(p->type == PARTICLE_ROTATINGYAW || p->type == PARTICLE_ROTATINGROLL || p->type == PARTICLE_ROTATINGYAWMINUS){  // keep it vertical, and rotate on axis
			VectorCopy(r_newrefdef.viewangles, v);
			v[0] = 0;  // keep weather particles vertical by removing pitch
			yawOrRoll += .5;
			if (yawOrRoll > 360)
				yawOrRoll = 0;
			if(p->type == PARTICLE_ROTATINGYAW)
				v[1] = yawOrRoll;
			else if(p->type == PARTICLE_ROTATINGROLL)
				v[2] = yawOrRoll;
			else
				v[1] = yawOrRoll+180;
			AngleVectors(v, NULL, right, up);	
			VectorScale(right, 3*scale, pright);
			VectorScale(up, 3*scale, pup);
		}
		else {
			VectorScale ( right, p->dist, pright );
			VectorScale ( up, p->dist, pup );
		}

		VectorSet (corner[0],
			p->origin[0] + (pup[0] + pright[0])*(-0.5),
			p->origin[1] + (pup[1] + pright[1])*(-0.5),
			p->origin[2] + (pup[2] + pright[2])*(-0.5));

		VectorSet ( corner[1],
			corner0[0] + pup[0], corner0[1] + pup[1], corner0[2] + pup[2]);
		VectorSet ( corner[2], corner0[0] + (pup[0]+pright[0]),
			corner0[1] + (pup[1]+pright[1]), corner0[2] + (pup[2]+pright[2]));
		VectorSet ( corner[3],
			corner0[0] + pright[0], corner0[1] + pright[1], corner0[2] + pright[2]);

		qglTexCoord2f( 1, 1 );
		qglVertex3fv( corner[0] );

		qglTexCoord2f( 0, 1 );
		qglVertex3fv ( corner[1] );

		qglTexCoord2f( 0, 0 );
		qglVertex3fv ( corner[2] );

		qglTexCoord2f( 1, 0 );
		qglVertex3fv ( corner[3] );
	}

	qglEnd ();
	qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	qglColor4f( 1,1,1,1 );
	qglDisable(GL_BLEND);
	qglDepthMask( GL_TRUE );	// back to normal Z buffering
	GL_TexEnv( GL_REPLACE );
}

/*
===============
R_SortParticles
===============
*/
#define R_CopyParticle(a,b) (a.type=b.type,a.alpha=b.alpha,a.color=b.color,a.dist=b.dist,a.scale=b.scale, a.texnum=b.texnum, a.blenddst=b.blenddst, a.blendsrc=b.blendsrc, VectorCopy(b.origin,a.origin), VectorCopy(b.angle, a.angle))

static void R_SortParticles (gparticle_t *particles, int Li, int Ri)
{
	int pivot;
	gparticle_t temppart;
	int li, ri;

mark0:
	li = Li;
	ri = Ri;

	pivot = particles[(Li+Ri) >> 1].type;
	while (li < ri)
	{
		while (particles[li].type < pivot) li++;
		while (particles[ri].type > pivot) ri--;

		if (li <= ri)
		{
			R_CopyParticle( temppart, particles[ri] );
			R_CopyParticle( particles[ri], particles[li] );
			R_CopyParticle( particles[li], temppart );

			li++;
			ri--;
		}
	}

	if ( Li < ri ) {
		R_SortParticles( particles, Li, ri );
	}
	if (li < Ri) {
		Li = li;
		goto mark0;
	}
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles (void)
{
	int				i;
	float			dist;
	gparticle_t		*gp;
	const particle_t *p;

	if ( !r_newrefdef.num_particles )
		return;

	gp = gparticles;
	num_gparticles = 0;
	for ( p = r_newrefdef.particles, i=0 ; i < r_newrefdef.num_particles ; i++,p++)
	{
		// hack a scale up to keep particles from disapearing
		dist = ( p->origin[0] - r_origin[0] ) * vpn[0] +
			    ( p->origin[1] - r_origin[1] ) * vpn[1] +
			    ( p->origin[2] - r_origin[2] ) * vpn[2];

		if (dist < 0)
			continue;
		else if (dist >= 40)
			dist = 2 + dist * 0.004;
		else
			dist = 2;

		gp->alpha = p->alpha;
		gp->color = p->color;
		gp->type = p->type;
		gp->dist = dist;
		gp->scale = p->scale;
		gp->texnum = p->texnum;
		gp->blenddst = p->blenddst;
		gp->blendsrc = p->blendsrc;
		VectorCopy ( p->origin, gp->origin );
		VectorCopy ( p->angle, gp->angle );

		gp++;
		num_gparticles++;

	}
	if(map_fog)
		qglDisable(GL_FOG);
	R_SortParticles ( gparticles, 0, num_gparticles - 1 );

	// we are always going to used textured particles!
	GL_DrawParticles( num_gparticles, gparticles, d_8to24table);
	if(map_fog)
		qglEnable(GL_FOG);
}

/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	if (!gl_polyblend->value)
		return;
	if (!v_blend[3])
		return;

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
	mleaf_t	*leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.vieworg, r_origin);

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);

	// === jitwater - MPO's code to draw reflective water
	if (g_drawing_refl)
	{
		vec3_t tmp;

		r_origin[2] = (2 * g_refl_Z[g_active_refl]) - r_origin[2]; // flip

		VectorCopy(r_newrefdef.viewangles, tmp);
		tmp[0] *= -1.0f;
		AngleVectors(tmp, vpn, vright, vup);

		if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		{
			vec3_t temp;

			leaf = Mod_PointInLeaf(r_origin, r_worldmodel);
			temp[0] = g_refl_X[g_active_refl];
			temp[1] = g_refl_Y[g_active_refl];

			//if (r_newrefdef.rdflags & RDF_UNDERWATER) todo
			if (r_newrefdef.vieworg[2] < g_refl_Z[g_active_refl])
				temp[2] = g_refl_Z[g_active_refl] - 1;
			else
				temp[2] = g_refl_Z[g_active_refl] + 1;

			leaf = Mod_PointInLeaf(temp, r_worldmodel);

			if (!(leaf->contents & CONTENTS_SOLID) && (leaf->cluster != r_viewcluster))
				r_viewcluster2 = leaf->cluster;
		}

		return;
	}
	// jitwater ===

// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] += 16;
			leaf = Mod_PointInLeaf (temp, r_worldmodel);
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

	//qglFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
	mesa_frustum( xmin, xmax, ymin, ymax, zNear, zFar ); //MPO
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	int		x, x2, y2, y, w, h;

	//
	// set up viewport
	//
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	// start MPO
	// MPO : we only want to set the viewport if we aren't drawing the reflection
	if (!g_drawing_refl)
	{
		qglViewport (x, y2, w, h);	// MPO : note this happens every frame interestingly enough
	}
	else
	{
		qglViewport(0, 0, g_reflTexW, g_reflTexH);	// width/height of texture size, not screen size
	}
	// stop MPO



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


    // start MPO
	// standard transformation
	if (!g_drawing_refl)
	{
	    qglRotatef (-r_newrefdef.viewangles[2],  1, 0, 0);	// MPO : this handles rolling (ie when we strafe side to side we roll slightly)
	    qglRotatef (-r_newrefdef.viewangles[0],  0, 1, 0);	// MPO : this handles up/down rotation
	    qglRotatef (-r_newrefdef.viewangles[1],  0, 0, 1);	// MPO : this handles left/right rotation
	    qglTranslatef (-r_newrefdef.vieworg[0],  -r_newrefdef.vieworg[1],  -r_newrefdef.vieworg[2]);
	    // MPO : this translate call puts the player at the proper spot in the map
	    // MPO : The map is drawn to absolute coordinates
	}
	// mirrored transformation for reflection
	else
	{
		R_DoReflTransform();
		qglTranslatef(0, 0, -0);
	}
	// end MPO

//	if ( gl_state.camera_separation != 0 && gl_state.stereo_enabled )
//		qglTranslatef ( gl_state.camera_separation, 0, 0 );

	qglGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//

	//	if (gl_cull->value)
	if ((gl_cull->value) && (!g_drawing_refl))	// MPO : we must disable culling when drawing the reflection

		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

// CDawg - start
	if (r_newrefdef.viewangles[2])
		qglEnable(GL_BLEND);
	else
		qglDisable(GL_BLEND);
// CDawg - end

//qglDisable(GL_BLEND); //CDawg <-- comment this line.

	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear
=============
*/
extern qboolean have_stencil;
void R_Clear (void)
{
	if (gl_clear->value)
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else
		qglClear (GL_DEPTH_BUFFER_BIT);

	gldepthmin = 0;
	gldepthmax = 1;
	qglDepthFunc (GL_LEQUAL);

	qglDepthRange (gldepthmin, gldepthmax);

	// Stencil shadows - MrG

	if (have_stencil && gl_shadows->integer) {

	qglClearStencil(1);

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
extern image_t *r_mirrortexture;
void R_DrawSpecialSurfaces(void);
void R_RenderView (refdef_t *fd)
{
	GLfloat colors[4] = {(GLfloat) fog.red, (GLfloat) fog.green, (GLfloat) fog.blue, (GLfloat) 0.1};

	numRadarEnts=0;

	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		Com_Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	if (r_speeds->value)
	{
		c_brush_polys = 0;
		c_alias_polys = 0;
		c_flares =0;
	}

	R_PushDlights ();
	
	if (gl_finish->value)
		qglFinish ();

	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	// start MPO
	// if we are doing a reflection, we want to do a clip plane now, after
	// we've set up our projection/modelview matrices

	if (g_drawing_refl)
	{
		double clipPlane[] = { 0, 0, 0, 0 };

		if(r_newrefdef.rdflags & RDF_UNDERWATER) {

			clipPlane[2] = -1;
			clipPlane[3] = g_refl_Z[g_active_refl];
		}

		else {

			clipPlane[2] = 1;
			clipPlane[3] = -g_refl_Z[g_active_refl];
		}


	    // we need clipping so we don't reflect objects underneath the water
		qglEnable(GL_CLIP_PLANE0);
		qglClipPlane(GL_CLIP_PLANE0, clipPlane);
	}
	// stop MPO

	R_MarkLeaves ();	// done here so we know if we're in water

	if(map_fog)
	{
		qglFogi(GL_FOG_MODE, GL_LINEAR);
		qglFogfv(GL_FOG_COLOR, colors);
		qglFogf(GL_FOG_START, fog.start);
		qglFogf(GL_FOG_END, fog.end);
		qglFogf(GL_FOG_DENSITY, fog.density);
		qglEnable(GL_FOG);
	}

	R_DrawWorld ();

	if(r_lensflare->value)
		R_RenderFlares ();

	R_DrawEntitiesOnList ();

	R_DrawSpecialSurfaces();

	R_DrawAlphaSurfaces ();

	qglLoadMatrixf (r_world_matrix); //moving trans brushes

	R_DrawParticles ();

	if(gl_mirror->value) {

		qglBindTexture(GL_TEXTURE_2D, r_mirrortexture->texnum);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0,
					0, 0, 0, 256, 512, 512);
	}

	R_BloomBlend( fd );//BLOOMS

// start MPO
	// if we are doing a reflection, turn off clipping now
	if (g_drawing_refl)
	{
		qglDisable(GL_CLIP_PLANE0);
	}
	// if we aren't doing a reflection then we can do the flash and r speeds
	// (we don't want them showing up in our reflection)
	else
	{
		R_Flash();

		if (r_speeds->value)
		{
			Com_Printf (PRINT_ALL, "%4i wpoly %4i epoly %i tex %i lmaps\n",
				c_brush_polys,
				c_alias_polys,
				c_visible_textures,
				c_visible_lightmaps);
		}
	}
	// stop MPO

	if(map_fog)
		qglDisable(GL_FOG);

	GL_DrawRadar();
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

static void GL_DrawColoredStereoLinePair( float r, float g, float b, float y )
{
	qglColor3f( r, g, b );
	qglVertex2f( 0, y );
	qglVertex2f( vid.width, y );
	qglColor3f( 0, 0, 0 );
	qglVertex2f( 0, y + 1 );
	qglVertex2f( vid.width, y + 1 );
}

static void GL_DrawStereoPattern( void )
{
	int i;

	if ( !( gl_config.renderer & GL_RENDERER_INTERGRAPH ) )
		return;

	if ( !gl_state.stereo_enabled )
		return;

	R_SetGL2D();

	qglDrawBuffer( GL_BACK_LEFT );

	for ( i = 0; i < 20; i++ )
	{
		qglBegin( GL_LINES );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 0 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 2 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 4 );
			GL_DrawColoredStereoLinePair( 1, 0, 0, 6 );
			GL_DrawColoredStereoLinePair( 0, 1, 0, 8 );
			GL_DrawColoredStereoLinePair( 1, 1, 0, 10);
			GL_DrawColoredStereoLinePair( 1, 1, 0, 12);
			GL_DrawColoredStereoLinePair( 0, 1, 0, 14);
		qglEnd();

		GLimp_EndFrame();
	}
}


/*
====================
R_SetLightLevel

====================
*/
void R_SetLightLevel (void)
{
	vec3_t		shadelight;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	// save off light value for server to look at (BIG HACK!)

	R_LightPoint (r_newrefdef.vieworg, shadelight, true);

	// pick the greatest component, which should be the same
	// as the mono value returned by software
	if (shadelight[0] > shadelight[1])
	{
		if (shadelight[0] > shadelight[2])
			r_lightlevel->value = 150*shadelight[0];
		else
			r_lightlevel->value = 150*shadelight[2];
	}
	else
	{
		if (shadelight[1] > shadelight[2])
			r_lightlevel->value = 150*shadelight[1];
		else
			r_lightlevel->value = 150*shadelight[2];
	}

}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/

void R_RenderFrame (refdef_t *fd)
{
	//start MPO
	g_refl_enabled = false;		//reset to false

	if(gl_reflection->value) {
		R_UpdateReflTex(fd);
	}

	R_RenderView( fd );

	R_SetLightLevel ();

	R_SetGL2D ();

	// start MPO
	// if debugging is enabled and reflections are enabled.. draw it
	if ((gl_reflection_debug->value) && (g_refl_enabled)) {

		R_DrawDebugReflTexture();
	}

	if(!g_refl_enabled) R_clear_refl();
	// end MPO
}


void R_Register( void )
{

	con_font = Cvar_Get ("con_font", "default", CVAR_ARCHIVE);

	r_lefthand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );
	r_norefresh = Cvar_Get ("r_norefresh", "0", 0);
	r_fullbright = Cvar_Get ("r_fullbright", "0", 0);
	r_drawentities = Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = Cvar_Get ("r_drawworld", "1", 0);
	r_novis = Cvar_Get ("r_novis", "0", 0);
	r_nocull = Cvar_Get ("r_nocull", "0", 0);
	r_lerpmodels = Cvar_Get ("r_lerpmodels", "1", 0);
	r_speeds = Cvar_Get ("r_speeds", "0", 0);

	r_lightlevel = Cvar_Get ("r_lightlevel", "0", 0);
	r_wave = Cvar_Get ("r_wave", "2", CVAR_ARCHIVE); // Water waves

	gl_nosubimage = Cvar_Get( "gl_nosubimage", "0", 0 );

	gl_particle_min_size = Cvar_Get( "gl_particle_min_size", ".2", CVAR_ARCHIVE );
	gl_particle_max_size = Cvar_Get( "gl_particle_max_size", "40", CVAR_ARCHIVE );
	gl_particle_size = Cvar_Get( "gl_particle_size", "40", CVAR_ARCHIVE );
	gl_particle_att_a = Cvar_Get( "gl_particle_att_a", "0.01", CVAR_ARCHIVE );
	gl_particle_att_b = Cvar_Get( "gl_particle_att_b", "0.0", CVAR_ARCHIVE );
	gl_particle_att_c = Cvar_Get( "gl_particle_att_c", "0.01", CVAR_ARCHIVE );

	gl_modulate = Cvar_Get ("gl_modulate", "2", CVAR_ARCHIVE );
	gl_log = Cvar_Get( "gl_log", "0", 0 );
	gl_bitdepth = Cvar_Get( "gl_bitdepth", "0", 0 );
	gl_mode = Cvar_Get( "gl_mode", "6", CVAR_ARCHIVE );
	gl_lightmap = Cvar_Get ("gl_lightmap", "0", 0);
	gl_shadows = Cvar_Get ("gl_shadows", "2", CVAR_ARCHIVE );
	gl_dynamic = Cvar_Get ("gl_dynamic", "0", CVAR_ARCHIVE);
	gl_nobind = Cvar_Get ("gl_nobind", "0", 0);
	gl_round_down = Cvar_Get ("gl_round_down", "1", 0);
	gl_picmip = Cvar_Get ("gl_picmip", "0", 0);
	gl_skymip = Cvar_Get ("gl_skymip", "0", 0);
	gl_showtris = Cvar_Get ("gl_showtris", "0", 0);
	gl_finish = Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE);
	gl_clear = Cvar_Get ("gl_clear", "0", 0);
	gl_cull = Cvar_Get ("gl_cull", "1", 0);
	gl_polyblend = Cvar_Get ("gl_polyblend", "1", 0);
	gl_flashblend = Cvar_Get ("gl_flashblend", "0", CVAR_ARCHIVE);
	gl_playermip = Cvar_Get ("gl_playermip", "0", 0);
#ifdef __unix__
	gl_driver = Cvar_Get( "gl_driver", "libGL.so.1", CVAR_ARCHIVE );
#else
	gl_driver = Cvar_Get( "gl_driver", "opengl32", CVAR_ARCHIVE );
#endif
	gl_texturemode = Cvar_Get( "gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	gl_texturealphamode = Cvar_Get( "gl_texturealphamode", "default", CVAR_ARCHIVE );
	gl_texturesolidmode = Cvar_Get( "gl_texturesolidmode", "default", CVAR_ARCHIVE );
	gl_lockpvs = Cvar_Get( "gl_lockpvs", "0", 0 );
	gl_rtlights = Cvar_Get("gl_rtlights", "0", CVAR_ARCHIVE );

	gl_vertex_arrays = Cvar_Get( "gl_vertex_arrays", "0", CVAR_ARCHIVE );

	gl_ext_swapinterval = Cvar_Get( "gl_ext_swapinterval", "1", CVAR_ARCHIVE );
	gl_ext_palettedtexture = Cvar_Get( "gl_ext_palettedtexture", "0", CVAR_ARCHIVE );
	gl_ext_multitexture = Cvar_Get( "gl_ext_multitexture", "1", CVAR_ARCHIVE );
	gl_ext_pointparameters = Cvar_Get( "gl_ext_pointparameters", "0", CVAR_ARCHIVE );
	gl_ext_compiled_vertex_array = Cvar_Get( "gl_ext_compiled_vertex_array", "1", CVAR_ARCHIVE );

	gl_drawbuffer = Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );
	gl_swapinterval = Cvar_Get( "gl_swapinterval", "1", CVAR_ARCHIVE );

	gl_3dlabs_broken = Cvar_Get( "gl_3dlabs_broken", "1", CVAR_ARCHIVE );

	r_shaders = Cvar_Get ("r_shaders", "1", CVAR_ARCHIVE);
	// Vic - begin

	r_overbrightbits = Cvar_Get( "r_overbrightbits", "2", CVAR_ARCHIVE );
	gl_ext_mtexcombine = Cvar_Get( "gl_ext_mtexcombine", "1", CVAR_ARCHIVE );

	// Vic - end

	gl_reflection		= Cvar_Get("gl_reflection", "0", CVAR_ARCHIVE);			// MPO
	gl_reflection_debug	= Cvar_Get("gl_reflection_debug", "0", CVAR_ARCHIVE);	// MPO
	gl_reflection_max	= Cvar_Get("gl_reflection_max", "2", CVAR_ARCHIVE);
	gl_mirror			= Cvar_Get("gl_mirror", "1", CVAR_ARCHIVE);

	vid_fullscreen = Cvar_Get( "vid_fullscreen", "0", CVAR_ARCHIVE );
	vid_gamma = Cvar_Get( "vid_gamma", "1.0", CVAR_ARCHIVE );
	vid_contrast = Cvar_Get( "vid_contrast", "1.0", CVAR_ARCHIVE);
	vid_ref = Cvar_Get( "vid_ref", "gl", CVAR_ARCHIVE );

	gl_normalmaps = Cvar_Get("gl_normalmaps", "0", CVAR_ARCHIVE);
	gl_parallaxmaps = Cvar_Get("gl_parallaxmaps", "0", CVAR_ARCHIVE); 
	gl_specular = Cvar_Get("gl_specular", "0", CVAR_ARCHIVE);

	r_lensflare = Cvar_Get( "r_lensflare", "1", CVAR_ARCHIVE );
	r_lensflare_intens = Cvar_Get ("r_lensflare_intens", "3", CVAR_ARCHIVE);

	r_minimap_size = Cvar_Get ("r_minimap_size", "256", CVAR_ARCHIVE );
	r_minimap_zoom = Cvar_Get ("r_minimap_zoom", "1", CVAR_ARCHIVE );
	r_minimap_style = Cvar_Get ("r_minimap_style", "1", CVAR_ARCHIVE );
	r_minimap = Cvar_Get ("r_minimap", "0", CVAR_ARCHIVE );

	sys_priority = Cvar_Get("sys_priority", "0", CVAR_ARCHIVE);
	sys_affinity = Cvar_Get("sys_affinity", "1", CVAR_ARCHIVE);

	gl_screenshot_type = Cvar_Get("gl_screenshot_type", "jpeg", CVAR_ARCHIVE);
	gl_screenshot_jpeg_quality = Cvar_Get("gl_screenshot_jpeg_quality", "85", CVAR_ARCHIVE);

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

	if ( vid_fullscreen->modified && !gl_config.allow_cds )
	{
		Com_Printf ("R_SetMode() - CDS not allowed with this driver\n" );
		Cvar_SetValue( "vid_fullscreen", !vid_fullscreen->value );
		vid_fullscreen->modified = false;
	}

	fullscreen = vid_fullscreen->value;

	vid_fullscreen->modified = false;
	gl_mode->modified = false;

	if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->value, fullscreen ) ) == rserr_ok )
	{
		gl_state.prev_mode = gl_mode->value;
	}
	else
	{
		if ( err == rserr_invalid_fullscreen )
		{
			Cvar_SetValue( "vid_fullscreen", 0);
			vid_fullscreen->modified = false;
			Com_Printf ("ref_gl::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->value, false ) ) == rserr_ok )
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
R_Init
===============
*/
int R_Init( void *hinstance, void *hWnd )
{
	char renderer_buffer[1000];
	char vendor_buffer[1000];
	int		err;
	int		j;
	extern float r_turbsin[256];
	const char *shaderStrings[1];
	unsigned char *shader_assembly;
    int		nResult;
    char	str[4096];

	gl_arb_fragment_program = Cvar_Get("gl_arb_fragment_program", "1", CVAR_ARCHIVE); // jit
	gl_glsl_shaders = Cvar_Get("gl_glsl_shaders", "0", CVAR_ARCHIVE); 

	for ( j = 0; j < 256; j++ )
	{
		r_turbsin[j] *= 0.5;
	}

	Com_Printf ("ref_gl version: "REF_VERSION"\n");

	Draw_GetPalette ();

	R_Register();

	VLight_Init();

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

	// set our "safe" modes
	gl_state.prev_mode = 3;

	// create the window and set up the context
	if ( !R_SetMode () )
	{
		QGL_Shutdown();
		Com_Printf ("ref_gl::R_Init() - could not R_SetMode()\n" );
		return -1;
	}

	VID_MenuInit();

	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = qglGetString (GL_VENDOR);
	Com_Printf ("GL_VENDOR: %s\n", gl_config.vendor_string );
	gl_config.renderer_string = qglGetString (GL_RENDERER);
	Com_Printf ("GL_RENDERER: %s\n", gl_config.renderer_string );
	gl_config.version_string = qglGetString (GL_VERSION);
	Com_Printf ("GL_VERSION: %s\n", gl_config.version_string );
	gl_config.extensions_string = qglGetString (GL_EXTENSIONS);
	Com_Printf ("GL_EXTENSIONS: %s\n", gl_config.extensions_string );

	strcpy( renderer_buffer, gl_config.renderer_string );
	Q_strlwr( renderer_buffer );

	strcpy( vendor_buffer, gl_config.vendor_string );
	Q_strlwr( vendor_buffer );

	if ( strstr( renderer_buffer, "voodoo" ) )
	{
		if ( !strstr( renderer_buffer, "rush" ) )
			gl_config.renderer = GL_RENDERER_VOODOO;
		else
			gl_config.renderer = GL_RENDERER_VOODOO_RUSH;
	}
	else if ( strstr( vendor_buffer, "sgi" ) )
		gl_config.renderer = GL_RENDERER_SGI;
	else if ( strstr( renderer_buffer, "permedia" ) )
		gl_config.renderer = GL_RENDERER_PERMEDIA2;
	else if ( strstr( renderer_buffer, "glint" ) )
		gl_config.renderer = GL_RENDERER_GLINT_MX;
	else if ( strstr( renderer_buffer, "glzicd" ) )
		gl_config.renderer = GL_RENDERER_REALIZM;
	else if ( strstr( renderer_buffer, "gdi" ) )
		gl_config.renderer = GL_RENDERER_MCD;
	else if ( strstr( renderer_buffer, "pcx2" ) )
		gl_config.renderer = GL_RENDERER_PCX2;
	else if ( strstr( renderer_buffer, "verite" ) )
		gl_config.renderer = GL_RENDERER_RENDITION;
	else
		gl_config.renderer = GL_RENDERER_OTHER;

	// power vr can't have anything stay in the framebuffer, so
	// the screen needs to redraw the tiled background every frame
	if ( gl_config.renderer & GL_RENDERER_POWERVR )
	{
		Cvar_Set( "scr_drawall", "1" );
	}
	else
	{
		Cvar_Set( "scr_drawall", "0" );
	}

	// MCD has buffering issues
	if ( gl_config.renderer == GL_RENDERER_MCD )
	{
		Cvar_SetValue( "gl_finish", 1 );
	}

	if ( gl_config.renderer & GL_RENDERER_3DLABS )
	{
		if ( gl_3dlabs_broken->value )
			gl_config.allow_cds = false;
		else
			gl_config.allow_cds = true;
	}
	else
	{
		gl_config.allow_cds = true;
	}

	if ( gl_config.allow_cds )
		Com_Printf ("...allowing CDS\n" );
	else
		Com_Printf ( "...disabling CDS\n" );

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

#ifdef _WIN32
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

	if (strstr(gl_config.extensions_string, "GL_EXT_point_parameters"))
	{
		if(gl_ext_pointparameters->value)
		{
			qglPointParameterfEXT = (void(APIENTRY*)(GLenum, GLfloat))qwglGetProcAddress("glPointParameterfEXT");
			qglPointParameterfvEXT = (void(APIENTRY*)(GLenum, const GLfloat*))qwglGetProcAddress("glPointParameterfvEXT");
		}
		else
		{
			Com_Printf ("...ignoring GL_EXT_point_parameters\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_point_parameters not found\n" );
	}

#ifdef __unix__
	if ( strstr( gl_config.extensions_string, "3DFX_set_global_palette" ))
	{
		if ( gl_ext_palettedtexture->value )
		{
			Com_Printf ("...using 3DFX_set_global_palette\n" );
			qgl3DfxSetPaletteEXT = ( void ( APIENTRY * ) (GLuint *) )qwglGetProcAddress( "gl3DfxSetPaletteEXT" );
			qglColorTableEXT = Fake_glColorTableEXT;
		}
		else
		{
			Com_Printf ("...ignoring 3DFX_set_global_palette\n" );
		}
	}
	else
	{
		Com_Printf ("...3DFX_set_global_palette not found\n" );
	}
#endif

	if ( !qglColorTableEXT &&
		strstr( gl_config.extensions_string, "GL_EXT_paletted_texture" ) &&
		strstr( gl_config.extensions_string, "GL_EXT_shared_texture_palette" ) )
	{
		if ( gl_ext_palettedtexture->value )
		{
			Com_Printf ("...using GL_EXT_shared_texture_palette\n" );
			qglColorTableEXT = ( void ( APIENTRY * ) ( int, int, int, int, int, const void * ) ) qwglGetProcAddress( "glColorTableEXT" );
		}
		else
		{
			Com_Printf ("...ignoring GL_EXT_shared_texture_palette\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_EXT_shared_texture_palette not found\n" );
	}

	if ( strstr( gl_config.extensions_string, "GL_ARB_multitexture" ) )
	{
		if ( gl_ext_multitexture->value )
		{
			Com_Printf ("...using GL_ARB_multitexture\n" );
			qglMTexCoord2fSGIS = ( void * ) qwglGetProcAddress( "glMultiTexCoord2fARB" );
			qglMTexCoord3fSGIS = ( void * ) qwglGetProcAddress( "glMultiTexCoord3fARB" );
			qglMultiTexCoord3fvARB = (void*)qwglGetProcAddress("glMultiTexCoord3fvARB");
			qglActiveTextureARB = ( void * ) qwglGetProcAddress( "glActiveTextureARB" );
			qglClientActiveTextureARB = ( void * ) qwglGetProcAddress( "glClientActiveTextureARB" );
			GL_TEXTURE0 = GL_TEXTURE0_ARB;
			GL_TEXTURE1 = GL_TEXTURE1_ARB;
			GL_TEXTURE2 = GL_TEXTURE2_ARB;
			GL_TEXTURE3 = GL_TEXTURE3_ARB;
		}
		else
		{
			Com_Printf ("...ignoring GL_ARB_multitexture\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_ARB_multitexture not found\n" );
	}

	if ( strstr( gl_config.extensions_string, "GL_SGIS_multitexture" ) )
	{
		if ( qglActiveTextureARB )
		{
			Com_Printf ("...GL_SGIS_multitexture deprecated in favor of ARB_multitexture\n" );
		}
		else if ( gl_ext_multitexture->value )
		{
			Com_Printf ("...using GL_SGIS_multitexture\n" );
			qglMTexCoord2fSGIS = ( void * ) qwglGetProcAddress( "glMTexCoord2fSGIS" );
			qglMTexCoord3fSGIS = ( void * ) qwglGetProcAddress( "glMTexCoord3fSGIS" );
			qglSelectTextureSGIS = ( void * ) qwglGetProcAddress( "glSelectTextureSGIS" );
			GL_TEXTURE0 = GL_TEXTURE0_SGIS;
			GL_TEXTURE1 = GL_TEXTURE1_SGIS;
		}
		else
		{
			Com_Printf ("...ignoring GL_SGIS_multitexture\n" );
		}
	}
	else
	{
		Com_Printf ("...GL_SGIS_multitexture not found\n" );
	}
	
	// Vic - begin
	gl_config.mtexcombine = false;
	if ( strstr( gl_config.extensions_string, "GL_ARB_texture_env_combine" ) )
	{
		if ( gl_ext_mtexcombine->value )
		{
			Com_Printf( "...using GL_ARB_texture_env_combine\n" );
			gl_config.mtexcombine = true;
		}
		else
		{
			Com_Printf( "...ignoring GL_ARB_texture_env_combine\n" );
		}
	}
	else
	{
		Com_Printf( "...GL_ARB_texture_env_combine not found\n" );
	}
	if ( !gl_config.mtexcombine )
	{
		if ( strstr( gl_config.extensions_string, "GL_EXT_texture_env_combine" ) )
		{
			if ( gl_ext_mtexcombine->value )
			{
				Com_Printf( "...using GL_EXT_texture_env_combine\n" );
				gl_config.mtexcombine = true;
			}
			else
			{
				Com_Printf( "...ignoring GL_EXT_texture_env_combine\n" );
			}
		}
		else
		{
			Com_Printf( "...GL_EXT_texture_env_combine not found\n" );
		}
	}
	// Vic - end

	// Texture Shader support - MrG
	if ( strstr( gl_config.extensions_string, "GL_NV_texture_shader" ) )
	{
		Com_Printf("...using GL_NV_texture_shader\n");
		gl_state.texshaders=true;
	}
	else
	{
		Com_Printf("...GL_NV_texture_shader not found\n");
		gl_state.texshaders=false;
	}

		// === jitwater
	if (!gl_arb_fragment_program->value)
	{
		gl_state.fragment_program = false;

	}
	else if (strstr(gl_config.extensions_string, "GL_ARB_fragment_program"))
	{
		gl_state.fragment_program = true;

		qglGenProgramsARB = (PFNGLGENPROGRAMSARBPROC)qwglGetProcAddress("glGenProgramsARB");
		qglDeleteProgramsARB = (PFNGLDELETEPROGRAMSARBPROC)qwglGetProcAddress("glDeleteProgramsARB");
		qglBindProgramARB = (PFNGLBINDPROGRAMARBPROC)qwglGetProcAddress("glBindProgramARB");
		qglProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)qwglGetProcAddress("glProgramStringARB");
		qglProgramEnvParameter4fARB =
			(PFNGLPROGRAMENVPARAMETER4FARBPROC)qwglGetProcAddress("glProgramEnvParameter4fARB");
		qglProgramLocalParameter4fARB =
			(PFNGLPROGRAMLOCALPARAMETER4FARBPROC)qwglGetProcAddress("glProgramLocalParameter4fARB");

		if (!(qglGenProgramsARB && qglDeleteProgramsARB && qglBindProgramARB &&
			qglProgramStringARB && qglProgramEnvParameter4fARB && qglProgramLocalParameter4fARB))
		{
			gl_state.fragment_program = false;
			Com_Printf("...GL_ARB_fragment_program not found\n");

		}
	}
	else
	{
		gl_state.fragment_program = false;
		Com_Printf("...GL_ARB_fragment_program not found\n");
	}

	//load glsl 
	if (!gl_glsl_shaders->value)
	{
		gl_state.glsl_shaders = false;

	}
	if (strstr(gl_config.extensions_string,  "GL_ARB_shader_objects" ) && gl_state.fragment_program)
	{

		gl_state.glsl_shaders = true;

		glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glCreateProgramObjectARB");
		glDeleteObjectARB         = (PFNGLDELETEOBJECTARBPROC)qwglGetProcAddress("glDeleteObjectARB");
		glUseProgramObjectARB     = (PFNGLUSEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glUseProgramObjectARB");
		glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC)qwglGetProcAddress("glCreateShaderObjectARB");
		glShaderSourceARB         = (PFNGLSHADERSOURCEARBPROC)qwglGetProcAddress("glShaderSourceARB");
		glCompileShaderARB        = (PFNGLCOMPILESHADERARBPROC)qwglGetProcAddress("glCompileShaderARB");
		glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)qwglGetProcAddress("glGetObjectParameterivARB");
		glAttachObjectARB         = (PFNGLATTACHOBJECTARBPROC)qwglGetProcAddress("glAttachObjectARB");
		glGetInfoLogARB           = (PFNGLGETINFOLOGARBPROC)qwglGetProcAddress("glGetInfoLogARB");
		glLinkProgramARB          = (PFNGLLINKPROGRAMARBPROC)qwglGetProcAddress("glLinkProgramARB");
		glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC)qwglGetProcAddress("glGetUniformLocationARB");
		glUniform3fARB            = (PFNGLUNIFORM3FARBPROC)qwglGetProcAddress("glUniform3fARB");
		glUniform1iARB            = (PFNGLUNIFORM1IARBPROC)qwglGetProcAddress("glUniform1iARB");
		glUniform1fARB		  = (PFNGLUNIFORM1FARBPROC)qwglGetProcAddress("glUniform1fARB");
		glUniformMatrix3fvARB	  = (PFNGLUNIFORMMATRIX3FVARBPROC)qwglGetProcAddress("glUniformMatrix3fvARB");

		if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
		    !glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB || 
		    !glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB || 
		    !glLinkProgramARB || !glGetUniformLocationARB || !glUniform3fARB ||
				!glUniform1iARB || !glUniform1fARB || !glUniformMatrix3fvARB)
		{
			Com_Printf("...One or more GL_ARB_shader_objects functions were not found");
			gl_state.glsl_shaders = false;
		}
	}
	else
	{            
		Com_Printf("...One or more GL_ARB_shader_objects functions were not found");
		gl_state.glsl_shaders = false;
	}

	if(gl_state.glsl_shaders) {
		int len;
		g_programObj = glCreateProgramObjectARB();
	
		//
		// Vertex shader
		//

		len = FS_LoadFile("scripts/vertex_shader.glsl", &shader_assembly);

		if (len > 0) {
			g_vertexShader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_vertexShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_vertexShader);
			glGetObjectParameterivARB( g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Vertex Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_programObj, g_vertexShader );
		else
		{
			Com_Printf("...Vertex Shader Compile Error");
		}

		//
		// Fragment shader
		//
		len = FS_LoadFile("scripts/fragment_shader.glsl", &shader_assembly);
		
		if(len > 0) {
			g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
			shaderStrings[0] = (char*)shader_assembly;
			glShaderSourceARB( g_fragmentShader, 1, shaderStrings, NULL );
			glCompileShaderARB( g_fragmentShader );
			glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );
		}
		else {
			Com_Printf("...Unable to Locate Fragment Shader");
			nResult = 0;
		}

		if( nResult )
			glAttachObjectARB( g_programObj, g_fragmentShader );
		else
		{
			Com_Printf("...Fragment Shader Compile Error");
		}

		glLinkProgramARB( g_programObj );
		glGetObjectParameterivARB( g_programObj, GL_OBJECT_LINK_STATUS_ARB, &nResult );

		if( !nResult )
		{
			glGetInfoLogARB( g_programObj, sizeof(str), NULL, str );
			Com_Printf("...Linking Error");
		}

		//
		// Locate some parameters by name so we can set them later...
		//

		g_location_testTexture = glGetUniformLocationARB( g_programObj, "testTexture" );
		g_location_eyePos = glGetUniformLocationARB( g_programObj, "Eye" );
		g_tangentSpaceTransform = glGetUniformLocationARB( g_programObj, "tangentSpaceTransform");
		g_location_heightTexture = glGetUniformLocationARB( g_programObj, "HeightTexture" );
		g_location_lmTexture = glGetUniformLocationARB( g_programObj, "lmTexture" );
		g_location_normalTexture = glGetUniformLocationARB( g_programObj, "NormalTexture" );
		g_location_fog = glGetUniformLocationARB( g_programObj, "FOG" );
		g_location_parallax = glGetUniformLocationARB( g_programObj, "PARALLAX" );
		g_location_dynamic = glGetUniformLocationARB( g_programObj, "DYNAMIC" );
		g_location_specular = glGetUniformLocationARB( g_programObj, "SPECULAR" );
		g_location_tangent = glGetUniformLocationARB( g_programObj, "sTangent" );
		g_location_lightPosition = glGetUniformLocationARB( g_programObj, "lightPosition" );
		g_location_lightColour = glGetUniformLocationARB( g_programObj, "lightColour" );
		g_location_lightCutoffSquared = glGetUniformLocationARB( g_programObj, "lightCutoffSquared" );
		g_location_surfaceColor = glGetUniformLocationARB( g_programObj, "surfaceColor" );
	}
	
	GL_SetDefaultState();

	/*
	** draw our stereo patterns
	*/
#if 0 // commented out until H3D pays us the money they owe us
	GL_DrawStereoPattern();
#endif

	GL_InitImages ();
	Mod_Init ();
	R_InitParticleTexture ();
	Draw_InitLocal ();

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		Com_Printf ("glGetError() = 0x%x\n", err);

	R_init_refl(gl_reflection_max->value);	// MPO : init reflections

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

	GL_ShutdownImages ();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();

	/*
	** shutdown our QGL subsystem
	*/
	QGL_Shutdown();

	if(gl_state.glsl_shaders) {
		glDeleteObjectARB( g_vertexShader );
		glDeleteObjectARB( g_fragmentShader );
		glDeleteObjectARB( g_programObj );
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

	if (con_font->modified)
		RefreshFont ();

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
		GLimp_EnableLogging( gl_log->value );
		gl_log->modified = false;
	}

	if ( gl_log->value )
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
			if ( Q_stricmp( gl_drawbuffer->string, "GL_FRONT" ) == 0 )
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
	GL_SetTexturePalette( r_rawpalette );

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
** R_DrawBeam
*/
void R_DrawBeam( entity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	qglDisable( GL_TEXTURE_2D );
	qglEnable( GL_BLEND );
	qglDepthMask( GL_FALSE );

	r = ( d_8to24table[e->skinnum & 0xFF] ) & 0xFF;
	g = ( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF;
	b = ( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF;

	r *= 1/255.0F;
	g *= 1/255.0F;
	b *= 1/255.0F;

	qglColor4f( r, g, b, e->alpha );

	qglBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		qglVertex3fv( start_points[i] );
		qglVertex3fv( end_points[i] );
		qglVertex3fv( start_points[(i+1)%NUM_BEAM_SEGS] );
		qglVertex3fv( end_points[(i+1)%NUM_BEAM_SEGS] );
	}
	qglEnd();

	qglEnable( GL_TEXTURE_2D );
	qglDisable( GL_BLEND );
	qglDepthMask( GL_TRUE );
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
