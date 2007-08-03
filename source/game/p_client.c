#include "g_local.h"
#include "m_player.h"

/* Number of gibs to throw on death with lots of damage (including Client Head, where applicable) */
#define DEATH_GIBS_TO_THROW 5
void ClientUserinfoChanged (edict_t *ent, char *userinfo, int whereFrom);
void SP_misc_teleporter_dest (edict_t *ent);

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
The normal starting point for a level.
*/
void SP_info_player_start(edict_t *self)
{
	return;
}

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for deathmatch games
*/
void SP_info_player_deathmatch(edict_t *self)
{
	if (!deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	//SP_misc_teleporter_dest (self);
}
void SP_info_player_red(edict_t *self)
{
	if (!deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	//SP_misc_teleporter_dest (self);
}
void SP_info_player_blue(edict_t *self)
{
	if (!deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	//SP_misc_teleporter_dest (self);
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The deathmatch intermission point will be at one of these
Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
*/
void SP_info_player_intermission(void)
{
}


//=======================================================================


void player_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	// player pain is handled at the end of the frame in P_DamageFeedback
	if(self->is_bot)
		self->oldenemy = other;
}


qboolean IsFemale (edict_t *ent)
{
	char		*info;

	if (!ent->client)
		return false;

	info = Info_ValueForKey (ent->client->pers.userinfo, "skin");
	if (info[0] == 'f' || info[0] == 'F')
		return true;
	return false;
}


void ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	int		mod, msg;
	char		*message;
	char		*message2;
	qboolean	ff;
	char		*chatmsg;
	char		*tauntmsg;
	char		cleanname[16], cleanname2[16];
	int			i, j, pos, total, place;
	edict_t		*cl_ent;

	if (deathmatch->value)
	{
		ff = meansOfDeath & MOD_FRIENDLY_FIRE;
		mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
		message = NULL;
		message2 = "";

		switch (mod)
		{
		case MOD_SUICIDE:
			message = "suicides";
			break;
		case MOD_FALLING:
			message = "cratered";
			break;
		case MOD_CRUSH:
			message = "was squished";
			break;
		case MOD_WATER:
			message = "sank like a rock";
			break;
		case MOD_SLIME:
			message = "melted";
			break;
		case MOD_LAVA:
			message = "does a back flip into the lava";
			break;
		case MOD_EXPLOSIVE:
		case MOD_BARREL:
			message = "blew up";
			break;
		case MOD_EXIT:
			message = "found a way out";
			break;
		case MOD_TARGET_LASER:
			message = "saw the light";
			break;
		case MOD_TARGET_BLASTER:
			message = "got blasted";
			break;
		case MOD_BOMB:
		case MOD_SPLASH:
		case MOD_TRIGGER_HURT:
			message = "was in the wrong place";
			break;
		}
		if (attacker == self)
		{
			switch (mod)
			{
			case MOD_CAMPING:
				message = "was killed for camping";
				break;
			case MOD_PLASMA_SPLASH:
				if (IsFemale(self))
					message = "melted herself";
				else
					message = "melted himself";
				break;
			case MOD_R_SPLASH:
				if (IsFemale(self))
					message = "blew herself up";
				else
					message = "blew himself up";
				break;
			case MOD_VAPORIZER:
				message = "should have used a smaller gun";
				break;
			default:
				if (IsFemale(self))
					message = "killed herself";
				else
					message = "killed himself";
				break;
			}
		}
		if (message)
		{
			safe_bprintf (PRINT_MEDIUM, "%s %s.\n", self->client->pers.netname, message);
			if (deathmatch->value) {
				self->client->resp.score--;
				self->client->resp.deaths++;
			}
			self->enemy = NULL;
			self->client->kill_streak = 0; //reset, you are dead
			return;
		}

		self->enemy = attacker;
		if (attacker && attacker->client)
		{
			//clean up name, get rid of escape chars
			j = 0;
			for (i = 0; i < 16; i++)
				cleanname[i] = 0;
			for (i = 0; i < strlen(self->client->pers.netname) && i < 16; i++) {
				if ( self->client->pers.netname[i] == '^' ) {
					i += 1;
					continue;
				}
				cleanname[j] = self->client->pers.netname[i]+128;
				j++;
			}
			//clean up name, get rid of escape chars
			j = 0;
			for (i = 0; i < 16; i++)
				cleanname2[i] = 0;
			for (i = 0; i < strlen(attacker->client->pers.netname) && i < 16; i++) {
					if ( attacker->client->pers.netname[i] == '^' ) {
						i += 1;
						continue;
					}
				cleanname2[j] = attacker->client->pers.netname[i]+128;
				j++;
			}
			if(!attacker->is_bot) {
				pos = 0;
				total = 0;
				for (i=0 ; i<game.maxclients ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || game.clients[i].resp.spectator)
						continue;

					if(attacker->client->resp.score+1 >= game.clients[i].resp.score)
						pos++;

					total++;
				}
				place = total - pos;
				if(place < 3) {
					switch(place) {
					case 0:
						safe_centerprintf(attacker, "You fragged %s\n1st place with %i frags\n", cleanname, attacker->client->resp.score+1);
						break;
					case 1:
						safe_centerprintf(attacker, "You fragged %s\n2nd place with %i frags\n", cleanname, attacker->client->resp.score+1);
						break;
					case 2:
						safe_centerprintf(attacker, "You fragged %s\n3rd place with %i frags\n", cleanname, attacker->client->resp.score+1);
						break;
					default:
						break;
					}
				}
				else
					safe_centerprintf(attacker, "You fragged %s\n", cleanname);

			}

			switch (mod)
			{
			case MOD_BLASTER:
				message = "was blasted by";
				break;
			case MOD_CGALTFIRE:
				message = "was blown away by";
				message2 = "'s chaingun burst";
				break;
			case MOD_CHAINGUN:
				message = "was cut in half by";
				message2 = "'s chaingun";
				break;
			case MOD_FLAME:
				message = "was burned by";
				message2 = "'s napalm";
				break;
			case MOD_ROCKET:
				message = "ate";
				message2 = "'s rocket";
				break;
			case MOD_R_SPLASH:
				message = "almost dodged";
				message2 = "'s rocket";
				break;
			case MOD_BEAMGUN:
				message = "was melted by";
				message2 = "'s beamgun";
				break;
			case MOD_DISRUPTOR:
				message = "was disrupted by";
				break;
			case MOD_SMARTGUN:
				message = "saw the pretty lights from";
				message2 = "'s smartgun";
				break;
			case MOD_VAPORIZER:
				message = "was disintegrated by";
				message2 = "'s vaporizer blast";
				break;
			case MOD_VAPORALTFIRE:
				message = "couldn't hide from";
				message2 = "'s vaporizer";
				break;
			case MOD_PLASMA_SPLASH: //blaster splash damage
				message = "was melted";
				message2 = "'s plasma";
				break;
			case MOD_TELEFRAG:
				message = "tried to invade";
				message2 = "'s personal space";
				break;
			case MOD_GRAPPLE:
				message = "was caught by";
				message2 = "'s grapple";
				break;
			}
			//here is where the bot chat features will be added.
			//default is on.  Setting to 1 turns it off.

#ifndef __unix__
			if ((!((int)(dmflags->value) & DF_BOTCHAT)) && self->is_bot)
			{
				msg = random() * 12;
				switch(msg){
				case 1:
					chatmsg = self->chatmsg1;
					break;
				case 2:
					chatmsg = self->chatmsg2;
					break;
				case 3:
					chatmsg = self->chatmsg3;
					break;
				case 4:
					chatmsg = self->chatmsg4;
					break;
				case 5:
					chatmsg = self->chatmsg5;
					break;
				case 6:
					chatmsg = self->chatmsg6;
					break;
				case 7:
					chatmsg = self->chatmsg7;
					break;
				case 8:
					chatmsg = self->chatmsg8;
					break;
				default:
					chatmsg = "";
					break;
				}
				safe_bprintf (PRINT_CHAT, chatmsg, self->client->pers.netname, attacker->client->pers.netname);
				safe_bprintf (PRINT_CHAT, "\n");

				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_SAYICON);
				gi.WritePosition (self->s.origin);
				gi.multicast (self->s.origin, MULTICAST_PVS);
			}
#else
			if ((!((int)(dmflags->value) & DF_BOTCHAT)) && self->is_bot)
			{
				msg = random() * 12;
				switch(msg){
				case 1:
					chatmsg = "%s: You are a real jerk %s!";
					break;
				case 2:
					chatmsg = "%s: Stop it %s, you punk!";
					break;
				case 3:
					chatmsg = "%s: Life was better alive, %s!";
					break;
				case 4:
					chatmsg = "%s: You will pay for this %s..";
					break;
				case 5:
					chatmsg = "%s: Wait till next time %s.";
					break;
				case 6:
					chatmsg = "%s: NOOOOO %s!!!";
					break;
				case 7:
					chatmsg = "%s: It hurts %s...it hurts...";
					break;
				case 8:
					chatmsg = "%s: You're using a bot %s!";
					break;
				default:
					chatmsg = "";
					break;
				}
				safe_bprintf (PRINT_CHAT, chatmsg, self->client->pers.netname, attacker->client->pers.netname);
				safe_bprintf (PRINT_CHAT, "\n");

				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_SAYICON);
				gi.WritePosition (self->s.origin);
				gi.multicast (self->s.origin, MULTICAST_PVS);
			}
#endif
			//bot taunts
			if(attacker->is_bot && (!attacker->client->ps.pmove.pm_flags & PMF_DUCKED) && attacker->skill == 3) {

				attacker->state = STATE_STAND;
				attacker->s.frame = FRAME_taunt01-1;
				attacker->client->anim_end = FRAME_taunt17;

				//print a taunt
				msg = random() * 9;
				switch(msg){
				case 1:
					tauntmsg = "%s: You should have used a bigger gun %s.\n";
					break;
				case 2:
					tauntmsg = "%s: You fight like your mom %s.\n";
					break;
				case 3:
					tauntmsg = "%s: And stay down %s!\n";
					break;
				case 4:
					tauntmsg = "%s: %s = pwned!\n";
					break;
				case 5:
					tauntmsg = "%s: All too easy, %s, all too easy.\n";
					break;
				case 6:
					tauntmsg = "%s: Ack! %s Ack! Ack!\n";
					break;
				case 7:
					tauntmsg = "%s: What a loser you are %s!\n";
					break;
				case 8:
					tauntmsg = "%s: %s, could you BE any more dead?\n";
					break;
				default:
					tauntmsg = "%s: You are useless to me, %s\n";
					break;
				}
				safe_bprintf (PRINT_CHAT, tauntmsg, attacker->client->pers.netname, self->client->pers.netname);
				//send an effect to show that the bot is taunting
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_SAYICON);
				gi.WritePosition (attacker->s.origin);
				gi.multicast (attacker->s.origin, MULTICAST_PVS);
			}

			if (message)
			{
				safe_bprintf (PRINT_MEDIUM,"%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);

				if (deathmatch->value)
				{
					if (ff) {
						attacker->client->resp.score--;
						attacker->client->resp.deaths++;
						if (((int)(dmflags->value) & DF_SKINTEAMS) && !ctf->value) {
							if(attacker->dmteam == RED_TEAM)
								red_team_score--;
							else
								blue_team_score--;
						}
					}
					else {
						attacker->client->resp.score++;

						//mutators
						if(vampire->value) {
							attacker->health+=20;
							if(attacker->health > attacker->max_health)
								attacker->health = attacker->max_health;
						}
						self->client->resp.deaths++;
						if (((int)(dmflags->value) & DF_SKINTEAMS)  && !ctf->value) {
							if(attacker->dmteam == RED_TEAM){
								red_team_score++;
								safe_bprintf(PRINT_MEDIUM, "Red Team scores!\n");
								gi.sound (self, CHAN_AUTO, gi.soundindex("misc/red_scores.wav"), 1, ATTN_NONE, 0);
							}
							else {
								blue_team_score++;
								safe_bprintf(PRINT_MEDIUM, "Blue Team scores!\n");
								gi.sound (self, CHAN_AUTO, gi.soundindex("misc/blue_scores.wav"), 1, ATTN_NONE, 0);

							}
						}
						//kill streaks
						attacker->client->kill_streak++;
						switch(attacker->client->kill_streak) {
							case 3:
								for (i=0 ; i<maxclients->value ; i++)
								{
									cl_ent = g_edicts + 1 + i;
									if (!cl_ent->inuse || cl_ent->is_bot)
										continue;
									safe_centerprintf(cl_ent, "%s is on a killing spree!\n", cleanname2);
								}
								break;
							case 5:
								for (i=0 ; i<maxclients->value ; i++)
								{
									cl_ent = g_edicts + 1 + i;
									if (!cl_ent->inuse || cl_ent->is_bot)
										continue;
									safe_centerprintf(cl_ent, "%s is on a rampage!\n", cleanname2);
								}
								gi.sound (self, CHAN_AUTO, gi.soundindex("misc/rampage.wav"), 1, ATTN_NONE, 0);
								break;
							case 8:
								for (i=0 ; i<maxclients->value ; i++)
								{
									cl_ent = g_edicts + 1 + i;
									if (!cl_ent->inuse || cl_ent->is_bot)
										continue;
									safe_centerprintf(cl_ent, "%s is unstoppable!\n", cleanname2);
								}
								break;
							case 10:
								for (i=0 ; i<maxclients->value ; i++)
								{
									cl_ent = g_edicts + 1 + i;
									if (!cl_ent->inuse || cl_ent->is_bot)
										continue;
									safe_centerprintf(cl_ent, "%s is a god!\n", cleanname2);
								}
								gi.sound (self, CHAN_AUTO, gi.soundindex("misc/godlike.wav"), 1, ATTN_NONE, 0);
								break;
							default:
								break;
						}
						if(self->client->kill_streak >=3) {
							for (i=0 ; i<maxclients->value ; i++)
								{
									cl_ent = g_edicts + 1 + i;
									if (!cl_ent->inuse || cl_ent->is_bot)
										continue;
									safe_centerprintf(cl_ent, "%s's killing spree\nended by %s!\n", cleanname, cleanname2);
							}
						}
					}

				}
				self->client->kill_streak = 0; //reset, you are dead
				return;
			}
		}

	}

	safe_bprintf (PRINT_MEDIUM,"%s died.\n", self->client->pers.netname);
	if (deathmatch->value) {
		self->client->resp.score--;
		self->client->resp.deaths++;
	}

}


void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);

void TossClientWeapon (edict_t *self)
{
	gitem_t		*item;
	edict_t		*drop;
	qboolean	quad;
	qboolean	sproing;
	qboolean	haste;
	float		spread;

	if ((!deathmatch->value) || instagib->value || rocket_arena->value)
		return;

	item = self->client->pers.weapon;
	if (! self->client->pers.inventory[self->client->ammo_index] )
		item = NULL;
	if (item && (strcmp (item->pickup_name, "Blaster") == 0))
		item = NULL;

	if (!((int)(dmflags->value) & DF_QUAD_DROP))
		quad = false;
	else
		quad = (self->client->quad_framenum > (level.framenum + 10));

	sproing = (self->client->sproing_framenum > (level.framenum + 10));
	haste = (self->client->haste_framenum > (level.framenum + 10));

	if ((item && quad) || (item && haste) || (item && sproing))
		spread = 22.5;
	else
		spread = 0.0;

	if (item)
	{
		self->client->v_angle[YAW] -= spread;
		drop = Drop_Item (self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
	}

	if (quad)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_quad"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->quad_framenum - level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}
	if (sproing)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_sproing"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->sproing_framenum - level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}
	if (haste)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_haste"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->haste_framenum - level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller (edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	vec3_t		dir;

	if (attacker && attacker != world && attacker != self)
	{
		VectorSubtract (attacker->s.origin, self->s.origin, dir);
	}
	else if (inflictor && inflictor != world && inflictor != self)
	{
		VectorSubtract (inflictor->s.origin, self->s.origin, dir);
	}
	else
	{
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	self->client->killer_yaw = 180/M_PI*atan2(dir[1], dir[0]);
}

/*
==================
player_die
==================
*/
void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int	n;
	char	*info;
	gitem_t	*vehicle;
	int	got_vehicle = 0;
	int	number_of_gibs = 0;

	vehicle = FindItemByClassname("item_bomber");

	if (self->client->pers.inventory[ITEM_INDEX(vehicle)]) {
		Reset_player(self);	//get the player out of the vehicle
		Jet_Explosion(self); //blow that bitch up!
		got_vehicle = 1; //so we know how to handle dropping it
	}

	vehicle = FindItemByClassname("item_strafer");

	if (self->client->pers.inventory[ITEM_INDEX(vehicle)]) {
		Reset_player(self);	//get the player out of the vehicle
		Jet_Explosion(self); //blow that bitch up!
		got_vehicle = 1; //so we know how to handle dropping it
	}

	vehicle = FindItemByClassname("item_hover");

	if (self->client->pers.inventory[ITEM_INDEX(vehicle)]) {
		Reset_player(self);	//get the player out of the vehicle
		Jet_Explosion(self); //blow that bitch up!
		got_vehicle = 1; //so we know how to handle dropping it
	}

	VectorClear (self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;


	info = Info_ValueForKey (self->client->pers.userinfo, "skin");

	self->s.modelindex2 = 0;	// remove linked weapon model
	if(info[0] == 'b')		//fix this ugly crap before ver 3.0!
		self->s.modelindex4 = 0; //remove brainlet gunrack

	if(ctf->value)
		self->s.modelindex4 = 0;	// remove linked ctf flag

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -8;

	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag)
	{
		self->client->respawn_time = level.time + 3.8;

		//go into 3rd person view
		if (deathmatch->value)
			if(!self->is_bot)
				DeathcamStart(self);

		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary (self, inflictor, attacker);
		if(got_vehicle) //special for vehicles
			VehicleDeadDrop(self);
		else {
			if(!excessive->value)
				TossClientWeapon (self);
		}
		if(ctf->value)
			CTFDeadDropFlag(self);
		if(self->in_deathball)
			DeadDropDeathball(self);

		CTFPlayerResetGrapple(self);

		if (deathmatch->value)
			Cmd_Help_f (self);		// show scores

	}

	// remove powerups
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->haste_framenum = 0;
	self->client->sproing_framenum = 0;

	// clear inventory
	memset(self->client->pers.inventory, 0, sizeof(self->client->pers.inventory));

	if (self->health < -40)
	{	// gib
		self->takedamage	= DAMAGE_NO;
		self->s.modelindex3	= 0;    //remove helmet, if a martian
		self->s.modelindex4	= 0;    //war machine rider

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_DEATHFIELD);
		gi.WritePosition (self->s.origin);
		gi.multicast (self->s.origin, MULTICAST_PVS);

		if(self->client->chasetoggle == 1)
		{
			/* If deathcam is active, switch client model to nothing */
			self->s.modelindex = 0;
			self->solid = SOLID_NOT;

			number_of_gibs = DEATH_GIBS_TO_THROW;
		}
		else
		{
			/* No deathcam, handle player's view and model with ThrowClientHead() */
			ThrowClientHead (self, damage);
			number_of_gibs = DEATH_GIBS_TO_THROW - 1;
		}

		if(self->ctype == 0) { //alien

			for (n= 0; n < number_of_gibs; n++)
				ThrowGib (self, "models/objects/gibs/mart_gut/tris.md2", damage, GIB_ORGANIC, EF_GREENGIB);
		}
		else if(self->ctype == 2) { //robot
			for (n= 0; n < number_of_gibs; n++) {
				ThrowGib (self, "models/objects/debris3/tris.md2", damage, GIB_METALLIC, 0);
				ThrowGib (self, "models/objects/debris1/tris.md2", damage, GIB_METALLIC, 0);
			}
			//blow up too :)
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_ROCKET_EXPLOSION);
			gi.WritePosition (self->s.origin);
			gi.multicast (self->s.origin, MULTICAST_PHS);
		}
		else { //human
			for (n= 0; n < number_of_gibs; n++)
				ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC, EF_GIB);
		}

	}
	else
	{	// normal death
		if (!self->deadflag)
		{
			static int i;

			i = (i+1)%3;
			// start a death animation
			self->client->anim_priority = ANIM_DEATH;

			switch (i)
			{
			//all player models are now using the longer set of death frames only
			case 0:
				self->s.frame = FRAME_death501-1;
				self->client->anim_end = FRAME_death518;
				break;
			case 1:
				self->s.frame = FRAME_death601-1;
				self->client->anim_end = FRAME_death620;
				break;
			case 2:
				self->s.frame = FRAME_death401-1;
				self->client->anim_end = FRAME_death422;
				break;

			}
			gi.sound (self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand()%4)+1)), 1, ATTN_NORM, 0);
		}
	}

	self->deadflag = DEAD_DEAD;

	gi.linkentity (self);
}

//=======================================================================

/*
==============
InitClientPersistant

This is only called when the game first initializes in single player,
but is called after each death and level change in deathmatch
==============
*/
void InitClientPersistant (gclient_t *client)
{
	gitem_t		*item;
	int			queue;

	if(g_duel->value) //need to save this off in duel mode.  Potentially dangerous?
		queue = client->pers.queue;

	memset (&client->pers, 0, sizeof(client->pers));

	if(g_duel->value)
		client->pers.queue = queue;

	//mutator - will need to have item
	if(instagib->value) {
		client->pers.inventory[ITEM_INDEX(FindItem("Alien Disruptor"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("cells"))] = g_maxcells->value;
		item = FindItem("Alien Disruptor");
	}
	else if(rocket_arena->value) {
		client->pers.inventory[ITEM_INDEX(FindItem("Rocket Launcher"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("rockets"))] = g_maxrockets->value;
		item = FindItem("Rocket Launcher");
	}
	else
		item = FindItem("Blaster");

	client->pers.selected_item = ITEM_INDEX(item);
	client->pers.inventory[client->pers.selected_item] = 1;

	client->pers.weapon = item;

	if(excessive->value) {
		//Allow custom health, even in excessive.
		client->pers.health 		= g_spawnhealth->value * 3;
		client->pers.max_bullets 	= g_maxbullets->value * 2.5;
		client->pers.max_shells		= g_maxshells->value * 5;
		client->pers.max_rockets	= g_maxrockets->value * 10;
		client->pers.max_grenades	= g_maxgrenades->value * 10;
		client->pers.max_cells		= g_maxcells->value * 2.5;
		client->pers.max_slugs		= g_maxslugs->value * 10;

		client->pers.inventory[ITEM_INDEX(FindItem("Rocket Launcher"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("rockets"))] = g_maxrockets->value * 10;
		client->pers.inventory[ITEM_INDEX(FindItem("Pulse Rifle"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("bullets"))] = g_maxbullets->value * 2.5;
		client->pers.inventory[ITEM_INDEX(FindItem("Alien Disruptor"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("Disruptor"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("cells"))] = g_maxcells->value * 2.5;
		client->pers.inventory[ITEM_INDEX(FindItem("Alien Smartgun"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("alien smart grenade"))] = g_maxshells->value * 5;
		client->pers.inventory[ITEM_INDEX(FindItem("Alien Vaporizer"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("slugs"))] = g_maxslugs->value * 10;
		client->pers.inventory[ITEM_INDEX(FindItem("Flame Thrower"))] = 1;
		client->pers.inventory[ITEM_INDEX(FindItem("napalm"))] = g_maxgrenades->value * 10;
	} else {
		client->pers.health 		= g_spawnhealth->value;
		client->pers.max_bullets 	= g_maxbullets->value;
		client->pers.max_shells		= g_maxshells->value;
		client->pers.max_rockets	= g_maxrockets->value;
		client->pers.max_grenades	= g_maxgrenades->value;
		client->pers.max_cells		= g_maxcells->value;
		client->pers.max_slugs		= g_maxslugs->value;
	}

	if(vampire->value)
		client->pers.max_health = g_maxhealth->value * 2;
	else if(excessive->value)
		client->pers.max_health = g_maxhealth->value * 3;
	else
		client->pers.max_health = g_maxhealth->value;

	if(grapple->value) {
		item = FindItem("Grapple");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
	}

	client->pers.connected = true;
}


void InitClientResp (gclient_t *client)
{
	memset (&client->resp, 0, sizeof(client->resp));
	client->resp.enterframe = level.framenum;
}

/*
==================
SaveClientData

Some information that should be persistant, like health,
is still stored in the edict structure, so it needs to
be mirrored out to the client structure before all the
edicts are wiped.
==================
*/
void SaveClientData (void)
{
	int		i;
	edict_t	*ent;

	for (i=0 ; i<game.maxclients ; i++)
	{
		ent = &g_edicts[1+i];
		if (!ent->inuse)
			continue;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
	}
}

void FetchClientEntData (edict_t *ent)
{
	ent->health = ent->client->pers.health;
	ent->max_health = ent->client->pers.max_health;
//	if (ent->client->pers.powerArmorActive)
		ent->flags |= FL_POWER_ARMOR;
}



/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
PlayersRangeFromSpot

Returns the distance to the nearest player from the given spot
================
*/
float	PlayersRangeFromSpot (edict_t *spot)
{
	edict_t	*player;
	float	bestplayerdistance;
	vec3_t	v;
	int		n;
	float	playerdistance;


	bestplayerdistance = 9999999;

	for (n = 1; n <= maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
			continue;

		if (player->health <= 0)
			continue;

		VectorSubtract (spot->s.origin, player->s.origin, v);
		playerdistance = VectorLength (v);

		if (playerdistance < bestplayerdistance)
			bestplayerdistance = playerdistance;
	}

	return bestplayerdistance;
}

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
edict_t *SelectRandomDeathmatchSpawnPoint (void)
{
	edict_t	*spot, *spot1, *spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
	{
		if ( spot1 ) {
			count--;
		}
		if ( spot2 ) {
			count--;
		}
	}

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find (spot, FOFS(classname), "info_player_deathmatch");
		if (spot == spot1 || spot == spot2)
			selection++;
	} while(selection--);

	return spot;
}
/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point, but NOT the two points closest
to other players
================
*/
edict_t *SelectRandomCTFSpawnPoint (void)
{
	edict_t	*spot, *spot1, *spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_red")) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
	{
		if ( spot1 ) {
			count--;
		}
		if ( spot2 ) {
			count--;
		}
	}

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find (spot, FOFS(classname), "info_player_red");
		if (spot == spot1 || spot == spot2)
			selection++;
	} while(selection--);

	return spot;
}
/*
================
SelectFarthestDeathmatchSpawnPoint

================
*/
edict_t *SelectFarthestDeathmatchSpawnPoint (void)
{
	edict_t	*bestspot;
	float	bestdistance, bestplayerdistance;
	edict_t	*spot;


	spot = NULL;
	bestspot = NULL;
	bestdistance = 0;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL)
	{
		bestplayerdistance = PlayersRangeFromSpot (spot);

		if (bestplayerdistance > bestdistance)
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot)
	{
		return bestspot;
	}

	// if there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	spot = G_Find (NULL, FOFS(classname), "info_player_deathmatch");

	return spot;
}

edict_t *SelectDeathmatchSpawnPoint (void)
{
	if ( (int)(dmflags->value) & DF_SPAWN_FARTHEST)
		return SelectFarthestDeathmatchSpawnPoint ();
	else
		return SelectRandomDeathmatchSpawnPoint ();
}

/*
================
SelectCTFSpawnPoint

go to a ctf point, but NOT the two points closest
to other players
================
*/
edict_t *SelectCTFSpawnPoint (edict_t *ent)
{
	edict_t	*spot, *spot1, *spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;
	char	*cname;


	switch (ent->dmteam) {
	case RED_TEAM:
		cname = "info_player_red";
		break;
	case BLUE_TEAM:
		cname = "info_player_blue";
		break;
	case NO_TEAM:
	default:
		return SelectRandomCTFSpawnPoint();
	}

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find (spot, FOFS(classname), cname)) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);
		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return SelectRandomDeathmatchSpawnPoint();

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
		count -= 2;

	selection = rand() % count;

	spot = NULL;
	do
	{
		spot = G_Find (spot, FOFS(classname), cname);
		if (spot == spot1 || spot == spot2)
			selection++;
	} while(selection--);

	return spot;
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, coop start, etc
============
*/
void	SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles)
{
	edict_t	*spot = NULL;

	if (deathmatch->value) {
		if (ctf->value || tca->value || cp->value || ((int)(dmflags->value) & DF_SKINTEAMS)) {
			spot = SelectCTFSpawnPoint(ent);
			if(!spot)
				spot = SelectDeathmatchSpawnPoint ();
		}
		else {
			spot = SelectDeathmatchSpawnPoint ();
			if(!spot)
				spot = SelectCTFSpawnPoint(ent); //dm on team based maps
		}
	}

	// find a single player start spot
	if (!spot)
	{
		spot = G_Find (spot, FOFS(classname), "info_player_start");
		if (!spot)
			gi.error ("Couldn't find spawn point!");
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);
}

//======================================================================


void InitBodyQue (void)
{
	int		i;
	edict_t	*ent;

	level.body_que = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++)
	{
		ent = G_Spawn();
		ent->classname = "bodyque";
	}
}

void body_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int	n;
	self->s.modelindex3 = 0;    //remove helmet, if a martian
	self->s.modelindex4 = 0;
	if (self->health < -40)
	{
		if(self->ctype == 0) { //alien

			for (n= 0; n < 4; n++)
				ThrowGib (self, "models/objects/gibs/mart_gut/tris.md2", damage, GIB_ORGANIC, EF_GREENGIB);
		}
		else if(self->ctype == 2) { //robot
			for (n= 0; n < 4; n++)
				ThrowGib (self, "models/objects/debris3/tris.md2", damage, GIB_METALLIC, 0);
			for (n= 0; n < 4; n++)
				ThrowGib (self, "models/objects/debris1/tris.md2", damage, GIB_METALLIC, 0);
			//blow up too :)
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_ROCKET_EXPLOSION);
			gi.WritePosition (self->s.origin);
			gi.multicast (self->s.origin, MULTICAST_PHS);

		}
		else { //human
			for (n= 0; n < 4; n++)
				ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC, EF_GIB);
		}
		self->s.origin[2] -= 48;
		ThrowClientHead (self, damage);
		self->takedamage = DAMAGE_NO;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( edict_t *ent ) {
	if ( level.time - ent->timestamp > 10.5 ) {
		// the body ques are never actually freed, they are just unlinked
		gi.unlinkentity( ent );
		ent->solid = SOLID_NOT;
		ent->s.modelindex = 0; //for good measure
		ent->s.modelindex2 = 0;
		ent->s.modelindex3 = 0;
		ent->s.modelindex4 = 0;
		return;
	}
	ent->nextthink = level.time + .1;
	ent->s.origin[2] -= 1;
}

void CopyToBodyQue (edict_t *ent)
{
	edict_t		*body;

	// grab a body que and cycle to the next one
	body = &g_edicts[(int)maxclients->value + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	gi.unlinkentity (ent);

	gi.unlinkentity (body);
	body->s = ent->s;
	body->s.number = body - g_edicts;

	body->svflags = ent->svflags;

	VectorCopy (ent->mins, body->mins);
	VectorCopy (ent->maxs, body->maxs);
	VectorCopy (ent->absmin, body->absmin);
	VectorCopy (ent->absmax, body->absmax);
	VectorCopy (ent->size, body->size);
	body->solid = ent->solid;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;
	body->die = body_die;
	body->takedamage = DAMAGE_YES;
	body->ctype = ent->ctype;

	body->timestamp = level.time;
	body->nextthink = level.time + 5;
	body->think = BodySink;

	gi.linkentity (body);
}


void respawn (edict_t *self)
{
	if (deathmatch->value)
	{

#if 0
/* Don't think this is needed - deathcam is removed at very end of death,
   so DeathcamRemove would get called twice */
		if(!self->is_bot)
			DeathcamRemove (self, "off");
#endif

// ACEBOT_ADD special respawning code
		if (self->is_bot)
		{
			ACESP_Respawn (self);
			return;
		}
// ACEBOT_END

		//spectator mode
		// spectator's don't leave bodies
		if (self->movetype != MOVETYPE_NOCLIP)
			CopyToBodyQue (self);
		//end spectator mode
		self->svflags &= ~SVF_NOCLIENT;
		PutClientInServer (self);

		// add a teleportation effect
		self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
		self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		self->client->ps.pmove.pm_time = 14;

		self->client->respawn_time = level.time;

		return;
	}

	// restart the entire server
	gi.AddCommandString ("menu_loadgame\n");
}

//spectator mode
void spectator_respawn (edict_t *ent)
{
	int i, numspec;

	// if the user wants to become a spectator, make sure he doesn't
	// exceed max_spectators

	if (ent->client->pers.spectator) {
		char *value = Info_ValueForKey (ent->client->pers.userinfo, "spectator");
		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value)) {
			gi.cprintf(ent, PRINT_HIGH, "Spectator password incorrect.\n");
			ent->client->pers.spectator = false;
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}

		// count spectators
		for (i = 1, numspec = 0; i <= maxclients->value; i++)
			if (g_edicts[i].inuse && g_edicts[i].client->pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value) {
			gi.cprintf(ent, PRINT_HIGH, "Server spectator limit is full.");
			ent->client->pers.spectator = false;
			// reset his spectator var
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}
	} else {
		// he was a spectator and wants to join the game
		// he must have the right password
		char *value = Info_ValueForKey (ent->client->pers.userinfo, "password");
		if (*password->string && strcmp(password->string, "none") &&
			strcmp(password->string, value)) {
			gi.cprintf(ent, PRINT_HIGH, "Password incorrect.\n");
			ent->client->pers.spectator = true;
			gi.WriteByte (svc_stufftext);
			gi.WriteString ("spectator 1\n");
			gi.unicast(ent, true);
			return;
		}
	}

	/* Remove deathcam if changed to spectator after death */
	if (ent->client->pers.spectator && ent->deadflag)
		DeathcamRemove (ent, "off");

	// clear client on respawn
	ent->client->resp.score = 0;

	ent->svflags &= ~SVF_NOCLIENT;
	PutClientInServer (ent);

	// add a teleportation effect
	if (!ent->client->pers.spectator)  {
		// send effect
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_LOGIN);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		// hold in place briefly
		ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->client->ps.pmove.pm_time = 14;
	}

	ent->client->respawn_time = level.time;

	if (ent->client->pers.spectator)
		gi.bprintf (PRINT_HIGH, "%s has moved to the sidelines\n", ent->client->pers.netname);
	else
		gi.bprintf (PRINT_HIGH, "%s joined the game\n", ent->client->pers.netname);
}
//end spectator mode

//==============================================================

/*
===========
PutClientInServer

Called when a player connects to a server or respawns in
a deathmatch.
============
*/
void PutClientInServer (edict_t *ent)
{
	vec3_t	mins = {-16, -16, -24};
	vec3_t	maxs = {16, 16, 32};
	int		index, armor_index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	gitem_t		*item;
	int		i, done;
	client_persistant_t	saved;
	client_respawn_t	resp;
	char	*info;
	char playermodel[MAX_OSPATH] = " ";
	char modelpath[MAX_OSPATH] = " ";
	FILE *file;
	char userinfo[MAX_INFO_STRING];

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	SelectSpawnPoint (ent, spawn_origin, spawn_angles);

	index = ent-g_edicts-1;
	client = ent->client;
	client->is_bot = 0;
	client->kill_streak = 0;

	resp = client->resp;
	memcpy (userinfo, client->pers.userinfo, sizeof(userinfo));
	InitClientPersistant (client);
	ClientUserinfoChanged (ent, userinfo, SPAWN);

	// clear everything but the persistant data
	saved = client->pers;
	memset (client, 0, sizeof(*client));
	client->pers = saved;
	if (client->pers.health <= 0)
		InitClientPersistant(client);
	client->resp = resp;

	// copy some data from the client to the entity
	FetchClientEntData (ent);

	// clear entity values
	ent->groundentity = NULL;
	ent->client = &game.clients[index];
	if(g_spawnprotect->value)
		ent->client->spawnprotected = true;
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->air_finished = level.time + 12;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/martianenforcer/tris.md2";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~FL_NO_KNOCKBACK;
	ent->svflags &= ~SVF_DEADMONSTER;
// ACEBOT_ADD
	ent->is_bot = false;
	ent->last_node = -1;
	ent->is_jumping = false;
// ACEBOT_END
	//vehicles
	ent->in_vehicle = false;

	//deathball
	ent->in_deathball = false;

	//anti-camp
	ent->suicide_timeout = level.time + 10.0;

	VectorCopy (mins, ent->mins);
	VectorCopy (maxs, ent->maxs);
	VectorClear (ent->velocity);

	// clear playerstate values
	memset (&ent->client->ps, 0, sizeof(client->ps));

	client->ps.pmove.origin[0] = spawn_origin[0]*8;
	client->ps.pmove.origin[1] = spawn_origin[1]*8;
	client->ps.pmove.origin[2] = spawn_origin[2]*8;

	//remove these if there are there
	if(ent->client->oldplayer)
		G_FreeEdict (ent->client->oldplayer);
	if(ent->client->chasecam)
		G_FreeEdict (ent->client->chasecam);

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
	ent->s.effects = 0;
	ent->s.skinnum = ent - g_edicts - 1;
	ent->s.modelindex = 255;		// will use the skin specified model
	ent->s.modelindex2 = 255;		// custom gun model
	info = Info_ValueForKey (ent->client->pers.userinfo, "skin");

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
		ent->s.modelindex3 = gi.modelindex(modelpath);
		fclose(file);
	}
	else
		ent->s.modelindex3 = 0;

	ent->s.modelindex4 = 0;
	if(!strcmp(playermodel, "war")) //special case
	{
		ent->s.modelindex4 = gi.modelindex("players/war/weapon.md2");
		ent->s.modelindex2 = 0;
	}
	else if(!strcmp(playermodel, "brainlet"))
		ent->s.modelindex4 = gi.modelindex("players/brainlet/gunrack.md2"); //brainlets have a mount

	//check for class file
	ent->ctype = 0; //alien is default
	sprintf(modelpath, "data1/players/%s/human", playermodel);
	Q2_FindFile (modelpath, &file);
	if(file) { //human
		ent->ctype = 1;
		if(classbased->value && !(rocket_arena->value || instagib->value || excessive->value)) {
			ent->health = ent->max_health = client->pers.max_health = client->pers.health = 100;
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
	else { //robot
		sprintf(modelpath, "data1/players/%s/robot", playermodel);
		Q2_FindFile (modelpath, &file);
		if(file) {
			ent->ctype = 2;
			if(classbased->value && !(rocket_arena->value || instagib->value || excessive->value)) {
				ent->health = ent->max_health = client->pers.max_health = client->pers.health = 85;
				armor_index = ITEM_INDEX(FindItem("Combat Armor"));
				client->pers.inventory[armor_index] += 175;
			}
			fclose(file);
		}
		else { //alien
			if(classbased->value && !(rocket_arena->value || instagib->value || excessive->value)) {
				ent->health = ent->max_health = client->pers.max_health = client->pers.health = 150;
				client->pers.inventory[ITEM_INDEX(FindItem("Alien Disruptor"))] = 1;
				client->pers.inventory[ITEM_INDEX(FindItem("cells"))] = 100;
				item = FindItem("Alien Disruptor");
				client->pers.selected_item = ITEM_INDEX(item);
				client->pers.inventory[client->pers.selected_item] = 1;
				client->pers.weapon = item;
			}
		}
	}

	ent->s.frame = 0;
	VectorCopy (spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1;	// make sure off ground
	VectorCopy (ent->s.origin, ent->s.old_origin);

	// set the delta angle
	for (i=0 ; i<3 ; i++)
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(spawn_angles[i] - client->resp.cmd_angles[i]);

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = spawn_angles[YAW];
	ent->s.angles[ROLL] = 0;
	VectorCopy (ent->s.angles, client->ps.viewangles);
	VectorCopy (ent->s.angles, client->v_angle);

	//spectator mode
	// spawn a spectator
	if (client->pers.spectator) {

		client->chase_target = NULL;
		client->resp.spectator = client->pers.spectator;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity (ent);
		return;
	} else if(!g_duel->value)
		client->resp.spectator = false;
	//end spectator mode

	if (!KillBox (ent))
	{	// could't spawn in?
	}
	ent->s.event = EV_OTHER_TELEPORT; //to fix "player flash" bug
	gi.linkentity (ent);

	// force the current weapon up
	client->newweapon = client->pers.weapon;
	ChangeWeapon (ent);

	client->spawnprotecttime = level.time;
}

void ClientPlaceInQueue(edict_t *ent)
{
	int		i;
	int		highpos, numplayers;
	
	highpos = numplayers = 0;

	for (i = 0; i < maxclients->value; i++) {
		if(g_edicts[i+1].inuse && g_edicts[i+1].client) { 
			if(g_edicts[i+1].client->pers.queue > highpos)
				highpos = g_edicts[i+1].client->pers.queue;
			if(g_edicts[i+1].client->pers.queue < 3) 	
				numplayers++;
		}
	}

	if(highpos < 2 && numplayers > 2) 
		highpos = 2; 

	if(!ent->client->pers.queue)
		ent->client->pers.queue = highpos+1;
}
void ClientCheckQueue(edict_t *ent)
{
	if(ent->client->pers.queue > 2) { //everyone in line remains a spectator
		ent->client->pers.spectator = ent->client->resp.spectator = true;
		ent->client->chase_target = NULL;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity (ent);
	}
	else 
		ent->client->pers.spectator = ent->client->resp.spectator =false;
}
void MoveClientsDownQueue(edict_t *ent)
{
	int		i;

	for (i = 0; i < maxclients->value; i++) { //move everyone down
			if(g_edicts[i+1].inuse && g_edicts[i+1].client) {
				if(g_edicts[i+1].client->pers.queue > ent->client->pers.queue) 
					g_edicts[i+1].client->pers.queue--;

				if(g_edicts[i+1].client->pers.queue == 2) { //make sure those who should be in game are
					g_edicts[i+1].client->pers.spectator = g_edicts[i+1].client->resp.spectator = false;
					g_edicts[i+1].svflags &= ~SVF_NOCLIENT;
					g_edicts[i+1].movetype = MOVETYPE_WALK;
					g_edicts[i+1].solid = SOLID_BBOX;
					if(!g_edicts[i+1].is_bot)
						PutClientInServer(g_edicts+i+1);	
					else
						ACESP_PutClientInServer (g_edicts+i+1,true,0);
					safe_bprintf(PRINT_HIGH, "%s has entered the duel!\n", g_edicts[i+1].client->pers.netname);
				}
			}
			
		}
		if(ent->client)
			ent->client->pers.queue = 0;
}

/*
=====================
ClientBeginDeathmatch

A client has just connected to the server in
deathmatch mode, so clear everything out before starting them.
=====================
*/
void ClientBeginDeathmatch (edict_t *ent)
{

	FILE	*motd_file;
	char	line[80];
	char	motd[500];
	static char current_map[55];

	G_InitEdict (ent);

	InitClientResp (ent->client);

	if(((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value)
		ent->dmteam = NO_TEAM;

	// locate ent at a spawn point
	if(!ent->client->pers.spectator) //fixes invisible player bugs caused by leftover svf_noclients
		ent->svflags &= ~SVF_NOCLIENT;
	PutClientInServer (ent);

	//in ctf, initially start in chase mode, and allow them to choose a team
	if(((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) {

		ent->client->pers.spectator = true;
		ent->client->chase_target = NULL;
		ent->client->resp.spectator = true;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity (ent);
		//bring up scoreboard if not on a team
		if(ent->dmteam == NO_TEAM) {
			ent->client->showscores = true;
			if(((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value)
				CTFScoreboardMessage (ent, NULL);
			else
				DeathmatchScoreboardMessage (ent, NULL);
			gi.unicast (ent, true);
			ent->teamset = true;
		}

	}
	
	//if duel mode, then check number of existing players.  If more there are already two in the game, force
	//this player to spectator mode, and assign a queue position(we can use the spectator cvar for this)
	else if(g_duel->value) {
		ClientPlaceInQueue(ent);
		ClientCheckQueue(ent);	
	}
	
	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_LOGIN);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	safe_bprintf (PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);

	if ((motd_file = fopen("arena/motd.txt", "rb")) != NULL)
	{
		// we successfully opened the file "motd.txt"
		if ( fgets(motd, 500, motd_file) )
		{
			// we successfully read a line from "motd.txt" into motd
			// ... read the remaining lines now
			while ( fgets(line, 80, motd_file) )
			{
				// add each new line to motd, to create a BIG message string.
				// we are using strcat: STRing conCATenation function here.
				strcat(motd, line);
			}

			// print our message.
			gi.centerprintf (ent, motd);
		}
		// be good now ! ... close the file
		fclose(motd_file);

	}
	else
	
	safe_centerprintf(ent,"\n======================================\nCodeRED ACE Bot's are running\non this server.\n\n'sv addbot' to add a new bot.\n'sv removebot <name>' to remove bot.\n======================================\n\n");

	// If the map changes on us, init and reload the nodes.  
	ACEND_InitNodes();
	ACEND_LoadNodes();
	ACESP_LoadBots(ent, 0);
	strcpy(current_map,level.mapname);

	// make sure all view stuff is valid
	ClientEndServerFrame (ent);
}


/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the game.  This will happen every level load.
============
*/
void ClientBegin (edict_t *ent)
{
	int		i;

	ent->client = game.clients + (ent - g_edicts - 1);

	for(i = 0; i < 8; i++) {
		ent->client->resp.weapon_shots[i] = 0;
		ent->client->resp.weapon_hits[i] = 0;
	}
	ent->client->kill_streak = 0;

	ClientBeginDeathmatch (ent);

}

/*
===========
ClientUserInfoChanged

called whenever the player updates a userinfo variable.

The game can override any of the settings in place
(forcing skins or names, etc) before copying it off.
============
*/
void ClientUserinfoChanged (edict_t *ent, char *userinfo, int whereFrom)
{
	char	*s;
	int		playernum;
	int		i, j, k, copychar, done;
	char playermodel[MAX_OSPATH] = " ";
	char playerskin[MAX_INFO_STRING] = " ";
	char modelpath[MAX_OSPATH] = " ";
	FILE *file;

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		if(ent->dmteam == RED_TEAM)
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/red");
		else if(ent->dmteam == BLUE_TEAM)
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/blue");
		else
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/default");

		ent->s.modelindex3 = gi.modelindex("players/martianenforcer/helmet.md2");
	}

	if(whereFrom != SPAWN && whereFrom != CONNECT)
		whereFrom = INGAME;

	if((((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) && (ent->dmteam == RED_TEAM || ent->dmteam == BLUE_TEAM))
		ent->client->pers.spectator = false; //cannot spectate if you've joined a team

	if((((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) && !whereFrom && (ent->dmteam != NO_TEAM)) {
		safe_bprintf (PRINT_MEDIUM, "Illegal to change teams after CTF match has started!\n");
		return;
	}

	// set name
	s = Info_ValueForKey (userinfo, "name");
	//fix player name if corrupted
	if(s != NULL && strlen(s) > 16)
		s[16] = 0;
	strncpy (ent->client->pers.netname, s, sizeof(ent->client->pers.netname)-1);

	//spectator mode
	// set spectator
	if(!g_duel->value) { //never fool with spectating in duel mode
		s = Info_ValueForKey (userinfo, "spectator");
		// spectators need to be reset in CTF
		if (deathmatch->value && *s && strcmp(s, "0"))
			ent->client->pers.spectator = atoi(s);
		else 
			ent->client->pers.spectator = false;
	}
	//end spectator mode

	// set skin
	s = Info_ValueForKey (userinfo, "skin");

	//do the team skin check, only forcibly set teams for bots
	if(ent->is_bot) {
		if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) //only do this for skin teams, red, blue
		{
			copychar = false;
			strcpy(playerskin, " ");
			strcpy(playermodel, " ");
			j = k = 0;
			for(i = 0; i <= strlen(s) && i < MAX_OSPATH; i++)
			{
				if(copychar){
					playerskin[k] = s[i];
					k++;
				}
				else {
					playermodel[j] = s[i];
					j++;
				}
				if(s[i] == '/')
					copychar = true;


			}
			playermodel[j] = 0;

			if((!strcmp(playerskin, "red"))	|| (!strcmp(playerskin, "blue"))) //was valid teamskin
			{
				if(!strcmp(playerskin, "red"))
				{
					ent->dmteam = RED_TEAM;
					if(whereFrom == CONNECT)
						red_team_cnt++;
				}
				else
				{
					ent->dmteam = BLUE_TEAM;
					if(whereFrom == CONNECT)
						blue_team_cnt++;
				}

			}
			else if(whereFrom != SPAWN && whereFrom != CONNECT && ent->teamset)//assign to team with fewest players(we need to remove from teams too)
			{
				if(blue_team_cnt < red_team_cnt)
				{
					safe_bprintf (PRINT_MEDIUM, "Invalid Team Skin!  Assigning to Blue Team...\n");
					strcpy(playerskin, "blue");
					blue_team_cnt++;
					ent->dmteam = BLUE_TEAM;
				}
				else
				{
					safe_bprintf (PRINT_MEDIUM, "Invalid Team Skin!  Assigning to Red Team...\n");
					strcpy(playerskin, "red");
					red_team_cnt++;
					ent->dmteam = RED_TEAM;
				}
			}
			if(strlen(playermodel) > 32) //something went wrong, or somebody is being malicious
				strcpy(playermodel, "martianenforcer/");
			strcpy(s, playermodel);
			strcat(s, playerskin);
			Info_SetValueForKey (userinfo, "skin", s);
		}
	}
	playernum = ent-g_edicts-1;

	// combine name and skin into a configstring
	gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", ent->client->pers.netname, s) );

	s = Info_ValueForKey (userinfo, "skin");

	i = 0;
	done = false;
	strcpy(playermodel, " ");
	while(!done)
	{
		if((s[i] == '/') || (s[i] == '\\'))
			done = true;
		playermodel[i] = s[i];
		if(i > 62)
			done = true;
		i++;
	}
	playermodel[i-1] = 0;

	sprintf(modelpath, "data1/players/%s/helmet.md2", playermodel);
	Q2_FindFile (modelpath, &file); //does a helmet exist?
	if(file) {
		sprintf(modelpath, "players/%s/helmet.md2", playermodel);
		ent->s.modelindex3 = gi.modelindex(modelpath);
		fclose(file);
	}
	else
		ent->s.modelindex3 = 0;

	ent->s.modelindex4 = 0;
	if(!strcmp(playermodel, "war")) //special case, presents a problem for CTF.  Grr.
	{
		ent->s.modelindex4 = gi.modelindex("players/war/weapon.md2");
		ent->s.modelindex2 = 0;
	}
	else if(!strcmp(playermodel, "brainlet"))
		ent->s.modelindex4 = gi.modelindex("players/brainlet/gunrack.md2"); //brainlets have a mount

	// fov
	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		ent->client->ps.fov = 90;
	}
	else
	{
		ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));
		if (ent->client->ps.fov < 1)
			ent->client->ps.fov = 90;
		else if (ent->client->ps.fov > 160)
			ent->client->ps.fov = 160;
	}

	// handedness
	s = Info_ValueForKey (userinfo, "hand");
	if (strlen(s))
	{
		ent->client->pers.hand = atoi(s);
	}

	// save off the userinfo in case we want to check something later
	strncpy (ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo)-1);


}
void ClientChangeSkin (edict_t *ent)
{
	char	*s;
	int		playernum;
	int		i, j, k, copychar;
	char playermodel[MAX_OSPATH] = " ";
	char playerskin[MAX_INFO_STRING] = " ";
	char modelpath[MAX_OSPATH] = " ";
	char		userinfo[MAX_INFO_STRING];

	//get the userinfo
	memcpy (userinfo, ent->client->pers.userinfo, sizeof(userinfo));

	// check for malformed or illegal info strings
	if (!Info_Validate(userinfo))
	{
		if(ent->dmteam == RED_TEAM)
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/red");
		else if(ent->dmteam == BLUE_TEAM)
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/blue");
		else
			strcpy (userinfo, "\\name\\badinfo\\skin\\martianenforcer/default");

		ent->s.modelindex3 = gi.modelindex("players/martianenforcer/helmet.md2");
	}

	// set name
	s = Info_ValueForKey (userinfo, "name");
	//fix player name if corrupted
	if(s != NULL && strlen(s) > 16)
		s[16] = 0;

	strncpy (ent->client->pers.netname, s, sizeof(ent->client->pers.netname)-1);

	// set skin
	s = Info_ValueForKey (userinfo, "skin");

	copychar = false;
	strcpy(playerskin, " ");
	strcpy(playermodel, " ");
	j = k = 0;
	for(i = 0; i <= strlen(s) && i < MAX_OSPATH; i++)
	{
		if(copychar){
			playerskin[k] = s[i];
			k++;
		}
		else {
			playermodel[j] = s[i];
			j++;
		}
		if(s[i] == '/')
			copychar = true;

	}
	playermodel[j] = 0;

	if(ent->dmteam == BLUE_TEAM)
	{
		safe_bprintf (PRINT_MEDIUM, "Joined Blue Team...\n");
		strcpy(playerskin, "blue");
		blue_team_cnt++;
	}
	else
	{
		safe_bprintf (PRINT_MEDIUM, "Joined Red Team...\n");
		strcpy(playerskin, "red");
		red_team_cnt++;
	}
	if(strlen(playermodel) > 32) //something went wrong, or somebody is being malicious
		strcpy(playermodel, "martianenforcer/");
	strcpy(s, playermodel);
	strcat(s, playerskin);
	Info_SetValueForKey (userinfo, "skin", s);

	playernum = ent-g_edicts-1;

	// combine name and skin into a configstring
	gi.configstring (CS_PLAYERSKINS+playernum, va("%s\\%s", ent->client->pers.netname, s) );

	// fov
	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		ent->client->ps.fov = 90;
	}
	else
	{
		ent->client->ps.fov = atoi(Info_ValueForKey(userinfo, "fov"));
		if (ent->client->ps.fov < 1)
			ent->client->ps.fov = 90;
		else if (ent->client->ps.fov > 160)
			ent->client->ps.fov = 160;
	}

	// handedness
	s = Info_ValueForKey (userinfo, "hand");
	if (strlen(s))
	{
		ent->client->pers.hand = atoi(s);
	}

	// save off the userinfo in case we want to check something later
	strncpy (ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo)-1);
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
The game can refuse entrance to a client by returning false.
If the client is allowed, the connection process will continue
and eventually get to ClientBegin()
Changing levels will NOT cause this to be called again, but
loadgames will.
============
*/

qboolean ClientConnect (edict_t *ent, char *userinfo)
{
	char	*value;
	int		i, numspec;

	// check to see if they are on the banned IP list
	value = Info_ValueForKey (userinfo, "ip");
	if (SV_FilterPacket(value)) {
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}

	//spectator mode
	// check for a spectator
	value = Info_ValueForKey (userinfo, "spectator");
	if (deathmatch->value && *value && strcmp(value, "0")) {
		

		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value)) {
			Info_SetValueForKey(userinfo, "rejmsg", "Spectator password required or incorrect.");
			return false;
		}

		// count spectators
		for (i = numspec = 0; i < maxclients->value; i++)
			if (g_edicts[i+1].inuse && g_edicts[i+1].client->pers.spectator)
				numspec++;

		if (numspec >= maxspectators->value) {
			Info_SetValueForKey(userinfo, "rejmsg", "Server spectator limit is full.");
			return false;
		}
	} else if(!ent->is_bot){
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if (*password->string && strcmp(password->string, "none") &&
			strcmp(password->string, value)) {
			Info_SetValueForKey(userinfo, "rejmsg", "Password required or incorrect.");
			return false;
		}
	}
	//end specator mode

	// they can connect
	ent->client = game.clients + (ent - g_edicts - 1);

	// if there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (ent->inuse == false)
	{
		// clear the respawning variables
		InitClientResp (ent->client);
		if (!game.autosaved || !ent->client->pers.weapon)
			InitClientPersistant (ent->client);
	}

	if(((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) {
		ent->dmteam = NO_TEAM;
		ent->teamset = false;
	}

	ClientUserinfoChanged (ent, userinfo, CONNECT);

	if (game.maxclients > 1)
		gi.dprintf ("%s connected\n", ent->client->pers.netname);

	ent->client->pers.connected = true;

	return true;
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.
============
*/
void ClientDisconnect (edict_t *ent)
{
	int	playernum;

	if (!ent->client)
		return;

	safe_bprintf (PRINT_HIGH, "%s disconnected\n", ent->client->pers.netname);

    if(ctf->value)
		CTFDeadDropFlag(ent);

	DeadDropDeathball(ent);

	if(ent->deadflag && ent->client->chasetoggle == 1)
		DeathcamRemove(ent, "off");

	if (((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value)  //adjust teams and scores
	{
		if(ent->dmteam == BLUE_TEAM)
			blue_team_cnt-=1;
		else
			red_team_cnt-=1;
	//		safe_bprintf(PRINT_HIGH, "disconnected - red: %i blue: %i\n", red_team_cnt, blue_team_cnt);
	}

	//if using bot thresholds, put the bot back in
	if(sv_botkickthreshold->integer)
		ACESP_LoadBots(ent, 1);

	//if in duel mode, we need to bump people down the queue if its the player in game leaving
	if(g_duel->value) 
		MoveClientsDownQueue(ent);

	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_LOGOUT);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	gi.unlinkentity (ent);
	ent->s.modelindex = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = false;
	
	playernum = ent-g_edicts-1;
	gi.configstring (CS_PLAYERSKINS+playernum, "");


}


//==============================================================


edict_t	*pm_passent;

// pmove doesn't need to know about passent and contentmask
trace_t	PM_trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	if (pm_passent->health > 0)
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return gi.trace (start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}

unsigned CheckBlock (void *b, int c)
{
	int	v,i;
	v = 0;
	for (i=0 ; i<c ; i++)
		v+= ((byte *)b)[i];
	return v;
}
void PrintPmove (pmove_t *pm)
{
	unsigned	c1, c2;

	c1 = CheckBlock (&pm->s, sizeof(pm->s));
	c2 = CheckBlock (&pm->cmd, sizeof(pm->cmd));
	Com_Printf ("sv %3i:%i %i\n", pm->cmd.impulse, c1, c2);
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame.
==============
*/
void ClientThink (edict_t *ent, usercmd_t *ucmd)
{
	gclient_t	*client;
	edict_t	*other;
	int		i, j;
	pmove_t	pm;
	qboolean sproing, haste;

	level.current_entity = ent;
	client = ent->client;

	if (level.intermissiontime)
	{

		client->ps.pmove.pm_type = PM_FREEZE;
		// can exit intermission after ten seconds
		if (level.time > level.intermissiontime + 10.0
			&& (ucmd->buttons & BUTTON_ANY) )
			level.exitintermission = true;
		return;
	}

	pm_passent = ent;

	if (ent->client->chase_target) {

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

	} else {


		// set up for pmove
		memset (&pm, 0, sizeof(pm));

		if (ent->movetype == MOVETYPE_NOCLIP)
			client->ps.pmove.pm_type = PM_SPECTATOR;
		else if (ent->s.modelindex != 255 && !(ent->in_vehicle) && !(client->chasetoggle)) //for vehicles or deathcam
			client->ps.pmove.pm_type = PM_GIB;
		else if (ent->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else
			client->ps.pmove.pm_type = PM_NORMAL;

		if(!client->chasetoggle)
		{
			client->ps.pmove.gravity = sv_gravity->value;
		}
		else
		{	/* No gravity to move the deathcam */
			client->ps.pmove.gravity = 0;
		}

		//vehicles
		if ( Jet_Active(ent) )
			Jet_ApplyJet( ent, ucmd );

		pm.s = client->ps.pmove;

		for (i=0 ; i<3 ; i++)
		{
			pm.s.origin[i] = ent->s.origin[i]*8;
			pm.s.velocity[i] = ent->velocity[i]*8;
		}

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
		{
			pm.snapinitial = true;
	//		gi.dprintf ("pmove changed!\n");
		}

		ucmd->forwardmove *= 1.3;

		pm.cmd = *ucmd;

		pm.trace = PM_trace;	// adds default parms
		pm.pointcontents = gi.pointcontents;

		//joust mode
		if(joustmode->value) {
			if(ent->groundentity)
				client->joustattempts = 0;
			if(pm.cmd.upmove >= 10) {
				client->joustattempts++;
				pm.joustattempts = client->joustattempts;
				if(pm.joustattempts == 10 || pm.joustattempts == 20) {
					gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
					PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
				}
			}
		}

		// perform a pmove
		gi.Pmove (&pm);

		// save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		for (i=0 ; i<3 ; i++)
		{
			ent->s.origin[i] = pm.s.origin[i]*0.125;
			//vehicles
			if ( !Jet_Active(ent) || (Jet_Active(ent)&&(fabs((float)pm.s.velocity[i]*0.125) < fabs(ent->velocity[i]))) )
				ent->velocity[i] = pm.s.velocity[i]*0.125;
		}

		VectorCopy (pm.mins, ent->mins);
		VectorCopy (pm.maxs, ent->maxs);

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

		//vehicles
		if ( Jet_Active(ent) )
			if( pm.groundentity ) 		/*are we on ground*/
				if ( Jet_AvoidGround(ent) )	/*then lift us if possible*/
					pm.groundentity = NULL;		/*now we are no longer on ground*/

		if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0))
		{
			sproing = client->sproing_framenum > level.framenum;
			haste = client->haste_framenum > level.framenum;
			if(sproing)
				gi.sound(ent, CHAN_VOICE, gi.soundindex("items/sproing.wav"), 1, ATTN_NORM, 0);
			if(haste) {
				gi.sound(ent, CHAN_VOICE, gi.soundindex("items/haste.wav"), 1, ATTN_NORM, 0);
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_EXPLOSION2);
				gi.WritePosition (ent->s.origin);
				gi.multicast (ent->s.origin, MULTICAST_PVS);
			}
			else
				gi.sound(ent, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
		}

		ent->viewheight = pm.viewheight;
		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundentity = pm.groundentity;
		if (pm.groundentity)
			ent->groundentity_linkcount = pm.groundentity->linkcount;

		if (ent->deadflag)
		{
			client->ps.viewangles[ROLL] = 40;
			client->ps.viewangles[PITCH] = -15;
			client->ps.viewangles[YAW] = client->killer_yaw;
		}
		else
		{
			VectorCopy (pm.viewangles, client->v_angle);
			VectorCopy (pm.viewangles, client->ps.viewangles);
		}

		if (client->ctf_grapple)
			CTFGrapplePull(client->ctf_grapple);

		gi.linkentity (ent);

		if (ent->movetype != MOVETYPE_NOCLIP)
			G_TouchTriggers (ent);

		// touch other objects
		for (i=0 ; i<pm.numtouch ; i++)
		{
			other = pm.touchents[i];
			for (j=0 ; j<i ; j++)
				if (pm.touchents[j] == other)
					break;
			if (j != i)
				continue;	// duplicated
			if (!other->touch)
				continue;
			other->touch (other, ent, NULL, NULL);
		}
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// save light level the player is standing on for
	// monster sighting AI
	ent->light_level = ucmd->lightlevel;

	//spectator mode
	if (client->latched_buttons & BUTTON_ATTACK)
	{
		if (client->resp.spectator) {

			client->latched_buttons = 0;

			if((ctf->value || tca->value || cp->value) && (ent->dmteam == RED_TEAM || ent->dmteam == BLUE_TEAM)) {
				client->pers.spectator = false; //we have a team, join
			//	safe_bprintf(PRINT_HIGH, "red: %i blue: %i\n", red_team_cnt, blue_team_cnt);
			}
			else if((((int)(dmflags->value) & DF_SKINTEAMS) || (ctf->value || tca->value || cp->value) && ent->dmteam == NO_TEAM) && client->resp.spectator < 3) {
				if(red_team_cnt < blue_team_cnt)
					ent->dmteam = RED_TEAM; //gonna put the autojoin here
				else
					ent->dmteam = BLUE_TEAM;
				client->pers.spectator = false;
				ClientChangeSkin(ent);
			//	safe_bprintf(PRINT_HIGH, "autojoin red: %i blue: %i\n", red_team_cnt, blue_team_cnt);
			}

			if (client->chase_target) {
				client->chase_target = NULL;
				client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			} else
				GetChaseTarget(ent);

		} else if (!client->weapon_thunk) {
			client->weapon_thunk = true;
			Think_Weapon (ent);
		}
	}

	if (client->resp.spectator) {
		if((((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) && client->resp.spectator < 3) {
			if(ent->dmteam == NO_TEAM && (level.time/2 == ceil(level.time/2)))
				safe_centerprintf(ent, "\n\n\nPress <fire> to autojoin\nor <jump> to join BLUE\nor <crouch> to join RED\n");
		}
		if (ucmd->upmove >= 10) {
			if(((((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) && ent->dmteam == NO_TEAM) && client->resp.spectator < 3) {
				ent->dmteam = BLUE_TEAM; //join BLUE
				client->pers.spectator = false;
				ClientChangeSkin(ent);
			}
			if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD)) {
				client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
				if (client->chase_target)
					ChaseNext(ent);
				else
					GetChaseTarget(ent);
			}
		}
		else if(((((int)(dmflags->value) & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) && ent->dmteam == NO_TEAM &&
			(ucmd->upmove < 0))  && client->resp.spectator < 3){

			ent->dmteam = RED_TEAM; //join RED
			client->pers.spectator = false;
			ClientChangeSkin(ent);
		}
		else
			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;

	}

	// update chase cam if being followed
	for (i = 1; i <= maxclients->value; i++) {
		other = g_edicts + i;
		if (other->inuse && other->client->chase_target == ent)
			UpdateChaseCam(other);
	}

	//mutators
	if((regeneration->value || excessive->value) && !ent->deadflag) {
		if((ent->health < ent->max_health) && (client->regen_framenum < level.framenum)) {
			client->regen_framenum = level.framenum + 5;
			ent->health+=2;
		}
	}

	//spawn protection has run out
	if(level.time > ent->client->spawnprotecttime + g_spawnprotect->integer)
		ent->client->spawnprotected = false;

	//lose one health every second
	if(g_losehealth->value && !ent->deadflag) {
		if(regeneration->value || excessive->value || vampire->value)
			return;
		if((ent->health > g_losehealth_num->value) && (client->losehealth_framenum < level.framenum)) {
			client->losehealth_framenum = level.framenum + 10;
			ent->health-=1;
		}
	}
	//safe_bprintf(PRINT_HIGH, "pos: %i\n", ent->client->pers.queue);

}


/*
==============
ClientBeginServerFrame

This will be called once for each server frame, before running
any other entities in the world.
==============
*/
void ClientBeginServerFrame (edict_t *ent)
{
	gclient_t	*client;
	int			buttonMask;

	if (level.intermissiontime)
		return;

	client = ent->client;

	//spectator mode
	if (deathmatch->value &&
		client->pers.spectator != client->resp.spectator &&
		(level.time - client->respawn_time) >= 5) {
		spectator_respawn(ent);
		return;
	}
	//end spectator mode

	//anti-camp
	if(anticamp->value) {
		if(excessive->value) {
			if(VectorLength(ent->velocity) > 450)
				ent->suicide_timeout = level.time + camptime->integer;
		}
		else {
			if(VectorLength(ent->velocity) > 300)
				ent->suicide_timeout = level.time + camptime->integer;
		}
		if(ent->suicide_timeout < level.time && ent->takedamage == DAMAGE_AIM
			&& !client->resp.spectator) {
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, ent->dmg, 0, DAMAGE_NO_ARMOR, MOD_SUICIDE);
			safe_centerprintf(ent, "Anticamp: move or die!\n");
		}
	}

	//spectator mode

	if (!client->weapon_thunk && !client->resp.spectator)
	//end spectator mode

		Think_Weapon (ent);
	else
		client->weapon_thunk = false;

	if (ent->deadflag)
	{
		// wait for any button just going down
		if ( level.time > client->respawn_time)
		{
			// in deathmatch, only wait for attack button
			if (deathmatch->value)
				buttonMask = BUTTON_ATTACK;
			else
				buttonMask = -1;

			//should probably add in a force respawn option
			if (( client->latched_buttons & buttonMask ) ||
				(deathmatch->value && ((int)dmflags->value & DF_FORCE_RESPAWN) ) )
			{

				if(!ent->is_bot)
					DeathcamRemove (ent, "off");

				respawn(ent);
				client->latched_buttons = 0;
			}
		}
		return;
	}

	// add player trail so monsters can follow
	if (!deathmatch->value)
		if (!visible (ent, PlayerTrail_LastSpot() ) )
			PlayerTrail_Add (ent->s.old_origin);

	client->latched_buttons = 0;
}
