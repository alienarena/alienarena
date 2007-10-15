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
// cl_tent.c -- client side temporary entities

#include "client.h"

cl_sustain_t	cl_sustains[MAX_SUSTAINS];

extern void CL_TeleportParticles (vec3_t org);

void CL_BlasterParticles (vec3_t org, vec3_t dir);
void CL_ExplosionParticles (vec3_t org);
void CL_MuzzleParticles (vec3_t org);
void CL_BlueMuzzleParticles (vec3_t org);
void CL_SmartMuzzle (vec3_t org);
void CL_Voltage(vec3_t org);
void CL_Deathfield (vec3_t org);
void CL_BFGExplosionParticles (vec3_t org);
void CL_DustParticles (vec3_t org);
void CL_BlueBlasterParticles (vec3_t org, vec3_t dir);

struct sfx_s	*cl_sfx_ric1;
struct sfx_s	*cl_sfx_ric2;
struct sfx_s	*cl_sfx_ric3;
struct sfx_s	*cl_sfx_lashit;
struct sfx_s	*cl_sfx_spark5;
struct sfx_s	*cl_sfx_spark6;
struct sfx_s	*cl_sfx_spark7;
struct sfx_s	*cl_sfx_railg;
struct sfx_s	*cl_sfx_rockexp;
struct sfx_s	*cl_sfx_watrexp;
struct sfx_s	*cl_sfx_footsteps[4];
struct sfx_s    *cl_sfx_metal_footsteps[4];

/*
=================
CL_RegisterTEntSounds
=================
*/
void CL_RegisterTEntSounds (void)
{
	int		i;
	char	name[MAX_QPATH];

	cl_sfx_ric1 = S_RegisterSound ("world/ric1.wav");
	cl_sfx_ric2 = S_RegisterSound ("world/ric2.wav");
	cl_sfx_ric3 = S_RegisterSound ("world/ric3.wav");
	cl_sfx_lashit = S_RegisterSound("weapons/lashit.wav");
	cl_sfx_railg = S_RegisterSound ("weapons/railgf1a.wav");
	cl_sfx_rockexp = S_RegisterSound ("weapons/rocklx1a.wav");

	S_RegisterSound ("player/land1.wav");
	S_RegisterSound ("player/fall2.wav");
	S_RegisterSound ("player/fall1.wav");

	for (i=0 ; i<4 ; i++)
	{
		Com_sprintf (name, sizeof(name), "player/step%i.wav", i+1);
		cl_sfx_footsteps[i] = S_RegisterSound (name);
	}
	for (i=0 ; i<4 ; i++)
	{
	   Com_sprintf (name, sizeof(name), "player/step_metal%i.wav", i+1);
	   cl_sfx_metal_footsteps[i] = S_RegisterSound (name);
	}
}

/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
	memset (cl_sustains, 0, sizeof(cl_sustains));
}

/*
=================
CL_ParseParticles
=================
*/
void CL_ParseParticles (void)
{
	int		color, count;
	vec3_t	pos, dir;

	MSG_ReadPos (&net_message, pos);
	MSG_ReadDir (&net_message, dir);

	color = MSG_ReadByte (&net_message);

	count = MSG_ReadByte (&net_message);

	CL_ParticleEffect (pos, dir, color, count);
}

//=============
//ROGUE
void CL_ParseSteam (void)
{
	vec3_t	pos, dir;
	int		id, i;
	int		r;
	int		cnt;
	int		color;
	int		magnitude;
	cl_sustain_t	*s, *free_sustain;

//	id = MSG_ReadShort (&net_message);		// an id of -1 is an instant effect
	id = 25;
	if (id != -1) // sustains
	{
//			Com_Printf ("Sustain effect id %d\n", id);
		free_sustain = NULL;
		for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
		{
			if (s->id == 0)
			{
				free_sustain = s;
				break;
			}
		}
		if (free_sustain)
		{
			s->id = id;
			s->count = MSG_ReadByte (&net_message);
			s->count = 10;//just for testing here
			MSG_ReadPos (&net_message, s->org);
			MSG_ReadDir (&net_message, s->dir);
			r = MSG_ReadByte (&net_message);
			s->color = r & 0xff;
			s->magnitude = 50;//MSG_ReadShort (&net_message);
			s->endtime = cl.time + 10000000;//MSG_ReadLong (&net_message);
			s->think = CL_ParticleSteamEffect2;
			s->thinkinterval = 1;
			s->nextthink = cl.time;
		}
		else
		{
//				Com_Printf ("No free sustains!\n");
			// FIXME - read the stuff anyway
			cnt = MSG_ReadByte (&net_message);
			MSG_ReadPos (&net_message, pos);
			MSG_ReadDir (&net_message, dir);
			r = MSG_ReadByte (&net_message);
			magnitude = MSG_ReadShort (&net_message);
			magnitude = MSG_ReadLong (&net_message); // really interval
		}
	}
	else // instant
	{
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		r = MSG_ReadByte (&net_message);
		magnitude = MSG_ReadShort (&net_message);
		color = r & 0xff;
		CL_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
//		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
	}
}

void CL_ParseFire (void)
{
	vec3_t	pos, dir;
	int		id, i;
	int		r;
	int		cnt;
	int		magnitude;
	cl_sustain_t	*s, *free_sustain;

//	id = MSG_ReadShort (&net_message);		// an id of -1 is an instant effect
	id = 25;
	if (id != -1) // sustains
	{
//			Com_Printf ("Sustain effect id %d\n", id);
		free_sustain = NULL;
		for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
		{
			if (s->id == 0)
			{
				free_sustain = s;
				break;
			}
		}
		if (free_sustain)
		{
			s->id = id;
			s->count = MSG_ReadByte (&net_message);
			s->count = 10;//just for testing here
			MSG_ReadPos (&net_message, s->org);
			MSG_ReadDir (&net_message, s->dir);
			r = MSG_ReadByte (&net_message);
			s->color = r & 0xff;
			s->magnitude = 150;//MSG_ReadShort (&net_message);
			s->endtime = cl.time + 10000000;//MSG_ReadLong (&net_message);
			s->think = CL_ParticleFireEffect2;
			s->thinkinterval = 1;
			s->nextthink = cl.time;
		}
		else
		{
//				Com_Printf ("No free sustains!\n");
			// FIXME - read the stuff anyway
			cnt = MSG_ReadByte (&net_message);
			MSG_ReadPos (&net_message, pos);
			MSG_ReadDir (&net_message, dir);
			r = MSG_ReadByte (&net_message);
			magnitude = MSG_ReadShort (&net_message);
			magnitude = MSG_ReadLong (&net_message); // really interval
		}
	}

}

void CL_ParseSmoke (void)
{
	vec3_t	pos, dir;
	int		id, i;
	int		r;
	int		cnt;
	int		magnitude;
	cl_sustain_t	*s, *free_sustain;

//	id = MSG_ReadShort (&net_message);		// an id of -1 is an instant effect
	id = 25;
	if (id != -1) // sustains
	{
//			Com_Printf ("Sustain effect id %d\n", id);
		free_sustain = NULL;
		for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
		{
			if (s->id == 0)
			{
				free_sustain = s;
				break;
			}
		}
		if (free_sustain)
		{
			s->id = id;
			s->count = MSG_ReadByte (&net_message);
			MSG_ReadPos (&net_message, s->org);
			MSG_ReadDir (&net_message, s->dir);
			r = MSG_ReadByte (&net_message);
			s->color = r & 0xff;
			s->magnitude = 400;//MSG_ReadShort (&net_message);
			s->endtime = cl.time + 10000000;//MSG_ReadLong (&net_message);
			s->think = CL_ParticleSmokeEffect2;
			s->thinkinterval = 1;
			s->nextthink = cl.time;
		}
		else
		{
//				Com_Printf ("No free sustains!\n");
			// FIXME - read the stuff anyway
			cnt = MSG_ReadByte (&net_message);
			MSG_ReadPos (&net_message, pos);
			MSG_ReadDir (&net_message, dir);
			r = MSG_ReadByte (&net_message);
			magnitude = MSG_ReadShort (&net_message);
			magnitude = MSG_ReadLong (&net_message); // really interval
		}
	}

}

//ROGUE
//=============


/*
=================
CL_ParseTEnt
=================
*/
extern void CL_MuzzleFlashParticle (vec3_t org, vec3_t angles, qboolean from_client);
static byte splash_color[] = {0x00, 0xe0, 0xb0, 0x50, 0xd0, 0xe0, 0xe8};

void CL_ParseTEnt (void)
{
	int		type;
	vec3_t	pos, pos2, dir;
	int		cnt;
	int		color;
	int		r;
	float	fcolor[3], intensity, alpha;
	trace_t	trace;
	static vec3_t mins = { -8, -8, -8 }; 
    static vec3_t maxs = { 8, 8, 8 }; 

	type = MSG_ReadByte (&net_message);

	switch (type)
	{
	case TE_BLOOD:			// bullet hitting flesh
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleEffect (pos, dir, 450, 60);// doing the blood here - color is red
		break;
	case TE_GREENBLOOD:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleEffect (pos, dir, 550, 60);// doing the blood here - color is green
		break;
	case TE_GUNSHOT:
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);

		if (type == TE_GUNSHOT) {
			CL_ParticleEffect (pos, dir, 425, 10);
			trace = CL_Trace ( pos, mins, maxs, pos, -1, MASK_SOLID, true, NULL); 
			if(trace.contents)
				CL_BulletMarks(pos, dir);
		}
		else
			CL_ParticleEffect (pos, dir, 425, 2);	// bullets, color is 0xe0

		CL_BulletSparks( pos, dir);

		if (type != TE_SPARKS)
		{
			// impact sound
			cnt = rand()&15;
			if (cnt == 1)
				S_StartSound (pos, 0, 0, cl_sfx_ric1, 1, ATTN_NORM, 0);
			else if (cnt == 2)
				S_StartSound (pos, 0, 0, cl_sfx_ric2, 1, ATTN_NORM, 0);
			else if (cnt == 3)
				S_StartSound (pos, 0, 0, cl_sfx_ric3, 1, ATTN_NORM, 0);
		}

		break;

	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		if (type == TE_SCREEN_SPARKS) {
			CL_LaserSparks (pos, dir, 0xd0, 20);
			trace = CL_Trace ( pos, mins, maxs, pos, -1, MASK_SOLID, true, NULL); 
			if(trace.contents)
				CL_BeamgunMark(pos, dir, 0.8);
		}
		else
			CL_ParticleEffect (pos, dir, 0xb0, 40);
		//FIXME : replace or remove this sound
		S_StartSound (pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_LASERBEAM:				// martian laser effect
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_LaserBeam (pos, pos2);
		break;

	case TE_SPLASH:			// bullet hitting water
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		r = MSG_ReadByte (&net_message);
		if (r > 6)
			color = 0x00;
		else
			color = splash_color[r];
		//CL_ParticleEffect (pos, dir, color, cnt); //out with the old.

		CL_SplashEffect (pos, dir, color, cnt);

		if (r == SPLASH_SPARKS)
		{
			r = rand() & 3;
			if (r == 0)
				S_StartSound (pos, 0, 0, cl_sfx_spark5, 1, ATTN_STATIC, 0);
			else if (r == 1)
				S_StartSound (pos, 0, 0, cl_sfx_spark6, 1, ATTN_STATIC, 0);
			else
				S_StartSound (pos, 0, 0, cl_sfx_spark7, 1, ATTN_STATIC, 0);
		}
		break;

	case TE_LASER_SPARKS:
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		color = MSG_ReadByte (&net_message);
		CL_ParticleEffect2 (pos, dir, color, cnt);
		break;

	case TE_BLASTER:			// blaster hitting wall
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_BlasterParticles (pos, dir);

		break;

	case TE_RAILTRAIL:			// railgun effect - let's try the heat missile thingy too!
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_DisruptorBeam (pos, pos2);
		trace = CL_Trace ( pos, mins, maxs, pos2, -1, MASK_SOLID, true, NULL); 
		if(trace.contents)
			CL_BeamgunMark(pos2, trace.plane.normal, 0.4);
		S_StartSound (pos, 0, 0, cl_sfx_railg, 1, ATTN_NONE, 0);
		break;

	case TE_EXPLOSION2: //using this for a "dust" explosion, ie big, big footsteps effect
		MSG_ReadPos (&net_message, pos);

		CL_DustParticles (pos);
		break;

	case TE_EXPLOSION1:
	case TE_ROCKET_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
		MSG_ReadPos (&net_message, pos);

		V_AddStain ( pos, 25, 15, 15, 15, 200 );
		V_AddStain ( pos, 25*3, 15, 15, 15, 66 );

		CL_ExplosionParticles (pos);
		if (type == TE_ROCKET_EXPLOSION_WATER)
			S_StartSound (pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			S_StartSound (pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);

		break;

	case TE_BFG_BIGEXPLOSION:
		MSG_ReadPos (&net_message, pos);
		CL_BFGExplosionParticles (pos);
		break;


	case TE_BUBBLETRAIL:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_BubbleTrail (pos, pos2);
		break;

	case TE_PARASITE_ATTACK:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_LaserBlast (pos, pos2);
		S_StartSound (pos, 0, 0, S_RegisterSound ("weapons/biglaser.wav"), 1, ATTN_NONE, 0);
		break;

	case TE_REDLASER:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);

		CL_RedBlasterBeam (pos, pos2);

		break;

	case TE_BOSSTPORT:			// boss teleporting to station
		MSG_ReadPos (&net_message, pos);
		CL_BigTeleportParticles (pos);
		S_StartSound (pos, 0, 0, S_RegisterSound ("misc/bigtele.wav"), 1, ATTN_NONE, 0);
		break;

	case TE_LIGHTNING:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_NewLightning (pos, pos2);
		break;

	case TE_VAPORBEAM:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_VaporizerBeam (pos, pos2);
		trace = CL_Trace ( pos, mins, maxs, pos2, -1, MASK_SOLID, true, NULL); 
		if(trace.contents)
			CL_VaporizerMarks(pos2, trace.plane.normal);	
		break;

	case TE_STEAM:
		CL_ParseSteam();
		break;
	case TE_FIRE:
		CL_ParseFire();
		break;
	case TE_SMOKE:
		CL_ParseSmoke();
		break;

	case TE_SAYICON:
		MSG_ReadPos(&net_message, pos);
		CL_SayIcon(pos);
		break;

	case TE_TELEPORT_EFFECT:
		MSG_ReadPos (&net_message, pos);
		CL_TeleportParticles (pos);
		break;

	case TE_LEADERBLASTER:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_RedBlasterBeam (pos, pos2);
		S_StartSound (pos, 0, 0, S_RegisterSound ("weapons/biglaser.wav"), 1, ATTN_NONE, 0);
		break;

	case TE_CHAINGUNSMOKE:
		MSG_ReadPos (&net_message, pos);
		CL_MuzzleParticles (pos);
		CL_MuzzleFlashParticle(pos, cl.refdef.viewangles, false);
		break;
	case TE_BLUE_MUZZLEFLASH:
		MSG_ReadPos (&net_message, pos);
		CL_BlueMuzzleParticles (pos);
		break;
	case TE_SMART_MUZZLEFLASH:
		MSG_ReadPos (&net_message, pos);
		CL_SmartMuzzle (pos);
		break;

	case TE_VOLTAGE:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_Voltage (pos);
		break;
	case TE_DEATHFIELD:
		MSG_ReadPos (&net_message, pos);
		CL_Deathfield (pos);
		break;
	case TE_BLASTERBEAM:			// blaster beam effect
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_BlasterBeam (pos, pos2);
		trace = CL_Trace ( pos, mins, maxs, pos2, -1, MASK_SOLID, true, NULL); 
		if(trace.contents)
			CL_BlasterMark(pos2, trace.plane.normal);
		break;

	case TE_STAIN:
		MSG_ReadPos (&net_message, pos);
		intensity = MSG_ReadFloat (&net_message);
		fcolor[0] = MSG_ReadByte (&net_message);
		fcolor[1] = MSG_ReadByte (&net_message);
		fcolor[2] = MSG_ReadByte (&net_message);
		alpha = MSG_ReadByte (&net_message);
		V_AddStain ( pos, intensity, fcolor[0], fcolor[1], fcolor[2], alpha );
		break;
	default:
		Com_Error (ERR_DROP, "CL_ParseTEnt: bad type");
	}
}

extern cvar_t *hand;

/* PMM - CL_Sustains */
void CL_ProcessSustain ()
{
	cl_sustain_t	*s;
	int				i;

	for (i=0, s=cl_sustains; i< MAX_SUSTAINS; i++, s++)
	{
		if (s->id)
			if ((s->endtime >= cl.time) && (cl.time >= s->nextthink))
			{
				s->think (s);
			}
			else if (s->endtime < cl.time)
				s->id = 0;
	}
}

/*
=================
CL_AddTEnts
=================
*/
void CL_AddTEnts (void)
{
	CL_ProcessSustain();
}
