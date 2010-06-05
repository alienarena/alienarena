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

d*_t structures are on-disk representations
m*_t structures are in-memory

  

*/
#include "r_iqm.h"
#include "r_matrixlib.h"
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

ALIAS MODELS

==============================================================================
*/

typedef struct
{
	byte			v[3];			// scaled byte to fit in frame mins/maxs
	byte			lightnormalindex;
	byte			latlong[2];
} mtrivertx_t;

typedef struct
{
	float			scale[3];		// multiply byte verts by this
	float			translate[3];	// then add this
	mtrivertx_t		*verts;			// variable sized
} maliasframe_t;

typedef struct
{
	char			name[MAX_SKINNAME];
} maliasskin_t;

typedef struct
{
	int				num_skins;
	maliasskin_t	*skins;

	int				num_xyz;
	int				num_tris;

	int				num_glcmds;		// dwords in strip/fan command list
	int				*glcmds;

	int				num_frames;
	maliasframe_t	*frames;
} maliasmdl_t;

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
typedef struct
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
	vec4_t		idx;
} mblendindex_t;

typedef struct
{
	vec4_t		weight;
} mblendweight_t;

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


#define	SURF_PLANEBACK		1
#define SURF_DRAWTURB		2
#define SURF_UNDERWATER		0x80

#define SurfaceIsTranslucent(surf) ((surf)->texinfo->flags & (SURF_TRANS33|SURF_TRANS66))
#define SurfaceIsAlphaBlended(surf) ((surf)->texinfo->flags & SURF_TRANS33 && (surf)->texinfo->flags & SURF_TRANS66)
#define SurfaceHasNoLightmap(surf) ((surf)->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP) && !SurfaceIsAlphaBlended((surf)))

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	unsigned short	v[2];
} medge_t;

/*
==============================================================================

VERTEX ARRAYS

==============================================================================
*/

typedef struct mtexinfo_s
{
	float		vecs[2][4];
	int			flags;
	int			numframes;
	struct mtexinfo_s	*next;		// animation chain
	image_t		*image;
	image_t		*normalMap;
	image_t		*heightMap;
	struct		rscript_t	*script;
	int			value;
} mtexinfo_t;

#define	VERTEXSIZE	10

typedef struct glpoly_s
{
	struct	glpoly_s	*next;
	struct	glpoly_s	*chain;
	int		numverts;
	float	verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2)
	int		flags;			// for SURF_UNDERWATER

	vec3_t		center;

} glpoly_t;

typedef struct msurface_s
{
	int			visframe;		// should be drawn when node is crossed

	cplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges
	
	int			texturemins[2];
	int			extents[2];

	int			light_s, light_t;	// gl lightmap coordinates
	int			dlight_s, dlight_t; // gl lightmap coordinates for dynamic lightmaps

	glpoly_t	*polys;				// multiple if warped
	struct	msurface_s	*texturechain;
	struct  msurface_s	*lightmapchain;
	struct	msurface_s	*specialchain;
	struct  msurface_s	*normalchain;

	mtexinfo_t	*texinfo;
	float		c_s, c_t;

// lighting info
	int			dlightframe;
	int			dlightbits;

	int			lightmaptexturenum;
	byte		styles[MAXLIGHTMAPS];
	float		cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	byte		*samples;		// [numstyles*surfsize]

	entity_t	*entity;
	
	float	tangentSpaceTransform[3][3];

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
} mleaf_t;


//===================================================================

//
// Whole model
//

typedef struct {
	int n[3];
} neighbors_t;

typedef enum {mod_bad, mod_brush, mod_sprite, mod_alias, mod_iqm } modtype_t;

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

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	dvis_t		*vis;

	byte		*lightdata;

	// for alias models and skins
	image_t		*skins[MAX_MD2SKINS];

	struct rscript_t	*script[MAX_MD2SKINS];

	int			extradatasize;
	void		*extradata;

	int			num_frames;

	//iqm skeletal model info
	mvertex_t		*animatevertexes;
	int				num_joints;
	iqmjoint_t		*joints;
	matrix3x4_t		*frames;
	int				num_poses;
	int				num_triangles;
	iqmtriangle_t	*tris;
	mnormal_t		*normal;
	mnormal_t		*animatenormal;
	mtangent_t		*tangent;
	mtangent_t		*animatetangent;
	mblendindex_t	*blendindexes;
	mblendweight_t	*blendweights;
	//end iqm

	fstvert_t	*st;
	neighbors_t *neighbors;	

	//vbo
	//vertCache_t	*vbo_st;
	//vertCache_t	*vbo_xyz;
	//vertCache_t	*vbo_color;
	//vertCache_t	*vbo_normals;

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

void	Mod_FreeAll (void);
void	Mod_Free (model_t *mod);
