/*
Copyright (C) 1997 - 2001 Id Software, Inc.

This program is free software; you can redistribute it and / or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 - 1307, USA.

*/ 

#include "r_local.h"
#include <GL/gl.h>
#include <GL/glu.h>

extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

// GL_EXT_framebuffer_object
GLboolean(APIENTRY * qglIsRenderbufferEXT) (GLuint renderbuffer);
void            (APIENTRY * qglBindRenderbufferEXT) (GLenum target, GLuint renderbuffer);
void            (APIENTRY * qglDeleteRenderbuffersEXT) (GLsizei n, const GLuint * renderbuffers);
void            (APIENTRY * qglGenRenderbuffersEXT) (GLsizei n, GLuint * renderbuffers);
void            (APIENTRY * qglRenderbufferStorageEXT) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void            (APIENTRY * qglGetRenderbufferParameterivEXT) (GLenum target, GLenum pname, GLint * params);

GLboolean(APIENTRY * qglIsFramebufferEXT) (GLuint framebuffer);
void            (APIENTRY * qglBindFramebufferEXT) (GLenum target, GLuint framebuffer);
void            (APIENTRY * qglDeleteFramebuffersEXT) (GLsizei n, const GLuint * framebuffers);
void            (APIENTRY * qglGenFramebuffersEXT) (GLsizei n, GLuint * framebuffers);

GLenum(APIENTRY * qglCheckFramebufferStatusEXT) (GLenum target);
void            (APIENTRY * qglFramebufferTexture1DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
														 GLint level);
void            (APIENTRY * qglFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
														 GLint level);
void            (APIENTRY * qglFramebufferTexture3DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
														 GLint level, GLint zoffset);
void            (APIENTRY * qglFramebufferRenderbufferEXT) (GLenum target, GLenum attachment, GLenum renderbuffertarget,
															GLuint renderbuffer);
void            (APIENTRY * qglGetFramebufferAttachmentParameterivEXT) (GLenum target, GLenum attachment, GLenum pname,
																		GLint * params);
void            (APIENTRY * qglGenerateMipmapEXT) (GLenum target);

// GL_EXT_framebuffer_blit
void			(APIENTRY * qglBlitFramebufferEXT) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

GLuint	fboId;
CasterGroup_t ShadowCasterGroups[40];

void getOpenGLFunctionPointers(void)
{
	// FBO
	qglGenFramebuffersEXT		= (PFNGLGENFRAMEBUFFERSEXTPROC)		qwglGetProcAddress("glGenFramebuffersEXT");
	qglBindFramebufferEXT		= (PFNGLBINDFRAMEBUFFEREXTPROC)		qwglGetProcAddress("glBindFramebufferEXT");
	qglFramebufferTexture2DEXT	= (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)	qwglGetProcAddress("glFramebufferTexture2DEXT");
	qglCheckFramebufferStatusEXT	= (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)	qwglGetProcAddress("glCheckFramebufferStatusEXT");
}

void generateShadowFBO()
{
	int shadowMapWidth = vid.width * r_shadowmapratio->value;
	int shadowMapHeight = vid.height * r_shadowmapratio->value;
	
	GLenum FBOstatus;

	getOpenGLFunctionPointers();

	if(!qglGenFramebuffersEXT || !qglBindFramebufferEXT || !qglFramebufferTexture2DEXT || !qglCheckFramebufferStatusEXT) {
		Com_Printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use FBO\n");
		return;
	}

	qglBindTexture(GL_TEXTURE_2D, r_depthtexture->texnum);

	// GL_LINEAR does not make sense for depth texture. However, next tutorial shows usage of GL_LINEAR and PCF
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		
	// Remove artefact on the edges of the shadowmap
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
		
	// No need to force GL_DEPTH_COMPONENT24, drivers usually give you the max precision if available 
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);

	// create a framebuffer object
	qglGenFramebuffersEXT(1, &fboId);
			
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId);

	// Instruct openGL that we won't bind a color texture with the currently binded FBO
	qglDrawBuffer(GL_NONE);
	qglReadBuffer(GL_NONE);
		
	// attach the texture to FBO depth attachment point
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D, r_depthtexture->texnum, 0);
		
	// check FBO status
	FBOstatus = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE_EXT)
		Com_Printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use FBO\n");
	else {
		Com_Printf("...Using framebuffer object\n");
	}
	
	// switch back to window-system-provided framebuffer
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}
#define M3D_INV_PI_DIV_180 (57.2957795130823229)
#define m3dRadToDeg(x)	((x)*M3D_INV_PI_DIV_180)

void setupMatrices(float position_x,float position_y,float position_z,float lookAt_x,float lookAt_y,float lookAt_z)
{

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	MYgluPerspective(120.0f, vid.width/vid.height, 10.0f, 4096.0f); 
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
	gluLookAt(position_x,position_y,position_z,lookAt_x,lookAt_y,lookAt_z,0,1,0);
}

void setTextureMatrix( void )
{
	static double modelView[16];
	static double projection[16];

	// Moving from unit cube [-1,1] to [0,1]  
	const GLdouble bias[16] = {	
		0.5, 0.0, 0.0, 0.0, 
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0};
	
	// Grab modelview and transformation matrices
	qglGetDoublev(GL_MODELVIEW_MATRIX, modelView);
	qglGetDoublev(GL_PROJECTION_MATRIX, projection);
	
	qglMatrixMode(GL_TEXTURE);
	qglActiveTextureARB(GL_TEXTURE7);
	qglBindTexture(GL_TEXTURE_2D, r_depthtexture->texnum);

	qglLoadIdentity();	
	qglLoadMatrixd(bias);
	
	// concatating all matrice into one.
	qglMultMatrixd (projection);
	qglMultMatrixd (modelView);
	
	// Go back to normal matrix mode
	qglMatrixMode(GL_MODELVIEW);
}

/*
================
R_RecursiveWorldNode
================
*/
static vec3_t modelorg;			// relative to viewpoint
extern void DrawGLTexturelessPoly (msurface_t *fa);
void R_RecursiveShadowMapWorldNode (mnode_t *node, int clipflags)
{
	int			c, side, sidebit;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot;

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

	if (dot >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_RecursiveShadowMapWorldNode (node->children[side], clipflags);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;		// wrong side

		if (surf->texinfo->flags & SURF_SKY)
		{	// no skies here
			continue;
		}
		else if (SurfaceIsTranslucent(surf) && !SurfaceIsAlphaBlended(surf))
		{	// no trans surfaces
			continue;
		}
		else
		{
			if ( qglMTexCoord2fSGIS && !( surf->flags & SURF_DRAWTURB ) )
			{
				DrawGLTexturelessPoly (surf);
			}
		}
	}

	// recurse down the back side
	R_RecursiveShadowMapWorldNode (node->children[!side], clipflags);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawShadowMapWorld (void)
{
	if (!r_drawworld->value)
		return;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;

	currentmodel = r_worldmodel;

	VectorCopy (r_newrefdef.vieworg, modelorg);

	R_RecursiveShadowMapWorldNode (r_worldmodel->nodes, 15);
}

void R_DrawDynamicCaster(void)
{
	int			sv_lnum = 0, lnum, i;
	vec3_t		lightVec, dist, mins, maxs;
	dlight_t	*dl;
	float		add, brightest = 0;
	trace_t		r_trace;

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	//get avg of dynamic lights that are nearby enough to matter.
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
	if(brightest > 0) { //we have a light
		dl = r_newrefdef.dlights;
		dl += sv_lnum; //our most influential light
	}
	else
		return; 

	qglBindTexture(GL_TEXTURE_2D, r_depthtexture->texnum);

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fboId); 
	
	// In the case we render the shadowmap to a higher resolution, the viewport must be modified accordingly.
	qglViewport(0,0,(int)(vid.width * r_shadowmapratio->value),(int)(vid.height * r_shadowmapratio->value));  //for now

	// Clear previous frame values
	qglClear( GL_DEPTH_BUFFER_BIT);
		
	//Disable color rendering, we only want to write to the Z-Buffer
	qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); 
		
	// Culling switching, rendering only frontfaces - to do - really?
	qglCullFace(GL_BACK);

	// attach the texture to FBO depth attachment point
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D, r_depthtexture->texnum, 0);
			
	//set camera
	setupMatrices(dl->origin[0],dl->origin[1],dl->origin[2]+64,dl->origin[0],dl->origin[1],dl->origin[2]-64);

	qglEnable( GL_POLYGON_OFFSET_FILL );
    qglPolygonOffset( 0.5f, 0.5f );
    
	//render world - very basic geometry
	R_DrawShadowMapWorld();

	//render entities near light
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{

		float thresh, offset;
		vec3_t temp;
		
		currententity = &r_newrefdef.entities[i];

		if (currententity->flags & RF_NOSHADOWS || currententity->flags & RF_TRANSLUCENT)
			continue;

		if(!currententity->model)
			continue;

		if (currententity->model->type != mod_alias)
		{
			continue;
		}
			
		//distance from light, if too far, don't render(to do - check against brightness for dist!)
		VectorSubtract(dl->origin, currententity->origin, dist);
		if(VectorLength(dist) > 256.0f)
			continue;						
	
		//trace visibility from light - we don't render objects the light doesn't hit!
		r_trace = CM_BoxTrace(dl->origin, currententity->origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
		if(r_trace.fraction != 1.0)
			continue;

		currentmodel = currententity->model;

		//make sure the entity is at a reasonable angle to the light source.  We don't want extreme angles producing giant shadows
		//as this will cause strange artifacts.
		VectorCopy(dl->origin, temp);
		temp[2] = currententity->origin[2];
		
		VectorSubtract(temp, currententity->origin, dist);
			
		// Only player models have these frames, which need to account for jump animations that often extend beyond original bbox calcs
		// hacky, but necessary.
		if(currententity->frame > 65 && currententity->frame < 69)
			offset = currentmodel->maxs[2] + 42;
		else				
			offset = currentmodel->maxs[2];
		thresh = dl->origin[2]+64 - (currententity->origin[2] + offset);
		if(VectorLength(dist) > 4.75f * thresh) { //within a good angle of light source
			continue; 
		}		

		//get view distance, set lod if available
		VectorSubtract(r_origin, currententity->origin, dist);
		if(VectorLength(dist) > 300) {
			if(currententity->lod2)
				currentmodel = currententity->lod2;
		}
		else if(VectorLength(dist) > 100) {
			if(currententity->lod1) 
				currentmodel = currententity->lod1;
		}
		
		R_DrawAliasModelCaster (currententity);
	}
	
	setTextureMatrix();

	qglDepthMask (1);		// back to writing  
	
	qglPolygonOffset( 0.0f, 0.0f );
    qglDisable( GL_POLYGON_OFFSET_FILL );

}
