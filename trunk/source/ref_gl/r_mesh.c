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

#include "r_local.h"
#include "vlights.h"

/*
=============================================================

  ALIAS MODELS

=============================================================
*/

#define NUMVERTEXNORMALS	162

float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};

static	vec4_t	s_lerped[MAX_VERTS];

static	vec3_t	s_normals[MAX_VERTS];

extern	vec3_t			lightspot;
vec3_t	shadevector;
float	shadelight[3];
vec3_t	lightdir;

#define MAX_MODEL_DLIGHTS 128
m_dlight_t model_dlights[MAX_MODEL_DLIGHTS];
int model_dlights_num;

extern  void GL_BlendFunction (GLenum sfactor, GLenum dfactor);
extern rscript_t *rs_glass;

extern cvar_t *cl_gun;

// precalculated dot products for quantized angles
#define SHADEDOT_QUANT 16
float	r_avertexnormal_dots[SHADEDOT_QUANT][256] =
#include "anormtab.h"
;

float	*shadedots = r_avertexnormal_dots[0];


void GL_LerpVerts( int nverts, dtrivertx_t *v, dtrivertx_t *ov, dtrivertx_t *verts, float *lerp, float move[3], float frontv[3], float backv[3] )
{
	int i;
	int shellscale;

	//PMM -- added RF_SHELL_DOUBLE, RF_SHELL_HALF_DAM
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
	{

		if(currententity->flags & RF_WEAPONMODEL)
			//change scale
			shellscale = .1;
		else
			shellscale = 1;

		for (i=0 ; i < nverts; i++, v++, ov++, lerp+=4 )
		{
			float *normal = r_avertexnormals[verts[i].lightnormalindex];

			lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0] + normal[0] * POWERSUIT_SCALE * shellscale;;
			lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1] + normal[1] * POWERSUIT_SCALE * shellscale;;
			lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2] + normal[2] * POWERSUIT_SCALE * shellscale;;
		}
	}
	else
	{
		for (i=0 ; i < nverts; i++, v++, ov++, lerp+=4)
		{
			lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0];
			lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1];
			lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2];
		}
	}

}

void GL_VlightAliasModel (vec3_t baselight, dtrivertx_t *verts, dtrivertx_t *ov, float backlerp, vec3_t lightOut)
{
	int i;
	float l;

	l = 2.0 * VLight_LerpLight( verts->lightnormalindex,
							ov->lightnormalindex,
							backlerp, lightdir, currententity->angles, false );

	VectorScale(baselight, l, lightOut);

	if (model_dlights_num)
		for (i=0;i<model_dlights_num;i++)
		{

			l = 2.0*VLight_LerpLight( verts->lightnormalindex,
									ov->lightnormalindex,
									backlerp, model_dlights[i].direction, currententity->angles, true );

			VectorMA(lightOut, l, model_dlights[i].color, lightOut);
		}

	for (i=0;i<3;i++)
	{
		if (lightOut[i]<0) lightOut[i] = 0;
		if (lightOut[i]>1) lightOut[i] = 1;
	}
}
/*
=============
GL_DrawAliasFrameLerp

interpolates between two frames and origins
FIXME: batch lerp all vertexes
=============
*/
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

float	tex_array[MAX_ARRAY][2];
float	vert_array[MAX_ARRAY][3];
float	col_array[MAX_ARRAY][4];
void GL_DrawAliasFrameLerp (dmdl_t *paliashdr, float backlerp)
{
	daliasframe_t	*frame, *oldframe;
	dtrivertx_t	*v, *ov, *verts;
	int		*order, *startorder, *tmp_order;
	int		count, tmp_count;
	float	frontlerp, l;
	float	alpha, basealpha;
	vec3_t	move, delta, vectors[3];
	vec3_t	frontv, backv;
	int		i;
	int		index_xyz;
	qboolean depthmaskrscipt = false, is_trans = false;
	rscript_t *rs = NULL;
	rs_stage_t *stage = NULL;
	int		va = 0;
	float mode;
	float	*lerp;
	vec3_t lightcolor;
	float ramp = 1.0;

	frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
		+ currententity->frame * paliashdr->framesize);
	verts = v = frame->verts;

	oldframe = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
		+ currententity->oldframe * paliashdr->framesize);
	ov = oldframe->verts;


	startorder = order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

	if (r_shaders->value)
			rs=(rscript_t *)currententity->script;

	VectorCopy(shadelight, lightcolor);
	for (i=0;i<model_dlights_num;i++)
		VectorAdd(lightcolor, model_dlights[i].color, lightcolor);
	VectorNormalize(lightcolor);

	if (currententity->flags & RF_TRANSLUCENT) {
		basealpha = alpha = currententity->alpha;

		rs=(rscript_t *)rs_glass;
		if(!rs)
			GL_Bind(r_reflecttexture->texnum);
	}
	else
		basealpha = alpha = 1.0;

	// PMM - added double shell
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
		GL_Bind(r_shelltexture->texnum);  // add this line

	frontlerp = 1.0 - backlerp;

	// move should be the delta back to the previous frame * backlerp
	VectorSubtract (currententity->oldorigin, currententity->origin, delta);
	AngleVectors (currententity->angles, vectors[0], vectors[1], vectors[2]);

	move[0] = DotProduct (delta, vectors[0]);	// forward
	move[1] = -DotProduct (delta, vectors[1]);	// left
	move[2] = DotProduct (delta, vectors[2]);	// up

	VectorAdd (move, oldframe->translate, move);

	for (i=0 ; i<3 ; i++)
	{
		move[i] = backlerp*move[i] + frontlerp*frame->translate[i];
	}

	for (i=0 ; i<3 ; i++)
	{
		frontv[i] = frontlerp*frame->scale[i];
		backv[i] = backlerp*oldframe->scale[i];
	}

	lerp = s_lerped[0];

	GL_LerpVerts( paliashdr->num_xyz, v, ov, verts, lerp, move, frontv, backv);

	if(currententity->flags & RF_VIEWERMODEL)
		return;

	VectorSubtract(currententity->origin, lightspot, lightdir);
		VectorNormalize ( lightdir );

	qglEnableClientState( GL_COLOR_ARRAY );

	if(( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) ) )
	{
		qglColor4f( shadelight[0], shadelight[1], shadelight[2], alpha);
		R_InitMeshVarrays();
		while (1)
		{
			// get the vertex count and primitive type
			count = *order++;
			va=0;
			if (!count)
				break;		// done
			if (count < 0)
			{
				count = -count;
				mode=GL_TRIANGLE_FAN;
			}
			else
				mode=GL_TRIANGLE_STRIP;

			do
			{
				// texture coordinates come from the draw list
				index_xyz = order[2];

				VA_SetElem2(tex_array[va],(s_lerped[index_xyz][1] + s_lerped[index_xyz][0]) * (1.0f / 40.0f), s_lerped[index_xyz][2] * (1.0f / 40.0f) - r_newrefdef.time * 0.5f);
				VA_SetElem3(vert_array[va],s_lerped[index_xyz][0],s_lerped[index_xyz][1],s_lerped[index_xyz][2]);
				VA_SetElem4(col_array[va], shadelight[0], shadelight[1], shadelight[2], calcEntAlpha(alpha, s_lerped[index_xyz]));
				va++;
				order += 3;
			} while (--count);

			if ( qglLockArraysEXT )
				qglLockArraysEXT( 0, va );
			qglDrawArrays(mode,0,va);
			if ( qglUnlockArraysEXT ) 
				qglUnlockArraysEXT();
		}
	}
	else if(!rs)
	{
		alpha = basealpha;
		R_InitMeshVarrays();
		while (1)
		{

			// get the vertex count and primitive type
			count = *order++;
			va=0;
			if (!count)
				break;		// done
			if (count < 0)
			{
				count = -count;
				mode=GL_TRIANGLE_FAN;
			}
			else
				mode=GL_TRIANGLE_STRIP;

			tmp_count=count;
			tmp_order=order;

			do
			{
				// texture coordinates come from the draw list
				index_xyz = order[2];
				l = shadedots[verts[index_xyz].lightnormalindex];

				if(gl_rtlights->value)
					GL_VlightAliasModel (shadelight, &verts[index_xyz], &ov[index_xyz], backlerp, lightcolor);

				VA_SetElem2(tex_array[va],((float *)order)[0], ((float *)order)[1]);
				VA_SetElem3(vert_array[va],s_lerped[index_xyz][0],s_lerped[index_xyz][1],s_lerped[index_xyz][2]);
				if(gl_rtlights->value)
					VA_SetElem4(col_array[va],lightcolor[0], lightcolor[1], lightcolor[2], calcEntAlpha(alpha, s_lerped[index_xyz]));
				else
					VA_SetElem4(col_array[va], shadelight[0], shadelight[1], shadelight[2], calcEntAlpha(alpha, s_lerped[index_xyz]));
				va++;
				order += 3;
			} while (--count);
			if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) {
				if ( qglLockArraysEXT )
					qglLockArraysEXT( 0, va );
				qglDrawArrays(mode,0,va);
				if ( qglUnlockArraysEXT ) 
					qglUnlockArraysEXT();
			}
	
		}

	}
	else
	{

		if (rs->stage && rs->stage->has_alpha)
		{
			is_trans = true;
			depthmaskrscipt = true;
		}

		if (depthmaskrscipt)
			qglDepthMask(false);

		R_InitMeshVarrays();
		while (1)
		{
			count = *order++;
			if (!count)
				break;		// done
			// get the vertex count and primitive type
			if (count < 0)
			{
				count = -count;
				mode=GL_TRIANGLE_FAN;
			}
			else
			{
				mode=GL_TRIANGLE_STRIP;
			}

			stage=rs->stage;
			tmp_count=count;
			tmp_order=order;

			while (stage)
			{
				count=tmp_count;
				order=tmp_order;
				va=0;

				if (stage->normalmap && !gl_normalmaps->value) {
					if(stage->next) {
						stage = stage->next;
						continue;
					}
				}

				if (stage->colormap.enabled)
					qglDisable (GL_TEXTURE_2D);
				else if (stage->anim_count)
					GL_Bind(RS_Animate(stage));
				else
					GL_Bind (stage->texture->texnum);

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

				if (stage->alphamask)
				{
					GLSTATE_ENABLE_ALPHATEST
				}
				else
				{
					GLSTATE_DISABLE_ALPHATEST
				}

				if(stage->normalmap && gl_normalmaps->value) {

					ramp = 2.0; //for getting brightness of normal maps up a bit

					qglDepthMask (GL_FALSE);
			 		qglEnable (GL_BLEND);

					// set the correct blending mode for normal maps
					qglBlendFunc (GL_ZERO, GL_SRC_COLOR);

					// and the texenv
					qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
					qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_DOT3_RGB_EXT);

				}

				do
				{
					if (!(stage->normalmap && !gl_normalmaps->value)) { //disable for normal stage if normals are disabled

						float os = ((float *)order)[0];
						float ot = ((float *)order)[1];
						vec3_t normal;
						int k;

						index_xyz = order[2];

						for (k=0;k<3;k++)
						normal[k] = r_avertexnormals[verts[index_xyz].lightnormalindex][k] +
						( r_avertexnormals[ov[index_xyz].lightnormalindex][k] -
						r_avertexnormals[verts[index_xyz].lightnormalindex][k] ) * backlerp;
						VectorNormalize ( normal );

						if (stage->envmap)
						{
							vec3_t envmapvec;

							VectorAdd(currententity->origin, s_lerped[index_xyz], envmapvec);
							RS_SetEnvmap (envmapvec, &os, &ot);

							os -= DotProduct (normal , vectors[1] );
							ot += DotProduct (normal, vectors[2] );
						}

						RS_SetTexcoords2D(stage, &os, &ot);

						VA_SetElem2(tex_array[va], os, ot);
						VA_SetElem3(vert_array[va],s_lerped[index_xyz][0],s_lerped[index_xyz][1],s_lerped[index_xyz][2]);

						{
							float red = 1, green = 1, blue = 1, nAlpha;

							nAlpha = RS_AlphaFuncAlias (stage->alphafunc,
							calcEntAlpha(alpha, s_lerped[index_xyz]), normal, s_lerped[index_xyz]);

							if (stage->lightmap)
							{
								if(gl_rtlights->value) {
									GL_VlightAliasModel (shadelight, &verts[index_xyz], &ov[index_xyz], backlerp, lightcolor);
									red = lightcolor[0] * ramp;
									green = lightcolor[1] * ramp;
									blue = lightcolor[2] * ramp;
								}
								else {
									red = shadelight[0] * ramp;
									green = shadelight[1] * ramp;
									blue = shadelight[2] * ramp;
								}
								//try to keep normalmapped stages from going completely dark
								if(stage->normalmap && gl_normalmaps->value) {
									if(red < .6) red = .6;
									if(green < .6) green = .6;
									if(blue < .6) blue = .6;
								}

							}

							if (stage->colormap.enabled)
							{
								red *= stage->colormap.red/255.0;
								green *= stage->colormap.green/255.0;
								blue *= stage->colormap.blue/255.0;
							}

							VA_SetElem4(col_array[va], red, green, blue, nAlpha);
						}
					}
					order += 3;
					va++;
				} while (--count);

				if (!(stage->normalmap && !gl_normalmaps->value)) //disable so that we
					//can still have shaders without normalmapped models if so chosen
				{
					if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) {
						if ( qglLockArraysEXT )
						qglLockArraysEXT( 0, va );
						qglDrawArrays(mode,0,va);
						if ( qglUnlockArraysEXT ) 
							qglUnlockArraysEXT();
					}
				}

				qglColor4f(1,1,1,1);
				if (stage->colormap.enabled)
					qglEnable (GL_TEXTURE_2D);

				if(stage->normalmap && gl_normalmaps->value) {

					ramp = 1.0;
					// back to replace mode
					qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

					// restore the original blend mode
					qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

					// switch off blending
					qglDisable (GL_BLEND);
					qglDepthMask (GL_TRUE);
				}

				stage=stage->next;
			}

		}

		if (depthmaskrscipt)
			qglDepthMask(true);
	}

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_TEXGEN

	qglDisableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM ) )
		qglEnable( GL_TEXTURE_2D );
}

//no lerping, static mesh
void GL_DrawAliasFrame (dmdl_t *paliashdr)
{
	daliasframe_t	*frame;
	dtrivertx_t	*v, *verts;
	int		*order, *startorder, *tmp_order;
	int		count, tmp_count;
	float	l;
	float	alpha, basealpha;
	vec3_t	vectors[3];
	int		i;
	int		index_xyz;
	qboolean depthmaskrscipt = false, is_trans = false;
	rscript_t *rs = NULL;
	rs_stage_t *stage = NULL;
	int		va = 0;
	float mode;
	vec3_t lightcolor;
	float ramp = 1.0;

	frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames);
	verts = v = frame->verts;
	
	startorder = order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

	if (r_shaders->value)
			rs=(rscript_t *)currententity->script;

	VectorCopy(shadelight, lightcolor);
	for (i=0;i<model_dlights_num;i++)
		VectorAdd(lightcolor, model_dlights[i].color, lightcolor);
	VectorNormalize(lightcolor);

	if (currententity->flags & RF_TRANSLUCENT) {
		basealpha = alpha = currententity->alpha;

		rs=(rscript_t *)rs_glass;
		if(!rs)
			GL_Bind(r_reflecttexture->texnum);
	}
	else
		basealpha = alpha = 1.0;

	AngleVectors (currententity->angles, vectors[0], vectors[1], vectors[2]);

	// PMM - added double shell
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
		GL_Bind(r_shelltexture->texnum);  // add this line

	if(currententity->flags & RF_VIEWERMODEL)
		return;

	VectorSubtract(currententity->origin, lightspot, lightdir);
		VectorNormalize ( lightdir );

	qglEnableClientState( GL_COLOR_ARRAY );

	if(( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) ) )
	{
		qglColor4f( shadelight[0], shadelight[1], shadelight[2], alpha);
		R_InitMeshVarrays();
		while (1)
		{
			// get the vertex count and primitive type
			count = *order++;
			va=0;
			if (!count)
				break;		// done
			if (count < 0)
			{
				count = -count;
				mode=GL_TRIANGLE_FAN;
			}
			else
				mode=GL_TRIANGLE_STRIP;

			do
			{
				// texture coordinates come from the draw list
				index_xyz = order[2];

				VA_SetElem2(tex_array[va],(currentmodel->r_mesh_verts[index_xyz][1] + currentmodel->r_mesh_verts[index_xyz][0]) * (1.0f / 40.0f), currentmodel->r_mesh_verts[index_xyz][2] * (1.0f / 40.0f) - r_newrefdef.time * 0.5f);
				VA_SetElem3(vert_array[va],currentmodel->r_mesh_verts[index_xyz][0],currentmodel->r_mesh_verts[index_xyz][1],currentmodel->r_mesh_verts[index_xyz][2]);
				VA_SetElem4(col_array[va], shadelight[0], shadelight[1], shadelight[2], calcEntAlpha(alpha, currentmodel->r_mesh_verts[index_xyz]));
				va++;
				order += 3;
			} while (--count);

			if ( qglLockArraysEXT )
				qglLockArraysEXT( 0, va );
			qglDrawArrays(mode,0,va);
			if ( qglUnlockArraysEXT ) 
				qglUnlockArraysEXT();
		}
	}
	else if(!rs)
	{
		alpha = basealpha;
		R_InitMeshVarrays();
		while (1)
		{

			// get the vertex count and primitive type
			count = *order++;
			va=0;
			if (!count)
				break;		// done
			if (count < 0)
			{
				count = -count;
				mode=GL_TRIANGLE_FAN;
			}
			else
				mode=GL_TRIANGLE_STRIP;

			tmp_count=count;
			tmp_order=order;

			do
			{
				// texture coordinates come from the draw list
				index_xyz = order[2];
				l = shadedots[verts[index_xyz].lightnormalindex];

				if(gl_rtlights->value)
					GL_VlightAliasModel (shadelight, &verts[index_xyz], &verts[index_xyz], 0, lightcolor);

				VA_SetElem2(tex_array[va],((float *)order)[0], ((float *)order)[1]);
				VA_SetElem3(vert_array[va],currentmodel->r_mesh_verts[index_xyz][0],currentmodel->r_mesh_verts[index_xyz][1],currentmodel->r_mesh_verts[index_xyz][2]);
				if(gl_rtlights->value)
					VA_SetElem4(col_array[va],lightcolor[0], lightcolor[1], lightcolor[2], calcEntAlpha(alpha, currentmodel->r_mesh_verts[index_xyz]));
				else
					VA_SetElem4(col_array[va], shadelight[0], shadelight[1], shadelight[2], calcEntAlpha(alpha, currentmodel->r_mesh_verts[index_xyz]));
				va++;
				order += 3;
			} while (--count);
			if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) {
				if ( qglLockArraysEXT )
					qglLockArraysEXT( 0, va );
				qglDrawArrays(mode,0,va);
				if ( qglUnlockArraysEXT ) 
					qglUnlockArraysEXT();
			}
	
		}

	}
	else
	{

		if (rs->stage && rs->stage->has_alpha)
		{
			is_trans = true;
			depthmaskrscipt = true;
		}

		if (depthmaskrscipt)
			qglDepthMask(false);

		R_InitMeshVarrays();
		while (1)
		{
			count = *order++;
			if (!count)
				break;		// done
			// get the vertex count and primitive type
			if (count < 0)
			{
				count = -count;
				mode=GL_TRIANGLE_FAN;
			}
			else
			{
				mode=GL_TRIANGLE_STRIP;
			}

			stage=rs->stage;
			tmp_count=count;
			tmp_order=order;

			while (stage)
			{
				count=tmp_count;
				order=tmp_order;
				va=0;

				if (stage->normalmap && !gl_normalmaps->value) {
					if(stage->next) {
						stage = stage->next;
						continue;
					}
				}

				if (stage->colormap.enabled)
					qglDisable (GL_TEXTURE_2D);
				else if (stage->anim_count)
					GL_Bind(RS_Animate(stage));
				else
					GL_Bind (stage->texture->texnum);

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

				if (stage->alphamask)
				{
					GLSTATE_ENABLE_ALPHATEST
				}
				else
				{
					GLSTATE_DISABLE_ALPHATEST
				}

				if(stage->normalmap && gl_normalmaps->value) {

					ramp = 2.0; //for getting brightness of normal maps up a bit

					qglDepthMask (GL_FALSE);
			 		qglEnable (GL_BLEND);

					// set the correct blending mode for normal maps
					qglBlendFunc (GL_ZERO, GL_SRC_COLOR);

					// and the texenv
					qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
					qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_DOT3_RGB_EXT);

				}

				do
				{
					if (!(stage->normalmap && !gl_normalmaps->value)) { //disable for normal stage if normals are disabled

						float os = ((float *)order)[0];
						float ot = ((float *)order)[1];
						vec3_t normal;
						int k;

						index_xyz = order[2];

						for (k=0;k<3;k++)
						normal[k] = r_avertexnormals[verts[index_xyz].lightnormalindex][k];
						VectorNormalize ( normal );

						if (stage->envmap)
						{
							vec3_t envmapvec;

							VectorAdd(currententity->origin, currentmodel->r_mesh_verts[index_xyz], envmapvec);
							RS_SetEnvmap (envmapvec, &os, &ot);

							os -= DotProduct (normal , vectors[1] );
							ot += DotProduct (normal, vectors[2] );
						}

						RS_SetTexcoords2D(stage, &os, &ot);

						VA_SetElem2(tex_array[va], os, ot);
						VA_SetElem3(vert_array[va],currentmodel->r_mesh_verts[index_xyz][0],currentmodel->r_mesh_verts[index_xyz][1],currentmodel->r_mesh_verts[index_xyz][2]);

						{
							float red = 1, green = 1, blue = 1, nAlpha;

							nAlpha = RS_AlphaFuncAlias (stage->alphafunc,
							calcEntAlpha(alpha, currentmodel->r_mesh_verts[index_xyz]), normal, currentmodel->r_mesh_verts[index_xyz]);

							if (stage->lightmap)
							{
								if(gl_rtlights->value) {
									GL_VlightAliasModel (shadelight, &verts[index_xyz], &verts[index_xyz], 0, lightcolor);
									red = lightcolor[0] * ramp;
									green = lightcolor[1] * ramp;
									blue = lightcolor[2] * ramp;
								}
								else {
									red = shadelight[0] * ramp;
									green = shadelight[1] * ramp;
									blue = shadelight[2] * ramp;
								}
								//try to keep normalmapped stages from going completely dark
								if(stage->normalmap && gl_normalmaps->value) {
									if(red < .6) red = .6;
									if(green < .6) green = .6;
									if(blue < .6) blue = .6;
								}

							}

							if (stage->colormap.enabled)
							{
								red *= stage->colormap.red/255.0;
								green *= stage->colormap.green/255.0;
								blue *= stage->colormap.blue/255.0;
							}

							VA_SetElem4(col_array[va], red, green, blue, nAlpha);
						}
					}
					order += 3;
					va++;
				} while (--count);

				if (!(stage->normalmap && !gl_normalmaps->value)) //disable so that we
					//can still have shaders without normalmapped models if so chosen
				{
					if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) {
						if ( qglLockArraysEXT )
						qglLockArraysEXT( 0, va );
						qglDrawArrays(mode,0,va);
						if ( qglUnlockArraysEXT ) 
							qglUnlockArraysEXT();
					}
				}

				qglColor4f(1,1,1,1);
				if (stage->colormap.enabled)
					qglEnable (GL_TEXTURE_2D);

				if(stage->normalmap && gl_normalmaps->value) {

					ramp = 1.0;
					// back to replace mode
					qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

					// restore the original blend mode
					qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

					// switch off blending
					qglDisable (GL_BLEND);
					qglDepthMask (GL_TRUE);
				}

				stage=stage->next;
			}

		}

		if (depthmaskrscipt)
			qglDepthMask(true);
	}

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_TEXGEN

	qglDisableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM ) )
		qglEnable( GL_TEXTURE_2D );
}

extern qboolean have_stencil;
extern	vec3_t			lightspot;

/*
=============
R_DrawAliasShadow
=============
*/
void R_DrawAliasShadowLerped (dmdl_t *paliashdr, int posenum)
{
	dtrivertx_t	*verts;
	int		*order;
	vec3_t	point;
	float	height, lheight;
	int		count;
	daliasframe_t	*frame;

	lheight = currententity->origin[2] - lightspot[2];

	frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
		+ currententity->frame * paliashdr->framesize);

	verts = frame->verts;

	height = 0;

	order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

	height = -lheight + 0.1f;

	// if above entity's origin, skip
	if ((currententity->origin[2]+height) > currententity->origin[2])
		return;

	if (r_newrefdef.vieworg[2] < (currententity->origin[2] + height))
		return;

	if (have_stencil && gl_shadows->integer) {
		qglDepthMask(0);
		qglEnable(GL_STENCIL_TEST);

		qglStencilFunc(GL_EQUAL,1,2);

		qglStencilOp(GL_KEEP,GL_KEEP,GL_INCR);

	}

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			qglBegin (GL_TRIANGLE_FAN);
		}
		else
			qglBegin (GL_TRIANGLE_STRIP);

		do
		{

			memcpy( point, s_lerped[order[2]], sizeof( point )  );

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;
			qglVertex3fv (point);

			order += 3;

		} while (--count);

		qglEnd ();
	}
	qglDepthMask(1);
	qglColor4f(1,1,1,1);
	if (have_stencil && gl_shadows->integer) qglDisable(GL_STENCIL_TEST);
}
void R_DrawAliasShadow(dmdl_t *paliashdr)
{
	dtrivertx_t	*verts;
	int		*order;
	vec3_t	point;
	float	height, lheight;
	int		count;
	daliasframe_t	*frame;

	lheight = currententity->origin[2] - lightspot[2];

	frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames);
	verts = frame->verts;

	height = 0;

	order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

	height = -lheight + 0.1f;

	// if above entity's origin, skip
	if ((currententity->origin[2]+height) > currententity->origin[2])
		return;

	if (r_newrefdef.vieworg[2] < (currententity->origin[2] + height))
		return;

	if (have_stencil && gl_shadows->integer) {
		qglDepthMask(0);
		qglEnable(GL_STENCIL_TEST);

		qglStencilFunc(GL_EQUAL,1,2);

		qglStencilOp(GL_KEEP,GL_KEEP,GL_INCR);

	}

	while (1)
	{
		// get the vertex count and primitive type
		count = *order++;
		if (!count)
			break;		// done
		if (count < 0)
		{
			count = -count;
			qglBegin (GL_TRIANGLE_FAN);
		}
		else
			qglBegin (GL_TRIANGLE_STRIP);

		do
		{

			memcpy( point, currentmodel->r_mesh_verts[order[2]], sizeof( point )  );

			point[0] -= shadevector[0]*(point[2]+lheight);
			point[1] -= shadevector[1]*(point[2]+lheight);
			point[2] = height;
			qglVertex3fv (point);

			order += 3;

		} while (--count);

		qglEnd ();
	}
	qglDepthMask(1);
	qglColor4f(1,1,1,1);
	if (have_stencil && gl_shadows->integer) qglDisable(GL_STENCIL_TEST);
}

/*
** R_CullAliasModel
*/
static qboolean R_CullAliasModel( vec3_t bbox[8], entity_t *e )
{
	int i;
	vec3_t		mins, maxs;
	dmdl_t	*paliashdr;
	vec3_t		vectors[3];
	vec3_t		thismins, oldmins, thismaxs, oldmaxs;
	daliasframe_t *pframe, *poldframe;
	vec3_t angles;

	paliashdr = (dmdl_t *)currentmodel->extradata;

	if ( ( e->frame >= paliashdr->num_frames ) || ( e->frame < 0 ) )
	{
		Com_Printf ("R_CullAliasModel %s: no such frame %d\n",
			currentmodel->name, e->frame);
		e->frame = 0;
	}
	if ( ( e->oldframe >= paliashdr->num_frames ) || ( e->oldframe < 0 ) )
	{
		Com_Printf ("R_CullAliasModel %s: no such oldframe %d\n",
			currentmodel->name, e->oldframe);
		e->oldframe = 0;
	}

	pframe = ( daliasframe_t * ) ( ( byte * ) paliashdr +
		                              paliashdr->ofs_frames +
									  e->frame * paliashdr->framesize);

	poldframe = ( daliasframe_t * ) ( ( byte * ) paliashdr +
		                              paliashdr->ofs_frames +
									  e->oldframe * paliashdr->framesize);


	/*
	** compute axially aligned mins and maxs
	*/
	if ( pframe == poldframe )
	{
		for ( i = 0; i < 3; i++ )
		{
			mins[i] = pframe->translate[i];
			maxs[i] = mins[i] + pframe->scale[i]*255;
		}
	}
	else
	{
		for ( i = 0; i < 3; i++ )
		{
			thismins[i] = pframe->translate[i];
			thismaxs[i] = thismins[i] + pframe->scale[i]*255;

			oldmins[i]  = poldframe->translate[i];
			oldmaxs[i]  = oldmins[i] + poldframe->scale[i]*255;

			if ( thismins[i] < oldmins[i] )
				mins[i] = thismins[i];
			else
				mins[i] = oldmins[i];

			if ( thismaxs[i] > oldmaxs[i] )
				maxs[i] = thismaxs[i];
			else
				maxs[i] = oldmaxs[i];
		}
	}

	/*
	** compute a full bounding box
	*/
	for ( i = 0; i < 8; i++ )
	{
		vec3_t   tmp;

		if ( i & 1 )
			tmp[0] = mins[0];
		else
			tmp[0] = maxs[0];

		if ( i & 2 )
			tmp[1] = mins[1];
		else
			tmp[1] = maxs[1];

		if ( i & 4 )
			tmp[2] = mins[2];
		else
			tmp[2] = maxs[2];

		VectorCopy( tmp, bbox[i] );
	}

	/*
	** rotate the bounding box
	*/
	VectorCopy( e->angles, angles );
	angles[YAW] = -angles[YAW];
	AngleVectors( angles, vectors[0], vectors[1], vectors[2] );

	for ( i = 0; i < 8; i++ )
	{
		vec3_t tmp;

		VectorCopy( bbox[i], tmp );

		bbox[i][0] = DotProduct( vectors[0], tmp );
		bbox[i][1] = -DotProduct( vectors[1], tmp );
		bbox[i][2] = DotProduct( vectors[2], tmp );

		VectorAdd( e->origin, bbox[i], bbox[i] );
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

		if ( aggregatemask )
		{
			return true;
		}

		return false;
	}
}

/*
=================
R_DrawAliasModel

=================
*/
void R_DrawAliasModel (entity_t *e)
{
	int			i;
	dmdl_t		*paliashdr;
	vec3_t		bbox[8];
	image_t		*skin;
	rscript_t	*rs = NULL;
	char	shortname[MAX_QPATH];
	extern qboolean g_drawing_refl;

	if ( !( e->flags & RF_WEAPONMODEL ) )
	{
		if ( R_CullAliasModel( bbox, e ) )
			return;
	}
	else
	{
		if ( r_lefthand->value == 2 || g_drawing_refl)
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
			shadelight[2] = 1.0;  //make it more of a cyan color...
		}
		if ( currententity->flags & RF_SHELL_BLUE )
		{
			shadelight[2] = 1.0;
			shadelight[1] = 0.4;
			shadelight[0] = 0.6; //we want this to look like electricity
		}
	}
	else if (currententity->flags & RF_FULLBRIGHT)
	{
		for (i=0 ; i<3 ; i++)
			shadelight[i] = 1.0;
	}
	else if(gl_rtlights->value)
	{
		int max = 3;

		if (max<0)max=0;
		if (max>MAX_MODEL_DLIGHTS)max=MAX_MODEL_DLIGHTS;

		R_LightPointDynamics (currententity->origin, shadelight, model_dlights,
			&model_dlights_num, max);
	}
	else
	{
		R_LightPoint (currententity->origin, shadelight, true);
	}
	if ( currententity->flags & RF_MINLIGHT )
	{
		for (i=0 ; i<3 ; i++)
			if (shadelight[i] > 0.1)
				break;
		if (i == 3)
		{
			shadelight[0] = 0.1;
			shadelight[1] = 0.1;
			shadelight[2] = 0.1;
		}
	}

	if ( currententity->flags & RF_GLOW )
	{	// bonus items will pulse with time
		float	scale;
		float	min;

		scale = 0.2 * sin(r_newrefdef.time*7);
		for (i=0 ; i<3 ; i++)
		{
			min = shadelight[i] * 0.8;
			shadelight[i] += scale;
			if (shadelight[i] < min)
				shadelight[i] = min;
		}
	}

// =================
// PGM	ir goggles color override
	if ( r_newrefdef.rdflags & RDF_IRGOGGLES && currententity->flags & RF_IR_VISIBLE)
	{
		shadelight[0] = 1.0;
		shadelight[1] = 0.0;
		shadelight[2] = 0.0;
	}
// PGM
// =================

	shadedots = r_avertexnormal_dots[((int)(currententity->angles[1] * (SHADEDOT_QUANT / 360.0))) & (SHADEDOT_QUANT - 1)];

	//
	// locate the proper data
	//

	c_alias_polys += paliashdr->num_tris;

	//
	// draw all the triangles
	//
	if (currententity->flags & RF_DEPTHHACK) // hack the depth range to prevent view model from poking into walls
		qglDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));

	if ((currententity->flags & RF_WEAPONMODEL) && r_lefthand->value != 2.0F)
    {
		extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

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
	e->angles[PITCH] = -e->angles[PITCH];	// sigh.
	R_RotateForEntity (e);
	e->angles[PITCH] = -e->angles[PITCH];	// sigh.

	// select skin
	if (currententity->skin) {
		skin = currententity->skin;	// custom player skin

		//get a script for it, maybe there is a better place for this, but I don't think there is
		rs = NULL;
		if (r_shaders->value ){
			COM_StripExtension ( currententity->skin->name, shortname );

			rs = RS_FindScript(shortname);
			if (rs)
			{
				RS_ReadyScript(rs);
				currententity->script = rs;
			}
			else
				currententity->script = NULL;
		}

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

	if(e->frame == 0 && currentmodel->num_frames == 1)
		GL_DrawAliasFrame(paliashdr);
	else
		GL_DrawAliasFrameLerp (paliashdr, currententity->backlerp);

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

	if (gl_shadows->value && !(currententity->flags & (RF_WEAPONMODEL | RF_NOSHADOWS)))
	{
		float casted;
		float an = currententity->angles[1]/180*M_PI;
		shadevector[0] = cos(-an);
		shadevector[1] = sin(-an);
		shadevector[2] = 1;
		VectorNormalize (shadevector);

		switch ((int)(gl_shadows->value))
		{
		case 0:
			break;
		case 1: //dynamic only - always cast something
			casted = R_ShadowLight (currententity->origin, shadevector, 0);
			qglPushMatrix ();
			qglTranslatef	(e->origin[0], e->origin[1], e->origin[2]);
			qglRotatef (e->angles[1], 0, 0, 1);
			qglDisable (GL_TEXTURE_2D);
			qglEnable (GL_BLEND);

			if (currententity->flags & RF_TRANSLUCENT)
				qglColor4f (0,0,0,0.4 * currententity->alpha); //Knightmare- variable alpha
			else
				qglColor4f (0,0,0,0.4);
			
			if(e->frame == 0 && currentmodel->num_frames == 1)
				R_DrawAliasShadow (paliashdr);
			else
				R_DrawAliasShadowLerped (paliashdr, currententity->frame);

			qglEnable (GL_TEXTURE_2D);
			qglDisable (GL_BLEND);
			qglPopMatrix ();

			break;
		case 2: //dynamic and world
			//world
			casted = R_ShadowLight (currententity->origin, shadevector, 1);
			qglPushMatrix ();
			qglTranslatef	(e->origin[0], e->origin[1], e->origin[2]);
			qglRotatef (e->angles[1], 0, 0, 1);
			qglDisable (GL_TEXTURE_2D);
			qglEnable (GL_BLEND);

			if (currententity->flags & RF_TRANSLUCENT)
				qglColor4f (0,0,0,casted * currententity->alpha);
			else
				qglColor4f (0,0,0,casted);

			if(e->frame == 0 && currentmodel->num_frames == 1)
				R_DrawAliasShadow (paliashdr);
			else
				R_DrawAliasShadowLerped (paliashdr, currententity->frame);

			qglEnable (GL_TEXTURE_2D);
			qglDisable (GL_BLEND);
			qglPopMatrix ();
			//dynamic
			casted = 0;
		 	casted = R_ShadowLight (currententity->origin, shadevector, 0);
			if (casted > 0) { //only draw if there's a dynamic light there
				qglPushMatrix ();
				qglTranslatef	(e->origin[0], e->origin[1], e->origin[2]);
				qglRotatef (e->angles[1], 0, 0, 1);
				qglDisable (GL_TEXTURE_2D);
				qglEnable (GL_BLEND);

				if (currententity->flags & RF_TRANSLUCENT)
					qglColor4f (0,0,0,casted * currententity->alpha);
				else
					qglColor4f (0,0,0,casted);

				if(e->frame == 0 && currentmodel->num_frames == 1)
					R_DrawAliasShadow (paliashdr);
				else
					R_DrawAliasShadowLerped (paliashdr, currententity->frame);

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


