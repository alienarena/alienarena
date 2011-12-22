/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2011 COR Entertainment, LLC.

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
// R_SURF.C: surface-related refresh code

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "r_local.h"

static vec3_t	modelorg;		// relative to viewpoint

vec3_t	r_worldLightVec;
dlight_t *dynLight;

msurface_t	*r_alpha_surfaces;
msurface_t	*r_rscript_surfaces;
msurface_t	*r_normalsurfaces;
msurface_t	*r_glsl_surfaces;
msurface_t  *r_glsl_dynamic_surfaces;

#define LIGHTMAP_BYTES 4

#define	BLOCK_WIDTH		128
#define	BLOCK_HEIGHT	128

#define	LIGHTMAP_SIZE	1024
#define	MAX_LIGHTMAPS	8

int		c_visible_lightmaps;
int		c_visible_textures;

#define GL_LIGHTMAP_FORMAT GL_RGBA

typedef struct
{
	int internal_format;
	int	current_lightmap_texture;

	msurface_t	*lightmap_surfaces[MAX_LIGHTMAPS];

	int			allocated[LIGHTMAP_SIZE];

	// the lightmap texture data needs to be kept in
	// main memory so texsubimage can update properly
	byte		lightmap_buffer[4*LIGHTMAP_SIZE*LIGHTMAP_SIZE];
} gllightmapstate_t;

static gllightmapstate_t gl_lms;

static void		LM_InitBlock( void );
static void		LM_UploadBlock( qboolean dynamic );
static qboolean	LM_AllocBlock (int w, int h, int *x, int *y);

extern void R_SetCacheState( msurface_t *surf );
extern void R_BuildLightMap (msurface_t *surf, byte *dest, int stride);

/*
=============================================================

	BRUSH MODELS

=============================================================
*/

/*
===============
BSP_TextureAnimation

Returns the proper texture for a given time and base texture
===============
*/
image_t *BSP_TextureAnimation (mtexinfo_t *tex)
{
	int		c;

	if (!tex->next)
		return tex->image;

	c = currententity->frame % tex->numframes;
	while (c)
	{
		tex = tex->next;
		c--;
	}

	return tex->image;
}

/*
================
BSP_DrawPoly
================
*/
void BSP_DrawPoly (msurface_t *fa, int flags)
{
	float	scroll;

	scroll = 0;
	if (flags & SURF_FLOWING)
	{
		scroll = -64 * ( (r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0) );
		if (scroll == 0.0)
			scroll = -64.0;
	}

	R_InitVArrays(VERT_SINGLE_TEXTURED);
	R_AddTexturedSurfToVArray (fa, scroll);
	R_KillVArrays();

}

/*
================
BSP_DrawTexturelessPoly
================
*/
void BSP_DrawTexturelessPoly (msurface_t *fa)
{

	R_InitVArrays(VERT_NO_TEXTURE);
	R_AddSurfToVArray (fa);
	R_KillVArrays();

}

void BSP_DrawTexturelessInlineBModel (entity_t *e)
{
	int			i;
	msurface_t	*psurf;

	//
	// draw texture
	//
	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];
	for (i=0 ; i<currentmodel->nummodelsurfaces ; i++, psurf++)
	{

		// draw the polygon
		BSP_DrawTexturelessPoly( psurf );
		psurf->visframe = r_framecount;
	}

	qglDisable (GL_BLEND);
	qglColor4f (1,1,1,1);
	GL_TexEnv( GL_REPLACE );
}

void BSP_DrawTexturelessBrushModel (entity_t *e)
{
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (currentmodel->nummodelsurfaces == 0)
		return;

	currententity = e;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = e->origin[i] - currentmodel->radius;
			maxs[i] = e->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (e->origin, currentmodel->mins, mins);
		VectorAdd (e->origin, currentmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs)) {
		return;
	}

	qglColor3f (1,1,1);

	VectorSubtract (r_newrefdef.vieworg, e->origin, modelorg);

	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (e->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

    qglPushMatrix ();
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug
	R_RotateForEntity (e);
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug

	BSP_DrawTexturelessInlineBModel (e);

	qglPopMatrix ();
}


/*
** BSP_DrawTriangleOutlines
*/
void BSP_DrawTriangleOutlines (void)
{
	int			i, j;
	glpoly_t	*p;

	if (!gl_showtris->value)
		return;

	qglDisable (GL_TEXTURE_2D);
	qglDisable (GL_DEPTH_TEST);
	qglColor4f (1,1,1,1);

	for (i=0 ; i<MAX_LIGHTMAPS ; i++)
	{
		msurface_t *surf;

		for ( surf = gl_lms.lightmap_surfaces[i]; surf != 0; surf = surf->lightmapchain )
		{
			p = surf->polys;
			for (j=2 ; j<p->numverts ; j++ )
			{
				qglBegin (GL_LINE_STRIP);
				qglVertex3fv (p->verts[0]);
				qglVertex3fv (p->verts[j-1]);
				qglVertex3fv (p->verts[j]);
				qglVertex3fv (p->verts[0]);
				qglEnd ();
			}
		}
	}

	qglEnable (GL_DEPTH_TEST);
	qglEnable (GL_TEXTURE_2D);
}

/*
================
BSP_RenderBrushPoly
================
*/
void BSP_RenderBrushPoly (msurface_t *fa)
{
	image_t		*image;
	float		scroll;

	c_brush_polys++;

	image = BSP_TextureAnimation (fa->texinfo);

	if (fa->flags & SURF_DRAWTURB)
	{
		GL_Bind( image->texnum );

		// warp texture, no lightmaps
		GL_TexEnv( GL_MODULATE );
		qglColor4f( gl_state.inverse_intensity,
			        gl_state.inverse_intensity,
					gl_state.inverse_intensity,
					1.0F );
		R_RenderWaterPolys(fa, 0, 1, 1);

		GL_TexEnv( GL_REPLACE );

		return;
	}
	else
	{
		GL_Bind( image->texnum );

		GL_TexEnv( GL_REPLACE );
	}

	if(fa->texinfo->has_normalmap) 
	{
		//add to normal chain
		fa->normalchain = r_normalsurfaces;
		r_normalsurfaces = fa;
	}


	if (SurfaceIsAlphaBlended(fa))
		qglEnable( GL_ALPHA_TEST );

	scroll = 0;
	if (fa->texinfo->flags & SURF_FLOWING)
	{
		scroll = -64 * ( (r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0) );
		if (scroll == 0.0)
			scroll = -64.0;
	}
	R_InitVArrays(VERT_SINGLE_TEXTURED);
	R_AddTexturedSurfToVArray (fa, scroll);
	R_KillVArrays();

	if (SurfaceIsAlphaBlended(fa))
	{
		qglDisable( GL_ALPHA_TEST );
		return;
	}
}

/*
================
DrawTextureChains
================
*/
void DrawTextureChains (void)
{
    int     i;
    msurface_t  *s;
    image_t     *image;

    c_visible_textures = 0;

    if ( !qglSelectTextureARB && !qglActiveTextureARB )
    {
        for ( i = 0, image=gltextures ; i<numgltextures ; i++,image++)
        {
            if (!image->registration_sequence)
                continue;
            s = image->texturechain;
            if (!s)
                continue;
            c_visible_textures++;

            for ( ; s ; s=s->texturechain)
                BSP_RenderBrushPoly (s);

            image->texturechain = NULL;
        }
    }
    else
    {
        for ( i = 0, image=gltextures ; i<numgltextures ; i++,image++)
        {
            if (!image->registration_sequence)
                continue;
            if (!image->texturechain)
                continue;
            c_visible_textures++;

            for ( s = image->texturechain; s ; s=s->texturechain)
            {
                if ( !( s->flags & SURF_DRAWTURB ) )
                    BSP_RenderBrushPoly (s);
            }
        }

        GL_EnableMultitexture( false );
        for ( i = 0, image=gltextures ; i<numgltextures ; i++,image++)
        {
            if (!image->registration_sequence)
                continue;
            s = image->texturechain;
            if (!s)
                continue;

            for ( ; s ; s=s->texturechain)
            {
                if ( s->flags & SURF_DRAWTURB )
                    BSP_RenderBrushPoly (s);
            }

            image->texturechain = NULL;
        }
    }

    GL_TexEnv( GL_REPLACE );
}

/*
================
R_DrawAlphaSurfaces

Draw water surfaces and windows.
The BSP tree is waled front to back, so unwinding the chain
of alpha_surfaces will draw back to front, giving proper ordering.
================
*/
void R_DrawAlphaSurfaces (void)
{
	msurface_t	*s;
	float		intens;
	rscript_t	*rs_shader;
	rs_stage_t	*stage = NULL;
	int			texnum = 0;
	float		scaleX = 1, scaleY = 1;

	// the textures are prescaled up for a better lighting range,
	// so scale it back down
	intens = gl_state.inverse_intensity;

	for (s=r_alpha_surfaces ; s ; s=s->texturechain)
	{
		qglLoadMatrixf (r_world_matrix); //moving trans brushes

		qglDepthMask ( GL_FALSE );
		qglEnable (GL_BLEND);
		GL_TexEnv( GL_MODULATE );

		GL_Bind(s->texinfo->image->texnum);
		c_brush_polys++;

		if (s->texinfo->flags & SURF_TRANS33)
			qglColor4f (intens, intens, intens, 0.33);
		else if (s->texinfo->flags & SURF_TRANS66)
			qglColor4f (intens, intens, intens, 0.66);
		else
			qglColor4f (intens, intens, intens, 1);

		//moving trans brushes
		if (s->entity)
		{
			s->entity->angles[0] = -s->entity->angles[0];	// stupid quake bug
			s->entity->angles[2] = -s->entity->angles[2];	// stupid quake bug
				R_RotateForEntity (s->entity);
			s->entity->angles[0] = -s->entity->angles[0];	// stupid quake bug
			s->entity->angles[2] = -s->entity->angles[2];	// stupid quake bug
		}

		if (s->flags & SURF_DRAWTURB) 
		{
			//water shaders
			if(r_shaders->value) 
			{
				rs_shader = (rscript_t *)s->texinfo->image->script;
				if(rs_shader) 
				{
					stage = rs_shader->stage;
					if(stage) 
					{	//for now, just map a reflection texture
						texnum = stage->texture->texnum; //pass this to renderwaterpolys
					}
					if(stage->scale.scaleX != 0 && stage->scale.scaleY !=0) 
					{
						scaleX = stage->scale.scaleX;
						scaleY = stage->scale.scaleY;
					}
				}
			}
			R_RenderWaterPolys (s, texnum, scaleX, scaleY);
		}
		else 
		{
			if(r_shaders->value && !(s->texinfo->flags & SURF_FLOWING)) 
			{
				rs_shader = (rscript_t *)s->texinfo->image->script;
				if(rs_shader) 
					RS_Surface(s);
				else
					BSP_DrawPoly (s, s->texinfo->flags);
			}
			else
				BSP_DrawPoly (s, s->texinfo->flags);
		}

	}

	GL_TexEnv( GL_REPLACE );
	qglColor4f (1,1,1,1);
	qglDisable (GL_BLEND);
	qglDepthMask ( GL_TRUE );

	r_alpha_surfaces = NULL;
}

/*
================
R_DrawRSSurfaces

Draw shader surfaces
================
*/
void R_DrawRSSurfaces (void)
{
	msurface_t	*s = r_rscript_surfaces;

	if(!s)
		return;

	if (!r_shaders->value)
	{
		r_rscript_surfaces = NULL;
		return;
	}

	qglDepthMask(false);
	qglShadeModel (GL_SMOOTH);

	qglEnable(GL_POLYGON_OFFSET_FILL);
	qglPolygonOffset(-3, -2);

	for (; s; s = s->rscriptchain)
		RS_Surface(s);

	qglDisable(GL_POLYGON_OFFSET_FILL);

	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_ALPHATEST

	qglDepthMask(true);

	r_rscript_surfaces = NULL;
}

/*
================
BSP_RenderLightmappedPoly

Main polygon rendering routine(all standard surfaces)
================
*/
int r_currTex = -9999; //only bind a texture if it is not the same as previous surface
static void BSP_RenderLightmappedPoly( msurface_t *surf )
{
	float	scroll;
	image_t *image = BSP_TextureAnimation( surf->texinfo );
	unsigned lmtex = surf->lightmaptexturenum;
	glpoly_t *p = surf->polys;

	if(p->numverts < 1)
		return;
		
	c_brush_polys++;

	if (SurfaceIsAlphaBlended(surf))
		qglEnable( GL_ALPHA_TEST );

	scroll = 0;
	if (surf->texinfo->flags & SURF_FLOWING)
	{
		scroll = -64 * ( (r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0) );
		if (scroll == 0.0)
			scroll = -64.0;
	}
		
	if(image->texnum != r_currTex)
	{
		qglActiveTextureARB(GL_TEXTURE0);
		qglBindTexture(GL_TEXTURE_2D, image->texnum );
	}
	qglActiveTextureARB(GL_TEXTURE1);
	qglBindTexture(GL_TEXTURE_2D, gl_state.lightmap_textures + lmtex );		

	if(surf->texinfo->has_normalmap) 
	{
		//add to normal chain
		surf->normalchain = r_normalsurfaces;
		r_normalsurfaces = surf;
	}
	
	if(gl_state.vbo && surf->has_vbo && !(surf->texinfo->flags & SURF_FLOWING)) 
	{
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);

		qglEnableClientState( GL_VERTEX_ARRAY );	
		qglVertexPointer(3, GL_FLOAT, 0, (void *)surf->vbo_pos);

		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);	
		qglTexCoordPointer(2, GL_FLOAT, 0, (void*)(surf->vbo_pos + surf->xyz_size));

		qglClientActiveTextureARB (GL_TEXTURE1);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);	
		qglTexCoordPointer(2, GL_FLOAT, 0, (void*)(surf->vbo_pos + surf->xyz_size + surf->st_size));

		KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER);
				
		R_DrawVarrays (GL_POLYGON, 0, p->numverts, true);

		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
	else
	{
		R_InitVArrays (VERT_MULTI_TEXTURED);
		R_AddLightMappedSurfToVArray (surf, scroll);
	}
	
	if (SurfaceIsAlphaBlended(surf))
		qglDisable( GL_ALPHA_TEST);
}

static void BSP_RenderGLSLLightmappedPoly( msurface_t *surf )
{
	float	scroll;
	unsigned lmtex = surf->lightmaptexturenum;
	glpoly_t *p = surf->polys;

	if(p->numverts < 1)
		return;
		
	c_brush_polys++;

	if (SurfaceIsAlphaBlended(surf))
		qglEnable( GL_ALPHA_TEST );

	scroll = 0;
	if (surf->texinfo->flags & SURF_FLOWING)
	{
		scroll = -64 * ( (r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0) );
		if (scroll == 0.0)
			scroll = -64.0;
	}	

	if(surf->texinfo->image->texnum != r_currTex) 
	{
		glUniform1iARB( g_location_surfTexture, 0);
		qglActiveTextureARB(GL_TEXTURE0);
		qglBindTexture(GL_TEXTURE_2D, surf->texinfo->image->texnum);
			
		glUniform1iARB( g_location_heightTexture, 1);
		qglActiveTextureARB(GL_TEXTURE1);
		qglBindTexture(GL_TEXTURE_2D, surf->texinfo->heightMap->texnum);
		
		glUniform1iARB( g_location_normalTexture, 2);
		qglActiveTextureARB(GL_TEXTURE2);
		qglBindTexture(GL_TEXTURE_2D, surf->texinfo->normalMap->texnum);
		KillFlags |= KILL_TMU2_POINTER;
	}

	glUniform1iARB( g_location_lmTexture, 3);
	qglActiveTextureARB(GL_TEXTURE3);
	qglBindTexture(GL_TEXTURE_2D, gl_state.lightmap_textures + lmtex);
	KillFlags |= KILL_TMU3_POINTER;

	glUniformMatrix3fvARB( g_tangentSpaceTransform,	1, GL_FALSE, (const GLfloat *) surf->tangentSpaceTransform );
	
	if(gl_state.vbo && surf->has_vbo && !(surf->texinfo->flags & SURF_FLOWING)) 
	{
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);

		qglEnableClientState( GL_VERTEX_ARRAY );	
		qglVertexPointer(3, GL_FLOAT, 0, (void *)surf->vbo_pos);

		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);	
		qglTexCoordPointer(2, GL_FLOAT, 0, (void*)(surf->vbo_pos + surf->xyz_size));

		qglClientActiveTextureARB (GL_TEXTURE1);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);	
		qglTexCoordPointer(2, GL_FLOAT, 0, (void*)(surf->vbo_pos + surf->xyz_size + surf->st_size));

		KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER);
				
		R_DrawVarrays (GL_POLYGON, 0, p->numverts, true);

		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
	else
	{
		R_InitVArrays (VERT_MULTI_TEXTURED);
		R_AddLightMappedSurfToVArray (surf, scroll);
	}
	
	if (SurfaceIsAlphaBlended(surf))
		qglDisable( GL_ALPHA_TEST);
}

static void BSP_RenderGLSLDynamicLightmappedPoly( msurface_t *surf )
{
	float	scroll;
	unsigned lmtex = surf->lightmaptexturenum;
	glpoly_t *p = surf->polys;

	if(p->numverts < 1)
		return;
		
	c_brush_polys++;

	if (SurfaceIsAlphaBlended(surf))
		qglEnable( GL_ALPHA_TEST );

	scroll = 0;
	if (surf->texinfo->flags & SURF_FLOWING)
	{
		scroll = -64 * ( (r_newrefdef.time / 40.0) - (int)(r_newrefdef.time / 40.0) );
		if (scroll == 0.0)
			scroll = -64.0;
	}

	if(gl_normalmaps->value && surf->texinfo->has_heightmap) 
		glUniform1iARB( g_location_parallax, 1);
	else
	{
		glUniform1iARB( g_location_parallax, 0);
		//add to normal chain(this must be done since this surface is not self shadowed with glsl by worldlights)
		surf->normalchain = r_normalsurfaces;
		r_normalsurfaces = surf;
	}

	if(surf->texinfo->image->texnum != r_currTex) 
	{
		glUniform1iARB( g_location_surfTexture, 0);
		qglActiveTextureARB(GL_TEXTURE0);
		qglBindTexture(GL_TEXTURE_2D, surf->texinfo->image->texnum);
			
		glUniform1iARB( g_location_heightTexture, 1);
		qglActiveTextureARB(GL_TEXTURE1);
		qglBindTexture(GL_TEXTURE_2D, surf->texinfo->heightMap->texnum);
		
		glUniform1iARB( g_location_normalTexture, 2);
		qglActiveTextureARB(GL_TEXTURE2);
		qglBindTexture(GL_TEXTURE_2D, surf->texinfo->normalMap->texnum);
		KillFlags |= KILL_TMU2_POINTER;
	}

	glUniform1iARB( g_location_lmTexture, 3);
	qglActiveTextureARB(GL_TEXTURE3);
	qglBindTexture(GL_TEXTURE_2D, gl_state.lightmap_textures + lmtex);
	KillFlags |= KILL_TMU3_POINTER;	
	
	glUniformMatrix3fvARB( g_tangentSpaceTransform,	1, GL_FALSE, (const GLfloat *) surf->tangentSpaceTransform );
	
	if(gl_state.vbo && surf->has_vbo && !(surf->texinfo->flags & SURF_FLOWING)) 
	{
		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, vboId);

		qglEnableClientState( GL_VERTEX_ARRAY );	
		qglVertexPointer(3, GL_FLOAT, 0, (void *)surf->vbo_pos);

		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);	
		qglTexCoordPointer(2, GL_FLOAT, 0, (void*)(surf->vbo_pos + surf->xyz_size));

		qglClientActiveTextureARB (GL_TEXTURE1);
		qglEnableClientState(GL_TEXTURE_COORD_ARRAY);	
		qglTexCoordPointer(2, GL_FLOAT, 0, (void*)(surf->vbo_pos + surf->xyz_size + surf->st_size));

		KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER);
				
		R_DrawVarrays (GL_POLYGON, 0, p->numverts, true);

		qglBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}
	else
	{
		R_InitVArrays (VERT_MULTI_TEXTURED);
		R_AddLightMappedSurfToVArray (surf, scroll);
	}
	
	if (SurfaceIsAlphaBlended(surf))
		qglDisable( GL_ALPHA_TEST);
}

void BSP_DrawGLSLSurfaces (void)
{
	msurface_t	*s = r_glsl_surfaces;

	if(!s)
		return;

	r_currTex = -99999;

	glUseProgramObjectARB( g_programObj );
	
	glUniform3fARB( g_location_eyePos, r_origin[0], r_origin[1], r_origin[2] );
	glUniform1iARB( g_location_fog, map_fog);
	glUniform3fARB( g_location_staticLightPosition, r_worldLightVec[0], r_worldLightVec[1], r_worldLightVec[2]);
		
	if(r_shadowmapcount == 2)
	{
		//static vegetation shadow
		glUniform1iARB( g_location_bspShadowmapTexture2, 6);
		qglActiveTextureARB(GL_TEXTURE6);
		qglBindTexture(GL_TEXTURE_2D, r_depthtexture2->texnum);

		glUniform1iARB( g_location_shadowmap, 1);
		glUniform1iARB( g_Location_statshadow, 1 );
	}
	else
	{
		glUniform1iARB( g_location_shadowmap, 0);
		glUniform1iARB( g_Location_statshadow, 0);
	}

	glUniform1iARB( g_location_dynamic, 0);
	glUniform1iARB( g_location_parallax, 1);
	
	for (; s; s = s->glslchain) {
		BSP_RenderGLSLLightmappedPoly(s);
		r_currTex = s->texinfo->image->texnum;
	}

	glUseProgramObjectARB( 0 );

	r_glsl_surfaces = NULL;
}

void BSP_DrawGLSLDynamicSurfaces (void)
{
	msurface_t	*s = r_glsl_dynamic_surfaces;
	dlight_t	*dl = NULL;
	int			lnum, sv_lnum = 0;
	float		add, brightest = 0;
	vec3_t		lightVec;
	float		lightCutoffSquared = 0.0f;
	qboolean	foundLight = false;

	if(!s)
		return;

	dl = r_newrefdef.dlights;
	for (lnum=0; lnum<r_newrefdef.num_dlights; lnum++, dl++)
	{
		VectorSubtract (r_origin, dl->origin, lightVec);
		add = dl->intensity - VectorLength(lightVec)/10;
		if (add > brightest) //only bother with lights close by
		{
			brightest = add;
			sv_lnum = lnum; //remember the position of most influencial light
		}
	}

	glUseProgramObjectARB( g_programObj );
	
	glUniform3fARB( g_location_eyePos, r_origin[0], r_origin[1], r_origin[2] );
	glUniform1iARB( g_location_fog, map_fog);
	glUniform3fARB( g_location_staticLightPosition, r_worldLightVec[0], r_worldLightVec[1], r_worldLightVec[2]);

	if(brightest > 0) 
	{ 
		//we have a light
		foundLight= true;
		dynLight = r_newrefdef.dlights;
		dynLight += sv_lnum; //our most influential light

		lightCutoffSquared = ( dynLight->intensity - DLIGHT_CUTOFF );

		if( lightCutoffSquared <= 0.0f )
			lightCutoffSquared = 0.0f;

		lightCutoffSquared *= 2.0f;
		lightCutoffSquared *= lightCutoffSquared;		

		r_currTex = -99999;		

		glUniform3fARB( g_location_lightPosition, dynLight->origin[0], dynLight->origin[1], dynLight->origin[2]);
		glUniform3fARB( g_location_lightColour, dynLight->color[0], dynLight->color[1], dynLight->color[2]);

		glUniform1fARB( g_location_lightCutoffSquared, lightCutoffSquared);
		
		if(gl_shadowmaps->value) 
		{
			//dynamic shadow
			glUniform1iARB( g_location_bspShadowmapTexture, 7);
			qglActiveTextureARB(GL_TEXTURE7);
			qglBindTexture(GL_TEXTURE_2D, r_depthtexture->texnum);

			glUniform1iARB( g_location_shadowmap, 1);
		}
		else
			glUniform1iARB( g_location_shadowmap, 0);		
	}

	glUniform1iARB( g_location_dynamic, foundLight);
	
	for (; s; s = s->glsldynamicchain) 
	{
		BSP_RenderGLSLDynamicLightmappedPoly(s);
		r_currTex = s->texinfo->image->texnum;
	}

	glUseProgramObjectARB( 0 );
	
	qglActiveTextureARB (GL_TEXTURE1);
	qglDisable (GL_TEXTURE_2D);	

	r_glsl_dynamic_surfaces = NULL;
}

/*
================
BSP_DrawNormalSurfaces

Fixed function rendering of non-glsl normalmapped surfaces
================
*/

static void BSP_InitNormalSurfaces ()
{
	qglActiveTextureARB (GL_TEXTURE0);
	qglDisable (GL_TEXTURE_2D);
	qglEnable (GL_TEXTURE_CUBE_MAP_ARB);

	qglBindTexture (GL_TEXTURE_CUBE_MAP_ARB, normalisationCubeMap);
	qglMatrixMode (GL_TEXTURE);
	qglLoadIdentity ();

	// rotate around Y to bring blue/pink to the front
	qglRotatef (145, 0, 1, 0);

	// now reposition so that the bright spot is center screen, and up a little
	qglRotatef (-45, 1, 0, 0);
	qglRotatef (-45, 0, 0, 1);

	//rotate by view angles(area in front of view brightens up, lower depth)
	qglRotatef (-r_newrefdef.viewangles[2], 1, 0, 0);
	qglRotatef (-r_newrefdef.viewangles[0], 0, 1, 0);
	qglRotatef (-r_newrefdef.viewangles[1], 0, 0, 1);

	// the next 2 statements will move the cmstr calculations into hardware so that we don;t
	// have to evaluate them once per vert...

	// translate after rotation
	qglTranslatef (r_newrefdef.vieworg[0], r_newrefdef.vieworg[1], r_newrefdef.vieworg[2]);

	// now flip everything by -1 again to mimic the software calculation
	// we won't bother batching this part up...
	qglScalef (-1, -1, -1);

	qglMatrixMode (GL_MODELVIEW);

	// basic replace texenv
	qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

void BSP_KillNormalTMUs(void) {

	//kill TMU1
	qglActiveTextureARB (GL_TEXTURE1);
	qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglDisable (GL_TEXTURE_2D);

	//kill TMU0
	qglActiveTextureARB (GL_TEXTURE0);
	qglMatrixMode (GL_TEXTURE);
	qglLoadIdentity ();
	qglMatrixMode (GL_MODELVIEW);
	qglDisable (GL_TEXTURE_CUBE_MAP_ARB);
	qglEnable (GL_TEXTURE_2D);
}

static void BSP_DrawNormalSurfaces (void)
{
	msurface_t *surf = r_normalsurfaces;

	// nothing to draw!
	if (!surf)
		return;

	if (!gl_normalmaps->value)
	{
		r_normalsurfaces = NULL;
		return;
	}

	R_InitVArrays (VERT_BUMPMAPPED);
	BSP_InitNormalSurfaces();

	qglActiveTextureARB (GL_TEXTURE1);
	qglEnable (GL_TEXTURE_2D);

	qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_DOT3_RGB);
	qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
	qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
	qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);

	qglEnable (GL_BLEND);

	// set the correct blending mode for normal maps
	qglBlendFunc (GL_ZERO, GL_SRC_COLOR);

	for (; surf; surf = surf->normalchain)
	{
		if (SurfaceIsAlphaBlended(surf))
			continue;

		qglBindTexture (GL_TEXTURE_2D, surf->texinfo->normalMap->texnum);
		
		R_AddTexturedSurfToVArray (surf, 0);
	}

	BSP_KillNormalTMUs();
	
	// restore original blend
	qglTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDisable (GL_BLEND);

	r_normalsurfaces = NULL;
}

void BSP_AddToTextureChain(msurface_t *surf)
{
	int map;
	qboolean is_dynamic = false;

	if(r_newrefdef.num_dlights && gl_state.glsl_shaders && gl_glsl_shaders->value)
	{
		for ( map = 0; map < MAXLIGHTMAPS && surf->styles[map] != 255; map++ )
		{
			if ( r_newrefdef.lightstyles[surf->styles[map]].white != surf->cached_light[map] )
				goto dynamic;
		}

		// dynamic this frame or dynamic previously
		if ( ( surf->dlightframe == r_framecount ) )
		{
	dynamic:
			if ( gl_dynamic->value )
			{
				if ( !SurfaceHasNoLightmap(surf) )
					is_dynamic = true;
			}
		}
	}

	if(is_dynamic && surf->texinfo->has_normalmap
		&& gl_state.glsl_shaders && gl_glsl_shaders->value) //always glsl for dynamic if it has a normalmap
	{
		surf->glsldynamicchain = r_glsl_dynamic_surfaces;
		r_glsl_dynamic_surfaces = surf;
	}
	else if(gl_normalmaps->value && surf->texinfo->has_heightmap
			&& surf->texinfo->has_normalmap
			&& gl_state.glsl_shaders && gl_glsl_shaders->value) 
	{
		surf->glslchain = r_glsl_surfaces;
		r_glsl_surfaces = surf;
	}
	else 
	{
		BSP_RenderLightmappedPoly(surf);
		r_currTex = surf->texinfo->image->texnum;
	}
}

/*
=================
BSP_DrawInlineBModel
=================
*/
void BSP_DrawInlineBModel ( void )
{
	int			i;
	cplane_t	*pplane;
	float		dot;
	msurface_t	*psurf;

	R_PushDlightsForBModel ( currententity );

	if ( currententity->flags & RF_TRANSLUCENT )
	{
		qglEnable (GL_BLEND);
		qglColor4f (1,1,1,0.25);
		GL_TexEnv( GL_MODULATE );
	}

	r_currTex = -99999;
		
	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];
	for (i=0 ; i<currentmodel->nummodelsurfaces ; i++, psurf++)
	{
		// find which side of the plane we are on
		pplane = psurf->plane;
		dot = DotProduct (modelorg, pplane->normal) - pplane->dist;

		// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON)) ||
			(!(psurf->flags & SURF_PLANEBACK) && (dot > BACKFACE_EPSILON)))
		{
			if (SurfaceIsTranslucent(psurf) && !SurfaceIsAlphaBlended(psurf))
			{	// add to the translucent chain
				psurf->texturechain = r_alpha_surfaces;
				r_alpha_surfaces = psurf;
				psurf->entity = currententity;
			}
			else if ( !( psurf->flags & SURF_DRAWTURB ) )
			{
				BSP_AddToTextureChain( psurf );
			}
			else
			{
				GL_EnableMultitexture( false );
				BSP_RenderBrushPoly( psurf );
				GL_EnableMultitexture( true );
			}

			psurf->visframe = r_framecount;
		}
	}

	//render all GLSL surfaces
	if(gl_state.glsl_shaders && gl_glsl_shaders->value)
	{
		BSP_DrawGLSLSurfaces();
		BSP_DrawGLSLDynamicSurfaces();
	}

	if ( !(currententity->flags & RF_TRANSLUCENT) )
	{
		GL_EnableMultitexture( false );
		BSP_DrawNormalSurfaces();
		GL_EnableMultitexture( true );
	}
	else
	{
		qglDisable (GL_BLEND);
		qglColor4f (1,1,1,1);
		GL_TexEnv( GL_REPLACE );
	}

	R_KillVArrays ();
}

/*
=================
R_DrawBrushModel
=================
*/
void R_DrawBrushModel ( void )
{
	vec3_t		mins, maxs;
	int			i;
	qboolean	rotated;

	if (currentmodel->nummodelsurfaces == 0)
		return;

	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	if (currententity->angles[0] || currententity->angles[1] || currententity->angles[2])
	{
		rotated = true;
		for (i=0 ; i<3 ; i++)
		{
			mins[i] = currententity->origin[i] - currentmodel->radius;
			maxs[i] = currententity->origin[i] + currentmodel->radius;
		}
	}
	else
	{
		rotated = false;
		VectorAdd (currententity->origin, currentmodel->mins, mins);
		VectorAdd (currententity->origin, currentmodel->maxs, maxs);
	}

	if (R_CullBox (mins, maxs)) {
		return;
	}

	qglColor3f (1,1,1);
	memset (gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

	VectorSubtract (r_newrefdef.vieworg, currententity->origin, modelorg);

	if (rotated)
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		VectorCopy (modelorg, temp);
		AngleVectors (currententity->angles, forward, right, up);
		modelorg[0] = DotProduct (temp, forward);
		modelorg[1] = -DotProduct (temp, right);
		modelorg[2] = DotProduct (temp, up);
	}

    qglPushMatrix ();
	currententity->angles[0] = -currententity->angles[0];	// stupid quake bug
	currententity->angles[2] = -currententity->angles[2];	// stupid quake bug
	R_RotateForEntity (currententity);
	currententity->angles[0] = -currententity->angles[0];	// stupid quake bug
	currententity->angles[2] = -currententity->angles[2];	// stupid quake bug

	GL_EnableMultitexture( true );
	GL_SelectTexture( GL_TEXTURE0);

	if ( !gl_config.mtexcombine ) {

		GL_TexEnv( GL_REPLACE );

		GL_SelectTexture( GL_TEXTURE1);

		if ( gl_lightmap->value )
			GL_TexEnv( GL_REPLACE );
		else
			GL_TexEnv( GL_MODULATE );

	} else {
		GL_TexEnv ( GL_COMBINE_EXT );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
		GL_SelectTexture( GL_TEXTURE1 );
		GL_TexEnv ( GL_COMBINE_EXT );

		if ( gl_lightmap->value ) {
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
		} else {
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_MODULATE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT );
		}

		if ( r_overbrightbits->value ) {
			qglTexEnvi ( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, r_overbrightbits->value );
		}

	}

	BSP_DrawInlineBModel ();
	GL_EnableMultitexture( false );

	qglPopMatrix ();
}

/*
=============================================================

	WORLD MODEL

=============================================================
*/

/*
================
BSP_RecursiveWorldNode
================
*/
void BSP_RecursiveWorldNode (mnode_t *node, int clipflags)
{
	int			c, side;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot;
	image_t		*image;
	rscript_t *rs_shader;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	if (!r_nocull->value)
	{
		int i, clipped;
		cplane_t *clipplane;

		for (i=0,clipplane=frustum ; i<4 ; i++,clipplane++)
		{
			clipped = BoxOnPlaneSide (node->minmaxs, node->minmaxs+3, clipplane);

			if (clipped == 1)
				clipflags &= ~(1<<i);	// node is entirely on screen
			else if (clipped == 2)
				return;					// fully clipped
		}
	}

	// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (! (r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		if (! (c = pleaf->nummarksurfaces) )
			return;

		do
		{
			(*mark++)->visframe = r_framecount;
		} while (--c);

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	plane = node->plane;

	switch (plane->type)
	{
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
		break;
	}

	side = !(dot >= 0);

	// recurse down the children, front side first
	BSP_RecursiveWorldNode (node->children[side], clipflags);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != side )
			continue;		// wrong side

		if ( !( surf->flags & SURF_DRAWTURB ) )
		{
			if (R_CullBox (surf->mins, surf->maxs)) 
				continue;
		}

		if (surf->texinfo->flags & SURF_SKY)
		{	// just adds to visible sky bounds
			R_AddSkySurface (surf);
		}
		else if (SurfaceIsTranslucent(surf) && !SurfaceIsAlphaBlended(surf))
		{	// add to the translucent chain
			surf->texturechain = r_alpha_surfaces;
			r_alpha_surfaces = surf;
			surf->entity = NULL;
		}
		else
		{
			if ( !( surf->flags & SURF_DRAWTURB ) )
			{
				BSP_AddToTextureChain( surf );
	
				if(r_shaders->value) { //only add to the chain if there is actually a shader
					rs_shader = (rscript_t *)surf->texinfo->image->script;
					if(rs_shader || (surf->flags & SURF_UNDERWATER)) {
						surf->rscriptchain = r_rscript_surfaces;
						r_rscript_surfaces = surf;
					}
				}
			}
			else
			{
				// the polygon is visible, so add it to the texture
				// sorted chain

				// FIXME: this is a hack for animation
				image = BSP_TextureAnimation (surf->texinfo);

				surf->texturechain = image->texturechain;
				image->texturechain = surf;

				if(r_shaders->value) { //only add to the chain if there is actually a shader
					rs_shader = (rscript_t *)surf->texinfo->image->script;
					if(rs_shader) {
						surf->rscriptchain = r_rscript_surfaces;
						r_rscript_surfaces = surf;
					}
				}
			}
		}
	}

	// recurse down the back side
	BSP_RecursiveWorldNode (node->children[!side], clipflags);
}

/*
=============
R_CalcWorldLights - this is the fallback for non deluxmapped bsp's
=============
*/
void R_CalcWorldLights( void )
{	
	int		i, j;
	vec3_t	lightAdd, temp;
	float	dist, weight;
	int		numlights = 0;

	if(gl_glsl_shaders->value && gl_state.glsl_shaders)
	{
		//get light position relative to player's position
		VectorClear(lightAdd);
		for (i = 0; i < r_lightgroups; i++) 
		{
			VectorSubtract(r_origin, LightGroups[i].group_origin, temp);
			dist = VectorLength(temp);
			if(dist == 0)
				dist = 1;
			dist = dist*dist;
			weight = (int)250000/(dist/(LightGroups[i].avg_intensity+1.0f));
			for(j = 0; j < 3; j++)
				lightAdd[j] += LightGroups[i].group_origin[j]*weight;
			numlights+=weight;
		}

		if(numlights > 0.0) 
		{
			for(i = 0; i < 3; i++)
				r_worldLightVec[i] = (lightAdd[i]/numlights + r_origin[i])/2.0;
		}
	}
}

/*
=============
R_DrawWorld
=============
*/
void R_DrawWorld (void)
{
	entity_t	ent;
	
	if (!r_drawworld->value)
		return;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;

	currentmodel = r_worldmodel;

	VectorCopy (r_newrefdef.vieworg, modelorg);

	// auto cycle the world frame for texture animation
	memset (&ent, 0, sizeof(ent));
	ent.frame = (int)(r_newrefdef.time*2);
	currententity = &ent;

	gl_state.currenttextures[0] = gl_state.currenttextures[1] = -1;

	qglColor3f (1,1,1);
	memset (gl_lms.lightmap_surfaces, 0, sizeof(gl_lms.lightmap_surfaces));

	R_CalcWorldLights();

	R_ClearSkyBox ();

	GL_EnableMultitexture( true );

	GL_SelectTexture( GL_TEXTURE0);

	if ( !gl_config.mtexcombine ) 
	{
		GL_TexEnv( GL_REPLACE );
			GL_SelectTexture( GL_TEXTURE1);

		if ( gl_lightmap->value )
			GL_TexEnv( GL_REPLACE );
		else
			GL_TexEnv( GL_MODULATE );

	} else 
	{
		GL_TexEnv ( GL_COMBINE_EXT );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );
		qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
		GL_SelectTexture( GL_TEXTURE1 );
		GL_TexEnv ( GL_COMBINE_EXT );

		if ( gl_lightmap->value ) 
		{
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
		} else 
		{
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_MODULATE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
			qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT );
		}

		if ( r_overbrightbits->value )
			qglTexEnvi ( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, r_overbrightbits->value );
	}

	r_currTex = -99999;

	BSP_RecursiveWorldNode (r_worldmodel->nodes, 15);

	//render all GLSL surfaces
	if(gl_state.glsl_shaders && gl_glsl_shaders->value)
	{
		BSP_DrawGLSLSurfaces();
		BSP_DrawGLSLDynamicSurfaces();
	}
	
	GL_EnableMultitexture( false );

	DrawTextureChains ();

	//render fixed function normalmap self shadowing
	BSP_DrawNormalSurfaces ();

	R_KillVArrays ();

	R_InitSun();

	qglDepthMask(0);
	R_DrawSkyBox();
	qglDepthMask(1);

	BSP_DrawTriangleOutlines ();
}


/*
===============
R_MarkLeaves

Mark the leaves and nodes that are in the PVS for the current
cluster
===============
*/
void R_MarkLeaves (void)
{
	byte	*vis;
	byte	fatvis[MAX_MAP_LEAFS/8];
	mnode_t	*node;
	int		i, c;
	mleaf_t	*leaf;
	int		cluster;

	if (r_oldviewcluster == r_viewcluster && r_oldviewcluster2 == r_viewcluster2 && !r_novis->value && r_viewcluster != -1)
		return;

	// development aid to let you run around and see exactly where
	// the pvs ends
	if (gl_lockpvs->value)
		return;

	r_visframecount++;
	r_oldviewcluster = r_viewcluster;
	r_oldviewcluster2 = r_viewcluster2;

	if (r_novis->value || r_viewcluster == -1 || !r_worldmodel->vis)
	{
		// mark everything
		for (i=0 ; i<r_worldmodel->numleafs ; i++)
			r_worldmodel->leafs[i].visframe = r_visframecount;
		for (i=0 ; i<r_worldmodel->numnodes ; i++)
			r_worldmodel->nodes[i].visframe = r_visframecount;
		return;
	}

	vis = Mod_ClusterPVS (r_viewcluster, r_worldmodel);
	// may have to combine two clusters because of solid water boundaries
	if (r_viewcluster2 != r_viewcluster)
	{
		memcpy (fatvis, vis, (r_worldmodel->numleafs+7)/8);
		vis = Mod_ClusterPVS (r_viewcluster2, r_worldmodel);
		c = (r_worldmodel->numleafs+31)/32;
		for (i=0 ; i<c ; i++)
			((int *)fatvis)[i] |= ((int *)vis)[i];
		vis = fatvis;
	}

	for (i=0,leaf=r_worldmodel->leafs ; i<r_worldmodel->numleafs ; i++, leaf++)
	{
		cluster = leaf->cluster;
		if (cluster == -1)
			continue;
		if (vis[cluster>>3] & (1<<(cluster&7)))
		{
			node = (mnode_t *)leaf;
			do
			{
				if (node->visframe == r_visframecount)
					break;
				node->visframe = r_visframecount;
				node = node->parent;
			} while (node);
		}
	}
}



/*
=============================================================================

  LIGHTMAP ALLOCATION

=============================================================================
*/

static void LM_InitBlock( void )
{
	memset( gl_lms.allocated, 0, sizeof( gl_lms.allocated ) );
}

static void LM_UploadBlock( qboolean dynamic )
{
	int texture;
	int height = 0;

	if ( dynamic )
	{
		texture = 0;
	}
	else
	{
		texture = gl_lms.current_lightmap_texture;
	}

	GL_Bind( gl_state.lightmap_textures + texture );
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ( dynamic )
	{
		int i;

		for ( i = 0; i < LIGHTMAP_SIZE; i++ )
		{
			if ( gl_lms.allocated[i] > height )
				height = gl_lms.allocated[i];
		}

		qglTexSubImage2D( GL_TEXTURE_2D,
						  0,
						  0, 0,
						  LIGHTMAP_SIZE, height,
						  GL_LIGHTMAP_FORMAT,
						  GL_UNSIGNED_BYTE,
						  gl_lms.lightmap_buffer );
	}
	else
	{
		qglTexImage2D( GL_TEXTURE_2D,
					   0,
					   gl_lms.internal_format,
					   LIGHTMAP_SIZE, LIGHTMAP_SIZE,
					   0,
					   GL_LIGHTMAP_FORMAT,
					   GL_UNSIGNED_BYTE,
					   gl_lms.lightmap_buffer );
		if ( ++gl_lms.current_lightmap_texture == MAX_LIGHTMAPS )
			Com_Error( ERR_DROP, "LM_UploadBlock() - MAX_LIGHTMAPS exceeded\n" );
	}
}

// returns a texture number and the position inside it
static qboolean LM_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;

	best = LIGHTMAP_SIZE;

	for (i=0 ; i<LIGHTMAP_SIZE-w ; i++)
	{
		best2 = 0;

		for (j=0 ; j<w ; j++)
		{
			if (gl_lms.allocated[i+j] >= best)
				break;
			if (gl_lms.allocated[i+j] > best2)
				best2 = gl_lms.allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			*x = i;
			*y = best = best2;
		}
	}

	if (best + h > LIGHTMAP_SIZE)
		return false;

	for (i=0 ; i<w ; i++)
		gl_lms.allocated[*x + i] = best + h;

	return true;
}

/*
================
BSP_BuildPolygonFromSurface
================
*/
void BSP_BuildPolygonFromSurface(msurface_t *fa)
{
	int			i, lindex, lnumverts;
	medge_t		*r_pedge;
	float		*vec;
	float		s, t;
	glpoly_t	*poly;

	vec3_t surfmaxs = {-99999999, -99999999, -99999999};
	vec3_t surfmins = {99999999, 99999999, 99999999};

	// reconstruct the polygon
	lnumverts = fa->numedges;

	//
	// draw texture
	//
	poly = Hunk_Alloc (sizeof(glpoly_t) + (lnumverts-4) * VERTEXSIZE*sizeof(float));
	poly->next = fa->polys;
	poly->numverts = lnumverts;
	fa->polys = poly;

	for (i=0 ; i<lnumverts ; i++)
	{
		lindex = currentmodel->surfedges[fa->firstedge + i];

		if (lindex > 0)
		{
			r_pedge = &currentmodel->edges[lindex];
			vec = currentmodel->vertexes[r_pedge->v[0]].position;
		}
		else
		{
			r_pedge = &currentmodel->edges[-lindex];
			vec = currentmodel->vertexes[r_pedge->v[1]].position;
		}

		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= fa->texinfo->image->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= fa->texinfo->image->height;

		VectorCopy (vec, poly->verts[i]);
		poly->verts[i][3] = s;
		poly->verts[i][4] = t;

		// set bbox for the surf used for culling
		if (vec[0] > surfmaxs[0]) surfmaxs[0] = vec[0];
		if (vec[1] > surfmaxs[1]) surfmaxs[1] = vec[1];
		if (vec[2] > surfmaxs[2]) surfmaxs[2] = vec[2];

		if (vec[0] < surfmins[0]) surfmins[0] = vec[0];
		if (vec[1] < surfmins[1]) surfmins[1] = vec[1];
		if (vec[2] < surfmins[2]) surfmins[2] = vec[2];

		//
		// lightmap texture coordinates
		//
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s -= fa->texturemins[0];
		s += fa->light_s*16;
		s += 8;
		s /= LIGHTMAP_SIZE*16; //fa->texinfo->texture->width;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t -= fa->texturemins[1];
		t += fa->light_t*16;
		t += 8;
		t /= LIGHTMAP_SIZE*16; //fa->texinfo->texture->height;

		poly->verts[i][5] = s;
		poly->verts[i][6] = t;

		//to do - check if needed
		s = DotProduct (vec, fa->texinfo->vecs[0]) + fa->texinfo->vecs[0][3];
		s /= 128;

		t = DotProduct (vec, fa->texinfo->vecs[1]) + fa->texinfo->vecs[1][3];
		t /= 128;

		poly->verts[i][7] = s;
		poly->verts[i][8] = t;
	}

	// store out the completed bbox
	VectorCopy (surfmins, fa->mins);
	VectorCopy (surfmaxs, fa->maxs);
}

/*
========================
BSP_CreateSurfaceLightmap
========================
*/
void BSP_CreateSurfaceLightmap (msurface_t *surf)
{
	int		smax, tmax;
	byte	*base;

	if (!surf->samples)
		return;

	if (surf->texinfo->flags & (SURF_SKY|SURF_WARP))
		return; //may not need this?

	smax = (surf->extents[0]>>4)+1;
	tmax = (surf->extents[1]>>4)+1;

	if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
	{
		LM_UploadBlock( false );
		LM_InitBlock();
		if ( !LM_AllocBlock( smax, tmax, &surf->light_s, &surf->light_t ) )
		{
			Com_Error( ERR_FATAL, "Consecutive calls to LM_AllocBlock(%d,%d) failed\n", smax, tmax );
		}
	}

	surf->lightmaptexturenum = gl_lms.current_lightmap_texture;

	base = gl_lms.lightmap_buffer;
	base += (surf->light_t * LIGHTMAP_SIZE + surf->light_s) * LIGHTMAP_BYTES;

	R_SetCacheState( surf );
	R_BuildLightMap (surf, base, LIGHTMAP_SIZE*LIGHTMAP_BYTES);
}

/*
==================
BSP_BeginBuildingLightmaps

==================
*/
void BSP_BeginBuildingLightmaps (model_t *m)
{
	static lightstyle_t	lightstyles[MAX_LIGHTSTYLES];
	int				i;
	byte *dummy;

	memset( gl_lms.allocated, 0, sizeof(gl_lms.allocated) );

	dummy = Z_Malloc(LIGHTMAP_BYTES * LIGHTMAP_SIZE * LIGHTMAP_SIZE);

	r_framecount = 1;		// no dlightcache

	GL_EnableMultitexture( true );
	GL_SelectTexture( GL_TEXTURE1);

	/*
	** setup the base lightstyles so the lightmaps won't have to be regenerated
	** the first time they're seen
	*/
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		lightstyles[i].rgb[0] = 1;
		lightstyles[i].rgb[1] = 1;
		lightstyles[i].rgb[2] = 1;
		lightstyles[i].white = 3;
	}
	r_newrefdef.lightstyles = lightstyles;

	if (!gl_state.lightmap_textures)
		gl_state.lightmap_textures	= TEXNUM_LIGHTMAPS;

	gl_lms.current_lightmap_texture = 1;

	gl_lms.internal_format = gl_tex_solid_format;

	/*
	** initialize the dynamic lightmap texture
	*/
	GL_Bind( gl_state.lightmap_textures + 0 );
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage2D( GL_TEXTURE_2D,
				   0,
				   gl_lms.internal_format,
				   LIGHTMAP_SIZE, LIGHTMAP_SIZE,
				   0,
				   GL_LIGHTMAP_FORMAT,
				   GL_UNSIGNED_BYTE,
				   dummy );

	Z_Free(dummy);
}

/*
=======================
BSP_EndBuildingLightmaps
=======================
*/
void BSP_EndBuildingLightmaps (void)
{
	LM_UploadBlock( false );
	GL_EnableMultitexture( false );
}

/*
========================
Mini map
========================
*/

void R_RecursiveRadarNode (mnode_t *node)
{
	int			c, side, sidebit;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot,distance;
	glpoly_t	*p;
	float		*v;
	int			i;

	if (node->contents == CONTENTS_SOLID)	return;		// solid

	if(r_minimap_zoom->value>=0.1) {
		distance = 1024.0/r_minimap_zoom->value;
	} else {
		distance = 1024.0;
	}

	if ( r_origin[0]+distance < node->minmaxs[0] ||
		 r_origin[0]-distance > node->minmaxs[3] ||
		 r_origin[1]+distance < node->minmaxs[1] ||
		 r_origin[1]-distance > node->minmaxs[4] ||
		 r_origin[2]+256 < node->minmaxs[2] ||
		 r_origin[2]-256 > node->minmaxs[5]) return;

	// if a leaf node, draw stuff
	if (node->contents != -1) {
		pleaf = (mleaf_t *)node;
		// check for door connected areas
		if (r_newrefdef.areabits) {
			// not visible
			if (! (r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) ) return;
		}
		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c) {
			do {
				(*mark)->visframe = r_framecount;
				mark++;
			} while (--c);
		}
		return;
	}

// node is just a decision point, so go down the apropriate sides
// find which side of the node we are on
	plane = node->plane;

	switch (plane->type) {
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0) {
		side = 0;
		sidebit = 0;
	} else {
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

// recurse down the children, front side first
	R_RecursiveRadarNode (node->children[side]);

  if(plane->normal[2]) {
		// draw stuff
		if(plane->normal[2]>0) for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
		{
			if (surf->texinfo->flags & SURF_SKY){
				continue;
			}


		}
	} else {
			qglDisable(GL_TEXTURE_2D);
		for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++) {
			float sColor,C[4];
			if (surf->texinfo->flags & SURF_SKY) continue;

			if (surf->texinfo->flags & (SURF_WARP|SURF_FLOWING|SURF_TRANS33|SURF_TRANS66)) {
				sColor=0.5;
			} else {
				sColor=0;
			}

			for ( p = surf->polys; p; p = p->chain ) {
				v = p->verts[0];
				qglBegin(GL_LINE_STRIP);
				for (i=0 ; i< p->numverts; i++, v+= VERTEXSIZE) {
					C[3]= (v[2]-r_origin[2])/512.0;
					if (C[3]>0) {

						C[0]=0.5;
						C[1]=0.5+sColor;
						C[2]=0.5;
						C[3]=1-C[3];

					}
					   else
					{
						C[0]=0.5;
						C[1]=sColor;
						C[2]=0;
						C[3]+=1;

					}

					if(C[3]<0) {
						C[3]=0;

					}
					qglColor4fv(C);
					qglVertex3fv (v);
				}

				qglEnd();
			}
		}
		qglEnable(GL_TEXTURE_2D);

  }
	// recurse down the back side
	R_RecursiveRadarNode (node->children[!side]);


}

int			numRadarEnts=0;
RadarEnt_t	RadarEnts[MAX_RADAR_ENTS];

void R_DrawRadar(void)
{
	int		i;
	float	fS[4]={0,0,-1.0/512.0,0};

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) return;
	if(!r_minimap->value) return;

	qglViewport (vid.width-r_minimap_size->value,0, r_minimap_size->value, r_minimap_size->value);

	qglDisable(GL_DEPTH_TEST);
  	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity ();


	if (r_minimap_style->value) {
		qglOrtho(-1024,1024,-1024,1024,-256,256);
	} else {
		qglOrtho(-1024,1024,-512,1536,-256,256);
	}

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity ();


		{
		qglStencilMask(255);
		qglClear(GL_STENCIL_BUFFER_BIT);
		qglEnable(GL_STENCIL_TEST);
		qglStencilFunc(GL_ALWAYS,4,4);
		qglStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);


		GLSTATE_ENABLE_ALPHATEST;
		qglAlphaFunc(GL_LESS,0.1);
		qglColorMask(0,0,0,0);

		qglColor4f(1,1,1,1);
		if(r_around)
			GL_Bind(r_around->texnum);
		qglBegin(GL_TRIANGLE_FAN);
		if (r_minimap_style->value){
			qglTexCoord2f(0,1); qglVertex3f(1024,-1024,1);
			qglTexCoord2f(1,1); qglVertex3f(-1024,-1024,1);
			qglTexCoord2f(1,0); qglVertex3f(-1024,1024,1);
			qglTexCoord2f(0,0); qglVertex3f(1024,1024,1);
		} else {
			qglTexCoord2f(0,1); qglVertex3f(1024,-512,1);
			qglTexCoord2f(1,1); qglVertex3f(-1024,-512,1);
			qglTexCoord2f(1,0); qglVertex3f(-1024,1536,1);
			qglTexCoord2f(0,0); qglVertex3f(1024,1536,1);
		}
		qglEnd();

		qglColorMask(1,1,1,1);
		GLSTATE_DISABLE_ALPHATEST;
		qglAlphaFunc(GL_GREATER, 0.5);
		qglStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
		qglStencilFunc(GL_NOTEQUAL,4,4);

	}

	if(r_minimap_zoom->value>=0.1) {
		qglScalef(r_minimap_zoom->value,r_minimap_zoom->value,r_minimap_zoom->value);
	}

	if (r_minimap_style->value) {
		qglPushMatrix();
		qglRotatef (90-r_newrefdef.viewangles[1],  0, 0, -1);

		qglDisable(GL_TEXTURE_2D);
		qglBegin(GL_TRIANGLES);
		qglColor4f(1,1,1,0.5);
		qglVertex3f(0,32,0);
		qglColor4f(1,1,0,0.5);
		qglVertex3f(24,-32,0);
		qglVertex3f(-24,-32,0);
		qglEnd();

		qglPopMatrix();
	} else {
		qglDisable(GL_TEXTURE_2D);
		qglBegin(GL_TRIANGLES);
		qglColor4f(1,1,1,0.5);
		qglVertex3f(0,32,0);
		qglColor4f(1,1,0,0.5);
		qglVertex3f(24,-32,0);
		qglVertex3f(-24,-32,0);
		qglEnd();

		qglRotatef (90-r_newrefdef.viewangles[1],  0, 0, 1);
	}
	qglTranslatef (-r_newrefdef.vieworg[0],  -r_newrefdef.vieworg[1],  -r_newrefdef.vieworg[2]);

	qglBegin(GL_QUADS);
	for(i=0;i<numRadarEnts;i++){
		float x=RadarEnts[i].org[0];
		float y=RadarEnts[i].org[1];
		float z=RadarEnts[i].org[2];
		qglColor4fv(RadarEnts[i].color);

		qglVertex3f(x+9, y+9, z);
		qglVertex3f(x+9, y-9, z);
		qglVertex3f(x-9, y-9, z);
		qglVertex3f(x-9, y+9, z);
	}
	qglEnd();

	qglEnable(GL_TEXTURE_2D);

	if(r_radarmap)
		GL_Bind(r_radarmap->texnum);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE);
	GLSTATE_ENABLE_BLEND;
	qglColor3f(1,1,1);

	fS[3]=0.5+ r_newrefdef.vieworg[2]/512.0;
	qglTexGenf(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);

	GLSTATE_ENABLE_TEXGEN;
	qglTexGenfv(GL_S,GL_OBJECT_PLANE,fS);

	// draw the stuff
	R_RecursiveRadarNode (r_worldmodel->nodes);

	qglBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	GLSTATE_DISABLE_TEXGEN;

	qglPopMatrix();

	qglViewport(0,0,vid.width,vid.height);

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglDisable(GL_STENCIL_TEST);
	GL_TexEnv( GL_REPLACE );
	GLSTATE_DISABLE_BLEND;
	qglEnable(GL_DEPTH_TEST);
    qglColor4f(1,1,1,1);

}

