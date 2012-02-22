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
#include "m_player.h"



static	edict_t		*current_player;
static	gclient_t	*current_client;

static	vec3_t	forward, right, up;
float	xyspeed;

float	bobmove;
int		bobcycle;		// odd cycles are right foot going forward
float	bobfracsin;		// sin(bobfrac*M_PI)

/*
===============
SV_CalcRoll

===============
*/
float SV_CalcRoll (vec3_t angles, vec3_t velocity)
{
	float	sign;
	float	side;
	float	value;

	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);

	value = sv_rollangle->value;

	if (side < sv_rollspeed->value)
		side = side * value / sv_rollspeed->value;
	else
		side = value;

	return side*sign;

}


/*
===============
P_DamageFeedback

Handles color blends and view kicks
===============
*/
void P_DamageFeedback (edict_t *player)
{
	gclient_t	*client;
	float	side;
	float	realcount, count, kick;
	vec3_t	v;
	int		r, l;
	static	vec3_t	acolor = {1.0, 1.0, 1.0};
	static	vec3_t	bcolor = {1.0, 0.0, 0.0};

	client = player->client;

	// flash the backgrounds behind the status numbers
	client->ps.stats[STAT_FLASHES] = 0;
	if (client->damage_blood)
		client->ps.stats[STAT_FLASHES] |= 1;
	if (client->damage_armor && !(player->flags & FL_GODMODE) && (client->invincible_framenum <= level.framenum))
		client->ps.stats[STAT_FLASHES] |= 2;

	// total points of damage shot at the player this frame
	count = (client->damage_blood + client->damage_armor);
	if (count == 0)
		return;		// didn't take any damage

	// start a pain animation if still in the player model
	if (client->anim_priority < ANIM_PAIN && player->s.modelindex == 255)
	{
		static int		i;

		client->anim_priority = ANIM_PAIN;
		if (client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			player->s.frame = FRAME_crpain1-1;
			client->anim_end = FRAME_crpain4;
		}
		else
		{
			i = (i+1)%3;
			switch (i)
			{
			case 0:
				player->s.frame = FRAME_pain101-1;
				client->anim_end = FRAME_pain104;
				break;
			case 1:
				player->s.frame = FRAME_pain201-1;
				client->anim_end = FRAME_pain204;
				break;
			case 2:
				player->s.frame = FRAME_pain301-1;
				client->anim_end = FRAME_pain304;
				break;
			}
		}
	}

	realcount = count;
	if (count < 10)
		count = 10;	// always make a visible effect

	// play an apropriate pain sound
	if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) && (client->invincible_framenum <= level.framenum))
	{
		r = 1 + (rand()&1);
		player->pain_debounce_time = level.time + 0.7;
		if (player->health < 25)
			l = 25;
		else if (player->health < 50)
			l = 50;
		else if (player->health < 75)
			l = 75;
		else
			l = 100;
		gi.sound (player, CHAN_VOICE, gi.soundindex(va("*pain%i_%i.wav", l, r)), 1, ATTN_NORM, 0);
	}

	// the total alpha of the blend is always proportional to count
	if (client->damage_alpha < 0)
		client->damage_alpha = 0;
	client->damage_alpha += count*0.01;
	if (client->damage_alpha < 0.2)
		client->damage_alpha = 0.2;
	if (client->damage_alpha > 0.6)
		client->damage_alpha = 0.6;		// don't go too saturated

	// the color of the blend will vary based on how much was absorbed
	// by different armors
	VectorClear (v);
	if (client->damage_armor)
		VectorMA (v, (float)client->damage_armor/realcount,  acolor, v);
	if (client->damage_blood)
		VectorMA (v, (float)client->damage_blood/realcount,  bcolor, v);
	VectorCopy (v, client->damage_blend);


	//
	// calculate view angle kicks
	//
	kick = abs(client->damage_knockback);
	if (kick && player->health > 0)	// kick of 0 means no view adjust at all
	{
		kick = kick * 100 / player->health;

		if (kick < count*0.5)
			kick = count*0.5;
		if (kick > 50)
			kick = 50;

		VectorSubtract (client->damage_from, player->s.origin, v);
		VectorNormalize (v);

		side = DotProduct (v, right);
		client->v_dmg_roll = kick*side*0.3;

		side = -DotProduct (v, forward);
		client->v_dmg_pitch = kick*side*0.3;

		client->v_dmg_time = level.time + DAMAGE_TIME;
	}

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}




/*
===============
SV_CalcViewOffset

Auto pitching on slopes?

  fall from 128: 400 = 160000
  fall from 256: 580 = 336400
  fall from 384: 720 = 518400
  fall from 512: 800 = 640000
  fall from 640: 960 =

  damage = deltavelocity*deltavelocity  * 0.0001

===============
*/
void SV_CalcViewOffset (edict_t *ent)
{
	float		*angles;
	float		bob;
	float		ratio;
	float		delta;
	vec3_t	v;

	// base angles
	angles = ent->client->ps.kick_angles;

	// if dead, fix the angle and don't add any kick
	if (ent->deadflag)
	{
		VectorClear (angles);

		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
	}
	else
	{
		// add angles based on weapon kick
		VectorCopy (ent->client->kick_angles, angles);

		// add angles based on damage kick
		ratio = (ent->client->v_dmg_time - level.time) / DAMAGE_TIME;
		if (ratio < 0)
		{
			ratio = 0;
			ent->client->v_dmg_pitch = 0;
			ent->client->v_dmg_roll = 0;
		}
		angles[PITCH] += ratio * ent->client->v_dmg_pitch;
		angles[ROLL] += ratio * ent->client->v_dmg_roll;

		// add pitch based on fall kick

// 		ratio = (ent->client->fall_time - level.time) / FALL_TIME;
// 		if (ratio < 0)
// 			ratio = 0;
// 		angles[PITCH] += ratio * ent->client->fall_value;

		// add angles based on velocity
		delta = DotProduct (ent->velocity, forward);
		angles[PITCH] += delta*run_pitch->value;

		delta = DotProduct (ent->velocity, right);
		angles[ROLL] += delta*run_roll->value;

		// add angles based on bob
		delta = bobfracsin * bob_pitch->value * xyspeed;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;		// crouching
		angles[PITCH] += delta;
		delta = bobfracsin * bob_roll->value * xyspeed;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;		// crouching
		if (bobcycle & 1)
			delta = -delta;
		angles[ROLL] += delta;
	}

	// base origin
	VectorClear (v);

	// add view height
	v[2] += ent->viewheight;

	// add fall height
	ratio = (ent->client->fall_time - level.time) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	v[2] -= ratio * ent->client->fall_value;

	// add bob height
	bob = bobfracsin * xyspeed * bob_up->value;
	if (bob > 6)
		bob = 6;
	v[2] += bob;

	// add kick offset
	VectorAdd (v, ent->client->kick_origin, v);

	// absolutely bound offsets
	// so the view can never be outside the player box
	if (!ent->client->chasetoggle)
	{
	if (v[0] < -14)
		v[0] = -14;
	else if (v[0] > 14)
		v[0] = 14;
	if (v[1] < -14)
		v[1] = -14;
	else if (v[1] > 14)
		v[1] = 14;
	if (v[2] < -22)
		v[2] = -22;
	else if (v[2] > 30)
		v[2] = 30;
	}
	else
	{
        	VectorSet (v, 0, 0, 0);
        	if (ent->client->chasecam != NULL)
        {
            ent->client->ps.pmove.origin[0] = ent->client->chasecam->s.origin[0]*8;
            ent->client->ps.pmove.origin[1] = ent->client->chasecam->s.origin[1]*8;
            ent->client->ps.pmove.origin[2] = ent->client->chasecam->s.origin[2]*8;
            VectorCopy (ent->client->chasecam->s.angles, ent->client->ps.viewangles);
        }
    }
	VectorCopy (v, ent->client->ps.viewoffset);
}

/*
==============
SV_CalcGunOffset
==============
*/
void SV_CalcGunOffset (edict_t *ent)
{
	int		i;

	// gun angles from bobbing
	ent->client->ps.gunangles[ROLL] = xyspeed * bobfracsin * 0.005;
	ent->client->ps.gunangles[YAW] = xyspeed * bobfracsin * 0.01;
	if (bobcycle & 1)
	{
		ent->client->ps.gunangles[ROLL] = -ent->client->ps.gunangles[ROLL];
		ent->client->ps.gunangles[YAW] = -ent->client->ps.gunangles[YAW];
	}

	ent->client->ps.gunangles[PITCH] = xyspeed * bobfracsin * 0.005;

	VectorClear (ent->client->ps.gunoffset);

	// gun_x / gun_y / gun_z are development tools
	for (i=0 ; i<3 ; i++)
	{
		ent->client->ps.gunoffset[i] += forward[i]*(gun_y->value);
		ent->client->ps.gunoffset[i] += right[i]*gun_x->value;
		ent->client->ps.gunoffset[i] += up[i]* (-gun_z->value);

	}

	//landing on jumps
	if(ent->s.event == EV_FALLSHORT || ent->s.event == EV_FALL || ent->s.event == EV_FALLFAR) {
		ent->client->ps.gunoffset[2] -=1.5;
		ent->client->ps.gunangles[PITCH] -= 1;
		ent->client->ps.gunangles[ROLL] -= 1;
	}
}


/*
=============
SV_AddBlend
=============
*/
void SV_AddBlend (float r, float g, float b, float a, float *v_blend)
{
	float	a2, a3;

	if (a <= 0)
		return;
	a2 = v_blend[3] + (1-v_blend[3])*a;	// new total alpha
	a3 = v_blend[3]/a2;		// fraction of color from old

	v_blend[0] = v_blend[0]*a3 + r*(1-a3);
	v_blend[1] = v_blend[1]*a3 + g*(1-a3);
	v_blend[2] = v_blend[2]*a3 + b*(1-a3);
	v_blend[3] = a2;
}

void ResetWeaponModel (edict_t *ent)
{
	char    *info;
	char	weaponame[64] = " ";
	char	weaponmodel[MAX_OSPATH] = " ";
	int i;
	int done;
	char	weaponpath[MAX_OSPATH] = " ";
	FILE *file;

	if (ent->in_vehicle) {
		return;
	}

	//set up code to set player world weapon model

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

	if( !Q_strcasecmp( ent->client->pers.weapon->view_model, "models/weapons/v_violator/tris.md2" ))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_violator.md2");
	else if( !Q_strcasecmp( ent->client->pers.weapon->view_model, "models/weapons/v_rocket/tris.md2"))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_rlauncher.md2");
	else if( !Q_strcasecmp(ent->client->pers.weapon->view_model,"models/weapons/v_blast/tris.md2"))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_blaster.md2");
	else if( !Q_strcasecmp(ent->client->pers.weapon->view_model,"models/weapons/v_bfg/tris.md2"))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_bfg.md2");
	else if( !Q_strcasecmp(ent->client->pers.weapon->view_model, "models/weapons/v_rail/tris.md2"))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_railgun.md2");
	else if( !Q_strcasecmp(ent->client->pers.weapon->view_model, "models/weapons/v_shotg2/tris.md2"))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_sshotgun.md2");
	else if( !Q_strcasecmp(ent->client->pers.weapon->view_model, "models/weapons/v_shotg/tris.md2"))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_shotgun.md2");
	else if( !Q_strcasecmp(ent->client->pers.weapon->view_model, "models/weapons/v_hyperb/tris.md2"))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_hyperblaster.md2");
	else if( !Q_strcasecmp(ent->client->pers.weapon->view_model, "models/weapons/v_chain/tris.md2"))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_chaingun.md2");
	else if( !Q_strcasecmp(ent->client->pers.weapon->view_model, "models/weapons/v_minderaser/tris.md2"))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_minderaser.md2");
	else if( !Q_strcasecmp(ent->client->pers.weapon->view_model, "vehicles/deathball/v_wep.md2"))
		sprintf(weaponmodel, "players/%s%s", weaponame, "w_machinegun.md2");

	sprintf(weaponpath, "%s", weaponmodel);
	Q2_FindFile (weaponpath, &file); //does it really exist?
	if(!file)
	{
		sprintf(weaponpath, "%s%s", weaponame, "weapon.md2"); //no w_weaps, do we have this model?
		Q2_FindFile (weaponpath, &file);
		if(!file) //server does not have this player model
			sprintf(weaponmodel, "players/martianenforcer/weapon.md2");//default player(martian)
		else
		{ //have the model, but it has no w_weaps
			sprintf(weaponmodel, "players/%s%s", weaponame, "weapon.md2"); //custom weapon
			fclose(file);
		}
	}
	else
		fclose(file);
	ent->s.modelindex2 = gi.modelindex(weaponmodel);
}

/*
=============
SV_CalcBlend
=============
*/
void SV_CalcBlend (edict_t *ent)
{
	int		contents;
	vec3_t	vieworg;
	int		remaining;

	ent->client->ps.blend[0] = ent->client->ps.blend[1] =
		ent->client->ps.blend[2] = ent->client->ps.blend[3] = 0;

	// add for contents
	VectorAdd (ent->s.origin, ent->client->ps.viewoffset, vieworg);
	contents = gi.pointcontents (vieworg);
	if (contents & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER) )
		ent->client->ps.rdflags |= RDF_UNDERWATER;
	else
		ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	if (contents & (CONTENTS_SOLID|CONTENTS_LAVA))
		SV_AddBlend (1.0, 0.3, 0.0, 0.6, ent->client->ps.blend);
	else if (contents & CONTENTS_SLIME)
		SV_AddBlend (0.0, 0.1, 0.05, 0.6, ent->client->ps.blend);
	else if (contents & CONTENTS_WATER)
		SV_AddBlend (0.5, 0.3, 0.2, 0.4, ent->client->ps.blend);

	// add for powerups
	//vehicles(flying)
	if ( Jet_Active(ent) )
	{

		/*update the fuel time*/
		ent->client->Jet_remaining = ent->client->Jet_framenum - level.framenum;

		/*if no fuel remaining, remove vehicle from inventory*/
		if ( ent->client->Jet_remaining == 0 ) {
		  ent->client->pers.inventory[ITEM_INDEX(FindItem("bomber"))] = 0;
		  ent->client->pers.inventory[ITEM_INDEX(FindItem("strafer"))] = 0;
		  ent->client->pers.inventory[ITEM_INDEX(FindItem("hover"))] = 0;
		  //set everything back
		  Reset_player(ent);

		}

		/*Play jetting sound every 0.6 secs (sound of monster icarus)*/
		if ( ((int)ent->client->Jet_remaining % 6) == 0 ) {
		  gi.sound (ent, CHAN_AUTO, gi.soundindex("vehicles/bomberidle.wav"), 0.9, ATTN_NORM, 0);

		  // send muzzle flash
		  gi.WriteByte (svc_muzzleflash);
		  gi.WriteShort (ent-g_edicts);
		  gi.WriteByte (MZ_RAILGUN | MZ_SILENCED);
		  gi.multicast (ent->s.origin, MULTICAST_PVS);
		}

		/*beginning to fade if 4 secs or less*/
		if (ent->client->Jet_remaining <= 40)
		  /*play on/off sound every sec*/
		  if ( ((int)ent->client->Jet_remaining % 10) == 0 )
			gi.sound(ent, CHAN_ITEM, gi.soundindex("vehicles/warning.wav"), 1, ATTN_NORM, 0);

		if (ent->client->Jet_remaining > 40 || ( (int)ent->client->Jet_remaining & 4) )
		  SV_AddBlend (0, 0, 1, 0.08, ent->client->ps.blend);
	}
	else if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage2.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4) )
			SV_AddBlend (0, 0, 1, 0.08, ent->client->ps.blend);
	}
	else if (ent->client->invincible_framenum > level.framenum || ent->client->spawnprotected)
	{
		remaining = ent->client->invincible_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4) )
			SV_AddBlend (1, 1, 0, 0.08, ent->client->ps.blend);
	}
	else if (ent->client->haste_framenum > level.framenum)
	{
		remaining = ent->client->haste_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/hasteout.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4) )
			SV_AddBlend (0.4, 1, 0.4, 0.04, ent->client->ps.blend);
	}
	else if (ent->client->sproing_framenum > level.framenum)
	{
		remaining = ent->client->sproing_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/sproingout.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4) )
			SV_AddBlend (0.4, 1, 0.4, 0.04, ent->client->ps.blend);
	}
	else if (ent->client->invis_framenum > level.framenum)
	{
		remaining = ent->client->invis_framenum - level.framenum;
		if (remaining == 30)	// beginning to fade
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1, ATTN_NORM, 0);
		if (remaining > 30 || (remaining & 4) )
			SV_AddBlend (0.4, 1, 0.4, 0.04, ent->client->ps.blend);
		if (remaining == 1) { //put weapon model back
			ResetWeaponModel(ent);
		}
	}

	// add for damage
	if (ent->client->damage_alpha > 0)
		SV_AddBlend (ent->client->damage_blend[0],ent->client->damage_blend[1]
		,ent->client->damage_blend[2], ent->client->damage_alpha, ent->client->ps.blend);

	if (ent->client->bonus_alpha > 0)
		SV_AddBlend (0.85, 0.7, 0.3, ent->client->bonus_alpha, ent->client->ps.blend);

	// drop the damage value
	ent->client->damage_alpha -= 0.06;
	if (ent->client->damage_alpha < 0)
		ent->client->damage_alpha = 0;

	// drop the bonus value
	ent->client->bonus_alpha -= 0.1;
	if (ent->client->bonus_alpha < 0)
		ent->client->bonus_alpha = 0;
}


/*
=================
P_FallingDamage
=================
*/
void P_FallingDamage (edict_t *ent)
{
	float	delta;
	int		damage;
	vec3_t	dir;

	if (ent->s.modelindex != 255)
		return;		// not in the player model

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if(joustmode->value)
		return;

	if ((ent->client->oldvelocity[2] < 0) && (ent->velocity[2] > ent->client->oldvelocity[2]) && (!ent->groundentity))
	{
		delta = ent->client->oldvelocity[2];
	}
	else
	{
		if (!ent->groundentity)
			return;
		delta = ent->velocity[2] - ent->client->oldvelocity[2];
	}
	delta = delta*delta * 0.0001;

	// never take damage if just release grapple or on grapple
	if (level.time - ent->client->ctf_grapplereleasetime <= FRAMETIME * 2 ||
		(ent->client->ctf_grapple &&
		ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY))
		return;

	// never take falling damage if completely underwater
	if (ent->waterlevel == 3)
		return;
	if (ent->waterlevel == 2)
		delta *= 0.25;
	if (ent->waterlevel == 1)
		delta *= 0.5;

	if (delta < 3)
		return;

// 	if (delta < 15)
// 	{
// 		ent->s.event = EV_FOOTSTEP;
// 		return;
// 	}

	ent->client->fall_value = delta;
	if (ent->client->fall_value > 20)
		ent->client->fall_value = 20;
	ent->client->fall_time = level.time + FALL_TIME;

	if (delta > 30)
	{
		if (ent->health > 0)
		{
			if (delta >= 55)
				ent->s.event = EV_FALLFAR;
			else
				ent->s.event = EV_FALL;
		}
		ent->pain_debounce_time = level.time;	// no normal pain sound
		damage = (delta-30)/2;
		if (damage < 1)
			damage = 1;
		if (damage > 10)
			damage = 10;
		VectorSet (dir, 0, 0, 1);

		if (!deathmatch->value || !(dmflags->integer & DF_NO_FALLING) )
			T_Damage (ent, world, world, dir, ent->s.origin, vec3_origin, damage, 0, 0, MOD_FALLING);
	}
	else
	{
		ent->s.event = EV_FALLSHORT;
		return;
	}
}



/*
=============
P_WorldEffects
=============
*/
void P_WorldEffects (void)
{
	int			waterlevel, old_waterlevel;

	if (current_player->movetype == MOVETYPE_NOCLIP)
	{
		current_player->air_finished = level.time + 12;	// don't need air
		return;
	}

	waterlevel = current_player->waterlevel;
	old_waterlevel = current_client->old_waterlevel;
	current_client->old_waterlevel = waterlevel;

	//
	// if just entered a water volume, play a sound
	//
	if (!old_waterlevel && waterlevel)
	{
		PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		if (current_player->watertype & CONTENTS_LAVA)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/lava_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_SLIME)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_WATER)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		current_player->flags |= FL_INWATER;

		// clear damage_debounce, so the pain sound will play immediately
		current_player->damage_debounce_time = level.time - 1;
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (old_waterlevel && ! waterlevel)
	{
		PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
		current_player->flags &= ~FL_INWATER;
	}

	//
	// check for head just going under water
	//
	if (old_waterlevel != 3 && waterlevel == 3)
	{
		gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_un.wav"), 1, ATTN_NORM, 0);
	}

	//
	// check for head just coming out of water
	//
	if (old_waterlevel == 3 && waterlevel != 3)
	{
		if (current_player->air_finished < level.time)
		{	// gasp for air
			gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/gasp1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		}
		else  if (current_player->air_finished < level.time + 11)
		{	// just break surface
			gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/gasp2.wav"), 1, ATTN_NORM, 0);
		}
	}

	//
	// check for drowning
	//
	if (waterlevel == 3)
	{
		// if out of air, start drowning
		if (current_player->air_finished < level.time)
		{	// drown!
			if (current_player->client->next_drown_time < level.time
				&& current_player->health > 0)
			{
				current_player->client->next_drown_time = level.time + 1;

				// take more damage the longer underwater
				current_player->dmg += 2;
				if (current_player->dmg > 15)
					current_player->dmg = 15;

				// play a gurp sound instead of a normal pain sound
				if (current_player->health <= current_player->dmg)
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/drown1.wav"), 1, ATTN_NORM, 0);
				else if (rand()&1)
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("*gurp1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("*gurp2.wav"), 1, ATTN_NORM, 0);

				current_player->pain_debounce_time = level.time;

				T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, current_player->dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	}
	else
	{
		current_player->air_finished = level.time + 12;
		current_player->dmg = 2;
	}

	//
	// check for sizzle damage
	//
	if (waterlevel && (current_player->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) )
	{
		if (current_player->watertype & CONTENTS_LAVA)
		{
			if (current_player->health > 0
				&& current_player->pain_debounce_time <= level.time
				&& current_client->invincible_framenum < level.framenum)
			{
				if (rand()&1)
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/burn1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/burn2.wav"), 1, ATTN_NORM, 0);
				current_player->pain_debounce_time = level.time + 1;
			}

			T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 3*waterlevel, 0, 0, MOD_LAVA);
		}

		if (current_player->watertype & CONTENTS_SLIME)
		{
			T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1*waterlevel, 0, 0, MOD_SLIME);
		}
	}
}
/*
===============
TeamEffects
===============
*/
void TeamEffects(edict_t *player)
{
	if(player->dmteam == RED_TEAM)
		player->s.effects |= EF_TEAM1;
	else if(player->dmteam == BLUE_TEAM)
		player->s.effects |= EF_TEAM2;
}

/*
===============
G_SetClientEffects
===============
*/
void G_SetClientEffects (edict_t *ent)
{
	int		remaining;

	ent->s.effects = 0;
	ent->s.renderfx = 0;

	if (ent->health <= 0 || level.intermissiontime)
		return;

	if(ctf->value)
		CTFEffects(ent);

	if ((dmflags->integer & DF_SKINTEAMS) || ctf->value || tca->value || cp->value )
		TeamEffects(ent);
	else if (g_dmlights->integer)
	    ent->s.effects |= EF_TEAM2;

	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;
		if (remaining > 30 || (remaining & 4) )
			ent->s.effects |= EF_QUAD;
	}

	if (ent->client->invincible_framenum > level.framenum)
	{
		remaining = ent->client->invincible_framenum - level.framenum;
		if (remaining > 30 || (remaining & 4) )
			ent->s.effects |= EF_PENT;
	}

	// show cheaters!!!
	if (ent->flags & FL_GODMODE)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);
	}

	if(ent->client->spawnprotected)
		ent->s.effects |= EF_PENT;

	if(ent->client->kill_streak >= 8)
		ent->s.effects |= EF_FLIES;

	//invisibility
	if(ent->client->invis_framenum > level.framenum) {
		ent->s.renderfx |= RF_TRANSLUCENT;
		ent->s.modelindex2 = 0;
	}

}


/*
===============
G_SetClientEvent
===============
*/
void G_SetClientEvent (edict_t *ent)
{
	if (ent->s.event)
		return;

	if ( ent->groundentity && xyspeed > 225)
	{
		if ( (int)(current_client->bobtime+bobmove) != bobcycle )
			ent->s.event = EV_FOOTSTEP;

		if ( ((ent->waterlevel == 1) || (ent->waterlevel == 2)) && (xyspeed > 100) )
		{
			if ( (int)(current_client->bobtime+bobmove) != bobcycle )
			{
				ent->s.event = EV_WADE;
			}
		}
	}
}

/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound (edict_t *ent)
{
	char	*weap;

	if (ent->client->pers.game_helpchanged != game.helpchanged)
	{
		ent->client->pers.game_helpchanged = game.helpchanged;
		ent->client->pers.helpchanged = 1;
	}

	// help beep (no more than three times)
	if (ent->client->pers.helpchanged && ent->client->pers.helpchanged <= 3 && !(level.framenum&63) )
	{
		ent->client->pers.helpchanged++;
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
		safe_centerprintf (ent, "Journal Entry - Press F1");
	}


	if (ent->client->pers.weapon)
		weap = ent->client->pers.weapon->classname;
	else
		weap = "";

	if (strcmp(weap, "weapon_bfg") == 0)
		ent->s.sound = gi.soundindex("weapons/vaporizer_hum.wav");
	else if (strcmp(weap, "weapon_shotgun") == 0)
		ent->s.sound = gi.soundindex("weapons/smartgun_hum.wav");
	else if (ent->client->weapon_sound)
		ent->s.sound = ent->client->weapon_sound;
	else
		ent->s.sound = 0;
}

/*
===============
G_SetClientFrame
===============
*/
void G_SetClientFrame (edict_t *ent)
{
	gclient_t	*client;
	qboolean	duck, run;
	//vehicles

	if (ent->in_vehicle) { //don't animate the vehicle(yet!)
		ent->s.frame = 0;
		return;
	}

	if (ent->s.modelindex != 255)
		return;		// not in the player model

	client = ent->client;

	if (client->ps.pmove.pm_flags & PMF_DUCKED)
		duck = true;
	else
		duck = false;
	if (xyspeed)
		run = true;
	else
		run = false;

	// check for stand/duck and stop/go transitions
	if (duck != client->anim_duck && client->anim_priority < ANIM_DEATH)
		goto newanim;
	if (run != client->anim_run && client->anim_priority == ANIM_BASIC)
		goto newanim;
	if (!ent->groundentity && client->anim_priority <= ANIM_WAVE)
		goto newanim;

	//check for running while shooting
	if (run && client->anim_priority == ANIM_ATTACK)
		goto newanim;

	if(client->anim_priority == ANIM_REVERSE)
	{
		if(ent->s.frame > client->anim_end)
		{
			ent->s.frame--;
			return;
		}
	}
	else if (ent->s.frame < client->anim_end)
	{	// continue an animation
		ent->s.frame++;
		return;
	}

	if (client->anim_priority == ANIM_DEATH)
		return;		// stay there
	if (client->anim_priority == ANIM_JUMP)
	{
		if (!ent->groundentity)
			return;		// stay there
		ent->client->anim_priority = ANIM_WAVE;
		ent->s.frame = FRAME_jump3;
		ent->client->anim_end = FRAME_jump6;
		return;
	}

newanim:
	// return to either a running or standing frame
	client->anim_priority = ANIM_BASIC;
	client->anim_duck = duck;
	client->anim_run = run;

	if (!ent->groundentity)
	{
	//ZOID: if on grapple, don't go into jump frame, go into standing
//frame
		if (client->ctf_grapple) {
			ent->s.frame = FRAME_stand01;
			client->anim_end = FRAME_stand40;
		} else {
//ZOID
		client->anim_priority = ANIM_JUMP;
		if (ent->s.frame != FRAME_jump2)
			ent->s.frame = FRAME_jump1;
		client->anim_end = FRAME_jump2;
		}
	}
	else if (run)
	{	// running
		if (duck)
		{
			ent->s.frame = FRAME_crwalk1;
			client->anim_end = FRAME_crwalk6;
		}
		else
		{
			ent->s.frame = FRAME_run1;
			client->anim_end = FRAME_run6;
		}
	}
	else
	{	// standing
		if (duck)
		{
			ent->s.frame = FRAME_crstnd01;
			client->anim_end = FRAME_crstnd19;
		}
		else
		{
			ent->s.frame = FRAME_stand01;
			client->anim_end = FRAME_stand40;
		}
	}
}


/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
void ClientEndServerFrame (edict_t *ent)
{
	float	bobtime;
	int		i;
	current_player = ent;
	current_client = ent->client;

	//
	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	//
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	//
	for (i=0 ; i<3 ; i++)
	{
		current_client->ps.pmove.origin[i] = ent->s.origin[i]*8.0;
		current_client->ps.pmove.velocity[i] = ent->velocity[i]*8.0;
	}

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (level.intermissiontime)
	{
		// FIXME: add view drifting here?
		current_client->ps.blend[3] = 0;
		current_client->ps.fov = 90;
		G_SetStats (ent);
		return;
	}

	AngleVectors (ent->client->v_angle, forward, right, up);

	// burn from lava, etc
	P_WorldEffects ();

	//
	// set model angles from view angles so other things in
	// the world can tell which direction you are looking
	//
	if (ent->client->v_angle[PITCH] > 180)
		ent->s.angles[PITCH] = (-360 + ent->client->v_angle[PITCH])/3;
	else
		ent->s.angles[PITCH] = ent->client->v_angle[PITCH]/3;
	ent->s.angles[YAW] = ent->client->v_angle[YAW];
	ent->s.angles[ROLL] = 0;
	ent->s.angles[ROLL] = SV_CalcRoll (ent->s.angles, ent->velocity)*4;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//

	xyspeed = sqrt(ent->velocity[0]*ent->velocity[0] + ent->velocity[1]*ent->velocity[1]);

	if (xyspeed < 5)
	{
		bobmove = 0;
		current_client->bobtime = 0;	// start at beginning of cycle again
	}
	else if (ent->groundentity)
	{	// so bobbing only cycles when on ground
		if (xyspeed > 210)
			bobmove = 0.25;
		else if (xyspeed > 100)
			bobmove = 0.125;
		else
			bobmove = 0.0625;
	}

	bobtime = (current_client->bobtime += bobmove);

	if (current_client->ps.pmove.pm_flags & PMF_DUCKED)
		bobtime *= 4;

	bobcycle = (int)bobtime;
	bobfracsin = fabs(sin(bobtime*M_PI));

	// detect hitting the floor
	P_FallingDamage (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// determine the view offsets
	SV_CalcViewOffset (ent);

	// determine the gun offsets
	SV_CalcGunOffset (ent);

	// determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	// FIXME: with client prediction, the contents
	// should be determined by the client
	SV_CalcBlend (ent);

	// chase cam stuff
	if (ent->client->resp.spectator)
		G_SetSpectatorStats(ent);
	else
		G_SetStats (ent);
	G_CheckChaseStats(ent);

	G_SetClientEvent (ent);

	G_SetClientEffects (ent);

	G_SetClientSound (ent);

	G_SetClientFrame (ent);

	VectorCopy (ent->velocity, ent->client->oldvelocity);
	VectorCopy (ent->client->ps.viewangles, ent->client->oldviewangles);

	// clear weapon kicks
	VectorClear (ent->client->kick_origin);
	VectorClear (ent->client->kick_angles);

	// if the scoreboard is up, update it
	if (ent->client->showscores && !(level.framenum & 31) )
	{
		if (ent->is_bot)
			return;

		DeathmatchScoreboardMessage (ent, ent->enemy, false);
		gi.unicast (ent, false);
	}
	if (ent->client->chasetoggle == 1)
        CheckDeathcam_Viewent(ent);

	if ( g_antilag->integer)
		G_StoreHistory( ent );
}

