/*
Copyright (C) 1997 - 2001 Id Software, Inc.

This program is free software; you can redistribute it and / or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 - 1307, USA.

*/ 

/*
==========================================================================================================================================

VERTEX ARRAYS SUBSYSTEM - Adopted from MHQ2 and modified for CRX

This was written to enable much the same rendering code to be used for almost everything.  A smallish enough vertex array buffer is
used to enable a balance of fast transfers of smaller chunks of data to the 3D card (with - hopefully - a decent level of amortization)
and a reduced number of API calls.  This system allows up to MAX_VERTS verts to be transferred at once.  In reality though, the transfer
has to take place at least every time the primitive type changes (that's vertex arrays for you).

==========================================================================================================================================
*/

#include "r_local.h"

// this must be equal to MAX_VERTS as it's possible for a single MDL to be composed of only one triangle fan or strip.
// the storage overhead is only in the order of 100K anyway, so it's no big deal.
// add 2 more verts for surface warping

float VArrayVerts[MAX_VARRAY_VERTS * MAX_VARRAY_VERTEX_SIZE];

// pointer for dynamic vert allocation
float *VArray = &VArrayVerts[0];

// arrays for normalmapping in glsl
static vec3_t NormalsArray[MAX_VERTICES];
static vec4_t TangentsArray[MAX_VERTICES];

// number of verts allocated
static int VertexCounter = 0;

float	tex_array[MAX_ARRAY][2];
float	vert_array[MAX_ARRAY][3];
float	col_array[MAX_ARRAY][4];
float	norm_array[MAX_ARRAY][3];

// sizes of our vertexes.  the vertex type can be used as an index into this array
int VertexSizes[] = {5, 5, 7, 7, 9, 11, 5, 3, 12, 5};

int KillFlags;

/*
=================
R_InitVArrays

Sets up the current vertex arrays we're going to use for rendering.
=================
*/
void R_InitVArrays (int varraytype)
{
	// it's assumed that the programmer has already called glDrawArrays for everything before calling this, so
	// here we will just re-init our pointer and counter
	VArray = &VArrayVerts[0];
	VertexCounter = 0;

	// init the kill flags so we'll know what to kill
	KillFlags = 0;

	// all vertex types bring up a vertex pointer
	// uses array indices 0, 1, 2
	qglEnableClientState (GL_VERTEX_ARRAY);
	qglVertexPointer (3, GL_FLOAT, sizeof (float) * VertexSizes[varraytype], &VArrayVerts[0]);

	// no texturing at all
	if (varraytype == VERT_NO_TEXTURE) return;

	// the simplest possible textured render uses a texcoord pointer for TMU 0
	if (varraytype == VERT_SINGLE_TEXTURED)
	{
		// uses array indices 3, 4
		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_SINGLE_TEXTURED], &VArrayVerts[3]);

		KillFlags |= KILL_TMU0_POINTER;

		return;
	}

	// 2 TMUs which share texcoords
	if (varraytype == VERT_DUAL_TEXTURED)
	{
		// uses array indices 3, 4
		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_DUAL_TEXTURED], &VArrayVerts[3]);

		// uses array indices 3, 4
		qglClientActiveTextureARB (GL_TEXTURE1);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_DUAL_TEXTURED], &VArrayVerts[3]);

		KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER);

		return;
	}

	// the bumpmapped setup gets to reuse the verts as it's texcoords for tmu 0
	if (varraytype == VERT_BUMPMAPPED)
	{
		// uses array indices 0, 1, 2
		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (3, GL_FLOAT, sizeof (float) * VertexSizes[VERT_BUMPMAPPED], &VArrayVerts[0]);

		// uses array indices 3, 4
		qglClientActiveTextureARB (GL_TEXTURE1);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_BUMPMAPPED], &VArrayVerts[3]);

		KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER);

		return;
	}

	if (varraytype == VERT_BUMPMAPPED_COLOURED)
	{
		// uses array indices 0, 1, 2
		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (3, GL_FLOAT, sizeof (float) * VertexSizes[VERT_BUMPMAPPED_COLOURED], &VArrayVerts[0]);

		// uses array indices 3, 4
		qglClientActiveTextureARB (GL_TEXTURE1);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_BUMPMAPPED_COLOURED], &VArrayVerts[3]);

		// uses array indices 5, 6, 7, 8
		qglEnableClientState (GL_COLOR_ARRAY);
		qglColorPointer (4, GL_FLOAT, sizeof (float) * VertexSizes[VERT_BUMPMAPPED_COLOURED], &VArrayVerts[5]);
		
		KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER | KILL_RGBA_POINTER);

		return;
	}

	// a standard 2 tmu multitexture needs 2 texcoord pointers.
	if (varraytype == VERT_MULTI_TEXTURED)
	{
		// uses array indices 3, 4
		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_MULTI_TEXTURED], &VArrayVerts[3]);

		// uses array indices 5, 6
		qglClientActiveTextureARB (GL_TEXTURE1);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_MULTI_TEXTURED], &VArrayVerts[5]);

		KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER);

		return;
	}

	// no texture is used here but we do specify a colour (used in Q2 for laser beams, etc)
	if (varraytype == VERT_COLOURED_UNTEXTURED)
	{
		// uses array indices 3, 4, 5, 6
		qglEnableClientState (GL_COLOR_ARRAY);
		qglColorPointer (4, GL_FLOAT, sizeof (float) * VertexSizes[VERT_COLOURED_UNTEXTURED], &VArrayVerts[3]);

		KillFlags |= KILL_RGBA_POINTER;

		return;
	}

	// single texture + colour (water, mdls, and so on)
	if (varraytype == VERT_COLOURED_TEXTURED)
	{
		// uses array indices 3, 4
		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_COLOURED_TEXTURED], &VArrayVerts[3]);

		// uses array indices 5, 6, 7, 8
		qglEnableClientState (GL_COLOR_ARRAY);
		qglColorPointer (4, GL_FLOAT, sizeof (float) * VertexSizes[VERT_COLOURED_TEXTURED], &VArrayVerts[5]);

		KillFlags |= (KILL_TMU0_POINTER | KILL_RGBA_POINTER);

		return;
	}

	// multi texture + colour
	if (varraytype == VERT_COLOURED_MULTI_TEXTURED)
	{
		// uses array indices 3, 4
		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_COLOURED_MULTI_TEXTURED], &VArrayVerts[3]);

		// uses array indices 5, 6
		qglClientActiveTextureARB (GL_TEXTURE1);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_COLOURED_MULTI_TEXTURED], &VArrayVerts[5]);

		// uses array indices 7, 8, 9, 10
		qglEnableClientState (GL_COLOR_ARRAY);
		qglColorPointer (4, GL_FLOAT, sizeof (float) * VertexSizes[VERT_COLOURED_MULTI_TEXTURED], &VArrayVerts[7]);

		KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER | KILL_RGBA_POINTER);

		return;
	}
	// textured with up to 3 TMU's, with normals
	if (varraytype == VERT_NORMAL_COLOURED_TEXTURED)
	{
		// uses array indices 3, 4
		qglClientActiveTextureARB (GL_TEXTURE0);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_NORMAL_COLOURED_TEXTURED], &VArrayVerts[3]);

		qglClientActiveTextureARB (GL_TEXTURE1);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_NORMAL_COLOURED_TEXTURED], &VArrayVerts[3]);

		qglClientActiveTextureARB (GL_TEXTURE2);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, sizeof (float) * VertexSizes[VERT_NORMAL_COLOURED_TEXTURED], &VArrayVerts[3]);

		// normal data
		qglEnableClientState( GL_NORMAL_ARRAY );
		
		KillFlags |= (KILL_TMU0_POINTER | KILL_TMU1_POINTER | KILL_TMU2_POINTER | KILL_NORMAL_POINTER);

		return;
	}
}


/*
=================
R_KillVArrays

Shuts down the specified vertex arrays.  Again, for maximum flexibility no programmer
hand-holding is done.
=================
*/
void R_KillVArrays (void)
{

	if(KillFlags & KILL_NORMAL_POINTER)
		qglDisableClientState (GL_NORMAL_ARRAY);

	if (KillFlags & KILL_RGBA_POINTER)
		qglDisableClientState (GL_COLOR_ARRAY);

	if (KillFlags & KILL_TMU3_POINTER)
	{
		qglClientActiveTextureARB (GL_TEXTURE3);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}

	if (KillFlags & KILL_TMU2_POINTER)
	{
		qglClientActiveTextureARB (GL_TEXTURE2);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}

	if (KillFlags & KILL_TMU1_POINTER)
	{
		qglClientActiveTextureARB (GL_TEXTURE1);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}

	if (KillFlags & KILL_TMU0_POINTER)
	{
		qglClientActiveTextureARB (GL_TEXTURE0);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}

	// always kill
	qglDisableClientState (GL_VERTEX_ARRAY);
}

void R_AddSurfToVArray (msurface_t *surf)
{
	glpoly_t *p = surf->polys;
	float	*v;	
	int i;

	for (; p; p = p->chain)
	{
		// reset pointer and counter
		VArray = &VArrayVerts[0];
		VertexCounter = 0;

		for (v = p->verts[0], i = 0 ; i < p->numverts; i++, v += VERTEXSIZE)
		{
			// copy in vertex data
			VArray[0] = v[0];
			VArray[1] = v[1];
			VArray[2] = v[2];

			// nothing else is needed
			// increment pointer and counter
			VArray += VertexSizes[VERT_NO_TEXTURE];
			VertexCounter++;
		}

		// draw the poly
		qglDrawArrays (GL_POLYGON, 0, VertexCounter);
	}
}

/*
=================
R_AddTexturedSurfToVArray

Adds a textured surf to the varray.  The surface can have 1 or more polys, and the VArray is flushed immediately
after each poly is rendered.

It's assumed that the programmer has set up the vertex arrays properly before calling this, otherwise weird things may happen!!!
=================
*/
void R_AddTexturedSurfToVArray (msurface_t *surf, float scroll)
{
	glpoly_t *p = surf->polys;
	float	*v;	
	int i;

	for (; p; p = p->chain)
	{
		// reset pointer and counter
		VArray = &VArrayVerts[0];
		VertexCounter = 0;

		for (v = p->verts[0], i = 0 ; i < p->numverts; i++, v += VERTEXSIZE)
		{
			// copy in vertex data
			VArray[0] = v[0];
			VArray[1] = v[1];
			VArray[2] = v[2];

			// world texture coords
			VArray[3] = v[3] + scroll;
			VArray[4] = v[4];

			// nothing else is needed
			// increment pointer and counter
			VArray += VertexSizes[VERT_SINGLE_TEXTURED];
			VertexCounter++;
		}

		// draw the poly
		qglDrawArrays (GL_POLYGON, 0, VertexCounter);
	}
}

/*
=================
R_AddLightMappedSurfToVArray

Adds a lightmapped surf to the varray.  The surface can have 1 or more polys, and the VArray is flushed immediately
after each poly is rendered.

It's assumed that the programmer has set up the vertex arrays properly before calling this, otherwise weird things may happen!!!
=================
*/
void R_AddLightMappedSurfToVArray (msurface_t *surf, float scroll)
{
	glpoly_t *p = surf->polys;
	float	*v;	
	int i;

	for (; p; p = p->chain)
	{
		// reset pointer and counter
		VArray = &VArrayVerts[0];
		VertexCounter = 0;

		for (v = p->verts[0], i = 0 ; i < p->numverts; i++, v += VERTEXSIZE)
		{
			// copy in vertex data
			VArray[0] = v[0];
			VArray[1] = v[1];
			VArray[2] = v[2];

			// world texture coords
			VArray[3] = v[3] + scroll;
			VArray[4] = v[4];

			// lightmap texture coords
			VArray[5] = v[5];
			VArray[6] = v[6];

			// nothing else is needed
			// increment pointer and counter
			VArray += VertexSizes[VERT_MULTI_TEXTURED];
			VertexCounter++;
		}

		// draw the poly
		qglDrawArrays (GL_POLYGON, 0, VertexCounter);

		/*
		 * Mapping tool. Outline the light-mapped polygons.
		 *  gl_showpolys == 1 : perform depth test.
		 *  gl_showpolys == 2 : disable depth test. everything in "visible set"
		 */
		if (gl_showpolys->value) // restricted cvar, maxclients = 1
		{
			qglDisable (GL_TEXTURE_2D);
			if (gl_showpolys->value > 1.9f)
			{ // lots of lines so make them narrower
				qglDisable(GL_DEPTH_TEST);
				qglLineWidth (2.0f);
			}
			else
			{ // less busy, wider line
				qglLineWidth (3.0f);
			}
			qglColor4f (1.0f, 1.0f, 1.0f, 1.0f);
			qglBegin (GL_LINE_LOOP);
			for (v = p->verts[0], i = 0; i < p->numverts; i++, v += VERTEXSIZE) {
				qglVertex3fv(p->verts[i]);
			}
			qglEnd();
			qglEnable (GL_DEPTH_TEST);
			qglEnable (GL_TEXTURE_2D);
		}

	}
}

void R_AddGLSLShadedSurfToVArray (msurface_t *surf, float scroll)
{
	glpoly_t *p = surf->polys;
	vec3_t	lightVec, lightAdd, temp;
	float	dist, weight;
	int		numlights;
	float	*v;	
	int		i, j;

	//send these to the shader program
	glUniformMatrix3fvARB( g_tangentSpaceTransform,	1, GL_FALSE, surf->tangentSpaceTransform );
	glUniform3fARB( g_location_eyePos, r_origin[0], r_origin[1], r_origin[2] );
	glUniform1iARB( g_location_fog, map_fog);

	//get light position relative to player's position
	numlights = 0;
	VectorClear(lightAdd);
	for (i = 0; i < r_lightgroups; i++) {	
		VectorSubtract(r_origin, LightGroups[i].group_origin, temp);
		dist = VectorLength(temp);
		if(dist == 0) 
			dist = 1;
		dist = dist*dist;
		weight = (int)250000/(dist/(LightGroups[i].avg_intensity+1.0f));
		for(j = 0; j < 3; j++) 
			lightAdd[j] += LightGroups[i].group_origin[j]*weight;
		numlights+=weight;				
	}

	if(numlights > 0.0) {
		for(i = 0; i < 3; i++) 
			lightVec[i] = (lightAdd[i]/numlights + r_origin[i])/2.0;
	}

	glUniform3fARB( g_location_staticLightPosition, lightVec[0], lightVec[1], lightVec[2]);

	for (; p; p = p->chain)
	{
		// reset pointer and counter
		VArray = &VArrayVerts[0];
		VertexCounter = 0;	

		for (v = p->verts[0], i = 0 ; i < p->numverts; i++, v += VERTEXSIZE)
		{
			// copy in vertex data
			VArray[0] = v[0];
			VArray[1] = v[1];
			VArray[2] = v[2];

			// world texture coords
			VArray[3] = v[3] + scroll;
			VArray[4] = v[4];

			// lightmap texture coords
			VArray[5] = v[5];
			VArray[6] = v[6];
	
			// nothing else is needed
			// increment pointer and counter
			VArray += VertexSizes[VERT_MULTI_TEXTURED];
			VertexCounter++;
		}	
		// draw the poly
		qglDrawArrays (GL_POLYGON, 0, VertexCounter);
	}
}

void R_AddGLSLShadedWarpSurfToVArray (msurface_t *surf, float scroll)
{
	glpoly_t *p = surf->polys;
	float	*v;	
	int		i;

	for (; p; p = p->chain)
	{
		vec3_t	tangent, lightPos;

		// reset pointer and counter
		VArray = &VArrayVerts[0];
		VertexCounter = 0;

		for (v = p->verts[0], i = 0 ; i < p->numverts; i++, v += VERTEXSIZE)
		{

			// copy in vertex data
			VArray[0] = v[0];
			VArray[1] = v[1];
			VArray[2] = v[2];

			// world texture coords
			VArray[3] = v[3] + scroll;
			VArray[4] = v[4];

			VectorCopy(surf->plane->normal, NormalsArray[VertexCounter]);

			// nothing else is needed
			// increment pointer and counter
			VArray += VertexSizes[VERT_NORMAL_COLOURED_TEXTURED];
			VertexCounter++;
		}

		qglNormalPointer(GL_FLOAT, 0, NormalsArray);

		AngleVectors(surf->plane->normal, NULL, tangent, NULL);

		//send these to the shader program
		glUniform3fARB( g_location_tangent, tangent[0], tangent[1], tangent[2]);

		for(i=0; i<3; i++) 
			lightPos[i] = r_origin[i];
		lightPos[2] += 512;

		glUniform3fARB( g_location_lightPos, lightPos[0], lightPos[1], lightPos[2]);
		glUniform1iARB( g_location_fogamount, map_fog);
		glUniform1fARB( g_location_time, rs_realtime);
	
		// draw the poly
		qglDrawArrays (GL_POLYGON, 0, VertexCounter);
	}
}


/*
====================
R_InitQuadVarrays

Used for 2 dimensional quads
====================
*/
void R_InitQuadVarrays(void) {

	qglEnableClientState (GL_VERTEX_ARRAY);
	qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

	qglTexCoordPointer (2, GL_FLOAT, sizeof(tex_array[0]), tex_array[0]); 
	qglVertexPointer (3, GL_FLOAT, sizeof(vert_array[0]), vert_array[0]);
	qglColorPointer (4, GL_FLOAT, sizeof(col_array[0]), col_array[0]);

	KillFlags |= KILL_TMU0_POINTER;
}
