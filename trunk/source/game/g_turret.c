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
// g_turret.c

#include "g_local.h"


void AnglesNormalize(vec3_t vec)
{
	while(vec[0] > 360)
		vec[0] -= 360;
	while(vec[0] < 0)
		vec[0] += 360;
	while(vec[1] > 360)
		vec[1] -= 360;
	while(vec[1] < 0)
		vec[1] += 360;
}

float SnapToEights(float x)
{
	x *= 8.0;
	if (x > 0.0)
		x += 0.5;
	else
		x -= 0.5;
	return 0.125 * (int)x;
}


void turret_blocked(edict_t *self, edict_t *other)
{
	edict_t	*attacker;

	if (other->takedamage)
	{
		if (self->teammaster->owner)
			attacker = self->teammaster->owner;
		else
			attacker = self->teammaster;
		T_Damage (other, self, attacker, vec3_origin, other->s.origin, vec3_origin, self->teammaster->dmg, 10, 0, MOD_CRUSH);
	}
}

/*QUAKED turret_breach (0 0 0) ?
This portion of the turret can change both pitch and yaw.
The model  should be made with a flat pitch.
It (and the associated base) need to be oriented towards 0.
Use "angle" to set the starting angle.

"speed"		default 50
"dmg"		default 10
"angle"		point this forward
"target"	point this at an info_notnull at the muzzle tip
"minpitch"	min acceptable pitch angle : default -30
"maxpitch"	max acceptable pitch angle : default 30
"minyaw"	min acceptable yaw angle   : default 0
"maxyaw"	max acceptable yaw angle   : default 360
*/

void turret_breach_fire (edict_t *self)
{
	vec3_t	f, r, u;
	vec3_t	start;
	int		damage;
	int		speed;

	AngleVectors (self->s.angles, f, r, u);
	VectorMA (self->s.origin, self->move_origin[0], f, start);
	VectorMA (start, self->move_origin[1], r, start);
	VectorMA (start, self->move_origin[2], u, start);

	damage = 100 + random() * 50;
	speed = 550 + 50 * skill->value;
	fire_rocket (self->teammaster->owner, start, f, damage, speed, 150, damage);
	gi.positioned_sound (start, self, CHAN_WEAPON, gi.soundindex("weapons/rocklf1a.wav"), 1, ATTN_NORM, 0);
}

void turret_breach_think (edict_t *self)
{
	edict_t	*ent;
	vec3_t	current_angles;
	vec3_t	delta;

	VectorCopy (self->s.angles, current_angles);
	AnglesNormalize(current_angles);

	AnglesNormalize(self->move_angles);
	if (self->move_angles[PITCH] > 180)
		self->move_angles[PITCH] -= 360;

	// clamp angles to mins & maxs
	if (self->move_angles[PITCH] > self->pos1[PITCH])
		self->move_angles[PITCH] = self->pos1[PITCH];
	else if (self->move_angles[PITCH] < self->pos2[PITCH])
		self->move_angles[PITCH] = self->pos2[PITCH];

	if ((self->move_angles[YAW] < self->pos1[YAW]) || (self->move_angles[YAW] > self->pos2[YAW]))
	{
		float	dmin, dmax;

		dmin = fabs(self->pos1[YAW] - self->move_angles[YAW]);
		if (dmin < -180)
			dmin += 360;
		else if (dmin > 180)
			dmin -= 360;
		dmax = fabs(self->pos2[YAW] - self->move_angles[YAW]);
		if (dmax < -180)
			dmax += 360;
		else if (dmax > 180)
			dmax -= 360;
		if (fabs(dmin) < fabs(dmax))
			self->move_angles[YAW] = self->pos1[YAW];
		else
			self->move_angles[YAW] = self->pos2[YAW];
	}

	VectorSubtract (self->move_angles, current_angles, delta);
	if (delta[0] < -180)
		delta[0] += 360;
	else if (delta[0] > 180)
		delta[0] -= 360;
	if (delta[1] < -180)
		delta[1] += 360;
	else if (delta[1] > 180)
		delta[1] -= 360;
	delta[2] = 0;

	if (delta[0] > self->speed * FRAMETIME)
		delta[0] = self->speed * FRAMETIME;
	if (delta[0] < -1 * self->speed * FRAMETIME)
		delta[0] = -1 * self->speed * FRAMETIME;
	if (delta[1] > self->speed * FRAMETIME)
		delta[1] = self->speed * FRAMETIME;
	if (delta[1] < -1 * self->speed * FRAMETIME)
		delta[1] = -1 * self->speed * FRAMETIME;

	VectorScale (delta, 1.0/FRAMETIME, self->avelocity);

	self->nextthink = level.time + FRAMETIME;

	for (ent = self->teammaster; ent; ent = ent->teamchain)
		ent->avelocity[1] = self->avelocity[1];

	// if we have adriver, adjust his velocities
	if (self->owner)
	{
		float	angle;
		float	target_z;
		float	diff;
		vec3_t	target;
		vec3_t	dir;

		// angular is easy, just copy ours
		self->owner->avelocity[0] = self->avelocity[0];
		self->owner->avelocity[1] = self->avelocity[1];

		// x & y
		angle = self->s.angles[1] + self->owner->move_origin[1];
		angle *= (M_PI*2 / 360);
		target[0] = SnapToEights(self->s.origin[0] + cos(angle) * self->owner->move_origin[0]);
		target[1] = SnapToEights(self->s.origin[1] + sin(angle) * self->owner->move_origin[0]);
		target[2] = self->owner->s.origin[2];

		VectorSubtract (target, self->owner->s.origin, dir);
		self->owner->velocity[0] = dir[0] * 1.0 / FRAMETIME;
		self->owner->velocity[1] = dir[1] * 1.0 / FRAMETIME;

		// z
		angle = self->s.angles[PITCH] * (M_PI*2 / 360);
		target_z = SnapToEights(self->s.origin[2] + self->owner->move_origin[0] * tan(angle) + self->owner->move_origin[2]);

		diff = target_z - self->owner->s.origin[2];
		self->owner->velocity[2] = diff * 1.0 / FRAMETIME;

		if (self->spawnflags & 65536)
		{
			turret_breach_fire (self);
			self->spawnflags &= ~65536;
		}
	}
}

void turret_breach_finish_init (edict_t *self)
{
	// get and save info for muzzle location
	if (!self->target)
	{
		gi.dprintf("%s at %s needs a target\n", self->classname, vtos(self->s.origin));
	}
	else
	{
		self->target_ent = G_PickTarget (self->target);
		VectorSubtract (self->target_ent->s.origin, self->s.origin, self->move_origin);
		G_FreeEdict(self->target_ent);
	}

	self->teammaster->dmg = self->dmg;
	self->think = turret_breach_think;
	self->think (self);
}

void SP_turret_breach (edict_t *self)
{
	self->solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel (self, self->model);

	if (!self->speed)
		self->speed = 50;
	if (!self->dmg)
		self->dmg = 10;

	if (!st.minpitch)
		st.minpitch = -30;
	if (!st.maxpitch)
		st.maxpitch = 30;
	if (!st.maxyaw)
		st.maxyaw = 360;

	self->pos1[PITCH] = -1 * st.minpitch;
	self->pos1[YAW]   = st.minyaw;
	self->pos2[PITCH] = -1 * st.maxpitch;
	self->pos2[YAW]   = st.maxyaw;

	self->ideal_yaw = self->s.angles[YAW];
	self->move_angles[YAW] = self->ideal_yaw;

	self->blocked = turret_blocked;

	self->think = turret_breach_finish_init;
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity (self);
}


/*QUAKED turret_base (0 0 0) ?
This portion of the turret changes yaw only.
MUST be teamed with a turret_breach.
*/

void SP_turret_base (edict_t *self)
{
	self->solid = SOLID_BSP;
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel (self, self->model);
	self->blocked = turret_blocked;
	gi.linkentity (self);
}


