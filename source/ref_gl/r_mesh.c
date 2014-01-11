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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

static vertCache_t	*vbo_st;
static vertCache_t	*vbo_xyz;
static vertCache_t	*vbo_normals;
static vertCache_t	*vbo_tangents;
static vertCache_t	*vbo_indices;

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

// should be able to handle all mesh types
void R_Mesh_FindVBO (model_t *mod, int framenum)
{
	if (!modtypes[mod->type].morphtarget)
		framenum = 0;

	vbo_st = R_VCFindCache(VBO_STORE_ST, mod);	
	vbo_xyz = R_VCFindCache(VBO_STORE_XYZ+framenum, mod);
	vbo_normals = R_VCFindCache(VBO_STORE_NORMAL+framenum, mod);
	vbo_tangents = R_VCFindCache(VBO_STORE_TANGENT+framenum, mod);
	
	if (!vbo_xyz || !vbo_st || !vbo_normals || !vbo_tangents)
		Com_Error (ERR_DROP, "Cannot find VBO for %s frame %d\n", mod->name, framenum);
	
	if (modtypes[mod->type].indexed)
	{
		vbo_indices = R_VCFindCache(VBO_STORE_INDICES, mod);
		if (!vbo_indices)
			Com_Error (ERR_DROP, "Cannot find IBO for %s frame %d\n", mod->name, framenum);
	}
}

// should be able to handle all mesh types
void R_Mesh_FreeVBO (model_t *mod)
{
	int framenum, maxframes = 1;
	
	if (modtypes[mod->type].morphtarget)
		maxframes = mod->num_frames;
	
	for (framenum = 0; framenum < maxframes; framenum++)
	{
		R_Mesh_FindVBO (mod, framenum);
		R_VCFree (vbo_xyz);
		R_VCFree (vbo_st);
		R_VCFree (vbo_normals);
		R_VCFree (vbo_tangents);
	}
	
	// indices should be the same for every frame
	if (modtypes[mod->type].indexed)
		R_VCFree (vbo_indices);
}

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
			if (currentmodel->type == mod_terrain)
			{
				r_trace.fraction = 1.0; //terrain meshes can actually occlude themselves. TODO: move to precompiled lightmaps for terrain.
			}
			if (!RagDoll && (currententity->flags & RF_WEAPONMODEL) && (LightGroups[i].group_origin[2] > meshOrigin[2]))
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

// TODO: does this actually do a different job from R_CullBox?
qboolean R_Mesh_CullBBox (vec3_t bbox[8])
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

	return aggregatemask != 0;
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
	
	if (r_worldmodel && currentmodel->type != mod_terrain) {
		//occulusion culling - why draw entities we cannot see?
		trace_t r_trace = CM_BoxTrace(r_origin, currententity->origin, currentmodel->maxs, currentmodel->mins, r_worldmodel->firstnode, MASK_OPAQUE);
		if(r_trace.fraction != 1.0)
			return true;
	}
	
	if (currentmodel->type == mod_md2)
		return MD2_CullModel ();
	else if (currentmodel->type == mod_iqm)
		return IQM_CullModel ();
	// New model types go here

	return false;
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
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
	
	mirror = glass = false;
	
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
	GL_BlendFunction (GL_ONE, GL_ONE);
}

// Should be able to handle all mesh types. This is the component of the 
// mesh rendering process that does not need to care which GLSL shader is
// being used-- they all support the same vertex data and vertex attributes.
void R_Mesh_DrawVBO (qboolean lerped)
{
	// setup
	R_Mesh_FindVBO (currentmodel, currententity->frame);
	
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
	
	// Note: the lerp position is sent separately as a uniform to the GLSL shader.
	if (modtypes[currentmodel->type].morphtarget && lerped)
	{
		R_Mesh_FindVBO (currentmodel, currententity->oldframe);
		
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
	
	// Note: bone positions are sent separately as uniforms to the GLSL shader.
	if (modtypes[currentmodel->type].skeletal)
	{
		glEnableVertexAttribArrayARB(ATTR_WEIGHTS_IDX);
		glVertexAttribPointerARB(ATTR_WEIGHTS_IDX, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(byte)*4, currentmodel->blendweights);
		glEnableVertexAttribArrayARB(ATTR_BONES_IDX);
		glVertexAttribPointerARB(ATTR_BONES_IDX, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(byte)*4, currentmodel->blendindexes);
	}
	
	// render
	if (modtypes[currentmodel->type].indexed)
	{
		GL_BindIBO(vbo_indices);
		qglDrawElements(GL_TRIANGLES, currentmodel->num_triangles*3, GL_UNSIGNED_INT, 0);
		GL_BindIBO(NULL);
	}
	else
	{
		qglDrawArrays (GL_TRIANGLES, 0, currentmodel->num_triangles*3);
	}
	
	// cleanup
	R_KillVArrays ();
	
	glDisableVertexAttribArrayARB (ATTR_TANGENT_IDX);
	
	if (modtypes[currentmodel->type].skeletal)
	{
		glDisableVertexAttribArrayARB(ATTR_WEIGHTS_IDX);
		glDisableVertexAttribArrayARB(ATTR_BONES_IDX);
	}
	
	if (lerped)
	{
		glDisableVertexAttribArrayARB (ATTR_OLDVTX_IDX);
		glDisableVertexAttribArrayARB (ATTR_OLDNORM_IDX);
		glDisableVertexAttribArrayARB (ATTR_OLDTAN_IDX);
	}
}

void R_Mesh_DrawVBO_Callback (void)
{
	R_Mesh_DrawVBO (false);
}

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
	
	// if true, then render through the RScript code
	qboolean	rs_slowpath = false;
	
	int			animtype = 0; // GLSL useGPUanim uniform
	
	rscript_t	*rs = NULL;
	
	if (r_shaders->integer)
		rs=(rscript_t *)currententity->script;
	
	//check for valid script
	if(rs && rs->stage)
	{
		if(	!strcmp("***r_notexture***", rs->stage->texture->name) || 
			((rs->stage->fx || rs->stage->glow) && !strcmp("***r_notexture***", rs->stage->texture2->name)) ||
			(rs->stage->cube && !strcmp("***r_notexture***", rs->stage->texture3->name)) ||
			rs->stage->num_blend_textures != 0 || rs->stage->next != NULL )
		{
			rs_slowpath = true;
		}
	}
	
	lerped = currententity->backlerp != 0.0 && (currententity->frame != 0 || currentmodel->num_frames != 1);
	
	VectorCopy(shadelight, lightcolor);
	for (i=0;i<model_dlights_num;i++)
		VectorAdd(lightcolor, model_dlights[i].color, lightcolor);
	VectorNormalize(lightcolor);

	frontlerp = 1.0 - currententity->backlerp;
	
	if (modtypes[currentmodel->type].morphtarget && lerped)
		animtype |= 2;
	
	if (modtypes[currentmodel->type].skeletal)
		animtype |= 1;
	
	// XXX: the vertex shader won't actually support a value of 3 for animtype
	// yet.
	
	if (animtype != 0 && rs_slowpath)
	{
		Com_Printf ("WARN: Cannot apply a multi-stage RScript to model %s\n", currentmodel->name);
		rs = NULL;
		rs_slowpath = false;
	}
	
	if (rs_slowpath)
	{
	    int lmtex = 0;
	    
	    if (currentmodel->lightmap != NULL)
	        lmtex = currentmodel->lightmap->texnum;
	    
		RS_Draw (rs, lmtex, vec3_origin, vec3_origin, false, false, R_Mesh_DrawVBO_Callback);
		
		return;
	}
	else if ((currententity->flags & RF_TRANSLUCENT) && !(currententity->flags & RF_SHELL_ANY))
	{
		qglDepthMask(false);
		
		R_Mesh_SetupGlassRender ();
		
		glUniform1iARB (g_location_g_useGPUanim, animtype);
		glUniform1fARB (g_location_g_lerp, frontlerp);
		
		if (modtypes[currentmodel->type].skeletal)
			glUniformMatrix3x4fvARB (g_location_g_outframe, currentmodel->num_joints, GL_FALSE, (const GLfloat *) currentmodel->outframe );
	}
	else
	{
		qboolean fragmentshader;
		
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
		
		glUniform1iARB (MESH_UNIFORM(useGPUanim), animtype);
		glUniform1fARB (MESH_UNIFORM(lerp), frontlerp);
		
		if (modtypes[currentmodel->type].skeletal)
			glUniformMatrix3x4fvARB (MESH_UNIFORM(outframe), currentmodel->num_joints, GL_FALSE, (const GLfloat *) currentmodel->outframe );
	}
	
	R_Mesh_DrawVBO (lerped);
	
	glUseProgramObjectARB( 0 );
	qglDepthMask(true);

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_TEXGEN
	
	// FIXME: make this unnecessary
	// Without this, the player options menu goes all funny due to
	// R_Mesh_SetupGlassRender changing the blendfunc. The proper solution is
	// to just never rely on the blendfunc being any default value.
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
	else if ((currententity->flags & RF_FULLBRIGHT) || currentmodel->type == mod_terrain)
	{
		// Treat terrain as fullbright for now. TODO: move to precompiled lightmaps for terrain.
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

	if (currentmodel->type != mod_md2) //TODO: use this for MD2 as well
		R_GenerateEntityShadow();
	
	// Don't render your own avatar unless it's for shadows
	if ((currententity->flags & RF_VIEWERMODEL))
		return;
	
	if (currentmodel->type == mod_md2)
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


	qglPushMatrix ();
	R_RotateForEntity (currententity);

	// select skin
	if (currententity->skin) 
		skin = currententity->skin;
	else
		skin = currentmodel->skins[0];
	if (!skin)
		skin = r_notexture;	// fallback..
	
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

void R_Mesh_DrawCasterFrame (float backlerp, qboolean lerped)
{
	int animtype = 0; // GLSL useGPUanim uniform
	
	glUseProgramObjectARB (g_blankmeshprogramObj);
	
	if (modtypes[currentmodel->type].morphtarget && lerped)
		animtype |= 2;
	
	if (modtypes[currentmodel->type].skeletal)
	{
		glUniformMatrix3x4fvARB( g_location_bm_outframe, currentmodel->num_joints, GL_FALSE, (const GLfloat *) currentmodel->outframe );
		animtype |= 1;
	}
	
	glUniform1iARB(g_location_bm_useGPUanim, animtype);
	glUniform1fARB(g_location_bm_lerp, 1.0-backlerp);
	
	R_Mesh_DrawVBO (lerped);

	glUseProgramObjectARB (0);
}

// TODO - alpha and alphamasks possible?
// Should support every mesh type
void R_Mesh_DrawCaster ( void )
{
	if ( currententity->flags & RF_WEAPONMODEL ) //don't draw weapon model shadow casters
		return;

	if ( currententity->flags & RF_SHELL_ANY ) //no shells
		return;

	if (!(currententity->flags & RF_VIEWERMODEL) && R_Mesh_CullModel ())
		return;

	qglPushMatrix ();
	R_RotateForEntity (currententity);
	
	if (currentmodel->type == mod_md2)
		MD2_SelectFrame ();
	else if (currentmodel->type == mod_iqm)
		IQM_AnimateFrame ();
	// New model types go here

	if(currententity->frame == 0 && currentmodel->num_frames == 1)
		R_Mesh_DrawCasterFrame(0, false);
	else
		R_Mesh_DrawCasterFrame(currententity->backlerp, true);

	qglPopMatrix();
}
