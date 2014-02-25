#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include <float.h>

// Maximums
#define DECAL_POLYS 256
#define DECAL_VERTS (2*(3*DECAL_POLYS)) // double to make room for temp new verts

void Decal_LoadVBO (model_t *mod, float *vposition, float *vnormal, float *vtangent, float *vtexcoord, unsigned int *vtriangles)
{
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec2_t), vtexcoord, VBO_STORE_ST, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec3_t), vposition, VBO_STORE_XYZ, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec3_t), vnormal, VBO_STORE_NORMAL, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec4_t), vtangent, VBO_STORE_TANGENT, mod);
	R_VCLoadData(VBO_STATIC, mod->num_triangles*3*sizeof(unsigned int), vtriangles, VBO_STORE_INDICES, mod);
}

extern void MD2_VecsForTris(
		const float *v0,
		const float *v1,
		const float *v2,
		const float *st0,
		const float *st1,
		const float *st2,
		vec3_t Tangent
		);


extern qboolean TriangleIntersectsBBox (const vec3_t v0, const vec3_t v1, const vec3_t v2, const vec3_t mins, const vec3_t maxs);
extern void AnglesToMatrix3x3 (vec3_t angles, float rotation_matrix[3][3]);

typedef struct
{
	int npolys;
	int nverts;
	unsigned int polys[DECAL_POLYS][3];
	vec3_t verts[DECAL_POLYS];
} decalprogress_t;

// FIXME: supports neither oriented terrain nor oriented decals!
void Mod_AddToDecalModel (const vec3_t mins, const vec3_t maxs, const vec3_t origin, const vec3_t angles, model_t *terrainmodel, decalprogress_t *out)
{
	int i, j, k, l;
	// For rotating terrain geometry into the decal's orientation
	vec3_t rotation_angles;
	float rotation_matrix[3][3];
	
	int trinum;
	
	vertCache_t	*vbo_xyz;
	vertCache_t	*vbo_indices;
	
	float *vposition;
	unsigned int *vtriangles;
	// If a vertex was at index n in the original terrain model, then in the
	// new decal model, it will be at index_table[n]-1. A value of 0 for 
	// index_table[n] indicates space for the vertex hasn't been allocated in
	// the new decal model yet.
	unsigned int *index_table;
	
	// Create the rotation matrices - we must apply the rotations in the 
	// opposite order or else we won't properly be preemptively undoing the 
	// rotation transformation applied later in the renderer.
	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
			rotation_matrix[i][j] = i == j;
	}
	for (i = 2; i >= 0; i--)
	{
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
	
	vbo_xyz = R_VCFindCache (VBO_STORE_XYZ, terrainmodel, NULL);
	GL_BindVBO (vbo_xyz);
	qglGetError ();
	vposition = qglMapBufferARB (GL_ARRAY_BUFFER_ARB, GL_READ_ONLY_ARB);
	GL_BindVBO (NULL);
	if (vposition == NULL)
	{
		Com_Printf ("Mod_AddToDecalModel: qglMapBufferARB on vertex positions: %u\n", qglGetError ());
		return;
	}
	
	vbo_indices = R_VCFindCache (VBO_STORE_INDICES, terrainmodel, NULL);
	GL_BindIBO (vbo_indices);
	vtriangles = qglMapBufferARB (GL_ELEMENT_ARRAY_BUFFER, GL_READ_ONLY_ARB);
	GL_BindIBO (NULL);
	if (vtriangles == NULL)
	{
		Com_Printf ("Mod_AddToDecalModel: qglMapBufferARB on vertex indices: %u\n", qglGetError ());
		return;
	}
	
	index_table = Z_Malloc (terrainmodel->numvertexes*sizeof(unsigned int));
	
	// Look for all the triangles from the terrain model that happen to occur
	// inside the oriented bounding box of the decal.
	for (trinum = 0; trinum < terrainmodel->num_triangles; trinum++)
	{
		int orig_idx;
		vec3_t verts[3];
		
		for (i = 0; i < 3; i++)
		{
			vec3_t tmp;
			VectorSubtract (&vposition[3*vtriangles[3*trinum+i]], origin, tmp);
			VectorClear (verts[i]);
			for (j = 0; j < 3; j++)
			{
				for (k = 0; k < 3; k++)
					verts[i][j] += tmp[k] * rotation_matrix[j][k];
			}
		}
		
		if (!TriangleIntersectsBBox (verts[0], verts[1], verts[2], mins, maxs))
			continue;
		
		for (i = 0; i < 3; i++)
		{
			orig_idx = vtriangles[3*trinum+i];
			if (index_table[orig_idx] == 0)
			{
				index_table[orig_idx] = ++out->nverts;
				if (out->nverts >= DECAL_VERTS)
					Com_Error (ERR_FATAL, "Mod_AddToDecalModel: DECAL_VERTS");
				VectorCopy (verts[i], out->verts[index_table[orig_idx]-1]);
			}
			
			out->polys[out->npolys][i] = index_table[orig_idx]-1;
		}
		out->npolys++;
		if (out->npolys >= DECAL_POLYS)
			Com_Error (ERR_FATAL, "Mod_AddToDecalModel: DECAL_POLYS");
	}
	
	Z_Free (index_table);
	
	GL_BindVBO (vbo_xyz);
	qglUnmapBufferARB (GL_ARRAY_BUFFER_ARB);
	GL_BindVBO (NULL);
	
	GL_BindIBO (vbo_indices);
	qglUnmapBufferARB (GL_ELEMENT_ARRAY_BUFFER_ARB);
	GL_BindIBO (NULL);
}

static entity_t *load_decalentity;

static void ClipTriangleAgainstBorder (int trinum, const vec3_t mins, const vec3_t maxs, int axisnum, qboolean maxborder, decalprogress_t *out)
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
			isoutside = coord > maxs[axisnum];
		else
			isoutside = coord < mins[axisnum];
		
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
				new_len = out->verts[insideidx[0]][axisnum] - maxs[axisnum];
			else
				new_len = out->verts[insideidx[0]][axisnum] - mins[axisnum];
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
			new_len = out->verts[insideidx[i]][axisnum] - maxs[axisnum];
		else
			new_len = out->verts[insideidx[i]][axisnum] - mins[axisnum];
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

static qboolean TriangleOutsideBounds (int trinum, const vec3_t mins, const vec3_t maxs, const decalprogress_t *mesh)
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
					isoutside = coord > maxs[axisnum];
				else
					isoutside = coord < mins[axisnum];
		
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
static void ReCullTriangles (const vec3_t mins, const vec3_t maxs, decalprogress_t *out)
{
	int outTriangle;
	int inTriangle;
	
	for (inTriangle = outTriangle = 0; inTriangle < out->npolys; inTriangle++)
	{
		if (TriangleOutsideBounds (inTriangle, mins, maxs, out))
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

void Mod_LoadDecalModel (model_t *mod, void *_buf)
{
	decalprogress_t data;
	int i, j;
	const char *tex_path = NULL;
	float *vnormal, *vtangent, *vtexcoord;
	image_t	*tex = NULL;
	char	*token;
	qboolean maxborder;
	char *buf = (char *)_buf;
	
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
	
	mod->type = mod_decal;
	
	if (!tex_path)
		Com_Error (ERR_DROP, "Mod_LoadDecalFile: Missing texture in %s!", mod->name);
	tex = GL_FindImage (tex_path, it_wall);
	mod->skins[0] = tex;
	
	memset (&data, 0, sizeof(data));
	
	for (i = 0; i < num_terrain_entities; i++)
		Mod_AddToDecalModel (mod->mins, mod->maxs, load_decalentity->origin, load_decalentity->angles, terrain_entities[i].model, &data);
	
	for (maxborder = false; maxborder <= true; maxborder++)
	{
		for (j = 0; j < 3; j++)
		{
			int lim = data.npolys;
			for (i = 0; i < lim; i++)
				ClipTriangleAgainstBorder (i, mod->mins, mod->maxs, j, maxborder, &data);
			ReCullTriangles (mod->mins, mod->maxs, &data);
		}
	}
	
	ReUnifyVertexes (&data);
	
	vnormal = Z_Malloc (data.nverts*sizeof(vec3_t));
	vtangent = Z_Malloc (data.nverts*sizeof(vec4_t));
	vtexcoord = Z_Malloc (data.nverts*sizeof(vec2_t));
	
	mod->numvertexes = data.nverts;
	mod->num_triangles = data.npolys;
	
	for (i = 0; i < data.nverts; i++)
	{
		for (j = 0; j < 2; j++)
			vtexcoord[2*i+!j] = 1.0-(data.verts[i][j] - mod->mins[j]) / (mod->maxs[j] - mod->mins[j]);
	}
	
	// Calculate normals and tangents
	for (i = 0; i < data.npolys; i++)
	{
		int j;
		vec3_t v1, v2, normal, tangent;
		unsigned int *triangle = data.polys[i];
		
		VectorSubtract (data.verts[triangle[0]], data.verts[triangle[1]], v1);
		VectorSubtract (data.verts[triangle[2]], data.verts[triangle[1]], v2);
		CrossProduct (v2, v1, normal);
		VectorScale (normal, -1.0/VectorLength(normal), normal);
		
		MD2_VecsForTris (	data.verts[triangle[0]], 
							data.verts[triangle[1]], 
							data.verts[triangle[2]],
							&vtexcoord[triangle[0]], 
							&vtexcoord[triangle[1]], 
							&vtexcoord[triangle[2]],
							tangent );
		
		for (j = 0; j < 3; j++)
		{
			VectorAdd (&vnormal[3*triangle[j]], normal, &vnormal[3*triangle[j]]);
			VectorAdd (&vtangent[4*triangle[j]], tangent, &vtangent[4*triangle[j]]);
		}
	}
	
	// Normalize the average normals and tangents
	for (i = 0; i < mod->numvertexes; i++)
	{
		VectorNormalize (&vnormal[3*i]);
		VectorNormalize (&vtangent[4*i]);
		vtangent[4*i+3] = 1.0;
	}
	
	Decal_LoadVBO (mod, &data.verts[0][0], vnormal, vtangent, vtexcoord, &data.polys[0][0]);
	
	Z_Free (vnormal);
	Z_Free (vtangent);
	Z_Free (vtexcoord);
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
