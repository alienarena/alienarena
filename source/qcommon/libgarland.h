// we always use doubles internally, but the outside world works in floats.
typedef float incoord_t;

// vertex index
typedef unsigned int idx_t; 

// The algorithm simplifies meshes in any number of dimensions. For example,
// a 3D mesh would obviously use up 3 dimensions. However, adding texcoords
// brings the total up to 5 dimensions (i.e. manipulating a mesh in 5D space.)
// If you were doing a skeletal model with 10 bones and blend weights for each
// bone, that would bring the total up to 15 dimensions. For now, we just do
// spacial and texture coordinates for a total of 5 dimensions.

#define AXES			5
#define _SCOORDS(s,x,y)	(y*s+x - y*(y+1)/2)
#define SCOORDS(s,x,y)	((y)>(x) ? _SCOORDS((s),(y),(x)) : _SCOORDS((s),(x),(y)))
#define ACOORDS(x,y)	SCOORDS(AXES,x,y)
#define MATSIZE			(1 + ACOORDS (AXES-1, AXES-1))

// The quadric of a plane is effectively a function that computes the squared
// distance of a given point from that plane. The values in this structure
// determines how it will multiply and add the elements of a point vector,
// similar to setting up the values in a matrix to get a desired
// transformation function. There are simpler functions that can also get the
// distance from a point to a plane, but quadrices have some nifty
// mathematical properties that are desirable here.
typedef struct
{
	// Symmetric nxn matrix; redundant values on one side of the diagonal are
	// ommitted, so we store fewer than nxn values. If n == 3, then matrix is
	// layed out like this:
	//	A[0]	A[1]	A[2]
	//	A[1]	A[3]	A[4]
	//	A[2]	A[4]	A[5]
	// You don't really need to worry about this, just use the ACOORDS macro.
	double	A[MATSIZE];
	
	// n-vector
	double	b[AXES];
	
	// constant
	double	c;
} quadric_t;

typedef struct edge_s
{
	struct vert_s	*vtx_a, *vtx_b;
	
	// triangles that use this edge
	struct tri_s	*tri_1, *tri_2;

	char		border;				// 1 if the edge is only used once
	char		cull;				// 1 if we're not keeping it
	double		contract_pos[AXES];	// calculated optimum position for contraction
	double		contract_error;		// cost of deleting this edge
	char		contract_flipvtx;	// if this is 1, contract into vertex b
	
	// for the binheap code
	unsigned int	sorted_pos;
} edge_t;

typedef struct edgelist_s
{
	edge_t				*edge;
	struct edgelist_s	*next;
} edgelist_t;

typedef struct trilist_s
{
	struct tri_s		*tri;
	struct trilist_s	*next;
} trilist_t;

typedef struct vert_s
{
	char		cull;	// 1 if we're not keeping it
	idx_t		idx;	// Used to assign the vertex index when creating the
						// new simplified geometry.
	double		pos[AXES];
	quadric_t	quadric;
	edgelist_t	*edges;
	trilist_t	*tris;
} vert_t;

typedef struct tri_s
{
	char	cull; // 1 if we're not keeping it
	vert_t	*verts[3];
} tri_t;

typedef struct
{
	idx_t		num_verts, num_tris;
	
	incoord_t	*vcoords;
	incoord_t	*vtexcoords;
	
	idx_t		*tris;
	
	// these fields are for internal use only
	idx_t		simplified_num_tris;
	idx_t		num_edges;
	edge_t		*edges;
	vert_t		*everts;
	tri_t		*etris;
	
	edgelist_t	*edgelinks;
	trilist_t	*trilinks;
	
	binheap_t	edgeheap;
	double		mins[AXES], maxs[AXES], scale[AXES];
} mesh_t;

void simplify_mesh (mesh_t *mesh, idx_t target_polycount);
