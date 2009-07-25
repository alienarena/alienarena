/*
Copyright (C) 2009 COR Entertainment

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

#include "r_local.h"

typedef float vec4_t[4];


/*
===============
SHADOW VOLUMES
===============
*/

vec3_t ShadowArray[MAX_SHADOW_VERTS];
static qboolean	triangleFacingLight	[MAX_INDICES / 3];

/*
==============
R_ShadowBlend
Draws projection shadow(s)
from stenciled volume
==============
*/

void R_ShadowBlend(float alpha)
{
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho(0, 1, 1, 0, -99999, 99999);

	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();
	qglLoadIdentity();

	qglColor4f (0,0,0, alpha);

	GLSTATE_DISABLE_ALPHATEST
	qglEnable( GL_BLEND );
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);

	qglEnable(GL_STENCIL_TEST);
	qglStencilFunc( GL_NOTEQUAL, 0, 0xFF);
	qglStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	qglBegin(GL_TRIANGLES);
	qglVertex2f(-5, -5);
	qglVertex2f(10, -5);
	qglVertex2f(-5, 10);
	qglEnd();

	qglMatrixMode(GL_PROJECTION);
	qglPopMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPopMatrix();

	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglDisable ( GL_BLEND );
	qglEnable (GL_TEXTURE_2D);
	qglEnable (GL_DEPTH_TEST);
	qglDisable(GL_STENCIL_TEST);

	qglColor4f(1,1,1,1);

}

void R_MarkShadowTriangles(dmdl_t *paliashdr, dtriangle_t *tris, vec3_t lightOrg, qboolean lerp){
	
	vec3_t	r_triangleNormals[MAX_INDICES / 3];
	vec3_t	temp, dir0, dir1;
	int		i;
	float	f;
	float	*v0, *v1, *v2;

		
	for (i = 0; i < paliashdr->num_tris; i++, tris++) {
		
		if(lerp) {

			v0 = (float*)currentmodel->s_lerped[tris->index_xyz[0]];
			v1 = (float*)currentmodel->s_lerped[tris->index_xyz[1]];
			v2 = (float*)currentmodel->s_lerped[tris->index_xyz[2]];
		}
		else {
			v0 = (float*)currentmodel->r_mesh_verts[tris->index_xyz[0]];
			v1 = (float*)currentmodel->r_mesh_verts[tris->index_xyz[1]];
			v2 = (float*)currentmodel->r_mesh_verts[tris->index_xyz[2]];
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
		 i < hdr->num_tris; i++, tris++, neighbors++) {
		if (!triangleFacingLight[i])
			continue;
		
		if (neighbors->n[0] < 0 || !triangleFacingLight[neighbors->n[0]]) {
			for (j = 0; j < 3; j++) {

				if(lerp) {
					v0[j] = currentmodel->s_lerped[tris->index_xyz[1]][j];
					v1[j] = currentmodel->s_lerped[tris->index_xyz[0]][j];
				}
				else {
					v0[j] = currentmodel->r_mesh_verts[tris->index_xyz[1]][j];
					v1[j] = currentmodel->r_mesh_verts[tris->index_xyz[0]][j];
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

		if (neighbors->n[1] < 0 || !triangleFacingLight[neighbors->n[1]]) {
			for (j = 0; j < 3; j++) {

				if(lerp) {

					v0[j] = currentmodel->s_lerped[tris->index_xyz[2]][j];
					v1[j] = currentmodel->s_lerped[tris->index_xyz[1]][j];
				}
				else {
					
					v0[j] = currentmodel->r_mesh_verts[tris->index_xyz[2]][j];
					v1[j] = currentmodel->r_mesh_verts[tris->index_xyz[1]][j];
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

		if (neighbors->n[2] < 0 || !triangleFacingLight[neighbors->n[2]]) {
			for (j = 0; j < 3; j++) {

				if(lerp) {

					v0[j] = currentmodel->s_lerped[tris->index_xyz[0]][j];
					v1[j] = currentmodel->s_lerped[tris->index_xyz[2]][j];
				}
				else {

					v0[j] = currentmodel->r_mesh_verts[tris->index_xyz[0]][j];
					v1[j] = currentmodel->r_mesh_verts[tris->index_xyz[2]][j];
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

	for (i = 0, tris = ot; i < hdr->num_tris; i++, tris++) {
		
		if (!triangleFacingLight[i])
			continue;
	
			for (j = 0; j < 3; j++) {

				if(lerp) {
					v0[j] = currentmodel->s_lerped[tris->index_xyz[0]][j];
					v1[j] = currentmodel->s_lerped[tris->index_xyz[1]][j];
					v2[j] = currentmodel->s_lerped[tris->index_xyz[2]][j];
				}
				else {

					v0[j] = currentmodel->r_mesh_verts[tris->index_xyz[0]][j];
					v1[j] = currentmodel->r_mesh_verts[tris->index_xyz[1]][j];
					v2[j] = currentmodel->r_mesh_verts[tris->index_xyz[2]][j];
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

			for (j = 0; j < 3; j++) {

				if(lerp) {

					v0[j] = currentmodel->s_lerped[tris->index_xyz[0]][j];
					v1[j] = currentmodel->s_lerped[tris->index_xyz[1]][j];
					v2[j] = currentmodel->s_lerped[tris->index_xyz[2]][j];
				}
				else {

					v0[j] = currentmodel->r_mesh_verts[tris->index_xyz[0]][j];
					v1[j] = currentmodel->r_mesh_verts[tris->index_xyz[1]][j];
					v2[j] = currentmodel->r_mesh_verts[tris->index_xyz[2]][j];
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

void GL_RenderVolumes(dmdl_t * paliashdr, vec3_t lightdir, int projdist, qboolean lerp)
{
	int incr = GL_INCR;
	int decr = GL_DECR;
	
	if(VectorCompare(lightdir, vec3_origin))
		return;
	
	qglEnable(GL_CULL_FACE);

	qglCullFace(GL_BACK);
	qglStencilOp(GL_KEEP, incr, GL_KEEP);
	BuildShadowVolume(paliashdr, lightdir, projdist, lerp);

	qglCullFace(GL_FRONT);
	qglStencilOp(GL_KEEP, decr, GL_KEEP);
	BuildShadowVolume(paliashdr, lightdir, projdist, lerp);
}

void GL_DrawAliasShadowVolume(dmdl_t * paliashdr, int posenumm, qboolean lerp)
{
	vec3_t light, light2, temp;
	int i, o;
	dlight_t *dl;
	worldLight_t *wl;
	float cost, sint;
	float is, it, dist;
	int worldlight = 0, dlight = 0;
	
	dl = r_newrefdef.dlights;

	if (r_newrefdef.vieworg[2] < (currententity->origin[2] - 10))
		return;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;
	
	VectorClear(light);
	
	cost =  cos(-currententity->angles[1] / 180 * M_PI), 
    sint =  sin(-currententity->angles[1] / 180 * M_PI);

	qglColorMask(0,0,0,0);
	qglEnable(GL_STENCIL_TEST);
	
	qglDepthMask(0);
	qglStencilFunc( GL_ALWAYS, 0x0, 0xFF);

	qglEnable(GL_POLYGON_OFFSET_FILL);
	qglPolygonOffset(0.0f, 100.0f);

	VectorClear(currententity->currentLightPos);

	for (i = 0; i < (r_newrefdef.num_dlights > 2 ? 2: r_newrefdef.num_dlights); i++, dl++) {


		if ((dl->origin[0] == currententity->origin[0]) &&
			(dl->origin[1] == currententity->origin[1]) &&
		    (dl->origin[2] == currententity->origin[2]))
		        continue;


		VectorSubtract(currententity->origin, dl->origin, temp);
		dist = VectorLength(temp);

		if (dist > dl->intensity*2.0)
			continue;		// big distance! - but check this so we dont' have sharp shadow loss

		for (o = 0; o < 3; o++)
			light[o] = -currententity->origin[o] + dl->origin[o];	

		is = light[0], it = light[1];
		light[0] = (cost * (is - 0) + sint * (0 - it) + 0);
		light[1] = (cost * (it - 0) + sint * (is - 0) + 0);
		light[2] += 8;
		VectorSubtract(currententity->origin, dl->origin, light2);
		VectorCopy(light2, currententity->currentLightPos);
		worldlight++;
		dlight++;
	}

	if(dlight)
		GL_RenderVolumes(paliashdr, light, 15, lerp);

	for (i=0; i<r_numWorldLights; i++) {

		wl = &r_worldLights[i];

		VectorSubtract(wl->origin, currententity->origin, temp);

		dist = VectorNormalize(temp);

		if (dist > 1500)
			continue;		// big distance!
						
		for (o = 0; o < 3; o++)
			light[o] = -currententity->origin[o] + wl->origin[o];	
		
		is = light[0], it = light[1];
		light[0] = (cost * (is - 0) + sint * (0 - it) + 0);
		light[1] = (cost * (it - 0) + sint * (is - 0) + 0);
		light[2] += currententity->model->maxs[2] + 56;
		VectorCopy(light, currententity->currentLightPos);

		worldlight++;
			
	}
	
	if (!worldlight) {

		VectorSet(light, 0, 0, 200);
	
		is = light[0], it = light[1];
		light[0] = (cost * (is - 0) + sint * (0 - it) + 0);
		light[1] = (cost * (it - 0) + sint * (is - 0) + 0);
		light[2] += 8;
		VectorCopy(light, currententity->currentLightPos);
	}
	
	GL_RenderVolumes(paliashdr, light, 15, lerp);

	qglDisable(GL_STENCIL_TEST);
	qglColorMask(1,1,1,1);
	
	qglDepthMask(1);
	qglDepthFunc(GL_LEQUAL);

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

void R_DrawShadowVolume(entity_t * e)
{
	dmdl_t *paliashdr;
	daliasframe_t *frame;
	vec3_t tmp;//, water;
	float rad;
	trace_t r_trace;
	
	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;
	
//	VectorAdd(e->origin, currententity->model->maxs, water); 
//	if(CL_PMpointcontents(water) & MASK_WATER)
//		return;
	
	VectorSubtract(currententity->model->maxs, currententity->model->mins, tmp);
	VectorScale (tmp, 1.666, tmp); 
	rad = VectorLength (tmp);
	
	if( R_CullSphere( e->origin, rad, 15 ) )
		return;
	
	if (r_worldmodel ) {
		//occulusion culling - why draw shadows of entities we cannot see?
	
		r_trace = CM_BoxTrace(r_origin, e->origin, currentmodel->maxs, currentmodel->mins, r_worldmodel->firstnode, MASK_OPAQUE);
		if(r_trace.fraction != 1.0)
			return;
	}

	paliashdr = (dmdl_t *) currentmodel->extradata;

	frame = (daliasframe_t *) ((byte *) paliashdr   + paliashdr->ofs_frames
						        + currententity->frame *
							  paliashdr->framesize);

	qglPushMatrix();
	qglDisable(GL_TEXTURE_2D);
	qglTranslatef(e->origin[0], e->origin[1], e->origin[2]);
	qglRotatef(e->angles[1], 0, 0, 1);

	if(e->frame == 0 && currentmodel->num_frames == 1) 
		GL_DrawAliasShadowVolume(paliashdr, currententity->frame, false);
	else
		GL_DrawAliasShadowVolume(paliashdr, currententity->frame, true);
		
	qglEnable(GL_TEXTURE_2D);
	qglPopMatrix();
}


/*=====================
World Shadows Triangles - this section is not used yet
=====================*/

vec3_t sVertexArray[1024];
static vec3_t modelorg;			// relative to viewpoint

void GL_DrawShadowTriangles(msurface_t * surf)
{
    glpoly_t *p;
    float *v;
    int i, cv;
       
    cv = surf->polys->numverts;
   	c_brush_polys++;		
   
	for (p = surf->polys; p; p = p->chain) {
            v = p->verts[0];
        
            for (i = 0; i < cv; i++, v += VERTEXSIZE)
                  VectorCopy(v, sVertexArray[i]);
        }
        
        if ( qglLockArraysEXT != 0 )
                            qglLockArraysEXT( 0, cv );    

        qglDrawArrays (GL_TRIANGLE_FAN, 0, cv);
        
        if ( qglUnlockArraysEXT != 0 )
                       qglUnlockArraysEXT();        
        		
        
}

void R_DrawBModelShadow(void)
{
	int i;
	cplane_t *pplane;
	float dot;
	msurface_t *psurf;
	
	psurf = &currentmodel->surfaces[currentmodel->firstmodelsurface];


	//
	// draw texture
	//
	for (i = 0; i < currentmodel->nummodelsurfaces; i++, psurf++) {
		// find which side of the node we are on
		pplane = psurf->plane;

		dot = DotProduct(modelorg, pplane->normal) - pplane->dist;
		
		// draw the polygon
		if (((psurf->flags & SURF_PLANEBACK) && (dot < -BACKFACE_EPSILON))
			|| (!(psurf->flags & SURF_PLANEBACK)
				&& (dot > BACKFACE_EPSILON))) {
			

				GL_DrawShadowTriangles(psurf);
				

				
	}
	}
	
}

void R_DrawBrushModelShadow(entity_t * e)
{
	vec3_t mins, maxs;
	int i;
	qboolean rotated;
	
	

	if (currentmodel->nummodelsurfaces == 0)
		return;

	currententity = e;
	
	if (e->angles[0] || e->angles[1] || e->angles[2]) {
		rotated = true;
		for (i = 0; i < 3; i++) {
			mins[i] = e->origin[i] - currentmodel->radius;
			maxs[i] = e->origin[i] + currentmodel->radius;
		}
	} else {
		rotated = false;
		VectorAdd(e->origin, currentmodel->mins, mins);
		VectorAdd(e->origin, currentmodel->maxs, maxs);
	}

	if (R_CullBox(mins, maxs))
		return;


	VectorSubtract(r_newrefdef.vieworg, e->origin, modelorg);

	if (rotated) {
		vec3_t temp;
		vec3_t forward, right, up;

		VectorCopy(modelorg, temp);
		AngleVectors(e->angles, forward, right, up);
		modelorg[0] = DotProduct(temp, forward);
		modelorg[1] = -DotProduct(temp, right);
		modelorg[2] = DotProduct(temp, up);
	}

	qglPushMatrix();
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug
	R_RotateForEntity(e);
	e->angles[0] = -e->angles[0];	// stupid quake bug
	e->angles[2] = -e->angles[2];	// stupid quake bug



	R_DrawBModelShadow();
	

	qglPopMatrix();
	
}

void R_RecursiveShadowWorldNode(mnode_t * node)
{
	int c, side, sidebit;
	cplane_t *plane;
	msurface_t *surf, **mark;
	mleaf_t *pleaf;
	float dot;
	

	if (node->contents == CONTENTS_SOLID)
		return;					// solid

	if (node->visframe != r_visframecount)
		return;

	if (R_CullBox(node->minmaxs, node->minmaxs + 3))
		return;

	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (! (r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		if (! (c = pleaf->nummarksurfaces) )
			return;

		do
		{
			(*mark++)->visframe = r_framecount;
		} while (--c);

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	plane = node->plane;

	switch (plane->type) {
	case PLANE_X:
		dot = modelorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = modelorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = modelorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct(modelorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0) {
		side = 0;
		sidebit = 0;
	} else {
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_RecursiveShadowWorldNode(node->children[side]);

	// draw stuff
	for (c = node->numsurfaces, surf =
		 r_worldmodel->surfaces + node->firstsurface; c; c--, surf++) {
		if (surf->visframe != r_framecount)
			continue;

		if ((surf->flags & SURF_PLANEBACK) != sidebit)
			continue;			// wrong side

			
			GL_DrawShadowTriangles(surf);
		
	}

	// recurse down the back side
	R_RecursiveShadowWorldNode(node->children[!side]);
}

void R_DrawShadowWorld(void)
{
	int i;

	if (!r_drawworld->value)
		return;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;
	
	if(!gl_shadows->value)
		return;
	
	qglEnableClientState(GL_VERTEX_ARRAY);
	qglVertexPointer(3, GL_FLOAT, 0, sVertexArray);
	
	qglEnable(GL_STENCIL_TEST);
    qglStencilFunc(GL_NOTEQUAL, 128, 255);
    qglStencilMask(0);
	qglDepthMask(0);
	
	qglDisable(GL_TEXTURE_2D);

    qglEnable(GL_BLEND);
    qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    qglColor4f(0, 0, 0, 0.25);    

	R_RecursiveShadowWorldNode(r_worldmodel->nodes);
	
	for (i = 0; i < r_newrefdef.num_entities; i++) {
		currententity = &r_newrefdef.entities[i];
		currentmodel = currententity->model;
		
		if (!currentmodel)
			continue;
		
		if (currentmodel->type != mod_brush)
			continue;

		R_DrawBrushModelShadow(currententity);
	}
	qglDisable(GL_STENCIL_TEST);
	qglDisableClientState(GL_VERTEX_ARRAY);
	qglDepthMask(1);
	qglColor4f(1, 1, 1, 1);
    qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    qglDisable(GL_BLEND);
    qglEnable(GL_TEXTURE_2D);
		
}

