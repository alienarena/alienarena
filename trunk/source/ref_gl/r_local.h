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

#ifdef _WIN32
#  include <windows.h>
#endif

#include <stdio.h>

#include <GL/gl.h>
#include <math.h>
#include "glext.h"

#ifndef __unix__
#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT GL_COLOR_INDEX
#endif
#endif

#include "../client/ref.h"
#include "../client/vid.h"

#include "qgl.h"
#include "r_math.h"
#define	REF_VERSION	"GL 0.01"

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2

#define	DLIGHT_CUTOFF	64

extern	viddef_t	vid;

#include "r_image.h"

//===================================================================

typedef enum
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

#include "r_model.h"

extern float	r_frametime;

void GL_BeginRendering (int *x, int *y, int *width, int *height);
void GL_EndRendering (void);

void GL_SetDefaultState( void );
void GL_UpdateSwapInterval( void );

extern	float	gldepthmin, gldepthmax;

#define	MAX_LBM_HEIGHT		512

#define BACKFACE_EPSILON	0.01

extern	entity_t	*currententity;
extern	model_t		*currentmodel;
extern	int			r_visframecount;
extern	int			r_framecount;
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;
extern 	int c_flares;
extern  int c_grasses;
extern	int c_beams;

extern	int			gl_filter_min, gl_filter_max;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_speeds;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_lerpmodels;

extern	cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level
extern  cvar_t  *r_wave; //water waves

extern cvar_t	*gl_ext_swapinterval;
extern cvar_t	*gl_ext_palettedtexture;
extern cvar_t	*gl_ext_multitexture;
extern cvar_t	*gl_ext_pointparameters;
extern cvar_t	*gl_ext_compiled_vertex_array;

extern cvar_t	*gl_particle_min_size;
extern cvar_t	*gl_particle_max_size;
extern cvar_t	*gl_particle_size;
extern cvar_t	*gl_particle_att_a;
extern cvar_t	*gl_particle_att_b;
extern cvar_t	*gl_particle_att_c;

extern	cvar_t	*gl_bitdepth;
extern	cvar_t	*gl_mode;
extern	cvar_t	*gl_log;
extern	cvar_t	*gl_lightmap;
extern	cvar_t	*gl_shadows;
extern	cvar_t	*gl_dynamic;
extern	cvar_t	*gl_nobind;
extern	cvar_t	*gl_round_down;
extern	cvar_t	*gl_picmip;
extern	cvar_t	*gl_skymip;
extern	cvar_t	*gl_showtris;
extern	cvar_t	*gl_showpolys;
extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_cull;
extern	cvar_t	*gl_poly;
extern	cvar_t	*gl_texsort;
extern	cvar_t	*gl_polyblend;
extern	cvar_t	*gl_flashblend;
extern	cvar_t	*gl_lightmaptype;
extern	cvar_t	*gl_modulate;
extern	cvar_t	*gl_playermip;
extern	cvar_t	*gl_drawbuffer;
extern	cvar_t	*gl_3dlabs_broken;
extern	cvar_t	*gl_driver;
extern	cvar_t	*gl_swapinterval;
extern	cvar_t	*gl_texturemode;
extern	cvar_t	*gl_texturealphamode;
extern	cvar_t	*gl_texturesolidmode;
extern	cvar_t	*gl_lockpvs;
extern	cvar_t	*gl_rtlights;

extern	cvar_t	*vid_fullscreen;
extern	cvar_t	*vid_gamma;
extern  cvar_t	*vid_contrast;

extern	cvar_t	*intensity;

extern cvar_t *r_anisotropic;
extern cvar_t *r_ext_max_anisotropy;

// Vic - begin

extern cvar_t	*r_overbrightbits;
extern cvar_t	*gl_ext_mtexcombine;

// Vic - end
extern cvar_t	*gl_normalmaps;
extern cvar_t	*gl_parallaxmaps;
extern cvar_t	*gl_specular;
extern cvar_t	*gl_glsl_postprocess;

extern	cvar_t	*r_shaders;
extern	cvar_t	*r_bloom;
extern	cvar_t	*r_lensflare;
extern	cvar_t	*r_lensflare_intens;
extern	cvar_t	*r_drawsun;

extern	qboolean	map_fog;
extern	char		map_music[128];
extern  unsigned	r_weather;

extern  cvar_t		*r_minimap;
extern  cvar_t		*r_minimap_size;
extern  cvar_t		*r_minimap_zoom;
extern  cvar_t		*r_minimap_style;

extern	cvar_t	*gl_mirror;

extern	cvar_t	*gl_arb_fragment_program; 
extern	cvar_t	*gl_glsl_shaders;


extern	cvar_t	*sys_affinity;
extern	cvar_t	*sys_priority;

extern	cvar_t	*gl_screenshot_type;
extern	cvar_t	*gl_screenshot_jpeg_quality;

extern	cvar_t	*r_legacy;

extern	int		gl_lightmap_format;
extern	int		gl_solid_format;
extern	int		gl_alpha_format;
extern	int		gl_tex_solid_format;
extern	int		gl_tex_alpha_format;

extern	int		c_visible_lightmaps;
extern	int		c_visible_textures;

extern	float	r_world_matrix[16];
extern float r_project_matrix[16];
extern int	r_viewport[4];
extern	float		r_farclip, r_farclip_min, r_farclip_bias;
void GL_Bind (int texnum);
void GL_MBind( GLenum target, int texnum );
void GL_TexEnv( GLenum value );
void GL_EnableMultitexture( qboolean enable );
void GL_SelectTexture( GLenum );
void vectoangles (vec3_t value1, vec3_t angles);
void R_LightPoint (vec3_t p, vec3_t color, qboolean addDynamic);
void R_LightPointDynamics (vec3_t p, vec3_t color, m_dlight_t *list, int *amount, int max);
void R_PushDlights (void);
void R_PushDlightsForBModel (entity_t *e);
void SetVertexOverbrights (qboolean toggle);
void RefreshFont (void);

//====================================================================
typedef struct
{
	vec3_t	origin;
	vec3_t  angle;
	float	alpha;
	float	dist;
	int		type;
	int		texnum;
	int		blenddst;
	int		blendsrc;
	int		color;
	float   scale;
} gparticle_t;

extern	model_t	*r_worldmodel;

extern	unsigned	d_8to24table[256];

extern	int		registration_sequence;


void V_AddBlend (float r, float g, float b, float a, float *v_blend);

int	R_Init( void *hinstance, void *hWnd );
void R_Shutdown( void );

void R_RenderView (refdef_t *fd);
void GL_ScreenShot_f (void);
void R_DrawAliasModel (entity_t *e);
void R_DrawBrushModel (entity_t *e);
void R_DrawBeam( entity_t *e );
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawAlphaSurfaces (void);
void R_RenderBrushPoly (msurface_t *fa);
void R_DrawSpecialSurfaces(void);
void R_InitParticleTexture (void);
void R_DrawParticles (void);
void GL_DrawRadar(void);
void Draw_InitLocal (void);
void GL_SubdivideSurface (msurface_t *fa);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
qboolean R_CullOrigin(vec3_t origin);
void R_RotateForEntity (entity_t *e);
void R_MarkLeaves (void);
void R_AddSkySurface (msurface_t *fa);
void GL_RenderWaterPolys (msurface_t *fa, int texnum, float scaleX, float scaleY);
void R_ClearSkyBox (void);
void R_DrawSkyBox (void);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);
float R_ShadowLight (vec3_t pos, vec3_t lightAdd, int type);
void R_DrawShadowVolume(entity_t * e);
void R_DrawShadowWorld(void);
void R_ShadowBlend(float alpha);
#ifdef __unix__
void R_ReadFogScript(char config_file[128]);
void R_ReadMusicScript(char config_file[128]);
#endif
void R_GLSLPostProcess(void);
void R_FB_InitTextures(void);

//VBO
void R_VCInit(void);
void GL_BindVBO(vertCache_t *cache);
vertCache_t *R_VCFindCache(vertStoreMode_t store, entity_t *ent);
vertCache_t *R_VCLoadData(vertCacheMode_t mode, int size, void *buffer, vertStoreMode_t store, entity_t *ent);
void R_VCShutdown(void);

//BLOOMS[start]
//
// r_bloom.c
//
void R_BloomBlend( refdef_t *fd );
void R_InitBloomTextures( void );
//BLOOMS[end]

//FLARES and SUN
void R_RenderFlares (void);
void R_DrawVegetationSurface (void);
void R_DrawBeamSurface ( void );
void R_InitSun();
void R_RenderSun();
vec3_t sun_origin;
qboolean spacebox;

//Player icons
extern float	scr_playericonalpha;

void	Draw_GetPicSize (int *w, int *h, char *name);
void	Draw_Pic (int x, int y, char *name);
void	Draw_ScaledPic (int x, int y, float scale, char *pic);
void	Draw_StretchPic (int x, int y, int w, int h, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_ColorChar (int x, int y, int num, vec4_t color);
void	Draw_ScaledChar(float x, float y, int num, float scale);
void	Draw_ScaledColorChar (float x, float y, int num, vec4_t color, float scale);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_FadeScreen (void);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_BeginFrame( float camera_separation );
void	R_SwapBuffers( int );
void	R_SetPalette ( const unsigned char *palette);

int		Draw_GetPalette (void);

void	GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight);

image_t *R_RegisterSkin (char *name);
image_t *R_RegisterParticlePic(char *name);
image_t *R_RegisterParticleNormal(char *name);
image_t *R_RegisterGfxPic(char *name);

void	LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits);
image_t	*GL_FindImage (char *name, imagetype_t type);
void	GL_TextureMode( char *string );
void	GL_ImageList_f (void);

void	GL_SetTexturePalette( unsigned palette[256] );

void	GL_InitImages (void);
void	GL_ShutdownImages (void);

void	GL_FreeUnusedImages (void);

void	GL_TextureAlphaMode( char *string );
void	GL_TextureSolidMode( char *string );

/*
** GL config stuff
*/
#define GL_RENDERER_VOODOO		0x00000001
#define GL_RENDERER_VOODOO2   	0x00000002
#define GL_RENDERER_VOODOO_RUSH	0x00000004
#define GL_RENDERER_BANSHEE		0x00000008
#define	GL_RENDERER_3DFX		0x0000000F

#define GL_RENDERER_PCX1		0x00000010
#define GL_RENDERER_PCX2		0x00000020
#define GL_RENDERER_PMX			0x00000040
#define	GL_RENDERER_POWERVR		0x00000070

#define GL_RENDERER_PERMEDIA2	0x00000100
#define GL_RENDERER_GLINT_MX	0x00000200
#define GL_RENDERER_GLINT_TX	0x00000400
#define GL_RENDERER_3DLABS_MISC	0x00000800
#define	GL_RENDERER_3DLABS		0x00000F00

#define GL_RENDERER_REALIZM		0x00001000
#define GL_RENDERER_REALIZM2	0x00002000
#define	GL_RENDERER_INTERGRAPH	0x00003000

#define GL_RENDERER_3DPRO		0x00004000
#define GL_RENDERER_REAL3D		0x00008000
#define GL_RENDERER_RIVA128		0x00010000
#define GL_RENDERER_DYPIC		0x00020000

#define GL_RENDERER_V1000		0x00040000
#define GL_RENDERER_V2100		0x00080000
#define GL_RENDERER_V2200		0x00100000
#define	GL_RENDERER_RENDITION	0x001C0000

#define GL_RENDERER_O2          0x00100000
#define GL_RENDERER_IMPACT      0x00200000
#define GL_RENDERER_RE			0x00400000
#define GL_RENDERER_IR			0x00800000
#define	GL_RENDERER_SGI			0x00F00000

#define GL_RENDERER_MCD			0x01000000
#define GL_RENDERER_OTHER		0x80000000

#define GLSTATE_DISABLE_ALPHATEST	if (gl_state.alpha_test) { qglDisable(GL_ALPHA_TEST); gl_state.alpha_test=false; }
#define GLSTATE_ENABLE_ALPHATEST	if (!gl_state.alpha_test) { qglEnable(GL_ALPHA_TEST); gl_state.alpha_test=true; }

#define GLSTATE_DISABLE_BLEND		if (gl_state.blend) { qglDisable(GL_BLEND); gl_state.blend=false; }
#define GLSTATE_ENABLE_BLEND		if (!gl_state.blend) { qglEnable(GL_BLEND); gl_state.blend=true; }

#define GLSTATE_DISABLE_TEXGEN		if (gl_state.texgen) { qglDisable(GL_TEXTURE_GEN_S); qglDisable(GL_TEXTURE_GEN_T); qglDisable(GL_TEXTURE_GEN_R); gl_state.texgen=false; }
#define GLSTATE_ENABLE_TEXGEN		if (!gl_state.texgen) { qglEnable(GL_TEXTURE_GEN_S); qglEnable(GL_TEXTURE_GEN_T); qglEnable(GL_TEXTURE_GEN_R); gl_state.texgen=true; }

#define GL_COMBINE                        0x8570
#define GL_DOT3_RGB                       0x86AE

typedef struct
{
	int         renderer;
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *extensions_string;
	qboolean	allow_cds;
	qboolean 	mtexcombine;
} glconfig_t;

typedef struct
{
	float		inverse_intensity;
	qboolean	fullscreen;

	int			prev_mode;

	unsigned char *d_16to8table;

	int			lightmap_textures;

	int			currenttextures[3];
	int			currenttmu;

	float		camera_separation;
	qboolean	stereo_enabled;

	qboolean	alpha_test;
	qboolean	blend;
	qboolean	texgen;
	qboolean	fragment_program; 
	qboolean	glsl_shaders;
	qboolean	separateStencil;
	qboolean	stencil_wrap;
	qboolean	vbo;

} glstate_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;

// vertex arrays

#define MAX_ARRAY MAX_PARTICLES*4

#define VA_SetElem2(v,a,b)		((v)[0]=(a),(v)[1]=(b))
#define VA_SetElem3(v,a,b,c)	((v)[0]=(a),(v)[1]=(b),(v)[2]=(c))
#define VA_SetElem4(v,a,b,c,d)	((v)[0]=(a),(v)[1]=(b),(v)[2]=(c),(v)[3]=(d))

extern float	tex_array[MAX_ARRAY][2];
extern float	vert_array[MAX_ARRAY][3];
extern float	col_array[MAX_ARRAY][4];
extern float	norm_array[MAX_ARRAY][3];

#define MAX_VARRAY_VERTS MAX_VERTS + 2
#define MAX_VARRAY_VERTEX_SIZE 11

#define MAX_VERTICES		16384
#define MAX_INDICES		MAX_VERTICES * 4
#define MAX_SHADOW_VERTS	16384

extern float VArrayVerts[MAX_VARRAY_VERTS * MAX_VARRAY_VERTEX_SIZE];
extern int VertexSizes[];
extern float *VArray;
static vec3_t NormalsArray[MAX_TRIANGLES*3];
extern vec3_t ShadowArray[MAX_SHADOW_VERTS];

// define our vertex types
#define VERT_SINGLE_TEXTURED			0		// verts and st for 1 tmu
#define VERT_BUMPMAPPED					1		// verts and st for 1 tmu, but with 2 texcoord pointers
#define VERT_MULTI_TEXTURED				2		// verts and st for 2 tmus
#define VERT_COLOURED_UNTEXTURED		3		// verts and colour
#define VERT_COLOURED_TEXTURED			4		// verts, st for 1 tmu and colour
#define VERT_COLOURED_MULTI_TEXTURED	5		// verts, st for 2 tmus and colour
#define VERT_DUAL_TEXTURED				6		// verts, st for 2 tmus both with same st
#define VERT_NO_TEXTURE					7		// verts only, no textures
#define VERT_BUMPMAPPED_COLOURED		8		// verts and st for 1 tmu, 2 texoord pointers and colour
#define VERT_NORMAL_COLOURED_TEXTURED	9		// verts and st for 1tmu and color, with normals

// vertex array kill flags
#define KILL_TMU0_POINTER	1
#define KILL_TMU1_POINTER	2
#define KILL_TMU2_POINTER	3
#define KILL_TMU3_POINTER	4
#define KILL_RGBA_POINTER	5
#define KILL_NORMAL_POINTER 6

// vertex array subsystem
void R_InitVArrays (int varraytype);
void R_KillVArrays (void);
void R_InitQuadVarrays(void);
void R_AddTexturedSurfToVArray (msurface_t *surf, float scroll);
void R_AddLightMappedSurfToVArray (msurface_t *surf, float scroll);
void R_AddGLSLShadedSurfToVArray (msurface_t *surf, float scroll, qboolean lightmap);
void R_AddGLSLShadedWarpSurfToVArray (msurface_t *surf, float scroll);
void R_KillNormalTMUs(void);

// stencil volumes
extern glStencilFuncSeparatePROC	qglStencilFuncSeparate;
extern glStencilOpSeparatePROC		qglStencilOpSeparate;
extern glStencilMaskSeparatePROC	qglStencilMaskSeparate;

//arb fragment
extern unsigned int g_water_program_id;

//glsl
extern GLhandleARB	g_programObj;
extern GLhandleARB	g_waterprogramObj;
extern GLhandleARB	g_meshprogramObj;
extern GLhandleARB	g_fbprogramObj;

extern GLhandleARB	g_vertexShader;
extern GLhandleARB	g_fragmentShader;

//standard bsp surfaces
extern GLuint		g_location_testTexture;
extern GLuint		g_location_eyePos;
extern GLuint		g_tangentSpaceTransform;
extern GLuint		g_location_heightTexture;
extern GLuint		g_location_lmTexture;
extern GLuint		g_location_normalTexture;
extern GLuint		g_location_bspShadowmapTexture;
extern GLuint		g_heightMapID;
extern GLuint		g_location_fog;
extern GLuint		g_location_parallax;
extern GLuint		g_location_dynamic;
extern GLuint		g_location_specular;
extern GLuint		g_location_lightPosition;
extern GLuint		g_location_lightColour;
extern GLuint		g_location_lightCutoffSquared;

//water
extern GLuint		g_location_baseTexture;
extern GLuint		g_location_normTexture;
extern GLuint		g_location_refTexture;
extern GLuint		g_location_tangent;
extern GLuint		g_location_time;
extern GLuint		g_location_lightPos;
extern GLuint		g_location_reflect;
extern GLuint		g_location_trans;
extern GLuint		g_location_fogamount;

//mesh
extern GLuint		g_location_meshlightPosition;
extern GLuint		g_location_meshnormal;
extern GLuint		g_location_baseTex;
extern GLuint		g_location_normTex;
extern GLuint		g_location_fxTex;
extern GLuint		g_location_color;
extern GLuint		g_location_meshNormal;
extern GLuint		g_location_meshTangent;
extern GLuint		g_location_meshTime;
extern GLuint		g_location_meshFog;
extern GLuint		g_location_useFX;
extern GLuint		g_location_useGlow;

//fullscreen
extern GLuint		g_location_framebuffTex;
extern GLuint		g_location_distortTex;
extern GLuint		g_location_frametime;
extern GLuint		g_location_fxType;
extern GLuint		g_location_fxPos;
extern GLuint		g_location_fxColor;
extern GLuint		g_location_fbSampleSize;

#define TURBSCALE2 (256.0 / (2 * M_PI)) 

// reduce runtime calcs
#define TURBSCALE 40.743665431525205956834243423364

extern float r_turbsin[];
/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( float camera_separation );
void		GLimp_EndFrame( void );
int 		GLimp_Init( void *hinstance, void *hWnd );
void		GLimp_Shutdown( void );
int     	GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen );
void		GLimp_AppActivate( qboolean active );
void		GLimp_EnableLogging( qboolean enable );
void		GLimp_LogNewFrame( void );

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

#include "r_script.h"
