/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development
Copyright (C) 2015-2019 GrangerHub

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, see <https://www.gnu.org/licenses/>

===========================================================================
*/

// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_public.h"

/*
============
BG_HP2SU

Convert health point into health subunits
============
*/
int BG_HP2SU( int healthPoints )
{
  if( healthPoints < ( INT_MAX / HEALTH_POINT_SUBUNIT_FACTOR ) )
    return healthPoints * HEALTH_POINT_SUBUNIT_FACTOR;

  // prevent integer overflow
  return INT_MAX;
}

/*
============
BG_SU2HP

Convert health subunits to health points
============
*/
int BG_SU2HP( int healthSubUnits )
{
  int healthPoints = healthSubUnits / HEALTH_POINT_SUBUNIT_FACTOR;

  // only let healthPoints = 0 if healthSubUnits is 0
  if( !healthPoints &&
      healthSubUnits )
    return 1;

  return healthPoints;
}

#ifdef Q3_VM
  #define FS_FOpenFileByMode trap_FS_FOpenFile
  #define FS_Read2 trap_FS_Read
  #define FS_Write trap_FS_Write
  #define FS_FCloseFile trap_FS_FCloseFile
  #define FS_Seek trap_FS_Seek
  #define FS_GetFileList trap_FS_GetFileList
  #define Cvar_VariableStringBuffer trap_Cvar_VariableStringBuffer
#else
  int  FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode );
  int  FS_Read2( void *buffer, int len, fileHandle_t f );
  void FS_FCloseFile( fileHandle_t f );
  int  FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
#endif

static const teamAttributes_t bg_teamList[ ] =
{
  {
    TEAM_NONE,             //int       number;
    "spectate",            //char     *name;
    "spectator",           //char     *name2;
    "Spectators",          //char     *humanName;
  },
  {
    TEAM_ALIENS,           //int       number;
    "aliens",              //char     *name;
    "alien",               //char     *name2;
    "Aliens",              //char     *humanName;
  },
  {
    TEAM_HUMANS,           //int       number;
    "humans",              //char     *name;
    "human",               //char     *name2;
    "Humans",              //char     *humanName;
  },
};

size_t bg_numTeams = ARRAY_LEN( bg_teamList );

static const teamAttributes_t nullTeam = { 0 };

static const modAttributes_t bg_modList[ ] =
{
  {
    MOD_UNKNOWN,             //meansOfDeath_t means_of_death;
    "MOD_UNKNOWN",           //char           *name;
    MODTYPE_GENERIC,         //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qfalse,                  //qboolean       last_spawn_protection;
    qfalse,                  //qboolean       friendly_fire_protection;
    qfalse,                  //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SHOTGUN,             //meansOfDeath_t means_of_death;
    "MOD_SHOTGUN",           //char           *name;
    MODTYPE_BALLISTIC,       //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_DBSHOTGUN,           //meansOfDeath_t means_of_death;
    "MOD_DBSHOTGUN",         //char           *name;
    MODTYPE_BALLISTIC,       //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_BLASTER,             //meansOfDeath_t means_of_death;
    "MOD_BLASTER",           //char           *name;
    MODTYPE_ENERGY,          //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_PAINSAW,             //meansOfDeath_t means_of_death;
    "MOD_PAINSAW",           //char           *name;
    MODTYPE_ENERGY,          //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_MACHINEGUN,          //meansOfDeath_t means_of_death;
    "MOD_MACHINEGUN",        //char           *name;
    MODTYPE_BALLISTIC,       //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_CHAINGUN,            //meansOfDeath_t means_of_death;
    "MOD_CHAINGUN",          //char           *name;
    MODTYPE_BALLISTIC,       //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_PRIFLE,              //meansOfDeath_t means_of_death;
    "MOD_PRIFLE",            //char           *name;
    MODTYPE_ENERGY,          //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_MDRIVER,             //meansOfDeath_t means_of_death;
    "MOD_MDRIVER",           //char           *name;
    MODTYPE_ENERGY,          //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LASGUN,              //meansOfDeath_t means_of_death;
    "MOD_LASGUN",            //char           *name;
    MODTYPE_ENERGY,          //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LCANNON,             //meansOfDeath_t means_of_death;
    "MOD_LCANNON",           //char           *name;
    MODTYPE_ENERGY,          //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LCANNON_SPLASH,      //meansOfDeath_t means_of_death;
    "MOD_LCANNON_SPLASH",    //char           *name;
    MODTYPE_ENERGY,          //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SPITFIRE_GAS_TRAIL,  //meansOfDeath_t means_of_death;
    "MOD_SPITFIRE_GAS_TRAIL",//char           *name;
    MODTYPE_BLAST,           //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_FLAMER,              //meansOfDeath_t means_of_death;
    "MOD_FLAMER",            //char           *name;
    MODTYPE_BURN,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_FLAMER_SPLASH,       //meansOfDeath_t means_of_death;
    "MOD_FLAMER_SPLASH",     //char           *name;
    MODTYPE_BURN,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_GRENADE,             //meansOfDeath_t means_of_death;
    "MOD_GRENADE",           //char           *name;
    MODTYPE_BLAST,           //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_FRAGNADE,            //meansOfDeath_t means_of_death;
    "MOD_FRAGNADE",          //char           *name;
    MODTYPE_SHRAPNEL,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LASERMINE,           //meansOfDeath_t means_of_death;
    "MOD_LASERMINE",         //char           *name;
    MODTYPE_BLAST,           //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_GRENADE_LAUNCHER,    //meansOfDeath_t means_of_death;
    "MOD_GRENADE_LAUNCHER",  //char           *name;
    MODTYPE_BLAST,           //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LIGHTNING,           //meansOfDeath_t means_of_death;
    "MOD_LIGHTNING",         //char           *name;
    MODTYPE_ZAP,             //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qfalse,                  //qboolean       self_radius_damage;
  },
  {
    MOD_LIGHTNING_EMP,       //meansOfDeath_t means_of_death;
    "MOD_LIGHTNING_EMP",     //char           *name;
    MODTYPE_ZAP,             //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qfalse,                  //qboolean       self_radius_damage;
  },
  {
    MOD_WATER,               //meansOfDeath_t means_of_death;
    "MOD_WATER",             //char           *name;
    MODTYPE_DROWN,           //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SLIME,               //meansOfDeath_t means_of_death;
    "MOD_SLIME",             //char           *name;
    MODTYPE_CHEMICAL,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qfalse,                  //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LAVA,                //meansOfDeath_t means_of_death;
    "MOD_LAVA",              //char           *name;
    MODTYPE_LAVA,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qfalse,                  //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_CRUSH,               //meansOfDeath_t means_of_death;
    "MOD_CRUSH",             //char           *name;
    MODTYPE_MOMENTUM,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qfalse,                  //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_DROP,                //meansOfDeath_t means_of_death;
    "MOD_DROP",              //char           *name;
    MODTYPE_FELL,            //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_TELEFRAG,            //meansOfDeath_t means_of_death;
    "MOD_TELEFRAG",          //char           *name;
    MODTYPE_GENERIC,         //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_FALLING,             //meansOfDeath_t means_of_death;
    "MOD_FALLING",           //char           *name;
    MODTYPE_FELL,            //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SUICIDE,             //meansOfDeath_t means_of_death;
    "MOD_SUICIDE",           //char           *name;
    MODTYPE_GENERIC,         //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qfalse,                  //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_TARGET_LASER,        //meansOfDeath_t means_of_death;
    "MOD_TARGET_LASER",      //char           *name;
    MODTYPE_ENERGY,          //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_TRIGGER_HURT,        //meansOfDeath_t means_of_death;
    "MOD_TRIGGER_HURT",      //char           *name;
    MODTYPE_GENERIC,         //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qfalse,                  //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SUFFOCATION,         //meansOfDeath_t means_of_death;
    "MOD_SUFFOCATION",       //char           *name;
    MODTYPE_SUFFOCATION,     //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SELFDESTRUCT,        //meansOfDeath_t means_of_death;
    "MOD_SELFDESTRUCT",      //char           *name;
    MODTYPE_BLAST,           //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_ABUILDER_CLAW,       //meansOfDeath_t means_of_death;
    "MOD_ABUILDER_CLAW",     //char           *name;
    MODTYPE_CLAW,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL0_BITE,         //meansOfDeath_t means_of_death;
    "MOD_LEVEL0_BITE",       //char           *name;
    MODTYPE_BITE,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL1_CLAW,         //meansOfDeath_t means_of_death;
    "MOD_LEVEL1_CLAW",       //char           *name;
    MODTYPE_CLAW,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL1_PCLOUD,       //meansOfDeath_t means_of_death;
    "MOD_LEVEL1_PCLOUD",     //char           *name;
    MODTYPE_CHEMICAL,        //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qfalse,                  //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL3_CLAW,         //meansOfDeath_t means_of_death;
    "MOD_LEVEL3_CLAW",       //char           *name;
    MODTYPE_CLAW,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL3_POUNCE,       //meansOfDeath_t means_of_death;
    "MOD_LEVEL3_POUNCE",     //char           *name;
    MODTYPE_MOMENTUM,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qfalse,                  //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL3_BOUNCEBALL,   //meansOfDeath_t means_of_death;
    "MOD_LEVEL3_BOUNCEBALL", //char           *name;
    MODTYPE_BALLISTIC,       //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL2_CLAW,         //meansOfDeath_t means_of_death;
    "MOD_LEVEL2_CLAW",       //char           *name;
    MODTYPE_CLAW,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL2_ZAP,          //meansOfDeath_t means_of_death;
    "MOD_LEVEL2_ZAP",        //char           *name;
    MODTYPE_ZAP,             //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qfalse,                  //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL4_CLAW,         //meansOfDeath_t means_of_death;
    "MOD_LEVEL4_CLAW",       //char           *name;
    MODTYPE_CLAW,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL4_TRAMPLE,      //meansOfDeath_t means_of_death;
    "MOD_LEVEL4_TRAMPLE",    //char           *name;
    MODTYPE_MOMENTUM,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_LEVEL4_CRUSH,        //meansOfDeath_t means_of_death;
    "MOD_LEVEL4_CRUSH",      //char           *name;
    MODTYPE_MOMENTUM,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SPITFIRE_POUNCE,     //meansOfDeath_t means_of_death;
    "MOD_SPITFIRE_POUNCE",   //char           *name;
    MODTYPE_MOMENTUM,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qfalse,                  //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SPITFIRE_ZAP,        //meansOfDeath_t means_of_death;
    "MOD_SPITFIRE_ZAP",      //char           *name;
    MODTYPE_ZAP,             //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qfalse,                  //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SPITFIRE_STING,      //meansOfDeath_t means_of_death;
    "MOD_SPITFIRE_STING",    //char           *name;
    MODTYPE_STING,           //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SLOWBLOB,            //meansOfDeath_t means_of_death;
    "MOD_SLOWBLOB",          //char           *name;
    MODTYPE_CHEMICAL,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_POISON,              //meansOfDeath_t means_of_death;
    "MOD_POISON",            //char           *name;
    MODTYPE_CHEMICAL,        //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qfalse,                  //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SWARM,               //meansOfDeath_t means_of_death;
    "MOD_SWARM",             //char           *name;
    MODTYPE_STING,           //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_HSPAWN,              //meansOfDeath_t means_of_death;
    "MOD_HSPAWN",            //char           *name;
    MODTYPE_BLAST,           //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qfalse,                  //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_TESLAGEN,            //meansOfDeath_t means_of_death;
    "MOD_TESLAGEN",          //char           *name;
    MODTYPE_ZAP,             //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_MGTURRET,            //meansOfDeath_t means_of_death;
    "MOD_MGTURRET",          //char           *name;
    MODTYPE_BALLISTIC,       //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_FLAME_TURRET,        //meansOfDeath_t means_of_death;
    "MOD_FLAME_TURRET",      //char           *name;
    MODTYPE_BURN,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_REACTOR,             //meansOfDeath_t means_of_death;
    "MOD_REACTOR",           //char           *name;
    MODTYPE_RADIATION,       //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_MEDISTAT,            //meansOfDeath_t means_of_death;
    "MOD_MEDISTAT",          //char           *name;
    MODTYPE_ENERGY,          //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_ASPAWN,              //meansOfDeath_t means_of_death;
    "MOD_ASPAWN",            //char           *name;
    MODTYPE_BLAST,           //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qfalse,                  //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_ATUBE,               //meansOfDeath_t means_of_death;
    "MOD_ATUBE",             //char           *name;
    MODTYPE_CHEMICAL,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_TRAP,                //meansOfDeath_t means_of_death;
    "MOD_TRAP",              //char           *name;
    MODTYPE_CHEMICAL,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SLIMER,              //meansOfDeath_t means_of_death;
    "MOD_SLIMER",            //char           *name;
    MODTYPE_CHEMICAL,        //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_OVERMIND,            //meansOfDeath_t means_of_death;
    "MOD_OVERMIND",          //char           *name;
    MODTYPE_MIND,            //mod_type_t     mod_type;
    qtrue,                   //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qtrue,                   //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_DECONSTRUCT,         //meansOfDeath_t means_of_death;
    "MOD_DECONSTRUCT",       //char           *name;
    MODTYPE_BUILD,           //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qfalse,                  //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_REPLACE,             //meansOfDeath_t means_of_death;
    "MOD_REPLACE",           //char           *name;
    MODTYPE_BUILD,           //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qfalse,                  //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_NOCREEP,             //meansOfDeath_t means_of_death;
    "MOD_NOCREEP",           //char           *name;
    MODTYPE_MIND,            //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qtrue,                   //qboolean       spawn_protected;
    qtrue,                   //qboolean       last_spawn_protection;
    qfalse,                  //qboolean       friendly_fire_protection;
    qtrue,                   //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  },
  {
    MOD_SLAP,                //meansOfDeath_t means_of_death;
    "MOD_SLAP",              //char           *name;
    MODTYPE_GENERIC,         //mod_type_t     mod_type;
    qfalse,                  //qboolean       hit_detected;
    qfalse,                  //qboolean       spawn_protected;
    qfalse,                  //qboolean       last_spawn_protection;
    qfalse,                  //qboolean       friendly_fire_protection;
    qfalse,                  //qboolean       can_poison;
    qtrue,                   //qboolean       self_radius_damage;
  }
};

size_t bg_numMODs = ARRAY_LEN( bg_modList );

static const modAttributes_t nullMOD = { 0 };

static const rankAttributes_t bg_rankList[ ] =
{
  {
    RANK_NONE,       //rank_t number;
    "",              //char   *name;
    ""               //char   *humanName;
  },
  {
    RANK_CAPTAIN,    //rank_t number;
    "Cpt.",          //char   *name;
    "Captain"        //char   *humanName;
  }
};

size_t bg_numRanks = ARRAY_LEN( bg_rankList );

static const rankAttributes_t nullRank = { 0 };

//
// XXX This MUST match the order of "buildable_t"!
//
static const buildableAttributes_t bg_buildableList[ ] =
{
  {
    BA_A_SPAWN,            //int       number;
    qtrue,                 //qboolean  enabled;
    "eggpod",              //char      *name;
    "Egg",                 //char      *humanName;
    "team_alien_spawn",    //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    ASPAWN_BP,             //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    ASPAWN_HEALTH,         //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    ASPAWN_REGEN,          //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    ASPAWN_SPLASHDAMAGE,   //int       splashDamage;
    ASPAWN_SPLASHRADIUS,   //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_ABUILD,           //weapon_t  buildWeapon[0]
      WP_NONE,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    ASPAWN_BT,             //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.0f,                  //float     minNormal;
    qtrue,                 //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    ASPAWN_CREEPSIZE,      //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    ASPAWN_VALUE,          //int       value;
    qfalse,                //qboolean  stackable;
    0,                     //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    CREEP_BASESIZE,        //float             rangeMarkerRange;
    SHC_LIGHT_GREEN,       //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SPAWN|
     ROLE_SUPPORT|
     ROLE_POWER_SOURCE|
     ROLE_PERVASIVE)       //int               role;
  },
  {
    BA_A_OVERMIND,         //int       number;
    qtrue,                 //qboolean  enabled;
    "overmind",            //char      *name;
    "Overmind",            //char      *humanName;
    "team_alien_overmind", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    OVERMIND_BP,           //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    OVERMIND_HEALTH,       //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    OVERMIND_REGEN,        //int       regenRate;
    0.15f,                 //float     ballisticDmgMod;
    OVERMIND_SPLASHDAMAGE, //int       splashDamage;
    OVERMIND_SPLASHRADIUS, //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_ABUILD,           //weapon_t  buildWeapon[0]
      WP_ABUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    OVERMIND_ATTACK_REPEAT, //int      nextthink;
    OVERMIND_BT,           //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    OVERMIND_CREEPSIZE,    //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qtrue,                 //qboolean  uniqueTest;
    OVERMIND_VALUE,        //int       value;
    qtrue,                 //qboolean  stackable;
    0,                     //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    CREEP_BASESIZE,        //float             rangeMarkerRange;
    SHC_DARK_GREEN,        //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_CORE|
     ROLE_SUPPORT|
     ROLE_POWER_SOURCE|
     ROLE_PERVASIVE)       //int               role;
  },
  {
    BA_A_BARRICADE,        //int       number;
    qtrue,                 //qboolean  enabled;
    "barricade",           //char      *name;
    "Barricade",           //char      *humanName;
    "team_alien_barricade", //char     *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    BARRICADE_BP,          //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    BARRICADE_HEALTH,      //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    BARRICADE_REGEN,       //int       regenRate;
    0.15f,                 //float     ballisticDmgMod;
    BARRICADE_SPLASHDAMAGE, //int      splashDamage;
    BARRICADE_SPLASHRADIUS, //int      splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_ABUILD,           //weapon_t  buildWeapon[0]
      WP_ABUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    BARRICADE_BT,          //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.707f,                //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qtrue,                 //qboolean  creepTest;
    BARRICADE_CREEPSIZE,   //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    BARRICADE_VALUE,       //int       value;
    qfalse,                //qboolean  stackable;
    BARRICADE_BAT_PWR,     //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    0.0f,                  //float             rangeMarkerRange;
    SHC_GREY,              //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SUPPORT)         //int               role;
  },
  {
    BA_A_ACIDTUBE,         //int       number;
    qtrue,                 //qboolean  enabled;
    "acid_tube",           //char      *name;
    "Acid Tube",           //char      *humanName;
    "team_alien_acid_tube", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    ACIDTUBE_BP,           //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    ACIDTUBE_HEALTH,       //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    ACIDTUBE_REGEN,        //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    ACIDTUBE_SPLASHDAMAGE, //int       splashDamage;
    ACIDTUBE_SPLASHRADIUS, //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_ABUILD,           //weapon_t  buildWeapon[0]
      WP_ABUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    200,                   //int       nextthink;
    ACIDTUBE_BT,           //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.0f,                  //float     minNormal;
    qtrue,                 //qboolean  invertNormal;
    qtrue,                 //qboolean  creepTest;
    ACIDTUBE_CREEPSIZE,    //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    ACIDTUBE_VALUE,        //int       value;
    qfalse,                //qboolean  stackable;
    ACIDTUBE_BAT_PWR,      //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    ACIDTUBE_RANGE,        //float             rangeMarkerRange;
    SHC_RED,               //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_OFFENSE)         //int               role;
  },
  {
    BA_A_TRAPPER,          //int       number;
    qtrue,                 //qboolean  enabled;
    "trapper",             //char      *name;
    "Trapper",             //char      *humanName;
    "team_alien_trapper",  //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    TRAPPER_BP,            //int       buildPoints;
    ( 1 << S2 )|( 1 << S3 ), //int     stages; //NEEDS ADV BUILDER SO S2 AND UP
    TRAPPER_HEALTH,        //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    TRAPPER_REGEN,         //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    TRAPPER_SPLASHDAMAGE,  //int       splashDamage;
    TRAPPER_SPLASHRADIUS,  //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_ABUILD,           //weapon_t  buildWeapon[0]
      WP_ABUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    TRAPPER_BT,            //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    TRAPPER_RANGE,         //int       turretRange;
    TRAPPER_REPEAT,        //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_LOCKBLOB_LAUNCHER,  //weapon_t  turretProjType;
    0.0f,                  //float     minNormal;
    qtrue,                 //qboolean  invertNormal;
    qtrue,                 //qboolean  creepTest;
    TRAPPER_CREEPSIZE,     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    TRAPPER_VALUE,         //int       value;
    qfalse,                //qboolean  stackable;
    TRAPPER_BAT_PWR,       //int       batteryPower;
    RMT_SPHERICAL_CONE_64, //rangeMarkerType_t rangeMarkerType;
    TRAPPER_RANGE,         //float             rangeMarkerRange;
    SHC_PINK,              //shaderColorEnum_t rangeMarkerColor;
    qtrue,                 //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_OFFENSE)         //int               role;
  },
  {
    BA_A_SIMER,            //int       buildNum;
    qtrue,                 //qboolean  enabled;
    "slime_zunge",         //char      *name;
    "Slimer",         //char      *humanName;
    "team_alien_slimer", //char   *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    SLIMER_BP,        //int       buildPoints;
    ( 1 << S3 ),           //int       stages;
    SLIMER_HEALTH,    //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    SLIMER_REGEN,     //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    SLIMER_SPLASHDAMAGE, //int    splashDamage;
    SLIMER_SPLASHRADIUS, //int    splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_ABUILD,           //weapon_t  buildWeapon[0]
      WP_ABUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    200,                   //int       nextthink;
    SLIMER_BT,        //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.0f,                  //float     minNormal;
    qtrue,                 //qboolean  invertNormal;
    qtrue,                 //qboolean  creepTest;
    SLIMER_CREEPSIZE, //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
	  SLIMER_VALUE,     //int       value;
    qfalse,                //qboolean  stackable;
    SLIMER_BAT_PWR,    //int       batteryPower;
    RMT_SPHERICAL_CONE_64, //rangeMarkerType_t rangeMarkerType;
    SLIMER_RANGE,     //float             rangeMarkerRange;
    SHC_PINK,              //shaderColorEnum_t rangeMarkerColor;
    qtrue,                 //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_OFFENSE)         //int               role;
  },
  {
    BA_A_BOOSTER,          //int       number;
    qtrue,                 //qboolean  enabled;
    "booster",             //char      *name;
    "Booster",             //char      *humanName;
    "team_alien_booster",  //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    BOOSTER_BP,            //int       buildPoints;
    ( 1 << S2 )|( 1 << S3 ), //int     stages;
    BOOSTER_HEALTH,        //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    BOOSTER_REGEN,         //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    BOOSTER_SPLASHDAMAGE,  //int       splashDamage;
    BOOSTER_SPLASHRADIUS,  //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_ABUILD,           //weapon_t  buildWeapon[0]
      WP_ABUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    BOOSTER_BT,            //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.707f,                //float     minNormal;
    qtrue,                //qboolean  invertNormal;
    qfalse,                 //qboolean  creepTest;
    BOOSTER_CREEPSIZE,     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    BOOSTER_VALUE,         //int       value;
    qfalse,                //qboolean  stackable;
    BOOSTER_BAT_PWR,       //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    CREEP_BASESIZE,        //float             rangeMarkerRange;
    SHC_LIGHT_GREEN,       //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SUPPORT|
     ROLE_PERVASIVE|
     ROLE_POWER_SOURCE)       //int               role;
  },
  {
    BA_A_HIVE,             //int       number;
    qtrue,                 //qboolean  enabled;
    "hive",                //char      *name;
    "Hive",                //char      *humanName;
    "team_alien_hive",     //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    HIVE_BP,               //int       buildPoints;
    ( 1 << S2 )|( 1 << S3 ),           //int       stages;
    HIVE_HEALTH,           //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    HIVE_REGEN,            //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    HIVE_SPLASHDAMAGE,     //int       splashDamage;
    HIVE_SPLASHRADIUS,     //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_ABUILD,           //weapon_t  buildWeapon[0]
      WP_ABUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    500,                   //int       nextthink;
    HIVE_BT,               //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_HIVE,               //weapon_t  turretProjType;
    0.0f,                  //float     minNormal;
    qtrue,                 //qboolean  invertNormal;
    qtrue,                 //qboolean  creepTest;
    HIVE_CREEPSIZE,        //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    HIVE_VALUE,            //int       value;
    qfalse,                //qboolean  stackable;
    HIVE_BAT_PWR,          //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    HIVE_SENSE_RANGE,      //float             rangeMarkerRange;
    SHC_YELLOW,            //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qtrue,                 //qboolean          rangeMarkerOriginAtTop;
    (ROLE_OFFENSE)         //int               role;
  },
  {
    BA_A_HOVEL,            //int       buildNum;
    qtrue,                 //qboolean  enabled;
    "hovel",               //char      *buildName;
    "Hovel",               //char      *humanName;
    "team_alien_hovel",    //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    HOVEL_BP,              //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ),           //int  stages
    HOVEL_HEALTH,          //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    HOVEL_REGEN,           //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    HOVEL_SPLASHDAMAGE,    //int       splashDamage;
    HOVEL_SPLASHRADIUS,    //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_ABUILD,           //weapon_t  buildWeapon[0]
      WP_ABUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    150,                   //int       nextthink;
    HOVEL_BT,              //int       buildTime;
    qtrue,                 //qboolean  activationEnt;
    (ACTF_TEAM|
     ACTF_ENT_ALIVE|
     ACTF_PL_ALIVE|
     ACTF_SPAWNED|
     ACTF_LINE_OF_SIGHT|
     ACTF_OCCUPY),         //int       activationFlags;
    (OCCUPYF_PM_TYPE|
     OCCUPYF_CONTENTS|
     OCCUPYF_CLIPMASK),    //int       occupationFlags;
    PM_FREEZE,             //pmtype_t  activationPm_type;
    CONTENTS_ASTRAL,       //int       activationContents;
    {MASK_DEADSOLID, CONTENTS_DOOR}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qtrue,                 //qboolean  creepTest;
    HOVEL_CREEPSIZE,       //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qtrue,                 //qboolean  uniqueTest;
    HOVEL_VALUE,           //int       value;
    qfalse,                //qboolean  stackable;
    HOVEL_BAT_PWR,         //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    0.0f,                  //float             rangeMarkerRange;
    SHC_GREY,              //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SUPPORT)         //int               role;
  },
  {
    BA_A_GRAPNEL,          //int       buildNum;
    qtrue,                 //qboolean  enabled;
    "grapnel",             //char      *buildName;
    "Grapnel",             //char      *humanName;
    "team_alien_grapnel",  //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    GRAPNEL_BP,            //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    GRAPNEL_HEALTH,        //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    GRAPNEL_REGEN,         //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    GRAPNEL_SPLASHDAMAGE,  //int       splashDamage;
    GRAPNEL_SPLASHRADIUS,  //int       splashRadius;
    MOD_ASPAWN,            //int       meansOfDeath;
    TEAM_ALIENS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_ABUILD,           //weapon_t  buildWeapon[0]
      WP_ABUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    150,                   //int       nextthink;
    GRAPNEL_BT,            //int       buildTime;
    qtrue,                 //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.0f,                  //float     minNormal;
    qtrue,                 //qboolean  invertNormal;
    qfalse,                 //qboolean  creepTest;
    GRAPNEL_CREEPSIZE,     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    GRAPNEL_VALUE,         //int       value;
    qtrue,                 //qboolean  stackable;
    GRAPNEL_BAT_PWR,       //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    CREEP_GRAPNEL_BASESIZE, //float             rangeMarkerRange;
    SHC_LIGHT_GREEN,       //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SUPPORT|
     ROLE_STRUCTUAL|
     ROLE_POWER_SOURCE)    //int               role;
  },
  {
    BA_H_SPAWN,            //int       number;
    qtrue,                 //qboolean  enabled;
    "telenode",            //char      *name;
    "Telenode",            //char      *humanName;
    "team_human_spawn",    //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    HSPAWN_BP,             //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    HSPAWN_HEALTH,         //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    0,                     //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    HSPAWN_SPLASHDAMAGE,   //int       splashDamage;
    HSPAWN_SPLASHRADIUS,   //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_HBUILD2,          //weapon_t  buildWeapon[0]
      WP_NONE,             //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    HSPAWN_BT,             //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    HSPAWN_VALUE,          //int       value;
    qfalse,                //qboolean  stackable;
    0,                     //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    0.0f,                  //float             rangeMarkerRange;
    SHC_GREY,              //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SPAWN|
     ROLE_SUPPORT)         //int               role;
  },
  {
    BA_H_TELEPORTER,       //int       number;
    qtrue,                 //qboolean  enabled;
    "teleporter",          //char      *name;
    "Teleporter",          //char      *humanName;
    "team_human_teleporter", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.0,                   //float     bounce;
    HTELEPORTER_BP,        //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    HTELEPORTER_HEALTH,    //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    0,                     //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    HTELEPORTER_SPLASHDAMAGE, //int       splashDamage;
    HTELEPORTER_SPLASHRADIUS, //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_HBUILD,           //weapon_t  buildWeapon[0]
      WP_HBUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    HTELEPORTER_BT,        //int       buildTime;
    qtrue,                 //qboolean  activationEnt;
    (ACTF_TEAM|
     ACTF_ENT_ALIVE|
     ACTF_PL_ALIVE|
     ACTF_SPAWNED|
     ACTF_POWERED|
     ACTF_GROUND|
     ACTF_OCCUPY),         //int       activationFlags;
    (OCCUPYF_ACTIVATE|
     OCCUPYF_UNTIL_INACTIVE|
     OCCUPYF_RESET_OTHER), //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    HTELEPORTER_VALUE,     //int       value;
    qfalse,                //qboolean  stackable;
    HTELEPORTER_BAT_PWR,    //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    0.0f,                  //float             rangeMarkerRange;
    SHC_GREY,              //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SUPPORT|
     ROLE_TRANSPORT)       //int               role;
  },
  {
    BA_H_MGTURRET,         //int       number;
    qtrue,                 //qboolean  enabled;
    "mgturret",            //char      *name;
    "Machinegun Turret",   //char      *humanName;
    "team_human_mgturret", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    MGTURRET_BP,           //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    MGTURRET_HEALTH,       //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    0,                     //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    MGTURRET_SPLASHDAMAGE, //int       splashDamage;
    MGTURRET_SPLASHRADIUS, //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_HBUILD,           //weapon_t  buildWeapon[0]
      WP_HBUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    MGTURRET_NEXTTHINK,    //int       nextthink;
    MGTURRET_BT,           //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    MGTURRET_RANGE,        //int       turretRange;
    MGTURRET_REPEAT,       //int       turretFireSpeed;
    MGTURRET_ANGULARSPEED, //float     turretAngularSpeed;
    MGTURRET_DCC_ANGULARSPEED, //float     turretDCCAngularSpeed;
    MGTURRET_GRAB_ANGULARSPEED, //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_MGTURRET,           //weapon_t  turretProjType;
    0.5f,                  //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    MGTURRET_VALUE,        //int       value;
    qfalse,                //qboolean  stackable;
    MGTURRET_BAT_PWR,       //int       batteryPower;
    RMT_SPHERICAL_CONE_240,//rangeMarkerType_t rangeMarkerType;
    MGTURRET_RANGE,        //float             rangeMarkerRange;
    SHC_ORANGE,            //shaderColorEnum_t rangeMarkerColor;
    qtrue,                 //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_OFFENSE)         //int               role;
  },
  {
    BA_H_FLAME_TURRET,     //int       number;
    qtrue,                 //qboolean  enabled;
    "flame_turret",        //char      *name;
    "Flame Turret",        //char      *humanName;
    "team_human_flame_turret", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    FLAME_TURRET_BP,       //int       buildPoints;
    ( 1 << S3 ),           //int  stages;
    FLAME_TURRET_HEALTH,   //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    0,                     //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    FLAME_TURRET_SPLASHDAMAGE, //int       splashDamage;
    FLAME_TURRET_SPLASHRADIUS, //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_HBUILD,           //weapon_t  buildWeapon[0]
      WP_HBUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    FLAME_TURRET_NEXTTHINK,//int       nextthink;
    FLAME_TURRET_BT,       //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    FLAME_TURRET_RANGE,    //int       turretRange;
    FLAME_TURRET_REPEAT,   //int       turretFireSpeed;
    FLAME_TURRET_ANGULARSPEED, //float     turretAngularSpeed;
    FLAME_TURRET_DCC_ANGULARSPEED, //float     turretDCCAngularSpeed;
    FLAME_TURRET_GRAB_ANGULARSPEED, //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_FLAME_TURRET,       //weapon_t  turretProjType;
    0.5f,                  //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    FLAME_TURRET_VALUE,    //int       value;
    qfalse,                //qboolean  stackable;
    FLAME_TURRET_BAT_PWR,  //int       batteryPower;
    RMT_SPHERICAL_CONE_240,//rangeMarkerType_t rangeMarkerType;
    FLAME_TURRET_RANGE,    //float             rangeMarkerRange;
    SHC_ORANGE,            //shaderColorEnum_t rangeMarkerColor;
    qtrue,                 //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_OFFENSE)         //int               role;
  },
  {
    BA_H_TESLAGEN,         //int       number;
    qtrue,                 //qboolean  enabled;
    "tesla",               //char      *name;
    "Tesla Generator",     //char      *humanName;
    "team_human_tesla",    //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    TESLAGEN_BP,           //int       buildPoints;
    ( 1 << S3 ),           //int       stages;
    TESLAGEN_HEALTH,       //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    0,                     //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    TESLAGEN_SPLASHDAMAGE, //int       splashDamage;
    TESLAGEN_SPLASHRADIUS, //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_HBUILD,           //weapon_t  buildWeapon[0]
      WP_HBUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    150,                   //int       nextthink;
    TESLAGEN_BT,           //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    TESLAGEN_RANGE,        //int       turretRange;
    TESLAGEN_REPEAT,       //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_TESLAGEN,           //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    TESLAGEN_VALUE,        //int       value;
    qfalse,                //qboolean  stackable;
    TESLAGEN_BAT_PWR,       //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    TESLAGEN_RANGE,        //float             rangeMarkerRange;
    SHC_VIOLET,            //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qtrue,                 //qboolean          rangeMarkerOriginAtTop;
    (ROLE_OFFENSE)         //int               role;
  },
  {
    BA_H_ARMOURY,          //int       number;
    qtrue,                 //qboolean  enabled;
    "arm",                 //char      *name;
    "Armoury",             //char      *humanName;
    "team_human_armoury",  //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    ARMOURY_BP,            //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    ARMOURY_HEALTH,        //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    0,                     //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    ARMOURY_SPLASHDAMAGE,  //int       splashDamage;
    ARMOURY_SPLASHRADIUS,  //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_HBUILD2,          //weapon_t  buildWeapon[0]
      WP_NONE,             //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    ARMOURY_BT,            //int       buildTime;
    qtrue,                 //qboolean  activationEnt;
    (ACTF_TEAM|
     ACTF_ENT_ALIVE|
     ACTF_PL_ALIVE|
     ACTF_SPAWNED|
     ACTF_POWERED),        //int activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.9f,                  //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    ARMOURY_VALUE,         //int       value;
    qtrue,                 //qboolean  stackable;
    ARMOURY_BAT_PWR,        //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    0.0f,                  //float             rangeMarkerRange;
    SHC_GREY,              //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SUPPORT)         //int               role;
  },
  {
    BA_H_DCC,              //int       number;
    qtrue,                 //qboolean  enabled;
    "dcc",                 //char      *name;
    "Defense Computer",    //char      *humanName;
    "team_human_dcc",      //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    DC_BP,                 //int       buildPoints;
    ( 1 << S2 )|( 1 << S3 ), //int     stages;
    DC_HEALTH,             //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    0,                     //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    DC_SPLASHDAMAGE,       //int       splashDamage;
    DC_SPLASHRADIUS,       //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_HBUILD,           //weapon_t  buildWeapon[0]
      WP_HBUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    DC_BT,                 //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.9f,                  //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qtrue,                 //qboolean  uniqueTest;
    DC_VALUE,              //int       value;
    qtrue,                 //qboolean  stackable;
    DC_BAT_PWR,            //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    DC_RANGE,              //float             rangeMarkerRange;
    SHC_GREEN_CYAN,        //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SUPPORT|
     ROLE_PERVASIVE)       //int               role;
  },
  {
    BA_H_MEDISTAT,         //int       number;
    qtrue,                 //qboolean  enabled;
    "medistat",            //char      *name;
    "Medistation",         //char      *humanName;
    "team_human_medistat", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    MEDISTAT_BP,           //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    MEDISTAT_HEALTH,       //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    0,                     //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    MEDISTAT_SPLASHDAMAGE, //int       splashDamage;
    MEDISTAT_SPLASHRADIUS, //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_HBUILD,           //weapon_t  buildWeapon[0]
      WP_HBUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    MEDISTAT_REPEAT,       //int       nextthink;
    MEDISTAT_BT,           //int       buildTime;
    qfalse,                //qboolean  activationEnt;
    0,                     //int       activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qtrue,                 //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    MEDISTAT_VALUE,        //int       value;
    qfalse,                //qboolean  stackable;
    MEDISTAT_BAT_PWR,       //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    0.0f,                  //float             rangeMarkerRange;
    SHC_GREY,              //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SUPPORT)         //int               role;
  },
  {
    BA_H_REACTOR,          //int       number;
    qtrue,                 //qboolean  enabled;
    "reactor",             //char      *name;
    "Reactor",             //char      *humanName;
    "team_human_reactor",  //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    REACTOR_BP,            //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    REACTOR_HEALTH,        //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    0,                     //int       regenRate;
    0.8f,                  //float     ballisticDmgMod;
    REACTOR_SPLASHDAMAGE,  //int       splashDamage;
    REACTOR_SPLASHRADIUS,  //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_HBUILD,           //weapon_t  buildWeapon[0]
      WP_HBUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    REACTOR_ATTACK_DCC_REPEAT, //int   nextthink;
    REACTOR_BT,            //int       buildTime;
    qtrue,                 //qboolean  activationEnt;
    (ACTF_TEAM|
     ACTF_ENT_ALIVE|
     ACTF_PL_ALIVE|
     ACTF_SPAWNED|
     ACTF_POWERED),        //int activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.95f,                 //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qtrue,                 //qboolean  uniqueTest;
    REACTOR_VALUE,         //int       value;
    qtrue,                 //qboolean  stackable;
    0,                     //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    REACTOR_BASESIZE,      //float             rangeMarkerRange;
    SHC_DARK_BLUE,         //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_CORE|
     ROLE_SUPPORT|
     ROLE_POWER_SOURCE|
     ROLE_PERVASIVE)       //int               role;
  },
  {
    BA_H_REPEATER,         //int       number;
    qtrue,                 //qboolean  enabled;
    "repeater",            //char      *name;
    "Repeater",            //char      *humanName;
    "team_human_repeater", //char      *entityName;
    TR_GRAVITY,            //trType_t  traj;
    0.1,                   //float     bounce;
    REPEATER_BP,           //int       buildPoints;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    REPEATER_HEALTH,       //int       health;
    0.80f,                 //float     maxHealthDecayRate;
    0,                     //int       minHealth;
    0,                     //int       regenRate;
    1.0f,                  //float     ballisticDmgMod;
    REPEATER_SPLASHDAMAGE, //int       splashDamage;
    REPEATER_SPLASHRADIUS, //int       splashRadius;
    MOD_HSPAWN,            //int       meansOfDeath;
    TEAM_HUMANS,           //int       team;
    {                      //weapon_t  buildWeapon[]
      WP_HBUILD,           //weapon_t  buildWeapon[0]
      WP_HBUILD2,          //weapon_t  buildWeapon[1]
      WP_NONE,             //weapon_t  buildWeapon[2]
      WP_NONE              //weapon_t  buildWeapon[3]
    },
    BANIM_IDLE1,           //int       idleAnim;
    100,                   //int       nextthink;
    REPEATER_BT,           //int       buildTime;
    qtrue,                 //qboolean  activationEnt;
    (ACTF_TEAM|
     ACTF_ENT_ALIVE|
     ACTF_PL_ALIVE|
     ACTF_SPAWNED|
     ACTF_POWERED),        //int activationFlags;
    0,                     //int       occupationFlags;
    PM_NORMAL,             //pmtype_t  activationPm_type;
    CONTENTS_BODY,         //int       activationContents;
    {MASK_PLAYERSOLID, 0}, //int       activationClipMask;
    0,                     //int       turretRange;
    0,                     //int       turretFireSpeed;
    0,                     //float     turretAngularSpeed;
    0,                     //float     turretDCCAngularSpeed;
    0,                     //float     turretGrabAngularSpeed;
    qfalse,                //qboolean  turretTrackOnlyOrigin;
    WP_NONE,               //weapon_t  turretProjType;
    0.5f,                  //float     minNormal;
    qfalse,                //qboolean  invertNormal;
    qfalse,                //qboolean  creepTest;
    0,                     //int       creepSize;
    qfalse,                //qboolean  dccTest;
    qfalse,                //qboolean  transparentTest;
    qfalse,                //qboolean  uniqueTest;
    REPEATER_VALUE,        //int       value;
    qfalse,                //qboolean  stackable;
    0,                     //int       batteryPower;
    RMT_SPHERE,            //rangeMarkerType_t rangeMarkerType;
    REPEATER_BASESIZE,     //float             rangeMarkerRange;
    SHC_LIGHT_BLUE,        //shaderColorEnum_t rangeMarkerColor;
    qfalse,                //qboolean          rangeMarkerUseNormal;
    qfalse,                //qboolean          rangeMarkerOriginAtTop;
    (ROLE_SUPPORT|
     ROLE_POWER_SOURCE|
     ROLE_PERVASIVE)       //int               role;
  }
};

size_t bg_numBuildables = ARRAY_LEN( bg_buildableList );

static const buildableAttributes_t nullBuildable = { 0 };

/*
==============
BG_BuildableByName
==============
*/
const buildableAttributes_t *BG_BuildableByName( const char *name )
{
  int i;

  for( i = 0; i < bg_numBuildables; i++ )
  {
    if( !Q_stricmp( bg_buildableList[ i ].name, name ) )
      return &bg_buildableList[ i ];
  }

  return &nullBuildable;
}

/*
==============
BG_BuildableByEntityName
==============
*/
const buildableAttributes_t *BG_BuildableByEntityName( const char *name )
{
  int i;

  for( i = 0; i < bg_numBuildables; i++ )
  {
    if( !Q_stricmp( bg_buildableList[ i ].entityName, name ) )
      return &bg_buildableList[ i ];
  }

  return &nullBuildable;
}

/*
==============
BG_Buildable
==============
*/
const buildableAttributes_t *BG_Buildable( buildable_t buildable )
{
  return ( buildable > BA_NONE && buildable < BA_NUM_BUILDABLES ) ?
    &bg_buildableList[ buildable - 1 ] : &nullBuildable;
}

/*
==============
BG_Weapon_Can_Build_Buildable
==============
*/
qboolean BG_Weapon_Can_Build_Buildable(weapon_t weapon, buildable_t buildable) {
  int i;

  if(weapon == WP_NONE || weapon >= WP_NUM_WEAPONS) {
    return qfalse;
  }

  if(buildable == BA_NONE || buildable >= BA_NUM_BUILDABLES) {
    return qfalse;
  }

  for(i = 0; i < 4; i++) {
    if(BG_Buildable(buildable)->buildWeapon[i] == weapon) {
      return qtrue;
    }
  }

  return qfalse;
}

/*
==============
BG_BuildableAllowedInStage
==============
*/
qboolean BG_BuildableAllowedInStage( buildable_t buildable,
                                     stage_t stage,
                                     int gameIsInWarmup  )
{
  int stages = BG_Buildable( buildable )->stages;

  if( gameIsInWarmup )
    return qtrue;

  if( stages & ( 1 << stage ) )
    return qtrue;
  else
    return qfalse;
}

/*
==============
BG_BuildableBoundingBox
==============
*/
void BG_BuildableBoundingBox( buildable_t buildable,
                              vec3_t mins, vec3_t maxs )
{
  buildableConfig_t *buildableConfig = BG_BuildableConfig( buildable );

  if( mins != NULL )
    VectorCopy( buildableConfig->mins, mins );

  if( maxs != NULL )
    VectorCopy( buildableConfig->maxs, maxs );
}


////////////////////////////////////////////////////////////////////////////////

static const classAttributes_t bg_classList[ ] =
{
  {
    PCL_NONE,                                       //int     number;
    qtrue,                                          //qboolean enabled;
    "spectator",                                    //char    *name;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ),            //int     stages;
    100000,                                         //int     health;
    0.0f,                                           //float   maxHealthDecayRate;
    0,                                              //int     minHealth;
    0.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    0,                                              //int     abilities;
    WP_NONE,                                        //weapon_t startWeapon;
    0.0f,                                           //float   buildDist;
    0.0f,                                           //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    90,                                             //int     fov;
    0.000f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    0.0f,                                           //float   landBob;
    0,                                              //int     steptime;
    600,                                            //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    270.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    0,                                              //int     cost;
    qtrue,                                          //qboolean warmupFree;
    0,                                              //int     value;
    TEAM_NONE                                       //team_t  team;
  },
  {
    PCL_ALIEN_BUILDER0,                             //int     number;
    qtrue,                                          //qboolean enabled;
    "builder",                                      //char    *name;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ),            //int     stages;
    ABUILDER_HEALTH,                                //int     health;
    0.0f,                                           //float   maxHealthDecayRate;
    ABUILDER_MIN_HEALTH,                            //int     minHealth;
    0.2f,                                           //float   fallDamage;
    ABUILDER_REGEN,                                 //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_FOVWARPS|
    SCA_WALLCLIMBER|SCA_ALIENSENSE|
    SCA_REGEN|SCA_CANHOVEL,                         //int    abilities;
    WP_ABUILD,                                      //weapon_t startWeapon;
    240.0f,                                         //float   buildDist;
    120.0f,                                         //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    110,                                            //int     fov;
    0.001f,                                         //float   bob;
    2.0f,                                           //float   bobCycle;
    4.5f,                                           //float   landBob;
    100,                                            //int     steptime;
    ABUILDER_SPEED,                                 //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    270.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    ABUILDER_COST,                                  //int     cost;
    qtrue,                                          //qboolean warmupFree;
    ABUILDER_VALUE,                                 //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_ALIEN_BUILDER0_UPG,                         //int     number;
    qtrue,                                          //qboolean enabled;
    "builderupg",                                   //char    *name;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ),            //int     stages;
    ABUILDER_UPG_HEALTH,                            //int     health;
    0.0f,                                           //float   maxHealthDecayRate;
    ABUILDER_UPG_MIN_HEALTH,                        //int     minHealth;
    0.0f,                                           //float   fallDamage;
    ABUILDER_UPG_REGEN,                             //float   regenRate;
    SCA_FOVWARPS|
    SCA_WALLCLIMBER|SCA_ALIENSENSE|
    SCA_REGEN|SCA_CANHOVEL,                         //int  abilities;
    WP_ABUILD2,                                     //weapon_t startWeapon;
    240.0f,                                         //float   buildDist;
    120.0f,                                         //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    110,                                            //int     fov;
    0.001f,                                         //float   bob;
    2.0f,                                           //float   bobCycle;
    4.5f,                                           //float   landBob;
    100,                                            //int     steptime;
    ABUILDER_UPG_SPEED,                             //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    337.5f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    ABUILDER_UPG_COST,                              //int     cost;
    qtrue,                                          //qboolean warmupFree;
    ABUILDER_UPG_VALUE,                             //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_ALIEN_LEVEL0,                               //int     number;
    qtrue,                                          //qboolean enabled;
    "level0",                                       //char    *name;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ),            //int     stages;
    LEVEL0_HEALTH,                                  //int     health;
    0.0f,                                           //float   maxHealthDecayRate;
    LEVEL0_MIN_HEALTH,                              //int     minHealth;
    0.0f,                                           //float   fallDamage;
    LEVEL0_REGEN,                                   //float   regenRate;
    SCA_WALLCLIMBER|SCA_FOVWARPS|SCA_ALIENSENSE|SCA_REGEN|SCA_CHARGE_STAMINA,    //int     abilities;
    WP_ALEVEL0,                                     //weapon_t startWeapon;
    0.0f,                                           //float   buildDist;
    0.0f,                                           //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    140,                                            //int     fov;
    0.0f,                                           //float   bob;
    2.5f,                                           //float   bobCycle;
    0.0f,                                           //float   landBob;
    25,                                             //int     steptime;
    LEVEL0_SPEED,                                   //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    400.0f,                                         //float   stopSpeed;
    250.0f,                                         //float   jumpMagnitude;
    2.0f,                                           //float   knockbackScale;
    LEVEL0_CHARGE_STAMINA_MAX,                      //int     chargeStaminaMax;
    LEVEL0_CHARGE_STAMINA_MIN,                      //int     chargeStaminaMin;
    LEVEL0_CHARGE_STAMINA_USE_RATE,                 //float   chargeStaminaUseRate;
    LEVEL0_CHARGE_STAMINA_RESTORE_RATE,             //float   chargeStaminaRestoreRate;
    LEVEL0_COST,                                    //int     cost;
    qtrue,                                          //qboolean warmupFree;
    LEVEL0_VALUE,                                   //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_ALIEN_LEVEL1,                               //int     number;
    qtrue,                                          //qboolean enabled;
    "level1",                                       //char    *name;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ),            //int     stages;
    LEVEL1_HEALTH,                                  //int     health;
    0.0f,                                           //float   maxHealthDecayRate;
    LEVEL1_MIN_HEALTH,                              //int     minHealth;
    0.0f,                                           //float   fallDamage;
    LEVEL1_REGEN,                                   //float   regenRate;
    SCA_FOVWARPS|SCA_WALLCLIMBER|SCA_ALIENSENSE|SCA_REGEN,    //int     abilities;
    WP_ALEVEL1,                                     //weapon_t startWeapon;
    0.0f,                                           //float   buildDist;
    0.0f,                                           //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    120,                                            //int     fov;
    0.001f,                                         //float   bob;
    1.8f,                                           //float   bobCycle;
    0.0f,                                           //float   landBob;
    60,                                             //int     steptime;
    LEVEL1_SPEED,                                   //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    300.0f,                                         //float   stopSpeed;
    310.0f,                                         //float   jumpMagnitude;
    1.2f,                                           //float   knockbackScale;
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    LEVEL1_COST,                                    //int     cost;
    qtrue,                                          //qboolean warmupFree;
    LEVEL1_VALUE,                                   //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_ALIEN_LEVEL1_UPG,                           //int     number;
    qtrue,                                          //qboolean enabled;
    "level1upg",                                    //char    *name;
    ( 1 << S2 )|( 1 << S3 ),                        //int     stages;
    LEVEL1_UPG_HEALTH,                              //int     health;
    0.0f,                                           //float   maxHealthDecayRate;
    LEVEL1_UPG_MIN_HEALTH,                          //int     minHealth;
    0.0f,                                           //float   fallDamage;
    LEVEL1_UPG_REGEN,                               //float   regenRate;
    SCA_FOVWARPS|SCA_WALLCLIMBER|SCA_ALIENSENSE|SCA_REGEN,    //int     abilities;
    WP_ALEVEL1_UPG,                                 //weapon_t startWeapon;
    0.0f,                                           //float   buildDist;
    0.0f,                                           //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    120,                                            //int     fov;
    0.001f,                                         //float   bob;
    1.8f,                                           //float   bobCycle;
    0.0f,                                           //float   landBob;
    60,                                             //int     steptime;
    LEVEL1_UPG_SPEED,                               //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    300.0f,                                         //float   stopSpeed;
    310.0f,                                         //float   jumpMagnitude;
    1.1f,                                           //float   knockbackScale;
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    LEVEL1_UPG_COST,                                //int     cost;
    qtrue,                                          //qboolean warmupFree;
    LEVEL1_UPG_VALUE,                               //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_ALIEN_LEVEL2,                               //int     number;
    qtrue,                                          //qboolean enabled;
    "level2",                                       //char    *name;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ),            //int     stages;
    LEVEL2_HEALTH,                                  //int     health;
    0.10f,                                          //float   maxHealthDecayRate;
    LEVEL2_MIN_HEALTH,                              //int     minHealth;
    0.0f,                                           //float   fallDamage;
    LEVEL2_REGEN,                                   //float   regenRate;
    SCA_WALLJUMPER|SCA_FOVWARPS|SCA_ALIENSENSE|SCA_REGEN,     //int     abilities;
    WP_ALEVEL2,                                     //weapon_t startWeapon;
    0.0f,                                           //float   buildDist;
    0.0f,                                           //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    90,                                             //int     fov;
    0.001f,                                         //float   bob;
    1.5f,                                           //float   bobCycle;
    0.0f,                                           //float   landBob;
    80,                                             //int     steptime;
    LEVEL2_SPEED,                                   //float   speed;
    10.0f,                                          //float   acceleration;
    3.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    380.0f,                                         //float   jumpMagnitude;
    0.8f,                                           //float   knockbackScale;
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    LEVEL2_COST,                                    //int     cost;
    qtrue,                                          //qboolean warmupFree;
    LEVEL2_VALUE,                                   //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_ALIEN_LEVEL2_UPG,                           //int     number;
    qtrue,                                          //qboolean enabled;
    "level2upg",                                    //char    *name;
    ( 1 << S2 )|( 1 << S3 ),                        //int     stages;
    LEVEL2_UPG_HEALTH,                              //int     health;
    0.15f,                                          //float   maxHealthDecayRate;
    LEVEL2_UPG_MIN_HEALTH,                          //int     minHealth;
    0.0f,                                           //float   fallDamage;
    LEVEL2_UPG_REGEN,                               //float   regenRate;
    SCA_WALLJUMPER|SCA_FOVWARPS|SCA_ALIENSENSE|SCA_REGEN,     //int     abilities;
    WP_ALEVEL2_UPG,                                 //weapon_t startWeapon;
    0.0f,                                           //float   buildDist;
    0.0f,                                           //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    90,                                             //int     fov;
    0.001f,                                         //float   bob;
    1.5f,                                           //float   bobCycle;
    0.0f,                                           //float   landBob;
    80,                                             //int     steptime;
    LEVEL2_UPG_SPEED,                               //float   speed;
    10.0f,                                          //float   acceleration;
    3.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    380.0f,                                         //float   jumpMagnitude;
    0.7f,                                           //float   knockbackScale
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    LEVEL2_UPG_COST,                                //int     cost;
    qtrue,                                          //qboolean warmupFree;
    LEVEL2_UPG_VALUE,                               //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_ALIEN_SPITFIRE,                             //int     number;
    qtrue,                                          //qboolean enabled;
    "spitfire",                                     //char    *name;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ),            //int     stages
    SPITFIRE_HEALTH,                                //int     health;
    0.15f,                                          //float   maxHealthDecayRate;
    SPITFIRE_MIN_HEALTH,                            //int     minHealth;
    0.0f,                                           //float   fallDamage;
    SPITFIRE_REGEN,                                 //float   regenRate;
    (SCA_FOVWARPS|SCA_ALIENSENSE),                  //int     abilities;
    WP_ASPITFIRE,                                   //weapon_t  startWeapon
    0.0f,                                           //float   buildDist;
    0.0f,                                           //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    90,                                             //int     fov;
    0.001f,                                         //float   bob;
    1.8f,                                           //float   bobCycle;
    0.0f,                                           //float   landBob;
    80,                                             //int     steptime;
    SPITFIRE_SPEED,                                 //float   speed;
    10.0f,                                          //float   acceleration;
    5.0f,                                           //float   airAcceleration;
    4.0f,                                           //float   friction;
    90.0f,                                          //float   stopSpeed;
    250.0f,                                         //float   jumpMagnitude;
    0.7f,                                           //float   knockbackScale;
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    SPITFIRE_COST,                                  //int     cost;
    qfalse,                                         //qboolean warmupFree;
    SPITFIRE_VALUE,                                 //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_ALIEN_LEVEL3,                               //int     number;
    qtrue,                                          //qboolean enabled;
    "level3",                                       //char    *name;
    ( 1 << S2 )|( 1 << S3 ),                        //int     stages;
    LEVEL3_HEALTH,                                  //int     health;
    0.25f,                                          //float   maxHealthDecayRate;
    LEVEL3_MIN_HEALTH,                              //int     minHealth;
    0.0f,                                           //float   fallDamage;
    LEVEL3_REGEN,                                   //float   regenRate;
    SCA_FOVWARPS|SCA_ALIENSENSE|SCA_REGEN|SCA_CHARGE_STAMINA, //int     abilities;
    WP_ALEVEL3,                                     //weapon_t startWeapon;
    0.0f,                                           //float   buildDist;
    0.0f,                                           //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    110,                                            //int     fov;
    0.0005f,                                        //float   bob;
    1.3f,                                           //float   bobCycle;
    0.0f,                                           //float   landBob;
    90,                                             //int     steptime;
    LEVEL3_SPEED,                                   //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    200.0f,                                         //float   stopSpeed;
    270.0f,                                         //float   jumpMagnitude;
    0.5f,                                           //float   knockbackScale;
    LEVEL3_CHARGE_STAMINA_MAX,                      //int     chargeStaminaMax;
    LEVEL3_CHARGE_STAMINA_MIN,                      //int     chargeStaminaMin;
    LEVEL3_CHARGE_STAMINA_USE_RATE,                 //float   chargeStaminaUseRate;
    LEVEL3_CHARGE_STAMINA_RESTORE_RATE,             //float   chargeStaminaRestoreRate;
    LEVEL3_COST,                                    //int     cost;
    qfalse,                                         //qboolean warmupFree;
    LEVEL3_VALUE,                                   //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_ALIEN_LEVEL3_UPG,                           //int     number;
    qtrue,                                          //qboolean enabled;
    "level3upg",                                    //char    *name;
    ( 1 << S3 ),                                    //int     stages;
    LEVEL3_UPG_HEALTH,                              //int     health;
    0.25f,                                          //float   maxHealthDecayRate;
    LEVEL3_MIN_HEALTH,                              //int     minHealth;
    0.0f,                                           //float   fallDamage;
    LEVEL3_UPG_REGEN,                               //float   regenRate;
    SCA_FOVWARPS|SCA_ALIENSENSE|SCA_REGEN|SCA_CHARGE_STAMINA, //int     abilities;
    WP_ALEVEL3_UPG,                                 //weapon_t startWeapon;
    0.0f,                                           //float   buildDist;
    0.0f,                                           //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    110,                                            //int     fov;
    0.0005f,                                        //float   bob;
    1.3f,                                           //float   bobCycle;
    0.0f,                                           //float   landBob;
    90,                                             //int     steptime;
    LEVEL3_UPG_SPEED,                               //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    200.0f,                                         //float   stopSpeed;
    270.0f,                                         //float   jumpMagnitude;
    0.4f,                                           //float   knockbackScale;
    LEVEL3_CHARGE_STAMINA_MAX_UPG,                  //int     chargeStaminaMax;
    LEVEL3_CHARGE_STAMINA_MIN_UPG,                  //int     chargeStaminaMin;
    LEVEL3_CHARGE_STAMINA_USE_RATE,                 //float   chargeStaminaUseRate;
    LEVEL3_CHARGE_STAMINA_RESTORE_RATE,             //float   chargeStaminaRestoreRate;
    LEVEL3_UPG_COST,                                //int     cost;
    qfalse,                                         //qboolean warmupFree;
    LEVEL3_UPG_VALUE,                               //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_ALIEN_LEVEL4,                               //int     number;
    qtrue,                                          //qboolean enabled;
    "level4",                                       //char    *name;
    ( 1 << S3 ),                                    //int     stages;
    LEVEL4_HEALTH,                                  //int     health;
    0.40f,                                          //float   maxHealthDecayRate;
    LEVEL4_MIN_HEALTH,                              //int     minHealth;
    0.0f,                                           //float   fallDamage;
    LEVEL4_REGEN,                                   //float   regenRate;
    SCA_FOVWARPS|SCA_ALIENSENSE|SCA_REGEN,                    //int     abilities;
    WP_ALEVEL4,                                     //weapon_t startWeapon;
    0.0f,                                           //float   buildDist;
    0.0f,                                           //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    90,                                             //int     fov;
    0.001f,                                         //float   bob;
    1.1f,                                           //float   bobCycle;
    0.0f,                                           //float   landBob;
    100,                                            //int     steptime;
    LEVEL4_SPEED,                                   //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    170.0f,                                         //float   jumpMagnitude;
    0.1f,                                           //float   knockbackScale;
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    LEVEL4_COST,                                    //int     cost;
    qfalse,                                         //qboolean warmupFree;
    LEVEL4_VALUE,                                   //int     value;
    TEAM_ALIENS                                     //team_t  team;
  },
  {
    PCL_HUMAN,                                      //int     number;
    qtrue,                                          //qboolean enabled;
    "human_base",                                   //char    *name;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ),            //int     stages;
    100000,                                         //int     health;
    0.0f,                                           //float   maxHealthDecayRate;
    0,                                              //int     minHealth;
    1.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_CANUSELADDERS|SCA_STAMINA,          //int     abilities;
    WP_NONE, //special-cased in g_client.c          //weapon_t startWeapon;
    250.0f,                                         //float   buildDist;
    125.0f,                                         //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    90,                                             //int     fov;
    0.002f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    8.0f,                                           //float   landBob;
    100,                                            //int     steptime;
    1.0f,                                           //float   speed;
    10.0f,                                          //float   acceleration;
    1.5f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    220.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    0,                                              //int     cost;
    qtrue,                                          //qboolean warmupFree;
    ALIEN_CREDITS_PER_KILL,                         //int     value;
    TEAM_HUMANS                                     //team_t  team;
  },
  {
    PCL_HUMAN_BSUIT,                                //int     number;
    qtrue,                                          //qboolean enabled;
    "human_bsuit",                                  //char    *name;
    ( 1 << S3 ),                                    //int     stages;
    100000,                                   //int     health;
    0.0f,                                           //float   maxHealthDecayRate;
    0,                                              //int     minHealth;
    1.0f,                                           //float   fallDamage;
    0.0f,                                           //float   regenRate;
    SCA_TAKESFALLDAMAGE|SCA_CANUSELADDERS|SCA_STAMINA,          //int     abilities;
    WP_NONE, //special-cased in g_client.c          //weapon_t startWeapon;
    250.0f,                                         //float   buildDist;
    125.0f,                                         //float   buildDistPrecise;
    qfalse,                                         //qboolean buildPreciseForce;
    90,                                             //int     fov;
    0.002f,                                         //float   bob;
    1.0f,                                           //float   bobCycle;
    5.0f,                                           //float   landBob;
    100,                                            //int     steptime;
    0.9f,                                           //float   speed;
    10.0f,                                          //float   acceleration;
    1.0f,                                           //float   airAcceleration;
    6.0f,                                           //float   friction;
    100.0f,                                         //float   stopSpeed;
    220.0f,                                         //float   jumpMagnitude;
    1.0f,                                           //float   knockbackScale;
    CHARGE_STAMINA_MAX,                             //int     chargeStaminaMax;
    CHARGE_STAMINA_MIN,                             //int     chargeStaminaMin;
    CHARGE_STAMINA_USE_RATE,                        //float   chargeStaminaUseRate;
    CHARGE_STAMINA_RESTORE_RATE,                    //float   chargeStaminaRestoreRate;
    0,                                              //int     cost;
    qtrue,                                          //qboolean warmupFree;
    ALIEN_CREDITS_PER_KILL,                         //int     value;
    TEAM_HUMANS                                     //team_t  team;
  }
};

size_t bg_numClasses = ARRAY_LEN( bg_classList );

static const classAttributes_t nullClass = { 0 };

/*
==============
BG_ClassByName
==============
*/
const classAttributes_t *BG_ClassByName( const char *name )
{
  int i;

  for( i = 0; i < bg_numClasses; i++ )
  {
    if( !Q_stricmp( bg_classList[ i ].name, name ) )
      return &bg_classList[ i ];
  }

  return &nullClass;
}

/*
==============
BG_Class
==============
*/
const classAttributes_t *BG_Class( class_t class )
{
  return ( class >= PCL_NONE && class < PCL_NUM_CLASSES ) ?
    &bg_classList[ class ] : &nullClass;
}

/*
==============
BG_ClassAllowedInStage
==============
*/
qboolean BG_ClassAllowedInStage( class_t class,
                                 stage_t stage,
                                 int gameIsInWarmup )
{
  return qtrue;
}

/*
==============
BG_ClassBoundingBox
==============
*/
void BG_ClassBoundingBox( class_t class,
                          vec3_t mins, vec3_t maxs,
                          vec3_t cmaxs, vec3_t dmins, vec3_t dmaxs )
{
  classConfig_t *classConfig = BG_ClassConfig( class );

  if( mins != NULL )
    VectorCopy( classConfig->mins, mins );

  if( maxs != NULL )
    VectorCopy( classConfig->maxs, maxs );

  if( cmaxs != NULL )
    VectorCopy( classConfig->crouchMaxs, cmaxs );

  if( dmins != NULL )
    VectorCopy( classConfig->deadMins, dmins );

  if( dmaxs != NULL )
    VectorCopy( classConfig->deadMaxs, dmaxs );
}

/*
==============
BG_ClassHasAbility
==============
*/
qboolean BG_ClassHasAbility( class_t class, int ability )
{
  int abilities = BG_Class( class )->abilities;

  return abilities & ability;
}

/*
==============
_BG_ClassCanEvolveFromTo
==============
*/
int _BG_ClassCanEvolveFromTo( class_t fclass,
                              class_t tclass,
                              int credits, int stage,
                              int gameIsInWarmup,
                              qboolean devMode )
{
  int tcost, netCost;
  qboolean warmupFree = gameIsInWarmup && BG_Class(tclass)->warmupFree;


  if( tclass == PCL_NONE ||
      fclass == PCL_NONE ||
      fclass == tclass ||
      tclass == PCL_HUMAN ||
      tclass == PCL_HUMAN_BSUIT ||
      !BG_ClassIsAllowed( tclass, devMode ) ||
      !BG_ClassAllowedInStage( tclass, stage, gameIsInWarmup ) ||
      (tclass == PCL_ALIEN_BUILDER0 && !(gameIsInWarmup)) )
    return -1;

  if( warmupFree )
    return 0;

  netCost = ( tcost = BG_Class( tclass )->cost * ALIEN_CREDITS_PER_KILL ) -
         ( BG_Class( fclass )->cost * ALIEN_CREDITS_PER_KILL );

  if( netCost > 0 )
  {
    if( netCost > credits )
    {
      // Can't afford this class
      return -1;
    }
    else
      return netCost;
  }

  return 0;
}

/*
==============
BG_ClassCanEvolveFromTo
==============
*/
int BG_ClassCanEvolveFromTo(
  class_t fclass, class_t tclass, int credits, int stage,
  int gameIsInWarmup, qboolean devMode, qboolean class_forced) {
  int value;

  if(class_forced) {
    return -1;
  }

  value = _BG_ClassCanEvolveFromTo(
    fclass, tclass, credits, stage, gameIsInWarmup, devMode);

  if(
    gameIsInWarmup && value >= 0 && fclass != PCL_ALIEN_BUILDER0 &&
    !BG_Class(tclass)->warmupFree) {
    value = _BG_ClassCanEvolveFromTo(
      PCL_ALIEN_BUILDER0, tclass, credits, stage, gameIsInWarmup, devMode);
  }

  return value;
}

/*
==============
BG_AlienCanEvolve
==============
*/
qboolean BG_AlienCanEvolve( class_t class, int credits, int stage,
                            int gameIsInWarmup, qboolean devMode,
                            qboolean class_forced )
{
  int i, j, tclass;

  if(class_forced) {
    return qfalse;
  }

  for( i = 0; i < bg_numClasses; i++ )
  {
    if( bg_classList[ i ].number != class )
      continue;

    for( j = 0; j < bg_numClasses; j++ )
    {
      tclass = bg_classList[ j ].number;
      if( 0 <= BG_ClassCanEvolveFromTo( class, tclass, credits, stage,
                                        gameIsInWarmup, devMode, class_forced ) )
        return qtrue;
    }

    return qfalse;
  }

  Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_AlienCanEvolve\n" );
  return qfalse;
}

/*
==============
BG_EvolveScale

While evolving, used for scaling health, damagement,
movement, etc.
==============
*/
float BG_EvolveScale( playerState_t *ps )
{
  if( ps->stats[ STAT_TEAM ] != TEAM_ALIENS )
    return 1.0f;

  if( !( ps->eFlags & EF_EVOLVING ) )
    return 1.0f;

  return MAX( 1.0f - ( ( (float)ps->stats[ STAT_MISC3 ] ) /
                     ( (float)ps->stats[ STAT_MISC2 ] ) ),
              0.50f );
}

/*
==============
BG_GetEvolveCoolDownDecayTimer
==============
*/
int BG_GetEvolveCoolDownDecayTimer( playerState_t *ps )
{
  return ( ( ps->misc[ MISC_MISC2 ] ) & 127 );
}

/*
==============
BG_GetEvolveCoolDown
==============
*/
int BG_GetEvolveCoolDown( playerState_t *ps )
{
  return ( ( ps->misc[ MISC_MISC2 ]  >> 7 ) & 65535 );
}

/*
==============
BG_SetEvolveCoolDownDecayTimer
==============
*/
void BG_SetEvolveCoolDownDecayTimer( playerState_t *ps,
                                     int decayTime )
{
  int coolDown = BG_GetEvolveCoolDown( ps );

  if( decayTime < 0 )
    decayTime = 0;
  else if( decayTime > 127 )
    decayTime = 127;

  ps->misc[ MISC_MISC2 ] = (coolDown<<7)|(decayTime);
}

/*
==============
BG_SetEvolveCoolDown
==============
*/
void BG_SetEvolveCoolDown( playerState_t *ps,
                                     int coolDown )
{
  int         decayTime = BG_GetEvolveCoolDownDecayTimer( ps );

  if( coolDown < 0 )
    coolDown = 0;
  else if( coolDown > 65535 )
    coolDown = 65535;

  ps->misc[ MISC_MISC2 ] = (coolDown<<7)|(decayTime);
}

////////////////////////////////////////////////////////////////////////////////

static const weaponAttributes_t bg_weapons[ ] =
{
  {
    WP_ALEVEL0,           //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,                //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "level0",             //char      *name;
    "Bite",               //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LEVEL0_BITE_REPEAT,   //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    LEVEL0_BITE_K_SCALE,  //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTimeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL1,           //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "level1",             //char      *name;
    "Claws",              //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LEVEL1_CLAW_REPEAT,   //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    LEVEL1_CLAW_K_SCALE,  //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL1_UPG,       //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "level1upg",          //char      *name;
    "Claws Upgrade",      //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LEVEL1_CLAW_U_REPEAT, //int       repeatRate1;
    LEVEL1_PCLOUD_REPEAT, //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    LEVEL1_CLAW_U_K_SCALE, //float    knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL2,           //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "level2",             //char      *name;
    "Bite",               //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LEVEL2_CLAW_REPEAT,   //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    LEVEL2_CLAW_K_SCALE,  //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL2_UPG,       //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "level2upg",          //char      *name;
    "Zap",                //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LEVEL2_CLAW_U_REPEAT, //int       repeatRate1;
    LEVEL2_AREAZAP_REPEAT, //int      repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    LEVEL2_CLAW_U_K_SCALE, //float    knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ASPITFIRE,         //int       weaponNum;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages
    SLOT_WEAPON,          //int       slots;
    "spitfire",           //char      *weaponName;
    "Spitfire",           //char      *humanName;
    SPITFIRE_GAS_TRAIL_PUFFS,//int       maxAmmo; 
    SPITFIRE_GAS_TRAIL_CLIPS,//int       maxClips;
    0,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qtrue,                //qboolean  usesBarbs;
    SPITFIRE_GAS_TRAIL_REGEN,//int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    SPITFIRE_ZAP_REPEAT,  //int       repeatRate1;
    SPITFIRE_POUNCE_REPEAT, //int       repeatRate2;
    SPITFIRE_GAS_TRAIL_REPEAT, //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    SPITFIRE_GAS_TRAIL_RELOAD_TIME,//int       reloadTime;
    SPITFIRE_ZAP_K_SCALE, //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qtrue,                //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_TERTIARY,       //weaponMode_t  weaponMode;
        TR_LINEAR,          //trType_t      trType;
        LEVEL3_BOUNCEBALL_LIFETIME,  //int       missileLifeTime;
        0,                  //int       missileSize;
        LEVEL3_BOUNCEBALL_SPEED  //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qtrue,              //qboolean       enabled;
        "gas_trail",        //char           *class_name;
        MOD_SPITFIRE_GAS_TRAIL,//meansOfDeath_t mod;
        MOD_SPITFIRE_GAS_TRAIL,//meansOfDeath_t splash_mod;
        SPITFIRE_GAS_TRAIL_DMG, //int            damage;
        SPITFIRE_GAS_TRAIL_DMG, //int            splash_damage;
        SPITFIRE_GAS_TRAIL_RADIUS, //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {-25.0f, -25.0f, -25.0f}, //vec3_t         mins;
        {25.0f, 25.0f, 25.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        SPITFIRE_GAS_TRAIL_SPEED, //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        SPITFIRE_GAS_TRAIL_LIFETIME, //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_FULL,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qtrue,              //qboolean       save_missiles;
        qtrue,              //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL3,           //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "level3",             //char      *name;
    "Pounce",             //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_POUNCE,    //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LEVEL3_CLAW_REPEAT,   //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    LEVEL3_CLAW_K_SCALE,  //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL3_UPG,       //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "level3upg",          //char      *name;
    "Pounce (upgrade)",   //char      *humanName;
    4,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qtrue,                //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_POUNCE,    //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LEVEL3_CLAW_U_REPEAT, //int       repeatRate1;
    0,                    //int       repeatRate2;
    LEVEL3_BOUNCEBALL_REPEAT, //int   repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    LEVEL3_CLAW_U_K_SCALE, //float    knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qtrue,                //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_TERTIARY,       //weaponMode_t  weaponMode;
        TR_GRAVITY,         //trType_t      trType;
        LEVEL3_BOUNCEBALL_LIFETIME,  //int       missileLifeTime;
        0,                  //int       missileSize;
        LEVEL3_BOUNCEBALL_SPEED  //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qtrue,              //qboolean       enabled;
        "bounceball",       //char           *class_name;
        MOD_LEVEL3_BOUNCEBALL,//meansOfDeath_t mod;
        MOD_LEVEL3_BOUNCEBALL,//meansOfDeath_t splash_mod;
        LEVEL3_BOUNCEBALL_DMG,//int            damage;
        LEVEL3_BOUNCEBALL_DMG,//int            splash_damage;
        LEVEL3_BOUNCEBALL_RADIUS,//float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        LEVEL3_BOUNCEBALL_SPEED,//float          speed;
        TR_GRAVITY,         //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        3000,               //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_FULL,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ALEVEL4,           //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "level4",             //char      *name;
    "Charge",             //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LEVEL4_CLAW_REPEAT,   //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    LEVEL4_CLAW_K_SCALE,  //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_BLASTER,           //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    0,                    //int       slots;
    "blaster",            //char      *name;
    "Blaster",            //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;;
    BLASTER_REPEAT,       //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    BLASTER_K_SCALE,      //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_LINEAR,          //trType_t      trType;
        BLASTER_LIFETIME,   //int       missileLifeTime;
        BLASTER_SIZE,       //int       missileSize;
        BLASTER_SPEED       //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "blaster",          //char           *class_name;
        MOD_BLASTER,        //meansOfDeath_t mod;
        MOD_BLASTER,        //meansOfDeath_t splash_mod;
        BLASTER_DMG,        //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {-BLASTER_SIZE, -BLASTER_SIZE, -BLASTER_SIZE}, //vec3_t         mins;
        {BLASTER_SIZE, BLASTER_SIZE, BLASTER_SIZE}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        BLASTER_SPEED,      //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        10000,              //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_MACHINEGUN,        //int       number;
    qtrue,                //qboolean enabled;
    RIFLE_PRICE,          //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "rifle",              //char      *name;
    "Rifle",              //char      *humanName;
    RIFLE_CLIPSIZE,       //int       maxAmmo;
    RIFLE_MAXCLIPS,       //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_REMAINDER_AMMO,//weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    RIFLE_REPEAT,         //int       repeatRate1;
    RIFLE_REPEAT2,        //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    RIFLE_ALT_BURST_ROUNDS,//int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    RIFLE_ALT_BURST_DELAY,//int       burstDelay2;
    0,                    //int       burstDelay3;
    RIFLE_RELOAD,         //int       reloadTime;
    RIFLE_K_SCALE,        //float     knockbackScale;
    RIFLE_RECOIL,         //float     recoil1;
    RIFLE_RECOIL,         //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_PAIN_SAW,          //int       number;
    qtrue,                //qboolean enabled;
    PAINSAW_PRICE,        //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "psaw",               //char      *name;
    "Pain Saw",           //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_OVERHEAT,  //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_CONTACT, //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qtrue,                //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    PAINSAW_OVERHEAT_TIME,//int       overheatTime;
    PAINSAW_COOLDOWN_TIME,//int       cooldownTime;
    PAINSAW_OVERHEAT_DELAY,//int       overheatWeaponDelayTime;
    PAINSAW_REPEAT,       //int       repeatRate1;
    PAINSAW_REPEAT,       //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    PAINSAW_RELOAD,       //int       reloadTime;
    PAINSAW_K_SCALE,      //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_SHOTGUN,           //int       number;
    qtrue,                //qboolean enabled;
    SHOTGUN_PRICE,        //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "shotgun",            //char      *name;
    "Shotgun",            //char      *humanName;
    SHOTGUN_SHELLS,       //int       maxAmmo;
    SHOTGUN_MAXCLIPS,     //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qtrue,                //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_REMAINDER_AMMO,//weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    SHOTGUN_REPEAT,       //int       repeatRate1;
    SHOTGUN_CHOKE_REPEAT, //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    SHOTGUN_RELOAD,       //int       reloadTime;
    SHOTGUN_K_SCALE,      //float     knockbackScale;
    SHOTGUN_RECOIL1,      //float     recoil1;
    SHOTGUN_RECOIL2,      //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qtrue,            //qboolean               predicted;
        SHOTGUN_PELLETS,  //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        SHOTGUN_SPREAD,   //float        spread;
        SHOTGUN_DMG,      //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        SHOTGUN_RANGE,    //float        range;
      },
      {                   //struct    splatter[1]
        qtrue,            //qboolean               predicted;
        SHOTGUN_PELLETS,  //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        SHOTGUN_CHOKE_SPREAD, //float        spread;
        SHOTGUN_CHOKE_DMG, //int          impactDamage;
        SHOTGUN_CHOKE_DMG_FALLOFF, //float        impactDamageFalloff;
        SHOTGUN_CHOKE_DMG_CAP, //int          impactDamageCap;
        SHOTGUN_CHOKE_DMG_FALLOFF, //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_DBSHOTGUN,         //int       number;
    qtrue,                //qboolean enabled;
    DBSHOTGUN_PRICE,      //int       price;
    qfalse,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "dbshotgun",          //char      *name;
    "Double Barreled Shotgun", //char      *humanName;
    DBSHOTGUN_SHELLS,     //int       maxAmmo;
    DBSHOTGUN_MAXCLIPS,   //int       maxClips;
    1,                    //int       ammoUsage1;
    2,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qtrue,                //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_ONE_ROUND_TO_ONE_CLIP, //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    DBSHOTGUN_REPEAT,     //int       repeatRate1;
    DBSHOTGUN_REPEAT,     //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    DBSHOTGUN_RELOAD,     //int       reloadTime;
    DBSHOTGUN_K_SCALE,    //float     knockbackScale;
    DBSHOTGUN_RECOIL1,    //float     recoil1;
    DBSHOTGUN_RECOIL2,    //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qtrue,            //qboolean               predicted;
        DBSHOTGUN_PELLETS, //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        DBSHOTGUN_SPREAD, //float        spread;
        DBSHOTGUN_DMG,    //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        DBSHOTGUN_RANGE,  //float        range;
      },
      {                   //struct    splatter[1]
        qtrue,            //qboolean               predicted;
        DBSHOTGUN_DOUBLE_PELLETS, //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        DBSHOTGUN_SPREAD, //float        spread;
        DBSHOTGUN_DMG,    //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        DBSHOTGUN_RANGE,  //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_LAS_GUN,           //int       number;
    qtrue,                //qboolean enabled;
    LASGUN_PRICE,         //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "lgun",               //char      *name;
    "Las Gun",            //char      *humanName;
    LASGUN_AMMO,          //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qtrue,                //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LASGUN_REPEAT,        //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    LASGUN_RELOAD,        //int       reloadTime;
    LASGUN_K_SCALE,       //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qtrue,                //qboolean  canZoom;
    20.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_MASS_DRIVER,       //int       number;
    qtrue,                //qboolean enabled;
    MDRIVER_PRICE,        //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "mdriver",            //char      *name;
    "Mass Driver",        //char      *humanName;
    MDRIVER_CLIPSIZE,     //int       maxAmmo;
    MDRIVER_MAXCLIPS,     //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qtrue,                //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_REMAINDER_AMMO, //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    MDRIVER_REPEAT,       //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    MDRIVER_RELOAD,       //int       reloadTime;
    MDRIVER_K_SCALE,      //float     knockbackScale;
    MDRIVER_RECOIL,       //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qtrue,                //qboolean  canZoom;
    20.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_CHAINGUN,          //int       number;
    qtrue,                //qboolean enabled;
    CHAINGUN_PRICE,       //int       price;
    qfalse,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "chaingun",           //char      *name;
    "Chaingun",           //char      *humanName;
    CHAINGUN_BULLETS,     //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_SPINUP,    //weapon_Option_A_t weaponOptionA;
    CHAINGUN_START_REPEAT, //int       spinUpStartRepeat;
    CHAINGUN_START_SPREAD, //int       spinUpStartSpread;
    CHAINGUN_SPINUP_TIME, //int       spinUpTime;
    CHAINGUN_SPINDOWN_TIME, //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    CHAINGUN_REPEAT,      //int       repeatRate1;
    CHAINGUN_REPEAT2,     //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    CHAINGUN_RELOAD,      //int       reloadTime;
    CHAINGUN_K_SCALE,     //float     knockbackScale;
    CHAINGUN_RECOIL,      //float     recoil1;
    CHAINGUN_RECOIL,      //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_FLAMER,            //int       number;
    qtrue,                //qboolean enabled;
    FLAMER_PRICE,         //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S2 )|( 1 << S3 ), //int    stages;
    SLOT_WEAPON,          //int       slots;
    "flamer",             //char      *name;
    "Flame Thrower",      //char      *humanName;
    FLAMER_GAS,           //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    FLAMER_REPEAT,        //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    FLAMER_RELOAD,        //int       reloadTime;
    FLAMER_K_SCALE,       //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "flame",            //char           *class_name;
        MOD_FLAMER,         //meansOfDeath_t mod;
        MOD_FLAMER_SPLASH,  //meansOfDeath_t splash_mod;
        FLAMER_DMG,         //int            damage;
        FLAMER_SPLASHDAMAGE,//int            splash_damage;
        FLAMER_RADIUS,      //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {-FLAMER_START_SIZE, -FLAMER_START_SIZE, -FLAMER_START_SIZE}, //vec3_t         mins;
        {FLAMER_START_SIZE, FLAMER_START_SIZE, FLAMER_START_SIZE}, //vec3_t         maxs;
        {4.5f, -7.0f, 0.0f},//vec3_t         muzzle_offset;
        FLAMER_SPEED,       //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        FLAMER_LIFETIME,    //int            explode_delay;
        qfalse,             //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qfalse,             //qboolean       relative_missile_speed_along_aim;
        FLAMER_LAG,         //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qtrue,              //qboolean       scale;
        0,                  //int            scale_start_time;
        FLAMER_EXPAND_TIME, //int            scale_stop_time;
        {-FLAMER_FULL_SIZE, -FLAMER_FULL_SIZE, -FLAMER_FULL_SIZE}, //vec3_t         scale_stop_mins;
        {FLAMER_FULL_SIZE, FLAMER_FULL_SIZE, FLAMER_FULL_SIZE}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {4.5f, -7.0f, 0.0f},//vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qfalse,             //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {4.5f, -7.0f, 0.0f},//vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qfalse,             //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_PULSE_RIFLE,       //int       number;
    qtrue,                //qboolean enabled;
    PRIFLE_PRICE,         //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S2 )|( 1 << S3 ), //int    stages;
    SLOT_WEAPON,          //int       slots;
    "prifle",             //char      *name;
    "Pulse Rifle",        //char      *humanName;
    PRIFLE_CLIPS,         //int       maxAmmo;
    PRIFLE_MAXCLIPS,      //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qtrue,                //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_REMAINDER_AMMO,//weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    PRIFLE_REPEAT,        //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    PRIFLE_RELOAD,        //int       reloadTime;
    PRIFLE_K_SCALE,       //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_LINEAR,          //trType_t      trType;
        PRIFLE_LIFETIME,    //int       missileLifeTime;
        PRIFLE_SIZE,        //int       missileSize;
        PRIFLE_SPEED        //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "pulse",            //char           *class_name;
        MOD_PRIFLE,         //meansOfDeath_t mod;
        MOD_PRIFLE,         //meansOfDeath_t splash_mod;
        PRIFLE_DMG,         //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {-PRIFLE_SIZE, -PRIFLE_SIZE, -PRIFLE_SIZE}, //vec3_t         mins;
        {PRIFLE_SIZE, PRIFLE_SIZE, PRIFLE_SIZE}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        PRIFLE_SPEED,       //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        10000,              //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_LUCIFER_CANNON,    //int       number;
    qtrue,                //qboolean enabled;
    LCANNON_PRICE,        //int       price;
    qfalse,               //qboolean  warmupFree;
    ( 1 << S3 ),          //int       stages;
    SLOT_WEAPON,          //int       slots;
    "lcannon",            //char      *name;
    "Lucifer Cannon",     //char      *humanName;
    LCANNON_AMMO,         //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qtrue,                //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LCANNON_REPEAT,       //int       repeatRate1;
    LCANNON_SECONDARY_REPEAT, //int   repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    LCANNON_RELOAD,       //int       reloadTime;
    LCANNON_K_SCALE,      //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_LINEAR,          //trType_t      trType;
        LCANNON_LIFETIME,   //int       missileLifeTime;
        LCANNON_SIZE,       //int       missileSize;
        LCANNON_SECONDARY_SPEED  //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_LINEAR,          //trType_t      trType;
        LCANNON_LIFETIME,   //int       missileLifeTime;
        LCANNON_SIZE,       //int       missileSize;
        LCANNON_SPEED_MIN   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "lcannon",          //char           *class_name;
        MOD_LCANNON,        //meansOfDeath_t mod;
        MOD_LCANNON_SPLASH, //meansOfDeath_t splash_mod;
        LCANNON_DAMAGE,     //int            damage;
        ((LCANNON_DAMAGE) / 2), //int            splash_damage;
        LCANNON_RADIUS,     //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {-LCANNON_SIZE, -LCANNON_SIZE, -LCANNON_SIZE}, //vec3_t         mins;
        {LCANNON_SIZE, LCANNON_SIZE, LCANNON_SIZE}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        LCANNON_SECONDARY_SPEED,//float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        10000,              //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qtrue,              //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        LCANNON_CHARGE_TIME_MAX, //int            charged_time_max;
        qtrue,              //qboolean       charged_damage;
        qtrue,              //qboolean       charged_speed;
        LCANNON_CHARGE_TIME_MIN,//int            charged_speed_min;
        LCANNON_SECONDARY_DAMAGE,//int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qtrue,              //qboolean       enabled;
        "lcannon",          //char           *class_name;
        MOD_LCANNON,        //meansOfDeath_t mod;
        MOD_LCANNON_SPLASH, //meansOfDeath_t splash_mod;
        LCANNON_SECONDARY_DAMAGE, //int            damage;
        (LCANNON_SECONDARY_DAMAGE / 2), //int            splash_damage;
        LCANNON_SECONDARY_RADIUS, //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {-LCANNON_SIZE, -LCANNON_SIZE, -LCANNON_SIZE}, //vec3_t         mins;
        {LCANNON_SIZE, LCANNON_SIZE, LCANNON_SIZE}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        LCANNON_SECONDARY_SPEED, //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        10000,              //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qtrue,              //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qtrue,              //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_GRENADE,           //int       number;
    qtrue,                //qboolean enabled;
    GRENADE_PRICE,        //int       price;
    qfalse,               //qboolean  warmupFree;
    ( 1 << S2 )|( 1 << S3 ), //int    stages;
    SLOT_NONE,            //int       slots;
    "grenade",            //char      *name;
    "Grenade",            //char      *humanName;
    1,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    GRENADE_REPEAT,       //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    GRENADE_K_SCALE,      //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_GRAVITY,         //trType_t      trType;
        GRENADE_LIFETIME,   //int       missileLifeTime;
        GRENADE_SIZE,       //int       missileSize;
        GRENADE_SPEED       //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "grenade",          //char           *class_name;
        MOD_GRENADE,        //meansOfDeath_t mod;
        MOD_GRENADE,        //meansOfDeath_t splash_mod;
        GRENADE_DAMAGE,     //int            damage;
        GRENADE_DAMAGE,     //int            splash_damage;
        GRENADE_RANGE,      //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {-3.0f, -3.0f, -3.0f}, //vec3_t         mins;
        {3.0f, 3.0f, 3.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        GRENADE_SPEED,      //float          speed;
        TR_GRAVITY,         //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        5000,               //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_HALF,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        GRENADE_HEALTH,     //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_HALF,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_HALF,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_FRAGNADE,          //int       number;
    qtrue,                //qboolean enabled;
    FRAGNADE_PRICE,       //int       price;
    qfalse,               //qboolean  warmupFree;
    ( 1 << S2 )|( 1 << S3 ), //int    stages;
    SLOT_NONE,            //int       slots;
    "fragnade",           //char      *name;
    "Fragmentation Grenade", //char      *humanName;
    1,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    FRAGNADE_REPEAT,      //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    FRAGNADE_K_SCALE,     //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_GRAVITY,         //trType_t      trType;
        FRAGNADE_LIFETIME,   //int       missileLifeTime;
        FRAGNADE_SIZE,       //int       missileSize;
        FRAGNADE_SPEED       //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "grenade",          //char           *class_name;
        MOD_FRAGNADE,       //meansOfDeath_t mod;
        MOD_FRAGNADE,       //meansOfDeath_t splash_mod;
        FRAGNADE_BLAST_DAMAGE, //int            damage;
        FRAGNADE_BLAST_DAMAGE, //int            splash_damage;
        FRAGNADE_BLAST_RANGE, //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {-3.0f, -3.0f, -3.0f}, //vec3_t         mins;
        {3.0f, 3.0f, 3.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        FRAGNADE_SPEED,     //float          speed;
        TR_GRAVITY,         //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        (2000 + 650),       //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_HALF,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        GRENADE_HEALTH,     //int            health;
        350.0f,             //float          kick_up_speed;
        2000,               //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_HALF,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_HALF,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        FRAGNADE_FRAGMENTS, //unsigned int number;
        SPLATP_MIRRORED_INVERSE_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_UNIFORM_ALTERNATING, //splatterDistribution_t distribution;
        FRAGNADE_PITCH_LAYERS, //unsigned int           pitchLayers;
        FRAGNADE_SPREAD, //float        spread;
        FRAGNADE_FRAGMENT_DMG, //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        FRAGNADE_RANGE, //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_LASERMINE,         //int       number;
    qtrue,                //qboolean enabled;
    GRENADE_PRICE,        //int       price;
    qfalse,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int    stages;
    SLOT_NONE,            //int       slots;
    "lasermine",          //char      *name;
    "Laser Mine",         //char      *humanName;
    1,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LASERMINE_REPEAT,     //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    LASERMINE_K_SCALE,    //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_GRAVITY,         //trType_t      trType;
        120000,             //int       missileLifeTime;
        3,                  //int       missileSize;
        LASERMINE_SPEED     //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "lasermine",        //char           *class_name;
        MOD_LASERMINE,      //meansOfDeath_t mod;
        MOD_LASERMINE,      //meansOfDeath_t splash_mod;
        LASERMINE_DAMAGE,   //int            damage;
        LASERMINE_DAMAGE,   //int            splash_damage;
        LASERMINE_RANGE,    //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {-3.0f, -3.0f, -3.0f}, //vec3_t         mins;
        {3.0f, 3.0f, 3.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        LASERMINE_SPEED,    //float          speed;
        TR_GRAVITY,         //trType_t       trajectory_type;
        LASERMINE_INIT_TIME,//int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_HALF,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qtrue,              //qboolean       impact_stick;
        qtrue,              //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        LASERMINE_HEALTH,   //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        LASERMINE_RANGE,    //float          tripwire_range;
        LASERMINE_CHECK_FREQUENCY, //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qtrue,              //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        LASERMINE_RANGE,    //float          tripwire_range;
        LASERMINE_CHECK_FREQUENCY, //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qtrue,              //qboolean       impact_stick;
        qtrue,              //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        LASERMINE_RANGE,    //float          tripwire_range;
        LASERMINE_CHECK_FREQUENCY, //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_LAUNCHER,          //int       number;
    qtrue,                //qboolean enabled;
    LAUNCHER_PRICE,       //int       price;
    qfalse,               //qboolean  warmupFree;
    ( 1 << S3 ),          //int       stages;
    SLOT_WEAPON,          //int       slots;
    "grenade_launcher",   //char      *name;
    "Grenade Launcher",   //char      *humanName;
    LAUNCHER_AMMO,        //int       maxAmmo;
    LAUNCHER_MAXCLIPS,    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    LAUNCHER_ROUND_PRICE, //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_REMAINDER_AMMO,//weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LAUNCHER_REPEAT,      //int       repeatRate1;
    LAUNCHER_REPEAT,      //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    LAUNCHER_RELOAD,      //int       reloadTime;
    LAUNCHER_K_SCALE,     //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_GRAVITY,         //trType_t      trType;
        GRENADE_LIFETIME,   //int       missileLifeTime;
        GRENADE_SIZE,       //int       missileSize;
        LAUNCHER_SPEED      //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "grenade",          //char           *class_name;
        MOD_GRENADE_LAUNCHER, //meansOfDeath_t mod;
        MOD_GRENADE_LAUNCHER, //meansOfDeath_t splash_mod;
        LAUNCHER_DAMAGE,    //int            damage;
        LAUNCHER_DAMAGE,    //int            splash_damage;
        LAUNCHER_RADIUS,    //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {-3.0f, -3.0f, -3.0f}, //vec3_t         mins;
        {3.0f, 3.0f, 3.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        LAUNCHER_SPEED,     //float          speed;
        TR_GRAVITY,         //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        5000,               //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qtrue,              //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qtrue,              //qboolean       enabled;
        "grenade2",         //char           *class_name;
        MOD_GRENADE_LAUNCHER, //meansOfDeath_t mod;
        MOD_GRENADE_LAUNCHER, //meansOfDeath_t splash_mod;
        LAUNCHER_DAMAGE,    //int            damage;
        LAUNCHER_DAMAGE,    //int            splash_damage;
        LAUNCHER_RADIUS,    //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {-3.0f, -3.0f, -3.0f}, //vec3_t         mins;
        {3.0f, 3.0f, 3.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        LAUNCHER_SPEED,     //float          speed;
        TR_GRAVITY,         //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        5000,               //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_HALF,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qtrue,              //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        1,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qtrue,              //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qfalse,             //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qtrue,              //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        1,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_LIGHTNING,         //int       number;
    qtrue,                //qboolean enabled;
    LIGHTNING_PRICE,      //int       price;
    qfalse,               //qboolean  warmupFree;
    ( 1 << S3 ),          //int       stages;
    SLOT_WEAPON,          //int       slots;
    "lightning",          //char      *name;
    "Lightning Gun",      //char      *humanName;
    LIGHTNING_AMMO,       //int       maxAmmo;
    LIGHTNING_MAXCLIPS,   //int       maxClips;
    1,                    //int       ammoUsage1;
    LIGHTNING_BALL_AMMO_USAGE,//int       ammoUsage2;
    0,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qtrue,                //qboolean  ammoPurchasable;
    qfalse,               //int       infiniteAmmo;
    qtrue,                //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    LIGHTNING_BOLT_CHARGE_TIME_MIN,//int       repeatRate1;
    LIGHTNING_BALL_REPEAT,//int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    LIGHTNING_BALL_BURST_ROUNDS,//int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    LIGHTNING_BALL_BURST_DELAY,//int       burstDelay2;
    0,                    //int       burstDelay3;
    LIGHTNING_RELOAD,     //int       reloadTime;
    LIGHTNING_K_SCALE,    //float     knockbackScale;
    50.0f,                //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qtrue,                //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qtrue,              //qboolean       enabled;
        "lightningBall",    //char           *class_name;
        MOD_LIGHTNING,      //meansOfDeath_t mod;
        MOD_LIGHTNING,      //meansOfDeath_t splash_mod;
        LIGHTNING_BALL_DAMAGE,//int            damage;
        LIGHTNING_BALL_SPLASH_DMG,//int            splash_damage;
        LIGHTNING_BALL_RADIUS2,//float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {-LIGHTNING_BALL_SIZE, -LIGHTNING_BALL_SIZE, -LIGHTNING_BALL_SIZE}, //vec3_t         mins;
        {LIGHTNING_BALL_SIZE, LIGHTNING_BALL_SIZE, LIGHTNING_BALL_SIZE}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        LIGHTNING_BALL_SPEED,//float          speed;
        TR_HALF_GRAVITY,    //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        LIGHTNING_BALL_LIFETIME,//int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_HALF,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        1,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qtrue,              //qboolean       save_missiles;
        qtrue,              //qboolean       detonate_saved_missiles;
        0,                  //int            detonate_saved_missiles_delay;
        0,                  //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qtrue,              //qboolean       enabled;
        "lightningEMP",     //char           *class_name;
        MOD_LIGHTNING_EMP,  //meansOfDeath_t mod;
        MOD_LIGHTNING_EMP,  //meansOfDeath_t splash_mod;
        LIGHTNING_EMP_DAMAGE,//int            damage;
        LIGHTNING_EMP_SPLASH_DMG,//int            splash_damage;
        LIGHTNING_EMP_RADIUS,//float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {-LIGHTNING_BALL_SIZE, -LIGHTNING_BALL_SIZE, -LIGHTNING_BALL_SIZE}, //vec3_t         mins;
        {LIGHTNING_BALL_SIZE, LIGHTNING_BALL_SIZE, LIGHTNING_BALL_SIZE}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        LIGHTNING_EMP_SPEED,//float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        1,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_LOCKBLOB_LAUNCHER, //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "lockblob",           //char      *name;
    "Lock Blob",          //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    500,                  //int       repeatRate1;
    500,                  //int       repeatRate2;
    500,                  //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    LOCKBLOB_K_SCALE,     //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_LINEAR,          //trType_t      trType;
        LOCKBLOB_LIFETIME,  //int       missileLifeTime;
        0,                  //int       missileSize;
        LOCKBLOB_SPEED      //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "lockblob",         //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        LOCKBLOB_DMG,       //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        500.0f,             //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        15000,              //int            explode_delay;
        qfalse,             //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_LOCK,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qfalse,             //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_LOCK,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qfalse,             //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_LOCK,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_HIVE,              //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "hive",               //char      *name;
    "Hive",               //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    500,                  //int       repeatRate1;
    500,                  //int       repeatRate2;
    500,                  //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    HIVE_K_SCALE,         //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "hive_swarm",       //char           *class_name;
        MOD_SWARM,          //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        HIVE_DMG,           //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        HIVE_SPEED,         //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        HIVE_LIFETIME,      //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_FULL,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qfalse,             //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        HIVE_DIR_CHANGE_PERIOD, //int            search_and_destroy_change_period;
        qtrue,              //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qfalse,             //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        HIVE_DIR_CHANGE_PERIOD, //int            search_and_destroy_change_period;
        qtrue,              //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qfalse,             //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        HIVE_DIR_CHANGE_PERIOD, //int            search_and_destroy_change_period;
        qtrue,              //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_TESLAGEN,          //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "teslagen",           //char      *name;
    "Tesla Generator",    //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qtrue,                //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    500,                  //int       repeatRate1;
    500,                  //int       repeatRate2;
    500,                  //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    TESLAGEN_K_SCALE,     //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_MGTURRET,          //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "mgturret",           //char      *name;
    "Machinegun Turret",  //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    0,                    //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    MGTURRET_K_SCALE,     //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_FLAME_TURRET,      //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,                //qboolean  warmupFree;
    ( 1 << S2 )|( 1 << S3 ), //int    stages;
    SLOT_WEAPON,          //int       slots;
    "flame_turret",       //char      *name;
    "Flame Turret",       //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    0,                    //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    FLAME_TURRET_K_SCALE, //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qfalse,               //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,               //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "flame_turret_flame", //char           *class_name;
        MOD_FLAME_TURRET,   //meansOfDeath_t mod;
        MOD_FLAME_TURRET,   //meansOfDeath_t splash_mod;
        FLAME_TURRET_DMG,   //int            damage;
        FLAME_TURRET_FIRE_SPLASHDAMAGE, //int            splash_damage;
        FLAME_TURRET_RADIUS, //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {-FLAME_TURRET_START_SIZE, -FLAME_TURRET_START_SIZE, -FLAME_TURRET_START_SIZE}, //vec3_t         mins;
        {FLAME_TURRET_START_SIZE, FLAME_TURRET_START_SIZE, FLAME_TURRET_START_SIZE}, //vec3_t         maxs;
        {0.0f, 0.0f, 8.0f}, //vec3_t         muzzle_offset;
        FLAME_TURRET_SPEED, //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        FLAME_TURRET_LIFETIME, //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        FLAMER_LAG,         //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qtrue,              //qboolean       scale;
        0,                  //int            scale_start_time;
        FLAME_TURRET_EXPAND_TIME, //int            scale_stop_time;
        {-FLAMER_FULL_SIZE, -FLAMER_FULL_SIZE, -FLAMER_FULL_SIZE}, //vec3_t         scale_stop_mins;
        {FLAMER_FULL_SIZE, FLAMER_FULL_SIZE, FLAMER_FULL_SIZE} //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_ABUILD,            //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "abuild",             //char      *name;
    "Alien build weapon", //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    ABUILDER_BUILD_REPEAT, //int      repeatRate1;
    ABUILDER_CLAW_REPEAT, //int       repeatRate2;
    ABUILDER_BLOB_REPEAT, //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    ABUILDER_CLAW_K_SCALE, //float    knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
        {                     //impactPrediction_t impactPrediction[0];
          WPM_TERTIARY,       //weaponMode_t  weaponMode;
          TR_GRAVITY,         //trType_t      trType;
          ABUILDER_BLOB_LIFETIME,  //int       missileLifeTime;
          0,                  //int       missileSize;
          ABUILDER_BLOB_SPEED //int       missileLaunchSpeed;
        },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_ABUILD2,           //int       number;
    qtrue,                //qboolean enabled;
    0,                    //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "abuildupg",          //char      *name;
    "Alien build weapon2", //char     *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    ABUILDER_BUILD_REPEAT, //int      repeatRate1;
    ABUILDER_CLAW_REPEAT, //int       repeatRate2;
    ABUILDER_BLOB_REPEAT, //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    ABUILDER_CLAW_K_SCALE, //float    knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qtrue,                //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_TERTIARY,       //weaponMode_t  weaponMode;
        TR_GRAVITY,         //trType_t      trType;
        ABUILDER_BLOB_LIFETIME,  //int       missileLifeTime;
        0,                  //int       missileSize;
        ABUILDER_BLOB_SPEED//int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_SLOW,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qtrue,              //qboolean       enabled;
        "slowblob",         //char           *class_name;
        MOD_SLOWBLOB,       //meansOfDeath_t mod;
        MOD_SLOWBLOB,       //meansOfDeath_t splash_mod;
        ABUILDER_BLOB_DMG,  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        ABUILDER_BLOB_SPEED,//float          speed;
        TR_GRAVITY,         //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        15000,              //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_SLOW,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qtrue,              //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_ALIENS           //team_t    team;
  },
  {
    WP_HBUILD,            //int       number;
    qtrue,                //qboolean enabled;
    HBUILD_PRICE,         //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    0,          //int       slots;
    "ckit",               //char      *name;
    "Construction Kit",   //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    HBUILD_REPEAT,        //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    0.0f,                 //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qfalse,                //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_HBUILD2,            //int       number;
    qtrue,                //qboolean enabled;
    HBUILD_PRICE,         //int       price;
    qtrue,               //qboolean  warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_WEAPON,          //int       slots;
    "ackit",               //char      *name;
    "Adv Construction Kit", //char      *humanName;
    0,                    //int       maxAmmo;
    0,                    //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    0,                    //int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    HBUILD_REPEAT,        //int       repeatRate1;
    0,                    //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    0,                    //int       reloadTime;
    0.0f,                 //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qfalse,               //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  },
  {
    WP_PORTAL_GUN,        //int       weaponNum;
    qtrue,                //qboolean enabled;
    PORTALGUN_PRICE,      //int       price;
    qfalse,               //qboolean  warmupFree;
    ( 1 << S3 ),          //int       stages
    SLOT_WEAPON,          //int       slots;
    "portalgun",          //char      *weaponName;
    "Portal Gun",         //char      *humanName;
    PORTALGUN_AMMO,       //int       maxAmmo;
    PORTALGUN_MAXCLIPS,   //int       maxClips;
    1,                    //int       ammoUsage1;
    1,                    //int       ammoUsage2;
    1,                    //int       ammoUsage3;
    qfalse,               //qboolean  allowPartialAmmoUsage;
    PORTALGUN_ROUND_PRICE,//int       roundPrice;
    qfalse,               //qboolean  ammoPurchasable;
    qtrue,                //int       infiniteAmmo;
    qfalse,               //int       usesEnergy;
    qfalse,               //qboolean  usesBarbs;
    15000,                //int       barbRegenTime;
    WEAPONOPTA_NONE,      //weapon_Option_A_t weaponOptionA;
    0,                    //int       spinUpStartRepeat;
    0,                    //int       spinUpStartSpread;
    0,                    //int       spinUpTime;
    0,                    //int       spinDownTime;
    OVERHEAT_FROM_USE,    //overheatType_t overheatType;
    qfalse,               //qboolean  overheatPrimaryMode;
    qfalse,               //qboolean  overheatAltMode;
    qfalse,               //qboolean  overheatThirdMode;
    0,                    //int       overheatTime;
    0,                    //int       cooldownTime;
    0,                    //int       overheatWeaponDelayTime;
    PORTALGUN_REPEAT,     //int       repeatRate1;
    PORTALGUN_REPEAT,     //int       repeatRate2;
    0,                    //int       repeatRate3;
    0,                    //int       burstRounds1;
    0,                    //int       burstRounds2;
    0,                    //int       burstRounds3;
    0,                    //int       burstDelay1;
    0,                    //int       burstDelay2;
    0,                    //int       burstDelay3;
    PORTALGUN_RELOAD,     //int       reloadTime;
    0.0f,                 //float     knockbackScale;
    0.0f,                 //float     recoil1;
    0.0f,                 //float     recoil2;
    0.0f,                 //float     recoil3;
    qtrue,                //qboolean  fully_auto1;
    qtrue,                //qboolean  fully_auto2;
    qtrue,                //qboolean  fully_auto3;
    qtrue,                //qboolean  hasAltMode;
    qfalse,               //qboolean  hasThirdMode;
    qfalse,               //qboolean  canZoom;
    90.0f,                //float     zoomFov;
    qtrue,                //qboolean  purchasable;
    qtrue,                //qboolean  longRanged;
    {
      {                     //impactPrediction_t impactPrediction[0];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      },
      {                     //impactPrediction_t impactPrediction[1];
        WPM_NONE,           //weaponMode_t  weaponMode;
        TR_STATIONARY,      //trType_t      trType;
        0,                  //int       missileLifeTime;
        0,                  //int       missileSize;
        0                   //int       missileLaunchSpeed;
      }
    },
    {                     //missileAttributes_t missile;
      {                   //missileAttributes_t missile[0];
        qtrue,              //qboolean       enabled;
        "portalgun",        //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {-PORTALGUN_SIZE, -PORTALGUN_SIZE, -PORTALGUN_SIZE}, //vec3_t         mins;
        {PORTALGUN_SIZE, PORTALGUN_SIZE, PORTALGUN_SIZE}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        PORTALGUN_SPEED,    //float          speed;
        TR_ACCEL,           //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_RED,         //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_PLAYERSOLID},//content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[1];
        qtrue,              //qboolean       enabled;
        "portalgun",        //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qfalse,             //qboolean       point_against_world;
        {-PORTALGUN_SIZE, -PORTALGUN_SIZE, -PORTALGUN_SIZE}, //vec3_t         mins;
        {PORTALGUN_SIZE, PORTALGUN_SIZE, PORTALGUN_SIZE}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        PORTALGUN_SPEED,    //float          speed;
        TR_ACCEL,           //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_BLUE,        //portal_t       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_PLAYERSOLID},//content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      },
      {                   //missileAttributes_t missile[2];
        qfalse,             //qboolean       enabled;
        "",                 //char           *class_name;
        MOD_UNKNOWN,        //meansOfDeath_t mod;
        MOD_UNKNOWN,        //meansOfDeath_t splash_mod;
        0,                  //int            damage;
        0,                  //int            splash_damage;
        0.0f,               //float          splash_radius;
        qtrue,              //qboolean       point_against_world;
        {0.0f, 0.0f, 0.0f}, //vec3_t         mins;
        {0.0f, 0.0f, 0.0f}, //vec3_t         maxs;
        {0.0f, 0.0f, 0.0f}, //vec3_t         muzzle_offset;
        0.0f,               //float          speed;
        TR_LINEAR,          //trType_t       trajectory_type;
        0,                  //int            activation_delay;
        0,                  //int            explode_delay;
        qtrue,              //qboolean       explode_miss_effect;
        qfalse,             //qboolean       team_only_trail;
        BOUNCE_NONE,        //bounce_t       bounce_type;
        qtrue,              //qboolean       damage_on_bounce;
        qtrue,              //qboolean       bounce_sound;
        qfalse,             //qboolean       impact_stick;
        qfalse,             //qboolean       impact_miss_effect;
        PORTAL_NONE,        //qboolean       impact_create_portal;
        0,                  //int            health;
        0.0f,               //float          kick_up_speed;
        0,                  //int            kick_up_time;
        0.0f,               //float          tripwire_range;
        0,                  //int            tripwire_check_frequency;
        0,                  //int            search_and_destroy_change_period;
        qfalse,             //qboolean       return_after_damage;
        qtrue,              //qboolean       has_knockback;
        IMPEDE_MOVE_NONE,   //impede_move_t  impede_move;
        0,                  //int            charged_time_max;
        qfalse,             //qboolean       charged_damage;
        qfalse,             //qboolean       charged_speed;
        0,                  //int            charged_speed_min;
        0,                  //int            charged_speed_min_damage_mod;
        qfalse,             //qboolean       save_missiles;
        qfalse,             //qboolean       detonate_saved_missiles;
        100,                //int            detonate_saved_missiles_delay;
        1000,               //int            detonate_saved_missiles_repeat;
        qfalse,             //qboolean       relative_missile_speed;
        qtrue,              //qboolean       relative_missile_speed_along_aim;
        1.0f,               //float          relative_missile_speed_lag;
        {0, MASK_SHOT},     //content_mask_t clip_mask;
        qfalse,             //qboolean       scale;
        0,                  //int            scale_start_time;
        0,                  //int            scale_stop_time;
        {0.0f, 0.0f, 0.0f}, //vec3_t         scale_stop_mins;
        {0.0f, 0.0f, 0.0f}  //vec3_t         scale_stop_maxs;
      }
    },
    {                     //struct    splatter
      {                   //struct    splatter[0]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[1]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      },
      {                   //struct    splatter[2]
        qfalse,           //qboolean               predicted;
        0,                //unsigned int number;
        SPLATP_SPHERICAL_CONE, //splatterPattern_t pattern;
        SPLATD_RANDOM,    //splatterDistribution_t distribution;
        0,                //unsigned int           pitchLayers;
        0,                //float        spread;
        0,                //int          impactDamage;
        0.0f,             //float        impactDamageFalloff;
        0,                //int          impactDamageCap;
        0.0f,             //float        range;
      }
    },
    TEAM_HUMANS           //team_t    team;
  }
};

size_t bg_numWeapons = ARRAY_LEN( bg_weapons );

static const weaponAttributes_t nullWeapon = { 0 };
static const missileAttributes_t nullMissile = { 0 };

/*
==============
BG_WeaponByName
==============
*/
const weaponAttributes_t *BG_WeaponByName( const char *name )
{
  int i;

  for( i = 0; i < bg_numWeapons; i++ )
  {
    if( !Q_stricmp( bg_weapons[ i ].name, name ) )
    {
      return &bg_weapons[ i ];
    }
  }

  return &nullWeapon;
}

/*
==============
BG_Weapon
==============
*/
const weaponAttributes_t *BG_Weapon( weapon_t weapon )
{
  return ( weapon > WP_NONE && weapon < WP_NUM_WEAPONS ) ?
    &bg_weapons[ weapon - 1 ] : &nullWeapon;
}

/*
==============
BG_Missile
==============
*/
const missileAttributes_t *BG_Missile(weapon_t weapon, weaponMode_t mode) {
  switch (mode) {
    case WPM_PRIMARY:
      return &BG_Weapon(weapon)->missile[0];
      break;

    case WPM_SECONDARY:
      return &BG_Weapon(weapon)->missile[1];
      break;

    case WPM_TERTIARY:
      return &BG_Weapon(weapon)->missile[2];
      break;

    default:
      return &nullMissile;
  }
}

/*
==============
BG_WeaponAllowedInStage
==============
*/
qboolean BG_WeaponAllowedInStage( weapon_t weapon, stage_t stage,
                                  int gameIsInWarmup )
{
  return qtrue;
}

/*
==============
BG_AmmoUsage
==============
*/
int BG_AmmoUsage(playerState_t *ps) {
  int ammo_usage;

  switch(ps->generic1) {
    case WPM_PRIMARY:
      ammo_usage = BG_Weapon(ps->weapon)->ammoUsage1;
      break;

    case WPM_SECONDARY:
      ammo_usage = BG_Weapon(ps->weapon)->ammoUsage2;
      break;

    case WPM_TERTIARY:
      ammo_usage = BG_Weapon(ps->weapon)->ammoUsage3;
      break;

    default:
      ammo_usage = 1;
      break;
  }

  if(
    (ammo_usage > 1) &&
    (ammo_usage > ps->ammo) &&
    BG_Weapon(ps->weapon)->allowPartialAmmoUsage) {
    ammo_usage = ps->ammo;
  }

  return ammo_usage;
}

/*
==============
BG_HasIncreasedAmmoCapacity
==============
*/
qboolean BG_HasIncreasedAmmoCapacity(int stats[ ], weapon_t weapon) {
  if(BG_Weapon(weapon)->usesEnergy &&
    (
      BG_InventoryContainsUpgrade(UP_BATTPACK, stats) ||
      BG_InventoryContainsUpgrade(UP_BATTLESUIT, stats))) {
    return qtrue;
  } else {
    return qfalse;
  }
}

/*
==============
BG_GetMaxAmmo
==============
*/
int BG_GetMaxAmmo(int stats[ ], weapon_t weapon) {
  if(BG_HasIncreasedAmmoCapacity(stats, weapon)) {
    return BG_Weapon(weapon)->maxAmmo * BATTPACK_MODIFIER;
  } else {
    return BG_Weapon(weapon)->maxAmmo;
  }
}

/*
==============
BG_HasIncreasedClipCapacity
==============
*/
qboolean BG_HasIncreasedClipCapacity(int stats[ ], weapon_t weapon) {
  if(
    !BG_Weapon(weapon)->usesEnergy &&
    !BG_Weapon(weapon)->infiniteAmmo &&
    BG_Weapon(weapon)->ammoPurchasable &&
    !BG_Weapon(weapon)->roundPrice &&
    BG_InventoryContainsUpgrade(UP_BATTLESUIT, stats)) {
    return qtrue;
  } else {
    return qfalse;
  }
}

/*
==============
BG_GetMaxClips
==============
*/
int BG_GetMaxClips(int stats[ ], weapon_t weapon) {
  if(BG_HasIncreasedClipCapacity(stats, weapon)) {
    return (BG_Weapon(weapon)->maxClips * 2) + 1;
  } else {
    return BG_Weapon(weapon)->maxClips;
  }
}

/*
==============
BG_GetClips

Makes use of a hack to allow in some cases for more than 16 clips restricted by
backwards compatable network code.  Breaking backwards compatable can make this
functions no longer needed.
==============
*/
int *BG_GetClips(playerState_t *ps, weapon_t weapon)
{
  Com_Assert(ps);

  if(BG_Weapon(weapon)->weaponOptionA == WEAPONOPTA_ONE_ROUND_TO_ONE_CLIP) {
    return &ps->misc[MISC_MISC3];
  } else {
    return &ps->clips;
  }
}

/*
==============
BG_ClipUssage
==============
*/
int BG_ClipUssage( playerState_t *ps )
{
  weapon_t weapon;
  int      *clips;

  Com_Assert(ps);

  weapon = ps->weapon;
  clips = BG_GetClips(ps, weapon);

  if(BG_Weapon(weapon)->weaponOptionA == WEAPONOPTA_ONE_ROUND_TO_ONE_CLIP) {
    int max_ammo = BG_GetMaxAmmo(ps->stats, weapon);
    int clips_to_load = max_ammo - ps->ammo;

    if(clips_to_load > *clips) {
      clips_to_load = *clips;
    }

    return clips_to_load;
  } else {
    if(*clips > 0) {
      return 1;
    } else {
      return 0;
    }
  }
}

/*
==============
BG_TotalPriceForWeapon

Returns the base price of a weapon plus the cost of full ammo (if ammo isn't free)
==============
*/
int BG_TotalPriceForWeapon( weapon_t weapon , int gameIsInWarmup )
{
  if( gameIsInWarmup && BG_Weapon( weapon )->warmupFree )
    return 0;

  if( !BG_Weapon( weapon )->roundPrice || BG_Weapon( weapon )->usesEnergy )
    return BG_Weapon( weapon )->price;

  return ( BG_Weapon( weapon )->roundPrice * BG_Weapon( weapon )->maxAmmo *
           ( BG_Weapon( weapon )->maxClips + 1 ) ) + BG_Weapon( weapon )->price;

}


////////////////////////////////////////////////////////////////////////////////

static const upgradeAttributes_t bg_upgrades[ ] =
{
  {
    UP_LIGHTARMOUR,         //int   number;
    qtrue,                  //qboolean enabled;
    LIGHTARMOUR_PRICE,      //int   price;
    qtrue,                  //qboolean warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_TORSO|SLOT_ARMS|SLOT_LEGS, //int   slots;
    "larmour",              //char  *name;
    "Light Armour",         //char  *humanName;
    "icons/iconu_larmour",
    qtrue,                  //qboolean  purchasable;
    qtrue,                  //qboolean  sellable;
    qfalse,                 //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  },
  {
    UP_HELMET,              //int   number;
    qtrue,                  //qboolean enabled;
    HELMET_PRICE,           //int   price;
    qfalse,                 //qboolean warmupFree;
    ( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_HEAD,              //int   slots;
    "helmet",               //char  *name;
    "Helmet",               //char  *humanName;
    "icons/iconu_helmet",
    qtrue,                  //qboolean  purchasable;
    qtrue,                  //qboolean  sellable;
    qfalse,                 //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  },
  {
    UP_MEDKIT,              //int   number;
    qtrue,                  //qboolean enabled;
    MEDKIT_PRICE,           //int   price;
    qtrue,                  //qboolean warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_NONE,              //int   slots;
    "medkit",               //char  *name;
    "Medkit",               //char  *humanName;
    "icons/iconu_atoxin",
    qtrue,                  //qboolean  purchasable;
    qfalse,                 //qboolean  sellable;
    qtrue,                  //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  },
  {
    UP_BIOPACK,              //int   upgradeNum;
    qtrue,                  //qboolean enabled;
    BIOPACK_PRICE,           //int   price;
    qfalse,                 //qboolean warmupFree;
    ( 1 << S2 )|( 1 << S2 )|( 1 << S3 ),//int  stages
    SLOT_BACKPACK,          //int   slots;
    "biopack",               //char  *upgradeName;
    "Biopack",               //char  *humanName;
    "icons/iconu_biopack",
    qtrue,                  //qboolean purchasable
    qtrue,                  //qboolean  sellable;
    qfalse,                 //qboolean usable
    TEAM_HUMANS             //team_t  team;
  },
  {
    UP_BATTPACK,            //int   number;
    qtrue,                  //qboolean enabled;
    BATTPACK_PRICE,         //int   price;
    qtrue,                  //qboolean warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_BACKPACK,          //int   slots;
    "battpack",             //char  *name;
    "Battery Pack",         //char  *humanName;
    "icons/iconu_battpack",
    qtrue,                  //qboolean  purchasable;
    qtrue,                  //qboolean  sellable;
    qfalse,                 //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  },
  {
    UP_JETPACK,             //int   number;
    qtrue,                  //qboolean enabled;
    JETPACK_PRICE,          //int   price;
    qfalse,                 //qboolean warmupFree;
    ( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_BACKPACK,          //int   slots;
    "jetpack",              //char  *name;
    "Jet Pack",             //char  *humanName;
    "icons/iconu_jetpack",
    qtrue,                  //qboolean  purchasable;
    qtrue,                  //qboolean  sellable;
    qfalse,                  //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  },
  {
    UP_BATTLESUIT,          //int   number;
    qtrue,                  //qboolean enabled;
    BSUIT_PRICE,            //int   price;
    qfalse,                 //qboolean warmupFree;
    ( 1 << S3 ),            //int   stages;
    SLOT_BACKPACK, //int  slots;
    "bsuit",                //char  *name;
    "Battlesuit",           //char  *humanName;
    "icons/iconu_bsuit",
    qtrue,                  //qboolean  purchasable;
    qtrue,                  //qboolean  sellable;
    qfalse,                 //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  },
  {
    UP_GRENADE,             //int   number;
    qtrue,                  //qboolean enabled;
    GRENADE_PRICE,          //int   price;
    qfalse,                 //qboolean warmupFree;
    ( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_NONE,              //int   slots;
    "gren",                 //char  *name;
    "Grenade",              //char  *humanName;
    0,
    qtrue,                  //qboolean  purchasable;
    qtrue,                  //qboolean  sellable;
    qtrue,                  //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  },
  {
    UP_FRAGNADE,            //int   number;
    qtrue,                  //qboolean enabled;
    FRAGNADE_PRICE,         //int   price;
    qfalse,                 //qboolean warmupFree;
    ( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_NONE,              //int   slots;
    "frag",                 //char  *name;
    "Fragmentation Grenade", //char  *humanName;
    0,
    qtrue,                  //qboolean  purchasable;
    qtrue,                  //qboolean  sellable;
    qtrue,                  //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  },
  {
    UP_LASERMINE,           //int   number;
    qtrue,                  //qboolean enabled;
    LASERMINE_PRICE,        //int   price;
    qfalse,                 //qboolean warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_NONE,              //int   slots;
    "lasmine",              //char  *name;
    "Laser Mine",           //char  *humanName;
    0,
    qtrue,                  //qboolean  purchasable;
    qtrue,                  //qboolean  sellable;
    qtrue,                  //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  },
  {
    UP_AMMO,                //int   number;
    qtrue,                  //qboolean enabled;
    0,                      //int   price;
    qfalse,                 //qboolean warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_NONE,              //int   slots;
    "ammo",                 //char  *name;
    "Ammunition",           //char  *humanName;
    0,
    qtrue,                  //qboolean  purchasable;
    qfalse,                 //qboolean  sellable;
    qfalse,                 //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  },
  {
    UP_JETFUEL,             //int   number;
    qtrue,                  //qboolean enabled;
    0,                      //int   price;
    qfalse,                 //qboolean warmupFree;
    ( 1 << S1 )|( 1 << S2 )|( 1 << S3 ), //int  stages;
    SLOT_NONE,              //int   slots;
    "jetfuel",              //char  *name;
    "Jet Pack Fuel",        //char  *humanName;
    0,
    qtrue,                  //qboolean  purchasable;
    qfalse,                 //qboolean  sellable;
    qfalse,                 //qboolean  usable;
    TEAM_HUMANS             //team_t    team;
  }
};

size_t bg_numUpgrades = ARRAY_LEN( bg_upgrades );

static const upgradeAttributes_t nullUpgrade = { 0 };

/*
==============
BG_UpgradeByName
==============
*/
const upgradeAttributes_t *BG_UpgradeByName( const char *name )
{
  int i;

  for( i = 0; i < bg_numUpgrades; i++ )
  {
    if( !Q_stricmp( bg_upgrades[ i ].name, name ) )
    {
      return &bg_upgrades[ i ];
    }
  }

  return &nullUpgrade;
}

/*
==============
BG_Upgrade
==============
*/
const upgradeAttributes_t *BG_Upgrade( upgrade_t upgrade )
{
  return ( upgrade > UP_NONE && upgrade < UP_NUM_UPGRADES ) ?
    &bg_upgrades[ upgrade - 1 ] : &nullUpgrade;
}

/*
==============
BG_UpgradeAllowedInStage
==============
*/
qboolean BG_UpgradeAllowedInStage( upgrade_t upgrade, stage_t stage,
                                   int gameIsInWarmup )
{
  return qtrue;
}

/*
=============================
BG_ModifyMissleLaunchVelocity
=============================
*/
void BG_ModifyMissleLaunchVelocity(
  vec3_t pVelocity, int speed, vec3_t mVelocity, weapon_t weapon,
  weaponMode_t mode) {
  vec3_t mDir, new_pVel;
  int    scale;

  VectorScale(
    pVelocity, BG_Missile(weapon, mode)->relative_missile_speed_lag, new_pVel);

  // FIXME add the velocity from a mover moving the firing player to the shot
  if(BG_Missile(weapon, mode)->relative_missile_speed) {
    if(
      (VectorLength( new_pVel ) > 2 * speed) ||
      !BG_Missile(weapon, mode)->relative_missile_speed_along_aim) {
      VectorAdd( mVelocity, new_pVel, mVelocity );
    } else {
      VectorNormalize2( mVelocity, mDir );
      scale = DotProduct( new_pVel , mDir );
      VectorMA( mVelocity, scale, mDir, mVelocity );
    }
  }

  return;
}

////////////////////////////////////////////////////////////////////////////////

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result )
{
  float   deltaTime;
  float   phase;
  vec3_t  dir;

  switch( tr->trType )
  {
    case TR_STATIONARY:
    case TR_INTERPOLATE:
      VectorCopy( tr->trBase, result );
      break;

    case TR_LINEAR:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
      break;

    case TR_SINE:
      deltaTime = ( atTime - tr->trTime ) / (float)tr->trDuration;
      phase = sin( deltaTime * M_PI * 2 );
      VectorMA( tr->trBase, phase, tr->trDelta, result );
      break;

    case TR_LINEAR_STOP:
      if( atTime > tr->trTime + tr->trDuration )
        atTime = tr->trTime + tr->trDuration;

      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      if( deltaTime < 0 )
        deltaTime = 0;

      VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
      break;

    case TR_GRAVITY:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
      result[ 2 ] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;   // FIXME: local gravity...
      break;

    case TR_BUOYANCY:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
      result[ 2 ] += 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;   // FIXME: local gravity...
      break;

    case TR_ACCEL:
      // time since missile fired in seconds
      deltaTime = ( atTime - tr->trTime ) * 0.001;

      // the .5*a*t^2 part. trDuration = acceleration,
      // phase gives the magnitude of the distance
      // we need to move
      phase = (tr->trDuration / 2) * (deltaTime * deltaTime);

      // Make dir equal to the velocity of the object
      VectorCopy (tr->trDelta, dir);

      // Sets the magnitude of vector dir to 1
      VectorNormalize (dir);

      // Move a distance "phase" in the direction "dir"
      // from our starting point
      VectorMA (tr->trBase, phase, dir, result);

      // The u*t part. Adds the velocity of the object
      // multiplied by the time to the last result.
      VectorMA (result, deltaTime, tr->trDelta, result);
      break;

    case TR_HALF_GRAVITY:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
      result[ 2 ] -= 0.5 * DEFAULT_GRAVITY * 0.5 * deltaTime * deltaTime;   // FIXME: local gravity...
      break;

    default:
      Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
      break;
  }
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result )
{
  float deltaTime;
  float phase;
  vec3_t dir;

  switch( tr->trType )
  {
    case TR_STATIONARY:
    case TR_INTERPOLATE:
      VectorClear( result );
      break;

    case TR_LINEAR:
      VectorCopy( tr->trDelta, result );
      break;

    case TR_SINE:
      deltaTime = ( atTime - tr->trTime ) / (float)tr->trDuration;
      phase = cos( deltaTime * M_PI * 2 );  // derivative of sin = cos
      phase *= 2 * M_PI * 1000 / (float)tr->trDuration;
      VectorScale( tr->trDelta, phase, result );
      break;

    case TR_LINEAR_STOP:
      if( atTime > tr->trTime + tr->trDuration || atTime < tr->trTime )
      {
        VectorClear( result );
        return;
      }
      VectorCopy( tr->trDelta, result );
      break;

    case TR_GRAVITY:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorCopy( tr->trDelta, result );
      result[ 2 ] -= DEFAULT_GRAVITY * deltaTime;   // FIXME: local gravity...
      break;

    case TR_BUOYANCY:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorCopy( tr->trDelta, result );
      result[ 2 ] += DEFAULT_GRAVITY * deltaTime;   // FIXME: local gravity...
      break;

    case TR_ACCEL:
      // time since missile fired in seconds
      deltaTime = ( atTime - tr->trTime ) * 0.001;

      // Turn magnitude of acceleration into a vector
      VectorCopy(tr->trDelta,dir);
      VectorNormalize (dir);
      VectorScale (dir, tr->trDuration, dir);

      // u + t * a = v
      VectorMA (tr->trDelta, deltaTime, dir, result);
      break;

    case TR_HALF_GRAVITY:
      deltaTime = ( atTime - tr->trTime ) * 0.001;  // milliseconds to seconds
      VectorCopy( tr->trDelta, result );
      result[ 2 ] -= DEFAULT_GRAVITY * 0.5 * deltaTime;   // FIXME: local gravity...
      break;

    default:
      Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
      break;
  }
}

char *eventnames[ ] =
{
  "EV_NONE",

  "EV_FOOTSTEP",
  "EV_FOOTSTEP_METAL",
  "EV_FOOTSTEP_SQUELCH",
  "EV_FOOTSPLASH",
  "EV_FOOTWADE",
  "EV_SWIM",

  "EV_STEP_4",
  "EV_STEP_8",
  "EV_STEP_12",
  "EV_STEP_16",

  "EV_STEPDN_4",
  "EV_STEPDN_8",
  "EV_STEPDN_12",
  "EV_STEPDN_16",

  "EV_FALL_SHORT",
  "EV_FALL_MEDIUM",
  "EV_FALL_FAR",
  "EV_FALLING",

  "EV_JUMP",
  "EV_JETJUMP",
  "EV_AIRPOUNCE",
  "EV_WATER_TOUCH", // foot touches
  "EV_WATER_LEAVE", // foot leaves
  "EV_WATER_UNDER", // head touches
  "EV_WATER_CLEAR", // head leaves

  "EV_NOAMMO",
  "EV_CHANGE_WEAPON",
  "EV_FIRE_WEAPON",
  "EV_FIRE_WEAPON2",
  "EV_FIRE_WEAPON3",

  "EV_PLAYER_RESPAWN", // for fovwarp effects
  "EV_PLAYER_TELEPORT_IN",
  "EV_PLAYER_TELEPORT_OUT",

  "EV_MISSILE_BOUNCE",    // eventParm will be the soundindex

  "EV_TRIPWIRE_ARMED",

  "EV_GENERAL_SOUND",
  "EV_GLOBAL_SOUND",    // no attenuation

  "EV_BULLET_HIT_FLESH",
  "EV_BULLET_HIT_WALL",

  "EV_SPLATTER",
  "EV_MASS_DRIVER",

  "EV_MISSILE_HIT",
  "EV_MISSILE_MISS",
  "EV_MISSILE_MISS_METAL",
  "EV_TESLATRAIL",
  "EV_SLIMERTRAIL",
  "EV_BULLET",        // otherEntity is the shooter

  "EV_LEV1_GRAB",
  "EV_LEV4_TRAMPLE_PREPARE",
  "EV_LEV4_TRAMPLE_START",

  "EV_PAIN",
  "EV_DEATH1",
  "EV_DEATH2",
  "EV_DEATH3",
  "EV_OBITUARY",

  "EV_GIB_PLAYER",
  "EV_GIB_BSUIT",
  "EV_GIB_SPITFIRE_WINGS",
  "EV_EXPLODE_MARAUDER",

  "EV_BUILD_FIRE",
  "EV_BUILD_CONSTRUCT",
  "EV_BUILD_DESTROY",
  "EV_BUILD_DELAY",     // can't build yet
  "EV_BUILD_REPAIR",    // repairing buildable
  "EV_BUILD_REPAIRED",  // buildable has full health
  "EV_HUMAN_BUILDABLE_EXPLOSION",
  "EV_ALIEN_BUILDABLE_EXPLOSION",
  "EV_ALIEN_ACIDTUBE",
  "EV_ALIEN_SLIMER",

  "EV_MEDKIT_USED",

  "EV_ALIEN_SPAWN_PROTECTION_ENDED",
  "EV_ALIEN_EVOLVE",
  "EV_ALIEN_EVOLVE_FAILED",

  "EV_DEBUG_LINE",
  "EV_STOPLOOPINGSOUND",
  "EV_TAUNT",
  
  "EV_OVERMIND_ATTACK", // overmind under attack
  "EV_OVERMIND_DYING",  // overmind close to death
  "EV_OVERMIND_SPAWNS", // overmind needs spawns

  "EV_DCC_ATTACK",      // dcc under attack

  "EV_MGTURRET_SPINUP", // trigger a sound

  "EV_RPTUSE_SOUND",    // trigger a sound
  "EV_LEV2_ZAP",

  "EV_JETPACK_DEACTIVATE",
  "EV_JETPACK_REFUEL",

  "EV_FIGHT"
};

/*
===============
BG_EventName
===============
*/
const char *BG_EventName( int num )
{
  if( num < 0 || num >= ARRAY_LEN( eventnames ) )
    return "UNKNOWN";

  return eventnames[ num ];
}

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void  Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps )
{
#ifdef DEBUG_EVENTS
  {
    char buf[ 256 ];
    Cvar_VariableStringBuffer( "showevents", buf, sizeof( buf ) );

    if( atof( buf ) != 0 )
    {
#ifdef GAME
      Com_Printf( " game event svt %5d -> %5d: num = %20s parm %d\n",
                  ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence,
                  BG_EventName( newEvent ), eventParm );
#else
      Com_Printf( "Cgame event svt %5d -> %5d: num = %20s parm %d\n",
                  ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence,
                  BG_EventName( newEvent ), eventParm );
#endif
    }
  }
#endif
  ps->events[ ps->eventSequence & ( MAX_PS_EVENTS - 1 ) ] = newEvent;
  ps->eventParms[ ps->eventSequence & ( MAX_PS_EVENTS - 1 ) ] = eventParm;
  ps->eventSequence++;
}


/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s,
                                  pmoveExt_t *pmext )
{
  int     i;
  float   *id_pointer;
  float   *contents_pointer;

  if( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_type == PM_FREEZE )
    s->eType = ET_INVISIBLE;
  else if( ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
    s->eType = ET_INVISIBLE;
  else
    s->eType = ET_PLAYER;

  s->number = ps->clientNum;

  s->pos.trType = TR_INTERPOLATE;
  VectorCopy( ps->origin, s->pos.trBase );

  //set the trDelta for flag direction
  VectorCopy( ps->velocity, s->pos.trDelta );

  s->apos.trType = TR_INTERPOLATE;
  VectorCopy( ps->viewangles, s->apos.trBase );

  s->time2 = ps->movementDir;
  s->legsAnim = ps->legsAnim;
  s->torsoAnim = ps->torsoAnim;
  s->weaponAnim = ps->weaponAnim;
  s->clientNum = ps->clientNum;   // ET_PLAYER looks here instead of at number
                    // so corpses can also reference the proper config
  s->eFlags = ps->eFlags;
  if( ps->misc[ MISC_HEALTH ] <= 0 )
    s->eFlags |= EF_DEAD;
  else
    s->eFlags &= ~EF_DEAD;

  if( ps->stats[ STAT_STATE ] & SS_BLOBLOCKED )
    s->eFlags |= EF_BLOBLOCKED;
  else
    s->eFlags &= ~EF_BLOBLOCKED;

  if( ps->externalEvent )
  {
    s->event = ps->externalEvent;
    s->eventParm = ps->externalEventParm;
  }
  else if( ps->entityEventSequence < ps->eventSequence )
  {
    int   seq;

    if( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS )
      ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;

    seq = ps->entityEventSequence & ( MAX_PS_EVENTS - 1 );
    s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
    s->eventParm = ps->eventParms[ seq ];
    ps->entityEventSequence++;
  }

  s->weapon = ps->weapon;
  s->groundEntityNum = ps->groundEntityNum;

  //store items held and active items in modelindex and modelindex2
  s->modelindex = 0;
  s->modelindex2 = 0;
  for( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
  {
    if( BG_InventoryContainsUpgrade( i, ps->stats ) )
    {
      s->modelindex |= 1 << i;

      if( BG_UpgradeIsActive( i, ps->stats ) )
        s->modelindex2 |= 1 << i;
    }
  }

  // use misc field to store team/class info:
  s->misc = ps->stats[ STAT_TEAM ] | ( ps->stats[ STAT_CLASS ] << 8 );

  // have to get the surfNormal through somehow...
  VectorCopy( ps->grapplePoint, s->angles2 );

  s->loopSound = ps->loopSound;
  s->generic1 = ps->generic1;

  if( s->generic1 <= WPM_NONE || s->generic1 >= WPM_NUM_WEAPONMODES )
    s->generic1 = WPM_PRIMARY;

  s->otherEntityNum = ps->otherEntityNum;

  s->otherEntityNum2 = ps->stats[ STAT_FLAGS ];

  if( ps->weapon == WP_ALEVEL2 )
    s->constantLight = ps->misc[ MISC_MISC ];
  else
    s->constantLight = ps->stats[ STAT_MISC2 ];

  id_pointer = (float *)(&ps->misc[MISC_ID]);
  contents_pointer = (float *)(&ps->misc[MISC_CONTENTS]);
  s->origin[0] = *id_pointer;
  s->origin[1] = *contents_pointer;

  // for predicted splatter patterns
  s->origin[2] = *((float *)&pmext->ammo_used);
  VectorCopy(pmext->dir_fired, s->angles);
  VectorCopy(pmext->muzzel_point_fired, s->origin2);
}


/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s,
                                             int time, pmoveExt_t *pmext )
{
  int     i;
  float   *id_pointer;
  float   *contents_pointer;

  if( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_type == PM_FREEZE )
    s->eType = ET_INVISIBLE;
  else if( ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
    s->eType = ET_INVISIBLE;
  else
    s->eType = ET_PLAYER;

  s->number = ps->clientNum;

  s->pos.trType = TR_LINEAR_STOP;
  VectorCopy( ps->origin, s->pos.trBase );

  // set the trDelta for flag direction and linear prediction
  VectorCopy( ps->velocity, s->pos.trDelta );
  // set the time for linear prediction
  s->pos.trTime = time;
  // set maximum extra polation time
  s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

  s->apos.trType = TR_INTERPOLATE;
  VectorCopy( ps->viewangles, s->apos.trBase );

  s->time2 = ps->movementDir;
  s->legsAnim = ps->legsAnim;
  s->torsoAnim = ps->torsoAnim;
  s->weaponAnim = ps->weaponAnim;
  s->clientNum = ps->clientNum;   // ET_PLAYER looks here instead of at number
                    // so corpses can also reference the proper config
  s->eFlags = ps->eFlags;

  if( ps->misc[ MISC_HEALTH ] <= 0 )
    s->eFlags |= EF_DEAD;
  else
    s->eFlags &= ~EF_DEAD;

  if( ps->stats[ STAT_STATE ] & SS_BLOBLOCKED )
    s->eFlags |= EF_BLOBLOCKED;
  else
    s->eFlags &= ~EF_BLOBLOCKED;

  if( ps->externalEvent )
  {
    s->event = ps->externalEvent;
    s->eventParm = ps->externalEventParm;
  }
  else if( ps->entityEventSequence < ps->eventSequence )
  {
    int   seq;

    if( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS )
      ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;

    seq = ps->entityEventSequence & ( MAX_PS_EVENTS - 1 );
    s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
    s->eventParm = ps->eventParms[ seq ];
    ps->entityEventSequence++;
  }

  s->weapon = ps->weapon;
  s->groundEntityNum = ps->groundEntityNum;

  //store items held and active items in modelindex and modelindex2
  s->modelindex = 0;
  s->modelindex2 = 0;

  for( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
  {
    if( BG_InventoryContainsUpgrade( i, ps->stats ) )
    {
      s->modelindex |= 1 << i;

      if( BG_UpgradeIsActive( i, ps->stats ) )
        s->modelindex2 |= 1 << i;
    }
  }

  // use misc field to store team/class info:
  s->misc = ps->stats[ STAT_TEAM ] | ( ps->stats[ STAT_CLASS ] << 8 );

  // have to get the surfNormal through somehow...
  VectorCopy( ps->grapplePoint, s->angles2 );

  s->loopSound = ps->loopSound;
  s->generic1 = ps->generic1;

  if( s->generic1 <= WPM_NONE || s->generic1 >= WPM_NUM_WEAPONMODES )
    s->generic1 = WPM_PRIMARY;

  s->otherEntityNum = ps->otherEntityNum;

  s->otherEntityNum2 = ps->stats[ STAT_FLAGS ];

  if( ps->weapon == WP_ALEVEL2 )
    s->constantLight = ps->misc[ MISC_MISC ];
  else
    s->constantLight = ps->stats[ STAT_MISC2 ];

  id_pointer = (float *)(&ps->misc[MISC_ID]);
  contents_pointer = (float *)(&ps->misc[MISC_CONTENTS]);
  s->origin[0] = *id_pointer;
  s->origin[1] = *contents_pointer;

  // for predicted splatter patterns
  s->origin[2] = *((float *)&pmext->ammo_used);
  VectorCopy(pmext->dir_fired, s->angles);
  VectorCopy(pmext->muzzel_point_fired, s->origin2);
}

/*
========================
BG_WeaponIsFull

Check if a weapon has full ammo
========================
*/
qboolean BG_WeaponIsFull( weapon_t weapon, int stats[ ], int ammo, int clips )
{
  int maxAmmo, maxClips;

  maxAmmo = BG_GetMaxAmmo(stats, weapon);
  maxClips = BG_GetMaxClips(stats, weapon);

  return (maxClips < clips) || (( maxAmmo == ammo ) && ( maxClips == clips ));
}

/*
========================
BG_InventoryContainsWeapon

Does the player hold a weapon?
========================
*/
qboolean BG_InventoryContainsWeapon(int weapon, int stats[ ]) {
  // humans always have a blaster and a ckit
  if(
    stats[STAT_TEAM] == TEAM_HUMANS &&
    (
      weapon == WP_BLASTER ||
      (weapon == WP_HBUILD && stats[STAT_WEAPON] != WP_HBUILD2))) {
    return qtrue;
  }

  return (stats[STAT_WEAPON] == weapon);
}

/*
========================
BG_SlotsForInventory

Calculate the slots used by an inventory and warn of conflicts
========================
*/
int BG_SlotsForInventory( int stats[ ] )
{
  int i, slot, slots;

  slots = BG_Weapon( stats[ STAT_WEAPON ] )->slots;
  if( stats[ STAT_TEAM ] == TEAM_HUMANS )
  {
    slots |= BG_Weapon( WP_BLASTER )->slots;
    if(stats[STAT_WEAPON] != WP_HBUILD2) {
      slots |= BG_Weapon( WP_HBUILD )->slots;
    }
  }

  for( i = UP_NONE; i < UP_NUM_UPGRADES; i++ )
  {
    if( BG_InventoryContainsUpgrade( i, stats ) )
    {
      slot = BG_Upgrade( i )->slots;

      // this check should never be true
      if( slots & slot )
      {
        Com_Printf( S_COLOR_YELLOW "WARNING: held item %d conflicts with "
                    "inventory slot %d\n", i, slot );
      }

      slots |= slot;
    }
  }

  return slots;
}

/*
========================
BG_AddUpgradeToInventory

Give the player an upgrade
========================
*/
void BG_AddUpgradeToInventory( int item, int stats[ ] )
{
  stats[ STAT_ITEMS ] |= ( 1 << item );
}

/*
========================
BG_RemoveUpgradeFromInventory

Take an upgrade from the player
========================
*/
void BG_RemoveUpgradeFromInventory( int item, int stats[ ] )
{
  stats[ STAT_ITEMS ] &= ~( 1 << item );
}

/*
========================
BG_InventoryContainsUpgrade

Does the player hold an upgrade?
========================
*/
qboolean BG_InventoryContainsUpgrade( int item, int stats[ ] )
{
  return( stats[ STAT_ITEMS ] & ( 1 << item ) );
}

/*
========================
BG_ActivateUpgrade

Activates an upgrade
========================
*/
void BG_ActivateUpgrade( int item, int stats[ ] )
{
  if( item != UP_JETPACK || stats[ STAT_FUEL ] >  0)
    stats[ STAT_ACTIVEITEMS ] |= ( 1 << item );
}

/*
========================
BG_DeactivateUpgrade

Deactivates an upgrade
========================
*/
void BG_DeactivateUpgrade( int item, int stats[ ] )
{
  stats[ STAT_ACTIVEITEMS ] &= ~( 1 << item );
}

/*
========================
BG_UpgradeIsActive

Is this upgrade active?
========================
*/
qboolean BG_UpgradeIsActive( int item, int stats[ ] )
{
  return( stats[ STAT_ACTIVEITEMS ] & ( 1 << item ) );
}

/*
===============
BG_RotateAxis

Shared axis rotation function
===============
*/
qboolean BG_RotateAxis( vec3_t surfNormal, vec3_t inAxis[ 3 ],
                        vec3_t outAxis[ 3 ], qboolean inverse, qboolean ceiling )
{
  vec3_t  refNormal = { 0.0f, 0.0f, 1.0f };
  vec3_t  ceilingNormal = { 0.0f, 0.0f, -1.0f };
  vec3_t  localNormal, xNormal;
  float   rotAngle;

  //the grapplePoint being a surfNormal rotation Normal hack... see above :)
  if( ceiling )
  {
    VectorCopy( ceilingNormal, localNormal );
    VectorCopy( surfNormal, xNormal );
  }
  else
  {
    //cross the reference normal and the surface normal to get the rotation axis
    VectorCopy( surfNormal, localNormal );
    CrossProduct( localNormal, refNormal, xNormal );
    VectorNormalize( xNormal );
  }

  //can't rotate with no rotation vector
  if( VectorLength( xNormal ) != 0.0f )
  {
    rotAngle = RAD2DEG( acos( DotProduct( localNormal, refNormal ) ) );

    if( inverse )
      rotAngle = -rotAngle;

    AngleNormalize180( rotAngle );

    //hmmm could get away with only one rotation and some clever stuff later... but i'm lazy
    RotatePointAroundVector( outAxis[ 0 ], xNormal, inAxis[ 0 ], -rotAngle );
    RotatePointAroundVector( outAxis[ 1 ], xNormal, inAxis[ 1 ], -rotAngle );
    RotatePointAroundVector( outAxis[ 2 ], xNormal, inAxis[ 2 ], -rotAngle );
  }
  else
    return qfalse;

  return qtrue;
}

/*
===============
BG_GetClientNormal

Get the normal for the surface the client is walking on
===============
*/
void BG_GetClientNormal( const playerState_t *ps, vec3_t normal )
{
  if( ps->stats[ STAT_STATE ] & SS_WALLCLIMBING )
  {
    if( ps->eFlags & EF_WALLCLIMBCEILING )
      VectorSet( normal, 0.0f, 0.0f, -1.0f );
    else
      VectorCopy( ps->grapplePoint, normal );
  }
  else
    VectorSet( normal, 0.0f, 0.0f, 1.0f );
}

/*
===============
BG_GetClientViewOrigin

Get the position of the client's eye, based on the client's position, the surface's normal, and client's view height
===============
*/
void BG_GetClientViewOrigin( const playerState_t *ps, vec3_t viewOrigin )
{
  vec3_t normal;
  BG_GetClientNormal( ps, normal );
  VectorMA( ps->origin, ps->viewheight, normal, viewOrigin );
}

/*
===============
BG_CalcMuzzlePointFromPS

set muzzle location relative to pivoting eye
===============
*/
void BG_CalcMuzzlePointFromPS( const playerState_t *ps, vec3_t forward,
                               vec3_t right, vec3_t up, vec3_t muzzlePoint )
{
  vec3_t normal, view_angles;
  float  xyspeed;
  float  bobfracsin, bob, bob2;
  int    bobcycle;

  VectorCopy(ps->viewangles, view_angles);

  // initialize bobbing values;
  bobcycle = (ps->misc[MISC_BOB_CYCLE] & 2048) >> 11;
  bobfracsin = fabs(sin((ps->misc[MISC_BOB_CYCLE] & 2047) / 2047.0 * M_PI));
  xyspeed =
    sqrt(ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1]);
  // the bob velocity should't get too fast to avoid jerking
  if(xyspeed > 300.0f) {
    xyspeed = 300.0f;
  }
  // bob amount is class dependant
  if( ps->persistant[PERS_SPECSTATE] != SPECTATOR_NOT) {
    bob2 = 0.0f;
  } else {
    bob2 = BG_Class(ps->stats[ STAT_CLASS ])->bob;
  }

  // adjust the angles for bobbing
  if(bob2 != 0.0f) {
    float speed;
    float delta;

    // make sure the bob is visible even at low speeds
    speed = xyspeed > 200 ? xyspeed : 200;

    delta = bobfracsin * bob2 * speed;
    if(ps->pm_flags & PMF_DUCKED) {
      delta *= 3;   // crouching
    }

    view_angles[ PITCH ] += delta;
    delta = bobfracsin * bob2 * speed;
    if(ps->pm_flags & PMF_DUCKED) {
      delta *= 3;   // crouching accentuates roll
    }

    if(bobcycle & 15) {
      delta = -delta;
    }

    view_angles[ ROLL ] += delta;
  }

  AngleVectors(view_angles, forward, right, up);
  VectorCopy( ps->origin, muzzlePoint );
  BG_GetClientNormal( ps, normal );
  VectorMA( muzzlePoint, ps->viewheight, normal, muzzlePoint );
  VectorMA( muzzlePoint, 1, forward, muzzlePoint );

  // add bob height
  bob = bobfracsin * xyspeed * bob2;

  if( bob > 6 )
    bob = 6;

  VectorMA( muzzlePoint, bob, normal, muzzlePoint );

  // snap to integer coordinates for more efficient network bandwidth usage
  SnapVector( muzzlePoint );
}

/*
===============
BG_GetValueOfPlayer

Returns the credit value of a player
===============
*/
int BG_GetValueOfPlayer( playerState_t *ps )
{
  int worth = 0;

  worth = BG_Class( ps->stats[ STAT_CLASS ] )->value;

  // Humans have worth from their equipment as well
  if( ps->stats[ STAT_TEAM ] == TEAM_HUMANS )
  {
    upgrade_t i;
    for( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
    {
      if( BG_InventoryContainsUpgrade( i, ps->stats ) )
        worth += BG_Upgrade( i )->price;
    }

    if( ps->stats[ STAT_WEAPON ] != WP_NONE )
    {
      int *ps_clips = BG_GetClips(ps, ps->stats[STAT_WEAPON]);
      int totalAmmo =
        ps->ammo +
        (
          (
            BG_Weapon(ps->stats[STAT_WEAPON])->weaponOptionA ==
            WEAPONOPTA_ONE_ROUND_TO_ONE_CLIP) ? (*ps_clips) :
              (
                (*ps_clips) *
                BG_GetMaxAmmo(ps->stats, ps->stats[STAT_WEAPON])));

      worth += BG_Weapon( ps->stats[ STAT_WEAPON ] )->price;
      if( !BG_Weapon( ps->stats[ STAT_WEAPON ] )->usesEnergy )
        worth += totalAmmo * BG_Weapon( ps->stats[ STAT_WEAPON ] )->roundPrice;
    }
  }

  return worth;
}

/*
=================
BG_PlayerCanChangeWeapon
=================
*/
qboolean BG_PlayerCanChangeWeapon( playerState_t *ps, pmoveExt_t *pmext )
{
  // Do not allow Lucifer Cannon "canceling" via weapon switch
  if( ps->weapon == WP_LUCIFER_CANNON &&
      ps->misc[ MISC_MISC ] > LCANNON_CHARGE_TIME_MIN )
    return qfalse;

  if( ps->weapon == WP_LIGHTNING &&
      ( ps->misc[ MISC_MISC ] > LIGHTNING_BOLT_CHARGE_TIME_MIN ||
        ( ps->stats[ STAT_STATE ] & SS_CHARGING ) ) )
    return qfalse;

  // The ckit's build timer must be complete before switching
  if( (ps->weapon == WP_HBUILD || ps->weapon == WP_ABUILD2) &&
      ps->misc[ MISC_MISC ] )
    return qfalse;

  //bursts must complete
  if( pmext->burstRoundsToFire[ 2 ] ||
      pmext->burstRoundsToFire[ 1 ] ||
      pmext->burstRoundsToFire[ 0 ] )
    return qfalse;

  return ps->weaponTime <= 0 || ps->weaponstate != WEAPON_FIRING;
}

/*
=================
BG_PlayerPoisonCloudTime
=================
*/
int BG_PlayerPoisonCloudTime( playerState_t *ps )
{
  int time = LEVEL1_PCLOUD_TIME;

  if( BG_InventoryContainsUpgrade( UP_BATTLESUIT, ps->stats ) )
    time -= BSUIT_PCLOUD_PROTECTION;
  if( BG_InventoryContainsUpgrade( UP_HELMET, ps->stats ) )
    time -= HELMET_PCLOUD_PROTECTION;
  if( BG_InventoryContainsUpgrade( UP_LIGHTARMOUR, ps->stats ) )
    time -= LIGHTARMOUR_PCLOUD_PROTECTION;

  return time;
}

/*
=================
BG_GetPlayerWeapon

Returns the players current weapon or the weapon they are switching to.
Only needs to be used for human weapons.
=================
*/
weapon_t BG_GetPlayerWeapon( playerState_t *ps )
{
  if( ps->persistant[ PERS_NEWWEAPON ] )
    return ps->persistant[ PERS_NEWWEAPON ];

  return ps->weapon;
}

/*
===============
atof_neg

atof with an allowance for negative values
===============
*/
float atof_neg( char *token, qboolean allowNegative )
{
  float value;

  value = atof( token );

  if( !allowNegative && value < 0.0f )
    value = 1.0f;

  return value;
}

/*
===============
atoi_neg

atoi with an allowance for negative values
===============
*/
int atoi_neg( char *token, qboolean allowNegative )
{
  int value;

  value = atoi( token );

  if( !allowNegative && value < 0 )
    value = 1;

  return value;
}

#define MAX_NUM_PACKED_ENTITY_NUMS 10

/*
===============
BG_UnpackEntityNumbers

Unpack entity numbers from an entityState_t
===============
*/
int BG_UnpackEntityNumbers( entityState_t *es, int *entityNums, int count )
{
  int i;

  if( count > MAX_NUM_PACKED_ENTITY_NUMS )
    count = MAX_NUM_PACKED_ENTITY_NUMS;

  for( i = 0; i < count; i++ )
  {
    int *entityNum = &entityNums[ i ];

    switch( i )
    {
      case 0: *entityNum = es->misc;                                      break;
      case 1: *entityNum = es->time;                                      break;
      case 2: *entityNum = (es->time >> GENTITYNUM_BITS);                 break;
      case 3: *entityNum = (es->time >> (GENTITYNUM_BITS * 2));           break;
      case 4: *entityNum = es->time2;                                     break;
      case 5: *entityNum = (es->time2 >> GENTITYNUM_BITS);                break;
      case 6: *entityNum = (es->time2 >> (GENTITYNUM_BITS * 2));          break;
      case 7: *entityNum = es->constantLight;                             break;
      case 8: *entityNum = (es->constantLight >> GENTITYNUM_BITS);        break;
      case 9: *entityNum = (es->constantLight >> (GENTITYNUM_BITS * 2));  break;
      default: Com_Error( ERR_FATAL, "Entity index %d not handled", i );  break;
    }

    *entityNum &= GENTITYNUM_MASK;

    if( *entityNum == ENTITYNUM_NONE )
      break;
  }

  return i;
}

/*
===============
BG_ParseCSVEquipmentList
===============
*/
void BG_ParseCSVEquipmentList( const char *string, weapon_t *weapons, int weaponsSize,
    upgrade_t *upgrades, int upgradesSize )
{
  char      buffer[ MAX_STRING_CHARS ];
  int       i = 0, j = 0;
  char      *p, *q;
  qboolean  EOS = qfalse;

  Q_strncpyz( buffer, string, MAX_STRING_CHARS );

  p = q = buffer;

  while( *p != '\0' )
  {
    //skip to first , or EOS
    while( *p != ',' && *p != '\0' )
      p++;

    if( *p == '\0' )
      EOS = qtrue;

    *p = '\0';

    //strip leading whitespace
    while( *q == ' ' )
      q++;

    if( weaponsSize )
      weapons[ i ] = BG_WeaponByName( q )->number;

    if( upgradesSize )
      upgrades[ j ] = BG_UpgradeByName( q )->number;

    if( weaponsSize && weapons[ i ] == WP_NONE &&
        upgradesSize && upgrades[ j ] == UP_NONE )
      Com_Printf( S_COLOR_YELLOW "WARNING: unknown equipment %s\n", q );
    else if( weaponsSize && weapons[ i ] != WP_NONE )
      i++;
    else if( upgradesSize && upgrades[ j ] != UP_NONE )
      j++;

    if( !EOS )
    {
      p++;
      q = p;
    }
    else
      break;

    if( i == ( weaponsSize - 1 ) || j == ( upgradesSize - 1 ) )
      break;
  }

  if( weaponsSize )
    weapons[ i ] = WP_NONE;

  if( upgradesSize )
    upgrades[ j ] = UP_NONE;
}

/*
===============
BG_ParseCSVClassList
===============
*/
void BG_ParseCSVClassList( const char *string, class_t *classes, int classesSize )
{
  char      buffer[ MAX_STRING_CHARS ];
  int       i = 0;
  char      *p, *q;
  qboolean  EOS = qfalse;

  Q_strncpyz( buffer, string, MAX_STRING_CHARS );

  p = q = buffer;

  while( *p != '\0' && i < classesSize - 1 )
  {
    //skip to first , or EOS
    while( *p != ',' && *p != '\0' )
      p++;

    if( *p == '\0' )
      EOS = qtrue;

    *p = '\0';

    //strip leading whitespace
    while( *q == ' ' )
      q++;

    classes[ i ] = BG_ClassByName( q )->number;

    if( classes[ i ] == PCL_NONE )
      Com_Printf( S_COLOR_YELLOW "WARNING: unknown class %s\n", q );
    else
      i++;

    if( !EOS )
    {
      p++;
      q = p;
    }
    else
      break;
  }

  classes[ i ] = PCL_NONE;
}

/*
===============
BG_ParseCSVBuildableList
===============
*/
void BG_ParseCSVBuildableList( const char *string, buildable_t *buildables, int buildablesSize )
{
  char      buffer[ MAX_STRING_CHARS ];
  int       i = 0;
  char      *p, *q;
  qboolean  EOS = qfalse;

  Q_strncpyz( buffer, string, MAX_STRING_CHARS );

  p = q = buffer;

  while( *p != '\0' && i < buildablesSize - 1 )
  {
    //skip to first , or EOS
    while( *p != ',' && *p != '\0' )
      p++;

    if( *p == '\0' )
      EOS = qtrue;

    *p = '\0';

    //strip leading whitespace
    while( *q == ' ' )
      q++;

    buildables[ i ] = BG_BuildableByName( q )->number;

    if( buildables[ i ] == BA_NONE )
      Com_Printf( S_COLOR_YELLOW "WARNING: unknown buildable %s\n", q );
    else
      i++;

    if( !EOS )
    {
      p++;
      q = p;
    }
    else
      break;
  }

  buildables[ i ] = BA_NONE;
}

typedef struct gameElements_s
{
  buildable_t       buildables[ BA_NUM_BUILDABLES ];
  class_t           classes[ PCL_NUM_CLASSES ];
  weapon_t          weapons[ WP_NUM_WEAPONS ];
  upgrade_t         upgrades[ UP_NUM_UPGRADES ];
} gameElements_t;

static gameElements_t bg_disabledGameElements;

/*
============
BG_InitAllowedGameElements
============
*/
void BG_InitAllowedGameElements( void )
{
  char cvar[ MAX_CVAR_VALUE_STRING ];

  Cvar_VariableStringBuffer( "g_disabledEquipment",
      cvar, MAX_CVAR_VALUE_STRING );

  BG_ParseCSVEquipmentList( cvar,
      bg_disabledGameElements.weapons, WP_NUM_WEAPONS,
      bg_disabledGameElements.upgrades, UP_NUM_UPGRADES );

  Cvar_VariableStringBuffer( "g_disabledClasses",
      cvar, MAX_CVAR_VALUE_STRING );

  BG_ParseCSVClassList( cvar,
      bg_disabledGameElements.classes, PCL_NUM_CLASSES );

  Cvar_VariableStringBuffer( "g_disabledBuildables",
      cvar, MAX_CVAR_VALUE_STRING );

  BG_ParseCSVBuildableList( cvar,
      bg_disabledGameElements.buildables, BA_NUM_BUILDABLES );
}

/*
============
BG_WeaponIsAllowed
============
*/
qboolean BG_WeaponIsAllowed( weapon_t weapon, qboolean devMode )
{
  int i;

  if( devMode )
    return qtrue;

  for( i = 0; i < WP_NUM_WEAPONS &&
      bg_disabledGameElements.weapons[ i ] != WP_NONE; i++ )
  {
    if( bg_disabledGameElements.weapons[ i ] == weapon )
      return qfalse;
  }

  return BG_Weapon( weapon )->enabled;
}

/*
============
BG_UpgradeIsAllowed
============
*/
qboolean BG_UpgradeIsAllowed( upgrade_t upgrade, qboolean devMode )
{
  int i;

  if( devMode )
    return qtrue;

  for( i = 0; i < UP_NUM_UPGRADES &&
      bg_disabledGameElements.upgrades[ i ] != UP_NONE; i++ )
  {
    if( bg_disabledGameElements.upgrades[ i ] == upgrade )
      return qfalse;
  }

  return BG_Upgrade( upgrade )->enabled;
}

/*
============
BG_ClassIsAllowed
============
*/
qboolean BG_ClassIsAllowed( class_t class, qboolean devMode )
{
  int i;

  if( devMode )
    return qtrue;

  for( i = 0; i < PCL_NUM_CLASSES &&
      bg_disabledGameElements.classes[ i ] != PCL_NONE; i++ )
  {
    if( bg_disabledGameElements.classes[ i ] == class )
      return qfalse;
  }

  return BG_Class( class )->enabled;
}

/*
============
BG_GetPainState
============
*/
pain_t BG_GetPainState( playerState_t *ps )
{
  int maxHealth = BG_Class( ps->stats[ STAT_CLASS ] )->health;
  int health = ps->misc[ MISC_HEALTH ];

  if( BG_InventoryContainsUpgrade( UP_BATTLESUIT, ps->stats ) )
  {
    maxHealth = BSUIT_MAX_ARMOR;
    health = ps->misc[ MISC_ARMOR ];
  }

  if( health < ( maxHealth * 3 ) / 32 )
    return PAIN_25;
  else if( health < ( maxHealth * 3 ) / 8 )
    return PAIN_50;
  else if( health < ( maxHealth * 3 ) / 4 )
    return PAIN_75;
  else
    return PAIN_100;
}

/*
============
BG_GetBsuitResaleValue
============
*/
int BG_GetBsuitResaleValue(int armor, int armor_to_gen) {
  float armor_fraction;
  int   price;

  armor_fraction = ((float)(armor + armor_to_gen))/((float)BSUIT_MAX_ARMOR);
  price = armor_fraction * (BG_Upgrade(UP_BATTLESUIT)->price);
  return MAX(price, BSUIT_PRICE_USED_MIN);
}

/*
============
BG_BuildableIsAllowed
============
*/
qboolean BG_BuildableIsAllowed( buildable_t buildable, qboolean devMode )
{
  int i;

  if( devMode )
    return qtrue;

  for( i = 0; i < BA_NUM_BUILDABLES &&
      bg_disabledGameElements.buildables[ i ] != BA_NONE; i++ )
  {
    if( bg_disabledGameElements.buildables[ i ] == buildable )
      return qfalse;
  }

  return BG_Buildable( buildable )->enabled;
}

/*
============
BG_LoadEmoticons
============
*/
int BG_LoadEmoticons( emoticon_t *emoticons, int num )
{
  int numFiles;
  char fileList[ MAX_EMOTICONS * ( MAX_EMOTICON_NAME_LEN + 9 ) ] = {""};
  int i;
  char *filePtr;
  int fileLen;
  int count;

  numFiles = FS_GetFileList( "emoticons", "x1.tga", fileList,
    sizeof( fileList ) );

  if( numFiles < 1 )
    return 0;

  filePtr = fileList;
  fileLen = 0;
  count = 0;
  for( i = 0; i < numFiles && count < num; i++, filePtr += fileLen + 1 )
  {
    fileLen = strlen( filePtr );
    if( fileLen < 9 || filePtr[ fileLen - 8 ] != '_' ||
        filePtr[ fileLen - 7 ] < '1' || filePtr[ fileLen - 7 ] > '9' )
    {
      Com_Printf( S_COLOR_YELLOW "skipping invalidly named emoticon \"%s\"\n",
        filePtr );
      continue;
    }
    if( fileLen - 8 > MAX_EMOTICON_NAME_LEN )
    {
      Com_Printf( S_COLOR_YELLOW "emoticon file name \"%s\" too long (>%d)\n",
        filePtr, MAX_EMOTICON_NAME_LEN + 8 );
      continue;
    }
    if( !FS_FOpenFileByMode( va( "emoticons/%s", filePtr ), NULL, FS_READ ) )
    {
      Com_Printf( S_COLOR_YELLOW "could not open \"emoticons/%s\"\n", filePtr );
      continue;
    }

    Q_strncpyz( emoticons[ count ].name, filePtr, fileLen - 8 + 1 );
#ifndef GAME
    emoticons[ count ].width = filePtr[ fileLen - 7 ] - '0';
#endif
    count++;
  }

  Com_Printf( "Loaded %d of %d emoticons (MAX_EMOTICONS is %d)\n",
    count, numFiles, MAX_EMOTICONS );
  return count;
}

/*
============
BG_Team
============
*/
const teamAttributes_t *BG_Team(team_t team) {
  return (team < NUM_TEAMS) ?
    &bg_teamList[team] : &nullTeam;
}

/*
============
BG_MOD
============
*/
const modAttributes_t *BG_MOD(meansOfDeath_t mod) {
  if(mod < NUM_MODS) {
    Com_Assert(mod == bg_modList[mod].means_of_death);
  }

  return ( mod < NUM_MODS ) ?
    &bg_modList[ mod ] : &nullMOD;
}

/*
============
BG_Rank
============
*/
const rankAttributes_t *BG_Rank(rank_t rank) {
  return (rank < NUM_OF_RANKS) ?
    &bg_rankList[rank] : &nullRank;
}

/*
================
BG_CreateRotationMatrix
================
*/
void BG_CreateRotationMatrix(vec3_t angles, vec3_t matrix[3]) {
  AngleVectors(angles, matrix[0], matrix[1], matrix[2]);
  VectorInverse(matrix[1]);
}

/*
================
BG_TransposeMatrix
================
*/
void BG_TransposeMatrix(vec3_t matrix[ 3 ], vec3_t transpose[ 3 ]) {
  int i, j;

  for(i = 0; i < 3; i++) {
    for(j = 0; j < 3; j++) {
      transpose[i][j] = matrix[j][i];
    }
  }
}

/*
================
BG_RotatePoint
================
*/
void BG_RotatePoint(vec3_t point, vec3_t matrix[3]) {
  vec3_t tvec;

  VectorCopy( point, tvec );
  point[0] = DotProduct(matrix[0], tvec);
  point[1] = DotProduct(matrix[1], tvec);
  point[2] = DotProduct(matrix[2], tvec);
}

/*
============
BG_EvaluateBBOXPoint
============
*/
void BG_EvaluateBBOXPoint( bboxPoint_t *bboxPoint, vec3_t origin,
                      const vec3_t minsIn, const vec3_t maxsIn )
{
  vec3_t      mins, maxs;
  const float inset = 1.0f;
  int         i;

  Com_Assert( bboxPoint );
  Com_Assert( origin );

  VectorCopy( origin, bboxPoint->point );

  // return the origin if either the mins or maxs is NULL
  if( !minsIn || !maxsIn )
    return;

  Com_Assert( minsIn[ 0 ] <= maxsIn[ 0 ] );
  Com_Assert( minsIn[ 1 ] <= maxsIn[ 1 ] );
  Com_Assert( minsIn[ 2 ] <= maxsIn[ 2 ] );

  VectorCopy( minsIn, mins );
  VectorCopy( maxsIn, maxs );

  //inset the points into the bbox
  for( i = 0; i < 3; i++ )
  {
    //ensure that the mins and maxs don't pass each other
    if( ( mins[ i ] + inset ) < ( maxs[ i ] - inset ) )
    {
      maxs[ i ] -= inset;
      mins[ i ] += inset;
    } else
    {
      //set the min and max values for the given axis equal to their mid point
      maxs[ i ] = mins[ i ] = ( ( maxs[ i ] + mins[ i ] ) / 2 );
    }
  }

  switch ( bboxPoint->num )
  {
    case BBXP_ORIGIN:
      //return the origin
      break;

    case BBXP_MIDPOINT:
      bboxPoint->point[2] += ( mins[2] + maxs[2] ) / 2;
      bboxPoint->point[1] += ( mins[1] + maxs[1] ) / 2;
      bboxPoint->point[0] += ( mins[0] + maxs[0] ) / 2;
      break;

    case BBXP_MIDFACE1:
      bboxPoint->point[2] += maxs[2];
      bboxPoint->point[1] += ( mins[1] + maxs[1] ) / 2;
      bboxPoint->point[0] += ( mins[0] + maxs[0] ) / 2;
      break;

    case BBXP_MIDFACE2:
      bboxPoint->point[2] += ( mins[2] + maxs[2] ) / 2;
      bboxPoint->point[1] += maxs[1];
      bboxPoint->point[0] += ( mins[0] + maxs[0] ) / 2;
      break;

    case BBXP_MIDFACE3:
      bboxPoint->point[2] += ( mins[2] + maxs[2] ) / 2;
      bboxPoint->point[1] += mins[1];
      bboxPoint->point[0] += ( mins[0] + maxs[0] ) / 2;
      break;

    case BBXP_MIDFACE4:
      bboxPoint->point[2] += ( mins[2] + maxs[2] ) / 2;
      bboxPoint->point[1] += ( mins[1] + maxs[1] ) / 2;
      bboxPoint->point[0] += maxs[0];
      break;

    case BBXP_MIDFACE5:
      bboxPoint->point[2] += ( mins[2] + maxs[2] ) / 2;
      bboxPoint->point[1] += ( mins[1] + maxs[1] ) / 2;
      bboxPoint->point[0] += mins[0];
      break;

    case BBXP_MIDFACE6:
      bboxPoint->point[2] += mins[2];
      bboxPoint->point[1] += ( mins[1] + maxs[1] ) / 2;
      bboxPoint->point[0] += ( mins[0] + maxs[0] ) / 2;
      break;

    case BBXP_VERTEX1:
      bboxPoint->point[2] += maxs[2];
      bboxPoint->point[1] += maxs[1];
      bboxPoint->point[0] += maxs[0];
      break;

    case BBXP_VERTEX2:
      bboxPoint->point[2] += maxs[2];
      bboxPoint->point[1] += mins[1];
      bboxPoint->point[0] += maxs[0];
      break;

    case BBXP_VERTEX3:
      bboxPoint->point[2] += maxs[2];
      bboxPoint->point[1] += maxs[1];
      bboxPoint->point[0] += mins[0];
      break;

    case BBXP_VERTEX4:
      bboxPoint->point[2] += maxs[2];
      bboxPoint->point[1] += mins[1];
      bboxPoint->point[0] += mins[0];
      break;

    case BBXP_VERTEX5:
      bboxPoint->point[2] += mins[2];
      bboxPoint->point[1] += maxs[1];
      bboxPoint->point[0] += maxs[0];
      break;

    case BBXP_VERTEX6:
      bboxPoint->point[2] += mins[2];
      bboxPoint->point[1] += mins[1];
      bboxPoint->point[0] += maxs[0];
      break;

    case BBXP_VERTEX7:
      bboxPoint->point[2] += mins[2];
      bboxPoint->point[1] += maxs[1];
      bboxPoint->point[0] += mins[0];
      break;

    case BBXP_VERTEX8:
      bboxPoint->point[2] += mins[2];
      bboxPoint->point[1] += mins[1];
      bboxPoint->point[0] += mins[0];
      break;

    default:
      Com_Error( ERR_DROP,
                 "BG_EvaluateBBOXPoint: unknown bboxPointNum: %i",
                 bboxPoint->num );
      break;
  }
}

/*
============
BG_PointIsInsideBBOX
============
*/
qboolean BG_PointIsInsideBBOX(
  const vec3_t point, const vec3_t mins, const vec3_t maxs) {
  Com_Assert(point);
  Com_Assert(mins);
  Com_Assert(maxs);

  if(
    point[0] < mins[0] ||
    point[1] < mins[1] ||
    point[2] < mins[2] ||
    point[0] > maxs[0] ||
    point[1] > maxs[1] ||
    point[2] > maxs[2]) {
    return qfalse;
  }

  return qtrue;
}

/*
============
BG_FindBBoxCenter
============
*/
void BG_FindBBoxCenter(
  const vec3_t origin, const vec3_t mins, const vec3_t maxs, vec3_t center) {
  vec3_t absmin;
  vec3_t absmax;
  int    i;

  VectorAdd(origin, mins, absmin);
  VectorAdd(origin, maxs, absmax);

  for(i = 0; i < 3; i++) {
    center[i] = (absmin[i] + absmax[i]) / 2;
  }
}

int cmdcmp( const void *a, const void *b )
{
  return Q_stricmp( (const char *)a, ((dummyCmd_t *)b)->name );
}

/*
============
BG_Scrim_Team_From_Playing_Team
============
*/
scrim_team_t BG_Scrim_Team_From_Playing_Team(scrim_t *scrim, team_t playing_team) {
  scrim_team_t scrim_team;

  if(playing_team == TEAM_NONE || playing_team >= NUM_TEAMS || playing_team < 0) {
    return SCRIM_TEAM_NONE;
  }

  for(scrim_team = 0; scrim_team < NUM_SCRIM_TEAMS; scrim_team++) {
    if(scrim_team == SCRIM_TEAM_NONE) {
      continue;
    }

    if(scrim->team[scrim_team].current_team == playing_team) {
      return scrim_team;
    }
  }

  return SCRIM_TEAM_NONE;
}
