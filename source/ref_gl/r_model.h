/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2011 COR Entertainment, LLC.

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

d*_t structures are on-disk representations
m*_t structures are in-memory



*/
#include "r_iqm.h"
/*
==============================================================================

WORLD LIGHTS

==============================================================================
*/

#define MAX_LIGHTS 4096

typedef struct
{
	vec3_t	origin;
	float	intensity;
	void	*surf;
	qboolean grouped;
} worldLight_t;

worldLight_t r_worldLights[MAX_LIGHTS];
int r_numWorldLights;

/*
==============================================================================

SUN LIGHTS

==============================================================================
*/

typedef struct
{
	char targetname[128];
	vec3_t	origin;
	vec3_t	target;
	qboolean has_Sun;
} sunLight_t;

sunLight_t *r_sunLight;

/*
==============================================================================

MODEL PRECACHING

==============================================================================
*/

//base player models and weapons for prechache
typedef struct PModelList_s {

    char *name;

} PModelList_t;

extern PModelList_t BasePModels[];
extern int PModelsCount;

typedef struct WModelList_s {

    char *name;

} WModelList_t;

extern WModelList_t BaseWModels[];
extern int WModelsCount;

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
typedef struct	mvertex_s
{
	vec3_t		position;
} mvertex_t;

typedef struct
{
	vec3_t		dir;
} mnormal_t;

typedef struct
{
	vec4_t		dir;
} mtangent_t;

typedef struct
{
	vec3_t		mins, maxs;
	float		radius;
	int			headnode;
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
} mmodel_t;


#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

//Internal surface flags, applied by the engine
#define	ISURF_PLANEBACK		0x1
#define ISURF_DRAWTURB		0x2
// same effect as SURF_UNDERWATER, but has to be separate because
// SURF_UNDERWATER applies to texinfos, whereas we have to work with
// individual surfaces.
#define ISURF_UNDERWATER	0x4 

#define TexinfoIsTranslucent(texinfo) ((texinfo)->flags & (SURF_TRANS33|SURF_TRANS66))
#define SurfaceIsTranslucent(surf) (TexinfoIsTranslucent ((surf)->texinfo))
#define TexinfoIsAlphaMasked(texinfo) ((texinfo)->flags & SURF_TRANS33 && (texinfo)->flags & SURF_TRANS66)
#define SurfaceIsAlphaMasked(surf) (TexinfoIsAlphaMasked ((surf)->texinfo))
#define SurfaceHasNoLightmap(surf) ((surf)->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP) && !SurfaceIsAlphaMasked (surf))

typedef struct
{
	// Data included in the BSP file.
	unsigned short	v[2];
	
	// Used at load time to identify "corner" edges (i.e. edges between two 
	// non-coplanar surfaces.) Corner edges are the only edges we draw in the
	// minimap. Unused after load time.
	int				usecount;
	cplane_t		*first_face_plane;
	
	// Calculated at load time, used to render the minimap. 
	vec3_t			mins, maxs;
	qboolean		iscorner;
	float			alpha, sColor;
} medge_t;

/*
==============================================================================

VERTEX ARRAYS

==============================================================================
*/

typedef struct 
{
	//for BSP rendering-- not always cleared each frame
	struct	msurface_s	*worldchain;
	//for entity rendering-- cleared for each brush model drawn
	struct	msurface_s	*entchain;
} surfchain_t;

typedef struct mtexinfo_s
{
	float		vecs[2][4];
	int			flags;
	int			numframes;
	struct mtexinfo_s	*next;		// animation chain
	struct mtexinfo_s	*equiv;		// equivalent texinfo
	image_t		*image;
	image_t		*normalMap;
	image_t		*heightMap;
	qboolean	has_normalmap;
	qboolean	has_heightmap;
	struct		rscript_s	*script;
	int			value;
	
	// Surface linked lists: all surfaces in these lists are grouped together
	// by texinfo so the renderer can take advantage of the fact that they 
	// share the same texture and the same surface flags.
	surfchain_t	glsl_surfaces, dynamic_surfaces, lightmap_surfaces;
	struct msurface_s *rscript_surfaces; // no brush models can have rscript surfs
} mtexinfo_t;

#define	VERTEXSIZE	10

typedef struct glpoly_s
{
	struct	glpoly_s	*next;		// warp surfaces only!
	int		numverts;
	float	verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)

} glpoly_t;

// FIXME: We really need a smaller version of this struct with less data in
// it, for use by BSP_RecursiveWorldNode. This one is 136 bytes, which is huge
// and killing our cache locality.
typedef struct msurface_s
{
	int			visframe;		// should be drawn when node is crossed
	
	int			iflags;			// internal flags, applied by the engine
	
	mtexinfo_t	*texinfo;

	cplane_t	*plane;
	
	//texture chains for batching
	struct	msurface_s	*texturechain;
	struct	msurface_s	*rscriptchain;
	
	//texture chain for lightstyle updating
	struct	msurface_s	*flickerchain;
	
	vec3_t mins;
	vec3_t maxs;

	int			texturemins[2];
	int			extents[2];

	glpoly_t	*polys;				// multiple if warped

	// Texcoords at the center of the face, used for rscript "rotate" keyword.
	float		c_s, c_t;

// lighting info
	int			dlightframe;
	int			dlightbits;
	
	int			lightmins[2];
	int			lightmaxs[2];

	int			lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];
	float		cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	byte		*samples;		// [numstyles*surfsize]
	float		lightmap_xscale, lightmap_yscale;

	entity_t	*entity;

	float		*tangentSpaceTransform;

	//vbo
	int vbo_first_vert;
	int vbo_num_verts;
	
	// XXX: for future reference, the glDrawRangeElements code was last seen
	// here at revision 3246.

} msurface_t;

typedef struct mnode_s
{
// common with leaf
	int			contents;		// -1, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// node specific
	cplane_t	*plane;
	struct mnode_s	*children[2];

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
} mnode_t;



typedef struct mleaf_s
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// leaf specific
	int			cluster;
	int			area;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
	
	int			minPVSleaf, maxPVSleaf;
} mleaf_t;


//===================================================================

//
// Whole model
//

typedef struct {
	int n[3];
} neighbors_t;

typedef struct {

	int hasHelmet;

	float RagDollDims[56];

} mragdoll_t;

typedef enum {mod_bad, mod_brush, mod_md2, mod_iqm, mod_terrain, mod_decal, num_modtypes} modtype_t;

// This array is a look-up table for the traits of various mesh formats.
static struct 
{
	// If true, this model type can cast shadowmaps.
	qboolean castshadowmap;
	
	// Morph target animation is what the MD2 format uses exclusively. Vertex
	// positions, normals, and tangents are stored in VBOs for every frame
	// and interpolated by the vertex shader. It's possible to combine this
	// with skeletal animation, although we don't support any formats that do
	// this yet.
	qboolean morphtarget; 
	
	// If true, the blendweights and blendindexes vertex attributes are used.
	// NOTE: this is still basically IQM-specific, we're not trying to
	// generalize across every possible skeletal format-- yet.
	qboolean skeletal;
	
	// True if the vertex data is indexed. If so, an IBO is used.
	qboolean indexed;
	
	// True if a polygon offset should be used to prevent z-fighting, as well
	// as if blending should be used.
	qboolean decal;
	
	// True if any sort of static per-pixel or per-vertex lighting should be
	// done on this mesh type. (The eventual goal is to do dynamic lighting on
	// all mesh types.)
	qboolean doShading;
} modtypes[num_modtypes] = 
{
	{true,	false,	false,	false,	false,	true},	// mod_bad- this is ignored
	{true,	false,	false,	false,	false,	true},	// mod_brush- BSP brush models- this is ignored
	{false,	true,	false,	false,	false,	true},	// mod_md2- TODO: these should use shadowmaps as well
	{true,	false,	true,	true,	false,	true},	// mod_iqm
	{false,	false,	false,	true,	false,	false},	// mod_terrain
	{false,	false,	false,	true,	true,	false},	// mod_decal
	// New model types go here
};

typedef enum	{	simplecolor_white, simplecolor_green, simplecolor_blue, 
					simplecolor_purple	} simplecolor_t;

typedef struct model_s
{
	char		name[MAX_QPATH];

	int			registration_sequence;

	modtype_t	type;

	int			flags;

//
// volume occupied by the model graphics
//
	vec3_t		mins, maxs;
	float		radius;
	vec3_t		bbox[8];

//
// brush model
//
	int			firstmodelsurface, nummodelsurfaces;

	int			numsubmodels;
	mmodel_t	*submodels;

	int			numplanes;
	cplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	int			firstnode;
	mnode_t		*nodes;

	int			numtexinfo;
	mtexinfo_t	*texinfo;
	int			num_unique_texinfos;
	mtexinfo_t	**unique_texinfo;

	int			numsurfaces;
	msurface_t	*surfaces;
	
	int			numTangentSpaceTransforms;
	float		*tangentSpaceTransforms;

	int			numsurfedges;
	int			*surfedges;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	dvis_t		*vis;

	byte		*lightdata;
	
	//minimum and maximum leaf indexes for each area
	int			area_min_leaf[MAX_MAP_AREAS];
	int			area_max_leaf[MAX_MAP_AREAS];
	int			num_areas;

	// for alias models and skins
	image_t		*skins[MAX_MD2SKINS];

	struct rscript_s	*script;

	int			extradatasize;
	void		*extradata;

	int			num_frames;

	//iqm skeletal model info
	unsigned int	version;
	int				num_joints;
	iqmjoint2_t		*joints; // when loading v1 files, joints are reencoded to v2 format
	matrix3x4_t		*frames;
	matrix3x4_t		*outframe;
	matrix3x4_t		*baseframe;
	int				num_poses;
	unsigned char	*blendindexes;
	unsigned char	*blendweights;
	char			skinname[MAX_QPATH];
	char			*jointname;
	//end iqm

	//md2 and iqm.
	int				num_triangles;
	// TODO: we can remove this when shadow volumes are gone.
	neighbors_t *neighbors;	
	
	// Is just a copy of the pointer to this model_t struct. That way you can
	// make a copy of this model_t and still be able to identify which mesh it
	// is without string comparisons. TODO: get rid of this and stop making 
	// copies!
	size_t			vbo_key; 
	
	//terrain only
	image_t         *lightmap;

	//ragdoll info
	int hasRagDoll;
	mragdoll_t ragdoll;
	
	//simple item texnum
	int					simple_texnum;
	simplecolor_t		simple_color;

} model_t;

//============================================================================

void	Mod_Init (void);
void	Mod_ClearAll (void);
model_t *Mod_ForName (char *name, qboolean crash);
mleaf_t *Mod_PointInLeaf (float *p, model_t *model);
byte	*Mod_ClusterPVS (int cluster, model_t *model);

void	Mod_Modellist_f (void);

void	*Hunk_Begin (int maxsize);
void	*Hunk_Alloc (int size);
int		Hunk_End (void);
void	Hunk_Free (void *base);

void	Mod_Free (model_t *mod);
void    Mod_FreeAll (void);
