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
// r_mesh.c: triangle model functions

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_ragdoll.h"
#include "r_lodcalc.h"

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

#define NUMVERTEXNORMALS	162

float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

static vec3_t VertexArray[MAX_VERTICES];
static vec2_t TexCoordArray[MAX_VERTICES];
static vec3_t NormalsArray[MAX_VERTICES];
static vec4_t TangentsArray[MAX_VERTICES];

static vertCache_t	*vbo_st;
static vertCache_t	*vbo_xyz;
static vertCache_t	*vbo_normals;
static vertCache_t *vbo_tangents;

extern	vec3_t	lightspot;
vec3_t	shadevector;
float	shadelight[3];

#define MAX_MODEL_DLIGHTS 128
m_dlight_t model_dlights[MAX_MODEL_DLIGHTS];
int model_dlights_num;

extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
extern rscript_t *rs_glass;
extern image_t *r_mirrortexture;

extern cvar_t *cl_gun;

cvar_t *gl_mirror;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anormtab.h"
;

/*
=============
MD2 Loading Routines

=============
*/

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

/*
========================
MD2_FindTriangleWithEdge

========================
*/
static int MD2_FindTriangleWithEdge(neighbors_t * neighbors, dtriangle_t * tris, int numtris, int triIndex, int edgeIndex){


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

void MD2_LoadVertexArrays(model_t *md2model){

	dmdl_t *md2;
	daliasframe_t *md2frame;
	dtrivertx_t	*md2v;
	int i;

	if(md2model->num_frames > 1)
		return;

	md2 = (dmdl_t *)md2model->extradata;

	md2frame = (daliasframe_t *)((byte *)md2 + md2->ofs_frames);

	if(md2->num_xyz > MAX_VERTS)
		return;

	md2model->vertexes = (mvertex_t*)Hunk_Alloc(md2->num_xyz * sizeof(mvertex_t));

	for(i = 0, md2v = md2frame->verts; i < md2->num_xyz; i++, md2v++){  // reconstruct the verts
		VectorSet(md2model->vertexes[i].position,
					md2v->v[0] * md2frame->scale[0] + md2frame->translate[0],
					md2v->v[1] * md2frame->scale[1] + md2frame->translate[1],
					md2v->v[2] * md2frame->scale[2] + md2frame->translate[2]);
	}

}

#if 0
byte MD2_Normal2Index(const vec3_t vec)
{
	int i, best;
	float d, bestd;

	bestd = best = 0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct (vec, r_avertexnormals[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}
#else
// for MD2 load speedup. Adapted from common.c::MSG_WriteDir()
byte MD2_Normal2Index(const vec3_t vec)
{
	int i, best;
	float d, bestd;
	float x,y,z;

	x = vec[0];
	y = vec[1];
	z = vec[2];

	best = 0;
	// frequently occurring axial cases, use known best index
	if ( x == 0.0f && y == 0.0f )
	{
		best = ( z >= 0.999f ) ? 5  : 84;
	}
	else if ( x == 0.0f && z == 0.0f )
	{
		best = ( y >= 0.999f ) ? 32 : 104;
	}
	else if ( y == 0.0f && z == 0.0f )
	{
		best = ( x >= 0.999f ) ? 52 : 143;
	}
	else
	{ // general case
		bestd = 0.0f;
		for ( i = 0 ; i < NUMVERTEXNORMALS ; i++ )
		{ // search for closest match
			d =	  (x*r_avertexnormals[i][0])
				+ (y*r_avertexnormals[i][1])
				+ (z*r_avertexnormals[i][2]);
			if ( d > 0.998f )
			{ // no other entry could be a closer match
				//  0.9679495 is max dot product between anorm.h entries
				best = i;
				break;
			}
			if ( d > bestd )
			{
				bestd = d;
				best = i;
			}
		}
	}

	return best;
}
#endif

void MD2_RecalcVertsLightNormalIdx (dmdl_t *pheader)
{
	int				i, j, k, l;
	daliasframe_t	*frame;
	dtrivertx_t		*verts, *v;
	vec3_t			normal, triangle[3], v1, v2;
	dtriangle_t		*tris = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);
	vec3_t	normals_[MAX_VERTS];

	//for all frames
	for (i=0; i<pheader->num_frames; i++)
	{
		frame = (daliasframe_t *)((byte *)pheader + pheader->ofs_frames + i * pheader->framesize);
		verts = frame->verts;

		memset(normals_, 0, pheader->num_xyz*sizeof(vec3_t));

		//for all tris
		for (j=0; j<pheader->num_tris; j++)
		{
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

			for (k=0; k<3; k++)
			{
				l = tris[j].index_xyz[k];
				VectorAdd(normals_[l], normal, normals_[l]);
			}
		}

		for (j=0; j<pheader->num_xyz; j++)
			for (k=j+1; k<pheader->num_xyz; k++)
				if(verts[j].v[0] == verts[k].v[0] && verts[j].v[1] == verts[k].v[1] && verts[j].v[2] == verts[k].v[2])
				{
					float *jnormal = r_avertexnormals[verts[j].lightnormalindex];
					float *knormal = r_avertexnormals[verts[k].lightnormalindex];
					if(DotProduct(jnormal, knormal)>=cos(DEG2RAD(45)))
					{
						VectorAdd(normals_[j], normals_[k], normals_[j]);
						VectorCopy(normals_[j], normals_[k]);
					}
				}

		//normalize average
		for (j=0; j<pheader->num_xyz; j++)
		{
			VectorNormalize(normals_[j]);
			verts[j].lightnormalindex = MD2_Normal2Index(normals_[j]);
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
static void MD2_VecsForTris(
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


void MD2_LoadVBO (model_t *mod)
{
	int i, j, k, framenum;
	
	dmdl_t			*paliashdr;
	dtriangle_t		*tris;
	dtrivertx_t		*verts;
	daliasframe_t	*frame;
	byte			*tangents;
	
	paliashdr = (dmdl_t *)mod->extradata;
	tris = (dtriangle_t *) ((byte *)paliashdr + paliashdr->ofs_tris);
	
	for (framenum = 0; framenum < mod->num_frames; framenum++)
	{
		int va = 0; // will eventually reach mod->num_triangles*3
		
		frame = (daliasframe_t *)
			((byte *)paliashdr + paliashdr->ofs_frames + framenum * paliashdr->framesize);
		verts = frame->verts;
		tangents = mod->tangents + paliashdr->num_xyz * framenum;
	
		for (i=0; i<paliashdr->num_tris; i++)
		{
			for (j=0; j<3; j++)
			{
				int index_xyz, index_st;
			
				index_xyz = tris[i].index_xyz[j];
				index_st = tris[i].index_st[j];
			
				for (k = 0; k < 3; k++)
					VertexArray[va][k] = verts[index_xyz].v[k] * frame->scale[k] + frame->translate[k];
			
				VectorCopy (r_avertexnormals[verts[index_xyz].lightnormalindex], NormalsArray[va]);
			
				VectorCopy (r_avertexnormals[tangents[index_xyz]], TangentsArray[va]);
				TangentsArray[va][3] = 1.0;
			
				TexCoordArray[va][0] = mod->st[index_st].s;
				TexCoordArray[va][1] = mod->st[index_st].t;
			
				va++;
			}
		}
	
		vbo_xyz = R_VCLoadData(VBO_STATIC, va*sizeof(vec3_t), VertexArray, VBO_STORE_XYZ+framenum, mod);
		vbo_st = R_VCLoadData(VBO_STATIC, va*sizeof(vec2_t), TexCoordArray, VBO_STORE_ST, mod);
		vbo_normals = R_VCLoadData(VBO_STATIC, va*sizeof(vec3_t), NormalsArray, VBO_STORE_NORMAL+framenum, mod);
		vbo_tangents = R_VCLoadData(VBO_STATIC, va*sizeof(vec4_t), TangentsArray, VBO_STORE_TANGENT+framenum, mod);
	}
}

qboolean MD2_FindVBO (model_t *mod, int framenum)
{
	vbo_xyz = R_VCFindCache(VBO_STORE_XYZ+framenum, mod);
	vbo_st = R_VCFindCache(VBO_STORE_ST, mod);
	vbo_normals = R_VCFindCache(VBO_STORE_NORMAL+framenum, mod);
	vbo_tangents = R_VCFindCache(VBO_STORE_TANGENT+framenum, mod);
	return vbo_xyz && vbo_st && vbo_normals && vbo_tangents;
}

/*
=================
Mod_LoadMD2Model
=================
*/
void Mod_LoadMD2Model (model_t *mod, void *buffer)
{
	int					i, j, k, l;
	dmdl_t				*pinmodel, *pheader, *paliashdr;
	dstvert_t			*pinst, *poutst;
	dtriangle_t			*pintri, *pouttri, *tris;
	daliasframe_t		*pinframe, *poutframe, *pframe;
	int					*pincmd, *poutcmd;
	int					version;
	int					cx;
	float				s, t;
	float				iw, ih;
	fstvert_t			*st;
	daliasframe_t		*frame;
	dtrivertx_t			*verts;
	byte				*tangents;
	vec3_t				tangents_[MAX_VERTS];
	char *pstring;
	int count;
	image_t *image;

	pinmodel = (dmdl_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
		Com_Printf("%s has wrong version number (%i should be %i)",
				 mod->name, version, ALIAS_VERSION);

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
// find neighbours
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

	mod->type = mod_alias;
	mod->num_frames = pheader->num_frames;

	//
	// load the glcmds
	//
	pincmd = (int *) ((byte *)pinmodel + pheader->ofs_glcmds);
	poutcmd = (int *) ((byte *)pheader + pheader->ofs_glcmds);
	for (i=0 ; i<pheader->num_glcmds ; i++)
		poutcmd[i] = LittleLong (pincmd[i]);

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
	mod->st = st = (fstvert_t*)Hunk_Alloc (cx);

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

	MD2_LoadVertexArrays(mod);

	MD2_RecalcVertsLightNormalIdx(pheader);

	cx = pheader->num_xyz * pheader->num_frames * sizeof(byte);

	// Calculate tangents
	mod->tangents = tangents = (byte*)Hunk_Alloc (cx);

	tris = (dtriangle_t *) ((byte *)pheader + pheader->ofs_tris);

	//for all frames
	for (i=0; i<pheader->num_frames; i++)
	{
		//set temp to zero
		memset(tangents_, 0, pheader->num_xyz*sizeof(vec3_t));

		frame = (daliasframe_t *)((byte *)pheader + pheader->ofs_frames + i * pheader->framesize);
		verts = frame->verts;

		//for all tris
		for (j=0; j<pheader->num_tris; j++)
		{
			vec3_t	vv0,vv1,vv2;
			vec3_t tangent;

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
				VectorAdd(tangents_[l], tangent, tangents_[l]);
			}
		}

		for (j=0; j<pheader->num_xyz; j++)
			for (k=j+1; k<pheader->num_xyz; k++)
				if(verts[j].v[0] == verts[k].v[0] && verts[j].v[1] == verts[k].v[1] && verts[j].v[2] == verts[k].v[2])
				{
					float *jnormal = r_avertexnormals[verts[j].lightnormalindex];
					float *knormal = r_avertexnormals[verts[k].lightnormalindex];
					// if(DotProduct(jnormal, knormal)>=cos(DEG2RAD(45)))
					if( DotProduct(jnormal, knormal) >= 0.707106781187 ) // cos of 45 degrees.
					{
						VectorAdd(tangents_[j], tangents_[k], tangents_[j]);
						VectorCopy(tangents_[j], tangents_[k]);
					}
				}

		//normalize averages
		for (j=0; j<pheader->num_xyz; j++)
		{
			VectorNormalize(tangents_[j]);
			tangents[i * pheader->num_xyz + j] = MD2_Normal2Index(tangents_[j]);
		}
	}

	mod->num_triangles = pheader->num_tris;

	paliashdr = (dmdl_t *)mod->extradata;

	//redo this using max/min from all frames
	pframe = ( daliasframe_t * ) ( ( byte * ) paliashdr +
									  paliashdr->ofs_frames);

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
	
	MD2_LoadVBO (mod);
}

//==============================================================
//Rendering functions


//This routine bascially finds the average light position, by factoring in all lights and
//accounting for their distance, visiblity, and intensity.
void R_GetLightVals(vec3_t meshOrigin, qboolean RagDoll)
{
	int i, j, lnum;
	dlight_t	*dl;
	float dist;
	vec3_t	temp, tempOrg, lightAdd;
	trace_t r_trace;
	vec3_t mins, maxs;
	float numlights, nonweighted_numlights, weight;
	float bob;
	qboolean copy;

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	//light shining down if there are no lights at all
	VectorCopy(meshOrigin, lightPosition);
	lightPosition[2] += 128;

	if((currententity->flags & RF_BOBBING) && !RagDoll)
		bob = currententity->bob;
	else
		bob = 0;

	VectorCopy(meshOrigin, tempOrg);
		tempOrg[2] += 24 - bob; //generates more consistent tracing

	numlights = nonweighted_numlights = 0;
	VectorClear(lightAdd);
	statLightIntensity = 0.0;
	
	if (!RagDoll)
	{
		copy = cl_persistent_ents[currententity->number].setlightstuff;
		for (i = 0; i < 3; i++)
		{
			if (fabs(meshOrigin[i] - cl_persistent_ents[currententity->number].oldorigin[i]) > 0.0001)
			{
				copy = false;
				break;
			}
		}
	}
	else
	{
		copy = false;
	}
	
	if (copy)
	{
		numlights =  cl_persistent_ents[currententity->number].oldnumlights;
		VectorCopy ( cl_persistent_ents[currententity->number].oldlightadd, lightAdd);
		statLightIntensity = cl_persistent_ents[currententity->number].oldlightintens;
	}
	else
	{
		for (i=0; i<r_lightgroups; i++)
		{
			if(!RagDoll && (currententity->flags & RF_WEAPONMODEL) && (LightGroups[i].group_origin[2] > meshOrigin[2]))
			{
				r_trace.fraction = 1.0; //don't do traces for lights above weapon models, not smooth enough
			}
			else
			{
				if (CM_inPVS (tempOrg, LightGroups[i].group_origin))
					r_trace = CM_BoxTrace(tempOrg, LightGroups[i].group_origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
				else
					r_trace.fraction = 0.0;
			}
				

			if(r_trace.fraction == 1.0)
			{
				VectorSubtract(meshOrigin, LightGroups[i].group_origin, temp);
				dist = VectorLength(temp);
				if(dist == 0)
					dist = 1;
				dist = dist*dist;
				weight = (int)250000/(dist/(LightGroups[i].avg_intensity+1.0f));
				for(j = 0; j < 3; j++)
				{
					lightAdd[j] += LightGroups[i].group_origin[j]*weight;
				}
				statLightIntensity += LightGroups[i].avg_intensity;
				numlights+=weight;
				nonweighted_numlights++;
			}
		}
		
		if (numlights > 0.0)
			statLightIntensity /= (float)nonweighted_numlights;
		
		cl_persistent_ents[currententity->number].oldnumlights = numlights;
		VectorCopy (lightAdd, cl_persistent_ents[currententity->number].oldlightadd);
		cl_persistent_ents[currententity->number].setlightstuff = true;
		VectorCopy (currententity->origin, cl_persistent_ents[currententity->number].oldorigin);
		cl_persistent_ents[currententity->number].oldlightintens = statLightIntensity;
	}

	if(numlights > 0.0) {
		for(i = 0; i < 3; i++)
		{
			statLightPosition[i] = lightAdd[i]/numlights;
		}
	}
	
	dynFactor = 0;
	if(gl_dynamic->integer != 0)
	{
		dl = r_newrefdef.dlights;
		//limit to five lights(maybe less)?
		for (lnum=0; lnum<(r_newrefdef.num_dlights > 5 ? 5: r_newrefdef.num_dlights); lnum++, dl++)
		{
			VectorSubtract(meshOrigin, dl->origin, temp);
			dist = VectorLength(temp);

			VectorCopy(meshOrigin, temp);
			temp[2] += 24; //generates more consistent tracing

			r_trace = CM_BoxTrace(temp, dl->origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);

			if(r_trace.fraction == 1.0)
			{
				if(dist < dl->intensity)
				{
					//make dynamic lights more influential than world
					for(j = 0; j < 3; j++)
						lightAdd[j] += dl->origin[j]*10*dl->intensity;
					numlights+=10*dl->intensity;

					VectorSubtract (dl->origin, meshOrigin, temp);
					dynFactor += (dl->intensity/20.0)/VectorLength(temp);
				}
			}
		}
	}

	if(numlights > 0.0) {
		for(i = 0; i < 3; i++)
			lightPosition[i] = lightAdd[i]/numlights;
	}
}

void R_ModelViewTransform(const vec3_t in, vec3_t out){
	const float *v = in;
	const float *m = r_world_matrix;

	out[0] = m[0] * v[0] + m[4] * v[1] + m[8]  * v[2] + m[12];
	out[1] = m[1] * v[0] + m[5] * v[1] + m[9]  * v[2] + m[13];
	out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
}


/*
** MD2_CullModel
*/
qboolean MD2_CullModel( void )
{
	int i;
	vec3_t	vectors[3];
	vec3_t  angles;
	vec3_t	dist;
	vec3_t bbox[8];

	VectorSubtract(r_origin, currententity->origin, dist);

	/*
	** rotate the bounding box
	*/
	VectorCopy( currententity->angles, angles );
	angles[YAW] = -angles[YAW];
	AngleVectors( angles, vectors[0], vectors[1], vectors[2] );

	for ( i = 0; i < 8; i++ )
	{
		vec3_t tmp;

		VectorCopy( currentmodel->bbox[i], tmp );

		bbox[i][0] = DotProduct( vectors[0], tmp );
		bbox[i][1] = -DotProduct( vectors[1], tmp );
		bbox[i][2] = DotProduct( vectors[2], tmp );

		VectorAdd( currententity->origin, bbox[i], bbox[i] );
	}

	{
		int p, f, aggregatemask = ~0;

		for ( p = 0; p < 8; p++ )
		{
			int mask = 0;

			for ( f = 0; f < 4; f++ )
			{
				float dp = DotProduct( frustum[f].normal, bbox[p] );

				if ( ( dp - frustum[f].dist ) < 0 )
				{
					mask |= ( 1 << f );
				}
			}

			aggregatemask &= mask;
		}

		if ( aggregatemask && (VectorLength(dist) > 150)) //so shadows don't blatantly disappear when out of frustom
		{
			return true;
		}

		return false;
	}
}

// should be able to handle all mesh types
static qboolean R_Mesh_CullModel (void)
{
	if((r_newrefdef.rdflags & RDF_NOWORLDMODEL ) && !(currententity->flags & RF_MENUMODEL))
		return true;
	
	if (currententity->team) //don't draw flag models, handled by sprites
		return true;
	
	if (!cl_gun->integer && (currententity->flags & RF_WEAPONMODEL))
		return true;
	
	if ((currententity->flags & RF_WEAPONMODEL))
		return r_lefthand->integer == 2;
	
	if (r_worldmodel) {
		//occulusion culling - why draw entities we cannot see?
		trace_t r_trace = CM_BoxTrace(r_origin, currententity->origin, currentmodel->maxs, currentmodel->mins, r_worldmodel->firstnode, MASK_OPAQUE);
		if(r_trace.fraction != 1.0)
			return true;
	}
	
	if (currentmodel->type == mod_alias)
		return MD2_CullModel ();
	else if (currentmodel->type == mod_iqm)
		return IQM_CullModel ();
	// New model types go here
}

static void R_Mesh_SetupShellRender (qboolean ragdoll, vec3_t lightcolor, qboolean fragmentshader, float alpha)
{
	int i;
	vec3_t lightVec, lightVal;
	
	//send light level and color to shader, ramp up a bit
	VectorCopy(lightcolor, lightVal);
	 for(i = 0; i < 3; i++) 
	 {
		if(lightVal[i] < shadelight[i]/2)
			lightVal[i] = shadelight[i]/2; //never go completely black
		lightVal[i] *= 5;
		lightVal[i] += dynFactor;
		if(lightVal[i] > 1.0+dynFactor)
			lightVal[i] = 1.0+dynFactor;
	}
	
	//brighten things slightly
	for (i = 0; i < 3; i++ )
		lightVal[i] *= (ragdoll?1.25:2.5);
			   
	//simple directional(relative light position)
	VectorSubtract(lightPosition, currententity->origin, lightVec);
	VectorMA(lightPosition, 1.0, lightVec, lightPosition);
	R_ModelViewTransform(lightPosition, lightVec);

	if (ragdoll)
		qglDepthMask(false);

	if (fragmentshader)
		glUseProgramObjectARB( g_meshprogramObj );
	else
		glUseProgramObjectARB( g_vertexonlymeshprogramObj );

	glUniform3fARB( MESH_UNIFORM(meshlightPosition), lightVec[0], lightVec[1], lightVec[2]);
	
	KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER);

	GL_MBind (0, r_shelltexture2->texnum);
	glUniform1iARB( MESH_UNIFORM(baseTex), 0);

	if (fragmentshader)
	{
		GL_MBind (1, r_shellnormal->texnum);
		glUniform1iARB( MESH_UNIFORM(normTex), 1);
	}

	glUniform1iARB( MESH_UNIFORM(useFX), 0);

	glUniform1iARB( MESH_UNIFORM(useGlow), 0);

	glUniform1iARB( MESH_UNIFORM(useCube), 0);

	glUniform1fARB( MESH_UNIFORM(useShell), ragdoll?1.6:0.4);

	glUniform3fARB( MESH_UNIFORM(color), lightVal[0], lightVal[1], lightVal[2]);

	glUniform1fARB( MESH_UNIFORM(meshTime), rs_realtime);

	glUniform1iARB( MESH_UNIFORM(meshFog), map_fog);
	
	// set up the fixed-function pipeline too
	if (!fragmentshader)
		qglColor4f( shadelight[0], shadelight[1], shadelight[2], alpha);
	
	GLSTATE_ENABLE_BLEND
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void R_Mesh_SetupStandardRender (int skinnum, rscript_t *rs, vec3_t lightcolor, qboolean fragmentshader)
{
	int i;
	
	//render with shaders - assume correct single pass glsl shader struct(let's put some checks in for this)
	vec3_t lightVec, lightVal;

	GLSTATE_ENABLE_ALPHATEST		

	//send light level and color to shader, ramp up a bit
	VectorCopy(lightcolor, lightVal);
	for(i = 0; i < 3; i++)
	{
		if(lightVal[i] < shadelight[i]/2)
			lightVal[i] = shadelight[i]/2; //never go completely black
		lightVal[i] *= 5;
		lightVal[i] += dynFactor;
		if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		{
			if(lightVal[i] > 1.5)
				lightVal[i] = 1.5;
		}
		else
		{
			if(lightVal[i] > 1.0+dynFactor)
				lightVal[i] = 1.0+dynFactor;
		}
	}
	
	if(r_newrefdef.rdflags & RDF_NOWORLDMODEL) //menu model
	{
		//fixed light source pointing down, slightly forward and to the left
		lightPosition[0] = -25.0;
		lightPosition[1] = 300.0;
		lightPosition[2] = 400.0;
		VectorMA(lightPosition, 5.0, lightVec, lightPosition);
		R_ModelViewTransform(lightPosition, lightVec);

		for (i = 0; i < 3; i++ )
		{
			lightVal[i] = 1.1;
		}
	}
	else
	{
		//simple directional(relative light position)
		VectorSubtract(lightPosition, currententity->origin, lightVec);
		VectorMA(lightPosition, 5.0, lightVec, lightPosition);
		R_ModelViewTransform(lightPosition, lightVec);

		//brighten things slightly
		for (i = 0; i < 3; i++ )
		{
			lightVal[i] *= 1.05;
		}
	}

	if (fragmentshader)
		glUseProgramObjectARB( g_meshprogramObj );
	else
		glUseProgramObjectARB( g_vertexonlymeshprogramObj );

	glUniform3fARB( MESH_UNIFORM(meshlightPosition), lightVec[0], lightVec[1], lightVec[2]);
	glUniform3fARB( MESH_UNIFORM(color), lightVal[0], lightVal[1], lightVal[2]);
	
	KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER | KILL_TMU2_POINTER);

	GL_MBind (0, skinnum);
	glUniform1iARB( MESH_UNIFORM(baseTex), 0);

	if (fragmentshader)
	{
		GL_MBind (1, rs->stage->texture->texnum);
		glUniform1iARB( MESH_UNIFORM(normTex), 1);

		GL_MBind (2, rs->stage->texture2->texnum);
		glUniform1iARB( MESH_UNIFORM(fxTex), 2);

		GL_MBind (3, rs->stage->texture3->texnum);
		glUniform1iARB( MESH_UNIFORM(fx2Tex), 3);
	}

	if(fragmentshader && rs->stage->fx)
		glUniform1iARB( MESH_UNIFORM(useFX), 1);
	else
		glUniform1iARB( MESH_UNIFORM(useFX), 0);

	if(fragmentshader && rs->stage->glow)
		glUniform1iARB( MESH_UNIFORM(useGlow), 1);
	else
		glUniform1iARB( MESH_UNIFORM(useGlow), 0);

	glUniform1fARB( MESH_UNIFORM(useShell), 0.0);	

	if(fragmentshader && rs->stage->cube)
	{
		glUniform1iARB( MESH_UNIFORM(useCube), 1);
		if(currententity->flags & RF_WEAPONMODEL)
			glUniform1iARB( MESH_UNIFORM(fromView), 1);
		else
			glUniform1iARB( MESH_UNIFORM(fromView), 0);
	}
	else
		glUniform1iARB( MESH_UNIFORM(useCube), 0);

	glUniform1fARB( MESH_UNIFORM(meshTime), rs_realtime);

	glUniform1iARB( MESH_UNIFORM(meshFog), map_fog);
}

void R_Mesh_SetupGlassRender (void)
{
	vec3_t lightVec, left, up;
	int type;
	qboolean mirror, glass;
	
	if ((r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		glass = true;
	else if(gl_mirror->integer)
		mirror = true;
	else
		glass = true;
	
	if (mirror && !(currententity->flags & RF_WEAPONMODEL))
		glass = true;
	
	glUseProgramObjectARB( g_glassprogramObj );
	
	R_ModelViewTransform(lightPosition, lightVec);
	glUniform3fARB( g_location_g_lightPos, lightVec[0], lightVec[1], lightVec[2]);

	AngleVectors (currententity->angles, NULL, left, up);
	glUniform3fARB( g_location_g_left, left[0], left[1], left[2]);
	glUniform3fARB( g_location_g_up, up[0], up[1], up[2]);
	
	type = 0;
	
	if (glass)
	{
		glUniform1iARB( g_location_g_refTexture, 0);
		GL_MBind (0, r_mirrorspec->texnum);
		type |= 2;
	}
	
	if (mirror)
	{
		glUniform1iARB( g_location_g_mirTexture, 1);
		GL_MBind (1, r_mirrortexture->texnum);
		type |= 1;
	}
	
	glUniform1iARB( g_location_g_type, type);
	
	glUniform1iARB( g_location_g_fog, map_fog);
	
	GLSTATE_ENABLE_BLEND
	qglBlendFunc (GL_ONE, GL_ONE);
}

void MD2_DrawVBO (qboolean lerped)
{
	if (!MD2_FindVBO (currentmodel, currententity->frame))
	{
		// TODO: remove this - the VBOs are getting unloaded for every new map
		MD2_LoadVBO (currentmodel);
		MD2_FindVBO (currentmodel, currententity->frame);
	}
	
	qglEnableClientState( GL_VERTEX_ARRAY );
	GL_BindVBO(vbo_xyz);
	qglVertexPointer(3, GL_FLOAT, 0, 0);
	
	KillFlags |= KILL_TMU0_POINTER;
	qglClientActiveTextureARB (GL_TEXTURE0);
	qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
	GL_BindVBO(vbo_st);
	qglTexCoordPointer(2, GL_FLOAT, 0, 0);
	
	KillFlags |= KILL_NORMAL_POINTER;
	qglEnableClientState( GL_NORMAL_ARRAY );
	GL_BindVBO(vbo_normals);
	qglNormalPointer(GL_FLOAT, 0, 0);

	glEnableVertexAttribArrayARB (ATTR_TANGENT_IDX);
	GL_BindVBO(vbo_tangents);
	glVertexAttribPointerARB(ATTR_TANGENT_IDX, 4, GL_FLOAT, GL_FALSE, sizeof(vec4_t), 0);
	
	if (lerped)
	{
		MD2_FindVBO (currentmodel, currententity->oldframe);
		
		glEnableVertexAttribArrayARB (ATTR_OLDVTX_IDX);
		GL_BindVBO (vbo_xyz);
		glVertexAttribPointerARB (ATTR_OLDVTX_IDX, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_t), 0);
		
		glEnableVertexAttribArrayARB (ATTR_OLDNORM_IDX);
		GL_BindVBO (vbo_normals);
		glVertexAttribPointerARB (ATTR_OLDNORM_IDX, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_t), 0);
		
		glEnableVertexAttribArrayARB (ATTR_OLDTAN_IDX);
		GL_BindVBO(vbo_tangents);
		glVertexAttribPointerARB(ATTR_OLDTAN_IDX, 4, GL_FLOAT, GL_FALSE, sizeof(vec4_t), 0);
	}
	
	GL_BindVBO (NULL);
	
	if (!(!cl_gun->integer && ( currententity->flags & RF_WEAPONMODEL )))
		qglDrawArrays (GL_TRIANGLES, 0, currentmodel->num_triangles*3);
	
	R_KillVArrays ();
	
	glDisableVertexAttribArrayARB (ATTR_TANGENT_IDX);
	if (lerped)
	{
		glDisableVertexAttribArrayARB (ATTR_OLDVTX_IDX);
		glDisableVertexAttribArrayARB (ATTR_OLDNORM_IDX);
		glDisableVertexAttribArrayARB (ATTR_OLDTAN_IDX);
	}
}

void IQM_DrawVBO (void);

/*
=============
R_Mesh_DrawFrame: should be able to handle all types of meshes.
=============
*/
void R_Mesh_DrawFrame (int skinnum, qboolean ragdoll, float shellAlpha)
{
	int			i;
	vec3_t		lightcolor;
	
	// only applicable to MD2
	qboolean	lerped;
	float		frontlerp;
	
	lerped = currententity->backlerp != 0.0 && (currententity->frame != 0 || currentmodel->num_frames != 1);
	
	VectorCopy(shadelight, lightcolor);
	for (i=0;i<model_dlights_num;i++)
		VectorAdd(lightcolor, model_dlights[i].color, lightcolor);
	VectorNormalize(lightcolor);

	if(lerped) 
		frontlerp = 1.0 - currententity->backlerp;

	if ((currententity->flags & RF_TRANSLUCENT) && !(currententity->flags & RF_SHELL_ANY))
	{
		qglDepthMask(false);
		
		R_Mesh_SetupGlassRender ();
		
		if (currentmodel->type == mod_alias)
		{
			glUniform1iARB (g_location_g_useGPUanim, lerped?2:0);
			glUniform1fARB (g_location_g_lerp, frontlerp);
		}
		else if (currentmodel->type == mod_iqm)
		{
			glUniform1iARB (g_location_g_useGPUanim, 1);
			glUniformMatrix3x4fvARB (g_location_g_outframe, currentmodel->num_joints, GL_FALSE, (const GLfloat *) currentmodel->outframe );
		}
		// New model types go here
	}
	else
	{
		qboolean fragmentshader;
		rscript_t *rs = NULL;
		
		if (r_shaders->integer)
			rs=(rscript_t *)currententity->script;
		
		if(rs && rs->stage->depthhack)
			qglDepthMask(false);
		
		if (currententity->flags & RF_SHELL_ANY)
		{
			fragmentshader = gl_normalmaps->integer;
			R_Mesh_SetupShellRender (ragdoll, lightcolor, fragmentshader, shellAlpha);
		}
		else
		{
			fragmentshader = rs && rs->stage->normalmap && gl_normalmaps->integer;
			R_Mesh_SetupStandardRender (skinnum, rs, lightcolor, fragmentshader);
		}
		
		if (currentmodel->type == mod_alias)
		{
			glUniform1iARB (MESH_UNIFORM(useGPUanim), lerped?2:0);
			glUniform1fARB (MESH_UNIFORM(lerp), frontlerp);
		}
		else if (currentmodel->type == mod_iqm)
		{
			glUniform1iARB(MESH_UNIFORM(useGPUanim), 1);
			glUniformMatrix3x4fvARB( MESH_UNIFORM(outframe), currentmodel->num_joints, GL_FALSE, (const GLfloat *) currentmodel->outframe );
		}
		// New model types go here
	}
	
	if (currentmodel->type == mod_alias)
		MD2_DrawVBO (lerped);
	else if (currentmodel->type == mod_iqm)
		IQM_DrawVBO ();
	// New model types go here
	
	glUseProgramObjectARB( 0 );
	qglDepthMask(true);

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_TEXGEN
	
	// FIXME: make this unnecessary
	// Without this, the player options menu goes all funny due to
	// R_Mesh_SetupGlassRender changing the blendfunc. The proper solution is
	// to just never rely on the blendfunc being any default value.
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

void R_Mesh_SetShadelight (void)
{
	int i;
	
	if ( currententity->flags & RF_SHELL_ANY )
	{
		VectorClear (shadelight);
		if (currententity->flags & RF_SHELL_HALF_DAM)
		{
				shadelight[0] = 0.56;
				shadelight[1] = 0.59;
				shadelight[2] = 0.45;
		}
		if ( currententity->flags & RF_SHELL_DOUBLE )
		{
			shadelight[0] = 0.9;
			shadelight[1] = 0.7;
		}
		if ( currententity->flags & RF_SHELL_RED )
			shadelight[0] = 1.0;
		if ( currententity->flags & RF_SHELL_GREEN )
		{
			shadelight[1] = 1.0;
			shadelight[2] = 0.6;
		}
		if ( currententity->flags & RF_SHELL_BLUE )
		{
			shadelight[2] = 1.0;
			shadelight[0] = 0.6;
		}
	}
	else if (currententity->flags & RF_FULLBRIGHT)
	{
		for (i=0 ; i<3 ; i++)
			shadelight[i] = 1.0;
	}
	else
	{
		R_LightPoint (currententity->origin, shadelight, true);
	}
	if ( currententity->flags & RF_MINLIGHT )
	{
		float minlight;

		if(gl_normalmaps->integer)
			minlight = 0.1;
		else
			minlight = 0.2;
		for (i=0 ; i<3 ; i++)
			if (shadelight[i] > minlight)
				break;
		if (i == 3)
		{
			shadelight[0] = minlight;
			shadelight[1] = minlight;
			shadelight[2] = minlight;
		}
	}

	if ( currententity->flags & RF_GLOW )
	{	// bonus items will pulse with time
		float	scale;
		float	minlight;

		scale = 0.2 * sin(r_newrefdef.time*7);
		if(gl_normalmaps->integer)
			minlight = 0.1;
		else
			minlight = 0.2;
		for (i=0 ; i<3 ; i++)
		{
			shadelight[i] += scale;
			if (shadelight[i] < minlight)
				shadelight[i] = minlight;
		}
	}
}

void MD2_SelectFrame (void)
{
	dmdl_t *paliashdr = (dmdl_t *)currentmodel->extradata;
	
	if ( (currententity->frame >= paliashdr->num_frames)
		|| (currententity->frame < 0) )
	{
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ( (currententity->oldframe >= paliashdr->num_frames)
		|| (currententity->oldframe < 0))
	{
		currententity->frame = 0;
		currententity->oldframe = 0;
	}

	if ( !r_lerpmodels->integer )
		currententity->backlerp = 0;
}

/*
=================
R_Mesh_Draw - animate and render a mesh. Should support all mesh types.
=================
*/
void R_Mesh_Draw ( void )
{
	image_t		*skin;

	if ( R_Mesh_CullModel () )
		return;
	
	R_GetLightVals(currententity->origin, false);

	if (currentmodel->type == mod_iqm) //TODO: use this for MD2 as well
		R_GenerateEntityShadow();
	
	// Don't render your own avatar unless it's for shadows
	if ((currententity->flags & RF_VIEWERMODEL))
		return;
	
	if (currentmodel->type == mod_alias)
		MD2_SelectFrame ();
	else if (currentmodel->type == mod_iqm)
		IQM_AnimateFrame ();
	// New model types go here

	R_Mesh_SetShadelight ();

	if (!(currententity->flags & RF_WEAPONMODEL))
		c_alias_polys += currentmodel->num_triangles; /* for rspeed_epoly count */

	if (currententity->flags & RF_DEPTHHACK) // hack the depth range to prevent view model from poking into walls
		qglDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));

	if (currententity->flags & RF_WEAPONMODEL)
	{
		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();

		if (r_lefthand->integer == 1)
		{
			qglScalef(-1, 1, 1);
			qglCullFace(GL_BACK);
		}
		if(r_newrefdef.fov_y < 75.0f)
			MYgluPerspective(r_newrefdef.fov_y, (float)r_newrefdef.width / (float)r_newrefdef.height, 4.0f, 4096.0f);
		else
			MYgluPerspective(75.0f, (float)r_newrefdef.width / (float)r_newrefdef.height, 4.0f, 4096.0f);

		qglMatrixMode(GL_MODELVIEW);
	}

	if (currententity->flags & RF_WEAPONMODEL || currentmodel->type == mod_alias)
	{
		qglPushMatrix ();
		currententity->angles[PITCH] = -currententity->angles[PITCH];	// sigh.
		R_RotateForEntity (currententity);
		currententity->angles[PITCH] = -currententity->angles[PITCH];	// sigh.
	}
	else if (currentmodel->type == mod_iqm)
	{
		qglPushMatrix ();
		currententity->angles[PITCH] = currententity->angles[ROLL] = 0;
		R_RotateForEntity (currententity);
	}

	// select skin
	if (currententity->skin) 
		skin = currententity->skin;
	else
		skin = currentmodel->skins[0];
	if (!skin)
		skin = r_notexture;	// fallback..
	
	//check for valid script
	if(currententity->script && currententity->script->stage)
	{
		if(!strcmp("***r_notexture***", currententity->script->stage->texture->name) || 
			((currententity->script->stage->fx || currententity->script->stage->glow) && !strcmp("***r_notexture***", currententity->script->stage->texture2->name)) ||
			(currententity->script->stage->cube && !strcmp("***r_notexture***", currententity->script->stage->texture3->name)))
		{
			currententity->script = NULL; //bad shader!
		}
	}
	
	GL_SelectTexture (0);
	qglShadeModel (GL_SMOOTH);
	GL_TexEnv( GL_MODULATE );

	R_Mesh_DrawFrame(skin->texnum, false, 0.33);

	GL_SelectTexture (0);
	GL_TexEnv( GL_REPLACE );
	qglShadeModel (GL_FLAT);

	qglPopMatrix ();
	
	if (currententity->flags & RF_WEAPONMODEL)
	{
		qglMatrixMode( GL_PROJECTION );
		qglPopMatrix();
		qglMatrixMode( GL_MODELVIEW );
		qglCullFace( GL_FRONT );
	}

	if (currententity->flags & RF_DEPTHHACK)
		qglDepthRange (gldepthmin, gldepthmax);
	
	qglColor4f (1,1,1,1);

	if(r_minimap->integer)
	{
	   if ( currententity->flags & RF_MONSTER)
	   {
			RadarEnts[numRadarEnts].color[0]= 1.0;
			RadarEnts[numRadarEnts].color[1]= 0.0;
			RadarEnts[numRadarEnts].color[2]= 2.0;
			RadarEnts[numRadarEnts].color[3]= 1.0;
		}
		else
			return;

		VectorCopy(currententity->origin,RadarEnts[numRadarEnts].org);
		numRadarEnts++;
	}
}

void MD2_DrawCasterFrame (float backlerp, qboolean lerped)
{
	glUseProgramObjectARB( g_blankmeshprogramObj );
	
	glUniform1iARB(g_location_bm_useGPUanim, lerped?2:0);

	if (!MD2_FindVBO (currentmodel, currententity->frame))
	{
		// TODO: remove this - the VBOs are getting unloaded for every new map
		MD2_LoadVBO (currentmodel);
		MD2_FindVBO (currentmodel, currententity->frame);
	}
	
	qglEnableClientState( GL_VERTEX_ARRAY );
	GL_BindVBO(vbo_xyz);
	qglVertexPointer(3, GL_FLOAT, 0, 0);
	
	if (lerped)
	{
		MD2_FindVBO (currentmodel, currententity->oldframe);
		
		glEnableVertexAttribArrayARB (ATTR_OLDVTX_IDX);
		GL_BindVBO (vbo_xyz);
		glVertexAttribPointerARB (ATTR_OLDVTX_IDX, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_t), 0);
		
		glUniform1fARB(g_location_bm_lerp, 1.0-backlerp);
	}
	
	qglDrawArrays (GL_TRIANGLES, 0, currentmodel->num_triangles*3);
	
	glUseProgramObjectARB( 0 );
	
	if (lerped)
		glDisableVertexAttribArrayARB (ATTR_OLDVTX_IDX);
}

//to do - alpha and alphamasks possible?
void MD2_DrawCaster ( void )
{
	if ( currententity->flags & RF_WEAPONMODEL ) //don't draw weapon model shadow casters
		return;

	if ( currententity->flags & RF_SHELL_ANY ) //no shells
		return;

	if (!(currententity->flags & RF_VIEWERMODEL) && R_Mesh_CullModel ())
		return;

	// draw it

	qglPushMatrix ();
	currententity->angles[PITCH] = -currententity->angles[PITCH];
	R_RotateForEntity (currententity);
	currententity->angles[PITCH] = -currententity->angles[PITCH];
	
	MD2_SelectFrame ();

	if(currententity->frame == 0 && currentmodel->num_frames == 1)
		MD2_DrawCasterFrame(0, false);
	else
		MD2_DrawCasterFrame(currententity->backlerp, true);

	qglPopMatrix();
}
