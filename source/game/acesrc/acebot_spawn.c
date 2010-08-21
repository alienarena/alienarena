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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game/g_local.h"
#include "game/m_player.h"
#include "acebot.h"

static size_t szr; // just for unused result warnings

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
	char full_path[MAX_OSPATH];

	gi.FullWritePath( full_path, sizeof(full_path), BOT_GAMEDATA"/bots.tmp" );
	if ( ( pOut = fopen( full_path, "wb" )) == NULL )
	{
		gi.dprintf("ACESP_SaveBots: fopen for write failed: %s\n", full_path );
		return;
	}

	// Get number of bots
	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;

		if (bot->inuse && bot->is_bot)
			count++;
	}

	szr = fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	for (i = maxclients->value; i > 0; i--)
	{
		bot = g_edicts + i + 1;

		if (bot->inuse && bot->is_bot)
			szr = fwrite(bot->client->pers.userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut);
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
	int i, j, count, spawnkicknum;
	char *info;
	char *skin;
	char bot_filename[MAX_OSPATH];
	char stem[MAX_QPATH];
	int found;
	int real_players, total_players;
	edict_t *cl_ent;

	if (((int)(dmflags->value) & DF_BOTS)) {
		return; // don't load any bots.
	}

	//bots and configurations will be loaded level specific
	// note: see FindBotNum(), custombots has priority over team. (?)
	if(sv_custombots->value)
		sprintf( stem, BOT_GAMEDATA"/custom%i.tmp", sv_custombots->integer);
	else if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value)
		strcpy( stem, BOT_GAMEDATA"/team.tmp");
	else
		sprintf( stem, BOT_GAMEDATA"/%s.tmp", level.mapname);

	if ( !gi.FullPath( bot_filename, sizeof(bot_filename), stem ))
	{
		gi.dprintf( "ACESP_LoadBots: not found: %s\n", stem );
		return;
	}
	if((pIn = fopen(bot_filename, "rb" )) == NULL)
	{
		gi.dprintf("ACESP_LoadBots: failed fopen for read: %s", bot_filename );
		return;
	}

	szr = fread(&count,sizeof (int),1,pIn);

	if(g_duel->value) {
		count = 1; //never more than 1 bot no matter what in duel mode
		spawnkicknum = 2;
	}
	else if(sv_botkickthreshold->integer)
		spawnkicknum = sv_botkickthreshold->integer;
	else
		spawnkicknum = 0;

	ent->client->resp.botnum = 0;

	//probably want to count total REAL players here...
	real_players = 0;

	if(spawnkicknum) { //test for now
		for (i=0 ; i<game.maxclients ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (cl_ent->inuse && !cl_ent->is_bot){
				cl_ent->client->resp.botnum = 0;
				if(g_duel->value)
					real_players++;
				else if(!game.clients[i].resp.spectator)
					real_players++;
			}
		}
		if(count > spawnkicknum) //no need to load more bots than this number
			count = spawnkicknum;
	}

	real_players -= playerleft; //done for when this is called as a player is disconnecting
	total_players = 0;

	for(i=0;i<count;i++)
	{
		total_players = real_players + i + 1;

		szr = fread(userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn);

		info = Info_ValueForKey (userinfo, "name");
		skin = Info_ValueForKey (userinfo, "skin");

		strcpy(ent->client->resp.bots[i].name, info);

		if(spawnkicknum) {
				for (j=0 ; j<game.maxclients ; j++)
				{
					cl_ent = g_edicts + 1 + j;
					if (cl_ent->inuse) {
						if(total_players <= spawnkicknum) //we actually added one
							cl_ent->client->resp.botnum = i+1;
						cl_ent->client->ps.botnum = cl_ent->client->resp.botnum;
						strcpy(cl_ent->client->ps.bots[i].name, info);
					}
				}
		}
		else {
			ent->client->resp.botnum++;
		}

		//look for existing bots of same name, so that server doesn't fill up with bots
		//when clients enter the game
		found = false;
		found = ACESP_FindBot(info);

		if(!found && ((total_players <= spawnkicknum) || !spawnkicknum)) {

			if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value)
				ACESP_SpawnBot(info, skin, NULL); //we may be changing the info further on
			else
				ACESP_SpawnBot (NULL, NULL, userinfo);
		}
		else if(found && ((total_players > spawnkicknum) && spawnkicknum))
			ACESP_KickBot(info);

	}

    fclose(pIn);

}

int ACESP_FindBotNum(void)
{
    FILE *pIn;
	int count;
	char bot_filename[MAX_OSPATH];
	char stem[MAX_QPATH];

	//bots and configurations are loaded level specific
	// note: see LoadBots, custombots have priority over team (?)
	if(sv_custombots->value)
		sprintf( stem, BOT_GAMEDATA"/custom%i.tmp", sv_custombots->integer);
	else if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value)
		strcpy( stem, BOT_GAMEDATA"/team.tmp");
	else
		sprintf( stem, BOT_GAMEDATA"/%s.tmp", level.mapname);

	if ( !gi.FullPath( bot_filename, sizeof(bot_filename), stem ))
	{
		gi.dprintf( "ACESP_FindBotNum: not found: %s\n", stem );
		return 0;
	}
	if((pIn = fopen(bot_filename, "rb" )) == NULL)
	{
		gi.dprintf("ACESP_FindBotNum: failed fopen for read: %s", bot_filename );
		return 0;
	}

	szr = fread(&count,sizeof (int),1,pIn);

	fclose(pIn);

	return count;
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

/*===
  ACECO_ReadConfig()

  System-independent bot configuration file reader.
  2010-08: Replaces function in acebot_config.cpp for Windows

  To be called with relative path to the .cfg file.

===*/
void ACECO_ReadConfig( char *config_file )
{
	char full_path[ MAX_OSPATH];
	FILE *fp;
	int k;
	size_t length, result;
	char *buffer;
	char *s;
	const char *delim = "\r\n";

	//set bot defaults(in case no bot config file is present for that bot)
	botvals.skill = 1; //medium
	strcpy(botvals.faveweap, "None");
	for(k = 1; k < 10; k++)
		botvals.weapacc[k] = 1.0;
	botvals.awareness = 0.7;

	strcpy( botvals.chatmsg1, "%s: You are a real jerk %s!"    );
	strcpy( botvals.chatmsg2, "%s: Wait till next time %s."    );
	strcpy( botvals.chatmsg3, "%s: Life was better alive, %s!" );
	strcpy( botvals.chatmsg4, "%s: You will pay for this %s!"  );
	strcpy( botvals.chatmsg5, "%s: You're using a bot %s!"     );
	strcpy( botvals.chatmsg6, "%s: I will be hunting you %s!"  );
	strcpy( botvals.chatmsg7, "%s: It hurts %s...it hurts..."  );
	strcpy( botvals.chatmsg8, "%s: Just a lucky shot %s!"      );

	if ( !gi.FullPath( full_path, sizeof(full_path), config_file ) )
	{ // bot not configured, use defaults
		return;
	}
	if ( (fp = fopen( full_path, "rb" )) == NULL )
	{
		gi.dprintf("ACECO_ReadConfig: failed open for read: %s\n", full_path );
		return;
	}
	if ( fseek(fp, 0, SEEK_END) )
	{ // seek error
		fclose( fp );
		return;
	}
	if ( (length = ftell(fp)) < 0L )
	{ // tell error
		fclose( fp );
		return;
	}
	if ( fseek(fp, 0, SEEK_SET) )
	{ // seek error
		fclose( fp );
		return;
	}
	buffer = malloc( length + 1);
	if ( buffer == NULL )
	{ // memory allocation error
		fclose( fp );
		return;
	}
	result = fread( buffer, 1, length, fp );
	fclose( fp );
	if ( result != length )
	{ // read error
		free( buffer );
		return;
	}
	buffer[length] = 0;

	// note: malloc'd buffer is modified by strtok
	if ( (s = strtok( buffer, delim )) != NULL )
		botvals.skill = atoi( s );

	if ( s &&  ((s = strtok( NULL, delim )) != NULL) )
		strncpy( botvals.faveweap, s, sizeof(botvals.faveweap)-1 );

	for(k = 1; k < 10; k++) {
		if ( s && ((s = strtok( NULL, delim )) != NULL ) )
			botvals.weapacc[k] = atof( s );
	}

	if ( s && ((s = strtok( NULL, delim)) != NULL) )
		botvals.awareness = atof( s );

	if ( s && ((s = strtok( NULL, delim)) != NULL) )
		strncpy( botvals.chatmsg1, s, sizeof(botvals.chatmsg1)-1 );
	if ( s && ((s = strtok( NULL, delim)) != NULL) )
		strncpy( botvals.chatmsg2, s, sizeof(botvals.chatmsg2)-1 );
	if ( s && ((s = strtok( NULL, delim)) != NULL) )
		strncpy( botvals.chatmsg3, s, sizeof(botvals.chatmsg3)-1 );
	if ( s && ((s = strtok( NULL, delim)) != NULL) )
		strncpy( botvals.chatmsg4, s, sizeof(botvals.chatmsg4)-1 );
	if ( s && ((s = strtok( NULL, delim)) != NULL) )
		strncpy( botvals.chatmsg5, s, sizeof(botvals.chatmsg5)-1 );
	if ( s && ((s = strtok( NULL, delim)) != NULL) )
		strncpy( botvals.chatmsg6, s, sizeof(botvals.chatmsg6)-1 );
	if ( s && ((s = strtok( NULL, delim)) != NULL) )
		strncpy( botvals.chatmsg7, s, sizeof(botvals.chatmsg7)-1 );
	if ( s && ((s = strtok( NULL, delim)) != NULL) )
		strncpy( botvals.chatmsg8, s, sizeof(botvals.chatmsg8)-1 );

	free( buffer );

}

///////////////////////////////////////////////////////////////////////
// Modified version of id's code
///////////////////////////////////////////////////////////////////////
void ACESP_PutClientInServer (edict_t *bot, qboolean respawn, int team)
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 32};
	int		index, armor_index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	gitem_t		*item;
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
	if(g_spawnprotect->value)
		bot->client->spawnprotected = true;
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

	sprintf(modelpath, "players/%s/helmet.md2", playermodel);
	Q2_FindFile (modelpath, &file); //does a helmet exist?
	if(file) {
		sprintf(modelpath, "players/%s/helmet.md2", playermodel);
		bot->s.modelindex3 = gi.modelindex(modelpath);
		fclose(file);
	}
	else
		bot->s.modelindex3 = 0;

	//check for class file
	bot->ctype = 0;
	sprintf(modelpath, "players/%s/human", playermodel);
	Q2_FindFile (modelpath, &file);
	if(file) { //human
		bot->ctype = 1;
		if(classbased->value && !(rocket_arena->value || instagib->value || excessive->value)) {
				armor_index = ITEM_INDEX(FindItem("Jacket Armor"));
				client->pers.inventory[armor_index] += 30;
				client->pers.inventory[ITEM_INDEX(FindItem("Rocket Launcher"))] = 1;
				client->pers.inventory[ITEM_INDEX(FindItem("rockets"))] = 10;
				item = FindItem("Rocket Launcher");
				client->pers.selected_item = ITEM_INDEX(item);
				client->pers.inventory[client->pers.selected_item] = 1;
				client->pers.weapon = item;
		}
		fclose(file);
	}
	else {
		sprintf(modelpath, "players/%s/robot", playermodel);
		Q2_FindFile (modelpath, &file);
		if(file) { //robot
			bot->ctype = 2;
			if(classbased->value && !(rocket_arena->value || instagib->value || excessive->value)) {
				bot->health = bot->max_health = client->pers.max_health = client->pers.health = 85;
				armor_index = ITEM_INDEX(FindItem("Jacket Armor"));
				client->pers.inventory[armor_index] += 175;
			}
			fclose(file);
		}
		else { //alien
			if(classbased->value && !(rocket_arena->value || instagib->value || excessive->value)) {
				bot->health = bot->max_health = client->pers.max_health = client->pers.health = 150;
				client->pers.inventory[ITEM_INDEX(FindItem("Alien Disruptor"))] = 1;
				client->pers.inventory[ITEM_INDEX(FindItem("cells"))] = 100;
				item = FindItem("Alien Disruptor");
				client->pers.selected_item = ITEM_INDEX(item);
				client->pers.inventory[client->pers.selected_item] = 1;
				client->pers.weapon = item;
			}
		}
	}

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
		sprintf( bot_configfilename, BOT_GAMEDATA"/%s.cfg", info );
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
	client->spawnprotecttime = level.time;

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
	edict_t *bot = NULL;
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
void ACESP_SetName(edict_t *bot, char *name, char *skin)
{
	float rnd;
	char userinfo[MAX_INFO_STRING];
	char bot_skin[MAX_INFO_STRING];
	char bot_name[MAX_INFO_STRING];
	char playerskin[MAX_INFO_STRING];
	char playermodel[MAX_INFO_STRING];
	int i, j, k, copychar;
	char *skin2;

	// Set the name for the bot.
	// name
	if(strlen(name) == 0)
	{
		sprintf(bot_name,"ACEBot_%d",bot->count);
		sprintf(bot_skin,"martianenforcer/default");
		skin2 = bot_skin;
	}
	else
	{
		strcpy(bot_name,name);
		skin2 = skin;
	}

	bot->dmteam = NO_TEAM; //default

	// skin

	if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) //only do this for skin teams, red, blue
	{
		copychar = false;
		strcpy(playerskin, " ");
		strcpy(playermodel, " ");
		j = k = 0;
		for(i = 0; i <= strlen(skin2) && i < MAX_INFO_STRING; i++)
		{
			if(copychar){
				playerskin[k] = skin2[i];
				k++;
			}
			else {
				playermodel[j] = skin2[i];
				j++;
			}
			if(skin2[i] == '/')
				copychar = true;


		}
		playermodel[j] = 0;

		if(blue_team_cnt < red_team_cnt)
		{
			strcpy(playerskin, "blue");
			//blue_team_cnt++;
			bot->dmteam = BLUE_TEAM;
		}
		else
		{
			strcpy(playerskin, "red");
			//red_team_cnt++;
			bot->dmteam = RED_TEAM;
		}

		strcpy(skin2, playermodel);
		strcat(skin2, playerskin);

	}

	if(strlen(skin2) == 0)
	{
		// randomly choose skin
		rnd = random();
		if(rnd  < 0.5)
			sprintf(bot_skin,"martianenforcer/red");

		else
			sprintf(bot_skin,"martianenforcer/blue");
	}
	else
		strcpy(bot_skin,skin2);

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
void ACESP_SpawnBot (char *name, char *skin, char *userinfo)
{
	edict_t *bot, *cl_ent;
	int	i;

	bot = ACESP_FindFreeClient ();

	if (!bot)
	{
		safe_bprintf (PRINT_MEDIUM, "Server is full, increase Maxclients.\n");
		return;
	}

	bot->yaw_speed = 37; // yaw speed. angle in degrees
	bot->inuse = true;
	bot->is_bot = true;

	// To allow bots to respawn
	if(userinfo == NULL)
		ACESP_SetName(bot, name, skin);
	else
	{
		bot->dmteam = NO_TEAM; //default
		if(!ClientConnect (bot, userinfo))
		{
			/* Tony: Sometimes bots are refused entry to servers - give up gracefully */
			safe_bprintf (PRINT_MEDIUM, "Bot was refused entry to server.\n");
			bot->inuse = false;
			bot->is_bot = false;
			return;
		}
	}

	G_InitEdict (bot);

	InitClientResp (bot->client);

	game.num_bots = 0;
	for (i=0; i<game.maxclients ; i++) //this is a more safe way to do this, ensuring we have perfect numbers
	{
		cl_ent = g_edicts + 1 + i;
		if (cl_ent->inuse && cl_ent->is_bot)
			game.num_bots++;
	}

	ACESP_PutClientInServer (bot,false,0);

	if(g_duel->value) {
		ClientPlaceInQueue(bot);
		ClientCheckQueue(bot);
	}

	// make sure all view stuff is valid
	ClientEndServerFrame (bot);

	ACEAI_PickLongRangeGoal(bot); // pick a new goal

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

///////////////////////////////////////////////////////////////////////
// Remove/Kick Bots
///////////////////////////////////////////////////////////////////////

/*===

 match_botname()

 case-sensitive name string compare, stripping color codes

===*/
static qboolean match_botname( edict_t *bot, const char *match_string )
{
	char *bot_netname = bot->client->pers.netname;
	size_t bot_netname_sizecnt = sizeof( bot->client->pers.netname );
	char *pchar_in = bot_netname;
	const char *pchar_match = match_string;
	qboolean matched = false;

	while ( *pchar_in && *pchar_match && bot_netname_sizecnt-- )
	{
		if ( *pchar_in == '^' )
		{ // escape char for color codes
			++pchar_in;
			if ( *pchar_in && bot_netname_sizecnt )
			{ // skip over color code
				++pchar_in;
				--bot_netname_sizecnt;
			}
		}
		else
		{
			if ( *pchar_in != *pchar_match )
			{ // no match
				break;
			}
			++pchar_in;
			++pchar_match;
			}
		if ( ( matched = !(*pchar_in) && !(*pchar_match) ) )
		{ // at end of both strings
			break;
		}
	}

	return matched;
}

/*===
 remove_bot()

 common routine for removal or kick of a bot
 adapted from player_die(), and previous ACESP_RemoveBot(), ACESP_KickBot()

=== */
static void remove_bot( edict_t *bot )
{

	VectorClear( bot->avelocity );

	if ( bot->in_vehicle )
	{
		VehicleDeadDrop( bot );
	}

	if(ctf->value)
	{
		CTFDeadDropFlag(bot);
	}

	if ( bot->in_deathball )
	{
		DeadDropDeathball(bot);
	}

	if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value
			|| cp->value)
	{ // team game, adjust teams
		if(bot->dmteam == BLUE_TEAM)
			blue_team_cnt-=1;
		else if ( bot->dmteam == RED_TEAM )
			red_team_cnt-=1;
	}

	if ( g_duel->value )
	{// duel mode, we need to bump people down the queue if its the player in game leaving
		MoveClientsDownQueue(bot);
		if( !bot->client->resp.spectator )
		{ // bot was in duel
			int j;
			for ( j = 1; j <= maxclients->value; j++)
			{ // clear scores of other players
				if ( g_edicts[j].inuse && g_edicts[j].client )
					g_edicts[j].client->resp.score = 0;
			}
		}
	}

	bot->inuse = false;
	bot->solid = SOLID_NOT;
	bot->classname = "disconnected";

	bot->s.modelindex = 0;
	bot->s.modelindex2= 0;
	bot->s.modelindex3 = 0;
	bot->s.modelindex4 = 0;
	bot->s.angles[0] = 0;  // ?
	bot->s.angles[2] = 0;  // ?
	bot->s.sound = 0;
	bot->client->weapon_sound = 0;

	// remove powerups
	bot->client->quad_framenum = 0;
	bot->client->invincible_framenum = 0;
	bot->client->haste_framenum = 0;
	bot->client->sproing_framenum = 0;
	bot->client->invis_framenum = 0;

	// clear inventory
	memset( bot->client->pers.inventory, 0, sizeof(bot->client->pers.inventory));

	bot->client->pers.connected = false;
	bot->client->resp.botnum--; //we have one less bot
	bot->client->ps.botnum = bot->client->resp.botnum;

	// particle effect for exit from game
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (bot-g_edicts);
	gi.WriteByte (MZ_LOGOUT);
	gi.multicast (bot->s.origin, MULTICAST_PVS);

	// blank the skin? not sure of purpose.
	gi.configstring( CS_PLAYERSKINS + ( ((ptrdiff_t)(bot - g_edicts))-1 ), "");

	// unlink from world
	gi.unlinkentity (bot);

}

// remove by server command, "removebot"
void ACESP_RemoveBot(char *name)
{
	int i;
	qboolean freed=false;
	edict_t *bot;

	for ( i=1; i <= maxclients->value; i++ )
	{
		bot = &g_edicts[i];
		if( bot->inuse && bot->is_bot )
		{ // client slot in use and is a bot
			if ( match_botname( bot, name ) || !strncmp(name,"all",3) )
			{ // case-sensitive, non-color-code sensitive match
				remove_bot( bot );
				freed = true; // one, or at least one of all removed
				game.num_bots--;
				safe_bprintf (PRINT_MEDIUM, "%s removed\n", bot->client->pers.netname);
			}
		}
	}

	if(!freed)
		safe_bprintf (PRINT_MEDIUM, "%s not found\n", name);

	// update bots.tmp
	ACESP_SaveBots();

}

// remove by automatic bot kick or by player vote
void ACESP_KickBot(char *name)
{
	int i;
	qboolean freed=false;
	edict_t *bot;

	for ( i=1; i <= maxclients->value; i++ )
	{
		bot = &g_edicts[i];
		if ( bot->inuse && bot->is_bot )
		{ // client slot in use and is a bot
			if ( match_botname( bot, name ))
			{ // case-sensitive, color-code-insensitive match
				remove_bot( bot );
				freed = true;
				game.num_bots--;
				safe_bprintf (PRINT_MEDIUM, "%s kicked\n", bot->client->pers.netname);
			}
		}
	}

	if(!freed)
		safe_bprintf (PRINT_MEDIUM, "%s not found\n", name);
}
