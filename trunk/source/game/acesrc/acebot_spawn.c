///////////////////////////////////////////////////////////////////////
//
//  ACE - Quake II Bot Base Code
//
//  Version 1.0
//
//  This file is Copyright(c), Steve Yeager 1998, All Rights Reserved
//
//
//	All other files are Copyright(c) Id Software, Inc.
//
//	Please see liscense.txt in the source directory for the copyright
//	information regarding those files belonging to Id Software, Inc.
//	
//	Should you decide to release a modified version of ACE, you MUST
//	include the following text (minus the BEGIN and END lines) in the 
//	documentation for your modification.
//
//	--- BEGIN ---
//
//	The ACE Bot is a product of Steve Yeager, and is available from
//	the ACE Bot homepage, at http://www.axionfx.com/ace.
//
//	This program is a modification of the ACE Bot, and is therefore
//	in NO WAY supported by Steve Yeager.

//	This program MUST NOT be sold in ANY form. If you have paid for 
//	this product, you should contact Steve Yeager immediately, via
//	the ACE Bot homepage.
//
//	--- END ---
//
//	I, Steve Yeager, hold no responsibility for any harm caused by the
//	use of this source code, especially to small children and animals.
//  It is provided as-is with no implied warranty or support.
//
//  I also wish to thank and acknowledge the great work of others
//  that has helped me to develop this code.
//
//  John Cricket    - For ideas and swapping code.
//  Ryan Feltrin    - For ideas and swapping code.
//  SABIN           - For showing how to do true client based movement.
//  BotEpidemic     - For keeping us up to date.
//  Telefragged.com - For giving ACE a home.
//  Microsoft       - For giving us such a wonderful crash free OS.
//  id              - Need I say more.
//  
//  And to all the other testers, pathers, and players and people
//  who I can't remember who the heck they were, but helped out.
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
//  acebot_spawn.c - This file contains all of the 
//                   spawing support routines for the ACE bot.
//
///////////////////////////////////////////////////////////////////////


#include "../g_local.h"
#include "../m_player.h"
#include "acebot.h"

#ifdef _WINDOWS
extern void ACECO_ReadConfig(char config_file[128]);
#endif

///////////////////////////////////////////////////////////////////////
// Had to add this function in this version for some reason.
// any globals are wiped out between level changes....so
// save the bots out to a file. 
//
// NOTE: There is still a bug when the bots are saved for
//       a dm game and then reloaded into a CTF game.
///////////////////////////////////////////////////////////////////////
void ACESP_SaveBots()
{
    edict_t *bot;
    FILE *pOut;
	int i,count = 0;

#ifdef __linux__
	if((pOut = fopen("botinfo/bots.tmp", "wb" )) == NULL)
#else
	if((pOut = fopen("botinfo\\bots.tmp", "wb" )) == NULL)
#endif
		return; // bail
	
	// Get number of bots
	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;

		if (bot->inuse && bot->is_bot)
			count++;
	}
	
	fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;

		if (bot->inuse && bot->is_bot)
			fwrite(bot->client->pers.userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut); 
	}
		
    fclose(pOut);
}

///////////////////////////////////////////////////////////////////////
// Had to add this function in this version for some reason.
// any globals are wiped out between level changes....so
// load the bots from a file.
//
// Side effect/benifit are that the bots persist between games.
///////////////////////////////////////////////////////////////////////
void ACESP_LoadBots(edict_t *ent, int playerleft)
{
    FILE *pIn;
	char userinfo[MAX_INFO_STRING];
	int i, j, count;
	char *info;
	char *skin;
	char bot_filename[128];
	int found;
	int real_players, total_players;
	edict_t *cl_ent;

//bots and configurations will be loaded level specific
	if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value)
		strcpy(bot_filename, "botinfo/team.tmp");
	else if(sv_custombots->value)
		sprintf(bot_filename, "botinfo/custom%i.tmp", sv_custombots->integer);
	else
		sprintf(bot_filename, "botinfo/%s.tmp", level.mapname);

	if((pIn = fopen(bot_filename, "rb" )) == NULL)
		return; // bail

	fread(&count,sizeof (int),1,pIn); 

	if (((int)(dmflags->value) & DF_BOTS)) {
		fclose(pIn);
		return; // don't load any preconfigured bots.
	}

	ent->client->resp.botnum = 0;

	//probably want to count total REAL players here...
	real_players = 0;

	if(sv_botkickthreshold->integer) { //test for now
		for (i=0 ; i<game.maxclients ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (cl_ent->inuse && !cl_ent->is_bot){
				cl_ent->client->resp.botnum = 0;
				if(!game.clients[i].resp.spectator)
					real_players++;
			}
		}
	}
	real_players -= playerleft; //done for when this is called as a player is disconnecting
	
	total_players = 0;
	for(i=0;i<count;i++)
	{
		
		total_players = real_players + i + 1;
		
		fread(userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn); 

		info = Info_ValueForKey (userinfo, "name");
		skin = Info_ValueForKey (userinfo, "skin");	

		strcpy(ent->client->resp.bots[i].name, info);

		if(sv_botkickthreshold->integer) { 
				for (j=0 ; j<game.maxclients ; j++)
				{
					cl_ent = g_edicts + 1 + j;
					if (cl_ent->inuse) {
						if(total_players <= sv_botkickthreshold->integer) //we actually added one
							cl_ent->client->resp.botnum = i+1;
						cl_ent->client->ps.botnum = cl_ent->client->resp.botnum;
						strcpy(cl_ent->client->ps.bots[i].name, info);
					}
				}
		}
		else
			ent->client->resp.botnum++;
        
		//look for existing bots of same name, so that server doesn't fill up with bots
		//when clients enter the game
		found = false;
		found = ACESP_FindBot(info);

		if(!found && ((total_players <= sv_botkickthreshold->integer) || !sv_botkickthreshold->integer)) { 

			if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value) 
				ACESP_SpawnBot(NULL, info, skin, NULL); //we may be changing the info further on
			else 
				ACESP_SpawnBot (NULL, NULL, NULL, userinfo);
			
		}
		else if(found && ((total_players > sv_botkickthreshold->integer) && sv_botkickthreshold->integer)) 
			ACESP_KickBot(info);
		
	}
		
    fclose(pIn);

}

///////////////////////////////////////////////////////////////////////
// Called by PutClient in Server to actually release the bot into the game
// Keep from killin' each other when all spawned at once
///////////////////////////////////////////////////////////////////////
void ACESP_HoldSpawn(edict_t *self)
{
	if (!KillBox (self))
	{	// could't spawn in?
	}

	gi.linkentity (self);

	self->think = ACEAI_Think;
	self->nextthink = level.time + FRAMETIME;

	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (self-g_edicts);
	gi.WriteByte (MZ_LOGIN);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	
	safe_bprintf (PRINT_MEDIUM, "%s entered the game\n", self->client->pers.netname);

}

#ifdef __linux__
void ACECO_ReadConfig(char config_file[128]) //use standard c routines for linux
{
	FILE *fp;
	int k, length, rslt;
	char a_string[128];
	char *buffer;
	char *s;

	//set bot defaults(in case no bot config file is present for that bot)
	botvals.skill = 1; //medium
	strcpy(botvals.faveweap, "None");
	for(k = 1; k < 10; k++) 
		botvals.weapacc[k] = 1.0;
	botvals.awareness = 0.7;

	strcpy( botvals.chatmsg1, "%s: I am the destroyer %s, not you."); 
	strcpy( botvals.chatmsg2, "%s: Are you using a bot %s?"); 
	strcpy( botvals.chatmsg3, "%s: %s is a dead man." ); 
	strcpy( botvals.chatmsg4, "%s: You will pay dearly %s!"); 
	strcpy( botvals.chatmsg5, "%s: Ackity Ack Ack!"); 
	strcpy( botvals.chatmsg6, "%s: Die %s!"); 
	strcpy( botvals.chatmsg7, "%s: My blood is your blood %s." ); 
	strcpy( botvals.chatmsg8, "%s: I will own you %s!"); 

	if((fp = fopen(config_file, "rb" )) == NULL)
	{
		safe_bprintf (PRINT_HIGH, "no file\n");
		return;
	}
	else
	{

#ifdef _WIN32
		length = filelength( fileno( fp  ) );
#else
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
#endif
		buffer = malloc( length );
		fread( buffer, length, 1, fp );
	}
	s = buffer;
	
	strcpy( a_string, COM_Parse( &s ) ); 
	botvals.skill = atoi(a_string);  //all we are getting in the linux version is the skill.
/*  //we have a problem here with COM_Parse.  It just seems to not work right with whitespace here
	strcpy( botvals.faveweap, COM_Parse( &s ) ); 
	safe_bprintf (PRINT_HIGH, "Weap %s\n", botvals.faveweap);
	for(k = 1; k < 10; k++) {
		strcpy( a_string, COM_Parse( &s ) ); 
		botvals.weapacc[k] = atof(a_string);
	}

	strcpy( a_string, COM_Parse( &s ) ); 
	botvals.awareness = atof(a_string); 

	strcpy( botvals.chatmsg1, COM_Parse( &s ) ); 
	strcpy( botvals.chatmsg2, COM_Parse( &s ) ); 
	strcpy( botvals.chatmsg3, COM_Parse( &s ) ); 
	strcpy( botvals.chatmsg4, COM_Parse( &s ) ); 
	strcpy( botvals.chatmsg5, COM_Parse( &s ) ); 
	strcpy( botvals.chatmsg6, COM_Parse( &s ) ); 
	strcpy( botvals.chatmsg7, COM_Parse( &s ) ); 
	strcpy( botvals.chatmsg8, COM_Parse( &s ) ); 
	 */
	if ( fp != 0 )
	{
		fp = 0;
		free( buffer );
	}
	else
	{
		FS_FreeFile( buffer );
	}
}
#endif

///////////////////////////////////////////////////////////////////////
// Modified version of id's code
///////////////////////////////////////////////////////////////////////
void ACESP_PutClientInServer (edict_t *bot, qboolean respawn, int team)
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 32};
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i, k, done;
	client_persistant_t	saved;
	client_respawn_t	resp;
	char    *info;
	char bot_configfilename[MAX_OSPATH];
	char playermodel[MAX_OSPATH] = " ";
	char modelpath[MAX_OSPATH] = " ";
	FILE *file;

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint (bot, spawn_origin, spawn_angles);
	
	index = bot-g_edicts-1;
	client = bot->client;

	// deathmatch wipes most client data every spawn
	if (deathmatch->value)
	{
		char userinfo[MAX_INFO_STRING];

		resp = bot->client->resp;
		memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant (client);
		ClientUserinfoChanged (bot, userinfo, SPAWN);
	}
	else
		memset (&resp, 0, sizeof(resp));
	
	// clear everything but the persistant data
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	client->resp = resp;
	
	// copy some data from the client to the entity
	FetchClientEntData (bot);
	
	// clear entity values
	bot->groundentity = NULL;
	bot->client = &game.clients[index];
	bot->takedamage = DAMAGE_AIM;
	bot->movetype = MOVETYPE_WALK;
	bot->viewheight = 24;
	bot->classname = "bot";
	bot->mass = 200;
	bot->solid = SOLID_BBOX;
	bot->deadflag = DEAD_NO;
	bot->air_finished = level.time + 12;
	bot->clipmask = MASK_PLAYERSOLID;
	bot->model = "players/martianenforcer/tris.md2";
	bot->pain = player_pain;
	bot->die = player_die;
	bot->waterlevel = 0;
	bot->watertype = 0;
	bot->flags &= ~FL_NO_KNOCKBACK;
	bot->svflags &= ~SVF_DEADMONSTER;
	bot->is_jumping = false;

	//vehicles
	bot->in_vehicle = false;

	//deathball
	bot->in_deathball = false;
	
	VectorCopy (mins, bot->mins);
	VectorCopy (maxs, bot->maxs);
	VectorClear (bot->velocity);

	// clear playerstate values
	memset (&bot->client->ps, 0, sizeof(client->ps));
	
	client->ps.pmove.origin[0] = spawn_origin[0]*8;
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;

	client->kill_streak = 0;

//ZOID
	client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
//ZOID

	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		client->ps.fov = 90;
	}
	else
	{
		client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));
		if (client->ps.fov < 1)
			client->ps.fov = 90;
		else if (client->ps.fov > 160)
			client->ps.fov = 160;
	}

	client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);

	// clear entity state values
	bot->s.effects = 0;
	bot->s.skinnum = bot - g_edicts - 1;
	bot->s.modelindex = 255;		// will use the skin specified model
	bot->s.modelindex2 = 255;		// custom gun model

	info = Info_ValueForKey (bot->client->pers.userinfo, "skin");
	i = 0;
	done = false;
	strcpy(playermodel, " ");
	while(!done)
	{
		if((info[i] == '/') || (info[i] == '\\'))
			done = true;
		playermodel[i] = info[i];
		if(i > 62)
			done = true;
		i++;
	}
	playermodel[i-1] = 0;

	sprintf(modelpath, "data1/players/%s/helmet.md2", playermodel);
	Q2_FindFile (modelpath, &file); //does a helmet exist?
	if(file) {
		sprintf(modelpath, "players/%s/helmet.md2", playermodel);
		bot->s.modelindex3 = gi.modelindex(modelpath);
		fclose(file);
	}
	else 
		bot->s.modelindex3 = 0;

	if(!strcmp(playermodel, "war")) //special case
	{
		bot->s.modelindex4 = gi.modelindex("players/war/weapon.md2");
		bot->s.modelindex2 = 0;
	}
	else if(!strcmp(playermodel, "brainlet"))	
		bot->s.modelindex4 = gi.modelindex("players/brainlet/gunrack.md2"); //brainlets have a mount

	bot->s.frame = 0;
	VectorCopy (spawn_origin, bot->s.origin);
	bot->s.origin[2] += 1;	// make sure off ground

	// set the delta angle
	for (i=0 ; i<3 ; i++)
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);

	bot->s.angles[PITCH] = 0;
	bot->s.angles[YAW] = spawn_angles[YAW];
	bot->s.angles[ROLL] = 0;
	VectorCopy (bot->s.angles, client->ps.viewangles);
	VectorCopy (bot->s.angles, client->v_angle);
	
	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon (bot);

	//so we can tell if a client is a bot
	client->is_bot = 1;

	bot->enemy = NULL;
	bot->movetarget = NULL; 
	bot->state = STATE_MOVE;

	// Set the current node
	bot->current_node = ACEND_FindClosestReachableNode(bot,NODE_DENSITY, NODE_ALL);
	bot->goal_node = bot->current_node;
	bot->next_node = bot->current_node;
	bot->next_move_time = level.time;		
	bot->suicide_timeout = level.time + 15.0;

	if(!respawn) {
		//if not a respawn, load bot configuration file(specific to each bot)
		info = Info_ValueForKey (bot->client->pers.userinfo, "name");
 
#ifdef __linux__
		sprintf(bot_configfilename, "botinfo/%s.cfg", info);
		//write something else for linux only
#else
		sprintf(bot_configfilename, "botinfo\\%s.cfg", info);
#endif	
		ACECO_ReadConfig(bot_configfilename);

		//set config items
		bot->skill = botvals.skill;
		strcpy(bot->faveweap, botvals.faveweap);
		for(k = 1; k < 10; k++) 
			bot->weapacc[k] = botvals.weapacc[k];
		bot->accuracy = 1.0; //start with this(changes when bot selects a weapon
		bot->awareness = botvals.awareness;
		strcpy(bot->chatmsg1, botvals.chatmsg1);
		strcpy(bot->chatmsg2, botvals.chatmsg2);
		strcpy(bot->chatmsg3, botvals.chatmsg3);
		strcpy(bot->chatmsg4, botvals.chatmsg4);
		strcpy(bot->chatmsg5, botvals.chatmsg5);
		strcpy(bot->chatmsg6, botvals.chatmsg6);
		strcpy(bot->chatmsg7, botvals.chatmsg7);
		strcpy(bot->chatmsg8, botvals.chatmsg8);

		//allow skill level settings to affect overall skills(single player games)
		if(skill->value == 0.0) {
			bot->skill = 0; //dumb as a box of rocks	
		}
		if(skill->value == 2.0) {
			bot->skill += 1;
			if(bot->skill > 3)
				bot->skill = 3;
		}
	}
	// If we are not respawning hold off for up to three seconds before releasing into game
    if(!respawn)
	{
		bot->think = ACESP_HoldSpawn;
		bot->nextthink = level.time + 0.1;
		bot->nextthink = level.time + random()*3.0; // up to three seconds
	}
	else
	{
		if (!KillBox (bot))
		{	// could't spawn in?
		}

		bot->s.event = EV_OTHER_TELEPORT; //fix "player flash" bug
		gi.linkentity (bot);

		bot->think = ACEAI_Think;
		bot->nextthink = level.time + FRAMETIME;

			// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (bot-g_edicts);
		gi.WriteByte (MZ_LOGIN);
		gi.multicast (bot->s.origin, MULTICAST_PVS);
	}
	
}

///////////////////////////////////////////////////////////////////////
// Respawn the bot
///////////////////////////////////////////////////////////////////////
void ACESP_Respawn (edict_t *self)
{
	CopyToBodyQue (self);
	
	ACESP_PutClientInServer (self,true,0);

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
	self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->client->ps.pmove.pm_time = 14;

	self->client->respawn_time = level.time;

}

///////////////////////////////////////////////////////////////////////
// Find a free client spot
///////////////////////////////////////////////////////////////////////
edict_t *ACESP_FindFreeClient (void)
{
	edict_t *bot;
	int	i;
	int max_count=0;
	
	// This is for the naming of the bots
	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;
		
		if(bot->count > max_count)
			max_count = bot->count;
	}

	// Check for free spot
	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;

		if (!bot->inuse)
			break;
	}

	bot->count = max_count + 1; // Will become bot name...

	if (bot->inuse)
		bot = NULL;
	
	return bot;
}

///////////////////////////////////////////////////////////////////////
// Set the name of the bot and update the userinfo
///////////////////////////////////////////////////////////////////////
void ACESP_SetName(edict_t *bot, char *name, char *skin, char *team)
{
	float rnd;
	char userinfo[MAX_INFO_STRING];
	char bot_skin[MAX_INFO_STRING];
	char bot_name[MAX_INFO_STRING];
	char playerskin[MAX_INFO_STRING];
	char playermodel[MAX_INFO_STRING];
	int i, j, k, copychar;

	// Set the name for the bot.
	// name
	if(strlen(name) == 0)
		sprintf(bot_name,"ACEBot_%d",bot->count);
	else
		strcpy(bot_name,name);

	// skin
	
	if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value) //only do this for skin teams, red, blue
	{
		copychar = false;
		strcpy(playerskin, " ");
		strcpy(playermodel, " ");
		j = k = 0;
		for(i = 0; i <= strlen(skin) && i < MAX_INFO_STRING; i++)
		{
			if(copychar){ 
				playerskin[k] = skin[i];
				k++;
			}
			else {
				playermodel[j] = skin[i];
				j++;
			}
			if(skin[i] == '/')
				copychar = true;
				
			
		}
		playermodel[j] = 0;
		
		if((!strcmp(playerskin, "red"))	|| (!strcmp(playerskin, "blue"))) //was valid teamskin
		{
			if(!strcmp(playerskin, "red"))
			{
				bot->dmteam = RED_TEAM;
	//			red_team_cnt++;
			}
			else
			{
				bot->dmteam = BLUE_TEAM;
	//			blue_team_cnt++;
			}

		}
		else //assign to team with fewest players
		{
			if(blue_team_cnt < red_team_cnt)
			{
				strcpy(playerskin, "blue");
	//			blue_team_cnt++;
				bot->dmteam = BLUE_TEAM;
			}
			else
			{
				strcpy(playerskin, "red");
	//			red_team_cnt++;
				bot->dmteam = RED_TEAM;
			}
		}
	
		strcpy(skin, playermodel);
		strcat(skin, playerskin);

	}
		
	if(strlen(skin) == 0)
	{
		// randomly choose skin 
		rnd = random();
		if(rnd  < 0.5)
			sprintf(bot_skin,"martianenforcer/red");
		
		else 
			sprintf(bot_skin,"martianenforcer/blue");
	}
	else 
		strcpy(bot_skin,skin);

	// initialise userinfo
	memset (userinfo, 0, sizeof(userinfo));

	// add bot's name/skin/hand to userinfo
	Info_SetValueForKey (userinfo, "name", bot_name);
	Info_SetValueForKey (userinfo, "skin", bot_skin);
	Info_SetValueForKey (userinfo, "hand", "2"); // bot is center handed for now!

	ClientConnect (bot, userinfo);

	ACESP_SaveBots(); // make sure to save the bots
}

///////////////////////////////////////////////////////////////////////
// Spawn the bot
///////////////////////////////////////////////////////////////////////
void ACESP_SpawnBot (char *team, char *name, char *skin, char *userinfo)
{
	edict_t *bot;
	char *info;
	char sound[64];

	bot = ACESP_FindFreeClient ();

	if (!bot)
	{
		safe_bprintf (PRINT_MEDIUM, "Server is full, increase Maxclients.\n");
		return;
	}

	bot->yaw_speed = 100; // yaw speed
	bot->inuse = true;
	bot->is_bot = true;

	// To allow bots to respawn
	if(userinfo == NULL)
		ACESP_SetName(bot, name, skin, team);
	else 
		ClientConnect (bot, userinfo);
	
	G_InitEdict (bot);

	InitClientResp (bot->client);

	//play sound announcing arrival of bot
	info = Info_ValueForKey (bot->client->pers.userinfo, "name");
	sprintf(sound, "bots/%s.wav", info);
	gi.sound (bot, CHAN_AUTO, gi.soundindex(sound), 1, ATTN_NONE, 0);

	ACESP_PutClientInServer (bot,false,0);

	// make sure all view stuff is valid
	ClientEndServerFrame (bot);

	ACEAI_PickLongRangeGoal(bot); // pick a new goal

}

///////////////////////////////////////////////////////////////////////
// Remove a bot by name or all bots
///////////////////////////////////////////////////////////////////////
void ACESP_RemoveBot(char *name)
{
	int i;
	qboolean freed=false;
	edict_t *bot;

	for(i=0;i<maxclients->value;i++)
	{
		bot = g_edicts + i + 1;
		if(bot->inuse)
		{
			if(bot->is_bot && (strcmp(bot->client->pers.netname,name)==0 || strcmp(name,"all")==0))
			{
				bot->health = 0;
				player_die (bot, bot, bot, 100000, vec3_origin);
				// don't even bother waiting for death frames
				bot->deadflag = DEAD_DEAD;
				bot->inuse = false;
				freed = true;
				safe_bprintf (PRINT_MEDIUM, "%s removed\n", bot->client->pers.netname);
			}

		}
	}

	if(!freed)	
		safe_bprintf (PRINT_MEDIUM, "%s not found\n", name);
	
	ACESP_SaveBots(); // Save them again
}

int ACESP_FindBot(char *name)
{
	int i;
	edict_t *bot;
	int foundbot = false;

	for(i=0;i<maxclients->value;i++)
	{
		bot = g_edicts + i + 1;
		if(bot->inuse)
		{
			if(bot->is_bot && (strcmp(bot->client->pers.netname,name)==0))
			{
				foundbot = true;
			}
		}
	}

	return foundbot;
}

void ACESP_KickBot(char *name)
{
	int i;
	qboolean freed=false;
	edict_t *bot;

	for(i=0;i<maxclients->value;i++)
	{
		bot = g_edicts + i + 1;
		if(bot->inuse)
		{
			if(bot->is_bot && (strcmp(bot->client->pers.netname,name)==0))
			{
			
				if(ctf->value)
					CTFDeadDropFlag(bot);

				DeadDropDeathball(bot);	

				if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value)  //adjust teams and scores
				{
					if(bot->dmteam == BLUE_TEAM)
						blue_team_cnt-=1;
					else
						red_team_cnt-=1;
				}
				
				//safe_bprintf(PRINT_HIGH, "(kicked) red: %i blue: %i\n", red_team_cnt, blue_team_cnt);
			
				// send effect
				gi.WriteByte (svc_muzzleflash);
				gi.WriteShort (bot-g_edicts);
				gi.WriteByte (MZ_LOGOUT);
				gi.multicast (bot->s.origin, MULTICAST_PVS);

				bot->deadflag = DEAD_DEAD;
				freed = true;
				gi.unlinkentity (bot);
				bot->s.modelindex = 0;
				bot->solid = SOLID_NOT;
				bot->inuse = false;
				bot->classname = "disconnected";
				bot->client->pers.connected = false;

				safe_bprintf (PRINT_MEDIUM, "%s was kicked\n", bot->client->pers.netname);
			}
			if(freed) {
				bot->client->resp.botnum--; //we have one less bot
				bot->client->ps.botnum = bot->client->resp.botnum;
			}

		}
	}

	if(!freed)	
		safe_bprintf (PRINT_MEDIUM, "%s not found\n", name);
}