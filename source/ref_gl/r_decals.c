#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include <float.h>

// Maximums
#define DECAL_POLYS MAX_TRIANGLES // This really is a bit high
#define DECAL_VERTS (2*(3*DECAL_POLYS)) // double to make room for temp new verts

extern qboolean TriangleIntersectsBBox
	(const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec3_t mins, const vec3_t maxs, vec3_t out_mins, vec3_t out_maxs);
extern void AnglesToMatrix3x3 (vec3_t angles, float rotation_matrix[3][3]);

// Creates a rotation matrix that will *undo* the specified rotation. We must
// apply the rotations in the opposite order or else we won't properly be
// preemptively undoing the rotation transformation applied later in the
// renderer.
void AnglesToMatrix3x3_Backwards (const vec3_t angles, float rotation_matrix[3][3])
{
	int i, j, k, l;
	
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
			rotation_matrix[i][j] = i == j;
	}
	for (i = 2; i >= 0; i--)
	{
		vec3_t rotation_angles;
		float prev_rotation_matrix[3][3];
		float temp_matrix[3][3];
		
		for (j = 0; j < 3; j++)
		{
			for (k = 0; k < 3; k++)
			{
				prev_rotation_matrix[j][k] = rotation_matrix[j][k];
				rotation_matrix[j][k] = 0.0;
			}
		}
		
		VectorClear (rotation_angles);
		rotation_angles[i] = -angles[i];
		AnglesToMatrix3x3 (rotation_angles, temp_matrix);
		
		for (j = 0; j < 3; j++)
		{
			for (k = 0; k < 3; k++)
			{
				for (l = 0; l < 3; l++)
					rotation_matrix[j][k] += prev_rotation_matrix[l][k] * temp_matrix[j][l];
			}
		}
	}
}

// This structure is used to accumulate 3D geometry that will make up a 
// finished decal mesh. Geometry is taken from the world (including terrain
// meshes and BSP surfaces,) clipped aginst the oriented bounding box of the
// decal, and retesselated as needed.
typedef struct
{
	int npolys;
	int nverts;
	unsigned int polys[DECAL_POLYS][3];
	vec3_t verts[DECAL_VERTS];
} decalprogress_t;

// This structure contains 3D geometry that might potentially get added to a
// decal. All the triangles in here will be checked against a decal's OBB as
// described above.
typedef struct
{
	int npolys;
	int nverts;
	unsigned int *vtriangles;
	mesh_framevbo_t *framevbo;
	// For moving terrain/mesh geometry to its actual position in the world
	vec3_t origin;
	// For rotating terrain/mesh geometry into its actual orientation
	float rotation_matrix[3][3];
} decalinput_t;

// This structure describes the orientation and position of the decal being
// created.
typedef struct
{
	// For transforming world-oriented geometry to the decal's position
	vec3_t origin;
	// For rotating world-oriented geometry into the decal's orientation
	float decal_space_matrix[3][3]; 
	// For clipping decal-oriented geometry against the decal's bounding box
	vec3_t mins, maxs;
} decalorientation_t;

// This function should be able to handle any indexed triangle geometry.
static void Mod_AddToDecalModel (const decalorientation_t *pos, const decalinput_t *in, decalprogress_t *out)
{
	int i, j, k;
	
	int trinum;
	
	// If a vertex was at index n in the original model, then in the new decal
	// model, it will be at index_table[n]-1. A value of 0 for index_table[n]
	// indicates space for the vertex hasn't been allocated in the new decal
	// model yet.
	unsigned int *index_table;
	
	index_table = Z_Malloc (in->nverts*sizeof(unsigned int));
	
	// Look for all the triangles from the original model that happen to occur
	// inside the oriented bounding box of the decal.
	for (trinum = 0; trinum < in->npolys; trinum++)
	{
		int orig_idx;
		vec3_t verts[3];
		
		for (i = 0; i < 3; i++)
		{
			vec3_t tmp, tmp2;
			
			// First, transform the mesh geometry into world space
			VectorCopy (in->framevbo[in->vtriangles[3*trinum+i]].vertex, tmp);
			VectorClear (tmp2);
			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 3; k++)
					tmp2[j] += tmp[k] * in->rotation_matrix[j][k];
			}
			VectorAdd (tmp2, in->origin, tmp2);
			
			// Then, transform it into decal space
			VectorSubtract (tmp2, pos->origin, tmp);
			VectorClear (verts[i]);
			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 3; k++)
					verts[i][j] += tmp[k] * pos->decal_space_matrix[j][k];
			}
		}
		
		if (!TriangleIntersectsBBox (verts[0], verts[1], verts[2], pos->mins, pos->maxs, NULL, NULL))
			continue;
		
		for (i = 0; i < 3; i++)
		{
			orig_idx = in->vtriangles[3*trinum+i];
			if (index_table[orig_idx] == 0)
			{
				index_table[orig_idx] = ++out->nverts;
				if (out->nverts >= DECAL_VERTS)
					Com_Error (ERR_FATAL, "Mod_AddTerrainToDecalModel: DECAL_VERTS");
				VectorCopy (verts[i], out->verts[index_table[orig_idx]-1]);
			}
			
			out->polys[out->npolys][i] = index_table[orig_idx]-1;
		}
		out->npolys++;
		if (out->npolys >= DECAL_POLYS)
			Com_Error (ERR_FATAL, "Mod_AddTerrainToDecalModel: DECAL_POLYS");
	}
	
	Z_Free (index_table);
	
}

// Add geometry from a terrain model to the decal
static void Mod_AddTerrainToDecalModel (const decalorientation_t *pos, entity_t *terrainentity, decalprogress_t *out)
{
	model_t *terrainmodel;
	
	GLuint vbo_xyz;
	
	decalinput_t in;
	
	terrainmodel = terrainentity->model;
	
	VectorCopy (terrainentity->origin, in.origin);
	AnglesToMatrix3x3 (terrainentity->angles, in.rotation_matrix);
	in.npolys = terrainmodel->num_triangles;
	in.nverts = terrainmodel->numvertexes;
	
	vbo_xyz = terrainmodel->vboIDs[(terrainmodel->typeFlags & MESH_INDEXED) ? 2 : 1];
	GL_BindVBO (vbo_xyz);
	in.framevbo = qglMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB);
	if (in.framevbo == NULL)
	{
		Com_Printf ("Mod_AddTerrainToDecalModel: qglMapBufferARB on vertex positions: %u\n", qglGetError ());
		return;
	}
	
	if ((terrainmodel->typeFlags & MESH_INDEXED))
	{
		GLuint vbo_indices = terrainmodel->vboIDs[1];
		GL_BindVBO (vbo_indices);
		in.vtriangles = qglMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB);
		if (in.vtriangles == NULL)
		{
		    GL_BindVBO (0); // Do we need this?
			Com_Printf ("Mod_AddTerrainToDecalModel: qglMapBufferARB on vertex indices: %u\n", qglGetError ());
			return;
		}
		
		Mod_AddToDecalModel (pos, &in, out);
		
		qglUnmapBufferARB (GL_ARRAY_BUFFER_ARB);
	}
	else
	{
		int i;
		
		// Fake indexing for MD2s.
		in.npolys = in.nverts / 3;
		in.vtriangles = Z_Malloc (in.nverts * sizeof(unsigned int));
		for (i = 0; i < in.nverts; i++)
			in.vtriangles[i] = i;
		
		Mod_AddToDecalModel (pos, &in, out);
		
		Z_Free (in.vtriangles);
	}
	
	GL_BindVBO (vbo_xyz);
	qglUnmapBufferARB (GL_ARRAY_BUFFER_ARB);
	GL_BindVBO (0);
}

// Add geometry from the BSP to the decal
static void Mod_AddBSPToDecalModel (const decalorientation_t *pos, decalprogress_t *out)
{
	int surfnum, vertnum, trinum;
	int i, n;
	float *v;
	decalinput_t in;
	
	VectorClear (in.origin);
	AnglesToMatrix3x3 (vec3_origin, in.rotation_matrix);
	
	// FIXME HACK: we generate a separate indexed mesh for each BSP surface.
	// TODO: generate indexed BSP VBOs in r_vbo.c, and then just use the same
	// code as we use for other mesh types.
	for (surfnum = 0; surfnum < r_worldmodel->numsurfaces; surfnum++)
	{
		msurface_t *surf = &r_worldmodel->surfaces[surfnum];
		
		if ((surf->texinfo->flags & (SURF_SKY|SURF_WARP|SURF_FLOWING|SURF_NODRAW)))
			continue;
		
		in.nverts = surf->polys->numverts;
		in.framevbo = Z_Malloc (in.nverts * sizeof(*in.framevbo));
		
		for (vertnum = 0, v = surf->polys->verts[0]; vertnum < in.nverts; vertnum++, v += VERTEXSIZE)
			VectorCopy (v, in.framevbo[vertnum].vertex);
		
		in.npolys = in.nverts - 2;
		in.vtriangles = Z_Malloc (in.npolys * 3 * sizeof(unsigned int));
		
		for (trinum = 1, n = 0; trinum < in.nverts-1; trinum++)
		{
			in.vtriangles[n++] = 0;
		
			for (i = trinum; i < trinum+2; i++)
				in.vtriangles[n++] = i;
		}
		
		Mod_AddToDecalModel (pos, &in, out);
		
		Z_Free (in.framevbo);
		Z_Free (in.vtriangles);
	}
}

// If a triangle is entirely inside the bounding box, this function does 
// nothing. Otherwise, it clips against one of the bounding planes of the box
// and then (if needed) retesselates the area being kept. If maxborder is 
// false and axisnum is 0, then the plane being clipped against is the plane
// defined by x = mins[0]. If maxborder is true and axisnum is 1, then the
// plane is y = maxs[1].
static void ClipTriangleAgainstBorder (int trinum, const decalorientation_t *pos, int axisnum, qboolean maxborder, decalprogress_t *out)
{
	int new_trinum;
	int i;
	unsigned int outsideidx[3], insideidx[3];
	unsigned newidx[3];
	int outside[3];
	int noutside = 0, ninside = 0;
	
	// For making sure any new triangles are facing the right way.
	vec3_t v0, v1, norm, norm2;
	qboolean flip;
	
	for (i = 0; i < 3; i++)
	{
		qboolean isoutside;
		float coord;
		unsigned int vertidx = out->polys[trinum][i];
		
		coord = out->verts[vertidx][axisnum];
		
		if (maxborder)
			isoutside = coord > pos->maxs[axisnum];
		else
			isoutside = coord < pos->mins[axisnum];
		
		if (isoutside)
		{
			outside[noutside] = i;
			outsideidx[noutside++] = vertidx;
		}
		else
		{
			insideidx[ninside++] = vertidx;
		}
	}
	
	assert (ninside+noutside == 3);
	
	// No need to clip anything
	if (ninside == 3)
		return;
	
	assert (ninside != 0);
	assert (noutside != 3);
	
	// No need to add a new vertex and retesselate the resulting quad, just
	// clip the two vertices that are outside.
	if (noutside == 2)
	{
		for (i = 0; i < 2; i++)
		{
			float old_len, new_len;
			vec3_t dir;
			float *new_vert = out->verts[out->nverts];
			
			if (out->nverts+1 >= DECAL_VERTS)
				Com_Error (ERR_FATAL, "ClipTriangleAgainstBorder: DECAL_VERTS");
			
			VectorSubtract (out->verts[outsideidx[i]], out->verts[insideidx[0]], dir);
			old_len = VectorNormalize (dir);
			
			if (maxborder)
				new_len = out->verts[insideidx[0]][axisnum] - pos->maxs[axisnum];
			else
				new_len = out->verts[insideidx[0]][axisnum] - pos->mins[axisnum];
			new_len *= old_len;
			new_len /= out->verts[insideidx[0]][axisnum] - out->verts[outsideidx[i]][axisnum];
			
			VectorMA (out->verts[insideidx[0]], new_len, dir, new_vert);
			out->polys[trinum][outside[i]] = out->nverts++;
		}
		
		return;
	}
	
	// If we reach this point, noutside == 1
	assert (noutside == 1);
	
	new_trinum = out->npolys++;
	
	if (out->npolys >= DECAL_POLYS)
		Com_Error (ERR_FATAL, "ClipTriangleAgainstBorder: DECAL_POLYS");
	
	for (i = 0; i < 2; i++)
	{
		float old_len, new_len;
		vec3_t dir;
		float *new_vert = out->verts[out->nverts];
		
		if (out->nverts+1 >= DECAL_VERTS)
			Com_Error (ERR_FATAL, "ClipTriangleAgainstBorder: DECAL_VERTS");
		
		newidx[i] = out->nverts++;
		
		VectorSubtract (out->verts[outsideidx[0]], out->verts[insideidx[i]], dir);
		old_len = VectorNormalize (dir);
		
		if (maxborder)
			new_len = out->verts[insideidx[i]][axisnum] - pos->maxs[axisnum];
		else
			new_len = out->verts[insideidx[i]][axisnum] - pos->mins[axisnum];
		new_len *= old_len;
		new_len /= out->verts[insideidx[i]][axisnum] - out->verts[outsideidx[0]][axisnum];
		
		VectorMA (out->verts[insideidx[i]], new_len, dir, new_vert);
	}
	
	VectorSubtract (out->verts[out->polys[trinum][1]], out->verts[out->polys[trinum][0]], v0);
	VectorSubtract (out->verts[out->polys[trinum][2]], out->verts[out->polys[trinum][0]], v1);
	CrossProduct (v0, v1, norm);
	VectorNormalize (norm);
	
	out->polys[trinum][outside[0]] = newidx[0];
	
	for (i = 0; i < 2; i++)
		out->polys[new_trinum][i] = newidx[i];
	out->polys[new_trinum][2] = insideidx[1];
	
	VectorSubtract (out->verts[out->polys[new_trinum][1]], out->verts[out->polys[new_trinum][0]], v0);
	VectorSubtract (out->verts[out->polys[new_trinum][2]], out->verts[out->polys[new_trinum][0]], v1);
	CrossProduct (v0, v1, norm2);
	VectorNormalize (norm2);
	
	flip = false;
	for (i = 0; i < 3; i++)
	{
		if (norm[i]/norm2[i] < 0.0)
			flip = true;
	}
	
	if (flip)
	{
		unsigned int tmpidx = out->polys[new_trinum][0];
		out->polys[new_trinum][0] = out->polys[new_trinum][1];
		out->polys[new_trinum][1] = tmpidx;
	}
}

static qboolean TriangleOutsideBounds (int trinum, const decalorientation_t *pos, const decalprogress_t *mesh)
{
	int axisnum;
	qboolean maxborder;
	
	for (maxborder = false; maxborder <= true; maxborder++)
	{
		for (axisnum = 0; axisnum < 3; axisnum++)
		{
			int i;
			int noutside = 0;
	
			for (i = 0; i < 3; i++)
			{
				qboolean isoutside;
				float coord;
				unsigned int vertidx = mesh->polys[trinum][i];
		
				coord = mesh->verts[vertidx][axisnum];
		
				if (maxborder)
					isoutside = coord > pos->maxs[axisnum];
				else
					isoutside = coord < pos->mins[axisnum];
		
				if (isoutside)
					noutside++;
			}
			
			if (noutside == 3)
				return true;
		}
	}
	
	return false;
}

// Sometimes, in the process of clipping a triangle against one border of the
// bounding box, one of the resulting split triangles is now completely
// outside another border. For example:
//            /\ 
//  .========/  \ 
//  |       /|___\ 
//  |        |
//  '========'
// After clipping against the top border, we get a trapezoid-shaped region
// which is tesselated into two triangles. Depending on the tesselation, one
// of those triangles may actually be entirely outside the bounding box.
//
// So for the sake of consistency, we re-cull every terrain triangle we 
// modify before re-clipping it against the next border.
static void ReCullTriangles (const decalorientation_t *pos, decalprogress_t *out)
{
	int outTriangle;
	int inTriangle;
	
	for (inTriangle = outTriangle = 0; inTriangle < out->npolys; inTriangle++)
	{
		if (TriangleOutsideBounds (inTriangle, pos, out))
			continue;
		
		if (outTriangle != inTriangle)
		{
			int i;
			
			for (i = 0; i < 3; i++)
				out->polys[outTriangle][i] = out->polys[inTriangle][i];
		}
		
		outTriangle++;
	}
	
	out->npolys = outTriangle;
}

static void ReUnifyVertexes (decalprogress_t *out)
{
	int trinum, vertnum, maxidx = 0, newidx = 0;
	unsigned int *index_table;
	vec3_t *new_verts;
	
	// Detect duplicate vertices and substitute the index of the first
	// occurance of each duplicate vertex
	for (trinum = 0; trinum < out->npolys; trinum++)
	{
		int i;
		for (i = 0; i < 3; i++)
		{
			int tmpidx, idx;
			
			tmpidx = out->polys[trinum][i];
			for (idx = 0; idx < tmpidx; idx++)
			{
				int j;
				
				for (j = 0; j < 3; j++)
				{
					if (fabs (out->verts[tmpidx][j] - out->verts[idx][j]) > FLT_EPSILON)
						break;
				}
				
				if (j == 3)
					break;
			}
			
			out->polys[trinum][i] = idx;
			
			if (maxidx <= idx)
				maxidx = idx+1;
		}
	}
	
	out->nverts = maxidx;
	
	// Prune redundant vertices- generate new indices
	index_table = Z_Malloc (maxidx*sizeof(unsigned int));
	for (trinum = 0; trinum < out->npolys; trinum++)
	{
		int i;
		for (i = 0; i < 3; i++)
		{
			if (index_table[out->polys[trinum][i]] == 0)
				index_table[out->polys[trinum][i]] = ++newidx;
			out->polys[trinum][i] = index_table[out->polys[trinum][i]] - 1;
		}
	}
	
	// Prune redundant vertices- rearrange the actual vertex data to match the
	// new indices
	new_verts = Z_Malloc (newidx*sizeof(vec3_t));
	for (vertnum = 0; vertnum < maxidx; vertnum++)
	{
		if (index_table[vertnum] != 0)
			VectorCopy (out->verts[vertnum], new_verts[index_table[vertnum]-1]);
	}
	memcpy (out->verts, new_verts, newidx*sizeof(vec3_t));
	
	out->nverts = newidx;
	
	Z_Free (index_table);
	Z_Free (new_verts);
}

static entity_t *load_decalentity;

void Mod_LoadDecalModel (model_t *mod, void *_buf)
{
	decalprogress_t data;
	int i, j;
	const char *tex_path = NULL;
	nonskeletal_basevbo_t *basevbo = NULL;
	mesh_framevbo_t *framevbo = NULL;
	image_t	*tex = NULL;
	char	*token;
	qboolean maxborder;
	char *buf = (char *)_buf;
	decalorientation_t pos;
	
	buf = strtok (buf, ";");
	while (buf)
	{
		token = COM_Parse (&buf);
		if (!buf && !(buf = strtok (NULL, ";")))
			break;

#define FILENAME_ATTR(cmd_name,out) \
		if (!Q_strcasecmp (token, cmd_name)) \
		{ \
			out = CopyString (COM_Parse (&buf)); \
			if (!buf) \
				Com_Error (ERR_DROP, "Mod_LoadDecalFile: EOL when expecting " cmd_name " filename! (File %s is invalid)", mod->name); \
		} 
		
		FILENAME_ATTR ("texture", tex_path)
		
#undef FILENAME_ATTR
		
		if (!Q_strcasecmp (token, "mins"))
		{
			for (i = 0; i < 3; i++)
			{
				mod->mins[i] = atof (COM_Parse (&buf));
				if (!buf)
					Com_Error (ERR_DROP, "Mod_LoadDecalFile: EOL when expecting mins %c axis! (File %s is invalid)", "xyz"[i], mod->name);
			}
		}
		if (!Q_strcasecmp (token, "maxs"))
		{
			for (i = 0; i < 3; i++)
			{
				mod->maxs[i] = atof (COM_Parse (&buf));
				if (!buf)
					Com_Error (ERR_DROP, "Mod_LoadDecalFile: EOL when expecting maxs %c axis! (File %s is invalid)", "xyz"[i], mod->name);
			}
		}
		
		//For forward compatibility-- if this file has a statement type we
		//don't recognize, or if a recognized statement type has extra
		//arguments supplied to it, then this is probably supported in a newer
		//newer version of CRX. But the best we can do is just fast-forward
		//through it.
		buf = strtok (NULL, ";");
	}

	/*
	** compute a full bounding box
	*/
	for ( i = 0; i < 8; i++ )
	{
		vec3_t   tmp;

		if ( i & 1 )
			tmp[0] = mod->mins[0];
		else
			tmp[0] = mod->maxs[0];

		if ( i & 2 )
			tmp[1] = mod->mins[1];
		else
			tmp[1] = mod->maxs[1];

		if ( i & 4 )
			tmp[2] = mod->mins[2];
		else
			tmp[2] = mod->maxs[2];

		VectorCopy( tmp, mod->bbox[i] );
	}	
	
	mod->type = mod_decal;
	mod->typeFlags = MESH_INDEXED | MESH_DECAL;
	
	if (!tex_path)
		Com_Error (ERR_DROP, "Mod_LoadDecalFile: Missing texture in %s!", mod->name);
	tex = GL_FindImage (tex_path, it_wall);
	mod->skins[0] = tex;
	
	memset (&data, 0, sizeof(data));
	
	VectorCopy (mod->mins, pos.mins);
	VectorCopy (mod->maxs, pos.maxs);
	VectorCopy (load_decalentity->origin, pos.origin);
	// Create a rotation matrix to apply to world/terrain geometry to
	// transform it into decal space.
	AnglesToMatrix3x3_Backwards (load_decalentity->angles, pos.decal_space_matrix);
	
	for (i = 0; i < num_terrain_entities; i++)
		Mod_AddTerrainToDecalModel (&pos, &terrain_entities[i], &data);
	
	Mod_AddBSPToDecalModel (&pos, &data);
	
	for (maxborder = false; maxborder <= true; maxborder++)
	{
		for (j = 0; j < 3; j++)
		{
			int lim = data.npolys;
			for (i = 0; i < lim; i++)
				ClipTriangleAgainstBorder (i, &pos, j, maxborder, &data);
			ReCullTriangles (&pos, &data);
		}
	}
	
	ReUnifyVertexes (&data);
	
	mod->numvertexes = data.nverts;
	mod->num_triangles = data.npolys;
	
	basevbo = Z_Malloc (mod->numvertexes * sizeof(*basevbo));
	framevbo = Z_Malloc (mod->numvertexes * sizeof(*framevbo));
	
	for (i = 0; i < data.nverts; i++)
	{
		for (j = 0; j < 2; j++)
			basevbo[i].st[!j] = 1.0-(data.verts[i][j] - mod->mins[j]) / (mod->maxs[j] - mod->mins[j]);
		VectorCopy (data.verts[i], framevbo[i].vertex);
	}
	
	R_Mesh_LoadVBO (mod, MESHLOAD_CALC_NORMAL|MESHLOAD_CALC_TANGENT, basevbo, &data.polys[0][0], framevbo);
	
	Z_Free (framevbo);
	Z_Free (basevbo);
}

void R_ParseDecalEntity (char *match, char *block)
{
	entity_t	*ent;
	int			i;
	char		*bl, *tok;
	
	char model_path[MAX_QPATH];
	
	// This is used to make sure each loaded decal entity gets a unique copy
	// of the model. No need to reset this or anything.
	char fake_path[MAX_QPATH];
	static int	num_decals = 0; 
	
	if (num_decal_entities == MAX_MAP_MODELS)
		Com_Error (ERR_DROP, "R_ParseDecalEntity: MAX_MAP_MODELS");
	
	ent = &decal_entities[num_decal_entities];
	memset (ent, 0, sizeof(*ent));
	ent->number = MAX_EDICTS+MAX_MAP_MODELS+MAX_ROCKS+num_decal_entities++;
	load_decalentity = ent;
	ent->flags = RF_FULLBRIGHT;
	
	Com_sprintf (fake_path, sizeof(fake_path), "*=decal%d=*", num_decals++);
	
	bl = block;
	while (1)
	{
		tok = Com_ParseExt(&bl, true);
		if (!tok[0])
			break;		// End of data

		if (!Q_strcasecmp("model", tok))
		{
			// Must insure the entity's angles and origin are loaded first
			strncpy (model_path, Com_ParseExt(&bl, false), sizeof(model_path));
		}
		else if (!Q_strcasecmp("angles", tok))
		{
			for (i = 0; i < 3; i++)
				ent->angles[i] = atof(Com_ParseExt(&bl, false));
		}
		else if (!Q_strcasecmp("angle", tok))
		{
			ent->angles[YAW] = atof(Com_ParseExt(&bl, false));
		}
		else if (!Q_strcasecmp("origin", tok))
		{
			for (i = 0; i < 3; i++)
				ent->origin[i] = atof(Com_ParseExt(&bl, false));
		}
		else
			Com_SkipRestOfLine(&bl);
	}
	
	ent->model = R_RegisterModel (model_path);
	strncpy (ent->model->name, fake_path, sizeof(fake_path));
}
