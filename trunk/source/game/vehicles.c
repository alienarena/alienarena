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
