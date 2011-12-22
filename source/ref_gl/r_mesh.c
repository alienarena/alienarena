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

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

#define NUMVERTEXNORMALS	162

float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

static  vec4_t  s_lerped[MAX_VERTS];

static vec3_t VertexArray[MAX_VERTICES];
static vec2_t TexCoordArray[MAX_VERTICES];
static vec3_t NormalsArray[MAX_VERTICES];
static vec4_t TangentsArray[MAX_VERTICES];

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

float	*shadedots = r_avertexnormal_dots[0];

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

#if 0
	// register all skins
	memcpy ((char *)pheader + pheader->ofs_skins, (char *)pinmodel + pheader->ofs_skins,
		pheader->num_skins*MAX_SKINNAME);
	for (i=0 ; i<pheader->num_skins ; i++)
		mod->skins[i] = GL_FindImage ((char *)pheader + pheader->ofs_skins + i*MAX_SKINNAME, it_skin);
#else
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
	}
#endif

	// load script
	if(pheader->num_skins)
	{
		char rs[MAX_OSPATH];

		strcpy(rs,(char *)pinmodel + LittleLong(pinmodel->ofs_skins));

		rs[strlen(rs)-4]=0;

		mod->script = RS_FindScript(rs);

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
}

//==============================================================
//Rendering functions

/*
=============
MD2_VlightModel

Vertex lighting for Alias models

Contrast has been added by finding a threshold point, and scaling values on either side in
opposite directions.  This gives the shading a more prounounced, defined look.

=============
*/
#if 0
void MD2_VlightModel (vec3_t baselight, dtrivertx_t *verts, vec3_t lightOut)
{
    int i;
    float l;

    l = shadedots[verts->lightnormalindex];
    VectorScale(baselight, l, lightOut);

    for (i=0; i<3; i++)
    {
		//add contrast - lights lighter, darks darker
		lightOut[i] += (lightOut[i] - 0.25);

		//keep values sane
        if (lightOut[i]<0) lightOut[i] = 0;
        if (lightOut[i]>1) lightOut[i] = 1;
    }
}
#else
/* Profiling shows this is a "hotspot". Calculation rearranged.
 * Leave original above for reference.
 */
void MD2_VlightModel( const vec3_t baselight, dtrivertx_t *verts, vec3_t lightOut )
{
	float l;
	float c;

	l = shadedots[verts->lightnormalindex];

	c = baselight[0] * l;
	c += ( c - 0.25 );  // [0,1] => [-0.25,1.75], then clamp to [0,1]
	c = c < 0.0f ? 0.0f : ( c > 1.0f ? 1.0f : c );
	lightOut[0] = c;

	c = baselight[1] * l;
	c += ( c - 0.25 );
	c = c < 0.0f ? 0.0f : ( c > 1.0f ? 1.0f : c );
	lightOut[1] = c;

	c = baselight[2] * l;
	c += ( c - 0.25 );
	c = c < 0.0f ? 0.0f : ( c > 1.0f ? 1.0f : c );
	lightOut[2] = c;

}
#endif

void MD2_LerpSelfShadowVerts( int nverts, dtrivertx_t *v, dtrivertx_t *ov, float *lerp, float move[3], float frontv[3], float backv[3] )
{
    int i;
    for (i=0 ; i < nverts; i++, v++, ov++, lerp+=4)
        {
            lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0];
            lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1];
            lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2];
        }
}

float calcEntAlpha (float alpha, vec3_t point)
{
	float newAlpha;
	vec3_t vert_len;

	newAlpha = alpha;

	if (!(currententity->flags&RF_TRANSLUCENT))
	{
		if (newAlpha<0) newAlpha = 0;
		if (newAlpha>1) newAlpha = 1;
		return newAlpha;
	}

	VectorSubtract(r_newrefdef.vieworg, point, vert_len);
	newAlpha *= VectorLength(vert_len);
	if (newAlpha>alpha)	newAlpha = alpha;

	if (newAlpha<0) newAlpha = 0;
	if (newAlpha>1) newAlpha = 1;

	return newAlpha;
}

//This routine bascially finds the average light position, by factoring in all lights and
//accounting for their distance, visiblity, and intensity.
void R_GetLightVals(vec3_t meshOrigin, qboolean RagDoll, qboolean dynamic)
{
	int i, j, lnum;
	dlight_t	*dl;
	float dist;
	vec3_t	temp, tempOrg, lightAdd;
	trace_t r_trace;
	vec3_t mins, maxs;
	float numlights, weight;
	float bob;

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

	numlights = 0;
	VectorClear(lightAdd);
	for (i=0; i<r_lightgroups; i++)
	{
		if(!RagDoll && (currententity->flags & RF_WEAPONMODEL) && (LightGroups[i].group_origin[2] > meshOrigin[2]))
			r_trace.fraction = 1.0; //don't do traces for lights above weapon models, not smooth enough
		else
			r_trace = CM_BoxTrace(tempOrg, LightGroups[i].group_origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);

		if(r_trace.fraction == 1.0)
		{
			VectorSubtract(meshOrigin, LightGroups[i].group_origin, temp);
			dist = VectorLength(temp);
			if(dist == 0)
				dist = 1;
			dist = dist*dist;
			weight = (int)250000/(dist/(LightGroups[i].avg_intensity+1.0f));
			for(j = 0; j < 3; j++)
				lightAdd[j] += LightGroups[i].group_origin[j]*weight;
			numlights+=weight;
		}
	}

	dynFactor = 0;
	if(dynamic)
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
						lightAdd[j] += dl->origin[j]*100*dl->intensity;
					numlights+=100*dl->intensity;

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
static qboolean MD2_CullModel( vec3_t bbox[8] )
{
	int i;
	vec3_t	vectors[3];
	vec3_t  angles;
	trace_t r_trace;
	vec3_t	dist;

	if (r_worldmodel ) {
		//occulusion culling - why draw entities we cannot see?

		r_trace = CM_BoxTrace(r_origin, currententity->origin, currentmodel->maxs, currentmodel->mins, r_worldmodel->firstnode, MASK_OPAQUE);
		if(r_trace.fraction != 1.0)
			return true;
	}

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

/*
=============
MD2_DrawFrame - standard md2 rendering
=============
*/
void MD2_DrawFrame (dmdl_t *paliashdr, float backlerp, qboolean lerped, int skinnum)
{
	daliasframe_t	*frame, *oldframe=NULL;
	dtrivertx_t	*v, *ov=NULL, *verts;
	dtriangle_t		*tris;
	float	frontlerp;
	float	alpha, basealpha;
	vec3_t	move, delta, vectors[3];
	vec3_t	frontv, backv;
	int		i, j;
	int		index_xyz, index_st;
	qboolean depthmaskrscipt = false;
	rscript_t *rs = NULL;
	rs_stage_t *stage = NULL;
	int		va = 0;
	float shellscale;
	vec3_t lightcolor;
	float   *lerp;
	fstvert_t *st;
	float os, ot, os2, ot2;
	unsigned offs, offs2;
	byte *tangents, *oldtangents = NULL;
	qboolean mirror = false;

	offs = paliashdr->num_xyz;

	if(lerped)
		frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
			+ currententity->frame * paliashdr->framesize);
	else
		frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames);

	verts = v = frame->verts;
	offs2 = offs*currententity->frame;
	tangents = currentmodel->tangents + offs2;

	if(lerped) {
		oldframe = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
			+ currententity->oldframe * paliashdr->framesize);
		ov = oldframe->verts;

		offs2 = offs*currententity->oldframe;
		oldtangents = currentmodel->tangents + offs2;
	}

	tris = (dtriangle_t *) ((byte *)paliashdr + paliashdr->ofs_tris);

	st = currentmodel->st;

	if (r_shaders->value)
			rs=(rscript_t *)currententity->script;

	VectorCopy(shadelight, lightcolor);
	for (i=0;i<model_dlights_num;i++)
		VectorAdd(lightcolor, model_dlights[i].color, lightcolor);
	VectorNormalize(lightcolor);

	if (currententity->flags & RF_TRANSLUCENT) {
		basealpha = alpha = currententity->alpha;

		if(rs_glass)
			rs=(rscript_t *)rs_glass;
		if(!rs)
			GL_Bind(r_reflecttexture->texnum);
		else if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL)) {
			if(gl_mirror->value)
				mirror = true;
		}
	}
	else
		basealpha = alpha = 1.0;

	if(lerped) {
		frontlerp = 1.0 - backlerp;

		// move should be the delta back to the previous frame * backlerp
		VectorSubtract (currententity->oldorigin, currententity->origin, delta);
	}

	AngleVectors (currententity->angles, vectors[0], vectors[1], vectors[2]);

	if(lerped) {
		move[0] = DotProduct (delta, vectors[0]);	// forward
		move[1] = -DotProduct (delta, vectors[1]);	// left
		move[2] = DotProduct (delta, vectors[2]);	// up

		VectorAdd (move, oldframe->translate, move);

		for (i=0 ; i<3 ; i++)
		{
			move[i] = backlerp*move[i] + frontlerp*frame->translate[i];
			frontv[i] = frontlerp*frame->scale[i];
			backv[i] = backlerp*oldframe->scale[i];
		}

		if(currententity->flags & RF_VIEWERMODEL) { //lerp the vertices for self shadows, and leave
			if(gl_shadows->value && !gl_shadowmaps->value) {
				lerp = s_lerped[0];
				MD2_LerpSelfShadowVerts( paliashdr->num_xyz, v, ov, lerp, move, frontv, backv);
				return;
			}
			else
				return;
		}
	}

	if(( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) ) )
	{
		qglColor4f( shadelight[0], shadelight[1], shadelight[2], alpha);

		va=0;
		VArray = &VArrayVerts[0];

		if(gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value) {

            vec3_t lightVec, lightVal;

			R_InitVArrays (VERT_NORMAL_COLOURED_TEXTURED);
            qglNormalPointer(GL_FLOAT, 0, NormalsArray);
			glEnableVertexAttribArrayARB (1);
			glVertexAttribPointerARB(1, 4, GL_FLOAT,GL_FALSE, 0, TangentsArray);

            R_GetLightVals(currententity->origin, false, true);

            //send light level and color to shader, ramp up a bit
            VectorCopy(lightcolor, lightVal);
            for(i = 0; i < 3; i++) {
                if(lightVal[i] < shadelight[i]/2)
                    lightVal[i] = shadelight[i]/2; //never go completely black
                lightVal[i] *= 5;
                lightVal[i] += dynFactor;
                if(lightVal[i] > 1.0+dynFactor)
                    lightVal[i] = 1.0+dynFactor;
            }

            //simple directional(relative light position)
            VectorSubtract(lightPosition, currententity->origin, lightVec);
            VectorMA(lightPosition, 1.0, lightVec, lightPosition);
            R_ModelViewTransform(lightPosition, lightVec);

            //brighten things slightly
            for (i = 0; i < 3; i++ )
                lightVal[i] *= 2.5;

            GL_EnableMultitexture( true );

            glUseProgramObjectARB( g_meshprogramObj );

            glUniform3fARB( g_location_meshlightPosition, lightVec[0], lightVec[1], lightVec[2]);

            GL_SelectTexture( GL_TEXTURE1);
            qglBindTexture (GL_TEXTURE_2D, r_shelltexture2->texnum);
            glUniform1iARB( g_location_baseTex, 1);

            GL_SelectTexture( GL_TEXTURE0);
            qglBindTexture (GL_TEXTURE_2D, r_shellnormal->texnum);
            glUniform1iARB( g_location_normTex, 0);

            GL_SelectTexture( GL_TEXTURE0);

            glUniform1iARB( g_location_useFX, 0);

            glUniform1iARB( g_location_useGlow, 0);

            glUniform3fARB( g_location_color, lightVal[0], lightVal[1], lightVal[2]);

            glUniform1fARB( g_location_meshTime, rs_realtime);

            glUniform1iARB( g_location_meshFog, map_fog);
        }
		else
		{
			R_InitVArrays (VERT_COLOURED_TEXTURED);
			GL_Bind(r_shelltexture->texnum);
		}

		for (i=0; i<paliashdr->num_tris; i++)
		{
			for (j=0; j<3; j++)
			{
				vec3_t normal;
				vec4_t tangent;
                int k;

				index_xyz = tris[i].index_xyz[j];
				index_st = tris[i].index_st[j];

				if((currententity->flags & (RF_WEAPONMODEL | RF_SHELL_GREEN)) || (gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value))
					shellscale = .4;
				else
					shellscale = 1.6;

				if(lerped)
				{
					VArray[0] = s_lerped[index_xyz][0] = move[0] + ov[index_xyz].v[0]*backv[0] + v[index_xyz].v[0]*frontv[0] + r_avertexnormals[verts[index_xyz].lightnormalindex][0] * shellscale;
					VArray[1] = s_lerped[index_xyz][1] = move[1] + ov[index_xyz].v[1]*backv[1] + v[index_xyz].v[1]*frontv[1] + r_avertexnormals[verts[index_xyz].lightnormalindex][1] * shellscale;
					VArray[2] = s_lerped[index_xyz][2] = move[2] + ov[index_xyz].v[2]*backv[2] + v[index_xyz].v[2]*frontv[2] + r_avertexnormals[verts[index_xyz].lightnormalindex][2] * shellscale;

					VArray[3] = (s_lerped[index_xyz][1] + s_lerped[index_xyz][0]) * (1.0f / 40.0f);
					VArray[4] = s_lerped[index_xyz][2] * (1.0f / 40.0f) - r_newrefdef.time * 0.5f;

					if(gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value)
					{
						for (k=0; k<3; k++)
						{
                            normal[k] = r_avertexnormals[verts[index_xyz].lightnormalindex][k] +
                            ( r_avertexnormals[ov[index_xyz].lightnormalindex][k] -
                            r_avertexnormals[verts[index_xyz].lightnormalindex][k] ) * backlerp;

							tangent[k] = r_avertexnormals[tangents[index_xyz]][k] +
							( r_avertexnormals[oldtangents[index_xyz]][k] -
							r_avertexnormals[tangents[index_xyz]][k] ) * backlerp;
						}
                    }
					else
					{

						VArray[5] = shadelight[0];
						VArray[6] = shadelight[1];
						VArray[7] = shadelight[2];
						VArray[8] = calcEntAlpha(alpha, s_lerped[index_xyz]);
					}
				}
				else
				{
					VArray[0] = currentmodel->vertexes[index_xyz].position[0];
					VArray[1] = currentmodel->vertexes[index_xyz].position[1];
					VArray[2] = currentmodel->vertexes[index_xyz].position[2];

					VArray[3] = (currentmodel->vertexes[index_xyz].position[1] + currentmodel->vertexes[index_xyz].position[0]) * (1.0f / 40.0f);
					VArray[4] = currentmodel->vertexes[index_xyz].position[2] * (1.0f / 40.0f) - r_newrefdef.time * 0.5f;

					if(gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value)
					{
                        for (k=0;k<3;k++)
						{
                            normal[k] = r_avertexnormals[verts[index_xyz].lightnormalindex][k];
							tangent[k] = r_avertexnormals[tangents[index_xyz]][k];
						}
                    }
					else
					{

						VArray[5] = shadelight[0];
						VArray[6] = shadelight[1];
						VArray[7] = shadelight[2];
						VArray[8] = calcEntAlpha(alpha, currentmodel->vertexes[index_xyz].position);
					}
				}
				tangent[3] = 1.0;

				if(gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value)
				{
                    VectorNormalize ( normal );
                    VectorCopy(normal, NormalsArray[va]); //shader needs normal array
					Vector4Copy(tangent, TangentsArray[va]);

                }

                // increment pointer and counter
                if(gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value)
                    VArray += VertexSizes[VERT_NORMAL_COLOURED_TEXTURED];
                else
                    VArray += VertexSizes[VERT_COLOURED_TEXTURED];
                va++;
            }
        }

		if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) 
		{
			R_DrawVarrays(GL_TRIANGLES, 0, va, false);
		}

		if(gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value) {
            glUseProgramObjectARB( 0 );
            GL_EnableMultitexture( false );
        }
	}
	else if(!rs)
	{
		va=0;
		VArray = &VArrayVerts[0];
		alpha = basealpha;
		R_InitVArrays (VERT_COLOURED_TEXTURED);
		GLSTATE_ENABLE_ALPHATEST

		for (i=0; i<paliashdr->num_tris; i++)
		{
			for (j=0; j<3; j++)
			{
				index_xyz = tris[i].index_xyz[j];
				index_st = tris[i].index_st[j];

				if(lerped) {
					MD2_VlightModel (shadelight, &verts[index_xyz], lightcolor);

					VArray[0] = s_lerped[index_xyz][0] = move[0] + ov[index_xyz].v[0]*backv[0] + v[index_xyz].v[0]*frontv[0];
					VArray[1] = s_lerped[index_xyz][1] = move[1] + ov[index_xyz].v[1]*backv[1] + v[index_xyz].v[1]*frontv[1];
					VArray[2] = s_lerped[index_xyz][2] = move[2] + ov[index_xyz].v[2]*backv[2] + v[index_xyz].v[2]*frontv[2];

					VArray[3] = st[index_st].s;
					VArray[4] = st[index_st].t;

					VArray[5] = lightcolor[0];
					VArray[6] = lightcolor[1];
					VArray[7] = lightcolor[2];
					VArray[8] = calcEntAlpha(alpha, s_lerped[index_xyz]);
				}
				else {
					MD2_VlightModel (shadelight, &verts[index_xyz], lightcolor);

					VArray[0] = currentmodel->vertexes[index_xyz].position[0];
					VArray[1] = currentmodel->vertexes[index_xyz].position[1];
					VArray[2] = currentmodel->vertexes[index_xyz].position[2];

					VArray[3] = st[index_st].s;
					VArray[4] = st[index_st].t;

					VArray[5] = lightcolor[0];
					VArray[6] = lightcolor[1];
					VArray[7] = lightcolor[2];
					VArray[8] = calcEntAlpha(alpha, currentmodel->vertexes[index_xyz].position);
				}

				// increment pointer and counter
				VArray += VertexSizes[VERT_COLOURED_TEXTURED];
				va++;
			}

		}
		if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) )
		{
			R_DrawVarrays(GL_TRIANGLES, 0, va, false);
		}
	}
	else if(rs)
	{
		if (rs->stage && rs->stage->has_alpha)
		{
			depthmaskrscipt = true;
		}

		if (depthmaskrscipt)
			qglDepthMask(false);

		stage=rs->stage;

		while (stage)
		{
			va=0;
			VArray = &VArrayVerts[0];
			GLSTATE_ENABLE_ALPHATEST

			if (stage->normalmap && (!gl_normalmaps->value || !gl_glsl_shaders->value || !gl_state.glsl_shaders))
			{
				if(stage->next)
				{
					stage = stage->next;
					continue;
				}
				else
					goto done;
			}

			if(!stage->normalmap)
			{
				if(mirror)
				{
					if( !(currententity->flags & RF_WEAPONMODEL))
					{
						R_InitVArrays(VERT_COLOURED_MULTI_TEXTURED);

						GL_EnableMultitexture( true );
						GL_SelectTexture( GL_TEXTURE0);
						GL_TexEnv ( GL_COMBINE_EXT );
						qglBindTexture (GL_TEXTURE_2D, r_mirrortexture->texnum);
						qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE );
						qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
						GL_SelectTexture( GL_TEXTURE1);
						GL_TexEnv ( GL_COMBINE_EXT );
						qglBindTexture (GL_TEXTURE_2D, r_mirrorspec->texnum);
						qglTexEnvi ( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE );
						qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
						qglTexEnvi ( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT );
					}
					else
					{
						R_InitVArrays (VERT_COLOURED_TEXTURED);
						GL_Bind(r_mirrortexture->texnum);
					}
				}
				else
				{
					R_InitVArrays (VERT_COLOURED_TEXTURED);
					GL_Bind (stage->texture->texnum);
				}

				if (stage->blendfunc.blend)
				{
					GL_BlendFunction(stage->blendfunc.source,stage->blendfunc.dest);
					GLSTATE_ENABLE_BLEND
				}
				else if (basealpha==1.0f)
				{
					GLSTATE_DISABLE_BLEND
				}
				else
				{
					GLSTATE_ENABLE_BLEND
					GL_BlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					alpha = basealpha;
				}

				if (stage->alphashift.min || stage->alphashift.speed)
				{
					if (!stage->alphashift.speed && stage->alphashift.min > 0)
					{
						alpha=basealpha*stage->alphashift.min;
					}
					else if (stage->alphashift.speed)
					{
						alpha=basealpha*sin(rs_realtime * stage->alphashift.speed);
						if (alpha < 0) alpha=-alpha*basealpha;
						if (alpha > stage->alphashift.max) alpha=basealpha*stage->alphashift.max;
						if (alpha < stage->alphashift.min) alpha=basealpha*stage->alphashift.min;
					}
				}
				else
					alpha=basealpha;

				if (!stage->alphamask)
				{
					GLSTATE_DISABLE_ALPHATEST
				}
			}

			if(stage->normalmap)
			{
				vec3_t lightVec, lightVal;

				if(gl_state.vbo && !lerped)
				{
					KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER | KILL_TMU2_POINTER | KILL_NORMAL_POINTER);
				}
				else 
				{
					R_InitVArrays (VERT_NORMAL_COLOURED_TEXTURED);
					qglNormalPointer(GL_FLOAT, 0, NormalsArray);
					glEnableVertexAttribArrayARB (1);
					glVertexAttribPointerARB(1, 4, GL_FLOAT, GL_FALSE, 0, TangentsArray);
				}

				R_GetLightVals(currententity->origin, false, true);

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

				if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
				{
					//fixed light source pointing down, slightly forward and to the left
					lightPosition[0] = -1.0;
					lightPosition[1] = 4.0;
					lightPosition[2] = 8.0;
					R_ModelViewTransform(lightPosition, lightVec);
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
						lightVal[i] *= 1.15;
					}
				}

				GL_EnableMultitexture( true );

				glUseProgramObjectARB( g_meshprogramObj );

				glUniform3fARB( g_location_meshlightPosition, lightVec[0], lightVec[1], lightVec[2]);

				GL_SelectTexture( GL_TEXTURE1);
				qglBindTexture (GL_TEXTURE_2D, skinnum);
				glUniform1iARB( g_location_baseTex, 1);

				GL_SelectTexture( GL_TEXTURE0);
				qglBindTexture (GL_TEXTURE_2D, stage->texture->texnum);
				glUniform1iARB( g_location_normTex, 0);

				GL_SelectTexture( GL_TEXTURE2);
				qglBindTexture (GL_TEXTURE_2D, stage->texture2->texnum);
				glUniform1iARB( g_location_fxTex, 2);

				GL_SelectTexture( GL_TEXTURE0);

				if(stage->fx)
					glUniform1iARB( g_location_useFX, 1);
				else
					glUniform1iARB( g_location_useFX, 0);

				if(stage->glow)
					glUniform1iARB( g_location_useGlow, 1);
				else
					glUniform1iARB( g_location_useGlow, 0);

				glUniform3fARB( g_location_color, lightVal[0], lightVal[1], lightVal[2]);

				//if using shadowmaps, offset self shadowed areas a bit so not to get too dark
				if(gl_shadowmaps->value && !(currententity->flags & (RF_WEAPONMODEL | RF_NOSHADOWS)))
					glUniform1fARB( g_location_minLight, 0.20);
				else
					glUniform1fARB( g_location_minLight, 0.15);

				glUniform1fARB( g_location_meshTime, rs_realtime);

				glUniform1iARB( g_location_meshFog, map_fog);

				if (gl_state.vbo && !lerped)
				{
					currentmodel->vbo_xyz = R_VCFindCache(VBO_STORE_XYZ, currententity);
					if (currentmodel->vbo_xyz) 
					{						
						currentmodel->vbo_st = R_VCFindCache(VBO_STORE_ST, currententity);
						if(currentmodel->vbo_st)
						{
							currentmodel->vbo_normals = R_VCFindCache(VBO_STORE_NORMAL, currententity);
							if(currentmodel->vbo_normals)
							{
								currentmodel->vbo_tangents = R_VCFindCache(VBO_STORE_TANGENT, currententity);
								if(currentmodel->vbo_tangents)
								{
									//Com_Printf("skipped\n");
									goto skipLoad;
								}
							}
						}
					}
				}
			}			

			for (i=0; i<paliashdr->num_tris; i++)
			{
				for (j=0; j<3; j++)
				{
					vec3_t normal;
					vec4_t tangent;
					int k;

					index_xyz = tris[i].index_xyz[j];
					index_st = tris[i].index_st[j];

					os = os2 = st[index_st].s;
					ot = ot2 = st[index_st].t;

					if(lerped)
					{
						VArray[0] = s_lerped[index_xyz][0] = move[0] + ov[index_xyz].v[0]*backv[0] + v[index_xyz].v[0]*frontv[0];
						VArray[1] = s_lerped[index_xyz][1] = move[1] + ov[index_xyz].v[1]*backv[1] + v[index_xyz].v[1]*frontv[1];
						VArray[2] = s_lerped[index_xyz][2] = move[2] + ov[index_xyz].v[2]*backv[2] + v[index_xyz].v[2]*frontv[2];

						for (k=0; k<3; k++)
						{
							normal[k] = r_avertexnormals[verts[index_xyz].lightnormalindex][k] +
							( r_avertexnormals[ov[index_xyz].lightnormalindex][k] -
							r_avertexnormals[verts[index_xyz].lightnormalindex][k] ) * backlerp;

							tangent[k] = r_avertexnormals[tangents[index_xyz]][k] +
							( r_avertexnormals[oldtangents[index_xyz]][k] -
							r_avertexnormals[tangents[index_xyz]][k] ) * backlerp;
						}
					}
					else
					{
						VArray[0] = currentmodel->vertexes[index_xyz].position[0];
						VArray[1] = currentmodel->vertexes[index_xyz].position[1];
						VArray[2] = currentmodel->vertexes[index_xyz].position[2];

						for (k=0;k<3;k++)
						{
							normal[k] = r_avertexnormals[verts[index_xyz].lightnormalindex][k];
							tangent[k] = r_avertexnormals[tangents[index_xyz]][k];
						}
					}

					VectorNormalize ( normal );
					tangent[3] = 1.0;

					if(stage->normalmap)
					{
						//send tangent to shader
						VectorCopy(normal, NormalsArray[va]); //shader needs normal array
						Vector4Copy(tangent, TangentsArray[va]);
					}

					if (stage->envmap)
					{
						vec3_t envmapvec;

						VectorAdd(currententity->origin, s_lerped[index_xyz], envmapvec);

						if(mirror)
						{
							if( !(currententity->flags & RF_WEAPONMODEL))
							{
									stage->scale.scaleX = -0.5;
									stage->scale.scaleY = 0.5;
									os -= DotProduct (normal , vectors[1]);
									ot += DotProduct (normal, vectors[2]);
							}
							else
							{
								stage->scale.scaleX = -1.0;
								stage->scale.scaleY = 1.0;
							}

						}
						else
						{
							RS_SetEnvmap (envmapvec, &os, &ot);

							if (currententity->flags & RF_TRANSLUCENT) //return to original glass script's scale(mostly for when going into menu)
								stage->scale.scaleX = stage->scale.scaleY = 0.5;

							os -= DotProduct (normal , vectors[1] );
							ot += DotProduct (normal, vectors[2] );
						}
					}

					RS_SetTexcoords2D(stage, &os, &ot);

					VArray[3] = os;
					VArray[4] = ot;

					if(mirror && !(currententity->flags & RF_WEAPONMODEL))
					{
						os2 -= DotProduct (normal , vectors[1] );
						ot2 += DotProduct (normal, vectors[2] );
						RS_SetTexcoords2D(stage, &os2, &ot2);

						VArray[5] = os2;
						VArray[6] = ot2;
					}

					if(!stage->normalmap)
					{
						float red = 1, green = 1, blue = 1, nAlpha;

						if(lerped)
							nAlpha = RS_AlphaFuncAlias (stage->alphafunc,
								calcEntAlpha(alpha, s_lerped[index_xyz]), normal, s_lerped[index_xyz]);
						else
							nAlpha = RS_AlphaFuncAlias (stage->alphafunc,
								calcEntAlpha(alpha, currentmodel->vertexes[index_xyz].position), normal, currentmodel->vertexes[index_xyz].position);

						if (stage->lightmap)
						{
							if(lerped)
								MD2_VlightModel (shadelight, &verts[index_xyz], lightcolor);
							else
								MD2_VlightModel (shadelight, &verts[index_xyz], lightcolor);
							red = lightcolor[0];
							green = lightcolor[1];
							blue = lightcolor[2];
						}

						if(mirror && !(currententity->flags & RF_WEAPONMODEL) )
						{
							VArray[7] = red;
							VArray[8] = green;
							VArray[9] = blue;
							VArray[10] = nAlpha;
						}
						else {
							VArray[5] = red;
							VArray[6] = green;
							VArray[7] = blue;
							VArray[8] = nAlpha;
						}
					}
					else if(gl_state.vbo && !lerped) 
					{
						VertexArray[va][0] = VArray[0];
						VertexArray[va][1] = VArray[1];
						VertexArray[va][2] = VArray[2];

						TexCoordArray[va][0] = VArray[3];
						TexCoordArray[va][1] = VArray[4];
					}

					// increment pointer and counter
					if(stage->normalmap)
						VArray += VertexSizes[VERT_NORMAL_COLOURED_TEXTURED];
					else if(mirror && !(currententity->flags & RF_WEAPONMODEL))
						VArray += VertexSizes[VERT_COLOURED_MULTI_TEXTURED];
					else
						VArray += VertexSizes[VERT_COLOURED_TEXTURED];
					va++;
				}
			}

			if(gl_state.vbo && !lerped && stage->normalmap)
			{
                currentmodel->vbo_xyz = R_VCLoadData(VBO_STATIC, va*sizeof(vec3_t), VertexArray, VBO_STORE_XYZ, currententity);
				currentmodel->vbo_st = R_VCLoadData(VBO_STATIC, va*sizeof(vec2_t), TexCoordArray, VBO_STORE_ST, currententity);
				currentmodel->vbo_normals = R_VCLoadData(VBO_STATIC, va*sizeof(vec3_t), NormalsArray, VBO_STORE_NORMAL, currententity);
				currentmodel->vbo_tangents = R_VCLoadData(VBO_STATIC, va*sizeof(vec4_t), TangentsArray, VBO_STORE_TANGENT, currententity);
				//Com_Printf("Loading mesh vbo.\n");
            }
skipLoad:
			if(gl_state.vbo && !lerped && stage->normalmap) 
			{
				qglEnableClientState( GL_VERTEX_ARRAY );
				GL_BindVBO(currentmodel->vbo_xyz);
				qglVertexPointer(3, GL_FLOAT, 0, 0);

				qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
				GL_BindVBO(currentmodel->vbo_st);
				qglTexCoordPointer(2, GL_FLOAT, 0, 0);

				qglEnableClientState( GL_NORMAL_ARRAY );
				GL_BindVBO(currentmodel->vbo_normals);
				qglNormalPointer(GL_FLOAT, 0, 0);

				glEnableVertexAttribArrayARB (1);
				GL_BindVBO(currentmodel->vbo_tangents);
				glVertexAttribPointerARB(1, 4, GL_FLOAT, GL_FALSE, sizeof(vec4_t), 0);

				if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL )))
					R_DrawVarrays(GL_TRIANGLES, 0, paliashdr->num_tris*3, true);
										
				GL_BindVBO(NULL);
	        }
         	else if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL )))
			{
				R_DrawVarrays(GL_TRIANGLES, 0, paliashdr->num_tris*3, false);
			}

			qglColor4f(1,1,1,1);

			if(mirror && !(currententity->flags & RF_WEAPONMODEL))
				GL_EnableMultitexture( false );

			if(stage->normalmap)
			{
				glUseProgramObjectARB( 0 );
				GL_EnableMultitexture( false );
			}

			stage=stage->next;
		}
	}
done:
	if (depthmaskrscipt)
		qglDepthMask(true);

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_TEXGEN

	qglDisableClientState( GL_NORMAL_ARRAY);
	qglDisableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glDisableVertexAttribArrayARB (1);

	R_KillVArrays ();	

	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM ) )
		qglEnable( GL_TEXTURE_2D );

}

extern qboolean have_stencil;
extern	vec3_t			lightspot;

/*
=============
MD2_DrawShadow
=============
*/
void MD2_DrawShadow(dmdl_t *paliashdr, qboolean lerped)
{
	dtrivertx_t	*verts;
	vec3_t	point;
	float	height, lheight;
	daliasframe_t	*frame;
	fstvert_t	*st;
	dtriangle_t		*tris;
	int		i, j;
	int		index_xyz, index_st;
	int		va = 0;

	lheight = currententity->origin[2] - lightspot[2];

	if(lerped)
		frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
			+ currententity->frame * paliashdr->framesize);
	else
		frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames);

	verts = frame->verts;

	tris = (dtriangle_t *) ((byte *)paliashdr + paliashdr->ofs_tris);

	st = currentmodel->st;

	height = -lheight + 0.1f;

	// if above entity's origin, skip
	if ((currententity->origin[2]+height) > currententity->origin[2])
		return;

	if (r_newrefdef.vieworg[2] < (currententity->origin[2] + height))
		return;

	if (have_stencil) {

		qglDepthMask(0);

		qglEnable(GL_STENCIL_TEST);

		qglStencilFunc(GL_EQUAL,1,2);

		qglStencilOp(GL_KEEP,GL_KEEP,GL_INCR);

	}

	va=0;
	VArray = &VArrayVerts[0];
	R_InitVArrays (VERT_SINGLE_TEXTURED);

	for (i=0; i<paliashdr->num_tris; i++)
	{
		for (j=0; j<3; j++)
		{
			index_xyz = tris[i].index_xyz[j];
			index_st = tris[i].index_st[j];

			if(lerped)
				memcpy( point, s_lerped[index_xyz], sizeof( point )  );
			else
				memcpy( point, currentmodel->vertexes[index_xyz].position, sizeof( point )  );

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;

			VArray[0] = point[0];
			VArray[1] = point[1];
			VArray[2] = point[2];

			VArray[3] = st[index_st].s;
			VArray[4] = st[index_st].t;

			// increment pointer and counter
			VArray += VertexSizes[VERT_SINGLE_TEXTURED];
			va++;
		}

	}

	R_DrawVarrays(GL_TRIANGLES, 0, va, false);

	qglDisableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	R_KillVArrays ();
	qglDepthMask(1);
	qglColor4f(1,1,1,1);
	if (have_stencil) qglDisable(GL_STENCIL_TEST);
}

/*
=================
R_DrawAliasModel - render alias models(using MD2 format)

=================
*/
void R_DrawAliasModel ( void )
{
	int			i;
	dmdl_t		*paliashdr;
	vec3_t		bbox[8];
	image_t		*skin;

	if((r_newrefdef.rdflags & RDF_NOWORLDMODEL ) && !(currententity->flags & RF_MENUMODEL))
		return;

	if(currententity->team) //don't draw flag models, handled by sprites
		return;

	if ( !( currententity->flags & RF_WEAPONMODEL ) )
	{
		if ( MD2_CullModel( bbox ) )
			return;
	}
	else
	{
		if ( r_lefthand->value == 2 )
			return;
	}

	paliashdr = (dmdl_t *)currentmodel->extradata;

	//
	// get lighting information
	//
	// PMM - rewrote, reordered to handle new shells & mixing
	// PMM - 3.20 code .. replaced with original way of doing it to keep mod authors happy
	//
	if ( currententity->flags & ( RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE) )
	{
		if(!cl_gun->value && (currententity->flags & RF_WEAPONMODEL))
			return;

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

		if(gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value)
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
		if(gl_glsl_shaders->value && gl_state.glsl_shaders && gl_normalmaps->value)
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

	shadedots = r_avertexnormal_dots[((int)(currententity->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];

	//
	// locate the proper data
	//

	if ( !(currententity->flags & RF_VIEWERMODEL) )
		c_alias_polys += paliashdr->num_tris; // for r_speeds epoly count

	//
	// draw all the triangles
	//
	if (currententity->flags & RF_DEPTHHACK) // hack the depth range to prevent view model from poking into walls
		qglDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));

	if ((currententity->flags & RF_WEAPONMODEL) && r_lefthand->value != 2.0F)
    {
		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();

		if (r_lefthand->value == 1.0F)
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

    qglPushMatrix ();
	currententity->angles[PITCH] = -currententity->angles[PITCH];	// sigh.
	R_RotateForEntity (currententity);
	currententity->angles[PITCH] = -currententity->angles[PITCH];	// sigh.

	// select skin
	if (currententity->skin) {
		skin = currententity->skin;	// custom player skin
	}
	else
	{
		if (currententity->skinnum >= MAX_MD2SKINS)
			skin = currentmodel->skins[0];
		else
		{
			skin = currentmodel->skins[currententity->skinnum];
			if (!skin)
				skin = currentmodel->skins[0];
		}
	}
	if (!skin)
		skin = r_notexture;	// fallback...
	GL_Bind(skin->texnum);

	// draw it

	qglShadeModel (GL_SMOOTH);

	GL_TexEnv( GL_MODULATE );

	if ( currententity->flags & ( RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE ) )
		qglEnable (GL_BLEND);
	else if ( currententity->flags & RF_TRANSLUCENT )
	{
		qglEnable (GL_BLEND);
		qglBlendFunc (GL_ONE, GL_ONE);
	}
	if (currententity->flags & RF_CUSTOMSKIN)
	{
		qglTexGenf(GL_S, GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
		qglTexGenf(GL_T, GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
		qglEnable(GL_TEXTURE_GEN_S);
		qglEnable(GL_TEXTURE_GEN_T);
		qglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	}

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

	if ( !r_lerpmodels->value )
		currententity->backlerp = 0;

	if(currententity->frame == 0 && currentmodel->num_frames == 1) {
		if(!(currententity->flags & RF_VIEWERMODEL))
			MD2_DrawFrame(paliashdr, 0, false, skin->texnum);
	}
	else
		MD2_DrawFrame(paliashdr, currententity->backlerp, true, skin->texnum);

	GL_TexEnv( GL_REPLACE );
	qglShadeModel (GL_FLAT);

	qglPopMatrix ();

	if ( ( currententity->flags & RF_WEAPONMODEL ) && ( r_lefthand->value != 2.0F ) )
	{
		qglMatrixMode( GL_PROJECTION );
		qglPopMatrix();
		qglMatrixMode( GL_MODELVIEW );
		qglCullFace( GL_FRONT );
	}

	if ( currententity->flags & ( RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE ) )
		qglDisable(GL_BLEND);
	else if ( currententity->flags & RF_TRANSLUCENT )
	{
		qglDisable (GL_BLEND);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	if( currententity->flags & RF_CUSTOMSKIN )
	{
		qglDisable(GL_TEXTURE_GEN_S);
		qglDisable(GL_TEXTURE_GEN_T);
		qglDisable (GL_BLEND);
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        qglColor4f(1,1,1,1);
	    qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	}
	if (currententity->flags & RF_DEPTHHACK)
		qglDepthRange (gldepthmin, gldepthmax);

	//simple stencil shadows
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL) && gl_shadows->integer && !gl_shadowmaps->integer && !(currententity->flags & (RF_WEAPONMODEL | RF_NOSHADOWS)))
	{
		float casted;
		float an = currententity->angles[1]/180*M_PI;
		shadevector[0] = cos(-an);
		shadevector[1] = sin(-an);
		shadevector[2] = 1;
		VectorNormalize (shadevector);

		switch (gl_shadows->integer)
		{
		case 0:
			break;
		case 1: //dynamic only - always cast something
			casted = SHD_ShadowLight (currententity->origin, currententity->angles, shadevector, 0);
			qglPushMatrix ();
			qglTranslatef	(currententity->origin[0], currententity->origin[1], currententity->origin[2]);
			qglRotatef (currententity->angles[1], 0, 0, 1);
			qglDisable (GL_TEXTURE_2D);
			qglEnable (GL_BLEND);

			if (currententity->flags & RF_TRANSLUCENT)
				qglColor4f (0,0,0,0.3 * currententity->alpha); //Knightmare- variable alpha
			else
				qglColor4f (0,0,0,0.3);

			if(currententity->frame == 0 && currentmodel->num_frames == 1)
				MD2_DrawShadow (paliashdr, false);
			else
				MD2_DrawShadow (paliashdr, true);

			qglEnable (GL_TEXTURE_2D);
			qglDisable (GL_BLEND);
			qglPopMatrix ();

			break;
		case 2: //dynamic and world
			//world
			casted = SHD_ShadowLight (currententity->origin, currententity->angles, shadevector, 1);
			qglPushMatrix ();
			qglTranslatef	(currententity->origin[0], currententity->origin[1], currententity->origin[2]);
			qglRotatef (currententity->angles[1], 0, 0, 1);
			qglDisable (GL_TEXTURE_2D);
			qglEnable (GL_BLEND);

			if (currententity->flags & RF_TRANSLUCENT)
				qglColor4f (0,0,0,casted * currententity->alpha);
			else
				qglColor4f (0,0,0,casted);

			if(currententity->frame == 0 && currentmodel->num_frames == 1)
				MD2_DrawShadow (paliashdr, false);
			else
				MD2_DrawShadow (paliashdr, true);

			qglEnable (GL_TEXTURE_2D);
			qglDisable (GL_BLEND);
			qglPopMatrix ();
			//dynamic
			casted = 0;
		 	casted = SHD_ShadowLight (currententity->origin, currententity->angles, shadevector, 0);
			if (casted > 0) { //only draw if there's a dynamic light there
				qglPushMatrix ();
				qglTranslatef	(currententity->origin[0], currententity->origin[1], currententity->origin[2]);
				qglRotatef (currententity->angles[1], 0, 0, 1);
				qglDisable (GL_TEXTURE_2D);
				qglEnable (GL_BLEND);

				if (currententity->flags & RF_TRANSLUCENT)
					qglColor4f (0,0,0,casted * currententity->alpha);
				else
					qglColor4f (0,0,0,casted);

				if(currententity->frame == 0 && currentmodel->num_frames == 1)
					MD2_DrawShadow (paliashdr, false);
				else
					MD2_DrawShadow (paliashdr, true);

				qglEnable (GL_TEXTURE_2D);
				qglDisable (GL_BLEND);
				qglPopMatrix ();
			}

			break;
		}
	}
	qglColor4f (1,1,1,1);

	if(r_minimap->value)
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

void MD2_DrawCasterFrame (dmdl_t *paliashdr, float backlerp, qboolean lerped)
{
	daliasframe_t	*frame, *oldframe;
	dtrivertx_t	*v, *ov=NULL, *verts;
	dtriangle_t		*tris;
	float	frontlerp;
	vec3_t	move, delta, vectors[3];
	vec3_t	frontv, backv;
	int		i, j;
	int		index_xyz, index_st;
	int		va = 0;
	fstvert_t *st;

	if(lerped)
		frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
			+ currententity->frame * paliashdr->framesize);
	else
		frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames);
	verts = v = frame->verts;

	if(lerped) {
		oldframe = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
			+ currententity->oldframe * paliashdr->framesize);
		ov = oldframe->verts;
	}

	tris = (dtriangle_t *) ((byte *)paliashdr + paliashdr->ofs_tris);

	st = currentmodel->st;

	if(lerped) {
		frontlerp = 1.0 - backlerp;

		// move should be the delta back to the previous frame * backlerp
		VectorSubtract (currententity->oldorigin, currententity->origin, delta);
	}

	AngleVectors (currententity->angles, vectors[0], vectors[1], vectors[2]);

	if(lerped) 
	{
		move[0] = DotProduct (delta, vectors[0]);	// forward
		move[1] = -DotProduct (delta, vectors[1]);	// left
		move[2] = DotProduct (delta, vectors[2]);	// up

		VectorAdd (move, oldframe->translate, move);

		for (i=0 ; i<3 ; i++)
		{
			move[i] = backlerp*move[i] + frontlerp*frame->translate[i];
			frontv[i] = frontlerp*frame->scale[i];
			backv[i] = backlerp*oldframe->scale[i];
		}
	}

	va=0;
	VArray = &VArrayVerts[0];
	R_InitVArrays (VERT_NO_TEXTURE);

	if (gl_state.vbo && !lerped)
	{
		currentmodel->vbo_xyz = R_VCFindCache(VBO_STORE_XYZ, currententity);
		if (currentmodel->vbo_xyz) 
		{
			//Com_Printf("Skipped.\n");
			goto skipLoad;
		}
	}

	for (i = 0; i < paliashdr->num_tris; i++)
	{
		for (j = 0; j < 3; j++)
		{
			index_xyz = tris[i].index_xyz[j];
			index_st = tris[i].index_st[j];

			if(lerped) 
			{
				VArray[0] = s_lerped[index_xyz][0] = move[0] + ov[index_xyz].v[0]*backv[0] + v[index_xyz].v[0]*frontv[0];
				VArray[1] = s_lerped[index_xyz][1] = move[1] + ov[index_xyz].v[1]*backv[1] + v[index_xyz].v[1]*frontv[1];
				VArray[2] = s_lerped[index_xyz][2] = move[2] + ov[index_xyz].v[2]*backv[2] + v[index_xyz].v[2]*frontv[2];

			}
			else 
			{
				VArray[0] = currentmodel->vertexes[index_xyz].position[0];
				VArray[1] = currentmodel->vertexes[index_xyz].position[1];
				VArray[2] = currentmodel->vertexes[index_xyz].position[2];

				if(gl_state.vbo) 
				{
					VertexArray[va][0] = VArray[0];
					VertexArray[va][1] = VArray[1];
					VertexArray[va][2] = VArray[2];
				}
			}

			// increment pointer and counter
			VArray += VertexSizes[VERT_NO_TEXTURE];
			va++;
		}
	}

	if(gl_state.vbo && !lerped)
	{
		currentmodel->vbo_xyz = R_VCLoadData(VBO_STATIC, va*sizeof(vec3_t), VertexArray, VBO_STORE_XYZ, currententity);
		//Com_Printf("Loading mesh vbo.\n");
    }

skipLoad:
	if(gl_state.vbo && !lerped) 
	{
		qglEnableClientState( GL_VERTEX_ARRAY );
		GL_BindVBO(currentmodel->vbo_xyz);
		qglVertexPointer(3, GL_FLOAT, 0, 0);   

		R_DrawVarrays(GL_TRIANGLES, 0, paliashdr->num_tris*3, true);
			
		GL_BindVBO(NULL);
    }
	else
	{    
		R_DrawVarrays(GL_TRIANGLES, 0, paliashdr->num_tris*3, false);
	}

	R_KillVArrays ();
}

//to do - alpha and alphamasks possible?
void MD2_DrawCaster ( void )
{
	vec3_t		bbox[8];
	dmdl_t		*paliashdr;

	if(currententity->team) //don't draw flag models, handled by sprites
		return;

	if ( currententity->flags & RF_WEAPONMODEL ) //don't draw weapon model shadow casters
		return;

	if ( currententity->flags & ( RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED | RF_SHELL_BLUE | RF_SHELL_DOUBLE) ) //no shells
		return;

	if ( MD2_CullModel( bbox ) )
		return;

	paliashdr = (dmdl_t *)currentmodel->extradata;

	// draw it

    qglPushMatrix ();
	currententity->angles[PITCH] = -currententity->angles[PITCH];
	R_RotateForEntity (currententity);
	currententity->angles[PITCH] = -currententity->angles[PITCH];

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

	if ( !r_lerpmodels->value )
		currententity->backlerp = 0;

	if(currententity->frame == 0 && currentmodel->num_frames == 1)
		MD2_DrawCasterFrame(paliashdr, 0, false);
	else
		MD2_DrawCasterFrame(paliashdr, currententity->backlerp, true);

	qglPopMatrix();
}
