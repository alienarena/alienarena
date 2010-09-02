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

//Deathball

static void DeathballThink(edict_t *ent)
{
	ent->nextthink = level.time + FRAMETIME;
}
void DeathballSetup (edict_t *ent)
{
	trace_t		tr;
	vec3_t		dest;
	float		*v;

	v = tv(-32,-32,-32);
	VectorCopy (v, ent->mins);
	v = tv(32,32,32);
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
		gi.dprintf ("DeathballSetup: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	VectorCopy (tr.endpos, ent->s.origin);

	gi.linkentity (ent);
	ent->s.frame = 229;
	ent->nextthink = level.time + FRAMETIME;
	ent->think = DeathballThink;
}
void DeathballDrop(edict_t *ent, gitem_t *item)
{
	if (rand() & 1)
		safe_cprintf(ent, PRINT_HIGH, "Only lusers drop the ball!\n");
	else
		safe_cprintf(ent, PRINT_HIGH, "Winners don't drop their balls!\n");
}
void ResetDeathball()
{
	char *c;
	edict_t *ent;

	ent = NULL;
	c = "item_deathball";
	while ((ent = G_Find (ent, FOFS(classname), c)) != NULL) {
		if (ent->spawnflags & DROPPED_ITEM)
			G_FreeEdict(ent);
		else {
			ent->svflags &= ~SVF_NOCLIENT;
			ent->solid = SOLID_TRIGGER;
			gi.linkentity(ent);
			ent->s.frame = 229;
			ent->s.event = EV_ITEM_RESPAWN;
		}
	}
}

static void DropDeathballTouch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	//owner (who dropped us) can't touch for two secs
	if (other == ent->owner &&
		ent->nextthink - level.time > 2)
		return;
	other->in_deathball = true;
	Touch_Item (ent, other, plane, surf);
}
static void DropDeathballThink(edict_t *ent)
{
	// auto return the ball
	// reset ball will remove ourselves

	ResetDeathball();
	safe_bprintf(PRINT_HIGH, "The ball has returned!\n");

}
void DeadDropDeathball(edict_t *self)
{
	edict_t *dropped = NULL;
	gitem_t *deathball_item;

	dropped = NULL;
	deathball_item = NULL;

	deathball_item = FindItemByClassname("item_deathball");

	if (self->client->pers.inventory[ITEM_INDEX(deathball_item)]) {
		dropped = Drop_Item(self, deathball_item);
		self->client->pers.inventory[ITEM_INDEX(deathball_item)] = 0;
		safe_bprintf(PRINT_HIGH, "%s lost the ball!\n",
			self->client->pers.netname);
		self->s.modelindex4 = 0;
		self->in_deathball = false;
	}

	if (dropped) {
		dropped->think = DropDeathballThink;
		dropped->nextthink = level.time + 30;
		dropped->touch = DropDeathballTouch;
		dropped->s.frame = 229;
	}
}

qboolean Pickup_deathball (edict_t *ent, edict_t *other)
{

	gitem_t *bomber;
	gitem_t *strafer;
	gitem_t *hover;
	gitem_t *deathball;
	char	cleanname[16];
	int i, j;
	edict_t	*cl_ent;

    //check for any vehicles, if we have one, don't get in a deathball!
	bomber = FindItemByClassname("item_bomber");
	strafer = FindItemByClassname("item_strafer");
	hover = FindItemByClassname("item_hover");
	if((other->client->pers.inventory[ITEM_INDEX(bomber)] == 1) || (other->client->pers.inventory[ITEM_INDEX(strafer)] == 1)
		|| (other->client->pers.inventory[ITEM_INDEX(hover)] == 1))
		return false;

	deathball = FindItemByClassname(ent->classname);

	// get in the ball
	if(other->client->pers.inventory[ITEM_INDEX(deathball)] == 1)
		return false; //can't get in a deathball if you're already in one! (would never happen)

	//put him in the ball(weapon model replaced with this)
	other->s.modelindex4 = gi.modelindex("vehicles/deathball/deathball.md2");

	other->in_deathball = true;

	other->client->pers.inventory[ITEM_INDEX(deathball)] = 1;
	other->client->newweapon = ent->item;

	if (!(ent->spawnflags & DROPPED_ITEM)) {
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	//clean up name, get rid of escape chars
	j = 0;
	for (i = 0; i < 16; i++)
		cleanname[i] = 0;
		for (i = 0; i < strlen(other->client->pers.netname) && i < 16; i++) {
			if ( other->client->pers.netname[i] == '^' ) {
				i += 1;
				continue;
			}
		cleanname[j] = other->client->pers.netname[i]+128;
		j++;
	}
	if((int)(dmflags->value) & DF_SKINTEAMS) {
		for (i=0 ; i<maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			safe_centerprintf(cl_ent, "%s got the ball!\n", cleanname);
		}
		safe_centerprintf(other, "You've got the ball!\nShoot the ball\ninto your enemy's goal!");
	}
	else
	{
		for (i=0 ; i<maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			safe_centerprintf(cl_ent, "%s got the ball!\n", cleanname);
		}
			safe_centerprintf(other, "You've got the ball!\nShoot the ball\ninto any goal!");
	}
	//play a sound here too
	gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/db_pickup.wav"), 1, ATTN_NONE, 0);

	return true;
}
