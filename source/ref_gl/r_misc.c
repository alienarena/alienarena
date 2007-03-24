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
image_t *r_detailtexture;
image_t *r_flare;

// MH - detail textures begin
// blatantly plagiarized darkplaces code
// no bound in Q2.
// this would be better as a #define but it's not runtime so it doesn't matter
int bound (int smallest, int val, int biggest)
{
	if (val < smallest) return smallest;
	if (val > biggest) return biggest;

	return val;
}


void fractalnoise(byte *noise, int size, int startgrid)
{
	int x, y, g, g2, amplitude, min, max, size1 = size - 1, sizepower, gridpower;
	int *noisebuf;
#define n(x,y) noisebuf[((y)&size1)*size+((x)&size1)]

	for (sizepower = 0;(1 << sizepower) < size;sizepower++);
	if (size != (1 << sizepower))
		Sys_Error("fractalnoise: size must be power of 2\n");

	for (gridpower = 0;(1 << gridpower) < startgrid;gridpower++);
	if (startgrid != (1 << gridpower))
		Sys_Error("fractalnoise: grid must be power of 2\n");

	startgrid = bound (0, startgrid, size);

	amplitude = 0xFFFF; // this gets halved before use
	noisebuf = malloc(size*size*sizeof(int));
	memset(noisebuf, 0, size*size*sizeof(int));

	for (g2 = startgrid;g2;g2 >>= 1)
	{
		// brownian motion (at every smaller level there is random behavior)
		amplitude >>= 1;
		for (y = 0;y < size;y += g2)
			for (x = 0;x < size;x += g2)
				n(x,y) += (rand()&amplitude);

		g = g2 >> 1;
		if (g)
		{
			// subdivide, diamond-square algorithm (really this has little to do with squares)
			// diamond
			for (y = 0;y < size;y += g2)
				for (x = 0;x < size;x += g2)
					n(x+g,y+g) = (n(x,y) + n(x+g2,y) + n(x,y+g2) + n(x+g2,y+g2)) >> 2;
			// square
			for (y = 0;y < size;y += g2)
				for (x = 0;x < size;x += g2)
				{
					n(x+g,y) = (n(x,y) + n(x+g2,y) + n(x+g,y-g) + n(x+g,y+g)) >> 2;
					n(x,y+g) = (n(x,y) + n(x,y+g2) + n(x-g,y+g) + n(x+g,y+g)) >> 2;
				}
		}
	}
	// find range of noise values
	min = max = 0;
	for (y = 0;y < size;y++)
		for (x = 0;x < size;x++)
		{
			if (n(x,y) < min) min = n(x,y);
			if (n(x,y) > max) max = n(x,y);
		}
	max -= min;
	max++;
	// normalize noise and copy to output
	for (y = 0;y < size;y++)
		for (x = 0;x < size;x++)
			*noise++ = (byte) (((n(x,y) - min) * 256) / max);
	free(noisebuf);
#undef n
}


void R_BuildDetailTexture (void) 
{ 
   int x, y, light; 
   float vc[3], vx[3], vy[3], vn[3], lightdir[3]; 

   // increase this if you want 
   #define DETAILRESOLUTION 256 

   byte data[DETAILRESOLUTION][DETAILRESOLUTION][4], noise[DETAILRESOLUTION][DETAILRESOLUTION]; 

   // this looks odd, but it's necessary cos Q2's uploader will lightscale the texture, which 
   // will cause all manner of unholy havoc.  So we need to run it through GL_LoadPic before 
   // we even fill in the data just to fill in the rest of the image_t struct, then we'll 
   // build the texture for OpenGL manually later on. 
   r_detailtexture = GL_LoadPic ("***detail***", (byte *) data, DETAILRESOLUTION, DETAILRESOLUTION, it_wall, 32); 

   lightdir[0] = 0.5; 
   lightdir[1] = 1; 
   lightdir[2] = -0.25; 
   VectorNormalize(lightdir); 

   fractalnoise(&noise[0][0], DETAILRESOLUTION, DETAILRESOLUTION >> 4); 
   for (y = 0;y < DETAILRESOLUTION;y++) 
   { 
      for (x = 0;x < DETAILRESOLUTION;x++) 
      { 
         vc[0] = x; 
         vc[1] = y; 
         vc[2] = noise[y][x] * (1.0f / 32.0f); 
         vx[0] = x + 1; 
         vx[1] = y; 
         vx[2] = noise[y][(x + 1) % DETAILRESOLUTION] * (1.0f / 32.0f); 
         vy[0] = x; 
         vy[1] = y + 1; 
         vy[2] = noise[(y + 1) % DETAILRESOLUTION][x] * (1.0f / 32.0f); 
         VectorSubtract(vx, vc, vx); 
         VectorSubtract(vy, vc, vy); 
         CrossProduct(vx, vy, vn); 
         VectorNormalize(vn); 
         light = 128 - DotProduct(vn, lightdir) * 128; 
         light = bound(0, light, 255); 
         data[y][x][0] = data[y][x][1] = data[y][x][2] = light; 
         data[y][x][3] = 255; 
      } 
   } 

   // now we build the texture manually.  you can reuse this code for auto mipmap generation 
   // in other contexts if you wish.  defines are in qgl.h 
   // first, bind the texture.  probably not necessary, but it seems like good practice 
   GL_Bind (r_detailtexture->texnum); 

   // upload the correct texture data without any lightscaling interference 
   gluBuild2DMipmaps (GL_TEXTURE_2D, GL_RGBA, DETAILRESOLUTION, DETAILRESOLUTION, GL_RGBA, GL_UNSIGNED_BYTE, (byte *) data); 

   // set some quick and ugly filtering modes.  detail texturing works by scrunching a 
   // large image into a small space, so there's no need for sexy filtering (change this, 
   // turn off the blend in R_DrawDetailSurfaces, and compare if you don't believe me). 
   // this also helps to take some of the speed hit from detail texturing away. 
   // fucks up for some reason so using different filtering. 
   qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min); 
   qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max); 

} 
// MH - detail textures end


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
	
	// MH - detail textures begin
	// and for detail textures
	R_BuildDetailTexture ();
	// MH - detail textures end

	//lava haze
//	R_InitSmokes();

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
GL_ScreenShot_f
================== 
*/  
void GL_ScreenShot_f (void) 
{
	byte		*buffer;
	char		picname[80]; 
	char		checkname[MAX_OSPATH];
	int			i, c, temp;
	FILE		*f;

	// create the scrnshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot", FS_Gamedir());
	Sys_Mkdir (checkname);

// 
// find a file name to save it to 
// 
	strcpy(picname,"codered00.tga");

	for (i=0 ; i<=99 ; i++) 
	{ 
		picname[5] = i/10 + '0'; 
		picname[6] = i%10 + '0'; 
		Com_sprintf (checkname, sizeof(checkname), "%s/scrnshot/%s", FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	} 
	if (i==100) 
	{
		Com_Printf ("SCR_ScreenShot_f: Couldn't create a file\n"); 
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

