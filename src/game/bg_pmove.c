/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

pmove_t     *pm;
pml_t       pml;

// movement parameters
float pm_stopspeed = 100.0f;
float pm_duckScale = 0.25f;
float pm_swimScale = 0.50f;
float pm_wadeScale = 0.70f;

float pm_accelerate = 10.0f;
float pm_wateraccelerate = 4.0f;
float pm_flyaccelerate = 4.0f;

float pm_friction = 6.0f;
float pm_waterfriction = 1.0f;
float pm_flightfriction = 4.0f;
float pm_spitfire_flyfriction = 3.5f;
float pm_spitfire_flywalkfriction = 5.0f;
float pm_spectatorfriction = 5.0f;

//TODO: Marauder doesn't have a double jump since I removed it because it'll stack heavily when wall climbing.
//It doesn't need it though, but may feel akward
float cpm_pm_jump_z = 0.5; //CPM: 100/270 (normal jumpvel is 270, doublejump default 100) = 0.37037
                           //However, that value feels too low and nothing much is acomplished since
                           //tremulous human jump default is only 240 ups, meaning half height.

//Clip time for maintaining velocity when clipping. For marauder it's actually equal to the jump
//repeat rate so we can't modify it to cpm's default. CPM Default is 400 I think, but I placed 200
//becausemarauder has 200, it's a bit tricky to get used to two different values at once.
float pm_cliptime = 200;

int   c_pmove = 0;

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent )
{
  BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum )
{
  int   i;

  if( entityNum == ENTITYNUM_WORLD )
    return;

  if( pm->numtouch == MAXTOUCH )
    return;

  // see if it is already added
  for( i = 0 ; i < pm->numtouch ; i++ )
  {
    if( pm->touchents[ i ] == entityNum )
      return;
  }

  // add it
  pm->touchents[ pm->numtouch ] = entityNum;
  pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
void PM_StartTorsoAnim( int anim )
{
  if( PM_Paralyzed( pm->ps->pm_type ) )
    return;

  pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
    | anim;
}

/*
===================
PM_StartWeaponAnim
===================
*/
static void PM_StartWeaponAnim( int anim )
{
  if( PM_Paralyzed( pm->ps->pm_type ) )
    return;

  pm->ps->weaponAnim = ( ( pm->ps->weaponAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
    | anim;
}

/*
===================
PM_StartLegsAnim
===================
*/
static void PM_StartLegsAnim( int anim )
{
  if( PM_Paralyzed( pm->ps->pm_type ) &&
      pm->ps->pm_type != PM_EVOLVING )
    return;

  //legsTimer is clamped too tightly for nonsegmented models
  if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
  {
    if( pm->ps->legsTimer > 0 )
      return;   // a high priority animation is running
  }
  else
  {
    if( pm->ps->torsoTimer > 0 )
      return;   // a high priority animation is running
  }

  pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
    | anim;
}

/*
===================
PM_ContinueLegsAnim
===================
*/
static void PM_ContinueLegsAnim( int anim )
{
  if( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim )
    return;

  //legsTimer is clamped too tightly for nonsegmented models
  if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
  {
    if( pm->ps->legsTimer > 0 )
      return;   // a high priority animation is running
  }
  else
  {
    if( pm->ps->torsoTimer > 0 )
      return;   // a high priority animation is running
  }

  PM_StartLegsAnim( anim );
}

/*
===================
PM_ContinueTorsoAnim
===================
*/
static void PM_ContinueTorsoAnim( int anim )
{
  if( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim )
    return;

  if( pm->ps->torsoTimer > 0 )
    return;   // a high priority animation is running

  PM_StartTorsoAnim( anim );
}

/*
===================
PM_ContinueWeaponAnim
===================
*/
static void PM_ContinueWeaponAnim( int anim )
{
  if( ( pm->ps->weaponAnim & ~ANIM_TOGGLEBIT ) == anim )
    return;

  PM_StartWeaponAnim( anim );
}

/*
===================
PM_ForceLegsAnim
===================
*/
static void PM_ForceLegsAnim( int anim )
{
  //legsTimer is clamped too tightly for nonsegmented models
  if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
    pm->ps->legsTimer = 0;
  else
    pm->ps->torsoTimer = 0;

  PM_StartLegsAnim( anim );
}


/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out )
{
  float t = -DotProduct( in, normal );
  VectorMA( in, t, normal, out );
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void )
{
  vec3_t   vec;
  float    *vel;
  float    speed, newspeed, control;
  float    drop, modifier;
  qboolean groundFriction = qfalse;

  vel = pm->ps->velocity;

  // make sure vertical velocity is NOT set to zero when wall climbing
  VectorCopy( vel, vec );
  if( pml.walking && !( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
    vec[ 2 ] = 0; // ignore slope movement

  speed = VectorLength( vec );

  if( pm->ps->pm_type == PM_SPITFIRE_FLY && speed < 20 &&
      ( pm->cmd.buttons & BUTTON_WALKING ) &&
      !(pm->cmd.forwardmove) && !(pm->cmd.rightmove) && !(pm->cmd.upmove) )
  {
    vel[ 0 ] = 0;
    vel[ 1 ] = 0;
    vel[ 2 ] = 0;
    return;
  }
  else if( speed < 1 )
  {
    vel[ 0 ] = 0;
    vel[ 1 ] = 0;   // allow sinking underwater
    // FIXME: still have z friction underwater?
    return;
  }

  drop = 0;

  // apply ground friction
  if( pm->waterlevel <= 1 )
  {
    if( ( pml.walking || pml.ladder ) && !( pml.groundTrace.surfaceFlags & SURF_SLICK ) )
    {
      // if getting knocked back, no friction
      if( !( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) )
      {
        float stopSpeed = BG_Class( pm->ps->stats[ STAT_CLASS ] )->stopSpeed;
        float friction = BG_Class( pm->ps->stats[ STAT_CLASS ] )->friction;

        control = speed < stopSpeed ? stopSpeed : speed;
        drop += control * friction * pml.frametime;
        groundFriction = qtrue;
      }
    }
  }

  // apply water friction even if just wading
  if( pm->waterlevel )
    drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;

  // apply flying friction
  if( pm->ps->pm_type == PM_JETPACK )
    drop += speed * pm_flightfriction * pml.frametime;

  if( pm->ps->pm_type == PM_SPECTATOR )
    drop += speed * pm_spectatorfriction * pml.frametime;

  if( pm->ps->pm_type == PM_SPITFIRE_FLY &&
      !( pm->cmd.upmove < 0 &&
         !( pm->cmd.buttons & BUTTON_WALKING ) &&
         pm->ps->groundEntityNum == ENTITYNUM_NONE &&
         ( pm->ps->commandTime > pm->pmext->pouncePayloadTime +
                                 SPITFIRE_PAYLOAD_DISCHARGE_TIME ) ) )
  {
    modifier = ( pm->cmd.buttons & BUTTON_WALKING ) ? pm_spitfire_flywalkfriction : pm_spitfire_flyfriction;
    drop += speed * modifier * pml.frametime;
  }

  // scale the velocity
  newspeed = speed - drop;
  if( newspeed < 0 )
    newspeed = 0;

  newspeed /= speed;

  vel[ 0 ] = vel[ 0 ] * newspeed;
  vel[ 1 ] = vel[ 1 ] * newspeed;
  // Don't reduce upward velocity while using a jetpack
  if( !( pm->ps->pm_type == PM_JETPACK &&
         vel[2] > 0 &&
         !groundFriction &&
         !pm->waterlevel ) )
    vel[ 2 ] = vel[ 2 ] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel )
{
#if 1
  // q2 style
  int     i;
  float   addspeed, accelspeed, currentspeed;

  currentspeed = DotProduct( pm->ps->velocity, wishdir );
  addspeed = wishspeed - currentspeed;
  if( addspeed <= 0 )
    return;

  accelspeed = accel * pml.frametime * wishspeed;
  if( accelspeed > addspeed )
    accelspeed = addspeed;

  for( i = 0; i < 3; i++ )
    pm->ps->velocity[ i ] += accelspeed * wishdir[ i ];
#else
  // proper way (avoids strafe jump maxspeed bug), but feels bad
  vec3_t    wishVelocity;
  vec3_t    pushDir;
  float     pushLen;
  float     canPush;

  VectorScale( wishdir, wishspeed, wishVelocity );
  VectorSubtract( wishVelocity, pm->ps->velocity, pushDir );
  pushLen = VectorNormalize( pushDir );

  canPush = accel * pml.frametime * wishspeed;
  if( canPush > pushLen )
    canPush = pushLen;

  VectorMA( pm->ps->velocity, canPush, pushDir, pm->ps->velocity );
#endif
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd, qboolean zFlight )
{
  int         max;
  float       total;
  float       scale;
  float       modifier = 1.0f;

  if ( BG_ClassHasAbility(pm->ps->stats[STAT_CLASS], SCA_STAMINA)
    && pm->ps->pm_type == PM_NORMAL )
  {
    qboolean wasSprinting;
    qboolean sprint;
    wasSprinting = sprint = pm->ps->stats[ STAT_STATE ] & SS_SPEEDBOOST;

    if( pm->ps->persistant[ PERS_STATE ] & PS_SPRINTTOGGLE )
    {
      // walking or crouching toggles off sprint
      if( ( pm->ps->pm_flags & PMF_SPRINTHELD ) &&
          ( ( cmd->buttons & BUTTON_WALKING ) || ( pm->cmd.upmove < 0 ) ) )
      {
        sprint = qfalse;
        pm->ps->pm_flags &= ~PMF_SPRINTHELD;
      }
      else if( ( cmd->buttons & BUTTON_SPRINT ) &&
               !( pm->ps->pm_flags & PMF_SPRINTHELD ) &&
               !( cmd->buttons & BUTTON_WALKING ) )
      {
        sprint = qtrue;
        pm->ps->pm_flags |= PMF_SPRINTHELD;
      }
    }
    else if ( pm->cmd.upmove >= 0 )
      sprint = cmd->buttons & BUTTON_SPRINT;

    if( sprint )
      pm->ps->stats[ STAT_STATE ] |= SS_SPEEDBOOST;
    else if( wasSprinting && !sprint )
      pm->ps->stats[ STAT_STATE ] &= ~SS_SPEEDBOOST;

    // Walk overrides sprint. We keep the state that we want to be sprinting
    //  (above), but don't apply the modifier, and in g_active we skip taking
    //  the stamina too.
    if( sprint && !( cmd->buttons & BUTTON_WALKING ) )
      modifier *= HUMAN_SPRINT_MODIFIER;
    else
      modifier *= HUMAN_JOG_MODIFIER;

    if( cmd->forwardmove < 0 )
    {
      //can't run backwards
      modifier *= HUMAN_BACK_MODIFIER;
    }
    else if( cmd->rightmove )
    {
      //can't move that fast sideways
      modifier *= HUMAN_SIDE_MODIFIER;
    }

    if( !zFlight )
    {
      //must have stamina to jum
      if( pm->ps->stats[ STAT_STAMINA ] < STAMINA_SLOW_LEVEL + STAMINA_JUMP_TAKE )
        cmd->upmove = 0;
    }

    //slow down once stamina depletes
    if( pm->ps->stats[ STAT_STAMINA ] <= STAMINA_SLOW_LEVEL )
      modifier *= (float)( pm->ps->stats[ STAT_STAMINA ] + STAMINA_MAX ) / (float)(STAMINA_SLOW_LEVEL + STAMINA_MAX);

    if( pm->ps->stats[ STAT_STATE ] & SS_CREEPSLOWED )
    {
      if( BG_InventoryContainsUpgrade( UP_LIGHTARMOUR, pm->ps->stats ) ||
          BG_InventoryContainsUpgrade( UP_BATTLESUIT, pm->ps->stats ) )
        modifier *= CREEP_ARMOUR_MODIFIER;
      else
        modifier *= CREEP_MODIFIER;
    }
    if( pm->ps->eFlags & EF_POISONCLOUDED )
    {
      if( BG_InventoryContainsUpgrade( UP_LIGHTARMOUR, pm->ps->stats ) ||
          BG_InventoryContainsUpgrade( UP_BATTLESUIT, pm->ps->stats ) )
        modifier *= PCLOUD_ARMOUR_MODIFIER;
      else
        modifier *= PCLOUD_MODIFIER;
    }
  }

  if( pm->ps->weapon == WP_ALEVEL4 && pm->ps->pm_flags & PMF_CHARGE )
    modifier *= 1.0f + ( pm->ps->stats[ STAT_MISC ] *
                         ( LEVEL4_TRAMPLE_SPEED - 1.0f ) /
                         LEVEL4_TRAMPLE_DURATION );					 

  //slow player if charging up for a pounce
  if( cmd->buttons & BUTTON_ATTACK2 )
  {
    if( pm->ps->weapon == WP_ALEVEL3 || pm->ps->weapon == WP_ALEVEL3_UPG )
      modifier *= LEVEL3_POUNCE_SPEED_MOD;
    else if ( pm->ps->weapon == WP_ASPITFIRE )
      modifier *= SPITFIRE_POUNCE_SPEED_MOD;
  }

  // the spitfire strafes and moves backwards slower when flying
  if( pm->ps->weapon == WP_ASPITFIRE &&
      pm->ps->groundEntityNum == ENTITYNUM_NONE )
  {
    cmd->rightmove *= SPITFIRE_SIDE_MODIFIER;
    if( cmd->forwardmove < 0 )
      cmd->forwardmove *= SPITFIRE_BACK_MODIFIER;
  }

  //slow the player if slow locked
  if( pm->ps->stats[ STAT_STATE ] & SS_SLOWLOCKED )
    modifier *= ABUILDER_BLOB_SPEED_MOD;

  if( pm->ps->pm_type == PM_GRABBED )
    modifier = 0.0f;

  max = abs( cmd->forwardmove );
  if( abs( cmd->rightmove ) > max )
    max = abs( cmd->rightmove );
  total = cmd->forwardmove * cmd->forwardmove + cmd->rightmove * cmd->rightmove;

  if( zFlight )
  {
    if( abs( cmd->upmove ) > max )
      max = abs( cmd->upmove );
    total += cmd->upmove * cmd->upmove;
  }

  if( !max )
    return 0;

  total = sqrt( total );

  scale = (float)pm->ps->speed * max / ( 127.0 * total ) * modifier;

  return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs relative
to the facing dir
================
*/
static void PM_SetMovementDir( void )
{
  if( pm->cmd.forwardmove || pm->cmd.rightmove )
  {
    if( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 )
      pm->ps->movementDir = 0;
    else if( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 )
      pm->ps->movementDir = 1;
    else if( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 )
      pm->ps->movementDir = 2;
    else if( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 )
      pm->ps->movementDir = 3;
    else if( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 )
      pm->ps->movementDir = 4;
    else if( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 )
      pm->ps->movementDir = 5;
    else if( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 )
      pm->ps->movementDir = 6;
    else if( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 )
      pm->ps->movementDir = 7;
  }
  else
  {
    // if they aren't actively going directly sideways,
    // change the animation to the diagonal so they
    // don't stop too crooked
    if( pm->ps->movementDir == 2 )
      pm->ps->movementDir = 1;
    else if( pm->ps->movementDir == 6 )
      pm->ps->movementDir = 7;
  }
}


/*
=============
PM_CheckCharge
=============
*/
static void PM_CheckCharge( void )
{
  if( pm->ps->weapon != WP_ALEVEL4 )
    return;

  if( pm->cmd.buttons & BUTTON_ATTACK2 &&
      !( pm->ps->stats[ STAT_STATE ] & SS_CHARGING ) )
  {
    pm->ps->pm_flags &= ~PMF_CHARGE;
    return;
  }

  if( pm->ps->stats[ STAT_MISC ] > 0 )
    pm->ps->pm_flags |= PMF_CHARGE;
  else
    pm->ps->pm_flags &= ~PMF_CHARGE;
}

/*
=============
PM_CheckPounce
=============
*/
static qboolean PM_CheckPounce( void )
{
  int jumpMagnitude;

  if( pm->ps->weapon != WP_ALEVEL3 &&
      pm->ps->weapon != WP_ALEVEL3_UPG )
    return qfalse;

  // We were pouncing, but we've landed
  if( pm->ps->groundEntityNum != ENTITYNUM_NONE &&
      ( pm->ps->pm_flags & PMF_CHARGE ) )
  {
    pm->ps->pm_flags &= ~PMF_CHARGE;
    pm->ps->weaponTime += LEVEL3_POUNCE_REPEAT;
    return qfalse;
  }

  // We're building up for a pounce
  if( pm->cmd.buttons & BUTTON_ATTACK2 )
  {
    pm->ps->pm_flags &= ~PMF_CHARGE;
    return qfalse;
  }

  // Can't start a pounce
  if( ( pm->ps->pm_flags & PMF_CHARGE ) ||
      pm->ps->stats[ STAT_MISC ] < LEVEL3_POUNCE_TIME_MIN ||
      pm->ps->groundEntityNum == ENTITYNUM_NONE )
    return qfalse;

  // Give the player forward velocity and simulate a jump
  pml.groundPlane = qfalse;
  pml.walking = qfalse;
  pm->ps->pm_flags |= PMF_CHARGE;
  pm->ps->groundEntityNum = ENTITYNUM_NONE;
  if( pm->ps->weapon == WP_ALEVEL3 )
    jumpMagnitude = pm->ps->stats[ STAT_MISC ] *
                    LEVEL3_POUNCE_JUMP_MAG / LEVEL3_POUNCE_TIME;
  else
    jumpMagnitude = pm->ps->stats[ STAT_MISC ] *
                    LEVEL3_POUNCE_JUMP_MAG_UPG / LEVEL3_POUNCE_TIME_UPG;
  VectorMA( pm->ps->velocity, jumpMagnitude, pml.forward, pm->ps->velocity );
  PM_AddEvent( EV_JUMP );

  // Play jumping animation
  if( pm->cmd.forwardmove >= 0 )
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
      PM_ForceLegsAnim( LEGS_JUMP );
    else
      PM_ForceLegsAnim( NSPA_JUMP );

    pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
  }
  else
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
      PM_ForceLegsAnim( LEGS_JUMPB );
    else
      PM_ForceLegsAnim( NSPA_JUMPBACK );

    pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
  }

  pm->pmext->pouncePayload = pm->ps->stats[ STAT_MISC ];
  pm->ps->stats[ STAT_MISC ] = 0;

  return qtrue;
}

/*
=============
PM_CheckAirPounce
For Spitfire only
=============
*/
static qboolean PM_CheckAirPounce( void )
{
  int jumpMagnitude;
  
  if( pm->ps->weapon != WP_ASPITFIRE )
    return qfalse;

  // We're building up for a pounce
  if( pm->cmd.buttons & BUTTON_ATTACK2)
  {
    pm->ps->pm_flags &= ~PMF_CHARGE;
    return qfalse;
  }
  
  // Can't start a pounce
  if( pm->ps->weapon == WP_ASPITFIRE )
  {
    if( pm->ps->stats[ STAT_MISC ] < SPITFIRE_POUNCE_TIME_MIN)
      return qfalse;
  }

  // Give the player forward velocity and simulate a jump
  pml.groundPlane = qfalse;
  pml.walking = qfalse;
  pm->ps->pm_flags |= PMF_CHARGE;
  
  
  if( pm->ps->weapon == WP_ASPITFIRE )
    jumpMagnitude = pm->ps->stats[ STAT_MISC ] * SPITFIRE_POUNCE_JUMP_MAG / SPITFIRE_POUNCE_TIME;
  else jumpMagnitude = 0;

  VectorMA( pm->ps->velocity, jumpMagnitude, pml.forward, pm->ps->velocity ); 
  PM_AddEvent( EV_AIRPOUNCE );
  pm->pmext->pouncePayloadTime = pm->ps->commandTime;

  // Play jumping animation
  if( pm->cmd.forwardmove >= 1 )
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
      PM_ForceLegsAnim( LEGS_JUMP );
    else
      PM_ForceLegsAnim( NSPA_JUMP );
      pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
  }
  else
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
      PM_ForceLegsAnim( NSPA_ATTACK1 );
    else
	  PM_ForceLegsAnim( NSPA_ATTACK1 );
      pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
  }
  
  pm->pmext->pouncePayload = pm->ps->stats[ STAT_MISC ];
  pm->ps->stats[ STAT_MISC ] = 0;
  if( BG_ClassHasAbility( pm->ps->stats[STAT_CLASS],  SCA_CHARGE_STAMINA ) )
    pm->ps->stats[ STAT_STAMINA ] -= pm->pmext->pouncePayload;
  return qtrue;
}

/*
=============
PM_CheckWallJump
=============
*/
static qboolean PM_CheckWallJump( void )
{
  vec3_t  dir, forward, right, movedir, point;
  vec3_t  refNormal = { 0.0f, 0.0f, 1.0f };
  float   normalFraction = 1.5f;
  float   cmdFraction = 1.0f;
  float   upFraction = 1.5f;
  trace_t trace;

  if( pm->waterlevel )
    return qfalse;

  if( !( BG_Class( pm->ps->stats[ STAT_CLASS ] )->abilities & SCA_WALLJUMPER ) )
    return qfalse;

  ProjectPointOnPlane( movedir, pml.forward, refNormal );
  VectorNormalize( movedir );

  if( pm->cmd.forwardmove < 0 )
    VectorNegate( movedir, movedir );

  //allow strafe transitions
  if( pm->cmd.rightmove )
  {
    VectorCopy( pml.right, movedir );

    if( pm->cmd.rightmove < 0 )
      VectorNegate( movedir, movedir );
  }

  //trace into direction we are moving
  VectorMA( pm->ps->origin, 0.25f, movedir, point );
  pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

  if( trace.fraction < 1.0f &&
      !( trace.surfaceFlags & ( SURF_SKY | SURF_SLICK ) ) &&
      trace.plane.normal[ 2 ] < MIN_WALK_NORMAL )
  {
    if( !VectorCompare( trace.plane.normal, pm->ps->grapplePoint ) )
    {
      VectorCopy( trace.plane.normal, pm->ps->grapplePoint );
    }
  }
  else
    return qfalse;

  if( pm->ps->pm_flags & PMF_RESPAWNED )
    return qfalse;    // don't allow jump until all buttons are up

  if( pm->cmd.upmove < 10 )
    // not holding jump
    return qfalse;

  if( pm->ps->pm_flags & PMF_TIME_WALLJUMP )
    return qfalse;

  // must wait for jump to be released
  if( pm->ps->pm_flags & PMF_JUMP_HELD &&
      pm->ps->grapplePoint[ 2 ] == 1.0f )
  {
    return qfalse;
  }

  pm->ps->pm_flags |= PMF_TIME_WALLJUMP;
  pm->ps->pm_time = 200;

  pml.groundPlane = qfalse;   // jumping away
  pml.walking = qfalse;
  pm->ps->pm_flags |= PMF_JUMP_HELD;
  pm->ps->persistant[PERS_JUMPTIME] = 0;
  pm->ps->pm_flags |= PMF_JUMPING;

  pm->ps->groundEntityNum = ENTITYNUM_NONE;

  ProjectPointOnPlane( forward, pml.forward, pm->ps->grapplePoint );
  ProjectPointOnPlane( right, pml.right, pm->ps->grapplePoint );

  VectorScale( pm->ps->grapplePoint, normalFraction, dir );

  if( pm->cmd.forwardmove > 0 )
    VectorMA( dir, cmdFraction, forward, dir );
  else if( pm->cmd.forwardmove < 0 )
    VectorMA( dir, -cmdFraction, forward, dir );

  if( pm->cmd.rightmove > 0 )
    VectorMA( dir, cmdFraction, right, dir );
  else if( pm->cmd.rightmove < 0 )
    VectorMA( dir, -cmdFraction, right, dir );

  VectorMA( dir, upFraction, refNormal, dir );
  VectorNormalize( dir );

  VectorMA( pm->ps->velocity, BG_Class( pm->ps->stats[ STAT_CLASS ] )->jumpMagnitude,
            dir, pm->ps->velocity );

  //for a long run of wall jumps the velocity can get pretty large, this caps it
  if( VectorLength( pm->ps->velocity ) > LEVEL2_WALLJUMP_MAXSPEED )
  {
    VectorNormalize( pm->ps->velocity );
    VectorScale( pm->ps->velocity, LEVEL2_WALLJUMP_MAXSPEED, pm->ps->velocity );
  }

  PM_AddEvent( EV_JUMP );

  if( pm->cmd.forwardmove >= 0 )
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
      PM_ForceLegsAnim( LEGS_JUMP );
    else
      PM_ForceLegsAnim( NSPA_JUMP );

    pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
  }
  else
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
      PM_ForceLegsAnim( LEGS_JUMPB );
    else
      PM_ForceLegsAnim( NSPA_JUMPBACK );

    pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
  }

  return qtrue;
}

/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( void )
{
  vec3_t normal;
  int jumpvel;

  if( pm->ps->groundEntityNum == ENTITYNUM_NONE )
    return qfalse;

  if( BG_Class( pm->ps->stats[ STAT_CLASS ] )->jumpMagnitude == 0.0f )
    return qfalse;

  //can't jump and pounce at the same time
  if( ( pm->ps->weapon == WP_ALEVEL3 ||
        pm->ps->weapon == WP_ALEVEL3_UPG ) &&
      pm->ps->stats[ STAT_MISC ] > 0 )
  {
    // disable bunny hop
    pm->ps->pm_flags &= ~PMF_ALL_HOP_FLAGS;
    return qfalse;
  }

  //can't jump and charge at the same time
  if( ( pm->ps->weapon == WP_ALEVEL4 ) &&
      pm->ps->stats[ STAT_MISC ] > 0 )
  {
    // disable bunny hop
    pm->ps->pm_flags &= ~PMF_ALL_HOP_FLAGS;
    return qfalse;
  }

  if( BG_ClassHasAbility(pm->ps->stats[STAT_CLASS], SCA_STAMINA) &&
      ( pm->ps->stats[ STAT_STAMINA ] < STAMINA_SLOW_LEVEL + STAMINA_JUMP_TAKE ) &&
      ( !BG_InventoryContainsUpgrade( UP_JETPACK, pm->ps->stats ) ||
        pm->ps->stats[ STAT_FUEL ] < JETPACK_ACT_BOOST_FUEL_USE ) )
  {
    // disable bunny hop
    pm->ps->pm_flags &= ~PMF_ALL_HOP_FLAGS;
    return qfalse;
  }

  //no bunny hopping off a dodge
  //SCA_DODGE? -vjr
  if( BG_ClassHasAbility(pm->ps->stats[STAT_CLASS], SCA_STAMINA) &&
      pm->ps->pm_time )
  {
    // disable bunny hop
    pm->ps->pm_flags &= ~PMF_ALL_HOP_FLAGS;
    return qfalse;
  }

  if( pm->ps->pm_flags & PMF_RESPAWNED )
    return qfalse;    // don't allow jump until all buttons are up

  if( pm->cmd.upmove < 10 )
    // not holding jump
    return qfalse;

  // can't jump whilst grabbed
  if( pm->ps->pm_type == PM_GRABBED )
  {
    // disable bunny hop
    pm->ps->pm_flags &= ~PMF_ALL_HOP_FLAGS;
    return qfalse;
  }

  if( !( pm->ps->pm_flags & PMF_JUMP_HELD ) )
  {
    if( !( pm->ps->pm_flags & PMF_JUMPING ) &&
        (pm->ps->pm_flags & PMF_HOPPED) )
      pm->ps->pm_flags |= PMF_BUNNY_HOPPING; // enable hunny hop
  } else if( pm->ps->persistant[PERS_JUMPTIME] < BUNNY_HOP_DELAY ||
             !(pm->ps->pm_flags & PMF_BUNNY_HOPPING) )
  {
    // don't bunnhy hop
    return qfalse;
  }

  //don't allow walljump for a short while after jumping from the ground
  if( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_WALLJUMPER ) )
  {
    pm->ps->pm_flags |= PMF_TIME_WALLJUMP;
    pm->ps->pm_time = 200;
  }

  pml.groundPlane = qfalse;   // jumping away
  pml.walking = qfalse;
  pm->ps->pm_flags |= PMF_JUMP_HELD;

  if( BG_ClassHasAbility(pm->ps->stats[STAT_CLASS], SCA_STAMINA) &&
      pm->humanStaminaMode )
    pm->ps->stats[ STAT_STAMINA ] -= STAMINA_JUMP_TAKE;

//Alright, let's start the jumping process.
  jumpvel = BG_Class( pm->ps->stats[ STAT_CLASS ] )->jumpMagnitude;

	 //ZdrytchX: check for double-jump
  if(pm->ps->stats[ STAT_STAMINA ] > 0 //derp doesn't have STAMINA_MIN_TO_JUMP :/
    || ( pm->ps->stats[ STAT_TEAM ] == TEAM_ALIENS
    && !BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_WALLJUMPER ) ) )
    //Trust me. You don't want marauders flying out of the map from 3 wall jumps.
		if (cpm_pm_jump_z)
    {
			if (pm->ps->persistant[PERS_JUMPTIME] < DOUBLE_JUMP_MAX_TIME)
      {
				jumpvel += (cpm_pm_jump_z * BG_Class( pm->ps->stats[ STAT_CLASS ] )->jumpMagnitude);
				//Adding requires me to multiply by class vel again
			}
			pm->ps->pm_time = pm_cliptime; //clip through walls with the same timer as walljump
	  }

  pm->ps->persistant[PERS_JUMPTIME] = 0;
  pm->ps->pm_flags |= PMF_JUMPING;

  pm->ps->groundEntityNum = ENTITYNUM_NONE;

  // jump away from wall
  BG_GetClientNormal( pm->ps, normal );

  if( pm->ps->velocity[ 2 ] < 0 )
    pm->ps->velocity[ 2 ] = 0;

  VectorMA( pm->ps->velocity, jumpvel,
            normal, pm->ps->velocity );

    PM_AddEvent( EV_JUMP );

  if( pm->cmd.forwardmove >= 0 )
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
      PM_ForceLegsAnim( LEGS_JUMP );
    else
      PM_ForceLegsAnim( NSPA_JUMP );

    pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
  }
  else
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
      PM_ForceLegsAnim( LEGS_JUMPB );
    else
      PM_ForceLegsAnim( NSPA_JUMPBACK );

    pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
  }

  return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean PM_CheckWaterJump( void )
{
  vec3_t  spot;
  int     cont;
  vec3_t  flatforward;
  vec3_t  mins, maxs;
  float   a, r;

  if( pm->ps->pm_time )
    return qfalse;

  if( pm->cmd.upmove < 10 )
    // not holding jump
    return qfalse;

  // check for water jump
  //if( pm->waterlevel != 2 )
  //  return qfalse;

  flatforward[ 0 ] = pml.forward[ 0 ];
  flatforward[ 1 ] = pml.forward[ 1 ];
  flatforward[ 2 ] = 0;
  VectorNormalize( flatforward );

  BG_ClassBoundingBox( pm->ps->stats[ STAT_CLASS ], mins, maxs, NULL, NULL, NULL );

  // bbox bottom
  spot[ 0 ] = pm->ps->origin[ 0 ];
  spot[ 1 ] = pm->ps->origin[ 1 ];
  spot[ 2 ] = pm->ps->origin[ 2 ] + mins[ 2 ];

  // project the flatforward vector onto the bbox (just a bit longer, so we'll actually hit a wall)
  // then add it to spot
  #define fmod(a,n) ((a)-(n)*floor((a)/(n)))
  a = pm->ps->viewangles[ YAW ] / ( 180.0f / M_PI );
  r = ( maxs[ 0 ] + 1 ) * 1.0f / cos( fmod( a+0.25f*M_PI, 0.5f*M_PI ) - 0.25f*M_PI );
  VectorMA( spot, r, flatforward, spot );

  cont = pm->pointcontents( spot, pm->ps->clientNum );

  if( !( cont & CONTENTS_SOLID ) )
    return qfalse;

  spot[ 2 ] = pm->ps->origin[ 2 ] + maxs[ 2 ];
  cont = pm->pointcontents( spot, pm->ps->clientNum );

  if( cont )
    return qfalse;

  // jump out of water
  VectorScale( pml.forward, 200, pm->ps->velocity );
  pm->ps->velocity[ 2 ] = 350;

  pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
  pm->ps->pm_time = 2000;

  return qtrue;
}


/*
==================
PM_CheckDodge

Checks the dodge key and starts a human dodge or sprint
==================
*/
/* Disable dodge
static qboolean PM_CheckDodge( void )
{
  vec3_t right, forward, velocity = { 0.0f, 0.0f, 0.0f };
  float jump, sideModifier;
  int i;

  if( !BG_ClassHasAbility(pm->ps->stats[STAT_CLASS], SCA_STAMINA) )
    return qfalse;

  // Landed a dodge
  if( ( pm->ps->pm_flags & PMF_CHARGE ) &&
      pm->ps->groundEntityNum != ENTITYNUM_NONE )
  {
    pm->ps->pm_flags = ( pm->ps->pm_flags & ~PMF_CHARGE ) | PMF_TIME_LAND;
    pm->ps->pm_time = HUMAN_DODGE_TIMEOUT;
  }

  // Reasons why we can't start a dodge or sprint
  if( pm->ps->pm_type != PM_NORMAL || pm->ps->stats[ STAT_STAMINA ] < STAMINA_SLOW_LEVEL + STAMINA_DODGE_TAKE ||
      ( pm->ps->pm_flags & PMF_DUCKED ) )
    return qfalse;

  // Can't dodge forward
  if( pm->cmd.forwardmove > 0 )
    return qfalse;

  // Reasons why we can't start a dodge only
  if( pm->ps->pm_flags & ( PMF_TIME_LAND | PMF_CHARGE ) ||
      pm->ps->groundEntityNum == ENTITYNUM_NONE ||
      !( pm->cmd.buttons & BUTTON_DODGE ) )
    return qfalse;

  // Dodge direction specified with movement keys
  if( ( !pm->cmd.rightmove && !pm->cmd.forwardmove ) || pm->cmd.upmove )
    return qfalse;

  AngleVectors( pm->ps->viewangles, NULL, right, NULL );
  forward[ 0 ] = -right[ 1 ];
  forward[ 1 ] = right[ 0 ];
  forward[ 2 ] = 0.0f;

  // Dodge magnitude is based on the jump magnitude scaled by the modifiers
  jump = BG_Class( pm->ps->stats[ STAT_CLASS ] )->jumpMagnitude;
  if( pm->cmd.rightmove && pm->cmd.forwardmove )
    jump *= ( 0.5f * M_SQRT2 );

  // Weaken dodge if slowed
  if( ( pm->ps->stats[ STAT_STATE ] & SS_SLOWLOCKED )  ||
      ( pm->ps->stats[ STAT_STATE ] & SS_CREEPSLOWED ) ||
      ( pm->ps->eFlags & EF_POISONCLOUDED ) )
    sideModifier = HUMAN_DODGE_SLOWED_MODIFIER;
  else
    sideModifier = HUMAN_DODGE_SIDE_MODIFIER;

  // The dodge sets minimum velocity
  if( pm->cmd.rightmove )
  {
    if( pm->cmd.rightmove < 0 )
      VectorNegate( right, right );
    VectorMA( velocity, jump * sideModifier, right, velocity );
  }

  if( pm->cmd.forwardmove )
  {
    if( pm->cmd.forwardmove < 0 )
      VectorNegate( forward, forward );
    VectorMA( velocity, jump * sideModifier, forward, velocity );
  }

  velocity[ 2 ] = jump * HUMAN_DODGE_UP_MODIFIER;

  // Make sure client has minimum velocity
  for( i = 0; i < 3; i++ )
  {
    if( ( velocity[ i ] < 0.0f &&
          pm->ps->velocity[ i ] > velocity[ i ] ) ||
        ( velocity[ i ] > 0.0f &&
          pm->ps->velocity[ i ] < velocity[ i ] ) )
      pm->ps->velocity[ i ] = velocity[ i ];
  }

  // Jumped away
  pml.groundPlane = qfalse;
  pml.walking = qfalse;
  pm->ps->groundEntityNum = ENTITYNUM_NONE;
  pm->ps->pm_flags |= PMF_CHARGE;
  pm->ps->stats[ STAT_STAMINA ] -= STAMINA_DODGE_TAKE;
  pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^
                       ANIM_TOGGLEBIT ) | LEGS_JUMP;
  PM_AddEvent( EV_JUMP );

  return qtrue;
}*/

//============================================================================


/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void )
{
  // waterjump has no control, but falls

  PM_StepSlideMove( qtrue, qfalse );

  pm->ps->velocity[ 2 ] -= pm->ps->gravity * pml.frametime;
  if( pm->ps->velocity[ 2 ] < 0 )
  {
    // cancel as soon as we are falling down again
    pm->ps->pm_flags &= ~PMF_ALL_TIMES;
    pm->ps->pm_time = 0;
  }
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void )
{
  int   i;
  vec3_t  wishvel;
  float wishspeed;
  vec3_t  wishdir;
  float scale;
  float vel;

  if( PM_CheckWaterJump( ) )
  {
    PM_WaterJumpMove();
    return;
  }
#if 0
  // jump = head for surface
  if ( pm->cmd.upmove >= 10 ) {
    if (pm->ps->velocity[2] > -300) {
      if ( pm->watertype == CONTENTS_WATER ) {
        pm->ps->velocity[2] = 100;
      } else if (pm->watertype == CONTENTS_SLIME) {
        pm->ps->velocity[2] = 80;
      } else {
        pm->ps->velocity[2] = 50;
      }
    }
  }
#endif
  PM_Friction( );

  scale = PM_CmdScale( &pm->cmd, qtrue );
  //
  // user intentions
  //
  if( !scale )
  {
    wishvel[ 0 ] = 0;
    wishvel[ 1 ] = 0;
    wishvel[ 2 ] = -60;   // sink towards bottom
  }
  else
  {
    for( i = 0; i < 3; i++ )
      wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;

    wishvel[ 2 ] += scale * pm->cmd.upmove;
  }

  VectorCopy( wishvel, wishdir );
  wishspeed = VectorNormalize( wishdir );

  if( wishspeed > pm->ps->speed * pm_swimScale )
    wishspeed = pm->ps->speed * pm_swimScale;

  PM_Accelerate( wishdir, wishspeed, pm_wateraccelerate );

  // make sure we can go up slopes easily under water
  if( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 )
  {
    vel = VectorLength( pm->ps->velocity );
    // slide along the ground plane
    PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

    VectorNormalize( pm->ps->velocity );
    VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
  }

  PM_SlideMove( qfalse );
}

/*
===================
PM_JetPackMove

Only with the jetpack
===================
*/
static void PM_JetPackMove( void )
{
  int     i;
  vec3_t  wishvel;
  float   wishspeed;
  vec3_t  wishdir;
  float   scale;

  //normal slowdown
  PM_Friction( );

  scale = PM_CmdScale( &pm->cmd, qfalse );

  // user intentions
  for( i = 0; i < 2; i++ )
    wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;

  if( pm->cmd.upmove > 0.0f )
  {
    vec3_t thrustDir = { 0.0f, 0.0f, 1.0f };

    if( pm->ps->viewangles[ PITCH ] > 0.0f )
      VectorCopy( pml.up, thrustDir ) ;

    // give an initial vertical boost when first activating the jet
    if( pm->ps->persistant[PERS_JUMPTIME] <= JETPACK_ACT_BOOST_TIME )
      VectorMA( wishvel, JETPACK_ACT_BOOST_SPEED, thrustDir, wishvel );
    else
      VectorMA( wishvel, JETPACK_FLOAT_SPEED, thrustDir, wishvel );
  }
  else if( pm->cmd.upmove < 0.0f )
    wishvel[ 2 ] = -JETPACK_SINK_SPEED;
  else
    wishvel[ 2 ] = 0.0f;

  // apply the appropirate amount of gravity if the upward velocity is greater
  // than the max upward jet velocity.
  if( pm->ps->velocity[ 2 ] > wishvel[ 2 ] )
    pm->ps->velocity[ 2 ] -= MIN( pm->ps->gravity * pml.frametime, pm->ps->velocity[ 2 ] - wishvel[ 2 ] );

  VectorCopy( wishvel, wishdir );
  wishspeed = VectorNormalize( wishdir );

  PM_Accelerate( wishdir, wishspeed, pm_flyaccelerate );

  PM_StepSlideMove( qfalse, qfalse );

  if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
    PM_ContinueLegsAnim( LEGS_LAND );
  else
    PM_ContinueLegsAnim( NSPA_LAND );

  // disable bunny hop
  pm->ps->pm_flags &= ~PMF_ALL_HOP_FLAGS;
}

/*
===================
PM_SpitfireFlyMove
Only with Spitfire
===================
*/
static void PM_SpitfireFlyMove( void )
{
  int      i;
  vec3_t   wishvel;
  float    wishspeed;
  vec3_t   wishdir;
  float    scale;
  float    accel = BG_Class( pm->ps->stats[ STAT_CLASS ] )->airAcceleration;
  qboolean isgliding = qfalse;
  qboolean basicflight = qfalse;
  qboolean ascend = qfalse;
  qboolean upmovePressed = qfalse;
  qboolean freefall = qfalse;
  qboolean gravity = qfalse;

  if( pm->ps->weapon != WP_ASPITFIRE )
    return;

  if( PM_CheckAirPounce( ) )
  {
    pm->ps->torsoTimer = 1200;
    return;
  }
  
  PM_CheckJump( );

  if( pm->ps->groundEntityNum == ENTITYNUM_NONE &&
      ( pm->ps->commandTime > pm->pmext->pouncePayloadTime +
                                             SPITFIRE_PAYLOAD_DISCHARGE_TIME ) )
  {
    basicflight = qtrue;

    if( pm->cmd.upmove > 0 )
    {
      upmovePressed = qtrue;
      pm->cmd.upmove = 0;
      if( pm->ps->persistant[PERS_JUMPTIME] > SPITFIRE_ASCEND_REPEAT )
      {
        ascend = qtrue;
        pm->ps->persistant[PERS_JUMPTIME] = 0;
      }
    } else if ( pm->cmd.upmove < 0 &&
           !( pm->cmd.buttons & BUTTON_WALKING ) )
    {
      freefall = qtrue;

      // project moves down to flat plane
      pml.forward[ 2 ] = 0;
      pml.right[ 2 ] = 0;
      VectorNormalize( pml.forward );
      VectorNormalize( pml.right );
    }

    //gliding
    if( !( pm->cmd.buttons & BUTTON_WALKING ) &&
        pm->ps->persistant[PERS_JUMPTIME] > SPITFIRE_ASCEND_REPEAT &&
        !freefall )
    {
      accel = SPITFIRE_GLIDE_ACCEL;
      isgliding = qtrue;
    }
  }

  scale = PM_CmdScale( &pm->cmd, freefall ? qfalse : qtrue );

  if( !freefall )
    scale *= scale * SPITFIRE_AIRSPEED_MOD; //for faster airspeed

  if( !scale )
  {
    wishvel[ 0 ] = 0;
    wishvel[ 1 ] = 0;
    wishvel[ 2 ] = 0;
  }
  else if( pm->ps->commandTime > pm->pmext->pouncePayloadTime +
                                               SPITFIRE_PAYLOAD_DISCHARGE_TIME )
  {
    for( i = 0; i < 3; i++ )
      wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;
    wishvel[ 2 ] += scale * pm->cmd.upmove;
  }
  else if( freefall )
  {
    for( i = 0; i < 2; i++ )
      wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;

    wishvel[ 2 ] = 0;
  }
  else
    for( i = 0; i < 3; i++ )
      wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove;

  if( isgliding )
  {
    float  pathspeed = VectorLength( pm->ps->velocity);

    //apply gravity
    pm->ps->velocity[ 2 ] -= pm->ps->gravity * pml.frametime;

    if( pathspeed )
    {
      vec3_t  pathdir, pathup, pathang;
      float  liftmod = pm->ps->gravity / pow( ( SPITFIRE_GLIDE_MOD ), 2 );
      float  dragmod = liftmod / tan( DEG2RAD( SPITFIRE_GLIDE_ANGLE ) );
      float  dragmag = dragmod * pow( pathspeed, 2 );
      float  liftmag = liftmod * pow( pathspeed, 2 );

      vectoangles( pm->ps->velocity, pathang );
      AngleVectors( pathang, pathdir, NULL, pathup );
      //apply drag
      VectorMA( pm->ps->velocity, -( dragmag * pml.frametime ), pathdir,
                pm->ps->velocity );

      //apply lift
      VectorMA( pm->ps->velocity, liftmag * pml.frametime, pathup,
                  pm->ps->velocity );
    }
  } else
  {
    //normal slowdown
    PM_Friction( );

    // hovering
    if( basicflight &&
        ( pm->cmd.buttons & BUTTON_WALKING ) )
    {
      int bobmove, old;

      // restrict ascent for hovering
      if( wishvel[ 2 ] > 0 )
        wishvel[ 2 ] = 0;

      //apply bobbing
      bobmove = BG_Class( pm->ps->stats[ STAT_CLASS ] )->bobCycle;
      old = pm->ps->bobCycle;
      pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;
    }
  }

  VectorCopy( wishvel, wishdir );
  wishspeed = VectorNormalize( wishdir );

  if( !isgliding || scale )
    PM_Accelerate( wishdir, wishspeed, accel );

  if( ascend )
  {
    //ascend
    VectorMA( pm->ps->velocity,
              SPITFIRE_ASCEND_MAG + pm->ps->gravity * pml.frametime,
              pml.up, pm->ps->velocity );

    PM_AddEvent( EV_JUMP );

    if( pm->cmd.forwardmove >= 0 )
    {
      if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
        PM_ForceLegsAnim( LEGS_LAND );
      else
        PM_ForceLegsAnim( NSPA_LAND );

      pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
    }
    else
    {
      if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
        PM_ForceLegsAnim( LEGS_LANDB );
      else
        PM_ForceLegsAnim( NSPA_LANDBACK );

      pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
    }
  } else if( !freefall )
    PM_ContinueLegsAnim( NSPA_SWIM );

  if( freefall )
  {
    // we may have a ground plane that is very steep, even
    // though we don't have a groundentity
    // slide along the steep plane
    if( pml.groundPlane )
      PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );
  }

  if( ( upmovePressed || freefall ) &&
        !isgliding )
    gravity = qtrue;

  PM_StepSlideMove( gravity, qfalse );
}

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void )
{
  int     i;
  vec3_t  wishvel;
  float   wishspeed;
  vec3_t  wishdir;
  float   scale;

  // normal slowdown
  PM_Friction( );

  scale = PM_CmdScale( &pm->cmd, qtrue );
  //
  // user intentions
  //
  if( !scale )
  {
    wishvel[ 0 ] = 0;
    wishvel[ 1 ] = 0;
    wishvel[ 2 ] = 0;
  }
  else
  {
    for( i = 0; i < 3; i++ )
      wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;

    wishvel[ 2 ] += scale * pm->cmd.upmove;
  }

  VectorCopy( wishvel, wishdir );
  wishspeed = VectorNormalize( wishdir );

  PM_Accelerate( wishdir, wishspeed, pm_flyaccelerate );

  PM_StepSlideMove( qfalse, qfalse );
}

/*
===================
PM_AirMove

===================
*/
static void PM_AirMove( void )
{
  int       i;
  vec3_t    wishvel;
  float     fmove, smove;
  vec3_t    wishdir;
  float     wishspeed;
  float     scale;
  usercmd_t cmd;

  PM_CheckWallJump( );
  PM_Friction( );

  fmove = pm->cmd.forwardmove;
  smove = pm->cmd.rightmove;

  cmd = pm->cmd;
  scale = PM_CmdScale( &cmd, qfalse );

  // set the movementDir so clients can rotate the legs for strafing
  PM_SetMovementDir( );

  // project moves down to flat plane
  pml.forward[ 2 ] = 0;
  pml.right[ 2 ] = 0;
  VectorNormalize( pml.forward );
  VectorNormalize( pml.right );

  for( i = 0; i < 2; i++ )
    wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;

  wishvel[ 2 ] = 0;

  VectorCopy( wishvel, wishdir );
  wishspeed = VectorNormalize( wishdir );
  wishspeed *= scale;

  // not on ground, so little effect on velocity
  PM_Accelerate( wishdir, wishspeed,
    BG_Class( pm->ps->stats[ STAT_CLASS ] )->airAcceleration );

  // we may have a ground plane that is very steep, even
  // though we don't have a groundentity
  // slide along the steep plane
  if( pml.groundPlane )
    PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

  PM_StepSlideMove( qtrue, qfalse );
}

/*
===================
PM_ClimbMove

===================
*/
static void PM_ClimbMove( void )
{
  int       i;
  vec3_t    wishvel;
  float     fmove, smove;
  vec3_t    wishdir;
  float     wishspeed;
  float     scale;
  usercmd_t cmd;
  float     accelerate;
  float     vel;

  if( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 )
  {
    // begin swimming
    PM_WaterMove( );
    return;
  }


  if( PM_CheckJump( ) || PM_CheckPounce( ) || PM_CheckAirPounce( ) )
  {
    // jumped away
    if( pm->waterlevel > 1 )
      PM_WaterMove( );
    else if( pm->ps->weapon == WP_ASPITFIRE )
      PM_SpitfireFlyMove( );
    else
      PM_AirMove( );

    return;
  }

  PM_Friction( );

  fmove = pm->cmd.forwardmove;
  smove = pm->cmd.rightmove;

  cmd = pm->cmd;
  scale = PM_CmdScale( &cmd, qfalse );

  // set the movementDir so clients can rotate the legs for strafing
  PM_SetMovementDir( );

  // project the forward and right directions onto the ground plane
  PM_ClipVelocity( pml.forward, pml.groundTrace.plane.normal, pml.forward );
  PM_ClipVelocity( pml.right, pml.groundTrace.plane.normal, pml.right );
  //
  VectorNormalize( pml.forward );
  VectorNormalize( pml.right );

  for( i = 0; i < 3; i++ )
    wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;

  // when going up or down slopes the wish velocity should Not be zero
//  wishvel[2] = 0;

  VectorCopy( wishvel, wishdir );
  wishspeed = VectorNormalize( wishdir );
  wishspeed *= scale;

  // clamp the speed lower if ducking
  if( pm->ps->pm_flags & PMF_DUCKED )
  {
    if( wishspeed > pm->ps->speed * pm_duckScale )
      wishspeed = pm->ps->speed * pm_duckScale;
  }

  // clamp the speed lower if wading or walking on the bottom
  if( pm->waterlevel )
  {
    float waterScale;

    waterScale = pm->waterlevel / 3.0;
    waterScale = 1.0 - ( 1.0 - pm_swimScale ) * waterScale;
    if( wishspeed > pm->ps->speed * waterScale )
      wishspeed = pm->ps->speed * waterScale;
  }

  // when a player gets hit, they temporarily lose
  // full control, which allows them to be moved a bit
  if( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
    accelerate = BG_Class( pm->ps->stats[ STAT_CLASS ] )->airAcceleration;
  else
    accelerate = BG_Class( pm->ps->stats[ STAT_CLASS ] )->acceleration;

  PM_Accelerate( wishdir, wishspeed, accelerate );

  if( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
    pm->ps->velocity[ 2 ] -= pm->ps->gravity * pml.frametime;

  vel = VectorLength( pm->ps->velocity );

  // slide along the ground plane
  PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

  // don't decrease velocity when going up or down a slope
  VectorNormalize( pm->ps->velocity );
  VectorScale( pm->ps->velocity, vel, pm->ps->velocity );

  // don't do anything if standing still
  if( !pm->ps->velocity[ 0 ] && !pm->ps->velocity[ 1 ] && !pm->ps->velocity[ 2 ] )
    return;

  PM_StepSlideMove( qfalse, qfalse );
}


/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void )
{
  int       i;
  vec3_t    wishvel;
  float     fmove, smove;
  vec3_t    wishdir;
  float     wishspeed;
  float     scale;
  usercmd_t cmd;
  float     accelerate;

  if( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 )
  {
    // begin swimming
    PM_WaterMove( );
    return;
  }

  if( PM_CheckJump( ) || PM_CheckPounce( ) || PM_CheckAirPounce( ) )
  {
    // jumped away
    if( pm->waterlevel > 1 )
      PM_WaterMove( );
    else if( pm->ps->weapon == WP_ASPITFIRE )
      PM_SpitfireFlyMove( );
    else
      PM_AirMove( );

    return;
  }

  //charging
  PM_CheckCharge( );

  PM_Friction( );

  fmove = pm->cmd.forwardmove;
  smove = pm->cmd.rightmove;

  cmd = pm->cmd;
  scale = PM_CmdScale( &cmd, qfalse );

  // set the movementDir so clients can rotate the legs for strafing
  PM_SetMovementDir( );

  // project moves down to flat plane
  pml.forward[ 2 ] = 0;
  pml.right[ 2 ] = 0;

  // project the forward and right directions onto the ground plane
  PM_ClipVelocity( pml.forward, pml.groundTrace.plane.normal, pml.forward );
  PM_ClipVelocity( pml.right, pml.groundTrace.plane.normal, pml.right );
  //
  VectorNormalize( pml.forward );
  VectorNormalize( pml.right );

  for( i = 0; i < 3; i++ )
    wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;

  // when going up or down slopes the wish velocity should Not be zero
//  wishvel[2] = 0;

  VectorCopy( wishvel, wishdir );
  wishspeed = VectorNormalize( wishdir );
  wishspeed *= scale;

  // clamp the speed lower if ducking
  if( pm->ps->pm_flags & PMF_DUCKED )
  {
    if( wishspeed > pm->ps->speed * pm_duckScale )
      wishspeed = pm->ps->speed * pm_duckScale;
  }

  // clamp the speed lower if wading or walking on the bottom
  if( pm->waterlevel )
  {
    float waterScale;

    waterScale = pm->waterlevel / 3.0;
    waterScale = 1.0 - ( 1.0 - pm_swimScale ) * waterScale;
    if( wishspeed > pm->ps->speed * waterScale )
      wishspeed = pm->ps->speed * waterScale;
  }

  // when a player gets hit, they temporarily lose
  // full control, which allows them to be moved a bit
  if( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
    accelerate = BG_Class( pm->ps->stats[ STAT_CLASS ] )->airAcceleration;
  else
    accelerate = BG_Class( pm->ps->stats[ STAT_CLASS ] )->acceleration;

  PM_Accelerate( wishdir, wishspeed, accelerate );

  //Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
  //Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

  if( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
    pm->ps->velocity[ 2 ] -= pm->ps->gravity * pml.frametime;
  else
  {
    // don't reset the z velocity for slopes
//    pm->ps->velocity[2] = 0;
  }

  // slide along the ground plane
  PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

  // don't do anything if standing still
  if( !pm->ps->velocity[ 0 ] && !pm->ps->velocity[ 1 ] )
    return;

  PM_StepSlideMove( qfalse, qfalse );

  //Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));

}


/*
===================
PM_LadderMove

Basically a rip of PM_WaterMove with a few changes
===================
*/
static void PM_LadderMove( void )
{
  int     i;
  vec3_t  wishvel;
  float   wishspeed;
  vec3_t  wishdir;
  float   scale;
  float   vel;

  PM_Friction( );

  scale = PM_CmdScale( &pm->cmd, qtrue );

  for( i = 0; i < 3; i++ )
    wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;

  wishvel[ 2 ] += scale * pm->cmd.upmove;

  VectorCopy( wishvel, wishdir );
  wishspeed = VectorNormalize( wishdir );

  if( wishspeed > pm->ps->speed * pm_swimScale )
    wishspeed = pm->ps->speed * pm_swimScale;

  PM_Accelerate( wishdir, wishspeed, pm_accelerate );

  //slanty ladders
  if( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0.0f )
  {
    vel = VectorLength( pm->ps->velocity );

    // slide along the ground plane
    PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

    VectorNormalize( pm->ps->velocity );
    VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
  }

  PM_SlideMove( qfalse );
}


/*
=============
PM_CheckLadder

Check to see if the player is on a ladder or not
=============
*/
static void PM_CheckLadder( void )
{
  vec3_t  forward, end;
  trace_t trace;

  //test if class can use ladders
  if( !BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_CANUSELADDERS ) )
  {
    pm->pmext->ladder = qfalse;
    pml.ladder = qfalse;
    return;
  }

  VectorCopy( pml.forward, forward );
  forward[ 2 ] = 0.0f;

  VectorMA( pm->ps->origin, 1.0f, forward, end );

  pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, MASK_PLAYERSOLID );

  if( ( trace.fraction < 1.0f ) && ( trace.surfaceFlags & SURF_LADDER ) )
  {
    pm->pmext->ladder = qtrue;
    pml.ladder = qtrue;

    if( BG_InventoryContainsUpgrade( UP_JETPACK, pm->ps->stats ) &&
        BG_UpgradeIsActive( UP_JETPACK, pm->ps->stats ) )
    {
      BG_DeactivateUpgrade( UP_JETPACK, pm->ps->stats );
      pm->ps->pm_type = PM_NORMAL;
    }
  }
  else
  {
    pm->pmext->ladder = qfalse;
    pml.ladder = qfalse;
  }
}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void )
{
  float forward;

  if( !pml.walking )
    return;

  // extra friction

  forward = VectorLength( pm->ps->velocity );
  forward -= 20;

  if( forward <= 0 )
    VectorClear( pm->ps->velocity );
  else
  {
    VectorNormalize( pm->ps->velocity );
    VectorScale( pm->ps->velocity, forward, pm->ps->velocity );
  }
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void )
{
  float   speed, drop, friction, control, newspeed;
  int     i;
  vec3_t  wishvel;
  float   fmove, smove;
  vec3_t  wishdir;
  float   wishspeed;
  float   scale;

  // friction

  speed = VectorLength( pm->ps->velocity );

  if( speed < 1 )
  {
    VectorCopy( vec3_origin, pm->ps->velocity );
  }
  else
  {
    drop = 0;

    friction = pm_friction * 1.5; // extra friction
    control = speed < pm_stopspeed ? pm_stopspeed : speed;
    drop += control * friction * pml.frametime;

    // scale the velocity
    newspeed = speed - drop;

    if( newspeed < 0 )
      newspeed = 0;

    newspeed /= speed;

    VectorScale( pm->ps->velocity, newspeed, pm->ps->velocity );
  }

  // accelerate
  scale = PM_CmdScale( &pm->cmd, qtrue );

  fmove = pm->cmd.forwardmove;
  smove = pm->cmd.rightmove;

  for( i = 0; i < 3; i++ )
    wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;

  wishvel[ 2 ] += pm->cmd.upmove;

  VectorCopy( wishvel, wishdir );
  wishspeed = VectorNormalize( wishdir );
  wishspeed *= scale;

  PM_Accelerate( wishdir, wishspeed, pm_accelerate );

  // move
  VectorMA( pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin );
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void )
{
  if( pm->ps->stats[ STAT_STATE ] & SS_CREEPSLOWED )
    return EV_FOOTSTEP_SQUELCH;

  if( pml.groundTrace.surfaceFlags & SURF_NOSTEPS )
    return 0;

  if( pml.groundTrace.surfaceFlags & SURF_METALSTEPS )
    return EV_FOOTSTEP_METAL;

  return EV_FOOTSTEP;
}


/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void )
{
  float   delta;
  float   dist;
  float   vel, acc;
  float   t;
  float   a, b, c, den;

  // decide which landing animation to use
  if( pm->ps->pm_flags & PMF_BACKWARDS_JUMP )
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
      PM_ForceLegsAnim( LEGS_LANDB );
    else
      PM_ForceLegsAnim( NSPA_LANDBACK );
  }
  else
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
      PM_ForceLegsAnim( LEGS_LAND );
    else
      PM_ForceLegsAnim( NSPA_LAND );
  }

  if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
    pm->ps->legsTimer = TIMER_LAND;
  else
    pm->ps->torsoTimer = TIMER_LAND;

  // calculate the exact velocity on landing
  dist = pm->ps->origin[ 2 ] - pml.previous_origin[ 2 ];
  vel = pml.previous_velocity[ 2 ];
  acc = -pm->ps->gravity;

  a = acc / 2;
  b = vel;
  c = -dist;

  den =  b * b - 4 * a * c;
  if( den < 0 )
    return;

  t = (-b - sqrt( den ) ) / ( 2 * a );

  delta = vel + t * acc;
  delta = delta*delta * 0.0001;

  // never take falling damage if completely underwater
  if( pm->waterlevel == 3 )
    return;

  // reduce falling damage if there is standing water
  if( pm->waterlevel == 2 )
    delta *= 0.25;

  if( pm->waterlevel == 1 )
    delta *= 0.5;

  if( delta < 1 )
    return;

  // create a local entity event to play the sound

  // SURF_NODAMAGE is used for bounce pads where you don't ever
  // want to take damage or play a crunch sound
  if( !( pml.groundTrace.surfaceFlags & SURF_NODAMAGE ) )
  {
    pm->ps->stats[ STAT_FALLDIST ] = delta;

    if( delta > AVG_FALL_DISTANCE )
    {
      if( PM_Alive( pm->ps->pm_type ) )
        PM_AddEvent( EV_FALL_FAR );
    }
    else if( delta > MIN_FALL_DISTANCE )
    {
      if( PM_Alive( pm->ps->pm_type ) )
        PM_AddEvent( EV_FALL_MEDIUM );
    }
    else
    {
      if( delta > 7 )
        PM_AddEvent( EV_FALL_SHORT );
      else
        PM_AddEvent( PM_FootstepForSurface( ) );
    }
  }

  // start footstep cycle over
  pm->ps->bobCycle = 0;
}


/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace )
{
  int       i, j, k;
  vec3_t    point;

  if( pm->debugLevel )
    Com_Printf("%i:allsolid\n", c_pmove);

  // jitter around
  for( i = -1; i <= 1; i++ )
  {
    for( j = -1; j <= 1; j++ )
    {
      for( k = -1; k <= 1; k++ )
      {
        VectorCopy( pm->ps->origin, point );
        point[ 0 ] += (float)i;
        point[ 1 ] += (float)j;
        point[ 2 ] += (float)k;
        pm->trace( trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

        if( !trace->allsolid )
        {
          point[ 0 ] = pm->ps->origin[ 0 ];
          point[ 1 ] = pm->ps->origin[ 1 ];
          point[ 2 ] = pm->ps->origin[ 2 ] - 0.25;

          pm->trace( trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
          pml.groundTrace = *trace;
          return qtrue;
        }
      }
    }
  }

  pm->ps->groundEntityNum = ENTITYNUM_NONE;
  pml.groundPlane = qfalse;
  pml.walking = qfalse;

  return qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void )
{
  trace_t   trace;
  vec3_t    point;

  if( pm->ps->groundEntityNum != ENTITYNUM_NONE )
  {
    // we just transitioned into freefall
    if( pm->debugLevel )
      Com_Printf( "%i:lift\n", c_pmove );

    // if they aren't in a jumping animation and the ground is a ways away, force into it
    // if we didn't do the trace, the player would be backflipping down staircases
    VectorCopy( pm->ps->origin, point );
    point[ 2 ] -= 64.0f;

    pm->trace( &trace, pm->ps->origin, NULL, NULL, point, pm->ps->clientNum, pm->tracemask );
    if( trace.fraction == 1.0f )
    {
      if( pm->cmd.forwardmove >= 0 )
      {
        if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
          PM_ForceLegsAnim( LEGS_JUMP );
        else
          PM_ForceLegsAnim( NSPA_JUMP );

        pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
      }
      else
      {
        if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
          PM_ForceLegsAnim( LEGS_JUMPB );
        else
          PM_ForceLegsAnim( NSPA_JUMPBACK );

        pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
      }
    }
  }

  if( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_TAKESFALLDAMAGE ) )
  {
    if( pm->ps->velocity[ 2 ] < FALLING_THRESHOLD && pml.previous_velocity[ 2 ] >= FALLING_THRESHOLD )
    {
      // disable bunny hop
      pm->ps->pm_flags &= ~PMF_ALL_HOP_FLAGS;

      PM_AddEvent( EV_FALLING );
    }
  }

  pm->ps->groundEntityNum = ENTITYNUM_NONE;
  pml.groundPlane = qfalse;
  pml.walking = qfalse;
}

/*
=============
PM_GroundClimbTrace
=============
*/
static void PM_GroundClimbTrace( void )
{
  vec3_t      surfNormal, movedir, lookdir, point;
  vec3_t      refNormal = { 0.0f, 0.0f, 1.0f };
  vec3_t      ceilingNormal = { 0.0f, 0.0f, -1.0f };
  vec3_t      toAngles, surfAngles;
  trace_t     trace;
  int         i;
  const float eps = 0.000001f;

  //used for delta correction
  vec3_t    traceCROSSsurf, traceCROSSref, surfCROSSref;
  float     traceDOTsurf, traceDOTref, surfDOTref, rTtDOTrTsTt;
  float     traceANGsurf, traceANGref, surfANGref;
  vec3_t    horizontal = { 1.0f, 0.0f, 0.0f }; //arbituary vector perpendicular to refNormal
  vec3_t    refTOtrace, refTOsurfTOtrace, tempVec;
  int       rTtANGrTsTt;
  float     ldDOTtCs, d;
  vec3_t    abc;

  BG_GetClientNormal( pm->ps, surfNormal );

  //construct a vector which reflects the direction the player is looking wrt the surface normal
  ProjectPointOnPlane( movedir, pml.forward, surfNormal );
  VectorNormalize( movedir );

  VectorCopy( movedir, lookdir );

  if( pm->cmd.forwardmove < 0 )
    VectorNegate( movedir, movedir );

  //allow strafe transitions
  if( pm->cmd.rightmove )
  {
    VectorCopy( pml.right, movedir );

    if( pm->cmd.rightmove < 0 )
      VectorNegate( movedir, movedir );
  }

  for( i = 0; i <= 4; i++ )
  {
    switch ( i )
    {
      case 0:
        //we are going to step this frame so skip the transition test
        if( PM_PredictStepMove( ) )
          continue;

        //trace into direction we are moving
        VectorMA( pm->ps->origin, 0.25f, movedir, point );
        pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
        break;

      case 1:
        //trace straight down anto "ground" surface
        //mask out CONTENTS_BODY to not hit other players and avoid the camera flipping out when
        // wallwalkers touch
        VectorMA( pm->ps->origin, -0.25f, surfNormal, point );
        pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask & ~CONTENTS_BODY );
        break;

      case 2:
        if( pml.groundPlane != qfalse && PM_PredictStepMove( ) )
        {
          //step down
          VectorMA( pm->ps->origin, -STEPSIZE, surfNormal, point );
          pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
        }
        else
          continue;
        break;

      case 3:
        //trace "underneath" BBOX so we can traverse angles > 180deg
        if( pml.groundPlane != qfalse )
        {
          VectorMA( pm->ps->origin, -16.0f, surfNormal, point );
          VectorMA( point, -16.0f, movedir, point );
          pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
        }
        else
          continue;
        break;

      case 4:
        //fall back so we don't have to modify PM_GroundTrace too much
        VectorCopy( pm->ps->origin, point );
        point[ 2 ] = pm->ps->origin[ 2 ] - 0.25f;
        pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
        break;
    }

    //if we hit something
    if( trace.fraction < 1.0f && !( trace.surfaceFlags & ( SURF_SKY | SURF_SLICK ) ) &&
        !( trace.entityNum != ENTITYNUM_WORLD && i != 4 ) )
    {
      if( i == 2 || i == 3 )
      {
        if( i == 2 )
          PM_StepEvent( pm->ps->origin, trace.endpos, surfNormal );

        VectorCopy( trace.endpos, pm->ps->origin );
      }

      //calculate a bunch of stuff...
      CrossProduct( trace.plane.normal, surfNormal, traceCROSSsurf );
      VectorNormalize( traceCROSSsurf );

      CrossProduct( trace.plane.normal, refNormal, traceCROSSref );
      VectorNormalize( traceCROSSref );

      CrossProduct( surfNormal, refNormal, surfCROSSref );
      VectorNormalize( surfCROSSref );

      //calculate angle between surf and trace
      traceDOTsurf = DotProduct( trace.plane.normal, surfNormal );
      traceANGsurf = RAD2DEG( acos( traceDOTsurf ) );

      if( traceANGsurf > 180.0f )
        traceANGsurf -= 180.0f;

      //calculate angle between trace and ref
      traceDOTref = DotProduct( trace.plane.normal, refNormal );
      traceANGref = RAD2DEG( acos( traceDOTref ) );

      if( traceANGref > 180.0f )
        traceANGref -= 180.0f;

      //calculate angle between surf and ref
      surfDOTref = DotProduct( surfNormal, refNormal );
      surfANGref = RAD2DEG( acos( surfDOTref ) );

      if( surfANGref > 180.0f )
        surfANGref -= 180.0f;

      //if the trace result and old surface normal are different then we must have transided to a new
      //surface... do some stuff...
      if( !VectorCompareEpsilon( trace.plane.normal, surfNormal, eps ) )
      {
        //if the trace result or the old vector is not the floor or ceiling correct the YAW angle
        if( !VectorCompareEpsilon( trace.plane.normal, refNormal, eps ) && !VectorCompareEpsilon( surfNormal, refNormal, eps ) &&
            !VectorCompareEpsilon( trace.plane.normal, ceilingNormal, eps ) && !VectorCompareEpsilon( surfNormal, ceilingNormal, eps ) )
        {
          //behold the evil mindfuck from hell
          //it has fucked mind like nothing has fucked mind before

          //calculate reference rotated through to trace plane
          RotatePointAroundVector( refTOtrace, traceCROSSref, horizontal, -traceANGref );

          //calculate reference rotated through to surf plane then to trace plane
          RotatePointAroundVector( tempVec, surfCROSSref, horizontal, -surfANGref );
          RotatePointAroundVector( refTOsurfTOtrace, traceCROSSsurf, tempVec, -traceANGsurf );

          //calculate angle between refTOtrace and refTOsurfTOtrace
          rTtDOTrTsTt = DotProduct( refTOtrace, refTOsurfTOtrace );
          rTtANGrTsTt = ANGLE2SHORT( RAD2DEG( acos( rTtDOTrTsTt ) ) );

          if( rTtANGrTsTt > 32768 )
            rTtANGrTsTt -= 32768;

          CrossProduct( refTOtrace, refTOsurfTOtrace, tempVec );
          VectorNormalize( tempVec );
          if( DotProduct( trace.plane.normal, tempVec ) > 0.0f )
            rTtANGrTsTt = -rTtANGrTsTt;

          //phew! - correct the angle
          pm->ps->delta_angles[ YAW ] -= rTtANGrTsTt;
        }

        //construct a plane dividing the surf and trace normals
        CrossProduct( traceCROSSsurf, surfNormal, abc );
        VectorNormalize( abc );
        d = DotProduct( abc, pm->ps->origin );

        //construct a point representing where the player is looking
        VectorAdd( pm->ps->origin, lookdir, point );

        //check whether point is on one side of the plane, if so invert the correction angle
        if( ( abc[ 0 ] * point[ 0 ] + abc[ 1 ] * point[ 1 ] + abc[ 2 ] * point[ 2 ] - d ) > 0 )
          traceANGsurf = -traceANGsurf;

        //find the . product of the lookdir and traceCROSSsurf
        if( ( ldDOTtCs = DotProduct( lookdir, traceCROSSsurf ) ) < 0.0f )
        {
          VectorInverse( traceCROSSsurf );
          ldDOTtCs = DotProduct( lookdir, traceCROSSsurf );
        }

        //set the correction angle
        traceANGsurf *= 1.0f - ldDOTtCs;

        if( !( pm->ps->persistant[ PERS_STATE ] & PS_WALLCLIMBINGFOLLOW ) )
        {
          //correct the angle
          pm->ps->delta_angles[ PITCH ] -= ANGLE2SHORT( traceANGsurf );
        }

        //transition from wall to ceiling
        //normal for subsequent viewangle rotations
        if( VectorCompareEpsilon( trace.plane.normal, ceilingNormal, eps ) )
        {
          CrossProduct( surfNormal, trace.plane.normal, pm->ps->grapplePoint );
          VectorNormalize( pm->ps->grapplePoint );
          pm->ps->eFlags |= EF_WALLCLIMBCEILING;
        }

        //transition from ceiling to wall
        //we need to do some different angle correction here cos GPISROTVEC
        if( VectorCompareEpsilon( surfNormal, ceilingNormal, eps ) )
        {
          vectoangles( trace.plane.normal, toAngles );
          vectoangles( pm->ps->grapplePoint, surfAngles );

          pm->ps->delta_angles[ 1 ] -= ANGLE2SHORT( ( ( surfAngles[ 1 ] - toAngles[ 1 ] ) * 2 ) - 180.0f );
        }
      }

      pml.groundTrace = trace;

      //so everything knows where we're wallclimbing (ie client side)
      pm->ps->eFlags |= EF_WALLCLIMB;

      //if we're not stuck to the ceiling then set grapplePoint to be a surface normal
      if( !VectorCompareEpsilon( trace.plane.normal, ceilingNormal, eps ) )
      {
        //so we know what surface we're stuck to
        VectorCopy( trace.plane.normal, pm->ps->grapplePoint );
        pm->ps->eFlags &= ~EF_WALLCLIMBCEILING;
      }

      //IMPORTANT: break out of the for loop if we've hit something
      break;
    }
    else if( trace.allsolid )
    {
      // do something corrective if the trace starts in a solid...
      if( !PM_CorrectAllSolid( &trace ) )
        return;
    }
  }

  if( trace.fraction >= 1.0f || ( trace.surfaceFlags & SURF_SLICK ) )
  {
    // if the trace didn't hit anything, we are in free fall
    PM_GroundTraceMissed( );
    pml.groundPlane = qfalse;
    pml.walking = qfalse;
    pm->ps->eFlags &= ~EF_WALLCLIMB;

    //just transided from ceiling to floor... apply delta correction
    if( pm->ps->eFlags & EF_WALLCLIMBCEILING )
    {
      vec3_t  forward, rotated, angles;

      AngleVectors( pm->ps->viewangles, forward, NULL, NULL );

      RotatePointAroundVector( rotated, pm->ps->grapplePoint, forward, 180.0f );
      vectoangles( rotated, angles );

      pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT( angles[ YAW ] - pm->ps->viewangles[ YAW ] );
    }

    pm->ps->eFlags &= ~EF_WALLCLIMBCEILING;

    //we get very bizarre effects if we don't do this :0
    VectorCopy( refNormal, pm->ps->grapplePoint );
    return;
  }

  pml.groundPlane = qtrue;
  pml.walking = qtrue;
  
  // hitting solid ground will end a waterjump
  if( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
  {
    pm->ps->pm_flags &= ~PMF_TIME_WATERJUMP;
    pm->ps->pm_time = 0;
  }

  pm->ps->groundEntityNum = trace.entityNum;

  // don't reset the z velocity for slopes
//  pm->ps->velocity[2] = 0;

  PM_AddTouchEnt( trace.entityNum );
}

/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void )
{
  vec3_t      point;
  vec3_t      refNormal = { 0.0f, 0.0f, 1.0f };
  trace_t     trace;

  if( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_WALLCLIMBER ) )
  {
    if( pm->ps->persistant[ PERS_STATE ] & PS_WALLCLIMBINGTOGGLE )
    {
      //toggle wall climbing if holding crouch
      if( pm->cmd.upmove < 0 && !( pm->ps->pm_flags & PMF_CROUCH_HELD ) )
      {
        if( !( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
          pm->ps->stats[ STAT_STATE ] |= SS_WALLCLIMBING;
        else if( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING )
          pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;

        pm->ps->pm_flags |= PMF_CROUCH_HELD;
      }
      else if( pm->cmd.upmove >= 0 )
        pm->ps->pm_flags &= ~PMF_CROUCH_HELD;
    }
    else
    {
      if( pm->cmd.upmove < 0 )
        pm->ps->stats[ STAT_STATE ] |= SS_WALLCLIMBING;
      else if( pm->cmd.upmove >= 0 )
        pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;
    }

    if( pm->ps->pm_type == PM_DEAD )
      pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;

    if( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING )
    {
      PM_GroundClimbTrace( );
      return;
    }

    //just transided from ceiling to floor... apply delta correction
    if( pm->ps->eFlags & EF_WALLCLIMBCEILING )
    {
      vec3_t  forward, rotated, angles;

      AngleVectors( pm->ps->viewangles, forward, NULL, NULL );

      RotatePointAroundVector( rotated, pm->ps->grapplePoint, forward, 180.0f );
      vectoangles( rotated, angles );

      pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT( angles[ YAW ] - pm->ps->viewangles[ YAW ] );
    }
  }

  pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;
  pm->ps->eFlags &= ~( EF_WALLCLIMB | EF_WALLCLIMBCEILING );

  point[ 0 ] = pm->ps->origin[ 0 ];
  point[ 1 ] = pm->ps->origin[ 1 ];
  point[ 2 ] = pm->ps->origin[ 2 ] - 0.25f;

  pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

  pml.groundTrace = trace;

  // do something corrective if the trace starts in a solid...
  if( trace.allsolid )
    if( !PM_CorrectAllSolid( &trace ) )
      return;

  //make sure that the surfNormal is reset to the ground
  VectorCopy( refNormal, pm->ps->grapplePoint );

  // if the trace didn't hit anything, we are in free fall
  if( trace.fraction == 1.0f )
  {
    qboolean  steppedDown = qfalse;

    // try to step down
    if( pml.groundPlane != qfalse && PM_PredictStepMove( ) && pm->ps->velocity[ 2 ] <= 0.0f )
    {
      //step down
      point[ 0 ] = pm->ps->origin[ 0 ];
      point[ 1 ] = pm->ps->origin[ 1 ];
      point[ 2 ] = pm->ps->origin[ 2 ] - STEPSIZE;
      pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

      //if we hit something
      if( trace.fraction < 1.0f )
      {
        PM_StepEvent( pm->ps->origin, trace.endpos, refNormal );
        VectorCopy( trace.endpos, pm->ps->origin );
        steppedDown = qtrue;
      }
    }

    if( !steppedDown )
    {
      PM_GroundTraceMissed( );
      pml.groundPlane = qfalse;
      pml.walking = qfalse;

      return;
    }
  }

  // check if getting thrown off the ground
  if( pm->ps->velocity[ 2 ] > 0.0f && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10.0f )
  {
    if( pm->debugLevel )
      Com_Printf( "%i:kickoff\n", c_pmove );

    // go into jump animation
    if( pm->cmd.forwardmove >= 0 )
    {
      if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
        PM_ForceLegsAnim( LEGS_JUMP );
      else
        PM_ForceLegsAnim( NSPA_JUMP );

      pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
    }
    else
    {
      if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
        PM_ForceLegsAnim( LEGS_JUMPB );
      else
        PM_ForceLegsAnim( NSPA_JUMPBACK );

      pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
    }

    pm->ps->groundEntityNum = ENTITYNUM_NONE;
    pml.groundPlane = qfalse;
    pml.walking = qfalse;
    return;
  }

  // slopes that are too steep will not be considered onground
  if( trace.plane.normal[ 2 ] < MIN_WALK_NORMAL )
  {
    if( pm->debugLevel )
      Com_Printf( "%i:steep\n", c_pmove );

    // FIXME: if they can't slide down the slope, let them
    // walk (sharp crevices)
    pm->ps->groundEntityNum = ENTITYNUM_NONE;
    pml.groundPlane = qtrue;
    pml.walking = qfalse;
    return;
  }

  pml.groundPlane = qtrue;
  pml.walking = qtrue;

  // hitting solid ground will end a waterjump
  if( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
  {
    pm->ps->pm_flags &= ~PMF_TIME_WATERJUMP;
    pm->ps->pm_time = 0;
  }

  if( pm->ps->groundEntityNum == ENTITYNUM_NONE )
  {
    // just hit the ground
    if( pm->debugLevel )
      Com_Printf( "%i:Land\n", c_pmove );

    // communicate the fall velocity to the server
    pm->pmext->fallVelocity = pml.previous_velocity[ 2 ];

    if( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_TAKESFALLDAMAGE ) )
      PM_CrashLand( );
  }

  if( pm->ps->pm_flags & PMF_JUMPING )
  {
    pm->ps->pm_flags &= ~PMF_JUMPING;
    pm->ps->pm_flags |= PMF_HOPPED;
  }

  pm->ps->groundEntityNum = trace.entityNum;

  // don't reset the z velocity for slopes
//  pm->ps->velocity[2] = 0;

  PM_AddTouchEnt( trace.entityNum );
}

/*
=============
PM_SetWaterLevel  FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel( void )
{
  vec3_t  point;
  int     cont;
  int     sample1;
  int     sample2;
  vec3_t  mins;

  //
  // get waterlevel, accounting for ducking
  //
  pm->waterlevel = 0;
  pm->watertype = 0;

  BG_ClassBoundingBox( pm->ps->stats[ STAT_CLASS ], mins, NULL, NULL, NULL, NULL );

  point[ 0 ] = pm->ps->origin[ 0 ];
  point[ 1 ] = pm->ps->origin[ 1 ];
  point[ 2 ] = pm->ps->origin[ 2 ] + mins[2] + 1;
  cont = pm->pointcontents( point, pm->ps->clientNum );

  if( cont & MASK_WATER )
  {
    sample2 = pm->ps->viewheight - mins[2];
    sample1 = sample2 / 2;

    pm->watertype = cont;
    pm->waterlevel = 1;
    point[ 2 ] = pm->ps->origin[ 2 ] + mins[2] + sample1;
    cont = pm->pointcontents( point, pm->ps->clientNum );

    if( cont & MASK_WATER )
    {
      // disable bunny hop
      pm->ps->pm_flags &= ~PMF_ALL_HOP_FLAGS;

      pm->waterlevel = 2;
      point[ 2 ] = pm->ps->origin[ 2 ] + mins[2] + sample2;
      cont = pm->pointcontents( point, pm->ps->clientNum );

      if( cont & MASK_WATER )
        pm->waterlevel = 3;
    }
  }
}


/*
==============
PM_SetViewheight
==============
*/
static void PM_SetViewheight( void )
{
  pm->ps->viewheight = ( pm->ps->pm_flags & PMF_DUCKED )
      ? BG_ClassConfig( pm->ps->stats[ STAT_CLASS ] )->crouchViewheight
      : BG_ClassConfig( pm->ps->stats[ STAT_CLASS ] )->viewheight;
}

/*
==============
PM_CheckDuck

Sets mins and maxs, and calls PM_SetViewheight
==============
*/
static void PM_CheckDuck (void)
{
  trace_t trace;
  vec3_t PCmins, PCmaxs, PCcmaxs;

  BG_ClassBoundingBox( pm->ps->stats[ STAT_CLASS ], PCmins, PCmaxs, PCcmaxs, NULL, NULL );

  pm->mins[ 0 ] = PCmins[ 0 ];
  pm->mins[ 1 ] = PCmins[ 1 ];

  pm->maxs[ 0 ] = PCmaxs[ 0 ];
  pm->maxs[ 1 ] = PCmaxs[ 1 ];

  pm->mins[ 2 ] = PCmins[ 2 ];

  if( pm->ps->pm_type == PM_DEAD )
  {
    pm->maxs[ 2 ] = -8;
    pm->ps->viewheight = PCmins[ 2 ] + DEAD_VIEWHEIGHT;
    return;
  }

  // If the standing and crouching bboxes are the same the class can't crouch
  if( ( pm->cmd.upmove < 0 ) && !VectorCompare( PCmaxs, PCcmaxs ) &&
      pm->ps->pm_type != PM_JETPACK )
  {
    // duck
    pm->ps->pm_flags |= PMF_DUCKED;
  }
  else
  {
    // stand up if possible
    if( pm->ps->pm_flags & PMF_DUCKED )
    {
      // try to stand up
      pm->maxs[ 2 ] = PCmaxs[ 2 ];
      pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
      if( !trace.allsolid )
        pm->ps->pm_flags &= ~PMF_DUCKED;
    }
  }

  if( pm->ps->pm_flags & PMF_DUCKED )
    pm->maxs[ 2 ] = PCcmaxs[ 2 ];
  else
    pm->maxs[ 2 ] = PCmaxs[ 2 ];

  PM_SetViewheight( );
}



//===================================================================


/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void )
{
  float     bobmove;
  int       old;
  qboolean  footstep;

  //
  // calculate speed and cycle to be used for
  // all cyclic walking effects
  //
  if( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_WALLCLIMBER ) && ( pml.groundPlane ) )
  {
    // FIXME: yes yes i know this is wrong
    pm->xyspeed = sqrt( pm->ps->velocity[ 0 ] * pm->ps->velocity[ 0 ]
                      + pm->ps->velocity[ 1 ] * pm->ps->velocity[ 1 ]
                      + pm->ps->velocity[ 2 ] * pm->ps->velocity[ 2 ] );
  }
  else
    pm->xyspeed = sqrt( pm->ps->velocity[ 0 ] * pm->ps->velocity[ 0 ]
      + pm->ps->velocity[ 1 ] * pm->ps->velocity[ 1 ] );

  if( pm->ps->groundEntityNum == ENTITYNUM_NONE )
  {
    // airborne leaves position in cycle intact, but doesn't advance
    if( pm->waterlevel > 1 )
    {
      if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
        PM_ContinueLegsAnim( LEGS_SWIM );
      else
        PM_ContinueLegsAnim( NSPA_SWIM );
    }

    return;
  }

  // if not trying to move
  if( !pm->cmd.forwardmove && !pm->cmd.rightmove )
  {
    if( pm->xyspeed < 5 )
    {
      pm->ps->bobCycle = 0; // start at beginning of cycle again
      if( pm->ps->pm_flags & PMF_DUCKED )
      {
        if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
          PM_ContinueLegsAnim( LEGS_IDLECR );
        else
          PM_ContinueLegsAnim( NSPA_STAND );
      }
      else
      {
        if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
          PM_ContinueLegsAnim( LEGS_IDLE );
        else
          PM_ContinueLegsAnim( NSPA_STAND );
      }
    }
    return;
  }


  footstep = qfalse;

  if( pm->ps->pm_flags & PMF_DUCKED )
  {
    bobmove = 0.5;  // ducked characters bob much faster

    if( pm->ps->pm_flags & PMF_BACKWARDS_RUN )
    {
      if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
        PM_ContinueLegsAnim( LEGS_BACKCR );
      else
      {
        if( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
          PM_ContinueLegsAnim( NSPA_WALKRIGHT );
        else if( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
          PM_ContinueLegsAnim( NSPA_WALKLEFT );
        else
          PM_ContinueLegsAnim( NSPA_WALKBACK );
      }
    }
    else
    {
      if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
        PM_ContinueLegsAnim( LEGS_WALKCR );
      else
      {
        if( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
          PM_ContinueLegsAnim( NSPA_WALKRIGHT );
        else if( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
          PM_ContinueLegsAnim( NSPA_WALKLEFT );
        else
          PM_ContinueLegsAnim( NSPA_WALK );
      }
    }

    // ducked characters never play footsteps
  }
  else
  {
    if( !( pm->cmd.buttons & BUTTON_WALKING ) )
    {
      bobmove = 0.4f; // faster speeds bob faster

      if( pm->ps->weapon == WP_ALEVEL4 && pm->ps->pm_flags & PMF_CHARGE )
        PM_ContinueLegsAnim( NSPA_CHARGE );
      else if( pm->ps->pm_flags & PMF_BACKWARDS_RUN )
      {
        if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
          PM_ContinueLegsAnim( LEGS_BACK );
        else
        {
          if( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
            PM_ContinueLegsAnim( NSPA_RUNRIGHT );
          else if( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
            PM_ContinueLegsAnim( NSPA_RUNLEFT );
          else
            PM_ContinueLegsAnim( NSPA_RUNBACK );
        }
      }
      else
      {
        if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
          PM_ContinueLegsAnim( LEGS_RUN );
        else
        {
          if( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
            PM_ContinueLegsAnim( NSPA_RUNRIGHT );
          else if( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
            PM_ContinueLegsAnim( NSPA_RUNLEFT );
          else
            PM_ContinueLegsAnim( NSPA_RUN );
        }
      }

      footstep = qtrue;
    }
    else
    {
      bobmove = 0.3f; // walking bobs slow
      if( pm->ps->pm_flags & PMF_BACKWARDS_RUN )
      {
        if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
          PM_ContinueLegsAnim( LEGS_BACKWALK );
        else
        {
          if( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
            PM_ContinueLegsAnim( NSPA_WALKRIGHT );
          else if( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
            PM_ContinueLegsAnim( NSPA_WALKLEFT );
          else
            PM_ContinueLegsAnim( NSPA_WALKBACK );
        }
      }
      else
      {
        if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
          PM_ContinueLegsAnim( LEGS_WALK );
        else
        {
          if( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
            PM_ContinueLegsAnim( NSPA_WALKRIGHT );
          else if( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
            PM_ContinueLegsAnim( NSPA_WALKLEFT );
          else
            PM_ContinueLegsAnim( NSPA_WALK );
        }
      }
    }
  }

  bobmove *= BG_Class( pm->ps->stats[ STAT_CLASS ] )->bobCycle;

  if( pm->ps->stats[ STAT_STATE ] & SS_SPEEDBOOST )
    bobmove *= HUMAN_SPRINT_MODIFIER;

  // check for footstep / splash sounds
  old = pm->ps->bobCycle;
  pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;

  // if we just crossed a cycle boundary, play an apropriate footstep event
  if( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 )
  {
    if( pm->waterlevel == 0 )
    {
      // on ground will only play sounds if running
      if( footstep && !pm->noFootsteps )
        PM_AddEvent( PM_FootstepForSurface( ) );
    }
    else if( pm->waterlevel == 1 )
    {
      // splashing
      PM_AddEvent( EV_FOOTSPLASH );
    }
    else if( pm->waterlevel == 2 )
    {
      // wading / swimming at surface
      PM_AddEvent( EV_SWIM );
    }
    else if( pm->waterlevel == 3 )
    {
      // no sound when completely underwater
    }
  }
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void )
{
  // FIXME?
  //
  // if just entered a water volume, play a sound
  //
  if( !pml.previous_waterlevel && pm->waterlevel )
    PM_AddEvent( EV_WATER_TOUCH );

  //
  // if just completely exited a water volume, play a sound
  //
  if( pml.previous_waterlevel && !pm->waterlevel )
    PM_AddEvent( EV_WATER_LEAVE );

  //
  // check for head just going under water
  //
  if( pml.previous_waterlevel != 3 && pm->waterlevel == 3 )
    PM_AddEvent( EV_WATER_UNDER );

  //
  // check for head just coming out of water
  //
  if( pml.previous_waterlevel == 3 && pm->waterlevel != 3 )
    PM_AddEvent( EV_WATER_CLEAR );
}


/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange( int weapon )
{
  if( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS )
    return;

  if( !BG_InventoryContainsWeapon( weapon, pm->ps->stats ) )
    return;

  if( pm->ps->weaponstate == WEAPON_DROPPING )
    return;

  // cancel a reload
  pm->ps->pm_flags &= ~PMF_WEAPON_RELOAD;
  if( pm->ps->weaponstate == WEAPON_RELOADING )
    pm->ps->weaponTime = 0;

  //special case to prevent storing a charged up lcannon
  if( pm->ps->weapon == WP_LUCIFER_CANNON )
    pm->ps->stats[ STAT_MISC ] = 0;

  if( weapon == WP_LUCIFER_CANNON )
    pm->pmext->luciAmmoReduction = 0;

  pm->ps->weaponstate = WEAPON_DROPPING;
  pm->ps->weaponTime += 200;
  pm->ps->persistant[ PERS_NEWWEAPON ] = weapon;

  //reset build weapon
  pm->ps->stats[ STAT_BUILDABLE ] = BA_NONE;

  if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
  {
    PM_StartTorsoAnim( TORSO_DROP );
    PM_StartWeaponAnim( WANIM_DROP );
  }
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void )
{
  int   weapon;

  PM_AddEvent( EV_CHANGE_WEAPON );
  weapon = pm->ps->persistant[ PERS_NEWWEAPON ];
  if( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS )
    weapon = WP_NONE;

  if( !BG_InventoryContainsWeapon( weapon, pm->ps->stats ) )
    weapon = WP_NONE;

  pm->ps->weapon = weapon;
  pm->ps->weaponstate = WEAPON_RAISING;
  pm->ps->weaponTime += 250;

  if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
  {
    PM_StartTorsoAnim( TORSO_RAISE );
    PM_StartWeaponAnim( WANIM_RAISE );
  }
}


/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation( void )
{
  if( pm->ps->weaponstate == WEAPON_READY )
  {
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
    {
      if( pm->ps->weapon == WP_BLASTER )
        PM_ContinueTorsoAnim( TORSO_STAND2 );
      else
        PM_ContinueTorsoAnim( TORSO_STAND );
    }

    PM_ContinueWeaponAnim( WANIM_IDLE );
  }
}


/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon( void )
{
  int           addTime = 200; //default addTime - should never be used
  qboolean      attack1 = pm->cmd.buttons & BUTTON_ATTACK;
  qboolean      attack2 = pm->cmd.buttons & BUTTON_ATTACK2;
  qboolean      attack3 = pm->cmd.buttons & BUTTON_USE_HOLDABLE;

  // Ignore weapons in some cases
  if( pm->ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
    return;

  // Check for dead player
  if( pm->ps->stats[ STAT_HEALTH ] <= 0 )
  {
    pm->ps->weapon = WP_NONE;
    return;
  }

  // Player is evolving
  if( pm->ps->pm_type == PM_EVOLVING )
    return;

  // Charging for a pounce or canceling a pounce
  if( pm->ps->weapon == WP_ALEVEL3 || pm->ps->weapon == WP_ALEVEL3_UPG ||
      ( pm->ps->weapon == WP_ASPITFIRE  ) )
  {
    int max;

    switch( pm->ps->weapon )
    {
      case WP_ALEVEL3:
        max = LEVEL3_POUNCE_TIME;
        break;

      case WP_ALEVEL3_UPG:
        max = LEVEL3_POUNCE_TIME_UPG;
        break;

      case WP_ASPITFIRE:
        max = SPITFIRE_POUNCE_TIME;
        break;

      default:
        max = LEVEL3_POUNCE_TIME_UPG;
        break;
    }

    if( BG_ClassHasAbility( pm->ps->stats[STAT_CLASS], SCA_CHARGE_STAMINA ) )
    {
      if( max > pm->ps->stats[STAT_STAMINA] )
        max = pm->ps->stats[STAT_STAMINA];
    }

    if( ( pm->cmd.buttons & BUTTON_ATTACK2 ) &&
        ( pm->ps->weapon != WP_ASPITFIRE ||
          pm->waterlevel <= 1 ) )
      pm->ps->stats[ STAT_MISC ] += pml.msec;
    else
      pm->ps->stats[ STAT_MISC ] -= pml.msec;

    if( pm->ps->stats[ STAT_MISC ] > max )
      pm->ps->stats[ STAT_MISC ] = max;
    else if( pm->ps->stats[ STAT_MISC ] < 0 )
      pm->ps->stats[ STAT_MISC ] = 0;
  }

  // Trample charge mechanics
  if( pm->ps->weapon == WP_ALEVEL4 )
  {
    // Charging up
    if( !( pm->ps->stats[ STAT_STATE ] & SS_CHARGING ) )
    {
      // Charge button held
      if( pm->ps->stats[ STAT_MISC ] < LEVEL4_TRAMPLE_CHARGE_TRIGGER &&
          ( pm->cmd.buttons & BUTTON_ATTACK2 ) )
      {
        pm->ps->stats[ STAT_STATE ] &= ~SS_CHARGING;
        if( pm->cmd.forwardmove > 0 )
        {
          int charge = pml.msec;
          vec3_t dir,vel;

          AngleVectors( pm->ps->viewangles, dir, NULL, NULL );
          VectorCopy( pm->ps->velocity, vel );
          vel[2] = 0;
          dir[2] = 0;
          VectorNormalize( vel );
          VectorNormalize( dir );

          charge *= DotProduct( dir, vel );

          pm->ps->stats[ STAT_MISC ] += charge;
        }
        else
          pm->ps->stats[ STAT_MISC ] = 0;
      }

      // Charge button released
      else if( !( pm->ps->stats[ STAT_STATE ] & SS_CHARGING ) )
      {
        if( pm->ps->stats[ STAT_MISC ] > LEVEL4_TRAMPLE_CHARGE_MIN )
        {
          if( pm->ps->stats[ STAT_MISC ] > LEVEL4_TRAMPLE_CHARGE_MAX )
            pm->ps->stats[ STAT_MISC ] = LEVEL4_TRAMPLE_CHARGE_MAX;
          pm->ps->stats[ STAT_MISC ] = pm->ps->stats[ STAT_MISC ] *
                                       LEVEL4_TRAMPLE_DURATION /
                                       LEVEL4_TRAMPLE_CHARGE_MAX;
          pm->ps->stats[ STAT_STATE ] |= SS_CHARGING;
          PM_AddEvent( EV_LEV4_TRAMPLE_START );
        }
        else
          pm->ps->stats[ STAT_MISC ] -= pml.msec;
      }
    }

    // maul cancels trample
    else if( attack1 )
    {
      pm->ps->stats[ STAT_MISC ] = 0;
    }

    // Discharging
    else
    {
      if( pm->ps->stats[ STAT_MISC ] < LEVEL4_TRAMPLE_CHARGE_MIN )
        pm->ps->stats[ STAT_MISC ] = 0;
      else
        pm->ps->stats[ STAT_MISC ] -= pml.msec;

      // If the charger has stopped moving take a chunk of charge away
      if( VectorLength( pm->ps->velocity ) < 64.0f || pm->cmd.rightmove )
        pm->ps->stats[ STAT_MISC ] -= LEVEL4_TRAMPLE_STOP_PENALTY * pml.msec;
    }

    // Charge is over
    if( pm->ps->stats[ STAT_MISC ] <= 0 || pm->cmd.forwardmove <= 0 )
    {
      pm->ps->stats[ STAT_MISC ] = 0;
      if( pm->ps->stats[ STAT_STATE ] & SS_CHARGING )
      {
        pm->ps->weaponTime = 0;
        pm->ps->stats[ STAT_STATE ] &= ~SS_CHARGING;
      }
    }
  }

  // don't allow attack until all buttons are up
  if( pm->ps->pm_flags & PMF_RESPAWNED )
    return;

  if( pm->ps->weapon == WP_LUCIFER_CANNON )
  {
    // Charging up a Lucifer Cannon
    pm->ps->eFlags &= ~EF_WARN_CHARGE;

    // Charging up and charging down
    if( !pm->ps->weaponTime &&
        ( pm->cmd.buttons & BUTTON_ATTACK ) )
    {
      if( ( pm->cmd.buttons & BUTTON_ATTACK2 ) && ( pm->ps->stats[ STAT_MISC ] > 0 ) )
      {
        pm->ps->stats[ STAT_MISC ] -= pml.msec;
        if( pm->ps->stats[ STAT_MISC ] < 0 )
          pm->ps->stats[ STAT_MISC ] = 0;

        if( pm->ps->stats[ STAT_MISC ] > 0 )
        {
          pm->pmext->luciAmmoReduction += LCANNON_CHARGE_AMMO_REDUCE * LCANNON_CHARGE_AMMO *
                                          ( ( float ) ( pml.msec ) ) / LCANNON_CHARGE_TIME_MAX;
          pm->ps->ammo -= ( int ) ( pm->pmext->luciAmmoReduction );
          pm->pmext->luciAmmoReduction -= ( int ) ( pm->pmext->luciAmmoReduction );

          if( pm->ps->ammo <= 0 )
          {
            pm->ps->ammo = 0;
            pm->pmext->luciAmmoReduction = 0;
          }
        }
      }
      else if( !( pm->cmd.buttons & BUTTON_ATTACK2 ) )
        pm->ps->stats[ STAT_MISC ] += pml.msec;
      if( pm->ps->stats[ STAT_MISC ] >= LCANNON_CHARGE_TIME_MAX )
        pm->ps->stats[ STAT_MISC ] = LCANNON_CHARGE_TIME_MAX;
      if( pm->ps->stats[ STAT_MISC ] > pm->ps->ammo * LCANNON_CHARGE_TIME_MAX /
                                              LCANNON_CHARGE_AMMO )
        pm->ps->stats[ STAT_MISC ] = pm->ps->ammo * LCANNON_CHARGE_TIME_MAX /
                                            LCANNON_CHARGE_AMMO;
    }

    // Set overcharging flag so other players can hear the warning beep
    if( pm->ps->stats[ STAT_MISC ] > LCANNON_CHARGE_TIME_WARN )
      pm->ps->eFlags |= EF_WARN_CHARGE;
  }

  // no bite during pounce
  if( ( pm->ps->weapon == WP_ALEVEL3 || pm->ps->weapon == WP_ALEVEL3_UPG )
      && ( pm->cmd.buttons & BUTTON_ATTACK )
      && ( pm->ps->pm_flags & PMF_CHARGE ) )
    return;

  // pump weapon delays (repeat times etc)
  if( pm->ps->weapon == WP_PORTAL_GUN  )
  {
    if( attack2 && pm->humanPortalCreateTime[ PORTAL_BLUE ] > 0 )
      pm->ps->weaponTime = pm->humanPortalCreateTime[ PORTAL_BLUE ];
    else if( attack1 && pm->humanPortalCreateTime[ PORTAL_RED ] > 0 )
      pm->ps->weaponTime = pm->humanPortalCreateTime[ PORTAL_RED ];
    else if( pm->ps->weaponTime > 0 )
      pm->ps->weaponTime -= pml.msec;
  }
  else if( pm->ps->weaponTime > 0 )
    pm->ps->weaponTime -= pml.msec;
  if( pm->ps->weaponTime < 0 )
    pm->ps->weaponTime = 0;
  if( pm->pmext->repairRepeatDelay > 0 )
    pm->pmext->repairRepeatDelay -= pml.msec;
  if( pm->pmext->repairRepeatDelay < 0 )
    pm->pmext->repairRepeatDelay = 0;
  if( pm->pmext->gasTime > 0 )
    pm->pmext->gasTime -= pml.msec;
  if( pm->pmext->gasTime < 0 )
    pm->pmext->gasTime = 0;

  // check for weapon change
  // can't change if weapon is firing, but can change
  // again if lowering or raising
  if( BG_PlayerCanChangeWeapon( pm->ps ) )
  {
    // must press use to switch weapons
    if( pm->cmd.buttons & BUTTON_USE_HOLDABLE )
    {
      if( !( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) )
      {
        if( pm->cmd.weapon < 32 )
        {
          //if trying to select a weapon, select it
          if( pm->ps->weapon != pm->cmd.weapon )
            PM_BeginWeaponChange( pm->cmd.weapon );
        }
        else
        {
          //if trying to toggle an upgrade, toggle it
          if( BG_InventoryContainsUpgrade( pm->cmd.weapon - 32, pm->ps->stats ) ) //sanity check
          {
            if( BG_UpgradeIsActive( pm->cmd.weapon - 32, pm->ps->stats ) )
              BG_DeactivateUpgrade( pm->cmd.weapon - 32, pm->ps->stats );
            else
              BG_ActivateUpgrade( pm->cmd.weapon - 32, pm->ps->stats );
          }
        }
        pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
      }
    }
    else
      pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;

    //something external thinks a weapon change is necessary
    if( pm->ps->pm_flags & PMF_WEAPON_SWITCH )
    {
      pm->ps->pm_flags &= ~PMF_WEAPON_SWITCH;
      if( pm->ps->weapon != WP_NONE )
      {
        // drop the current weapon
        PM_BeginWeaponChange( pm->ps->persistant[ PERS_NEWWEAPON ] );
      }
      else
      {
        // no current weapon, so just raise the new one
        PM_FinishWeaponChange( );
      }
    }
  }

  // release advanced basilisk gas ( better out than in )
  if( BG_Weapon( pm->ps->weapon )->hasAltMode &&
      ( pm->ps->weapon == WP_ALEVEL1_UPG ) &&
      ( pm->pmext->gasTime <= 0 ) && attack2 )
  {
    pm->ps->generic1 = WPM_SECONDARY;
    PM_AddEvent( EV_FIRE_WEAPON2 );
    pm->pmext->gasTime += BG_Weapon( pm->ps->weapon )->repeatRate2;  // basi gas has an independent timer
  }
      

  if( pm->ps->weaponTime > 0 )
    return;

  // change weapon if time
  if( pm->ps->weaponstate == WEAPON_DROPPING )
  {
    PM_FinishWeaponChange( );
    return;
  }

  // Set proper animation
  if( pm->ps->weaponstate == WEAPON_RAISING )
  {
    pm->ps->weaponstate = WEAPON_READY;
    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
    {
      if( pm->ps->weapon == WP_BLASTER )
        PM_ContinueTorsoAnim( TORSO_STAND2 );
      else
        PM_ContinueTorsoAnim( TORSO_STAND );
    }

    PM_ContinueWeaponAnim( WANIM_IDLE );

    return;
  }

  // check for out of ammo
  if( !pm->ps->ammo && !pm->ps->clips && !BG_Weapon( pm->ps->weapon )->infiniteAmmo )
  {
    if( attack1 ||
        ( BG_Weapon( pm->ps->weapon )->hasAltMode && attack2 ) ||
        ( BG_Weapon( pm->ps->weapon )->hasThirdMode && attack3 ) )
    {
      PM_AddEvent( EV_NOAMMO );
      pm->ps->weaponTime += 500;
    }

    if( pm->ps->weaponstate == WEAPON_FIRING )
      pm->ps->weaponstate = WEAPON_READY;

    return;
  }

  //done reloading so give em some ammo
  if( pm->ps->weaponstate == WEAPON_RELOADING )
  {
    pm->ps->clips--;
    pm->ps->ammo = BG_Weapon( pm->ps->weapon )->maxAmmo;

    if( BG_Weapon( pm->ps->weapon )->usesEnergy &&
        ( BG_InventoryContainsUpgrade( UP_BATTPACK, pm->ps->stats ) ||
          BG_InventoryContainsUpgrade( UP_BATTLESUIT, pm->ps->stats ) ) )
      pm->ps->ammo *= BATTPACK_MODIFIER;

    if( pm->ps->weapon == WP_LUCIFER_CANNON )
      pm->pmext->luciAmmoReduction = 0;

    //allow some time for the weapon to be raised
    pm->ps->weaponstate = WEAPON_RAISING;
    PM_StartTorsoAnim( TORSO_RAISE );
    pm->ps->weaponTime += 250;
    return;
  }

  // check for end of clip
  if( !BG_Weapon( pm->ps->weapon )->infiniteAmmo &&
      ( pm->ps->ammo <= 0 || ( pm->ps->pm_flags & PMF_WEAPON_RELOAD ) ) &&
      pm->ps->clips > 0 )
  {
    pm->ps->pm_flags &= ~PMF_WEAPON_RELOAD;
    pm->ps->weaponstate = WEAPON_RELOADING;

    //drop the weapon
    PM_StartTorsoAnim( TORSO_DROP );
    PM_StartWeaponAnim( WANIM_RELOAD );

    pm->ps->weaponTime += BG_Weapon( pm->ps->weapon )->reloadTime;
    return;
  }

  //check if non-auto primary/secondary attacks are permited
  switch( pm->ps->weapon )
  {
    case WP_ALEVEL0:
      //venom is only autohit
      return;

    case WP_ALEVEL1:
    case WP_ALEVEL1_UPG:
      // autoswipe while grabbing
      if( pm->ps->stats[ STAT_STATE ] & SS_GRABBING )
      {
        attack1 = qtrue;
      } else if( !attack1 && !attack2 && !attack3 )
      {
        pm->ps->weaponTime = 0;
        pm->ps->weaponstate = WEAPON_READY;
        return;
      }
      break;
    case WP_ALEVEL3:
    case WP_ALEVEL3_UPG:
      //pouncing has primary secondary AND autohit procedures
      // pounce is autohit
      if( !attack1 && !attack2 && !attack3 )
        return;
      break;
	  
	case WP_ASPITFIRE:
      //pouncing has primary secondary AND autohit procedures
      // pounce is autohit
      if( !attack1 && !attack2 )
        return;
      break;

    case WP_LUCIFER_CANNON:
      attack3 = qfalse;

      // Can't fire secondary while primary is charging
      if( attack1 || pm->ps->stats[ STAT_MISC ] > 0 )
        attack2 = qfalse;

      if( ( attack1 || pm->ps->stats[ STAT_MISC ] == 0 ) && !attack2 )
      {
        pm->ps->weaponTime = 0;

        // Charging
        if( pm->ps->stats[ STAT_MISC ] < LCANNON_CHARGE_TIME_MAX )
        {
          pm->ps->weaponstate = WEAPON_READY;
          return;
        }
      }

      if( pm->ps->stats[ STAT_MISC ] > LCANNON_CHARGE_TIME_MIN )
      {
        // The lucifer cannon has overcharged
        // Fire primary attack
        attack1 = qtrue;
        attack2 = qfalse;
      }
      else if( pm->ps->stats[ STAT_MISC ] > 0 )
      {
        // Not enough charge
        pm->ps->stats[ STAT_MISC ] = 0;
        pm->ps->weaponTime = 0;
        pm->ps->weaponstate = WEAPON_READY;
        return;
      }
      else if( !attack2 )
      {
        // Idle
        pm->ps->weaponTime = 0;
        pm->ps->weaponstate = WEAPON_READY;
        return;
      }
      break;

    case WP_LAS_GUN:
    case WP_MASS_DRIVER:
      attack2 = attack3 = qfalse;
      // attack2 is handled on the client for zooming (cg_view.c)

      if( !attack1 )
      {
        pm->ps->weaponTime = 0;
        pm->ps->weaponstate = WEAPON_READY;
        return;
      }
      break;

    case WP_LAUNCHER:
      if (!attack1 && !attack2 )
      {
        pm->ps->weaponTime = 0;
        pm->ps->weaponstate = WEAPON_READY;
        return;
      }
      break;

    default:
      if( !attack1 && !attack2 && !attack3 )
      {
        pm->ps->weaponTime = 0;
        pm->ps->weaponstate = WEAPON_READY;
        return;
      }
      break;
  }

  // fire events for non auto weapons
  if( attack3 )
  {
    if( BG_Weapon( pm->ps->weapon )->hasThirdMode )
    {
      //hacky special case for slowblob
      if( pm->ps->weapon == WP_ALEVEL3_UPG && !pm->ps->ammo )
      {
        pm->ps->weaponTime += 200;
        return;
      }

      pm->ps->generic1 = WPM_TERTIARY;
      PM_AddEvent( EV_FIRE_WEAPON3 );
      addTime = BG_Weapon( pm->ps->weapon )->repeatRate3;
    }
    else
    {
      pm->ps->weaponTime = 0;
      pm->ps->weaponstate = WEAPON_READY;
      pm->ps->generic1 = WPM_NOTFIRING;
      return;
    }
  }
  else if( attack2 && ( pm->ps->weapon != WP_ALEVEL1_UPG ) )
  {
    if( BG_Weapon( pm->ps->weapon )->hasAltMode )
    {
      pm->ps->generic1 = WPM_SECONDARY;
      PM_AddEvent( EV_FIRE_WEAPON2 );
      addTime = BG_Weapon( pm->ps->weapon )->repeatRate2;
    }
    else
    {
      pm->ps->weaponTime = 0;
      pm->ps->weaponstate = WEAPON_READY;
      pm->ps->generic1 = WPM_NOTFIRING;
      return;
    }
  }
  else if( attack1 )
  {
    pm->ps->generic1 = WPM_PRIMARY;
    PM_AddEvent( EV_FIRE_WEAPON );
    addTime = BG_Weapon( pm->ps->weapon )->repeatRate1;
  }

  // fire events for autohit weapons
  if( pm->autoWeaponHit[ pm->ps->weapon ] )
  {
    switch( pm->ps->weapon )
    {
      case WP_ALEVEL0:
        pm->ps->generic1 = WPM_PRIMARY;
        PM_AddEvent( EV_FIRE_WEAPON );
        addTime = BG_Weapon( pm->ps->weapon )->repeatRate1;
        break;
		
      case WP_ASPITFIRE:
        pm->ps->generic1 = WPM_SECONDARY;
        PM_AddEvent( EV_FIRE_WEAPON2 );
        addTime = BG_Weapon( pm->ps->weapon )->repeatRate2;
        break;

      case WP_ALEVEL3:
      case WP_ALEVEL3_UPG:
        pm->ps->generic1 = WPM_SECONDARY;
        PM_AddEvent( EV_FIRE_WEAPON2 );
        addTime = BG_Weapon( pm->ps->weapon )->repeatRate2;
        break;

      default:
        break;
    }
  }

  if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
  {
    //FIXME: this should be an option in the client weapon.cfg
    switch( pm->ps->weapon )
    {
      case WP_FLAMER:
        if( pm->ps->weaponstate == WEAPON_READY )
        {
          PM_StartTorsoAnim( TORSO_ATTACK );
          PM_StartWeaponAnim( WANIM_ATTACK1 );
        }
        break;

      case WP_BLASTER:
        PM_StartTorsoAnim( TORSO_ATTACK2 );
        PM_StartWeaponAnim( WANIM_ATTACK1 );
        break;

      default:
        PM_StartTorsoAnim( TORSO_ATTACK );
        PM_StartWeaponAnim( WANIM_ATTACK1 );
        break;
    }
  }
  else
  {
    int num = rand( );

    //FIXME: it would be nice to have these hard coded policies in
    //       weapon.cfg
    switch( pm->ps->weapon )
    {
      case WP_ALEVEL1_UPG:
      case WP_ALEVEL1:
        if( attack1 )
        {
          num /= RAND_MAX / 6 + 1;
          PM_ForceLegsAnim( NSPA_ATTACK1 );
          PM_StartWeaponAnim( WANIM_ATTACK1 + num );
        }
        break;

      case WP_ALEVEL2_UPG:
        if( attack2 )
        {
          PM_ForceLegsAnim( NSPA_ATTACK2 );
          PM_StartWeaponAnim( WANIM_ATTACK7 );
        }
      case WP_ALEVEL2:
        if( attack1 )
        {
          num /= RAND_MAX / 6 + 1;
          PM_ForceLegsAnim( NSPA_ATTACK1 );
          PM_StartWeaponAnim( WANIM_ATTACK1 + num );
        }
        break;

      case WP_ALEVEL4:
        num /= RAND_MAX / 3 + 1;
        PM_ForceLegsAnim( NSPA_ATTACK1 + num );
        PM_StartWeaponAnim( WANIM_ATTACK1 + num );
        break;

      default:
        if( attack1 )
        {
          PM_ForceLegsAnim( NSPA_ATTACK1 );
          PM_StartWeaponAnim( WANIM_ATTACK1 );
        }
        else if( attack2 )
        {
          PM_ForceLegsAnim( NSPA_ATTACK2 );
          PM_StartWeaponAnim( WANIM_ATTACK2 );
        }
        else if( attack3 )
        {
          PM_ForceLegsAnim( NSPA_ATTACK3 );
          PM_StartWeaponAnim( WANIM_ATTACK3 );
        }
        break;
    }

    pm->ps->torsoTimer = TIMER_ATTACK;
  }

  pm->ps->weaponstate = WEAPON_FIRING;

  // take an ammo away if not infinite
  if( !BG_Weapon( pm->ps->weapon )->infiniteAmmo ||
      ( pm->ps->weapon == WP_ALEVEL3_UPG && attack3 ) )
  {
    // Special case for lcannon
    if( pm->ps->weapon == WP_LUCIFER_CANNON && attack1 && !attack2 )
      pm->ps->ammo -= ( pm->ps->stats[ STAT_MISC ] * LCANNON_CHARGE_AMMO +
                LCANNON_CHARGE_TIME_MAX - 1 ) / LCANNON_CHARGE_TIME_MAX;
    else
      pm->ps->ammo--;

    // Stay on the safe side
    if( pm->ps->ammo < 0 )
      pm->ps->ammo = 0;
  }

  //FIXME: predicted angles miss a problem??
  if( pm->ps->weapon == WP_CHAINGUN )
  {
    if( pm->ps->pm_flags & PMF_DUCKED ||
        BG_InventoryContainsUpgrade( UP_BATTLESUIT, pm->ps->stats ) )
    {
      pm->ps->delta_angles[ PITCH ] -= ANGLE2SHORT( ( ( random() * 0.5 ) - 0.125 ) * ( 30 / (float)addTime ) );
      pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT( ( ( random() * 0.5 ) - 0.25 ) * ( 30.0 / (float)addTime ) );
    }
    else
    {
      pm->ps->delta_angles[ PITCH ] -= ANGLE2SHORT( ( ( random() * 8 ) - 2 ) * ( 30.0 / (float)addTime ) );
      pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT( ( ( random() * 8 ) - 4 ) * ( 30.0 / (float)addTime ) );
    }
  }

  pm->ps->weaponTime += addTime;
}

/*
================
PM_Animate
================
*/
static void PM_Animate( void )
{
  if( PM_Paralyzed( pm->ps->pm_type ) )
    return;

  if( pm->cmd.buttons & BUTTON_GESTURE )
  {
    if( !pm->tauntSpam && pm->ps->tauntTimer > 0 )
        return;

    if( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
    {
      if( pm->ps->torsoTimer == 0 )
      {
        PM_StartTorsoAnim( TORSO_GESTURE );
        pm->ps->torsoTimer = TIMER_GESTURE;
        if( !pm->tauntSpam )
          pm->ps->tauntTimer = TIMER_GESTURE;

        PM_AddEvent( EV_TAUNT );
      }
    }
    else
    {
      if( pm->ps->torsoTimer == 0 )
      {
        PM_ForceLegsAnim( NSPA_GESTURE );
        pm->ps->torsoTimer = TIMER_GESTURE;
        if( !pm->tauntSpam )
          pm->ps->tauntTimer = TIMER_GESTURE;

        PM_AddEvent( EV_TAUNT );
      }
    }
  }
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void )
{
  // drop misc timing counter
  if( pm->ps->pm_time )
  {
    if( pml.msec >= pm->ps->pm_time )
    {
      pm->ps->pm_flags &= ~PMF_ALL_TIMES;
      pm->ps->pm_time = 0;
    }
    else
      pm->ps->pm_time -= pml.msec;
  }

  // drop animation counter
  if( pm->ps->legsTimer > 0 )
  {
    pm->ps->legsTimer -= pml.msec;

    if( pm->ps->legsTimer < 0 )
      pm->ps->legsTimer = 0;
  }

  if( pm->ps->torsoTimer > 0 )
  {
    pm->ps->torsoTimer -= pml.msec;

    if( pm->ps->torsoTimer < 0 )
      pm->ps->torsoTimer = 0;
  }

  if( pm->ps->tauntTimer > 0 )
  {
    pm->ps->tauntTimer -= pml.msec;

    if( pm->ps->tauntTimer < 0 )
    {
      pm->ps->tauntTimer = 0;
    }
  }

  // the jump timer increases
  if( pm->ps->persistant[PERS_JUMPTIME] < 0 )
    pm->ps->persistant[PERS_JUMPTIME] = 0;
  else
    pm->ps->persistant[PERS_JUMPTIME] += pml.msec;
}


/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd )
{
  short   temp[ 3 ];
  int     i;
  vec3_t  axis[ 3 ], rotaxis[ 3 ];
  vec3_t  tempang;

  if( ps->pm_type == PM_INTERMISSION )
    return;   // no view changes at all

  if( ps->pm_type != PM_SPECTATOR && ps->stats[ STAT_HEALTH ] <= 0 )
    return;   // no view changes at all

  // circularly clamp the angles with deltas
  for( i = 0; i < 3; i++ )
  {
    if( i == ROLL )
    {
      // Guard against speed hack
      temp[ i ] = ps->delta_angles[ i ];

#ifdef CGAME
      // Assert here so that if cmd->angles[ i ] becomes non-zero
      // for a legitimate reason we can tell where and why it's
      // being ignored
      assert( cmd->angles[ i ] == 0 );
#endif

    }
    else
      temp[ i ] = cmd->angles[ i ] + ps->delta_angles[ i ];

    if( i == PITCH )
    {
      // don't let the player look up or down more than 90 degrees
      if( temp[ i ] > 16000 )
      {
        ps->delta_angles[ i ] = 16000 - cmd->angles[ i ];
        temp[ i ] = 16000;
      }
      else if( temp[ i ] < -16000 )
      {
        ps->delta_angles[ i ] = -16000 - cmd->angles[ i ];
        temp[ i ] = -16000;
      }
    }
    tempang[ i ] = SHORT2ANGLE( temp[ i ] );
  }

  //convert viewangles -> axis
  AnglesToAxis( tempang, axis );

  if( !( ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) ||
      !BG_RotateAxis( ps->grapplePoint, axis, rotaxis, qfalse,
                      ps->eFlags & EF_WALLCLIMBCEILING ) )
    AxisCopy( axis, rotaxis );

  //convert the new axis back to angles
  AxisToAngles( rotaxis, tempang );

  //force angles to -180 <= x <= 180
  for( i = 0; i < 3; i++ )
  {
    while( tempang[ i ] > 180.0f )
      tempang[ i ] -= 360.0f;

    while( tempang[ i ] < -180.0f )
      tempang[ i ] += 360.0f;
  }

  PM_CalculateAngularVelocity( ps, cmd );

  //actually set the viewangles
  for( i = 0; i < 3; i++ )
    ps->viewangles[ i ] = tempang[ i ];

  //pull the view into the lock point
  if( ps->pm_type == PM_GRABBED &&
      !BG_InventoryContainsUpgrade( UP_BATTLESUIT, ps->stats ) )
  {
    vec3_t  dir, angles;

    ByteToDir( ps->stats[ STAT_VIEWLOCK ], dir );
    vectoangles( dir, angles );

    for( i = 0; i < 3; i++ )
    {
      float diff = AngleSubtract( ps->viewangles[ i ], angles[ i ] );

      while( diff > 180.0f )
        diff -= 360.0f;
      while( diff < -180.0f )
        diff += 360.0f;

      if( diff < -90.0f )
        ps->delta_angles[ i ] += ANGLE2SHORT( fabs( diff ) - 90.0f );
      else if( diff > 90.0f )
        ps->delta_angles[ i ] -= ANGLE2SHORT( fabs( diff ) - 90.0f );

      if( diff < 0.0f )
        ps->delta_angles[ i ] += ANGLE2SHORT( fabs( diff ) * 0.05f );
      else if( diff > 0.0f )
        ps->delta_angles[ i ] -= ANGLE2SHORT( fabs( diff ) * 0.05f );
    }
  }
}

void PM_CalculateAngularVelocity( playerState_t *ps, const usercmd_t *cmd )
{
  short   temp[ 3 ];
  int     i;
  vec3_t  axis[ 3 ], rotaxis[ 3 ];
  vec3_t  tempang, cartNew, cartOld;

  if( ps->pm_type == PM_INTERMISSION )
    return;   // no view changes at all

  if( ps->pm_type != PM_SPECTATOR && ps->stats[ STAT_HEALTH ] <= 0 )
    return;   // no view changes at all

  // circularly clamp the angles with deltas
  for( i = 0; i < 3; i++ )
  {
    if( i == ROLL )
    {
      // Guard against speed hack
      temp[ i ] = ps->delta_angles[ i ];

#ifdef CGAME
      // Assert here so that if cmd->angles[ i ] becomes non-zero
      // for a legitimate reason we can tell where and why it's
      // being ignored
      assert( cmd->angles[ i ] == 0 );
#endif

    }
    else
      temp[ i ] = cmd->angles[ i ] + ps->delta_angles[ i ];

    if( i == PITCH )
    {
      // don't let the player look up or down more than 90 degrees
      if( temp[ i ] > 16000 )
      {
        temp[ i ] = 16000;
      }
      else if( temp[ i ] < -16000 )
      {
        temp[ i ] = -16000;
      }
    }
    tempang[ i ] = SHORT2ANGLE( temp[ i ] );
  }

  //convert viewangles -> axis
  AnglesToAxis( tempang, axis );

  if( !( ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) ||
      !BG_RotateAxis( ps->grapplePoint, axis, rotaxis, qfalse,
                      ps->eFlags & EF_WALLCLIMBCEILING ) )
    AxisCopy( axis, rotaxis );

  //convert the new axis back to angles
  AxisToAngles( rotaxis, tempang );

  //force angles to -180 <= x <= 180
  for( i = 0; i < 3; i++ )
  {
    while( tempang[ i ] > 180.0f )
      tempang[ i ] -= 360.0f;

    while( tempang[ i ] < -180.0f )
      tempang[ i ] += 360.0f;
  }

  // actually calculate the angular velocity

  if( pm->pmext->updateAnglesTime != pm->ps->commandTime )
  {
    VectorCopy( pm->pmext->previousUpdateAngles, pm->pmext->previousFrameAngles );
    pm->pmext->diffAnglesPeriod = (float)( pm->ps->commandTime - pm->pmext->updateAnglesTime );
  }

  if( !( VectorCompare( tempang, pm->pmext->previousUpdateAngles ) &&
         pm->pmext->updateAnglesTime == pm->ps->commandTime ) )
  {
    VectorCopy( tempang, pm->pmext->previousUpdateAngles );

    cartOld[0] = (float)sin( (double)( pm->pmext->previousFrameAngles[ PITCH  ] + 90 ) ) *
                 (float)cos( (double)( pm->pmext->previousFrameAngles[ YAW ] ) );

    cartOld[1] = (float)sin( (double)( pm->pmext->previousFrameAngles[ PITCH  ] + 90 ) ) *
                 (float)sin( (double)( pm->pmext->previousFrameAngles[ YAW ] ) );

    cartOld[2] = (float)cos( (double)( pm->pmext->previousFrameAngles[ PITCH  ] + 90 ) );

    cartNew[0] = (float)sin( (double)( tempang[ PITCH ] + 90 ) ) *
                 (float)cos( (double)( tempang[ YAW ] ) );

    cartNew[1] = (float)sin( (double)( tempang[ PITCH ] + 90 ) ) *
                 (float)sin( (double)( tempang[ YAW ] ) );

    cartNew[2] = (float)cos( (double)( tempang[ PITCH ] + 90 ) );

    VectorSubtract( cartNew,  cartOld, pm->pmext->angularVelocity );

    VectorScale( pm->pmext->angularVelocity, 1.000f/pm->pmext->diffAnglesPeriod,
                 pm->pmext->angularVelocity );
  }

  pm->pmext->updateAnglesTime = pm->ps->commandTime;
}

/*
================
PmoveSingle

================
*/

void PmoveSingle( pmove_t *pmove )
{
  pm = pmove;

  // this counter lets us debug movement problems with a journal
  // by setting a conditional breakpoint for the previous frame
  c_pmove++;

  // clear results
  pm->numtouch = 0;
  pm->watertype = 0;
  pm->waterlevel = 0;

  if( pm->ps->stats[ STAT_HEALTH ] <= 0 )
    pm->tracemask &= ~CONTENTS_BODY;  // corpses can fly through bodies

  // make sure walking button is clear if they are running, to avoid
  // proxy no-footsteps cheats
  if( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 )
    pm->cmd.buttons &= ~BUTTON_WALKING;

  // set the firing flag for continuous beam weapons
  if( !(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION &&
      ( pm->cmd.buttons & BUTTON_ATTACK ) &&
      ( ( pm->ps->ammo > 0 || pm->ps->clips > 0 ) || BG_Weapon( pm->ps->weapon )->infiniteAmmo ) )
    pm->ps->eFlags |= EF_FIRING;
  else
    pm->ps->eFlags &= ~EF_FIRING;

  // set the firing flag for continuous beam weapons
  if( !(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION &&
      ( pm->cmd.buttons & BUTTON_ATTACK2 ) &&
      ( ( pm->ps->ammo > 0 || pm->ps->clips > 0 ) || BG_Weapon( pm->ps->weapon )->infiniteAmmo ) )
    pm->ps->eFlags |= EF_FIRING2;
  else
    pm->ps->eFlags &= ~EF_FIRING2;

  // set the firing flag for continuous beam weapons
  if( !(pm->ps->pm_flags & PMF_RESPAWNED) && pm->ps->pm_type != PM_INTERMISSION &&
      ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) &&
      ( ( pm->ps->ammo > 0 || pm->ps->clips > 0 ) || BG_Weapon( pm->ps->weapon )->infiniteAmmo ) )
    pm->ps->eFlags |= EF_FIRING3;
  else
    pm->ps->eFlags &= ~EF_FIRING3;


  // clear the respawned flag if attack and use are cleared
  if( pm->ps->stats[STAT_HEALTH] > 0 &&
      !( pm->cmd.buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) )
    pm->ps->pm_flags &= ~PMF_RESPAWNED;

  // if talk button is down, dissallow all other input
  // this is to prevent any possible intercept proxy from
  // adding fake talk balloons
  if( pmove->cmd.buttons & BUTTON_TALK )
  {
    pmove->cmd.buttons = BUTTON_TALK;
    pmove->cmd.forwardmove = 0;
    pmove->cmd.rightmove = 0;

    if( pmove->cmd.upmove > 0 )
      pmove->cmd.upmove = 0;
  }

  // clear all pmove local vars
  memset( &pml, 0, sizeof( pml ) );

  // determine the time
  pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;

  if( pml.msec < 1 )
    pml.msec = 1;
  else if( pml.msec > 200 )
    pml.msec = 200;

  pm->ps->commandTime = pmove->cmd.serverTime;

  // save old org in case we get stuck
  VectorCopy( pm->ps->origin, pml.previous_origin );

  // save old velocity for crashlanding
  VectorCopy( pm->ps->velocity, pml.previous_velocity );

  pml.frametime = pml.msec * 0.001;

  AngleVectors( pm->ps->viewangles, pml.forward, pml.right, pml.up );

  if( pm->cmd.upmove < 10 )
  {
    // not holding jump

    if( pm->ps->pm_flags & PMF_HOPPED )
    {
      // disable bunny hop
      pm->ps->pm_flags &= ~PMF_ALL_HOP_FLAGS;
    }

    pm->ps->pm_flags &= ~PMF_JUMP_HELD;

    // deactivate the jet
    if( BG_InventoryContainsUpgrade( UP_JETPACK, pm->ps->stats ) &&
        BG_UpgradeIsActive( UP_JETPACK, pm->ps->stats ) )
    {
      BG_DeactivateUpgrade( UP_JETPACK, pm->ps->stats );
      pm->ps->pm_type = PM_NORMAL;
    }
  }

  // decide if backpedaling animations should be used
  if( pm->cmd.forwardmove < 0 )
    pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
  else if( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) )
    pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;

  if( PM_Paralyzed( pm->ps->pm_type ) )
  {
    pm->cmd.forwardmove = 0;
    pm->cmd.rightmove = 0;
    pm->cmd.upmove = 0;
  }

  if( pm->ps->pm_type == PM_SPECTATOR )
  {
    // update the viewangles
    PM_UpdateViewAngles( pm->ps, &pm->cmd );
    PM_CheckDuck( );
    PM_FlyMove( );
    PM_DropTimers( );
    return;
  }

  if( pm->ps->weapon == WP_ABUILD ||
      pm->ps->weapon == WP_ABUILD2 ||
      pm->ps->weapon == WP_HBUILD )
  {
    // check for precise buildable placement mode
    if( pm->cmd.buttons & BUTTON_WALKING )
      pm->ps->stats[ STAT_STATE ] |= SS_PRECISE_BUILD;
    else
      pm->ps->stats[ STAT_STATE ] &= ~SS_PRECISE_BUILD;
  }
    

  if( pm->ps->pm_type == PM_NOCLIP )
  {
    PM_UpdateViewAngles( pm->ps, &pm->cmd );
    PM_NoclipMove( );
    PM_SetViewheight( );
    PM_Weapon( );
    PM_DropTimers( );
    return;
  }

  if( pm->ps->pm_type == PM_FREEZE)
    return;   // no movement at all

  if( pm->ps->pm_type == PM_INTERMISSION )
    return;   // no movement at all

  // set watertype, and waterlevel
  PM_SetWaterLevel( );
  pml.previous_waterlevel = pmove->waterlevel;

  // set mins, maxs, and viewheight
  PM_CheckDuck( );

  PM_CheckLadder( );

  // jet activation
  if( pm->cmd.upmove >= 10 &&
      BG_InventoryContainsUpgrade( UP_JETPACK, pm->ps->stats ) &&
      !BG_UpgradeIsActive( UP_JETPACK, pm->ps->stats ) &&
      ( pm->ps->groundEntityNum == ENTITYNUM_NONE ||
        pm->ps->stats[ STAT_STATE ] & SS_GRABBED ) &&
      !(pm->ps->pm_flags & PMF_JUMP_HELD) &&
      pm->ps->stats[ STAT_FUEL ] > JETPACK_FUEL_MIN_START &&
      ( pm->ps->pm_type == PM_NORMAL || 
        ( pm->ps->pm_type == PM_GRABBED && 
          pm->ps->stats[ STAT_STATE ] & SS_GRABBED ) ) &&
      !pml.ladder )
  {
    BG_ActivateUpgrade( UP_JETPACK, pm->ps->stats );
    pm->ps->pm_type = PM_JETPACK;

    if( pm->ps->stats[ STAT_FUEL ] >= JETPACK_ACT_BOOST_FUEL_USE )
    {
      // give an upwards boost when activating the jet
      pm->ps->persistant[PERS_JUMPTIME] = 0;
      PM_AddEvent( EV_JETJUMP );
    }
  }

  // set groundentity
  PM_GroundTrace( );

  // update the viewangles
  PM_UpdateViewAngles( pm->ps, &pm->cmd );

  if( pm->ps->pm_type == PM_DEAD || pm->ps->pm_type == PM_GRABBED ||
      pm->ps->pm_type == PM_EVOLVING )
    PM_DeadMove( );

  PM_DropTimers( );

  // Disable dodge
  //PM_CheckDodge( );

  // cancel the spitfire's airpounce
  if(  pm->ps->weapon == WP_ASPITFIRE &&
       pm->pmext->pouncePayload > 0 &&
       ( pm->ps->commandTime > pm->pmext->pouncePayloadTime +
                               SPITFIRE_PAYLOAD_DISCHARGE_TIME ||
         pm->ps->speed >= VectorLength( pm->ps->velocity) ) )
  {
    pm->pmext->pouncePayload = 0;
    pm->pmext->pouncePayloadTime = -1;
  }

  if( pm->ps->pm_type == PM_JETPACK )
    PM_JetPackMove( );
  else if( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
    PM_WaterJumpMove( );
  else if( pm->waterlevel > 1 )
    PM_WaterMove( );
  else if( pml.ladder )
    PM_LadderMove( );
  else if( pml.walking )
  {
    if( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_WALLCLIMBER ) &&
        ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
      PM_ClimbMove( ); // walking on any surface
    else
      PM_WalkMove( ); // walking on ground
  }
  else if( pm->ps->pm_type == PM_SPITFIRE_FLY )
    PM_SpitfireFlyMove( );
  else
    PM_AirMove( );

  PM_Animate( );

  // set groundentity, watertype, and waterlevel
  PM_GroundTrace( );

  // update the viewangles
  PM_UpdateViewAngles( pm->ps, &pm->cmd );

  PM_SetWaterLevel( );

  // weapons
  PM_Weapon( );

  // torso animation
  PM_TorsoAnimation( );

  // footstep events / legs animations
  PM_Footsteps( );

  // entering / leaving water splashes
  PM_WaterEvents( );

  // snap some parts of playerstate to save network bandwidth
  #ifdef Q3_VM
    trap_SnapVector( pm->ps->velocity );
  #else
    Q_SnapVector( pm->ps->velocity );
  #endif
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove( pmove_t *pmove )
{
  int finalTime;

  finalTime = pmove->cmd.serverTime;

  if( finalTime < pmove->ps->commandTime )
    return; // should not happen

  if( finalTime > pmove->ps->commandTime + 1000 )
    pmove->ps->commandTime = finalTime - 1000;

  pmove->ps->pmove_framecount = ( pmove->ps->pmove_framecount + 1 ) & ( ( 1 << PS_PMOVEFRAMECOUNTBITS ) - 1 );

  // chop the move up if it is too long, to prevent framerate
  // dependent behavior
  while( pmove->ps->commandTime != finalTime )
  {
    int   msec;

    msec = finalTime - pmove->ps->commandTime;

    if( pmove->pmove_fixed )
    {
      if( msec > pmove->pmove_msec )
        msec = pmove->pmove_msec;
    }
    else
    {
      if( msec > 66 )
        msec = 66;
    }

    pmove->cmd.serverTime = pmove->ps->commandTime + msec;
    PmoveSingle( pmove );
  }
}
