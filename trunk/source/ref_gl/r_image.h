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

/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

typedef enum 
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky,
	it_bumpmap
} imagetype_t;

typedef struct image_s
{
	char	name[MAX_QPATH];			// game path, including extension
	char	bare_name[MAX_QPATH];		// filename only, as called when searching
	imagetype_t	type;
	int		width, height;				// source image
	int		upload_width, upload_height;	// after power of two and picmip
	int		registration_sequence;		// 0 = free
	struct msurface_s	*texturechain;	// for sort-by-texture world drawing
	int		texnum;						// gl texture binding
	float	sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	qboolean	scrap;
	qboolean	has_alpha;
	qboolean paletted;
	qboolean is_cin;					// Heffo - To identify a cin texture's image_t
	void *script;

} image_t;

#define	TEXNUM_LIGHTMAPS	1024
#define	TEXNUM_SCRAPS		1152
#define	TEXNUM_IMAGES		1153

#define		MAX_GLTEXTURES	4096 //was 1024

//particles
extern image_t		*r_notexture;
extern image_t		*r_particletexture;
extern image_t		*r_smoketexture; 
extern image_t		*r_explosiontexture;
extern image_t		*r_explosion1texture;
extern image_t		*r_explosion2texture;
extern image_t		*r_explosion3texture;
extern image_t		*r_explosion4texture;
extern image_t		*r_explosion5texture;
extern image_t		*r_explosion6texture;
extern image_t		*r_explosion7texture;
extern image_t		*r_bloodtexture;
extern image_t		*r_pufftexture;
extern image_t		*r_blastertexture;
extern image_t		*r_bflashtexture;
extern image_t		*r_cflashtexture;
extern image_t		*r_leaderfieldtexture;
extern image_t		*r_deathfieldtexture;
extern image_t      *r_shelltexture;     // c14 added shell texture
extern image_t		*r_hittexture;
extern image_t		*r_bubbletexture;
extern image_t		*r_reflecttexture;
extern image_t		*r_shottexture;
extern image_t		*r_sayicontexture;
extern image_t		*r_flaretexture;

extern	image_t		gltextures[MAX_GLTEXTURES];
extern	int			numgltextures;

extern image_t		*r_flare;

extern	image_t		*r_detailtexture;
