/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2010 COR Entertainment, LLC.

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
	int		mod=0, msg;
	char		*message;
	char		*message2;
	qboolean	ff;
	char		*chatmsg;
	char		*tauntmsg;
	char		cleanname[16], cleanname2[16];
	int			i, pos, total, place;
	edict_t		*cl_ent;
	gitem_t		*it;

	if (deathmatch->value)
	{
		ff = meansOfDeath & MOD_FRIENDLY_FIRE;
		mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
		message = NULL;
		chatmsg = NULL;
		tauntmsg = NULL;
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
			//clean up names, get rid of escape chars
			G_CleanPlayerName ( self->client->pers.netname , cleanname );
			G_CleanPlayerName ( attacker->client->pers.netname , cleanname2 );

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
			case MOD_VIOLATOR:
				message = "was probed by";
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
			case MOD_HEADSHOT:
				message = "had its head blown off by";
			}
			//here is where the bot chat features will be added.
			//default is on.  Setting to 1 turns it off.

			if ( !(dmflags->integer & DF_BOTCHAT) && self->is_bot)
			{
				msg = random() * 9;
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
					chatmsg = "%s: Stop it %s, you punk!";
					break;
				}
				if(chatmsg) {
					safe_bprintf (PRINT_CHAT, chatmsg, self->client->pers.netname, attacker->client->pers.netname);
					safe_bprintf (PRINT_CHAT, "\n");

					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (TE_SAYICON);
					gi.WritePosition (self->s.origin);
					gi.multicast (self->s.origin, MULTICAST_PVS);
				}
			}

			//bot taunts
			if(!(dmflags->integer & DF_BOTCHAT) && attacker->is_bot) {

				if(!(attacker->client->ps.pmove.pm_flags & PMF_DUCKED)) {
					attacker->state = STATE_STAND;
					attacker->s.frame = FRAME_taunt01-1;
					attacker->client->anim_end = FRAME_taunt17;

					//print a taunt, or send taunt sound
					msg = random() * 24;
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
					case 9:
					case 10:
					case 11:
					case 12:
					case 13:
					case 14:
					case 15:
					case 16:
					case 17:
					case 18:
					case 19:
					case 20:
					case 21:
					case 22:
					case 23:
					case 24:
						Cmd_VoiceTaunt_f(attacker);
						break;
					default:
						tauntmsg = "%s: You are useless to me, %s\n";
						break;
					}
					if(tauntmsg) {
						safe_bprintf (PRINT_CHAT, tauntmsg, attacker->client->pers.netname, self->client->pers.netname);
						//send an effect to show that the bot is taunting
						gi.WriteByte (svc_temp_entity);
						gi.WriteByte (TE_SAYICON);
						gi.WritePosition (attacker->s.origin);
						gi.multicast (attacker->s.origin, MULTICAST_PVS);
					}
				}
			}

			if (message)
			{
				safe_bprintf (PRINT_MEDIUM,"%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);

				if (deathmatch->value)
				{
					if (ff) {
						attacker->client->resp.score--;
						attacker->client->resp.deaths++;
						if ((dmflags->integer & DF_SKINTEAMS) && !ctf->value) {
							if(attacker->dmteam == RED_TEAM)
								red_team_score--;
							else
								blue_team_score--;
						}
					}
					else {

						attacker->client->resp.score++;

						if(!self->groundentity) {
							attacker->client->resp.reward_pts+=3;
							safe_centerprintf(attacker, "Midair shot!\n");
						}
						else
							attacker->client->resp.reward_pts++;

						if(mod == MOD_HEADSHOT) { //3 more pts for a headshot
							attacker->client->resp.reward_pts+=3;
							safe_centerprintf(attacker, "HEADSHOT!\n");
							gi.sound(attacker, CHAN_AUTO, gi.soundindex("misc/headshot.wav"), 1, ATTN_STATIC, 0);
						}

						//mutators
						if(vampire->value) {
							attacker->health+=20;
							if(attacker->health > attacker->max_health)
								attacker->health = attacker->max_health;
						}
						self->client->resp.deaths++;

						if ((dmflags->integer & DF_SKINTEAMS)  && !ctf->value) {
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
								attacker->client->resp.reward_pts+=10;
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
								attacker->client->resp.reward_pts+=20;
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
				if(attacker->client->resp.reward_pts >= g_reward->integer && !attacker->client->resp.powered) { //give them speed and invis powerups
					it = FindItem("Invisibility");
					attacker->client->pers.inventory[ITEM_INDEX(it)] += 1;

					it = FindItem("Sproing");
					attacker->client->pers.inventory[ITEM_INDEX(it)] += 1;

					it = FindItem("Haste");
					attacker->client->pers.inventory[ITEM_INDEX(it)] += 1;

					attacker->client->resp.powered = true;

					gi.sound (attacker, CHAN_AUTO, gi.soundindex("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
				}
				self->client->kill_streak = 0; //reset, you are dead
				return;
			}
		}

	}

	if(mod == MOD_DEATHRAY) {
		safe_bprintf(PRINT_MEDIUM, "%s killed by Deathray!\n", self->client->pers.netname);

		//immune player (activator) gets score increase
		for (i=0 ; i<maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			if(cl_ent->client)
				if(cl_ent->client->rayImmunity)
					cl_ent->client->resp.score++;
		}
		return;
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
	if (item && (strcmp (item->pickup_name, "Violator") == 0))
		item = NULL;

	if (!(dmflags->integer & DF_QUAD_DROP))
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
	if (sproing && !self->client->resp.powered)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item (self, FindItemByClassname ("item_sproing"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->sproing_framenum - level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}
	if (haste && !self->client->resp.powered)
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
	int	got_vehicle = 0;
	int	number_of_gibs = 0;
	int	gib_effect = EF_GREENGIB;
	int hasFlag = false;
	gitem_t *it, *flag1_item, *flag2_item;
	int mod;

	mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;

	if (self->in_vehicle) {
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

		if(ctf->value) {
			//check to see if they had a flag
			flag1_item = flag2_item = NULL;

			flag1_item = FindItemByClassname("item_flag_red");
			flag2_item = FindItemByClassname("item_flag_blue");

			if (self->client->pers.inventory[ITEM_INDEX(flag1_item)] || self->client->pers.inventory[ITEM_INDEX(flag1_item)])
				hasFlag = true;

			CTFDeadDropFlag(self);
			if(anticamp->value && meansOfDeath == MOD_SUICIDE && hasFlag) {

				//make campers really pay for hiding flags
				if(self->dmteam == BLUE_TEAM)
					CTFResetFlag(RED_TEAM);
				else
					CTFResetFlag(BLUE_TEAM);
			}
		}
		if(self->in_deathball)
			DeadDropDeathball(self);

		CTFPlayerResetGrapple(self);

		if (deathmatch->value)
			Cmd_Help_f (self);		// show scores

		if(self->health < -40 && attacker->client) {
			attacker->client->resp.reward_pts++;
			if(attacker->client->resp.reward_pts >= g_reward->integer && !attacker->client->resp.powered) { //give them speed and invis powerups
				it = FindItem("Invisibility");
				attacker->client->pers.inventory[ITEM_INDEX(it)] += 1;

				it = FindItem("Sproing");
				attacker->client->pers.inventory[ITEM_INDEX(it)] += 1;

				it = FindItem("Haste");
				attacker->client->pers.inventory[ITEM_INDEX(it)] += 1;

				attacker->client->resp.powered = true;

				gi.sound (attacker, CHAN_VOICE, gi.soundindex("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
			}
		}

	}

	// remove powerups
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->haste_framenum = 0;
	self->client->sproing_framenum = 0;
	self->client->invis_framenum = 0;

	// clear inventory
	memset(self->client->pers.inventory, 0, sizeof(self->client->pers.inventory));

	if (self->health < -40)
	{	// gib
		self->takedamage	= DAMAGE_NO;
		self->s.modelindex3	= 0;    //remove helmet, if a martian

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

			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_DEATHFIELD);
			gi.WritePosition (self->s.origin);
			gi.multicast (self->s.origin, MULTICAST_PVS);

			for (n= 0; n < number_of_gibs; n++) {
				if(mod == MOD_R_SPLASH || mod == MOD_ROCKET)
					ThrowGib (self, "models/objects/gibs/mart_gut/tris.md2", damage, GIB_METALLIC, EF_SHIPEXHAUST);
				else
					ThrowGib (self, "models/objects/gibs/mart_gut/tris.md2", damage, GIB_METALLIC, EF_GREENGIB);
				ThrowGib (self, "models/objects/debris2/tris.md2", damage, GIB_METALLIC, 0);
			}
		}
		else if(self->ctype == 2) { //robot
			gib_effect = 0;
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

			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_DEATHFIELD2);
			gi.WritePosition (self->s.origin);
			gi.WriteDir (self->s.angles);
			gi.multicast (self->s.origin, MULTICAST_PVS);

			gib_effect = EF_GIB;
			for (n= 0; n < number_of_gibs; n++) {
				if(mod == MOD_R_SPLASH || mod == MOD_ROCKET)
					ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_METALLIC, EF_SHIPEXHAUST);
				else
					ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_METALLIC, EF_GIB);
			}
		}

		if(self->usegibs) {
			if(mod == MOD_R_SPLASH || mod == MOD_ROCKET)
				gib_effect = EF_SHIPEXHAUST;
			ThrowGib (self, self->head, damage, GIB_ORGANIC, gib_effect);
			ThrowGib (self, self->leg, damage, GIB_ORGANIC, gib_effect);
			ThrowGib (self, self->leg, damage, GIB_ORGANIC, gib_effect);
			ThrowGib (self, self->body, damage, GIB_ORGANIC, gib_effect);
			ThrowGib (self, self->arm, damage, GIB_ORGANIC, gib_effect);
			ThrowGib (self, self->arm, damage, GIB_ORGANIC, gib_effect);
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

	gi.sound (self, CHAN_VOICE, gi.soundindex("misc/death.wav"), 1, ATTN_STATIC, 0);

	self->deadflag = DEAD_DEAD;
	self->takedamage	= DAMAGE_NO;

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
	int			queue=0;

	if(g_duel->value) //need to save this off in duel mode.  Potentially dangerous?
		queue = client->pers.queue;

	memset (&client->pers, 0, sizeof(client->pers));

	if(g_duel->value)
		client->pers.queue = queue;

	if(!rocket_arena->value) { //gets a violator, unless RA
		item = FindItem("Violator");
		client->pers.selected_item = ITEM_INDEX(item);
		client->pers.inventory[client->pers.selected_item] = 1;
		client->pers.weapon = item;
	}

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

	//if powered up, give back powerups
	if(client->resp.powered) {
		item = FindItem("Invisibility");
		client->pers.inventory[ITEM_INDEX(item)] += 1;

		item = FindItem("Sproing");
		client->pers.inventory[ITEM_INDEX(item)] += 1;

		item = FindItem("Haste");
		client->pers.inventory[ITEM_INDEX(item)] += 1;
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
	char	whichteam[32];

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	if(random() < 0.5)
		strcpy(whichteam, "info_player_red");
	else
		strcpy(whichteam, "info_player_blue");

	while ((spot = G_Find (spot, FOFS(classname), whichteam)) != NULL)
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
		spot = G_Find (spot, FOFS(classname), whichteam);
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
	if ( dmflags->integer & DF_SPAWN_FARTHEST)
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
		if (ctf->value || tca->value || cp->value || (dmflags->integer & DF_SKINTEAMS)) {
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
		ent->s.modelindex = 0; //for good measure
		ent->s.modelindex2 = 0;
		ent->s.modelindex3 = 0;
		ent->s.modelindex4 = 0;
		return;
	}
	ent->nextthink = level.time + .1;
	ent->s.origin[2] -= 1;
	ent->s.effects |= EF_COLOR_SHELL;
	ent->s.renderfx |= RF_SHELL_GREEN;
	ent->solid = SOLID_NOT; //don't gib sinking bodies
}

void CopyToBodyQue (edict_t *ent)
{
	edict_t		*body;

	// grab a body que and cycle to the next one
	body = &g_edicts[maxclients->integer + level.body_que + 1];
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
	body->solid = SOLID_NOT;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;
	body->die = NULL;
	body->takedamage = DAMAGE_NO;
	body->ctype = ent->ctype;
	body->usegibs = ent->usegibs;
	body->timestamp = level.time;
	body->nextthink = level.time + 5;
	body->think = BodySink;

	if(body->usegibs) {
		strcpy(body->arm, ent->arm);
		strcpy(body->leg, ent->leg);
		strcpy(body->head, ent->head);
		strcpy(body->body, ent->body);
	}

	gi.linkentity (body);
}


void respawn (edict_t *self)
{
	if (deathmatch->value)
	{

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
		char *value = Info_ValueForKey (ent->client->pers.userinfo, "spectator_password");
		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value)) {
			gi.cprintf(ent, PRINT_HIGH, "%s", "Spectator password incorrect.\n");
			ent->client->pers.spectator = 0;
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
			gi.cprintf(ent, PRINT_HIGH, "%s", "Server spectator limit is full.");
			ent->client->pers.spectator = 0;
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
			strcmp(password->string, value))
		{
			gi.cprintf(ent, PRINT_HIGH, "%s", "Password incorrect.\n");
			ent->client->pers.spectator = 1;
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
	{ // pers.spectator > 0, entering spectator mode
		safe_bprintf (PRINT_HIGH, "%s has moved to the sidelines\n", ent->client->pers.netname);
	}
	else
	{ // pers.spectator==0, leaving spectator mode
		safe_bprintf (PRINT_HIGH, "%s joined the game\n", ent->client->pers.netname);
	}
	if ( sv_botkickthreshold && sv_botkickthreshold->integer )
	{ // player entered or left playing field, update for auto botkick
		ACESP_LoadBots( ent );
	}


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
	client->homing_shots = 0;
	client->mapvote = 0;
	client->lasttaunttime = 0;
	client->rayImmunity = false;

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

	if (deathmatch->value && (dmflags->integer & DF_FIXED_FOV))
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

	sprintf(modelpath, "players/%s/helmet.md2", playermodel);
	Q2_FindFile (modelpath, &file); //does a helmet exist?
	if(file) {
		sprintf(modelpath, "players/%s/helmet.md2", playermodel);
		ent->s.modelindex3 = gi.modelindex(modelpath);
		fclose(file);
	}
	else
		ent->s.modelindex3 = 0;

	ent->s.modelindex4 = 0;

	//check for class file
	ent->ctype = 0; //alien is default
	sprintf(modelpath, "players/%s/human", playermodel);
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
		sprintf(modelpath, "players/%s/robot", playermodel);
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
	if ( client->pers.spectator != 0 )
	{
		client->chase_target = NULL;
		client->resp.spectator = client->pers.spectator;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity (ent);
		return;
	}
	else if ( !g_duel->integer )
		client->resp.spectator = 0;
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

	//unlagged
	if ( g_antilag->integer) {
		G_ResetHistory( ent );
		// and this is as good a time as any to clear the saved state
		client->saved.leveltime = 0;
	}
}

void ClientPlaceInQueue(edict_t *ent)
{
	int		i;
	int		highestpos, induel, numplayers;

	highestpos = induel = numplayers = 0;

	for (i = 0; i < maxclients->value; i++) {
		if(g_edicts[i+1].inuse && g_edicts[i+1].client) {
			if(g_edicts[i+1].client->pers.queue > highestpos) //only count players that are actually in
				highestpos = g_edicts[i+1].client->pers.queue;
			if(g_edicts[i+1].client->pers.queue && g_edicts[i+1].client->pers.queue < 3)
				induel++;
			if(g_edicts[i+1].client->pers.queue) //only count players that are actually in
				numplayers++;
		}
	}

	if(induel > 1) //make sure no more than two are in the duel at once
		if(highestpos < 2)
			highestpos = 2; //in case two people somehow managed to have pos 1
	if(highestpos < numplayers)
		highestpos = numplayers;

	if(!ent->client->pers.queue)
		ent->client->pers.queue = highestpos+1;
}
void ClientCheckQueue(edict_t *ent)
{
	int		i;
	int		numplayers = 0;

	if(ent->client->pers.queue > 2) { //everyone in line remains a spectator
		ent->client->pers.spectator = ent->client->resp.spectator = 1;
		ent->client->chase_target = NULL;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity (ent);
	}
	else {
		for (i = 0; i < maxclients->value; i++) {
			if(g_edicts[i+1].inuse && g_edicts[i+1].client) {
				if(!g_edicts[i+1].client->pers.spectator && g_edicts[i+1].client->pers.queue)
					numplayers++;
			}
		}
		if(numplayers < 3) //only put him in if there are less than two
			ent->client->pers.spectator = ent->client->resp.spectator = 0;
	}
}
void MoveClientsDownQueue(edict_t *ent)
{
	int		i;
	qboolean putonein = false;

	for (i = 0; i < maxclients->value; i++) { //move everyone down
		if(g_edicts[i+1].inuse && g_edicts[i+1].client) {
			if(g_edicts[i+1].client->pers.queue > ent->client->pers.queue)
				g_edicts[i+1].client->pers.queue--;

			if(!putonein && g_edicts[i+1].client->pers.queue == 2 && g_edicts[i+1].client->resp.spectator) { //make sure those who should be in game are
				g_edicts[i+1].client->pers.spectator = g_edicts[i+1].client->resp.spectator = false;
				g_edicts[i+1].svflags &= ~SVF_NOCLIENT;
				g_edicts[i+1].movetype = MOVETYPE_WALK;
				g_edicts[i+1].solid = SOLID_BBOX;
				if(!g_edicts[i+1].is_bot)
					PutClientInServer(g_edicts+i+1);
				else
					ACESP_PutClientInServer( g_edicts+i+1, true ); // respawn
				safe_bprintf(PRINT_HIGH, "%s has entered the duel!\n", g_edicts[i+1].client->pers.netname);
				putonein = true;
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
	char	motd_file_name[MAX_QPATH];
	char	line[80];
	char	motd[500];

	G_InitEdict (ent);

	InitClientResp (ent->client);

	ent->dmteam = NO_TEAM;

	// locate ent at a spawn point
	if(!ent->client->pers.spectator) //fixes invisible player bugs caused by leftover svf_noclients
		ent->svflags &= ~SVF_NOCLIENT;
	PutClientInServer (ent);

	//in ctf, initially start in chase mode, and allow them to choose a team
	if( TEAM_GAME ) {
		ent->client->pers.spectator = 1;
		ent->client->chase_target = NULL;
		ent->client->resp.spectator = 1;
		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity (ent);
		//bring up scoreboard if not on a team
		if(ent->dmteam == NO_TEAM)
		{
			ent->client->showscores = true;
			CTFScoreboardMessage (ent, NULL, false);
			gi.unicast (ent, true);
			ent->teamset = true; // used to error check team skin
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

	// get the name for the MOTD file
	if ( motdfile && motdfile->string && motdfile->string[0] )
		Com_sprintf (motd_file_name, sizeof(motd_file_name), "arena/%s", motdfile->string);
	else
		strcpy (motd_file_name, "arena/motd.txt");

	if ((motd_file = fopen(motd_file_name, "rb")) != NULL)
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

	if(g_callvote->value)
		safe_cprintf(ent, PRINT_HIGH, "Call voting is ^2ENABLED\n");
	else
		safe_cprintf(ent, PRINT_HIGH, "Call voting is ^1DISABLED\n");

	if(g_antilag->value)
		safe_cprintf(ent, PRINT_HIGH, "Antilag is ^2ENABLED\n");
	else
		safe_cprintf(ent, PRINT_HIGH, "Antilag is ^1DISABLED\n");

	//check bots with each player connect
	ACESP_LoadBots( ent );

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

	ent->client->homing_shots = 0;

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
	edict_t *cl_ent;
	int		playernum;
	int		i, j, k;
	qboolean duplicate, done, copychar;
	char playermodel[MAX_OSPATH] = " ";
	char playerskin[MAX_INFO_STRING] = " ";
	char modelpath[MAX_OSPATH] = " ";
	char slot[2];
	char colorName[32];
	FILE *file;
	teamcensus_t teamcensus;

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

	if(playervote.called && whereFrom == INGAME)
		return; //do not allow people to change info during votes

	if(((dmflags->integer & DF_SKINTEAMS) || ctf->value || tca->value || cp->value) && !whereFrom && (ent->dmteam != NO_TEAM)) {
		safe_bprintf (PRINT_MEDIUM, "Changing settings not allowed during a team game\n");
		return;
	}

	// set name
	s = Info_ValueForKey (userinfo, "name");
	strcpy(colorName, s);
	i = j = k = 0;
	while ( s && s[i] && i < 47 && j < 15 )
	{ // validate name
		ent->client->pers.netname[i] = s[i];
		if ( k == 0 )
		{
			if ( s[i] == Q_COLOR_ESCAPE )
				k = 1;
			else
				j ++;
		}
		else if ( k == 1 )
		{
			if ( s[i] == Q_COLOR_ESCAPE )
			{
				j += 2;
				if ( j == 16 )
				{
					i--;
					break;
				}
			}
			k = 0;
		}
		i ++;
	}
	ent->client->pers.netname[i] = 0;

	//spectator mode
	if ( !g_duel->value )
	{ // never fool with spectating in duel mode
		// set spectator
		s = Info_ValueForKey (userinfo, "spectator");
		if ( TEAM_GAME )
		{ //
			if ( strcmp( s, "2" ) )
			{ // not special team game spectate
				// so make sure it stays 0
				Info_SetValueForKey( userinfo, "spectator", "0");
			}
			else
			{
				ent->client->pers.spectator = 2;
			}
		}
		else
		{
			if ( deathmatch->value && *s && strcmp(s, "0") )
				ent->client->pers.spectator = atoi(s);
			else
				ent->client->pers.spectator = 0;
		}
	}
	//end spectator mode

	// set skin
	s = Info_ValueForKey (userinfo, "skin");

	// do the team skin check
	if ( TEAM_GAME
			&& ent->client->pers.spectator != 2 && ent->client->resp.spectator != 2 )
	{
		copychar = false;
		strcpy( playerskin, " " );
		strcpy( playermodel, " " );
		j = k = 0;
		for ( i = 0; i <= strlen( s ) && i < MAX_OSPATH; i++ )
		{
				if(copychar){
				playerskin[k] = s[i];
				k++;
			}
				else {
				playermodel[j] = s[i];
				j++;
			}
			if ( s[i] == '/' )
				copychar = true;
		}
		playermodel[j] = 0;

		if ( whereFrom != SPAWN && whereFrom != CONNECT && ent->teamset )
		{ // ingame and team is supposed to be set, error check skin
			if ( strcmp( playerskin, "red" ) && strcmp( playerskin, "blue" ) )
			{ // skin error, fix team assignment
				ent->dmteam = NO_TEAM;
				TeamCensus( &teamcensus ); // apply team balancing rules
				ent->dmteam = teamcensus.team_for_real;
				safe_bprintf( PRINT_MEDIUM,
						"Invalid Team Skin!  Assigning to %s Team...\n",
						(ent->dmteam == RED_TEAM ? "Red" : "Blue") );
				ClientBegin( ent ); // this might work
			}
		}
		strcpy( playerskin, (ent->dmteam == RED_TEAM ? "red" : "blue") );

		if(strlen(playermodel) > 32) //something went wrong, or somebody is being malicious
			strcpy(playermodel, "martianenforcer/");
		strcpy(s, playermodel);
		strcat(s, playerskin);
		Info_SetValueForKey (userinfo, "skin", s);
	}

	playernum = ent-g_edicts-1;

	//check for duplicates(but not on respawns)
	duplicate = false;
	if(whereFrom != SPAWN) {

		for (j=0; j<maxclients->value ; j++) {

			cl_ent = g_edicts + 1 + j;
			if (!cl_ent->inuse)
				continue;

			if(!strcmp(ent->client->pers.netname, cl_ent->client->pers.netname)) {

				if(playernum != j)
					duplicate = true;
			}
		}

		if(duplicate && playernum < 100) { //just paranoia, should never be more than 64

			sprintf(slot, "%i", playernum);

			if(strlen(ent->client->pers.netname) < 31) { //small enough, just add to end

				strcat(ent->client->pers.netname, slot);
			}
			else { //need to lop off end first

				ent->client->pers.netname[30] = 0;
				strcat(ent->client->pers.netname, slot);
			}

			Info_SetValueForKey (userinfo, "name", ent->client->pers.netname);
			safe_bprintf(PRINT_HIGH, "Was a duplicate, changing name to %s\n", ent->client->pers.netname);
		}
	}

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

	sprintf(modelpath, "players/%s/helmet.md2", playermodel);
	Q2_FindFile (modelpath, &file); //does a helmet exist?
	if(file) {
		sprintf(modelpath, "players/%s/helmet.md2", playermodel);
		ent->s.modelindex3 = gi.modelindex(modelpath);
		fclose(file);
	}
	else
		ent->s.modelindex3 = 0;

	ent->s.modelindex4 = 0;

	//do gib checking here
	//check for gib file
	ent->usegibs = 0; //alien is default
	sprintf(modelpath, "players/%s/usegibs", playermodel);
	Q2_FindFile (modelpath, &file);
	if(file) { //use model specific gibs
		ent->usegibs = 1;
		sprintf(ent->head, "players/%s/head.md2", playermodel);
		sprintf(ent->body, "players/%s/body.md2", playermodel);
		sprintf(ent->leg, "players/%s/leg.md2", playermodel);
		sprintf(ent->arm, "players/%s/arm.md2", playermodel);
		fclose(file);
	}

	// fov
	if (deathmatch->value && (dmflags->integer & DF_FIXED_FOV))
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
	char *s;
	int  playernum;
	int  i, j, k, copychar;
	char playermodel[MAX_OSPATH] = " ";
	char playerskin[MAX_INFO_STRING] = " ";
	char userinfo[MAX_INFO_STRING];

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

	// fix player name if corrupted
	if ( s != NULL )
	{
		i = j = 0;
		while ( s[i] )
		{
			if ( Q_IsColorString( &s[i] ) )
			{ // 2 char color escape sequence
				i += 2;
				if ( i > 47 )
					break;
			}
			else
			{ // printable chars
				if ( s[i] < 32 || s[i] > 126 )
					s[i] = 32;
				++i;
				++j;
				if ( i > 47 || j > 15 )
					break;
			}
		}
		s[i] = 0;
	}

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


	if( ent->dmteam == BLUE_TEAM)
	{
		if ( !ent->is_bot )
			safe_bprintf (PRINT_MEDIUM, "Joined Blue Team...\n");
		strcpy(playerskin, "blue");
	}
	else
	{
		if ( !ent->is_bot )
			safe_bprintf (PRINT_MEDIUM, "Joined Red Team...\n");
		strcpy(playerskin, "red");
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
	if (deathmatch->value && (dmflags->integer & DF_FIXED_FOV))
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

	if ( TEAM_GAME  && strcmp(value, "0") )
	{ // for team game, force spectator off
		Info_SetValueForKey( userinfo, "spectator", "0" );
		ent->client->pers.spectator = 0;
	}

	if (deathmatch->value && *value && strcmp(value, "0")) {

		value = Info_ValueForKey( userinfo, "spectator_password" );

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

	// for real players in team games, team is to be selected
	ent->dmteam = NO_TEAM;
	ent->teamset = false;

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
	int	playernum, i;

	if (!ent->client)
		return;

	safe_bprintf (PRINT_HIGH, "%s disconnected\n", ent->client->pers.netname);

    if(ctf->value)
		CTFDeadDropFlag(ent);

	DeadDropDeathball(ent);

	if(ent->deadflag && ent->client->chasetoggle == 1)
		DeathcamRemove(ent, "off");

	//if in duel mode, we need to bump people down the queue if its the player in game leaving
	if(g_duel->value) {
		MoveClientsDownQueue(ent);
		if(!ent->client->resp.spectator) {
			for (i = 0; i < maxclients->value; i++)  //clear scores if player was in duel
				if(g_edicts[i+1].inuse && g_edicts[i+1].client && !g_edicts[i+1].is_bot)
					g_edicts[i+1].client->resp.score = 0;
		}
	}
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

	// if using bot thresholds, put the bot back in(duel always uses them)
	if ( sv_botkickthreshold->integer || g_duel->integer )
	{
		ACESP_LoadBots( ent );
	}

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
======
 TeamCensus

 tallies up players and bots in game
 used by auto bot kick, and auto team selection
 implements team balancing rules
======
 */
void TeamCensus( teamcensus_t *teamcensus )
{
	int i;
	int diff;
	int dmteam;
	int red_sum;
	int blue_sum;

	int real_red = 0;
	int real_blue = 0;
	int bots_red = 0;
	int bots_blue = 0;
	int team_for_real = NO_TEAM;
	int team_for_bot = NO_TEAM;

	for ( i = 1; i <= game.maxclients; i++ )
	{ // count only clients that have joined teams
		if ( g_edicts[i].inuse )
		{
			dmteam = g_edicts[i].dmteam;
			if ( g_edicts[i].is_bot )
			{
				if ( dmteam == RED_TEAM )
				{
					++bots_red;
				}
				else if ( dmteam == BLUE_TEAM )
				{
					++bots_blue;
				}
			}
			else
			{
				if ( dmteam == RED_TEAM )
				{
					++real_red;
				}
				else if ( dmteam == BLUE_TEAM )
				{
					++real_blue;
				}
			}
		}
	}
	red_sum = real_red + bots_red;
	blue_sum = real_blue + bots_blue;

	// team balancing rules
	if ( red_sum == blue_sum )
	{ // teams are of equal size
		if ( bots_blue == bots_red )
		{ // with equal number of both real players and bots
			// assign by scoring or random selection
			if ( red_team_score > blue_team_score )
			{ // leader gets the bot
				// real player being a good sport joins the team that is behind
				if ( tca->integer )
				{
					team_for_bot = BLUE_TEAM;
					team_for_real = RED_TEAM;
				}
				else
				{
					team_for_bot = RED_TEAM;
					team_for_real = BLUE_TEAM;
				}
			}
			else if ( blue_team_score > red_team_score )
			{
				if ( tca->integer )
				{
					team_for_bot = RED_TEAM;
					team_for_real = BLUE_TEAM;
				}
				else
				{
					team_for_bot = BLUE_TEAM;
					team_for_real = RED_TEAM;
				}
			}
			else if ( rand() & 1 )
			{
				team_for_real = team_for_bot = RED_TEAM;
			}
			else
			{
				team_for_real = team_for_bot = BLUE_TEAM;
			}
		}
		else if ( bots_blue > bots_red )
		{ // with more blue bots than red
			// put bot on team with fewer bots (red)
			// real player on team with fewer real players (blue)
			team_for_bot = RED_TEAM;
			team_for_real = BLUE_TEAM;
		}
		else
		{ // with more red bots than blue
			// put bot on team with fewer bots (blue)
			// real player on team with fewer real players (red)
			team_for_bot = BLUE_TEAM;
			team_for_real = RED_TEAM;
		}
	}
	else
	{ // teams are of unequal size
		diff = red_sum - blue_sum;
		if ( diff > 1 )
		{ // red is 2 or more larger
			team_for_real = team_for_bot = BLUE_TEAM;
		}
		else if ( diff < -1 )
		{ // blue is 2 or more larger
			team_for_real = team_for_bot = RED_TEAM;
		}
		else if ( real_blue == real_red )
		{ // with equal numbers of real players
			if ( bots_blue > bots_red )
			{ // blue team is larger by 1 bot
				team_for_real = team_for_bot = RED_TEAM;
			}
			else
			{ // red_team is larger by 1 bot
				team_for_real = team_for_bot = BLUE_TEAM;
			}
		}
		else
		{ // with equal numbers of bots
			if ( real_blue > real_red )
			{ // blue team is larger by 1 real player
				team_for_real = team_for_bot = RED_TEAM;
			}
			else
			{ // red team is larger by 1 real player
				team_for_real = team_for_bot = BLUE_TEAM;
			}
		}
	}

	teamcensus->total = red_sum + blue_sum; //   sum of all
	teamcensus->real = real_red + real_blue; //  sum of real players
	teamcensus->bots = bots_red + bots_blue; //  sum of bots
	teamcensus->red = real_red + bots_red; //    sum of red team members
	teamcensus->blue = real_blue + bots_blue; // sum of blue team members
	teamcensus->real_red = real_red; //   red team players in game
	teamcensus->real_blue = real_blue; // blue team players in game
	teamcensus->bots_red = bots_red; //   red team bots in game
	teamcensus->bots_blue = bots_blue; // blue team bots in game
	teamcensus->team_for_real = team_for_real; // team for real player to join
	teamcensus->team_for_bot = team_for_bot; //   team for bot to join
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
	int		i, j, mostvotes, n_candidates;
	int		map_candidates[4];
	pmove_t	pm;
	qboolean sproing, haste;
	vec3_t addspeed, forward, up, right;
	teamcensus_t teamcensus;

	level.current_entity = ent;
	client = ent->client;

	//unlagged
	if ( g_antilag->integer)
		client->attackTime = gi.Sys_Milliseconds();

	if (level.intermissiontime)
	{
		client->ps.pmove.pm_type = PM_FREEZE;

		// can exit intermission after 10 seconds, or 20 if map voting enables
		// (voting will only work if g_mapvote wasn't modified during intermission)
		if (g_mapvote->value && ! g_mapvote->modified) {

			//print out results, track winning map
			mostvotes = 0;

			for (j = 0; j < 4; j++) {
				if (votedmap[j].tally > mostvotes)
					mostvotes = votedmap[j].tally;
			}

			if ( g_voterand && g_voterand->value )
			{
				// we're using a random value for the next map
				// if a choice needs to be done
				n_candidates = 0;
				for ( j = 0 ; j < 4 ; j ++ ) {
					if ( votedmap[j].tally < mostvotes )
						continue;
					map_candidates[n_candidates ++] = j;
				}

				j = random() * (n_candidates - 1);
				strcpy(level.changemap, votedmap[map_candidates[j]].mapname);
			}
			else
			{
				// "old" voting system, take the first map that
				// has enough votes
				for ( j = 0 ; j < 4 ; j ++ ) {
					i = (j + 1) % 4;
					if ( votedmap[i].tally < mostvotes )
						continue;
					strcpy(level.changemap, votedmap[i].mapname);
					break;
				}
			}

			if (level.time > level.intermissiontime + 20.0
				&& (ucmd->buttons & BUTTON_ANY) )
				level.exitintermission = true;
		}
		else {
			if (level.time > level.intermissiontime + 10.0
				&& (ucmd->buttons & BUTTON_ANY) )
				level.exitintermission = true;
		}
		return;
	}
	else if ( g_mapvote && g_mapvote->modified )
	{
		g_mapvote->modified = false;
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
		}

		ucmd->forwardmove *= 1.3;

		//dodging
		client->dodge = false;
		if((level.time - client->lastdodge) > 1.0 && ent->groundentity && ucmd->forwardmove == 0 && ucmd->sidemove != 0 && client->moved == false
			&& ((level.time - client->lastmovetime) < .15))
		{
			if((ucmd->sidemove < 0 && client->lastsidemove < 0) || (ucmd->sidemove > 0 && client->lastsidemove > 0)) {
				if(ucmd->sidemove > 0)
					client->dodge = 1;
				else
					client->dodge = -1;
				ucmd->upmove += 100;
			}
		}

		if(ucmd->sidemove != 0 || ucmd->forwardmove != 0) {
			client->lastmovetime = level.time;
			client->lastsidemove = ucmd->sidemove;
			client->moved = true;
		}
		else //we had a frame with no movement
			client->moved = false;

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

		//check for a dodge, and peform if true
		if(client->dodge != 0) {
			AngleVectors (ent->s.angles, forward, addspeed, up);
			addspeed[0] *= 300*client->dodge;
			addspeed[1] *= 300*client->dodge;
			//limit to reasonable
			for(i = 0; i < 2; i++) {
				if(addspeed[i] > 800)
					addspeed[i] = 800;
				if(addspeed[i] < -800)
					addspeed[i] = -800;
			}
			VectorAdd(ent->velocity, addspeed, ent->velocity);
			client->dodge = false;
			client->lastdodge = client->lastmovetime = level.time;
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
			if(sproing) {
				gi.sound(ent, CHAN_VOICE, gi.soundindex("items/sproing.wav"), 1, ATTN_NORM, 0);
				ent->velocity[2] += 400;
			}
			if(haste) {
				AngleVectors (ent->s.angles, addspeed, right, up);
				addspeed[0] *= 400;
				addspeed[1] *= 400;
				for(i = 0; i < 2; i++) {
				if(addspeed[i] > 200)
					addspeed[i] = 200;
				}
				VectorAdd(ent->velocity, addspeed, ent->velocity);

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

	if ( client->resp.spectator == 0 )
	{ // regular (non-spectator) mode
		if (client->latched_buttons & BUTTON_ATTACK)
		{
			if (!client->weapon_thunk)
			{
				client->weapon_thunk = true;
				Think_Weapon (ent);
			}
		}
	}
	else
	{ // spectator mode

		if ( TEAM_GAME && client->resp.spectator < 2 )
		{ // team selection state

			if ( client->pers.spectator == 2 )
			{ // entered with special team spectator mode on
				// force it off, does not appear to help much. (?)
				ent->dmteam = NO_TEAM;
				client->pers.spectator = 0;
			}

			if ( ent->dmteam == RED_TEAM || ent->dmteam == BLUE_TEAM )
			{
				client->pers.spectator = 0;
			}

			if ( client->pers.spectator == 1 && ent->dmteam == NO_TEAM
					&& (level.time / 2 == ceil( level.time / 2 )) )
			{ // send "how to join" message
				if ( g_autobalance->integer )
				{
					safe_centerprintf( ent, "\n\n\nPress <fire> to join\n"
						"autobalanced team\n" );
				}
				else
				{
					safe_centerprintf( ent, "\n\n\nPress <fire> to autojoin\n"
						"or <jump> to join BLUE\n"
						"or <crouch> to join RED\n" );
				}
			}

			if ( client->latched_buttons & BUTTON_ATTACK )
			{ // <fire> to auto join
				client->latched_buttons = 0;
				if ( client->pers.spectator == 1 && ent->dmteam == NO_TEAM)
				{
					TeamCensus( &teamcensus ); // apply team balance rules
					ent->dmteam = teamcensus.team_for_real;
					client->pers.spectator = 0;
					ClientChangeSkin( ent );
				}
			}

			if ( ucmd->upmove >= 10 && ent->dmteam == NO_TEAM )
			{
				if ( !g_autobalance->integer )
				{ // jump to join blue
					ent->dmteam = BLUE_TEAM;
					client->pers.spectator = 0;
					ClientChangeSkin( ent );
				}
			}
			else if ( ucmd->upmove < 0 && ent->dmteam == NO_TEAM )
			{
				if ( !g_autobalance->integer )
				{ // crouch to join red
					ent->dmteam = RED_TEAM;
					client->pers.spectator = 0;
					ClientChangeSkin( ent );
				}
			}
			else
			{
				client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
			}
		} /* team selection state */
		else
		{ // regular spectator
			if ( client->latched_buttons & BUTTON_ATTACK )
			{
				client->latched_buttons = 0;
				if ( client->chase_target )
				{
					client->chase_target = NULL;
					client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
				}
				else
				{
					GetChaseTarget( ent );
				}
			}
			if ( ucmd->upmove >= 10 )
			{
				if ( !(client->ps.pmove.pm_flags & PMF_JUMP_HELD) )
				{
					client->ps.pmove.pm_flags |= PMF_JUMP_HELD;
					if ( client->chase_target )
						ChaseNext( ent );
					else
						GetChaseTarget( ent );
				}
			}
			else
			{
				client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
			}
		} /* regular spectator */
	} /* spectator mode */

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
	if ( deathmatch->value && client->pers.spectator != client->resp.spectator
			&& (level.time - client->respawn_time) >= 5 )
	{
		if ( TEAM_GAME && client->pers.spectator == 2 )
		{ // special team game spectator mode (has problems)
			if ( client->resp.spectator == 1 )
			{ // special team spectator mode
				spectator_respawn( ent );
				client->resp.spectator = 2;
			}
		}
		else if ( TEAM_GAME && client->resp.spectator == 2 )
		{ // pers != resp, exit spectator mode not allowed
			client->pers.spectator = 2;
		}
		else
		{ // enter or exit spectator mode
			spectator_respawn( ent );
		}
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
				buttonMask = BUTTON_ATTACK | BUTTON_ATTACK2;
			else
				buttonMask = -1;

			//should probably add in a force respawn option
			if (( client->latched_buttons & buttonMask ) ||
				(deathmatch->value && (dmflags->integer & DF_FORCE_RESPAWN) ) )
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
