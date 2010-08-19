/*
Copyright (C) 2009 COR Entertainment, LLC

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

/*
===============
SHADOW VOLUMES
===============
*/

extern glStencilFuncSeparatePROC	qglStencilFuncSeparate;
extern glStencilOpSeparatePROC		qglStencilOpSeparate;
extern glStencilMaskSeparatePROC	qglStencilMaskSeparate;

extern void R_ShadowBlend(float alpha);

vec3_t ShadowArray[MAX_SHADOW_VERTS];
static qboolean	triangleFacingLight	[MAX_INDICES / 3];

static vec4_t shadow_lerped[MAX_VERTS];

void GL_LerpVerts(int nverts, dtrivertx_t *v, dtrivertx_t *ov, float *lerp, float move[3], float frontv[3], float backv[3])
{
    int i;

    for (i = 0; i < nverts; i++, v++, ov++, lerp += 4) {
        lerp[0] = move[0] + ov->v[0]*backv[0] + v->v[0]*frontv[0];
        lerp[1] = move[1] + ov->v[1]*backv[1] + v->v[1]*frontv[1];
        lerp[2] = move[2] + ov->v[2]*backv[2] + v->v[2]*frontv[2];
    }
}

void R_MarkShadowTriangles(dmdl_t *paliashdr, dtriangle_t *tris, vec3_t lightOrg, qboolean lerp)
{
	vec3_t	r_triangleNormals[MAX_INDICES / 3];
	vec3_t	temp, dir0, dir1;
	int		i;
	float	f;
	float	*v0, *v1, *v2;

	for (i = 0; i < paliashdr->num_tris; i++, tris++)
	{
		if(lerp)
		{
			v0 = (float*)shadow_lerped[tris->index_xyz[0]];
			v1 = (float*)shadow_lerped[tris->index_xyz[1]];
			v2 = (float*)shadow_lerped[tris->index_xyz[2]];
		}
		else
		{
			v0 = (float*)currentmodel->vertexes[tris->index_xyz[0]].position;
			v1 = (float*)currentmodel->vertexes[tris->index_xyz[1]].position;
			v2 = (float*)currentmodel->vertexes[tris->index_xyz[2]].position;
		}

		//Calculate shadow volume triangle normals
		VectorSubtract( v0, v1, dir0 );
		VectorSubtract( v2, v1, dir1 );

		CrossProduct( dir0, dir1, r_triangleNormals[i] );

		// Find front facing triangles
		VectorSubtract(lightOrg, v0, temp);
		f = DotProduct(temp, r_triangleNormals[i]);

		triangleFacingLight[i] = f > 0;
	}
}

void R_MarkIQMShadowTriangles(vec3_t lightOrg)
{
	vec3_t	r_triangleNormals[MAX_INDICES / 3];
	vec3_t	temp, dir0, dir1;
	int		i;
	float	f;
	float	*v0, *v1, *v2;

	for (i = 0; i < currentmodel->num_triangles; i++)
	{
		v0 = (float*)currentmodel->animatevertexes[currentmodel->tris[i].vertex[0]].position;
		v1 = (float*)currentmodel->animatevertexes[currentmodel->tris[i].vertex[1]].position;
		v2 = (float*)currentmodel->animatevertexes[currentmodel->tris[i].vertex[2]].position;

		//Calculate shadow volume triangle normals
		VectorSubtract( v0, v1, dir0 );
		VectorSubtract( v2, v1, dir1 );

		CrossProduct( dir0, dir1, r_triangleNormals[i] );

		// Find front facing triangles
		VectorSubtract(lightOrg, v0, temp);
		f = DotProduct(temp, r_triangleNormals[i]);

		triangleFacingLight[i] = f > 0;
	}
}

void BuildShadowVolume(dmdl_t * hdr, vec3_t light, float projectdistance, qboolean lerp)
{
	dtriangle_t *ot, *tris;
	neighbors_t *neighbors;
	int i, j, shadow_vert = 0, index = 0;
	unsigned	ShadowIndex[MAX_INDICES];
	vec3_t v0, v1, v2, v3;
	vec3_t currentShadowLight;

	daliasframe_t *frame;
	dtrivertx_t *verts;

	frame = (daliasframe_t *) ((byte *) hdr + hdr->ofs_frames
							   + currententity->frame * hdr->framesize);
	verts = frame->verts;

	ot = tris = (dtriangle_t *) ((unsigned char *) hdr + hdr->ofs_tris);

	R_MarkShadowTriangles(hdr, tris, light, lerp);

	VectorCopy(light, currentShadowLight);

	for (i = 0, tris = ot, neighbors = currentmodel->neighbors;
		 i < hdr->num_tris; i++, tris++, neighbors++)
	{
		if (!triangleFacingLight[i])
			continue;

		if (neighbors->n[0] < 0 || !triangleFacingLight[neighbors->n[0]])
		{
			for (j = 0; j < 3; j++)
			{
				if(lerp)
				{
					v0[j] = shadow_lerped[tris->index_xyz[1]][j];
					v1[j] = shadow_lerped[tris->index_xyz[0]][j];
				}
				else
				{
					v0[j] = currentmodel->vertexes[tris->index_xyz[1]].position[j];
					v1[j] = currentmodel->vertexes[tris->index_xyz[0]].position[j];
				}

				v2[j] = v1[j] + ((v1[j] - light[j]) * projectdistance);
				v3[j] = v0[j] + ((v0[j] - light[j]) * projectdistance);

			}

			VA_SetElem3(ShadowArray[shadow_vert+0], v0[0], v0[1], v0[2]);
			VA_SetElem3(ShadowArray[shadow_vert+1], v1[0], v1[1], v1[2]);
			VA_SetElem3(ShadowArray[shadow_vert+2], v2[0], v2[1], v2[2]);
			VA_SetElem3(ShadowArray[shadow_vert+3], v3[0], v3[1], v3[2]);


			ShadowIndex[index++] = shadow_vert+0;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+2;
			shadow_vert +=4;
		}

		if (neighbors->n[1] < 0 || !triangleFacingLight[neighbors->n[1]])
		{
			for (j = 0; j < 3; j++)
			{
				if(lerp)
				{
					v0[j] = shadow_lerped[tris->index_xyz[2]][j];
					v1[j] = shadow_lerped[tris->index_xyz[1]][j];
				}
				else
				{
					v0[j] = currentmodel->vertexes[tris->index_xyz[2]].position[j];
					v1[j] = currentmodel->vertexes[tris->index_xyz[1]].position[j];
				}

				v2[j] = v1[j] + ((v1[j] - light[j]) * projectdistance);
				v3[j] = v0[j] + ((v0[j] - light[j]) * projectdistance);
			}

			VA_SetElem3(ShadowArray[shadow_vert+0], v0[0], v0[1], v0[2]);
			VA_SetElem3(ShadowArray[shadow_vert+1], v1[0], v1[1], v1[2]);
			VA_SetElem3(ShadowArray[shadow_vert+2], v2[0], v2[1], v2[2]);
			VA_SetElem3(ShadowArray[shadow_vert+3], v3[0], v3[1], v3[2]);


			ShadowIndex[index++] = shadow_vert+0;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+2;
			shadow_vert +=4;
		}

		if (neighbors->n[2] < 0 || !triangleFacingLight[neighbors->n[2]])
		{
			for (j = 0; j < 3; j++)
			{
				if(lerp)
				{
					v0[j] = shadow_lerped[tris->index_xyz[0]][j];
					v1[j] = shadow_lerped[tris->index_xyz[2]][j];
				}
				else
				{
					v0[j] = currentmodel->vertexes[tris->index_xyz[0]].position[j];
					v1[j] = currentmodel->vertexes[tris->index_xyz[2]].position[j];
				}

				v2[j] = v1[j] + ((v1[j] - light[j]) * projectdistance);
				v3[j] = v0[j] + ((v0[j] - light[j]) * projectdistance);
			}


			VA_SetElem3(ShadowArray[shadow_vert+0], v0[0], v0[1], v0[2]);
			VA_SetElem3(ShadowArray[shadow_vert+1], v1[0], v1[1], v1[2]);
			VA_SetElem3(ShadowArray[shadow_vert+2], v2[0], v2[1], v2[2]);
			VA_SetElem3(ShadowArray[shadow_vert+3], v3[0], v3[1], v3[2]);


			ShadowIndex[index++] = shadow_vert+0;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+2;
			shadow_vert +=4;
		}
	}

	 // triangle is frontface and therefore casts shadow,
     // output front and back caps for shadow volume front cap

	for (i = 0, tris = ot; i < hdr->num_tris; i++, tris++)
	{
		if (!triangleFacingLight[i])
			continue;

		for (j = 0; j < 3; j++)
		{
			if(lerp)
			{
				v0[j] = shadow_lerped[tris->index_xyz[0]][j];
				v1[j] = shadow_lerped[tris->index_xyz[1]][j];
				v2[j] = shadow_lerped[tris->index_xyz[2]][j];
			}
			else
			{
				v0[j] = currentmodel->vertexes[tris->index_xyz[0]].position[j];
				v1[j] = currentmodel->vertexes[tris->index_xyz[1]].position[j];
				v2[j] = currentmodel->vertexes[tris->index_xyz[2]].position[j];
			}
		}
		VA_SetElem3(ShadowArray[shadow_vert+0], v0[0], v0[1], v0[2]);
		VA_SetElem3(ShadowArray[shadow_vert+1], v1[0], v1[1], v1[2]);
		VA_SetElem3(ShadowArray[shadow_vert+2], v2[0], v2[1], v2[2]);

		ShadowIndex[index++] = shadow_vert+0;
		ShadowIndex[index++] = shadow_vert+1;
		ShadowIndex[index++] = shadow_vert+2;
		shadow_vert +=3;

		// rear cap (with flipped winding order)
			for (j = 0; j < 3; j++)
		{
				if(lerp)
			{
				v0[j] = shadow_lerped[tris->index_xyz[0]][j];
				v1[j] = shadow_lerped[tris->index_xyz[1]][j];
				v2[j] = shadow_lerped[tris->index_xyz[2]][j];
			}
				else
			{
				v0[j] = currentmodel->vertexes[tris->index_xyz[0]].position[j];
				v1[j] = currentmodel->vertexes[tris->index_xyz[1]].position[j];
				v2[j] = currentmodel->vertexes[tris->index_xyz[2]].position[j];
			}

			v0[j] = v0[j] + ((v0[j] - light[j]) * projectdistance);
			v1[j] = v1[j] + ((v1[j] - light[j]) * projectdistance);
			v2[j] = v2[j] + ((v2[j] - light[j]) * projectdistance);
		}

		VA_SetElem3(ShadowArray[shadow_vert+0], v0[0], v0[1], v0[2]);
		VA_SetElem3(ShadowArray[shadow_vert+1], v1[0], v1[1], v1[2]);
		VA_SetElem3(ShadowArray[shadow_vert+2], v2[0], v2[1], v2[2]);

		ShadowIndex[index++] = shadow_vert+2;
		ShadowIndex[index++] = shadow_vert+1;
		ShadowIndex[index++] = shadow_vert+0;
		shadow_vert +=3;
	}

	if ( qglLockArraysEXT != 0 )
           qglLockArraysEXT( 0, shadow_vert );

	qglDrawElements(GL_TRIANGLES, index, GL_UNSIGNED_INT, ShadowIndex);

	if ( qglUnlockArraysEXT != 0 )
             qglUnlockArraysEXT();
}


void BuildIQMShadowVolume(vec3_t light, float projectdistance)
{
	int i, j, shadow_vert = 0, index = 0;
	unsigned	ShadowIndex[MAX_INDICES];
	vec3_t v0, v1, v2, v3;
	vec3_t currentShadowLight;

	R_MarkIQMShadowTriangles(light);

	VectorCopy(light, currentShadowLight);

	//note - do we really need this extra pointer stuff?  Just access it directly

	for (i = 0;	i < currentmodel->num_triangles; i++)
	{
		if (!triangleFacingLight[i])
			continue;

		if (currentmodel->neighbors[i].n[0] < 0 || !triangleFacingLight[currentmodel->neighbors[i].n[0]])
		{
			for (j = 0; j < 3; j++)
			{

				v0[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[1]].position[j];
				v1[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[0]].position[j];

				v2[j] = v1[j] + ((v1[j] - light[j]) * projectdistance);
				v3[j] = v0[j] + ((v0[j] - light[j]) * projectdistance);

			}

			VA_SetElem3(ShadowArray[shadow_vert+0], v0[0], v0[1], v0[2]);
			VA_SetElem3(ShadowArray[shadow_vert+1], v1[0], v1[1], v1[2]);
			VA_SetElem3(ShadowArray[shadow_vert+2], v2[0], v2[1], v2[2]);
			VA_SetElem3(ShadowArray[shadow_vert+3], v3[0], v3[1], v3[2]);


			ShadowIndex[index++] = shadow_vert+0;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+2;
			shadow_vert +=4;
		}

		if (currentmodel->neighbors[i].n[1] < 0 || !triangleFacingLight[currentmodel->neighbors[i].n[1]])
		{
			for (j = 0; j < 3; j++) {

				v0[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[2]].position[j];
				v1[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[1]].position[j];

				v2[j] = v1[j] + ((v1[j] - light[j]) * projectdistance);
				v3[j] = v0[j] + ((v0[j] - light[j]) * projectdistance);
			}

			VA_SetElem3(ShadowArray[shadow_vert+0], v0[0], v0[1], v0[2]);
			VA_SetElem3(ShadowArray[shadow_vert+1], v1[0], v1[1], v1[2]);
			VA_SetElem3(ShadowArray[shadow_vert+2], v2[0], v2[1], v2[2]);
			VA_SetElem3(ShadowArray[shadow_vert+3], v3[0], v3[1], v3[2]);


			ShadowIndex[index++] = shadow_vert+0;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+2;
			shadow_vert +=4;
		}

		if (currentmodel->neighbors[i].n[2] < 0 || !triangleFacingLight[currentmodel->neighbors[i].n[2]])
		{
			for (j = 0; j < 3; j++)
			{

				v0[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[0]].position[j];
				v1[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[2]].position[j];

				v2[j] = v1[j] + ((v1[j] - light[j]) * projectdistance);
				v3[j] = v0[j] + ((v0[j] - light[j]) * projectdistance);
			}

			VA_SetElem3(ShadowArray[shadow_vert+0], v0[0], v0[1], v0[2]);
			VA_SetElem3(ShadowArray[shadow_vert+1], v1[0], v1[1], v1[2]);
			VA_SetElem3(ShadowArray[shadow_vert+2], v2[0], v2[1], v2[2]);
			VA_SetElem3(ShadowArray[shadow_vert+3], v3[0], v3[1], v3[2]);

			ShadowIndex[index++] = shadow_vert+0;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+3;
			ShadowIndex[index++] = shadow_vert+1;
			ShadowIndex[index++] = shadow_vert+2;
			shadow_vert +=4;
		}
	}

	 // triangle is frontface and therefore casts shadow,
     // output front and back caps for shadow volume front cap

	for (i = 0; i < currentmodel->num_triangles; i++)
	{
		if (!triangleFacingLight[i])
			continue;

		for (j = 0; j < 3; j++)
		{
			v0[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[0]].position[j];
			v1[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[1]].position[j];
			v2[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[2]].position[j];
		}

		VA_SetElem3(ShadowArray[shadow_vert+0], v0[0], v0[1], v0[2]);
		VA_SetElem3(ShadowArray[shadow_vert+1], v1[0], v1[1], v1[2]);
		VA_SetElem3(ShadowArray[shadow_vert+2], v2[0], v2[1], v2[2]);

		ShadowIndex[index++] = shadow_vert+0;
		ShadowIndex[index++] = shadow_vert+1;
		ShadowIndex[index++] = shadow_vert+2;
		shadow_vert +=3;

		// rear cap (with flipped winding order)

		for (j = 0; j < 3; j++)
		{
			v0[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[0]].position[j];
			v1[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[1]].position[j];
			v2[j] = currentmodel->animatevertexes[currentmodel->tris[i].vertex[2]].position[j];

			v0[j] = v0[j] + ((v0[j] - light[j]) * projectdistance);
			v1[j] = v1[j] + ((v1[j] - light[j]) * projectdistance);
			v2[j] = v2[j] + ((v2[j] - light[j]) * projectdistance);

		}

		VA_SetElem3(ShadowArray[shadow_vert+0], v0[0], v0[1], v0[2]);
		VA_SetElem3(ShadowArray[shadow_vert+1], v1[0], v1[1], v1[2]);
		VA_SetElem3(ShadowArray[shadow_vert+2], v2[0], v2[1], v2[2]);

		ShadowIndex[index++] = shadow_vert+2;
		ShadowIndex[index++] = shadow_vert+1;
		ShadowIndex[index++] = shadow_vert+0;
		shadow_vert +=3;
	}

	if ( qglLockArraysEXT != 0 )
           qglLockArraysEXT( 0, shadow_vert );

	qglDrawElements(GL_TRIANGLES, index, GL_UNSIGNED_INT, ShadowIndex);

	if ( qglUnlockArraysEXT != 0 )
             qglUnlockArraysEXT();
}

void GL_RenderVolumes(dmdl_t * paliashdr, vec3_t lightdir, int projdist, qboolean lerp)
{
	int incr = gl_state.stencil_wrap ? GL_INCR_WRAP_EXT : GL_INCR;
	int decr = gl_state.stencil_wrap ? GL_DECR_WRAP_EXT : GL_DECR;

	if(VectorCompare(lightdir, vec3_origin))
		return;

	if(gl_state.separateStencil)
	{
		qglDisable(GL_CULL_FACE);

		qglStencilOpSeparate(GL_BACK, GL_KEEP,  incr, GL_KEEP);
		qglStencilOpSeparate(GL_FRONT, GL_KEEP, decr, GL_KEEP);

		BuildShadowVolume(paliashdr, lightdir, projdist, lerp);

		qglEnable(GL_CULL_FACE);
	}
	else
	{
		qglEnable(GL_CULL_FACE);

		qglCullFace(GL_BACK);
		qglStencilOp(GL_KEEP, incr, GL_KEEP);
		BuildShadowVolume(paliashdr, lightdir, projdist, lerp);

		qglCullFace(GL_FRONT);
		qglStencilOp(GL_KEEP, decr, GL_KEEP);
		BuildShadowVolume(paliashdr, lightdir, projdist, lerp);
	}
}

void GL_RenderIQMVolumes(vec3_t lightdir, int projdist)
{
	int incr = gl_state.stencil_wrap ? GL_INCR_WRAP_EXT : GL_INCR;
	int decr = gl_state.stencil_wrap ? GL_DECR_WRAP_EXT : GL_DECR;

	if(VectorCompare(lightdir, vec3_origin))
		return;

	if(gl_state.separateStencil)
	{
		qglDisable(GL_CULL_FACE);

		qglStencilOpSeparate(GL_BACK, GL_KEEP,  incr, GL_KEEP);
		qglStencilOpSeparate(GL_FRONT, GL_KEEP, decr, GL_KEEP);

		BuildIQMShadowVolume(lightdir, projdist);

		qglEnable(GL_CULL_FACE);
	}
	else
	{
		qglEnable(GL_CULL_FACE);

		qglCullFace(GL_BACK);
		qglStencilOp(GL_KEEP, incr, GL_KEEP);

		BuildIQMShadowVolume(lightdir, projdist);

		qglCullFace(GL_FRONT);
		qglStencilOp(GL_KEEP, decr, GL_KEEP);
		BuildIQMShadowVolume(lightdir, projdist);
	}
}

void GL_DrawAliasShadowVolume(dmdl_t * paliashdr, qboolean lerp)
{
	vec3_t light, temp, tempOrg;
	int i, j, o;
	float cost, sint;
	float is, it, dist;
	int worldlight = 0;
	float numlights, weight;
	float bob, project;
	trace_t	r_trace;
	vec3_t mins, maxs, lightAdd;

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	if(currententity->flags & RF_BOBBING)
		bob = currententity->bob;
	else
		bob = 0;

	VectorCopy(currententity->origin, tempOrg);
	tempOrg[2] -= bob;

	VectorClear(light);

	cost =  cos(-currententity->angles[1] / 180 * M_PI),
    sint =  sin(-currententity->angles[1] / 180 * M_PI);

	numlights = 0;
	VectorClear(lightAdd);
	for (i=0; i<r_lightgroups; i++)
	{
		if(LightGroups[i].group_origin[2] < currententity->origin[2] - bob)
			continue; //don't bother with world lights below the ent, creates undesirable shadows

		//need a trace(not for self model, too jerky when lights are blocked and reappear)
		if(!(currententity->flags & RF_VIEWERMODEL))
		{
			r_trace = CM_BoxTrace(tempOrg, LightGroups[i].group_origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
			if(r_trace.fraction != 1.0)
				continue;
		}

		VectorSubtract(LightGroups[i].group_origin, currententity->origin, temp);

		dist = VectorLength(temp);

		//accum and weight
		weight = (int)250000/(dist/(LightGroups[i].avg_intensity+1.0f));
		for(j = 0; j < 3; j++)
			lightAdd[j] += LightGroups[i].group_origin[j]*weight;
		numlights+=weight;

		if(numlights > 0.0)
		{
			for(o = 0; o < 3; o++)
				light[o] = -currententity->origin[o] + lightAdd[o]/numlights;

			is = light[0], it = light[1];
			light[0] = (cost * (is - 0) + sint * (0 - it) + 0);
			light[1] = (cost * (it - 0) + sint * (is - 0) + 0);
			light[2] += currententity->model->maxs[2] + 56;
		}

		worldlight++;
	}

	if(!worldlight)
	{ //no lights found, create light straight down

		VectorSet(light, 0, 0, 200);

		is = light[0], it = light[1];
		light[0] = (cost * (is - 0) + sint * (0 - it) + 0);
		light[1] = (cost * (it - 0) + sint * (is - 0) + 0);
		light[2] += 8;
	}

	if(currentmodel->maxs[2] - currentmodel->mins[2] > 200)
		project = 2.0;
	else
		project = 1.5;

	GL_RenderVolumes(paliashdr, light, project, lerp);
}

void GL_DrawIQMShadowVolume( void )
{
	vec3_t light, temp, tempOrg;
	int i, j, o;
	float cost, sint;
	float is, it, dist;
	int worldlight = 0;
	float numlights, weight;
	float bob, project;
	trace_t	r_trace;
	vec3_t mins, maxs, lightAdd;

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	if(currententity->flags & RF_BOBBING)
		bob = currententity->bob;
	else
		bob = 0;

	VectorCopy(currententity->origin, tempOrg);
	tempOrg[2] -= bob;

	VectorClear(light);

	cost =  cos(-currententity->angles[1] / 180 * M_PI),
    sint =  sin(-currententity->angles[1] / 180 * M_PI);

	numlights = 0;
	VectorClear(lightAdd);
	for (i=0; i<r_lightgroups; i++)
	{
		if(LightGroups[i].group_origin[2] < currententity->origin[2] - bob)
			continue; //don't bother with world lights below the ent, creates undesirable shadows

		//need a trace(not for self model, too jerky when lights are blocked and reappear)
		if(!(currententity->flags & RF_VIEWERMODEL))
		{
			r_trace = CM_BoxTrace(tempOrg, LightGroups[i].group_origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
			if(r_trace.fraction != 1.0)
				continue;
		}

		VectorSubtract(LightGroups[i].group_origin, currententity->origin, temp);

		dist = VectorLength(temp);

		//accum and weight
		weight = (int)250000/(dist/(LightGroups[i].avg_intensity+1.0f));
		for(j = 0; j < 3; j++)
			lightAdd[j] += LightGroups[i].group_origin[j]*weight;
		numlights+=weight;

		if(numlights > 0.0)
		{
			for(o = 0; o < 3; o++)
				light[o] = -currententity->origin[o] + lightAdd[o]/numlights;

			is = light[0], it = light[1];
			light[0] = (cost * (is - 0) + sint * (0 - it) + 0);
			light[1] = (cost * (it - 0) + sint * (is - 0) + 0);
			light[2] += currententity->model->maxs[2] + 56;
		}

		worldlight++;
	}

	if(!worldlight)
	{ //no lights found, create light straight down

		VectorSet(light, 0, 0, 200);

		is = light[0], it = light[1];
		light[0] = (cost * (is - 0) + sint * (0 - it) + 0);
		light[1] = (cost * (it - 0) + sint * (is - 0) + 0);
		light[2] += 8;
	}

	if(currentmodel->maxs[2] - currentmodel->mins[2] > 200)
		project = 2.0;
	else
		project = 1.5;

	GL_RenderIQMVolumes(light, project);
}

/*==============
Vis's CullSphere
==============*/

qboolean R_CullSphere( const vec3_t centre, const float radius, const int clipflags )
{
	int		i;
	cplane_t *p;

	if (r_nocull->value)
		return false;

	for (i=0,p=frustum ; i<4; i++,p++)
	{
		if ( !(clipflags & (1<<i)) ) {
			continue;
		}

		if ( DotProduct ( centre, p->normal ) - p->dist <= -radius )
			return true;
	}

	return false;
}

int CL_PMpointcontents(vec3_t point);
extern float modelpitch;
extern double degreeToRadian(double degree);
void R_DrawShadowVolume()
{
	dmdl_t *paliashdr;
    vec3_t move, delta, vectors[3];
    vec3_t frontv, backv;
    int i;
	qboolean lerped;

	if(currentmodel->type == mod_alias)
	{
		daliasframe_t *frame, *oldframe;
		dtrivertx_t *v, *ov, *verts;
		float   *lerp;
		float frontlerp;

		paliashdr = (dmdl_t *) currentmodel->extradata;

		if ( (currententity->frame >= paliashdr->num_frames)
			|| (currententity->frame < 0) )	{

			currententity->frame = 0;
			currententity->oldframe = 0;
		}

		if(currententity->frame == 0 && currentmodel->num_frames == 1)
			lerped = false;
		else
			lerped = true;

		if(lerped)
			frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
				+ currententity->frame * paliashdr->framesize);
		else
			frame = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames);
		verts = v = frame->verts;

		if(lerped) {

			if ( (currententity->oldframe >= paliashdr->num_frames)
				|| (currententity->oldframe < 0)) {

				currententity->frame = 0;
				currententity->oldframe = 0;
			}

			oldframe = (daliasframe_t *)((byte *)paliashdr + paliashdr->ofs_frames
				+ currententity->oldframe * paliashdr->framesize);
			ov = oldframe->verts;

			if ( !r_lerpmodels->value )
				currententity->backlerp = 0;

			frontlerp = 1.0 - currententity->backlerp;

			// move should be the delta back to the previous frame * backlerp
			VectorSubtract(currententity->oldorigin, currententity->origin, delta);
			AngleVectors(currententity->angles, vectors[0], vectors[1],
						 vectors[2]);

			move[0] = DotProduct(delta, vectors[0]);	// forward
			move[1] = -DotProduct(delta, vectors[1]);	// left
			move[2] = DotProduct(delta, vectors[2]);	// up

			if(oldframe && oldframe->translate)
				VectorAdd(move, oldframe->translate, move);

			for (i = 0; i < 3; i++) {
				move[i] =
					currententity->backlerp * move[i] +
					frontlerp * frame->translate[i];
				frontv[i] = frontlerp * frame->scale[i];
				backv[i] = currententity->backlerp * oldframe->scale[i];
			}

			lerp = shadow_lerped[0];

			GL_LerpVerts(paliashdr->num_xyz, v, ov, lerp, move, frontv, backv);
		}
	}

	qglPushMatrix();
	qglDisable(GL_TEXTURE_2D);
	qglTranslatef(currententity->origin[0], currententity->origin[1], currententity->origin[2]);
	qglRotatef(currententity->angles[1], 0, 0, 1);

	if(currentmodel->type == mod_alias)
		GL_DrawAliasShadowVolume(paliashdr, lerped);
	else
	{
		float time, frame;

		modelpitch = degreeToRadian(currententity->angles[PITCH]);

		//frame interpolation
		time = (Sys_Milliseconds() - currententity->frametime) / 100;
		if(time > 1.0)
			time = 1.0;

		if((currententity->frame == currententity->oldframe ) && !inAnimGroup(currententity->frame, currententity->oldframe))
			time = 0;

		//Check for stopped death anims
		if(currententity->frame == 257 || currententity->frame == 237 || currententity->frame == 219)
			time = 0;

		frame = currententity->frame + time;

		GL_AnimateIQMFrame(frame, NextFrame(currententity->frame));

		GL_DrawIQMShadowVolume();
	}

	qglEnable(GL_TEXTURE_2D);
	qglPopMatrix();
}


void R_CastShadow(void)
{
	int i;
	vec3_t dist, tmp;
	float rad;
    trace_t r_trace;

	//note - we use a combination of stencil volumes(for world light shadows) and shadowmaps(for dynamic shadows)
	if (!gl_shadowmaps->value)
		return;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, sizeof(vec3_t), ShadowArray);

	qglClear(GL_STENCIL_BUFFER_BIT);

	qglColorMask(0,0,0,0);
	qglEnable(GL_STENCIL_TEST);

	qglDepthFunc (GL_LESS);
	qglDepthMask(0);

	qglEnable(GL_POLYGON_OFFSET_FILL);
	qglPolygonOffset(0.0f, 1000.0f);

	if(gl_state.separateStencil)
		qglStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 0x0, 0xFF);
	else
		qglStencilFunc( GL_ALWAYS, 0x0, 0xFF);

	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		currententity = &r_newrefdef.entities[i];

		if (currententity->
		flags & (RF_SHELL_HALF_DAM | RF_SHELL_GREEN | RF_SHELL_RED |
				 RF_SHELL_BLUE | RF_SHELL_DOUBLE |
				 RF_WEAPONMODEL | RF_NOSHADOWS | RF_TRANSLUCENT))
				 continue;

		currentmodel = currententity->model;

		if (!currentmodel)
			continue;

		if (currentmodel->type != mod_alias && currentmodel->type != mod_iqm)
			continue;

		if (r_newrefdef.vieworg[2] < (currententity->origin[2] - 10))
			continue;

		VectorSubtract(currententity->model->maxs, currententity->model->mins, tmp);
		VectorScale (tmp, 1.666, tmp);
		rad = VectorLength (tmp);

		if( R_CullSphere( currententity->origin, rad, 15 ) )
			continue;

		if (r_worldmodel ) {
			//occulusion culling - why draw shadows of entities we cannot see?
			r_trace = CM_BoxTrace(r_origin, currententity->origin, currentmodel->maxs, currentmodel->mins, r_worldmodel->firstnode, MASK_OPAQUE);
			if(r_trace.fraction != 1.0)
				continue;
		}

		//get distance, set lod if available
		VectorSubtract(r_origin, currententity->origin, dist);

		//cull by distance if soft shadows(to do - test/tweak this)
		if(VectorLength(dist) > 1024 && gl_state.hasFBOblit && atoi(&gl_config.version_string[0]) >= 3.0)
			continue;

		if(VectorLength(dist) > 1000) {
			if(currententity->lod2)
				currentmodel = currententity->lod2;
		}
		else if(VectorLength(dist) > 500) {
			if(currententity->lod1)
				currentmodel = currententity->lod1;
		}

		R_DrawShadowVolume();
	}

	qglDisableClientState(GL_VERTEX_ARRAY);
	qglColorMask(1,1,1,1);

	qglDepthMask(1);
	qglDepthFunc(GL_LEQUAL);
	qglDisable(GL_POLYGON_OFFSET_FILL);

	R_ShadowBlend(0.3);

}


/*
===============
R_ShadowLight - Planar stencil shadows
===============
*/
void vectoangles (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
	// PMM - fixed to correct for pitch of 0
		if (value1[0])
			yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = 270;

		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

float R_ShadowLight (vec3_t entPos, vec3_t lightAdd, int type)
{
	int			lnum, i;
	dlight_t	*dl;
	vec3_t		pos, dist, angle, mins, maxs;
	trace_t		r_trace;
	float		add, shadowdist, bob;
	float   	intens = 0;

	if (!r_worldmodel)
		return 0;
	if (!r_worldmodel->lightdata) //keep old lame shadow
		return 0;

	VectorCopy(entPos, pos);

	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	VectorClear(lightAdd);
	//
	// add dynamic light shadow angles
	//
	if(!type) {
		dl = r_newrefdef.dlights;
		for (lnum=0; lnum<r_newrefdef.num_dlights; lnum++, dl++)
		{

			VectorSubtract (dl->origin, pos, dist);
			add = sqrt(dl->intensity - VectorLength(dist));
			VectorNormalize(dist);
			if (add > 0)
			{
				VectorScale(dist, add, dist);
				VectorAdd (lightAdd, dist, lightAdd);
				intens = 0.3;
			}
		}
	}
	//
	// add world light shadow angles
	//
	if(currententity->flags & RF_BOBBING)
		bob = currententity->bob;
	else
		bob = 0;

	pos[2] -= bob;

	if(gl_shadows->integer == 2 && type) {

		for (i=0; i<r_lightgroups; i++) {

			if(LightGroups[i].group_origin[2] < currententity->origin[2] - bob)
				continue; //don't bother with world lights below the ent, creates undesirable shadows

			//need a trace(not for self model, too jerky when lights are blocked and reappear)
			if(!(currententity->flags & RF_VIEWERMODEL)) {
				r_trace = CM_BoxTrace(pos, LightGroups[i].group_origin, mins, maxs, r_worldmodel->firstnode, MASK_OPAQUE);
				if(r_trace.fraction != 1.0)
					continue;
			}

			VectorSubtract (LightGroups[i].group_origin, pos, dist);
			add = sqrt(LightGroups[i].avg_intensity*50.0f - VectorLength(dist));
			VectorNormalize(dist);
			if (add > 0)
			{
				VectorScale(dist, sqrt(add), dist);
				VectorAdd (lightAdd, dist, lightAdd);
				intens+=(LightGroups[i].avg_intensity - VectorLength(dist)*3.5f); //darken shadows where light is stronger
			}
		}

		//cap some limits of lightness, darkness, subtley.
		if (intens < 0.1)
			intens = 0.1;
		if (intens > 0.3)
			intens = 0.3;
	}

	// Barnes improved code
	shadowdist = VectorNormalize(lightAdd)/5.0;
	if (shadowdist > 4) shadowdist = 4;
	if (shadowdist <= 0) // old style static shadow
	{
		angle[PITCH] = currententity->angles[PITCH];
		angle[YAW] =   -currententity->angles[YAW];
		angle[ROLL] =   currententity->angles[ROLL];
		shadowdist = 1;
	}
	else // shadow from dynamic lights
	{
		vectoangles (lightAdd, angle);
		angle[YAW] -= currententity->angles[YAW];
	}
	AngleVectors (angle, dist, NULL, NULL);
	VectorScale (dist, shadowdist, lightAdd);
	// end Barnes improved code

	return intens;
}
