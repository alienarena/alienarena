/*
==============================================================================

Cow NPC - an NPC that respawns after dying, and will follow players until being
caught by their team's tractor beam or "cow trap" devices.

These NPC's are very basic, very dumb creatures that will just walk towards 
whatever client they might see.  The goal is for the players to lure them into
traps, and without them getting "killed", in which case they would respawn at 
their original position.  When a cow walks into a trap, the team who owns the 
trap will get rewarded with points, and that cow will be respawned.

==============================================================================
*/

#include "g_local.h"
#include "cow.h"

static int sound_moo;
static int sound_groan;
static int sound_step1;

int kick = 0;

void cow_pain (edict_t *self, edict_t *other, float kick, int damage);

void cow_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_moo, 1, ATTN_NORM, 0);
}

mframe_t cow_frames_stand [] =
{
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL
	
};
mmove_t cow_move_stand = {FRAME_stand01, FRAME_stand20, cow_frames_stand, NULL};

void cow_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &cow_move_stand;
}

void cow_step (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_step1, 1, ATTN_NORM, 0);
	//perhaps draw a beam to the player that is leading it?
	if(self->enemy) {
		if(self->enemy->dmteam == BLUE_TEAM) {
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_BLASTERBEAM);
			gi.WritePosition (self->s.origin);
			gi.WritePosition (self->enemy->s.origin);
			gi.multicast (self->s.origin, MULTICAST_PHS);   
		}
		else {	
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte (TE_REDLASER);
			gi.WritePosition (self->s.origin);
			gi.WritePosition (self->enemy->s.origin);
			gi.multicast (self->s.origin, MULTICAST_PHS);   
		}

	}
	
}

mframe_t cow_frames_walk [] =
{
	ai_run, 42.0, NULL,
	ai_run, 22.0, NULL,
	ai_run, 5.0, NULL,
	ai_run, 3.0, NULL,
	ai_run, 0.0, NULL,
	ai_run, 0.0, cow_step,
	ai_run, 0.0, NULL,
	ai_run, 17.0, NULL,
	ai_run, 42.0, NULL,
	ai_run, 14.0, NULL,
	ai_run, 5.0, cow_step,
	ai_run, 0.0, NULL
};
mmove_t cow_move_walk = {FRAME_walk01, FRAME_walk12, cow_frames_walk, NULL};

void cow_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &cow_move_walk;
}

void cow_run (edict_t *self)
{
	if ((self->monsterinfo.aiflags & AI_STAND_GROUND))
		self->monsterinfo.currentmove = &cow_move_stand;
	else
		self->monsterinfo.currentmove = &cow_move_walk;
}

void cow_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_moo, 1, ATTN_IDLE, 0);
	self->monsterinfo.currentmove = &cow_move_walk;
}

mframe_t cow_frames_pain [] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t cow_move_pain = {FRAME_pain01, FRAME_pain11, cow_frames_pain, cow_walk};

void cow_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	gi.sound (self, CHAN_VOICE, sound_groan, 1, ATTN_NORM, 0);

	self->monsterinfo.currentmove = &cow_move_pain;
}

void SP_npc_cow (edict_t *self)
{
	
	// pre-caches
	sound_moo  = gi.soundindex ("misc/cow/moo.wav");
	sound_groan = gi.soundindex ("misc/cow/groan.wav");
	sound_step1 = gi.soundindex ("misc/cow/step1.wav");

	self->s.modelindex = gi.modelindex("models/cow/tris.md2");
	self->s.modelindex3 = gi.modelindex("models/cow/helmet.md2");
	
	VectorSet (self->mins, -32, -32, -2);
	VectorSet (self->maxs, 32, 32, 56);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->classname = "cow";

	self->max_health = 150;
	self->health = self->max_health;
	self->gib_health = -40;
	self->mass = 250;

	self->pain = cow_pain;
	self->die = NULL;
	self->monsterinfo.stand = cow_stand;
	self->monsterinfo.walk = cow_walk;
	self->monsterinfo.run = cow_walk;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = cow_walk;
	self->monsterinfo.melee = cow_walk;
	self->monsterinfo.sight = cow_sight;
	self->monsterinfo.search = cow_search;
	self->s.renderfx |= RF_MONSTER;

	self->monsterinfo.currentmove = &cow_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
	self->monsterinfo.aiflags = AI_NPC;
	self->dmteam = NO_TEAM;

	gi.linkentity (self);

	walkmonster_start (self);

	VectorCopy(self->s.origin, self->s.spawn_pos); //remember where the cow began 
}

