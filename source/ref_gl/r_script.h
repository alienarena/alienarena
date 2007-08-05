#ifndef __GL_SCRIPT__
#define __GL_SCRIPT__

//CVARS
extern cvar_t *rs_dynamic_time;

#define ALPHAFUNC_BASIC		1
#define ALPHAFUNC_GLOSS		2
#define ALPHAFUNC_NORMAL	3

// Animation loop
typedef struct anim_stage_s 
{
	image_t					*texture;	// texture
	char					name[MAX_OSPATH];	// texture name
	struct anim_stage_s		*next;		// next anim stage
} anim_stage_t;


typedef struct random_stage_s 
{
	image_t					*texture;	// texture
	char					name[MAX_OSPATH];	// texture name
	struct random_stage_s	*next;		// next anim stage
} random_stage_t;

// Blending
typedef struct 
{
	int			source;		// source blend value
	int			dest;		// dest blend value
	qboolean	blend;		// are we going to blend?
} blendfunc_t;

// Alpha shifting
typedef struct {
	float		min, max;	// min/max alpha values
	float		speed;		// shifting speed
} alphashift_t;

// scaling
typedef struct
{
	char	typeX, typeY;	// scale types
	float	scaleX, scaleY;	// scaling factors
} rs_scale_t;

// scrolling
typedef struct
{
	char	typeX, typeY;	// scroll types
	float	speedX, speedY;	// speed of scroll
} rs_scroll_t;

// colormap
typedef struct
{
	qboolean enabled;
	int	red, green, blue;	// colors - duh!
} rs_colormap_t;

typedef struct
{
	qboolean enabled;
	float speed;
	int	start, end;
} rs_frames_t;

// Script stage
typedef struct rs_stage_s 
{
	image_t					*texture;				// texture
	char					name[MAX_OSPATH];		// tex name

	char					model[MAX_OSPATH];		// name of model
	
	anim_stage_t			*anim_stage;	// first animation stage
	float					anim_delay;		// Delay between anim frames
	float					last_anim_time; // gametime of last frame change
	char					anim_count;		// number of animation frames
	anim_stage_t			*last_anim;		// pointer to last anim

	random_stage_t			*rand_stage;	// random linked list
	int						rand_count;		// number of random pics

	rs_colormap_t			colormap;		// color mapping
	blendfunc_t				blendfunc;		// image blending
	alphashift_t			alphashift;		// alpha shifting
	rs_scroll_t				scroll;			// tcmod
	rs_scale_t				scale;			// tcmod
	
	float					rot_speed;		// rotate speed (0 for no rotate);
	float					depthhack;		// fake zdepth

	vec3_t					origin;			//origin for models;
	vec3_t					angle;			//angles for models;
	rs_frames_t				frames;			//frames

	qboolean				dynamic;		// dynamic texture

	qboolean				envmap;			// fake envmapping - spheremapping
	qboolean				lightmap;		// lightmap this stage?
	qboolean				alphamask;		// alpha masking?

	int						alphafunc;		// software alpha effects

	qboolean				has_alpha;		// for sorting
	qboolean				lensflare;		// for adding lensflares
	qboolean				normalmap;		// for normalmaps

	struct rs_stage_s		*next;			// next stage
} rs_stage_t;

typedef struct rs_stagekey_s
{
	char *stage;
	void (*func)(rs_stage_t *shader, char **token);
} rs_stagekey_t;

typedef struct
{
	qboolean enable;
	int width;
	int height;
} rs_picsize_t;

// Base script
typedef struct rscript_s 
{
	char					name[MAX_OSPATH];	// name of script
	
	unsigned char			subdivide;	// Heffo - chop the surface up this much for vertex warping
	float					warpdist;	// Heffo - vertex warping distance;
	float					warpsmooth;	// Heffo - vertex warping smoothness;
	float					warpspeed;	// Heffo - vertex warping speed;

	qboolean				mirror;		// mirror
	qboolean				model;		// model hack

	rs_picsize_t			picsize;	//force pic size for hud etc

	qboolean				dontflush;	// dont flush from memory on map change
	qboolean				ready;		// readied by the engine?
	rs_stage_t				*stage;		// first rendering stage
	struct rscript_s		*next;		// next script in linked list

} rscript_t;

typedef struct rs_scriptkey_s
{
	char *script;
	void (*func)(rscript_t *rs, char **token);
} rs_scriptkey_t;

void RS_SetEnvmap (vec3_t v, float *os, float *ot);
void RS_LoadScript(char *script);
void RS_FreeAllScripts(void);
void RS_ReloadImageScriptLinks (void);
void RS_FreeScript(rscript_t *rs);
void RS_FreeUnmarked(void);
rscript_t *RS_FindScript(char *name);
void RS_ReadyScript(rscript_t *rs);
#ifdef _WINDOWS
void RS_ScanPathForScripts(void);
#endif
int RS_Animate(rs_stage_t *stage);
void RS_UpdateRegistration(void);
void RS_DrawSurface (msurface_t *surf, qboolean lightmap);
void RS_SetTexcoords (rs_stage_t *stage, float *os, float *ot, msurface_t *fa);
void RS_SetTexcoords2D (rs_stage_t *stage, float *os, float *ot);
void RS_SpecialSurface (msurface_t *surf);
void RS_LoadSpecialScripts(void);
float RS_AlphaFuncAlias (int alphafunc, float alpha, vec3_t normal, vec3_t org);

#define RS_DrawPolyNoLightMap(surf)	RS_DrawSurface((surf),false)

extern float rs_realtime;

#endif // __GL_SCRIPT__
