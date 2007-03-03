#include "g_local.h"

void DeathcamTrack (edict_t *ent);

/*  The ent is the owner of the chasecam  */
void DeathcamStart (edict_t *ent)
{
	
	/* This creates a tempory entity we can manipulate within this
	 * function */
	edict_t      *chasecam;
        
	/* Tell everything that looks at the toggle that our chasecam is on
	 * and working */
	ent->client->chasetoggle = 1;

    /* Make out gun model "non-existent" so it's more realistic to the
     * player using the chasecam */
    ent->client->ps.gunindex = 0;
        
    chasecam = G_Spawn ();
    chasecam->owner = ent;
    chasecam->solid = SOLID_NOT;
    chasecam->movetype = MOVETYPE_FLYMISSILE;

	/* Now, make the angles of the player model, (!NOT THE HUMAN VIEW!) be
     * copied to the same angle of the chasecam entity */
    VectorCopy (ent->s.angles, chasecam->s.angles);
        
    /* Clear the size of the entity, so it DOES technically have a size,
     * but that of '0 0 0'-'0 0 0'. (xyz, xyz). mins = Minimum size,
     * maxs = Maximum size */
    VectorClear (chasecam->mins);
    VectorClear (chasecam->maxs);
	VectorClear (chasecam->velocity);
        
    /* Make the chasecam's origin (position) be the same as the player
     * entity's because as the camera starts, it will force itself out
     * slowly backwards from the player model */
     VectorCopy (ent->s.origin, chasecam->s.origin);
	 VectorCopy (ent->s.origin, chasecam->death_origin);
       
     chasecam->classname = "chasecam";
     chasecam->nextthink = level.time + 0.100;
     chasecam->think = DeathcamTrack;
	 ent->client->chasecam = chasecam;     
     ent->client->oldplayer = G_Spawn();
	    
}

void DeathcamRemove (edict_t *ent, char *opt)
{
        
   	ent->client->chasetoggle = 0;

    /* Stop the chasecam from moving */
    VectorClear (ent->client->chasecam->velocity);
    
	G_FreeEdict (ent->client->oldplayer);
    G_FreeEdict (ent->client->chasecam);
        
}

/* The "ent" is the chasecam */   
void DeathcamTrack (edict_t *ent)
{


	trace_t      tr;
    vec3_t       spot1, spot2;
    vec3_t       forward, right, up;

    ent->nextthink = level.time + 0.100;
       
	AngleVectors (ent->s.angles, forward, right, up);

	//JKD - 7/16/06 - tweaked this slightly so that it's a little closer to the body
    /* find position for camera to end up */
    VectorMA (ent->death_origin, -150, forward, spot1);

    /* Move camera destination up slightly too */
	spot1[2] += 30; 
    
	/* Make sure we don't go outside the level */
	tr = gi.trace (ent->s.origin, NULL, NULL, spot1, ent, false);
        
    /* subtract the endpoint from the start point for length and
     * direction manipulation */
    VectorSubtract (tr.endpos, ent->s.origin, spot2);

	/* Use this to modify camera velocity */
	VectorCopy(spot2, ent->velocity);
               
}


void CheckDeathcam_Viewent (edict_t *ent)
{
	gclient_t       *cl;
    
	if (!ent->client->oldplayer->client)
    {
        cl = (gclient_t *) malloc(sizeof(gclient_t));
        ent->client->oldplayer->client = cl;
    }

    if (ent->client->oldplayer)
    {
		ent->client->oldplayer->s.frame = ent->s.frame;
		/* Copy the origin, the speed, and the model angle, NOT
         * literal angle to the display entity */
        VectorCopy (ent->s.origin, ent->client->oldplayer->s.origin);
        VectorCopy (ent->velocity, ent->client->oldplayer->velocity);
        VectorCopy (ent->s.angles, ent->client->oldplayer->s.angles);
     }
	 ent->client->oldplayer->s = ent->s;

	 gi.linkentity (ent->client->oldplayer);
        
}
