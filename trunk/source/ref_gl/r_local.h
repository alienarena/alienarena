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

#if defined WIN32_VARIANT
#  include <windows.h>
#endif

#include <stdio.h>

#include <GL/gl.h>
#include <math.h>
#include "glext.h"

#ifndef GL_COLOR_INDEX8_EXT
#define GL_COLOR_INDEX8_EXT GL_COLOR_INDEX
#endif

#include "client/ref.h"
#include "client/vid.h"

#include "qgl.h"
#include "r_math.h"

#if !defined min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

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

#define	MAX_LBM_HEIGHT		1024

#define BACKFACE_EPSILON	0.01

extern	entity_t	*currententity;
extern	model_t		*currentmodel;
extern	int			r_visframecount;
extern	int			r_framecount;
extern  int			r_shadowmapcount;	
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;
extern 	int c_flares;
extern  int c_grasses;
extern	int c_beams;

extern	int			gl_filter_min, gl_filter_max;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;
mleaf_t			*r_viewleaf, *r_viewleaf2;

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_lerpmodels;

extern	cvar_t	*r_lightlevel;	// FIXME: This is a HACK to get the client's light level
extern  cvar_t  *r_wave; //water waves

extern cvar_t	*gl_ext_swapinterval;
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
extern	cvar_t	*gl_dynamic;
extern	cvar_t	*gl_nobind;
extern	cvar_t	*gl_picmip;
extern	cvar_t	*gl_skymip;
extern	cvar_t	*gl_showtris;
extern	cvar_t	*gl_showpolys;
extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_cull;
extern	cvar_t	*gl_poly;
extern	cvar_t	*gl_polyblend;
extern	cvar_t	*gl_modulate;
extern	cvar_t	*gl_drawbuffer;
extern	cvar_t	*gl_driver;
extern	cvar_t	*gl_swapinterval;
extern	cvar_t	*gl_texturemode;
extern	cvar_t	*gl_texturealphamode;
extern	cvar_t	*gl_texturesolidmode;
extern	cvar_t	*gl_lockpvs;
extern	cvar_t	*gl_vlights;
extern  cvar_t	*gl_usevbo;

extern	cvar_t	*vid_fullscreen;
extern	cvar_t	*vid_gamma;
extern  cvar_t	*vid_contrast;

extern cvar_t *r_anisotropic;
extern cvar_t *r_alphamasked_anisotropic;
extern cvar_t *r_ext_max_anisotropy;

extern cvar_t	*r_overbrightbits;

extern cvar_t	*gl_normalmaps;
extern cvar_t	*gl_bspnormalmaps;
extern cvar_t	*gl_shadowmaps;
extern cvar_t	*gl_fog;

extern	cvar_t	*r_shaders;
extern	cvar_t	*r_bloom;
extern	cvar_t	*r_lensflare;
extern	cvar_t	*r_lensflare_intens;
extern	cvar_t	*r_drawsun;
extern  cvar_t	*r_godrays;
extern  cvar_t	*r_godray_intensity;
extern	cvar_t	*r_optimize;

extern	cvar_t	*r_lightmapfiles;

extern  cvar_t	*r_ragdolls;
extern  cvar_t  *r_ragdoll_debug;

extern	qboolean	map_fog;
#if defined WIN32_VARIANT
extern	char		map_music[MAX_PATH];
#else
extern	char		map_music[MAX_OSPATH];
#endif
extern  unsigned	r_weather;
extern unsigned		r_nosun;
extern float		r_sunX;
extern float		r_sunY;
extern float		r_sunZ;

extern  cvar_t		*r_minimap;
extern  cvar_t		*r_minimap_size;
extern  cvar_t		*r_minimap_zoom;
extern  cvar_t		*r_minimap_style;

extern	cvar_t	*gl_mirror;

extern	cvar_t	*sys_affinity;
extern	cvar_t	*sys_priority;

extern	cvar_t	*gl_screenshot_type;
extern	cvar_t	*gl_screenshot_jpeg_quality;

extern  cvar_t  *r_test;

extern	int		gl_lightmap_format;
extern	int		gl_solid_format;
extern	int		gl_alpha_format;
extern	int		gl_tex_solid_format;
extern	int		gl_tex_alpha_format;

extern	int		c_visible_lightmaps;
extern	int		c_visible_textures;

extern	float	r_world_matrix[16];
extern	float	r_project_matrix[16];
extern	int		r_viewport[4];
extern	float	r_farclip, r_farclip_min, r_farclip_bias;

extern	int		r_origin_leafnum;

//Image
void R_InitImageSubsystem(void);
void GL_Bind (int texnum);
void GL_MBind (int target, int texnum);
void GL_TexEnv (GLenum value);
void GL_EnableTexture (int target, qboolean enable);
void GL_EnableMultitexture (qboolean enable);
void GL_SelectTexture (int target);
void GL_InvalidateTextureState (void);
void RefreshFont (void);

extern void vectoangles (vec3_t value1, vec3_t angles);

// dynamic lights
void R_StaticLightPoint (vec3_t p, vec3_t color);
void R_DynamicLightPoint (vec3_t p, vec3_t color);
void R_LightPoint (vec3_t p, vec3_t color, qboolean addDynamic);
void R_PushDlights (void);
void R_PushDlightsForBModel (entity_t *e);
void SetVertexOverbrights (qboolean toggle);

//====================================================================
extern	model_t	*r_worldmodel;

extern	unsigned	d_8to24table[256];

extern	int		registration_sequence;

extern void V_AddBlend (float r, float g, float b, float a, float *v_blend);

// "fake" terrain entities parsed directly out of the BSP on the client side.
int			num_terrain_entities;
entity_t	terrain_entities[MAX_MAP_MODELS];

//Renderer main loop
extern int	R_Init( void *hinstance, void *hWnd );
extern void R_Shutdown( void );

extern void R_SetupViewport (void);
extern void R_RenderView (refdef_t *fd);
extern void GL_ScreenShot_f (void);
extern void R_Mesh_Draw (void);
extern void R_DrawBrushModel (void);
extern void R_DrawWorldSurfs (void);
extern void R_RenderDlights (void);
extern void R_DrawAlphaSurfaces (void);
extern void R_InitParticleTexture (void);
extern void R_DrawParticles (void);
extern void R_DrawRadar(void);
extern void R_DrawVehicleHUD (void);
extern void R_ClearSkyBox (void);
extern void R_DrawSkyBox (void);
extern void Draw_InitLocal (void);

//Renderer utils
extern void R_SubdivideSurface (msurface_t *fa, int firstedge, int numedges);
extern qboolean R_CullBox (vec3_t mins, vec3_t maxs);
extern qboolean R_CullOrigin(vec3_t origin);
extern qboolean R_CullSphere( const vec3_t centre, const float radius, const int clipflags );
void R_SetLightingMode (qboolean environment_color);
extern void R_RotateForEntity (entity_t *e);
extern void R_MarkWorldSurfs (void);
extern void R_AddSkySurface (msurface_t *fa);
extern void R_RenderWaterPolys (msurface_t *fa, int texnum, float scaleX, float scaleY);
extern void R_ReadFogScript(char config_file[128]);
extern void R_ReadMusicScript(char config_file[128]);

//Lights
extern dlight_t *dynLight;
extern vec3_t lightspot;
extern void R_MarkLights (dlight_t *light, int bit, mnode_t *node);
extern void  VLight_Init (void);
extern float VLight_GetLightValue ( vec3_t normal, vec3_t dir, float apitch, float ayaw );

//BSP 
extern GLuint normalisationCubeMap;
extern image_t *r_droplets;
extern image_t *r_droplets_nm;
extern image_t *r_blooddroplets;
extern image_t *r_blooddroplets_nm;
extern void BSP_DrawTexturelessPoly (msurface_t *fa);
extern void BSP_DrawTexturelessBrushModel (entity_t *e);

//Postprocess
void R_GLSLPostProcess(void);
void R_FB_InitTextures(void);

//VBO
extern qboolean use_vbo;
extern GLuint vboId;
extern GLuint eboId;
extern int totalVBOsize;
extern int	vboPosition;
extern void R_LoadVBOSubsystem(void);
extern void R_VCShutdown(void);
extern void VB_WorldVCInit(void);
extern void VB_BuildVBOBufferSize(msurface_t *surf);
extern void VB_BuildWorldVBO(void);
void GL_SetupWorldVBO (void);
void GL_BindVBO(vertCache_t *cache);
void GL_BindIBO(vertCache_t *cache);
vertCache_t *R_VCFindCache(vertStoreMode_t store, model_t *mod);
vertCache_t *R_VCLoadData(vertCacheMode_t mode, int size, void *buffer, vertStoreMode_t store, model_t *mod);
extern void R_VCFree(vertCache_t *cache);

//Light Bloom
extern void R_BloomBlend( refdef_t *fd );
extern void R_InitBloomTextures( void );

//Flares and Sun
int r_numflares;
extern int c_flares;
flare_t r_flares[MAX_FLARES];
extern void Mod_AddFlareSurface (msurface_t *surf, int type );
extern void R_RenderFlares (void);
extern void R_ClearFlares(void);

vec3_t sun_origin;
qboolean spacebox;
qboolean draw_sun;
float sun_x;
float sun_y;
float sun_size;
extern void R_InitSun();
extern void R_RenderSun();

//Vegetation
int r_numgrasses;
extern int c_grasses;
grass_t r_grasses[MAX_GRASSES];
qboolean r_hasleaves;
extern void Mod_AddVegetationSurface (msurface_t *surf, int texnum, vec3_t color, float size, char name[MAX_QPATH], int type);
extern void R_DrawVegetationSurface (void);
extern void R_ClearGrasses(void);
extern void R_FinalizeGrass(model_t *mod); 

//Simple Items
extern void R_SI_InitTextures( void );
extern void R_DrawSimpleItems( void );
void R_SetSimpleTexnum (model_t *loadmodel, const char *pathname);

//Light beams/volumes
int r_numbeams;
extern int c_beams;
beam_t r_beams[MAX_BEAMS];
extern void Mod_AddBeamSurface (msurface_t *surf, int texnum, vec3_t color, float size, char name[MAX_QPATH], int type, float xang, float yang,
	qboolean rotating);
extern void R_DrawBeamSurface ( void );
extern void R_ClearBeams(void);

//Player icons
extern float	scr_playericonalpha;

//Team colors
int r_teamColor;
qboolean r_gotFlag;
qboolean r_lostFlag;

extern void	Draw_GetPicSize (int *w, int *h, const char *name);
extern void	Draw_Pic (float x, float y, const char *name);
extern void	Draw_ScaledPic (float x, float y, float scale, const char *pic);
extern void	Draw_StretchPic (float x, float y, float w, float h, const char *name);
extern void	Draw_FadeScreen (void);

extern void	R_BeginFrame( float camera_separation );
extern void	R_SwapBuffers( int );
extern void	R_SetPalette ( const unsigned char *palette);
extern int	Draw_GetPalette (void);

image_t *R_RegisterSkin (char *name);
image_t *R_RegisterParticlePic(const char *name);
image_t *R_RegisterParticleNormal(const char *name);
image_t *R_RegisterGfxPic(const char *name);

extern void	LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
extern image_t *GL_FindFreeImage (char *name, int width, int height, imagetype_t type);
extern image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits);
extern image_t *GL_GetImage( const char * name );
extern image_t	*GL_FindImage (char *name, imagetype_t type);
extern void	GL_FreeImage( image_t * image );
extern void	GL_TextureMode( char *string );
extern void	GL_ImageList_f (void);

extern void	GL_InitImages (void);
extern void	GL_ShutdownImages (void);

extern void	GL_FreeUnusedImages (void);

extern void	GL_TextureAlphaMode( char *string );
extern void	GL_TextureSolidMode( char *string );

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
} glconfig_t;

typedef struct
{
	 float       inverse_intensity;
    qboolean    fullscreen;

    int         prev_mode;

    unsigned char *d_16to8table;

    int         lightmap_textures;

#define MAX_TMUS 8
    int         currenttextures[MAX_TMUS];
    int			currenttexturemodes[MAX_TMUS];
    qboolean	enabledtmus[MAX_TMUS];
    int         currenttmu;
    qboolean	tmuswitch_done;
    qboolean    currenttmu_defined;

    float       camera_separation;
    qboolean    stereo_enabled;

    qboolean    alpha_test;
    qboolean    blend;
    qboolean    texgen;
    qboolean    separateStencil;
    qboolean    stencil_wrap;
    qboolean    fbo;
    qboolean    hasFBOblit;

	qboolean	ati;

} glstate_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;

// vertex arrays
extern int KillFlags;

#define MAX_ARRAY (MAX_PARTICLES*4)

#define VA_SetElem2(v,a,b)		((v)[0]=(a),(v)[1]=(b))
#define VA_SetElem3(v,a,b,c)	((v)[0]=(a),(v)[1]=(b),(v)[2]=(c))
#define VA_SetElem4(v,a,b,c,d)	((v)[0]=(a),(v)[1]=(b),(v)[2]=(c),(v)[3]=(d))

extern float	tex_array[MAX_ARRAY][2];
extern float	vert_array[MAX_ARRAY][3];
extern float	norm_array[MAX_ARRAY][3];
extern float	tan_array[MAX_ARRAY][4];
extern float	col_array[MAX_ARRAY][4];

#define MAX_VARRAY_VERTS (MAX_VERTS + 2)
#define MAX_VARRAY_VERTEX_SIZE 11

#define MAX_VERTICES 16384
#define MAX_INDICES	(MAX_VERTICES * 4)
#define MAX_SHADOW_VERTS 16384

extern float VArrayVerts[MAX_VARRAY_VERTS * MAX_VARRAY_VERTEX_SIZE];
extern int VertexSizes[];
extern float *VArray;
extern vec3_t ShadowArray[MAX_SHADOW_VERTS];

// define our vertex types
#define VERT_SINGLE_TEXTURED			0		// verts and st for 1 tmu
#define VERT_MULTI_TEXTURED				2		// verts and st for 2 tmus
#define VERT_COLOURED_TEXTURED			4		// verts, st for 1 tmu and colour
#define VERT_NO_TEXTURE					7		// verts only, no textures

// looks like these should be bit flags (2010-08)
// apparently not - this was not working as bit flags, works fine as ints
#define KILL_TMU0_POINTER	1
#define KILL_TMU1_POINTER	2
#define KILL_TMU2_POINTER	3
#define KILL_TMU3_POINTER	4
#define KILL_TMU4_POINTER   5
#define KILL_TMU5_POINTER	6
#define KILL_RGBA_POINTER	7
#define KILL_NORMAL_POINTER 8

// vertex array subsystem
void R_InitVArrays (int varraytype);
void R_KillVArrays (void);
void R_DrawVarrays(GLenum mode, GLint first, GLsizei count);
void R_InitQuadVarrays(void);
void R_AddSurfToVArray (msurface_t *surf);
void R_AddShadowSurfToVArray (msurface_t *surf, vec3_t origin);
void R_AddTexturedSurfToVArray (msurface_t *surf);
void R_AddGLSLShadedWarpSurfToVArray (msurface_t *surf, float scroll);
void R_KillNormalTMUs(void);

//shadows
extern  void R_InitShadowSubsystem(void);
extern  void R_CastShadow(void);
extern	cvar_t		*r_shadowmapscale;
extern  int			r_lightgroups;
extern  image_t		*r_depthtexture;
extern	image_t		*r_depthtexture2;
extern  image_t		*r_colorbuffer;
extern GLuint   fboId[3];
extern GLuint	rboId;
extern vec3_t	r_worldLightVec;
typedef struct	LightGroup 
{
	vec3_t	group_origin;
	vec3_t	accum_origin;
	float	avg_intensity;
	float	dist;
} LightGroup_t;
extern			LightGroup_t LightGroups[MAX_LIGHTS];

extern void		R_CheckFBOExtensions (void);
extern void		R_GenerateShadowFBO(void);
extern void		R_Mesh_DrawCaster (void);
extern void		IQM_DrawRagDollCaster (int);
extern void		R_DrawDynamicCaster(void);
extern void		R_DrawVegetationCaster(void);
extern void		R_DrawEntityCaster(entity_t *ent);
extern void		R_GenerateEntityShadow( void );
extern void		R_GenerateRagdollShadow( int RagDollID );
extern void		BSP_DrawShadowPoly (msurface_t *fa, vec3_t origin);
extern void		R_DrawShadowMapWorld(qboolean forEnt, vec3_t origin);
int				FB_texture_width, FB_texture_height;
float			fadeShadow;
cvar_t			*r_shadowcutoff;

//shader programs
extern void	R_LoadGLSLPrograms(void);

//glsl
GLhandleARB g_programObj;
GLhandleARB g_shadowprogramObj;
GLhandleARB g_waterprogramObj;
GLhandleARB g_meshprogramObj;
GLhandleARB g_vertexonlymeshprogramObj;
GLhandleARB g_glassprogramObj;
GLhandleARB g_blankmeshprogramObj;
GLhandleARB g_fbprogramObj;
GLhandleARB g_blurprogramObj;
GLhandleARB g_rblurprogramObj;
GLhandleARB g_dropletsprogramObj;
GLhandleARB g_godraysprogramObj;

GLhandleARB g_vertexShader;
GLhandleARB g_fragmentShader;

//vertex attribute indexes
//blame NVIDIA for this idiocy:
// http://stackoverflow.com/questions/528028/glvertexattrib-which-attribute-indices-are-predefined
#define ATTR_TANGENT_IDX	1
#define ATTR_WEIGHTS_IDX	11
#define ATTR_BONES_IDX 		12
#define ATTR_OLDVTX_IDX		13
#define ATTR_OLDNORM_IDX	14
#define ATTR_OLDTAN_IDX		15

//standard bsp surfaces
GLuint		g_location_surfTexture;
GLuint		g_location_eyePos;
GLuint		g_tangentSpaceTransform;
GLuint		g_location_heightTexture;
GLuint		g_location_lmTexture;
GLuint		g_location_normalTexture;
GLuint		g_location_bspShadowmapTexture;
GLuint		g_location_bspShadowmapTexture2;
GLuint		g_location_fog;
GLuint		g_location_parallax;
GLuint		g_location_dynamic;
GLuint		g_location_shadowmap;
GLuint		g_Location_statshadow;
GLuint		g_location_xOffs;
GLuint		g_location_yOffs;
GLuint		g_location_lightPosition;
GLuint		g_location_staticLightPosition;
GLuint		g_location_lightColour;
GLuint		g_location_lightCutoffSquared;
GLuint		g_location_liquid;
GLuint		g_location_shiny;
GLuint		g_location_rsTime;
GLuint		g_location_liquidTexture;
GLuint		g_location_liquidNormTex;
GLuint		g_location_chromeTex;

//shadow on white bsp polys
GLuint		g_location_entShadow;
GLuint		g_location_fadeShadow;
GLuint		g_location_xOffset;
GLuint		g_location_yOffset;

//water
GLuint		g_location_baseTexture;
GLuint		g_location_normTexture;
GLuint		g_location_refTexture;
GLuint		g_location_waterEyePos;
GLuint		g_location_tangentSpaceTransform;
GLuint		g_location_time;
GLuint		g_location_lightPos;
GLuint		g_location_reflect;
GLuint		g_location_trans;
GLuint		g_location_fogamount;

#define MESH_UNIFORM(name) \
	((fragmentshader)?g_location_##name:g_location_vo_##name)

//mesh
GLuint		g_location_meshlightPosition;
GLuint		g_location_baseTex;
GLuint		g_location_normTex;
GLuint		g_location_fxTex;
GLuint		g_location_fx2Tex;
GLuint		g_location_color;
GLuint		g_location_meshTime;
GLuint		g_location_meshFog;
GLuint		g_location_useFX;
GLuint		g_location_useGlow;
GLuint		g_location_useShell;
GLuint		g_location_useCube;
GLuint		g_location_useGPUanim;
GLuint		g_location_outframe;
GLuint		g_location_fromView;
GLuint		g_location_lerp;

//vertex-only mesh
GLuint		g_location_vo_meshlightPosition;
GLuint		g_location_vo_baseTex;
GLuint		g_location_vo_normTex;
GLuint		g_location_vo_fxTex;
GLuint		g_location_vo_fx2Tex;
GLuint		g_location_vo_color;
GLuint		g_location_vo_meshTime;
GLuint		g_location_vo_meshFog;
GLuint		g_location_vo_useFX;
GLuint		g_location_vo_useGlow;
GLuint		g_location_vo_useShell;
GLuint		g_location_vo_useCube;
GLuint		g_location_vo_useGPUanim;
GLuint		g_location_vo_outframe;
GLuint		g_location_vo_fromView;
GLuint		g_location_vo_lerp;

//glass
GLuint		g_location_g_type; // 1 means mirror only, 2 means glass only, 3 means both
GLuint		g_location_g_left;
GLuint		g_location_g_up;
GLuint		g_location_g_mirTexture;
GLuint		g_location_g_refTexture;
GLuint		g_location_g_lightPos;
GLuint		g_location_g_fog;
GLuint		g_location_g_outframe;
GLuint		g_location_g_useGPUanim;
GLuint		g_location_g_lerp;

//blank mesh
GLuint		g_location_bm_outframe;
GLuint		g_location_bm_useGPUanim;
GLuint		g_location_bm_lerp;

//fullscreen distortion effects
GLuint		g_location_framebuffTex;
GLuint		g_location_distortTex;
GLuint		g_location_dParams;
GLuint		g_location_fxPos;

//gaussian blur
GLuint		g_location_scale;
GLuint		g_location_source;

//radial blur	
GLuint		g_location_rscale;
GLuint		g_location_rsource;
GLuint		g_location_rparams;

//water droplets
GLuint		g_location_drSource;
GLuint		g_location_drTex;
GLuint		g_location_drTime;
GLuint		g_location_drParams;

//god rays
GLuint		g_location_lightPositionOnScreen;
GLuint		g_location_sunTex;
GLuint		g_location_godrayScreenAspect;
GLuint		g_location_sunRadius;

//Shared mesh items
extern vec3_t	shadelight;
extern vec3_t	shadevector;
extern int		model_dlights_num;
extern m_dlight_t model_dlights[128];
extern image_t	*r_mirrortexture;
extern cvar_t	*cl_gun;
vec3_t lightPosition;
vec3_t statLightPosition;
float statLightIntensity;
float  dynFactor;
extern void	R_GetLightVals(vec3_t origin, qboolean RagDoll);
extern void R_ModelViewTransform(const vec3_t in, vec3_t out);
extern void GL_BlendFunction (GLenum sfactor, GLenum dfactor);
extern void R_Mesh_DrawFrame (int skinnum, qboolean ragdoll, float shellAlpha);
qboolean R_Mesh_CullBBox (vec3_t bbox[8]);

//iqm
#define pi 3.14159265
extern float modelpitch;
extern double degreeToRadian(double degree);
extern qboolean Mod_INTERQUAKEMODEL_Load(model_t *mod, void *buffer);
extern qboolean IQM_ReadSkinFile(char skin_file[MAX_OSPATH], char *skinpath);
extern void R_DrawINTERQUAKEMODEL(void);
extern qboolean IQM_InAnimGroup(int frame, int oldframe);
extern int IQM_NextFrame(int frame);
extern void IQM_AnimateRagdoll(int RagDollID);
extern void IQM_DrawFrame(int skinnum, qboolean ragdoll, float shellAlpha);
extern void IQM_DrawShadow(vec3_t origin);
extern qboolean IQM_CullModel( void );
extern void IQM_AnimateFrame (void);

//md2
extern qboolean MD2_CullModel( void );
extern void Mod_LoadMD2Model (model_t *mod, void *buffer);

//terrain
void Mod_LoadTerrainModel (model_t *mod, void *_buf);

//Ragdoll
int r_DrawingRagDoll;
int r_SurfaceCount;

#define TURBSCALE2 (256.0 / (2 * M_PI))

// reduce runtime calcs
#define TURBSCALE 40.743665431525205956834243423364

extern float r_turbsin[];
/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

extern void		GLimp_BeginFrame( float camera_separation );
extern void		GLimp_EndFrame( void );
extern qboolean	GLimp_Init( void *hinstance, void *hWnd );
extern void		GLimp_Shutdown( void );
extern rserr_t    	GLimp_SetMode( unsigned *pwidth, unsigned *pheight, int mode, qboolean fullscreen );
extern void		GLimp_AppActivate( qboolean active );
extern void		GLimp_EnableLogging( qboolean enable );
extern void		GLimp_LogNewFrame( void );

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

#include "r_script.h"
