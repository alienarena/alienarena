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

#include "r_local.h"

#ifdef __unix__
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

image_t		*draw_chars;

extern	qboolean	scrap_dirty;
extern  void GL_BlendFunction (GLenum sfactor, GLenum dfactor);
extern cvar_t *con_font;
void Scrap_Upload (void);


void RefreshFont (void)
{

	draw_chars = GL_FindImage (va("fonts/%s.tga", con_font->string), it_pic);
	if (!draw_chars)
	{
		draw_chars = GL_FindImage ("pics/conchars.tga", it_pic);
		Cvar_Set( "con_font", "default" );
	}

	GL_Bind( draw_chars->texnum );

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	con_font->modified = false;
}

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
	image_t	*Draw_FindPic (char *name);

	RefreshFont();
}

/*
================
Draw_Char

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void Draw_Char (int x, int y, int num)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;
	
	if ( (num&127) == 32 )
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	GL_Bind (draw_chars->texnum);

	qglBegin (GL_QUADS);
	qglTexCoord2f (fcol, frow);
	qglVertex2f (x, y);
	qglTexCoord2f (fcol + size, frow);
	qglVertex2f (x+8, y);
	qglTexCoord2f (fcol + size, frow + size);
	qglVertex2f (x+8, y+8);
	qglTexCoord2f (fcol, frow + size);
	qglVertex2f (x, y+8);
	qglEnd ();
}
static byte R_FloatToByte( float x )
{
	union {
		float			f;
		unsigned int	i;
	} f2i;

	// shift float to have 8bit fraction at base of number
	f2i.f = x + 32768.0f;
	f2i.i &= 0x7FFFFF;

	// then read as integer and kill float bits...
	return ( byte )min( f2i.i, 255 );
}

void Draw_ColorChar (int x, int y, int num, vec4_t color)
{
	int				row, col;
	float			frow, fcol, size;
	byte			colors[4];

	colors[0] = R_FloatToByte( color[0] );
	colors[1] = R_FloatToByte( color[1] );
	colors[2] = R_FloatToByte( color[2] );
	colors[3] = R_FloatToByte( color[3] );

	num &= 255;
	
	if ( (num&127) == 32 )
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	GL_Bind (draw_chars->texnum);

	qglColor4ubv( colors );
	GL_TexEnv(GL_MODULATE);
	qglBegin (GL_QUADS);
	qglTexCoord2f (fcol, frow);
	qglVertex2f (x, y);
	qglTexCoord2f (fcol + size, frow);
	qglVertex2f (x+8, y);
	qglTexCoord2f (fcol + size, frow + size);
	qglVertex2f (x+8, y+8);
	qglTexCoord2f (fcol, frow + size);
	qglVertex2f (x, y+8);
	qglEnd ();
	qglColor4f( 1,1,1,1 );
}

void Draw_ScaledChar (int x, int y, int num, int scale)
					
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;
	
	if ( (num&127) == 32 )
		return;		// space

	if (y <= -8)
		return;			// totally off screen
	
	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	GL_Bind(draw_chars->texnum);

	qglBegin (GL_QUADS);
	qglTexCoord2f (fcol, frow);
	qglVertex2f (x, y);
	qglTexCoord2f (fcol + size, frow);
	qglVertex2f (x+scale, y);
	qglTexCoord2f (fcol + size, frow + size);
	qglVertex2f (x+scale, y+scale);
	qglTexCoord2f (fcol, frow + size);
	qglVertex2f (x, y+scale);
	qglEnd();

}
void Draw_ScaledColorChar (int x, int y, int num, vec4_t color, int scale)
{
	int				row, col;
	float			frow, fcol, size;
	byte			colors[4];

	colors[0] = R_FloatToByte( color[0] );
	colors[1] = R_FloatToByte( color[1] );
	colors[2] = R_FloatToByte( color[2] );
	colors[3] = R_FloatToByte( color[3] );

	num &= 255;
	
	if ( (num&127) == 32 )
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num>>4;
	col = num&15;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	GL_Bind (draw_chars->texnum);

	qglColor4ubv( colors );
	GL_TexEnv(GL_MODULATE);
	qglBegin (GL_QUADS);
	qglTexCoord2f (fcol, frow);
	qglVertex2f (x, y);
	qglTexCoord2f (fcol + size, frow);
	qglVertex2f (x+scale, y);
	qglTexCoord2f (fcol + size, frow + size);
	qglVertex2f (x+scale, y+scale);
	qglTexCoord2f (fcol, frow + size);
	qglVertex2f (x, y+scale);
	qglEnd ();
	qglColor4f( 1,1,1,1 );
}
/*
=============
R_RegisterPic
=============
*/
image_t	*R_RegisterPic (char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "pics/%s.pcx", name);
		gl = GL_FindImage (fullname, it_pic);
	}
	else
		gl = GL_FindImage (name+1, it_pic);

	return gl;
}

image_t	*R_RegisterParticlePic (char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "particles/%s.tga", name);
		gl = GL_FindImage (fullname, it_pic);
	}
	else
		gl = GL_FindImage (name+1, it_pic);

	return gl;
}

image_t	*R_RegisterGfxPic (char *name)
{
	image_t *gl;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "gfx/%s.tga", name);
		gl = GL_FindImage (fullname, it_pic);
	}
	else
		gl = GL_FindImage (name+1, it_pic);

	return gl;
}
/*
=============
Draw_GetPicSize
=============
*/
void ShaderResizePic( image_t *gl, int *w, int *h)
{
	rscript_t *rs = NULL;
	char shortname[MAX_QPATH];

	COM_StripExtension ( gl->name, shortname );
	rs=RS_FindScript(shortname);
	
	if (!rs)
		return;
	if (!rs->picsize.enable)
		return;

}
void Draw_GetPicSize (int *w, int *h, char *pic)
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
	ShaderResizePic(gl, w, h);
}
#define DIV254BY255 (0.9960784313725490196078431372549f)
void Draw_ShaderPic (image_t *gl)
{
	rscript_t *rs = NULL;
	float	alpha,s,t;
	rs_stage_t *stage;
	char shortname[MAX_QPATH];

	COM_StripExtension ( gl->name, shortname );
	rs=RS_FindScript(shortname);
	
	if (!rs) 
	{
		qglDisable (GL_ALPHA_TEST);
		qglEnable (GL_BLEND);
		GLSTATE_DISABLE_ALPHATEST
		GLSTATE_ENABLE_BLEND
		GL_TexEnv( GL_MODULATE );
		qglColor4f(1,1,1,DIV254BY255);
		VA_SetElem4(col_array[0], 1,1,1,1);
		VA_SetElem4(col_array[1], 1,1,1,1);
		VA_SetElem4(col_array[2], 1,1,1,1);
		VA_SetElem4(col_array[3], 1,1,1,1);
		GL_Bind (gl->texnum);
		VA_SetElem2(tex_array[0],gl->sl, gl->tl);
		VA_SetElem2(tex_array[1],gl->sh, gl->tl);
		VA_SetElem2(tex_array[2],gl->sh, gl->th);
		VA_SetElem2(tex_array[3],gl->sl, gl->th);
		qglDrawArrays (GL_QUADS, 0, 4);
		qglEnable (GL_ALPHA_TEST);
		qglDisable (GL_BLEND);
	} 
	else 
	{
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
			VA_SetElem4(col_array[0], red,green,blue, alpha);
			VA_SetElem4(col_array[1], red,green,blue, alpha);
			VA_SetElem4(col_array[2], red,green,blue, alpha);
			VA_SetElem4(col_array[3], red,green,blue, alpha);

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

			qglDrawArrays(GL_QUADS,0,4);

			qglColor4f(1,1,1,1);
			if (stage->colormap.enabled)
				qglEnable (GL_TEXTURE_2D);

			stage=stage->next;
		}

		qglColor4f(1,1,1,1);
		GLSTATE_ENABLE_ALPHATEST
		GLSTATE_DISABLE_BLEND
		GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

/*
=============
Draw_StretchPic
=============
*/
void Draw_StretchPic (int x, int y, int w, int h, char *pic)
{
	image_t *gl;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
//		Com_Printf ("Can't find pic: %s\n", pic);
		return;
	}

	if (scrap_dirty)
		Scrap_Upload ();

	VA_SetElem2(vert_array[0],x, y);
	VA_SetElem2(vert_array[1],x+w, y);
	VA_SetElem2(vert_array[2],x+w, y+h);
	VA_SetElem2(vert_array[3],x, y+h);

	Draw_ShaderPic(gl);
}


/*
=============
Draw_Pic
=============
*/
void Draw_Pic (int x, int y, char *pic)
{
	image_t *gl;
	int w, h;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
//		Com_Printf ("Can't find pic: %s\n", pic);
		return;
	}

	w = gl->width;
	h = gl->height;

	if (scrap_dirty)
		Scrap_Upload ();

	VA_SetElem2(vert_array[0],x, y);
	VA_SetElem2(vert_array[1],x+w, y);
	VA_SetElem2(vert_array[2],x+w, y+h);
	VA_SetElem2(vert_array[3],x, y+h);

	Draw_ShaderPic(gl);
}

/*
=============
Draw_ScaledPic
=============
*/

void Draw_ScaledPic (int x, int y, float scale, char *pic)
{
	image_t *gl;
	int w, h;
	float xoff, yoff;

	gl = R_RegisterPic (pic);
	if (!gl)
	{
//		Com_Printf ("Can't find pic: %s\n", pic);
		return;
	}

	w = gl->width;
	h = gl->height;

	xoff = (w*scale-w);
	yoff = (h*scale-h);

	if (scrap_dirty)
		Scrap_Upload ();

	VA_SetElem2(vert_array[0],x, y);
	VA_SetElem2(vert_array[1],x+w+xoff, y);
	VA_SetElem2(vert_array[2],x+w+xoff, y+h+yoff);
	VA_SetElem2(vert_array[3],x, y+h+yoff);

	Draw_ShaderPic(gl);
}
/*
=============
Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void Draw_TileClear (int x, int y, int w, int h, char *pic)
{
	image_t	*image;

	image = R_RegisterPic (pic);
	if (!image)
	{
//		Com_Printf ("Can't find pic: %s\n", pic);
		return;
	}

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) )  && !image->has_alpha)
		qglDisable (GL_ALPHA_TEST);

	GL_Bind (image->texnum);
	qglBegin (GL_QUADS);
	qglTexCoord2f (x/64.0, y/64.0);
	qglVertex2f (x, y);
	qglTexCoord2f ( (x+w)/64.0, y/64.0);
	qglVertex2f (x+w, y);
	qglTexCoord2f ( (x+w)/64.0, (y+h)/64.0);
	qglVertex2f (x+w, y+h);
	qglTexCoord2f ( x/64.0, (y+h)/64.0 );
	qglVertex2f (x, y+h);
	qglEnd ();

	if ( ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) )  && !image->has_alpha)
		qglEnable (GL_ALPHA_TEST);
}


/*
=============
Draw_Fill

Fills a box of pixels with a single color
=============
*/
void Draw_Fill (int x, int y, int w, int h, int c)
{
	union
	{
		unsigned	c;
		byte		v[4];
	} color;

	if ( (unsigned)c > 255)
		Com_Error (ERR_FATAL, "Draw_Fill: bad color");

	qglDisable (GL_TEXTURE_2D);

	color.c = d_8to24table[c];
	qglColor3f (color.v[0]/255.0,
		color.v[1]/255.0,
		color.v[2]/255.0);

	qglBegin (GL_QUADS);

	qglVertex2f (x,y);
	qglVertex2f (x+w, y);
	qglVertex2f (x+w, y+h);
	qglVertex2f (x, y+h);

	qglEnd ();
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


//====================================================================


/*
=============
Draw_StretchRaw
=============
*/
extern unsigned	r_rawpalette[256];

void Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data)
{
	unsigned	image32[256*256];
	unsigned char image8[256*256];
	int			i, j, trows;
	byte		*source;
	int			frac, fracstep;
	float		hscale;
	int			row;
	float		t;

	GL_Bind (0);

	if (rows<=256)
	{
		hscale = 1;
		trows = rows;
	}
	else
	{
		hscale = rows/256.0;
		trows = 256;
	}
	t = rows*hscale / 256;

	if ( !qglColorTableEXT )
	{
		unsigned *dest;

		for (i=0 ; i<trows ; i++)
		{
			row = (int)(i*hscale);
			if (row > rows)
				break;
			source = data + cols*row;
			dest = &image32[i*256];
			fracstep = cols*0x10000/256;
			frac = fracstep >> 1;
			for (j=0 ; j<256 ; j++)
			{
				dest[j] = r_rawpalette[source[frac>>16]];
				frac += fracstep;
			}
		}

		qglTexImage2D (GL_TEXTURE_2D, 0, gl_tex_solid_format, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, image32);
	}
	else
	{
		unsigned char *dest;

		for (i=0 ; i<trows ; i++)
		{
			row = (int)(i*hscale);
			if (row > rows)
				break;
			source = data + cols*row;
			dest = &image8[i*256];
			fracstep = cols*0x10000/256;
			frac = fracstep >> 1;
			for (j=0 ; j<256 ; j++)
			{
				dest[j] = source[frac>>16];
				frac += fracstep;
			}
		}

		qglTexImage2D( GL_TEXTURE_2D, 
			           0, 
					   GL_COLOR_INDEX8_EXT, 
					   256, 256, 
					   0, 
					   GL_COLOR_INDEX, 
					   GL_UNSIGNED_BYTE, 
					   image8 );
	}
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) 
		qglDisable (GL_ALPHA_TEST);

	qglBegin (GL_QUADS);
	qglTexCoord2f (0, 0);
	qglVertex2f (x, y);
	qglTexCoord2f (1, 0);
	qglVertex2f (x+w, y);
	qglTexCoord2f (1, t);
	qglVertex2f (x+w, y+h);
	qglTexCoord2f (0, t);
	qglVertex2f (x, y+h);
	qglEnd ();

	if ( ( gl_config.renderer == GL_RENDERER_MCD ) || ( gl_config.renderer & GL_RENDERER_RENDITION ) ) 
		qglEnable (GL_ALPHA_TEST);
}

