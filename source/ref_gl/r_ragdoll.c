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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_ragdoll.h"

//This file will handle all of the ragdoll calculations, etc.

//Notes:

//There are alot of tutorials in which to glean this information.  As best I can tell we will have to do the following

//We will need to get a velocity vector.  This could be done by either calculating the distance from entity origin's last frame, or
//by some other means such as a nearby effect like a rocket explosion to get the vector.  We also may need to figure out which part
//of the ragdoll to apply this velocity to - ie, say an explosion hits their feet rather than their torso.  For starting out, let's
//concentrate on the torso.

//Once the frames are at the death stage, the ragdoll takes over.

//The world objects are created each time a ragdoll is spawned.  They consist of surfaces that are in proximity of the spawned
//ragdoll.  This has to be done this way, because building a massive object out of the entire map makes ODE craw to a very 
//slow pace.  Each ragdoll will have a space to contain it's world geometry, which will get cleared when that ragdoll expires.
//We will use a recursive world function for this, and filter out unneeded surfaces.  

//The ragdoll will need to either be hardcoded in, or read in at load time.  To begin, we will
//hardcode one in.  Since our playermodels use roughly the same skeleton, this shouldn't be too bad for initial testing.
//Constraints are pretty straightforward, and very easy to deal with.  ODE allows for a variety of joint types that make
//this much easier.

//So how does the ragoll then animate the mesh?  We need to get rotation and location vectors from the
//ragdoll, and apply them to the models skeleton.  We would need to use the same tech we use for player pitch spine bending to
//manipulate the skeleton.  We get rotation and location of body parts, and that is it.

//Ragdolls will be batched and drawn just as regular entities, which also means that entities should not be drawn in their normal
//batch once in a death animation.  When death animations begin, a ragdoll will be generated with the appropriate properties including
//position, velocity, etc.  The ragdoll will be timestamped, and will be removed from the stack after a certain amount of time has
//elapsed.  We also need to know some information from the entity to transfer to the ragdoll, such as the model and skin.

//This tutorial is straightforward and useful, and though it's in python, it's easily translated into C code.

//There are several examples at http://opende.sourceforge.net/wiki/index.php/HOWTO_rag-doll

cvar_t *r_ragdolls;

vec3_t rightAxis, leftAxis, upAxis, downAxis, bkwdAxis, fwdAxis;

signed int sign(signed int x)
{
	if( x > 0.0 )
		return 1.0;
	else
		return -1.0;
}

void norm3(vec3_t v, vec3_t out)
{
	float l = VectorLength(v);

	if(l > 0.0)
	{
		out[0] = v[0] / l;
		out[1] = v[1] / l;
		out[2] = v[2] / l;

		return;
	}
	else
	{
		VectorClear(out);
	}
}

//vec_t* project3(vec3_t v, vec3_t d)
//{
//	return mul3(v, DotProduct(norm3(v), d));
//}

double acosdot3(vec3_t a, vec3_t b)
{
	double x;

	x = DotProduct(a, b);
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

matrix3x3_t* calcRotMatrix(vec3_t axis, float angle)
{
	float cosTheta = cosf(angle);
	float sinTheta = sinf(angle);
	float t = 1.0 - cosTheta;

	matrix3x3_t *rm = NULL;

	rm->a[0] = t * pow(axis[0],2) + cosTheta;
	rm->a[1] = t * axis[0] * axis[1] - sinTheta * axis[2];
	rm->a[2] = t * axis[0] * axis[2] + sinTheta * axis[1];
	rm->b[0] = t * axis[0] * axis[1] + sinTheta * axis[2];
	rm->b[1] = t * pow(axis[1], 2) + cosTheta;
	rm->b[2] = t * axis[1] * axis[2] - sinTheta * axis[0];
	rm->c[0] = t * axis[0] * axis[2] - sinTheta * axis[1];
	rm->c[1] = t * axis[1] * axis[2] + sinTheta * axis[0];
	rm->c[2] = t * pow(axis[2], 2) + cosTheta;

	return rm;
}

matrix4x4_t* makeOpenGLMatrix(matrix3x3_t r, vec3_t p)
{
	matrix4x4_t *rm = NULL;

	rm->a[0] = r.a[0];
	rm->a[1] = r.b[0];
	rm->a[2] = r.c[0];
	rm->a[3] = 0.0;
	rm->b[0] = r.a[1];
	rm->b[1] = r.b[1];
	rm->b[2] = r.c[1];
	rm->b[3] = 0.0;
	rm->c[0] = r.a[2];
	rm->c[1] = r.b[2];
	rm->c[2] = r.c[2];
	rm->c[3] = 0.0;
	rm->d[0] = p[0];
	rm->d[1] = p[1];
	rm->d[2] = p[2];
	rm->d[3] = 1.0;

	return rm;
}

//routine to create ragdoll body parts between two joints
void R_addBody(int RagDollID, char *name, int objectID, vec3_t p1, vec3_t p2, float radius, float density)
{
	//Adds a capsule body between joint positions p1 and p2 and with given
	//radius to the ragdoll.
	float length;
	vec3_t xa, ya, za, temp;
	dMatrix3 rot;
	const dReal* initialQ;
	dQuaternion initialQuaternion;

	VectorAdd(p1, currententity->origin, p1);
	VectorAdd(p2, currententity->origin, p2);

	//cylinder length not including endcaps, make capsules overlap by half
	//radius at joints
	VectorSubtract(p1, p2, temp);
	length = VectorLength(temp) - radius;

	//create body id
	RagDoll[RagDollID].RagDollObject[objectID].body = dBodyCreate(RagDollWorld);	

	//creat the geometry and give it a name
	RagDoll[RagDollID].RagDollObject[objectID].geom = dCreateCapsule (RagDollSpace, radius, length);
	dGeomSetData(RagDoll[RagDollID].RagDollObject[objectID].geom, name);
	dGeomSetBody(RagDoll[RagDollID].RagDollObject[objectID].geom, RagDoll[RagDollID].RagDollObject[objectID].body);

	//set it's mass
	dMassSetCapsule (&RagDoll[RagDollID].RagDollObject[objectID].mass, density, 3, radius, length);
	dBodySetMass(RagDoll[RagDollID].RagDollObject[objectID].body, &RagDoll[RagDollID].RagDollObject[objectID].mass);

	//define body rotation automatically from body axis
	VectorSubtract(p2, p1, za);
	norm3(za, za);

	VectorSet(temp, 1.0, 0.0, 0.0);
	if (abs(DotProduct(za, temp)) < 0.7)
		VectorSet(xa, 1.0, 0.0, 0.0);
	else
		VectorSet(xa, 0.0, 1.0, 0.0);

	CrossProduct(za, xa, ya);

	CrossProduct(ya, za, xa);
	norm3(xa, xa);
	CrossProduct(za, xa, ya);

	rot[0] = xa[0];
	rot[1] = ya[0];
	rot[2] = za[0];
	rot[3] = xa[1];
	rot[4] = ya[1];
	rot[5] = za[1];
	rot[6] = xa[2];
	rot[7] = ya[2];
	rot[8] = za[2];

	VectorAdd(p1, p2, temp);
	VectorScale(temp, 0.5, temp);

	dBodySetPosition(RagDoll[RagDollID].RagDollObject[objectID].body, temp[0], temp[1], temp[2]);
	dBodySetRotation(RagDoll[RagDollID].RagDollObject[objectID].body, rot);
	dBodySetForce(RagDoll[RagDollID].RagDollObject[objectID].body, 0, 0, 0);
	dBodySetLinearVel(RagDoll[RagDollID].RagDollObject[objectID].body, 0, 0, 0);
	dBodySetAngularVel(RagDoll[RagDollID].RagDollObject[objectID].body, 0, 0, 0);

	initialQ = dBodyGetQuaternion(RagDoll[RagDollID].RagDollObject[objectID].body);
	initialQuaternion[0] = initialQ[0];
	initialQuaternion[1] = initialQ[1];
	initialQuaternion[2] = initialQ[2];
	initialQuaternion[3] = initialQ[3];
	dBodySetQuaternion(RagDoll[RagDollID].RagDollObject[objectID].body, initialQuaternion);
}

//joint creation routines
void R_addFixedJoint(int RagDollID, int jointID, dBodyID body1, dBodyID body2)
{
		RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateFixed(RagDollWorld, 0);
		dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);
}

void R_addHingeJoint(int RagDollID, int jointID, dBodyID body1, dBodyID body2, vec3_t anchor, vec3_t axis, float loStop, float hiStop)
{
	VectorAdd(anchor, currententity->origin, anchor);

	RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateHinge(RagDollWorld, 0);

	dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);

	dJointSetHingeAnchor(RagDoll[RagDollID].RagDollJoint[jointID], anchor[0], anchor[1], anchor[2]);
    dJointSetHingeAxis(RagDoll[RagDollID].RagDollJoint[jointID], axis[0], axis[1], axis[2]);
    dJointSetHingeParam(RagDoll[RagDollID].RagDollJoint[jointID], dParamLoStop, loStop);
    dJointSetHingeParam(RagDoll[RagDollID].RagDollJoint[jointID], dParamHiStop,  hiStop);
}

void R_addBallJoint(int RagDollID, int jointID, dBodyID body1, dBodyID body2, vec3_t anchor)
{
	VectorAdd(anchor, currententity->origin, anchor);

	RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateBall(RagDollWorld, 0);

	dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);

	dJointSetBallAnchor(RagDoll[RagDollID].RagDollJoint[jointID], anchor[0], anchor[1], anchor[2]);
}

void R_CreateWorldObject( void )
{
	// Initialize the world
	RagDollWorld = dWorldCreate();

	dWorldSetGravity(RagDollWorld, 0.0, -4.0, 0.0);

	RagDollSpace = dSimpleSpaceCreate(0);

	contactGroup = dJointGroupCreate(0);

	dWorldSetAutoDisableFlag(RagDollWorld, 1);
		
	//axi used to determine constrained joint rotations
	VectorSet(rightAxis, 1.0, 0.0, 0.0);
	VectorSet(leftAxis, -1.0, 0.0, 0.0);
	VectorSet(upAxis, 0.0, 1.0, 0.0);
	VectorSet(downAxis, 0.0, -1.0, 0.0);
	VectorSet(bkwdAxis, 0.0, 0.0, 1.0);
	VectorSet(fwdAxis, 0.0, 0.0, -1.0);	

	lastODEUpdate = Sys_Milliseconds();

	r_DrawingRagDoll = false;
}

void R_DestroyWorldObject( void )
{
	if(RagDollWorld)
		dWorldDestroy(RagDollWorld);

	if(RagDollSpace)
		dSpaceDestroy (RagDollSpace);

	if(contactGroup)
		dJointGroupDestroy(contactGroup);
}

//For creating the surfaces for the ragdoll to collide with
void GL_BuildODEGeoms(msurface_t *surf)
{
	glpoly_t *p = surf->polys;
	float	*v;
	int		i, j, k, offset, VertexCounter;
	int		ODEIndexCount = 0;
	float	ODEVerts[MAX_VARRAY_VERTS]; //can, should this be done dynamically?
	int		ODEIndices[MAX_INDICES];
	int		ODETris = 0;
	//winding order for ODE
	const int indices[6] = {2,1,0,
							3,2,0};

	if(r_SurfaceCount > MAX_SURFACES - 1 )
		return;

	// reset pointer and counter
	VertexCounter = k = 0;

	for (; p; p = p->chain)
	{
		for (v = p->verts[0], i = 0 ; i < p->numverts; i++, v += VERTEXSIZE)
		{

			ODEVerts[VertexCounter] = v[0];
			ODEVerts[VertexCounter+1] = v[1];
			ODEVerts[VertexCounter+2] = v[2];

			VertexCounter++;
		}

		//create indices for each tri
		ODETris = p->numverts - 2; //First 3 verts = 1 tri, each vert after the third creates a new triangle
		ODEIndexCount += 3*ODETris; //3 indices per tri

		//this next block is to create indices for the entire mesh.  I think it should work in theory, but
		//it's only my theory, and not confirmed just yet.
		j = offset = 0;
		for(i = 0; i < ODETris; i++)
		{
			if(j > 3)
			{
				j = 0;
				offset+=2;
			}
			ODEIndices[k+0] = indices[0+j]+offset;
			ODEIndices[k+1] = indices[1+j]+offset;
			ODEIndices[k+2] = indices[2+j]+offset;
			j+=3;
			k+=3;
		}
	}

	//we need to build the trimesh geometry for this surface
	triMesh[r_SurfaceCount] = dGeomTriMeshDataCreate();

	//// Build the mesh from the data
	dGeomTriMeshDataBuildSimple(triMesh[r_SurfaceCount], (dReal*)ODEVerts,
		VertexCounter, (dTriIndex*)ODEIndices, ODEIndexCount);

	WorldGeometry[r_SurfaceCount] = dCreateTriMesh(RagDollSpace, triMesh[r_SurfaceCount], NULL, NULL, NULL);
	dGeomSetData(WorldGeometry[r_SurfaceCount], "surface");

	dGeomSetBody(WorldGeometry[r_SurfaceCount], WorldBody);

	r_SurfaceCount++;
}

//build and set initial position of ragdoll - note this will need to get some information from the skeleton on the first death frame
void R_RagdollBody_Init( int RagDollID, vec3_t origin )
{
	vec3_t temp, p1, p2;
	float density;

	VectorCopy(origin, RagDoll[RagDollID].origin);
	RagDoll[RagDollID].spawnTime = Sys_Milliseconds();
	RagDoll[RagDollID].destroyed = false;

	//set upper body
	VectorSet(RagDoll[RagDollID].R_SHOULDER_POS, -SHOULDER_W * 0.5, SHOULDER_H, 0.0);
	VectorSet(RagDoll[RagDollID].L_SHOULDER_POS, SHOULDER_W * 0.5, SHOULDER_H, 0.0);

	VectorSet(temp, UPPER_ARM_LEN, 0.0, 0.0);
	VectorSubtract(RagDoll[RagDollID].R_SHOULDER_POS, temp, RagDoll[RagDollID].R_ELBOW_POS);
	VectorAdd(RagDoll[RagDollID].L_SHOULDER_POS, temp, RagDoll[RagDollID].L_ELBOW_POS);

	VectorSet(temp, FORE_ARM_LEN, 0.0, 0.0);
	VectorSubtract(RagDoll[RagDollID].R_ELBOW_POS, temp, RagDoll[RagDollID].R_WRIST_POS);
	VectorAdd(RagDoll[RagDollID].L_ELBOW_POS, temp, RagDoll[RagDollID].L_WRIST_POS);

	VectorSet(temp, HAND_LEN, 0.0, 0.0);
	VectorSubtract(RagDoll[RagDollID].R_WRIST_POS, temp,RagDoll[RagDollID].R_FINGERS_POS);
	VectorAdd(RagDoll[RagDollID].L_WRIST_POS, temp, RagDoll[RagDollID].L_FINGERS_POS);

	//set lower body
	VectorSet(RagDoll[RagDollID].R_HIP_POS, -LEG_W * 0.5, HIP_H, 0.0);
	VectorSet(RagDoll[RagDollID].L_HIP_POS, LEG_W * 0.5, HIP_H, 0.0);
	VectorSet(RagDoll[RagDollID].R_KNEE_POS, -LEG_W * 0.5, KNEE_H, 0.0);
	VectorSet(RagDoll[RagDollID].L_KNEE_POS, LEG_W * 0.5, KNEE_H, 0.0);
	VectorSet(RagDoll[RagDollID].R_ANKLE_POS, -LEG_W * 0.5, ANKLE_H, 0.0);
	VectorSet(RagDoll[RagDollID].L_ANKLE_POS, LEG_W * 0.5, ANKLE_H, 0.0);

	VectorSet(temp, 0.0, 0.0, HEEL_LEN);
	VectorSubtract(RagDoll[RagDollID].R_ANKLE_POS, temp, RagDoll[RagDollID].R_HEEL_POS);
	VectorSubtract(RagDoll[RagDollID].L_ANKLE_POS, temp, RagDoll[RagDollID].L_HEEL_POS);
	VectorSet(temp, 0.0, 0.0, FOOT_LEN);
	VectorAdd(RagDoll[RagDollID].R_ANKLE_POS, temp, RagDoll[RagDollID].R_TOES_POS);
	VectorAdd(RagDoll[RagDollID].L_ANKLE_POS, temp, RagDoll[RagDollID].L_TOES_POS);

	//build the ragdoll parts
	density = 1.0; //for now

	VectorSet(p1, -CHEST_W * 0.5, CHEST_H, 0.0);
	VectorSet(p2, CHEST_W * 0.5, CHEST_H, 0.0);
	R_addBody(RagDollID, "chest", CHEST, p1, p2, 0.13, density); 

	VectorSet(p1, 0.0, CHEST_H - 0.1, 0.0);
	VectorSet(p2, 0.0, HIP_H + 0.1, 0.0);
	R_addBody(RagDollID, "belly", BELLY, p1, p2, 0.125, density);
	
	R_addFixedJoint(RagDollID, MIDSPINE, RagDoll[RagDollID].RagDollObject[CHEST].body, RagDoll[RagDollID].RagDollObject[BELLY].body);

	VectorSet(p1, -PELVIS_W * 0.5, HIP_H, 0.0);
	VectorSet(p2, PELVIS_W * 0.5, HIP_H, 0.0);
	R_addBody(RagDollID, "pelvis", PELVIS, p1, p2, 0.125, density);

	R_addFixedJoint(RagDollID, LOWSPINE, RagDoll[RagDollID].RagDollObject[BELLY].body, RagDoll[RagDollID].RagDollObject[PELVIS].body);

	VectorSet(p1, 0.0, HEAD_H, 0.0);
	VectorSet(p2, 0.0, -HEAD_H, 0.0);
	R_addBody(RagDollID, "head", HEAD, p1, p2, 0.11, density);

	VectorSet(p1, 0.0, NECK_H, 0.0);
	R_addBallJoint(RagDollID, NECK, RagDoll[RagDollID].RagDollObject[CHEST].body, RagDoll[RagDollID].RagDollObject[HEAD].body, p1);

	R_addBody(RagDollID, "rightupperleg", RIGHTUPPERLEG, RagDoll[RagDollID].R_HIP_POS, RagDoll[RagDollID].R_KNEE_POS, 0.11, density);

	R_addBallJoint(RagDollID, RIGHTHIP, RagDoll[RagDollID].RagDollObject[PELVIS].body, RagDoll[RagDollID].RagDollObject[RIGHTUPPERLEG].body,
		RagDoll[RagDollID].R_HIP_POS);

	R_addBody(RagDollID, "leftupperleg", LEFTUPPERLEG, RagDoll[RagDollID].L_HIP_POS, RagDoll[RagDollID].L_KNEE_POS, 0.11, density);

	R_addBallJoint(RagDollID, LEFTHIP, RagDoll[RagDollID].RagDollObject[PELVIS].body, RagDoll[RagDollID].RagDollObject[LEFTUPPERLEG].body,
		RagDoll[RagDollID].L_HIP_POS);
	
	R_addBody(RagDollID, "rightlowerleg", RIGHTLOWERLEG, RagDoll[RagDollID].R_KNEE_POS, RagDoll[RagDollID].R_ANKLE_POS, 0.09, density);

	R_addHingeJoint(RagDollID, RIGHTKNEE, RagDoll[RagDollID].RagDollObject[RIGHTUPPERLEG].body, 
		RagDoll[RagDollID].RagDollObject[RIGHTLOWERLEG].body, RagDoll[RagDollID].R_KNEE_POS, leftAxis, 0.0, M_PI * 0.75);

	R_addBody(RagDollID, "leftlowerleg", LEFTLOWERLEG, RagDoll[RagDollID].L_KNEE_POS, RagDoll[RagDollID].L_ANKLE_POS, 0.09, density);

	R_addHingeJoint(RagDollID, LEFTKNEE, RagDoll[RagDollID].RagDollObject[LEFTUPPERLEG].body, 
		RagDoll[RagDollID].RagDollObject[LEFTLOWERLEG].body, RagDoll[RagDollID].L_KNEE_POS, leftAxis, 0.0, M_PI * 0.75);

	R_addBody(RagDollID, "rightfoot", RIGHTFOOT, RagDoll[RagDollID].R_HEEL_POS, RagDoll[RagDollID].R_TOES_POS, 0.09, density);

	R_addHingeJoint(RagDollID, RIGHTANKLE, RagDoll[RagDollID].RagDollObject[RIGHTLOWERLEG].body, 
		RagDoll[RagDollID].RagDollObject[RIGHTFOOT].body, RagDoll[RagDollID].R_ANKLE_POS, rightAxis, M_PI * -0.1, M_PI * 0.05);

	R_addBody(RagDollID, "leftfoot", LEFTFOOT, RagDoll[RagDollID].L_HEEL_POS, RagDoll[RagDollID].L_TOES_POS, 0.09, density);

	R_addHingeJoint(RagDollID, LEFTANKLE, RagDoll[RagDollID].RagDollObject[LEFTLOWERLEG].body, 
		RagDoll[RagDollID].RagDollObject[LEFTFOOT].body, RagDoll[RagDollID].L_ANKLE_POS, rightAxis, M_PI * -0.1, M_PI * 0.05);

	R_addBody(RagDollID, "rightupperarm", RIGHTUPPERARM, RagDoll[RagDollID].R_SHOULDER_POS, RagDoll[RagDollID].R_ELBOW_POS, 0.08, density);

	R_addBallJoint(RagDollID, RIGHTSHOULDER, RagDoll[RagDollID].RagDollObject[CHEST].body, 
		RagDoll[RagDollID].RagDollObject[RIGHTUPPERARM].body, RagDoll[RagDollID].R_SHOULDER_POS);

	R_addBody(RagDollID, "leftupperarm", LEFTUPPERARM, RagDoll[RagDollID].L_SHOULDER_POS, RagDoll[RagDollID].L_ELBOW_POS, 0.08, density);

	R_addBallJoint(RagDollID, LEFTSHOULDER, RagDoll[RagDollID].RagDollObject[CHEST].body, 
		RagDoll[RagDollID].RagDollObject[LEFTUPPERARM].body, RagDoll[RagDollID].L_SHOULDER_POS);

	R_addBody(RagDollID, "rightforearm", RIGHTFOREARM, RagDoll[RagDollID].R_ELBOW_POS, RagDoll[RagDollID].R_WRIST_POS, 0.08, density);

	R_addHingeJoint(RagDollID, RIGHTELBOW, RagDoll[RagDollID].RagDollObject[RIGHTUPPERARM].body, 
		RagDoll[RagDollID].RagDollObject[RIGHTFOREARM].body, RagDoll[RagDollID].R_ELBOW_POS, downAxis, 0.0, M_PI * 0.6);

	R_addBody(RagDollID, "leftforearm", LEFTFOREARM, RagDoll[RagDollID].L_ELBOW_POS, RagDoll[RagDollID].L_WRIST_POS, 0.08, density);

	R_addHingeJoint(RagDollID, LEFTELBOW, RagDoll[RagDollID].RagDollObject[LEFTUPPERARM].body, 
		RagDoll[RagDollID].RagDollObject[LEFTFOREARM].body, RagDoll[RagDollID].L_ELBOW_POS,  upAxis, 0.0, M_PI * 0.6);

	R_addBody(RagDollID, "righthand", RIGHTHAND, RagDoll[RagDollID].R_WRIST_POS, RagDoll[RagDollID].R_FINGERS_POS, 0.075, density);

	R_addHingeJoint(RagDollID, RIGHTWRIST, RagDoll[RagDollID].RagDollObject[RIGHTFOREARM].body, 
		RagDoll[RagDollID].RagDollObject[RIGHTHAND].body, RagDoll[RagDollID].R_WRIST_POS, fwdAxis, M_PI * -0.1, M_PI * 0.2);

	R_addBody(RagDollID, "leftthand", LEFTHAND, RagDoll[RagDollID].L_WRIST_POS, RagDoll[RagDollID].L_FINGERS_POS, 0.075, density);

	R_addHingeJoint(RagDollID, LEFTWRIST, RagDoll[RagDollID].RagDollObject[LEFTFOREARM].body, 
		RagDoll[RagDollID].RagDollObject[LEFTHAND].body, RagDoll[RagDollID].L_WRIST_POS, bkwdAxis, M_PI * -0.1, M_PI * 0.2);

	//we need some information from our current entity
	RagDoll[RagDollID].ragDollMesh = (model_t *)malloc (sizeof(model_t));
	memcpy(RagDoll[RagDollID].ragDollMesh, currententity->model, sizeof(model_t)); 

	//to do - get bone rotations from first death frame, apply to ragdoll for initial position
	//we will need to set the velocity based on origin vs old_origin
}

/*
	Callback function for the collide() method.

	This function checks if the given geoms do collide and creates contact
	joints if they do.
*/
static void near_callback(void *data, dGeomID geom1, dGeomID geom2)
{
	dContact contact[MAX_CONTACTS];
	int i, numc;
	dJointID j;
	dBodyID body1 = dGeomGetBody(geom1);
	dBodyID body2 = dGeomGetBody(geom2);

	if (dAreConnected(body1, body2))
		return;

	// create contact joints
	for (i = 0; i < MAX_CONTACTS; i++) {
		contact[i].surface.mode = dContactBounce; // Bouncy surface
		contact[i].surface.bounce = 0.2;
		contact[i].surface.mu = 500.0; // Friction
		contact[i].surface.mu2 = 0;
		contact[i].surface.bounce_vel = 0.1;
	}

	if (numc = dCollide(geom1, geom2, MAX_CONTACTS, &contact[0].geom, sizeof(dContact)))
    {
        // To add each contact point found to our joint group we call dJointCreateContact which is just one of the many
        // different joint types available.  
        for (i = 0; i < numc; i++)
        {
            // dJointCreateContact needs to know which world and joint group to work with as well as the dContact
            // object itself. It returns a new dJointID which we then use with dJointAttach to finally create the
            // temporary contact joint between the two geom bodies.
            j = dJointCreateContact(RagDollWorld, contactGroup, &contact[i]);
			dJointAttach(j, body1, body2);
        }
    }
}

void R_DestroyRagDoll(int RagDollID, qboolean nuke)
{
	int i;

	VectorSet(RagDoll[RagDollID].origin, 0, 0, 0);

	if(RagDoll[RagDollID].ragDollMesh)
		free(RagDoll[RagDollID].ragDollMesh);

	if(!nuke)
		return; //next few things are causing problems if done during action

	//destroy surfaces - better to track actual num of surfaces instead of max_surfaces!
	for(i = 0; i < MAX_SURFACES; i++)
	{
		dGeomDestroy(RagDoll[RagDollID].WorldGeometry[i]);
	}

	//we also want to destroy all ragdoll bodies and joints for this ragdoll
	for(i = CHEST; i <= LEFTHAND; i++)
	{
		dGeomDestroy(RagDoll[RagDollID].RagDollObject[i].geom);
		dBodyDestroy(RagDoll[RagDollID].RagDollObject[i].body);
	}

	for(i = MIDSPINE; i <= LEFTWRIST; i++)
		dJointDestroy(RagDoll[RagDollID].RagDollJoint[i]);
}

//This is called on every map load
void R_ClearAllRagdolls( void )
{
	int RagDollID;

	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		R_DestroyRagDoll(RagDollID, false);
		RagDoll[RagDollID].destroyed = true;
	}

	r_DrawingRagDoll = false;
}

void R_AddNewRagdoll( vec3_t origin )
{
	int RagDollID;
	vec3_t dist;

	r_DrawingRagDoll = true; //we are rendering a Ragdoll somewhere

	//add a ragdoll, look for first open slot
	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		VectorSubtract(origin, RagDoll[RagDollID].origin, dist);
		if(VectorLength(dist) < 16)
			break; //likely spawned from same ent, this may need tweaking
		if(RagDoll[RagDollID].destroyed)
		{
			//need to create geometry in the vicinity of the ragdoll(not the entire map, this is painfully slow)
			//for initial testing, let's just build a simple plane at the feet of the player
			R_RagdollBody_Init(RagDollID, origin);
			//to do - need to add nearby surfaces anytime a ragdoll is spawned
			Com_Printf("Added a ragdoll\n");
			break;
		}
	}
}

void R_RenderAllRagdolls ( void )
{
	int RagDollID;
	int numRagDolls = 0;

	if(!r_ragdolls->value)
		return;

	//main physics loop
	if(!r_DrawingRagDoll)
		return;

	//note - this is called each server frame, not each render frame, and interpolated 
	if((Sys_Milliseconds() - lastODEUpdate) > 100)
	{
		dSpaceCollide(RagDollSpace, 0, &near_callback);

		dWorldQuickStep(RagDollWorld, 0.05); //this is likely wrong, check!

		// Remove all temporary collision joints now that the world has been stepped
		dJointGroupEmpty(contactGroup);

		lastODEUpdate = Sys_Milliseconds();
	}

	//Iterate though the ragdoll stack, and render each one that is active.

	//This function would look very similar to the iqm/alias entity routine, but will call a
	//different animation function.  This function will keep track of the time as well, and
	//remove any expired ragdolls off of the stack.

	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		if(Sys_Milliseconds() - RagDoll[RagDollID].spawnTime > RAGDOLL_DURATION)
		{
			if(!RagDoll[RagDollID].destroyed)
			{
				//add routines to destroy all ragdoll elements, namely geometry that build for it collide with
				R_DestroyRagDoll(RagDollID, false);
				RagDoll[RagDollID].destroyed = true;
				Com_Printf("Destroyed a ragdoll");
			}
			continue;
		}
		else 
		{
			//we handle the ragdoll's physics, then render the mesh with skeleton adjusted by ragdoll
			//body object positions
			numRagDolls++;
		}
	}

	if(!numRagDolls)
		r_DrawingRagDoll = false; //no sense in coming back in here until we add a ragdoll
}
