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

#include "r_local.h"
#include <GL/glu.h>

image_t		gltextures[MAX_GLTEXTURES];
int			numgltextures;
int			base_textureid;		// gltextures[i] = base_textureid+i

static byte			 intensitytable[256];
static unsigned char gammatable[256];

cvar_t		*intensity;

extern cvar_t	*cl_hudimage1; //custom huds
extern cvar_t	*cl_hudimage2;

unsigned	d_8to24table[256];

qboolean GL_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean is_sky );
qboolean GL_Upload32 (unsigned *data, int width, int height,  qboolean mipmap, qboolean is_normalmap);
qboolean GL_Upload32_hires (unsigned *data, int width, int height,  qboolean mipmap, qboolean is_normalmap);

int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
int		gl_filter_max = GL_LINEAR;

GLenum bFunc1 = -1;
GLenum bFunc2 = -1;
void GL_BlendFunction (GLenum sfactor, GLenum dfactor)
{
	if (sfactor!=bFunc1 || dfactor!=bFunc2)
	{
		bFunc1 = sfactor;
		bFunc2 = dfactor;

		qglBlendFunc(bFunc1, bFunc2);
	}
}

GLenum shadeModelMode = -1;

void GL_ShadeModel (GLenum mode)
{
	if (mode!=shadeModelMode)
	{
		shadeModelMode = mode;
		qglShadeModel(mode);
	}
}


void GL_SetTexturePalette( unsigned palette[256] )
{
	int i;
	unsigned char temptable[768];

	if ( qglColorTableEXT && gl_ext_palettedtexture->value )
	{
		for ( i = 0; i < 256; i++ )
		{
			temptable[i*3+0] = ( palette[i] >> 0 ) & 0xff;
			temptable[i*3+1] = ( palette[i] >> 8 ) & 0xff;
			temptable[i*3+2] = ( palette[i] >> 16 ) & 0xff;
		}

		qglColorTableEXT( GL_SHARED_TEXTURE_PALETTE_EXT,
						   GL_RGB,
						   256,
						   GL_RGB,
						   GL_UNSIGNED_BYTE,
						   temptable );
	}
}

void GL_EnableMultitexture( qboolean enable )
{
	if ( !qglSelectTextureSGIS && !qglActiveTextureARB )
		return;

	if ( enable )
	{
		GL_SelectTexture( GL_TEXTURE1 );
		qglEnable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	else
	{
		GL_SelectTexture( GL_TEXTURE1 );
		qglDisable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	GL_SelectTexture( GL_TEXTURE0 );
	GL_TexEnv( GL_REPLACE );
}

void GL_SelectTexture( GLenum texture )
{
	int tmu;

	if ( !qglSelectTextureSGIS && !qglActiveTextureARB )
		return;

	if ( texture == GL_TEXTURE0 )
	{
		tmu = 0;
	}
	else
	{
		tmu = 1;
	}

	if ( tmu == gl_state.currenttmu )
	{
		return;
	}

	gl_state.currenttmu = tmu;

	if ( qglSelectTextureSGIS )
	{
		qglSelectTextureSGIS( texture );
	}
	else if ( qglActiveTextureARB )
	{
		qglActiveTextureARB( texture );
		qglClientActiveTextureARB( texture );
	}
}

void GL_TexEnv( GLenum mode )
{
	static int lastmodes[2] = { -1, -1 };

	if ( mode != lastmodes[gl_state.currenttmu] )
	{
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		lastmodes[gl_state.currenttmu] = mode;
	}
}

void GL_Bind (int texnum)
{
	extern	image_t	*draw_chars;

	if (gl_nobind->value && draw_chars)		// performance evaluation option
		texnum = draw_chars->texnum;
	if ( gl_state.currenttextures[gl_state.currenttmu] == texnum)
		return;
	gl_state.currenttextures[gl_state.currenttmu] = texnum;
	qglBindTexture (GL_TEXTURE_2D, texnum);
}

void GL_MBind( GLenum target, int texnum )
{
	GL_SelectTexture( target );
	if ( target == GL_TEXTURE0 )
	{
		if ( gl_state.currenttextures[0] == texnum )
			return;
	}
	else
	{
		if ( gl_state.currenttextures[1] == texnum )
			return;
	}
	GL_Bind( texnum );
}

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

#define NUM_GL_MODES (sizeof(modes) / sizeof (glmode_t))

typedef struct
{
	char *name;
	int mode;
} gltmode_t;

gltmode_t gl_alpha_modes[] = {
	{"default", 4},
	{"GL_RGBA", GL_RGBA},
	{"GL_RGBA8", GL_RGBA8},
	{"GL_RGB5_A1", GL_RGB5_A1},
	{"GL_RGBA4", GL_RGBA4},
	{"GL_RGBA2", GL_RGBA2},
};

#define NUM_GL_ALPHA_MODES (sizeof(gl_alpha_modes) / sizeof (gltmode_t))

gltmode_t gl_solid_modes[] = {
	{"default", 3},
	{"GL_RGB", GL_RGB},
	{"GL_RGB8", GL_RGB8},
	{"GL_RGB5", GL_RGB5},
	{"GL_RGB4", GL_RGB4},
	{"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
	{"GL_RGB2", GL_RGB2_EXT},
#endif
};

#define NUM_GL_SOLID_MODES (sizeof(gl_solid_modes) / sizeof (gltmode_t))

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( char *string )
{
	int		i;
	image_t	*glt;

	for (i=0 ; i< NUM_GL_MODES ; i++)
	{
		if ( !Q_stricmp( modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_MODES)
	{
		Com_Printf ("bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_sky )
		{
			GL_Bind (glt->texnum);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
GL_TextureAlphaMode
===============
*/
void GL_TextureAlphaMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_ALPHA_MODES ; i++)
	{
		if ( !Q_stricmp( gl_alpha_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_ALPHA_MODES)
	{
		Com_Printf ("bad alpha texture mode name\n");
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
GL_TextureSolidMode
===============
*/
void GL_TextureSolidMode( char *string )
{
	int		i;

	for (i=0 ; i< NUM_GL_SOLID_MODES ; i++)
	{
		if ( !Q_stricmp( gl_solid_modes[i].name, string ) )
			break;
	}

	if (i == NUM_GL_SOLID_MODES)
	{
		Com_Printf ("bad solid texture mode name\n");
		return;
	}

	gl_tex_solid_format = gl_solid_modes[i].mode;
}

/*
===============
GL_ImageList_f
===============
*/
void	GL_ImageList_f (void)
{
	int		i;
	image_t	*image;
	int		texels;
	const char *palstrings[2] =
	{
		"RGB",
		"PAL"
	};

	Com_Printf ("------------------\n");
	texels = 0;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->upload_width*image->upload_height;
		switch (image->type)
		{
		case it_skin:
			Com_Printf ("M");
			break;
		case it_sprite:
			Com_Printf ("S");
			break;
		case it_wall:
			Com_Printf ("W");
			break;
		case it_pic:
			Com_Printf ("P");
			break;
		default:
			Com_Printf (" ");
			break;
		}

		Com_Printf (" %3i %3i %s: %s\n",
			image->upload_width, image->upload_height, palstrings[image->paletted], image->name);
	}
	Com_Printf ("Total texel count (not counting mipmaps): %i\n", texels);
}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up inefficient hardware / drivers

=============================================================================
*/

#define	MAX_SCRAPS		1
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT];
qboolean	scrap_dirty;

// returns a texture number and the position inside it
int Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	return -1;
//	Sys_Error ("Scrap_AllocBlock: full");
}

int	scrap_uploads;

void Scrap_Upload (void)
{
	scrap_uploads++;
	GL_Bind(TEXNUM_SCRAPS);
	GL_Upload8 (scrap_texels[0], BLOCK_WIDTH, BLOCK_HEIGHT, false, false );
	scrap_dirty = false;
}

/*
=================================================================

PCX LOADING

=================================================================
*/


/*
==============
LoadPCX
==============
*/
void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte	*raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = FS_LoadFile (filename, (void **)&raw);
	if (!raw)
	{
		Com_DPrintf ("Bad pcx file %s\n", filename);
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;

    pcx->xmin = LittleShort(pcx->xmin);
    pcx->ymin = LittleShort(pcx->ymin);
    pcx->xmax = LittleShort(pcx->xmax);
    pcx->ymax = LittleShort(pcx->ymax);
    pcx->hres = LittleShort(pcx->hres);
    pcx->vres = LittleShort(pcx->vres);
    pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
    pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 1024
		|| pcx->ymax >= 512)
	{
		Com_Printf ("Bad pcx file %s\n", filename);
		FS_FreeFile (pcx);
		return;
	}

	out = malloc ( (pcx->ymax+1) * (pcx->xmax+1) );

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = malloc(768);
		memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax+1;
	if (height)
		*height = pcx->ymax+1;

	for (y=0 ; y<=pcx->ymax ; y++, pix += pcx->xmax+1)
	{
		for (x=0 ; x<=pcx->xmax ; )
		{
			dataByte = *raw++;

			if ((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while (runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if ( raw - (byte *)pcx > len)
	{
		Com_DPrintf ("PCX file %s was malformed", filename);
		free (*pic);
		*pic = NULL;
	}

	FS_FreeFile (pcx);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=============
LoadTGA
=============
*/
void LoadTGA (char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	int		length;
	TargaHeader		targa_header;
	byte			*targa_rgba;
	byte tmp[2];

	*pic = NULL;

	//
	// load the file
	//
	length = FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
	{
		Com_DPrintf ("Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;
	
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_index = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	tmp[0] = buf_p[0];
	tmp[1] = buf_p[1];
	targa_header.colormap_length = LittleShort ( *((short *)tmp) );
	buf_p+=2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.y_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.width = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.height = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type!=2 
		&& targa_header.image_type!=10) {
		//Com_Error (ERR_DROP, "LoadTGA: Only type 2 and 10 targa RGB images supported\n");
		FS_FreeFile (buffer);
		return;
	}

	if (targa_header.colormap_type !=0 
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24)) {
		//Com_Error (ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");
		FS_FreeFile (buffer);
		return;
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = malloc (numPixels*4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment
	
	if (targa_header.image_type==2) {  // Uncompressed, RGB images
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) {
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) {
					case 24:
							
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
					case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
				}
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
						case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = 255;
								break;
						case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								break;
					}
	
					for(j=0;j<packetSize;j++) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						switch (targa_header.pixel_size) {
							case 24:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
						}
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}						
					}
				}
			}
			breakOut:;
		}
	}

	FS_FreeFile (buffer);
}


/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/


/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct
{
	short		x, y;
} floodfill_t;

// must be a power of 2
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

#define FLOODFILL_STEP( off, dx, dy ) \
{ \
	if (pos[off] == fillcolor) \
	{ \
		pos[off] = 255; \
		fifo[inpt].x = x + (dx), fifo[inpt].y = y + (dy); \
		inpt = (inpt + 1) & FLOODFILL_FIFO_MASK; \
	} \
	else if (pos[off] != 255) fdc = pos[off]; \
}

void R_FloodFillSkin( byte *skin, int skinwidth, int skinheight )
{
	byte				fillcolor = *skin; // assume this is the pixel to fill
	floodfill_t			fifo[FLOODFILL_FIFO_SIZE];
	int					inpt = 0, outpt = 0;
	int					filledcolor = -1;
	int					i;

	if (filledcolor == -1)
	{
		filledcolor = 0;
		// attempt to find opaque black
		for (i = 0; i < 256; ++i)
			if (d_8to24table[i] == (255 << 0)) // alpha 1.0
			{
				filledcolor = i;
				break;
			}
	}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((fillcolor == filledcolor) || (fillcolor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[inpt].x = 0, fifo[inpt].y = 0;
	inpt = (inpt + 1) & FLOODFILL_FIFO_MASK;

	while (outpt != inpt)
	{
		int			x = fifo[outpt].x, y = fifo[outpt].y;
		int			fdc = filledcolor;
		byte		*pos = &skin[x + skinwidth * y];

		outpt = (outpt + 1) & FLOODFILL_FIFO_MASK;

		if (x > 0)				FLOODFILL_STEP( -1, -1, 0 );
		if (x < skinwidth - 1)	FLOODFILL_STEP( 1, 1, 0 );
		if (y > 0)				FLOODFILL_STEP( -skinwidth, 0, -1 );
		if (y < skinheight - 1)	FLOODFILL_STEP( skinwidth, 0, 1 );
		skin[x + skinwidth * y] = fdc;
	}
}

//=======================================================


/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[1024], p2[1024];
	byte		*pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth*0x10000/outwidth;

	frac = fracstep>>2;
	for (i=0 ; i<outwidth ; i++)
	{
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for (i=0 ; i<outwidth ; i++)
	{
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++)
		{
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
}

/*
================
GL_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void GL_LightScaleTexture (unsigned *in, int inwidth, int inheight, qboolean only_gamma )
{
	if ( only_gamma )
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[p[0]];
			p[1] = gammatable[p[1]];
			p[2] = gammatable[p[2]];
		}
	}
	else
	{
		int		i, c;
		byte	*p;

		p = (byte *)in;

		c = inwidth*inheight;
		for (i=0 ; i<c ; i++, p+=4)
		{
			p[0] = gammatable[intensitytable[p[0]]];
			p[1] = gammatable[intensitytable[p[1]]];
			p[2] = gammatable[intensitytable[p[2]]];
		}
	}
}

/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/
void GL_MipMap (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
		{
			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
		}
	}
}


int		upload_width, upload_height;
qboolean uploaded_paletted;
// Fixed 256x256 texture size limitation - MrG
qboolean GL_Upload32_hires (unsigned *data, int width, int height,  qboolean mipmap, qboolean is_normalmap)
{	int		samples;
	unsigned 	*scaled;
	int		scaled_width, scaled_height;
	int		i, c;
	byte		*scan;
	int comp;
 
    uploaded_paletted = false;    // scan the texture for any non-255 alpha 
    c = width*height; 
    scan = ((byte *)data) + 3; 
    samples = gl_solid_format; 
    for (i=0 ; i<c ; i++, scan += 4) 
    { 
        if ( *scan != 255 ) 
        { 
            samples = gl_alpha_format; 
            break; 
        } 
    } 
    comp = (samples == gl_solid_format) ? gl_tex_solid_format : gl_tex_alpha_format; 

    { 
        int powers_of_two[] = {16,32,64,128,256,512,1024,2048,4096}; 
        int max_size; 
        int i; 
        qglGetIntegerv(GL_MAX_TEXTURE_SIZE,&max_size); 
        scaled_width = scaled_height = 0; 

        for (i=0; i<8; i++) 
        { 
            if (width >= powers_of_two[i] && width < powers_of_two[i+1]) { 
                if (width > ((powers_of_two[i] + powers_of_two[i+1])/2)) 
                    scaled_width = powers_of_two[i+1]; 
                else 
                    scaled_width = powers_of_two[i]; 
            } else if (width == powers_of_two[i+1]) { 
                scaled_width = powers_of_two[i+1]; 
            } 
            if (scaled_width && scaled_height) 
                break; 
            if (height >= powers_of_two[i] && height < powers_of_two[i+1]) { 
                if (height > ((powers_of_two[i] + powers_of_two[i+1])/2)) 
                    scaled_height = powers_of_two[i+1]; 
                else 
                    scaled_height = powers_of_two[i]; 
            } else if (height == powers_of_two[i+1]) { 
                scaled_height = powers_of_two[i+1]; 
            }            if (scaled_width && scaled_height) 
                break; 
        } 

        if (scaled_width > max_size) 
            scaled_width = max_size; 
        if (scaled_height > max_size) 
            scaled_height = max_size; 
    } 

    if (scaled_width != width || scaled_height != height) { 
        scaled=malloc((scaled_width * scaled_height) * 4); 
        GL_ResampleTexture(data,width,height,scaled,scaled_width,scaled_height); 
    } else { 
        scaled_width=width; 
        scaled_height=height; 
        scaled=data; 
    } 

	if(!is_normalmap)
		GL_LightScaleTexture (scaled, scaled_width, scaled_height, !mipmap);
    
	if (mipmap) 
        gluBuild2DMipmaps (GL_TEXTURE_2D, samples, scaled_width, scaled_height, GL_RGBA, GL_UNSIGNED_BYTE, scaled); 
    else   
        qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled); 
     
    if (scaled_width != width || scaled_height != height) 
        free(scaled); 

    upload_width =scaled_width; 
	upload_height = scaled_height; 
    if (mipmap) 
    { 
        qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min); 
        qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max); 
    } 
    else 
    {         
        qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max); 
        qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max); 
    }     
    return (samples == gl_alpha_format); 
} 

/*
===============
GL_Upload32

Returns has_alpha
===============
*/
void GL_BuildPalettedTexture( unsigned char *paletted_texture, unsigned char *scaled, int scaled_width, int scaled_height )
{
	int i;

	for ( i = 0; i < scaled_width * scaled_height; i++ )
	{
		unsigned int r, g, b, c;

		r = ( scaled[0] >> 3 ) & 31;
		g = ( scaled[1] >> 2 ) & 63;
		b = ( scaled[2] >> 3 ) & 31;

		c = r | ( g << 5 ) | ( b << 11 );

		paletted_texture[i] = gl_state.d_16to8table[c];

		scaled += 4;
	}
}

qboolean GL_Upload32 (unsigned *data, int width, int height, qboolean mipmap, qboolean is_normalmap)
{
	int			samples;
	unsigned	scaled[256*256];
	unsigned char paletted_texture[256*256];
	int			scaled_width, scaled_height;
	int			i, c;
	byte		*scan;
	int comp;

	uploaded_paletted = false;

	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	if (gl_round_down->value && scaled_width > width && mipmap)
		scaled_width >>= 1;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;
	if (gl_round_down->value && scaled_height > height && mipmap)
		scaled_height >>= 1;

	// let people sample down the world textures for speed
	if (mipmap)
	{
		scaled_width >>= (int)gl_picmip->value;
		scaled_height >>= (int)gl_picmip->value;
	}

	// don't ever bother with >256 textures
	if (scaled_width > 256)
		scaled_width = 256;
	if (scaled_height > 256)
		scaled_height = 256;

	if (scaled_width < 1)
		scaled_width = 1;
	if (scaled_height < 1)
		scaled_height = 1;

	upload_width = scaled_width;
	upload_height = scaled_height;

	if (scaled_width * scaled_height > sizeof(scaled)/4)
		Sys_Error ("GL_Upload32: too big");

	// scan the texture for any non-255 alpha
	c = width*height;
	scan = ((byte *)data) + 3;
	samples = gl_solid_format;
	for (i=0 ; i<c ; i++, scan += 4)
	{
		if ( *scan != 255 )
		{
			samples = gl_alpha_format;
			break;
		}
	}

	if (samples == gl_solid_format)
	    comp = gl_tex_solid_format;
	else if (samples == gl_alpha_format)
	    comp = gl_tex_alpha_format;
	else {
	    comp = samples;
	}

#if 0
	if (mipmap)
		gluBuild2DMipmaps (GL_TEXTURE_2D, samples, width, height, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	else if (scaled_width == width && scaled_height == height)
		qglTexImage2D (GL_TEXTURE_2D, 0, comp, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	else
	{
		gluScaleImage (GL_RGBA, width, height, GL_UNSIGNED_BYTE, trans,
			scaled_width, scaled_height, GL_UNSIGNED_BYTE, scaled);
		qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	}
#else

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
			// paletted texture.
			if ( qglColorTableEXT && gl_ext_palettedtexture->value && samples == gl_solid_format )
			{
				uploaded_paletted = true;
				GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) data, scaled_width, scaled_height );
				qglTexImage2D( GL_TEXTURE_2D,
							  0,
							  GL_COLOR_INDEX8_EXT,
							  scaled_width,
							  scaled_height,
							  0,
							  GL_COLOR_INDEX,
							  GL_UNSIGNED_BYTE,
							  paletted_texture );
			}
			else
			{
				qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			}
			goto done;
		}
		memcpy (scaled, data, width*height*4);
	}
	else
		GL_ResampleTexture (data, width, height, scaled, scaled_width, scaled_height);

	if(!is_normalmap)
		GL_LightScaleTexture (scaled, scaled_width, scaled_height, !mipmap );

	// textures.
	if ( qglColorTableEXT && gl_ext_palettedtexture->value && ( samples == gl_solid_format ) )
	{
		uploaded_paletted = true;
		GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) scaled, scaled_width, scaled_height );
		qglTexImage2D( GL_TEXTURE_2D,
					  0,
					  GL_COLOR_INDEX8_EXT,
					  scaled_width,
					  scaled_height,
					  0,
					  GL_COLOR_INDEX,
					  GL_UNSIGNED_BYTE,
					  paletted_texture );
	}
	else
	{
		qglTexImage2D( GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled );
	}

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;

			// paletted textures.
			if ( qglColorTableEXT && gl_ext_palettedtexture->value && samples == gl_solid_format )
			{
				uploaded_paletted = true;
				GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) scaled, scaled_width, scaled_height );
				qglTexImage2D( GL_TEXTURE_2D,
							  miplevel,
							  GL_COLOR_INDEX8_EXT,
							  scaled_width,
							  scaled_height,
							  0,
							  GL_COLOR_INDEX,
							  GL_UNSIGNED_BYTE,
							  paletted_texture );
			}
			else
			{
				qglTexImage2D (GL_TEXTURE_2D, miplevel, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
			}
		}
	}
done: ;
#endif


	if (mipmap)
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}

	return (samples == gl_alpha_format);
}

/*
===============
GL_Upload8

Returns has_alpha
===============
*/
qboolean GL_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean is_sky )
{
	unsigned	trans[512*256];
	int			i, s;
	int			p;

	s = width*height;

	if (s > sizeof(trans)/4)
		Com_Error (ERR_DROP, "GL_Upload8: too large");

	if ( qglColorTableEXT && 
		 gl_ext_palettedtexture->value && 
		 is_sky )
	{
		qglTexImage2D( GL_TEXTURE_2D,
					  0,
					  GL_COLOR_INDEX8_EXT,
					  width,
					  height,
					  0,
					  GL_COLOR_INDEX,
					  GL_UNSIGNED_BYTE,
					  data );

		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

		return false;
	}
	else
	{
		for (i=0 ; i<s ; i++)
		{
			p = data[i];
			trans[i] = d_8to24table[p];

			if (p == 255)
			{	// transparent, so scan around for another color
				// to avoid alpha fringes
				// FIXME: do a full flood fill so mips work...
				if (i > width && data[i-width] != 255)
					p = data[i-width];
				else if (i < s-width && data[i+width] != 255)
					p = data[i+width];
				else if (i > 0 && data[i-1] != 255)
					p = data[i-1];
				else if (i < s-1 && data[i+1] != 255)
					p = data[i+1];
				else
					p = 0;
				// copy rgb components
				((byte *)&trans[i])[0] = ((byte *)&d_8to24table[p])[0];
				((byte *)&trans[i])[1] = ((byte *)&d_8to24table[p])[1];
				((byte *)&trans[i])[2] = ((byte *)&d_8to24table[p])[2];
			}
		}
		if(gl_texres->value)
			return GL_Upload32_hires (trans, width, height, mipmap, false);
		else
			return GL_Upload32 (trans, width, height, mipmap, false);
	}
}


/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits)
{
	image_t		*image;
	int			i;

	// find a free image_t
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!image->texnum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			Com_Error (ERR_DROP, "MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];

	if (strlen(name) >= sizeof(image->name))
		Com_Error (ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
	strcpy (image->name, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	if (type == it_skin && bits == 8)
		R_FloodFillSkin(pic, width, height);

	// load little pics into the scrap
	if (image->type == it_pic && bits == 8
		&& image->width < 64 && image->height < 64)
	{
		int		x, y;
		int		i, j, k;
		int		texnum;

		texnum = Scrap_AllocBlock (image->width, image->height, &x, &y);
		if (texnum == -1)
			goto nonscrap;
		scrap_dirty = true;

		// copy the texels into the scrap block
		k = 0;
		for (i=0 ; i<image->height ; i++)
			for (j=0 ; j<image->width ; j++, k++)
				scrap_texels[texnum][(y+i)*BLOCK_WIDTH + x + j] = pic[k];
		image->texnum = TEXNUM_SCRAPS + texnum;
		image->scrap = true;
		image->has_alpha = true;
		image->sl = (x+0.01)/(float)BLOCK_WIDTH;
		image->sh = (x+image->width-0.01)/(float)BLOCK_WIDTH;
		image->tl = (y+0.01)/(float)BLOCK_WIDTH;
		image->th = (y+image->height-0.01)/(float)BLOCK_WIDTH;
	}
	else
	{
nonscrap: 
		image->scrap = false;
		image->texnum = TEXNUM_IMAGES + (image - gltextures);
		GL_Bind(image->texnum);
		if (bits == 8)
			image->has_alpha = GL_Upload8 (pic, width, height, (image->type != it_pic && image->type != it_sky), image->type == it_sky );
		else {
			if(gl_texres->value)
				image->has_alpha = GL_Upload32_hires ((unsigned *)pic, width, height, (image->type != it_pic && image->type != it_sky), image->type == it_bump );
			else
				image->has_alpha = GL_Upload32 ((unsigned *)pic, width, height, (image->type != it_pic && image->type != it_sky), image->type == it_bump );
		}
		image->upload_width = upload_width;		// after power of 2 and scales
		image->upload_height = upload_height;
		image->paletted = uploaded_paletted;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
	}

	return image;
}


/*
================
GL_LoadWal
================
*/
image_t *GL_LoadWal (char *name)
{
	miptex_t	*mt;
	int			width, height, ofs;
	image_t		*image;

	FS_LoadFile (name, (void **)&mt);
	if (!mt)
	{
		Com_Printf ("GL_FindImage: can't load %s\n", name);
		return r_notexture;
	}

	width = LittleLong (mt->width);
	height = LittleLong (mt->height);
	ofs = LittleLong (mt->offsets[0]);

	image = GL_LoadPic (name, (byte *)mt + ofs, width, height, it_wall, 8);

	FS_FreeFile ((void *)mt);

	return image;
}

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t	*GL_FindImage (char *name, imagetype_t type)
{
	image_t	*image;
	int		i, len;
	byte	*pic, *palette;
	int		width, height;
	char	shortname[MAX_QPATH];

	if (!name)
		return NULL;	//	Com_Error (ERR_DROP, "GL_FindImage: NULL name");
	len = strlen(name);
	if (len<5)
		return NULL;	//	Com_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);

	//if HUD, then we want to load the one according to what it is set to.
	if(!strcmp(name, "pics/i_health.pcx")) 
			strcpy(name, cl_hudimage1->string);
	if(!strcmp(name, "pics/i_score.pcx")) 
			strcpy(name, cl_hudimage2->string);

	// look for it
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!strcmp(name, image->name))
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	// strip off .pcx, .tga, etc...
	COM_StripExtension ( name, shortname );

	//
	// load the pic from disk
	//
	image = NULL;
	pic = NULL;
	palette = NULL;

	// try to load .tga first
	LoadTGA (va("%s.tga", shortname), &pic, &width, &height);
	if (pic) 
	{
		image = GL_LoadPic (name, pic, width, height, type, 32);
		goto done;
	}

	// then comes .pcx
	LoadPCX (va("%s.pcx", shortname), &pic, &palette, &width, &height);
	if (pic) 
	{
		image = GL_LoadPic (name, pic, width, height, type, 8);
		goto done;
	}

	// then comes .wal
	if (type == it_wall)
		image = GL_LoadWal (va("%s.wal", shortname));

done:
	if (pic)
		free(pic);
	if (palette)
		free(palette);

	//image->script = RS_FindScript(shortname); god only knows why this crashes everything

	return image; 
}



/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return GL_FindImage (name, it_skin);
}


/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages (void)
{
	int		i;
	image_t	*image;

	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;
    r_shelltexture->registration_sequence = registration_sequence;    // c14 added shell texture
	r_reflecttexture->registration_sequence = registration_sequence;
	r_flare->registration_sequence = registration_sequence;
	r_smoketexture->registration_sequence = registration_sequence;
  	r_explosiontexture->registration_sequence = registration_sequence;
	r_explosion1texture->registration_sequence = registration_sequence;
	r_explosion2texture->registration_sequence = registration_sequence;
	r_explosion3texture->registration_sequence = registration_sequence;
	r_explosion4texture->registration_sequence = registration_sequence;
	r_explosion5texture->registration_sequence = registration_sequence;
	r_explosion6texture->registration_sequence = registration_sequence;
	r_explosion7texture->registration_sequence = registration_sequence;
	r_bloodtexture->registration_sequence = registration_sequence;
	r_pufftexture->registration_sequence = registration_sequence;
	r_blastertexture->registration_sequence = registration_sequence;
	r_bflashtexture->registration_sequence = registration_sequence;
	r_cflashtexture->registration_sequence = registration_sequence;
	r_leaderfieldtexture->registration_sequence = registration_sequence;
	r_deathfieldtexture->registration_sequence = registration_sequence;
	r_hittexture->registration_sequence = registration_sequence;
	r_bubbletexture->registration_sequence = registration_sequence;
	r_shottexture->registration_sequence = registration_sequence;
	r_bullettexture->registration_sequence = registration_sequence;
	r_sayicontexture->registration_sequence = registration_sequence;
	r_voltagetexture->registration_sequence = registration_sequence;
	r_cubemap->registration_sequence = registration_sequence;

	//minimap stuff - only if those textures are present, as this was a feature added later on
	//and if crx has been updated via galaxy, clients may not have these images
	if(r_radarmap)
		r_radarmap->registration_sequence = registration_sequence;
	if(r_around)
		r_around->registration_sequence = registration_sequence;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_pic)
			continue;		// don't free pics
		// free it
		qglDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}


/*
===============
Draw_GetPalette
===============
*/
int Draw_GetPalette (void)
{
	int		i;
	int		r, g, b;
	unsigned	v;
	byte	*pic, *pal;
	int		width, height;

	// get the palette

	LoadPCX ("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
		Com_Error (ERR_FATAL, "Couldn't load pics/colormap.pcx");

	for (i=0 ; i<256 ; i++)
	{
		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];
		
		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		d_8to24table[i] = LittleLong(v);
	}

	d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent

	free (pic);
	free (pal);

	return 0;
}


/*
===============
GL_InitImages
===============
*/
void	GL_InitImages (void)
{
	int		i, j;
	float	g = vid_gamma->value;

	registration_sequence = 1;

	// init intensity conversions
	intensity = Cvar_Get ("intensity", "1", 0);

	if ( intensity->value <= 1 )
		Cvar_Set( "intensity", "1" );

	gl_state.inverse_intensity = 1 / intensity->value;

	Draw_GetPalette ();

	if ( qglColorTableEXT )
	{
		FS_LoadFile( "pics/16to8.dat", &gl_state.d_16to8table );
		if ( !gl_state.d_16to8table )
			Sys_Error( ERR_FATAL, "Couldn't load pics/16to8.pcx");
	}

	if ( gl_config.renderer & ( GL_RENDERER_VOODOO | GL_RENDERER_VOODOO2 ) )
	{
		g = 1.0F;
	}

	for ( i = 0; i < 256; i++ )
	{
		if ( g == 1 )
		{
			gammatable[i] = i;
		}
		else
		{
			float inf;

			inf = 255 * pow ( (i+0.5)/255.5 , g ) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	for (i=0 ; i<256 ; i++)
	{
		j = i*intensity->value;
		if (j > 255)
			j = 255;
		intensitytable[i] = j;
	}
	R_InitBloomTextures();//BLOOMS
}

/*
===============
GL_ShutdownImages
===============
*/
void	GL_ShutdownImages (void)
{
	int		i;
	image_t	*image;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot
		// free it
		qglDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}

