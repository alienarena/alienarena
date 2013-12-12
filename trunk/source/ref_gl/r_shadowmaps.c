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

void R_CheckFBOExtensions (void)
{
	gl_state.fbo = true;
	gl_state.hasFBOblit = false;

	qglGenFramebuffersEXT		= (PFNGLGENFRAMEBUFFERSEXTPROC)		qwglGetProcAddress("glGenFramebuffersEXT");
	qglBindFramebufferEXT		= (PFNGLBINDFRAMEBUFFEREXTPROC)		qwglGetProcAddress("glBindFramebufferEXT");
	qglFramebufferTexture2DEXT	= (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)	qwglGetProcAddress("glFramebufferTexture2DEXT");
	qglCheckFramebufferStatusEXT	= (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)	qwglGetProcAddress("glCheckFramebufferStatusEXT");
	qglGenRenderbuffersEXT		= (PFNGLGENRENDERBUFFERSEXTPROC)qwglGetProcAddress("glGenRenderbuffersEXT");
	qglBindRenderbufferEXT		= (PFNGLBINDRENDERBUFFEREXTPROC)qwglGetProcAddress("glBindRenderbufferEXT");
	qglRenderbufferStorageEXT	= (PFNGLRENDERBUFFERSTORAGEEXTPROC)qwglGetProcAddress("glRenderbufferStorageEXT");
	qglFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)qwglGetProcAddress("glFramebufferRenderbufferEXT");
	qglBlitFramebufferEXT = (PFNGLBLITFRAMEBUFFEREXTPROC)qwglGetProcAddress("glBlitFramebufferEXT");

	if(!qglGenFramebuffersEXT || !qglBindFramebufferEXT || !qglFramebufferTexture2DEXT || !qglCheckFramebufferStatusEXT
		|| !qglGenRenderbuffersEXT || !qglBindRenderbufferEXT || !qglRenderbufferStorageEXT || !qglFramebufferRenderbufferEXT)
	{
		Com_Printf("...Cannot find OpenGL Framebuffer extension, CANNOT use FBO\n");
		gl_state.fbo = false;
		return;
	}

	// Framebuffer object blit
	if (strstr(gl_config.extensions_string, "GL_EXT_framebuffer_blit"))
	{
		Com_Printf("...using GL_EXT_framebuffer_blit\n");
		gl_state.hasFBOblit = true;
	} else
	{
		Com_Printf("...GL_EXT_framebuffer_blit not found\n");
	}
	
	//must check for ability to blit(Many old ATI drivers do not support)
	//TODO: redundant with previous check?
	if(gl_state.hasFBOblit) {
		if(!qglBlitFramebufferEXT) {
			Com_Printf("glBlitFramebufferEXT not found...\n");
			gl_state.hasFBOblit = false;
		}
	}
}
    

//used for post process stencil volume blurring and shadowmapping
void R_GenerateShadowFBO()
{
	int shadowMapWidth = vid.width * r_shadowmapscale->value;
	int shadowMapHeight = vid.height * r_shadowmapscale->value;
	GLenum FBOstatus;
	
	if (!gl_state.fbo || !gl_state.hasFBOblit)
	return;

	//FBO for shadowmapping
	qglBindTexture(GL_TEXTURE_2D, r_depthtexture->texnum);

	// GL_LINEAR removes pixelation - GL_NEAREST removes artifacts on outer edges in some cases
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Remove artefact on the edges of the shadowmap
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

	// This is to allow usage of shadow2DProj function in the shader
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	qglTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY); 

	// No need to force GL_DEPTH_COMPONENT24, drivers usually give you the max precision if available
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
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

	//Second FBO for shadowmapping
	qglBindTexture(GL_TEXTURE_2D, r_depthtexture2->texnum);

	// GL_LINEAR removes pixelation - GL_NEAREST removes artifacts on outer edges in some cases
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// This is to allow usage of shadow2DProj function in the shader
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	qglTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY); 

	// No need to force GL_DEPTH_COMPONENT24, drivers usually give you the max precision if available
	qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	qglBindTexture(GL_TEXTURE_2D, 0);

	// create a framebuffer object
	qglGenFramebuffersEXT(1, &fboId[1]);

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId[1]);

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

	//FBO for capturing stencil volumes

    qglBindTexture(GL_TEXTURE_2D, r_colorbuffer->texnum);
    qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vid.width, vid.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    qglBindTexture(GL_TEXTURE_2D, 0);

	qglGenFramebuffersEXT(1, &fboId[2]);
    qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId[2]);

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

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId[2]);

	// check FBO status
	FBOstatus = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if(FBOstatus != GL_FRAMEBUFFER_COMPLETE_EXT)
		Com_Printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use secondary FBO\n");

	// In the case we render the shadowmap to a higher resolution, the viewport must be modified accordingly.
	qglViewport(0,0,vid.width,vid.height); 
	
	// Initialize frame values.
	// This only makes a difference if the viewport is less than the screen
	// size, like when the netgraph is on-- otherwise it's redundant with 
	// later glClear calls.
	qglClearColor (1, 1, 1, 1);
	qglClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
	
	// back to previous screen coordinates
	R_SetupViewport ();

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
	MYgluPerspective(120.0f, r_newrefdef.width/r_newrefdef.height, 10.0f, 4096.0f);
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
	
	GL_InvalidateTextureState (); // FIXME

	// Grab modelview and transformation matrices
	qglGetDoublev(GL_MODELVIEW_MATRIX, modelView);
	qglGetDoublev(GL_PROJECTION_MATRIX, projection);

	qglMatrixMode(GL_TEXTURE);
	if(mapnum)
		GL_MBind (6, r_depthtexture2->texnum);
	else
		GL_MBind (7, r_depthtexture->texnum);

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
			if (!(clipflags & (1<<i)))
				continue;
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

		if (R_CullBox (surf->mins, surf->maxs)) 
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
			if (!( surf->iflags & ISURF_DRAWTURB ) )
			{
				BSP_DrawTexturelessPoly (surf);
			}
		}
	}

	// recurse down the back side
	SM_RecursiveWorldNode (node->children[1], clipflags);
}


inline float point_dist_from_plane (cplane_t *plane, vec3_t point)
{
	switch (plane->type)
	{
	case PLANE_X:
		return point[0] - plane->dist;
	case PLANE_Y:
		return point[1] - plane->dist;
	case PLANE_Z:
		return point[2] - plane->dist;
	default:
		return DotProduct (point, plane->normal) - plane->dist;
	}
}

/*
================
SM_RecursiveWorldNode2 - this variant of the classic routine renders only one side for shadowing
================
*/

static float fadeshadow_cutoff;

void SM_RecursiveWorldNode2 (mnode_t *node, int clipflags, vec3_t origin, vec3_t absmins, vec3_t absmaxs)
{
	int			c;
	float		dist, dist_model, dist_light;
	int			side, side_model, side_light;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	qboolean	caster_off_plane;

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
			if (!(clipflags & (1<<i)))
				continue;
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
	
	dist = point_dist_from_plane (plane, r_origin);
	dist_model = point_dist_from_plane (plane, origin);
	dist_light = point_dist_from_plane (plane, statLightPosition);
	
	side = dist < 0;
	side_model = dist_model < 0;
	side_light = dist_light < 0;
	
	dist = fabs (dist);
	dist_model = fabs (dist_model);
	dist_light = fabs (dist_light);
	
	//TODO: figure out the cutoff distance based on mins and maxs?
	caster_off_plane = (dist_model > 64.0f) || (BoxOnPlaneSide (absmins, absmaxs, plane) != 3);
	
	if (side != side_model && caster_off_plane)
	{
		if (side != side_light)
		{
			if (dist_light < dist_model)
				goto skip_draw;
		}
		else
			goto skip_draw;
	}
	
	// recurse down the children, front side first
	SM_RecursiveWorldNode2 (node->children[side], clipflags, origin, absmins, absmaxs);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if (R_CullBox (surf->mins, surf->maxs)) 
			continue;
			
		if (surf->texinfo->flags & SURF_SKY)
		{	// no skies here
			continue;
		}
		else if (SurfaceIsTranslucent(surf) || SurfaceIsAlphaBlended(surf))
		{	// no trans surfaces
			continue;
		}
		else
		{
			if (!( surf->iflags & ISURF_DRAWTURB ) )
			{
				BSP_DrawShadowPoly (surf, origin);
			}
		}
	}
	
	if (side == side_model && caster_off_plane)
	{
		if (side == side_light)
		{
			if (dist_light < dist_model)
				return;
		}
		else
			return;
	}
	
skip_draw:

	if (dist >= fadeshadow_cutoff+512)
		return;

	// recurse down the back side
	SM_RecursiveWorldNode2 (node->children[!side], clipflags, origin, absmins, absmaxs);
}


/*
=============
R_DrawWorld
=============
*/

void R_DrawShadowMapWorld (qboolean forEnt, vec3_t origin)
{
	int i;

	if (!r_drawworld->value)
		return;

	if ( r_newrefdef.rdflags & RDF_NOWORLDMODEL )
		return;

	if(forEnt)
	{
		vec3_t absmins, absmaxs;
		
		glUseProgramObjectARB( g_shadowprogramObj );

		glUniform1iARB( g_location_entShadow, 6);
		qglActiveTextureARB(GL_TEXTURE6);
		qglBindTexture(GL_TEXTURE_2D, r_depthtexture2->texnum);

		glUniform1fARB( g_location_xOffset, 1.0/(r_newrefdef.width*r_shadowmapscale->value));
		glUniform1fARB( g_location_yOffset, 1.0/(r_newrefdef.height*r_shadowmapscale->value));

		glUniform1fARB( g_location_fadeShadow, fadeShadow );
		
		R_InitVArrays(VERT_NO_TEXTURE);
		
		VectorAdd (currentmodel->mins, origin, absmins);
		VectorAdd (currentmodel->maxs, origin, absmaxs);

		SM_RecursiveWorldNode2 (r_worldmodel->nodes, 15, origin, absmins, absmaxs);
		
		R_KillVArrays();

		glUseProgramObjectARB( 0 );

		qglActiveTextureARB(GL_TEXTURE0);

		return;
	}
	else
	{
		R_InitVArrays(VERT_NO_TEXTURE);
		
		SM_RecursiveWorldNode (r_worldmodel->nodes, 15);
		
		R_KillVArrays();

		//draw brush models(not for ent shadow, for now)
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
}

#include "r_lodcalc.h"

extern cvar_t *cl_simpleitems;
void R_DrawDynamicCaster(void)
{
	int		i;
	vec3_t	dist, mins, maxs;
	trace_t	r_trace;
	int		RagDollID;
	vec3_t origin = {0, 0, 0};

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	r_shadowmapcount = 0;
		
	if(!dynLight)  
		return; //we have no lights of consequence

	qglBindTexture(GL_TEXTURE_2D, r_depthtexture->texnum);

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fboId[0]);

	// In the case we render the shadowmap to a higher resolution, the viewport must be modified accordingly.
	qglViewport(0,0,(int)(vid.width * r_shadowmapscale->value),(int)(vid.height * r_shadowmapscale->value));  

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
	R_DrawShadowMapWorld(false, origin);

	//render entities near light
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		currententity = &r_newrefdef.entities[i];

		if (currententity->flags & RF_NOSHADOWS || currententity->flags & RF_TRANSLUCENT)
			continue;

		if(!currententity->model)
			continue;

		if (currententity->model->type <= mod_brush) 
			continue;
		
		if (cl_simpleitems->integer && currententity->model->simple_texnum != 0)
			continue; //TODO: simple items casting shadows?

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

		//get distance
		VectorSubtract(r_origin, currententity->origin, dist);
		
		//set lod if available
		if(VectorLength(dist) > LOD_DIST*(3.0/5.0))
		{
			if(currententity->lod2)
				currentmodel = currententity->lod2;
		}
		else if(VectorLength(dist) > LOD_DIST*(1.0/5.0))
		{
			if(currententity->lod1)
				currentmodel = currententity->lod1;
		}
		if (currententity->lod2)
			currentmodel = currententity->lod2;

		R_Mesh_DrawCaster ();
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
	
	GL_InvalidateTextureState (); // FIXME

}

void R_Vectoangles (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;

	if (value1[1] == 0.0f && value1[0] == 0.0f )
	{
		yaw = 0.0f;
		if (value1[2] > 0.0f)
			pitch = 90.0f;
		else
			pitch = 270.0f;
	}
	else
	{
	// PMM - fixed to correct for pitch of 0
		if (value1[0])
			yaw = (atan2(value1[1], value1[0]) * 180.0f / M_PI);
		else if (value1[1] > 0)
			yaw = 90.0f;
		else
			yaw = 270.0f;

		if (yaw < 0.0f)
			yaw += 360.0f;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (atan2(value1[2], forward) * 180.0f / M_PI);
		if (pitch < 0.0f)
			pitch += 360.0f;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0.0f;
}

void R_DrawVegetationCasters ( qboolean forShadows )
{
	int		i, k;
	grass_t *grass;
	float   scale;
	vec3_t	dir, origin, angle, right, up, corner[4];
	float	*corner0 = corner[0];
	float	sway;

	if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	grass = r_grasses;

	R_InitVArrays (VERT_SINGLE_TEXTURED);

	for (i=0; i<r_numgrasses; i++, grass++) 
	{
		if(!grass->type)
			continue; //only deal with leaves, grass shadows look kind of bad

		if(grass->sunVisible) 
		{
			scale = 10.0*grass->size;

			VectorSubtract(r_sunLight->origin, r_sunLight->target, dir);
			R_Vectoangles(dir, angle);
		
			AngleVectors(angle, NULL, right, up);
			VectorScale(right, scale, right);
			VectorScale(up, scale, up);

			VectorCopy(grass->origin, origin);

			//render grass polygon
			
			GL_SelectTexture (0);
			qglBindTexture (GL_TEXTURE_2D, grass->texnum);

			GLSTATE_ENABLE_ALPHATEST

			qglColor4f( 0, 0, 0, 1 );

			VectorSet (corner[0],
				origin[0] + (up[0] + right[0])*(-0.5),
				origin[1] + (up[1] + right[1])*(-0.5),
				origin[2] + (up[2] + right[2])*(-0.5));

			sway = 3;
			
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

			for(k = 0; k < 4; k++) 
			{
				VArray[0] = corner[k][0];
				VArray[1] = corner[k][1];
				VArray[2] = corner[k][2];

				switch(k) 
				{
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

			R_DrawVarrays(GL_QUADS, 0, 4);

			if(forShadows)
				r_shadowmapcount = 2;
		}
	}

	R_KillVArrays ();

	qglColor4f( 1,1,1,1 );
	GLSTATE_DISABLE_ALPHATEST
	
	GL_InvalidateTextureState (); // FIXME
}

void R_DrawVegetationCaster(void)
{	
	if(!r_sunLight->has_Sun || !r_hasleaves)
		return; //no point if there is no sun or leaves!

	qglBindTexture(GL_TEXTURE_2D, r_depthtexture2->texnum); 

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fboId[1]); 

	// In the case we render the shadowmap to a higher resolution, the viewport must be modified accordingly.
	qglViewport(0,0,(int)(vid.width * r_shadowmapscale->value),(int)(vid.height * r_shadowmapscale->value)); 

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
	R_DrawVegetationCasters(true); 
	
	SM_SetTextureMatrix(1);
		
	qglDepthMask (1);		// back to writing

	qglPolygonOffset( 0.0f, 0.0f );
	qglDisable( GL_POLYGON_OFFSET_FILL );
	qglEnable(GL_CULL_FACE);
}

void R_DrawEntityCaster(entity_t *ent)
{		
	vec3_t	dist, mins, maxs;
	trace_t	r_trace;
	model_t *prev;

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	r_shadowmapcount = 0;

	//check caster validity
	if (ent->flags & RF_NOSHADOWS || ent->flags & RF_TRANSLUCENT)
		return;

	if(!ent->model)
		return;	
	
	if (cl_simpleitems->integer && ent->model->simple_texnum != 0)
		return;

	if(r_ragdolls->value && ent->model->type == mod_iqm && ent->model->hasRagDoll)
	{
		if(ent->frame > 198)
			return;
	}
		
	//trace visibility from light - we don't render objects the light doesn't hit!
	r_trace = CM_BoxTrace(statLightPosition, ent->origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
	if(r_trace.fraction != 1.0)
		return;

	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fboId[1]); 

	// In the case we render the shadowmap to a higher resolution, the viewport must be modified accordingly.
	qglViewport(0,0,(int)(vid.width * r_shadowmapscale->value),(int)(vid.height * r_shadowmapscale->value));  

	//Clear previous frame values
	qglClear( GL_DEPTH_BUFFER_BIT);

	//Disable color rendering, we only want to write to the Z-Buffer
	qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// Culling switching, rendering only frontfaces
	qglDisable(GL_CULL_FACE);

	// attach the texture to FBO depth attachment point
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D, r_depthtexture2->texnum, 0);

	//get light origin 
	//set camera
	SM_SetupMatrices(statLightPosition[0],statLightPosition[1],statLightPosition[2]+64,ent->origin[0],ent->origin[1],ent->origin[2]-128);

	qglEnable( GL_POLYGON_OFFSET_FILL );
	qglPolygonOffset( 0.5f, 0.5f );	

	//we could loop the entities here, and render any nearby models, to make sure we get shadows on the this entity if a mesh is nearby. 
	//this would be only if we want to do self shadowing, or have player shadows cast on other mesh objects.  At this time, I think that the
	//performance losses would not be worth doing this, but we can revisit.

	prev = currentmodel;
	
	currentmodel = ent->model;

	//get view distance
	VectorSubtract(r_origin, ent->origin, dist);
		
	//set lod if available
	if(VectorLength(dist) > LOD_DIST*(3.0/5.0))
	{
		if(currententity->lod2)
			currentmodel = currententity->lod2;
	}
	else if(VectorLength(dist) > LOD_DIST*(1.0/5.0))
	{
		if(currententity->lod1)
			currentmodel = currententity->lod1;
	}
	if (currententity->lod2)
		currentmodel = currententity->lod2;

	R_Mesh_DrawCaster ();
	
	SM_SetTextureMatrix(1);
		
	qglDepthMask (1);		// back to writing

	qglPolygonOffset( 0.0f, 0.0f );
	qglDisable( GL_POLYGON_OFFSET_FILL );
	qglEnable(GL_CULL_FACE);

	// back to previous screen coordinates
	R_SetupViewport ();

	qglPopMatrix();
	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);	

	r_shadowmapcount = 1;
	
	currentmodel = prev;
	
	GL_InvalidateTextureState (); // FIXME
}

void R_GenerateEntityShadow( void )
{
	if(gl_shadowmaps->integer && gl_normalmaps->integer)
	{
		vec3_t dist, tmp;
		float rad;

		if((r_newrefdef.rdflags & RDF_NOWORLDMODEL) || (currententity->flags & RF_MENUMODEL))
			return;

		if (r_newrefdef.vieworg[2] < (currententity->origin[2] - 128))
			return;

		VectorSubtract(currententity->model->maxs, currententity->model->mins, tmp);
		VectorScale (tmp, 1.666, tmp);
		rad = VectorLength (tmp);		

		if( R_CullSphere( currententity->origin, rad, 15 ) )
			return;

		//get view distance
		VectorSubtract(r_origin, currententity->origin, dist);

		//fade out shadows both by distance from view, and by distance from light
		fadeshadow_cutoff = r_shadowcutoff->value * (LOD_DIST/LOD_BASE_DIST); 

		if (r_shadowcutoff->value < 0.1)
			fadeShadow = 1.0;
		else if (VectorLength (dist) > fadeshadow_cutoff)
		{
			fadeShadow = VectorLength(dist) - fadeshadow_cutoff;
			if (fadeShadow > 512)
				return;
			else
				fadeShadow = 1.0 - fadeShadow/512.0; //fade out smoothly over 512 units.
		}
		else
			fadeShadow = 1.0;
		
		qglEnable(GL_DEPTH_TEST);
		qglClearColor(0,0,0,1.0f);

		qglHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);

		R_DrawEntityCaster(currententity);

		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);

		//Enabling color write (previously disabled for light POV z-buffer rendering)
		qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		//re-render affected polys with shadowmap for this entity
		if(r_shadowmapcount > 0)
		{
			qglEnable( GL_BLEND );
			qglBlendFunc (GL_ZERO, GL_SRC_COLOR);

			R_DrawShadowMapWorld(true, currententity->origin);

			qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			qglDisable ( GL_BLEND );
		}

		r_shadowmapcount = 0;
	}
}

void R_DrawRagdollCaster(int RagDollID)
{		
	vec3_t mins, maxs;
	trace_t	r_trace;

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	r_shadowmapcount = 0;
		
	//trace visibility from light - we don't render objects the light doesn't hit!
	r_trace = CM_BoxTrace(statLightPosition, RagDoll[RagDollID].origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
	if(r_trace.fraction != 1.0)
		return;

	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT,fboId[1]); 

	// In the case we render the shadowmap to a higher resolution, the viewport must be modified accordingly.
	qglViewport(0,0,(int)(vid.width * r_shadowmapscale->value),(int)(vid.height * r_shadowmapscale->value));  

	//Clear previous frame values
	qglClear( GL_DEPTH_BUFFER_BIT);

	//Disable color rendering, we only want to write to the Z-Buffer
	qglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// Culling switching, rendering only frontfaces
	qglDisable(GL_CULL_FACE);

	// attach the texture to FBO depth attachment point
	qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D, r_depthtexture2->texnum, 0);

	//get light origin 
	//set camera
	SM_SetupMatrices(statLightPosition[0],statLightPosition[1],statLightPosition[2]+64,RagDoll[RagDollID].origin[0],RagDoll[RagDollID].origin[1],RagDoll[RagDollID].origin[2]-128);

	qglEnable( GL_POLYGON_OFFSET_FILL );
	qglPolygonOffset( 0.5f, 0.5f );	
	
	IQM_DrawRagDollCaster ( RagDollID );
	
	SM_SetTextureMatrix(1);
		
	qglDepthMask (1);		// back to writing

	qglPolygonOffset( 0.0f, 0.0f );
	qglDisable( GL_POLYGON_OFFSET_FILL );
	qglEnable(GL_CULL_FACE);

	// back to previous screen coordinates
	R_SetupViewport ();

	qglPopMatrix();
	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);	

	r_shadowmapcount = 1;
	
	GL_InvalidateTextureState (); // FIXME
}

void R_GenerateRagdollShadow( int RagDollID )
{
	if(gl_shadowmaps->integer && gl_normalmaps->integer)
	{
		vec3_t dist, tmp;
		float rad;

		VectorSubtract(RagDoll[RagDollID].ragDollMesh->maxs, RagDoll[RagDollID].ragDollMesh->mins, tmp);
		VectorScale (tmp, 1.666, tmp);
		rad = VectorLength (tmp);		

		if( R_CullSphere( RagDoll[RagDollID].origin, rad, 15 ) )
			return;

		//get view distance
		VectorSubtract(r_origin, RagDoll[RagDollID].origin, dist);

		//fade out shadows both by distance from view, and by distance from light
		fadeshadow_cutoff = r_shadowcutoff->value * (LOD_DIST/LOD_BASE_DIST); 

		if (r_shadowcutoff->value < 0.1)
			fadeShadow = 1.0;
		else if (VectorLength (dist) > fadeshadow_cutoff)
		{
			fadeShadow = VectorLength(dist) - fadeshadow_cutoff;
			if (fadeShadow > 512)
				return;
			else
				fadeShadow = 1.0 - fadeShadow/512.0; //fade out smoothly over 512 units.
		}
		else
			fadeShadow = 1.0;		
		
		qglEnable(GL_DEPTH_TEST);
		qglClearColor(0,0,0,1.0f);

		qglHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);

		R_DrawRagdollCaster(RagDollID);

		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);

		//Enabling color write (previously disabled for light POV z-buffer rendering)
		qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		//re-render affected polys with shadowmap for this entity(note - blend "if darker").
		if(r_shadowmapcount > 0)
		{
			qglEnable( GL_BLEND );
			qglBlendFunc (GL_ZERO, GL_SRC_COLOR);

			R_DrawShadowMapWorld(true, RagDoll[RagDollID].origin);

			qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			qglDisable ( GL_BLEND );
		}

		currentmodel = RagDoll[RagDollID].ragDollMesh;	
		
		r_shadowmapcount = 0;
	}
}
