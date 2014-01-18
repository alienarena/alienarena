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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_ragdoll.h"

// this must be equal to MAX_VERTS as it's possible for a single MDL to be composed of only one triangle fan or strip.
// the storage overhead is only in the order of 100K anyway, so it's no big deal.
// add 2 more verts for surface warping

float VArrayVerts[MAX_VARRAY_VERTS * MAX_VARRAY_VERTEX_SIZE];

// pointer for dynamic vert allocation
float *VArray = &VArrayVerts[0];

// arrays for normalmapping in glsl
static vec3_t NormalsArray[MAX_VERTICES];
// static vec4_t TangentsArray[MAX_VERTICES]; // unused

// number of verts allocated
static int VertexCounter = 0;

float	tex_array[MAX_ARRAY][2];
float	st_array[MAX_ARRAY][2];
float	vert_array[MAX_ARRAY][3];
float	norm_array[MAX_ARRAY][3];
float	tan_array[MAX_ARRAY][4];
float	col_array[MAX_ARRAY][4];

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
	BSP_InvalidateVBO ();
	
	if(KillFlags & KILL_NORMAL_POINTER)
		qglDisableClientState (GL_NORMAL_ARRAY);

	if (KillFlags & KILL_RGBA_POINTER)
		qglDisableClientState (GL_COLOR_ARRAY);

	if (KillFlags & KILL_TMU5_POINTER)
	{
		qglClientActiveTextureARB (GL_TEXTURE5);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}

	if (KillFlags & KILL_TMU4_POINTER)
	{
		qglClientActiveTextureARB (GL_TEXTURE4);
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
	}

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

void R_DrawVarrays(GLenum mode, GLint first, GLsizei count)
{
	if(count < 1)
		return; //do not send arrays of zero size to GPU!

	GL_BindVBO(NULL); //make sure that we aren't using an invalid buffer

	qglDrawArrays (mode, first, count);
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
