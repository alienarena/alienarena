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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "server.h"

static size_t szr; // just for quieting unused result warnings

/*
=============================================================================

Encode a client frame onto the network channel

=============================================================================
*/


/*
=============
SV_EmitPacketEntities

Writes a delta update of an entity_state_t list to the message.
=============
*/
void SV_EmitPacketEntities (client_t *cl, client_frame_t *from, client_frame_t *to, sizebuf_t *msg)
{
	entity_state_t	*oldent=NULL, *newent=NULL;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		from_num_entities;
	int		bits;

#if 0
	if (numprojs)
		MSG_WriteByte (msg, svc_packetentities2);
	else
#endif
		MSG_WriteByte (msg, svc_packetentities);

	if (!from)
		from_num_entities = 0;
	else
		from_num_entities = from->num_entities;

	newindex = 0;
	oldindex = 0;
	while (newindex < to->num_entities || oldindex < from_num_entities)
	{
		// Hard limit: always prevent overflow to MAX_MSGLEN to prevent crashes (even for loopback/single-player)
		if (msg->cursize > MAX_MSGLEN) {
			break;
		}

		if (newindex >= to->num_entities)
			newnum = 9999;
		else
		{
			newent = &svs.client_entities[(to->first_entity+newindex)%svs.num_client_entities];
			newnum = newent->number;
		}

		if (oldindex >= from_num_entities)
			oldnum = 9999;
		else
		{
			oldent = &svs.client_entities[(from->first_entity+oldindex)%svs.num_client_entities];
			oldnum = oldent->number;
		}

		if (newnum == oldnum)
		{	// delta update from old position
			// because the force parm is false, this will not result
			// in any bytes being emited if the entity has not changed at all
			// note that players are always 'newentities', this updates their oldorigin always
			// and prevents warping
			MSG_WriteDeltaEntity (oldent, newent, msg, false, newent->number <= maxclients->value);
			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{	// this is a new entity, send it from the baseline
			MSG_WriteDeltaEntity (&sv.baselines[newnum], newent, msg, true, true);
			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
			bits = U_REMOVE;
			if (oldnum >= 256)
				bits |= U_NUMBER16 | U_MOREBITS1;

			MSG_WriteByte (msg,	bits&255 );
			if (bits & 0x0000ff00)
				MSG_WriteByte (msg,	(bits>>8)&255 );

			if (bits & U_NUMBER16)
				MSG_WriteShort (msg, oldnum);
			else
				MSG_WriteByte (msg, oldnum);

			oldindex++;
			continue;
		}
	}

	MSG_WriteShort (msg, 0);	// end of packetentities
}



/*
=============
SV_WritePlayerstateToClient

=============
*/
void SV_WritePlayerstateToClient (client_frame_t *from, client_frame_t *to, sizebuf_t *msg)
{
	int				i;
	int				pflags;
	player_state_t	*ps, *ops;
	player_state_t	dummy;
	int				statbits;

	ps = &to->ps;
	if (!from)
	{
		memset (&dummy, 0, sizeof(dummy));
		ops = &dummy;
	}
	else
		ops = &from->ps;

	//
	// determine what needs to be sent
	//
	pflags = 0;

	if (ps->pmove.pm_type != ops->pmove.pm_type)
		pflags |= PS_M_TYPE;

	if (ps->pmove.origin[0] != ops->pmove.origin[0]
		|| ps->pmove.origin[1] != ops->pmove.origin[1]
		|| ps->pmove.origin[2] != ops->pmove.origin[2] )
		pflags |= PS_M_ORIGIN;

	if (ps->pmove.velocity[0] != ops->pmove.velocity[0]
		|| ps->pmove.velocity[1] != ops->pmove.velocity[1]
		|| ps->pmove.velocity[2] != ops->pmove.velocity[2] )
		pflags |= PS_M_VELOCITY;

	if (ps->pmove.pm_time != ops->pmove.pm_time)
		pflags |= PS_M_TIME;

	if (ps->pmove.pm_flags != ops->pmove.pm_flags)
		pflags |= PS_M_FLAGS;

	if (ps->pmove.gravity != ops->pmove.gravity)
		pflags |= PS_M_GRAVITY;

	if (ps->pmove.delta_angles[0] != ops->pmove.delta_angles[0]
		|| ps->pmove.delta_angles[1] != ops->pmove.delta_angles[1]
		|| ps->pmove.delta_angles[2] != ops->pmove.delta_angles[2] )
		pflags |= PS_M_DELTA_ANGLES;


	if (ps->viewoffset[0] != ops->viewoffset[0]
		|| ps->viewoffset[1] != ops->viewoffset[1]
		|| ps->viewoffset[2] != ops->viewoffset[2] )
		pflags |= PS_VIEWOFFSET;

	if (ps->viewangles[0] != ops->viewangles[0]
		|| ps->viewangles[1] != ops->viewangles[1]
		|| ps->viewangles[2] != ops->viewangles[2] )
		pflags |= PS_VIEWANGLES;

	if (ps->kick_angles[0] != ops->kick_angles[0]
		|| ps->kick_angles[1] != ops->kick_angles[1]
		|| ps->kick_angles[2] != ops->kick_angles[2] )
		pflags |= PS_KICKANGLES;

	if (ps->blend[0] != ops->blend[0]
		|| ps->blend[1] != ops->blend[1]
		|| ps->blend[2] != ops->blend[2]
		|| ps->blend[3] != ops->blend[3] )
		pflags |= PS_BLEND;

	if (ps->fov != ops->fov)
		pflags |= PS_FOV;

	if (ps->rdflags != ops->rdflags)
		pflags |= PS_RDFLAGS;

	if (ps->gunframe != ops->gunframe
		|| (int)(ops->gunoffset[0]*4) != (int)(ps->gunoffset[0]*4)
		|| (int)(ops->gunoffset[1]*4) != (int)(ps->gunoffset[1]*4)
		|| (int)(ops->gunoffset[2]*4) != (int)(ps->gunoffset[2]*4)
		|| (int)(ops->gunangles[0]*4) != (int)(ps->gunangles[0]*4)
		|| (int)(ops->gunangles[1]*4) != (int)(ps->gunangles[1]*4)
		|| (int)(ops->gunangles[2]*4) != (int)(ps->gunangles[2]*4) )
		pflags |= PS_WEAPONFRAME;

	pflags |= PS_WEAPONINDEX;

	//
	// write it
	//
	MSG_WriteByte (msg, svc_playerinfo);
	MSG_WriteShort (msg, pflags);

	//
	// write the pmove_state_t
	//
	if (pflags & PS_M_TYPE)
		MSG_WriteByte (msg, ps->pmove.pm_type);

	if (pflags & PS_M_ORIGIN)
	{
		MSG_WriteSizeInt (msg, coord_bytes, ps->pmove.origin[0]);
		MSG_WriteSizeInt (msg, coord_bytes, ps->pmove.origin[1]);
		MSG_WriteSizeInt (msg, coord_bytes, ps->pmove.origin[2]);
	}

	if (pflags & PS_M_VELOCITY)
	{
		MSG_WriteShort (msg, ps->pmove.velocity[0]);
		MSG_WriteShort (msg, ps->pmove.velocity[1]);
		MSG_WriteShort (msg, ps->pmove.velocity[2]);
	}

	if (pflags & PS_M_TIME)
		MSG_WriteByte (msg, ps->pmove.pm_time);

	if (pflags & PS_M_FLAGS)
		MSG_WriteByte (msg, ps->pmove.pm_flags);

	if (pflags & PS_M_GRAVITY)
		MSG_WriteShort (msg, ps->pmove.gravity);

	if (pflags & PS_M_DELTA_ANGLES)
	{
		MSG_WriteShort (msg, ps->pmove.delta_angles[0]);
		MSG_WriteShort (msg, ps->pmove.delta_angles[1]);
		MSG_WriteShort (msg, ps->pmove.delta_angles[2]);
	}

	//
	// write the rest of the player_state_t
	//
	if (pflags & PS_VIEWOFFSET)
	{
		MSG_WriteChar (msg, ps->viewoffset[0]*4);
		MSG_WriteChar (msg, ps->viewoffset[1]*4);
		MSG_WriteChar (msg, ps->viewoffset[2]*4);
	}

	if (pflags & PS_VIEWANGLES)
	{
		MSG_WriteAngle16 (msg, ps->viewangles[0]);
		MSG_WriteAngle16 (msg, ps->viewangles[1]);
		MSG_WriteAngle16 (msg, ps->viewangles[2]);
	}

	if (pflags & PS_KICKANGLES)
	{
		MSG_WriteChar (msg, ps->kick_angles[0]*4);
		MSG_WriteChar (msg, ps->kick_angles[1]*4);
		MSG_WriteChar (msg, ps->kick_angles[2]*4);
	}

	if (pflags & PS_WEAPONINDEX)
	{
		MSG_WriteByte (msg, ps->gunindex);
	}

	if (pflags & PS_WEAPONFRAME)
	{
		MSG_WriteByte (msg, ps->gunframe);
		MSG_WriteChar (msg, ps->gunoffset[0]*4);
		MSG_WriteChar (msg, ps->gunoffset[1]*4);
		MSG_WriteChar (msg, ps->gunoffset[2]*4);
		MSG_WriteChar (msg, ps->gunangles[0]*4);
		MSG_WriteChar (msg, ps->gunangles[1]*4);
		MSG_WriteChar (msg, ps->gunangles[2]*4);
	}

	if (pflags & PS_BLEND)
	{
		MSG_WriteByte (msg, ps->blend[0]*255);
		MSG_WriteByte (msg, ps->blend[1]*255);
		MSG_WriteByte (msg, ps->blend[2]*255);
		MSG_WriteByte (msg, ps->blend[3]*255);
	}
	if (pflags & PS_FOV)
		MSG_WriteByte (msg, ps->fov);
	if (pflags & PS_RDFLAGS)
		MSG_WriteByte (msg, ps->rdflags);

	// send stats
	statbits = 0;
	for (i=0 ; i<MAX_STATS ; i++)
		if (ps->stats[i] != ops->stats[i])
			statbits |= 1<<i;
	MSG_WriteLong (msg, statbits);
	for (i=0 ; i<MAX_STATS ; i++)
		if (statbits & (1<<i) )
			MSG_WriteShort (msg, ps->stats[i]);
}


/*
==================
SV_WriteFrameToClient
==================
*/
void SV_WriteFrameToClient (client_t *client, sizebuf_t *msg)
{
	client_frame_t		*frame, *oldframe;
	int					lastframe;
	int                 max_history;

	// Project 100 Smart Delta:
	// For old clients keep a 3 frames margin to prevent wrapping errors.
	max_history = client->p100 ? (UPDATE_BACKUP - 1) : (UPDATE_BACKUP - 3);

	//Com_Printf ("%i -> %i\n", client->lastframe, sv.framenum);
	// this is the frame we are creating
	frame = &client->frames[sv.framenum & UPDATE_MASK];

	if (client->lastframe <= 0)
	{	// client is asking for a retransmit
		oldframe = NULL;
		lastframe = -1;
	}
	else if (sv.framenum - client->lastframe >= max_history)
	{	// client hasn't gotten a good message through in a long time
//		Com_Printf ("%s: Delta request from out-of-date packet.\n", client->name);
		oldframe = NULL;
		lastframe = -1;
	}
	else
	{	// we have a valid message to delta from
		oldframe = &client->frames[client->lastframe & UPDATE_MASK];
		lastframe = client->lastframe;
	}

	MSG_WriteByte (msg, svc_frame);
	MSG_WriteLong (msg, sv.framenum);
	MSG_WriteLong (msg, lastframe);	// what we are delta'ing from
	MSG_WriteByte (msg, client->surpressCount);	// rate dropped packets
	client->surpressCount = 0;

	// send over the areabits
	MSG_WriteByte (msg, frame->areabytes);
	SZ_Write (msg, frame->areabits, frame->areabytes);

	// delta encode the playerstate
	SV_WritePlayerstateToClient (oldframe, frame, msg);

	// delta encode the entities
	SV_EmitPacketEntities (client, oldframe, frame, msg);
}


/*
=============================================================================

Build a client frame structure

=============================================================================
*/

byte		fatpvs[65536/8];	// 32767 is MAX_MAP_LEAFS

/*
============
SV_FatPVS

The client will interpolate the view position,
so we can't use a single PVS point
===========
*/
void SV_FatPVS (vec3_t org)
{
	int		leafs[64];
	int		i, j, count;
	int		longs;
	byte	*src;
	vec3_t	mins, maxs;

	for (i=0 ; i<3 ; i++)
	{
		mins[i] = org[i] - 8;
		maxs[i] = org[i] + 8;
	}

	count = CM_BoxLeafnums (mins, maxs, leafs, 64, NULL);
	if (count < 1)
		Com_Error (ERR_FATAL, "SV_FatPVS: count < 1");
	longs = (CM_NumClusters()+31)>>5;

	// convert leafs to clusters
	for (i=0 ; i<count ; i++)
		leafs[i] = CM_LeafCluster(leafs[i]);

	memcpy (fatpvs, CM_ClusterPVS(leafs[0]), longs<<2);
	// or in all the other leaf bits
	for (i=1 ; i<count ; i++)
	{
		for (j=0 ; j<i ; j++)
			if (leafs[i] == leafs[j])
				break;
		if (j != i)
			continue;		// already have the cluster we want
		src = CM_ClusterPVS(leafs[i]);
		for (j=0 ; j<longs ; j++)
			((long *)fatpvs)[j] |= ((long *)src)[j];
	}
}


/*
=============
SV_BuildClientFrame

Decides which entities are going to be visible to the client, and
copies off the playerstat and areabits.
=============
*/
extern float sv_model_bounds[MAX_MODELS];
extern cvar_t *sv_entity_cull;
extern cvar_t *sv_entity_cull_max;
extern cvar_t *sv_entity_budget;
extern cvar_t *sv_entity_budget_max;
extern cvar_t *sv_tickrate;
extern cvar_t *sv_entity_cull_vel_away_dot;

#define ENTITY_CULL_WINDOW_MS		1000	// how often (ms) to re-evaluate the adaptive angle
#define ENTITY_CULL_CONFIRM_WINDOWS	2		// consecutive over-budget windows required before escalating

/*
=============
SV_UpdateAdaptiveEntityCull

Re-evaluates a client's adaptive entity culling angle, at most once every
ENTITY_CULL_WINDOW_MS. Uses the LEAST number of entities seen visible to
this client during the window (not the average or latest), and only
escalates culling after ENTITY_CULL_CONFIRM_WINDOWS consecutive windows
confirm sustained overload - biasing the whole system toward rendering
entities rather than culling them when in doubt. Relaxes back down
immediately (single window) once load drops, since there's no reason to
delay showing more detail once it's cheap enough again.

This only reacts to what's actually visible to THIS client right now (PVS
and angular culling already applied), so it naturally adapts to map layout
and player position - a cluttered but well-compartmentalized map won't
trigger this the way an open map exposing lots of entities at once will,
without needing any per-map tuning.
=============
*/
static void SV_UpdateAdaptiveEntityCull (client_t *client, int sent_entities)
{
	float	tickrate_scale;
	float	budget, budget_max;
	float	t;
	float	cull, rad, tanv;

	client->auto_entity_last_count = sent_entities;	// for status/diagnostics display only

	if (client->entity_cull > 0.0f)
		return;		// client has an explicit override, adaptive scaling doesn't apply

	// Track the least-loaded frame seen this window.
	if (client->auto_entity_window_min < 0 || sent_entities < client->auto_entity_window_min)
		client->auto_entity_window_min = sent_entities;

	if (svs.realtime < client->auto_entity_cull_next_update)
		return;		// not time to re-evaluate yet, keep the current angle

	client->auto_entity_cull_next_update = svs.realtime + ENTITY_CULL_WINDOW_MS;

	if (sv_entity_cull->value <= 0.0f)
	{
		// sv_entity_cull 0 means culling is fully disabled; don't let
		// adaptive scaling re-enable it.
		client->auto_entity_cull = 0.0f;
		client->auto_entity_cull_tan_sq = 0.0f;
		client->auto_entity_window_min = -1;
		client->auto_entity_over_streak = 0;
		return;
	}

	tickrate_scale = 100.0f / (float)sv_tickrate->integer;
	budget = sv_entity_budget->value * tickrate_scale;
	budget_max = sv_entity_budget_max->value * tickrate_scale;

	if (client->auto_entity_window_min > budget) {
		// Over budget this window - only escalate after a few consecutive
		// confirming windows (sustained load), not on a single spike.
		if (client->auto_entity_over_streak < ENTITY_CULL_CONFIRM_WINDOWS - 1)
		{
			client->auto_entity_over_streak++;
			client->auto_entity_window_min = -1;
			return;		// not confirmed yet, keep the current (less aggressive) angle
		}
	} else {
		client->auto_entity_over_streak = 0;
	}

	if (budget_max <= budget) {
		t = (client->auto_entity_window_min > budget) ? 1.0f : 0.0f;
	} else {
		t = (client->auto_entity_window_min - budget) / (budget_max - budget);
		if (t < 0.0f)
			t = 0.0f;
		if (t > 1.0f)
			t = 1.0f;
	}

	cull = sv_entity_cull->value + t * (sv_entity_cull_max->value - sv_entity_cull->value);

	client->auto_entity_window_min = -1;

	if (cull == client->auto_entity_cull)
		return;		// no change, skip recomputing tan_sq

	client->auto_entity_cull = cull;
	rad = cull * 3.14159f / 180.0f;
	tanv = tanf(rad);
	client->auto_entity_cull_tan_sq = tanv * tanv;
}

void SV_BuildClientFrame (client_t *client)
{
	int		e, i;
	vec3_t	org;
	edict_t	*ent;
	edict_t	*clent;
	edict_t	*orig_clent;
	client_frame_t	*frame;
	entity_state_t	*state;
	int		l;
	int		clientarea, clientcluster;
	int		leafnum;
	int		c_fullsend;
	byte	*clientphs;
	byte	*bitvector;
	
	client_t	*redir_client;
	int			redir_num;
	float		entity_cull_tan_sq;
	
	orig_clent = client->edict;
	if (!orig_clent->client)
		return;		// not in game yet
		
	redir_num = client->edict->redirect_number;
	if (redir_num != client->edict->s.number)
	{
		for (i=0, redir_client = svs.clients ; i<maxclients->integer; i++, redir_client++)
			if (redir_num == redir_client->edict->s.number)
				break;
		if (i == maxclients->integer)
			redir_client = client;
	}
	else
		redir_client = client;
	
	clent = redir_client->edict;
	if (!clent->client)
		return;		// not in game yet

	// this is the frame we are creating
	frame = &client->frames[sv.framenum & UPDATE_MASK];

	frame->senttime = svs.realtime; // save it for ping calc later

	// find the client's PVS
	for (i=0 ; i<3 ; i++)
		org[i] = clent->client->ps.pmove.origin[i]*0.125 + clent->client->ps.viewoffset[i];

	leafnum = CM_PointLeafnum (org);
	clientarea = CM_LeafArea (leafnum);
	clientcluster = CM_LeafCluster (leafnum);

	// calculate the visible areas
	frame->areabytes = CM_WriteAreaBits (frame->areabits, clientarea);

	// grab the current player_state_t
	frame->ps = clent->client->ps;
	
	if (client != redir_client)
	{
		//some adjustments for ghost mode
		if (frame->ps.pmove.pm_type != PM_DEAD)
			frame->ps.pmove.pm_type = PM_FREEZE;
		frame->ps.fov = client->edict->client->ps.fov;
	}


	SV_FatPVS (org);
	clientphs = CM_ClusterPHS (clientcluster);

	if (client->entity_cull > 0.0f) {
		entity_cull_tan_sq = client->entity_cull_tan_sq;
	} else {
		entity_cull_tan_sq = client->auto_entity_cull_tan_sq;
	}

	// Persistence rule: once an entity has been sent to this client, it
	// stays exempt from angular-size culling as long as the player isn't
	// clearly moving away from it (this includes standing still, where
	// there's no clear direction at all - default to "not moving away").
	// This avoids entities popping out mid-approach or flickering as the
	// adaptive angle changes, without instantly revealing far-away
	// not-yet-visible entities just because the player is facing them.
	vec3_t	player_vel_dir;
	float	player_speed;
	qboolean has_vel_dir = false;

	player_vel_dir[0] = clent->client->ps.pmove.velocity[0] * 0.125f;
	player_vel_dir[1] = clent->client->ps.pmove.velocity[1] * 0.125f;
	player_vel_dir[2] = clent->client->ps.pmove.velocity[2] * 0.125f;
	player_speed = VectorLength (player_vel_dir);

	if (player_speed > 1.0f)	// enough speed for a meaningful direction
	{
		VectorScale (player_vel_dir, 1.0f / player_speed, player_vel_dir);
		has_vel_dir = true;
	}

	// build up the list of visible entities
	frame->num_entities = 0;
	frame->first_entity = svs.next_client_entities;

	c_fullsend = 0;

	// Raw count of entities that pass every check EXCEPT the angular-size
	// cull itself (i.e. what WOULD be sent if entity_cull_tan_sq were 0).
	// This is what the adaptive system measures load with - using the
	// post-cull frame->num_entities instead would create a feedback loop
	// (escalating cull reduces the count, which then looks "under budget",
	// causing it to relax, which raises the count again, causing it to
	// re-escalate... an oscillation, seen as entities flickering in and
	// out every few seconds even while standing still).
	int raw_visible_count = 0;

	for (e=1 ; e<ge->num_edicts ; e++)
	{
		ent = EDICT_NUM(e);
		qboolean angular_culled = false;
		qboolean entity_added = false;

		// ignore ents without visible models
		if (ent->svflags & SVF_NOCLIENT)
			continue;

		// ignore ents without visible models unless they have an effect
		if (!ent->s.modelindex && !ent->s.effects && !ent->s.sound
			&& !ent->s.event)
			continue;

		// ignore if not touching a PV leaf
		if (ent != clent)
		{
			if (!CM_AreasConnected (clientarea, ent->areanum))
			{	// doors can legally straddle two areas, so
				// we may need to check another one
				if (!ent->areanum2
					|| !CM_AreasConnected (clientarea, ent->areanum2))
					goto entity_persistence_update;		// blocked by a door
			}

			// FIXME: if an ent has a model and a sound, but isn't
			// in the PVS, only the PHS, clear the model
			if (ent->s.sound)
			{
				bitvector = fatpvs;	//clientphs;
			}
			else
				bitvector = fatpvs;

			if (ent->num_clusters == -1)
			{	// too many leafs for individual check, go by headnode
				if (!CM_HeadnodeVisible (ent->headnode, bitvector))
					goto entity_persistence_update;
				c_fullsend++;
			}
			else
			{	// check individual leafs
				for (i=0 ; i < ent->num_clusters ; i++)
				{
					l = ent->clusternums[i];
					if (bitvector[l >> 3] & (1 << (l&7) ))
						break;
				}
				if (i == ent->num_clusters)
					goto entity_persistence_update;		// not visible
			}

			if (!ent->s.modelindex)
			{	// don't send sounds if they will be attenuated away
				vec3_t	delta;
				float	len;

				VectorSubtract (org, ent->s.origin, delta);
				len = VectorLength (delta);
				if (len > 400)
					goto entity_persistence_update;
			}

			// This entity passed every check above - it's "visible" for
			// load-measurement purposes regardless of angular culling.
			raw_visible_count++;

			// Cull entities that are far away to keep the server performant.
			// Protection layers:
			// 1. SVF_ALWAYS_SEND - gameplay critical items
			// 2. Brush models (inline models, name starts with '*') - never cull map structure
			// 3. Zero/tiny bounding box (<=4) - projectiles/effects, never cull
			// 4. Angular size culling for all other entities
			// 5. Persistence - once visible, stays visible unless the player
			//    is clearly moving away from it (see sv_entity_cull_vel_away_dot)
			if (entity_cull_tan_sq > 0.0f && !(ent->svflags & SVF_ALWAYS_SEND) &&
				(ent->s.modelindex == 0 || sv.configstrings[CS_MODELS + ent->s.modelindex][0] != '*')) {

				// Entity size from cached model bounds (side-array, doesn't touch ent->mins/maxs)
				float entity_size = 0;
				if (ent->s.modelindex > 0 && ent->s.modelindex < MAX_MODELS)
					entity_size = sv_model_bounds[ent->s.modelindex];

				// Only cull entities with a known model size (>0 means SV_ReadModelBounds succeeded)
				if (entity_size > 0) {
					vec3_t	dist_vec;
					float	dist_sq;
					qboolean was_visible, moving_away = false;
					int		byte_idx = e >> 3;
					byte	bit_mask = 1 << (e & 7);

					was_visible = (client->entity_was_visible[byte_idx] & bit_mask) != 0;

					// Distance to entity origin (not absbox — we intentionally don't
					// inflate the entity's absbox, so use origin directly)
					VectorSubtract(org, ent->s.origin, dist_vec);
					dist_sq = DotProduct(dist_vec, dist_vec);

					if (was_visible && has_vel_dir && dist_sq > 0.0f)
					{
						float	inv_dist = 1.0f / sqrtf(dist_sq);
						vec3_t	to_ent_dir;
						float	dot;

						// dist_vec points from entity to player (org - origin);
						// negate it to get the direction from player to entity.
						to_ent_dir[0] = -dist_vec[0] * inv_dist;
						to_ent_dir[1] = -dist_vec[1] * inv_dist;
						to_ent_dir[2] = -dist_vec[2] * inv_dist;

						dot = DotProduct (player_vel_dir, to_ent_dir);	// +1 ahead, -1 behind

						if (dot < sv_entity_cull_vel_away_dot->value)
							moving_away = true;
					}

					// atan(size/dist) < threshold  <=>  size^2 < dist_sq * tan^2(threshold)
					if (!(was_visible && !moving_away) && entity_size * entity_size < dist_sq * entity_cull_tan_sq)
						angular_culled = true;  // Angular size too small, cull it
				}
			}

			if (angular_culled)
				goto entity_persistence_update;
		}
		
		if (ent->s.number != e)
		{ // note: server has limited info about entities
			Com_DPrintf("Fixing ent->s.number: %i to %i for a %s\n",
					ent->s.number, e, (ent->client ? "client" : "non-client") );
			ent->s.number = e;
		}
		
		if (ent == orig_clent && orig_clent != clent)
		{
			Com_Printf ("CAN'T HAPPEN?\n");
			goto entity_persistence_update;
		}
		
		// add it to the circular client_entities array
		state = &svs.client_entities[svs.next_client_entities%svs.num_client_entities];
		*state = ent->s;
		if (ent == clent && orig_clent != clent)
			state->number = orig_clent->s.number;

		// don't mark players missiles as solid
		if (ent->owner == client->edict)
			state->solid = 0;

		svs.next_client_entities++;
		frame->num_entities++;
		entity_added = true;

entity_persistence_update:
		if (ent != clent)
		{
			int		byte_idx = e >> 3;
			byte	bit_mask = 1 << (e & 7);

			if (entity_added)
				client->entity_was_visible[byte_idx] |= bit_mask;
			else
				client->entity_was_visible[byte_idx] &= ~bit_mask;
		}
	}

	SV_UpdateAdaptiveEntityCull (client, raw_visible_count);
}


/*
==================
SV_RecordDemoMessage

Save everything in the world out without deltas.
Used for recording footage for merged or assembled demos
==================
*/
void SV_RecordDemoMessage (void)
{
	int			e;
	edict_t		*ent;
	entity_state_t	nostate;
	sizebuf_t	buf;
	byte		buf_data[32768];
	int			len;

	if (!svs.demofile)
		return;

	memset (&nostate, 0, sizeof(nostate));
	SZ_Init (&buf, buf_data, sizeof(buf_data));
	SZ_SetName (&buf, "Demo message buffer", false);

	// write a frame message that doesn't contain a player_state_t
	MSG_WriteByte (&buf, svc_frame);
	MSG_WriteLong (&buf, sv.framenum);

	MSG_WriteByte (&buf, svc_packetentities);

	e = 1;
	ent = EDICT_NUM(e);
	while (e < ge->num_edicts)
	{
		// ignore ents without visible models unless they have an effect
		if (ent->inuse &&
			ent->s.number &&
			(ent->s.modelindex || ent->s.effects || ent->s.sound || ent->s.event) &&
			!(ent->svflags & SVF_NOCLIENT))
			MSG_WriteDeltaEntity (&nostate, &ent->s, &buf, false, true);

		e++;
		ent = EDICT_NUM(e);
	}

	MSG_WriteShort (&buf, 0);		// end of packetentities

	// now add the accumulated multicast information
	SZ_Write (&buf, svs.demo_multicast.data, svs.demo_multicast.cursize);
	SZ_Clear (&svs.demo_multicast);

	// now write the entire message to the file, prefixed by the length
	len = LittleLong (buf.cursize);
	szr = fwrite (&len, 4, 1, svs.demofile);
	szr = fwrite (buf.data, buf.cursize, 1, svs.demofile);
}

