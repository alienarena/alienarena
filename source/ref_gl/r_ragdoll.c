/*
Copyright (C) 2010-2014 COR Entertainment, LLC.

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

//Once a model's frames are at the death stage, the ragdoll takes over.

//The world objects are created each time a map is loaded.

//We get rotation and location vectors from the
//ragdoll, and apply them to the models skeleton. 

//Ragdolls are batched and drawn just as regular entities, which also means that entities should not be drawn in their normal
//batch once in a death animation.  When death animations begin, a ragdoll is be generated with the appropriate properties including
//position, angles, etc.  The ragdoll is timestamped, and is removed from the stack after a certain amount of time has
//elapsed.  We also take some information from the entity to transfer to the ragdoll, such as the model and skin.

cvar_t *r_ragdolls;
cvar_t *r_ragdoll_debug;

vec3_t rightAxis, leftAxis, upAxis, downAxis, bkwdAxis, fwdAxis;

signed int sign(signed int x)
{
	if (x >= 0)
		return 1;
	else
		return -1;
}

void norm3(vec3_t v, vec3_t out)
{
	float l = VectorLength(v);

	if (l > 0.0f)
	{
		out[0] = v[0] / l;
		out[1] = v[1] / l;
		out[2] = v[2] / l;

		return;
	}
	else
	{
		VectorClear (out);
	}
}

extern void Matrix3x4ForEntity(matrix3x4_t *out, entity_t *ent);
extern void Matrix3x4_Invert(matrix3x4_t *out, matrix3x4_t in);
extern void Matrix3x4_Multiply(matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2);
extern void Matrix3x4_Add(matrix3x4_t *out, matrix3x4_t mat1, matrix3x4_t mat2);
extern void Matrix3x4_Scale(matrix3x4_t *out, matrix3x4_t in, float scale);

//routine to create ragdoll body parts between two joints
void RGD_addBody (int RagDollID, matrix3x4_t *bindmat, char *name, int objectID, vec3_t p1, vec3_t p2, float radius, float density)
{
	//Adds a capsule body between joint positions p1 and p2 and with given
	//radius to the ragdoll.
	int i, nans;
	float length;
	vec3_t xa, ya, za, temp;
	matrix3x4_t initmat;
	dMatrix3 rot;

	p1[1] -= currentmodel->ragdoll.RagDollDims[GLOBAL_Y_OFF];
	p1[2] += currentmodel->ragdoll.RagDollDims[GLOBAL_Z_OFF];
	p2[1] -= currentmodel->ragdoll.RagDollDims[GLOBAL_Y_OFF];
	p2[2] += currentmodel->ragdoll.RagDollDims[GLOBAL_Z_OFF];

	//cylinder length not including endcaps, make capsules overlap by half
	//radius at joints
	VectorSubtract(p1, p2, temp);
	length = VectorLength(temp) - radius;
	if (length <= 0.0f)
		length = 0.1f;

	//create body id
	RagDoll[RagDollID].RagDollObject[objectID].body = dBodyCreate(RagDollWorld);

	//creat the geometry and give it a name
	RagDoll[RagDollID].RagDollObject[objectID].geom = dCreateCapsule (RagDollSpace, radius, length);
	dGeomSetData (RagDoll[RagDollID].RagDollObject[objectID].geom, name);
	dGeomSetBody (RagDoll[RagDollID].RagDollObject[objectID].geom, RagDoll[RagDollID].RagDollObject[objectID].body);

	//set it's mass
	dMassSetCapsule (&RagDoll[RagDollID].RagDollObject[objectID].mass, density, 3, radius, length);
	dBodySetMass (RagDoll[RagDollID].RagDollObject[objectID].body, &RagDoll[RagDollID].RagDollObject[objectID].mass);

	//define body rotation automatically from body axis
	VectorSubtract(p2, p1, za);
	norm3(za, za);

	VectorSet(temp, 1.0f, 0.0f, 0.0f);
	if (fabsf (DotProduct(za, temp)) < 0.7f)
		VectorSet (xa, 1.0f, 0.0f, 0.0f);
	else
		VectorSet (xa, 0.0f, 1.0f, 0.0f);

	CrossProduct (za, xa, ya);

	CrossProduct (ya, za, xa);
	norm3 (xa, xa);
	CrossProduct (za, xa, ya);

	Vector4Set (initmat.a, xa[0], ya[0], za[0], 0.5f*(p1[0] + p2[0]));
	Vector4Set (initmat.b, xa[1], ya[1], za[1], 0.5f*(p1[1] + p2[1]));
	Vector4Set (initmat.c, xa[2], ya[2], za[2], 0.5f*(p1[2] + p2[2]));

	nans = 0;
	for(i = 0; i < 4; i++)
	{
		if(IS_NAN(initmat.a[i])) { initmat.a[i] = 0; nans++; }
		if(IS_NAN(initmat.b[i])) { initmat.b[i] = 0; nans++; }
		if(IS_NAN(initmat.c[i])) { initmat.c[i] = 0; nans++; }
	}
	if(nans > 0)
	{
		if(r_ragdoll_debug->integer)
			Com_Printf("There was a NaN in creating body %i\n", objectID);
	}

	Matrix3x4_Invert(&RagDoll[RagDollID].RagDollObject[objectID].initmat, initmat);

	Matrix3x4_Multiply(&initmat, bindmat[objectID], initmat);

	Vector4Set(&rot[0], initmat.a[0], initmat.a[1], initmat.a[2], 0);
	Vector4Set(&rot[4], initmat.b[0], initmat.b[1], initmat.b[2], 0);
	Vector4Set(&rot[8], initmat.c[0], initmat.c[1], initmat.c[2], 0);

	dBodySetPosition (RagDoll[RagDollID].RagDollObject[objectID].body, initmat.a[3], initmat.b[3], initmat.c[3]);
	dBodySetRotation (RagDoll[RagDollID].RagDollObject[objectID].body, rot);
	dBodySetForce (RagDoll[RagDollID].RagDollObject[objectID].body, 0, 0, 0);
	dBodySetLinearVel (RagDoll[RagDollID].RagDollObject[objectID].body, 0, 40, 150); //a little initial upward velocity
	dBodySetAngularVel (RagDoll[RagDollID].RagDollObject[objectID].body, 0, 0, 0);
	// This appears to make ODE less crashy.
	dBodySetGyroscopicMode (RagDoll[RagDollID].RagDollObject[objectID].body, 0);

}

//joint creation routines
void RGD_addFixedJoint(int RagDollID, matrix3x4_t *bindmat, int jointID, int object1, int object2)
{
	dBodyID body1 = RagDoll[RagDollID].RagDollObject[object1].body;
	dBodyID body2 = RagDoll[RagDollID].RagDollObject[object2].body;

	RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateFixed(RagDollWorld, 0);
	dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);
	dJointSetFixed(RagDoll[RagDollID].RagDollJoint[jointID]);
}

void RGD_addHingeJoint(int RagDollID, matrix3x4_t *bindmat, int jointID, int object1, int object2, vec3_t anchor, vec3_t axis, float loStop, float hiStop)
{
	dBodyID body1 = RagDoll[RagDollID].RagDollObject[object1].body;
	dBodyID body2 = RagDoll[RagDollID].RagDollObject[object2].body;
	vec3_t wanchor, waxis;

	anchor[1] -= currentmodel->ragdoll.RagDollDims[GLOBAL_Y_OFF];
	anchor[2] += currentmodel->ragdoll.RagDollDims[GLOBAL_Z_OFF];

	wanchor[0] = 0.5f*(DotProduct(bindmat[object1].a, anchor) + bindmat[object1].a[3] + DotProduct(bindmat[object2].a, anchor) + bindmat[object2].a[3]);
	wanchor[1] = 0.5f*(DotProduct(bindmat[object1].b, anchor) + bindmat[object1].b[3] + DotProduct(bindmat[object2].b, anchor) + bindmat[object2].b[3]);
	wanchor[2] = 0.5f*(DotProduct(bindmat[object1].c, anchor) + bindmat[object1].c[3] + DotProduct(bindmat[object2].c, anchor) + bindmat[object2].c[3]);
	waxis[0] = DotProduct(bindmat[object1].a, axis) + DotProduct(bindmat[object2].a, axis);
	waxis[1] = DotProduct(bindmat[object1].b, axis) + DotProduct(bindmat[object2].b, axis);
	waxis[2] = DotProduct(bindmat[object1].c, axis) + DotProduct(bindmat[object2].c, axis);
	VectorNormalize(waxis);
	
	if (waxis[0] == 0.0f && waxis[1] == 0.0f && waxis[2] == 0.0f)
	{
		if (r_ragdoll_debug->integer)
			Com_Printf("RGD_addHingeJoint: waxis is zero vector\n");
		return;
	}

	VectorAdd(anchor, RagDoll[RagDollID].origin, anchor);

	RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateHinge(RagDollWorld, 0);

	dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);

	dJointSetHingeAnchor(RagDoll[RagDollID].RagDollJoint[jointID], wanchor[0], wanchor[1], wanchor[2]);
	dJointSetHingeAxis(RagDoll[RagDollID].RagDollJoint[jointID], waxis[0], waxis[1], waxis[2]);
	dJointSetHingeParam(RagDoll[RagDollID].RagDollJoint[jointID], dParamLoStop, loStop);
	dJointSetHingeParam(RagDoll[RagDollID].RagDollJoint[jointID], dParamHiStop,  hiStop);
}

void RGD_addBallJoint(int RagDollID, matrix3x4_t *bindmat, int jointID, int object1, int object2, vec3_t anchor)
{
	dBodyID body1 = RagDoll[RagDollID].RagDollObject[object1].body;
	dBodyID body2 = RagDoll[RagDollID].RagDollObject[object2].body;
	vec3_t wanchor;

	anchor[1] -= currentmodel->ragdoll.RagDollDims[GLOBAL_Y_OFF];
	anchor[2] += currentmodel->ragdoll.RagDollDims[GLOBAL_Z_OFF];

	wanchor[0] = 0.5f*(DotProduct(bindmat[object1].a, anchor) + bindmat[object1].a[3] + DotProduct(bindmat[object2].a, anchor) + bindmat[object2].a[3]);
	wanchor[1] = 0.5f*(DotProduct(bindmat[object1].b, anchor) + bindmat[object1].b[3] + DotProduct(bindmat[object2].b, anchor) + bindmat[object2].b[3]);
	wanchor[2] = 0.5f*(DotProduct(bindmat[object1].c, anchor) + bindmat[object1].c[3] + DotProduct(bindmat[object2].c, anchor) + bindmat[object2].c[3]);

	RagDoll[RagDollID].RagDollJoint[jointID] = dJointCreateBall(RagDollWorld, 0);

	dJointAttach(RagDoll[RagDollID].RagDollJoint[jointID], body1, body2);

	dJointSetBallAnchor(RagDoll[RagDollID].RagDollJoint[jointID], wanchor[0], wanchor[1], wanchor[2]);
}

void RGD_addUniversalJoint(int RagDollID, matrix3x4_t *bindmat, int jointID, int object1, int object2, vec3_t anchor, vec3_t axis1, vec3_t axis2,
	float loStop1, float hiStop1, float loStop2, float hiStop2)
{
	vec3_t wanchor, waxis1, waxis2;

	dBodyID body1 = RagDoll[RagDollID].RagDollObject[object1].body;
	dBodyID body2 = RagDoll[RagDollID].RagDollObject[object2].body;

	anchor[1] -= currentmodel->ragdoll.RagDollDims[GLOBAL_Y_OFF];
	anchor[2] += currentmodel->ragdoll.RagDollDims[GLOBAL_Z_OFF];

	wanchor[0] = 0.5*(DotProduct(bindmat[object1].a, anchor) + bindmat[object1].a[3] + DotProduct(bindmat[object2].a, anchor) + bindmat[object2].a[3]);
	wanchor[1] = 0.5*(DotProduct(bindmat[object1].b, anchor) + bindmat[object1].b[3] + DotProduct(bindmat[object2].b, anchor) + bindmat[object2].b[3]);
	wanchor[2] = 0.5*(DotProduct(bindmat[object1].c, anchor) + bindmat[object1].c[3] + DotProduct(bindmat[object2].c, anchor) + bindmat[object2].c[3]);

	waxis1[0] = DotProduct(bindmat[object1].a, axis1) + DotProduct(bindmat[object2].a, axis1);
	waxis1[1] = DotProduct(bindmat[object1].b, axis1) + DotProduct(bindmat[object2].b, axis1);
	waxis1[2] = DotProduct(bindmat[object1].c, axis1) + DotProduct(bindmat[object2].c, axis1);
	VectorNormalize(waxis1);
	
	if (waxis1[0] == 0.0f && waxis1[1] == 0.0f && waxis1[2] == 0.0f)
	{
		if (r_ragdoll_debug->integer)
			Com_Printf("RGD_addUniversalJoint: waxis1 is zero vector\n");
		return;
	}

	waxis2[0] = DotProduct(bindmat[object1].a, axis2) + DotProduct(bindmat[object2].a, axis2);
	waxis2[1] = DotProduct(bindmat[object1].b, axis2) + DotProduct(bindmat[object2].b, axis2);
	waxis2[2] = DotProduct(bindmat[object1].c, axis2) + DotProduct(bindmat[object2].c, axis2);
	VectorNormalize(waxis2);
	
	if (waxis2[0] == 0.0f && waxis2[1] == 0.0f && waxis2[2] == 0.0f)
	{
		if (r_ragdoll_debug->integer)
			Com_Printf("RGD_addUniversalJoint: waxis2 is zero vector\n");
		return;
	}
	
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

#if !defined WIN32_VARIANT
/* Constants:
 *  CFM -- "Constraint Force Mixing"
 *  ERP -- "Error Reduction Parameter"
 * See ODE documentation for more info.
 *
 * These are needed to get ragdolls to come to complete rest.
 * They are determined empirically by observing when the ragdolls on
 * the floor stop vibrating.
 *
 * We will just hard code them, until there is some reason not to.
 */
const dReal world_cfm_setting = 5.5e-6;
const dReal world_erp_setting = 0.3;
#endif

void RGD_CreateWorldObject( void )
{
	// Initialize the world
	RagDollWorld = dWorldCreate();

	dWorldSetGravity(RagDollWorld, 0.0, 0.0, -512.0);

#if !defined WIN32_VARIANT
	/* Added to support dWorldQuickStep(), which is needed for ODE v0.12
	 */
	dWorldSetCFM( RagDollWorld, world_cfm_setting );
	dWorldSetERP( RagDollWorld, world_erp_setting );
	Com_Printf("RagDollWorld Settings: CFM (%.12f)  ERP (%.3f)\n",
			world_cfm_setting, world_erp_setting );
#endif

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
}

void RGD_DestroyWorldObject( void )
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
void RGD_RagdollBody_Init( int RagDollID, vec3_t origin, char name[MAX_QPATH] )
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

	strcpy (RagDoll[RagDollID].name, name);
	
	memcpy (&RagDollEntity[RagDollID], currententity, sizeof(entity_t));
	RagDollEntity[RagDollID].ragdoll = true;
	RagDollEntity[RagDollID].RagDollData = &RagDoll[RagDollID];
	RagDollEntity[RagDollID].flags = 0;

	// IQM is currently the only supported mesh type for ragdolls
	assert (currententity->model->type == mod_iqm);
	IQM_AnimateFrame (RagDoll[RagDollID].initframe);

	_VectorCopy(currententity->angles, RagDoll[RagDollID].angles);
	VectorCopy(origin, RagDoll[RagDollID].origin);
	VectorCopy(origin, RagDoll[RagDollID].curPos);
	RagDoll[RagDollID].spawnTime = Sys_Milliseconds();
	RagDoll[RagDollID].destroyed = false;

	memset(bindmat, 0, sizeof(bindmat));
	memset(bindweight, 0, sizeof(bindweight));
	for(i = 0; i < currentmodel->num_joints; i++)
	{
		for(j = 0; j < RagDollBindsCount; j++)
		{
			if(!strcmp(&currentmodel->jointname[currentmodel->joints[i].name], RagDollBinds[j].name))
			{
				int object = RagDollBinds[j].object;
				if (!IS_NAN (RagDoll[RagDollID].initframe[i].a[0])) {
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
	VectorSet(R_SHOULDER_POS, -currentmodel->ragdoll.RagDollDims[SHOULDER_W] * 0.5f, 0.0f,
		currentmodel->ragdoll.RagDollDims[SHOULDER_H]);
	VectorSet(L_SHOULDER_POS, currentmodel->ragdoll.RagDollDims[SHOULDER_W] * 0.5f, 0.0f,
		currentmodel->ragdoll.RagDollDims[SHOULDER_H]);

	VectorSet(R_ELBOW_POS, -currentmodel->ragdoll.RagDollDims[ELBOW_X_OFF],
		currentmodel->ragdoll.RagDollDims[ELBOW_Y_OFF],
		currentmodel->ragdoll.RagDollDims[ELBOW_Z_OFF]);
	VectorSet(L_ELBOW_POS, currentmodel->ragdoll.RagDollDims[ELBOW_X_OFF],
		currentmodel->ragdoll.RagDollDims[ELBOW_Y_OFF],
		currentmodel->ragdoll.RagDollDims[ELBOW_Z_OFF]);

	VectorSet(R_WRIST_POS, -currentmodel->ragdoll.RagDollDims[WRIST_X_OFF],
		-currentmodel->ragdoll.RagDollDims[WRIST_Y_OFF],
		currentmodel->ragdoll.RagDollDims[WRIST_Z_OFF]);
	VectorSet(L_WRIST_POS, currentmodel->ragdoll.RagDollDims[WRIST_X_OFF],
		-currentmodel->ragdoll.RagDollDims[WRIST_Y_OFF],
		currentmodel->ragdoll.RagDollDims[WRIST_Z_OFF]);

	VectorSet(R_FINGERS_POS, -currentmodel->ragdoll.RagDollDims[FINGERS_X_OFF],
		-currentmodel->ragdoll.RagDollDims[FINGERS_Y_OFF],
		currentmodel->ragdoll.RagDollDims[FINGERS_Z_OFF]);
	VectorSet(L_FINGERS_POS, currentmodel->ragdoll.RagDollDims[FINGERS_X_OFF],
		-currentmodel->ragdoll.RagDollDims[FINGERS_Y_OFF],
		currentmodel->ragdoll.RagDollDims[FINGERS_Z_OFF]);

	//set lower body
	VectorSet(R_HIP_POS, -currentmodel->ragdoll.RagDollDims[LEG_W] * 0.5f, 0.0f,
		currentmodel->ragdoll.RagDollDims[HIP_H]);
	VectorSet(L_HIP_POS, currentmodel->ragdoll.RagDollDims[LEG_W] * 0.5f, 0.0f,
		currentmodel->ragdoll.RagDollDims[HIP_H]);

	VectorSet(R_KNEE_POS, -currentmodel->ragdoll.RagDollDims[KNEE_X_OFF],
		-currentmodel->ragdoll.RagDollDims[KNEE_Y_OFF],
		currentmodel->ragdoll.RagDollDims[KNEE_Z_OFF]);
	VectorSet(L_KNEE_POS, currentmodel->ragdoll.RagDollDims[KNEE_X_OFF],
		-currentmodel->ragdoll.RagDollDims[KNEE_Y_OFF],
		currentmodel->ragdoll.RagDollDims[KNEE_Z_OFF]);

	VectorSet(R_ANKLE_POS, -currentmodel->ragdoll.RagDollDims[ANKLE_X_OFF],
		-currentmodel->ragdoll.RagDollDims[ANKLE_Y_OFF],
		currentmodel->ragdoll.RagDollDims[ANKLE_Z_OFF]);
	VectorSet(L_ANKLE_POS, currentmodel->ragdoll.RagDollDims[ANKLE_X_OFF],
		-currentmodel->ragdoll.RagDollDims[ANKLE_Y_OFF],
		currentmodel->ragdoll.RagDollDims[ANKLE_Z_OFF]);

	VectorSet(R_HEEL_POS, R_ANKLE_POS[0], R_ANKLE_POS[1] - currentmodel->ragdoll.RagDollDims[HEEL_LEN], R_ANKLE_POS[2]);
	VectorSet(L_HEEL_POS, L_ANKLE_POS[0], L_ANKLE_POS[1] - currentmodel->ragdoll.RagDollDims[HEEL_LEN], L_ANKLE_POS[2]);

	VectorSet(R_TOES_POS, R_ANKLE_POS[0], R_ANKLE_POS[1] + currentmodel->ragdoll.RagDollDims[FOOT_LEN], R_ANKLE_POS[2]);
	VectorSet(L_TOES_POS, L_ANKLE_POS[0], L_ANKLE_POS[1] + currentmodel->ragdoll.RagDollDims[FOOT_LEN], L_ANKLE_POS[2]);

	//build the ragdoll parts
	density = 1.0; //for now

	VectorSet(p1, 0.0, 0.0,	currentmodel->ragdoll.RagDollDims[NECK_H] - 0.1f);
	VectorSet(p2, 0.0, 0.0,	currentmodel->ragdoll.RagDollDims[HIP_H] + 0.1f);
	RGD_addBody(RagDollID, bindmat, "chest", CHEST, p1, p2, currentmodel->ragdoll.RagDollDims[CHEST_W]/2.0f, density);

	VectorSet(p1, R_HIP_POS[0] + 0.1, R_HIP_POS[1], R_HIP_POS[2] - 0.1f);
	VectorSet(p2, L_HIP_POS[0] - 0.1, L_HIP_POS[1], L_HIP_POS[2] - 0.1f);
	RGD_addBody(RagDollID, bindmat, "pelvis", PELVIS, p1, p2, currentmodel->ragdoll.RagDollDims[PELVIS_W]/2.0f, density);

	RGD_addFixedJoint(RagDollID, bindmat, LOWSPINE, CHEST, PELVIS);

	VectorSet(p1, 0.0, 0.0, currentmodel->ragdoll.RagDollDims[HEAD_H]);
	VectorSet(p2, 0.0, 0.0, currentmodel->ragdoll.RagDollDims[NECK_H] + 0.1f);
	RGD_addBody(RagDollID, bindmat, "head", HEAD, p1, p2, currentmodel->ragdoll.RagDollDims[HEAD_W]/2.0f, density);

	VectorSet(p1, 0.0, 0.0, currentmodel->ragdoll.RagDollDims[NECK_H]);
	RGD_addUniversalJoint(RagDollID, bindmat, NECK, CHEST, HEAD, p1, upAxis, rightAxis,
		currentmodel->ragdoll.RagDollDims[HEAD_LOSTOP1] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[HEAD_HISTOP1] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[HEAD_LOSTOP2] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[HEAD_HISTOP2] * (float)M_PI);

	//right leg
	VectorSet(p1, R_HIP_POS[0] - 0.1f, R_HIP_POS[1], R_HIP_POS[2] - 0.1f);
	VectorSet(p2, R_KNEE_POS[0], R_KNEE_POS[1], R_KNEE_POS[2] + 0.1f);
	RGD_addBody(RagDollID, bindmat, "rightupperleg", RIGHTUPPERLEG, p1, p2, currentmodel->ragdoll.RagDollDims[THIGH_W]/2.0f, density);

	RGD_addUniversalJoint(RagDollID, bindmat, RIGHTHIP, PELVIS, RIGHTUPPERLEG, R_HIP_POS,	bkwdAxis, rightAxis,
		currentmodel->ragdoll.RagDollDims[HIP_LOSTOP1] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[HIP_HISTOP1] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[HIP_LOSTOP2] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[HIP_HISTOP2] * (float)M_PI);

	VectorSet(p1, R_KNEE_POS[0], R_KNEE_POS[1], R_KNEE_POS[2] - 0.1f);
	VectorSet(p2, R_ANKLE_POS[0], R_ANKLE_POS[1], R_ANKLE_POS[2]);
	RGD_addBody(RagDollID, bindmat, "rightlowerleg", RIGHTLOWERLEG, p1, p2, currentmodel->ragdoll.RagDollDims[SHIN_W]/2.0f, density);

	RGD_addHingeJoint(RagDollID, bindmat, RIGHTKNEE, RIGHTUPPERLEG,
		RIGHTLOWERLEG, R_KNEE_POS, leftAxis, currentmodel->ragdoll.RagDollDims[KNEE_LOSTOP] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[KNEE_HISTOP] * (float)M_PI);

	RGD_addBody(RagDollID, bindmat, "rightfoot", RIGHTFOOT, R_TOES_POS, R_HEEL_POS,
		currentmodel->ragdoll.RagDollDims[FOOT_W]/2.0f, density);

	RGD_addHingeJoint(RagDollID, bindmat, RIGHTANKLE, RIGHTLOWERLEG,
		RIGHTFOOT, R_ANKLE_POS, rightAxis, currentmodel->ragdoll.RagDollDims[ANKLE_LOSTOP] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[ANKLE_HISTOP] * (float)M_PI);

	//left leg
	VectorSet(p1, L_HIP_POS[0] + 0.1f, L_HIP_POS[1], L_HIP_POS[2] - 0.1f);
	VectorSet(p2, L_KNEE_POS[0], L_KNEE_POS[1], L_KNEE_POS[2] + 0.1f);
	RGD_addBody(RagDollID, bindmat, "leftupperleg", LEFTUPPERLEG, p1, p2, currentmodel->ragdoll.RagDollDims[THIGH_W]/2.0f, density);

	RGD_addUniversalJoint(RagDollID, bindmat, LEFTHIP, PELVIS,LEFTUPPERLEG, L_HIP_POS, fwdAxis, rightAxis,
		currentmodel->ragdoll.RagDollDims[HIP_LOSTOP1] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[HIP_HISTOP1] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[HIP_LOSTOP2] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[HIP_HISTOP2] * (float)M_PI);

	VectorSet(p1, L_KNEE_POS[0], L_KNEE_POS[1], L_KNEE_POS[2] - 0.1);
	VectorSet(p2, L_ANKLE_POS[0], L_ANKLE_POS[1], L_ANKLE_POS[2]);
	RGD_addBody(RagDollID, bindmat, "leftlowerleg", LEFTLOWERLEG, p1, p2, currentmodel->ragdoll.RagDollDims[SHIN_W]/2.0, density);

	RGD_addHingeJoint(RagDollID, bindmat, LEFTKNEE, LEFTUPPERLEG,
		LEFTLOWERLEG, L_KNEE_POS, leftAxis, currentmodel->ragdoll.RagDollDims[KNEE_LOSTOP] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[KNEE_HISTOP] * (float)M_PI);

	RGD_addBody(RagDollID, bindmat, "leftfoot", LEFTFOOT, L_TOES_POS, L_HEEL_POS,
		currentmodel->ragdoll.RagDollDims[FOOT_W]/2.0f, density);

	RGD_addHingeJoint(RagDollID, bindmat, LEFTANKLE, LEFTLOWERLEG,
		LEFTFOOT, L_ANKLE_POS, rightAxis, currentmodel->ragdoll.RagDollDims[ANKLE_LOSTOP] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[ANKLE_HISTOP] * (float)M_PI);

	//right arm
	VectorSet(p1, R_SHOULDER_POS[0] - 0.1f, R_SHOULDER_POS[1], R_SHOULDER_POS[2]);
	VectorSet(p2, R_ELBOW_POS[0], R_ELBOW_POS[1], R_ELBOW_POS[2] + 0.1f);
	RGD_addBody(RagDollID, bindmat, "rightupperarm", RIGHTUPPERARM, p1, p2, currentmodel->ragdoll.RagDollDims[BICEP_W]/2.0f, density);

	RGD_addUniversalJoint(RagDollID, bindmat, RIGHTSHOULDER, CHEST, RIGHTUPPERARM, R_SHOULDER_POS, bkwdAxis, rightAxis,
		currentmodel->ragdoll.RagDollDims[SHOULDER_LOSTOP1] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[SHOULDER_HISTOP1] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[SHOULDER_LOSTOP2] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[SHOULDER_HISTOP2] * (float)M_PI);

	VectorSet(p1, R_ELBOW_POS[0], R_ELBOW_POS[1], R_ELBOW_POS[2] - 0.1f);
	VectorSet(p2, R_WRIST_POS[0], R_WRIST_POS[1], R_WRIST_POS[2] + 0.1f);
	RGD_addBody(RagDollID, bindmat, "rightforearm", RIGHTFOREARM, p1, p2, currentmodel->ragdoll.RagDollDims[FOREARM_W]/2.0f, density);

	RGD_addHingeJoint(RagDollID, bindmat, RIGHTELBOW, RIGHTUPPERARM,
		RIGHTFOREARM, R_ELBOW_POS, downAxis, currentmodel->ragdoll.RagDollDims[ELBOW_LOSTOP] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[ELBOW_HISTOP] * (float)M_PI);

	VectorSet(p1, R_WRIST_POS[0], R_WRIST_POS[1], R_WRIST_POS[2] - 0.1f);
	VectorSet(p2, R_FINGERS_POS[0], R_FINGERS_POS[1], R_FINGERS_POS[2]);
	RGD_addBody(RagDollID, bindmat, "righthand", RIGHTHAND, p1, p2, currentmodel->ragdoll.RagDollDims[HAND_W]/2.0f, density);

	RGD_addHingeJoint(RagDollID, bindmat, RIGHTWRIST, RIGHTFOREARM,
		RIGHTHAND, R_WRIST_POS, fwdAxis, currentmodel->ragdoll.RagDollDims[WRIST_LOSTOP] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[WRIST_HISTOP] * (float)M_PI);

	//left arm
	VectorSet(p1, L_SHOULDER_POS[0] + 0.1f, L_SHOULDER_POS[1], L_SHOULDER_POS[2]);
	VectorSet(p2, L_ELBOW_POS[0], L_ELBOW_POS[1], L_ELBOW_POS[2] + 0.1f);
	RGD_addBody(RagDollID, bindmat, "leftupperarm", LEFTUPPERARM, p1, p2, currentmodel->ragdoll.RagDollDims[BICEP_W]/2.0f, density);

	RGD_addUniversalJoint(RagDollID, bindmat, LEFTSHOULDER, CHEST, LEFTUPPERARM, L_SHOULDER_POS, fwdAxis, rightAxis,
		currentmodel->ragdoll.RagDollDims[SHOULDER_LOSTOP1] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[SHOULDER_HISTOP1] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[SHOULDER_LOSTOP2] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[SHOULDER_HISTOP2] * (float)M_PI);

	VectorSet(p1, L_ELBOW_POS[0], L_ELBOW_POS[1], L_ELBOW_POS[2] - 0.1f);
	VectorSet(p2, L_WRIST_POS[0], L_WRIST_POS[1], L_WRIST_POS[2] + 0.1f);
	RGD_addBody(RagDollID, bindmat, "leftforearm", LEFTFOREARM, p1, p2, currentmodel->ragdoll.RagDollDims[FOREARM_W]/2.0f, density);

	RGD_addHingeJoint(RagDollID, bindmat, LEFTELBOW, LEFTUPPERARM,
		LEFTFOREARM, L_ELBOW_POS,  upAxis, currentmodel->ragdoll.RagDollDims[ELBOW_LOSTOP] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[ELBOW_HISTOP] * (float)M_PI);

	VectorSet(p1, L_WRIST_POS[0], L_WRIST_POS[1], L_WRIST_POS[2] - 0.1f);
	VectorSet(p2, L_FINGERS_POS[0], L_FINGERS_POS[1], L_FINGERS_POS[2]);
	RGD_addBody(RagDollID, bindmat, "leftthand", LEFTHAND, p1, p2, currentmodel->ragdoll.RagDollDims[HAND_W]/2.0f, density);

	RGD_addHingeJoint(RagDollID, bindmat, LEFTWRIST, LEFTFOREARM,
		LEFTHAND, L_WRIST_POS, bkwdAxis, currentmodel->ragdoll.RagDollDims[WRIST_LOSTOP] * (float)M_PI,
		currentmodel->ragdoll.RagDollDims[WRIST_HISTOP] * (float)M_PI);
}

//For creating the surfaces for the ragdoll to collide with
//track total verts externally
int totalODEVerts;

//For adding a single BSP surface(comprised of multiple triangles).
void RGD_BuildODEGeoms(msurface_t *surf)
{
	glpoly_t *p;
	float	*v;
	int		i;
	int polyStart;
	p = surf->polys;
	if(RagDollTriWorld.numODEVerts + p->numverts > RagDollTriWorld.maxODEVerts)
	{
		int growVerts = RagDollTriWorld.maxODEVerts;
		dVector3 *newVerts;
		while(RagDollTriWorld.numODEVerts + p->numverts > growVerts)
			growVerts += GROW_ODE_VERTS;
		newVerts = (dVector3 *)realloc(RagDollTriWorld.ODEVerts, growVerts*sizeof(dVector3));
		if(!newVerts) return;
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
		if(!newTris) return;
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

	totalODEVerts = RagDollTriWorld.numODEVerts;
}

//One terrain triangle.  These are indexed, so we do need the proper indexes.
void RGD_BuildODETerrainGeoms(vec3_t vertex[3], int indx0, int indx1, int indx2)
{
	int		i;

	if(RagDollTriWorld.numODEVerts + 3 > RagDollTriWorld.maxODEVerts)
	{
		int growVerts = RagDollTriWorld.maxODEVerts;
		dVector3 *newVerts;
		while(RagDollTriWorld.numODEVerts + 3 > growVerts)
			growVerts += GROW_ODE_VERTS;
		newVerts = (dVector3 *)realloc(RagDollTriWorld.ODEVerts, growVerts*sizeof(dVector3));
		if(!newVerts) return;
		RagDollTriWorld.maxODEVerts = growVerts;
		RagDollTriWorld.ODEVerts = newVerts;
	}

	for(i = 0; i < 3; i++)
	{
		RagDollTriWorld.ODEVerts[RagDollTriWorld.numODEVerts][0] = vertex[i][0];
		RagDollTriWorld.ODEVerts[RagDollTriWorld.numODEVerts][1] = vertex[i][1];
		RagDollTriWorld.ODEVerts[RagDollTriWorld.numODEVerts][2] = vertex[i][2];
		RagDollTriWorld.numODEVerts++;
	}	

	if(RagDollTriWorld.numODETris + 1 > RagDollTriWorld.maxODETris)
	{
		int growTris = RagDollTriWorld.maxODETris;
		dTriIndex *newTris;
		while(RagDollTriWorld.numODETris + 1 > growTris)
			growTris += GROW_ODE_TRIS;
		newTris = (dTriIndex *)realloc(RagDollTriWorld.ODETris, growTris*sizeof(dTriIndex[3]));
		if(!newTris) return;
		RagDollTriWorld.maxODETris = growTris;
		RagDollTriWorld.ODETris = newTris;
	}

	RagDollTriWorld.ODETris[RagDollTriWorld.numODETris*3+0] = totalODEVerts + indx2;
	RagDollTriWorld.ODETris[RagDollTriWorld.numODETris*3+1] = totalODEVerts + indx1;
	RagDollTriWorld.ODETris[RagDollTriWorld.numODETris*3+2] = totalODEVerts + indx0;
	RagDollTriWorld.numODETris++;
}

/*
=============
R_DrawWorldTrimesh
=============
*/
void RGD_BuildWorldTrimesh ( void )
{
	msurface_t *surf;	

	Com_Printf("building trimesh for bsp\n");

	RagDollTriWorld.numODEVerts = RagDollTriWorld.numODETris = 0;

	for (surf = &r_worldmodel->surfaces[r_worldmodel->firstmodelsurface]; surf < &r_worldmodel->surfaces[r_worldmodel->firstmodelsurface + r_worldmodel->nummodelsurfaces] ; surf++)
	{
		if (surf->texinfo->flags & SURF_SKY)
		{   // no skies here
			continue;
		}
		else
		{
			if (!( surf->iflags & ISURF_DRAWTURB ) )
			{
				RGD_BuildODEGeoms(surf);
			}
		}
	}		
}

void RGD_FinalizeWorldTrimesh ( void )
{
	dMatrix3 rot;

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

	Com_Printf("Trimesh finished\n");
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
	
	RagDoll[RagDollID].destroyed = true; // Do not simulate
	RagDollEntity[RagDollID].nodraw = true; // Do not render

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

void RGD_DestroyWorldTrimesh( void )
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
	}
	RGD_DestroyWorldTrimesh();
}

void RGD_AddNewRagdoll( vec3_t origin, char name[MAX_QPATH] )
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

	//add a ragdoll, look for first open slot
	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		if(RagDoll[RagDollID].destroyed)
		{
			RGD_RagdollBody_Init(RagDollID, origin, name);

			if(r_ragdoll_debug->integer == 2)
			{
				Com_Printf("RagDoll name: %s Ent name: %s Mesh: %s Ent Mesh: %s\n", RagDoll[RagDollID].name, name,
					currentmodel->name, currentmodel->name);

				for(i = 0; i < 27; i++)
					Com_Printf("ragdoll val %i: %4.2f\n", i, currentmodel->ragdoll.RagDollDims[i]);
			}

			if(r_ragdoll_debug->integer)
				Com_Printf("Added a ragdoll @ %4.2f,%4.2f,%4.2f\n", RagDoll[RagDollID].origin[0], RagDoll[RagDollID].origin[1],
					RagDoll[RagDollID].origin[2]);

			//break glass effect for helmets so we don't need to render them
			if(currentmodel->ragdoll.hasHelmet)
				CL_GlassShards(RagDoll[RagDollID].origin, dir, 5);
			break;
		}
	}
}

// Iterate through the ragdoll array, detect expired ragdolls, apply ragdoll
// forcess, lift ghosts, and do physics simulation. 
void R_SimulateAllRagdolls ( void )
{
	int RagDollID;
	int i;
	qboolean ragdoll_visible = false; // At least one is on screen

	if(!r_ragdolls->integer)
		return;

	for(RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
	{
		float dur;
		
		if(RagDoll[RagDollID].destroyed)
			continue;

		dur = Sys_Milliseconds() - RagDoll[RagDollID].spawnTime;

		if(dur > RAGDOLL_DURATION)
		{
			R_DestroyRagDoll(RagDollID, true);
			if(r_ragdoll_debug->integer)
				Com_Printf("Destroyed a ragdoll");
		}
		else
		{
			//we handle the ragdoll's physics, then render the mesh with skeleton adjusted by ragdoll
			//body object positions
			const	dReal *odePos;
			int		shellEffect = false;

			odePos = dBodyGetPosition (RagDoll[RagDollID].RagDollObject[CHEST].body);
			VectorSet (RagDoll[RagDollID].curPos, odePos[0], odePos[1], odePos[2]);

			if(dur > (RAGDOLL_DURATION - 2500))
			{
				dBodySetLinearVel(RagDoll[RagDollID].RagDollObject[CHEST].body, 0, 0, 50); //lift into space

				RagDollEntity[RagDollID].shellAlpha = (1 - dur/RAGDOLL_DURATION);
				shellEffect = true;
			}

			VectorCopy (RagDoll[RagDollID].curPos, RagDollEntity[RagDollID].origin);
			RagDollEntity[RagDollID].flags = shellEffect ? RF_SHELL_GREEN : 0;

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
						if(RagDoll[RagDollID].RagDollForces[i].force > 200 || RagDoll[RagDollID].RagDollForces[i].force < -200)
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


			if(r_ragdoll_debug->integer)
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
						R_DrawMark (org, 4.0f, RGBA (0, 1, 0, 1));
					else if (i > LEFTFOOT)
						R_DrawMark (org, 4.0f, RGBA (0, 0, 1, 1));
					else
						R_DrawMark (org, 4.0f, RGBA (1, 0, 0, 1));
				}
			}

			ragdoll_visible = true;
		}
	}

	if (ragdoll_visible) //here we handle the physics
	{
#if defined WIN32_VARIANT
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
#else
		/* ODE library 0.12 does not support dWorldStepFast1.
		 * Using dWorldQuickStep. See RGD_CreateWorldObject(), above.
		 */
		int frametime = Sys_Milliseconds() - lastODEUpdate;
		
#define SIM_CAP (1000/120)
		// Simulate the ragdoll world EXACTLY 1/120th of a second at a time.
		// This seems to make ODE a lot less crashy.
		while (frametime > SIM_CAP)
		{
			dSpaceCollide(RagDollSpace, 0, &near_callback);
			dWorldQuickStep( RagDollWorld, ((dReal)SIM_CAP)/1000.0 );
			// Remove all temporary collision joints now that the world has been stepped
			dJointGroupEmpty(contactGroup);
			lastODEUpdate += SIM_CAP;
			frametime -= SIM_CAP;
		}
		return;
#endif
	}

	lastODEUpdate = Sys_Milliseconds();
}

void R_ApplyForceToRagdolls(vec3_t origin, float force)
{
	int RagDollID, i;

	if(!r_ragdolls->integer)
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
