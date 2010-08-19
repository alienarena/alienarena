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
// r_math.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../game/q_shared.h"
#include "r_math.h"

const mat4x4_t mat4x4_identity =
{
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
};

void Matrix4_Identity( mat4x4_t m )
{
	int i;

	for( i = 0; i < 16; i++ ) {
		if( i == 0 || i == 5 || i == 10 || i == 15 )
			m[i] = 1.0;
		else
			m[i] = 0.0;
	}
}

void Matrix4_Copy( const mat4x4_t m1, mat4x4_t m2 )
{
	int i;

	for( i = 0; i < 16; i++ )
		m2[i] = m1[i];
}

qboolean Matrix4_Compare( const mat4x4_t m1, const mat4x4_t m2 )
{
	int i;

	for( i = 0; i < 16; i++ )
		if( m1[i] != m2[i] )
			return false;

	return true;
}

void Matrix4_Multiply( const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out )
{
	out[0]  = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2] + m1[12] * m2[3];
	out[1]  = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2] + m1[13] * m2[3];
	out[2]  = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2] + m1[14] * m2[3];
	out[3]  = m1[3] * m2[0] + m1[7] * m2[1] + m1[11] * m2[2] + m1[15] * m2[3];
	out[4]  = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6] + m1[12] * m2[7];
	out[5]  = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6] + m1[13] * m2[7];
	out[6]  = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6] + m1[14] * m2[7];
	out[7]  = m1[3] * m2[4] + m1[7] * m2[5] + m1[11] * m2[6] + m1[15] * m2[7];
	out[8]  = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10] + m1[12] * m2[11];
	out[9]  = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10] + m1[13] * m2[11];
	out[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10] + m1[14] * m2[11];
	out[11] = m1[3] * m2[8] + m1[7] * m2[9] + m1[11] * m2[10] + m1[15] * m2[11];
	out[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12] * m2[15];
	out[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13] * m2[15];
	out[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14] * m2[15];
	out[15] = m1[3] * m2[12] + m1[7] * m2[13] + m1[11] * m2[14] + m1[15] * m2[15];
}

void Matrix4_MultiplyFast( const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out )
{
	out[0]  = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2];
	out[1]  = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2];
	out[2]  = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2];
	out[3]  = 0.0f;
	out[4]  = m1[0] * m2[4] + m1[4] * m2[5] + m1[8] * m2[6];
	out[5]  = m1[1] * m2[4] + m1[5] * m2[5] + m1[9] * m2[6];
	out[6]  = m1[2] * m2[4] + m1[6] * m2[5] + m1[10] * m2[6];
	out[7]  = 0.0f;
	out[8]  = m1[0] * m2[8] + m1[4] * m2[9] + m1[8] * m2[10];
	out[9]  = m1[1] * m2[8] + m1[5] * m2[9] + m1[9] * m2[10];
	out[10] = m1[2] * m2[8] + m1[6] * m2[9] + m1[10] * m2[10];
	out[11] = 0.0f;
	out[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[8] * m2[14] + m1[12];
	out[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[9] * m2[14] + m1[13];
	out[14] = m1[2] * m2[12] + m1[6] * m2[13] + m1[10] * m2[14] + m1[14];
	out[15] = 1.0f;
}

void Matrix4_Rotate( mat4x4_t m, vec_t angle, vec_t x, vec_t y, vec_t z )
{
	mat4x4_t t, b;
	vec_t c = cos( DEG2RAD(angle) );
	vec_t s = sin( DEG2RAD(angle) );
	vec_t mc = 1 - c, t1, t2;

	t[0]  = (x * x * mc) + c;
	t[5]  = (y * y * mc) + c;
	t[10] = (z * z * mc) + c;

	t1 = y * x * mc;
	t2 = z * s;
	t[1] = t1 + t2;
	t[4] = t1 - t2;

	t1 = x * z * mc;
	t2 = y * s;
	t[2] = t1 - t2;
	t[8] = t1 + t2;

	t1 = y * z * mc;
	t2 = x * s;
	t[6] = t1 + t2;
	t[9] = t1 - t2;

	t[3] = t[7] = t[11] = t[12] = t[13] = t[14] = 0;
	t[15] = 1;

	Matrix4_Copy( m, b );
	Matrix4_MultiplyFast( b, t, m );
}

void Matrix4_Translate( mat4x4_t m, vec_t x, vec_t y, vec_t z )
{
	m[12] = m[0] * x + m[4] * y + m[8]  * z + m[12];
	m[13] = m[1] * x + m[5] * y + m[9]  * z + m[13];
	m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
	m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

void Matrix4_Scale( mat4x4_t m, vec_t x, vec_t y, vec_t z )
{
	m[0] *= x; m[4] *= y; m[8]  *= z;
	m[1] *= x; m[5] *= y; m[9]  *= z;
	m[2] *= x; m[6] *= y; m[10] *= z;
	m[3] *= x; m[7] *= y; m[11] *= z;
}

void Matrix4_Transpose( const mat4x4_t m, mat4x4_t out )
{
	out[0] = m[0]; out[1] = m[4]; out[2] = m[8]; out[3] = m[12];
	out[4] = m[1]; out[5] = m[5]; out[6] = m[9]; out[7] = m[13];
	out[8] = m[2]; out[9] = m[6]; out[10] = m[10]; out[11] = m[14];
	out[12] = m[3]; out[13] = m[7]; out[14] = m[11]; out[15] = m[15];
}

void Matrix4_Matrix( const mat4x4_t in, vec3_t out[3] )
{
	out[0][0] = in[0];
	out[0][1] = in[4];
	out[0][2] = in[8];

	out[1][0] = in[1];
	out[1][1] = in[5];
	out[1][2] = in[9];

	out[2][0] = in[2];
	out[2][1] = in[6];
	out[2][2] = in[10];
}

void Matrix4_Multiply_Vector( const mat4x4_t m, const vec4_t v, vec4_t out )
{
	out[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12] * v[3];
	out[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13] * v[3];
	out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14] * v[3];
	out[3] = m[3] * v[0] + m[7] * v[1] + m[11] * v[2] + m[15] * v[3];
}

//============================================================================

void Matrix4_Copy2D( const mat4x4_t m1, mat4x4_t m2 )
{
	m2[0] = m1[0];
	m2[1] = m1[1];
	m2[4] = m1[4];
	m2[5] = m1[5];
	m2[12] = m1[12];
	m2[13] = m1[13];
}

void Matrix4_Multiply2D( const mat4x4_t m1, const mat4x4_t m2, mat4x4_t out )
{
	out[0]  = m1[0] * m2[0] + m1[4] * m2[1];
	out[1]  = m1[1] * m2[0] + m1[5] * m2[1];
	out[4]  = m1[0] * m2[4] + m1[4] * m2[5];
	out[5]  = m1[1] * m2[4] + m1[5] * m2[5];
	out[12] = m1[0] * m2[12] + m1[4] * m2[13] + m1[12];
	out[13] = m1[1] * m2[12] + m1[5] * m2[13] + m1[13];
}

void Matrix4_Scale2D( mat4x4_t m, vec_t x, vec_t y )
{
	m[0] *= x;
	m[1] *= x;
	m[4] *= y;
	m[5] *= y;
}

void Matrix4_Translate2D( mat4x4_t m, vec_t x, vec_t y )
{
	m[12] += x;
	m[13] += y;
}

void Matrix4_Stretch2D( mat4x4_t m, vec_t s, vec_t t )
{
	m[0] *= s;
	m[1] *= s;
	m[4] *= s;
	m[5] *= s;
	m[12] = s * m[12] + t;
	m[13] = s * m[13] + t;
}
