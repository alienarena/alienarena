#include "g_local.h"

/*
============
G_ResetHistory

Clear out the given client's history (should be called when the teleport bit is flipped)
============
*/
void G_ResetHistory( edict_t *ent ) {
	int		i;
	float	time;

	// fill up the history with data (assume the current position)
	ent->client->historyHead = NUM_CLIENT_HISTORY - 1;
	for ( i = ent->client->historyHead, time = level.time; i >= 0; i--, time -= .05 ) {
		VectorCopy( ent->mins, ent->client->history[i].mins );
		VectorCopy( ent->maxs, ent->client->history[i].maxs );
		VectorCopy( ent->s.origin, ent->client->history[i].currentOrigin );
		ent->client->history[i].leveltime = time;
	}
}


/*
============
G_StoreHistory

Keep track of where the client's been
============
*/
void G_StoreHistory( edict_t *ent ) {
	int		head;

	ent->client->historyHead++;
	if ( ent->client->historyHead >= NUM_CLIENT_HISTORY ) {
		ent->client->historyHead = 0;
	}

	head = ent->client->historyHead;

	// store all the collision-detection info and the time
	VectorCopy( ent->mins, ent->client->history[head].mins );
	VectorCopy( ent->maxs, ent->client->history[head].maxs );
	VectorCopy( ent->s.origin, ent->client->history[head].currentOrigin );
	SnapVector( ent->client->history[head].currentOrigin ); 
	ent->client->history[head].leveltime = level.time;
}


/*
=============
TimeShiftLerp

Used below to interpolate between two previous vectors
Returns a vector "frac" times the distance between "start" and "end"
=============
*/
static void TimeShiftLerp( float frac, vec3_t start, vec3_t end, vec3_t result ) {

	result[0] = start[0] + frac * ( end[0] - start[0] );
	result[1] = start[1] + frac * ( end[1] - start[1] );
	result[2] = start[2] + frac * ( end[2] - start[2] );
}


/*
=================
G_TimeShiftClient

Move a client back to where he was at the specified "time"
=================
*/
void G_TimeShiftClient( edict_t *ent, float time, qboolean debug, edict_t *debugger ) {
	int		j, k;

	char	str[MAX_STRING_CHARS];

	if(0) { //debug
		Com_sprintf(str, sizeof(str), "print \"head: %i, %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f time: %8.4f\n\"",
			ent->client->historyHead,
			ent->client->history[0].leveltime,
			ent->client->history[1].leveltime,
			ent->client->history[2].leveltime,
			ent->client->history[3].leveltime,
			ent->client->history[4].leveltime,
			ent->client->history[5].leveltime,
			ent->client->history[6].leveltime,
			ent->client->history[7].leveltime,
			ent->client->history[8].leveltime,
			ent->client->history[9].leveltime,
			ent->client->history[10].leveltime,
			ent->client->history[11].leveltime,
			ent->client->history[12].leveltime,
			ent->client->history[13].leveltime,
			ent->client->history[14].leveltime,
			ent->client->history[15].leveltime,
			ent->client->history[16].leveltime,
			time );
		safe_bprintf(PRINT_HIGH, "%s\n", str);
	}
	
	// find two entries in the history whose times sandwich "time"
	// assumes no two adjacent records have the same timestamp
	j = k = ent->client->historyHead;
	do {
		
		if ( ent->client->history[j].leveltime <= time )
			break;

		k = j;
		j--;
		if ( j < 0 ) {
			j = NUM_CLIENT_HISTORY - 1;
		}
	}
	while ( j != ent->client->historyHead );

	// if we got past the first iteration above, we've sandwiched (or wrapped)
	if ( j != k ) {
		// make sure it doesn't get re-saved
		if ( ent->client->saved.leveltime != level.time ) {
			// save the current origin and bounding box
			VectorCopy( ent->mins, ent->client->saved.mins );
			VectorCopy( ent->maxs, ent->client->saved.maxs );
			VectorCopy( ent->s.origin, ent->client->saved.currentOrigin );
			ent->client->saved.leveltime = level.time;
		}
	
		// if we haven't wrapped back to the head, we've sandwiched, so
		// we shift the client's position back to where he was at "time"
		if ( j != ent->client->historyHead ) {
			float	frac = (float)(time - ent->client->history[j].leveltime) /
				(float)(ent->client->history[k].leveltime - ent->client->history[j].leveltime);

			// interpolate between the two origins to give position at time index "time"
			TimeShiftLerp( frac,
				ent->client->history[j].currentOrigin, ent->client->history[k].currentOrigin,
				ent->s.origin );

			// lerp these too, just for fun (and ducking)
			TimeShiftLerp( frac,
				ent->client->history[j].mins, ent->client->history[k].mins,
				ent->mins );

			TimeShiftLerp( frac,
				ent->client->history[j].maxs, ent->client->history[k].maxs,
				ent->maxs );

			//debug
			if(1)
				safe_bprintf(PRINT_HIGH, "backward reconciliation\n");

			// this will recalculate absmin and absmax
			gi.linkentity( ent );

		
		} else {
			// we wrapped, so grab the earliest
			VectorCopy( ent->client->history[k].currentOrigin, ent->s.origin );
			VectorCopy( ent->client->history[k].mins, ent->mins );
			VectorCopy( ent->client->history[k].maxs, ent->maxs );

			if(0)
				safe_bprintf(PRINT_HIGH, "no backward reconciliation\n");

			// this will recalculate absmin and absmax
			gi.linkentity( ent );
		}
	}
}


/*
=====================
G_TimeShiftAllClients

Move ALL clients back to where they were at the specified "time",
except for "skip"
=====================
*/
void G_TimeShiftAllClients( int time, edict_t *skip ) {
	int			i;
	edict_t	*ent;
	
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;	

		if ( ent->client && ent->inuse && !ent->client->resp.spectator && ent != skip ) {
			G_TimeShiftClient( ent, time, false, skip );
		}
	}
}


/*
================
G_DoTimeShiftFor

Decide what time to shift everyone back to, and do it
================
*/
void G_DoTimeShiftFor( edict_t *ent ) {	
	
	//check this, because this will be different for alien arena for sure.
//	int wpflags[10] = { 0, 0, 2, 4, 0, 0, 8, 16, 0, 0 };
	
//	int wpflag = wpflags[ent->client->ps.weapon];
	float time;

	// don't time shift for mistakes or bots
	if ( !ent->inuse || !ent->client || ent->is_bot ) {
		return;
	}
 
	if ( g_antilag->integer > 1) { 
		// do the full lag compensation
		time = ent->client->attackTime;
	}
	else {
		// do just 50ms
		time = level.previousTime + ent->client->frameOffset; 

	}

	G_TimeShiftAllClients( time, ent );
}


/*
===================
G_UnTimeShiftClient

Move a client back to where he was before the time shift
===================
*/
void G_UnTimeShiftClient( edict_t *ent ) {
	// if it was saved
	if ( ent->client->saved.leveltime == level.time ) {
		// move it back
		VectorCopy( ent->client->saved.mins, ent->mins );
		VectorCopy( ent->client->saved.maxs, ent->maxs );
		VectorCopy( ent->client->saved.currentOrigin, ent->s.origin );
		ent->client->saved.leveltime = 0;

		// this will recalculate absmin and absmax
		gi.linkentity( ent );
	}
}


/*
=======================
G_UnTimeShiftAllClients

Move ALL the clients back to where they were before the time shift,
except for "skip"
=======================
*/
void G_UnTimeShiftAllClients( edict_t *skip ) {
	int			i;
	edict_t	*ent;

	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		if ( ent->client && ent->inuse && !ent->client->resp.spectator && ent != skip ) {
			G_UnTimeShiftClient( ent );
		}
	}
}


/*
==================
G_UndoTimeShiftFor

Put everyone except for this client back where they were
==================
*/
void G_UndoTimeShiftFor( edict_t *ent ) {

	// don't un-time shift for mistakes or bots
	if ( !ent->inuse || !ent->client || (ent->is_bot) ) {
		return;
	}

	G_UnTimeShiftAllClients( ent );
}
