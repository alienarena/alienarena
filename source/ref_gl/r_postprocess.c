/*
Copyright (C) 2009 COR Entertainment

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

#define EXPLOSION 1
#define PAIN 2

extern int KillFlags;
extern float v_blend[4];
extern void R_TransformVectorToScreen( refdef_t *rd, vec3_t in, vec2_t out );
void R_DrawBloodEffect (void);

image_t *r_framebuffer;
image_t *r_distortwave;

vec3_t r_explosionOrigin;
int r_drawing_fbeffect;
int	r_fbFxType;
float r_fbeffectTime;
int frames;

void R_GLSLPostProcess(void)
{
	vec2_t fxScreenPos;
	int offsetX, offsetY;
	vec3_t	vec;
	float	dot, r_fbeffectLen;
	vec3_t	forward, mins, maxs;
	trace_t r_trace;
	float hScissor, wScissor;

	if(!gl_glsl_postprocess->value || !gl_glsl_shaders->value || vid.width > 2048 || !gl_state.glsl_shaders)
		return;

	if(r_fbFxType == EXPLOSION) 
	{
		//is it in our view?
		AngleVectors (r_newrefdef.viewangles, forward, NULL, NULL);
		VectorSubtract (r_explosionOrigin, r_newrefdef.vieworg, vec);
		VectorNormalize (vec);
		dot = DotProduct (vec, forward);
		if (dot <= 0.3)
			r_drawing_fbeffect = false;

		//is anything blocking it from view?
		VectorSet(mins, 0, 0, 0);
		VectorSet(maxs,	0, 0, 0);

		r_trace = CM_BoxTrace(r_origin, r_explosionOrigin, maxs, mins, r_worldmodel->firstnode, MASK_VISIBILILITY);
		if (r_trace.fraction != 1.0)
			r_drawing_fbeffect = false;
	}

	//if not doing stuff, return
	if(!r_drawing_fbeffect)
		return;

	frames++;

	if(r_fbFxType == PAIN)		
	{
		r_fbeffectLen = 0.1;
		R_DrawBloodEffect();
	}
	else
		r_fbeffectLen = 0.2;

	//set up full screen workspace
	qglViewport( 0, 0, viddef.width, viddef.height );
	qglDisable( GL_DEPTH_TEST );
	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity ();
	qglOrtho(0, viddef.width, viddef.height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
    qglLoadIdentity ();
	qglDisable(GL_CULL_FACE);

	qglDisable( GL_BLEND );
	qglEnable( GL_TEXTURE_2D );

	qglViewport(0,0,FB_texture_width,FB_texture_height);

	//we need to grab the frame buffer
	qglBindTexture(GL_TEXTURE_2D, r_framebuffer->texnum);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0,
				0, 0, 0, 0, FB_texture_width, FB_texture_height);

	qglViewport(0,0,viddef.width, viddef.height);

	//render quad on screen

	offsetY = viddef.height - FB_texture_height;
	offsetX = viddef.width - FB_texture_width;

	hScissor = (float)viddef.height/(float)FB_texture_height;
	wScissor = (float)viddef.width/(float)FB_texture_width;
		
	qglEnableClientState (GL_VERTEX_ARRAY);
	qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

	qglTexCoordPointer (2, GL_FLOAT, sizeof(tex_array[0]), tex_array[0]);
	qglVertexPointer (2, GL_FLOAT, sizeof(vert_array[0]), vert_array[0]);
	qglColorPointer (4, GL_FLOAT, sizeof(col_array[0]), col_array[0]);

	VA_SetElem2(vert_array[0],0, viddef.height);
	VA_SetElem2(vert_array[1],viddef.width-offsetX, viddef.height);
	VA_SetElem2(vert_array[2],viddef.width-offsetX, offsetY);
	VA_SetElem2(vert_array[3],0, offsetY);

	VA_SetElem2(tex_array[0],r_framebuffer->sl, r_framebuffer->tl);
	VA_SetElem2(tex_array[1],r_framebuffer->sh, r_framebuffer->tl);
	VA_SetElem2(tex_array[2],r_framebuffer->sh, r_framebuffer->th);
	VA_SetElem2(tex_array[3],r_framebuffer->sl, r_framebuffer->th);

	if(r_fbFxType == EXPLOSION)
	{
		//create a distortion wave effect at point of explosion
		glUseProgramObjectARB( g_fbprogramObj );

		qglActiveTextureARB(GL_TEXTURE1);
		qglBindTexture (GL_TEXTURE_2D,r_framebuffer->texnum);
		glUniform1iARB( g_location_framebuffTex, 1);
		KillFlags |= KILL_TMU0_POINTER;

		qglActiveTextureARB(GL_TEXTURE0);
	
		if(r_distortwave)
			qglBindTexture(GL_TEXTURE_2D, r_distortwave->texnum);
		glUniform1iARB( g_location_distortTex, 0);
		KillFlags |= KILL_TMU1_POINTER;

		qglActiveTextureARB(GL_TEXTURE0);

		glUniform2fARB( g_location_dParams, wScissor, hScissor);

		fxScreenPos[0] = fxScreenPos[1] = 0;

		//get position of focal point of warp
		R_TransformVectorToScreen(&r_newrefdef, r_explosionOrigin, fxScreenPos);

		fxScreenPos[0] /= viddef.width; 
		fxScreenPos[1] /= viddef.height;

		fxScreenPos[0] -= (0.5 + (abs((float)offsetX)/1024.0)*0.25); 
		fxScreenPos[1] -= (0.5 + (abs((float)offsetY)/1024.0)*0.15); 

		fxScreenPos[0] -= (float)frames*.001;
		fxScreenPos[1] -= (float)frames*.001;
		glUniform2fARB( g_location_fxPos, fxScreenPos[0], fxScreenPos[1]);
		
		qglDrawArrays (GL_QUADS, 0, 4);

		glUseProgramObjectARB( 0 );
	}
	else
	{
		//do a radial blur
		glUseProgramObjectARB( g_rblurprogramObj );

		qglActiveTextureARB(GL_TEXTURE0);
		qglBindTexture(GL_TEXTURE_2D, r_framebuffer->texnum);
		KillFlags |= KILL_TMU0_POINTER;

		glUniform1iARB( g_location_rsource, 0);

		glUniform3fARB( g_location_rscale, 1.0, wScissor, hScissor);

		glUniform3fARB( g_location_rparams, viddef.width/2.0, viddef.height/2.0, 0.25);

		qglDrawArrays (GL_QUADS, 0, 4);

		glUseProgramObjectARB( 0 );
	}

	R_KillVArrays();

	if(rs_realtime > r_fbeffectTime+r_fbeffectLen) 
	{
		frames = 0;
		r_drawing_fbeffect = false; //done effect
	}

	return;
}

/*
==============
R_ShadowBlend
Draws projection shadow(s)
from stenciled volume
==============
*/
image_t *r_colorbuffer;

void R_ShadowBlend(float alpha)
{
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho(0, 1, 1, 0, -99999, 99999);

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity();

	if(gl_state.fbo && gl_state.hasFBOblit && atoi(&gl_config.version_string[0]) >= 3.0) 
	{
		alpha/=1.5; //necessary because we are blending two quads

		//blit the stencil mask from main buffer
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, fboId[1]);

		qglBlitFramebufferEXT(0, 0, vid.width, vid.height, 0, 0, viddef.width, viddef.height,
			GL_STENCIL_BUFFER_BIT, GL_NEAREST);

		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);

		//render offscreen
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId[1]);

		qglDisable(GL_STENCIL_TEST);
		GLSTATE_DISABLE_ALPHATEST
		qglEnable( GL_BLEND );
		qglDisable (GL_DEPTH_TEST);
		qglDisable (GL_TEXTURE_2D);

		qglColor4f (1,1,1, 1);

		qglBegin(GL_TRIANGLES);
		qglVertex2f(-5, -5);
		qglVertex2f(10, -5);
		qglVertex2f(-5, 10);
		qglEnd();
	}

	qglColor4f (0,0,0, alpha);

	GLSTATE_DISABLE_ALPHATEST
	qglEnable( GL_BLEND );
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);

	qglEnable(GL_STENCIL_TEST);
	qglStencilFunc( GL_NOTEQUAL, 0, 0xFF);
	qglStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	qglBegin(GL_TRIANGLES);
	qglVertex2f(-5, -5);
	qglVertex2f(10, -5);
	qglVertex2f(-5, 10);
	qglEnd();

	if(gl_state.fbo && gl_state.hasFBOblit && atoi(&gl_config.version_string[0]) >= 3.0) 
	{
		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		//revert settings
		qglMatrixMode( GL_PROJECTION );
		qglLoadIdentity ();
		qglOrtho(0, vid.width, vid.height, 0, -10, 100);
		qglDisable(GL_CULL_FACE);

		qglEnable( GL_BLEND );
		qglEnable( GL_TEXTURE_2D );

		qglBlendFunc (GL_ZERO, GL_SRC_COLOR);
		qglDisable (GL_DEPTH_TEST);
		qglDisable(GL_STENCIL_TEST);

		qglColor4f (1,1,1,1);

		//render quad on screen	into FBO
		//and blur it vertically

		glUseProgramObjectARB( g_blurprogramObj );

		qglActiveTextureARB(GL_TEXTURE0);
		qglBindTexture(GL_TEXTURE_2D, r_colorbuffer->texnum);

		glUniform1iARB( g_location_source, 0);

		glUniform2fARB( g_location_scale, 4.0/vid.width, 2.0/vid.height);

		qglEnableClientState (GL_VERTEX_ARRAY);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

		qglTexCoordPointer (2, GL_FLOAT, sizeof(tex_array[0]), tex_array[0]);
		qglVertexPointer (2, GL_FLOAT, sizeof(vert_array[0]), vert_array[0]);
		qglColorPointer (4, GL_FLOAT, sizeof(col_array[0]), col_array[0]);

		VA_SetElem2(vert_array[0],0, vid.height);
		VA_SetElem2(vert_array[1],vid.width, vid.height);
		VA_SetElem2(vert_array[2],vid.width, 0);
		VA_SetElem2(vert_array[3],0, 0);

		VA_SetElem2(tex_array[0],r_colorbuffer->sl, r_colorbuffer->tl);
		VA_SetElem2(tex_array[1],r_colorbuffer->sh, r_colorbuffer->tl);
		VA_SetElem2(tex_array[2],r_colorbuffer->sh, r_colorbuffer->th);
		VA_SetElem2(tex_array[3],r_colorbuffer->sl, r_colorbuffer->th);

		qglDrawArrays (GL_QUADS, 0, 4);

		//now blur horizontally

		glUniform1iARB( g_location_source, 0);

		glUniform2fARB( g_location_scale, 2.0/vid.width, 4.0/vid.height);

		qglEnableClientState (GL_VERTEX_ARRAY);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

		qglTexCoordPointer (2, GL_FLOAT, sizeof(tex_array[0]), tex_array[0]);
		qglVertexPointer (2, GL_FLOAT, sizeof(vert_array[0]), vert_array[0]);
		qglColorPointer (4, GL_FLOAT, sizeof(col_array[0]), col_array[0]);

		VA_SetElem2(vert_array[0],0, vid.height);
		VA_SetElem2(vert_array[1],vid.width, vid.height);
		VA_SetElem2(vert_array[2],vid.width, 0);
		VA_SetElem2(vert_array[3],0, 0);

		VA_SetElem2(tex_array[0],r_colorbuffer->sl, r_colorbuffer->tl);
		VA_SetElem2(tex_array[1],r_colorbuffer->sh, r_colorbuffer->tl);
		VA_SetElem2(tex_array[2],r_colorbuffer->sh, r_colorbuffer->th);
		VA_SetElem2(tex_array[3],r_colorbuffer->sl, r_colorbuffer->th);

		qglDrawArrays (GL_QUADS, 0, 4);

		R_KillVArrays();

		glUseProgramObjectARB(0);
	}

	//revert settings
	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();

	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDisable ( GL_BLEND );
	qglEnable (GL_TEXTURE_2D);
	qglEnable (GL_DEPTH_TEST);
	qglDisable(GL_STENCIL_TEST);
	qglEnable(GL_CULL_FACE);

	qglColor4f(1,1,1,1);
}

/*
=================
R_FB_InitTextures
=================
*/

void R_FB_InitTextures( void )
{
	byte	*data;
	int		size;

	//find closer power of 2 to screen size
	for (FB_texture_width = 1;FB_texture_width < viddef.width;FB_texture_width *= 2);
	for (FB_texture_height = 1;FB_texture_height < viddef.height;FB_texture_height *= 2);

	//limit to 2048x2048 - anything larger is generally going to cause problems, and AA doesn't support res higher
	if(FB_texture_width > 2048)
		FB_texture_width = 2048;
	if(FB_texture_height > 2048)
		FB_texture_height = 2048;

	//init the framebuffer texture
	size = FB_texture_width * FB_texture_height * 4;
	data = malloc( size );
	memset( data, 255, size );
	r_framebuffer = GL_LoadPic( "***r_framebuffer***", (byte *)data, FB_texture_width, FB_texture_height, it_pic, 3 );
	free ( data );

	//init the various FBO textures
	size = FB_texture_width * FB_texture_height * 4;
	data = malloc( size );
	memset( data, 255, size );
	r_colorbuffer = GL_LoadPic( "***r_colorbuffer***", (byte *)data, FB_texture_width, FB_texture_height, it_pic, 3 );
	free ( data );

	//init the distortion textures - to do move this to r_misc?
	r_distortwave = GL_FindImage("gfx/distortwave.jpg",it_pic);
}

extern int vehicle_hud;
extern cvar_t *cl_vehicle_huds;
void R_DrawVehicleHUD (void)
{	
	image_t *gl = NULL;
	rscript_t *rs = NULL;
	float	alpha;
	rs_stage_t *stage;
	char shortname[MAX_QPATH];
	
	//draw image over screen
	if(!cl_vehicle_huds->value)
		return;

	switch(vehicle_hud)
	{
		case 1:
			gl = R_RegisterPic ("hud_bomber");
			break;
		case 2:
			gl = R_RegisterPic ("hud_strafer");
			break;
		case 3:
			gl = R_RegisterPic ("hud_hover");
			break;
		case 0:
		default:
			break;
	}

	
	if (!gl)
	{
		return;
	}

	GLSTATE_ENABLE_BLEND

	qglActiveTextureARB(GL_TEXTURE0);
	qglBindTexture(GL_TEXTURE_2D, gl->texnum);
		
	qglEnableClientState (GL_VERTEX_ARRAY);
	qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

	qglTexCoordPointer (2, GL_FLOAT, sizeof(tex_array[0]), tex_array[0]);
	qglVertexPointer (2, GL_FLOAT, sizeof(vert_array[0]), vert_array[0]);
	qglColorPointer (4, GL_FLOAT, sizeof(col_array[0]), col_array[0]);

	VA_SetElem2(vert_array[0],0, 0);
	VA_SetElem2(vert_array[1],vid.width, 0);
	VA_SetElem2(vert_array[2],vid.width, vid.height);
	VA_SetElem2(vert_array[3],0, vid.height);

	VA_SetElem2(tex_array[0],gl->sl, gl->tl);
	VA_SetElem2(tex_array[1],gl->sh, gl->tl);
	VA_SetElem2(tex_array[2],gl->sh, gl->th);
	VA_SetElem2(tex_array[3],gl->sl, gl->th);

	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity ();
	qglOrtho(0, viddef.width, viddef.height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity ();
	
	qglDrawArrays (GL_QUADS, 0, 4);
	
	COM_StripExtension ( gl->name, shortname );
	rs=RS_FindScript(shortname);
	
	if(rs)
	{
		RS_ReadyScript(rs);

		stage=rs->stage;
		while (stage)
		{
			alpha=1.0f;
			if (stage->alphashift.min || stage->alphashift.speed)
			{
				if (!stage->alphashift.speed && stage->alphashift.min > 0)
				{
					alpha=stage->alphashift.min;
				}
				else if (stage->alphashift.speed)
				{
					alpha=sin(rs_realtime * stage->alphashift.speed);
					alpha=(alpha+1)*0.5f;
					if (alpha > stage->alphashift.max) alpha=stage->alphashift.max;
					if (alpha < stage->alphashift.min) alpha=stage->alphashift.min;
				}
			}			
			
			qglEnableClientState (GL_VERTEX_ARRAY);
			qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

			qglTexCoordPointer (2, GL_FLOAT, sizeof(tex_array[0]), tex_array[0]);
			qglVertexPointer (2, GL_FLOAT, sizeof(vert_array[0]), vert_array[0]);
			qglColorPointer (4, GL_FLOAT, sizeof(col_array[0]), col_array[0]);

			VA_SetElem2(vert_array[0],0, 0);
			VA_SetElem2(vert_array[1],vid.width, 0);
			VA_SetElem2(vert_array[2],vid.width, vid.height);
			VA_SetElem2(vert_array[3],0, vid.height);

			qglColor4f(1,1,1, alpha);
			VA_SetElem4(col_array[0], 1,1,1, alpha);
			VA_SetElem4(col_array[1], 1,1,1, alpha);
			VA_SetElem4(col_array[2], 1,1,1, alpha);
			VA_SetElem4(col_array[3], 1,1,1, alpha);

			if (stage->anim_count)
				GL_Bind(RS_Animate(stage));
			else
				GL_Bind (stage->texture->texnum);

			VA_SetElem2(tex_array[0],gl->sl, gl->tl);
			VA_SetElem2(tex_array[1],gl->sh, gl->tl);
			VA_SetElem2(tex_array[2],gl->sh, gl->th);
			VA_SetElem2(tex_array[3],gl->sl, gl->th);

			qglMatrixMode( GL_PROJECTION );
			qglLoadIdentity ();
			qglOrtho(0, viddef.width, viddef.height, 0, -10, 100);
			qglMatrixMode( GL_MODELVIEW );
			qglLoadIdentity ();

			qglDrawArrays(GL_QUADS,0,4);

			stage=stage->next;
		}	
	}
	qglColor4f(1,1,1,1);
	GLSTATE_DISABLE_BLEND

	R_KillVArrays();
}

void R_DrawBloodEffect (void)
{	
	image_t *gl = NULL;
	
	gl = R_RegisterPic ("blood_ring");
	
	if (!gl)
	{
		return;
	}

	qglEnable (GL_BLEND);

	qglActiveTextureARB(GL_TEXTURE0);
	qglBindTexture(GL_TEXTURE_2D, gl->texnum);
		
	qglEnableClientState (GL_VERTEX_ARRAY);
	qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

	qglTexCoordPointer (2, GL_FLOAT, sizeof(tex_array[0]), tex_array[0]);
	qglVertexPointer (2, GL_FLOAT, sizeof(vert_array[0]), vert_array[0]);
	qglColorPointer (4, GL_FLOAT, sizeof(col_array[0]), col_array[0]);

	VA_SetElem2(vert_array[0],0, 0);
	VA_SetElem2(vert_array[1],vid.width, 0);
	VA_SetElem2(vert_array[2],vid.width, vid.height);
	VA_SetElem2(vert_array[3],0, vid.height);

	VA_SetElem2(tex_array[0],gl->sl, gl->tl);
	VA_SetElem2(tex_array[1],gl->sh, gl->tl);
	VA_SetElem2(tex_array[2],gl->sh, gl->th);
	VA_SetElem2(tex_array[3],gl->sl, gl->th);

	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity ();
	qglOrtho(0, viddef.width, viddef.height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
    qglLoadIdentity ();
	
	qglDrawArrays (GL_QUADS, 0, 4);

	qglDisable (GL_BLEND);

	R_KillVArrays();	
}



