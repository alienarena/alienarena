/*
Copyright (C) 2010 COR Entertainment, LLC.

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

#include "r_local.h"
#include "r_ragdoll.h"
#include <GL/gl.h>

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

GLuint	fboId[3];
GLuint	rboId;

void getOpenGLFunctionPointers(void)
{
	// FBO
	qglGenFramebuffersEXT		= (PFNGLGENFRAMEBUFFERSEXTPROC)		qwglGetProcAddress("glGenFramebuffersEXT");
	qglBindFramebufferEXT		= (PFNGLBINDFRAMEBUFFEREXTPROC)		qwglGetProcAddress("glBindFramebufferEXT");
	qglFramebufferTexture2DEXT	= (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)	qwglGetProcAddress("glFramebufferTexture2DEXT");
	qglCheckFramebufferStatusEXT	= (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)	qwglGetProcAddress("glCheckFramebufferStatusEXT");
	qglGenRenderbuffersEXT		= (PFNGLGENRENDERBUFFERSEXTPROC)qwglGetProcAddress("glGenRenderbuffersEXT");
    qglBindRenderbufferEXT		= (PFNGLBINDRENDERBUFFEREXTPROC)qwglGetProcAddress("glBindRenderbufferEXT");
    qglRenderbufferStorageEXT	= (PFNGLRENDERBUFFERSTORAGEEXTPROC)qwglGetProcAddress("glRenderbufferStorageEXT");
	qglFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)qwglGetProcAddress("glFramebufferRenderbufferEXT");
	qglBlitFramebufferEXT = (PFNGLBLITFRAMEBUFFEREXTPROC)qwglGetProcAddress("glBlitFramebufferEXT");
}

//used for post process stencil volume blurring and shadowmapping
void R_GenerateShadowFBO()
{
	int shadowMapWidth = vid.width * r_shadowmapratio->value;
    int shadowMapHeight = vid.height * r_shadowmapratio->value;
	GLenum FBOstatus;

	gl_state.fbo = true;

	getOpenGLFunctionPointers();

	if(!qglGenFramebuffersEXT || !qglBindFramebufferEXT || !qglFramebufferTexture2DEXT || !qglCheckFramebufferStatusEXT
		|| !qglGenRenderbuffersEXT || !qglBindRenderbufferEXT || !qglRenderbufferStorageEXT || !qglFramebufferRenderbufferEXT)
	{
		Com_Printf("...GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use FBO\n");
		gl_state.fbo = false;
		return;
	}

	// Framebuffer object blit
	gl_state.hasFBOblit = false;
	if (strstr(gl_config.extensions_string, "GL_EXT_framebuffer_blit"))
	{
		Com_Printf("...using GL_EXT_framebuffer_blit\n");
		gl_state.hasFBOblit = true;
	} else
	{
		Com_Printf("...GL_EXT_framebuffer_blit not found\n");
		gl_state.hasFBOblit = false;
	}

	//FBO for shadowmapping
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
	qglGenFramebuffersEXT(1, &fboId[0]);

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId[0]);

	// Instruct openGL that we won't bind a color texture with the currently binded FBO
	qglDrawBuffer(GL_NONE);
	qglReadBuffer(GL_NONE);

	// attach the texture to FBO depth attachment point
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D, r_depthtexture->texnum, 0);

	// check FBO status
	FBOstatus = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		Com_Printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use FBO\n");
		gl_state.fbo = false;
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	//FBO for capturing stencil volumes

	//must check for abilit to blit(Many old ATI drivers do not support)
	if(gl_state.hasFBOblit) {
		if(!qglBlitFramebufferEXT) {
			Com_Printf("qglBlitFramebufferEXT not found...\n");
			//no point in continuing on
			gl_state.hasFBOblit = false;
			return;
		}
	}

    qglBindTexture(GL_TEXTURE_2D, r_colorbuffer->texnum);
    qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vid.width, vid.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    qglBindTexture(GL_TEXTURE_2D, 0);

	qglGenFramebuffersEXT(1, &fboId[1]);
    qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId[1]);

    qglGenRenderbuffersEXT(1, &rboId);
    qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, rboId);
    qglRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_STENCIL_EXT, vid.width, vid.height);
    qglBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);

    // attach a texture to FBO color attachement point
    qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, r_colorbuffer->texnum, 0);

    // attach a renderbuffer to depth attachment point
    qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rboId);

	// attach a renderbuffer to stencil attachment point
    qglFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, rboId);

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId[1]);

	// check FBO status
	FBOstatus = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE_EXT)
		Com_Printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use secondary FBO\n");

	//Second FBO for shadowmapping
	qglBindTexture(GL_TEXTURE_2D, r_depthtexture2->texnum);

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
	qglGenFramebuffersEXT(1, &fboId[2]);

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId[2]);

	// Instruct openGL that we won't bind a color texture with the currently binded FBO
	qglDrawBuffer(GL_NONE);
	qglReadBuffer(GL_NONE);

	// attach the texture to FBO depth attachment point
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D, r_depthtexture2->texnum, 0);

	// check FBO status
	FBOstatus = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		Com_Printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use FBO\n");
		gl_state.fbo = false;
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}

	// switch back to window-system-provided framebuffer
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

#define M3D_INV_PI_DIV_180 (57.2957795130823229)
#define m3dRadToDeg(x)	((x)*M3D_INV_PI_DIV_180)


static void normalize( float vec[3] )
{
	float len;
	len = sqrt( vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] );
	if ( len > 0 ) {
		vec[0] /= len;
		vec[1] /= len;
		vec[2] /= len;
	}
}

static void cross( float result[3] , float v1[3] , float v2[3] )
{
	result[0] = v1[1] * v2[2] - v1[2] * v2[1];
	result[1] = v1[2] * v2[0] - v1[0] * v2[2];
	result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}


static void lookAt( float position_x , float position_y , float position_z , float lookAt_x , float lookAt_y , float lookAt_z )
{
	float forward[3] , side[3] , up[3];
	float matrix[4][4];
	int i;

	// forward = normalized( lookAt - position )
	forward[0] = lookAt_x - position_x;
	forward[1] = lookAt_y - position_y;
	forward[2] = lookAt_z - position_z;
	normalize( forward );

	// side = normalized( forward x [0;1;0] )
	side[0] = -forward[2];
	side[1] = 0;
	side[2] = forward[0];
	normalize( side );

	// up = side x forward
	cross( up , side , forward );

	// Build matrix
	for ( i = 0 ; i < 3 ; i ++ ) {
		matrix[i][0] = side[i];
		matrix[i][1] = up[i];
		matrix[i][2] = - forward[i];
	}
	for ( i = 0 ; i < 3 ; i ++ )
		matrix[i][3] = matrix[3][i] = 0;
	matrix[3][3] = 1;

	qglMultMatrixf( (const GLfloat *) &matrix[0][0] );
	qglTranslated( -position_x , -position_y , -position_z );
}

void SM_SetupMatrices(float position_x,float position_y,float position_z,float lookAt_x,float lookAt_y,float lookAt_z)
{

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	MYgluPerspective(120.0f, vid.width/vid.height, 10.0f, 4096.0f);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
	lookAt( position_x , position_y , position_z , lookAt_x , lookAt_y , lookAt_z );
}

void SM_SetTextureMatrix( qboolean mapnum )
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
	if(mapnum)
	{		
		qglActiveTextureARB(GL_TEXTURE6); 
		qglBindTexture(GL_TEXTURE_2D, r_depthtexture2->texnum);
	}
	else
	{
		qglActiveTextureARB(GL_TEXTURE7);
		qglBindTexture(GL_TEXTURE_2D, r_depthtexture->texnum);
	}

	qglLoadIdentity();
	qglLoadMatrixd(bias);

	// concatating all matrice into one.
	qglMultMatrixd (projection);
	qglMultMatrixd (modelView);

	// Go back to normal matrix mode
	qglMatrixMode(GL_MODELVIEW);

	qglActiveTextureARB(GL_TEXTURE0);
}

/*
================
SM_RecursiveWorldNode - this variant of the classic routine renders both sides for shadowing
================
*/

void SM_RecursiveWorldNode (mnode_t *node, int clipflags)
{
	int			c;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;

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

	// recurse down the children, front side first
	SM_RecursiveWorldNode (node->children[0], clipflags);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

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
			if (!( surf->flags & SURF_DRAWTURB ) )
			{
				BSP_DrawTexturelessPoly (surf);
			}
		}
	}

	// recurse down the back side
	SM_RecursiveWorldNode (node->children[1], clipflags);
}


/*
=============
R_DrawWorld
=============
*/
void R_DrawShadowMapWorld (void)
{
	int i;

	if (!r_drawworld->value)
		return;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;

	SM_RecursiveWorldNode (r_worldmodel->nodes, 15);

	//draw brush models
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// transluscent

		currentmodel = currententity->model;

		if (!currentmodel)
		{
			continue;
		}
		if( currentmodel->type == mod_brush)
			BSP_DrawTexturelessBrushModel (currententity);
		else
			continue;
	}
}

void R_DrawDynamicCaster(void)
{
	int		i;
	vec3_t	dist, mins, maxs;
	trace_t	r_trace;
	int		RagDollID;

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	r_shadowmapcount = 0;
		
	if(!dynLight)  
		return; //we have no lights of consequence

	qglBindTexture(GL_TEXTURE_2D, r_depthtexture->texnum);

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fboId[0]);

	// In the case we render the shadowmap to a higher resolution, the viewport must be modified accordingly.
	qglViewport(0,0,(int)(vid.width * r_shadowmapratio->value),(int)(vid.height * r_shadowmapratio->value));  //for now

	// Clear previous frame values
	qglClear( GL_DEPTH_BUFFER_BIT);

	//Disable color rendering, we only want to write to the Z-Buffer
	qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// Culling switching, rendering only frontfaces
	qglCullFace(GL_BACK);

	// attach the texture to FBO depth attachment point
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D, r_depthtexture->texnum, 0);

	//set camera
	SM_SetupMatrices(dynLight->origin[0],dynLight->origin[1],dynLight->origin[2]+64,dynLight->origin[0],dynLight->origin[1],dynLight->origin[2]-128);

	qglEnable( GL_POLYGON_OFFSET_FILL );
    qglPolygonOffset( 0.5f, 0.5f );

	//render world - very basic geometry
	R_DrawShadowMapWorld();

	//render entities near light
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];

		if (currententity->flags & RF_NOSHADOWS || currententity->flags & RF_TRANSLUCENT)
			continue;

		if(!currententity->model)
			continue;

		if (currententity->model->type != mod_alias && currententity->model->type != mod_iqm)
		{
			continue;
		}

		if(r_ragdolls->value && currententity->model->type == mod_iqm && currententity->model->hasRagDoll)
		{
			if(currententity->frame > 198)
				continue;
		}

		//distance from light, if too far, don't render(to do - check against brightness for dist!)
		VectorSubtract(dynLight->origin, currententity->origin, dist);
		if(VectorLength(dist) > 256.0f)
			continue;

		//trace visibility from light - we don't render objects the light doesn't hit!
		r_trace = CM_BoxTrace(dynLight->origin, currententity->origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
		if(r_trace.fraction != 1.0)
			continue;

		currentmodel = currententity->model;

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

		if(currentmodel->type == mod_iqm)
			IQM_DrawCaster ();
		else
			MD2_DrawCaster ();
	}

	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		if(RagDoll[RagDollID].destroyed)
	        continue;
		
		//distance from light, if too far, don't render(to do - check against brightness for dist!)
		VectorSubtract(dynLight->origin, RagDoll[RagDollID].curPos, dist);
		if(VectorLength(dist) > 256.0f)
			continue;

		//trace visibility from light - we don't render objects the light doesn't hit!
		r_trace = CM_BoxTrace(dynLight->origin, RagDoll[RagDollID].curPos, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
		if(r_trace.fraction != 1.0)
			continue;

		IQM_DrawRagDollCaster (RagDollID);
	}

	SM_SetTextureMatrix(0);

	r_shadowmapcount = 1;

	dynLight = NULL; //done with dynamic light shadows for this frame

	qglDepthMask (1);		// back to writing

	qglPolygonOffset( 0.0f, 0.0f );
    qglDisable( GL_POLYGON_OFFSET_FILL );

}

void R_DrawVegetationCasters ( void )
{
    int		i, k;
	grass_t *grass;
    float   scale;
	vec3_t	origin, mins, maxs, angle, right, up, corner[4];
	float	*corner0 = corner[0];
	qboolean visible;
	trace_t r_trace;
	float	sway;

	if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	grass = r_grasses;

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs,	0, 0, 0);

	R_InitVArrays (VERT_SINGLE_TEXTURED);

    for (i=0; i<r_numgrasses; i++, grass++) {

		scale = 10.0*grass->size;

		VectorCopy(r_newrefdef.viewangles, angle); // to do - this angle should always be calc'ed from the sun/target

		if(!grass->type)
			angle[0] = 0;  // keep vertical by removing pitch(grass and plants grow upwards)

		AngleVectors(angle, NULL, right, up);
		VectorScale(right, scale, right);
		VectorScale(up, scale, up);
		VectorCopy(grass->origin, origin);

		// adjust vertical position, scaled
		origin[2] += (grass->texsize/32) * grass->size;

		if(!grass->type) {
			r_trace = CM_BoxTrace(r_origin, origin, maxs, mins, r_worldmodel->firstnode, MASK_VISIBILILITY);
			visible = r_trace.fraction == 1.0;
		}
		else
			visible = true; //leaves tend to use much larger images, culling results in undesired effects

		//to do - cull all of these for pathline to sunlight

		if(visible) {

			//render grass polygon
			GL_Bind(grass->texnum);

			GLSTATE_ENABLE_ALPHATEST

			qglColor4f( grass->color[0],grass->color[1],grass->color[2], 1 );

			VectorSet (corner[0],
				origin[0] + (up[0] + right[0])*(-0.5),
				origin[1] + (up[1] + right[1])*(-0.5),
				origin[2] + (up[2] + right[2])*(-0.5));

			//the next two statements create a slight swaying in the wind
			//perhaps we should add a parameter to control ammount in shader?

			if(grass->type) {
				sway = 3;
			}
			else
				sway = 2;

			VectorSet ( corner[1],
				corner0[0] + up[0] + sway*sin (rs_realtime*sway),
				corner0[1] + up[1] + sway*sin (rs_realtime*sway),
				corner0[2] + up[2]);

			VectorSet ( corner[2],
				corner0[0] + (up[0]+right[0] + sway*sin (rs_realtime*sway)),
				corner0[1] + (up[1]+right[1] + sway*sin (rs_realtime*sway)),
				corner0[2] + (up[2]+right[2]));

			VectorSet ( corner[3],
				corner0[0] + right[0],
				corner0[1] + right[1],
				corner0[2] + right[2]);

			VArray = &VArrayVerts[0];

			for(k = 0; k < 4; k++) {

				VArray[0] = corner[k][0];
				VArray[1] = corner[k][1];
				VArray[2] = corner[k][2];

				switch(k) {
					case 0:
						VArray[3] = 1;
						VArray[4] = 1;
						break;
					case 1:
						VArray[3] = 0;
						VArray[4] = 1;
						break;
					case 2:
						VArray[3] = 0;
						VArray[4] = 0;
						break;
					case 3:
						VArray[3] = 1;
						VArray[4] = 0;
						break;
				}

				VArray += VertexSizes[VERT_SINGLE_TEXTURED];
			}

			if(qglLockArraysEXT)
				qglLockArraysEXT(0, 4);

			qglDrawArrays(GL_QUADS,0,4);

			if(qglUnlockArraysEXT)
				qglUnlockArraysEXT();

			r_shadowmapcount = 2;
		}
	}

	R_KillVArrays ();

	qglColor4f( 1,1,1,1 );
	GLSTATE_DISABLE_ALPHATEST
}

void R_DrawVegetationCaster(void)
{	
	qglBindTexture(GL_TEXTURE_2D, r_depthtexture2->texnum); 

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fboId[2]); 

	// In the case we render the shadowmap to a higher resolution, the viewport must be modified accordingly.
	qglViewport(0,0,(int)(vid.width * r_shadowmapratio->value),(int)(vid.height * r_shadowmapratio->value));  //for now

	//Clear previous frame values
	qglClear( GL_DEPTH_BUFFER_BIT);

	//Disable color rendering, we only want to write to the Z-Buffer
	qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// Culling switching, rendering only frontfaces
	qglDisable(GL_CULL_FACE);

	// attach the texture to FBO depth attachment point
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D, r_depthtexture2->texnum, 0);

	//get sun light origin and target from map info, at map load
	//set camera
	SM_SetupMatrices(r_sunLight->origin[0],r_sunLight->origin[1],r_sunLight->origin[2],r_sunLight->target[0],r_sunLight->target[1],r_sunLight->target[2]);

	qglEnable( GL_POLYGON_OFFSET_FILL );
    qglPolygonOffset( 0.5f, 0.5f );

	//render vegetation
	R_DrawVegetationCasters(); 
	
	SM_SetTextureMatrix(1);
		
	qglDepthMask (1);		// back to writing

	qglPolygonOffset( 0.0f, 0.0f );
    qglDisable( GL_POLYGON_OFFSET_FILL );
	qglEnable(GL_CULL_FACE);

}
