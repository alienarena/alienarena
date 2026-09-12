/*
Copyright (C) 2009 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "g_local.h"
#include "qcommon/qcommon.h"

/*
============
G_ResetHistory

Clear out the given client's history (should be called when the teleport bit is flipped)
============
*/
void G_ResetHistory(edict_t* ent) {
	int i, time = level.leveltime;

	// fill up the history with data (assume the current position)
	ent->client->historyHead = NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE;
	for (i = ent->client->historyHead; i >= 0; --i, time -= FRAMETIME_MS) {
		clientHistory_t* history = &ent->client->history[i];

		VectorCopy(ent->mins, history->mins);
		VectorCopy(ent->maxs, history->maxs);
		VectorCopy(ent->s.origin, history->currentOrigin);
		history->leveltime = time;
	}
}


/*
============
G_StoreHistory

Keep track of where the client's been
============
*/
void G_StoreHistory(edict_t* ent) {
	int historyHead;
	clientHistory_t* history;

	++ent->client->historyHead;
	if (ent->client->historyHead > NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE) {
		ent->client->historyHead = 0;
	}
	historyHead = ent->client->historyHead;
	history = &ent->client->history[historyHead];

	// store all the collision-detection info and the time
	VectorCopy(ent->mins, history->mins);
	VectorCopy(ent->maxs, history->maxs);
	VectorCopy(ent->s.origin, history->currentOrigin);
	SnapVector(history->currentOrigin);
	history->leveltime = level.leveltime;
}


/*
=============
TimeShiftLerp

Used below to interpolate between two previous vectors
Returns a vector "frac" times the distance between "start" and "end"
=============
*/
static void TimeShiftLerp(float frac, const vec3_t start, const vec3_t end, vec3_t result) {
	int i;
	for (i = 0; i < 3; ++i) {
		result[i] = start[i] + frac * (end[i] - start[i]);
	}
}


/*
=================
G_TimeShiftClient

Move a client back to where he was at the specified time
=================
*/
void G_TimeShiftClient(edict_t *ent, int time, qboolean debug, edict_t *debugger) {
    int i, j, k;
    int failSafeCounter = 0;
	int historyHead;
	qboolean corruption_detected = false;

	// Fix for rocket funround crash/loop,
	// when time has value 0 it gets stuck in the do/while loop below.
    if (time <= 0) {
        Com_Printf("G_TimeShiftClient: time <= 0, %d, exit\n", time);
        return;
    }

    historyHead = ent->client->historyHead;
    
    // ===== CORRUPTION DETECTION: Validate historyHead bounds =====
    if (historyHead > NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE) {
		Com_Printf("G_TimeShiftClient: CORRUPTION DETECTED - historyHead %d exceeds max %d\n", 
			historyHead, NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE);
        G_ResetHistory(ent);
        return;
    }
    
    if (historyHead < 0) {
		Com_Printf("G_TimeShiftClient: CORRUPTION DETECTED - historyHead %d is negative\n", historyHead);
        G_ResetHistory(ent);
        return;
    }

	// ===== CORRUPTION DETECTION: Validate leveltime consistency =====
	// Check that leveltime values are reasonable (not wildly out of sequence)
	int leveltime_checks_passed = 0;
	int leveltime_checks_failed = 0;
	
	// Spot-check several recent history entries for monotonicity and reasonable gaps
	// Only check indices within the active buffer range (0 to NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE)
	for (i = 0; i < 3 && i <= NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE; ++i) {
		// Walk backwards in the circular buffer, staying within valid range
		int check_idx = historyHead - i;
		if (check_idx < 0) {
			check_idx += NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE + 1;
		}
		
		// Safety: don't check beyond the active buffer
		if (check_idx > NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE || check_idx < 0) {
			continue;
		}
		
		int stored_leveltime = ent->client->history[check_idx].leveltime;
		
		if (stored_leveltime > level.leveltime) {
			Com_Printf("G_TimeShiftClient: CORRUPTION DETECTED - history[%d].leveltime (%d) > current leveltime (%d)\n",
				check_idx, stored_leveltime, level.leveltime);
			corruption_detected = true;
			leveltime_checks_failed++;
		} else {
			leveltime_checks_passed++;
		}
	}
	
	if (g_antilagdebug->integer > 1) {
		Com_Printf("G_TimeShiftClient: leveltime consistency check - passed: %d, failed: %d\n",
			leveltime_checks_passed, leveltime_checks_failed);
	}

	// ===== AUTO-RESET ON CORRUPTION =====
	if (corruption_detected) {
		Com_Printf("G_TimeShiftClient: Initiating AUTO-RESET due to detected corruption for client %d\n",
			ent - g_edicts - 1);
		G_ResetHistory(ent);
		return;
    }

	// find two entries in the history whose times sandwich "time"
	// assumes no two adjacent records have the same timestamp
	j = k = historyHead;

	do {
		if (ent->client->history[j].leveltime <= time) {
			if (g_antilagdebug->integer > 0) {
				Com_Printf("G_TimeShiftClient: found time at index %d (leveltime %d <= target time %d)\n",
					j, ent->client->history[j].leveltime, time);
			}
			break;
		}

		// Exit the loop in case the number of iterations is larger than the history
		if (failSafeCounter > NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE + 1) {
			Com_Printf("G_TimeShiftClient: FAILSAFE - Counter reached %d, exiting loop. Attempting auto-reset.\n", 
				NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE + 1);
			Com_Printf("G_TimeShiftClient: Client %d - searching for time %d from historyHead %d\n",
				ent - g_edicts - 1, time, historyHead);

			// Failsafe triggered - likely corrupted state
			G_ResetHistory(ent);
			return;
		}

		k = j;
		--j;
		if (j < 0) {
			j = NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE;
		}
		++failSafeCounter;
    } while (j != historyHead);

	// if we got past the first iteration above, we've sandwiched (or wrapped)
	if (j != k) {
		if (g_antilagdebug->integer > 0) {
			Com_Printf("Head: %i, reconciled time at: %i, attacker ping: %i, client ping: %i\n",
				ent->client->historyHead, j, debugger->client->ping, ent->client->ping);
		}

		// make sure it doesn't get re-saved
		if (ent->client->saved.leveltime != level.leveltime) {
			VectorCopy(ent->mins, ent->client->saved.mins);
			VectorCopy(ent->maxs, ent->client->saved.maxs);
			VectorCopy(ent->s.origin, ent->client->saved.currentOrigin);
			ent->client->saved.leveltime = level.leveltime;
		}

		// if we haven't wrapped back to the head, we've sandwiched, so
		// we shift the client's position back to where he was at "time"
		if (j != ent->client->historyHead) {
			float frac = (float)(time - ent->client->history[j].leveltime) /
				(float)(ent->client->history[k].leveltime - ent->client->history[j].leveltime);
			
			// interpolate between the two origins to give position at time index "time"
			for (i = 0; i < 3; ++i) {
				ent->s.origin[i] = ent->client->history[j].currentOrigin[i] +
					frac * (ent->client->history[k].currentOrigin[i] - ent->client->history[j].currentOrigin[i]);

				ent->mins[i] = ent->client->history[j].mins[i] +
					frac * (ent->client->history[k].mins[i] - ent->client->history[j].mins[i]);

				ent->maxs[i] = ent->client->history[j].maxs[i] +
					frac * (ent->client->history[k].maxs[i] - ent->client->history[j].maxs[i]);
			}
			
			// this will recalculate absmin and absmax
			gi.linkentity(ent);
		} else {			
			// we wrapped, so grab the earliest
			VectorCopy(ent->client->history[k].currentOrigin, ent->s.origin);
			VectorCopy(ent->client->history[k].mins, ent->mins);
			VectorCopy(ent->client->history[k].maxs, ent->maxs);

			// this will recalculate absmin and absmax
			gi.linkentity(ent);
		}
    }
	else {
		// Could not find matching history pair - use appropriate fallback entry
		int oldest_idx = (historyHead + 1) % (NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE + 1);
		int oldest_time = ent->client->history[oldest_idx].leveltime;
		int newest_time = ent->client->history[historyHead].leveltime;
		int fallback_idx;
		
		// Choose fallback based on which side of the history window we're on
		if (time < oldest_time) {
			// Target is too old - use oldest entry
			fallback_idx = oldest_idx;
			if (g_antilagdebug->integer > 0) {
				Com_Printf("G_TimeShiftClient: target time %d is %dms TOO OLD, using oldest entry at index %d\n", 
					time, oldest_time - time, oldest_idx);
			}
		} else {
			// Target is too new - use newest entry (current position)
			fallback_idx = historyHead;
			if (g_antilagdebug->integer > 0) {
				Com_Printf("G_TimeShiftClient: target time %d is %dms TOO NEW, using newest entry at index %d\n", 
					time, time - newest_time, historyHead);
			}
		}
		
		// make sure it doesn't get re-saved
		if (ent->client->saved.leveltime != level.leveltime) {
			VectorCopy(ent->mins, ent->client->saved.mins);
			VectorCopy(ent->maxs, ent->client->saved.maxs);
			VectorCopy(ent->s.origin, ent->client->saved.currentOrigin);
			ent->client->saved.leveltime = level.leveltime;
		}
		
		// Apply fallback position
		VectorCopy(ent->client->history[fallback_idx].currentOrigin, ent->s.origin);
		VectorCopy(ent->client->history[fallback_idx].mins, ent->mins);
		VectorCopy(ent->client->history[fallback_idx].maxs, ent->maxs);

		// this will recalculate absmin and absmax
		gi.linkentity(ent);
	}
}

/*
=====================
G_TimeShiftAllClients

Move ALL clients back to where they were at the specified "time",
except for "skip"
=====================
*/
void G_TimeShiftAllClients( int time, edict_t *skip ) 
{
	int		i;
	edict_t	*ent;

	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		if (player_participating (ent) && ent != skip)
			G_TimeShiftClient (ent, time, false, skip);
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
	int time;
	int ping;
	int effectivePing;
	int thresholdHigh;
	int thresholdLow;
	int maxPing;
	float onewayFactor;
	int compensation;

	// don't time shift for mistakes or bots
	if ( !ent->inuse || !ent->client || ent->is_bot ) 
	{
		return;
	}

	ping = ent->client->ping;
	thresholdHigh = g_antilag_high_ping_threshold ? g_antilag_high_ping_threshold->integer : DEFAULT_ANTILAG_HIGH_PING_THRESHOLD;
	thresholdLow = g_antilag_low_ping_threshold ? g_antilag_low_ping_threshold->integer : DEFAULT_ANTILAG_LOW_PING_THRESHOLD;
	maxPing = g_antilag_max_ping ? g_antilag_max_ping->integer : DEFAULT_ANTILAG_MAX_PING;
	onewayFactor = g_antilag_oneway_factor ? g_antilag_oneway_factor->value : DEFAULT_ONEWAY_FACTOR;

	if (ping < thresholdHigh) {
		effectivePing = ping;
	} else {
		// Diminishing returns: full compensation below threshold, then 50% of ping above that
		effectivePing = thresholdHigh + ((ping - thresholdHigh) >> 1);  // >> 1 is faster than / 2
	}

	// One-way ping is approximately half of RTT (round trip time)
	if (effectivePing < thresholdLow) {
		// Use a low-ping threshold to keep fairness, stability and to limit ping inflation abuse
		compensation = thresholdLow * onewayFactor;
	} else {
		compensation = effectivePing * onewayFactor;
	}

	time = ent->client->attackTime - compensation;

	if (g_antilagdebug->integer > 0) {
		Com_Printf("leveltime: %i, raw ping: %i, effectivePing: %i, attackTime: %i, low ping threshold: %i, compensation: %i, corrected time: %i\n",
			level.leveltime, ping, effectivePing, ent->client->attackTime, thresholdLow, compensation, time);
	}

	G_TimeShiftAllClients( time, ent );
}


/*
===================
G_UnTimeShiftClient

Move a client back to where he was before the time shift
===================
*/
void G_UnTimeShiftClient( edict_t *ent ) 
{
	// if it was saved
	if ( ent->client->saved.leveltime == level.leveltime ) {
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
void G_UnTimeShiftAllClients( edict_t *skip ) 
{
	int 	i;
	edict_t	*ent;

	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		if (player_participating (ent) && ent != skip)
			G_UnTimeShiftClient (ent);
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
	if ( !ent->inuse || !ent->client || ent->is_bot ) 
	{
		return;
	}

	G_UnTimeShiftAllClients( ent );
}


/*
==================
G_AntilagProjectile

Simulate any extra frames to get the projectile "caught up" on the current
state of the game. 
==================
*/
void G_AntilagProjectile(edict_t* ent) {
	edict_t *owner;
	int frameTime = FRAMETIME_MS;
	int rawPing;
	int effectivePing;
	int time;
	int thresholdHigh;
	int thresholdLow;
	int maxPing;
	float onewayFactor;

	// Save a copy of the player who fired the shot. The reason not to refer
	// to ent->owner directly is because if the projectile hits something,
	// its contents will be cleared during the call to G_RunEntity.
	owner = ent->owner;

	// don't antilag mistakes or bots
	if ( !ent || !ent->inuse || !owner || !owner->inuse || !owner->client || owner->is_bot ) 
	{
		return;
	}

	rawPing = ent->owner->client->ping;
	thresholdHigh = g_antilag_high_ping_threshold ? g_antilag_high_ping_threshold->integer : DEFAULT_ANTILAG_HIGH_PING_THRESHOLD;
	thresholdLow = g_antilag_low_ping_threshold ? g_antilag_low_ping_threshold->integer : DEFAULT_ANTILAG_LOW_PING_THRESHOLD;
	maxPing = g_antilag_max_ping ? g_antilag_max_ping->integer : DEFAULT_ANTILAG_MAX_PING;
	onewayFactor = g_antilag_oneway_factor ? g_antilag_oneway_factor->value : DEFAULT_ONEWAY_FACTOR;

	if (rawPing < thresholdHigh) {
		effectivePing = rawPing;
	} else {
		// Diminishing returns: full compensation below threshold, then 50% of ping above that
		effectivePing = thresholdHigh + ((rawPing - thresholdHigh) >> 1);  // >> 1 is faster than / 2
	}

	// cap at maximum effective ping
	if (effectivePing > maxPing) {
		effectivePing = maxPing;
	}

	// One-way ping is approximately half of RTT (round trip time)
	if (effectivePing < thresholdLow) {
		// Use a low-ping threshold to keep fairness, stability and to limit ping inflation abuse
		time = ent->owner->client->attackTime - (thresholdLow * onewayFactor);
	} else {
		time = ent->owner->client->attackTime - (effectivePing * onewayFactor);
	}
	// }

	// Handle the full lag compensation frames
	while (effectivePing > frameTime) {
		// We're using one-way ping estimation, so don't subtract frameTime
		if (g_antilagdebug->integer > 0)
		{
			Com_Printf("Full lag compensation, raw ping %d, effective %d, time %d\n", rawPing, effectivePing, time);
		}		
		G_TimeShiftAllClients(time, owner);
		G_RunEntity(ent, FRAMETIME); // Simulate the projectile for one frame
		G_UnTimeShiftAllClients(owner);
		if (!ent->inuse)
		{
			return;
		}

		effectivePing -= frameTime;
	}

	// Handle the remaining lag compensation (if any)
	if (effectivePing > 0) {
		time -= effectivePing;
		if (g_antilagdebug->integer > 0)
		{
			Com_Printf("Remaining lag compensation, raw ping %d, effective %d, time %d\n", rawPing, effectivePing, time);
		}		
		G_TimeShiftAllClients(time, owner);
		G_RunEntity(ent, effectivePing / 1000.0f); // Convert ping to seconds for the final frame
		G_UnTimeShiftAllClients(owner);
	}
}
