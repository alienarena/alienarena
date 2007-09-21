#include "g_local.h"


/*we get silly velocity-effects when we are on ground and try to
  accelerate, so lift us a little bit if possible*/
qboolean Jet_AvoidGround( edict_t *ent )
{
  vec3_t		new_origin;
  trace_t	trace;
  qboolean	success;

  /*Check if there is enough room above us before we change origin[2]*/
  new_origin[0] = ent->s.origin[0];
  new_origin[1] = ent->s.origin[1];
  new_origin[2] = ent->s.origin[2] + 0.5;
  trace = gi.trace( ent->s.origin, ent->mins, ent->maxs, new_origin, ent, MASK_MONSTERSOLID );

  if ( success=(trace.plane.normal[2]==0) )	/*no ceiling?*/
    ent->s.origin[2] += 0.5;			/*then make sure off ground*/

  return success;
}


/*This function returns true if the jet is activated
  (surprise, surprise)*/
qboolean Jet_Active( edict_t *ent )
{
  return ( ent->client->Jet_framenum >= level.framenum );
}


/*If a player dies with activated jetpack this function will be called
  and produces a little explosion*/
void Jet_Explosion( edict_t *ent )
{
  
  gi.WriteByte( svc_temp_entity );
  gi.WriteByte( TE_EXPLOSION1 );   /*TE_EXPLOSION2 is possible too*/
  gi.WritePosition( ent->s.origin );
  gi.multicast( ent->s.origin, MULTICAST_PVS );
 
}


/*The lifting effect is done through changing the origin, it
  gives the best results. Of course its a little dangerous because
  if we dont take care, we can move into solid*/
void Jet_ApplyLifting( edict_t *ent )
{
  float		delta;
  vec3_t	new_origin;
  trace_t	trace;
  int 		time = 24;     /*must be >0, time/10 = time in sec for a
                                 complete cycle (up/down)*/
  float		amplitude = 2.0;

  /*calculate the z-distance to lift in this step*/
  delta = sin( (float)((level.framenum%time)*(360/time))/180*M_PI ) * amplitude;
  delta = (float)((int)(delta*8))/8; /*round to multiples of 0.125*/

  VectorCopy( ent->s.origin, new_origin );
  new_origin[2] += delta;

  if( VectorLength(ent->velocity) == 0 )
  {
     /*i dont know the reason yet, but there is some floating so we
       have to compensate that here (only if there is no velocity left)*/
     new_origin[0] -= 0.125;
     new_origin[1] -= 0.125;
     new_origin[2] -= 0.125;
  }

  /*before we change origin, its important to check that we dont go
    into solid*/
  trace = gi.trace( ent->s.origin, ent->mins, ent->maxs, new_origin, ent, MASK_MONSTERSOLID );
  if ( trace.plane.normal[2] == 0 )
    VectorCopy( new_origin, ent->s.origin );
}


/*This function applys some sparks to your jetpack, this part is
  exactly copied from Muce's and SumFuka's JetPack-tutorial and does a
  very nice effect.*/
void Jet_ApplySparks ( edict_t *ent )
{
  vec3_t  forward, right;
  vec3_t  pack_pos, jet_vector;

  AngleVectors(ent->client->v_angle, forward, right, NULL);
  VectorScale (forward, -7, pack_pos);
  VectorAdd (pack_pos, ent->s.origin, pack_pos);
  pack_pos[2] += (ent->viewheight);
  VectorScale (forward, -50, jet_vector);

  gi.WriteByte (svc_temp_entity);
  gi.WriteByte (TE_SPARKS);
  gi.WritePosition (pack_pos);
  gi.WriteDir (jet_vector);
  gi.multicast (pack_pos, MULTICAST_PVS);
}


/*if the angle of the velocity vector is different to the viewing
  angle (flying curves or stepping left/right) we get a dotproduct
  which is here used for rolling*/
void Jet_ApplyRolling( edict_t *ent, vec3_t right )
{
  float roll,
        value = 0.05,
        sign = -1;    /*set this to +1 if you want to roll contrariwise*/

  roll = DotProduct( ent->velocity, right ) * value * sign;
  ent->client->kick_angles[ROLL] = roll;
}


/*Now for the main movement code. The steering is a lot like in water, that
  means your viewing direction is your moving direction. You have three
  direction Boosters: the big Main Booster and the smaller up-down and
  left-right Boosters.
  There are only 2 adds to the code of the first tutorial: the Jet_next_think
  and the rolling.
  The other modifications results in the use of the built-in quake functions,
  there is no change in moving behavior (reinventing the wheel is a lot of
  "fun" and a BIG waste of time ;-))*/
void Jet_ApplyJet( edict_t *ent, usercmd_t *ucmd )
{
  float	direction;
  vec3_t acc;
  vec3_t forward, right;
  int    i;
  gitem_t *vehicle;
	
  vehicle = FindItemByClassname("item_hover");

  /*clear gravity so we dont have to compensate it with the Boosters*/
  if(!(ent->client->pers.inventory[ITEM_INDEX(vehicle)]))	
	ent->client->ps.pmove.gravity = 0;
  else //make hovercraft fall quicker
	ent->client->ps.pmove.gravity = sv_gravity->value * 4;

  /*calculate the direction vectors dependent on viewing direction
    (length of the vectors forward/right is always 1, the coordinates of
    the vectors are values of how much youre looking in a specific direction
    [if youre looking up to the top, the x/y values are nearly 0 the
    z value is nearly 1])*/
  AngleVectors( ent->client->v_angle, forward, right, NULL );

  /*Run jet only 10 times a second so movement dont depends on fps
    because ClientThink is called as often as possible
    (fps<10 still is a problem ?)*/
  if ( ent->client->Jet_next_think <= level.framenum )
  {
    ent->client->Jet_next_think = level.framenum + 1;

    /*clear acceleration-vector*/
    VectorClear( acc );

    /*if we are moving forward or backward add MainBooster acceleration
      (60)*/
    if ( ucmd->forwardmove )
    {
      /*are we accelerating backward or forward?*/
      direction = (ucmd->forwardmove<0) ? -1.0 : 1.0;

      /*add the acceleration for each direction*/
	  if(!(ent->client->pers.inventory[ITEM_INDEX(vehicle)])) {
		  acc[0] += direction * forward[0] * 60;
		  acc[1] += direction * forward[1] * 60;	
		  acc[2] += direction * forward[2] * 60;
	  }
	  else {
		  acc[0] += direction * forward[0] * 120;
		  acc[1] += direction * forward[1] * 120;	
	  }
    }

    /*if we sidestep add Left-Right-Booster acceleration (40)*/
    if ( ucmd->sidemove )
    {
      /*are we accelerating left or right*/
      direction = (ucmd->sidemove<0) ? -1.0 : 1.0;

      /*add only to x and y acceleration*/
      acc[0] += right[0] * direction * 40;
      acc[1] += right[1] * direction * 40;
    }

    /*if we crouch or jump add Up-Down-Booster acceleration (30)*/
	/*if we are in a hovercraft, we won't do this */
//	if(!(ent->client->pers.inventory[ITEM_INDEX(vehicle)])) {
		if ( ucmd->upmove )
		  acc[2] += ucmd->upmove > 0 ? 30 : -30;
//	}

    /*now apply some friction dependent on velocity (higher velocity results
      in higher friction), without acceleration this will reduce the velocity
      to 0 in a few steps*/
    ent->velocity[0] += -(ent->velocity[0]/6.0);
    ent->velocity[1] += -(ent->velocity[1]/6.0);
	ent->velocity[2] += -(ent->velocity[2]/7.0);

    /*then accelerate with the calculated values. If the new acceleration for
      a direction is smaller than an earlier, the friction will reduce the speed
      in that direction to the new value in a few steps, so if youre flying
      curves or around corners youre floating a little bit in the old direction*/
    VectorAdd( ent->velocity, acc, ent->velocity );

    /*round velocitys (is this necessary?)*/
    ent->velocity[0] = (float)((int)(ent->velocity[0]*8))/8;
    ent->velocity[1] = (float)((int)(ent->velocity[1]*8))/8;
	ent->velocity[2] = (float)((int)(ent->velocity[2]*8))/8;

    /*Bound velocitys so that friction and acceleration dont need to be
      synced on maxvelocitys*/
    for ( i=0 ; i<2 ; i++) /*allow z-velocity to be greater*/
    {
		if(!(ent->client->pers.inventory[ITEM_INDEX(vehicle)])) {
		  if (ent->velocity[i] > 300)
			ent->velocity[i] = 300;
		  else if (ent->velocity[i] < -300)
			ent->velocity[i] = -300;
		}
		else {
		  if (ent->velocity[i] > 600)
			ent->velocity[i] = 600;
		  else if (ent->velocity[i] < -600)
			ent->velocity[i] = -600;
		}
    }

    /*add some gentle up and down when idle (not accelerating)*/
    if( VectorLength(acc) == 0 )
      Jet_ApplyLifting( ent );

  }//if ( ent->client->Jet_next_think...

  /*add rolling when we fly curves or boost left/right*/
  Jet_ApplyRolling( ent, right );

  /*last but not least add some smoke*/
  Jet_ApplySparks( ent );

}

void Use_Jet ( edict_t *ent)
{
    
    /*jetpack in inventory but no fuel time? must be one of the
      give all/give jetpack cheats, so put fuel in*/
    if ( ent->client->Jet_remaining == 0 )
      ent->client->Jet_remaining = 700;  

    if ( Jet_Active(ent) ) 
      ent->client->Jet_framenum = 0; 
    else
      ent->client->Jet_framenum = level.framenum + ent->client->Jet_remaining;
    
    gi.sound( ent, CHAN_ITEM, gi.soundindex("vehicles/got_in.wav"), 0.8, ATTN_NORM, 0 );
}
static void VehicleThink(edict_t *ent)
{
	ent->nextthink = level.time + FRAMETIME;
}
void VehicleSetup (edict_t *ent)
{
	trace_t		tr;
	vec3_t		dest;
	float		*v;

	v = tv(-64,-64,-24);
	VectorCopy (v, ent->mins);
	v = tv(64,64,64);
	VectorCopy (v, ent->maxs);

	if (ent->model)
		gi.setmodel (ent, ent->model);
	else
		gi.setmodel (ent, ent->item->world_model);

	if(!strcmp(ent->classname, "item_bomber"))
		ent->s.modelindex3 = gi.modelindex("vehicles/bomber/helmet.md2");

	if(!strcmp(ent->classname, "item_hover"))
		ent->s.modelindex3 = gi.modelindex("vehicles/hover/flames.md2");

	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;  
	ent->touch = Touch_Item;

	v = tv(0,0,-128);
	VectorAdd (ent->s.origin, v, dest);

	tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid)
	{
		gi.dprintf ("VehicleSetup: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	VectorCopy (tr.endpos, ent->s.origin);

	gi.linkentity (ent);

	ent->nextthink = level.time + FRAMETIME;
	ent->think = VehicleThink;
}

static void VehicleDropTouch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	//owner (who dropped us) can't touch for two secs
	if (other == ent->owner && 
		ent->nextthink - level.time > 2)
		return;

	Touch_Item (ent, other, plane, surf);
}

static void VehicleDropThink(edict_t *ent)
{
	//let it hang around for awhile
	ent->nextthink = level.time + 20;
	ent->think = G_FreeEdict;
}

void VehicleDeadDrop(edict_t *self)
{
	edict_t *dropped;
	gitem_t *vehicle;

	dropped = NULL;
	
	vehicle = FindItemByClassname("item_bomber"); 
	
	if (self->client->pers.inventory[ITEM_INDEX(vehicle)]) {
		dropped = Drop_Item(self, vehicle);
		self->client->pers.inventory[ITEM_INDEX(vehicle)] = 0;
		safe_bprintf(PRINT_HIGH, "Bomber is abandoned!\n");
	}

	if (dropped) {
		dropped->think = VehicleDropThink;
		dropped->nextthink = level.time + 5;
		dropped->touch = VehicleDropTouch;
		dropped->s.frame = 0;
		return;
	}
	//didn't find a bomber, try a strafer
	vehicle = FindItemByClassname("item_strafer"); 
	
	if (self->client->pers.inventory[ITEM_INDEX(vehicle)]) {
		dropped = Drop_Item(self, vehicle);
		self->client->pers.inventory[ITEM_INDEX(vehicle)] = 0;
		safe_bprintf(PRINT_HIGH, "Strafer is abandoned!\n");
	}

	if (dropped) {
		dropped->think = VehicleDropThink;
		dropped->nextthink = level.time + 5;
		dropped->touch = VehicleDropTouch;
		dropped->s.frame = 0;
		return;
	}
	//didn't find a strafer, try a hovercraft
	vehicle = FindItemByClassname("item_hover"); 
	
	if (self->client->pers.inventory[ITEM_INDEX(vehicle)]) {
		dropped = Drop_Item(self, vehicle);
		self->client->pers.inventory[ITEM_INDEX(vehicle)] = 0;
		safe_bprintf(PRINT_HIGH, "Hovercraft is abandoned!\n");
	}

	if (dropped) {
		dropped->think = VehicleDropThink;
		dropped->nextthink = level.time + 5;
		dropped->touch = VehicleDropTouch;
		dropped->s.frame = 0;
		return;
	}

}
void Reset_player(edict_t *ent)
{
	char		userinfo[MAX_INFO_STRING];
	int		i, done;
	char playermodel[MAX_OSPATH] = " ";
	char playerskin[MAX_INFO_STRING] = " ";
	char modelpath[MAX_OSPATH] = " ";
	FILE *file;
	char *s;

	//set everything back
	if(instagib->value)
		ent->client->newweapon = FindItem("Alien Disruptor");
	else if(rocket_arena->value)
		ent->client->newweapon = FindItem("Rocket Launcher");
	else
		ent->client->newweapon = FindItem("blaster");
		  		
	memcpy (userinfo, ent->client->pers.userinfo, sizeof(userinfo));
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
		  
	ent->s.modelindex = 255;

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
	if(!strcmp(playermodel, "war")) //special case
	{
	   	ent->s.modelindex4 = gi.modelindex("players/war/weapon.md2");
		ent->s.modelindex2 = 0;
	}
	else if(!strcmp(playermodel, "brainlet"))	
		ent->s.modelindex4 = gi.modelindex("players/brainlet/gunrack.md2"); //brainlets have a mount
	
	ent->in_vehicle = false;
}
qboolean Leave_vehicle(edict_t *ent, gitem_t *item) 
{

	Reset_player(ent);
	
	ent->client->Jet_framenum = level.framenum;
		
	ent->client->pers.inventory[ITEM_INDEX(item)] = 0;

	gi.sound( ent, CHAN_ITEM, gi.soundindex("vehicles/got_in.wav"), 0.8, ATTN_NORM, 0 );

	Drop_Item (ent, item);

	safe_bprintf(PRINT_HIGH, "Vehicle has been dropped!\n");
	return true;
}

qboolean Get_in_vehicle (edict_t *ent, edict_t *other)
{
	
	gitem_t *vehicle; 

	float		*v;

	vehicle = NULL;

    if(other->in_vehicle) //already in a vehicle
		return false;

	vehicle = FindItemByClassname(ent->classname);

	//put him in the vehicle
	if(!strcmp(ent->classname, "item_bomber")) {
		other->s.modelindex = gi.modelindex("vehicles/bomber/tris.md2");
		other->s.modelindex2 = 0;
		other->s.modelindex3 = gi.modelindex("vehicles/bomber/helmet.md2");
		other->s.modelindex4 = 0;
	}
	else if(!strcmp(ent->classname, "item_hover")) {
		other->s.modelindex = gi.modelindex("vehicles/hover/tris.md2");
		other->s.modelindex2 = 0;
		other->s.modelindex3 = gi.modelindex("vehicles/hover/flames.md2");
		other->s.modelindex4 = 0;
	}
	else {
		other->s.modelindex = gi.modelindex("vehicles/strafer/tris.md2");
		other->s.modelindex2 = 0;
		other->s.modelindex3 = 0;
		other->s.modelindex4 = 0;
	}
	other->in_vehicle = true;
	other->client->Jet_remaining = 500;

	v = tv(-64,-64,-24);
	VectorCopy (v,other->mins);
	v = tv(64,64,64);
	VectorCopy (v, other->maxs);
	other->s.origin[2] +=24;
	
	other->client->pers.inventory[ITEM_INDEX(vehicle)] = 1;
	other->client->newweapon = ent->item;
	
	if (!(ent->spawnflags & DROPPED_ITEM)) {
		SetRespawn (ent, 60);
	}
	
	Use_Jet(other);

	ent->owner = other;

	return true;
}
