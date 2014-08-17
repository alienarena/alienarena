/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2004-2014 COR Entertainment, LLC

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

// r_md2.c: MD2 file format loader

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

static vec3_t VertexArray[MAX_VERTICES];
static vec2_t TexCoordArray[MAX_VERTICES];
static vec3_t NormalsArray[MAX_VERTICES];
static vec4_t TangentsArray[MAX_VERTICES];

/*
========================
MD2_FindTriangleWithEdge
TODO: we can remove this when shadow volumes are gone.
========================
*/
static int MD2_FindTriangleWithEdge(neighbors_t * neighbors, dtriangle_t * tris, int numtris, int triIndex, int edgeIndex)
{
	int i, j, found = -1, foundj = 0;
	dtriangle_t *current = &tris[triIndex];
	qboolean dup = false;

	for (i = 0; i < numtris; i++) {
		if (i == triIndex)
			continue;

		for (j = 0; j < 3; j++) {
			if (((current->index_xyz[edgeIndex] == tris[i].index_xyz[j]) &&
				 (current->index_xyz[(edgeIndex + 1) % 3] ==
				  tris[i].index_xyz[(j + 1) % 3]))
				||
				((current->index_xyz[edgeIndex] ==
				  tris[i].index_xyz[(j + 1) % 3])
				 && (current->index_xyz[(edgeIndex + 1) % 3] ==
					 tris[i].index_xyz[j]))) {
				// no edge for this model found yet?
				if (found == -1) {
					found = i;
					foundj = j;
				} else
					dup = true;	// the three edges story
			}
		}
	}

	// normal edge, setup neighbor pointers
	if (!dup && found != -1) {
		neighbors[found].n[foundj] = triIndex;
		return found;
	}
	// naughty edge let no-one have the neighbor
	return -1;
}

/*
===============
MD2_BuildTriangleNeighbors
TODO: we can remove this when shadow volumes are gone.
===============
*/
static void MD2_BuildTriangleNeighbors(neighbors_t * neighbors,
									   dtriangle_t * tris, int numtris)
{
	int i, j;

	// set neighbors to -1
	for (i = 0; i < numtris; i++) {
		for (j = 0; j < 3; j++)
			neighbors[i].n[j] = -1;
	}

	// generate edges information (for shadow volumes)
	// NOTE: We do this with the original vertices not the reordered onces
	// since reordering them
	// duplicates vertices and we only compare indices
	for (i = 0; i < numtris; i++) {
		for (j = 0; j < 3; j++) {
			if (neighbors[i].n[j] == -1)
				neighbors[i].n[j] =
					MD2_FindTriangleWithEdge(neighbors, tris, numtris, i,
											 j);
		}
	}
}

#if 0
void MD2_VecsForTris(float *v0, float *v1, float *v2, float *st0, float *st1, float *st2, vec3_t Tangent)
{
	vec3_t	vec1, vec2;
	vec3_t	planes[3];
	float	tmp;
	int		i;

	for (i=0; i<3; i++)
	{
		vec1[0] = v1[i]-v0[i];
		vec1[1] = st1[0]-st0[0];
		vec1[2] = st1[1]-st0[1];
		vec2[0] = v2[i]-v0[i];
		vec2[1] = st2[0]-st0[0];
		vec2[2] = st2[1]-st0[1];
		VectorNormalize(vec1);
		VectorNormalize(vec2);
		CrossProduct(vec1,vec2,planes[i]);
	}

	for (i=0; i<3; i++)
	{
		tmp = 1.0 / planes[i][0];
		Tangent[i] = -planes[i][1]*tmp;
	}
	VectorNormalize(Tangent);
}
#else
// Math rearrangement for MD2 load speedup
void MD2_VecsForTris(
		const float *v0,
		const float *v1,
		const float *v2,
		const float *st0,
		const float *st1,
		const float *st2,
		vec3_t Tangent
		)
{
	vec3_t vec1, vec2;
	vec3_t planes[3];
	float tmp;
	float vec1_y, vec1_z, vec1_nrml;
	float vec2_y, vec2_z, vec2_nrml;
	int i;

	vec1_y = st1[0]-st0[0];
	vec1_z = st1[1]-st0[1];
	vec1_nrml = (vec1_y*vec1_y) + (vec1_z*vec1_z); // partial for normalize

	vec2_y = st2[0]-st0[0];
	vec2_z = st2[1]-st0[1];
	vec2_nrml = (vec2_y*vec2_y) + (vec2_z*vec2_z); // partial for normalize

	for (i=0; i<3; i++)
	{
		vec1[0] = v1[i]-v0[i];
		// VectorNormalize(vec1);
		tmp = (vec1[0] * vec1[0]) + vec1_nrml;
		tmp = sqrt(tmp);
		if ( tmp > 0.0 )
		{
			tmp = 1.0 / tmp;
			vec1[0] *= tmp;
			vec1[1] = vec1_y * tmp;
			vec1[2] = vec1_z * tmp;
		}

		vec2[0] = v2[i]-v0[i];
		// --- VectorNormalize(vec2);
		tmp = (vec2[0] * vec2[0]) + vec2_nrml;
		tmp = sqrt(tmp);
		if ( tmp > 0.0 )
		{
			tmp = 1.0 / tmp;
			vec2[0] *= tmp;
			vec2[1] = vec2_y * tmp;
			vec2[2] = vec2_z * tmp;
		}

		// --- CrossProduct(vec1,vec2,planes[i]);
		planes[i][0] = vec1[1]*vec2[2] - vec1[2]*vec2[1];
		planes[i][1] = vec1[2]*vec2[0] - vec1[0]*vec2[2];
		planes[i][2] = vec1[0]*vec2[1] - vec1[1]*vec2[0];
		// ---

		tmp = 1.0 / planes[i][0];
		Tangent[i] = -planes[i][1]*tmp;
	}

	VectorNormalize(Tangent);
}
#endif

static void MD2_PopulateFrameArrays (dmdl_t *pheader, fstvert_t *st, int framenum)
{
	int				i, j, k, l, va;
	daliasframe_t	*frame;
	dtrivertx_t		*verts, *v;
	vec3_t			normal, triangle[3], v1, v2;
	dtriangle_t		*tris = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);
	vec3_t	normals_[MAX_VERTS];
	vec3_t	tangents_[MAX_VERTS];

	frame = (daliasframe_t *)((byte *)pheader + pheader->ofs_frames + framenum * pheader->framesize);
	verts = frame->verts;

	memset(normals_, 0, pheader->num_xyz*sizeof(vec3_t));
	memset(tangents_, 0, pheader->num_xyz*sizeof(vec3_t));

	//for all tris
	for (j=0; j<pheader->num_tris; j++)
	{
		vec3_t	vv0,vv1,vv2;
		vec3_t tangent;
		
		//make 3 vec3_t's of this triangle's vertices
		for (k=0; k<3; k++)
		{
			l = tris[j].index_xyz[k];
			v = &verts[l];
			for (l=0; l<3; l++)
				triangle[k][l] = v->v[l];
		}

		//calculate normal
		VectorSubtract(triangle[0], triangle[1], v1);
		VectorSubtract(triangle[2], triangle[1], v2);
		CrossProduct(v2,v1, normal);
		VectorScale(normal, -1.0/VectorLength(normal), normal);
		
		// calculate tangent
		vv0[0] = (float)verts[tris[j].index_xyz[0]].v[0];
		vv0[1] = (float)verts[tris[j].index_xyz[0]].v[1];
		vv0[2] = (float)verts[tris[j].index_xyz[0]].v[2];
		vv1[0] = (float)verts[tris[j].index_xyz[1]].v[0];
		vv1[1] = (float)verts[tris[j].index_xyz[1]].v[1];
		vv1[2] = (float)verts[tris[j].index_xyz[1]].v[2];
		vv2[0] = (float)verts[tris[j].index_xyz[2]].v[0];
		vv2[1] = (float)verts[tris[j].index_xyz[2]].v[1];
		vv2[2] = (float)verts[tris[j].index_xyz[2]].v[2];

		MD2_VecsForTris(vv0, vv1, vv2,
					&st[tris[j].index_st[0]].s,
					&st[tris[j].index_st[1]].s,
					&st[tris[j].index_st[2]].s,
					tangent);
		
		for (k=0; k<3; k++)
		{
			l = tris[j].index_xyz[k];
			VectorAdd(normals_[l], normal, normals_[l]);
			VectorAdd(tangents_[l], tangent, tangents_[l]);
		}
	}

	for (j=0; j<pheader->num_xyz; j++)
		for (k=j+1; k<pheader->num_xyz; k++)
			if(verts[j].v[0] == verts[k].v[0] && verts[j].v[1] == verts[k].v[1] && verts[j].v[2] == verts[k].v[2])
			{
				// Have to make normalized versions or else the dot-product
				// won't come out right.
				vec3_t jnormal, knormal;
				VectorCopy (normals_[j], jnormal);
				VectorNormalize (jnormal);
				VectorCopy (normals_[k], knormal);
				VectorNormalize (knormal);
				
				// 45 degrees is our threshold for merging vertex normals.
				if(DotProduct(jnormal, knormal)>=cos(DEG2RAD(45)))
				{
					VectorAdd(normals_[j], normals_[k], normals_[j]);
					VectorCopy(normals_[j], normals_[k]);
					VectorAdd(tangents_[j], tangents_[k], tangents_[j]);
					VectorCopy(tangents_[j], tangents_[k]);
				}
			}

	//normalize average
	for (j=0; j<pheader->num_xyz; j++)
	{
		VectorNormalize(normals_[j]);
		VectorNormalize(tangents_[j]);
	}
	
	// Now that the normals and tangents are calculated, prepare them to be
	// loaded into the VBO.
	for (va = 0; va < pheader->num_tris*3; va++)
	{
	    int index_xyz = tris[va/3].index_xyz[va%3];
	    
	    VectorCopy (normals_[index_xyz], NormalsArray[va]);
	    
		VectorCopy (tangents_[index_xyz], TangentsArray[va]);
		TangentsArray[va][3] = 1.0;
		
		for (i = 0; i < 3; i++)
			VertexArray[va][i] = verts[index_xyz].v[i] * frame->scale[i] + frame->translate[i];
	}
}

void MD2_LoadVBO (model_t *mod, dmdl_t *pheader, fstvert_t *st)
{
	int framenum, va;
	
	dtriangle_t		*tris;
	
	tris = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);
	
	for (framenum = 0; framenum < mod->num_frames; framenum++)
	{
		MD2_PopulateFrameArrays (pheader, st, framenum);
	    
		R_VCLoadData (VBO_STATIC, pheader->num_tris*3*sizeof(vec3_t), VertexArray, VBO_STORE_XYZ+framenum, mod);
		R_VCLoadData (VBO_STATIC, pheader->num_tris*3*sizeof(vec3_t), NormalsArray, VBO_STORE_NORMAL+framenum, mod);
		R_VCLoadData (VBO_STATIC, pheader->num_tris*3*sizeof(vec4_t), TangentsArray, VBO_STORE_TANGENT+framenum, mod);
	}
	
	for (va = 0; va < pheader->num_tris*3; va++)
	{
		int index_st = tris[va/3].index_st[va%3];
	
		TexCoordArray[va][0] = st[index_st].s;
		TexCoordArray[va][1] = st[index_st].t;
	}
	
	R_VCLoadData (VBO_STATIC, pheader->num_tris*3*sizeof(vec2_t), TexCoordArray, VBO_STORE_ST, mod);
}

/*
=================
Mod_LoadMD2Model
=================
*/
void Mod_LoadMD2Model (model_t *mod, void *buffer)
{
	int					i, j;
	dmdl_t				*pinmodel, *pheader;
	dstvert_t			*pinst, *poutst;
	dtriangle_t			*pintri, *pouttri, *tris;
	daliasframe_t		*pinframe, *poutframe, *pframe;
	int					version;
	int					cx;
	float				s, t;
	float				iw, ih;
	fstvert_t			*st;
	char *pstring;
	int count;
	image_t *image;

	pinmodel = (dmdl_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
		Com_Printf("%s has wrong version number (%i should be %i)",
				 mod->name, version, ALIAS_VERSION);

	// TODO: we can stop permanently keeping this (change it to normal
	// Z_Malloc) as soon as shadow volumes are gone.
	mod->extradata = Hunk_Begin (0x300000);
	pheader = Hunk_Alloc (LittleLong(pinmodel->ofs_end));

	// byte swap the header fields and sanity check
	for (i=0 ; i<sizeof(dmdl_t)/sizeof(int) ; i++)
		((int *)pheader)[i] = LittleLong (((int *)buffer)[i]);

	if (pheader->skinheight > MAX_LBM_HEIGHT)
		Com_Printf("model %s has a skin taller than %d", mod->name,
				   MAX_LBM_HEIGHT);

	if (pheader->num_xyz <= 0)
		Com_Printf("model %s has no vertices", mod->name);

	if (pheader->num_xyz > MAX_VERTS)
		Com_Printf("model %s has too many vertices", mod->name);

	if (pheader->num_st <= 0)
		Com_Printf("model %s has no st vertices", mod->name);

	if (pheader->num_tris <= 0)
		Com_Printf("model %s has no triangles", mod->name);

	if (pheader->num_frames <= 0)
		Com_Printf("model %s has no frames", mod->name);

//
// load base s and t vertices
//
	pinst = (dstvert_t *) ((byte *)pinmodel + pheader->ofs_st);
	poutst = (dstvert_t *) ((byte *)pheader + pheader->ofs_st);

	for (i=0 ; i<pheader->num_st ; i++)
	{
		poutst[i].s = LittleShort (pinst[i].s);
		poutst[i].t = LittleShort (pinst[i].t);
	}

//
// load triangle lists
//
	pintri = (dtriangle_t *) ((byte *)pinmodel + pheader->ofs_tris);
	pouttri = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);

	for (i=0 ; i<pheader->num_tris ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			pouttri[i].index_xyz[j] = LittleShort (pintri[i].index_xyz[j]);
			pouttri[i].index_st[j] = LittleShort (pintri[i].index_st[j]);
		}
	}

//
// find neighbours - TODO: we can remove this when shadow volumes are gone.
//
	mod->neighbors = Hunk_Alloc(pheader->num_tris * sizeof(neighbors_t));
	MD2_BuildTriangleNeighbors(mod->neighbors, pouttri, pheader->num_tris);

//
// load the frames
//
	for (i=0 ; i<pheader->num_frames ; i++)
	{
		pinframe = (daliasframe_t *) ((byte *)pinmodel
			+ pheader->ofs_frames + i * pheader->framesize);
		poutframe = (daliasframe_t *) ((byte *)pheader
			+ pheader->ofs_frames + i * pheader->framesize);

		memcpy (poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j=0 ; j<3 ; j++)
		{
			poutframe->scale[j] = LittleFloat (pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat (pinframe->translate[j]);
		}
		// verts are all 8 bit, so no swapping needed
		memcpy (poutframe->verts, pinframe->verts,
			pheader->num_xyz*sizeof(dtrivertx_t));

	}

	mod->type = mod_md2;
	mod->num_frames = pheader->num_frames;
	
	// skin names are not always valid or file may not exist
	// do not register skins that cannot be found to eliminate extraneous
	//  file system searching.
	pstring = &((char*)pheader)[ pheader->ofs_skins ];
	count = pheader->num_skins;
	if ( count )
	{ // someday .md2's that do not have skins may have zero for num_skins
		memcpy( pstring, (char *)pinmodel + pheader->ofs_skins, count*MAX_SKINNAME );
		i = 0;
		while ( count-- )
		{
			pstring[MAX_SKINNAME-1] = '\0'; // paranoid
			image = GL_FindImage( pstring, it_skin);
			if ( image != NULL )
				mod->skins[i++] = image;
			else
				pheader->num_skins--; // the important part: adjust skin count
			pstring += MAX_SKINNAME;
		}
		
		// load script
		if (mod->skins[0] != NULL)
			mod->script = mod->skins[0]->script;
		if (mod->script)
			RS_ReadyScript( mod->script );
	}

	cx = pheader->num_st * sizeof(fstvert_t);
	st = (fstvert_t*)Z_Malloc (cx);

	// Calculate texcoords for triangles
	iw = 1.0 / pheader->skinwidth;
	ih = 1.0 / pheader->skinheight;
	for (i=0; i<pheader->num_st ; i++)
	{
		s = poutst[i].s;
		t = poutst[i].t;
		st[i].s = (s + 1.0) * iw; //tweak by one pixel
		st[i].t = (t + 1.0) * ih;
	}

	cx = pheader->num_xyz * pheader->num_frames * sizeof(byte);

	tris = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);

	mod->num_triangles = pheader->num_tris;

	//redo this using max/min from all frames
	pframe = ( daliasframe_t * ) ( ( byte * ) pheader +
									  pheader->ofs_frames);

	/*
	** compute axially aligned mins and maxs
	*/
	for ( i = 0; i < 3; i++ )
	{
		mod->mins[i] = pframe->translate[i];
		mod->maxs[i] = mod->mins[i] + pframe->scale[i]*255;
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
	
	MD2_LoadVBO (mod, pheader, st);
	
	Z_Free (st);
	
	mod->extradatasize = Hunk_End ();
}

void MD2_SelectFrame (void)
{
	if ( (currententity->frame >= currentmodel->num_frames)
		|| (currententity->frame < 0) )
	{
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ( (currententity->oldframe >= currentmodel->num_frames)
		|| (currententity->oldframe < 0))
	{
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ( !r_lerpmodels->integer )
		currententity->backlerp = 0;
}
