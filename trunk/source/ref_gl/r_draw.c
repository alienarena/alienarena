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

// r_draw.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

extern	qboolean	scrap_dirty;
extern cvar_t *con_font;
void Scrap_Upload (void);

//small dot used for failsafe blank texture
byte	blanktexture[16][16] =
{
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
}

/*
=============
R_RegisterPic
=============
*/
image_t	*R_RegisterPic (const char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "pics/%s.pcx", name);
		gl = GL_FindImage (fullname, it_pic);
	}
	else
	{ // note: levelshot graphic is loaded here
		strcpy( fullname, &name[1] );
		gl = GL_FindImage( fullname, it_pic );
		// gl = GL_FindImage (name+1, it_pic);
		// sometimes gets "evelshot" instead of "levelshot", compiler bug???
	}

	return gl;
}

image_t	*R_RegisterParticlePic (const char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "particles/%s.tga", name);
		gl = GL_FindImage (fullname, it_particle);
	}
	else
	{ // 2010-10 match above workaround (paranoid)
		strcpy( fullname, &name[1] );
		gl = GL_FindImage( fullname, it_particle );
	}

	return gl;
}

image_t	*R_RegisterParticleNormal (const char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "particles/%s.tga", name);
		gl = GL_FindImage (fullname, it_pic);
	}
	else
	{ // 2010-10 match above workaround (paranoid)
		strcpy( fullname, &name[1] );
		gl = GL_FindImage( fullname, it_pic );
	}

	return gl;
}

image_t	*R_RegisterGfxPic (const char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "gfx/%s.tga", name);
		gl = GL_FindImage (fullname, it_pic);
	}
	else
	{ // 2010-10 match above workaround (paranoid)
		strcpy( fullname, &name[1] );
		gl = GL_FindImage( fullname, it_pic );
	}


	return gl;
}

image_t	*R_RegisterPlayerIcon (const char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "players/%s.tga", name);
		gl = GL_FindImage (fullname, it_pic);
	}
	else
	{ // 2010-10 match above workaround (paranoid)
		strcpy( fullname, &name[1] );
		gl = GL_FindImage( fullname, it_pic );
	}

	return gl;
}

/*
=============
Draw_PicExists
=============
*/
qboolean Draw_PicExists (const char *pic)
{
	return R_RegisterPic (pic) != NULL;
}

/*
=============
Draw_GetPicSize
=============
*/
void Draw_GetPicSize (int *w, int *h, const char *pic)
{
	image_t *gl;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		*w = *h = -1;
		return;
	}
	*w = gl->width;
	*h = gl->height;
}

#define DIV254BY255 (0.9960784313725490196078431372549f)
/*
=============
Draw_AlphaStretchPic
- Note: If tiling is true, the texture wrapping flags are adjusted to prevent
        gaps from appearing if the texture is tiled with itself or with other
        textures. This adjustment is permanent, although it would be easy to
        change the code to undo it after rendering.
=============
*/
enum draw_tiling_s
{
	draw_without_tiling,
	draw_with_tiling
};
void Draw_AlphaStretchImage (float x, float y, float w, float h, const image_t *gl, float alphaval, enum draw_tiling_s tiling)
{
	rscript_t *rs;
	float	alpha,s,t;
	rs_stage_t *stage;
	char shortname[MAX_QPATH];
	float xscale, yscale;
	float cropped_x, cropped_y, cropped_w, cropped_h;
	
	if (scrap_dirty)
		Scrap_Upload ();
	
	COM_StripExtension ( gl->name, shortname );
	
	rs = gl->script;

	//if we draw the red team bar, we are on red team
	if(!strcmp(shortname, "pics/i_team1"))
		r_teamColor = 1;
	else if(!strcmp(shortname, "pics/i_team2"))
		r_teamColor = 2;

	R_InitQuadVarrays();
	
	GL_SelectTexture (0);
	
	if (!rs)
	{
		qglDisable (GL_ALPHA_TEST);
		qglEnable (GL_BLEND);
		GLSTATE_DISABLE_ALPHATEST
		GLSTATE_ENABLE_BLEND
		GL_TexEnv( GL_MODULATE );
		
		xscale = (float)w/(float)gl->upload_width;
		yscale = (float)h/(float)gl->upload_height;
		
		cropped_x = x + xscale*(float)gl->crop_left;
		cropped_y = y + yscale*(float)gl->crop_top;
	
		cropped_w = xscale*(float)gl->crop_width; 
		cropped_h = yscale*(float)gl->crop_height;
		
		VA_SetElem2(vert_array[0], cropped_x,			cropped_y);
		VA_SetElem2(vert_array[1], cropped_x+cropped_w, cropped_y);
		VA_SetElem2(vert_array[2], cropped_x+cropped_w, cropped_y+cropped_h);
		VA_SetElem2(vert_array[3], cropped_x,			cropped_y+cropped_h);
	
		qglColor4f(1,1,1, alphaval);

		//set color of hud by team
		if(r_teamColor == 1) {
			if(!strcmp(shortname, "pics/i_health"))
				qglColor4f(1, .2, .2, alphaval);
		}
		else if(r_teamColor == 2) {
			if(!strcmp(shortname, "pics/i_health"))
				qglColor4f(.1, .4, .8, alphaval);
		}

		GL_Bind (gl->texnum);
		if (tiling == draw_with_tiling)
		{
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
			qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		}
		VA_SetElem2(tex_array[0], gl->crop_sl, gl->crop_tl);
		VA_SetElem2(tex_array[1], gl->crop_sh, gl->crop_tl);
		VA_SetElem2(tex_array[2], gl->crop_sh, gl->crop_th);
		VA_SetElem2(tex_array[3], gl->crop_sl, gl->crop_th);
		R_DrawVarrays (GL_QUADS, 0, 4);
		qglEnable (GL_ALPHA_TEST);
		qglDisable (GL_BLEND);
		GLSTATE_DISABLE_BLEND
		GLSTATE_ENABLE_ALPHATEST
		R_KillVArrays();
	}
	else
	{
		VA_SetElem2(vert_array[0], x,	y);
		VA_SetElem2(vert_array[1], x+w, y);
		VA_SetElem2(vert_array[2], x+w, y+h);
		VA_SetElem2(vert_array[3], x,	y+h);
	
		RS_ReadyScript(rs);

		stage=rs->stage;
		while (stage)
		{
			float red = 1, green = 1, blue = 1;

			if (stage->blendfunc.blend)
			{
				GLSTATE_ENABLE_BLEND
				GL_BlendFunction(stage->blendfunc.source,stage->blendfunc.dest);
			}
			else
			{
				GLSTATE_DISABLE_BLEND
			}

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

			if (stage->alphamask)
			{
				GLSTATE_ENABLE_ALPHATEST
			}
			else
			{
				GLSTATE_DISABLE_ALPHATEST
			}

			if (stage->colormap.enabled)
			{
				red = stage->colormap.red/255.0;
				green = stage->colormap.green/255.0;
				blue = stage->colormap.blue/255.0;
			}

			qglColor4f(red,green,blue, alpha);

			if (stage->colormap.enabled)
				qglDisable (GL_TEXTURE_2D);
			else if (stage->anim_count)
				GL_Bind(RS_Animate(stage));
			else
				GL_Bind (stage->texture->texnum);

			s = 0; t = 1;
			RS_SetTexcoords2D (stage, &s, &t);
			VA_SetElem2(tex_array[3],s, t);
			s = 0; t = 0;
			RS_SetTexcoords2D (stage, &s, &t);
			VA_SetElem2(tex_array[0],s, t);
			s = 1; t = 0;
			RS_SetTexcoords2D (stage, &s, &t);
			VA_SetElem2(tex_array[1],s, t);
			s = 1; t = 1;
			RS_SetTexcoords2D (stage, &s, &t);
			VA_SetElem2(tex_array[2],s, t);

			R_DrawVarrays(GL_QUADS, 0, 4);

			qglColor4f(1,1,1,1);
			if (stage->colormap.enabled)
				qglEnable (GL_TEXTURE_2D);

			stage=stage->next;
		}

		qglColor4f(1,1,1,1);
		GLSTATE_ENABLE_ALPHATEST
		GLSTATE_DISABLE_BLEND
		GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		R_KillVArrays();
	}
}

void Draw_AlphaStretchTilingPic (float x, float y, float w, float h, const char *pic, float alphaval)
{
	image_t *gl;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		return;
	}
	
	Draw_AlphaStretchImage (x, y, w, h, gl, alphaval, draw_with_tiling);
}

/*
=============
Draw_AlphaStretchPic
=============
*/
void Draw_AlphaStretchPic (float x, float y, float w, float h, const char *pic, float alphaval)
{
	image_t *gl;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		return;
	}
	
	Draw_AlphaStretchImage (x, y, w, h, gl, alphaval, draw_without_tiling);
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic (float x, float y, float w, float h, const char *pic)
{
	Draw_AlphaStretchPic (x, y, w, h, pic, DIV254BY255);
}

/*
=============
Draw_ScaledPic
=============
*/
void Draw_ScaledPic (float x, float y, float scale, const char *pic)
{
	image_t *gl;
	float w, h;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
		return;
	}

	w = (float)gl->width*scale;
	h = (float)gl->height*scale;
	
	Draw_AlphaStretchImage (x, y, w, h, gl, DIV254BY255, draw_without_tiling);
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic (float x, float y, const char *pic)
{
	Draw_ScaledPic (x, y, 1.0, pic);
}

/*
=============
Draw_AlphaStretchPlayerIcon
=============
*/
void Draw_AlphaStretchPlayerIcon (int x, int y, int w, int h, const char *pic, float alphaval)
{
	image_t *gl;

	gl = R_RegisterPlayerIcon (pic);
	if (!gl)
	{
		return;
	}

	Draw_AlphaStretchImage (x, y, w, h, gl, alphaval, draw_without_tiling);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (float x, float y, float w, float h, const float rgba[])
{
	qglDisable (GL_TEXTURE_2D);
	// FIXME HACK
	qglDisable (GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
	GLSTATE_ENABLE_BLEND;
	GLSTATE_DISABLE_ALPHATEST;
	qglColor4fv (rgba);
	
	qglBegin (GL_QUADS);
		qglVertex2f (x,y);
		qglVertex2f (x+w, y);
		qglVertex2f (x+w, y+h);
		qglVertex2f (x, y+h);
	qglEnd ();
	
	GLSTATE_DISABLE_BLEND;
	qglColor3f (1,1,1);
	qglEnable (GL_TEXTURE_2D);
}
//=============================================================================

/*
================
Draw_FadeScreen
================
*/
void Draw_FadeScreen (void)
{
	qglEnable (GL_BLEND);
	qglDisable (GL_TEXTURE_2D);
	qglColor4f (0, 0, 0, 0.8);
	qglBegin (GL_QUADS);

	qglVertex2f (0,0);
	qglVertex2f (vid.width, 0);
	qglVertex2f (vid.width, vid.height);
	qglVertex2f (0, vid.height);

	qglEnd ();
	qglColor4f (1,1,1,1);
	qglEnable (GL_TEXTURE_2D);
	qglDisable (GL_BLEND);
}


//=============================================================================

/*
================
RGBA - This really should be a macro, but MSVC doesn't support C99.
================
*/

float *RGBA (float r, float g, float b, float a)
{
	static float ret[4];
	
	ret[0] = r;
	ret[1] = g;
	ret[2] = b;
	ret[3] = a;
	
	return ret;
}
