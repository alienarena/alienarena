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

static  vec4_t  s_lerped[MAX_VERTS];

static	vec3_t	s_normals[MAX_VERTS];

extern	vec3_t	lightspot;
vec3_t	shadevector;
float	shadelight[3];
vec3_t	lightdir;

#define MAX_MODEL_DLIGHTS 128
m_dlight_t model_dlights[MAX_MODEL_DLIGHTS];
int model_dlights_num;

extern  void GL_BlendFunction (GLenum sfactor, GLenum dfactor);
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
GL_VlightAliasModel

Vertex lighting for Alias models

When rtlights is set we get a smoother, more accurate model shading.  If normalmaps are on
we scale the light a bit less, as normalmaps tend to add a bit of contrast.

Contrast has been added by finding a threshold point, and scaling values on either side in 
opposite directions.  This gives the shading a more prounounced, defined look.

=============
*/
void GL_VlightAliasModel (vec3_t baselight, dtrivertx_t *verts, dtrivertx_t *ov, float backlerp, vec3_t lightOut)
{
    int i;
    float l;
    float lscale;

    lscale = 3.0;

	VectorSubtract(currententity->origin, lightspot, lightdir);
	VectorNormalize ( lightdir );

    if (gl_rtlights->value)
    {
        l = lscale * VLight_LerpLight (verts->lightnormalindex, ov->lightnormalindex,
                                backlerp, lightdir, currententity->angles, false);

        VectorScale(baselight, l, lightOut);

        if (model_dlights_num)
            for (i=0; i<model_dlights_num; i++)
            {
                l = lscale * VLight_LerpLight (verts->lightnormalindex, ov->lightnormalindex,
                    backlerp, model_dlights[i].direction, currententity->angles, true );
                VectorMA(lightOut, l, model_dlights[i].color, lightOut);
            }
    }
    else
    {
        l = shadedots[verts->lightnormalindex];
        VectorScale(baselight, l, lightOut);
    }

    for (i=0; i<3; i++)
    {        
		//add contrast - lights lighter, darks darker
		lightOut[i] += (lightOut[i] - 0.25);
		
		//keep values sane
        if (lightOut[i]<0) lightOut[i] = 0;
        if (lightOut[i]>1) lightOut[i] = 1;
    }
}

void GL_LerpSelfShadowVerts( int nverts, dtrivertx_t *v, dtrivertx_t *ov, dtrivertx_t *verts, float *lerp, float move[3], float frontv[3], float backv[3] )
{
    int i;
    for (i=0 ; i < nverts; i++, v++, ov++, lerp+=4)
        {
            lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0];
            lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1];
            lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2];
        }
}

/*
=============
GL_DrawAliasFrameLerp

interpolates between two frames and origins
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

//This routine bascially finds the average light position, by factoring in all lights and 
//accounting for their distance, visiblity, and intensity.

vec3_t	lightPosition;
float	dynFactor;
void GL_GetLightVals()
{
	int i, j, lnum;
	dlight_t	*dl;
	worldLight_t *wl;
	float dist, xdist, ydist;
	vec3_t	temp, lightAdd;
	trace_t r_trace;
	vec3_t mins, maxs;
	float numlights, weight;
	
	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	//light shining down if there are no lights at all
	VectorCopy(currententity->origin, lightPosition);
	lightPosition[2] += 128; 

	numlights = 0;
	VectorClear(lightAdd);
	for (i=0; i<r_numWorldLights; i++) {

		wl = &r_worldLights[i];

		VectorCopy(currententity->origin, temp);
		temp[2] += 24; //generates more consistent tracing

		if((currententity->flags & RF_WEAPONMODEL) && (wl->origin[2] > currententity->origin[2]))
			r_trace.fraction = 1.0; //don't do traces for weapon models, not smooth enough
		else
			r_trace = CM_BoxTrace(temp, wl->origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
		
		if(r_trace.fraction == 1.0) {
			VectorSubtract(currententity->origin, wl->origin, temp);
			dist = VectorLength(temp);
			if(dist == 0)
				dist = 1;
			dist = dist*dist;
			weight = (int)250000/dist;
			for(j = 0; j < 3; j++) 
				lightAdd[j] += wl->origin[j]*weight;
			numlights+=weight;				
		}
	}

	dynFactor = 0;
	if((!(currententity->flags & RF_NOSHADOWS)) || currententity->flags & RF_MINLIGHT) {
		dl = r_newrefdef.dlights;
		//limit to five lights(maybe less)?
		for (lnum=0; lnum<(r_newrefdef.num_dlights > 5 ? 5: r_newrefdef.num_dlights); lnum++, dl++) {
			
			VectorSubtract(currententity->origin, dl->origin, temp);
			dist = VectorLength(temp);

			VectorCopy(currententity->origin, temp);
			temp[2] += 24; //generates more consistent tracing
		
			r_trace = CM_BoxTrace(temp, dl->origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
			
			if(r_trace.fraction == 1.0) {
				if(dist < 100) {
					VectorCopy(dl->origin, temp);
					//translate for when viewangles are negative - done because otherwise the
					//lighting effect is backwards - stupid quake bug rearing it's head?
					if(r_newrefdef.viewangles[1] < 0) {
						//translate according viewangles
						xdist = temp[0] - currententity->origin[0];
						ydist = temp[1] - currententity->origin[1];
						temp[0] -= 2 * xdist;
						temp[1] -= 2 * ydist;
					}
					//make dynamic lights more influential than world
					for(j = 0; j < 3; j++)
						lightAdd[j] += temp[j]*100;
					numlights+=100;
                
					VectorSubtract (dl->origin, currententity->origin, temp);
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
** R_CullAliasModel
*/
static qboolean R_CullAliasModel( vec3_t bbox[8], entity_t *e )
{
	int i;
	vec3_t	vectors[3];
	vec3_t  angles;
	trace_t r_trace;
	vec3_t	dist;

	if (r_worldmodel ) {
		//occulusion culling - why draw entities we cannot see?
	
		r_trace = CM_BoxTrace(r_origin, e->origin, currentmodel->maxs, currentmodel->mins, r_worldmodel->firstnode, MASK_OPAQUE);
		if(r_trace.fraction != 1.0)
			return true;
	}

	VectorSubtract(r_origin, e->origin, dist);

	/*
	** rotate the bounding box
	*/
	VectorCopy( e->angles, angles );
	angles[YAW] = -angles[YAW];
	AngleVectors( angles, vectors[0], vectors[1], vectors[2] );

	for ( i = 0; i < 8; i++ )
	{
		vec3_t tmp;

		VectorCopy( currentmodel->bbox[i], tmp );

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

		if ( aggregatemask && (VectorLength(dist) > 150)) //so shadows don't blatantly disappear when out of frustom
		{
			return true;
		}

		return false;
	}
}

//legacy code - this is for ancient hardware that cannot handle GL_TRIANGLE useage
void GL_DrawAliasFrameLegacy (dmdl_t *paliashdr, float backlerp, qboolean lerped)
{
    daliasframe_t   *frame, *oldframe;
    dtrivertx_t *v, *ov, *verts;
    int     *order, *startorder, *tmp_order;
    int     count, tmp_count;
    float   frontlerp;
    float   alpha, basealpha;
    vec3_t  move, delta, vectors[3];
    vec3_t  frontv, backv;
    int     i;
    int     index_xyz;
    qboolean depthmaskrscipt = false;
    rscript_t *rs = NULL;
    rs_stage_t *stage = NULL;
    int     va = 0;
    float   mode;
    vec3_t lightcolor;
    float   *lerp;

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

    startorder = order = (int *)((byte *)paliashdr + paliashdr->ofs_glcmds);

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

    if(lerped) {
        frontlerp = 1.0 - backlerp;

        // move should be the delta back to the previous frame * backlerp
        VectorSubtract (currententity->oldorigin, currententity->origin, delta);
    }

    AngleVectors (currententity->angles, vectors[0], vectors[1], vectors[2]);

    if(lerped) {
        move[0] = DotProduct (delta, vectors[0]);   // forward
        move[1] = -DotProduct (delta, vectors[1]);  // left
        move[2] = DotProduct (delta, vectors[2]);   // up

        VectorAdd (move, oldframe->translate, move);

        for (i=0 ; i<3 ; i++)
        {
            move[i] = backlerp*move[i] + frontlerp*frame->translate[i];
            frontv[i] = frontlerp*frame->scale[i];
            backv[i] = backlerp*oldframe->scale[i];
        }

        if(currententity->flags & RF_VIEWERMODEL) { //lerp the vertices for self shadows, and leave
            lerp = s_lerped[0];
            GL_LerpSelfShadowVerts( paliashdr->num_xyz, v, ov, verts, lerp, move, frontv, backv);
            return;
        }
    }

    qglEnableClientState( GL_COLOR_ARRAY );

    if(( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) ) )
    {
        qglColor4f( shadelight[0], shadelight[1], shadelight[2], alpha);
        R_InitVArrays (VERT_COLOURED_TEXTURED);
        while (1)
        {
            float shellscale;
            // get the vertex count and primitive type
            count = *order++;
            va=0;
            VArray = &VArrayVerts[0];

            if (!count)
                break;      // done
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

                shellscale = .1;
                
                if(lerped) {
                    VArray[0] = s_lerped[index_xyz][0] = move[0] + ov[index_xyz].v[0]*backv[0] + v[index_xyz].v[0]*frontv[0] + r_avertexnormals[verts[index_xyz].lightnormalindex][0] * POWERSUIT_SCALE * shellscale;;
                    VArray[1] = s_lerped[index_xyz][1] = move[1] + ov[index_xyz].v[1]*backv[1] + v[index_xyz].v[1]*frontv[1] + r_avertexnormals[verts[index_xyz].lightnormalindex][1] * POWERSUIT_SCALE * shellscale;;
                    VArray[2] = s_lerped[index_xyz][2] = move[2] + ov[index_xyz].v[2]*backv[2] + v[index_xyz].v[2]*frontv[2] + r_avertexnormals[verts[index_xyz].lightnormalindex][2] * POWERSUIT_SCALE * shellscale;;

                    VArray[3] = (s_lerped[index_xyz][1] + s_lerped[index_xyz][0]) * (1.0f / 40.0f);
                    VArray[4] = s_lerped[index_xyz][2] * (1.0f / 40.0f) - r_newrefdef.time * 0.5f;

                    VArray[5] = shadelight[0];
                    VArray[6] = shadelight[1];
                    VArray[7] = shadelight[2];
                    VArray[8] = calcEntAlpha(alpha, s_lerped[index_xyz]);
                }
                else {
                    VArray[0] = currentmodel->r_mesh_verts[index_xyz][0];
                    VArray[1] = currentmodel->r_mesh_verts[index_xyz][1];
                    VArray[2] = currentmodel->r_mesh_verts[index_xyz][2];

                    VArray[3] = (currentmodel->r_mesh_verts[index_xyz][1] + currentmodel->r_mesh_verts[index_xyz][0]) * (1.0f / 40.0f);
                    VArray[4] = currentmodel->r_mesh_verts[index_xyz][2] * (1.0f / 40.0f) - r_newrefdef.time * 0.5f;

                    VArray[5] = shadelight[0];
                    VArray[6] = shadelight[1];
                    VArray[7] = shadelight[2];
                    VArray[8] = calcEntAlpha(alpha, currentmodel->r_mesh_verts[index_xyz]);
                }

                // increment pointer and counter
                VArray += VertexSizes[VERT_COLOURED_TEXTURED];
                va++;
                order += 3;
            } while (--count);
            if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ){

				if(qglLockArraysEXT)						
					qglLockArraysEXT(0, va);

				qglDrawArrays(mode,0,va);
				
				if(qglUnlockArraysEXT)						
					qglUnlockArraysEXT();
			}
        }
    }
    else if(!rs)
    {
        alpha = basealpha;
        R_InitVArrays (VERT_COLOURED_TEXTURED);
        GLSTATE_ENABLE_ALPHATEST
        while (1)
        {

            // get the vertex count and primitive type
            count = *order++;
            va=0;
            VArray = &VArrayVerts[0];

            if (!count)
                break;      // done
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

                if(lerped) {
                    GL_VlightAliasModel (shadelight, &verts[index_xyz], &ov[index_xyz], backlerp, lightcolor);

                    VArray[0] = s_lerped[index_xyz][0] = move[0] + ov[index_xyz].v[0]*backv[0] + v[index_xyz].v[0]*frontv[0];
                    VArray[1] = s_lerped[index_xyz][1] = move[1] + ov[index_xyz].v[1]*backv[1] + v[index_xyz].v[1]*frontv[1];
                    VArray[2] = s_lerped[index_xyz][2] = move[2] + ov[index_xyz].v[2]*backv[2] + v[index_xyz].v[2]*frontv[2];

                    VArray[3] = ((float *) order)[0];
                    VArray[4] = ((float *) order)[1];

                    VArray[5] = lightcolor[0];
                    VArray[6] = lightcolor[1];
                    VArray[7] = lightcolor[2];
                    VArray[8] = calcEntAlpha(alpha, s_lerped[index_xyz]);
                }
                else {
                    GL_VlightAliasModel (shadelight, &verts[index_xyz], &verts[index_xyz], 0, lightcolor);

                    VArray[0] = currentmodel->r_mesh_verts[index_xyz][0];
                    VArray[1] = currentmodel->r_mesh_verts[index_xyz][1];
                    VArray[2] = currentmodel->r_mesh_verts[index_xyz][2];

                    VArray[3] = ((float *) order)[0];
                    VArray[4] = ((float *) order)[1];

                    VArray[5] = lightcolor[0];
                    VArray[6] = lightcolor[1];
                    VArray[7] = lightcolor[2];
                    VArray[8] = calcEntAlpha(alpha, currentmodel->r_mesh_verts[index_xyz]);
                }

                // increment pointer and counter
                VArray += VertexSizes[VERT_COLOURED_TEXTURED];
                va++;
                order += 3;
            } while (--count);
            if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) {

				if(qglLockArraysEXT)						
					qglLockArraysEXT(0, va);

				qglDrawArrays(mode,0,va);
				
				if(qglUnlockArraysEXT)						
					qglUnlockArraysEXT();
			}
        }

    }
    else
    {

        if (rs->stage && rs->stage->has_alpha)
        {
            depthmaskrscipt = true;
        }

        if (depthmaskrscipt)
            qglDepthMask(false);

        while (1)
        {
            count = *order++;
            if (!count)
                break;      // done
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

			R_InitVArrays (VERT_COLOURED_TEXTURED);

            while (stage)
            {
                count=tmp_count;
                order=tmp_order;
                va=0;
                VArray = &VArrayVerts[0];

                if (stage->normalmap) { //not doing normalmaps for this routine
                    if(stage->next) {
                        stage = stage->next;
                        continue;
                    }
                }

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

                alpha=basealpha;

                if (stage->alphamask)
                {
                    GLSTATE_ENABLE_ALPHATEST
                }
                else
                {
                    GLSTATE_DISABLE_ALPHATEST
                }
                
                do
                {
                    float os = ((float *)order)[0];
                    float ot = ((float *)order)[1];
                    float os2 = ((float *)order)[0];
                    float ot2 = ((float *)order)[1];
                    vec3_t normal;
                    int k;

                    index_xyz = order[2];

                    if(lerped) {

                        VArray[0] = s_lerped[index_xyz][0] = move[0] + ov[index_xyz].v[0]*backv[0] + v[index_xyz].v[0]*frontv[0];
                        VArray[1] = s_lerped[index_xyz][1] = move[1] + ov[index_xyz].v[1]*backv[1] + v[index_xyz].v[1]*frontv[1];
                        VArray[2] = s_lerped[index_xyz][2] = move[2] + ov[index_xyz].v[2]*backv[2] + v[index_xyz].v[2]*frontv[2];

                        for (k=0; k<3; k++)
                        normal[k] = r_avertexnormals[verts[index_xyz].lightnormalindex][k] +
                        ( r_avertexnormals[ov[index_xyz].lightnormalindex][k] -
                        r_avertexnormals[verts[index_xyz].lightnormalindex][k] ) * backlerp;
                    }
                    else {
                        VArray[0] = currentmodel->r_mesh_verts[index_xyz][0];
                        VArray[1] = currentmodel->r_mesh_verts[index_xyz][1];
                        VArray[2] = currentmodel->r_mesh_verts[index_xyz][2];

                        for (k=0;k<3;k++)
                        normal[k] = r_avertexnormals[verts[index_xyz].lightnormalindex][k];
                    }

                    VectorNormalize ( normal );

                    if (stage->envmap)
                    {
                        vec3_t envmapvec;

                        VectorAdd(currententity->origin, s_lerped[index_xyz], envmapvec);

                        RS_SetEnvmap (envmapvec, &os, &ot);

                        if (currententity->flags & RF_TRANSLUCENT) //return to original glass script's scale(mostly for when going into menu)
                            stage->scale.scaleX = stage->scale.scaleY = 0.5;

                        os -= DotProduct (normal , vectors[1] );
                        ot += DotProduct (normal, vectors[2] );
                    }

                    RS_SetTexcoords2D(stage, &os, &ot);

                    VArray[3] = os;
                    VArray[4] = ot;

                    {
                        float red = 1, green = 1, blue = 1, nAlpha;
  
                        if(lerped)
                            nAlpha = RS_AlphaFuncAlias (stage->alphafunc,
                                calcEntAlpha(alpha, s_lerped[index_xyz]), normal, s_lerped[index_xyz]);
                        else
                            nAlpha = RS_AlphaFuncAlias (stage->alphafunc,
                                calcEntAlpha(alpha, currentmodel->r_mesh_verts[index_xyz]), normal, currentmodel->r_mesh_verts[index_xyz]);

                        if (stage->lightmap) {
                            if(lerped)
                                GL_VlightAliasModel (shadelight, &verts[index_xyz], &ov[index_xyz], backlerp, lightcolor);
                            else
                                GL_VlightAliasModel (shadelight, &verts[index_xyz], &verts[index_xyz], 0, lightcolor);
                            red = lightcolor[0];
                            green = lightcolor[1];
                            blue = lightcolor[2];
                        }
                     
						VArray[5] = red;
                        VArray[6] = green;
                        VArray[7] = blue;
                        VArray[8] = nAlpha;
                    }

                    // increment pointer and counter
                    VArray += VertexSizes[VERT_COLOURED_TEXTURED];
                    order += 3;
                    va++;
                } while (--count);

                if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) {

					if(qglLockArraysEXT)						
						qglLockArraysEXT(0, va);

					qglDrawArrays(mode,0,va);
					
					if(qglUnlockArraysEXT)						
						qglUnlockArraysEXT();
				}
                
                qglColor4f(1,1,1,1);

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

    R_KillVArrays ();

    if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM ) )
        qglEnable( GL_TEXTURE_2D );

}

extern qboolean have_stencil;
extern  vec3_t          lightspot;

/*
=============
R_DrawAliasShadow
=============
*/
void R_DrawAliasShadowLegacy(dmdl_t *paliashdr, qboolean lerped)
{
    dtrivertx_t *verts;
    int     *order;
    vec3_t  point;
    float   height, lheight;
    int     count;
    daliasframe_t   *frame;

    lheight = currententity->origin[2] - lightspot[2];

    if(lerped)
        frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
            + currententity->frame * paliashdr->framesize);
    else
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
            break;      // done
        if (count < 0)
        {
            count = -count;
            qglBegin (GL_TRIANGLE_FAN);
        }
        else
            qglBegin (GL_TRIANGLE_STRIP);

        do
        {

            if(lerped)
                memcpy( point, s_lerped[order[2]], sizeof( point )  );
            else
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


void GL_DrawAliasFrame (dmdl_t *paliashdr, float backlerp, qboolean lerped, int skinnum)
{
	daliasframe_t	*frame, *oldframe;
	dtrivertx_t	*v, *ov, *verts;
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
	qboolean mirror = false;
	qboolean use_vbo = true;

	if(r_legacy->value) {
		GL_DrawAliasFrameLegacy (paliashdr, backlerp, lerped);
			return;
	}

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
		else if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL)) {
			if(gl_mirror->value)
				mirror = true;
		}
	}
	else
		basealpha = alpha = 1.0;

	// PMM - added double shell
	if ( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) )
		GL_Bind(r_shelltexture->texnum);  // add this line

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
			lerp = s_lerped[0];
			GL_LerpSelfShadowVerts( paliashdr->num_xyz, v, ov, verts, lerp, move, frontv, backv);
			return;
		}
	}

	qglEnableClientState( GL_COLOR_ARRAY );

	if(( currententity->flags & ( RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE | RF_SHELL_HALF_DAM) ) )
	{
		qglColor4f( shadelight[0], shadelight[1], shadelight[2], alpha);
		R_InitVArrays (VERT_COLOURED_TEXTURED);

		// get the vertex count and primitive type
		va=0;
		VArray = &VArrayVerts[0];

		for (i=0; i<paliashdr->num_tris; i++)
		{
			for (j=0; j<3; j++)
			{					
				index_xyz = tris[i].index_xyz[j];
 
				
				shellscale = .1;
					
				if(lerped) {
					VArray[0] = s_lerped[index_xyz][0] = move[0] + ov[index_xyz].v[0]*backv[0] + v[index_xyz].v[0]*frontv[0] + r_avertexnormals[verts[index_xyz].lightnormalindex][0] * POWERSUIT_SCALE * shellscale;;
					VArray[1] = s_lerped[index_xyz][1] = move[1] + ov[index_xyz].v[1]*backv[1] + v[index_xyz].v[1]*frontv[1] + r_avertexnormals[verts[index_xyz].lightnormalindex][1] * POWERSUIT_SCALE * shellscale;;
					VArray[2] = s_lerped[index_xyz][2] = move[2] + ov[index_xyz].v[2]*backv[2] + v[index_xyz].v[2]*frontv[2] + r_avertexnormals[verts[index_xyz].lightnormalindex][2] * POWERSUIT_SCALE * shellscale;;

					VArray[3] = (s_lerped[index_xyz][1] + s_lerped[index_xyz][0]) * (1.0f / 40.0f);
					VArray[4] = s_lerped[index_xyz][2] * (1.0f / 40.0f) - r_newrefdef.time * 0.5f;

					VArray[5] = shadelight[0];
					VArray[6] = shadelight[1];
					VArray[7] = shadelight[2];
					VArray[8] = calcEntAlpha(alpha, s_lerped[index_xyz]);		
				}
				else {
					VArray[0] = currentmodel->r_mesh_verts[index_xyz][0];
					VArray[1] = currentmodel->r_mesh_verts[index_xyz][1];
					VArray[2] = currentmodel->r_mesh_verts[index_xyz][2];

					VArray[3] = (currentmodel->r_mesh_verts[index_xyz][1] + currentmodel->r_mesh_verts[index_xyz][0]) * (1.0f / 40.0f);
					VArray[4] = currentmodel->r_mesh_verts[index_xyz][2] * (1.0f / 40.0f) - r_newrefdef.time * 0.5f;

					VArray[5] = shadelight[0];
					VArray[6] = shadelight[1];
					VArray[7] = shadelight[2];
					VArray[8] = calcEntAlpha(alpha, currentmodel->r_mesh_verts[index_xyz]);		
				}

				// increment pointer and counter
				VArray += VertexSizes[VERT_COLOURED_TEXTURED];
				va++;
			} 				
		}
		if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) {

				if(qglLockArraysEXT)						
					qglLockArraysEXT(0, va);

				qglDrawArrays(GL_TRIANGLES,0,va);
				
				if(qglUnlockArraysEXT)						
					qglUnlockArraysEXT();
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
					GL_VlightAliasModel (shadelight, &verts[index_xyz], &ov[index_xyz], backlerp, lightcolor);

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
					GL_VlightAliasModel (shadelight, &verts[index_xyz], &verts[index_xyz], 0, lightcolor);

					VArray[0] = currentmodel->r_mesh_verts[index_xyz][0];
					VArray[1] = currentmodel->r_mesh_verts[index_xyz][1];
					VArray[2] = currentmodel->r_mesh_verts[index_xyz][2];

					VArray[3] = st[index_st].s;
					VArray[4] = st[index_st].t;

					VArray[5] = lightcolor[0];
					VArray[6] = lightcolor[1];
					VArray[7] = lightcolor[2];
					VArray[8] = calcEntAlpha(alpha, currentmodel->r_mesh_verts[index_xyz]);			
				}

				// increment pointer and counter
				VArray += VertexSizes[VERT_COLOURED_TEXTURED];
				va++;			
			} 	

		}
		if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) {

				if(qglLockArraysEXT)						
					qglLockArraysEXT(0, va);

				qglDrawArrays(GL_TRIANGLES,0,va);
				
				if(qglUnlockArraysEXT)						
					qglUnlockArraysEXT();
		}

	}
	else
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
			use_vbo = true;
			va=0;
			VArray = &VArrayVerts[0];
			GLSTATE_ENABLE_ALPHATEST

			if (stage->normalmap && (!gl_normalmaps->value || !gl_glsl_shaders->value || !gl_state.glsl_shaders)) {
				if(stage->next) {
					stage = stage->next;
					continue;
				}	
				else
					goto done;
			}

			if(!stage->normalmap) {
				use_vbo = false; //to do - moved here because non-bumpmapped meshes were not lighting properly
				if(mirror) {	
					if( !(currententity->flags & RF_WEAPONMODEL)) {
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
						GL_Bind(r_mirrortexture->texnum);
				}
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

				if (!stage->alphamask)
				{
					GLSTATE_DISABLE_ALPHATEST
				}
			}

			if(stage->normalmap) {

				vec3_t lightVec, lightVal;

				GL_GetLightVals();

				//send light level and color to shader, ramp up a bit
				VectorCopy(lightcolor, lightVal);
				for(i = 0; i < 3; i++) {
					if(lightVal[i] < shadelight[i]/2)
						lightVal[i] = shadelight[i]/2; //never go completely black
					lightVal[i] *= 5;
					lightVal[i] += dynFactor;
					if(r_newrefdef.rdflags & RDF_NOWORLDMODEL) {
						if(lightVal[i] > 1.5)
							lightVal[i] = 1.5;
					}
					else {
						if(lightVal[i] > 1.0+dynFactor)
							lightVal[i] = 1.0+dynFactor;
					}
				}

				if(r_newrefdef.rdflags & RDF_NOWORLDMODEL) { //fixed light source

					//fixed light source pointing down, slightly forward and to the left
					lightPosition[0] = -1.0; 
					lightPosition[1] = 4.0; 
					lightPosition[2] = 8.0; 
					R_ModelViewTransform(lightPosition, lightVec);
				}
				else { 
					//simple directional(relative light position)
					VectorSubtract(lightPosition, currententity->origin, lightVec);
					if(dynFactor == 0.0) //do for world lights only
					{
						VectorMA(lightPosition, 1.0, lightVec, lightPosition);
						R_ModelViewTransform(lightPosition, lightVec);
					}

					//brighten things slightly 
					for (i = 0; i < 3; i++ )
						lightVal[i] *= 1.25;
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

				glUniform1fARB( g_location_meshTime, rs_realtime);

				glUniform1iARB( g_location_meshFog, map_fog);
			}
				
			//some other todo's here - non-bumpmapped meshes are not lit correctly.
		
	/*		if (gl_state.vbo && use_vbo) 
			{
				currentmodel->vbo_xyz = R_VCFindCache(VBO_STORE_XYZ, currententity);
				if (currentmodel->vbo_xyz) {
					//Com_Printf("skipped\n");
					goto skipLoad;
				}
			}*/

			for (i=0; i<paliashdr->num_tris; i++)
			{
				for (j=0; j<3; j++)
				{	
					vec3_t normal, tangent;
					int k;
					
					index_xyz = tris[i].index_xyz[j];
					index_st = tris[i].index_st[j];

					os = os2 = st[index_st].s;
					ot = ot2 = st[index_st].t;					

					if(lerped) {

						VArray[0] = s_lerped[index_xyz][0] = move[0] + ov[index_xyz].v[0]*backv[0] + v[index_xyz].v[0]*frontv[0];
						VArray[1] = s_lerped[index_xyz][1] = move[1] + ov[index_xyz].v[1]*backv[1] + v[index_xyz].v[1]*frontv[1];
						VArray[2] = s_lerped[index_xyz][2] = move[2] + ov[index_xyz].v[2]*backv[2] + v[index_xyz].v[2]*frontv[2];

						for (k=0; k<3; k++)
						normal[k] = r_avertexnormals[verts[index_xyz].lightnormalindex][k] +
						( r_avertexnormals[ov[index_xyz].lightnormalindex][k] -
						r_avertexnormals[verts[index_xyz].lightnormalindex][k] ) * backlerp;
					}
					else {
						VArray[0] = currentmodel->r_mesh_verts[index_xyz][0];
						VArray[1] = currentmodel->r_mesh_verts[index_xyz][1];
						VArray[2] = currentmodel->r_mesh_verts[index_xyz][2];

						for (k=0;k<3;k++)
							normal[k] = r_avertexnormals[verts[index_xyz].lightnormalindex][k];
					}

					VectorNormalize ( normal );

					if(stage->normalmap) { //send tangent to shader
						AngleVectors(normal, NULL, tangent, NULL);
						VectorCopy(normal, NormalsArray[va]); //shader needs normal array
						glUniform3fARB( g_location_meshTangent, tangent[0], tangent[1], tangent[2] );
					}

					if (stage->envmap)
					{
						vec3_t envmapvec;

						use_vbo = false;
							
						VectorAdd(currententity->origin, s_lerped[index_xyz], envmapvec);

						if(mirror) {
							if( !(currententity->flags & RF_WEAPONMODEL)) {
									stage->scale.scaleX = -0.5; 
									stage->scale.scaleY = 0.5;
									os -= DotProduct (normal , vectors[1]);
									ot += DotProduct (normal, vectors[2]);
							}
							else {
								stage->scale.scaleX = -1.0;
								stage->scale.scaleY = 1.0;
							}
		
						}
						else {
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

					if(mirror && !(currententity->flags & RF_WEAPONMODEL)) {
						os2 -= DotProduct (normal , vectors[1] );
						ot2 += DotProduct (normal, vectors[2] );
						RS_SetTexcoords2D(stage, &os2, &ot2);

						VArray[5] = os2;
						VArray[6] = ot2;
					}
			
					if(!stage->normalmap) {

						float red = 1, green = 1, blue = 1, nAlpha;

						if(lerped) 
							nAlpha = RS_AlphaFuncAlias (stage->alphafunc,
								calcEntAlpha(alpha, s_lerped[index_xyz]), normal, s_lerped[index_xyz]);
						else
							nAlpha = RS_AlphaFuncAlias (stage->alphafunc,
								calcEntAlpha(alpha, currentmodel->r_mesh_verts[index_xyz]), normal, currentmodel->r_mesh_verts[index_xyz]);

						if (stage->lightmap) { 
							if(lerped)
								GL_VlightAliasModel (shadelight, &verts[index_xyz], &ov[index_xyz], backlerp, lightcolor);
							else
								GL_VlightAliasModel (shadelight, &verts[index_xyz], &verts[index_xyz], 0, lightcolor);
							red = lightcolor[0];
							green = lightcolor[1];
							blue = lightcolor[2];						
						}

						if(mirror && !(currententity->flags & RF_WEAPONMODEL) ) {
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

					if(gl_state.vbo && use_vbo) {

						vert_array[va][0] = VArray[0];
						vert_array[va][1] = VArray[1];
						vert_array[va][2] = VArray[2];

						//if(!stage->normalmap) {
						//	col_array[va][0] = VArray[5];
						//	col_array[va][1] = VArray[6];
						//	col_array[va][2] = VArray[7];
						//	col_array[va][3] = VArray[8];
						//}

						norm_array[va][0] = normal[0];
						norm_array[va][1] = normal[1];
						norm_array[va][2] = normal[2];
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

			if(gl_state.vbo && use_vbo) { 

				currentmodel->vbo_xyz = R_VCLoadData(VBO_DYNAMIC, va*sizeof(vec3_t), &vert_array, VBO_STORE_XYZ, currententity);
			//	if(!stage->normalmap)
			//		currententity->vbo_lightp = R_VCLoadData(VBO_DYNAMIC, va*sizeof(vec4_t), &col_array, VBO_STORE_ANY, currententity);
				currentmodel->vbo_normals = R_VCLoadData(VBO_DYNAMIC, va*sizeof(vec3_t), &norm_array, VBO_STORE_NORMAL, currententity);
			}				
//skipLoad:			
			
			if(gl_state.vbo && use_vbo) {

				qglEnableClientState( GL_VERTEX_ARRAY );
				GL_BindVBO(currentmodel->vbo_xyz);
				qglVertexPointer(3, GL_FLOAT, 0, 0);

				qglEnableClientState(GL_TEXTURE_COORD_ARRAY);
				GL_BindVBO(currentmodel->vbo_st);
				qglTexCoordPointer(2, GL_FLOAT, 0, 0);	
				
			//	if(!stage->normalmap) {
			//		qglEnableClientState( GL_COLOR_ARRAY );
			//		GL_BindVBO(currententity->vbo_lightp);
			//		qglColorPointer (4, GL_FLOAT, 0, 0);
			//	}

				qglEnableClientState( GL_NORMAL_ARRAY );
				GL_BindVBO(currentmodel->vbo_normals);
				qglNormalPointer(GL_FLOAT, 0, 0);
			}
			else {
				if(stage->normalmap) {
				
					R_InitVArrays (VERT_NORMAL_COLOURED_TEXTURED);
					qglNormalPointer(GL_FLOAT, 0, NormalsArray);
				}
				else {
						
					if(mirror && !(currententity->flags & RF_WEAPONMODEL)) 
						R_InitVArrays(VERT_COLOURED_MULTI_TEXTURED);
					else
						R_InitVArrays (VERT_COLOURED_TEXTURED);
				}
			}
			
			if (!(!cl_gun->value && ( currententity->flags & RF_WEAPONMODEL ) ) ) {

				if(qglLockArraysEXT)						
					qglLockArraysEXT(0, va);

				qglDrawArrays(GL_TRIANGLES,0,va);
				
				if(qglUnlockArraysEXT)						
					qglUnlockArraysEXT();
			}
			
			qglColor4f(1,1,1,1);
			
			if(mirror && !(currententity->flags & RF_WEAPONMODEL))
				GL_EnableMultitexture( false );

			if(stage->normalmap) {

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

	R_KillVArrays ();

	if (gl_state.vbo)
		GL_BindVBO(NULL);

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
void R_DrawAliasShadow(dmdl_t *paliashdr, qboolean lerped)
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

	if(r_legacy->value) {
		R_DrawAliasShadowLegacy( paliashdr, lerped);
		return;
	}

	lheight = currententity->origin[2] - lightspot[2];

	if(lerped)
		frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
			+ currententity->frame * paliashdr->framesize);
	else
		frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames);
	
	verts = frame->verts;

	tris = (dtriangle_t *) ((byte *)paliashdr + paliashdr->ofs_tris);

	st = currentmodel->st;

	height = 0;

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
				memcpy( point, currentmodel->r_mesh_verts[index_xyz], sizeof( point )  );

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

	qglDrawArrays(GL_TRIANGLES,0,va);
	
	qglDisableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	R_KillVArrays ();
	qglDepthMask(1);
	qglColor4f(1,1,1,1);
	if (have_stencil) qglDisable(GL_STENCIL_TEST);
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

	if((r_newrefdef.rdflags & RDF_NOWORLDMODEL ) && !(e->flags & RF_MENUMODEL))
		return;

	if(e->team) //don't draw flag models, handled by sprites
		return;
	
	if ( !( e->flags & RF_WEAPONMODEL ) )
	{
		if ( R_CullAliasModel( bbox, e ) )
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
			shadelight[2] = 0.6;  //make it more of a cyan color...
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
	else if(gl_rtlights->value && !gl_normalmaps->value) //no need when normalmaps are used, which should be for all meshes.
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

	if(e->frame == 0 && currentmodel->num_frames == 1) {
		if(!(currententity->flags & RF_VIEWERMODEL))
			GL_DrawAliasFrame(paliashdr, 0, false, skin->texnum);
	}
	else
		GL_DrawAliasFrame(paliashdr, currententity->backlerp, true, skin->texnum);

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

	//old legacy shadows
	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL) && gl_shadows->value && gl_shadows->value < 3 && !(currententity->flags & (RF_WEAPONMODEL | RF_NOSHADOWS)))
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
				R_DrawAliasShadow (paliashdr, false);
			else
				R_DrawAliasShadow (paliashdr, true);

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
				R_DrawAliasShadow (paliashdr, false);
			else
				R_DrawAliasShadow (paliashdr, true);

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
					R_DrawAliasShadow (paliashdr, false);
				else
					R_DrawAliasShadow (paliashdr, true);

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


