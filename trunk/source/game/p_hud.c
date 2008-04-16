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
#include "g_local.h"



/*
======================================================================

INTERMISSION

======================================================================
*/

void MoveClientToIntermission (edict_t *ent)
{
	if (deathmatch->value)
		ent->client->showscores = true;
	VectorCopy (level.intermission_origin, ent->s.origin);
	ent->client->ps.pmove.origin[0] = level.intermission_origin[0]*8;
	ent->client->ps.pmove.origin[1] = level.intermission_origin[1]*8;
	ent->client->ps.pmove.origin[2] = level.intermission_origin[2]*8;
	VectorCopy (level.intermission_angle, ent->client->ps.viewangles);

	ent->client->ps.pmove.pm_type = PM_FREEZE;
	ent->client->ps.gunindex = 0;
	ent->client->ps.blend[3] = 0;
	ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	// clean up powerup info
	ent->client->quad_framenum = 0;
	ent->client->invincible_framenum = 0;
	ent->client->haste_framenum = 0;
    ent->client->sproing_framenum = 0;
	ent->client->grenade_blew_up = false;
	ent->client->grenade_time = 0;

	ent->viewheight = 0;
	ent->s.modelindex = 0;
	ent->s.modelindex2 = 0;
	ent->s.modelindex3 = 0;
	ent->s.modelindex = 0;
	ent->s.effects = 0;
	ent->s.sound = 0;
	ent->solid = SOLID_NOT;

	// add the layout

	if (deathmatch->value)
	{
		if(g_mapvote->value)
			DeathmatchScoreboardMessage (ent, NULL, true);
		else
			DeathmatchScoreboardMessage (ent, NULL, false);
		gi.unicast (ent, true);
	}

}
void dumb_think(edict_t *ent) {

	//just a generic think
    
    ent->nextthink = level.time + 0.100;   
}
void PlaceWinnerOnVictoryPad(edict_t *winner, int offset) 
{
	edict_t *pad;
	edict_t *chasecam;
	gclient_t *cl;
	vec3_t forward, right, movedir, origin;
	int zoffset; //for moving dead players to the right place

	if(winner->deadflag == DEAD_DEAD)
		zoffset = -40;
	else  {
		zoffset = 0;
		if(winner->in_vehicle)
			Reset_player(winner);
	}

	VectorCopy (level.intermission_angle, winner->s.angles);
	
	//move it infront of everyone
	AngleVectors (level.intermission_angle, forward, right, NULL);

	VectorMA (level.intermission_origin, 100+abs(offset), forward, winner->s.origin);
	VectorMA (winner->s.origin, offset, right, winner->s.origin);
	winner->s.origin[2] +=8;
	
	winner->client->ps.pmove.origin[0] = winner->s.origin[0];
	winner->client->ps.pmove.origin[1] = winner->s.origin[1];
	winner->client->ps.pmove.origin[2] = winner->s.origin[2];

	if (deathmatch->value)
		winner->client->showscores = true;
	
	winner->client->ps.gunindex = 0;
	winner->client->ps.pmove.pm_type = PM_FREEZE;
	winner->client->ps.blend[3] = 0;
	winner->client->ps.rdflags &= ~RDF_UNDERWATER;

	// clean up powerup info
	winner->client->quad_framenum = 0;
	winner->client->invincible_framenum = 0;
	winner->client->haste_framenum = 0;
    winner->client->sproing_framenum = 0;
	winner->client->grenade_blew_up = false;
	winner->client->grenade_time = 0;

	winner->s.effects = EF_ROTATE;
	winner->s.renderfx = (RF_FULLBRIGHT | RF_GLOW | RF_NOSHADOWS);
	
	winner->s.sound = 0;
	winner->solid = SOLID_NOT;
	
	// add the layout

	if (deathmatch->value)
	{
		if(g_mapvote->value)
			DeathmatchScoreboardMessage (winner, NULL, true);
		else
			DeathmatchScoreboardMessage (winner, NULL, false);
		gi.unicast (winner, true);
	}

	//create a new entity for the pad
	pad = G_Spawn();
	VectorMA (winner->s.origin, 8, right, pad->s.origin);	
	VectorCopy (level.intermission_angle, pad->s.angles);

	pad->s.origin[2] -= 8;
	pad->movetype = MOVETYPE_NONE;
	pad->solid = SOLID_NOT;
	pad->s.renderfx = (RF_FULLBRIGHT | RF_GLOW | RF_NOSHADOWS);
	pad->s.modelindex = gi.modelindex("models/objects/dmspot/tris.md2");
	pad->think = NULL;
	pad->classname = "pad";
	gi.linkentity (pad);

	movedir[0] = movedir[1] = 0;
	movedir[2] = -1;
	VectorCopy(pad->s.origin, origin);
	origin[2] -= 24;

	//if map is going to repeat - don't put these here as we have no way to remove them 
	//if map is not reloaded
	if(strcmp(level.mapname, level.changemap)) {
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_STEAM);
		gi.WriteByte (100);
		gi.WritePosition (origin);
		gi.WriteDir (movedir);
		gi.WriteByte (0);
		gi.multicast (origin, MULTICAST_PVS);
	}

	//now we want to allow the winners to actually see themselves on the podium, creating a
	//simple chasecam

	winner->s.origin[2] += zoffset;

	if(winner->is_bot) { //who cares if bots can not see themselves!
		winner->takedamage = DAMAGE_NO; //so they stop burning and suiciding
		gi.linkentity(winner); //link because we changed position!
		return;
	}

    winner->client->chasetoggle = 1;
       
    chasecam = G_Spawn ();
    chasecam->owner = winner;
    chasecam->solid = SOLID_NOT;
    chasecam->movetype = MOVETYPE_FLYMISSILE;

	VectorCopy (level.intermission_angle, chasecam->s.angles);

	VectorClear (chasecam->mins);
    VectorClear (chasecam->maxs);
    
	VectorCopy (level.intermission_origin, chasecam->s.origin);
       
    chasecam->classname = "chasecam";
    chasecam->think = NULL;
	winner->client->chasecam = chasecam;     
    winner->client->oldplayer = G_Spawn();

	if (!winner->client->oldplayer->client)
    {
        cl = (gclient_t *) malloc(sizeof(gclient_t));
        winner->client->oldplayer->client = cl;
/*        printf("+++ Podiumcam = %p\n", winner->client->oldplayer->client); */
    }

    if (winner->client->oldplayer)
    {
		winner->client->oldplayer->s.frame = winner->s.frame;
	    VectorCopy (winner->s.origin, winner->client->oldplayer->s.origin);
		VectorCopy (winner->s.angles, winner->client->oldplayer->s.angles);
    }
	winner->client->oldplayer->s = winner->s;

	gi.linkentity (winner->client->oldplayer);

	 //move the winner back to the intermission point
	VectorCopy (level.intermission_origin, winner->s.origin);
	winner->client->ps.pmove.origin[0] = level.intermission_origin[0]*8;
	winner->client->ps.pmove.origin[1] = level.intermission_origin[1]*8;
	winner->client->ps.pmove.origin[2] = level.intermission_origin[2]*8;
	VectorCopy (level.intermission_angle, winner->client->ps.viewangles);

	winner->s.modelindex = 0;
	winner->s.modelindex2 = 0;
	winner->s.modelindex3 = 0;
	winner->s.modelindex = 0;
	winner->s.effects = 0;
	winner->s.sound = 0;
	winner->solid = SOLID_NOT;
}

void BeginIntermission (edict_t *targ)
{
	int		i;
	edict_t	*ent, *client;
	edict_t *winner = NULL;
	edict_t *firstrunnerup = NULL;
	edict_t *secondrunnerup = NULL;
	edict_t *cl_ent;
	int high_score, low_score;

	winner = NULL;
	firstrunnerup = NULL;
	secondrunnerup = NULL;

	if (level.intermissiontime)
		return;		// already activated

	if (((int)(dmflags->value) & DF_BOT_AUTOSAVENODES)) 
			ACECM_Store(); //store the nodes automatically when changing levels.

	game.autosaved = false;

	// respawn any dead clients
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		if (client->health <= 0) {
			respawn(client);
			client->deadflag = DEAD_DEAD; //so we can know if he's dead for placement offsetting
		}
		if(!client->is_bot)
			safe_centerprintf(client, "Type \"vote #\" to vote for next map!");
	}

	level.intermissiontime = level.time;
	level.changemap = targ->map;

	level.exitintermission = 0;

	// find an intermission spot
	ent = G_Find (NULL, FOFS(classname), "info_player_intermission");
	if (!ent)
	{	// the map creator forgot to put in an intermission point...
		ent = G_Find (NULL, FOFS(classname), "info_player_start");
		if (!ent)
			ent = G_Find(NULL, FOFS(classname), "info_player_blue");
		if (!ent)
			ent = G_Find (NULL, FOFS(classname), "info_player_deathmatch");
	}
	else
	{	// chose one of four spots
		i = rand() & 3;
		while (i--)
		{
			ent = G_Find (ent, FOFS(classname), "info_player_intermission");
			if (!ent)	// wrap around the list
				ent = G_Find (ent, FOFS(classname), "info_player_intermission");
		}
	}

	VectorCopy (ent->s.origin, level.intermission_origin);
	VectorCopy (ent->s.angles, level.intermission_angle);
		
	low_score = 0;
	//get the lowest score in the game, and use that as the base high score to start
	for (i=0; i<game.maxclients; i++) {
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || game.clients[i].resp.spectator)
			continue;
		if(game.clients[i].resp.score <= low_score)
			low_score = game.clients[i].resp.score;
	}

	//get the winning player's info
	high_score = low_score;
	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || game.clients[i].resp.spectator)
			continue;

		if(game.clients[i].resp.score >= high_score) {
			winner = cl_ent;
			high_score = game.clients[i].resp.score;
		}

	}
	//get the first runner up's info
	high_score = low_score;
	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || game.clients[i].resp.spectator || (cl_ent == winner))
			continue;

		if(game.clients[i].resp.score >= high_score) {
			firstrunnerup = cl_ent;
			high_score = game.clients[i].resp.score;
		}

	}
	//get the second runner up's info
	high_score = low_score;
	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || game.clients[i].resp.spectator || (cl_ent == winner) ||
			(cl_ent == firstrunnerup))
			continue;

		if(game.clients[i].resp.score >= high_score) {
			secondrunnerup = cl_ent;
			high_score = game.clients[i].resp.score;
		}
	}
	
	if(!winner)
		winner = g_edicts;
	if(!firstrunnerup)
		firstrunnerup = g_edicts;
	if(!secondrunnerup)
		secondrunnerup = g_edicts;

	// move all clients but the winners to the intermission point
	for (i=0 ; i<maxclients->value ; i++)
	{
		client = g_edicts + 1 + i;
		if (!client->inuse)
			continue;
		if((client != winner) && (client != firstrunnerup) && (client != secondrunnerup))
			MoveClientToIntermission (client);
	}

	if (!((int)(dmflags->value) & DF_BOT_LEVELAD)) {
			if ((!((int)(dmflags->value) & DF_SKINTEAMS)) && !(ctf->value || tca->value || cp->value)) { //don't do this in team play
				if(winner->is_bot)
					gi.sound (ent, CHAN_AUTO, gi.soundindex("world/botwon.wav"), 1, ATTN_NONE, 0);
				else				
					gi.sound (winner, CHAN_AUTO, gi.soundindex("world/youwin.wav"), 1, ATTN_STATIC, 0);
			}
	}

	if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) //team stuff
	{
		if(blue_team_score > red_team_score) {
			if(ctf->value || tca->value || cp->value)
				gi.sound (client, CHAN_AUTO, gi.soundindex("misc/blue_wins_ctf.wav"), 1, ATTN_NONE, 0);
			else
				gi.sound (client, CHAN_AUTO, gi.soundindex("misc/blue_wins.wav"), 1, ATTN_NONE, 0);
		}
		else {
			if(ctf->value || tca->value || cp->value)
				gi.sound (client, CHAN_AUTO, gi.soundindex("misc/red_wins_ctf.wav"), 1, ATTN_NONE, 0);
			
			else
				gi.sound (client, CHAN_AUTO, gi.soundindex("misc/red_wins.wav"), 1, ATTN_NONE, 0);
		}
	}
	
	//place winner on victory pads, ala Q3
	if(winner && winner->client && winner->inuse)
		PlaceWinnerOnVictoryPad(winner, 0);
	if(firstrunnerup && firstrunnerup->client && firstrunnerup->inuse)
		PlaceWinnerOnVictoryPad(firstrunnerup, 32);
	if(secondrunnerup && secondrunnerup->client && secondrunnerup->inuse)
		PlaceWinnerOnVictoryPad(secondrunnerup, -32);

}

//duel code
int highestpos, numplayers;
void MoveEveryoneDownQueue(void) {
	int i, induel = 0;

	for (i = 0; i < maxclients->value; i++) {
		if(g_edicts[i+1].inuse && g_edicts[i+1].client) {
			//move everyone else down a notch(never less than 0)
			if(induel > 1 && g_edicts[i+1].client->pers.queue <= 3) //houston, we have a problem
				return; //abort, stop moving people
			if(g_edicts[i+1].client->pers.queue > 1) 
					g_edicts[i+1].client->pers.queue-=1; 
			if(g_edicts[i+1].client->pers.queue < 3)
				induel++;
		}		
	}
}
void CheckDuelWinner(void) {
	int	i;
	int highscore, induel;

	highscore = 0;
	numplayers = 0;
	highestpos = 0;
	induel = 0;

	for (i = 0; i < maxclients->value; i++) {
		if(g_edicts[i+1].inuse && g_edicts[i+1].client) { 
			if(g_edicts[i+1].client->resp.score > highscore)
				highscore = g_edicts[i+1].client->resp.score;
			if(g_edicts[i+1].client->pers.queue > highestpos)
				highestpos = g_edicts[i+1].client->pers.queue;
			numplayers++;
		}
	}
	if(highestpos < numplayers)
		highestpos = numplayers;

	for (i = 0; i < maxclients->value; i++) {
		if(g_edicts[i+1].inuse && g_edicts[i+1].client) {
			if((g_edicts[i+1].client->resp.score < highscore) && g_edicts[i+1].client->pers.queue < 3) {
				g_edicts[i+1].client->pers.queue = highestpos+1; //loser, kicked to the back of the line
				highestpos++; 
			}
		}
	}

	MoveEveryoneDownQueue();

	//last resort checking after positions have changed
	//check for any screwups and correct the queue
	while(induel < 2 && numplayers > 1) {
		induel = 0;
		for (i = 0; i < maxclients->value; i++) {
			if(g_edicts[i+1].inuse && g_edicts[i+1].client) {
				if(g_edicts[i+1].client->pers.queue < 3 && g_edicts[i+1].client->pers.queue)
					induel++;			
			}
		}
		if(induel < 2) //something is wrong(i.e winner left), move everyone down.
			MoveEveryoneDownQueue();
	}
}

void EndIntermission(void)
{
	int		i;
	edict_t	*ent;

	if(g_duel->value)
		CheckDuelWinner();

	for (i=0 ; i<maxclients->value; i++)
	{
		ent = g_edicts + 1 + i;
        if (!ent->inuse || ent->client->resp.spectator)
            continue;

        if(!ent->is_bot && ent->client->chasetoggle > 0)
        {
            ent->client->chasetoggle = 0;
            /* Stop the chasecam from moving */
            VectorClear (ent->client->chasecam->velocity);

            if(ent->client->oldplayer->client != NULL)
            {
/*                printf("--- Podiumcam = %p\n", ent->client->oldplayer->client);*/
                free(ent->client->oldplayer->client);
            }

            G_FreeEdict (ent->client->oldplayer);
            G_FreeEdict (ent->client->chasecam);
        }

    }

}

/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage (edict_t *ent, edict_t *killer, int mapvote)
{
	char	entry[1024];
	char	string[1400];
	int		stringlength;
	int		i, j, k;
	int		sorted[MAX_CLIENTS];
	int		sortedscores[MAX_CLIENTS];
	int		score, total;
	int		x, y;
	gclient_t	*cl;
	edict_t		*cl_ent;
	char    acc[16];
	char	weapname[16];

	if (ent->is_bot)
		return;

	if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) {
		CTFScoreboardMessage (ent, killer, mapvote);
		return;
	}
	// sort the clients by score
	total = 0;
	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || (!g_duel->value && game.clients[i].resp.spectator))
			continue;

		score = game.clients[i].resp.score;
		for (j=0 ; j<total ; j++)
		{
			if (score > sortedscores[j])
				break;
		}
		for (k=total ; k>j ; k--)
		{
			sorted[k] = sorted[k-1];
			sortedscores[k] = sortedscores[k-1];
		}
		sorted[j] = i;
		sortedscores[j] = score;
		total++;
	}
	
	// print level name and exit rules
	string[0] = 0;
	
	stringlength = strlen(string);
	
	// add the clients in sorted order
	if (total > 12)
		total = 12;

	for (i=0 ; i<total ; i++)
	{
		cl = &game.clients[sorted[i]];
		cl_ent = g_edicts + 1 + sorted[i];

		x = (i>=6) ? 160 : 0;
		y = 32 + 32 * (i%6);

		// add a background
		Com_sprintf (entry, sizeof(entry),
			"xv %i yv %i picn %s ",x+32, y, "tag2");
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
		
		// send the layout
		if(!cl->resp.spectator)
			Com_sprintf (entry, sizeof(entry),
				"client %i %i %i %i %i %i ",
				x, y, sorted[i], cl->resp.score, cl->ping, (level.framenum - cl->resp.enterframe)/600);
		else //duel mode will have queued spectators
			Com_sprintf (entry, sizeof(entry),
				"queued %i %i %i %i %i %i ",
				x, y, sorted[i], cl->resp.score, cl->ping, cl->pers.queue-2);
	
		
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
	}

	//weapon accuracy

	//add a background
	x = 0;
	y = (total>=6) ? (32+(32*5)) : (32*total);
	for (i=0 ; i<3 ; i++)
	{
	
		Com_sprintf (entry, sizeof(entry),
			"xv %i yv %i picn %s ", x, y+((i+1)*32+24), "tag2");
		j = strlen(entry);
		if (stringlength + j > 1024)
			break;
		strcpy (string + stringlength, entry);
		stringlength += j;
		
		// send the layout
		if (stringlength + j > 1024)
			break;
	}

	x = 0;
	y = (total>=6) ? (32+(32*5)) : (32*total);
	Com_sprintf(entry, sizeof(entry),
		"xv %i yv %i string Accuracy ", x, y+56);
	j = strlen(entry);
	if(stringlength + j < 1024) {
		strcpy(string + stringlength, entry);
		stringlength +=j;
	}
	for(i = 0; i < 9; i++) {

		//in case of scenarios where a weapon is scoring multiple hits per shot(ie smartgun)
		if(ent->client->resp.weapon_hits[i] > ent->client->resp.weapon_shots[i])
			ent->client->resp.weapon_hits[i] = ent->client->resp.weapon_shots[i];

		if(ent->client->resp.weapon_shots[i] == 0)
			strcpy(acc, "0%");
		else {
			sprintf(acc, "%i", (100*ent->client->resp.weapon_hits[i]/ent->client->resp.weapon_shots[i]));
			strcat(acc, "%");
		}
		switch (i) {
			case 0:
				strcpy(weapname, "blaster");
				break;
			case 1:
				strcpy(weapname, "disruptor");
				break;
			case 2:
				strcpy(weapname, "smartgun");
				break;
			case 3:
				strcpy(weapname, "chaingun");
				break;
			case 4:
				strcpy(weapname, "flame");
				break;
			case 5:
				strcpy(weapname, "rocket");
				break;
			case 6:
				strcpy(weapname, "beamgun");
				break;
			case 7:
				strcpy(weapname, "vaporizer");
				break;
			case 8:
				strcpy(weapname, "violator");
				break;
		}
		Com_sprintf(entry, sizeof(entry),
			"xv %i yv %i string %s xv %i string %s ", x, y+((i+1)*9)+64, weapname, x+96, acc);
		j = strlen(entry);
		if(stringlength + j < 1024) {
			strcpy(string + stringlength, entry);
			stringlength +=j;
		}
	}
	//map voting
	if(mapvote) {
		y = 64;
		x = 0;
		Com_sprintf(entry, sizeof(entry), 
			"xv %i yt %i string Vote.for.next.map: ", x, y);
		j = strlen(entry);
		if(stringlength + j < 1024) {
			strcpy(string + stringlength, entry);
			stringlength +=j;
		}
		for(i=0; i<4; i++) {
			
			Com_sprintf(entry, sizeof(entry), 
			"xv %i yt %i string %i.%s ", x, y+((i+1)*9)+9, i+1, votedmap[i].mapname);
			j = strlen(entry);
			if(stringlength + j < 1024) {
				strcpy(string + stringlength, entry);
				stringlength +=j;
			}
		}

	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}


/*
==================
DeathmatchScoreboard

Draw instead of help message.
Note that it isn't that hard to overflow the 1400 byte message limit!
==================
*/
void DeathmatchScoreboard (edict_t *ent)
{
// ACEBOT_ADD
	if (ent->is_bot)
		return;
// ACEBOT_END

	DeathmatchScoreboardMessage (ent, ent->enemy, false);
	gi.unicast (ent, true);
}


/*
==================
Cmd_Score_f

Display the scoreboard
==================
*/
void Cmd_Score_f (edict_t *ent)
{
	ent->client->showinventory = false;
	ent->client->showhelp = false;

	if (!deathmatch->value)
		return;

	if (ent->client->showscores)
	{
		ent->client->showscores = false;
		return;
	}

	ent->client->showscores = true;
	DeathmatchScoreboard (ent);
}


/*
==================
HelpComputer

Draw help computer.
==================
*/
void HelpComputer (edict_t *ent)
{
	char	string[1024];
	char	*sk;

// ACEBOT_ADD
	if (ent->is_bot)
		return;
// ACEBOT_END

	if (skill->value == 0)
		sk = "easy";
	else if (skill->value == 1)
		sk = "medium";
	else if (skill->value == 2)
		sk = "hard";
	else
		sk = "hard+";
	//We are going to simplify this layout considerably.
	// send the layout
	Com_sprintf (string, sizeof(string),
		"xv -32 yv 0 picn help "			// background
//		"xv 202 yv 12 string2 \"%s\" "		// skill
//		"xv 0 yv 24 cstring2 \"%s\" "		// level name
		"xv 0 yv 54 cstring2 \"%s\" ",		// help 1
//		"xv 0 yv 110 cstring2 \"%s\" "		// help 2
//		"xv 50 yv 164 string2 \" kills     goals    secrets\" "
//		"xv 50 yv 172 string2 \"%3i/%3i     %i/%i       %i/%i\" ", 
//		sk,
//		level.level_name,
		game.helpmessage1);
//		game.helpmessage2,
//		level.killed_monsters, level.total_monsters, 
//		level.found_goals, level.total_goals,
//		level.found_secrets, level.total_secrets);

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
	gi.unicast (ent, true);
}


/*
==================
Cmd_Help_f

Display the current help message
==================
*/
void Cmd_Help_f (edict_t *ent)
{
	// this is for backwards compatability
	if (deathmatch->value)
	{
		Cmd_Score_f (ent);
		return;
	}

	ent->client->showinventory = false;
	ent->client->showscores = false;

	if (ent->client->showhelp && (ent->client->pers.game_helpchanged == game.helpchanged))
	{
		ent->client->showhelp = false;
		return;
	}

	ent->client->showhelp = true;
	ent->client->pers.helpchanged = 0;
	HelpComputer (ent);
}


//=======================================================================

/*
===============
G_SetStats
===============
*/
void G_SetStats (edict_t *ent)
{
	gitem_t		*item;
	int			index;
	edict_t *e2;
	int i, j;
	int high_score = 0;
	gitem_t *flag1_item, *flag2_item;

	
	flag1_item = FindItemByClassname("item_flag_red");
	flag2_item = FindItemByClassname("item_flag_blue");

	//
	// health
	//
	ent->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
	ent->client->ps.stats[STAT_HEALTH] = ent->health;

	//
	// ammo
	//
	if (!ent->client->ammo_index /* || !ent->client->pers.inventory[ent->client->ammo_index] */)
	{
		ent->client->ps.stats[STAT_AMMO_ICON] = 0;
		ent->client->ps.stats[STAT_AMMO] = 0;
	}
	else
	{
		item = &itemlist[ent->client->ammo_index];
		ent->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex (item->icon);
		ent->client->ps.stats[STAT_AMMO] = ent->client->pers.inventory[ent->client->ammo_index];
	}
	
	//
	// armor
	//

	index = ArmorIndex (ent);
	if (index)
	{
		item = GetItemByIndex (index);
		ent->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex (item->icon);
		ent->client->ps.stats[STAT_ARMOR] = ent->client->pers.inventory[index];
	}
	else
	{
		ent->client->ps.stats[STAT_ARMOR_ICON] = 0;
		ent->client->ps.stats[STAT_ARMOR] = 0;
	}

	//
	// pickup message
	//
	if (level.time > ent->client->pickup_msg_time)
	{
		ent->client->ps.stats[STAT_PICKUP_ICON] = 0;
		ent->client->ps.stats[STAT_PICKUP_STRING] = 0;
	}

	//
	// timers
	//
	if (ent->client->quad_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_quad");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->quad_framenum - level.framenum)/10;
	}
	else if (ent->client->invincible_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_invulnerability");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->invincible_framenum - level.framenum)/10;
	}
	else if (ent->client->haste_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_haste");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->haste_framenum - level.framenum)/10;
	}
	else if (ent->client->sproing_framenum > level.framenum)
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex ("p_sproing");
		ent->client->ps.stats[STAT_TIMER] = (ent->client->sproing_framenum - level.framenum)/10;
	}
	else
	{
		ent->client->ps.stats[STAT_TIMER_ICON] = 0;
		ent->client->ps.stats[STAT_TIMER] = 0;
	}

	//
	// selected item
	//
	if (ent->client->pers.selected_item == -1)
		ent->client->ps.stats[STAT_SELECTED_ICON] = 0;
	else
		ent->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex (itemlist[ent->client->pers.selected_item].icon);

	ent->client->ps.stats[STAT_SELECTED_ITEM] = ent->client->pers.selected_item;

	//ctf
	if(ctf->value) { 
		if (ent->client->pers.inventory[ITEM_INDEX(flag1_item)])
			ent->client->ps.stats[STAT_FLAG_ICON] = gi.imageindex ("i_flag1");
		else if (ent->client->pers.inventory[ITEM_INDEX(flag2_item)])
			ent->client->ps.stats[STAT_FLAG_ICON] = gi.imageindex ("i_flag2");
		else { //do teams
			if(ent->dmteam == RED_TEAM)
				ent->client->ps.stats[STAT_FLAG_ICON] = gi.imageindex ( "i_team1");
			else if(ent->dmteam == BLUE_TEAM)
				ent->client->ps.stats[STAT_FLAG_ICON] = gi.imageindex ( "i_team2");
			else
				ent->client->ps.stats[STAT_FLAG_ICON] = gi.imageindex ( "bar_loading");
		}
	}
	else { //do teams
		if(ent->dmteam == RED_TEAM)
			ent->client->ps.stats[STAT_FLAG_ICON] = gi.imageindex ( "i_team1");
		else if(ent->dmteam == BLUE_TEAM)
			ent->client->ps.stats[STAT_FLAG_ICON] = gi.imageindex ( "i_team2");
		else
			ent->client->ps.stats[STAT_FLAG_ICON] = gi.imageindex ( "bar_loading");
	}

	//
	// layouts
	//
	ent->client->ps.stats[STAT_LAYOUTS] = 0;

	if (deathmatch->value)
	{
		if (ent->client->pers.health <= 0 || level.intermissiontime
			|| ent->client->showscores)
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	}
	else
	{
		if (ent->client->showscores || ent->client->showhelp)
			ent->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (ent->client->showinventory && ent->client->pers.health > 0)
			ent->client->ps.stats[STAT_LAYOUTS] |= 2;
	}

	//
	// frags and deaths
	//
	ent->client->ps.stats[STAT_FRAGS] = ent->client->resp.score;
	ent->client->ps.stats[STAT_DEATHS] = ent->client->resp.deaths;

	// highest scorer
	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;
	
		if(e2->client->resp.score > high_score)
			high_score = e2->client->resp.score;
	}
	ent->client->ps.stats[STAT_HIGHSCORE] = high_score;

	//bot score info
	ent->client->ps.botnum = ent->client->resp.botnum;
	if(ent->client->resp.botnum) {
		for(i = 0; i < ent->client->resp.botnum; i++) {
			strcpy(ent->client->ps.bots[i].name, ent->client->resp.bots[i].name);
			//tally bot scores
			for(j = 0, e2 = g_edicts + 1; j < maxclients->value; j++, e2++) {
				if(!strcmp(ent->client->resp.bots[i].name, e2->client->pers.netname))
					ent->client->ps.bots[i].score = e2->client->resp.score;
			}
		}
	}
	//end bot score info

	//
	// help icon / current weapon if not shown
	//
	if (ent->client->pers.weapon)
		ent->client->ps.stats[STAT_HELPICON] = gi.imageindex (ent->client->pers.weapon->icon);
	else
		ent->client->ps.stats[STAT_HELPICON] = 0;

	ent->client->ps.stats[STAT_SPECTATOR] = 0;

	ent->client->ps.stats[STAT_SCOREBOARD] = gi.imageindex ("i_score");

	//team
	ent->client->ps.stats[STAT_REDSCORE] = red_team_score;
	ent->client->ps.stats[STAT_BLUESCORE] = blue_team_score;

	//spectator mode
	ent->client->ps.stats[STAT_SPECTATOR] = 0;
}

/*
===============
G_CheckChaseStats
===============
*/
void G_CheckChaseStats (edict_t *ent)
{
	int i;
	gclient_t *cl;

	for (i = 1; i <= maxclients->value; i++) {
		cl = g_edicts[i].client;
		if (!g_edicts[i].inuse || cl->chase_target != ent)
			continue;
		memcpy(cl->ps.stats, ent->client->ps.stats, sizeof(cl->ps.stats));
		G_SetSpectatorStats(g_edicts + i);
	}
}

/*
===============
G_SetSpectatorStats
===============
*/
void G_SetSpectatorStats (edict_t *ent)
{
	gclient_t *cl = ent->client;

	if (!cl->chase_target)
		G_SetStats (ent);

	cl->ps.stats[STAT_SPECTATOR] = 1;

	// layouts are independant in spectator
	cl->ps.stats[STAT_LAYOUTS] = 0;
	if (cl->pers.health <= 0 || level.intermissiontime || cl->showscores)
		cl->ps.stats[STAT_LAYOUTS] |= 1;
	if (cl->showinventory && cl->pers.health > 0)
		cl->ps.stats[STAT_LAYOUTS] |= 2;

	if (cl->chase_target && cl->chase_target->inuse)
		cl->ps.stats[STAT_CHASE] = CS_PLAYERSKINS + 
			(cl->chase_target - g_edicts) - 1;
	else
		cl->ps.stats[STAT_CHASE] = 0;
}

