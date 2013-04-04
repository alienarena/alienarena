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

#include "g_local.h"

qboolean	Pickup_Weapon (edict_t *ent, edict_t *other);
void		Use_Weapon (edict_t *ent, gitem_t *inv);
void		Drop_Weapon (edict_t *ent, gitem_t *inv);

#ifdef ALTERIA
void Weapon_Punch ( edict_t *ent);
#else
void Weapon_Blaster (edict_t *ent);
void Weapon_AlienBlaster (edict_t *ent);
void Weapon_Violator (edict_t *ent);
void Weapon_Smartgun (edict_t *ent);
void Weapon_Chain (edict_t *ent);
void Weapon_Flame (edict_t *ent);
void Weapon_Disruptor (edict_t *ent);
void Weapon_RocketLauncher (edict_t *ent);
void Weapon_Beamgun (edict_t *ent);
void Weapon_Vaporizer (edict_t *ent);
void Weapon_Minderaser (edict_t *ent);
void Weapon_Bomber (edict_t *ent);
void Weapon_Strafer (edict_t *ent);
void Weapon_Deathball (edict_t *ent);
void Weapon_Hover (edict_t *ent);
void Weapon_TacticalBomb ( edict_t *ent);
#endif

gitem_armor_t jacketarmor_info	= { 25,  50, .30, .00, ARMOR_JACKET};
gitem_armor_t combatarmor_info	= { 50, 100, .60, .30, ARMOR_COMBAT};
gitem_armor_t bodyarmor_info	= {100, 200, .80, .60, ARMOR_BODY};

static int	jacket_armor_index;
static int	combat_armor_index;
static int	body_armor_index;

#define HEALTH_IGNORE_MAX	1
#define HEALTH_TIMED		2

void Use_Quad (edict_t *ent, gitem_t *item);
static int	quad_drop_timeout_hack;

//======================================================================

/*
===============
GetItemByIndex
===============
*/
gitem_t	*GetItemByIndex (int index)
{
	if (index == 0 || index >= game.num_items)
		return NULL;

	return &itemlist[index];
}


/*
===============
FindItemByClassname

===============
*/
gitem_t	*FindItemByClassname (char *classname)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i=0 ; i<game.num_items ; i++, it++)
	{
		if (!it->classname)
			continue;
		if (!Q_strcasecmp(it->classname, classname))
			return it;
	}

	return NULL;
}

/*
===============
FindItem

===============
*/
gitem_t	*FindItem (char *pickup_name)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i=0 ; i<game.num_items ; i++, it++)
	{
		if (!it->pickup_name)
			continue;
		if (!Q_strcasecmp(it->pickup_name, pickup_name))
			return it;
	}

	return NULL;
}

//======================================================================

static void drop_temp_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	Touch_Item (ent, other, plane, surf);
}

static void drop_make_touchable (edict_t *ent)
{
	ent->touch = Touch_Item;
	if (deathmatch->integer)
	{
		if(!g_tactical->integer) //in tactical mode, we don't remove dropped items
		{
			ent->nextthink = level.time + 29;
			ent->think = G_FreeEdict;
		}
	}
}

float mindEraserTime;
void SpawnMinderaser(edict_t *ent)
{
	edict_t *minderaser, *cl_ent;
	int i;

	for (i = 0; i < g_maxclients->integer; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->is_bot)
			continue;
		safe_centerprintf(cl_ent, "A Mind Eraser has spawned!\n");
	}

	//to do - play level wide klaxxon 
	gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "misc/minderaser.wav" ), 1, ATTN_NONE, 0 );

	minderaser = G_Spawn();
	VectorCopy(ent->s.origin, minderaser->s.origin);
	minderaser->spawnflags = DROPPED_PLAYER_ITEM;
	minderaser->model = "models/weapons/g_minderaser/tris.md2";
	minderaser->classname = "weapon_minderaser";
	minderaser->item = FindItem ("Minderaser");
	minderaser->s.effects = minderaser->item->world_model_flags;
	minderaser->s.renderfx = RF_GLOW;
	VectorSet (minderaser->mins, -15, -15, -15);
	VectorSet (minderaser->maxs, 15, 15, 15);
	gi.setmodel (minderaser, minderaser->item->world_model);
	minderaser->solid = SOLID_TRIGGER;
	minderaser->health = 100;
	minderaser->movetype = MOVETYPE_TOSS;
	minderaser->touch = drop_temp_touch;
	minderaser->owner = NULL;

	SetRespawn (ent, 1000000); //huge delay until ME is picked up from pad.			
	minderaser->replaced_weapon = ent; //remember this entity

	mindEraserTime = level.time;
}

void DoRespawn (edict_t *ent)
{
	char szTmp[64];
	//Add mind eraser spawn here, if it's been two minutes since last respawn of it,
	//go ahead and set the next weapon spawned to be the mind eraser
	//need to check if first part of name is weapon
	if(level.time > mindEraserTime + 120.0)
	{
		strcpy(szTmp, ent->classname);
		szTmp[6] = 0;
		if(!Q_strcasecmp(szTmp, "weapon"))
		{
			SpawnMinderaser(ent);
			return;
		}
	}

	if (ent->team)
	{
		edict_t	*master;
		int	count;
		int choice;

		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->chain, count++)
			;

		choice = rand() % count;

		for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
			;
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	gi.linkentity (ent);

	// send an effect
	ent->s.event = EV_ITEM_RESPAWN;
}

void SetRespawn (edict_t *ent, float delay)
{

    if (	ent->item && g_duel->integer && 
    		ent->item->weapmodel != WEAP_MINDERASER		)
	{
		switch (ent->item->flags)
		{
			//TODO: playtest this and adjust the scaling factors.
			case IT_WEAPON:
				delay *= 3.0;
				break;
			case IT_POWERUP:
			case IT_AMMO: //intentional fallthrough
				delay *= 2.0;
				break;
			case IT_ARMOR:
			case IT_HEALTH: //intentional fallthrough
				delay *= 1.5;
				break;
			default:
				break;
		}
	}
	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->nextthink = level.time + delay;
	ent->think = DoRespawn;
	gi.linkentity (ent);
}


//======================================================================

qboolean Pickup_Powerup (edict_t *ent, edict_t *other)
{
	int		quantity;

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];
	if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
		return false;

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	if (deathmatch->integer)
	{
		int randomSpawn;
		//Phenax - Add random time to quad spawn
		if(ent->item->use == Use_Quad && g_randomquad->integer)
			randomSpawn = 10 + rand() % (30 - 10); //10 to 30 seconds randomness
		else
			randomSpawn = 0;

		if (!(ent->spawnflags & DROPPED_ITEM) )
			SetRespawn (ent, ent->item->quantity + randomSpawn);
		if ((dmflags->integer & DF_INSTANT_ITEMS) || (ent->item->use == Use_Quad && (ent->spawnflags & DROPPED_PLAYER_ITEM)))
		{
			if ((ent->item->use == Use_Quad) && (ent->spawnflags & DROPPED_PLAYER_ITEM))
				quad_drop_timeout_hack = (ent->nextthink - level.time) / FRAMETIME;
			ent->item->use (other, ent->item);
		}
	}

	return true;
}

void Drop_General (edict_t *ent, gitem_t *item)
{
	Drop_Item (ent, item);
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);
}


//======================================================================

qboolean Pickup_Adrenaline (edict_t *ent, edict_t *other)
{
	if (!deathmatch->integer)
		other->max_health += 1;

	if (other->health < other->max_health)
		other->health = other->max_health;

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->integer))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

//======================================================================

void Use_Quad (edict_t *ent, gitem_t *item)
{
	int		timeout;

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (quad_drop_timeout_hack)
	{
		timeout = quad_drop_timeout_hack;
		quad_drop_timeout_hack = 0;
	}
	else
	{
		timeout = 300;
	}

	if (ent->client->quad_framenum > level.framenum)
		ent->client->quad_framenum += timeout;
	else
		ent->client->quad_framenum = level.framenum + timeout;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

void	Use_Invulnerability (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->invincible_framenum > level.framenum)
		ent->client->invincible_framenum += 300;
	else
		ent->client->invincible_framenum = level.framenum + 300;

	//add full armor
	ent->client->pers.inventory[combat_armor_index] = 200;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

void Use_Haste (edict_t *ent, gitem_t *item)
{
	gitem_t *it;

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if(ent->client->resp.powered) {
		it = FindItem("Sproing");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Invisibility");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		ent->client->resp.reward_pts = 0;
		ent->client->resp.powered = false;
	}

	if (ent->client->haste_framenum > level.framenum)
		ent->client->haste_framenum += 300;
	else
		ent->client->haste_framenum = level.framenum + 300;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/powerup.wav"), 1, ATTN_NORM, 0);
}
//======================================================================

void Use_Sproing (edict_t *ent, gitem_t *item)
{
	gitem_t *it;

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if(ent->client->resp.powered) {

		it = FindItem("Invisibility");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Haste");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		ent->client->resp.reward_pts = 0;
		ent->client->resp.powered = false;
	}

	if (ent->client->sproing_framenum > level.framenum)
		ent->client->sproing_framenum += 300;
	else
		ent->client->sproing_framenum = level.framenum + 300;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/powerup.wav"), 1, ATTN_NORM, 0);
}

void Use_Invisibility (edict_t *ent, gitem_t *item)
{
	gitem_t *it;

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if(ent->client->resp.powered) {
		it = FindItem("Sproing");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = FindItem("Haste");
		ent->client->pers.inventory[ITEM_INDEX(it)] = 0;

		ent->client->resp.reward_pts = 0;
		ent->client->resp.powered = false;
	}

	if (ent->client->invis_framenum > level.framenum)
		ent->client->invis_framenum += 300;
	else
		ent->client->invis_framenum = level.framenum + 300;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/powerup.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

qboolean Pickup_Key (edict_t *ent, edict_t *other)
{
	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
	return true;
}

//======================================================================

qboolean Add_Ammo (edict_t *ent, gitem_t *item, int count, qboolean weapon, qboolean dropped)
{
	int			index;
	int			max, base;
	gitem_t     *failedswitch;

	if (!ent->client)
		return false;

	if (item->tag == AMMO_BULLETS) 
	{
		max = ent->client->pers.max_bullets;
		base = BASE_BULLETS;
	}
	else if (item->tag == AMMO_SHELLS) 
	{
		max = ent->client->pers.max_shells;
		base = BASE_SHELLS;
	}
	else if (item->tag == AMMO_ROCKETS) 
	{
		max = ent->client->pers.max_rockets;
		base = BASE_ROCKETS;
	}
	else if (item->tag == AMMO_GRENADES) 
	{
		max = ent->client->pers.max_grenades;
		base = BASE_GRENADES;
	}
	else if (item->tag == AMMO_CELLS) 
	{
		max = ent->client->pers.max_cells;
		base = BASE_CELLS;
	}
	else if (item->tag == AMMO_SLUGS) 
	{
		max = ent->client->pers.max_slugs;
		base = BASE_SLUGS;
	}
	else if (item->tag == AMMO_SEEKERS) 
	{
		max = ent->client->pers.max_seekers;
		base = BASE_SEEKERS;
	}
	else if (item->tag == AMMO_BOMBS) 
	{
		max = ent->client->pers.max_bombs;
		base = BASE_BOMBS;
	}
	else
		return false;

	index = ITEM_INDEX(item);

	if (ent->client->pers.inventory[index] == max)
		return false;

	if (weapon && !dropped && (ent->client->pers.inventory[index] > 0))
		count = 1; //already has weapon -- not dropped. Give him 1 ammo.

	//if less than base ammo, restock ammo fully
	if(ent->client->pers.inventory[index] < base) //less than base ammount
		ent->client->pers.inventory[index] = base;
	else
		ent->client->pers.inventory[index] += count;

	if (ent->client->pers.inventory[index] > max)
		ent->client->pers.inventory[index] = max;
    
    failedswitch = ent->client->pers.lastfailedswitch;
    if (failedswitch && failedswitch->ammo && 
        (FindItem(failedswitch->ammo) == item) && 
        (level.framenum - ent->client->pers.failedswitch_framenum) < 5)
		ent->client->newweapon = failedswitch;

	return true;
}

qboolean Pickup_Ammo (edict_t *ent, edict_t *other)
{
	int			oldcount;
	int			count;
	qboolean		weapon;

	weapon = (ent->item->flags & IT_WEAPON);
	if ( weapon && ( dmflags->integer & DF_INFINITE_AMMO ) )
		count = 1000;
	else if (ent->count)
		count = ent->count;
	else
		count = ent->item->quantity;

	oldcount = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	if (!Add_Ammo (other, ent->item, count, false, true)) //Not 'dropped' but give full ammo even if ammo > 0
		return false;

	if (weapon && !oldcount)
	{
		if (other->client->pers.weapon != ent->item && ( !deathmatch->integer || other->client->pers.weapon == FindItem("Blaster") || other->client->pers.weapon == FindItem("Alien Blaster")) )
			other->client->newweapon = ent->item;
	}

	if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) && (deathmatch->integer))
	{
		if(g_tactical->integer)
		{
			if(!strcmp(ent->classname, "ammo_cells") || !strcmp(ent->classname, "ammo_shells"))
			{
				if(!tacticalScore.alienPowerSource)
					SetRespawn (ent, 20); //on backup power, generate ammo much slower
				else
					SetRespawn (ent, 5);
			}
			else if(!strcmp(ent->classname, "ammo_rockets") || !strcmp(ent->classname, "ammo_bullets") || !strcmp(ent->classname, "ammo_grenades"))
			{
				if(!tacticalScore.humanPowerSource)
					SetRespawn (ent, 20);
				else
					SetRespawn (ent, 5);
			}
		}
		else
			SetRespawn (ent, 30);
	}

	return true;
}

void Drop_Ammo (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	int		index;

	index = ITEM_INDEX(item);
	dropped = Drop_Item (ent, item);
	if (ent->client->pers.inventory[index] >= item->quantity)
		dropped->count = item->quantity;
	else
		dropped->count = ent->client->pers.inventory[index];

	if (ent->client->pers.weapon &&
		ent->client->pers.weapon->tag == AMMO_GRENADES &&
		item->tag == AMMO_GRENADES &&
		ent->client->pers.inventory[index] - dropped->count <= 0) {
		safe_cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
		G_FreeEdict(dropped);
		return;
	}

	ent->client->pers.inventory[index] -= dropped->count;
	ValidateSelectedItem (ent);
}


//======================================================================

void MegaHealth_think (edict_t *self)
{
	if (self->owner->health > self->owner->max_health)
	{
		self->nextthink = level.time + 1;
		self->owner->health -= 1;
		return;
	}

	if (!(self->spawnflags & DROPPED_ITEM) && (deathmatch->integer))
		SetRespawn (self, 20);
	else
		G_FreeEdict (self);
}
void Healthbox_think (edict_t *self)
{

	self->nextthink = level.time + 7;
	self->s.effects = EF_ROTATE;
	self->s.renderfx = RF_GLOW;
	return;
}
qboolean Pickup_Health (edict_t *ent, edict_t *other)
{
	if (!(ent->style & HEALTH_IGNORE_MAX))
		if (other->health >= other->max_health)
		{
			ent->s.effects |= EF_ROTATE;
			ent->think = Healthbox_think;
			ent->nextthink = level.time + 7;
			return false;
		}

	other->health += ent->count;

	if (!(ent->style & HEALTH_IGNORE_MAX))
	{
		if (other->health > other->max_health)
			other->health = other->max_health;
	}

	if (ent->style & HEALTH_TIMED)
	{
		ent->think = MegaHealth_think;
		ent->nextthink = level.time + 5;
		ent->owner = other;
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	else
	{
		if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->integer))
			SetRespawn (ent, 30);
	}

	return true;
}

//======================================================================

int ArmorIndex (edict_t *ent)
{
	if (!ent->client)
		return 0;

	if (ent->client->pers.inventory[jacket_armor_index] > 0)
		return jacket_armor_index;

	if (ent->client->pers.inventory[combat_armor_index] > 0)
		return combat_armor_index;

	if (ent->client->pers.inventory[body_armor_index] > 0)
		return body_armor_index;

	return 0;
}

qboolean Pickup_Armor (edict_t *ent, edict_t *other)
{
	int				old_armor_index;
	gitem_armor_t	*oldinfo;
	gitem_armor_t	*newinfo;
	int				newcount;
	float			salvage;
	int				salvagecount;

	// get info on new armor
	newinfo = (gitem_armor_t *)ent->item->info;

	old_armor_index = ArmorIndex (other);

	// handle armor shards specially
	if (ent->item->tag == ARMOR_SHARD)
	{
		if (!old_armor_index)
			other->client->pers.inventory[jacket_armor_index] = 5;
		else
			other->client->pers.inventory[old_armor_index] += 5;
	}

	// if player has no armor, just use it
	else if (!old_armor_index)
	{
		other->client->pers.inventory[ITEM_INDEX(ent->item)] = newinfo->base_count;
	}

	// use the better armor
	else
	{
		// get info on old armor
		if (old_armor_index == jacket_armor_index)
			oldinfo = &jacketarmor_info;
		else if (old_armor_index == combat_armor_index)
			oldinfo = &combatarmor_info;
		else // (old_armor_index == body_armor_index)
			oldinfo = &bodyarmor_info;

		if (newinfo->normal_protection > oldinfo->normal_protection)
		{
			// calc new armor values
			salvage = oldinfo->normal_protection / newinfo->normal_protection;
			salvagecount = salvage * other->client->pers.inventory[old_armor_index];
			newcount = newinfo->base_count + salvagecount;
			if (newcount > newinfo->max_count)
				newcount = newinfo->max_count;

			// zero count of old armor so it goes away
			other->client->pers.inventory[old_armor_index] = 0;

			// change armor to new item with computed value
			other->client->pers.inventory[ITEM_INDEX(ent->item)] = newcount;
		}
		else
		{
			// calc new armor values
			salvage = newinfo->normal_protection / oldinfo->normal_protection;
			salvagecount = salvage * newinfo->base_count;
			newcount = other->client->pers.inventory[old_armor_index] + salvagecount;
			if (newcount > oldinfo->max_count)
				newcount = oldinfo->max_count;

			// if we're already maxed out then we don't need the new armor
			if (other->client->pers.inventory[old_armor_index] >= newcount)
				return false;

			// update current armor value
			other->client->pers.inventory[old_armor_index] = newcount;
		}
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->integer))
		SetRespawn (ent, 20);
	
	return true;
}

//======================================================================

/*
===============
Touch_Item
===============
*/
void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	qboolean	taken;

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup
	if (!ent->item->pickup)
		return;		// not a grabbable item?

	taken = ent->item->pickup(ent, other);

	if (taken)
	{
		// flash the screen
		other->client->bonus_alpha = 0.25;

		// show icon and name on status bar
		other->client->pickup_msg_time = level.time + 3.0;

		// change selected item
		if (ent->item->use)
			other->client->pers.selected_item = other->client->ps.stats[STAT_SELECTED_ITEM] = ITEM_INDEX(ent->item);

		if (ent->item->pickup == Pickup_Health)
		{
			if (ent->count == 5)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/s_health.wav"), 1, ATTN_NORM, 0);
			else if (ent->count == 10)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1, ATTN_NORM, 0);
			else if (ent->count == 25)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/l_health.wav"), 1, ATTN_NORM, 0);
			else // (ent->count == 100)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/m_health.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent->item->pickup_sound)
		{
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent->item->pickup_sound), 1, ATTN_NORM, 0);
		}
	}

	if (!(ent->spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets (ent, other);
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;

	if(g_tactical->integer) //items do not respawn in tactical mode(except ammo when ammo depots are running)
	{
		if((!strcmp(ent->classname, "ammo_cells") || !strcmp(ent->classname, "ammo_shells")) && tacticalScore.alienAmmoDepot)
			return;
		else if((!strcmp(ent->classname, "ammo_rockets") || !strcmp(ent->classname, "ammo_bullets") || !strcmp(ent->classname, "ammo_grenades")) && tacticalScore.humanAmmoDepot)
			return;

		G_FreeEdict (ent); 
		return;
	}

	if (ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM))
	{
		if (ent->flags & FL_RESPAWN)
			ent->flags &= ~FL_RESPAWN;
		else
			G_FreeEdict (ent);
	}
}

//======================================================================

edict_t *Drop_Item (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	vec3_t	forward, right;
	vec3_t	offset;

	dropped = G_Spawn();

	dropped->classname = item->classname;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.effects = item->world_model_flags;
	dropped->s.renderfx = RF_GLOW;
	VectorSet (dropped->mins, -15, -15, -15);
	VectorSet (dropped->maxs, 15, 15, 15);
	gi.setmodel (dropped, dropped->item->world_model);
	if(!strcmp(item->classname, "item_bomber"))
		dropped->s.modelindex3 = gi.modelindex("vehicles/bomber/helmet.md2");
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;
	dropped->touch = drop_temp_touch;
	dropped->owner = ent;

	if (ent->client)
	{
		trace_t	trace;

		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource (ent->s.origin, offset, forward, right, dropped->s.origin);
		trace = gi.trace (ent->s.origin, dropped->mins, dropped->maxs,
			dropped->s.origin, ent, CONTENTS_SOLID);
		VectorCopy (trace.endpos, dropped->s.origin);
	}
	else
	{
		AngleVectors (ent->s.angles, forward, right, NULL);
		VectorCopy (ent->s.origin, dropped->s.origin);
	}

	VectorScale (forward, 100, dropped->velocity);
	dropped->velocity[2] = 300;

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.time + 1;

	gi.linkentity (dropped);

	return dropped;
}

void Use_Item (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->svflags &= ~SVF_NOCLIENT;
	ent->use = NULL;

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->touch = Touch_Item;
	}

	gi.linkentity (ent);
}

//======================================================================

/*
================
droptofloor
================
*/
void droptofloor (edict_t *ent)
{
	trace_t		tr;
	vec3_t		dest;
	float		*v;

	v = tv(-15,-15,-15);
	VectorCopy (v, ent->mins);
	v = tv(15,15,15);
	VectorCopy (v, ent->maxs);

	if (ent->model)
		gi.setmodel (ent, ent->model);
	else
		gi.setmodel (ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;

	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;

	v = tv(0,0,-128);
	VectorAdd (ent->s.origin, v, dest);

	tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid)
	{
		gi.dprintf ("droptofloor: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	VectorCopy (tr.endpos, ent->s.origin);

	if (ent->team)
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;

		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		if (ent == ent->teammaster)
		{
			ent->nextthink = level.time + FRAMETIME;
			ent->think = DoRespawn;
		}
	}

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
		ent->s.effects &= ~EF_ROTATE;
		ent->s.renderfx &= ~RF_GLOW;
	}

	if (ent->spawnflags & ITEM_TRIGGER_SPAWN)
	{
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		ent->use = Use_Item;
	}

	gi.linkentity (ent);
}


/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem (gitem_t *it)
{
	char	*s, *start;
	char	data[MAX_QPATH];
	int		len;
	gitem_t	*ammo;

	if (!it)
		return;

	if (it->pickup_sound)
		gi.soundindex (it->pickup_sound);
	if (it->world_model)
		gi.modelindex (it->world_model);
	if (it->view_model)
		gi.modelindex (it->view_model);
	if (it->icon)
		gi.imageindex (it->icon);

	// parse everything for its ammo
	if (it->ammo && it->ammo[0])
	{
		ammo = FindItem (it->ammo);
		if (ammo != it)
			PrecacheItem (ammo);
	}

	// parse the space seperated precache string for other items
	s = it->precaches;
	if (!s || !s[0])
		return;

	while (*s)
	{
		start = s;
		while (*s && *s != ' ')
			s++;

		len = s-start;
		if (len >= MAX_QPATH || len < 5)
			gi.error ("PrecacheItem: %s has bad precache string", it->classname);
		memcpy (data, start, len);
		data[len] = 0;
		if (*s)
			s++;

		// determine type based on extension
		if (!strcmp(data+len-3, "md2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "sp2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "wav"))
			gi.soundindex (data);
		if (!strcmp(data+len-3, "pcx"))
			gi.imageindex (data);
	}
}

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void SpawnItem (edict_t *ent, gitem_t *item)
{
	PrecacheItem (item);

	if (ent->spawnflags)
	{
		if (strcmp(ent->classname, "key_power_cube") != 0)
		{
			ent->spawnflags = 0;
			gi.dprintf("%s at %s has invalid spawnflags set\n", ent->classname, vtos(ent->s.origin));
		}
	}

	// some items will be prevented in deathmatch
	if (deathmatch->integer)
	{
		if ( dmflags->integer & DF_NO_ARMOR )
		{
			if (item->pickup == Pickup_Armor)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( dmflags->integer & DF_NO_ITEMS )
		{
			if (item->pickup == Pickup_Powerup)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( dmflags->integer & DF_NO_HEALTH )
		{
			if (item->pickup == Pickup_Health || item->pickup == Pickup_Adrenaline)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( dmflags->integer & DF_INFINITE_AMMO )
		{
			if ( (item->flags == IT_AMMO) || (strcmp(ent->classname, "weapon_bfg") == 0) )
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if(excessive->integer || instagib->integer || rocket_arena->integer || insta_rockets->integer )
		{
			if (item->flags == IT_AMMO || (strcmp(ent->classname, "weapon_bfg") == 0) ||
				(strcmp(ent->classname, "weapon_hyperblaster") == 0) ||
				(strcmp(ent->classname, "weapon_railgun") == 0) ||
				(strcmp(ent->classname, "weapon_rocketlauncher") == 0) ||
				(strcmp(ent->classname, "weapon_grenadelauncher") == 0) ||
				(strcmp(ent->classname, "weapon_chaingun") == 0) ||
				(strcmp(ent->classname, "weapon_supershotgun") == 0) ||
				(strcmp(ent->classname, "weapon_shotgun") == 0))
			{
				G_FreeEdict (ent);
				return;
			}
		}
	}

	//CTF
	//Don't spawn the flags unless enabled
	if (!ctf->integer &&
		(strcmp(ent->classname, "item_flag_red") == 0 ||
		strcmp(ent->classname, "item_flag_blue") == 0)) {
		G_FreeEdict(ent);
		return;
	}

	ent->item = item;
	ent->nextthink = level.time + 2 * FRAMETIME;    // items start after other solids
	ent->think = droptofloor;
	if (strcmp(ent->classname, "item_flag_red") && //flags are special and don't get this
		strcmp(ent->classname, "item_flag_blue")) {
		ent->s.effects = EF_ROTATE;

	}
	ent->s.renderfx = RF_GLOW;
	if((strcmp(ent->classname, "Health") == 0)) {
		ent->s.modelindex2 = gi.modelindex("models/items/healing/globe/tris.md2");
	}
	else if((strcmp(ent->classname, "item_quad") == 0)) {
		ent->s.modelindex2 = gi.modelindex("models/items/quaddama/unit.md2");
	}

	if (ent->model)
		gi.modelindex (ent->model);

	//flags are server animated and have special handling
	if (strcmp(ent->classname, "item_flag_red") == 0 ||
		strcmp(ent->classname, "item_flag_blue") == 0) {
		ent->think = CTFFlagSetup;
	}

	//vehicles have special handling
	if((strcmp(ent->classname, "item_bomber") == 0) || (strcmp(ent->classname, "item_strafer") == 0)
		|| (strcmp(ent->classname, "item_hover") == 0))
		ent->think = VehicleSetup;

	//ditto for deathball
	if(strcmp(ent->classname, "item_deathball") == 0)
		ent->think = DeathballSetup;
}

//======================================================================

gitem_t	itemlist[] =
{
	{
		NULL
	},	// leave index 0 alone

	//
	// ARMOR
	//

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_body",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/body/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_bodyarmor",
/* pickup */	"Body Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&bodyarmor_info,
		ARMOR_BODY,
/* precache */ ""
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_combat",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/combat/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_combatarmor",
/* pickup */	"Combat Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&combatarmor_info,
		ARMOR_COMBAT,
/* precache */ ""
	},

/*QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_jacket",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/jacket/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_jacketarmor",
/* pickup */	"Jacket Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&jacketarmor_info,
		ARMOR_JACKET,
/* precache */ ""
	},

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_shard",
		Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar2_pkup.wav",
		"models/items/armor/shard/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_jacketarmor",
/* pickup */	"Armor Shard",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		ARMOR_SHARD,
/* precache */ ""
	},

//CTF
/*QUAKED item_flag_team1 (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		"item_flag_red",
		CTFPickup_Flag,
		NULL,
		CTFDrop_Flag, //Should this be null if we don't want players to drop it manually?
		NULL,
		NULL,
		"models/items/flags/flag1.md2", EF_TEAM1,
		NULL,
/* icon */		"i_flag1",
/* pickup */	"Red Flag",
/* width */		2,
		0,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ "misc/red_scores.wav misc/red_takes.wav misc/red_increases.wav misc/red_wins.wav misc/scores_tied.wav misc/red_picked.wav"
	},

/*QUAKED item_flag_team2 (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		"item_flag_blue",
		CTFPickup_Flag,
		NULL,
		CTFDrop_Flag, //Should this be null if we don't want players to drop it manually?
		NULL,
		NULL,
		"models/items/flags/flag2.md2", EF_TEAM2,
		NULL,
/* icon */		"i_flag2",
/* pickup */	"Blue Flag",
/* width */		2,
		0,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ "misc/blue_scores.wav misc/blue_takes.wav misc/blue_increases.wav misc/blue_wins.wav misc/blue_picked.wav"
	},

#ifndef ALTERIA
/*QUAKED item_bomber (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		"item_bomber",
		Get_in_vehicle,
		NULL,
		Leave_vehicle,
		Weapon_Bomber,
		NULL,
		"vehicles/bomber/tris.md2", 0,
		"vehicles/bomber/v_wep.md2",
/* icon */		"bomber",
/* pickup */	"Bomber",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_BOMBER,
		NULL,
		0,
		NULL
	},
/*QUAKED item_stafer (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		"item_strafer",
		Get_in_vehicle,
		NULL,
		Leave_vehicle,
		Weapon_Strafer,
		NULL,
		"vehicles/strafer/tris.md2", 0,
		"vehicles/strafer/v_wep.md2",
/* icon */		"strafer",
/* pickup */	"Strafer",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_STRAFER,
		NULL,
		0,
		NULL
	},

	/*QUAKED item_hover (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		"item_hover",
		Get_in_vehicle,
		NULL,
		Leave_vehicle,
		Weapon_Hover,
		NULL,
		"vehicles/hover/tris.md2", 0,
		"vehicles/hover/v_wep.md2",
/* icon */		"hover",
/* pickup */	"Hover",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_HOVER,
		NULL,
		0,
		NULL
	},

//Tactical bombs
	{
		"item_alien_bomb",
		NULL, //Pickup_alienBomb, //only owner will be able to pick this item back up
		Use_Weapon,
		NULL, 
		Weapon_TacticalBomb, //fire out bomb, not too far, like prox mines 
		NULL,
		"models/tactical/alien_bomb.iqm", 0,
		"vehicles/deathball/v_wep.md2", //will use db's vweap for bombs and detonators
/* icon */		"abomb", 
/* pickup */	"Alien Bomb",
		0,
		1,
		"bombs",
		IT_WEAPON,
		WEAP_ABOMB,
		NULL,
		0,
		NULL
	},

	{
		"item_human_bomb",
		NULL, //Pickup_humanBomb, //only owner will be able to pick this item back up
		Use_Weapon,
		NULL, 
		Weapon_TacticalBomb, //fire out bomb, not too far, like prox mines 
		NULL,
		"models/tactical/human_bomb.iqm", 0,
		"vehicles/deathball/v_wep.md2", //will use db's vweap for bombs and detonators
/* icon */		"abomb", 
/* pickup */	"Human Bomb",
		0,
		1,
		"bombs",
		IT_WEAPON,
		WEAP_HBOMB,
		NULL,
		0,
		NULL
	},

/*QUAKED item_deathball (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		"item_deathball",
		Pickup_deathball,
		NULL,
		DeathballDrop,
		Weapon_Deathball,
		NULL,
		"vehicles/deathball/deathball.md2", 0,
		"vehicles/deathball/v_wep.md2",
/* icon */		"grapple",
/* pickup */	"Deathball",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_DEATHBALL,
		NULL,
		0,
		NULL
	},

	//a fake item for bots to use as a target for throwing a deathball at.
	{
		"item_dbtarget",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		"models/objects/blank/tris.md2", 0,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL,
		IT_POWERUP,
		WEAP_DEATHBALL,
		NULL,
		0,
		NULL
	},
	{
		"item_red_dbtarget",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		"models/objects/blank/tris.md2", 0,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL,
		IT_POWERUP,
		WEAP_DEATHBALL,
		NULL,
		0,
		NULL
	},
	{
		"item_blue_dbtarget",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		"models/objects/blank/tris.md2", 0,
		NULL,
		NULL,
		NULL,
		0,
		0,
		NULL,
		IT_POWERUP,
		WEAP_DEATHBALL,
		NULL,
		0,
		NULL
	},
#endif
	//
	// WEAPONS
	//
/* weapon_grapple (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		"weapon_grapple",
		NULL,
		Use_Weapon,
		NULL,
		CTFWeapon_Grapple,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_machn/tris.md2",
/* icon */		"grapple",
/* pickup */	"Grapple",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_GRAPPLE,
		NULL,
		0,
/* precache */ "weapons/electroball.wav"
	},

#ifdef ALTERIA
	//note some of this is clearly temporary placeholder
	{
		"weapon_warrior_punch",
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Punch,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_warriorhands/tris.md2",
/* icon */		"warriorpunch",
/* pickup */	"Warriorpunch",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_VIOLATOR,
		NULL,
		0,
/* precache */ "weapons/viofire1.wav weapons/viofire2.wav"
	},

	{
		"weapon_wizard_punch",
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Punch,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_wizardhands/tris.md2",
/* icon */		"wizardpunch",
/* pickup */	"Wizardpunch",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_VIOLATOR,
		NULL,
		0,
/* precache */ "weapons/viofire1.wav weapons/viofire2.wav"
	},
#else
/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		"weapon_blaster",
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Blaster,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_blast/tris.md2",
/* icon */		"blaster",
/* pickup */	"Blaster",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_BLASTER,
		NULL,
		0,
/* precache */ "weapons/blastf1a.wav misc/lasfly.wav"
	},

	/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		"weapon_alienblaster",
		NULL,
		Use_Weapon,
		NULL,
		Weapon_AlienBlaster,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_alienblast/tris.md2",
/* icon */		"alienblaster",
/* pickup */	"Alien Blaster",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_BLASTER,
		NULL,
		0,
/* precache */ "weapons/blastf1a.wav misc/lasfly.wav" //to do tactical - change sound
	},

	{
		"weapon_violator",
		NULL,
		Use_Weapon,
		NULL,
		Weapon_Violator,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_violator/tris.md2",
/* icon */		"violator",
/* pickup */	"Violator",
		0,
		0,
		NULL,
		IT_WEAPON,
		WEAP_VIOLATOR,
		NULL,
		0,
/* precache */ "weapons/viofire1.wav weapons/viofire2.wav"
	},

/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_shotgun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Smartgun,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg/tris.md2", EF_ROTATE,
		"models/weapons/v_shotg/tris.md2",
/* icon */		"smartgun",
/* pickup */	"Alien Smartgun",
		0,
		1,
		"Alien Smart Grenade",
		IT_WEAPON,
		WEAP_SMARTGUN,
		NULL,
		0,
/* precache */ "weapons/clank.wav weapons/shotgf1b.wav weapons/smartgun_hum.wav"
	},

/*QUAKED weapon_supershotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_supershotgun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Chain,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg2/tris.md2", EF_ROTATE,
		"models/weapons/v_shotg2/tris.md2",
/* icon */		"chaingun",
/* pickup */	"Pulse Rifle",
		0,
		2,
		"Bullets",
		IT_WEAPON,
		WEAP_CHAINGUN,
		NULL,
		0,
/* precache */ "weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf4b.wav"
	},

/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_chaingun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Flame,
		"misc/w_pkup.wav",
		"models/weapons/g_chain/tris.md2", EF_ROTATE,
		"models/weapons/v_chain/tris.md2",
/* icon */		"flamethrower",
/* pickup */	"Flame Thrower",
		0,
		1,
		"Napalm",
		IT_WEAPON,
		WEAP_FLAMETHROWER,
		NULL,
		0,
/* precache */ "weapons/grenlb1b.wav weapons/grenlf1a.wav"
	},

/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_rocketlauncher",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_RocketLauncher,
		"misc/w_pkup.wav",
		"models/weapons/g_rocket/tris.md2", EF_ROTATE,
		"models/weapons/v_rocket/tris.md2",
/* icon */		"rocketlauncher",
/* pickup */	"Rocket Launcher",
		0,
		1,
		"Rockets",
		IT_WEAPON,
		WEAP_ROCKETLAUNCHER,
		NULL,
		0,
/* precache */ "models/objects/rocket/tris.md2 weapons/rockfly.wav weapons/rocklf1a.wav models/objects/debris2/tris.md2"
	},

/*QUAKED weapon_hyperblaster (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_hyperblaster",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Disruptor,
		"misc/w_pkup.wav",
		"models/weapons/g_hyperb/tris.md2", EF_ROTATE,
		"models/weapons/v_hyperb/tris.md2",
/* icon */		"disruptor",
/* pickup */	"Alien Disruptor",
		0,
		1,
		"Cells",
		IT_WEAPON,
		WEAP_DISRUPTOR,
		NULL,
		0,
/* precache */ "weapons/railgf1a.wav"
	},

/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_railgun",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Beamgun,
		"misc/w_pkup.wav",
		"models/weapons/g_rail/tris.md2", EF_ROTATE,
		"models/weapons/v_rail/tris.md2",
/* icon */		"beamgun",
/* pickup */	"Disruptor",
		0,
		1,
		"Cells",
		IT_WEAPON,
		WEAP_BEAMGUN,
		NULL,
		0,
/* precache */ "weapons/hyprbf1a.wav"
	},

/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_bfg",
		Pickup_Weapon,
		Use_Weapon,
		Drop_Weapon,
		Weapon_Vaporizer,
		"misc/w_pkup.wav",
		"models/weapons/g_bfg/tris.md2", EF_ROTATE,
		"models/weapons/v_bfg/tris.md2",
/* icon */		"vaporizor",
/* pickup */	"Alien Vaporizer",
		0,
		1,
		"Slugs",
		IT_WEAPON,
		WEAP_VAPORIZER,
		NULL,
		0,
/* precache */ "weapons/energyfield.wav smallmech/sight.wav weapons/vaporizer_hum.wav"
	},

	{
		"weapon_minderaser",
		Pickup_Weapon,
		Use_Weapon,
		NULL, //never drop
		Weapon_Minderaser,
		"misc/w_pkup.wav",
		"models/weapons/g_minderaser/tris.md2", EF_ROTATE,
		"models/weapons/v_minderaser/tris.md2",
/* icon */		"minderaser",
/* pickup */	"Minderaser",
		0,
		1,
		"Seekers", 
		IT_WEAPON,
		WEAP_MINDERASER,
		NULL,
		0,
/* precache */ "weapons/clank.wav weapons/minderaserfire.wav weapons/shotgf1b.wav weapons/smartgun_hum.wav"
	},
#endif
	//
	// AMMO ITEMS
	//
/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_shells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/shells/medium/tris.md2", 0,
		NULL,
/* icon */		"a_shells",
/* pickup */	"Alien Smart Grenade",
/* width */		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SHELLS,
/* precache */ ""
	},
/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_grenades",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/grenades/medium/tris.md2", 0,
		NULL,
/* icon */		"a_grenades",
/* pickup */	"Napalm",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_GRENADES,
/* precache */ ""
	},

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_bullets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/bullets/medium/tris.md2", 0,
		NULL,
/* icon */		"a_bullets",
/* pickup */	"Bullets",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_BULLETS,
/* precache */ ""
	},

/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_cells",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/cells/medium/tris.md2", 0,
		NULL,
/* icon */		"a_cells",
/* pickup */	"Cells",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_CELLS,
/* precache */ ""
	},

/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_rockets",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/rockets/medium/tris.md2", 0,
		NULL,
/* icon */		"a_rockets",
/* pickup */	"Rockets",
/* width */		3,
		5,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_ROCKETS,
/* precache */ ""
	},

/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_slugs",
		Pickup_Ammo,
		NULL,
		Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		NULL,
		0,
		NULL,
/* icon */		"a_slugs",
/* pickup */	"Slugs",
/* width */		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SLUGS,
/* precache */ ""
	},

	{
		"ammo_seekers",
		Pickup_Ammo, //set to null
		NULL,
		Drop_Ammo, //set to null
		NULL,
		"misc/am_pkup.wav",
		NULL,
		0,
		NULL,
/* icon */		"a_seekers",
/* pickup */	"Seekers",
/* width */		3,
		1,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SEEKERS,
/* precache */ ""
	},

	{
		"ammo_bombs",
		Pickup_Ammo, //set to null
		NULL,
		Drop_Ammo, //set to null
		NULL,
		"misc/am_pkup.wav",
		NULL,
		0,
		NULL,
/* icon */		"a_bombs",
/* pickup */	"Bombs",
/* width */		3,
		1,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_BOMBS,
/* precache */ ""
	},

	//
	// POWERUP ITEMS
	//
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_quad",
		Pickup_Powerup,
		Use_Quad,
		Drop_General,
		NULL,
		"items/powerup.wav",
		"models/items/quaddama/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_quad",
/* pickup */	"Double Damage", //now double damage, rather than quad
/* width */		2,
		150,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/damage.wav items/damage2.wav items/damage3.wav"
	},

/*QUAKED item_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_invulnerability",
		Pickup_Powerup,
		Use_Invulnerability,
		Drop_General,
		NULL,
		"items/powerup.wav",
		"models/items/invulner/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_invulnerability",
/* pickup */	"Alien Force", //now "Alien Force" - Damage reduced to 1/3, max armor added
/* width */		2,
		300,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/protect.wav items/protect2.wav items/protect4.wav"
	},

/*QUAKED item_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16)
gives +1 to maximum health
*/
	{
		"item_adrenaline",
		Pickup_Adrenaline,
		NULL,
		NULL,
		NULL,
		"items/n_health.wav",
		"models/items/adrenaline/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_adrenaline",
/* pickup */	"Adrenaline",
/* width */		2,
		60,
		NULL,
		IT_HEALTH,
		0,
		NULL,
		0,
/* precache */ ""
	},

	{
		NULL,
		Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/l_health.wav",
		NULL, 0,
		NULL,
/* icon */		"i_health",
/* pickup */	"Health",
/* width */		3,
		0,
		NULL,
		IT_HEALTH,
		0,
		NULL,
		0,
/* precache */ "items/s_health.wav items/n_health.wav items/l_health.wav items/m_health.wav"
	},

	{
		"item_haste",
		Pickup_Powerup,
		Use_Haste,
		Drop_General,
		NULL,
		"items/powerup.wav",
		"models/items/haste/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_haste",
/* pickup */	"Haste",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/hasteout.wav"
	},
	{
		"item_sproing",
		Pickup_Powerup,
		Use_Sproing,
		Drop_General,
		NULL,
		"items/powerup.wav",
		"models/items/sproing/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_sproing",
/* pickup */	"Sproing",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/sproingout.wav"
	},

	//these next two powerups are never placed in maps
	{
		"item_invisibility",
		Pickup_Powerup,
		Use_Invisibility,
		NULL,
		NULL,
		"items/powerup.wav",
		NULL,
		EF_ROTATE,
		NULL,
		NULL,
		"Invisibility",
		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/protect2.wav"
	},

	// end of list marker
	{NULL}
};


/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health (edict_t *self)
{
	if ( deathmatch->integer && (dmflags->integer & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}
	self->model = "models/items/healing/medium/tris.md2";
	self->classname = "Health";
	self->count = 10;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/n_health.wav");
}

/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_small (edict_t *self)
{
	if ( deathmatch->integer && (dmflags->integer & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/small/tris.md2";
	self->count = 5;
	self->classname = "Health";
	SpawnItem (self, FindItem ("Health"));
	self->style = HEALTH_IGNORE_MAX;
	gi.soundindex ("items/s_health.wav");
}

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_large (edict_t *self)
{
	if ( deathmatch->integer && (dmflags->integer & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/large/tris.md2";
	self->classname = "Health";
	self->count = 25;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/l_health.wav");
}

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_mega (edict_t *self)
{
	if ( deathmatch->integer && (dmflags->integer & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/mega_h/tris.md2";
	self->count = 100;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/m_health.wav");
	self->style = HEALTH_IGNORE_MAX|HEALTH_TIMED;
}


void InitItems (void)
{
	game.num_items = sizeof(itemlist)/sizeof(itemlist[0]) - 1;
}



/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames (void)
{
	int		i;
	gitem_t	*it;

	for (i=0 ; i<game.num_items ; i++)
	{
		it = &itemlist[i];
		gi.configstring (CS_ITEMS+i, it->pickup_name);
	}

	jacket_armor_index = ITEM_INDEX(FindItem("Jacket Armor"));
	combat_armor_index = ITEM_INDEX(FindItem("Combat Armor"));
	body_armor_index   = ITEM_INDEX(FindItem("Body Armor"));
}
