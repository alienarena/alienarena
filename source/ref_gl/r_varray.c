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

// number of verts allocated
static int VertexCounter = 0;

float	tex_array[MAX_ARRAY][2];
float	vert_array[MAX_ARRAY][3];
float	col_array[MAX_ARRAY][4];

// sizes of our vertexes.  the vertex type can be used as an index into this array
int VertexSizes[] = {5, 5, 7, 7, 9, 11, 5, 3, 12};

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

void R_AddParallaxLightMappedSurfToVArray (msurface_t *surf, float scroll)
{
	glpoly_t *p = surf->polys;
	float	*v;	
	int i;
	

	for (; p; p = p->chain)
	{
		vec3_t	v01, v02, temp1, temp2, temp3;
		vec3_t	normal, binormal, tangent;
		float	s = 0;
		float	*vec;
		float	tangentSpaceTransform[3][3];

		// reset pointer and counter
		VArray = &VArrayVerts[0];
		VertexCounter = 0;

		_VectorSubtract( p->verts[ 1 ], p->verts[0], v01 );
		vec = p->verts[0];
		_VectorCopy(surf->plane->normal, normal); 

		for (v = p->verts[0], i = 0 ; i < p->numverts; i++, v += VERTEXSIZE)
		{

			float currentLength;
			vec3_t currentNormal;

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

			//do calculations for normal, tangent and binormal
			if( i > 1) {
				_VectorSubtract( p->verts[ i ], p->verts[0], temp1 );

				CrossProduct( temp1, v01, currentNormal );
				currentLength = VectorLength( currentNormal );

				if( currentLength > s )
				{
					s = currentLength;
					_VectorCopy( currentNormal, normal );

					vec = p->verts[i];
					_VectorCopy( temp1, v02 );

				}
			}

			// nothing else is needed
			// increment pointer and counter
			VArray += VertexSizes[VERT_MULTI_TEXTURED];
			VertexCounter++;
		}

		VectorNormalize( normal ); //we have the largest normal

		tangentSpaceTransform[ 0 ][ 2 ] = normal[ 0 ];
		tangentSpaceTransform[ 1 ][ 2 ] = normal[ 1 ];
		tangentSpaceTransform[ 2 ][ 2 ] = normal[ 2 ];

		//now get the tangent
		s = ( p->verts[ 1 ][ 3 ] - p->verts[ 0 ][ 3 ] )
			* ( vec[ 4 ] - p->verts[ 0 ][ 4 ] );
		s -= ( vec[ 3 ] - p->verts[ 0 ][ 3 ] )
			 * ( p->verts[ 1 ][ 4 ] - p->verts[ 0 ][ 4 ] );
		s = 1.0f / s;

		VectorScale( v01, vec[ 4 ] - p->verts[ 0 ][ 4 ], temp1 );
		VectorScale( v02, p->verts[ 1 ][ 4 ] - p->verts[ 0 ][ 4 ], temp2 );
		_VectorSubtract( temp1, temp2, temp3 );
		VectorScale( temp3, s, tangent );
		VectorNormalize( tangent ); 

		tangentSpaceTransform[ 0 ][ 0 ] = tangent[ 0 ];
		tangentSpaceTransform[ 1 ][ 0 ] = tangent[ 1 ];
		tangentSpaceTransform[ 2 ][ 0 ] = tangent[ 2 ];

		//now get the binormal
		VectorScale( v02, p->verts[ 1 ][ 3 ] - p->verts[ 0 ][ 3 ], temp1 );
		VectorScale( v01, vec[ 3 ] - p->verts[ 0 ][ 3 ], temp2 );
		_VectorSubtract( temp1, temp2, temp3 );
		VectorScale( temp3, s, binormal );
		VectorNormalize( binormal ); 

		tangentSpaceTransform[ 0 ][ 1 ] = binormal[ 0 ];
		tangentSpaceTransform[ 1 ][ 1 ] = binormal[ 1 ];
		tangentSpaceTransform[ 2 ][ 1 ] = binormal[ 2 ];

		//send these to the shader program
		glUniformMatrix3fvARB( g_tangentSpaceTransform,
								1,
								GL_FALSE,
								tangentSpaceTransform );
		glUniform3fARB( g_location_eyePos, r_origin[0], r_origin[1], r_origin[2] );
		glUniform1iARB( g_location_fog, map_fog);
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
