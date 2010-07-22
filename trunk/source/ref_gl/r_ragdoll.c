/*
Copyright (C) 2010 COR Entertainment, LLC.

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
#include "r_ragdoll.h"
#include "ode/ode.h"

//This file will handle all of the ragdoll calculations, etc.

//Notes:

//There are alot of tutorials in which to glean this information.  As best I can tell we will have to do the following

//We will need to get a velocity vector.  This could be done by either calculating the distance from entity origin's last frame, or
//by some other means such as a nearby effect like a rocket explosion to get the vector.  We also may need to figure out which part
//of the ragdoll to apply this velocity to - ie, say an explosion hits their feet rather than their torso.  For starting out, let's 
//concentrate on the torso.

//Once the frames are at the death stage, the ragdoll takes over.

//From what I best understand from reading the tuts, we have to build a "world" in which things collide.  These would be done easily
//by the existing bsp drawing routines, and simply putting some calls before the vertex arrays are built.  

//The ragdoll will need to either be hardcoded in, or created by an external program and read in at load time.  To begin, we will
//hardcode one in, using the tuts.  Since our playermodels use roughly the same skeleton, this shouldn't be too bad.

//So how does the ragoll then animate the mesh?  As best I can tell, we would need to get rotation and location vectors from the
//ragdoll, and apply them to the models skeleton.  We would need to use the same tech we use for player pitch spine bending to 
//manipulate the skeleton.  Note - the ragdoll is not actually drawn.  We get rotation and location of body parts, and that is it.

//This tutorial is straightforward and useful, and though it's in python, it's easily translated into C code.

//There are several examples at http://opende.sourceforge.net/wiki/index.php/HOWTO_rag-doll


//Stage 1 - math funcs needed - note some of these we already have, but for now we will write new ones

signed int sign(signed int x)
{
	if( x > 0.0 )
		return 1.0;
	else
		return -1.0;
}

float len3(vec3_t v)
{
	return VectorLength(v);
}

vec_t* neg3(vec3_t v)
{
	vec_t *rv = NULL;

	rv[0] = -v[0];
	rv[1] = -v[1];
	rv[2] = -v[2];

	return rv;
}

vec_t* add3(vec3_t a, vec3_t b)
{
	vec_t *rv = NULL;

	rv[0] = a[0] + b[0];
	rv[1] = a[1] + b[1];
	rv[2] = a[2] + b[2];
	
	return rv;
}

vec_t* sub3(vec3_t a, vec3_t b)
{
	vec_t *rv = NULL;

	rv[0] = a[0] - b[0];
	rv[1] = a[1] - b[1];
	rv[2] = a[2] - b[2];

	return rv;
}

vec_t* mul3(vec3_t v, float s)
{
	vec_t *rv = NULL;

	rv[0] = v[0] * s;
	rv[1] = v[1] * s;
	rv[2] = v[2] * s;

	return rv;
}

vec_t* div3(vec3_t v, float s)
{
	vec_t *rv = NULL;

	rv[0] = v[0] / s;
	rv[1] = v[1] / s;
	rv[2] = v[2] / s;

	return rv;
}

float dist3(vec3_t a, vec3_t b)
{
	return VectorLength(sub3(a, b));
}

vec_t* norm3(vec3_t v)
{
	vec_t *rv = NULL;
	float l = len3(v);

	if(l > 0.0)
	{
		rv[0] = v[0] / l;
		rv[1] = v[1] / l;
		rv[2] = v[2] / l;

		return rv;
	}
	else
	{
		VectorClear(rv);
		return rv;
	}
}

float dot3(vec3_t a, vec3_t b)
{
	return(DotProduct(a, b));
}

vec_t* cross(vec3_t a, vec3_t b)
{
	vec_t *crossP = NULL;
	
	CrossProduct(a, b, crossP);
	return crossP;
}

vec_t* project3(vec3_t v, vec3_t d)
{
	return mul3(v, dot3(norm3(v), d));
}

double acosdot3(vec3_t a, vec3_t b)
{
	double x;

	x = dot3(a, b);
	if(x < -1.0)
		return pi;
	else if(x > 1.0)
		return 0.0;
	else
		return acos(x);
}

vec_t* rotate3(matrix3x3_t m, vec3_t v)
{
	vec_t *rv = NULL;

	rv[0] = v[0] * m.a[0] + v[1] * m.a[1] + v[2] * m.a[2];
	rv[1] = v[0] * m.b[0] + v[1] * m.b[1] + v[2] * m.b[2];
	rv[2] = v[0] * m.c[0] + v[1] * m.c[1] + v[2] * m.c[2];

	return rv;
}

matrix3x3_t* invert3x3(matrix3x3_t m)
{
	matrix3x3_t *rm = NULL;

	rm->a[0] = m.a[0];
	rm->a[1] = m.b[0];
	rm->a[2] = m.c[0];
	rm->b[0] = m.a[1];
	rm->b[1] = m.b[1];
	rm->b[2] = m.c[1];
	rm->c[0] = m.a[2];
	rm->c[1] = m.b[2];
	rm->c[2] = m.c[2];

	return rm;
}

vec_t* zaxis(matrix3x3_t m)
{
	vec_t *rm = NULL;

	rm[0] = m.a[2];
	rm[1] = m.b[2];
	rm[2] = m.c[2];

	return rm;
}

/*
def calcRotMatrix(axis, angle):
	"""
	Returns the row-major 3x3 rotation matrix defining a rotation around axis by
	angle.
	"""
	cosTheta = cos(angle)
	sinTheta = sin(angle)
	t = 1.0 - cosTheta
	return (
		t * axis[0]**2 + cosTheta,
		t * axis[0] * axis[1] - sinTheta * axis[2],
		t * axis[0] * axis[2] + sinTheta * axis[1],
		t * axis[0] * axis[1] + sinTheta * axis[2],
		t * axis[1]**2 + cosTheta,
		t * axis[1] * axis[2] - sinTheta * axis[0],
		t * axis[0] * axis[2] - sinTheta * axis[1],
		t * axis[1] * axis[2] + sinTheta * axis[0],
		t * axis[2]**2 + cosTheta)

def makeOpenGLMatrix(r, p):
	"""
	Returns an OpenGL compatible (column-major, 4x4 homogeneous) transformation
	matrix from ODE compatible (row-major, 3x3) rotation matrix r and position
	vector p.
	"""
	return (
		r[0], r[3], r[6], 0.0,
		r[1], r[4], r[7], 0.0,
		r[2], r[5], r[8], 0.0,
		p[0], p[1], p[2], 1.0)

*/

void R_RagdollBody_Init( void )
{
	vec3_t temp;

	//init positions - note - these should probably be read in from the skeleton's initial death anim position
	//or the previous frame's position if possible
	vec3_t R_SHOULDER_POS; 
	vec3_t L_SHOULDER_POS;
	vec3_t R_ELBOW_POS;
	vec3_t L_ELBOW_POS;
	vec3_t R_WRIST_POS;
	vec3_t L_WRIST_POS;
	vec3_t R_FINGERS_POS;
	vec3_t L_FINGERS_POS;

	vec3_t R_HIP_POS; 
	vec3_t L_HIP_POS;
	vec3_t R_KNEE_POS; 
	vec3_t L_KNEE_POS; 
	vec3_t R_ANKLE_POS; 
	vec3_t L_ANKLE_POS;
	vec3_t R_HEEL_POS;
	vec3_t L_HEEL_POS;
	vec3_t R_TOES_POS;
	vec3_t L_TOES_POS;

	//set upper body
	VectorSet(R_SHOULDER_POS, -SHOULDER_W * 0.5, SHOULDER_H, 0.0);
	VectorSet(L_SHOULDER_POS, SHOULDER_W * 0.5, SHOULDER_H, 0.0);

	VectorSet(temp, UPPER_ARM_LEN, 0.0, 0.0);
	VectorCopy(sub3(R_SHOULDER_POS, temp), R_ELBOW_POS);
	VectorCopy(add3(L_SHOULDER_POS, temp), L_ELBOW_POS);
	VectorSet(temp, FORE_ARM_LEN, 0.0, 0.0);
	VectorCopy(sub3(R_ELBOW_POS, temp), R_WRIST_POS);
	VectorCopy(add3(L_ELBOW_POS, temp), L_WRIST_POS);
	VectorSet(temp, HAND_LEN, 0.0, 0.0);
	VectorCopy(sub3(R_WRIST_POS, temp), R_FINGERS_POS);
	VectorCopy(add3(L_WRIST_POS, temp), L_FINGERS_POS);

	//set lower body
	VectorSet(R_HIP_POS, -LEG_W * 0.5, HIP_H, 0.0);
	VectorSet(L_HIP_POS, LEG_W * 0.5, HIP_H, 0.0);
	VectorSet(R_KNEE_POS, -LEG_W * 0.5, KNEE_H, 0.0);
	VectorSet(L_KNEE_POS, LEG_W * 0.5, KNEE_H, 0.0);
	VectorSet(R_ANKLE_POS, -LEG_W * 0.5, ANKLE_H, 0.0);
	VectorSet(L_ANKLE_POS, LEG_W * 0.5, ANKLE_H, 0.0);

	VectorSet(temp, 0.0, 0.0, HEEL_LEN);
	VectorCopy(sub3(R_ANKLE_POS, temp), R_HEEL_POS);
	VectorCopy(sub3(L_ANKLE_POS, temp), L_HEEL_POS);
	VectorSet(temp, 0.0, 0.0, FOOT_LEN);
	VectorCopy(add3(R_ANKLE_POS, temp), R_TOES_POS);
	VectorCopy(add3(L_ANKLE_POS, temp), L_TOES_POS);
}
