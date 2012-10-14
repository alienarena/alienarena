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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

// for jpeglib
#define HAVE_PROTOTYPES

#include "r_local.h"

#if defined HAVE_JPEG_JPEGLIB_H
#include "jpeg/jpeglib.h"
#else
#include "jpeglib.h"
#endif

image_t		gltextures[MAX_GLTEXTURES];
image_t		*r_mirrortexture;
image_t		*r_depthtexture;
image_t		*r_depthtexture2;
int			numgltextures;
int			base_textureid;		// gltextures[i] = base_textureid+i

extern cvar_t	*cl_hudimage1; //custom huds
extern cvar_t	*cl_hudimage2;

unsigned	d_8to24table[256];

qboolean GL_Upload8 (byte *data, int width, int height,  qboolean mipmap, qboolean is_sky );
qboolean GL_Upload32 (unsigned *data, int width, int height,  qboolean mipmap, qboolean is_normalmap);

int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
int		gl_filter_max = GL_LINEAR;

void R_InitImageSubsystem(void)
{
	int		aniso_level, max_aniso;

	if ( strstr( gl_config.extensions_string, "GL_ARB_multitexture" ) )
	{
		Com_Printf ("...using GL_ARB_multitexture\n" );
		qglMTexCoord2fARB = ( void * ) qwglGetProcAddress( "glMultiTexCoord2fARB" );
		qglMTexCoord3fARB = ( void * ) qwglGetProcAddress( "glMultiTexCoord3fARB" );
		qglMultiTexCoord3fvARB = (void*)qwglGetProcAddress("glMultiTexCoord3fvARB");
		qglActiveTextureARB = ( void * ) qwglGetProcAddress( "glActiveTextureARB" );
		qglClientActiveTextureARB = ( void * ) qwglGetProcAddress( "glClientActiveTextureARB" );
		GL_TEXTURE0 = GL_TEXTURE0_ARB;
		GL_TEXTURE1 = GL_TEXTURE1_ARB;
		GL_TEXTURE2 = GL_TEXTURE2_ARB;
		GL_TEXTURE3 = GL_TEXTURE3_ARB;
		GL_TEXTURE4 = GL_TEXTURE4_ARB;
		GL_TEXTURE5 = GL_TEXTURE5_ARB;
		GL_TEXTURE6 = GL_TEXTURE6_ARB;
		GL_TEXTURE7 = GL_TEXTURE7_ARB;
	}
	else
	{
		Com_Printf ("...GL_ARB_multitexture not found\n" );
	}
	
	gl_config.mtexcombine = false;
	if ( strstr( gl_config.extensions_string, "GL_ARB_texture_env_combine" ) )
	{
		Com_Printf( "...using GL_ARB_texture_env_combine\n" );
		gl_config.mtexcombine = true;
	}
	else
	{
		Com_Printf( "...GL_ARB_texture_env_combine not found\n" );
	}
	if ( !gl_config.mtexcombine )
	{
		if ( strstr( gl_config.extensions_string, "GL_EXT_texture_env_combine" ) )
		{
			Com_Printf( "...using GL_EXT_texture_env_combine\n" );
			gl_config.mtexcombine = true;
		}
		else
		{
			Com_Printf( "...GL_EXT_texture_env_combine not found\n" );
		}
	}

	if (strstr(gl_config.extensions_string, "GL_EXT_texture_filter_anisotropic"))
	{
		qglGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max_aniso);

		r_ext_max_anisotropy = Cvar_Get("r_ext_max_anisotropy", "1", CVAR_ARCHIVE );
		Cvar_SetValue("r_ext_max_anisotropy", max_aniso);

		r_anisotropic = Cvar_Get("r_anisotropic", "16", CVAR_ARCHIVE);
		if (r_anisotropic->integer >= r_ext_max_anisotropy->integer)
			Cvar_SetValue("r_anisotropic", r_ext_max_anisotropy->integer);
		if (r_anisotropic->integer <= 0)
			Cvar_SetValue("r_anisotropic", 1);

		aniso_level = r_anisotropic->integer;

		if (r_anisotropic->integer == 1)
			Com_Printf("...ignoring GL_EXT_texture_filter_anisotropic\n");
		else
			Com_Printf("...using GL_EXT_texture_filter_anisotropic\n");
	}
	else
	{
		Com_Printf("...GL_EXT_texture_filter_anisotropic not found\n");
		r_anisotropic = Cvar_Get("r_anisotropic", "0", CVAR_ARCHIVE);
		r_ext_max_anisotropy = Cvar_Get("r_ext_max_anisotropy", "0", CVAR_ARCHIVE);
	}
}

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

void GL_EnableMultitexture( qboolean enable )
{
	if ( !qglActiveTextureARB )
		return;

	if ( enable )
	{
		GL_SelectTexture( GL_TEXTURE1 );
		qglEnable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );

		GL_SelectTexture( GL_TEXTURE2 );
		qglEnable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );

		GL_SelectTexture( GL_TEXTURE3 );
		qglEnable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	else
	{
		GL_SelectTexture( GL_TEXTURE1 );
		qglDisable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );

		GL_SelectTexture( GL_TEXTURE2 );
		qglDisable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );

		GL_SelectTexture( GL_TEXTURE3 );
		qglDisable( GL_TEXTURE_2D );
		GL_TexEnv( GL_REPLACE );
	}
	GL_SelectTexture( GL_TEXTURE0 );
	GL_TexEnv( GL_REPLACE );
}

void GL_SelectTexture( GLenum texture )
{
	int tmu;

	if ( !qglActiveTextureARB )
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

	if ( qglActiveTextureARB )
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

	if (gl_nobind->integer && draw_chars)		// performance evaluation option
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
		if ( !Q_strcasecmp( modes[i].name, string ) )
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
		if (glt->type != it_pic && glt->type != it_particle && glt->type != it_sky )
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
		if ( !Q_strcasecmp( gl_alpha_modes[i].name, string ) )
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
		if ( !Q_strcasecmp( gl_solid_modes[i].name, string ) )
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
		case it_particle:
			Com_Printf ("A");
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
#define	BLOCK_WIDTH		1024
#define	BLOCK_HEIGHT	512

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT*4];
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
/*	Sys_Error ("Scrap_AllocBlock: full");*/
}

int	scrap_uploads;

void Scrap_Upload (void)
{
	GL_Bind(TEXNUM_SCRAPS);
	GL_Upload32 ((unsigned *)scrap_texels[scrap_uploads++], BLOCK_WIDTH, BLOCK_HEIGHT, false, false );
	scrap_dirty = false;
}

//just a guessed size-- this isn't necessarily raw RGBA data, it's the
//encoded image data.
#define	STATIC_RAWDATA_SIZE	(1024*1024*4+256)
byte	static_rawdata[STATIC_RAWDATA_SIZE];

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
	len = FS_LoadFile_TryStatic (	filename, (void **)&raw, static_rawdata, 
									STATIC_RAWDATA_SIZE);
	if (!raw)
	{
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
		if ((byte *)pcx != static_rawdata)
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

	if ((byte *)pcx != static_rawdata)
		FS_FreeFile (pcx);
}
/*
=================================================================

JPEG LOADING

By Robert 'Heffo' Heffernan

=================================================================
*/

static JOCTET eoi_buffer[2] = {(JOCTET)0xFF, (JOCTET)JPEG_EOI};
static qboolean crjpg_corrupted;

void crjpg_null(j_decompress_ptr cinfo)
{
}

int crjpg_fill_input_buffer(j_decompress_ptr cinfo)
{
    Com_Printf("Premature end of JPEG data\n");
    cinfo->src->next_input_byte = eoi_buffer;
    cinfo->src->bytes_in_buffer = 2;
    crjpg_corrupted = true;
    return 1;
}

void crjpg_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{

    cinfo->src->next_input_byte += (size_t) num_bytes;
    cinfo->src->bytes_in_buffer -= (size_t) num_bytes;

    if (cinfo->src->bytes_in_buffer < 0)
    {
		Com_Printf("Premature end of JPEG data\n");
		cinfo->src->next_input_byte = eoi_buffer;
    	cinfo->src->bytes_in_buffer = 2;
    	crjpg_corrupted = true;
    }
}

void crjpg_mem_src(j_decompress_ptr cinfo, byte *mem, int len)
{
    cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_source_mgr));
    cinfo->src->init_source = crjpg_null;
    cinfo->src->fill_input_buffer = crjpg_fill_input_buffer;
    cinfo->src->skip_input_data = crjpg_skip_input_data;
    cinfo->src->resync_to_restart = jpeg_resync_to_restart;
    cinfo->src->term_source = crjpg_null;
    cinfo->src->bytes_in_buffer = len;
    cinfo->src->next_input_byte = mem;
}

#define DSTATE_START	200	/* after create_decompress */
#define DSTATE_INHEADER	201	/* reading header markers, no SOS yet */

/*
==============
LoadJPG
==============
*/
#define	STATIC_SCANLINE_SIZE	(1024*3)
byte	static_scanline[STATIC_SCANLINE_SIZE];
void LoadJPG (char *filename, byte **pic, int *width, int *height)
{
	struct jpeg_decompress_struct	cinfo;
	struct jpeg_error_mgr			jerr;
	byte							*rawdata, *rgbadata, *scanline, *p, *q;
	int								rawsize, i;

	crjpg_corrupted = false;
	// Load JPEG file into memory
	rawsize = FS_LoadFile_TryStatic	(	filename, (void **)&rawdata,
										static_rawdata, STATIC_RAWDATA_SIZE);
	if (!rawdata)
	{
		return;
	}

	// Knightmare- check for bad data
	if (	rawdata[6] != 'J'
		||	rawdata[7] != 'F'
		||	rawdata[8] != 'I'
		||	rawdata[9] != 'F') {
		if (rawdata != static_rawdata)
			FS_FreeFile(rawdata);
		return;
	}

	// Initialise libJpeg Object
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	// Feed JPEG memory into the libJpeg Object
	crjpg_mem_src(&cinfo, rawdata, rawsize);

	// Process JPEG header
	jpeg_read_header(&cinfo, true); // bombs out here

	// Start Decompression
	jpeg_start_decompress(&cinfo);

	// Check Color Components
	if(cinfo.output_components != 3)
	{
		jpeg_destroy_decompress(&cinfo);
		if (rawdata != static_rawdata)
			FS_FreeFile(rawdata);
		return;
	}

	// Allocate Memory for decompressed image
	rgbadata = malloc(cinfo.output_width * cinfo.output_height * 4);
	if(!rgbadata)
	{
		jpeg_destroy_decompress(&cinfo);
		if (rawdata != static_rawdata)
			FS_FreeFile(rawdata);
		return;
	}

	// Pass sizes to output
	*width = cinfo.output_width; *height = cinfo.output_height;

	// Allocate Scanline buffer
	if (cinfo.output_width * 3 < STATIC_SCANLINE_SIZE)
	{
		scanline = &static_scanline[0];
	}
	else
	{
		scanline = malloc(cinfo.output_width * 3);
		if(!scanline)
		{
			free(rgbadata);
			jpeg_destroy_decompress(&cinfo);
			if (rawdata != static_rawdata)
				FS_FreeFile(rawdata);
			return;
		}
	}

	// Read Scanlines, and expand from RGB to RGBA
	q = rgbadata;
	while(cinfo.output_scanline < cinfo.output_height)
	{
		p = scanline;
		jpeg_read_scanlines(&cinfo, &scanline, 1);

		for(i=0; i<cinfo.output_width; i++)
		{
			q[0] = p[0];
			q[1] = p[1];
			q[2] = p[2];
			q[3] = 255;

			p+=3; q+=4;
		}
	}

	// Free the scanline buffer
	if (scanline != &static_scanline[0])
		free(scanline);

	// Finish Decompression
	jpeg_finish_decompress(&cinfo);

	// Destroy JPEG object
	jpeg_destroy_decompress(&cinfo);

	// Free the raw data now that it's done being processed
	if (rawdata != static_rawdata)
    	FS_FreeFile(rawdata);
    
    if (crjpg_corrupted)
    	Com_Printf ("JPEG file %s is likely corrupted, please obtain a fresh copy.\n", filename);

	// Return the 'rgbadata'
	*pic = rgbadata;
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
	length = FS_LoadFile_TryStatic	(	name, (void **)&buffer, 
										static_rawdata, STATIC_RAWDATA_SIZE);
	if (!buffer)
	{
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
		if (buffer != static_rawdata)
			FS_FreeFile (buffer);
		return;
	}

	if (targa_header.colormap_type !=0
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24)) {
		//Com_Error (ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");
		if (buffer != static_rawdata)
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
		unsigned char red=0,green=0,blue=0,alphabyte=0,packetHeader,packetSize,j;
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

	if (buffer != static_rawdata)
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
R_FilterTexture

Applies brightness and contrast to the specified image while optionally computing
the image's average color.  Also handles image inversion and monochrome.  This is
all munged into one function to reduce loops on level load.
*/
void R_FilterTexture(unsigned *in, int width, int height)
{
	byte *pcb;
	int count;
	float fcb;
	int ix;
	static byte lut[256];
	static int first_time = 1;
	static float lut_gamma = 0.0f;
	static float lut_contrast = 0.0f;

	if ( lut_gamma != vid_gamma->value || lut_contrast != vid_contrast->value || first_time ) {
		first_time = 0;
		lut_gamma = vid_gamma->value;
		lut_contrast = vid_contrast->value;
		// build lookup table
		for ( ix = 0; ix < sizeof(lut); ix++ ) {
			fcb = (float)ix / 255.0f;
			fcb *= lut_gamma;
			if (fcb < 0.0f )
				fcb = 0.0f;
			fcb -= 0.5f;
			fcb *= lut_contrast;
			fcb += 0.5f;
			fcb *= 255.0f;
			if ( fcb >= 255.0f )
				lut[ix] = 255 ;
			else if (fcb <= 0.0f )
				lut[ix] = 0;
			else
				lut[ix] = (byte)fcb;
		}
	}

	// apply to image
	pcb = (byte*)in;
	count = width * height;
	while ( count-- ) {
		*pcb = lut[*pcb];
		++pcb;
		*pcb = lut[*pcb];
		++pcb;
		*pcb = lut[*pcb];
		++pcb;
		++pcb;
	}

}

static int	powers_of_two[] = {16,32,64,128,256,512,1024,2048,4096};
int		upload_width, upload_height;
qboolean	uploaded_paletted;


qboolean GL_Upload32 (unsigned *data, int width, int height,  qboolean mipmap, qboolean is_normalmap)
{
	int		samples;
	unsigned 	*scaled;
	int		scaled_width, scaled_height;
	int		i, c;
	byte		*scan;
	int comp;

	if(mipmap && !is_normalmap)
		R_FilterTexture(data, width, height);

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
			}
			if (scaled_width && scaled_height)
				break;
		}
		// let people sample down the world textures for speed
		if (mipmap)
		{
		    for (i = 0; i < gl_picmip->integer; i++)
		    {
		    	if (scaled_width <= 1 || scaled_height <= 1)
		    		break;
				scaled_width >>= 1;
				scaled_height >>= 1;
			}
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
		scaled=data;
	}

	if (mipmap)
		qglTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
	qglTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);

	if (scaled_width != width || scaled_height != height)
		free(scaled);

	upload_width = scaled_width;
	upload_height = scaled_height;
	if ( mipmap )
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	else
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_anisotropic->value);

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
	
	for (i=0 ; i<s ; i++)
    {
		p = data[i];
        trans[i] = d_8to24table[p];

        if (p == 255)
        {   // transparent, so scan around for another color
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
    return GL_Upload32 (trans, width, height, mipmap, false);
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

	if ( name[0] != '*' )
	{ // not a special name, remove the path part
		strcpy( image->bare_name, COM_SkipPath( name ) );
	}

	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	if (type == it_skin && bits == 8)
		R_FloodFillSkin(pic, width, height);

	// load little pics into the scrap
	if (image->type == it_particle && bits != 8
		&& image->width <= 128 && image->height <= 128)
	{
		int		x, y;
		int		i, j, k, k_s, l;
		int		texnum;

		texnum = Scrap_AllocBlock (image->width, image->height, &x, &y);
		if (texnum == -1)
			goto nonscrap;
		scrap_dirty = true;
		
		// copy the texels into the scrap block
		k = 0;
		for (i=0 ; i<image->height ; i++)
			for (j=0 ; j<image->width ; j++)
			    for (l = 0; l < 4; l++, k++)
				    scrap_texels[texnum][((y+i)*BLOCK_WIDTH + x + j)*4+l] = pic[k];
		image->texnum = TEXNUM_SCRAPS + texnum;
		image->scrap = true;
		image->has_alpha = true;
		image->sl = (double)(x+0.5)/(double)BLOCK_WIDTH;
		image->sh = (double)(x+image->width-0.5)/(double)BLOCK_WIDTH;
		image->tl = (double)(y+0.5)/(double)BLOCK_HEIGHT;
		image->th = (double)(y+image->height-0.5)/(double)BLOCK_HEIGHT;
	}
	else
	{
nonscrap:
		image->scrap = false;
		image->texnum = TEXNUM_IMAGES + (image - gltextures);
		GL_Bind(image->texnum);
		if (bits == 8) {
			image->has_alpha = GL_Upload8 (pic, width, height, (image->type != it_pic && image->type != it_sky), image->type == it_sky );
		} else {
			image->has_alpha = GL_Upload32 ((unsigned *)pic, width, height, (image->type != it_pic && image->type != it_particle && image->type != it_sky), image->type == it_bump );
		}
		image->upload_width = upload_width;		// after power of 2 and scales
		image->upload_height = upload_height;
		image->paletted = uploaded_paletted;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
	}

	COMPUTE_HASH_KEY( image->hash_key, name, i );
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
GL_GetImage

Finds an image, do not attempt to load it
===============
*/
image_t *GL_GetImage( const char * name )
{
	image_t *	image = NULL;
	unsigned int	hash_key;
	int		i;

	COMPUTE_HASH_KEY( hash_key, name, i );
	for ( i = 0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (hash_key == image->hash_key && !strcmp(name, image->name))
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	return NULL;
}


/*
===============
GL_FreeImage

Frees an image slot and deletes the texture
===============
*/
void GL_FreeImage( image_t * image )
{
	if ( ! image->texnum )
		return;
	qglDeleteTextures (1, (unsigned *)&image->texnum );
	if ( gl_state.currenttextures[gl_state.currenttmu] == image->texnum ) {
		gl_state.currenttextures[ gl_state.currenttmu ] = 0;
	}
	memset (image, 0, sizeof(*image));
}


/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t	*GL_FindImage (char *name, imagetype_t type)
{
	image_t		*image = NULL;
	int		len;
	byte		*pic, *palette;
	int		width, height;
	char		shortname[MAX_QPATH];

	if (!name)
		goto ret_image;	//	Com_Error (ERR_DROP, "GL_FindImage: NULL name");
	len = strlen(name);
	if (len < 5)
		goto ret_image;	//	Com_Error (ERR_DROP, "GL_FindImage: bad name: %s", name);

	//if HUD, then we want to load the one according to what it is set to.
	if(!strcmp(name, "pics/i_health.pcx"))
			strcpy(name, cl_hudimage1->string);
	if(!strcmp(name, "pics/i_score.pcx"))
			strcpy(name, cl_hudimage2->string);

	// look for it
	image = GL_GetImage( name );
	if ( image != NULL )
		return image;

	// strip off .pcx, .tga, etc...
	COM_StripExtension ( name, shortname );

	//
	// load the pic from disk
	//
	pic = NULL;
	palette = NULL;

	// try to load .tga first
	LoadTGA (va("%s.tga", shortname), &pic, &width, &height);
	if (pic)
	{
		image = GL_LoadPic (name, pic, width, height, type, 32);
		goto done;
	}
	LoadJPG (va("%s.jpg", shortname), &pic, &width, &height);
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

ret_image:
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

	// never free r_notexture
	r_notexture->registration_sequence = registration_sequence;
	
	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_pic || image->type == it_sprite || image->type == it_particle)
			continue;		// don't free pics or particles
		// free it
		qglDeleteTextures (1, (unsigned *)&image->texnum );
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

// TODO: have r_newrefdef available before calling these, put r_mirroretxture
// in that struct, base it on the viewport specified in that struct. (Needed
// for splitscreen.)
void R_InitMirrorTextures( void )
{
	byte	*data;
	int		size;
	int		size_oneside;

	//init the partial screen texture
	size_oneside = ceil((512.0f/1080.0f)*(float)viddef.height);
	size = size_oneside * size_oneside * 4;
	data = malloc( size );
	memset( data, 255, size );
	r_mirrortexture = GL_LoadPic( "***r_mirrortexture***", (byte *)data, size_oneside, size_oneside, it_pic, 32 );
	free ( data );
}

void R_InitDepthTextures( void )
{
	byte	*data;
	int		size, texture_height, texture_width;

	//find closer power of 2 to screen size
	for (texture_width = 1;texture_width < viddef.width;texture_width *= 2);
	for (texture_height = 1;texture_height < viddef.height;texture_height *= 2);

	//limit to 2048x2048 - anything larger is generally going to cause problems, and AA doesn't support res higher
	if(texture_width > 2048)
		texture_width = 2048;
	if(texture_height > 2048)
		texture_height = 2048;

	//init the framebuffer textures
	size = texture_width * texture_height * 4;

	data = malloc( size );
	memset( data, 255, size );
	r_depthtexture = GL_LoadPic( "***r_depthtexture***", (byte *)data, texture_width, texture_height, it_pic, 3 );
	r_depthtexture2 = GL_LoadPic( "***r_depthtexture2***", (byte *)data, texture_width, texture_height, it_pic, 3 );
	free ( data );
}


/*
===============
GL_InitImages
===============
*/
void	GL_InitImages (void)
{

	registration_sequence = 1;

	gl_state.inverse_intensity = 1;

	Draw_GetPalette ();

	R_InitBloomTextures();//BLOOMS
	R_InitMirrorTextures();//MIRRORS
	R_InitDepthTextures();//DEPTH(SHADOWMAPS)
	R_FB_InitTextures();//FULLSCREEN EFFECTS
	R_SI_InitTextures();//SIMPLE ITEMS
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
		qglDeleteTextures (1, (unsigned *)&image->texnum);
		memset (image, 0, sizeof(*image));
	}
}

