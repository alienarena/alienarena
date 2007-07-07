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
// cl_newfx.c -- MORE entity effects parsing and management(mostly steam and fire effects here)

#include "client.h"
#include "../ref_gl/r_image.h"
#include "../ref_gl/qgl.h"

extern cparticle_t	particles[MAX_PARTICLES];
extern int			cl_numparticles;
extern cvar_t		*vid_ref;

extern void MakeNormalVectors (vec3_t forward, vec3_t right, vec3_t up);

#ifdef _WIN32
static __inline cparticle_t *new_particle (void)
#else
static inline cparticle_t *new_particle (void)
#endif
{
	cparticle_t	*p;
	extern cparticle_t	*active_particles, *free_particles;

	if (!free_particles)
		return NULL;

	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	p->type = 0;
	p->time = cl.time;

	return (active_particles = p);
}

/*
======
vectoangles2 - this is duplicated in the game DLL, but I need it here.
======
*/
void vectoangles2 (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;
	
	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
	// PMM - fixed to correct for pitch of 0
		if (value1[0])
			yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = 270;

		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

/*
===============
CL_SmokeTrail
===============
*/
void CL_SmokeTrail (vec3_t start, vec3_t end, int colorStart, int colorRun, int spacing)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	VectorScale (vec, spacing, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= spacing;

		if (!(p = new_particle()))
			return;
		VectorClear (p->accel);
		
		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.5);
		p->color = colorStart + (rand() % colorRun);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*3;
			p->accel[j] = 0;
		}
		p->vel[2] = 20 + crand()*5;

		VectorAdd (move, vec, move);
	}
}

/*
===============
CL_ParticleSteamEffect

Puffs with velocity along direction, with some randomness thrown in
===============
*/
void CL_ParticleSteamEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude)
{
	int			i, j;
	cparticle_t	*p;
	float		d;
	vec3_t		r, u;

	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<count ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->color = color + (rand()&7);

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + magnitude*0.1*crand();
		}
		VectorScale (dir, magnitude, p->vel);
		d = crand()*magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*magnitude/3;
		VectorMA (p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY/2;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand()*0.3);

		p->fromsustainedeffect = true;
	}
}

void CL_ParticleSteamEffect2 (cl_sustain_t *self)
{
	int			i, j;
	cparticle_t	*p;
	float		d;
	vec3_t		r, u;
	vec3_t		dir;

	VectorCopy (self->dir, dir);
	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<self->count ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_GUNSHOT;//self->color + (rand()&7);
		p->texnum = r_pufftexture->texnum;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 4 + (rand()&2);
		p->scalevel = 0;
		p->color = 15;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = self->org[j] + self->magnitude*0.1*crand();
		}
		VectorScale (dir, self->magnitude, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY/2;
		p->alpha = 0.1;

		p->alphavel = -1.0 / (4.5 + frand()*0.3);

		p->fromsustainedeffect = true;
	}
	self->nextthink += self->thinkinterval;
}
void CL_ParticleFireEffect2 (cl_sustain_t *self)
{
	int			i, j;
	cparticle_t	*p;
	float		d;
	vec3_t		r, u;
	vec3_t		dir;

	VectorCopy (self->dir, dir);
	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<1 ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_FIREBALL;
		p->texnum = r_explosiontexture->texnum;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->color = 0xe0;
		p->scale = 24 + (rand()&7);
		p->scalevel = 4;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = self->org[j] + self->magnitude*0.1*crand();

		}
		p->vel[2] = 100;
		VectorScale (dir, self->magnitude, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY/3;
		p->alpha = 0.03;

		p->alphavel = -.015 / (0.8 + frand()*0.3);

		p->fromsustainedeffect = true;
	}
	self->nextthink += self->thinkinterval;
}
void CL_ParticleSmokeEffect2 (cl_sustain_t *self)
{
	int			i, j;
	cparticle_t	*p;
	float		d;
	vec3_t		r, u;
	vec3_t		dir;

	VectorCopy (self->dir, dir);
	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<self->count ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_SMOKE;
		p->texnum = r_smoketexture->texnum;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 6 + (rand()&7);
		p->color = 14;
		p->scalevel = 4.5;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = self->org[j] + self->magnitude*0.1*crand();
		}
		p->vel[2] = 10;
		VectorScale (dir, self->magnitude, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY/3;
		p->alpha = 0.03;

		p->alphavel = -.015 / (7.8 + frand()*0.3);

		p->fromsustainedeffect = true;
	}
	self->nextthink += self->thinkinterval;
}
