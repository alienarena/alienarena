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
	dReal test;

	VectorAdd(p1, RagDoll[RagDollID].origin, p1);
	VectorAdd(p2, RagDoll[RagDollID].origin, p2);

	//remember, players are offset 24 units from ground
	p1[2] -= 24;
	p2[2] -= 24; 

	//cylinder length not including endcaps, make capsules overlap by half
	//radius at joints
	VectorSubtract(p1, p2, temp);
	length = VectorLength(temp) - radius;
	if(length <= 0)
		length = 0.1f;

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

	if(!IS_NAN(xa[0]))
		rot[0] = xa[0];
	else
		rot[0] = 0;
	if(!IS_NAN(ya[0]))
		rot[1] = ya[0];
	else
		ya[0] = 0;
	if(!IS_NAN(za[0]))
		rot[2] = za[0];
	else 
		rot[2] = 0;
	if(!IS_NAN(xa[1]))
		rot[3] = xa[1];
	else
		rot[3] = 0;
	if(!IS_NAN(ya[1]))
		rot[4] = ya[1];
	else
		rot[4] = 0;
	if(!IS_NAN(za[1]))
		rot[5] = za[1];
	else
		rot[5] = 0;
	if(!IS_NAN(xa[2]))
		rot[6] = xa[2];
	else
		rot[6] = 0;
	if(!IS_NAN(ya[2]))
		rot[7] = ya[2];
	else
		rot[7] = 0;
	if(!IS_NAN(za[2]))
		rot[8] = za[2];
	else
		rot[8] = 0;

	VectorAdd(p1, p2, temp);
	VectorScale(temp, 0.5, temp);

	test = VectorLength(temp);
	if(IS_NAN(test))
	{
		Com_Printf("There was a NaN in creating body %i\n", objectID);
		VectorCopy(RagDoll[RagDollID].origin, temp);
	}

	dBodySetPosition(RagDoll[RagDollID].RagDollObject[objectID].body, temp[0], temp[1], temp[2]);
	dBodySetRotation(RagDoll[RagDollID].RagDollObject[objectID].body, rot); //this may not be needed here, check
	dBodySetForce(RagDoll[RagDollID].RagDollObject[objectID].body, 0, 0, 0);
	dBodySetLinearVel(RagDoll[RagDollID].RagDollObject[objectID].body, 0, 40, 150); //for testing, a little upward velocity
	dBodySetAngularVel(RagDoll[RagDollID].RagDollObject[objectID].body, 0, 0, 0);
		
	dQFromAxisAndAngle (initialQuaternion, 1,1,1,1); //test this more
	dBodySetQuaternion(RagDoll[RagDollID].RagDollObject[objectID].body, initialQuaternion);
}

//joint creation routines
void R_addFixedJoint(int RagDollID, int jointID, dBodyID body1, dBodyID body2)
{
		RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateFixed(RagDollWorld, 0);
		dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);
		dJointSetFixed(RagDoll[RagDollID].RagDollJoint[jointID]);
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

	dWorldSetGravity(RagDollWorld, 0.0, 0.0, -256.0); 

	RagDollSpace = dSimpleSpaceCreate(0);

	contactGroup = dJointGroupCreate(0);

	//dWorldSetAutoDisableFlag(RagDollWorld, 1); //probably *don't* want this
		
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
//Notes - right now this is building a geometry for each convex poly, this might the cleanest and easiest way, but it 
//may be nicer and more efficient to build a geometry for the entire surface.  r_surfacecount need to be renamed if we keep 
//things this way.
void GL_BuildODEGeoms(msurface_t *surf, int RagDollID)
{
	glpoly_t *p;
	vec3_t dist, center, mins, maxs;
	float	*v;
	int		i, j, k, offset, VertexCounter;
	int		ODEIndexCount = 0;
	float	ODEVerts[MAX_VERTS];
	int		ODEIndices[MAX_VERTS*4];
	int		ODETris = 0;
	dTriMeshDataID triMesh;
	//winding order for ODE
	const int indices[6] = {2,1,0,
							3,2,0}; 
	
	if(r_SurfaceCount > MAX_SURFACES - 1 )
		return;

	// reset pointer and counter
	VertexCounter = k = 0;	

	for ( p = surf->polys; p; p = p->chain ) 
	{
		if(r_SurfaceCount > MAX_SURFACES - 1 )
			break;
		
		VectorSet(mins, 999999, 999999, 999999);
		VectorSet(maxs, -999999, -999999, -999999);
		
		for (v = p->verts[0], i = 0 ; i < p->numverts; i++, v += VERTEXSIZE)
		{

			ODEVerts[3*VertexCounter] = v[0];
			ODEVerts[3*VertexCounter+1] = v[1];
			ODEVerts[3*VertexCounter+2] = v[2];

			if(v[0] > maxs[0])   maxs[0] = v[0];
            if(v[1] > maxs[1])   maxs[1] = v[1];
            if(v[2] > maxs[2])   maxs[2] = v[2];

            if(v[0] < mins[0])   mins[0] = v[0];
            if(v[1] < mins[1])   mins[1] = v[1];
            if(v[2] < mins[2])   mins[2] = v[2];

			VertexCounter++;
		}

		//find poly center
		center[0] = (mins[0] + maxs[0]) /2;
		center[1] = (mins[1] + maxs[1]) /2;
		center[2] = (mins[2] + maxs[2]) /2;

		//cull distant polys - we may need to adjust the distance based on velocities and other factors
		VectorSubtract(RagDoll[RagDollID].origin, center, dist);
		if(VectorLength(dist) > 512)
			continue;
		
		//create indices for each tri
		ODETris = p->numverts - 2; //First 3 verts = 1 tri, each vert after the third creates a new triangle
		ODEIndexCount += 3*ODETris; //3 indices per tri

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

		//we need to build the trimesh geometry for this poly
		triMesh = dGeomTriMeshDataCreate();

		// Build the mesh from the data
		dGeomTriMeshDataBuildSimple(triMesh, (dReal*)ODEVerts,
			VertexCounter, (dTriIndex*)ODEIndices, ODEIndexCount);

		RagDoll[RagDollID].WorldGeometry[r_SurfaceCount] = dCreateTriMesh(RagDollSpace, triMesh, NULL, NULL, NULL);
		dGeomSetData(RagDoll[RagDollID].WorldGeometry[r_SurfaceCount], "surface");

		// this geom has no body
		dGeomSetBody(RagDoll[RagDollID].WorldGeometry[r_SurfaceCount], 0);
				
		//debug stuff
		VectorCopy(center, RagDoll[RagDollID].surforigins[r_SurfaceCount]);

		r_SurfaceCount++;
	}	
}

//build and set initial position of ragdoll - note this will need to get some information from the skeleton on the first death frame
void R_RagdollBody_Init( int RagDollID, vec3_t origin )
{
	//Ragdoll  positions
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

	vec3_t p1, p2;
	float density;

	//we need some information from the current entity
	RagDoll[RagDollID].ragDollMesh = (model_t *)malloc (sizeof(model_t));
	memcpy(RagDoll[RagDollID].ragDollMesh, currententity->model, sizeof(model_t)); 

	RagDoll[RagDollID].script = (rscript_t *)malloc (sizeof(rscript_t));
	memcpy(RagDoll[RagDollID].script, currententity->script, sizeof(rscript_t));

	RagDoll[RagDollID].texnum = currententity->skin->texnum;
	RagDoll[RagDollID].flags = currententity->flags;
	_VectorCopy(currententity->angles, RagDoll[RagDollID].angles); 
	VectorCopy(origin, RagDoll[RagDollID].origin);
	VectorCopy(origin, RagDoll[RagDollID].curPos);
	RagDoll[RagDollID].spawnTime = Sys_Milliseconds();
	RagDoll[RagDollID].destroyed = false;

	//set upper body
	VectorSet(R_SHOULDER_POS, -SHOULDER_W * 0.5, 0.0, SHOULDER_H);
	VectorSet(L_SHOULDER_POS, SHOULDER_W * 0.5, 0.0, SHOULDER_H);

	VectorSet(R_ELBOW_POS, R_SHOULDER_POS[0] - 0.01*RAGDOLLSCALE, R_SHOULDER_POS[1], R_SHOULDER_POS[2] - UPPER_ARM_LEN);
	VectorSet(L_ELBOW_POS, L_SHOULDER_POS[0] + 0.01*RAGDOLLSCALE, L_SHOULDER_POS[1], L_SHOULDER_POS[2] - UPPER_ARM_LEN);

	VectorSet(R_WRIST_POS, R_ELBOW_POS[0], R_ELBOW_POS[1], R_ELBOW_POS[2] - FORE_ARM_LEN);
	VectorSet(L_WRIST_POS, L_ELBOW_POS[0], L_ELBOW_POS[1], L_ELBOW_POS[2] - FORE_ARM_LEN);

	VectorSet(R_FINGERS_POS, R_WRIST_POS[0], R_WRIST_POS[1], R_WRIST_POS[2] - HAND_LEN);
	VectorSet(L_FINGERS_POS, L_WRIST_POS[0], L_WRIST_POS[1], L_WRIST_POS[2] - HAND_LEN);

	//set lower body
	VectorSet(R_HIP_POS, -LEG_W * 0.5,  0.0, HIP_H);
	VectorSet(L_HIP_POS, LEG_W * 0.5, 0.0, HIP_H);
	VectorSet(R_KNEE_POS, -LEG_W * 0.5, 0.0, KNEE_H);
	VectorSet(L_KNEE_POS, LEG_W * 0.5, 0.0, KNEE_H);
	VectorSet(R_ANKLE_POS, -LEG_W * 0.5, 0.0, ANKLE_H);
	VectorSet(L_ANKLE_POS, LEG_W * 0.5, 0.0, ANKLE_H);

	VectorSet(R_HEEL_POS, R_ANKLE_POS[0], R_ANKLE_POS[1] - HEEL_LEN, R_ANKLE_POS[2]);
	VectorSet(L_HEEL_POS, L_ANKLE_POS[0], L_ANKLE_POS[1] - HEEL_LEN, L_ANKLE_POS[2]);

	VectorSet(R_TOES_POS, R_ANKLE_POS[0], R_ANKLE_POS[1] + FOOT_LEN, R_ANKLE_POS[2]);
	VectorSet(L_TOES_POS, L_ANKLE_POS[0], L_ANKLE_POS[1] + FOOT_LEN, L_ANKLE_POS[2]);

	//build the ragdoll parts
	density = 1.0; //for now

	VectorSet(p1, -CHEST_W * 0.5, 0.0, CHEST_H);
	VectorSet(p2, CHEST_W * 0.5, 0.0, CHEST_H);
	R_addBody(RagDollID, "chest", CHEST, p1, p2, 0.13*RAGDOLLSCALE, density); 

	VectorSet(p1, 0.0, 0.0, CHEST_H - 0.01*RAGDOLLSCALE);
	VectorSet(p2, 0.0, 0.0, HIP_H + 0.01*RAGDOLLSCALE);
	R_addBody(RagDollID, "belly", BELLY, p1, p2, 0.125*RAGDOLLSCALE, density);
	
	VectorSet(p1, 0.0, 0.0, HIP_H);
	R_addBallJoint(RagDollID, MIDSPINE, RagDoll[RagDollID].RagDollObject[CHEST].body, RagDoll[RagDollID].RagDollObject[BELLY].body, p1);

	VectorSet(p1, -(PELVIS_W * 0.5 + 0.01*RAGDOLLSCALE), 0.0, HIP_H);
	VectorSet(p2, PELVIS_W * 0.5 + 0.01*RAGDOLLSCALE, 0.0, HIP_H);
	R_addBody(RagDollID, "pelvis", PELVIS, p1, p2, 0.125*RAGDOLLSCALE, density);

	R_addFixedJoint(RagDollID, LOWSPINE, RagDoll[RagDollID].RagDollObject[BELLY].body, RagDoll[RagDollID].RagDollObject[PELVIS].body);

	VectorSet(p1, 0.0, 0.0, HEAD_H);
	VectorSet(p2, 0.0, 0.0, NECK_H);
	R_addBody(RagDollID, "head", HEAD, p1, p2, 0.11*RAGDOLLSCALE, density);

	VectorSet(p1, 0.0, 0.0, NECK_H);
	R_addBallJoint(RagDollID, NECK, RagDoll[RagDollID].RagDollObject[CHEST].body, RagDoll[RagDollID].RagDollObject[HEAD].body, p1);
	
	//right leg
	VectorSet(p1, R_HIP_POS[0] - 0.01*RAGDOLLSCALE, R_HIP_POS[1], R_HIP_POS[2]);
	VectorSet(p2, R_KNEE_POS[0], R_KNEE_POS[1], R_KNEE_POS[2] + 0.01*RAGDOLLSCALE);
	R_addBody(RagDollID, "rightupperleg", RIGHTUPPERLEG, p1, p2, 0.11*RAGDOLLSCALE, density);

	R_addBallJoint(RagDollID, RIGHTHIP, RagDoll[RagDollID].RagDollObject[PELVIS].body, RagDoll[RagDollID].RagDollObject[RIGHTUPPERLEG].body,
		R_HIP_POS);

	VectorSet(p1, R_KNEE_POS[0], R_KNEE_POS[1], R_KNEE_POS[2] - 0.01*RAGDOLLSCALE);
	VectorSet(p2, R_ANKLE_POS[0], R_ANKLE_POS[1], R_ANKLE_POS[2]);
	R_addBody(RagDollID, "rightlowerleg", RIGHTLOWERLEG, p1, p2, 0.09*RAGDOLLSCALE, density);

	R_addHingeJoint(RagDollID, RIGHTKNEE, RagDoll[RagDollID].RagDollObject[RIGHTUPPERLEG].body, 
		RagDoll[RagDollID].RagDollObject[RIGHTLOWERLEG].body, R_KNEE_POS, leftAxis, 0.0, M_PI * 0.75);

	R_addBody(RagDollID, "rightfoot", RIGHTFOOT, R_TOES_POS, R_HEEL_POS, 0.09*RAGDOLLSCALE, density);

	R_addHingeJoint(RagDollID, RIGHTANKLE, RagDoll[RagDollID].RagDollObject[RIGHTLOWERLEG].body, 
		RagDoll[RagDollID].RagDollObject[RIGHTFOOT].body, R_ANKLE_POS, rightAxis, M_PI * -0.1, M_PI * 0.05);

	//left leg
	VectorSet(p1, L_HIP_POS[0] + 0.01*RAGDOLLSCALE, L_HIP_POS[1], L_HIP_POS[2]);
	VectorSet(p2, L_KNEE_POS[0], L_KNEE_POS[1], L_KNEE_POS[2] + 0.01*RAGDOLLSCALE);
	R_addBody(RagDollID, "leftupperleg", LEFTUPPERLEG, p1, p2, 0.11*RAGDOLLSCALE, density);

	R_addBallJoint(RagDollID, LEFTHIP, RagDoll[RagDollID].RagDollObject[PELVIS].body, RagDoll[RagDollID].RagDollObject[LEFTUPPERLEG].body,
		L_HIP_POS);	

	VectorSet(p1, L_KNEE_POS[0], L_KNEE_POS[1], L_KNEE_POS[2] - 0.01*RAGDOLLSCALE);
	VectorSet(p2, L_ANKLE_POS[0], L_ANKLE_POS[1], L_ANKLE_POS[2]);
	R_addBody(RagDollID, "leftlowerleg", LEFTLOWERLEG, p1, p2, 0.09*RAGDOLLSCALE, density);

	R_addHingeJoint(RagDollID, LEFTKNEE, RagDoll[RagDollID].RagDollObject[LEFTUPPERLEG].body, 
		RagDoll[RagDollID].RagDollObject[LEFTLOWERLEG].body, L_KNEE_POS, leftAxis, 0.0, M_PI * 0.75);
	
	R_addBody(RagDollID, "leftfoot", LEFTFOOT, L_TOES_POS, L_HEEL_POS, 0.09*RAGDOLLSCALE, density);

	R_addHingeJoint(RagDollID, LEFTANKLE, RagDoll[RagDollID].RagDollObject[LEFTLOWERLEG].body, 
		RagDoll[RagDollID].RagDollObject[LEFTFOOT].body, L_ANKLE_POS, rightAxis, M_PI * -0.1, M_PI * 0.05);

	//right arm
	VectorSet(p1, R_SHOULDER_POS[0] - 0.01*RAGDOLLSCALE, R_SHOULDER_POS[1], R_SHOULDER_POS[2]);
	VectorSet(p2, R_ELBOW_POS[0], R_ELBOW_POS[1], R_ELBOW_POS[2] + 0.01*RAGDOLLSCALE);
	R_addBody(RagDollID, "rightupperarm", RIGHTUPPERARM, p1, p2, 0.08*RAGDOLLSCALE, density);

	R_addBallJoint(RagDollID, RIGHTSHOULDER, RagDoll[RagDollID].RagDollObject[CHEST].body, 
		RagDoll[RagDollID].RagDollObject[RIGHTUPPERARM].body, R_SHOULDER_POS);

	VectorSet(p1, R_ELBOW_POS[0], R_ELBOW_POS[1], R_ELBOW_POS[2] - 0.01*RAGDOLLSCALE);
	VectorSet(p2, R_WRIST_POS[0], R_WRIST_POS[1], R_WRIST_POS[2] + 0.01*RAGDOLLSCALE);
	R_addBody(RagDollID, "rightforearm", RIGHTFOREARM, p1, p2, 0.08*RAGDOLLSCALE, density);

	R_addHingeJoint(RagDollID, RIGHTELBOW, RagDoll[RagDollID].RagDollObject[RIGHTUPPERARM].body, 
		RagDoll[RagDollID].RagDollObject[RIGHTFOREARM].body, R_ELBOW_POS, downAxis, 0.0, M_PI * 0.6);	

	VectorSet(p1, R_WRIST_POS[0], R_WRIST_POS[1], R_WRIST_POS[2] - 0.01*RAGDOLLSCALE);
	VectorSet(p2, R_FINGERS_POS[0], R_FINGERS_POS[1], R_FINGERS_POS[2]);
	R_addBody(RagDollID, "righthand", RIGHTHAND, p1, p2, 0.075*RAGDOLLSCALE, density);

	R_addHingeJoint(RagDollID, RIGHTWRIST, RagDoll[RagDollID].RagDollObject[RIGHTFOREARM].body, 
		RagDoll[RagDollID].RagDollObject[RIGHTHAND].body, R_WRIST_POS, fwdAxis, M_PI * -0.1, M_PI * 0.2);

	//left arm
	VectorSet(p1, L_SHOULDER_POS[0] + 0.01*RAGDOLLSCALE, L_SHOULDER_POS[1], L_SHOULDER_POS[2]);
	VectorSet(p2, L_ELBOW_POS[0], L_ELBOW_POS[1], L_ELBOW_POS[2] + 0.01*RAGDOLLSCALE);
	R_addBody(RagDollID, "leftupperarm", LEFTUPPERARM, p1, p2, 0.08*RAGDOLLSCALE, density);

	R_addBallJoint(RagDollID, LEFTSHOULDER, RagDoll[RagDollID].RagDollObject[CHEST].body, 
		RagDoll[RagDollID].RagDollObject[LEFTUPPERARM].body, L_SHOULDER_POS);
	
	VectorSet(p1, L_ELBOW_POS[0], L_ELBOW_POS[1], L_ELBOW_POS[2] - 0.01*RAGDOLLSCALE);
	VectorSet(p2, L_WRIST_POS[0], L_WRIST_POS[1], L_WRIST_POS[2] + 0.01*RAGDOLLSCALE);
	R_addBody(RagDollID, "leftforearm", LEFTFOREARM, p1, p2, 0.08*RAGDOLLSCALE, density);

	R_addHingeJoint(RagDollID, LEFTELBOW, RagDoll[RagDollID].RagDollObject[LEFTUPPERARM].body, 
		RagDoll[RagDollID].RagDollObject[LEFTFOREARM].body, L_ELBOW_POS,  upAxis, 0.0, M_PI * 0.6);
		
	VectorSet(p1, L_WRIST_POS[0], L_WRIST_POS[1], L_WRIST_POS[2] - 0.01*RAGDOLLSCALE);
	VectorSet(p2, L_FINGERS_POS[0], L_FINGERS_POS[1], L_FINGERS_POS[2]);
	R_addBody(RagDollID, "leftthand", LEFTHAND, p1, p2, 0.075*RAGDOLLSCALE, density);

	R_addHingeJoint(RagDollID, LEFTWRIST, RagDoll[RagDollID].RagDollObject[LEFTFOREARM].body, 
		RagDoll[RagDollID].RagDollObject[LEFTHAND].body, L_WRIST_POS, bkwdAxis, M_PI * -0.1, M_PI * 0.2);
	
	//to do - get bone rotations from first death frame, apply to ragdoll for initial position
	//we will need to set the velocity based on origin vs old_origin
}

void R_DrawMark (vec3_t origin, int type)
{

	int		i;
	float scale;

	if(type)
		scale = 0.25;
	else
		scale = 1.0;

    qglPushMatrix ();
	qglTranslatef (origin[0],  origin[1],  origin[2]);

	qglDisable (GL_TEXTURE_2D);
	if(type == 1)
		qglColor3f (1,0,0);
	else if(type == 2)
		qglColor3f (0,1,0);
	else if(type == 3)
		qglColor3f (0,0,1);
	else
		qglColor3f(1,1,1);

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, -scale*16);
	for (i=0 ; i<=4 ; i++)
		qglVertex3f (scale*16*cos(i*M_PI/2), scale*16*sin(i*M_PI/2), 0);
	qglEnd ();

	qglBegin (GL_TRIANGLE_FAN);
	qglVertex3f (0, 0, scale*16);
	for (i=4 ; i>=0 ; i--)
		qglVertex3f (scale*16*cos(i*M_PI/2), scale*16*sin(i*M_PI/2), 0);
	qglEnd ();

	qglColor3f (1,1,1);
	qglPopMatrix ();
	qglEnable (GL_TEXTURE_2D);
}
static vec3_t ragdollorg;			// relative to viewpoint
void R_RecursiveODEWorldNode (mnode_t *node, int clipflags, int RagDollID)
{
	int			c, side, sidebit;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	float		dot;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid
	if (node->visframe != r_visframecount)
		return;
	if (!r_nocull->value)
	{
		int i, clipped;
		cplane_t *clipplane;

		for (i=0,clipplane=frustum ; i<4 ; i++,clipplane++)
		{
			clipped = BoxOnPlaneSide (node->minmaxs, node->minmaxs+3, clipplane);

			if (clipped == 1)
				clipflags &= ~(1<<i);	// node is entirely on screen
			else if (clipped == 2)
				return;					// fully clipped
		}
	}

	// if a leaf node, draw stuff
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

	switch (plane->type)
	{
	case PLANE_X:
		dot = ragdollorg[0] - plane->dist;
		break;
	case PLANE_Y:
		dot = ragdollorg[1] - plane->dist;
		break;
	case PLANE_Z:
		dot = ragdollorg[2] - plane->dist;
		break;
	default:
		dot = DotProduct (ragdollorg, plane->normal) - plane->dist;
		break;
	}

	if (dot >= 0)
	{
		side = 0;
		sidebit = 0;
	}
	else
	{
		side = 1;
		sidebit = SURF_PLANEBACK;
	}

	// recurse down the children, front side first
	R_RecursiveODEWorldNode (node->children[side], clipflags, RagDollID);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if ( (surf->flags & SURF_PLANEBACK) != sidebit )
			continue;		// wrong side

		if (surf->texinfo->flags & SURF_SKY)
		{	// no skies here
			continue;
		}
		else
		{
			if (!( surf->flags & SURF_DRAWTURB ) )
				GL_BuildODEGeoms(surf, RagDollID);
		}
	}

	// recurse down the back side
	R_RecursiveODEWorldNode (node->children[!side], clipflags, RagDollID);
}


/*
=============
R_DrawWorld
=============
*/
void R_BuildRagdollSurfaces (vec3_t origin, int RagDollID)
{
	r_SurfaceCount = 0;

	currentmodel = r_worldmodel;

	VectorCopy (origin, ragdollorg);

	R_RecursiveODEWorldNode (r_worldmodel->nodes, 15, RagDollID);

	RagDoll[RagDollID].numsurfaces = r_SurfaceCount;
}

/*
	Callback function for the collide() method.

	This function checks if the given geoms do collide and creates contact
	joints if they do.
*/

//note - since bsp geometries don't have bodies, we need to check on some demos to make sure we are getting proper
//collision information returned in those cases of ragdoll body colliding with bsp geometries.
static void near_callback(void *data, dGeomID geom1, dGeomID geom2)
{
	dContact contact[MAX_CONTACTS];
	int i, numc;
	dJointID j;
	dBodyID body1 = dGeomGetBody(geom1);
	dBodyID body2 = dGeomGetBody(geom2);

	if (dGeomIsSpace(geom1) || dGeomIsSpace(geom2))
	{   // colliding a space with something
		dSpaceCollide2(geom1, geom2, data, &near_callback);
		
		// now colliding all geoms internal to the space(s)
		if (dGeomIsSpace(geom1))
		{
			dSpaceID o1_spaceID = (dSpaceID)geom1; 
			dSpaceCollide(o1_spaceID, data, &near_callback);
		}
		if (dGeomIsSpace(geom2))
		{
			dSpaceID o2_spaceID = (dSpaceID)geom2; 
			dSpaceCollide(o2_spaceID, data, &near_callback);
		}
	}
	else 
	{
		if(body1 && body2)
		{
			if (dAreConnected(body1, body2))
				return;
		}
		else
			return;

		for(i = 0; i < MAX_CONTACTS; i++)
		{
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
				if(body1 && body2)
					dJointAttach(j, body1, body2);
			}
		}
	}
}

void R_DestroyRagDoll(int RagDollID, qboolean nuke)
{
	int i;

	VectorSet(RagDoll[RagDollID].origin, 0, 0, 0);

	//clear any allocated mem
	if(RagDoll[RagDollID].ragDollMesh)
		free(RagDoll[RagDollID].ragDollMesh);	

	if(RagDoll[RagDollID].script)
		free(RagDoll[RagDollID].script);

	if(!nuke)
		return; 

	//destroy surfaces - to do: faster to track actual num of surfaces instead of max_surfaces, so remember to fix!
	for(i = 0; i < RagDoll[RagDollID].numsurfaces; i++)
	{
		if(RagDoll[RagDollID].WorldGeometry[i])
			dGeomDestroy(RagDoll[RagDollID].WorldGeometry[i]);
		RagDoll[RagDollID].WorldGeometry[i] = NULL;
	}
	RagDoll[RagDollID].numsurfaces = 0;
	
	//we also want to destroy all ragdoll bodies and joints for this ragdoll
	for(i = CHEST; i <= LEFTHAND; i++)
	{
		if(RagDoll[RagDollID].RagDollObject[i].geom)
			dGeomDestroy(RagDoll[RagDollID].RagDollObject[i].geom);
		RagDoll[RagDollID].RagDollObject[i].geom = NULL;
		if(RagDoll[RagDollID].RagDollObject[i].body)
			dBodyDestroy(RagDoll[RagDollID].RagDollObject[i].body);
		RagDoll[RagDollID].RagDollObject[i].body = NULL;
	}

	for(i = MIDSPINE; i <= LEFTWRIST; i++) 
	{
		if(RagDoll[RagDollID].RagDollJoint[i])
			dJointDestroy(RagDoll[RagDollID].RagDollJoint[i]);
		RagDoll[RagDollID].RagDollJoint[i] = NULL;
	}
}

//This is called on every map load
void R_ClearAllRagdolls( void )
{
	int RagDollID;

	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		R_DestroyRagDoll(RagDollID, true);
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
		if(VectorLength(dist) < 32)
			break; //likely spawned from same ent, this may need tweaking or better tracking method, for now it'll do
		
		if(RagDoll[RagDollID].destroyed)
		{
			//need to create geometry in the vicinity of the ragdoll(not the entire map, this is painfully slow)
			//for initial testing, let's just build a simple plane at the feet of the player
			R_RagdollBody_Init(RagDollID, origin);
			//add nearby surfaces anytime a ragdoll is spawned
			R_BuildRagdollSurfaces (RagDoll[RagDollID].origin, RagDollID);
			Com_Printf("Added a ragdoll @ %4.2f,%4.2f,%4.2f\n", RagDoll[RagDollID].origin[0], RagDoll[RagDollID].origin[1], 
				RagDoll[RagDollID].origin[2]);
			break;
		}
	}
}

//Ragdoll rendering routines
static qboolean R_CullRagDolls( int RagDollID )
{
	int i;
	vec3_t	vectors[3];
	vec3_t  angles;
	trace_t r_trace;
	vec3_t	dist;
	vec3_t bbox[8];

	if (r_worldmodel )
	{
		//occulusion culling - why draw entities we cannot see?

		r_trace = CM_BoxTrace(r_origin, RagDoll[RagDollID].curPos, currentmodel->maxs, currentmodel->mins, r_worldmodel->firstnode, MASK_OPAQUE);
		if(r_trace.fraction != 1.0)
			return true;
	}

	VectorSubtract(r_origin, RagDoll[RagDollID].curPos, dist);

	/*
	** rotate the bounding box
	*/
	VectorCopy( RagDoll[RagDollID].angles, angles );
	angles[YAW] = -angles[YAW];
	AngleVectors( angles, vectors[0], vectors[1], vectors[2] );

	for ( i = 0; i < 8; i++ )
	{
		vec3_t tmp;

		VectorCopy( RagDoll[RagDollID].ragDollMesh->bbox[i], tmp );

		bbox[i][0] = DotProduct( vectors[0], tmp );
		bbox[i][1] = -DotProduct( vectors[1], tmp );
		bbox[i][2] = DotProduct( vectors[2], tmp );

		VectorAdd( RagDoll[RagDollID].curPos, bbox[i], bbox[i] );
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

void R_RenderAllRagdolls ( void )
{
	int RagDollID;
	int numRagDolls = 0;
	int i;

	if(!r_ragdolls->value)
		return;

	//Iterate though the ragdoll stack, and render each one that is active.

	//This function would look very similar to the iqm/alias entity routine, but will call a
	//different animation function.  This function will keep track of the time as well, and
	//remove any expired ragdolls off of the stack.

	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		if(Sys_Milliseconds() - RagDoll[RagDollID].spawnTime > RAGDOLL_DURATION)
		{
			//if expired, and not destroyed, destroy it
			if(!RagDoll[RagDollID].destroyed)
			{				
				R_DestroyRagDoll(RagDollID, true);
				RagDoll[RagDollID].destroyed = true;
				Com_Printf("Destroyed a ragdoll");
			}
			continue;
		}
		else 
		{
			//we handle the ragdoll's physics, then render the mesh with skeleton adjusted by ragdoll
			//body object positions
			//Note - I am not sure yet how we should handle helmets for martians.  My gut tells me that 
			//we are going to have to have the helmet share the ragdoll of it's host body, but that is tricky.
			//One one hand, if a mesh ragdoll is translucent and spawned in close proximity to a solid one, 
			//maybe we can somehow assign that mesh to use the solid mesh's ragdoll.  

			//render the meshes
			qglShadeModel (GL_SMOOTH);
			GL_TexEnv( GL_MODULATE );

			R_LightPoint (RagDoll[RagDollID].origin, shadelight, true);

			qglPushMatrix ();

			//GL_AnimateIQMRagdoll(RagDollID);

			//GL_DrawIQMRagDollFrame(RagDollID, RagDoll[RagDollID].texnum);

			GL_TexEnv( GL_REPLACE );
			qglShadeModel (GL_FLAT);

			qglPopMatrix ();
			qglColor4f (1,1,1,1);

			//debug - mark surface centers
			for(i = 0; i < RagDoll[RagDollID].numsurfaces; i++)
			{
				R_DrawMark(RagDoll[RagDollID].surforigins[i], 0);
			}
			for(i = CHEST; i <= LEFTHAND; i++)
			{
				vec3_t org;
				const dReal *odePos;

				if(!RagDoll[RagDollID].RagDollObject[i].body)
					continue;

				odePos = dBodyGetPosition (RagDoll[RagDollID].RagDollObject[i].body);
				VectorSet(org, odePos[0], odePos[1], odePos[2]);
				if(i == HEAD)
					R_DrawMark(org, 2);
				else if (i > LEFTFOOT)
					R_DrawMark(org, 3);
				else
					R_DrawMark(org, 1); 
			}
			
			numRagDolls++;
		}
	}

	if(!numRagDolls)
		r_DrawingRagDoll = false; //no sense in coming back in here until we add a ragdoll

	if(r_DrawingRagDoll) //here we handle the physics
	{		
		if((Sys_Milliseconds() - lastODEUpdate) > 1) //this may not be needed but it's safe to use to throttle things if needed
		{
			dSpaceCollide(RagDollSpace, 0, &near_callback);

			//20 can be adjusted for smoothness vs speed
			dWorldStepFast1(RagDollWorld, (Sys_Milliseconds() - lastODEUpdate)/1000.0f, 20);

			// Remove all temporary collision joints now that the world has been stepped
			dJointGroupEmpty(contactGroup);

			lastODEUpdate = Sys_Milliseconds();
		}
	}
}
