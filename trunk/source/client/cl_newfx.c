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
// cl_newfx.c -- MORE entity effects parsing and management

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
======
CL_ColorFlash - flash of light
======
*/
void CL_ColorFlash (vec3_t pos, int ent, int intensity, float r, float g, float b)
{
	cdlight_t	*dl;

	dl = CL_AllocDlight (ent);
	VectorCopy (pos,  dl->origin);
	dl->radius = intensity;
	dl->minlight = 250;
	dl->die = cl.time + 100;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
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

void CL_FlameEffects (centity_t *ent, vec3_t origin)
{
	int			n, count;
	int			j;
	cparticle_t	*p;

	count = rand() & 0xF;

	for(n=0;n<count;n++)
	{
		if (!(p = new_particle()))
			return;
		
		VectorClear (p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.2);
		p->color = 226 + (rand() % 4);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = origin[j] + crand()*5;
			p->vel[j] = crand()*5;
		}
		p->vel[2] = crand() * -10;
		p->accel[2] = -PARTICLE_GRAVITY;
	}

	count = rand() & 0x7;

	for(n=0;n<count;n++)
	{
		if (!(p = new_particle()))
			return;

		VectorClear (p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.5);
		p->color = 0 + (rand() % 4);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = origin[j] + crand()*3;
		}
		p->vel[2] = 20 + crand()*5;
	}

}


/*
===============
CL_GenericParticleEffect
===============
*/
void CL_GenericParticleEffect (vec3_t org, vec3_t dir, int color, int count, int numcolors, int dirspread, float alphavel)
{
	int			i, j;
	cparticle_t	*p;
	float		d;

	for (i=0 ; i<count ; i++)
	{
		if (!(p = new_particle()))
			return;

		if (numcolors > 1)
			p->color = color + (rand() & numcolors);
		else
			p->color = color;

		d = rand() & dirspread;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = crand()*20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
//		VectorCopy (accel, p->accel);
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand()*alphavel);
//		p->alphavel = alphavel;
	}
}

/*
===============
CL_BubbleTrail2 (lets you control the # of bubbles by setting the distance between the spawns)

===============
*/
void CL_BubbleTrail2 (vec3_t start, vec3_t end, int dist)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			i, j;
	cparticle_t	*p;
	float		dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = dist;
	VectorScale (vec, dec, vec);

	for (i=0 ; i<len ; i+=dec)
	{
		if (!(p = new_particle()))
			return;

		VectorClear (p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.1);
		p->color = 4 + (rand()&7);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*2;
			p->vel[j] = crand()*10;
		}
		p->org[2] -= 4;
//		p->vel[2] += 6;
		p->vel[2] += 20;

		VectorAdd (move, vec, move);
	}
}


void CL_Heatbeam (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	vec3_t		right, up;
	int			i;
	float		c, s;
	vec3_t		dir;
	float		ltime;
	float		step = 44.0, rstep;
	float		start_pt;
	float		rot;
	float		variance;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	VectorCopy (cl.v_right, right);
	VectorCopy (cl.v_up, up);
	VectorMA (move, -0.5, right, move);
	VectorMA (move, -0.5, up, move);

	ltime = (float) cl.time/1000.0;
	start_pt = fmod(ltime*96.0,step);
	VectorMA (move, start_pt, vec, move);

	VectorScale (vec, step, vec);
	rstep = M_PI/60.0;
	for (i=start_pt+40 ; i<len ; i+=step)
	{

		for (rot = 0; rot < M_PI*2; rot += rstep)
		{
			if (!(p = new_particle()))
				return;
			
			VectorClear (p->accel);
			variance = 50.0;
			c = cos(rot)*10;
			s = sin(rot)*5;
			
			// trim it so it looks like it's starting at the origin
			if (i < 40)
			{
				VectorScale (right, c*(i/10.0), dir);
				VectorMA (dir, s*(i/10.0), up, dir);
			}
			else
			{
				VectorScale (right, c, dir);
				VectorMA (dir, s, up, dir);
			}
		
			p->alpha = 0.5;
			p->alphavel = -2.0 / (1+frand()*0.2);
			p->color = 0x74 + (rand()&7);
			p->scale = 8 + (rand()&7) ;
			p->scalevel = 0;
			
			p->type = PARTICLE_FIREBALL;
			p->texnum = r_explosiontexture->texnum;
			p->blendsrc = GL_SRC_ALPHA;
			p->blenddst = GL_ONE;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = move[j] + dir[j]*3;
				p->vel[j] = 0;
			}
		}
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

	for (i=0 ; i<self->count ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_FIREBALL;//self->color + (rand()&7);
		p->texnum = r_explosiontexture->texnum;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->color = 0xe0;
		p->scale = 4 + (rand()&7);
		p->scalevel = 0;
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

		p->accel[0] = p->accel[1] = -100;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 0.3;

		p->alphavel = -1.0 / (0.8 + frand()*0.3);

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
		p->scalevel = 1.5;
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
		p->accel[2] = -PARTICLE_GRAVITY/2;
		p->alpha = 0.1;

		p->alphavel = -1.0 / (7.8 + frand()*0.3);

		p->fromsustainedeffect = true;
	}
	self->nextthink += self->thinkinterval;
}
/*
===============
CL_TrackerTrail
===============
*/
void CL_TrackerTrail (vec3_t start, vec3_t end, int particleColor)
{
	vec3_t		move;
	vec3_t		vec;
	vec3_t		forward,right,up,angle_dir;
	float		len;
	int			j;
	cparticle_t	*p;
	int			dec;
	float		dist;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	VectorCopy(vec, forward);
	vectoangles2 (forward, angle_dir);
	AngleVectors (angle_dir, forward, right, up);

	dec = 3;
	VectorScale (vec, 3, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		if (!(p = new_particle()))
			return;
		VectorClear (p->accel);
		
		p->alpha = 1.0;
		p->alphavel = -2.0;
		p->color = particleColor;
		dist = DotProduct(move, forward);
		VectorMA(move, 8 * cos(dist), up, p->org);
		for (j=0 ; j<3 ; j++)
		{
//			p->org[j] = move[j] + crand();
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
		p->vel[2] = 5;

		VectorAdd (move, vec, move);
	}
}

void CL_Tracker_Shell(vec3_t origin)
{
	vec3_t			dir;
	int				i;
	cparticle_t		*p;

	for(i=0;i<300;i++)
	{
		if (!(p = new_particle()))
			return;
		VectorClear (p->accel);
		
		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = 0;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
	
		VectorMA(origin, 40, dir, p->org);
	}
}

void CL_MonsterPlasma_Shell(vec3_t origin)
{
	vec3_t			dir;
	int				i;
	cparticle_t		*p;

	for(i=0;i<40;i++)
	{
		if (!(p = new_particle()))
			return;
		VectorClear (p->accel);

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = 0xe0;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
	
		VectorMA(origin, 10, dir, p->org);
//		VectorMA(origin, 10*(((rand () & 0x7fff) / ((float)0x7fff))), dir, p->org);
	}
}

void CL_Tracker_Explode(vec3_t	origin)
{
	vec3_t			dir, backdir;
	int				i;
	cparticle_t		*p;

	for(i=0;i<300;i++)
	{
		if (!(p = new_particle()))
			return;
		VectorClear (p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0;
		p->color = 0;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		VectorNormalize(dir);
		VectorScale(dir, -1, backdir);
	
		VectorMA(origin, 64, dir, p->org);
		VectorScale(backdir, 64, p->vel);
	}
	
}

/*
===============
CL_TagTrail

===============
*/
void CL_TagTrail (vec3_t start, vec3_t end, float color)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	int			dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	while (len >= 0)
	{
		len -= dec;

		if (!(p = new_particle()))
			return;
		VectorClear (p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.8+frand()*0.2);
		p->color = color;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*16;
			p->vel[j] = crand()*5;
			p->accel[j] = 0;
		}

		VectorAdd (move, vec, move);
	}
}

/*
===============
CL_ColorExplosionParticles
===============
*/
void CL_ColorExplosionParticles (vec3_t org, int color, int run)
{
	int			i, j;
	cparticle_t	*p;

	for (i=0 ; i<128 ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->color = color + (rand() % run);

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (rand()%256)-128;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.4 / (0.6 + frand()*0.2);
	}
}

/*
===============
CL_ParticleSmokeEffect - like the steam effect, but unaffected by gravity
===============
*/
void CL_ParticleSmokeEffect (vec3_t org, vec3_t dir, int color, int count, int magnitude)
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
//			p->vel[j] = dir[j]*magnitude;
		}
		VectorScale (dir, magnitude, p->vel);
		d = crand()*magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*magnitude/3;
		VectorMA (p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = p->accel[2] = 0;
		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
}

/*
===============
CL_BlasterParticles2

Wall impact puffs (Green)
===============
*/
void CL_BlasterParticles2 (vec3_t org, vec3_t dir, unsigned int color)
{
	int			i, j;
	cparticle_t	*p;
	float		d;
	int			count;

	count = 40;
	for (i=0 ; i<count ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->color = color + (rand()&7);

		d = rand()&15;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = dir[j] * 30 + crand()*40;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
}

/*
===============
CL_BlasterTrail2

Green!
===============
*/
void CL_BlasterTrail2 (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	cparticle_t	*p;
	int			dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 5;
	VectorScale (vec, 5, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		if (!(p = new_particle()))
			return;
		VectorClear (p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.3+frand()*0.2);
		p->color = 0xd0;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand()*5;
			p->accel[j] = 0;
		}

		VectorAdd (move, vec, move);
	}
}
