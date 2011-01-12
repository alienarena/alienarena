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
cvar_t *r_ragdoll_debug;

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

extern void Matrix3x4ForEntity(matrix3x4_t *out, entity_t *ent);
extern void Matrix3x4_Invert(matrix3x4_t *out, matrix3x4_t in);
extern void Matrix3x4_Multiply(matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2);
extern void Matrix3x4_Add(matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2);
extern void Matrix3x4_Scale(matrix3x4_t *out, matrix3x4_t in, float scale);

//routine to create ragdoll body parts between two joints
void R_addBody(int RagDollID, matrix3x4_t *bindmat, char *name, int objectID, vec3_t p1, vec3_t p2, float radius, float density)
{
	//Adds a capsule body between joint positions p1 and p2 and with given
	//radius to the ragdoll.
	int i, nans;
	float length;
	vec3_t xa, ya, za, temp;
	matrix3x4_t initmat;
	dMatrix3 rot;

	p1[1] -= RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[GLOBAL_Y_OFF];
	p1[2] += RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[GLOBAL_Z_OFF];
	p2[1] -= RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[GLOBAL_Y_OFF];
	p2[2] += RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[GLOBAL_Z_OFF];

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

	Vector4Set(initmat.a, xa[0], ya[0], za[0], 0.5*(p1[0] + p2[0]));
	Vector4Set(initmat.b, xa[1], ya[1], za[1], 0.5*(p1[1] + p2[1]));
	Vector4Set(initmat.c, xa[2], ya[2], za[2], 0.5*(p1[2] + p2[2]));

	nans = 0;
	for(i = 0; i < 4; i++)
	{
		if(IS_NAN(initmat.a[i])) { initmat.a[i] = 0; nans++; }
		if(IS_NAN(initmat.b[i])) { initmat.b[i] = 0; nans++; }
		if(IS_NAN(initmat.c[i])) { initmat.c[i] = 0; nans++; }
	}
    if(nans > 0)
    {
        if(r_ragdoll_debug->value)
            Com_Printf("There was a NaN in creating body %i\n", objectID);
    }

	Matrix3x4_Invert(&RagDoll[RagDollID].RagDollObject[objectID].initmat, initmat);

	Matrix3x4_Multiply(&initmat, bindmat[objectID], initmat);

	Vector4Set(&rot[0], initmat.a[0], initmat.a[1], initmat.a[2], 0);
    Vector4Set(&rot[4], initmat.b[0], initmat.b[1], initmat.b[2], 0);
	Vector4Set(&rot[8], initmat.c[0], initmat.c[1], initmat.c[2], 0);

	dBodySetPosition(RagDoll[RagDollID].RagDollObject[objectID].body, initmat.a[3], initmat.b[3], initmat.c[3]);
	dBodySetRotation(RagDoll[RagDollID].RagDollObject[objectID].body, rot);
	dBodySetForce(RagDoll[RagDollID].RagDollObject[objectID].body, 0, 0, 0);
	dBodySetLinearVel(RagDoll[RagDollID].RagDollObject[objectID].body, 0, 40, 150); //a little initial upward velocity
	dBodySetAngularVel(RagDoll[RagDollID].RagDollObject[objectID].body, 0, 0, 0);

}

//joint creation routines
void R_addFixedJoint(int RagDollID, matrix3x4_t *bindmat, int jointID, int object1, int object2)
{
    dBodyID body1 = RagDoll[RagDollID].RagDollObject[object1].body;
    dBodyID body2 = RagDoll[RagDollID].RagDollObject[object2].body;

	RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateFixed(RagDollWorld, 0);
	dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);
	dJointSetFixed(RagDoll[RagDollID].RagDollJoint[jointID]);
}

void R_addHingeJoint(int RagDollID, matrix3x4_t *bindmat, int jointID, int object1, int object2, vec3_t anchor, vec3_t axis, float loStop, float hiStop)
{
	dBodyID body1 = RagDoll[RagDollID].RagDollObject[object1].body;
	dBodyID body2 = RagDoll[RagDollID].RagDollObject[object2].body;
	vec3_t wanchor, waxis;

	anchor[1] -= RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[GLOBAL_Y_OFF];
	anchor[2] += RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[GLOBAL_Z_OFF];

	wanchor[0] = 0.5*(DotProduct(bindmat[object1].a, anchor) + bindmat[object1].a[3] + DotProduct(bindmat[object2].a, anchor) + bindmat[object2].a[3]);
    wanchor[1] = 0.5*(DotProduct(bindmat[object1].b, anchor) + bindmat[object1].b[3] + DotProduct(bindmat[object2].b, anchor) + bindmat[object2].b[3]);
    wanchor[2] = 0.5*(DotProduct(bindmat[object1].c, anchor) + bindmat[object1].c[3] + DotProduct(bindmat[object2].c, anchor) + bindmat[object2].c[3]);
    waxis[0] = DotProduct(bindmat[object1].a, axis) + DotProduct(bindmat[object2].a, axis);
    waxis[1] = DotProduct(bindmat[object1].b, axis) + DotProduct(bindmat[object2].b, axis);
    waxis[2] = DotProduct(bindmat[object1].c, axis) + DotProduct(bindmat[object2].c, axis);
	VectorNormalize(waxis);

	VectorAdd(anchor, RagDoll[RagDollID].origin, anchor);

	RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateHinge(RagDollWorld, 0);

	dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);

	dJointSetHingeAnchor(RagDoll[RagDollID].RagDollJoint[jointID], wanchor[0], wanchor[1], wanchor[2]);
    dJointSetHingeAxis(RagDoll[RagDollID].RagDollJoint[jointID], waxis[0], waxis[1], waxis[2]);
    dJointSetHingeParam(RagDoll[RagDollID].RagDollJoint[jointID], dParamLoStop, loStop);
    dJointSetHingeParam(RagDoll[RagDollID].RagDollJoint[jointID], dParamHiStop,  hiStop);
}

void R_addBallJoint(int RagDollID, matrix3x4_t *bindmat, int jointID, int object1, int object2, vec3_t anchor)
{
    dBodyID body1 = RagDoll[RagDollID].RagDollObject[object1].body;
    dBodyID body2 = RagDoll[RagDollID].RagDollObject[object2].body;
    vec3_t wanchor;

	anchor[1] -= RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[GLOBAL_Y_OFF];
	anchor[2] += RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[GLOBAL_Z_OFF];

    wanchor[0] = 0.5*(DotProduct(bindmat[object1].a, anchor) + bindmat[object1].a[3] + DotProduct(bindmat[object2].a, anchor) + bindmat[object2].a[3]);
    wanchor[1] = 0.5*(DotProduct(bindmat[object1].b, anchor) + bindmat[object1].b[3] + DotProduct(bindmat[object2].b, anchor) + bindmat[object2].b[3]);
    wanchor[2] = 0.5*(DotProduct(bindmat[object1].c, anchor) + bindmat[object1].c[3] + DotProduct(bindmat[object2].c, anchor) + bindmat[object2].c[3]);

	RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateBall(RagDollWorld, 0);

	dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);

	dJointSetBallAnchor(RagDoll[RagDollID].RagDollJoint[jointID], wanchor[0], wanchor[1], wanchor[2]);
}

void R_addUniversalJoint(int RagDollID, matrix3x4_t *bindmat, int jointID, int object1, int object2, vec3_t anchor, vec3_t axis1, vec3_t axis2,
	float loStop1, float hiStop1, float loStop2, float hiStop2)
{
	vec3_t wanchor, waxis1, waxis2;

	dBodyID body1 = RagDoll[RagDollID].RagDollObject[object1].body;
    dBodyID body2 = RagDoll[RagDollID].RagDollObject[object2].body;

	anchor[1] -= RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[GLOBAL_Y_OFF];
	anchor[2] += RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[GLOBAL_Z_OFF];

    wanchor[0] = 0.5*(DotProduct(bindmat[object1].a, anchor) + bindmat[object1].a[3] + DotProduct(bindmat[object2].a, anchor) + bindmat[object2].a[3]);
    wanchor[1] = 0.5*(DotProduct(bindmat[object1].b, anchor) + bindmat[object1].b[3] + DotProduct(bindmat[object2].b, anchor) + bindmat[object2].b[3]);
    wanchor[2] = 0.5*(DotProduct(bindmat[object1].c, anchor) + bindmat[object1].c[3] + DotProduct(bindmat[object2].c, anchor) + bindmat[object2].c[3]);

	waxis1[0] = DotProduct(bindmat[object1].a, axis1) + DotProduct(bindmat[object2].a, axis1);
    waxis1[1] = DotProduct(bindmat[object1].b, axis1) + DotProduct(bindmat[object2].b, axis1);
    waxis1[2] = DotProduct(bindmat[object1].c, axis1) + DotProduct(bindmat[object2].c, axis1);
	VectorNormalize(waxis1);

	waxis2[0] = DotProduct(bindmat[object1].a, axis2) + DotProduct(bindmat[object2].a, axis2);
    waxis2[1] = DotProduct(bindmat[object1].b, axis2) + DotProduct(bindmat[object2].b, axis2);
    waxis2[2] = DotProduct(bindmat[object1].c, axis2) + DotProduct(bindmat[object2].c, axis2);
	VectorNormalize(waxis2);

	RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateUniversal(RagDollWorld, 0);

	dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);

	dJointSetUniversalAnchor(RagDoll[RagDollID].RagDollJoint[jointID], wanchor[0], wanchor[1], wanchor[2]);
	dJointSetUniversalAxis1(RagDoll[RagDollID].RagDollJoint[jointID], waxis1[0], waxis1[1], waxis1[2]);
	dJointSetUniversalAxis2(RagDoll[RagDollID].RagDollJoint[jointID], waxis2[0], waxis2[1], waxis2[2]);
	dJointSetUniversalParam(RagDoll[RagDollID].RagDollJoint[jointID], dParamHiStop1,  hiStop1);
	dJointSetUniversalParam(RagDoll[RagDollID].RagDollJoint[jointID], dParamLoStop1,  loStop1);
	dJointSetUniversalParam(RagDoll[RagDollID].RagDollJoint[jointID], dParamHiStop2,  hiStop2);
	dJointSetUniversalParam(RagDoll[RagDollID].RagDollJoint[jointID], dParamLoStop2,  loStop2);

}

void R_CreateWorldObject( void )
{
	// Initialize the world
	RagDollWorld = dWorldCreate();

	dWorldSetGravity(RagDollWorld, 0.0, 0.0, -512.0);

	RagDollSpace = dSimpleSpaceCreate(0);

	contactGroup = dJointGroupCreate(0);

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
	{
		dWorldDestroy(RagDollWorld);
		RagDollWorld = NULL;
	}

	if(RagDollSpace)
	{
		dSpaceDestroy (RagDollSpace);
		RagDollSpace = NULL;
	}

	if(contactGroup)
	{
		dJointGroupDestroy(contactGroup);
		contactGroup = NULL;
	}
}

RagDollBind_t RagDollBinds[] =
{
	{ "hip.l", PELVIS }, { "hip.r", PELVIS },
    { "Spine", CHEST }, { "Spine.001", CHEST },
    { "Head", HEAD },
    { "thigh.r", RIGHTUPPERLEG },
    { "thigh.l", LEFTUPPERLEG },
    { "shin.r", RIGHTLOWERLEG },
    { "shin.l", LEFTLOWERLEG },
    { "bicep.r", RIGHTUPPERARM },
    { "bicep.l", LEFTUPPERARM },
    { "forearm.r", RIGHTFOREARM },
    { "forearm.l", LEFTFOREARM },
    { "hand01.r", RIGHTHAND },
    { "hand01.l", LEFTHAND },
	{ "hand02.r", RIGHTHAND },
    { "hand02.l", LEFTHAND },
	{ "hand03.r", RIGHTHAND },
    { "hand03.l", LEFTHAND },
	{ "wrist.l", LEFTHAND },
	{ "wrist.r", RIGHTHAND },
    { "foot.r", RIGHTFOOT }, { "toe.r", RIGHTFOOT },
    { "foot.l", LEFTFOOT }, { "toe.l", LEFTFOOT }
};
int RagDollBindsCount = (int)(sizeof(RagDollBinds)/sizeof(RagDollBinds[0]));

//build and set initial position of ragdoll
void R_RagdollBody_Init( int RagDollID, vec3_t origin, char name[MAX_QPATH] )
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

	matrix3x4_t entmat, temp, bindmat[MAX_RAGDOLL_OBJECTS];
 	int bindweight[MAX_RAGDOLL_OBJECTS];

	int i, j;

	//we need some information from the current entity

	strcpy(RagDoll[RagDollID].name, name);

	RagDoll[RagDollID].ragDollMesh = (model_t *)malloc (sizeof(model_t));
	memcpy(RagDoll[RagDollID].ragDollMesh, currententity->model, sizeof(model_t));

    RagDoll[RagDollID].initframe = (matrix3x4_t *)malloc (currententity->model->num_joints*sizeof(matrix3x4_t));
    memcpy(RagDoll[RagDollID].initframe, currententity->model->outframe, currententity->model->num_joints*sizeof(matrix3x4_t));

	if(r_shaders->value && currententity->script)
	{
		RagDoll[RagDollID].script = (rscript_t *)malloc (sizeof(rscript_t));
		memcpy(RagDoll[RagDollID].script, currententity->script, sizeof(rscript_t));

		if(currententity->script->stage)
		{
			RagDoll[RagDollID].script->stage = (rs_stage_t *)malloc ( sizeof(rs_stage_t));
			memcpy(RagDoll[RagDollID].script->stage, currententity->script->stage, sizeof(rs_stage_t));

			if(currententity->script->stage->next)
			{
				RagDoll[RagDollID].script->stage->next = (rs_stage_t *)malloc ( sizeof(rs_stage_t));
				memcpy(RagDoll[RagDollID].script->stage->next, currententity->script->stage->next, sizeof(rs_stage_t));
				RagDoll[RagDollID].script->stage->next->next = NULL;
			}
			else
				RagDoll[RagDollID].script->stage->next = NULL;
		}
		else
			RagDoll[RagDollID].script->stage = NULL;
	}
	else
		RagDoll[RagDollID].script = NULL;

	RagDoll[RagDollID].texnum = currententity->skin->texnum;
	RagDoll[RagDollID].flags = currententity->flags;
	_VectorCopy(currententity->angles, RagDoll[RagDollID].angles);
	VectorCopy(origin, RagDoll[RagDollID].origin);
	VectorCopy(origin, RagDoll[RagDollID].curPos);
	RagDoll[RagDollID].spawnTime = Sys_Milliseconds();
	RagDoll[RagDollID].destroyed = false;

	memset(bindmat, 0, sizeof(bindmat));
	memset(bindweight, 0, sizeof(bindweight));
	for(i = 0; i < RagDoll[RagDollID].ragDollMesh->num_joints; i++)
	{
		for(j = 0; j < RagDollBindsCount; j++)
		{
			if(!strcmp(&RagDoll[RagDollID].ragDollMesh->jointname[RagDoll[RagDollID].ragDollMesh->joints[i].name], RagDollBinds[j].name))
			{
				int object = RagDollBinds[j].object;
				if ( ! IS_NAN( RagDoll[RagDollID].initframe[i].a[0] ) ) {
					Matrix3x4_Add(&bindmat[object], bindmat[object], RagDoll[RagDollID].initframe[i]);
				}
				bindweight[object]++;
				break;
			}
		}
	}
	Matrix3x4ForEntity(&entmat, currententity);
	for(i = 0; i < MAX_RAGDOLL_OBJECTS; i++)
	{
		if(bindweight[i])
		{
			Matrix3x4_Scale(&temp, bindmat[i], 1.0/bindweight[i]);
			VectorNormalize(temp.a);
			VectorNormalize(temp.b);
			VectorNormalize(temp.c);
			Matrix3x4_Multiply(&bindmat[i], entmat, temp);
		}
		else bindmat[i] = entmat;
	}

	//set upper body
	VectorSet(R_SHOULDER_POS, -RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_W] * 0.5, 0.0,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_H]);
	VectorSet(L_SHOULDER_POS, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_W] * 0.5, 0.0,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_H]);

	VectorSet(R_ELBOW_POS, -RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ELBOW_X_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ELBOW_Y_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ELBOW_Z_OFF]);
	VectorSet(L_ELBOW_POS, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ELBOW_X_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ELBOW_Y_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ELBOW_Z_OFF]);

	VectorSet(R_WRIST_POS, -RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[WRIST_X_OFF],
		-RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[WRIST_Y_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[WRIST_Z_OFF]);
	VectorSet(L_WRIST_POS, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[WRIST_X_OFF],
		-RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[WRIST_Y_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[WRIST_Z_OFF]);

	VectorSet(R_FINGERS_POS, -RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FINGERS_X_OFF],
		-RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FINGERS_Y_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FINGERS_Z_OFF]);
	VectorSet(L_FINGERS_POS, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FINGERS_X_OFF],
		-RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FINGERS_Y_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FINGERS_Z_OFF]);

	//set lower body
	VectorSet(R_HIP_POS, -RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[LEG_W] * 0.5,  0.0,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_H]);
	VectorSet(L_HIP_POS, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[LEG_W] * 0.5, 0.0,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_H]);

	VectorSet(R_KNEE_POS, -RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[KNEE_X_OFF],
		-RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[KNEE_Y_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[KNEE_Z_OFF]);
	VectorSet(L_KNEE_POS, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[KNEE_X_OFF],
		-RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[KNEE_Y_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[KNEE_Z_OFF]);

	VectorSet(R_ANKLE_POS, -RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ANKLE_X_OFF],
		-RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ANKLE_Y_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ANKLE_Z_OFF]);
	VectorSet(L_ANKLE_POS, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ANKLE_X_OFF],
		-RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ANKLE_Y_OFF],
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ANKLE_Z_OFF]);

	VectorSet(R_HEEL_POS, R_ANKLE_POS[0], R_ANKLE_POS[1] - RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HEEL_LEN], R_ANKLE_POS[2]);
	VectorSet(L_HEEL_POS, L_ANKLE_POS[0], L_ANKLE_POS[1] - RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HEEL_LEN], L_ANKLE_POS[2]);

	VectorSet(R_TOES_POS, R_ANKLE_POS[0], R_ANKLE_POS[1] + RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FOOT_LEN], R_ANKLE_POS[2]);
	VectorSet(L_TOES_POS, L_ANKLE_POS[0], L_ANKLE_POS[1] + RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FOOT_LEN], L_ANKLE_POS[2]);

	//build the ragdoll parts
	density = 1.0; //for now

	VectorSet(p1, 0.0, 0.0,	RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[NECK_H] - 0.1);
	VectorSet(p2, 0.0, 0.0,	RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_H] + 0.1);
	R_addBody(RagDollID, bindmat, "chest", CHEST, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[CHEST_W]/2.0, density);

	VectorSet(p1, R_HIP_POS[0] + 0.1, R_HIP_POS[1], R_HIP_POS[2] - 0.1);
	VectorSet(p2, L_HIP_POS[0] - 0.1, L_HIP_POS[1], L_HIP_POS[2] - 0.1);
	R_addBody(RagDollID, bindmat, "pelvis", PELVIS, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[PELVIS_W]/2.0, density);

	R_addFixedJoint(RagDollID, bindmat, LOWSPINE, CHEST, PELVIS);

	VectorSet(p1, 0.0, 0.0, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HEAD_H]);
	VectorSet(p2, 0.0, 0.0, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[NECK_H] + 0.1);
	R_addBody(RagDollID, bindmat, "head", HEAD, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HEAD_W]/2.0, density);

	VectorSet(p1, 0.0, 0.0, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[NECK_H]);
	R_addUniversalJoint(RagDollID, bindmat, NECK, CHEST, HEAD, p1, upAxis, rightAxis,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HEAD_LOSTOP1] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HEAD_HISTOP1] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HEAD_LOSTOP2] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HEAD_HISTOP2] * M_PI);

	//right leg
	VectorSet(p1, R_HIP_POS[0] - 0.1, R_HIP_POS[1], R_HIP_POS[2] - 0.1);
	VectorSet(p2, R_KNEE_POS[0], R_KNEE_POS[1], R_KNEE_POS[2] + 0.1);
	R_addBody(RagDollID, bindmat, "rightupperleg", RIGHTUPPERLEG, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[THIGH_W]/2.0, density);

	R_addUniversalJoint(RagDollID, bindmat, RIGHTHIP, PELVIS, RIGHTUPPERLEG, R_HIP_POS,	bkwdAxis, rightAxis,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_LOSTOP1] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_HISTOP1] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_LOSTOP2] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_HISTOP2] * M_PI);

	VectorSet(p1, R_KNEE_POS[0], R_KNEE_POS[1], R_KNEE_POS[2] - 0.1);
	VectorSet(p2, R_ANKLE_POS[0], R_ANKLE_POS[1], R_ANKLE_POS[2]);
	R_addBody(RagDollID, bindmat, "rightlowerleg", RIGHTLOWERLEG, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHIN_W]/2.0, density);

	R_addHingeJoint(RagDollID, bindmat, RIGHTKNEE, RIGHTUPPERLEG,
		RIGHTLOWERLEG, R_KNEE_POS, leftAxis, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[KNEE_LOSTOP] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[KNEE_HISTOP] * M_PI);

	R_addBody(RagDollID, bindmat, "rightfoot", RIGHTFOOT, R_TOES_POS, R_HEEL_POS,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FOOT_W]/2.0, density);

	R_addHingeJoint(RagDollID, bindmat, RIGHTANKLE, RIGHTLOWERLEG,
		RIGHTFOOT, R_ANKLE_POS, rightAxis, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ANKLE_LOSTOP] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ANKLE_HISTOP] * M_PI);

	//left leg
	VectorSet(p1, L_HIP_POS[0] + 0.1, L_HIP_POS[1], L_HIP_POS[2] - 0.1);
	VectorSet(p2, L_KNEE_POS[0], L_KNEE_POS[1], L_KNEE_POS[2] + 0.1);
	R_addBody(RagDollID, bindmat, "leftupperleg", LEFTUPPERLEG, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[THIGH_W]/2.0, density);

	R_addUniversalJoint(RagDollID, bindmat, LEFTHIP, PELVIS,LEFTUPPERLEG, L_HIP_POS, fwdAxis, rightAxis,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_LOSTOP1] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_HISTOP1] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_LOSTOP2] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HIP_HISTOP2] * M_PI);

	VectorSet(p1, L_KNEE_POS[0], L_KNEE_POS[1], L_KNEE_POS[2] - 0.1);
	VectorSet(p2, L_ANKLE_POS[0], L_ANKLE_POS[1], L_ANKLE_POS[2]);
	R_addBody(RagDollID, bindmat, "leftlowerleg", LEFTLOWERLEG, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHIN_W]/2.0, density);

	R_addHingeJoint(RagDollID, bindmat, LEFTKNEE, LEFTUPPERLEG,
		LEFTLOWERLEG, L_KNEE_POS, leftAxis, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[KNEE_LOSTOP] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[KNEE_HISTOP] * M_PI);

	R_addBody(RagDollID, bindmat, "leftfoot", LEFTFOOT, L_TOES_POS, L_HEEL_POS,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FOOT_W]/2.0, density);

	R_addHingeJoint(RagDollID, bindmat, LEFTANKLE, LEFTLOWERLEG,
		LEFTFOOT, L_ANKLE_POS, rightAxis, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ANKLE_LOSTOP] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ANKLE_HISTOP] * M_PI);

	//right arm
	VectorSet(p1, R_SHOULDER_POS[0] - 0.1, R_SHOULDER_POS[1], R_SHOULDER_POS[2]);
	VectorSet(p2, R_ELBOW_POS[0], R_ELBOW_POS[1], R_ELBOW_POS[2] + 0.1);
	R_addBody(RagDollID, bindmat, "rightupperarm", RIGHTUPPERARM, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[BICEP_W]/2.0, density);

	R_addUniversalJoint(RagDollID, bindmat, RIGHTSHOULDER, CHEST, RIGHTUPPERARM, R_SHOULDER_POS, bkwdAxis, rightAxis,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_LOSTOP1] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_HISTOP1] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_LOSTOP2] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_HISTOP2] * M_PI);

	VectorSet(p1, R_ELBOW_POS[0], R_ELBOW_POS[1], R_ELBOW_POS[2] - 0.1);
	VectorSet(p2, R_WRIST_POS[0], R_WRIST_POS[1], R_WRIST_POS[2] + 0.1);
	R_addBody(RagDollID, bindmat, "rightforearm", RIGHTFOREARM, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FOREARM_W]/2.0, density);

	R_addHingeJoint(RagDollID, bindmat, RIGHTELBOW, RIGHTUPPERARM,
		RIGHTFOREARM, R_ELBOW_POS, downAxis, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ELBOW_LOSTOP] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ELBOW_HISTOP] * M_PI);

	VectorSet(p1, R_WRIST_POS[0], R_WRIST_POS[1], R_WRIST_POS[2] - 0.1);
	VectorSet(p2, R_FINGERS_POS[0], R_FINGERS_POS[1], R_FINGERS_POS[2]);
	R_addBody(RagDollID, bindmat, "righthand", RIGHTHAND, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HAND_W]/2.0, density);

	R_addHingeJoint(RagDollID, bindmat, RIGHTWRIST, RIGHTFOREARM,
		RIGHTHAND, R_WRIST_POS, fwdAxis, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[WRIST_LOSTOP] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[WRIST_HISTOP] * M_PI);

	//left arm
	VectorSet(p1, L_SHOULDER_POS[0] + 0.1, L_SHOULDER_POS[1], L_SHOULDER_POS[2]);
	VectorSet(p2, L_ELBOW_POS[0], L_ELBOW_POS[1], L_ELBOW_POS[2] + 0.1);
	R_addBody(RagDollID, bindmat, "leftupperarm", LEFTUPPERARM, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[BICEP_W]/2.0, density);

	R_addUniversalJoint(RagDollID, bindmat, LEFTSHOULDER, CHEST, LEFTUPPERARM, L_SHOULDER_POS, fwdAxis, rightAxis,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_LOSTOP1] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_HISTOP1] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_LOSTOP2] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[SHOULDER_HISTOP2] * M_PI);

	VectorSet(p1, L_ELBOW_POS[0], L_ELBOW_POS[1], L_ELBOW_POS[2] - 0.1);
	VectorSet(p2, L_WRIST_POS[0], L_WRIST_POS[1], L_WRIST_POS[2] + 0.1);
	R_addBody(RagDollID, bindmat, "leftforearm", LEFTFOREARM, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[FOREARM_W]/2.0, density);

	R_addHingeJoint(RagDollID, bindmat, LEFTELBOW, LEFTUPPERARM,
		LEFTFOREARM, L_ELBOW_POS,  upAxis, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ELBOW_LOSTOP] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[ELBOW_HISTOP] * M_PI);

	VectorSet(p1, L_WRIST_POS[0], L_WRIST_POS[1], L_WRIST_POS[2] - 0.1);
	VectorSet(p2, L_FINGERS_POS[0], L_FINGERS_POS[1], L_FINGERS_POS[2]);
	R_addBody(RagDollID, bindmat, "leftthand", LEFTHAND, p1, p2, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[HAND_W]/2.0, density);

	R_addHingeJoint(RagDollID, bindmat, LEFTWRIST, LEFTFOREARM,
		LEFTHAND, L_WRIST_POS, bkwdAxis, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[WRIST_LOSTOP] * M_PI,
		RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[WRIST_HISTOP] * M_PI);
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

//For creating the surfaces for the ragdoll to collide with
void GL_BuildODEGeoms(msurface_t *surf)
{
	glpoly_t *p;
	float	*v;
	int		i;
    int polyStart;
	for ( p = surf->polys; p; p = p->chain )
	{
        if(RagDollTriWorld.numODEVerts + p->numverts > RagDollTriWorld.maxODEVerts)
        {
            int growVerts = RagDollTriWorld.maxODEVerts;
            dVector3 *newVerts;
            while(RagDollTriWorld.numODEVerts + p->numverts > growVerts)
                growVerts += GROW_ODE_VERTS;
            newVerts = (dVector3 *)realloc(RagDollTriWorld.ODEVerts, growVerts*sizeof(dVector3));
            if(!newVerts) break;
            RagDollTriWorld.maxODEVerts = growVerts;
            RagDollTriWorld.ODEVerts = newVerts;
        }

        polyStart = RagDollTriWorld.numODEVerts;

		for (v = p->verts[0]; v < p->verts[p->numverts]; v += VERTEXSIZE)
		{

			RagDollTriWorld.ODEVerts[RagDollTriWorld.numODEVerts][0] = v[0];
			RagDollTriWorld.ODEVerts[RagDollTriWorld.numODEVerts][1] = v[1];
			RagDollTriWorld.ODEVerts[RagDollTriWorld.numODEVerts][2] = v[2];
			RagDollTriWorld.numODEVerts++;
		}

        if(RagDollTriWorld.numODETris + p->numverts-2 > RagDollTriWorld.maxODETris)
        {
            int growTris = RagDollTriWorld.maxODETris;
            dTriIndex *newTris;
            while(RagDollTriWorld.numODETris + p->numverts-2 > growTris)
                growTris += GROW_ODE_TRIS;
            newTris = (dTriIndex *)realloc(RagDollTriWorld.ODETris, growTris*sizeof(dTriIndex[3]));
            if(!newTris) break;
            RagDollTriWorld.maxODETris = growTris;
            RagDollTriWorld.ODETris = newTris;
        }

        for (i = 2; i < p->numverts; i++)
        {
            RagDollTriWorld.ODETris[RagDollTriWorld.numODETris*3+0] = polyStart + i;
            RagDollTriWorld.ODETris[RagDollTriWorld.numODETris*3+1] = polyStart + i - 1;
            RagDollTriWorld.ODETris[RagDollTriWorld.numODETris*3+2] = polyStart;
            RagDollTriWorld.numODETris++;
        }
	}
}

/*
=============
R_DrawWorldTrimesh
=============
*/
void R_BuildWorldTrimesh ( void )
{
    msurface_t *surf;
    dMatrix3 rot;

    RagDollTriWorld.numODEVerts = RagDollTriWorld.numODETris = 0;

    for (surf = &r_worldmodel->surfaces[r_worldmodel->firstmodelsurface]; surf < &r_worldmodel->surfaces[r_worldmodel->firstmodelsurface + r_worldmodel->nummodelsurfaces] ; surf++)
    {
        if (surf->texinfo->flags & SURF_SKY)
        {   // no skies here
            continue;
        }
        else
        {
            if (!( surf->flags & SURF_DRAWTURB ) )
            {
                GL_BuildODEGeoms(surf);
            }
        }
    }

    dRSetIdentity(rot);

    //we need to build the trimesh geometry
    RagDollTriWorld.triMesh = dGeomTriMeshDataCreate();

    // Build the mesh from the data
    dGeomTriMeshDataBuildSimple(RagDollTriWorld.triMesh, (dReal*)RagDollTriWorld.ODEVerts, RagDollTriWorld.numODEVerts,
        RagDollTriWorld.ODETris, RagDollTriWorld.numODETris*3);

    RagDollTriWorld.geom = dCreateTriMesh(RagDollSpace, RagDollTriWorld.triMesh, NULL, NULL, NULL);
    dGeomSetData(RagDollTriWorld.geom, "surface");

    // this geom has no body
    dGeomSetBody(RagDollTriWorld.geom, 0);

    dGeomSetPosition(RagDollTriWorld.geom, 0, 0, 0);
    dGeomSetRotation(RagDollTriWorld.geom, rot);
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

		for(i = 0; i < MAX_CONTACTS; i++)
		{
			contact[i].surface.mode = dContactBounce; // Bouncy surface
			contact[i].surface.bounce = 0.2;
			contact[i].surface.mu = dInfinity; // Friction
			contact[i].surface.mu2 = 0;
			contact[i].surface.bounce_vel = 0.1;
		}

		if (( numc = dCollide(geom1, geom2, MAX_CONTACTS, &contact[0].geom, sizeof(dContact))) != 0)
		{
			// To add each contact point found to our joint group we call dJointCreateContact which is just one of the many
			// different joint types available.
			for (i = 0; i < numc; i++)
			{
				// dJointCreateContact needs to know which world and joint group to work with as well as the dContact
				// object itself. It returns a new dJointID which we then use with dJointAttach to finally create the
				// temporary contact joint between the two geom bodies.
				j = dJointCreateContact(RagDollWorld, contactGroup, contact + i);
				dJointAttach(j, body1, body2);
			}
		}
	}
}

void R_DestroyRagDoll(int RagDollID, qboolean nuke)
{
	int i;

	VectorSet(RagDoll[RagDollID].origin, 0, 0, 0);

	//destroy forces
	for( i = 0; i < MAX_FORCES; i++)
	{
		RagDoll[RagDollID].RagDollForces[i].destroyed = true;
	}

	//clear any allocated mem
	if(RagDoll[RagDollID].ragDollMesh)
	{
		free(RagDoll[RagDollID].ragDollMesh);
		RagDoll[RagDollID].ragDollMesh = NULL;
	}

    if(RagDoll[RagDollID].initframe)
	{
        free(RagDoll[RagDollID].initframe);
		RagDoll[RagDollID].initframe = NULL;
	}

	if(RagDoll[RagDollID].script)
	{
		if(RagDoll[RagDollID].script->stage)
		{
			if(RagDoll[RagDollID].script->stage->next)
			{
				free(RagDoll[RagDollID].script->stage->next);
				RagDoll[RagDollID].script->stage->next = NULL;
			}
			free(RagDoll[RagDollID].script->stage);
			RagDoll[RagDollID].script->stage = NULL;
		}

		free(RagDoll[RagDollID].script);
		RagDoll[RagDollID].script = NULL;
	}

	if(!nuke)
		return;

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

void R_DestroyWorldTrimesh( void )
{
	if(RagDollTriWorld.geom)
		dGeomDestroy(RagDollTriWorld.geom);
	RagDollTriWorld.geom = NULL;
    if(RagDollTriWorld.ODEVerts)
        free(RagDollTriWorld.ODEVerts);
    RagDollTriWorld.ODEVerts = NULL;
    RagDollTriWorld.maxODEVerts = 0;
    if(RagDollTriWorld.ODETris)
        free(RagDollTriWorld.ODETris);
    RagDollTriWorld.ODETris = NULL;
    RagDollTriWorld.maxODETris = 0;
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
	R_DestroyWorldTrimesh();
	r_DrawingRagDoll = false;
}

void R_AddNewRagdoll( vec3_t origin, char name[MAX_QPATH] )
{
	int RagDollID, i;
	vec3_t dist;
	vec3_t dir;

	//check to see if we already have spawned a ragdoll for this entity
	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		if(!RagDoll[RagDollID].destroyed)
		{
			VectorSubtract(origin, RagDoll[RagDollID].origin, dist);
			if(VectorLength(dist) < 64 && !strcmp(RagDoll[RagDollID].name, name))
				return;
		}
	}

	VectorSet(dir, 6, 6, 20); //note this is temporary hardcoded value(we should use the velocity of the ragdoll to influence this)

	r_DrawingRagDoll = true; //we are rendering a Ragdoll somewhere

	//add a ragdoll, look for first open slot
	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		if(RagDoll[RagDollID].destroyed)
		{
			R_RagdollBody_Init(RagDollID, origin, name);

			if(r_ragdoll_debug->value == 2)
			{
				Com_Printf("RagDoll name: %s Ent name: %s Mesh: %s Ent Mesh: %s\n", RagDoll[RagDollID].name, name,
					RagDoll[RagDollID].ragDollMesh->name, currentmodel->name);

				for(i = 0; i < 27; i++)
					Com_Printf("ragdoll val %i: %4.2f\n", i, RagDoll[RagDollID].ragDollMesh->ragdoll.RagDollDims[i]);
			}

			if(r_ragdoll_debug->value)
				Com_Printf("Added a ragdoll @ %4.2f,%4.2f,%4.2f\n", RagDoll[RagDollID].origin[0], RagDoll[RagDollID].origin[1],
					RagDoll[RagDollID].origin[2]);

			//break glass effect for helmets so we don't need to render them
			if(RagDoll[RagDollID].ragDollMesh->ragdoll.hasHelmet)
				CL_GlassShards(RagDoll[RagDollID].origin, dir, 5);
			break;
		}
	}
}

//Ragdoll rendering routines
qboolean R_CullRagDolls( int RagDollID )
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

		r_trace = CM_BoxTrace(r_origin, RagDoll[RagDollID].curPos, RagDoll[RagDollID].ragDollMesh->maxs, RagDoll[RagDollID].ragDollMesh->mins, r_worldmodel->firstnode, MASK_OPAQUE);
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

		if ( aggregatemask && (VectorLength(dist) > 150))
		{
			return true;
		}

		return false;
	}
}

extern qboolean have_stencil;
extern vec3_t lightspot;
void R_DrawRagDollShadow(int RagDollID)
{
	vec3_t	point, point2, mins, maxs;
	float	height;
	int		i, j;
	int		index_xyz, index_st;
	int		va = 0;
	trace_t r_trace;

	//trace to get ground
	VectorSet(mins, 0, 0, 0);
	VectorSet(maxs, 0, 0, 0);

	VectorCopy(RagDoll[RagDollID].curPos, point);
	point[2] += 24;
	VectorCopy(RagDoll[RagDollID].curPos, point2);
	point2[2] -= 1024;

	r_trace = CM_BoxTrace(point, point2, mins, maxs, r_worldmodel->firstnode, MASK_SOLID);

	if(r_trace.fraction == 1.0)
		return;

	height = r_trace.endpos[2] + 0.1f;

	if (r_newrefdef.vieworg[2] < height)
		return;

	if (have_stencil)
	{
		qglDepthMask(0);
		qglEnable(GL_STENCIL_TEST);
		qglStencilFunc(GL_EQUAL,1,2);
		qglStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
	}

	va=0;
	VArray = &VArrayVerts[0];
	R_InitVArrays (VERT_SINGLE_TEXTURED);

	for (i=0; i<RagDoll[RagDollID].ragDollMesh->num_triangles; i++)
    {
        for (j=0; j<3; j++)
        {
            index_xyz = index_st = RagDoll[RagDollID].ragDollMesh->tris[i].vertex[j];

			memcpy( point, RagDoll[RagDollID].ragDollMesh->animatevertexes[index_xyz].position, sizeof( point )  );

			point[0] -= shadevector[0];
			point[1] -= shadevector[1];
			point[2] = height;

			VArray[0] = point[0];
			VArray[1] = point[1];
			VArray[2] = point[2];

			VArray[3] = currentmodel->st[index_st].s;
            VArray[4] = currentmodel->st[index_st].t;

			// increment pointer and counter
			VArray += VertexSizes[VERT_SINGLE_TEXTURED];
			va++;
		}
	}

	qglDrawArrays(GL_TRIANGLES,0,va);

	qglDisableClientState( GL_COLOR_ARRAY );
	qglEnableClientState( GL_TEXTURE_COORD_ARRAY );

	R_KillVArrays ();
	qglDepthMask(1);
	qglColor4f(1,1,1,1);
	if (1)
		qglDisable(GL_STENCIL_TEST);
}

void R_RenderAllRagdolls ( void )
{
	int RagDollID;
	int i;

	if(!r_ragdolls->value)
		return;

	//Iterate though the ragdoll stack, and render each one that is active.

	//This function is very similar to the iqm/alias entity routine, but will call a
	//different animation function.  This function will keep track of the time as well, and
	//remove any expired ragdolls off of the stack.

	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		float dur;

		if(RagDoll[RagDollID].destroyed)
	        continue;

		dur = Sys_Milliseconds() - RagDoll[RagDollID].spawnTime;

		if(dur > RAGDOLL_DURATION)
		{
			R_DestroyRagDoll(RagDollID, true);
            RagDoll[RagDollID].destroyed = true;
            if(r_ragdoll_debug->value)
				Com_Printf("Destroyed a ragdoll");
		}
		else
		{
			//we handle the ragdoll's physics, then render the mesh with skeleton adjusted by ragdoll
			//body object positions
			const	dReal *odePos;
			int		shellEffect = false;
			float	shellAlpha = 1.0f; // quiet warning; is initialized if used.

			odePos = dBodyGetPosition (RagDoll[RagDollID].RagDollObject[CHEST].body);
			VectorSet(RagDoll[RagDollID].curPos, odePos[0], odePos[1], odePos[2]);

			if ( R_CullRagDolls( RagDollID ) )
				continue;

			//render the meshes
			qglShadeModel (GL_SMOOTH);
			GL_TexEnv( GL_MODULATE );

			R_LightPoint (RagDoll[RagDollID].curPos, shadelight, true);

			if(dur > (RAGDOLL_DURATION - 2500))
			{
				VectorClear (shadelight);
				shadelight[1] = 1.0;
				shadelight[2] = 0.6;

				dBodySetLinearVel(RagDoll[RagDollID].RagDollObject[CHEST].body, 0, 0, 50); //lift into space

				shellAlpha = (1 - dur/RAGDOLL_DURATION);
				shellEffect = true;
			}

			GL_AnimateIQMRagdoll(RagDollID);

			GL_DrawIQMRagDollFrame(RagDollID, RagDoll[RagDollID].texnum, shellAlpha, shellEffect);

			GL_TexEnv( GL_REPLACE );
			qglShadeModel (GL_FLAT);

			//simple stencil shadows
			if (gl_shadows->integer && !shellEffect && !gl_shadowmaps->integer)
			{
				float casted;
				float an = RagDoll[RagDollID].angles[1]/180*M_PI;
				shadevector[0] = cos(-an);
				shadevector[1] = sin(-an);
				shadevector[2] = 1;
				VectorNormalize (shadevector);

				currentmodel = RagDoll[RagDollID].ragDollMesh;

				switch (gl_shadows->integer)
				{
					case 0:
						break;
					case 1: //dynamic only - always cast something
						casted = R_ShadowLight (RagDoll[RagDollID].curPos, RagDoll[RagDollID].angles, shadevector, 0);
						qglDisable (GL_TEXTURE_2D);
						qglEnable (GL_BLEND);

						qglColor4f (0,0,0,0.3);

						R_DrawRagDollShadow (RagDollID);

						qglEnable (GL_TEXTURE_2D);
						qglDisable (GL_BLEND);
						break;
					case 2: //dynamic and world
						//world
						casted = R_ShadowLight (RagDoll[RagDollID].curPos, RagDoll[RagDollID].angles, shadevector, 1);

						qglDisable (GL_TEXTURE_2D);
						qglEnable (GL_BLEND);

						qglColor4f (0,0,0,casted);

						R_DrawRagDollShadow (RagDollID);

						qglEnable (GL_TEXTURE_2D);
						qglDisable (GL_BLEND);
						//dynamic
						casted = 0;
						casted = R_ShadowLight (RagDoll[RagDollID].curPos, RagDoll[RagDollID].angles, shadevector, 0);
						if (casted > 0)
						{ //only draw if there's a dynamic light there
							qglDisable (GL_TEXTURE_2D);
							qglEnable (GL_BLEND);

							qglColor4f (0,0,0,casted);

							R_DrawRagDollShadow (RagDollID);

							qglEnable (GL_TEXTURE_2D);
							qglDisable (GL_BLEND);
						}
						break;
				}
			}
			qglColor4f (1,1,1,1);

			//apply forces from explosions
			for(i = 0; i < MAX_FORCES; i++)
			{
				if(!RagDoll[RagDollID].RagDollForces[i].destroyed)
				{
					if(Sys_Milliseconds() - RagDoll[RagDollID].RagDollForces[i].spawnTime < 2000)
					{
						VectorSubtract(RagDoll[RagDollID].RagDollForces[i].org, RagDoll[RagDollID].curPos, RagDoll[RagDollID].RagDollForces[i].dir);
						RagDoll[RagDollID].RagDollForces[i].force /= VectorLength(RagDoll[RagDollID].RagDollForces[i].dir);
						VectorNormalize(RagDoll[RagDollID].RagDollForces[i].dir);
						RagDoll[RagDollID].RagDollForces[i].force *= 500;
						if(RagDoll[RagDollID].RagDollForces[i].force > 10000)
							RagDoll[RagDollID].RagDollForces[i].force = 10000;

						//add a force if it's sufficient enough to do anything
						if(RagDoll[RagDollID].RagDollForces[i].force > 200)
						{
							dBodySetLinearVel(RagDoll[RagDollID].RagDollObject[CHEST].body, -RagDoll[RagDollID].RagDollForces[i].dir[0] *
							RagDoll[RagDollID].RagDollForces[i].force, -RagDoll[RagDollID].RagDollForces[i].dir[1] *
							RagDoll[RagDollID].RagDollForces[i].force, -RagDoll[RagDollID].RagDollForces[i].dir[2] *
							RagDoll[RagDollID].RagDollForces[i].force);
						}
						RagDoll[RagDollID].RagDollForces[i].destroyed = true; //destroy if used
					}
					else
						RagDoll[RagDollID].RagDollForces[i].destroyed = true; //destroy if expired
				}
			}


			if(r_ragdoll_debug->value)
			{
				//debug - draw ragdoll bodies
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
			}

			r_DrawingRagDoll = true;
		}
	}

	if(r_DrawingRagDoll) //here we handle the physics
	{
		int ODEIterationsPerFrame;
		int frametime = Sys_Milliseconds() - lastODEUpdate;

		//iterations need to be adjusted for framerate.
		ODEIterationsPerFrame = frametime;

		//clamp it
		if(ODEIterationsPerFrame > MAX_ODESTEPS)
			ODEIterationsPerFrame = MAX_ODESTEPS;
		if(ODEIterationsPerFrame < MIN_ODESTEPS)
			ODEIterationsPerFrame = MIN_ODESTEPS;

		dSpaceCollide(RagDollSpace, 0, &near_callback);

		dWorldStepFast1(RagDollWorld, (float)(frametime/1000.0f), ODEIterationsPerFrame);

		// Remove all temporary collision joints now that the world has been stepped
		dJointGroupEmpty(contactGroup);
	}
	lastODEUpdate = Sys_Milliseconds();
	r_DrawingRagDoll = false;
}

void R_ApplyForceToRagdolls(vec3_t origin, float force)
{
	int RagDollID, i;

	if(!r_ragdolls->value)
		return;

	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		for(i = 0; i < MAX_FORCES; i++)
		{
			if(RagDoll[RagDollID].RagDollForces[i].destroyed)
			{
				VectorCopy(origin, RagDoll[RagDollID].RagDollForces[i].org);
				RagDoll[RagDollID].RagDollForces[i].force = force;
				RagDoll[RagDollID].RagDollForces[i].spawnTime = Sys_Milliseconds();
				RagDoll[RagDollID].RagDollForces[i].destroyed = false;
				break;
			}
		}
	}
}
