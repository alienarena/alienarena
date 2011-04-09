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

#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if !defined HAVE__STRDUP
#define _strdup strdup
#endif

#include "g_local.h"

game_locals_t	game;
level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
spawn_temp_t	st;
g_vote_t		playervote;

int	sm_meat_index;
int meansOfDeath;
int bot_won;

char *winningmap;

edict_t		*g_edicts;

cvar_t	*deathmatch;
cvar_t  *ctf;
cvar_t  *tca;
cvar_t  *cp;
//mutators
cvar_t  *instagib;
cvar_t  *rocket_arena;
cvar_t  *low_grav;
cvar_t  *regeneration;
cvar_t  *vampire;
cvar_t  *excessive;
cvar_t  *grapple;
cvar_t  *classbased;

//duel mode
cvar_t	*g_duel;

cvar_t	*g_losehealth;
cvar_t	*g_losehealth_num;

//weapons
cvar_t	*wep_selfdmgmulti;

//health/max health/max ammo
cvar_t	*g_spawnhealth;
cvar_t	*g_maxhealth;
cvar_t	*g_maxbullets;
cvar_t	*g_maxshells;
cvar_t	*g_maxrockets;
cvar_t	*g_maxgrenades;
cvar_t	*g_maxcells;
cvar_t	*g_maxslugs;

//quick weapon change
cvar_t  *quickweap;

//anti-camp
cvar_t  *anticamp;
cvar_t  *camptime;

//random quad
cvar_t  *g_randomquad;

//warmup
cvar_t  *warmuptime;

//spawn protection
cvar_t  *g_spawnprotect;

//joust mode
cvar_t  *joustmode;

//map voting
cvar_t	*g_mapvote;
cvar_t	*g_voterand;
cvar_t	*g_votemode;
cvar_t	*g_votesame;

//call voting
cvar_t	*g_callvote;

//reward point threshold
cvar_t	*g_reward;

//force autobalanced teams
cvar_t	*g_autobalance;

cvar_t	*dmflags;
cvar_t	*skill;
cvar_t	*fraglimit;
cvar_t	*timelimit;
cvar_t	*password;
cvar_t	*spectator_password;
cvar_t	*needpass;
cvar_t	*g_maxclients;
cvar_t	*maxspectators;
cvar_t	*maxentities;
cvar_t	*g_select_empty;
cvar_t	*g_dedicated;
cvar_t	*motdfile;

cvar_t	*filterban;

cvar_t	*sv_maxvelocity;
cvar_t	*sv_gravity;

cvar_t	*sv_rollspeed;
cvar_t	*sv_rollangle;
cvar_t	*gun_x;
cvar_t	*gun_y;
cvar_t	*gun_z;

cvar_t	*run_pitch;
cvar_t	*run_roll;
cvar_t	*bob_up;
cvar_t	*bob_pitch;
cvar_t	*bob_roll;

cvar_t	*sv_cheats;

cvar_t	*flood_msgs;
cvar_t	*flood_persecond;
cvar_t	*flood_waitdelay;

cvar_t	*sv_maplist;

cvar_t  *g_background_music;

cvar_t  *sv_botkickthreshold;
cvar_t  *sv_custombots;

//unlagged
cvar_t	*g_antilag;
cvar_t	*g_antilagdebug;

void SpawnEntities (char *mapname, char *entities, char *spawnpoint);
void ClientThink (edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect (edict_t *ent, char *userinfo);
void ClientUserinfoChanged (edict_t *ent, char *userinfo, int whereFrom);
void ClientDisconnect (edict_t *ent);
void ClientBegin (edict_t *ent);
void ClientCommand (edict_t *ent);
void RunEntity (edict_t *ent);
void InitGame (void);
void G_RunFrame (void);
int ACESP_FindBotNum(void);
extern long filelength(int);

static size_t szr;

static int g_filelength( FILE *f )
{
	int length = -1;

#if defined HAVE_FILELENGTH

	length = filelength( fileno( f ) );

#elif defined HAVE_FSTAT

	struct stat statbfr;
	int result;

	result = fstat( fileno(f), &statbfr );
	if ( result != -1 )
	{
		length = (int)statbfr.st_size;
	}

#else

	long int pos;

	pos = ftell( f );
	fseek( f, 0L, SEEK_END );
	length = ftell( f );
	fseek( f, pos, SEEK_SET );

#endif

	return length;
}


//===================================================================


void ShutdownGame (void)
{
	gi.dprintf ("==== ShutdownGame ====\n");

	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);
}


/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
game_export_t *GetGameAPI (game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;
	globals.ACESP_FindBotNum = ACESP_FindBotNum;

	globals.RunFrame = G_RunFrame;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}


#define Z_MAGIC		0x1d1d

typedef struct zhead_s
{
	struct	zhead_s  *prev, *next;
	short   magic;
	short   tag;
	int	size;
} zhead_t;

zhead_t		z_chain;
int		z_count, z_bytes;


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames (void)
{
	int		i;
	edict_t	*ent;

	// calc the player views now that all pushing
	// and damage has been added
	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		ClientEndServerFrame (ent);
	}

}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
edict_t *CreateTargetChangeLevel(char *map)
{
	edict_t *ent;

	ent = G_Spawn ();
	ent->classname = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}

/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel (void)
{
	edict_t		*ent;
	char *s, *t, *f;
	static const char *seps = " ,\n\r";
	char *buffer;
	char  mapsname[MAX_OSPATH];
	int length;
	static char **mapnames;
	static int	  nummaps;
	int i;
	FILE *fp;

	/* Search for dead players in order to remove DeathCam and free mem */
	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + i + 1;
		if (!ent->inuse || ent->client->resp.spectator)
			continue;
		if(!ent->is_bot && ent->deadflag)
			DeathcamRemove (ent, "off");
	}

	//call voting
	if(g_callvote->value) {
		playervote.called = false;
		playervote.yay = 0;
		playervote.nay = 0;
		playervote.command[0] = 0;
	}

	//map voting
	if(g_mapvote->value) {
		level.changemap = level.mapname;

		// initialise map list using the current map's name
		// this is done in all modes just in case
		for ( i = 0 ; i < 4 ; i ++ )
		{
			strcpy(votedmap[i].mapname, level.mapname);
			votedmap[i].tally = 0;
		}

		// if there is a map list, time to choose
		if ( sv_maplist && sv_maplist->string && *(sv_maplist->string) )
		{
			char * names[200]; // 200 map names should be fine
			int n_maps = 0;
			int same_index = -1;
			int mode = ( g_votemode ? g_votemode->value : 0 );
			qboolean same = ( g_votesame ? (g_votesame->value == 1) : 1 );

			memset( names, 0 , sizeof(names) );
			s = _strdup( sv_maplist->string );
			t = strtok( s, seps );
			do {
				// if using the same map is disallowed, skip it
				if ( !Q_strcasecmp(t, level.mapname) )
				{
					if ( same_index == -1 )
						same_index = n_maps;
					if ( !same )
						continue;
				}

				// avoid duplicates
				for ( i = 0 ; i < n_maps ; i ++ )
				{
					if ( ! Q_strcasecmp( t , names[i] ) )
						break;
				}
				if ( i < n_maps )
					continue;

				names[n_maps ++] = t;
			} while ( (t = strtok( NULL, seps)) != NULL );

			if ( n_maps > 0 )
			{
				// if the map list has been changed and the
				// current map is no longer in it, prevent
				// screw-ups
				if ( same_index == -1 )
					same_index = 0;

				if ( mode == 0 )
				{
					// standard vote mode, take the next
					// 4 maps from the list
					for ( i = 0 ; i < 4 ; i ++ )
						strcpy( votedmap[i].mapname, names[(same_index + i) % n_maps] );
				}
				else
				{
					// random selection
					for ( i = 0 ; i < 4 && n_maps > 0 ; i ++ )
					{
						int map = random() * (n_maps - 1);
						strcpy( votedmap[i].mapname, names[map] );
						names[map] = names[n_maps - 1];
						n_maps --;
					}
				}
			}

			free(s);
		}
	}

	// stay on same level flag
	if (dmflags->integer & DF_SAME_LEVEL)
	{
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		return;
	}

	if (bot_won && !(dmflags->integer & DF_BOT_LEVELAD) && !ctf->value)
	{
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		return;
	}

	if( ( ctf->integer || cp->integer ) && ! g_dedicated->integer )
	{ //ctf will just stay on same level unless specified by dedicated list
		BeginIntermission (CreateTargetChangeLevel (level.mapname));
		return;
	}
	// see if it's in the map list
	if (*sv_maplist->string) {
		s = _strdup(sv_maplist->string);
		f = NULL;
		t = strtok(s, seps);
		while (t != NULL) {
			if (Q_strcasecmp(t, level.mapname) == 0) {
				// it's in the list, go to the next one
				t = strtok(NULL, seps);
				if (t == NULL) { // end of list, go to first one
					if (f == NULL) // there isn't a first one, same level
						BeginIntermission (CreateTargetChangeLevel (level.mapname) );
					else
						BeginIntermission (CreateTargetChangeLevel (f) );
				} else
					BeginIntermission (CreateTargetChangeLevel (t) );
				free(s);
				return;
			}
			if (!f)
				f = t;
			t = strtok(NULL, seps);
		}
		free(s);
	}

	if(ctf->integer) { //wasn't in the dedicated list
		BeginIntermission (CreateTargetChangeLevel (level.mapname));
		return;
	}

    //check the maps.lst file and read in those, which will overide anything in the level
	//write this code here

	/*
	** load the list of map names
	*/
	if ( !gi.FullPath( mapsname, sizeof(mapsname), "maps.lst" ) )
	{ // no maps.lst.
		// note: originally this was looked for in "./data1/" only
		//   hope it is ok to use FullPath() search path.
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		return;
	}
	if ( ( fp = fopen( mapsname, "rb" ) ) == 0 )
	{
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
			return;
	}

	length = g_filelength( fp );
	buffer = malloc( length + 1 );
	szr = fread( buffer, length, 1, fp );
	buffer[length] = 0;

	s = buffer;

	i = 0;
	while ( i < length )
	{
		if ( s[i] == '\r' )
			nummaps++;
		i++;
	}

	mapnames = malloc( sizeof( char * ) * ( nummaps + 1 ) );
	memset( mapnames, 0, sizeof( char * ) * ( nummaps + 1 ) );

	s = buffer;

	for ( i = 0; i < nummaps; i++ )
	{
    char  shortname[MAX_TOKEN_CHARS];
    char  longname[MAX_TOKEN_CHARS];
		char  scratch[200];
#if defined WIN32_VARIANT
		int  j;
#endif
		int l;

		strcpy( shortname, COM_Parse( &s ) );
		l = strlen(shortname);
#if defined WIN32_VARIANT
		for (j=0 ; j<l ; j++)
			shortname[j] = toupper(shortname[j]);
#endif
		strcpy( longname, COM_Parse( &s ) );
		Com_sprintf( scratch, sizeof( scratch ), "%s", shortname );

		mapnames[i] = malloc( strlen( scratch ) + 1 );
		strcpy( mapnames[i], scratch );
	}
	mapnames[nummaps] = 0;

	fclose(fp);
	free( buffer );

	//find map, goto next map - if one doesn't exist, repeat list
	//stick something in here to filter out CTF, and just make it loop back
	for (i = 0; i < nummaps; i++) {
		if (Q_strcasecmp(mapnames[i], level.mapname) == 0) {
			if(mapnames[i+1][0])
				BeginIntermission (CreateTargetChangeLevel (mapnames[i+1]) );
			else if(mapnames[0][0]) //no more maps, repeat list
				BeginIntermission (CreateTargetChangeLevel (mapnames[0]) );
		}
	}

	if (level.nextmap[0]) // go to a specific map
		BeginIntermission (CreateTargetChangeLevel (level.nextmap) );
	else {	// search for a changelevel
		ent = G_Find (NULL, FOFS(classname), "target_changelevel");
		if (!ent)
		{	// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			BeginIntermission (CreateTargetChangeLevel (level.mapname) );
			return;
		}
		BeginIntermission (ent);
	}
}


/*
=================
CheckNeedPass
=================
*/
void CheckNeedPass (void)
{
	int need;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (password->modified || spectator_password->modified)
	{
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_strcasecmp(password->string, "none"))
			need |= 1;
		if (*spectator_password->string && Q_strcasecmp(spectator_password->string, "none"))
			need |= 2;

		gi.cvar_set("needpass", va("%d", need));
	}
}
/*
=================
ResetLevel
=================
*/
void ResetLevel (void) //for resetting players and items after warmup
{
	int i;
	edict_t	*ent;
	gitem_t *item;

	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + i + 1;
		if (!ent->inuse || ent->client->resp.spectator)
			continue;
		// locate ent at a spawn point and reset everything
		InitClientResp (ent->client);
		if(ent->is_bot)
		{
			// respawn bots after warmup
			ACESP_PutClientInServer( ent, true );
		}
		else {
			if(ent->deadflag)
				DeathcamRemove (ent, "off");
			PutClientInServer (ent);
		}
		ent->client->homing_shots = 0;
	}

	ACESP_SaveBots(); // update bots.tmp and client bot information

	blue_team_score = 0;
	red_team_score = 0;

	//reset level items
	for (i=1, ent=g_edicts+i ; i < globals.num_edicts ; i++,ent++) {

		if (!ent->inuse)
			continue;
		if(ent->client) //not players
			continue;

		//only items, not triggers, trains, etc
		for (i=0,item=itemlist ; i<game.num_items ; i++,item++)
		{
			if (!item->classname)
				continue;

			if (!strcmp(item->classname, ent->classname))
			{	// found it
				DoRespawn(ent);
				break;
			}
		}

	}

	if(g_callvote->value)
		safe_bprintf(PRINT_HIGH, "Call voting is ^2ENABLED\n");
	else
		safe_bprintf(PRINT_HIGH, "Call voting is ^1DISABLED\n");

	if(g_antilag->value)
		safe_bprintf(PRINT_HIGH, "Antilag is ^2ENABLED\n");
	else
		safe_bprintf(PRINT_HIGH, "Antilag is ^1DISABLED\n");

	return;
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules (void)
{
	int			i, top_score, int_time;
	float		real_time;
	gclient_t	*cl;
	edict_t		*cl_ent;

	if(!tca->value && !ctf->value && !cp->value && !(dmflags->integer & DF_SKINTEAMS)) {
		if(level.time <= warmuptime->value) {
			if((warmuptime->value - level.time ) == 3) {
				cl_ent = g_edicts + 1; //need only one for broadcast sound
				if (cl_ent->inuse && !cl_ent->is_bot)
					gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/three.wav"), 1, ATTN_NONE, 0);
			}
			if((warmuptime->value - level.time ) == 2) {
				cl_ent = g_edicts + 1;
				if (cl_ent->inuse && !cl_ent->is_bot)
					gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/two.wav"), 1, ATTN_NONE, 0);
			}
			if((warmuptime->value - level.time ) == 1) {
				cl_ent = g_edicts + 1;
				if (cl_ent->inuse && !cl_ent->is_bot)
					gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/one.wav"), 1, ATTN_NONE, 0);
			}
			if(level.time == warmuptime->value) {
				cl_ent = g_edicts + 1;
				if (cl_ent->inuse && !cl_ent->is_bot)
					gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/fight.wav"), 1, ATTN_NONE, 0);
					safe_centerprintf(cl_ent, "FIGHT!\n");
				ResetLevel();
			}
			else if(level.time == ceil(level.time)){ //do only on the whole numer to avoid overflowing
				for (i=0 ; i<g_maxclients->value ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || cl_ent->is_bot)
						continue;
					int_time = warmuptime->value - level.time;
					real_time = warmuptime->value - level.time;
					if(int_time <= 3) {
						if(int_time == real_time)
							safe_centerprintf(cl_ent, "%i...\n", int_time);
					}
					else if(int_time/2 == real_time/2)
						safe_centerprintf(cl_ent, "%i...\n", int_time);
				}
			}
		}
	}

	if (level.intermissiontime)
		return;

	if (!deathmatch->value)
		return;

	if (timelimit->value)
	{
		if (level.time >= timelimit->value*60 && ((tca->value || ctf->value  || cp->value || (dmflags->integer & DF_SKINTEAMS)) || level.time > warmuptime->value))
		{
			safe_bprintf (PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel ();
			return;
		}
	}

	if (fraglimit->value && ((tca->value || ctf->value || cp->value || (dmflags->integer & DF_SKINTEAMS)) || level.time > warmuptime->value))
	{

		//team scores
		if ((dmflags->integer & DF_SKINTEAMS) || ctf->value || cp->value) //it's all about the team!
		{
			if(blue_team_score >= fraglimit->value)
			{
				safe_bprintf(PRINT_HIGH, "Blue Team wins!\n");

				bot_won = 0; //we don't care if it's a bot that wins

				EndDMLevel();
				return;
			}
			if(red_team_score >= fraglimit->value)
			{
				safe_bprintf(PRINT_HIGH, "Red Team wins!\n");

				bot_won = 0; //we don't care if it's a bot that wins

				EndDMLevel();
				return;
			}
		}
		else {
			top_score = 0;
			for (i=0 ; i<g_maxclients->value ; i++)
			{
				cl = game.clients + i;
				if (!g_edicts[i+1].inuse)
					continue;

				if(cl->resp.score > top_score)
					top_score = cl->resp.score; //grab the top score

				if (cl->resp.score >= fraglimit->value)
				{
					if(cl->is_bot){
						bot_won = 1; //a bot has won the match
						safe_bprintf (PRINT_HIGH, "Fraglimit hit by bot.\n");
					}
					else
					{
						bot_won = 0;
						safe_bprintf (PRINT_HIGH, "Fraglimit hit.\n");
					}

					EndDMLevel ();
					return;
				}
			}
			if(!tca->value && !ctf->value && !cp->value) {
				i = fraglimit->value - top_score;
				switch(i) {
					case 3:
						if(!print3){
							for (i=0 ; i<g_maxclients->value ; i++)
							{
								cl_ent = g_edicts + 1 + i;
								if (!cl_ent->inuse || cl_ent->is_bot)
									continue;
								if(i == 0)
									gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/3frags.wav"), 1, ATTN_NONE, 0);
								safe_centerprintf(cl_ent, "3 frags remain!\n");
							}
							print3 = true;
						}
						break;
					case 2:
						if(!print2) {
							for (i=0 ; i<g_maxclients->value ; i++)
							{
								cl_ent = g_edicts + 1 + i;
								if (!cl_ent->inuse || cl_ent->is_bot)
									continue;
								if(i == 0)
									gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/2frags.wav"), 1, ATTN_NONE, 0);
								safe_centerprintf(cl_ent, "2 frags remain!\n");
							}
							print2 = true;
						}
						break;
					case 1:
						if(!print1) {
							for (i=0 ; i<g_maxclients->value ; i++)
							{
								cl_ent = g_edicts + 1 + i;
								if (!cl_ent->inuse || cl_ent->is_bot)
									continue;
								if(i == 0)
									gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/1frags.wav"), 1, ATTN_NONE, 0);
								safe_centerprintf(cl_ent, "1 frag remains!\n");
							}
							print1 = true;
						}
						break;
					default:
						break;
				}
			}
		}
	}
	if(tca->value) {
		if(blue_team_score == 0) {

			safe_bprintf(PRINT_HIGH, "Red Team wins!\n");

			bot_won = 0; //we don't care if it's a bot that wins

			EndDMLevel();
			return;
		}
		if(red_team_score == 0) {

			safe_bprintf(PRINT_HIGH, "Blue Team wins!\n");

			bot_won = 0; //we don't care if it's a bot that wins

			EndDMLevel();
			return;
		}
	}
}

/*
=============
ExitLevel
=============
*/
void ExitLevel (void)
{
	int			i;
	edict_t		*ent;
	char		command [256];

	if(strcmp(level.mapname, level.changemap) || timelimit->value) {
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", level.changemap);
		gi.AddCommandString (command);
	}

    //Note-- whenever the map command fails (for instance, misspelled bsp name
    //in the server cfg,) it will just play another game on the same map. As
    //of right here in the code, there is no way to detect if that is going to
    //happen, so we initialize another game on this map just in case. This
    //fixes many longstanding bugs where misspelled map names in the cfg would
    //cause servers to glitch out. 
    // -Max
    
	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames ();
	EndIntermission();

	// clear some things before going to next level
	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->health > ent->client->pers.max_health)
			ent->health = ent->client->pers.max_health;

		ent->client->resp.score = 0;
		ent->client->resp.deaths = 0;
		ent->client->resp.reward_pts = 0;
		ent->client->homing_shots = 0;
		ent->takedamage = DAMAGE_AIM;
		ent->solid = SOLID_BBOX;
		ent->deadflag = DEAD_NO;
		if(ent->is_bot) {
			ACESP_PutClientInServer( ent, true );
		} else {
			PutClientInServer (ent);
		}
		if(g_duel->value) {
			ClientPlaceInQueue(ent);
			ClientCheckQueue(ent);
		}
	}
	for (i=1, ent=g_edicts+i ; i < globals.num_edicts ; i++,ent++) {

		if (!ent->inuse || ent->client)
			continue;
		//remove podiums
		if(!strcmp(ent->classname, "pad"))
			G_FreeEdict(ent);
		if(tca->value)
			ent->powered = true;
	}
	if(tca->value) {
		blue_team_score = 4;
		red_team_score = 4;
	}
	else {
		red_team_score = 0;
		blue_team_score = 0;
	}
	print1 = print2 = print3 = false;

}

/*
================
G_Ban

Ban player
================
*/
/*
=================
SV_WriteIP_f
=================
*/
void G_Ban (char *ip)
{
	FILE	*f;
	char	name[MAX_OSPATH];
	cvar_t	*game;
	int		i;

	//add to banlist file
	game = gi.cvar("game", "", 0);

	if (!*game->string)
		sprintf (name, "%s/listip.cfg", GAMEVERSION);
	else
		sprintf (name, "%s/listip.cfg", game->string);

	safe_cprintf (NULL, PRINT_HIGH, "Writing %s.\n", name);

	f = fopen (name, "ab");
	if (!f)
	{
		safe_cprintf (NULL, PRINT_HIGH, "Couldn't open %s\n", name);
		return;
	}

	fprintf (f, "sv addip %s\n", ip);

	fclose (f);

	//add to current ban list
	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].compare == 0xffffffff)
			break;		// free spot
	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			safe_cprintf (NULL, PRINT_HIGH, "IP filter list is full\n");
			return;
		}
		numipfilters++;
	}

	if (!StringToFilter (ip, &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

/*
================
G_NameMatch

 A name entered in the console may (unlikely) or may not have color codes.
 This is a case-sensitive match, because a case-insensitive match would
   complicate matters.

 Used by sv removebot <name> and callvote kicks

================
*/
qboolean G_NameMatch( const char* netname, const char *kickname )
{
	const char* p_netname = netname;
	const char* p_kickname = kickname;
	int char_count = 16;
	int len_count = 49; // maximum length of 16 char color name (16 * 3 + nul)
	qboolean matched = false;

	while ( len_count > 0 && char_count > 0)
	{
		if ( *p_netname != *p_kickname )
		{
			if ( Q_IsColorString( p_netname ) )
			{ // netname has color code, kickname does not, match still possible
				p_netname += 2;
				len_count -= 2;
			}
			else if ( Q_IsColorString( p_kickname ))
			{ // kickname has color code, netname does not, match still possible
				p_kickname += 2;
				len_count -= 2;
			}
			else
			{ // no match. mismatched chars, or end of one or the other
				break;
			}
		}
		else if ( !*p_netname && !*p_kickname )
		{ // end of both strings,
			matched = true;
			break;
		}
		else
		{ // chars matched
			if ( Q_IsColorString( p_netname ) && Q_IsColorString( p_kickname ) )
			{ // color does not have to match
				p_netname += 2;
				p_kickname += 2;
				len_count -= 2;
			}
			else
			{
				++p_netname;
				++p_kickname;
				--len_count;
				--char_count;
			}
		}
	}

	return matched;
}

/*
================
G_ParseVoteCommand

Parse and execute command
================
*/
// helper function
static edict_t* find_kick_target( const char *name )
{
	edict_t* kick_target = NULL;
	edict_t* ent;
	int i;

	for ( i = 1 ; i <= g_maxclients->integer ; i++ )
	{
		ent = &g_edicts[i];
		if ( !ent->inuse )
			continue;
		if ( ent->client )
		{
			if ( G_NameMatch( ent->client->pers.netname, name ) )
			{
				kick_target = ent;
				break;
			}
		}
	}

	return kick_target;
}

void G_ParseVoteCommand (void)
{
	int i, j;
	char command[128];
	char args[128];
	edict_t *ent;
	qboolean donearg = false;
	char	*value;

	//separate command from args
	i = j = 0;
	while(i < 128) {

		if(playervote.command[i] == ' ')
			donearg = true;

		if(!donearg)
			command[i] = playervote.command[i];
		else
			command[i] = 0;

		if(donearg && i < 127) { //skip the space between command and arg
			args[j] = playervote.command[i+1];
			j++;
		}
		i++;
	}

	if ( !strcmp(command, "kick") )
	{ // kick player or bot
		ent = find_kick_target( args );
		if ( ent )
		{
			if ( ent->is_bot )
			{
				if ( sv_botkickthreshold && sv_botkickthreshold->integer )
				{
					safe_bprintf(PRINT_HIGH,
						"Auto bot kick enabled, callvote kick <bot> disabled.\n%s not kicked.\n",
						 ent->client->pers.netname);
				}
				else
				{
					ACESP_KickBot( ent );
				}
			}
			else
			{
				safe_bprintf(PRINT_HIGH, "%s was kicked\n", args);
				ClientDisconnect (ent);
			}
		}
		else
		{
			safe_bprintf( PRINT_HIGH, "Did not find %s here.\n", args );
		}
	}
	else if ( !strcmp( command, "kickban") )
	{ // kickban player
		ent = find_kick_target( args );
		if ( ent )
		{
			if ( ent->is_bot )
			{ //
				safe_bprintf(PRINT_HIGH,
					"%s is a bot, use \"kick\", not \"kickban.\"\n", ent->client->pers.netname);
			}
			else
			{
				safe_bprintf(PRINT_HIGH, "%s was kickbanned\n", args);
				ClientDisconnect (ent);
				value = Info_ValueForKey (ent->client->pers.userinfo, "ip");
				G_Ban(value);
			}
		}
		else
		{
			safe_bprintf( PRINT_HIGH, "Did not find %s here.\n", args );
		}
	}
	else if(!strcmp(command, "fraglimit")) { //change fraglimit
		gi.cvar_set("fraglimit", args);
		safe_bprintf(PRINT_HIGH, "Fraglimit changed to %s\n", args);
	}
	else if(!strcmp(command, "timelimit")) { //change timelimit
		gi.cvar_set("timelimit", args);
		safe_bprintf(PRINT_HIGH, "Timelimit changed to %s\n", args);
	}
	else if(!strcmp(command, "map")) { //change map
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", args);
		gi.AddCommandString (command);
	}
	else
		safe_bprintf(PRINT_HIGH, "Invalid command!");

}


/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
void G_RunFrame (void)
{
	int		i, numActiveClients = 0;
	edict_t	*ent;

	level.previousTime = gi.Sys_Milliseconds() - 100; //100 milleseconds(1/10 of a second)

	level.framenum++;
	level.time = level.framenum*FRAMETIME;

	// choose a client for monsters to target this frame
	AI_SetSightClient ();

	// exit intermissions

	if (level.exitintermission)
	{
		ExitLevel ();
		return;
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//

	ent = &g_edicts[0];
	for (i=0 ; i<globals.num_edicts ; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

		VectorCopy (ent->s.origin, ent->s.old_origin);

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;
			if ( !(ent->flags & (FL_SWIM|FL_FLY)) && (ent->svflags & SVF_MONSTER) )
			{
				M_CheckGround (ent);
			}
		}

		if (i > 0 && i <= g_maxclients->value)
		{
			ClientBeginServerFrame (ent);
		}

		if(ent->inuse && ent->client && !ent->is_bot)
		{
			if ( ent->s.number <= g_maxclients->value )
			{ // count actual players, not deathcam entities
				numActiveClients++;
			}
		}

		//this next block of code may not be practical for a server running at 10fps
/*		if(ent->movetype & MOVETYPE_FLYMISSILE) {
			//unlagged
			if ( g_antilag->integer)
				G_TimeShiftAllClients( level.previousTime, NULL );

			G_RunEntity (ent);

			//unlagged
			if ( g_antilag->integer)
				G_UnTimeShiftAllClients( NULL );
		}
		else*/
			G_RunEntity (ent);
	}

	// see if it is time to end a deathmatch
	CheckDMRules ();

	// see if needpass needs updated
	CheckNeedPass ();

	// build the playerstate_t structures for all players
	ClientEndServerFrames ();

	//unlagged
	if ( g_antilag->integer)
		level.frameStartTime = level.time;

	//call voting
	if(g_callvote->value && playervote.called) {

		playervote.time = level.time;
		if(playervote.time-playervote.starttime > 15 ){ //15 seconds
			//execute command if votes are sufficient
			if(numActiveClients <= 2 && playervote.yay > playervote.nay+1) { //minimum of 2 votes to pass
				safe_bprintf(PRINT_HIGH, "Vote ^2Passed\n");

				//parse command(we will allow kick, map, fraglimit, timelimit).
				G_ParseVoteCommand();

			}
			else if(playervote.yay > 2 && playervote.yay > playervote.nay+1) { //3-1 minimum to pass
				safe_bprintf(PRINT_HIGH, "Vote ^2Passed\n");

				//parse command(we will allow kick, map, fraglimit, timelimit).
				G_ParseVoteCommand();

			}
			else
				safe_bprintf(PRINT_HIGH, "Vote ^1Failed\n");

			//clear
			playervote.called = false;
			playervote.yay = playervote.nay = 0;
			playervote.command[0] = 0;

			//do each ent
			for (i=0 ; i<g_maxclients->value ; i++)
			{
				ent = g_edicts + 1 + i;
				if (!ent->inuse || ent->is_bot)
					continue;
				ent->client->resp.voted = false;
			}
		}
	}
}

