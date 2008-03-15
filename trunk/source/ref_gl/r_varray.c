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
#define MAX_VARRAY_VERTS MAX_VERTS + 2
#define MAX_VARRAY_VERTEX_SIZE 11

float VArrayVerts[MAX_VARRAY_VERTS * MAX_VARRAY_VERTEX_SIZE];

// pointer for dynamic vert allocation
static float *VArray = &VArrayVerts[0];

// number of verts allocated
static int VertexCounter = 0;

// vertex array kill flags
#define KILL_TMU0_POINTER	1
#define KILL_TMU1_POINTER	2
#define KILL_RGBA_POINTER	4

// sizes of our vertexes.  the vertex type can be used as an index into this array
int VertexSizes[] = {5, 5, 7, 7, 9, 11, 5, 3};


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
	if (KillFlags & KILL_RGBA_POINTER)
		qglDisableClientState (GL_COLOR_ARRAY);

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
	}
}

/*
====================
R_InitMeshVarrays

This is used for operations that required more 
manipulation of texture coordinates than the above 
code provides.  Perhaps we should write more complex
functions that can use the other array subsystem.
====================
*/
void R_InitMeshVarrays(void) {

	qglEnableClientState (GL_VERTEX_ARRAY);
	qglEnableClientState (GL_TEXTURE_COORD_ARRAY);

	qglTexCoordPointer (2, GL_FLOAT, sizeof(tex_array[0]), tex_array[0]); 
	qglVertexPointer (3, GL_FLOAT, sizeof(vert_array[0]), vert_array[0]);
	qglColorPointer (4, GL_FLOAT, sizeof(col_array[0]), col_array[0]);

	KillFlags |= KILL_TMU0_POINTER;
}
