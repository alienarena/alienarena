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

// r_mesh.c: triangle model rendering functions, shared by all mesh types

#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_ragdoll.h"

// FIXME: globals
static vec3_t worldlight, shadelight, totallight, totalLightPosition;

extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
extern image_t *r_mirrortexture;

extern cvar_t *cl_gun;

cvar_t *gl_mirror;

#if 0
static void R_Mesh_VecsForTris(float *v0, float *v1, float *v2, float *st0, float *st1, float *st2, vec3_t Tangent)
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
static void R_Mesh_VecsForTris(
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

static void R_Mesh_CompleteNormalsTangents
	(model_t *mod, int calcflags, const nonskeletal_basevbo_t *basevbo, mesh_framevbo_t *framevbo, const unsigned int *vtriangles)
{
	int i, j;
	unsigned int nonindexed_triangle[3];
	const skeletal_basevbo_t *basevbo_sk = (const skeletal_basevbo_t *)basevbo;
	qboolean *merged, *do_merge;
	merged = Z_Malloc (mod->numvertexes * sizeof(qboolean));
	do_merge = Z_Malloc (mod->numvertexes * sizeof(qboolean));
	
	for (i = 0; i < mod->num_triangles; i++)
	{
		vec3_t v1, v2, normal, tangent;
		const unsigned int *triangle;
		
		if ((mod->typeFlags & MESH_INDEXED))
		{
			triangle = &vtriangles[3*i];
		}
		else
		{
			triangle = nonindexed_triangle;
			for (j = 0; j < 3; j++)
				nonindexed_triangle[j] = 3*i+j;
		}
		
		if ((calcflags & MESHLOAD_CALC_NORMAL))
		{
			VectorSubtract (framevbo[triangle[0]].vertex, framevbo[triangle[1]].vertex, v1);
			VectorSubtract (framevbo[triangle[2]].vertex, framevbo[triangle[1]].vertex, v2);
			CrossProduct (v2, v1, normal);
			VectorScale (normal, -1.0f, normal);
			
			for (j = 0; j < 3; j++)
				VectorAdd (framevbo[triangle[j]].normal, normal, framevbo[triangle[j]].normal);
		}
		
		if ((calcflags & MESHLOAD_CALC_TANGENT))
		{
			const nonskeletal_basevbo_t *basevbo_vert[3];
			
			for (j = 0; j < 3; j++)
			{
				if ((mod->typeFlags & MESH_SKELETAL))
					basevbo_vert[j] = &basevbo_sk[triangle[j]].common;
				else
					basevbo_vert[j] = &basevbo[triangle[j]];
			}
			
			R_Mesh_VecsForTris (	framevbo[triangle[0]].vertex,
									framevbo[triangle[1]].vertex,
									framevbo[triangle[2]].vertex,
									basevbo_vert[0]->st,
									basevbo_vert[1]->st,
									basevbo_vert[2]->st,
									tangent );
		
			for (j = 0; j < 3; j++)
				VectorAdd (framevbo[triangle[j]].tangent, tangent, framevbo[triangle[j]].tangent);
		}
	}
	
	// In the case of texture seams in the mesh, we merge tangents/normals for
	// verts with the same positions. This is also necessary for non-indexed
	// meshes.
	for (i = 0; i < mod->numvertexes; i++)
	{
		if (merged[i]) continue;
		
		// Total normals/tangents up
		for (j = i + 1; j < mod->numvertexes; j++)
		{
			if (VectorCompare (framevbo[i].vertex, framevbo[j].vertex))
			{
				do_merge[j] = true;
				
				if ((calcflags & MESHLOAD_CALC_NORMAL))
					VectorAdd (framevbo[i].normal, framevbo[j].normal, framevbo[i].normal);
				if ((calcflags & MESHLOAD_CALC_TANGENT))
					VectorAdd (framevbo[i].tangent, framevbo[j].tangent, framevbo[i].tangent);
			}
		}
		
		// Normalize the average normals and tangents
		if ((calcflags & MESHLOAD_CALC_NORMAL))
		{
			VectorNormalize (framevbo[i].normal);
		}
		if ((calcflags & MESHLOAD_CALC_TANGENT))
		{
			VectorNormalize (framevbo[i].tangent);
			framevbo[i].tangent[3] = 1.0;
		}
		
		// Copy the averages where they need to go
		for (j = i + 1; j < mod->numvertexes; j++)
		{
			if (do_merge[j])
			{
				merged[j] = true;
				do_merge[j] = false;
				
				if ((calcflags & MESHLOAD_CALC_NORMAL))
					VectorCopy (framevbo[i].normal, framevbo[j].normal);
				if ((calcflags & MESHLOAD_CALC_TANGENT))
					Vector4Copy (framevbo[i].tangent, framevbo[j].tangent);
			}
		}
	}
	
	Z_Free (merged);
	Z_Free (do_merge);
}

void R_Mesh_LoadVBO (model_t *mod, int calcflags, ...)
{
	const int maxframes = ((mod->typeFlags & MESH_MORPHTARGET) ? mod->num_frames : 1);
	const int frames_idx = ((mod->typeFlags & MESH_INDEXED) ? 2 : 1);
	mesh_framevbo_t *framevbo;
	const unsigned int *vtriangles = NULL;
	const nonskeletal_basevbo_t *basevbo;
	va_list ap;
	
	mod->vboIDs = malloc ((maxframes + frames_idx) * sizeof(*mod->vboIDs));
	qglGenBuffersARB (maxframes + frames_idx, mod->vboIDs);
	
	va_start (ap, calcflags);
	
	GL_BindVBO (mod->vboIDs[0]);
	if ((mod->typeFlags & MESH_SKELETAL))
	{
		const skeletal_basevbo_t *basevbo_sk = va_arg (ap, const skeletal_basevbo_t *);
		qglBufferDataARB (GL_ARRAY_BUFFER_ARB, mod->numvertexes * sizeof(*basevbo_sk), basevbo_sk, GL_STATIC_DRAW_ARB);
		basevbo = &basevbo_sk->common;
	}
	else
	{
		basevbo = va_arg (ap, const nonskeletal_basevbo_t *);
		qglBufferDataARB (GL_ARRAY_BUFFER_ARB, mod->numvertexes * sizeof(*basevbo), basevbo, GL_STATIC_DRAW_ARB);
	}
	
	if ((mod->typeFlags & MESH_INDEXED))
	{
		GL_BindVBO (mod->vboIDs[1]);
		vtriangles = va_arg (ap, const unsigned int *);
		qglBufferDataARB (GL_ARRAY_BUFFER_ARB, mod->num_triangles * 3 * sizeof(unsigned int), vtriangles, GL_STATIC_DRAW_ARB);
	}
	
	if (!(mod->typeFlags & MESH_MORPHTARGET))
	{
		framevbo = va_arg (ap, mesh_framevbo_t *);
		if (calcflags != 0)
			R_Mesh_CompleteNormalsTangents (mod, calcflags, basevbo, framevbo, vtriangles);
		GL_BindVBO (mod->vboIDs[frames_idx]);
		qglBufferDataARB (GL_ARRAY_BUFFER_ARB, mod->numvertexes * sizeof(*framevbo), framevbo, GL_STATIC_DRAW_ARB);
	}
	else
	{
		int framenum;
		Mesh_GetFrameVBO_Callback callback = va_arg (ap, Mesh_GetFrameVBO_Callback);
		void *data = va_arg (ap, void *);
		
		for (framenum = 0; framenum < mod->num_frames; framenum++)
		{
			mesh_framevbo_t *framevbo = callback (data, framenum);
			if (calcflags != 0)
				R_Mesh_CompleteNormalsTangents (mod, calcflags, basevbo, framevbo, vtriangles);
			GL_BindVBO (mod->vboIDs[framenum + frames_idx]);
			qglBufferDataARB (GL_ARRAY_BUFFER_ARB, mod->numvertexes * sizeof(*framevbo), framevbo, GL_STATIC_DRAW_ARB);
		}
	}
	
	GL_BindVBO (0);
	
	va_end (ap);
}

void R_Mesh_FreeVBO (model_t *mod)
{
	const int maxframes = ((mod->typeFlags & MESH_MORPHTARGET) ? mod->num_frames : 1);
	const int frames_idx = ((mod->typeFlags & MESH_INDEXED) ? 2 : 1);
	
	qglDeleteBuffersARB (maxframes + frames_idx, mod->vboIDs);
	free (mod->vboIDs);
}

//This routine bascially finds the average light position, by factoring in all lights and
//accounting for their distance, visiblity, and intensity.
void R_GetLightVals(vec3_t meshOrigin, qboolean RagDoll)
{
	int i, j, lnum;
	dlight_t *dl;
	float dist;
	vec3_t staticlight, dynamiclight;
	vec3_t	temp, tempOrg, lightAdd;
	float numlights, nonweighted_numlights, weight;
	float bob;
	qboolean copy;

	//light shining down if there are no lights at all
	VectorCopy (meshOrigin, statLightPosition);
	statLightPosition[2] += 128;

	if ((currententity->flags & RF_BOBBING) && !RagDoll)
		bob = currententity->bob;
	else
		bob = 0;

	VectorCopy(meshOrigin, tempOrg);
		tempOrg[2] += 24 - bob; //generates more consistent tracing

	numlights = nonweighted_numlights = 0;
	VectorClear(lightAdd);
	
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
		VectorCopy (cl_persistent_ents[currententity->number].oldlightadd, lightAdd);
		VectorCopy (cl_persistent_ents[currententity->number].oldstaticlight, staticlight);
	}
	else
	{
		R_StaticLightPoint (currententity->origin, staticlight);
		VectorCopy (staticlight, cl_persistent_ents[currententity->number].oldstaticlight);
		for (i = 0; i < r_lightgroups; i++)
		{
			if (currentmodel->type == mod_terrain || currentmodel->type == mod_decal) {}
				//terrain meshes can actually occlude themselves. TODO: move to precompiled lightmaps for terrain.
			else if (!RagDoll && (currententity->flags & RF_WEAPONMODEL) && (LightGroups[i].group_origin[2] > meshOrigin[2])) {}
				//don't do traces for lights above weapon models, not smooth enough
			else if (!CM_inPVS (tempOrg, LightGroups[i].group_origin))
				continue;
			else if (!CM_FastTrace (tempOrg, LightGroups[i].group_origin, r_worldmodel->firstnode, MASK_OPAQUE))
				continue;

			VectorSubtract(meshOrigin, LightGroups[i].group_origin, temp);
			dist = VectorLength(temp);
			if (dist == 0)
				dist = 1;
			dist = dist*dist;
			weight = (int)250000/(dist/(LightGroups[i].avg_intensity+1.0f));
			for (j = 0; j < 3; j++)
				lightAdd[j] += LightGroups[i].group_origin[j]*weight;
			numlights+=weight;
			nonweighted_numlights++;
		}
		
		cl_persistent_ents[currententity->number].oldnumlights = numlights;
		VectorCopy (lightAdd, cl_persistent_ents[currententity->number].oldlightadd);
		cl_persistent_ents[currententity->number].setlightstuff = true;
		VectorCopy (currententity->origin, cl_persistent_ents[currententity->number].oldorigin);
	}

	if (numlights > 0.0)
	{
		for (i = 0; i < 3; i++)
			statLightPosition[i] = lightAdd[i]/numlights;
	}
	
	VectorCopy (staticlight, worldlight);
	
	// everything after this line is used only for subsurface scattering.
	
	if (gl_dynamic->integer != 0)
	{
		R_DynamicLightPoint (currententity->origin, dynamiclight);
		dl = r_newrefdef.dlights;
		//limit to five lights(maybe less)?
		for (lnum=0; lnum<(r_newrefdef.num_dlights > 5 ? 5: r_newrefdef.num_dlights); lnum++, dl++)
		{
			VectorSubtract (meshOrigin, dl->origin, temp);
			dist = VectorLength (temp);
			if (dist >= dl->intensity)
				continue;

			VectorCopy(meshOrigin, temp);
			temp[2] += 24; //generates more consistent tracing

			if (CM_FastTrace (temp, dl->origin, r_worldmodel->firstnode, MASK_OPAQUE))
			{
				//make dynamic lights more influential than world
				for (j = 0; j < 3; j++)
					lightAdd[j] += dl->origin[j]*10*dl->intensity;
				numlights+=10*dl->intensity;

				VectorSubtract (dl->origin, meshOrigin, temp);
			}
		}
	}
	else
	{
		VectorClear (dynamiclight);
	}
	
	VectorAdd (staticlight, dynamiclight, totallight);

	if (numlights > 0.0)
	{
		for (i = 0; i < 3; i++)
			totalLightPosition[i] = lightAdd[i]/numlights;
	}
}

void R_ModelViewTransform(const vec3_t in, vec3_t out){
	const float *v = in;
	const float *m = r_world_matrix;

	out[0] = m[0] * v[0] + m[4] * v[1] + m[8]  * v[2] + m[12];
	out[1] = m[1] * v[0] + m[5] * v[1] + m[9]  * v[2] + m[13];
	out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
}

// Unlike R_CullBox, this takes an *oriented* (rotated) bounding box.
static qboolean R_Mesh_CullBBox (vec3_t bbox[8])
{
	int p, f, aggregatemask = ~0;

	for (p = 0; p < 8; p++)
	{
		int mask = 0;

		for (f = 0; f < 4; f++)
		{
			if (DotProduct (frustum[f].normal, bbox[p]) < frustum[f].dist)
				mask |= 1 << f;
		}

		aggregatemask &= mask;
	}

	return aggregatemask != 0;
}

// should be able to handle all mesh types
static qboolean R_Mesh_CullModel (void)
{
	int		i;
	vec3_t	vectors[3];
	vec3_t  angles;
	vec3_t	dist;
	vec3_t	bbox[8];
	
	if ((r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		return !(currententity->flags & RF_MENUMODEL);
	
	if (!cl_gun->integer && (currententity->flags & RF_WEAPONMODEL))
		return true;
	
	if ((currententity->flags & RF_WEAPONMODEL))
		return r_lefthand->integer == 2;

	//Do not cull large meshes - too many artifacts occur
	if((currentmodel->maxs[0] - currentmodel->mins[0] > 72 || currentmodel->maxs[1] - currentmodel->mins[1] > 72 
		|| currentmodel->maxs[2] - currentmodel->mins[2] > 72) && currententity->frame < 199)
		return false;
	
	/*
	** rotate the bounding box
	*/
	VectorCopy (currententity->angles, angles);
	angles[YAW] = -angles[YAW];
	AngleVectors (angles, vectors[0], vectors[1], vectors[2]);

	for (i = 0; i < 8; i++)
	{
		vec3_t tmp;

		VectorCopy (currentmodel->bbox[i], tmp);

		bbox[i][0] = DotProduct (vectors[0], tmp);
		bbox[i][1] = -DotProduct (vectors[1], tmp);
		bbox[i][2] = DotProduct (vectors[2], tmp);

		VectorAdd (currententity->origin, bbox[i], bbox[i]);
	}
	
	// Occlusion culling-- check if *any* part of the mesh is visible. 
	// Possible to have meshes culled that shouldn't be, but quite rare.
	// HACK: culling rocks is currently too slow, so we don't.
	// TODO: parallelize?
	if (r_worldmodel && currentmodel->type != mod_terrain && currentmodel->type != mod_decal && !strstr (currentmodel->name, "rock"))
	{
		qboolean unblocked = CM_FastTrace (currententity->origin, r_origin, r_worldmodel->firstnode, MASK_OPAQUE);
		for (i = 0; i < 8 && !unblocked; i++)
			unblocked = CM_FastTrace (bbox[i], r_origin, r_worldmodel->firstnode, MASK_OPAQUE);
		
		if (!unblocked)
			return true;
	}
	
	VectorSubtract(r_origin, currententity->origin, dist);

	// Keep nearby meshes so shadows don't blatantly disappear when out of frustom
	if (VectorLength(dist) > 150 && R_Mesh_CullBBox (bbox))
		return true;
	
	// TODO: could probably find a better place for this.
	if (r_ragdolls->integer && (currentmodel->typeFlags & MESH_SKELETAL) && !currententity->ragdoll)
	{
		//Ragdolls take over at beginning of each death sequence
		if	(	!(currententity->flags & RF_TRANSLUCENT) &&
				currentmodel->hasRagDoll && 
				(currententity->frame == 199 || 
				currententity->frame == 220 ||
				currententity->frame == 238)
			)
		{
			RGD_AddNewRagdoll (currententity->origin, currententity->name);
		}
		//Do not render deathframes if using ragdolls - do not render translucent helmets
		if ((currentmodel->hasRagDoll || (currententity->flags & RF_TRANSLUCENT)) && currententity->frame > 198)
			return true;
	}

	return false;
}

static void R_Mesh_SetupAnimUniforms (mesh_anim_uniform_location_t *uniforms)
{
	int			animtype;
	
	// only applicable to MD2
	qboolean	lerped;
	float		frontlerp;
	
	lerped = currententity->backlerp != 0.0 && (currententity->frame != 0 || currentmodel->num_frames != 1);
	frontlerp = 1.0 - currententity->backlerp;
	
	animtype = 0;
	
	if ((currentmodel->typeFlags & MESH_MORPHTARGET) && lerped)
		animtype |= 2;
	
	if ((currentmodel->typeFlags & MESH_SKELETAL))
	{
		static matrix3x4_t outframe[SKELETAL_MAX_BONEMATS];
		assert (currentmodel->type == mod_iqm);
		IQM_AnimateFrame (outframe);
		glUniformMatrix3x4fvARB (uniforms->outframe, currentmodel->num_joints, GL_FALSE, (const GLfloat *) outframe);
		animtype |= 1;
	}
	
	// XXX: the vertex shader won't actually support a value of 3 for animtype
	// yet.
	
	glUniform1iARB (uniforms->useGPUanim, animtype);
	glUniform1fARB (uniforms->lerp, frontlerp);
}

// Cobbled together from two different functions. TODO: clean this mess up!
static void R_Mesh_SetupStandardRender (int skinnum, rscript_t *rs, vec3_t lightcolor, qboolean fragmentshader, qboolean shell)
{
	float alpha;
	mesh_uniform_location_t *uniforms = fragmentshader ? &mesh_uniforms[CUR_NUM_DLIGHTS] : &mesh_vertexonly_uniforms[CUR_NUM_DLIGHTS];
	
	if (shell || (currentmodel->typeFlags & MESH_DECAL))
	{
		GLSTATE_ENABLE_BLEND
	}
	else
	{
		GLSTATE_ENABLE_ALPHATEST
	}
	
	// FIXME HACK
	// TODO: other meshes might want dynamic shell alpha too!
	if (!shell)
		alpha = 1.0f;
	if (currententity->ragdoll)
		alpha = currententity->shellAlpha;
	else
		alpha = 0.33f;
	
	if (fragmentshader)
		glUseProgramObjectARB (g_meshprogramObj[CUR_NUM_DLIGHTS]);
	else
		glUseProgramObjectARB (g_vertexonlymeshprogramObj[CUR_NUM_DLIGHTS]);
	
	R_SetDlightUniforms (&uniforms->dlight_uniforms);
	
	// FIXME: the GLSL shader doesn't support shell alpha!
	glUniform3fARB (uniforms->staticLightColor, shadelight[0], shadelight[1], shadelight[2]);
	
	{
		vec3_t lightVec, tmp;
		VectorSubtract (statLightPosition, currententity->origin, tmp);
		VectorMA (statLightPosition, 5.0, tmp, tmp);
		R_ModelViewTransform (tmp, lightVec);
		glUniform3fARB (uniforms->staticLightPosition, lightVec[0], lightVec[1], lightVec[2]);
	}
	
	GL_MBind (0, shell ? r_shelltexture2->texnum : skinnum);
	glUniform1iARB (uniforms->baseTex, 0);

	if (fragmentshader)
	{
		glUniform1iARB (uniforms->normTex, 1);
		
		if (!shell)
		{
			GL_MBind (1, rs->stage->texture->texnum);

			GL_MBind (2, rs->stage->texture2->texnum);
			glUniform1iARB (uniforms->fxTex, 2);

			GL_MBind (3, rs->stage->texture3->texnum);
			glUniform1iARB (uniforms->fx2Tex, 3);
		}
		else
		{
			GL_MBind (1, r_shellnormal->texnum);
		}
		
		glUniform1iARB (uniforms->useFX, shell ? 0 : rs->stage->fx);
		glUniform1iARB (uniforms->useGlow, shell ? 0 : rs->stage->glow);
		glUniform1iARB (uniforms->useCube, shell ? 0 : rs->stage->cube);
		
		if ((currententity->flags & RF_WEAPONMODEL))
			glUniform1iARB (uniforms->fromView, 1);
		else
			glUniform1iARB (uniforms->fromView, 0);
		
		// for subsurface scattering, we hack in the old algorithm.
		if (!shell && !rs->stage->cube)
		{
			vec3_t lightVec, tmp;
			VectorSubtract (totalLightPosition, currententity->origin, tmp);
			VectorMA (totalLightPosition, 5.0, tmp, tmp);
			R_ModelViewTransform (tmp, lightVec);
			glUniform3fARB (uniforms->totalLightPosition, lightVec[0], lightVec[1], lightVec[2]);
			glUniform3fARB (uniforms->totalLightColor, totallight[0], totallight[1], totallight[2]);
		}
	}
	
	glUniform1fARB (uniforms->time, rs_realtime);
	
	glUniform1iARB (uniforms->fog, map_fog);
	glUniform1iARB (uniforms->team, currententity->team);

	glUniform1fARB (uniforms->useShell, shell ? (currententity->ragdoll?1.6:0.4) : 0.0);
	
	glUniform1iARB (uniforms->doShading, (currentmodel->typeFlags & MESH_DOSHADING) != 0);
	
	R_Mesh_SetupAnimUniforms (&uniforms->anim_uniforms);
	
	// set up the fixed-function pipeline too
	if (!fragmentshader)
		qglColor4f (shadelight[0], shadelight[1], shadelight[2], alpha);
}

static void R_Mesh_SetupGlassRender (void)
{
	vec3_t lightVec, left, up;
	int type;
	qboolean mirror, glass;
	
	mirror = glass = false;
	
	if ((r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		glass = true;
	else if (gl_mirror->integer)
		mirror = true;
	else
		glass = true;
	
	if (mirror && !(currententity->flags & RF_WEAPONMODEL))
		glass = true;
	
	glUseProgramObjectARB (g_glassprogramObj);
	
	R_ModelViewTransform (statLightPosition, lightVec);
	glUniform3fARB (glass_uniforms.lightPos, lightVec[0], lightVec[1], lightVec[2]);

	AngleVectors (currententity->angles, NULL, left, up);
	glUniform3fARB (glass_uniforms.left, left[0], left[1], left[2]);
	glUniform3fARB (glass_uniforms.up, up[0], up[1], up[2]);
	
	type = 0;
	
	if (glass)
	{
		glUniform1iARB (glass_uniforms.refTexture, 0);
		GL_MBind (0, r_mirrorspec->texnum);
		type |= 2;
	}
	
	if (mirror)
	{
		glUniform1iARB (glass_uniforms.mirTexture, 1);
		GL_MBind (1, r_mirrortexture->texnum);
		type |= 1;
	}
	
	glUniform1iARB (glass_uniforms.type, type);
	
	glUniform1iARB (glass_uniforms.fog, map_fog);
	
	GLSTATE_ENABLE_BLEND
	GL_BlendFunction (GL_ONE, GL_ONE);
	
	R_Mesh_SetupAnimUniforms (&glass_uniforms.anim_uniforms);
}

// Should be able to handle all mesh types. This is the component of the 
// mesh rendering process that does not need to care which GLSL shader is
// being used-- they all support the same vertex data and vertex attributes.
static void R_Mesh_DrawVBO (qboolean lerped)
{
	const int typeFlags = currentmodel->typeFlags;
	const int frames_idx =  ((typeFlags & MESH_INDEXED) ? 2 : 1);
	const int framenum = ((typeFlags & MESH_MORPHTARGET) ? currententity->frame : 0);
	const size_t framestride = sizeof(mesh_framevbo_t);
	const size_t basestride = (typeFlags & MESH_SKELETAL) ? sizeof(skeletal_basevbo_t) : sizeof(nonskeletal_basevbo_t);
	
	// base (non-frame-specific) data
	GL_BindVBO (currentmodel->vboIDs[0]);
	R_TexCoordPointer (0, basestride, (void *)FOFS (nonskeletal_basevbo_t, st));
	if ((typeFlags & MESH_SKELETAL))
	{
		// Note: bone positions are sent separately as uniforms to the GLSL shader.
		R_AttribPointer (ATTR_WEIGHTS_IDX, 4, GL_UNSIGNED_BYTE, GL_TRUE, basestride, (void *)FOFS (skeletal_basevbo_t, blendweights));
		R_AttribPointer (ATTR_BONES_IDX, 4, GL_UNSIGNED_BYTE, GL_FALSE, basestride, (void *)FOFS (skeletal_basevbo_t, blendindices));
	}
	
	// primary frame data
	GL_BindVBO (currentmodel->vboIDs[framenum + frames_idx]);
	R_VertexPointer (3, framestride, (void *)FOFS (mesh_framevbo_t, vertex));
	R_NormalPointer (framestride, (void *)FOFS (mesh_framevbo_t, normal));
	R_AttribPointer (ATTR_TANGENT_IDX, 4, GL_FLOAT, GL_FALSE, framestride, (void *)FOFS (mesh_framevbo_t, tangent));
	
	// secondary frame data
	if ((typeFlags & MESH_MORPHTARGET) && lerped)
	{
		// Note: the lerp position is sent separately as a uniform to the GLSL shader.
		GL_BindVBO (currentmodel->vboIDs[currententity->oldframe + frames_idx]);
		R_AttribPointer (ATTR_OLDVTX_IDX, 3, GL_FLOAT, GL_FALSE, framestride, (void *)FOFS (mesh_framevbo_t, vertex));
		R_AttribPointer (ATTR_OLDNORM_IDX, 3, GL_FLOAT, GL_FALSE, framestride, (void *)FOFS (mesh_framevbo_t, normal));
		R_AttribPointer (ATTR_OLDTAN_IDX, 4, GL_FLOAT, GL_FALSE, framestride, (void *)FOFS (mesh_framevbo_t, tangent));
	}
	
	GL_BindVBO (0);
	
	// render
	if ((typeFlags & MESH_INDEXED))
	{
		GL_BindIBO (currentmodel->vboIDs[1]);
		qglDrawElements (GL_TRIANGLES, currentmodel->num_triangles*3, GL_UNSIGNED_INT, 0);
		GL_BindIBO (0);
	}
	else
	{
		qglDrawArrays (GL_TRIANGLES, 0, currentmodel->num_triangles*3);
	}
	
	R_KillVArrays ();
}

static void R_Mesh_DrawVBO_Callback (void)
{
	R_Mesh_DrawVBO (false);
}

/*
=============
R_Mesh_DrawFrame: should be able to handle all types of meshes.
=============
*/
static void R_Mesh_DrawFrame (int skinnum)
{
	vec3_t		lightcolor;
	
	// only applicable to MD2
	qboolean	lerped;
	
	// if true, then render through the RScript code
	qboolean	rs_slowpath = false;
	
	rscript_t	*rs = NULL;
	
	if (r_shaders->integer)
		rs=(rscript_t *)currententity->script;
	
	//check for valid script
	if (rs && rs->stage)
	{
		if (!strcmp("***r_notexture***", rs->stage->texture->name) || 
			((rs->stage->fx || rs->stage->glow) && !strcmp("***r_notexture***", rs->stage->texture2->name)) ||
			(rs->stage->cube && !strcmp("***r_notexture***", rs->stage->texture3->name)) ||
			rs->stage->num_blend_textures != 0 || rs->stage->next != NULL || currentmodel->lightmap != NULL )
		{
			rs_slowpath = true;
		}
	}
	
	lerped = currententity->backlerp != 0.0 && (currententity->frame != 0 || currentmodel->num_frames != 1);
	
	VectorCopy (shadelight, lightcolor);
	VectorNormalize (lightcolor);
	
	if ((((currentmodel->typeFlags & MESH_MORPHTARGET) && lerped) || (currentmodel->typeFlags & MESH_SKELETAL) != 0) && rs_slowpath)
	{
		// FIXME: rectify this.
		Com_Printf ("WARN: Cannot apply a multi-stage RScript %s to model %s\n", rs->outname, currentmodel->name);
		rs = NULL;
		rs_slowpath = false;
	}
	
	if (rs_slowpath)
	{
		int lmtex = 0;
		
		if (currentmodel->lightmap != NULL)
			lmtex = currentmodel->lightmap->texnum;
		
		RS_Draw (	rs, lmtex, vec3_origin, vec3_origin, false,
					lmtex != 0 ? rs_lightmap_on : rs_lightmap_off,
					true, R_Mesh_DrawVBO_Callback );
		
		return;
	}
	else if ((currententity->flags & RF_TRANSLUCENT) && !(currententity->flags & RF_SHELL_ANY))
	{
		qglDepthMask(false);
		
		R_Mesh_SetupGlassRender ();
	}
	else
	{
		qboolean fragmentshader;
		
		if (rs && rs->stage->depthhack)
			qglDepthMask(false);
		
		if (currententity->flags & RF_SHELL_ANY)
		{
			fragmentshader = gl_normalmaps->integer;
			R_Mesh_SetupStandardRender (skinnum, rs, lightcolor, fragmentshader, true);
		}
		else
		{
			fragmentshader = rs && rs->stage->normalmap && gl_normalmaps->integer;
			R_Mesh_SetupStandardRender (skinnum, rs, lightcolor, fragmentshader, false);
		}
	}
	
	R_Mesh_DrawVBO (lerped);
	
	glUseProgramObjectARB (0);
	qglDepthMask (true);

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_TEXGEN
	
	// FIXME: make this unnecessary
	// Without this, the player options menu goes all funny due to
	// R_Mesh_SetupGlassRender changing the blendfunc. The proper solution is
	// to just never rely on the blendfunc being any default value.
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

static void R_Mesh_DrawBlankMeshFrame (void)
{
	glUseProgramObjectARB (g_blankmeshprogramObj);
	R_Mesh_SetupAnimUniforms (&blankmesh_uniforms);
	
	R_Mesh_DrawVBO (currententity->frame != 0 || currentmodel->num_frames != 1);

	glUseProgramObjectARB (0);
}

static void R_Mesh_SetShadelight (void)
{
	int i;
	
	if ((currententity->flags & RF_SHELL_ANY))
	{
		VectorClear (shadelight);
		if ((currententity->flags & RF_SHELL_HALF_DAM))
			VectorSet (shadelight, 0.56, 0.59, 0.45);
		if ((currententity->flags & RF_SHELL_DOUBLE))
		{
			shadelight[0] = 0.9;
			shadelight[1] = 0.7;
		}
		if ((currententity->flags & RF_SHELL_RED))
			shadelight[0] = 1.0;
		if ((currententity->flags & RF_SHELL_GREEN))
		{
			shadelight[1] = 1.0;
			shadelight[2] = 0.6;
		}
		if ((currententity->flags & RF_SHELL_BLUE))
		{
			shadelight[2] = 1.0;
			shadelight[0] = 0.6;
		}
	}
	else if ((currententity->flags & RF_FULLBRIGHT) || currentmodel->type == mod_terrain || currentmodel->type == mod_decal)
	{
		// Treat terrain as fullbright for now. TODO: move to precompiled lightmaps for terrain.
		VectorSet (shadelight, 1.0, 1.0, 1.0);
	}
	else
	{
		VectorCopy (worldlight, shadelight); // worldlight is set in R_GetLightVals
	}
	if ((currententity->flags & RF_MINLIGHT))
	{
		float minlight;

		if (gl_normalmaps->integer)
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

	if ((currententity->flags & RF_GLOW))
	{	// bonus items will pulse with time
		float	scale;
		float	minlight;

		scale = 0.2 * sin(r_newrefdef.time*7);
		if (gl_normalmaps->integer)
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

/*
=================
R_Mesh_Draw - animate and render a mesh. Should support all mesh types.
=================
*/
void R_Mesh_Draw (void)
{
	image_t		*skin;

	if (R_Mesh_CullModel ())
		return;
	
	R_GetLightVals (currententity->origin, false);

	if ((currentmodel->typeFlags & MESH_CASTSHADOWMAP))
		R_GenerateEntityShadow();
	
	// Don't render your own avatar unless it's for shadows
	if ((currententity->flags & RF_VIEWERMODEL))
		return;
	
	if (currentmodel->type == mod_md2)
		MD2_SelectFrame ();
	// New model types go here

	R_Mesh_SetShadelight ();

	if (!(currententity->flags & RF_WEAPONMODEL))
		c_alias_polys += currentmodel->num_triangles; /* for rspeed_epoly count */

	if ((currententity->flags & RF_DEPTHHACK)) // hack the depth range to prevent view model from poking into walls
		qglDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));

	if ((currententity->flags & RF_WEAPONMODEL))
	{
		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();

		if (r_lefthand->integer == 1)
		{
			qglScalef(-1, 1, 1);
			qglCullFace(GL_BACK);
		}
		if (r_newrefdef.fov_y < 75.0f)
			MYgluPerspective(r_newrefdef.fov_y, (float)r_newrefdef.width / (float)r_newrefdef.height, 4.0f, 4096.0f);
		else
			MYgluPerspective(75.0f, (float)r_newrefdef.width / (float)r_newrefdef.height, 4.0f, 4096.0f);

		qglMatrixMode(GL_MODELVIEW);
	}


	qglPushMatrix ();
	if (!currententity->ragdoll) // HACK
		R_RotateForEntity (currententity);

	// select skin
	if (currententity->skin) 
		skin = currententity->skin;
	else
		skin = currentmodel->skins[0];
	if (!skin)
		skin = r_notexture;	// fallback..
	
	qglShadeModel (GL_SMOOTH);
	GL_MTexEnv (0, GL_MODULATE);
	
	if ((currentmodel->typeFlags & MESH_DECAL))
	{
		qglEnable (GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset (-3, -2);
		qglDepthMask (false);
	}

	R_Mesh_DrawFrame (skin->texnum);
	
	if ((currentmodel->typeFlags & MESH_DECAL))
	{
		qglDisable (GL_POLYGON_OFFSET_FILL);
		qglDepthMask (true);
	}
	
	if (((currentmodel->typeFlags & MESH_DECAL) && gl_showdecals->integer > 1) || gl_showtris->integer)
	{
		qglLineWidth (3.0);
		qglPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
		qglDepthMask (false);
		if (gl_showtris->integer > 1) qglDisable (GL_DEPTH_TEST);
		R_Mesh_DrawBlankMeshFrame ();
		if (gl_showtris->integer > 1) qglEnable (GL_DEPTH_TEST);
		qglDepthMask (true);
		qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		qglLineWidth (1.0);
	}
	
	GL_MTexEnv (0, GL_REPLACE);
	qglShadeModel (GL_FLAT);

	qglPopMatrix ();
	
	if ((currententity->flags & RF_WEAPONMODEL))
	{
		qglMatrixMode (GL_PROJECTION);
		qglPopMatrix ();
		qglMatrixMode (GL_MODELVIEW);
		qglCullFace (GL_FRONT);
	}

	if ((currententity->flags & RF_DEPTHHACK)) // restore depth range
		qglDepthRange (gldepthmin, gldepthmax);
	
	qglColor4f (1,1,1,1);

	if (r_minimap->integer && !currententity->ragdoll)
	{
		if ((currententity->flags & RF_MONSTER))
		{
			Vector4Set (RadarEnts[numRadarEnts].color, 1.0, 0.0, 2.0, 1.0);
			VectorCopy (currententity->origin, RadarEnts[numRadarEnts].org);
			numRadarEnts++;
		}
	}
}

// TODO - alpha and alphamasks possible?
// Should support every mesh type
void R_Mesh_DrawCaster (void)
{
	// don't draw weapon model shadow casters or shells
	if ((currententity->flags & (RF_WEAPONMODEL|RF_SHELL_ANY)))
		return;

	if (!(currententity->flags & RF_VIEWERMODEL) && R_Mesh_CullModel ())
		return;

	qglPushMatrix ();
	if (!currententity->ragdoll) // HACK
		R_RotateForEntity (currententity);
	
	if (currentmodel->type == mod_md2)
		MD2_SelectFrame ();
	// New model types go here

	R_Mesh_DrawBlankMeshFrame ();

	qglPopMatrix();
}
