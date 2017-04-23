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
// r_bloom.c: 2D lighting post process effect

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

/*
==============================================================================

						LIGHT BLOOMS

==============================================================================
*/

static int		BLOOM_SIZE;

cvar_t		*r_bloom_alpha;
cvar_t		*r_bloom_diamond_size;
cvar_t		*r_bloom_intensity;
cvar_t		*r_bloom_darken;
cvar_t		*r_bloom_sample_size;
cvar_t		*r_bloom_fast_sample;

image_t	*r_bloomscratchtexture;
image_t	*r_bloomscratchtexture2;
image_t	*r_bloomeffecttexture;
image_t	*r_midsizetexture;
static GLuint bloomscratchFBO, bloomscratch2FBO, midsizeFBO, bloomeffectFBO;
static GLuint bloom_fullsize_downsampling_rbo_FBO;
static GLuint bloom_fullsize_downsampling_RBO;

static int		r_midsizetexture_size;
static int		screen_texture_width, screen_texture_height;

//current refdef size:
static int	curView_x;
static int	curView_y;
static int	curView_width;
static int	curView_height;

#define R_Bloom_Quad( x, y, width, height )	\
	qglBegin(GL_QUADS);						\
	qglTexCoord2f(	0,			1.0);		\
	qglVertex2f(	x,			y);			\
	qglTexCoord2f(	0,			0);			\
	qglVertex2f(	x,			y+height);	\
	qglTexCoord2f(	1.0,		0);			\
	qglVertex2f(	x+width,	y+height);	\
	qglTexCoord2f(	1.0,		1.0);		\
	qglVertex2f(	x+width,	y);			\
	qglEnd();


/*
=================
R_Bloom_AllocRBO

Create a 24-bit RBO with specified size and attach it to an FBO
=================
*/
static void R_Bloom_AllocRBO (int width, int height, GLuint *RBO, GLuint *FBO)
{
	// create the RBO
	qglGenRenderbuffersEXT (1, RBO);
    qglBindRenderbufferEXT (GL_RENDERBUFFER_EXT, *RBO);
    qglRenderbufferStorageEXT (GL_RENDERBUFFER_EXT, GL_RGB, width, height);
    qglBindRenderbufferEXT (GL_RENDERBUFFER_EXT, 0);
    
    // create up the FBO
	qglGenFramebuffersEXT (1, FBO);
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, *FBO);
	
	// bind the RBO to it
	qglFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, *RBO);
	
	//clean up
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
}



/*
=================
R_Bloom_InitEffectTexture
=================
*/
static void R_Bloom_InitEffectTexture (void)
{
	float	bloomsizecheck;

	if( r_bloom_sample_size->integer < 32 )
		Cvar_SetValue ("r_bloom_sample_size", 32);

	//make sure bloom size is a power of 2
	BLOOM_SIZE = r_bloom_sample_size->integer;
	bloomsizecheck = (float)BLOOM_SIZE;
	while(bloomsizecheck > 1.0f) bloomsizecheck /= 2.0f;
	if( bloomsizecheck != 1.0f )
	{
		BLOOM_SIZE = 32;
		while( BLOOM_SIZE < r_bloom_sample_size->integer )
			BLOOM_SIZE *= 2;
	}

	//make sure bloom size doesn't have stupid values
	if( BLOOM_SIZE > screen_texture_width ||
		BLOOM_SIZE > screen_texture_height )
		BLOOM_SIZE = min( screen_texture_width, screen_texture_height );

	if( BLOOM_SIZE != r_bloom_sample_size->integer )
		Cvar_SetValue ("r_bloom_sample_size", BLOOM_SIZE);

	r_bloomeffecttexture = R_Postprocess_AllocFBOTexture ("***r_bloomeffecttexture***", BLOOM_SIZE, BLOOM_SIZE, &bloomeffectFBO);
}

/*
=================
R_Bloom_InitTextures
=================
*/
void checkFBOExtensions (void);
static void R_Bloom_InitTextures (void)
{
	if (!gl_state.fbo || !gl_state.hasFBOblit)
	{
		Com_Printf ("FBO Failed, disabling bloom.\n");
		Cvar_SetValue ("r_bloom", 0);
		return;
	}
	
	GL_SelectTexture (0);
	
	qglGetError ();
	
	//find closer power of 2 to screen size
	for (screen_texture_width = 1;screen_texture_width < vid.width;screen_texture_width *= 2);
	for (screen_texture_height = 1;screen_texture_height < vid.height;screen_texture_height *= 2);

	//validate bloom size and init the bloom effect texture
	R_Bloom_InitEffectTexture();

	//init the "scratch" texture
	r_bloomscratchtexture = R_Postprocess_AllocFBOTexture ("***r_bloomscratchtexture***", BLOOM_SIZE, BLOOM_SIZE, &bloomscratchFBO);
	r_bloomscratchtexture2 = R_Postprocess_AllocFBOTexture ("***r_bloomscratchtexture2***", BLOOM_SIZE, BLOOM_SIZE, &bloomscratch2FBO);
	
	//init the screen-size RBO
	R_Bloom_AllocRBO (vid.width, vid.height, &bloom_fullsize_downsampling_RBO, &bloom_fullsize_downsampling_rbo_FBO);

	//if screensize is more than 2x the bloom effect texture, set up for stepped downsampling
	r_midsizetexture = NULL;
	r_midsizetexture_size = 0;
	if( viddef.width > (BLOOM_SIZE * 2) && !r_bloom_fast_sample->integer )
	{
		r_midsizetexture_size = (int)(BLOOM_SIZE * 2);
		// mid-size texture
		r_midsizetexture = R_Postprocess_AllocFBOTexture ("***r_midsizetexture***", r_midsizetexture_size, r_midsizetexture_size, &midsizeFBO);
	}

}

/*
=================
R_InitBloomTextures
=================
*/
void R_InitBloomTextures( void )
{

	r_bloom = Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );
	r_bloom_alpha = Cvar_Get( "r_bloom_alpha", "0.2", CVAR_ARCHIVE );
	r_bloom_diamond_size = Cvar_Get( "r_bloom_diamond_size", "8", CVAR_ARCHIVE );
	r_bloom_intensity = Cvar_Get( "r_bloom_intensity", "0.5", CVAR_ARCHIVE );
	r_bloom_darken = Cvar_Get( "r_bloom_darken", "8", CVAR_ARCHIVE );
	r_bloom_sample_size = Cvar_Get( "r_bloom_sample_size", "128", CVAR_ARCHIVE );
	r_bloom_fast_sample = Cvar_Get( "r_bloom_fast_sample", "0", CVAR_ARCHIVE );

	BLOOM_SIZE = 0;
	if( !r_bloom->integer )
		return;

	R_Bloom_InitTextures ();
}


/*
=================
R_Bloom_DrawEffect
=================
*/
static void R_Bloom_DrawEffect (void)
{
	GL_SelectTexture (0);
	GL_Bind (r_bloomeffecttexture->texnum);
	GLSTATE_ENABLE_BLEND
	GL_BlendFunction (GL_ONE, GL_ONE);
	qglColor4f(r_bloom_alpha->value, r_bloom_alpha->value, r_bloom_alpha->value, 1.0f);
	GL_TexEnv(GL_MODULATE);
	
	qglBegin(GL_QUADS);
	qglTexCoord2f(	0,							1.0	);
	qglVertex2f(	curView_x,					curView_y	);
	qglTexCoord2f(	0,							0	);
	qglVertex2f(	curView_x,					curView_y + curView_height	);
	qglTexCoord2f(	1.0,						0	);
	qglVertex2f(	curView_x + curView_width,	curView_y + curView_height	);
	qglTexCoord2f(	1.0,						1.0	);
	qglVertex2f(	curView_x + curView_width,	curView_y	);
	qglEnd();

	GLSTATE_DISABLE_BLEND
}


/*
=================
R_Bloom_DoGuassian
=================
*/
static void R_Bloom_DoGuassian (void)
{
	int			i;
	static float intensity;
	
	//set up sample size workspace
	qglViewport (0, 0, BLOOM_SIZE, BLOOM_SIZE);
	qglMatrixMode (GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho (0, 1, 1, 0, -10, 100);
	qglMatrixMode (GL_MODELVIEW);
	
	GL_MBind (0, r_bloomeffecttexture->texnum);
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, bloomscratchFBO);
	
	GL_SetupWholeScreen2DVBO (wholescreen_fliptextured);
	
	//darkening passes
	if (r_bloom_darken->value)
	{
		if (r_test->integer)
		{
			float exp = r_bloom_darken->value + 1.0f;
			glUseProgramObjectARB (g_colorexpprogramObj);
			glUniform1iARB (colorexp_uniforms.source, 0);
			glUniform4fARB (colorexp_uniforms.exponent, exp, exp, exp, 1.0f);
			R_DrawVarrays (GL_QUADS, 0, 4);
		}
		else
		{
			qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
			GLSTATE_ENABLE_BLEND
			GL_BlendFunction (GL_DST_COLOR, GL_ZERO);
			GL_SelectTexture (0);
			GL_TexEnv (GL_MODULATE);
			for(i=0; i<r_bloom_darken->integer ;i++)
				R_DrawVarrays (GL_QUADS, 0, 4);
		}
		qglCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, BLOOM_SIZE, BLOOM_SIZE);
	}
	
	
	glUseProgramObjectARB (g_blurprogramObj);
	glUniform1iARB (g_location_source, 0);
	GLSTATE_DISABLE_BLEND
	
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, bloomscratch2FBO);
	glUniform2fARB (g_location_scale, 0, r_bloom_diamond_size->value/BLOOM_SIZE/8.0f);
	R_DrawVarrays (GL_QUADS, 0, 4);
	GL_MBind (0, r_bloomscratchtexture2->texnum);
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, bloomscratchFBO);
	glUniform2fARB (g_location_scale, r_bloom_diamond_size->value/BLOOM_SIZE/8.0f, 0);
	R_DrawVarrays (GL_QUADS, 0, 4);
	
	
	glUseProgramObjectARB (g_colorscaleprogramObj);
	glUniform1iARB (colorscale_uniforms.source, 0);
	intensity = 4.0 * r_bloom_intensity->value;
	glUniform3fARB (colorscale_uniforms.scale, intensity, intensity, intensity);
	
	GL_BlendFunction (GL_ONE, GL_ONE_MINUS_SRC_COLOR);
	GLSTATE_ENABLE_BLEND
	
	GL_MBind (0, r_bloomscratchtexture->texnum);
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, bloomeffectFBO);
	R_DrawVarrays (GL_QUADS, 0, 4);
	
	glUseProgramObjectARB (0);
	R_KillVArrays ();
	
	//restore full screen workspace
	qglViewport( 0, 0, viddef.width, viddef.height );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity ();
	qglOrtho(0, viddef.width, viddef.height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity ();
	
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, BLOOM_SIZE, BLOOM_SIZE);

	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

/*
=================
R_Bloom_FullsizeRBOUpdate

This updates the full size RBO from the screen. The whole thing is guaranteed
to be updated 60 times a second, but it does it a 1/4 of the screen at a time
if the framerate is high enough. It does it in horizontal slices instead of
quadrants because that way the GPU doesn't have to skip over part of each row
of pixels. Tearing isn't an issue because it'll just be blurred to mush
anyway.
=================
*/
static void R_Bloom_FullsizeRBOUpdate (void)
{
	static int	cur_section = 0;
	static int	last_time = 0;
	int			i, num_sections, cur_time;
	int			y;
	
	cur_time = Sys_Milliseconds();
	num_sections = (cur_time-last_time+2)/4;
	if (num_sections > 4) num_sections = 4;
	if (num_sections == 0) return;
	
	for (i = 0; i < num_sections; i++)
	{
		y = cur_section*(viddef.height/4);
		qglBlitFramebufferEXT(0, y, viddef.width, y+viddef.height/4, 0, y, viddef.width, y+viddef.height/4,
			GL_COLOR_BUFFER_BIT, GL_LINEAR);
		cur_section = (cur_section + 1) % 4;
	}
	last_time = cur_time;
}

/*
=================
R_Bloom_DownsampleView

Creates a downscaled, blurred version of the screen, leaving it in the
"scratch" and "effect" textures (identical in both.) This function is name is
a bit confusing, because "downsampling" means two things here:
 1) Creating a scaled-down version of an image
 2) Converting a multisampled image to a non-multisampled image the same size,
    which is necessary if MSAA is enabled in the graphics driver settings
The function name uses meaning 1.
=================
*/
static void R_Bloom_DownsampleView (void)
{
	GLSTATE_DISABLE_BLEND
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	
	GL_SelectTexture (0);
	
	if (gl_state.msaaEnabled)
	{
		// If MSAA is enabled, the FBO blitting needs an extra step.
		// Copy onto full-screen sized RBO first, to go from the multisample
		// format of the screen to a non-multisampled format.
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, bloom_fullsize_downsampling_rbo_FBO);
		R_Bloom_FullsizeRBOUpdate ();
		// Set the downsampled RBO as the read framebuffer, then run the rest
		// of the code as normal.
		qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, bloom_fullsize_downsampling_rbo_FBO);
	}
	
	//stepped downsample
	if( r_midsizetexture_size )
	{
		// copy into small sized FBO (equivalent to copying into full screen
		// sized FBO and then drawing that onto the small sized FBO later.)
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, bloomscratchFBO);
		qglBlitFramebufferEXT(0, 0, viddef.width, viddef.height, 0, 0, BLOOM_SIZE, BLOOM_SIZE,
			GL_COLOR_BUFFER_BIT, GL_LINEAR);
		// copy into downsampling (mid-sized) FBO
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, midsizeFBO);
		qglBlitFramebufferEXT(0, 0, viddef.width, viddef.height, 0, 0, r_midsizetexture_size, r_midsizetexture_size,
			GL_COLOR_BUFFER_BIT, GL_LINEAR);
		
		
		// create the finished downsampled version of the texture by blending 
		// the small-sized FBO and the mid-sized FBO onto a small-sized FBO,
		// hoping it adds some blur.
		
		// Store first of all in the bloom effect texture, since we don't want
		// to draw the scratch texture onto itself. 
		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, bloomeffectFBO);
		
		// mid-size
		GL_Bind(r_midsizetexture->texnum);
		qglColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
		R_Bloom_Quad( 0,  viddef.height-BLOOM_SIZE, BLOOM_SIZE, BLOOM_SIZE);
		// small-size
		GLSTATE_ENABLE_BLEND
		GL_BlendFunction (GL_ONE, GL_ONE);
		qglColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
		GL_Bind(r_bloomscratchtexture->texnum);
		R_Bloom_Quad( 0,  viddef.height-BLOOM_SIZE, BLOOM_SIZE, BLOOM_SIZE );
		qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		GLSTATE_DISABLE_BLEND
		
	} else {	//downsample simple

		qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, bloomeffectFBO);
		qglBlitFramebufferEXT(0, 0, viddef.width, viddef.height, 0, 0, BLOOM_SIZE, BLOOM_SIZE,
			GL_COLOR_BUFFER_BIT, GL_LINEAR);
		
	}
	
	// Blit the finished downsampled texture onto a second FBO. We end up with
	// with two copies, which DoGuassian will take advantage of.
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, bloomscratchFBO);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, bloomeffectFBO);
	qglBlitFramebufferEXT(0, 0, BLOOM_SIZE, BLOOM_SIZE, 0, 0, BLOOM_SIZE, BLOOM_SIZE,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
	qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, 0);
	qglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, 0);
}

/*
=================
R_BloomBlend
=================
*/
void R_BloomBlend (refdef_t *fd)
{

	if( !(fd->rdflags & RDF_BLOOM) || !r_bloom->integer )
		return;

	if( !BLOOM_SIZE )
		R_Bloom_InitTextures();
	
	// previous function can set this if there's no FBO
	if (!r_bloom->integer)
		return;

	if( screen_texture_width < BLOOM_SIZE ||
		screen_texture_height < BLOOM_SIZE )
		return;
	
	GL_SelectTexture (0);

	//set up full screen workspace
	qglViewport( 0, 0, viddef.width, viddef.height );
	qglDisable( GL_DEPTH_TEST );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity ();
	qglOrtho(0, viddef.width, viddef.height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity ();
	qglDisable(GL_CULL_FACE);

	GLSTATE_DISABLE_BLEND
	GL_EnableTexture (0, true);

	qglColor4f( 1, 1, 1, 1 );

	//set up current sizes
	// TODO: get rid of these nasty globals
	curView_x = fd->x;
	curView_y = fd->y;
	curView_width = fd->width;
	curView_height = fd->height;

	//create the bloom image
	R_Bloom_DownsampleView();
	R_Bloom_DoGuassian();

	R_Bloom_DrawEffect();

	qglColor3f (1,1,1);
	GLSTATE_DISABLE_BLEND
	GL_EnableTexture (0, true);
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDepthMask (1);
}

