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
// r_misc.c

#include "r_local.h"
#include <GL/glu.h>

//For screenshots
#ifdef _WIN32
#include "jpeg/jpeglib.h"
#endif
#ifndef _WIN32
#include <jpeglib.h>
#endif


image_t *r_flare;
image_t *r_cubemap;

#define CUBEMAP_MAXSIZE 32

static void getCubeVector (int i, int cubesize, int x, int y, float *vector)
{
	float s, t, sc, tc, mag;

	s = ((float) x + 0.5) / (float) cubesize;
	t = ((float) y + 0.5) / (float) cubesize;
	sc = s * 2.0 - 1.0;
	tc = t * 2.0 - 1.0;

	// cleaned manky braces again here...
	switch (i)
	{
	case 0: vector[0] = 1.0; vector[1] = - tc; vector[2] = - sc; break;
	case 1: vector[0] = - 1.0; vector[1] = - tc; vector[2] = sc; break;
	case 2: vector[0] = sc; vector[1] = 1.0; vector[2] = tc; break;
	case 3: vector[0] = sc; vector[1] = - 1.0; vector[2] = - tc; break;
	case 4: vector[0] = sc; vector[1] = - tc; vector[2] = 1.0; break;
	case 5: vector[0] = - sc; vector[1] = - tc; vector[2] = - 1.0; break;
	}

	mag = 1.0 / sqrt (vector[0]* vector[0]+ vector[1]* vector[1]+ vector[2]* vector[2]);

	vector[0]*= mag;
	vector[1]*= mag;
	vector[2]*= mag;
}

void R_InitCubemapTextures (void)
{
	// this texture doesn't need to be huge. I've seen examples where it goes up to 128 or higher, but 32
	// is perfectly adequate. Who am I to argue with Mr. Kilgard?
	float vector[3];
	int i, x, y;
	GLubyte pixels[CUBEMAP_MAXSIZE * CUBEMAP_MAXSIZE * 4];

	// set up a dummy for it to get a valid image_t filled in.  we'll be deleting and recreating the texture data
	// using glTexImage2D when we actually set up each individual one.  we generate it as a wall texture to ensure that
	// we get texnums generated for the backup textures as well
	r_cubemap = GL_LoadPic ("***cubemap***", (byte *) pixels, CUBEMAP_MAXSIZE, CUBEMAP_MAXSIZE, it_pic, 32);

	// switch the texture state
	qglDisable (GL_TEXTURE_2D);
	qglEnable (GL_TEXTURE_CUBE_MAP_EXT);

	// bind to the regular texture number
	qglBindTexture (GL_TEXTURE_CUBE_MAP_EXT, r_cubemap->texnum);

	// set up the texture parameters - did a clamp on R as well...
	qglTexParameteri (GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	qglTexParameteri (GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	qglTexParameteri (GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	qglTexParameteri (GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameteri (GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	for (i = 0; i < 6; i++)
	{
		for (y = 0; y < CUBEMAP_MAXSIZE; y++)
		{
			for (x = 0; x < CUBEMAP_MAXSIZE; x++)
			{
				getCubeVector (i, CUBEMAP_MAXSIZE, x, y, vector);

				pixels[4 * (y * CUBEMAP_MAXSIZE + x) + 0] = 128 + 127 * vector[0];
				pixels[4 * (y * CUBEMAP_MAXSIZE + x) + 1] = 128 + 127 * vector[1];
				pixels[4 * (y * CUBEMAP_MAXSIZE + x) + 2] = 128 + 127 * vector[2];
			}
		}

		// cube map me baybeee
		qglTexImage2D (GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT + i, 0, GL_RGBA, CUBEMAP_MAXSIZE, CUBEMAP_MAXSIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	}

	// done - restore the texture state
	qglDisable (GL_TEXTURE_CUBE_MAP_EXT);
	qglEnable (GL_TEXTURE_2D);
}
#undef CUBEMAP_MAXSIZE
/*
==================
R_InitParticleTexture
==================
*/
byte	dottexture[16][16] =
{
	{0,0,0,0,0,2,6,7,7,5,2,0,0,0,0,0},
    {0,0,0,1,8,20,33,40,39,30,18,7,0,0,0,0},
    {0,0,1,12,35,60,78,87,87,76,56,30,10,0,0,0},
    {0,0,9,37,72,103,125,136,135,122,98,67,31,7,0,0},
    {0,3,25,66,108,142,168,180,179,164,137,102,60,20,2,0},
	{0,9,44,91,137,175,206,220,219,201,170,130,84,38,6,0},
    {0,15,58,109,157,200,233,249,248,229,194,150,101,51,11,0},
    {1,18,66,118,167,212,245,254,254,242,206,159,110,57,14,0},
    {1,18,64,116,165,209,243,254,254,239,203,158,108,56,13,0},
    {0,13,54,104,152,193,225,242,240,221,187,144,97,47,9,0},
    {0,7,39,84,128,165,194,209,207,190,160,122,77,32,5,0},
    {0,2,20,57,97,130,154,167,166,152,125,91,51,15,1,0},
    {0,0,6,27,60,89,111,121,120,108,85,55,23,5,0,0},
    {0,0,0,7,24,46,63,72,72,61,43,20,6,0,0,0},
    {0,0,0,0,4,11,20,26,26,20,10,3,0,0,0,0},
    {0,0,0,0,0,0,2,3,3,2,0,0,0,0,0,0},
};

void R_InitParticleTexture (void)
{
	int		x,y;
	byte	data[16][16][4];
	char flares[MAX_QPATH];
	//
	// particle texture
	//
	for (x=0 ; x<16 ; x++)
	{
		for (y=0 ; y<16 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]; // c14 Just this line changes
		}
	}
	r_particletexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);

	r_smoketexture = R_RegisterParticlePic("smoke");//etc etc etc...
    r_particletexture = R_RegisterParticlePic("particle");
	r_explosiontexture = R_RegisterParticlePic("explosion");
	r_explosion1texture = R_RegisterParticlePic("r_explod_1");
	r_explosion2texture = R_RegisterParticlePic("r_explod_2");
	r_explosion3texture = R_RegisterParticlePic("r_explod_3");
	r_explosion4texture = R_RegisterParticlePic("r_explod_4");
	r_explosion5texture = R_RegisterParticlePic("r_explod_5");
	r_explosion6texture = R_RegisterParticlePic("r_explod_6");
	r_explosion7texture = R_RegisterParticlePic("r_explod_7");
	r_bloodtexture = R_RegisterParticlePic("blood");
	r_pufftexture = R_RegisterParticlePic("puff");
	r_blastertexture = R_RegisterParticlePic("blaster");
	r_bflashtexture = R_RegisterParticlePic("bflash");
	r_cflashtexture = R_RegisterParticlePic("cflash");
	r_leaderfieldtexture = R_RegisterParticlePic("leaderfield");
	r_deathfieldtexture = R_RegisterParticlePic("deathfield");
	r_shelltexture = R_RegisterParticlePic("shell");
	r_hittexture = R_RegisterParticlePic("aflash");
	r_bubbletexture = R_RegisterParticlePic("bubble");
	r_reflecttexture = R_RegisterParticlePic("reflect");
	r_shottexture = R_RegisterParticlePic("dflash");
	r_sayicontexture = R_RegisterParticlePic("sayicon");
	r_flaretexture = R_RegisterParticlePic("flare");
	r_beamtexture = R_RegisterGfxPic("greenlightning");
	r_beam2texture = R_RegisterGfxPic("greenline");
	r_beam3texture = R_RegisterGfxPic("electrics3d");
	r_bullettexture = R_RegisterParticlePic("bullethole");
	r_radarmap = GL_FindImage("gfx/radar/radarmap",it_pic);
	r_around = GL_FindImage("gfx/radar/around",it_pic);
	if (!r_particletexture) {                                 //c14 add this line
		r_particletexture = GL_LoadPic ("***particle***", (byte *)data, 16, 16, it_sprite, 32);
    }                                                         //c14 add this line

	//
	// also use this for bad textures, but without alpha
	//
	for (x=0 ; x<16 ; x++)
	{
		for (y=0 ; y<16 ; y++)
		{
			data[y][x][0] = dottexture[x&3][y&3]*255;
			data[y][x][1] = 0; // dottexture[x&3][y&3]*255;
			data[y][x][2] = 0; //dottexture[x&3][y&3]*255;
			data[y][x][3] = 255;
		}
	}
	r_notexture = GL_LoadPic ("***r_notexture***", (byte *)data, 16, 16, it_wall, 32);

	//R_InitCubemapTextures (); //for future use

	Com_sprintf (flares, sizeof(flares), "gfx/flares/flare0.tga");
	r_flare = GL_FindImage(flares, it_pic);

}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;

/*
==================
GL_ScreenShot_JPEG
==================
*/
void GL_ScreenShot_JPEG(void)
{
	struct		jpeg_compress_struct cinfo;
	struct		jpeg_error_mgr jerr;
	byte			*rgbdata;
	JSAMPROW	s[1];
	FILE			*f;
	char			picname[80], checkname[MAX_OSPATH];
	int			i, offset;

	/* Create the scrnshots directory if it doesn't exist */
	Com_sprintf(checkname, sizeof(checkname), "%s/scrnshot", FS_Gamedir());
	Sys_Mkdir(checkname);

	strcpy(picname,"AlienArena_000.jpg");

	for (i=0 ; i<=999 ; i++)
	{
		picname[11] = i/100     + '0';
		picname[12] = (i/10)%10 + '0';
		picname[13] = i%10      + '0';

		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	}

	if (i == 1000) {
		Com_Printf(PRINT_ALL, "GL_ScreenShot_JPEG: Couldn't create a file (You probably have taken to many screenshots!)\n");
		return;
	}

	/* Open the file for Binary Output */
	f = fopen(checkname, "wb");

	if (!f) {
		Com_Printf(PRINT_ALL, "GL_ScreenShot_JPEG: Couldn't create a file\n");
		return;
	}

	/* Allocate room for a copy of the framebuffer */
	rgbdata = malloc(vid.width * vid.height * 3);

	if (!rgbdata) {
		fclose(f);
		return;
	}

	/* Read the framebuffer into our storage */
	qglReadPixels(0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, rgbdata);

	/* Initialise the JPEG compression object */
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, f);

	/* Setup JPEG Parameters */
	cinfo.image_width = vid.width;
	cinfo.image_height = vid.height;
	cinfo.in_color_space = JCS_RGB;
	cinfo.input_components = 3;
	jpeg_set_defaults(&cinfo);

	if ((gl_screenshot_jpeg_quality->value >= 101) || (gl_screenshot_jpeg_quality->value <= 0))
		Cvar_Set("gl_screenshot_jpeg_quality", "85");

	jpeg_set_quality(&cinfo, gl_screenshot_jpeg_quality->value, TRUE);

	/* Start Compression */
	jpeg_start_compress(&cinfo, true);

	/* Feed Scanline data */
	offset = (cinfo.image_width * cinfo.image_height * 3) - (cinfo.image_width * 3);

	while (cinfo.next_scanline < cinfo.image_height) {
		s[0] = &rgbdata[offset - (cinfo.next_scanline * (cinfo.image_width * 3))];
		jpeg_write_scanlines(&cinfo, s, 1);
	}

	/* Finish Compression */
	jpeg_finish_compress(&cinfo);

	/* Destroy JPEG object */
	jpeg_destroy_compress(&cinfo);

	/* Close File */
	fclose(f);

	/* Free Temp Framebuffer */
	free(rgbdata);

	/* Done! */
	Com_Printf ("Wrote %s\n", picname);
}

/*
==================
GL_ScreenShot_TGA
==================
*/
void GL_ScreenShot_TGA (void)
{
	byte		*buffer;
	char		picname[80];
	char		checkname[MAX_OSPATH];
	int		i, c, temp;
	FILE		*f;

	// create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot", FS_Gamedir());
	Sys_Mkdir (checkname);

	// find a file name to save it to
	strcpy(picname,"AlienArena_000.tga");

	for (i=0 ; i<=999 ; i++)
	{
		picname[11] = i/100     + '0';
		picname[12] = (i/10)%10 + '0';
		picname[13] = i%10      + '0';
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	}

	if (i==1000)
	{
		Com_Printf ("GL_ScreenShot_TGA: Couldn't create file %s (You probably have taken to many screenshots!)\n", picname);
		return;
	}

	buffer = malloc(vid.width*vid.height*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vid.width&255;
	buffer[13] = vid.width>>8;
	buffer[14] = vid.height&255;
	buffer[15] = vid.height>>8;
	buffer[16] = 24;	// pixel size

	qglReadPixels (0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 );

	// swap rgb to bgr
	c = 18+vid.width*vid.height*3;
	for (i=18 ; i<c ; i+=3)
	{
		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	f = fopen (checkname, "wb");
	fwrite (buffer, 1, c, f);
	fclose (f);

	free (buffer);
	Com_Printf ("Wrote %s\n", picname);
}

/*
==================
GL_ScreenShot_f
==================
*/
void GL_ScreenShot_f (void)
{
	if (strcmp(gl_screenshot_type->string, "jpeg") == 0)
		GL_ScreenShot_JPEG();
	else if (strcmp(gl_screenshot_type->string, "tga") == 0)
		GL_ScreenShot_TGA();
	else {
		Com_Printf("Invalid screenshot type %s; resetting to jpeg.\n", gl_screenshot_type->string);
		Cvar_Set("gl_screenshot_type", "jpeg");
		GL_ScreenShot_JPEG();
	}
}

/*
** GL_Strings_f
*/
void GL_Strings_f( void )
{
	Com_Printf ("GL_VENDOR: %s\n", gl_config.vendor_string );
	Com_Printf ("GL_RENDERER: %s\n", gl_config.renderer_string );
	Com_Printf ("GL_VERSION: %s\n", gl_config.version_string );
	Com_Printf ("GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	qglClearColor (1,0, 0.5 , 0.5);
	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.666);

	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);

	qglColor4f (1,1,1,1);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel (GL_FLAT);

	GL_TextureMode( gl_texturemode->string );
	GL_TextureAlphaMode( gl_texturealphamode->string );
	GL_TextureSolidMode( gl_texturesolidmode->string );

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv( GL_REPLACE );

	if ( qglPointParameterfEXT )
	{
		float attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		qglEnable( GL_POINT_SMOOTH );
		qglPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value );
		qglPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value );
		qglPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, attenuations );
	}

	if ( qglColorTableEXT && gl_ext_palettedtexture->value )
	{
		qglEnable( GL_SHARED_TEXTURE_PALETTE_EXT );

		GL_SetTexturePalette( d_8to24table );
	}

	GL_UpdateSwapInterval();
}

void GL_UpdateSwapInterval( void )
{
	if ( gl_swapinterval->modified )
	{
		gl_swapinterval->modified = false;

		if ( !gl_state.stereo_enabled )
		{
#ifdef _WIN32
			if ( qwglSwapIntervalEXT )
				qwglSwapIntervalEXT( gl_swapinterval->value );
#endif
		}
	}
}

