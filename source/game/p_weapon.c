// g_weapon.c

#include "g_local.h"
#include "m_player.h"


static qboolean	is_quad;
static byte		is_silenced;
static qboolean altfire;

float damage_buildup = 1.0;

void P_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	vec3_t	_distance;

	VectorCopy (distance, _distance);
	if (client->pers.hand == LEFT_HANDED)
		_distance[1] *= -1;
	else if (client->pers.hand == CENTER_HANDED)
		_distance[1] = 0;
	G_ProjectSource (point, _distance, forward, right, result);
}


/*
===============
PlayerNoise

Each player can have two noise objects associated with it:
a personal noise (jumping, pain, weapon firing), and a weapon
target noise (bullet wall impacts)

Monsters that don't directly see the player can move
to a noise in hopes of seeing the player from there.
===============
*/
void PlayerNoise(edict_t *who, vec3_t where, int type)
{
	edict_t		*noise;

	if (type == PNOISE_WEAPON)
	{
		if (who->client->silencer_shots)
		{
			who->client->silencer_shots--;
			return;
		}
	}

	if (deathmatch->value)
		return;

	if (who->flags & FL_NOTARGET)
		return;


	if (!who->mynoise)
	{
		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet (noise->mins, -8, -8, -8);
		VectorSet (noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise = noise;

		noise = G_Spawn();
		noise->classname = "player_noise";
		VectorSet (noise->mins, -8, -8, -8);
		VectorSet (noise->maxs, 8, 8, 8);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise2 = noise;
	}

	if (type == PNOISE_SELF || type == PNOISE_WEAPON)
	{
		noise = who->mynoise;
		level.sound_entity = noise;
		level.sound_entity_framenum = level.framenum;
	}
	else // type == PNOISE_IMPACT
	{
		noise = who->mynoise2;
		level.sound2_entity = noise;
		level.sound2_entity_framenum = level.framenum;
	}

	VectorCopy (where, noise->s.origin);
	VectorSubtract (where, noise->maxs, noise->absmin);
	VectorAdd (where, noise->maxs, noise->absmax);
	noise->teleport_time = level.time;
	gi.linkentity (noise);
}


qboolean Pickup_Weapon (edict_t *ent, edict_t *other)
{
	int		index;
	gitem_t		*ammo;

	if (other->in_vehicle) {
		return false;
	}

	index = ITEM_INDEX(ent->item);

	//mutators
	if(instagib->value || rocket_arena->value)
		return false; //why pick them up in these modes?

	if ( ( ((int)(dmflags->value) & DF_WEAPONS_STAY))
		&& other->client->pers.inventory[index])
	{
		if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM) ) )
			return false;	// leave the weapon for others to pickup
	}

	other->client->pers.inventory[index]++;


	if (!(ent->spawnflags & DROPPED_ITEM) )
	{
		// give them some ammo with it
		ammo = FindItem (ent->item->ammo);
		if ( (int)dmflags->value & DF_INFINITE_AMMO )
			Add_Ammo (other, ammo, 1000, true, true);
		else if (ent->spawnflags & DROPPED_PLAYER_ITEM)
			Add_Ammo (other, ammo, ammo->quantity, true, true); //DROPPED WEAPON give full ammo
		else
			Add_Ammo (other, ammo, ammo->quantity, true, false);

		if (! (ent->spawnflags & DROPPED_PLAYER_ITEM) )
		{
			if (deathmatch->value)
			{
				if ((int)(dmflags->value) & DF_WEAPONS_STAY)
					ent->flags |= FL_RESPAWN;
				else {
					//weapon = FindItem (ent->item->weapon);
					if(ent->item->weapmodel == WEAP_VAPORIZER)
						SetRespawn (ent, 10);
					else
						SetRespawn (ent, 5); 
				}
			}
		}
	}

	if (other->client->pers.weapon != ent->item &&
		(other->client->pers.inventory[index] == 1) &&
		( !deathmatch->value || other->client->pers.weapon == FindItem("blaster") ) )
		other->client->newweapon = ent->item;

	return true;
}

/*
===========
FS_FOpenFile

Finds the file in the search path.
returns filesize and an open FILE *
Used for streaming data out of either a pak file or
a seperate file.
===========
*/

int Q2_FindFile (char *filename, FILE **file)
{
	char	name[MAX_OSPATH];
	cvar_t	*game;
	qboolean found = false;

	game = gi.cvar("game", "", 0);

	if (!*game->string) //if there is a gamedir try here first
		sprintf (name, "%s/%s", GAMEVERSION, filename);
	else
		sprintf (name, "%s/%s", game->string, filename);

	*file = fopen (name, "rb");
	if (!*file) {
		*file = NULL;
		found = false;
	}
	else
		return 1;

	if(!found) { //try basedir
		sprintf (name, "%s/%s", GAMEVERSION, filename);
		*file = fopen (name, "rb");
		if (!*file) {
			*file = NULL;
			return -1;
		}
		else
			return 1;
	}
	else
		return -1;
}

/*
===============
ChangeWeapon

The old weapon has been dropped all the way, so make the new one
current
===============
*/
void ChangeWeapon (edict_t *ent)
{
	char    *info;
	char	weaponame[64] = " ";
	char	weaponmodel[MAX_OSPATH] = " ";
	int i;
	int done;
	char	weaponpath[MAX_OSPATH] = " ";
	FILE *file;

	ent->client->pers.lastweapon = ent->client->pers.weapon;
	ent->client->pers.weapon = ent->client->newweapon;
	ent->client->newweapon = NULL;
	ent->client->machinegun_shots = 0;

	// set visible model
	if (ent->s.modelindex == 255) {
		if (ent->client->pers.weapon)
			i = ((ent->client->pers.weapon->weapmodel & 0xff) << 8);
		else
			i = 0;
		ent->s.skinnum = (ent - g_edicts - 1) | i;
	}

	if (ent->client->pers.weapon && ent->client->pers.weapon->ammo)
		ent->client->ammo_index = ITEM_INDEX(FindItem(ent->client->pers.weapon->ammo));
	else
		ent->client->ammo_index = 0;

	if (!ent->client->pers.weapon)
	{	// dead
		ent->client->ps.gunindex = 0;
		return;
	}

	ent->client->weaponstate = WEAPON_ACTIVATING;
	ent->client->ps.gunframe = 0;
	ent->client->ps.gunindex = gi.modelindex(ent->client->pers.weapon->view_model);

	if (ent->in_vehicle) {
		return;
	}

	//set up code to set player world weapon model, as well as some hacks :(

	info = Info_ValueForKey (ent->client->pers.userinfo, "skin");

	i = 0;
	done = 0;
	strcpy(weaponame, " ");
	weaponame[0] = 0;
	while(!done)
	{
		if((info[i] == '/') || (info[i] == '\\'))
			done = 1;
		weaponame[i] = info[i];
		if(i > 63)
			done = 1;
		i++;
	}
	strcpy(weaponmodel, " ");
	weaponmodel[0] = 0;

	sprintf(weaponmodel, "players/%s%s", weaponame, "weapon.md2"); //default

	if(ent->client->pers.weapon->view_model == "models/weapons/v_violator/tris.md2")
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_violator.md2");
	if(ent->client->pers.weapon->view_model == "models/weapons/v_rocket/tris.md2")
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_rlauncher.md2");
	if(ent->client->pers.weapon->view_model == "models/weapons/v_blast/tris.md2")
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_blaster.md2");
	if(ent->client->pers.weapon->view_model == "models/weapons/v_bfg/tris.md2")
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_bfg.md2");
	if(ent->client->pers.weapon->view_model == "models/weapons/v_rail/tris.md2")
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_railgun.md2");
	if(ent->client->pers.weapon->view_model == "models/weapons/v_shotg2/tris.md2")
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_sshotgun.md2");
	if(ent->client->pers.weapon->view_model == "models/weapons/v_shotg/tris.md2")
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_shotgun.md2");
	if(ent->client->pers.weapon->view_model == "models/weapons/v_hyperb/tris.md2")
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_hyperblaster.md2");
	if(ent->client->pers.weapon->view_model == "models/weapons/v_chain/tris.md2")
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_chaingun.md2");
	if(ent->client->pers.weapon->view_model == "vehicles/deathball/v_wep.md2")
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_machinegun.md2");


	sprintf(weaponpath, "%s", weaponmodel);
	Q2_FindFile (weaponpath, &file); //does it really exist?
	if(!file) {
		sprintf(weaponpath, "%s", weaponame, "weapon.md2"); //no w_weaps, do we have this model?
		Q2_FindFile (weaponpath, &file);
		if(!file) //server does not have this player model
			sprintf(weaponmodel, "players/martianenforcer/weapon.md2");//default player(martian)
		else { //have the model, but it has no w_weaps
			sprintf(weaponmodel, "players/%s%s", weaponame, "weapon.md2"); //custom weapon
			fclose(file);
		}
	}
	else
		fclose(file);
	ent->s.modelindex2 = gi.modelindex(weaponmodel);

	//play a sound like in Q3, except for blaster, so it doesn't do it on spawn.
	if(!(ent->client->pers.weapon->view_model == "models/weapons/v_blast/tris.md2"))
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/whoosh.wav"), 1, ATTN_NORM, 0);

	ent->client->anim_priority = ANIM_PAIN;
	if(ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
			ent->s.frame = FRAME_crpain1;
			ent->client->anim_end = FRAME_crpain4;
	}
	else
	{
			ent->s.frame = FRAME_pain301;
			ent->client->anim_end = FRAME_pain304;

	}

}

/*
=================
NoAmmoWeaponChange
=================
*/
void NoAmmoWeaponChange (edict_t *ent)
{
	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("cells"))]
		&& ent->client->pers.inventory[ITEM_INDEX(FindItem("Disruptor"))] )
	{
		ent->client->newweapon = FindItem ("Disruptor");
		return;
	}

	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("rockets"))]
		&& ent->client->pers.inventory[ITEM_INDEX(FindItem("Rocket Launcher"))] )
	{
		ent->client->newweapon = FindItem ("Rocket Launcher");
		return;
	}

	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("napalm"))]
		&& ent->client->pers.inventory[ITEM_INDEX(FindItem("Flame Thrower"))] )
	{
		ent->client->newweapon = FindItem ("Flame Thrower");
		return;
	}

	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("bullets"))] > 1
		&& ent->client->pers.inventory[ITEM_INDEX(FindItem("Pulse Rifle"))] )
	{
		ent->client->newweapon = FindItem ("Pulse Rifle");
		return;
	}

	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("alien smart grenade"))]
		&& ent->client->pers.inventory[ITEM_INDEX(FindItem("Alien Smartgun"))] )
	{
		ent->client->newweapon = FindItem ("Alien Smartgun");
		return;
	}

	if ( ent->client->pers.inventory[ITEM_INDEX(FindItem("cells"))]
		&& ent->client->pers.inventory[ITEM_INDEX(FindItem("Alien Disruptor"))] )
	{
		ent->client->newweapon = FindItem ("Alien Disruptor");
		return;
	}

	ent->client->newweapon = FindItem ("blaster");
}

/*
=================
Think_Weapon

Called by ClientBeginServerFrame and ClientThink
=================
*/
void Think_Weapon (edict_t *ent)
{
	// if just died, put the weapon away
	if (ent->health < 1)
	{
		ent->client->newweapon = NULL;
		ChangeWeapon (ent);
	}

	// call active weapon think routine
	if (ent->client->pers.weapon && ent->client->pers.weapon->weaponthink)
	{
		is_quad = (ent->client->quad_framenum > level.framenum);
		if (ent->client->silencer_shots)
			is_silenced = MZ_SILENCED;
		else
			is_silenced = 0;
		ent->client->pers.weapon->weaponthink (ent);
	}
}


/*
================
Use_Weapon

Make the weapon ready if there is ammo
================
*/
void Use_Weapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;

	if (ent->in_vehicle || ent->in_deathball) {
		return;
	}

	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return;

	if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO))
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);

		if (!ent->client->pers.inventory[ammo_index])
		{
			safe_cprintf (ent, PRINT_HIGH, "No %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}

		if (ent->client->pers.inventory[ammo_index] < item->quantity)
		{
			safe_cprintf (ent, PRINT_HIGH, "Not enough %s for %s.\n", ammo_item->pickup_name, item->pickup_name);
			return;
		}
	}

	// change to this weapon when down
	ent->client->newweapon = item;
}



/*
================
Drop_Weapon
================
*/
void Drop_Weapon (edict_t *ent, gitem_t *item)
{
	int		index;

	if (((int)(dmflags->value) & DF_WEAPONS_STAY) || instagib->value || rocket_arena->value)
		return;

	index = ITEM_INDEX(item);
	// see if we're already using it
	if ( ((item == ent->client->pers.weapon) || (item == ent->client->newweapon))&& (ent->client->pers.inventory[index] == 1) )
	{
		safe_cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	Drop_Item (ent, item);
	ent->client->pers.inventory[index]--;
}


/*
================
Weapon_Generic

A generic function to handle the basics of weapon thinking
================
*/
#define FRAME_FIRE_FIRST		(FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST		(FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST	(FRAME_IDLE_LAST + 1)

void Weapon_Generic (edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int *pause_frames, int *fire_frames, void (*fire)(edict_t *ent))
{
	int		n;

	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if(excessive->value || quickweap->value) {
			ChangeWeapon (ent);
			return;
		}
		else if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ChangeWeapon (ent);
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if(excessive->value || quickweap->value) {
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}
		else if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST-2)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING))
	{
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST+2;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK) )
		{
			ent->client->spawnprotected = false;

			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if ((!ent->client->ammo_index) ||
				( ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity))
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;

				// start the animation
				if(!ent->client->anim_run) { //looks better than skating, eh?
					ent->client->anim_priority = ANIM_ATTACK;
					if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
					{
						ent->s.frame = FRAME_crattak1-1;
						ent->client->anim_end = FRAME_crattak9;
					}
					else
					{
						ent->s.frame = FRAME_attack1-1;
						ent->client->anim_end = FRAME_attack8;
					}
				}
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
		}
		//alt fire
		else if ( ((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK2) )
		{
			ent->client->spawnprotected = false;

			ent->client->latched_buttons &= ~BUTTON_ATTACK2;
			if ((!ent->client->ammo_index) ||
				( ent->client->pers.inventory[ent->client->ammo_index] >= ent->client->pers.weapon->quantity))
			{
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;

				// start the animation
				if(!ent->client->anim_run) { //looks better than skating, eh?
					ent->client->anim_priority = ANIM_ATTACK;
					if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
					{
						ent->s.frame = FRAME_crattak1-1;
						ent->client->anim_end = FRAME_crattak9;
					}
					else
					{
						ent->s.frame = FRAME_attack1-1;
						ent->client->anim_end = FRAME_attack8;
					}
				}
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
		}
		else
		{
			if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
			{
				ent->client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (pause_frames)
			{
				for (n = 0; pause_frames[n]; n++)
				{
					if (ent->client->ps.gunframe == pause_frames[n])
					{
						if (rand()&15)
							return;
					}
				}
			}

			ent->client->ps.gunframe++;
			return;
		}
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
		for (n = 0; fire_frames[n]; n++)
		{
			if (ent->client->ps.gunframe == fire_frames[n])
			{
				if (ent->client->quad_framenum > level.framenum)
					gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1, ATTN_NORM, 0);

				fire (ent);
				break;
			}
		}

		if (!fire_frames[n])
			ent->client->ps.gunframe++;

		if (ent->client->ps.gunframe == FRAME_IDLE_FIRST+1)
			ent->client->weaponstate = WEAPON_READY;
	}
}

void weapon_plasma_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;

	int		damage;
	int		kick;
	int		buildup;

	if(instagib->value) {
		damage = 200;
		kick = 200;
	} else {
		damage = 60;
		kick = 60;
	}

	if (is_quad)
	{
		damage *= 2;
		kick *= 2;
	}

	//alt fire
	if (ent->client->buttons & BUTTON_ATTACK2) {
		ent->client->ps.fov = 20;
		buildup = damage_buildup; //int, for the pic flag
		ent->client->ps.stats[STAT_ZOOMED] = buildup;
		damage_buildup += 0.1; //the longer you hold the key, the stronger the blast
		if(damage_buildup > 3.0)
			damage_buildup = 3.0;
		if(damage_buildup < 3.0) //play a sound
			gi.sound(ent, CHAN_AUTO, gi.soundindex("world/laser1.wav"), 1, ATTN_NORM, 0);
		return;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 32, 5,  ent->viewheight-5);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_plasma (ent, start, forward, damage*damage_buildup, kick);

	//alt fire - reset some things
	ent->client->ps.fov = atoi(Info_ValueForKey(ent->client->pers.userinfo, "fov")); //alt fire - reset the fov;
	ent->client->ps.stats[STAT_ZOOMED] = 0;
	damage_buildup = 1.0;

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_RAILGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if ((! ( (int)dmflags->value & DF_INFINITE_AMMO ) ) && !instagib->value )
		ent->client->pers.inventory[ent->client->ammo_index]= ent->client->pers.inventory[ent->client->ammo_index]-2;
}

void Weapon_Disruptor (edict_t *ent)
{
	static int	pause_frames[]	= {42, 0};
	static int	fire_frames[]	= {5, 0};
	static int	excessive_fire_frames[] = {5,7,9,11,0};

	if(excessive->value)
		Weapon_Generic (ent, 4, 12, 42, 46, pause_frames, excessive_fire_frames, weapon_plasma_fire);
	else
		Weapon_Generic (ent, 4, 12, 42, 46, pause_frames, fire_frames, weapon_plasma_fire);
}

void weapon_energy_field_fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int		damage = 100;
	int		radius_damage = 100;
	int		damage_radius = 150;
	int		kick = 200;

	if (is_quad)
	{
		radius_damage *=2; 
		damage *= 2;
		kick *= 4;
	}

	if(ent->client->buttons & BUTTON_ATTACK2)
		ent->altfire = true;
	else if(ent->client->buttons & BUTTON_ATTACK) {
		ent->altfire = false;
		if (ent->client->pers.inventory[ent->client->ammo_index] < 2) {
			ent->client->ps.gunframe = 19;
			NoAmmoWeaponChange(ent);
		}
	}

	if(ent->client->ps.gunframe == 7)
		gi.sound(ent, CHAN_AUTO, gi.soundindex("smallmech/sight.wav"), 1, ATTN_NORM, 0);

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 32, 5,  ent->viewheight-5);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if(ent->client->ps.gunframe == 12) {

		VectorAdd(start, forward, start);
		start[2]+=6;
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLUE_MUZZLEFLASH);
		gi.WritePosition (start);
		gi.multicast (start, MULTICAST_PVS);
	}

	if(ent->client->ps.gunframe == 13) {
		if(ent->altfire) {//alt fire
			AngleVectors (ent->client->v_angle, forward, right, NULL);
			VectorScale (forward, -2, ent->client->kick_origin);
			ent->client->kick_angles[0] = -1;
			VectorSet(offset, 32, 5, ent->viewheight-4);
			P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
			forward[0] = forward[0] * 4.6;
			forward[1] = forward[1] * 4.6;
			forward[2] = forward[2] * 4.6;
			fire_bomb (ent, start, forward, damage, 250, damage_radius, radius_damage, 8);
			if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
				ent->client->pers.inventory[ent->client->ammo_index]= ent->client->pers.inventory[ent->client->ammo_index]-1;

		}
		else {
				fire_energy_field (ent, start, forward, damage, kick);
				if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
					ent->client->pers.inventory[ent->client->ammo_index]= ent->client->pers.inventory[ent->client->ammo_index]-2;
		}

		// send muzzle flash
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_RAILGUN | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		VectorAdd(start, forward, start);
		start[2]+=6;
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLUE_MUZZLEFLASH);
		gi.WritePosition (start);
		gi.multicast (start, MULTICAST_PVS);


		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/energyfield.wav"), 1, ATTN_NORM, 0);
		ent->client->weapon_sound = 0;

	}
	ent->client->ps.gunframe++;

}
void Weapon_Vaporizer (edict_t *ent)
{
	static int	pause_frames[]	= {48, 0};
	static int	fire_frames[]	= {6, 7, 12, 13, 0};

	Weapon_Generic (ent, 5, 18, 48, 52, pause_frames, fire_frames, weapon_energy_field_fire);
}

/*
======================================================================

Flame Thrower

======================================================================
*/

void weapon_flamethrower_fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int	damage = 25;
	float	damage_radius = 200;

	if((ent->client->buttons & BUTTON_ATTACK2) && ent->client->ps.gunframe == 6) { //shoot a fireball

		AngleVectors (ent->client->v_angle, forward, right, NULL);

		VectorSet(offset, 8, 8, ent->viewheight-8);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

		fire_fireball (ent, start, forward, damage, 1500, damage_radius, 100);

		// send muzzle flash
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_GRENADE | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		ent->client->ps.gunframe++;

		PlayerNoise(ent, start, PNOISE_WEAPON);

		if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) ) {
			ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity*10;
			if(ent->client->pers.inventory[ent->client->ammo_index] < 0)
				ent->client->pers.inventory[ent->client->ammo_index] = 0;
		}

		return;
	}

	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		ent->client->ps.gunframe = 17;
		return;
	}
	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_GRENADE | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (is_quad) 
		damage *= 2;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	fire_flamethrower (ent, start, forward, damage, 500, damage_radius);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) ) {
		ent->client->pers.inventory[ent->client->ammo_index] -= ent->client->pers.weapon->quantity;
		if(ent->client->pers.inventory[ent->client->ammo_index] < 0)
			ent->client->pers.inventory[ent->client->ammo_index] = 0;
	}

}

void Weapon_Flame (edict_t *ent)
{
	static int	pause_frames[]	= {36, 0};
	static int	fire_frames[]	= {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0};

	Weapon_Generic (ent, 5, 16, 36, 40, pause_frames, fire_frames, weapon_flamethrower_fire);
}

/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius;
	int		radius_damage;

	damage = 100 + (int)(random() * 20.0);
	radius_damage = 120;
	damage_radius = 120;
	if (is_quad)
	{
		damage *= 2; 
		radius_damage *= 2;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, 2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	VectorSet(offset, 4, 4, ent->viewheight-2);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if(ent->client->buttons & BUTTON_ATTACK2) //alt fire
	{
		if(excessive->value) //no homers in excessive!
			fire_rocket (ent, start, forward, damage, 900, damage_radius, radius_damage);
		else
			fire_homingrocket (ent, start, forward, damage, 250, damage_radius, radius_damage);
	}
	else
		fire_rocket (ent, start, forward, damage, 900, damage_radius, radius_damage);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_ROCKET | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if ((! ( (int)dmflags->value & DF_INFINITE_AMMO ) ) && !rocket_arena->value)
		ent->client->pers.inventory[ent->client->ammo_index]--;
}

void Weapon_RocketLauncher (edict_t *ent)
{
	static int	pause_frames[]	= {52, 0};
	static int	fire_frames[]	= {6, 0};
	static int	excessive_fire_frames[]	= {5,7,9,11,13, 0};

	if(excessive->value)
		Weapon_Generic (ent, 5, 14, 52, 56, pause_frames, excessive_fire_frames, Weapon_RocketLauncher_Fire);
	else
		Weapon_Generic (ent, 5, 14, 52, 56, pause_frames, fire_frames, Weapon_RocketLauncher_Fire);
}

/*
======================================================================

BLASTER

======================================================================
*/

void Blaster_Fire (edict_t *ent, vec3_t g_offset, int damage, qboolean hyper, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	if (is_quad)
		damage *= 2; 

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	if(!hyper) {
		VectorScale (forward, -3, ent->client->kick_origin);
		ent->client->kick_angles[0] = -3;
	}

	if(hyper && (ent->client->buttons & BUTTON_ATTACK))
		VectorSet(offset, 32, 6, ent->viewheight-8);
	else if(hyper && (ent->client->buttons & BUTTON_ATTACK2))
		VectorSet(offset, 32, 6, ent->viewheight-10);
	else
		VectorSet(offset, 30, 6, ent->viewheight-5);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if(hyper) {
		if(ent->client->buttons & BUTTON_ATTACK2) {//alt fire
			ent->altfire = !ent->altfire;
			if(ent->altfire) {
				gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/blastf1a.wav"), 1, ATTN_NORM, 0);
				fire_blasterball (ent, start, forward, damage*3, 1000, effect, hyper);
			}
		}
		else {
			gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1, ATTN_NORM, 0);
			fire_blaster (ent, start, forward, damage, 2800, effect, hyper);
		}
	}
	else {
		if(ent->client->buttons & BUTTON_ATTACK2) { //alt fire
			fire_blaster_beam (ent, start, forward, (int)((float)damage/1.4), 0, false);
			gi.sound(ent, CHAN_AUTO, gi.soundindex("vehicles/shootlaser.wav"), 1, ATTN_NORM, 0);
		}
		else
			fire_blasterball (ent, start, forward, damage, 1200, effect, hyper);
	}
	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	if (hyper)
		gi.WriteByte (MZ_HYPERBLASTER | is_silenced);
	else if (ent->client->buttons & BUTTON_ATTACK2)
		gi.WriteByte (MZ_RAILGUN | is_silenced);
	else
		gi.WriteByte (MZ_BLASTER | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
	PlayerNoise(ent, start, PNOISE_WEAPON);
	
	//create visual muzzle flash sprite!
	if(!hyper || (ent->client->buttons & BUTTON_ATTACK2))
	{
		VectorAdd(start, forward, start);
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLUE_MUZZLEFLASH);
		gi.WritePosition (start);
		gi.multicast (start, MULTICAST_PVS);
	}
}


void Weapon_Blaster_Fire (edict_t *ent)
{
	int		damage;

	damage = 30;

	Blaster_Fire (ent, vec3_origin, damage, false, EF_BLASTER);
	ent->client->ps.gunframe++;
}

void Weapon_Blaster (edict_t *ent)
{
	static int	pause_frames[]	= {52, 0};
	static int	fire_frames[]	= {5,0};
	static int  excessive_fire_frames[] = {5,6,7,8,0};

	if(excessive->value)
		Weapon_Generic (ent, 4, 8, 52, 55, pause_frames, excessive_fire_frames, Weapon_Blaster_Fire);
	else
		Weapon_Generic (ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Blaster_Fire);
}

//vehicles
void Weapon_Bomber_Fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius;
	int		radius_damage;

	damage = 150;
	radius_damage = 175;
	damage_radius = 250;
	if (is_quad)
	{
		damage *= 2; 
		radius_damage *= 2;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	VectorSet(offset, 8, 8, ent->viewheight-4);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	
	if(ent->client->buttons & BUTTON_ATTACK2 && ent->client->ps.gunframe != 12) {
		fire_rocket (ent, start, forward, damage/3, 1400, damage_radius/2, radius_damage/2);
		gi.sound (ent, CHAN_WEAPON, gi.soundindex("weapons/rocklr1b.wav"), 1, ATTN_NORM, 0);
		ent->client->ps.gunframe = 12;
	}
	else if(ent->client->ps.gunframe != 6){
		forward[0] = forward[0] * 2.6;
		forward[1] = forward[1] * 2.6;

		fire_bomb (ent, start, forward, damage, 250, damage_radius, radius_damage, 8);
		gi.sound (ent, CHAN_WEAPON, gi.soundindex("vehicles/shootbomb.wav"), 1, ATTN_NORM, 0);
	}

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_BFG | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	//might want to work with this some
	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;
}
void Weapon_Bomber (edict_t *ent)
{
	static int	pause_frames[]	= {30, 0};
	static int	fire_frames[]	= {6,12,0};
	static int	excessive_fire_frames[]	= {6,8,10,12,0};

	if(excessive->value)
		Weapon_Generic (ent, 5, 16, 39, 45, pause_frames, excessive_fire_frames, Weapon_Bomber_Fire);
	else
		Weapon_Generic (ent, 5, 16, 39, 45, pause_frames, fire_frames, Weapon_Bomber_Fire);
}
void Weapon_Strafer_Fire (edict_t *ent)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;
	int damage;
	float	damage_radius;
	int		radius_damage;

	radius_damage = 100;
	damage_radius = 100;

	if(excessive->value)
		damage = 60;
	else
		damage = 20;

	if (is_quad)
		damage *= 2; 

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 40, 6, ent->viewheight-5);

	right[0] = right[0] * 5;
	right[1] = right[1] * 5;

	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if(ent->client->buttons & BUTTON_ATTACK2) 
		fire_rocket (ent, start, forward, damage, 1200, damage_radius, radius_damage);
	else
		fire_blaster_beam (ent, start, forward, damage, 0, true);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_BFG | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	VectorAdd(start, forward, start);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BLUE_MUZZLEFLASH);
	gi.WritePosition (start);
	gi.multicast (start, MULTICAST_PVS);

	//now do the other side

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 40, 6, ent->viewheight-5);

	right[0] = right[0] * -5;
	right[1] = right[1] * -5;

	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if(ent->client->buttons & BUTTON_ATTACK2) {
		fire_rocket (ent, start, forward, damage, 1200, damage_radius, radius_damage);
		gi.sound (ent, CHAN_WEAPON, gi.soundindex("weapons/rocklr1b.wav"), 1, ATTN_NORM, 0);
	}
	else {		
		fire_blaster_beam (ent, start, forward, damage, 0, true);
		gi.sound (ent, CHAN_WEAPON, gi.soundindex("vehicles/shootlaser.wav"), 1, ATTN_NORM, 0);
	}

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_BFG | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	VectorAdd(start, forward, start);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BLUE_MUZZLEFLASH);
	gi.WritePosition (start);
	gi.multicast (start, MULTICAST_PVS);

	ent->client->ps.gunframe++;
}
void Weapon_Strafer (edict_t *ent) //for now
{
	static int	pause_frames[]	= {30, 0};
	static int	fire_frames[]	= {6,0};
	static int	excessive_fire_frames[]	= {6,8,10,0};

	if(excessive->value)
		Weapon_Generic (ent, 5, 11, 33, 39, pause_frames, excessive_fire_frames, Weapon_Strafer_Fire);
	else
		Weapon_Generic (ent, 5, 11, 33, 39, pause_frames, fire_frames, Weapon_Strafer_Fire);
}
void Weapon_Hover_Fire (edict_t *ent)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;
	int damage;

	if(excessive->value)
		damage = 200;
	else
		damage = 20;

	if (is_quad)
		damage *= 2; 

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 40, 0, ent->viewheight-5);

	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	if(ent->client->buttons & BUTTON_ATTACK2) {
		fire_blasterball (ent, start, forward, damage*6, 2200, EF_ROCKET, false);
		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/hypbrl1a.wav"), 1, ATTN_NORM, 0);
	}
	else if(ent->client->ps.gunframe == 6) {
		fire_hover_beam (ent, start, forward, damage, 0, true);
		gi.sound (ent, CHAN_WEAPON, gi.soundindex("weapons/biglaser.wav"), 1, ATTN_NORM, 0);
		
		VectorAdd(start, forward, start);
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_CHAINGUNSMOKE);
		gi.WritePosition (start);
		gi.multicast (start, MULTICAST_PVS);
	}

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_BFG | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
}
void Weapon_Hover (edict_t *ent) //for now
{
	static int	pause_frames[]	= {30, 0};
	static int	fire_frames[]	= {6,8,10,0};
	static int	excessive_fire_frames[]	= {6,8,10,0};

	if(excessive->value)
		Weapon_Generic (ent, 5, 11, 33, 39, pause_frames, excessive_fire_frames, Weapon_Hover_Fire);
	else
		Weapon_Generic (ent, 5, 11, 33, 39, pause_frames, fire_frames, Weapon_Hover_Fire);
}
void Weapon_Beamgun_Fire (edict_t *ent)
{
	vec3_t	offset;
	int		effect;
	int		damage;

	if (!((ent->client->buttons & BUTTON_ATTACK) || (ent->client->buttons & BUTTON_ATTACK2)))
	{
		ent->client->ps.gunframe = 25;
	}
	else
	{
		if (! ent->client->pers.inventory[ent->client->ammo_index] )
		{
			if (level.time >= ent->pain_debounce_time)
			{
				gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
				ent->pain_debounce_time = level.time + 1;
			}
			NoAmmoWeaponChange (ent);
		}
		else
		{
			offset[0] = 0;
			offset[1] = 0;
			offset[2] = 3;

			if ((ent->client->ps.gunframe == 6) || (ent->client->ps.gunframe == 9))
				effect = EF_HYPERBLASTER;
			else
				effect = 0;
		
			if(excessive->value)
				damage = 25;
			else
				damage = 10; 
					
			Blaster_Fire (ent, offset, damage, true, effect);
			if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
				ent->client->pers.inventory[ent->client->ammo_index]--;
		}

		ent->client->ps.gunframe++;
		if (ent->client->ps.gunframe == 24 && ent->client->pers.inventory[ent->client->ammo_index])
			ent->client->ps.gunframe = 6;
	}
}

void Weapon_Beamgun (edict_t *ent)
{
	static int	pause_frames[]	= {53, 0};
	static int	fire_frames[]	= {6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 24, 0};

	Weapon_Generic (ent, 5, 24, 53, 57, pause_frames, fire_frames, Weapon_Beamgun_Fire);
}

/*
======================================================================

MACHINEGUN / CHAINGUN

======================================================================
*/

void Machinegun_Fire (edict_t *ent)
{
	int			i;
	int			shots;
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;
	int			damage;
	int			kick = 2;

	if(excessive->value)
		damage = 60;
	else
		damage = 18;

	if ((ent->client->ps.gunframe == 5) && !(ent->client->buttons & BUTTON_ATTACK || ent->client->buttons & BUTTON_ATTACK2))
	{
		ent->client->ps.gunframe = 14;
		ent->client->weapon_sound = 0;
		return;
	}
	else if ((ent->client->ps.gunframe == 13) && (ent->client->buttons & BUTTON_ATTACK || ent->client->buttons & BUTTON_ATTACK2)
		&& ent->client->pers.inventory[ent->client->ammo_index])
	{
		ent->client->ps.gunframe = 5;
	}
	else if (ent->client->buttons & BUTTON_ATTACK2 && ent->client->ps.gunframe > 6)
	{
		if(ent->client->ps.gunframe == 7 || ent->client->ps.gunframe == 12) {
			ent->client->ps.gunframe = 14;
			return;
		}
		ent->altfire = true;
		ent->client->ps.gunframe = 14;
	}
	else if (ent->client->buttons & BUTTON_ATTACK2)
	{
		ent->client->ps.gunframe++;
		ent->altfire = true;
	}
	else if (ent->client->buttons & BUTTON_ATTACK){
		ent->client->ps.gunframe++;
		ent->altfire = false;
	}
	else
		ent->client->ps.gunframe++;

	shots = 1;

	if (ent->client->pers.inventory[ent->client->ammo_index] < 0)
		ent->client->pers.inventory[ent->client->ammo_index]= 0;

	if (ent->client->pers.inventory[ent->client->ammo_index] < shots)
		shots = ent->client->pers.inventory[ent->client->ammo_index];

	if (!shots)
	{
		if (level.time >= ent->pain_debounce_time)
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		return;
	}

	if (is_quad)
	{
		damage *= 2; 
		kick *= 2;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);//was up
	if(ent->client->ps.gunframe == 6 || ent->client->ps.gunframe == 8 || ent->client->ps.gunframe == 10 || ent->client->ps.gunframe == 12)
	{	
		if(ent->altfire)
			ent->client->kick_angles[0] = -3; /* Kick view up */
		else
		{
			ent->client->kick_angles[2] = (random() - 0.5)*3; /* Twist the view around a bit */
			ent->client->kick_angles[0] = -1; /* tiny kick for pulsing effect */
		}
	}

	if(ent->client->ps.gunframe == 6 && ent->client->buttons & BUTTON_ATTACK2) {
		int bullet_count = DEFAULT_SSHOTGUN_COUNT;
		/* If we're low on ammo, fire less shots */
		if(ent->client->pers.inventory[ent->client->ammo_index] < DEFAULT_SSHOTGUN_COUNT/2)
			bullet_count = ent->client->pers.inventory[ent->client->ammo_index] * 2;
		VectorSet(offset, 1, 1, ent->viewheight-0.5);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
		fire_shotgun (ent, start, forward, damage/2, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, bullet_count, MOD_CGALTFIRE);

		//play a booming sound
		gi.sound(ent, CHAN_AUTO, gi.soundindex("world/rocket.wav"), 1, ATTN_NORM, 0);

		// send muzzle flash
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte ((MZ_CHAINGUN1 + shots - 1) | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		//create smoke effect

		forward[0] = forward[0] * 24;
		forward[1] = forward[1] * 24;
		right[0] = right[0] * 3;
		right[1] = right[1] * 3;
		start[2] -= 2;

		VectorAdd(start, forward, start);
		VectorAdd(start, right, start);
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_CHAINGUNSMOKE);
		gi.WritePosition (start);
		gi.multicast (start, MULTICAST_PVS);

		if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
			ent->client->pers.inventory[ent->client->ammo_index] -= 10;

		//kick it ahead, we don't want spinning
		ent->client->ps.gunframe = 12;

	}
	else if(!ent->altfire){
		for (i=0 ; i<shots ; i++)
		{
			VectorSet(offset, 1, 1, ent->viewheight-0.5);
			P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
			fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
		}

		// send muzzle flash
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte ((MZ_CHAINGUN1 + shots - 1) | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		//create visual muzzle flash sprite!

		forward[0] = forward[0] * 24;
		forward[1] = forward[1] * 24;
		right[0] = right[0] * 3;
		right[1] = right[1] * 3;
		start[2] -= 2;

		VectorAdd(start, forward, start);
		VectorAdd(start, right, start);
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_CHAINGUNSMOKE);
		gi.WritePosition (start);
		gi.multicast (start, MULTICAST_PVS);
		if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
			ent->client->pers.inventory[ent->client->ammo_index] -= shots;
	}

}

void Weapon_Chain (edict_t *ent)
{
	static int	pause_frames[]	= {43, 0};
	static int	fire_frames[]	= {5, 6, 7, 8, 9, 10, 11, 12, 13};

	Weapon_Generic (ent, 4, 14, 43, 46, pause_frames, fire_frames, Machinegun_Fire);

}

void weapon_floater_fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damage_radius;
	int		radius_damage;

	damage = 100 + (int)(random() * 20.0);
	radius_damage = 120;
	damage_radius = 120;
	if (is_quad || excessive->value)
	{
		damage *= 2; 
		radius_damage *= 2;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	VectorSet(offset, 8, 8, ent->viewheight-4);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	forward[0] = forward[0] * 2.6;
	forward[1] = forward[1] * 2.6;
	forward[2] = forward[2] * 2.6;
	if(ent->altfire) {
		if(excessive->value)
			fire_floater (ent, start, forward, damage, 400, damage_radius, radius_damage, 8);
		else
			fire_prox (ent, start, forward, damage-50, 200, damage_radius, radius_damage-50, 8);
	}
	else
		fire_floater (ent, start, forward, damage, 500, damage_radius, radius_damage, 8);
	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_SHOTGUN | is_silenced);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

//create visual muzzle flash sprite!

	forward[0] = forward[0] * 10;
	forward[1] = forward[1] * 10;

	VectorAdd(start, forward, start);
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SMART_MUZZLEFLASH);
	gi.WritePosition (start);
	gi.multicast (start, MULTICAST_PVS);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;
}

void Weapon_Smartgun (edict_t *ent)
{
	static int	pause_frames[]	= {31, 0};
	static int	fire_frames[]	= {6, 0};

	if(ent->client->buttons & BUTTON_ATTACK2)
		ent->altfire = true;
	else if(ent->client->buttons & BUTTON_ATTACK)
		ent->altfire = false;

	Weapon_Generic (ent, 3, 11, 31, 35, pause_frames, fire_frames, weapon_floater_fire);
}

void Weapon_Deathball_Fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -3, ent->client->kick_origin);
	ent->client->kick_angles[0] = -3;

	VectorSet(offset, 32, 5,  ent->viewheight-5);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);


	if(ent->client->ps.gunframe == 7) {

		fire_deathball (ent, start, forward, 550);

		// send muzzle flash
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_RAILGUN | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		VectorAdd(start, forward, start);
		start[2]+=6;
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLUE_MUZZLEFLASH);
		gi.WritePosition (start);
		gi.multicast (start, MULTICAST_PVS);


		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/energyfield.wav"), 1, ATTN_NORM, 0);
		ent->client->weapon_sound = 0;



	}
	ent->client->ps.gunframe++;

}
void Weapon_Deathball (edict_t *ent)
{
	static int	pause_frames[]	= {33, 0};
	static int	fire_frames[]	= {7,0};

	Weapon_Generic (ent, 5, 11, 33, 39, pause_frames, fire_frames, Weapon_Deathball_Fire);
}

/*
======================================================================

VIOALATOR

======================================================================
*/

void Violator_Fire (edict_t *ent)
{
	vec3_t		start;
	vec3_t		forward, right, left, back;
	vec3_t		offset;
	int			damage;
	int			kick = 4;

	if(excessive->value || instagib->value)
		damage = 200;
	else
		damage = 40;
	
	if ((ent->client->ps.gunframe == 6) && !(ent->client->buttons & BUTTON_ATTACK || ent->client->buttons & BUTTON_ATTACK2))
	{
		ent->client->ps.gunframe = 14;
		ent->client->weapon_sound = 0;
		return;
	}
	else if (ent->client->ps.gunframe == 14 && (ent->client->buttons & BUTTON_ATTACK || ent->client->buttons & BUTTON_ATTACK2))
	{
		ent->client->ps.gunframe = 6;
	}
	else if (ent->client->buttons & BUTTON_ATTACK2 && ent->client->ps.gunframe > 6)
	{
		if(ent->client->ps.gunframe == 7 || ent->client->ps.gunframe == 13) {
			ent->client->ps.gunframe = 14;
			return;
		}
		ent->altfire = true;
		ent->client->ps.gunframe = 14;
	}
	else if (ent->client->buttons & BUTTON_ATTACK2)
	{
		ent->client->ps.gunframe++;
		ent->altfire = true;
	}
	else if (ent->client->buttons & BUTTON_ATTACK){
		ent->client->ps.gunframe++;
		ent->altfire = false;
	}
	else
		ent->client->ps.gunframe++;

	if (is_quad) { 
		damage *= 2;
		kick *=2;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorScale (forward, -6 * random(), ent->client->kick_origin);
	ent->client->kick_angles[0] = -6 * random();
	VectorScale(forward, 10, forward);
	VectorScale(right, 10, right);
	VectorScale (right, -10, left);
	VectorScale (forward, -10, back);

	if(ent->client->ps.gunframe == 6 && ent->client->buttons & BUTTON_ATTACK2) {
	
		VectorSet(offset, 1, 1, ent->viewheight-0.5);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
		fire_violator(ent, start, forward, damage/2, kick*20, 1);
		fire_violator(ent, start, right, damage/2, kick*20, 1);
		fire_violator(ent, start, left, damage/2, kick*20, 1);
		fire_violator(ent, start, back, damage/2, kick*20, 1);

		ent->client->resp.weapon_shots[8]++;

		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/viofire2.wav"), 1, ATTN_NORM, 0);

		// send muzzle flash and effects
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_RAILGUN | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);
		
		VectorScale(forward, 1.4, forward);
		VectorAdd(start, forward, start);
		VectorScale(right, -0.5, right);
		VectorAdd(start, right, start);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_VOLTAGE);
		gi.WritePosition (start);
		gi.WriteDir(forward);
		gi.multicast (start, MULTICAST_PVS);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLUE_MUZZLEFLASH);
		gi.WritePosition (start);
		gi.multicast (start, MULTICAST_PVS);

		//kick it ahead, we don't want it continuing to pump
		ent->client->ps.gunframe = 12;

	}
	else if(!ent->altfire){

		VectorSet(offset, 1, 1, ent->viewheight-0.5);
		P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
		fire_violator(ent, start, forward, damage, kick, 0);

		gi.sound(ent, CHAN_AUTO, gi.soundindex("weapons/viofire1.wav"), 1, ATTN_NORM, 0);

		ent->client->resp.weapon_shots[8]++;

		// send muzzle flash
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_RAILGUN | is_silenced);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		VectorScale(forward, 1.4, forward);
		VectorAdd(start, forward, start);
		VectorScale(right, -0.5, right);
		VectorAdd(start, right, start);
	
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_VOLTAGE);
		gi.WritePosition (start);
		gi.WriteDir(forward);
		gi.multicast (start, MULTICAST_PVS);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLUE_MUZZLEFLASH);
		gi.WritePosition (start);
		gi.multicast (start, MULTICAST_PVS);


	}

}

void Weapon_Violator (edict_t *ent)
{
	static int	pause_frames[]	= {43, 0};
	static int	fire_frames[]	= {5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
	static int	bot_fire_frames[] = {5, 6, 7, 8, 9, 10, 11, 12, 13};
	if(ent->is_bot) //done because bots need one frame in which to "escape" from firing 
		Weapon_Generic (ent, 4, 14, 43, 46, pause_frames, bot_fire_frames, Violator_Fire);
	else
		Weapon_Generic (ent, 4, 14, 43, 46, pause_frames, fire_frames, Violator_Fire);

}

