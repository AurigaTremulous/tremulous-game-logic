/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2013 Darklegion Development
Copyright (C) 2015-2018 GrangerHub

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

#define ACTIVATION_ENT_RANGE            64 // default maximum range a client can activate an activation entity

// health and damage are specified in this file as subunits of
// health and damage points for better finetuning.
#define HEALTH_POINT_SUBUNIT_FACTOR     1000 // health and damage points are made up of this many sub-units.

/*
 * ALIEN weapons
 *
 * _REPEAT  - time in msec until the weapon can be used again
 * _DMG     - amount of damage the weapon does
 *
 * ALIEN_WDMG_MODIFIER - overall damage modifier for coarse tuning
 *
 */

#define ALIEN_WDMG_MODIFIER         1.0f
#define AWDM(d)                     ((int)((float)d*ALIEN_WDMG_MODIFIER))

#define ABUILDER_BUILD_REPEAT       500
#define ABUILDER_CLAW_DMG           AWDM(20000)
#define ABUILDER_CLAW_RANGE         64.0f
#define ABUILDER_CLAW_WIDTH         4.0f
#define ABUILDER_CLAW_REPEAT        1000
#define ABUILDER_CLAW_K_SCALE       1.0f
#define ABUILDER_BLOB_DMG           AWDM(4000)
#define ABUILDER_BLOB_REPEAT        1000
#define ABUILDER_BLOB_SPEED         800.0f
#define ABUILDER_BLOB_SPEED_MOD     0.5f
#define ABUILDER_BLOB_TIME          2000
#define ABUILDER_BLOB_LIFETIME      15000

#define LEVEL0_BITE_DMG             AWDM(48000)
#define LEVEL0_BITE_RANGE           64.0f
#define LEVEL0_BITE_WIDTH           6.0f
#define LEVEL0_BITE_REPEAT          500
#define LEVEL0_BITE_K_SCALE         1.0f
#define LEVEL0_POUNCE_RANGE         48.0f
#define LEVEL0_POUNCE_WIDTH         14.0f
#define LEVEL0_POUNCE_TIME          600      // msec for full Dretch pounce
#define LEVEL0_POUNCE_TIME_MIN      200      // msec before which pounce cancels  
#define LEVEL0_POUNCE_REPEAT        320      // msec before a new pounce starts
#define LEVEL0_POUNCE_SPEED_MOD     0.75f    // walking speed modifier for pounce charging
#define LEVEL0_POUNCE_JUMP_MAG      600      // Dretch pounce jump power

#define LEVEL1_CLAW_DMG                AWDM(32000)
#define LEVEL1_CLAW_RANGE              96.0f
#define LEVEL1_CLAW_U_RANGE            LEVEL1_CLAW_RANGE + 3.0f
#define LEVEL1_CLAW_WIDTH              10.0f
#define LEVEL1_CLAW_REPEAT             600
#define LEVEL1_CLAW_U_REPEAT           500
#define LEVEL1_CLAW_K_SCALE            1.0f
#define LEVEL1_CLAW_U_K_SCALE          1.0f
#define LEVEL1_GRAB_RANGE              LEVEL1_CLAW_RANGE
#define LEVEL1_GRAB_U_RANGE            LEVEL1_GRAB_RANGE + 3.0f
#define LEVEL1_GRAB_TIME               900
#define LEVEL1_GRAB_U_TIME             950
#define LEVEL1_GRAB_REPEAT             1200
#define LEVEL1_GRAB_U_REPEAT           1000
#define LEVEL1_PCLOUD_DMG              AWDM(4000)
#define LEVEL1_PCLOUD_RANGE            120.0f
#define LEVEL1_PCLOUD_REPEAT           2000
#define LEVEL1_PCLOUD_TIME             10000
#define LEVEL1_REGEN_MOD               (2.0f)
#define LEVEL1_UPG_REGEN_MOD           (3.0f)
#define LEVEL1_REGEN_SCOREINC          AVM(100) // score added for healing per 10s
#define LEVEL1_UPG_REGEN_SCOREINC      AVM(200)
#define LEVEL1_INVISIBILITY_DELAY      800
#define LEVEL1_INVISIBILITY_PAIN_DELAY 1600
#define LEVEL1_CHARGE_RATE             4.0f
#define LEVEL1_UPG_CHARGE_RATE         6.0f
#define LEVEL1_INVISIBILITY_TIME       20000
#define LEVEL1_UPG_INVISIBILITY_TIME   36000

#define LEVEL2_CLAW_DMG             AWDM(50000)
#define LEVEL2_CLAW_RANGE           85.0f
#define LEVEL2_CLAW_U_RANGE         LEVEL2_CLAW_RANGE + 2.0f
#define LEVEL2_CLAW_WIDTH           17.0f
#define LEVEL2_CLAW_REPEAT          500
#define LEVEL2_CLAW_K_SCALE         1.0f
#define LEVEL2_CLAW_U_REPEAT        400
#define LEVEL2_CLAW_U_K_SCALE       1.0f
#define LEVEL2_AREAZAP_DMG          AWDM(60000)
#define LEVEL2_AREAZAP_RANGE        200.0f
#define LEVEL2_AREAZAP_CHAIN_RANGE  150.0f
#define LEVEL2_AREAZAP_CHAIN_FALLOFF 8.0f
#define LEVEL2_AREAZAP_WIDTH        15.0f
#define LEVEL2_AREAZAP_REPEAT       1500
#define LEVEL2_AREAZAP_TIME         1000
#define LEVEL2_AREAZAP_MAX_TARGETS  5
#define LEVEL2_WALLJUMP_MAXSPEED    1000.0f

#define LEVEL3_CLAW_DMG             AWDM(80000)
#define LEVEL3_CLAW_RANGE           96.0f
#define LEVEL3_CLAW_UPG_RANGE       LEVEL3_CLAW_RANGE // + 3.0f          
#define LEVEL3_CLAW_WIDTH           16.0f
#define LEVEL3_CLAW_REPEAT          700
#define LEVEL3_CLAW_K_SCALE         1.0f
#define LEVEL3_CLAW_U_REPEAT        600
#define LEVEL3_CLAW_U_K_SCALE       1.0f
#define LEVEL3_POUNCE_DMG           AWDM(80000)
#define LEVEL3_POUNCE_RANGE         72.0f
#define LEVEL3_POUNCE_UPG_RANGE     LEVEL3_POUNCE_RANGE + 3.0f
#define LEVEL3_POUNCE_WIDTH         16.0f
#define LEVEL3_POUNCE_TIME          700      // msec for full Dragoon pounce
#define LEVEL3_POUNCE_TIME_UPG      700      // msec for full Adv. Dragoon pounce
#define LEVEL3_POUNCE_TIME_MIN      200      // msec before which pounce cancels  
#define LEVEL3_POUNCE_REPEAT        400      // msec before a new pounce starts
#define LEVEL3_POUNCE_SPEED_MOD     0.75f    // walking speed modifier for pounce charging
#define LEVEL3_POUNCE_JUMP_MAG      700      // Dragoon pounce jump power
#define LEVEL3_POUNCE_JUMP_MAG_UPG  800      // Adv. Dragoon pounce jump power
#define LEVEL3_BOUNCEBALL_DMG       AWDM(110000)
#define LEVEL3_BOUNCEBALL_REPEAT    1000
#define LEVEL3_BOUNCEBALL_SPEED     1000.0f
#define LEVEL3_BOUNCEBALL_RADIUS    0
#define LEVEL3_BOUNCEBALL_REGEN     12500    // msec until new barb
#define LEVEL3_BOUNCEBALL_LIFETIME  3000

#define LEVEL4_CLAW_DMG             AWDM(100000)
#define LEVEL4_CLAW_RANGE           75.0f
#define LEVEL4_CLAW_WIDTH           20.0f
#define LEVEL4_CLAW_HEIGHT          20.0f
#define LEVEL4_CLAW_REPEAT          750
#define LEVEL4_CLAW_K_SCALE         1.0f

#define LEVEL4_TRAMPLE_DMG             AWDM(111000)
#define LEVEL4_TRAMPLE_SPEED           2.0f
#define LEVEL4_TRAMPLE_CHARGE_MIN      750   // minimum msec to start a charge
#define LEVEL4_TRAMPLE_CHARGE_MAX      1500  // msec to maximum charge stored
#define LEVEL4_TRAMPLE_CHARGE_TRIGGER  3000  // msec charge starts on its own
#define LEVEL4_TRAMPLE_DURATION        3000  // msec trample lasts on full charge
#define LEVEL4_TRAMPLE_STOP_PENALTY    1     // charge lost per msec when stopped
#define LEVEL4_TRAMPLE_REPEAT          1000   // msec before a trample will rehit a player

#define LEVEL4_CRUSH_DAMAGE_PER_V      500.0f // damage per falling velocity
#define LEVEL4_CRUSH_DAMAGE            120000 // to players only
#define LEVEL4_CRUSH_REPEAT            500    // player damage repeat

#define SPITFIRE_ZAP_MAX_TARGETS        6
#define SPITFIRE_ZAP_REPEAT             1250
#define SPITFIRE_ZAP_K_SCALE            1.0f
#define SPITFIRE_ZAP_RANGE              300.0f
#define SPITFIRE_ZAP_DMG                AWDM(24000)
#define SPITFIRE_ZAP_TIME               150
#define SPITFIRE_ASCEND_REPEAT          400
#define SPITFIRE_ASCEND_MAG             800.0f
#define SPITFIRE_POUNCE_JUMP_MAG        2250
#define SPITFIRE_POUNCE_REPEAT          250
#define SPITFIRE_POUNCE_TIME            1200
#define SPITFIRE_POUNCE_TIME_MIN        250
#define SPITFIRE_POUNCE_DMG             AWDM(60000)
#define SPITFIRE_POUNCE_RANGE           48.0f
#define SPITFIRE_POUNCE_WIDTH           14.0f
#define SPITFIRE_POUNCE_SPEED_MOD       0.95f
#define SPITFIRE_PAYLOAD_DISCHARGE_TIME ( SPITFIRE_POUNCE_TIME + SPITFIRE_POUNCE_REPEAT )
#define SPITFIRE_POUNCE_MAX_STAMINA     ( SPITFIRE_POUNCE_TIME * 20 )
#define SPITFIRE_POUNCE_STAMINA_RESTORE 15
#define SPITFIRE_BACK_MODIFIER          0.8f
#define SPITFIRE_SIDE_MODIFIER          0.9f
#define SPITFIRE_AIRSPEED_MOD           0.36f
#define SPITFIRE_GLIDE_ANGLE            15.0f //optimal angle of attack
#define SPITFIRE_GLIDE_MOD              1000
#define SPITFIRE_GLIDE_ACCEL            3.8f
#define SPITFIRE_HOVER_BOB              0.001f //for view bobbing from hovering

/*
 * ALIEN classes
 *
 * _SPEED   - fraction of Q3A run speed the class can move
 * _REGEN   - health per second regained
 *
 * ALIEN_HLTH_MODIFIER - overall health modifier for coarse tuning
 *
 */

#define MAX_EVOLVE_PERIOD           10000 // maximum time in ms that evolving requires.

#define ALIEN_HLTH_MODIFIER         1.0f
#define AHM(h)                      ((int)((float)h*ALIEN_HLTH_MODIFIER))

#define ALIEN_VALUE_MODIFIER        1.0f
#define AVM(h)                      ((int)((float)h*ALIEN_VALUE_MODIFIER))

#define ALIEN_HP_RESERVE_REGEN_MOD  (1.0f/9.0f)
#define ALIEN_HP_RESERVE_MAX        2.0f
#define ALIEN_EVO_HP_RESERVE_GAIN   (1.0f/8.0f)

#define ABUILDER_SPEED              0.9f
#define ABUILDER_VALUE              AVM(((float)(ALIEN_CREDITS_PER_KILL)) / 2.0f)
#define ABUILDER_HEALTH             AHM(75000)
#define ABUILDER_REGEN              (0.04f * ABUILDER_HEALTH)
#define ABUILDER_COST               0

#define ABUILDER_UPG_SPEED          0.9f
#define ABUILDER_UPG_VALUE          AVM(((float)(ALIEN_CREDITS_PER_KILL)) * 1.5f)
#define ABUILDER_UPG_HEALTH         AHM(100000)
#define ABUILDER_UPG_REGEN          (0.04f * ABUILDER_UPG_HEALTH)
#define ABUILDER_UPG_COST           0

#define LEVEL0_SPEED                1.4f
#define LEVEL0_VALUE                AVM(ALIEN_CREDITS_PER_KILL)
#define LEVEL0_HEALTH               AHM(38000)
#define LEVEL0_REGEN                (0.05f * LEVEL0_HEALTH)
#define LEVEL0_COST                 0

#define LEVEL1_SPEED                1.25f
#define LEVEL1_VALUE                AVM(1 * ALIEN_CREDITS_PER_KILL)
#define LEVEL1_HEALTH               AHM(75000)
#define LEVEL1_REGEN                (0.03f * LEVEL1_HEALTH)
#define LEVEL1_COST                 1

#define LEVEL1_UPG_SPEED            1.25f
#define LEVEL1_UPG_VALUE            AVM(2 * ALIEN_CREDITS_PER_KILL)
#define LEVEL1_UPG_HEALTH           AHM(100000)
#define LEVEL1_UPG_REGEN            (0.03f * LEVEL1_UPG_HEALTH)
#define LEVEL1_UPG_COST             3

#define LEVEL2_SPEED                1.35f
#define LEVEL2_VALUE                AVM(2 * ALIEN_CREDITS_PER_KILL)
#define LEVEL2_HEALTH               AHM(190000)
#define LEVEL2_REGEN                (0.03f * LEVEL2_HEALTH)
#define LEVEL2_COST                 2

#define LEVEL2_UPG_SPEED            1.35f
#define LEVEL2_UPG_VALUE            AVM(4 * ALIEN_CREDITS_PER_KILL)
#define LEVEL2_UPG_HEALTH           AHM(220000)
#define LEVEL2_UPG_REGEN            (0.03f * LEVEL2_UPG_HEALTH)
#define LEVEL2_UPG_COST             4

#define LEVEL3_SPEED                1.1f
#define LEVEL3_VALUE                AVM(3 * ALIEN_CREDITS_PER_KILL)
#define LEVEL3_HEALTH               AHM(275000)
#define LEVEL3_REGEN                (0.03f * LEVEL3_HEALTH)
#define LEVEL3_COST                 5

#define LEVEL3_UPG_SPEED            1.1f
#define LEVEL3_UPG_VALUE            AVM(5 * ALIEN_CREDITS_PER_KILL)
#define LEVEL3_UPG_HEALTH           AHM(400000)
#define LEVEL3_UPG_REGEN            (10000)
#define LEVEL3_UPG_COST             7

#define LEVEL4_SPEED                1.2f
#define LEVEL4_VALUE                AVM(8 * ALIEN_CREDITS_PER_KILL)
#define LEVEL4_HEALTH               AHM(700000)
#define LEVEL4_REGEN                (15000)
#define LEVEL4_COST                 10

#define SPITFIRE_SPEED                1.3f
#define SPITFIRE_VALUE                AVM(3 * ALIEN_CREDITS_PER_KILL)
#define SPITFIRE_HEALTH               AHM(165000)
#define SPITFIRE_REGEN                (0.03f * SPITFIRE_HEALTH)
#define SPITFIRE_COST                 3

/*
 * ALIEN buildables
 *
 * _BP            - build points required for this buildable
 * _BT            - build time required for this buildable
 * _REGEN         - the amount of health per second regained
 * _SPLASHDAMGE   - the amount of damage caused by this buildable when melting
 * _SPLASHRADIUS  - the radius around which it does this damage
 *
 * CREEP_BASESIZE - the maximum distance a buildable can be from an egg/overmind
 * ALIEN_BHLTH_MODIFIER - overall health modifier for coarse tuning
 * ALIEN_BDMG_MODIFIER  - overall damage modifier for coarse tuning
 * ALIEN_BMAXHEALTH_DECAY - the amount the max health is reduced by from repair
 *
 */

#define ALIEN_BHLTH_MODIFIER        1.0f
#define ABHM(h)                     ((int)((float)h*ALIEN_BHLTH_MODIFIER))
#define ALIEN_BDMG_MODIFIER         1.0f
#define ABDM(d)                     ((int)((float)d*ALIEN_BDMG_MODIFIER))

#define ALIEN_BREGEN_MODIFIER       (1.0f)
#define ABRM(r)                     ((int)((float)r*ALIEN_BREGEN_MODIFIER))

#define ALIEN_BMAXHEALTH_DECAY( h )  ( h / 2 )

#define CREEP_BASESIZE              700
#define CREEP_TIMEOUT               1000
#define CREEP_MODIFIER              0.5f
#define CREEP_ARMOUR_MODIFIER       0.75f
#define ALIEN_CREEP_RESERVE         10000 // amount of time an alien buildable
                                          //can survive without a creep source
#define CREEP_SCALEDOWN_TIME        3000
#define ALIEN_CREEP_BLAST_DELAY     2000

#define PCLOUD_MODIFIER             0.5f
#define PCLOUD_ARMOUR_MODIFIER      0.75f

#define ASPAWN_BP                   12
#define ASPAWN_BT                   7500
#define ASPAWN_HEALTH               ABHM(250000)
#define ASPAWN_REGEN                ABRM(8000)
#define ASPAWN_SPLASHDAMAGE         ABDM(50000)
#define ASPAWN_SPLASHRADIUS         100
#define ASPAWN_CREEPSIZE            120
#define ASPAWN_VALUE                ( ( 3 * LEVEL0_VALUE ) / 2 )

#define BARRICADE_BP                8
#define BARRICADE_BT                10000
#define BARRICADE_HEALTH            ABHM(300000)
#define BARRICADE_REGEN             ABRM(14000)
#define BARRICADE_SPLASHDAMAGE      ABDM(50000)
#define BARRICADE_SPLASHRADIUS      100
#define BARRICADE_CREEPSIZE         120
#define BARRICADE_SHRINKPROP        0.25f
#define BARRICADE_SHRINKTIMEOUT     500
#define BARRICADE_VALUE             ( ( 3 * LEVEL0_VALUE ) / 2 )
#define BARRICADE_BAT_PWR           25000

#define BOOSTER_BP                  12
#define BOOSTER_BT                  7500
#define BOOSTER_HEALTH              ABHM(150000)
#define BOOSTER_REGEN               ABRM(8000)
#define BOOSTER_SPLASHDAMAGE        ABDM(50000)
#define BOOSTER_SPLASHRADIUS        100
#define BOOSTER_CREEPSIZE           120
#define BOOSTER_REGEN_MOD           (2.75f)
#define BOOSTER_VALUE               ( ( 9 * LEVEL0_VALUE ) / 2 )
#define BOOSTER_BAT_PWR             25000
#define BOOST_TIME                  30000
#define BOOST_WARN_TIME             15000

#define ACIDTUBE_BP                 8
#define ACIDTUBE_BT                 7500
#define ACIDTUBE_HEALTH             ABHM(125000)
#define ACIDTUBE_REGEN              ABRM(10000)
#define ACIDTUBE_SPLASHDAMAGE       ABDM(50000)
#define ACIDTUBE_SPLASHRADIUS       300
#define ACIDTUBE_CREEPSIZE          120
#define ACIDTUBE_DAMAGE             ABDM(6000)
#define ACIDTUBE_RANGE              450.0f
#define ACIDTUBE_REPEAT             200
#define ACIDTUBE_REPEAT_ANIM        2000
#define ACIDTUBE_VALUE              ( ( 3 * LEVEL0_VALUE ) / 4 )
#define ACIDTUBE_BAT_PWR            25000

#define HIVE_BP                     12
#define HIVE_BT                     10000
#define HIVE_HEALTH                 ABHM(125000)
#define HIVE_REGEN                  ABRM(10000)
#define HIVE_SPLASHDAMAGE           ABDM(30000)
#define HIVE_SPLASHRADIUS           200
#define HIVE_CREEPSIZE              120
#define HIVE_SENSE_RANGE            500.0f
#define HIVE_LIFETIME               3000
#define HIVE_REPEAT                 3000
#define HIVE_K_SCALE                1.0f
#define HIVE_DMG                    ABDM(80000)
#define HIVE_SPEED                  320.0f
#define HIVE_DIR_CHANGE_PERIOD      500
#define HIVE_VALUE                  ( ( 3 * LEVEL0_VALUE ) / 2 )
#define HIVE_BAT_PWR                25000

#define HOVEL_BP                     0
#define HOVEL_BT                     7500
#define HOVEL_HEALTH                 ABHM(375000)
#define HOVEL_REGEN                  ABRM(20000)
#define HOVEL_SPLASHDAMAGE           ABDM(20000)
#define HOVEL_SPLASHRADIUS           200
#define HOVEL_CREEPSIZE              120
#define HOVEL_VALUE                  ( ( 9 * LEVEL0_VALUE ) / 8 )
#define HOVEL_BAT_PWR                25000

#define GRAPNEL_BP                  2
#define GRAPNEL_BT                  6000
#define GRAPNEL_HEALTH              ABHM(175000)
#define GRAPNEL_REGEN               ABRM(20000)
#define GRAPNEL_SPLASHDAMAGE        ABDM(20000)
#define GRAPNEL_SPLASHRADIUS        100
#define GRAPNEL_CREEPSIZE           30
#define GRAPNEL_VALUE               ( LEVEL0_VALUE / 2 )
#define GRAPNEL_BAT_PWR             25000

#define TRAPPER_BP                  8
#define TRAPPER_BT                  6000
#define TRAPPER_HEALTH              ABHM(50000)
#define TRAPPER_REGEN               ABRM(6000)
#define TRAPPER_SPLASHDAMAGE        ABDM(15000)
#define TRAPPER_SPLASHRADIUS        100
#define TRAPPER_CREEPSIZE           30
#define TRAPPER_RANGE               400
#define TRAPPER_REPEAT              1000
#define TRAPPER_VALUE               ( ( 3 * LEVEL0_VALUE ) / 4 )
#define TRAPPER_BAT_PWR             25000
#define LOCKBLOB_SPEED              650.0f
#define LOCKBLOB_SPEED_MOD          0.1f
#define LOCKBLOB_LOCKTIME           10000
#define LOCKBLOB_DOT                0.85f // max angle = acos( LOCKBLOB_DOT )
#define LOCKBLOB_K_SCALE            1.0f
#define LOCKBLOB_LIFETIME           15000

#define SLIME_ZUNGE_BP              12
#define SLIME_ZUNGE_BT              8000
#define SLIME_ZUNGE_HEALTH          ABHM(180000)
#define SLIME_ZUNGE_REGEN           ABRM(12000)
#define SLIME_ZUNGE_SPLASHDAMAGE    ABDM(15000)
#define SLIME_ZUNGE_SPLASHRADIUS    200
#define SLIME_ZUNGE_CREEPSIZE       170
#define SLIME_ZUNGE_VALUE           ( ( 9 * LEVEL0_VALUE ) / 4 )
#define SLIME_ZUNGE_BAT_PWR         25000
#define SLIME_ZUNGE_DMGRADIUS       120
#define SLIME_ZUNGE_REPEAT          300
#define SLIME_ZUNGE_RANGE           300
#define SLIME_ZUNGE_DOT             0.79f // max angle = acos( SLIME_ZUNGE )

#define OVERMIND_BP                 0
#define OVERMIND_BT                 10000
#define OVERMIND_HEALTH             ABHM(1337000)
#define OVERMIND_REGEN              ABRM(6000)
#define OVERMIND_SPLASHDAMAGE       ABDM(15000)
#define OVERMIND_SPLASHRADIUS       300
#define OVERMIND_CREEPSIZE          120
#define OVERMIND_ATTACK_RANGE       150.0f
#define OVERMIND_ATTACK_REPEAT      1000
#define OVERMIND_VALUE              ( ( 27 * LEVEL0_VALUE ) / 4 )

/*
 * ALIEN misc
 *
 * ALIENSENSE_RANGE - the distance alien sense is useful for
 *
 */

#define ALIENSENSE_RANGE            1000.0f
#define REGEN_BOOST_RANGE           200.0f

#define ALIEN_POISON_TIME           5000
#define ALIEN_POISON_DMG            5000
#define ALIEN_POISON_DIVIDER        (1.0f/1.32f) //about 1.0/(time`th root of damage)

#define ALIEN_SPAWN_REPEAT_TIME     10000
#define ALIEN_SPAWN_PROTECTION_TIME 3000

#define ALIEN_HIVEMIND_LINK_TIME    150000 //amount of time aliens can survive without any eggs/overmind

#define ALIEN_REGEN_DAMAGE_TIME     2000 //msec since damage that regen starts again
#define ALIEN_REGEN_NOCREEP_MOD     (1.0f/2.0f) //regen off creep

#define ALIEN_MAX_FRAGS             49
#define ALIEN_MAX_CREDITS           (ALIEN_MAX_FRAGS*ALIEN_CREDITS_PER_KILL)
#define ALIEN_CREDITS_PER_KILL      200
#define ALIEN_TK_SUICIDE_PENALTY    150

/*
 * HUMAN weapons
 *
 * _REPEAT  - time between firings
 * _RELOAD  - time needed to reload
 * _PRICE   - amount in credits weapon costs
 *
 * HUMAN_WDMG_MODIFIER - overall damage modifier for coarse tuning
 *
 */

#define HUMAN_WDMG_MODIFIER         1.0f
#define HWDM(d)                     ((int)((float)d*HUMAN_WDMG_MODIFIER))

#define BLASTER_REPEAT              300
#define BLASTER_K_SCALE             1.0f
#define BLASTER_SPREAD              200
#define BLASTER_SPEED               1400
#define BLASTER_DMG                 HWDM(5000)
#define BLASTER_SIZE                5
#define BLASTER_LIFETIME            10000

#define RIFLE_CLIPSIZE              48
#define RIFLE_MAXCLIPS              5
#define RIFLE_REPEAT                100
#define RIFLE_K_SCALE               0.5f
#define RIFLE_RECOIL                24.0f
#define RIFLE_RELOAD                2000
#define RIFLE_PRICE                 0
#define RIFLE_SPREAD                0
#define RIFLE_DMG                   HWDM(4400)
#define RIFLE_REPEAT2               50
#define RIFLE_SPREAD2               0
#define RIFLE_DMG2                  HWDM(4400)
#define RIFLE_ALT_BURST_ROUNDS      4
#define RIFLE_ALT_BURST_DELAY       200

#define PAINSAW_PRICE               100
#define PAINSAW_REPEAT              75
#define PAINSAW_RELOAD              1000
#define PAINSAW_K_SCALE             1.0f
#define PAINSAW_DAMAGE              HWDM(11000)
#define PAINSAW_RANGE               64.0f
#define PAINSAW_WIDTH               0.0f
#define PAINSAW_HEIGHT              8.0f

#define GRENADE_PRICE               150
#define GRENADE_HEALTH              5
#define GRENADE_REPEAT              0
#define GRENADE_K_SCALE             1.0f
#define GRENADE_DAMAGE              HWDM(250000)
#define GRENADE_RANGE               250.0f
#define GRENADE_SPEED               400.0f
#define GRENADE_SIZE                3.0f        // missile bounding box
#define GRENADE_LIFETIME            5000
#define GRENADE_SPLASH_BATTLESUIT   0.75f // reduction of damage fraction for
                                          //grenade nade splash on battlesuits

#define SHOTGUN_PRICE               175
#define SHOTGUN_SHELLS              10
#define SHOTGUN_PELLETS             15 //used to sync server and client side
#define SHOTGUN_MAXCLIPS            3
#define SHOTGUN_REPEAT              1000
#define SHOTGUN_K_SCALE             1.0f
#define SHOTGUN_RECOIL1             256.0f
#define SHOTGUN_RECOIL2             512.0f
#define SHOTGUN_RELOAD              2000
#define SHOTGUN_SPREAD              700
#define SHOTGUN_DMG                 HWDM(4000)
#define SHOTGUN_RANGE               (8192 * 12)
#define SHOTGUN_CHOKE_REPEAT        1300
#define SHOTGUN_CHOKE_DMG           HWDM(5375)
#define SHOTGUN_CHOKE_DMG_CAP       HWDM(5000)
#define SHOTGUN_CHOKE_SPREAD        450
#define SHOTGUN_CHOKE_DMG_FALLOFF   2048.0f

#define LASGUN_PRICE                275
#define LASGUN_AMMO                 200
#define LASGUN_REPEAT               180
#define LASGUN_K_SCALE              1.0f
#define LASGUN_RELOAD               2000
#define LASGUN_DAMAGE               HWDM(9000)

#define MDRIVER_PRICE               400
#define MDRIVER_CLIPSIZE            5
#define MDRIVER_MAXCLIPS            4
#define MDRIVER_DMG                 HWDM(38000)
#define MDRIVER_REPEAT              1000
#define MDRIVER_RECOIL              30.0f
#define MDRIVER_K_SCALE             1.0f
#define MDRIVER_RELOAD              2000
#define MDRIVER_MAX_HITS            3

#define CHAINGUN_PRICE              450
#define CHAINGUN_BULLETS            300
#define CHAINGUN_REPEAT             80
#define CHAINGUN_K_SCALE            1.0f
#define CHAINGUN_RECOIL             32.0f
#define CHAINGUN_SPREAD             0
#define CHAINGUN_DMG                HWDM(5500)
#define CHAINGUN_RELOAD             2000
#define CHAINGUN_REPEAT2            150
#define CHAINGUN_SPREAD2            0
#define CHAINGUN_DMG2               HWDM(5500)

#define FLAMER_PRICE                450
#define FLAMER_GAS                  200
#define FLAMER_REPEAT               200
#define FLAMER_K_SCALE              2.0f
#define FLAMER_DMG                  HWDM(20000)
#define FLAMER_SPLASHDAMAGE         HWDM(10000)
#define FLAMER_RADIUS               50       // splash radius
#define FLAMER_SIZE                 15        // missile bounding box
#define FLAMER_LIFETIME             700
#define FLAMER_SPEED                500.0f
#define FLAMER_LAG                  0.65f    // the amount of player velocity that is added to the fireball
#define FLAMER_RELOAD               2000

#define PRIFLE_PRICE                500
#define PRIFLE_CLIPS                40
#define PRIFLE_MAXCLIPS             5
#define PRIFLE_REPEAT               100
#define PRIFLE_K_SCALE              1.0f
#define PRIFLE_RELOAD               2000
#define PRIFLE_DMG                  HWDM(9000)
#define PRIFLE_SPEED                1200
#define PRIFLE_SIZE                 5
#define PRIFLE_LIFETIME             10000

#define LCANNON_PRICE               900
#define LCANNON_AMMO                90
#define LCANNON_K_SCALE             1.0f
#define LCANNON_REPEAT              500
#define LCANNON_RELOAD              2000
#define LCANNON_DAMAGE              HWDM(265000)
#define LCANNON_RADIUS              150      // primary splash damage radius
#define LCANNON_SIZE                5        // missile bounding box radius
#define LCANNON_SECONDARY_DAMAGE    HWDM( LCANNON_DAMAGE / LCANNON_CHARGE_AMMO )
#define LCANNON_SECONDARY_RADIUS    75       // secondary splash damage radius
#define LCANNON_SECONDARY_SPEED     900
#define LCANNON_SECONDARY_RELOAD    2000
#define LCANNON_SECONDARY_REPEAT    500
#define LCANNON_SPEED_MIN           550
#define LCANNON_CHARGE_TIME_MAX     2000
#define LCANNON_CHARGE_TIME_MIN     500
#define LCANNON_CHARGE_TIME_WARN    ( LCANNON_CHARGE_TIME_MAX - ( 2000 / 3 ) )
#define LCANNON_CHARGE_AMMO         10       // ammo cost of a full charge shot
#define LCANNON_CHARGE_AMMO_REDUCE  0.25f    // rate at which luci ammo is reduced when using +attack2
#define LCANNON_SPLASH_LIGHTARMOUR  0.50f    // reduction of damage fraction for luci splash on light armour
#define LCANNON_SPLASH_HELMET       0.75f    // reduction of damage fraction for luci splash on helmets
#define LCANNON_SPLASH_BATTLESUIT   0.50f    // reduction of damage fraction for luci splash on battlesuits
#define LCANNON_LIFETIME            10000

#define PORTALGUN_PRICE             1250
#define PORTALGUN_AMMO              0
#define PORTALGUN_MAXCLIPS          0
#define PORTALGUN_ROUND_PRICE       0
#define PORTALGUN_REPEAT            200     // repeat period if a portal isn't created
#define PORTAL_CREATED_REPEAT       1500    // repeat period if a portal is created
#define PORTALGUN_RELOAD            2000
#define PORTALGUN_SPEED             8000
#define PORTALGUN_SIZE              25       // missile bounding box radius
#define PORTAL_LIFETIME             120000   // max time a portal can exist
#define PORTAL_HEALTH               750000
#define PORTALGUN_DAMAGE           HWDM(6000)

#define LAUNCHER_PRICE               600
#define LAUNCHER_AMMO                6
#define LAUNCHER_MAXCLIPS            0
#define LAUNCHER_ROUND_PRICE         400
#define LAUNCHER_REPEAT              1000
#define LAUNCHER_K_SCALE             1.0f
#define LAUNCHER_RELOAD              3000
#define LAUNCHER_DAMAGE              HWDM(220000)
#define LAUNCHER_RADIUS              175
#define LAUNCHER_SPEED               1200

#define LIGHTNING_PRICE                1000
#define LIGHTNING_AMMO                 600
#define LIGHTNING_MAXCLIPS             0
#define LIGHTNING_RELOAD               750
#define LIGHTNING_K_SCALE              0.0f
#define LIGHTNING_BOLT_DPS             HWDM(35000)
#define LIGHTNING_BOLT_CHARGE_TIME_MIN 150
#define LIGHTNING_BOLT_CHARGE_TIME_MAX 1250
#define LIGHTNING_BOLT_BEAM_DURATION   350
#define LIGHTNING_BOLT_RANGE_MAX       768
#define LIGHTNING_BOLT_NOGRND_DMG_MOD  0.50f
#define LIGHTNING_BALL_AMMO_USAGE      6
#define LIGHTNING_BALL_REPEAT          400
#define LIGHTNING_BALL_BURST_ROUNDS    5
#define LIGHTNING_BALL_BURST_DELAY     2000
#define LIGHTNING_BALL_LIFETIME        5000
#define LIGHTNING_BALL_DAMAGE          HWDM( 20000 )
#define LIGHTNING_BALL_SPLASH_DMG      HWDM( 20000 )
#define LIGHTNING_BALL_RADIUS1         75
#define LIGHTNING_BALL_RADIUS2         150
#define LIGHTNING_BALL_SIZE            5
#define LIGHTNING_BALL_SPEED           750
#define LIGHTNING_EMP_DAMAGE           HWDM( 0 )
#define LIGHTNING_EMP_SPLASH_DMG       HWDM( 0 )
#define LIGHTNING_EMP_RADIUS           0
#define LIGHTNING_EMP_SPEED            1500

#define HBUILD_PRICE                0
#define HBUILD_REPEAT               1000
#define HBUILD_HEALRATE             HBRM(18000)

/*
 * HUMAN upgrades
 */

#define LIGHTARMOUR_PRICE           70
#define LIGHTARMOUR_POISON_PROTECTION 1000
#define LIGHTARMOUR_PCLOUD_PROTECTION 1000

#define HELMET_PRICE                125
#define HELMET_RANGE                1000.0f
#define HELMET_POISON_PROTECTION    1000
#define HELMET_PCLOUD_PROTECTION    3500

#define MEDKIT_PRICE                0

#define BATTPACK_PRICE              150
#define BATTPACK_MODIFIER           1.5f //modifier for extra energy storage available

#define JETPACK_FULL_FUEL_PRICE     300
#define JETPACK_PRICE               ( 125 + JETPACK_FULL_FUEL_PRICE )
#define JETPACK_FLOAT_SPEED         175.0f //up movement speed
#define JETPACK_SINK_SPEED          192.0f //down movement speed
#define JETPACK_DISABLE_TIME        500 //time to disable the jetpack when player damaged
#define JETPACK_DISABLE_CHANCE      0.3f
#define JETPACK_FUEL_FULL           5000 //can't exceed 32767
#define JETPACK_FUEL_LOW            1000
#define JETPACK_FUEL_USAGE          16 //every 100ms
#define JETPACK_FUEL_MIN_START      150 // Minimum fuel required to start the jet
#define JETPACK_ACT_BOOST_FUEL_USE  ( JETPACK_FUEL_USAGE * 2 )
#define JETPACK_ACT_BOOST_TIME      750
#define JETPACK_ACT_BOOST_SPEED     300.0f
#define JETPACK_FUEL_RECHARGE       5 // every 100ms

#define BSUIT_PRICE                 1000
#define BSUIT_PRICE_USED            200
#define BSUIT_POISON_PROTECTION     3000
#define BSUIT_PCLOUD_PROTECTION     6000
#define BSUIT_MAX_ARMOR             750000
#define BSUIT_ARMOR_LOW             250000 // Value at which the armor indicator starts to flash

#define MEDKIT_POISON_IMMUNITY_TIME 0
#define MEDKIT_STARTUP_TIME         4000
#define MEDKIT_STARTUP_SPEED        5

/*
 * HUMAN buildables
 *
 * _BP            - build points required for this buildable
 * _BT            - build time required for this buildable
 * _SPLASHDAMGE   - the amount of damage caused by this buildable when it blows up
 * _SPLASHRADIUS  - the radius around which it does this damage
 *
 * REACTOR_BASESIZE - the maximum distance a buildable can be from a reactor
 * REPEATER_BASESIZE - the maximum distance a buildable can be from a repeater
 * HUMAN_BHLTH_MODIFIER - overall health modifier for coarse tuning
 * HUMAN_BDMG_MODIFIER  - overall damage modifier for coarse tuning
 * ALIEN_BMAXHEALTH_DECAY - the amount the max health is reduced by from repair
 *
 */

#define HUMAN_BHLTH_MODIFIER        1.25f
#define HBHM(h)                     ((int)((float)h*HUMAN_BHLTH_MODIFIER))
#define HUMAN_BDMG_MODIFIER         1.0f
#define HBDM(d)                     ((int)((float)d*ALIEN_WDMG_MODIFIER))

#define HUMAN_BREGEN_MODIFIER       (1.0f)
#define HBRM(r)                     ((int)((float)r*HUMAN_BREGEN_MODIFIER))

#define HUMAN_BMAXHEALTH_DECAY(h)  ( h / 2 )

#define REACTOR_BASESIZE            1000
#define REPEATER_BASESIZE           500
#define HUMAN_DETONATION_DELAY      2500

#define HSPAWN_BP                   10
#define HSPAWN_BT                   10000
#define HSPAWN_HEALTH               HBHM(310000)
#define HSPAWN_SPLASHDAMAGE         HBDM(50000)
#define HSPAWN_SPLASHRADIUS         100
#define HSPAWN_VALUE                ( ( 3 * ALIEN_CREDITS_PER_KILL ) / 2 )

#define HTELEPORTER_BP              10
#define HTELEPORTER_BT              10000
#define HTELEPORTER_HEALTH          HBHM(310000)
#define HTELEPORTER_SPLASHDAMAGE    HBDM(50000)
#define HTELEPORTER_SPLASHRADIUS    100
#define HTELEPORTER_VALUE           ( ( 3 * ALIEN_CREDITS_PER_KILL ) / 2 )
#define HTELEPORTER_COOLDOWN_TIME   2000
#define HTELEPORTER_BAT_PWR         30000  // amount of time teleporter's can remained powered without
                                           // an external power source

#define MEDISTAT_BP                 8
#define MEDISTAT_BT                 10000
#define MEDISTAT_HEALTH             HBHM(190000)
#define MEDISTAT_REPEAT             100
#define MEDISTAT_SPLASHDAMAGE       HBDM(50000)
#define MEDISTAT_SPLASHRADIUS       100
#define MEDISTAT_VALUE              ( ( 3 * ALIEN_CREDITS_PER_KILL ) / 2 )
#define MEDISTAT_BAT_PWR            25000

#define MGTURRET_BP                 8
#define MGTURRET_BT                 10000
#define MGTURRET_HEALTH             HBHM(190000)
#define MGTURRET_SPLASHDAMAGE       HBDM(100000)
#define MGTURRET_SPLASHRADIUS       100
#define MGTURRET_ANGULARSPEED       14
#define MGTURRET_ACCURACY_TO_FIRE   10
#define MGTURRET_VERTICALCAP        45  // +/- maximum pitch
#define MGTURRET_REPEAT             140  // repeat after full spin up
#define MGTURRET_REPEAT_START       300 // the repeat at the start of spinup
#define MGTURRET_NEXTTHINK          50
#define MGTURRET_K_SCALE            1.0f
#define MGTURRET_RANGE              500.0f
#define MGTURRET_SPREAD             200
#define MGTURRET_DMG                HBDM(5500)
#define MGTURRET_SPINUP_TIME        3500 // time to increase the fire rate to full speed
#define MGTURRET_SPINDOWN_TIME      4000 // time it takes a ret to spindown from full speed to complete stop
#define MGTURRET_SPINUP_SKIPFIRE    400 // the initial shot skips below this spinup value
#define MGTURRET_VALUE              ( ( 3 * ALIEN_CREDITS_PER_KILL ) / 4 )
#define MGTURRET_BAT_PWR            25000
#define MGTURRET_DCC_ANGULARSPEED   18
#define MGTURRET_GRAB_ANGULARSPEED  0

#define TESLAGEN_BP                 10
#define TESLAGEN_BT                 15000
#define TESLAGEN_HEALTH             HBHM(220000)
#define TESLAGEN_SPLASHDAMAGE       HBDM(50000)
#define TESLAGEN_SPLASHRADIUS       100
#define TESLAGEN_REPEAT             250
#define TESLAGEN_K_SCALE            4.0f
#define TESLAGEN_RANGE              250
#define TESLAGEN_DMG                HBDM(6000)
#define TESLAGEN_VALUE              ( ( 3 * ALIEN_CREDITS_PER_KILL ) / 4 )
#define TESLAGEN_BAT_PWR            25000

#define DC_BP                       8
#define DC_BT                       10000
#define DC_HEALTH                   HBHM(190000)
#define DC_SPLASHDAMAGE             HBDM(50000)
#define DC_SPLASHRADIUS             100
#define DC_ATTACK_PERIOD            10000 // how often to spam "under attack"
#define DC_HEALRATE                 4000
#define DC_RANGE                    10000
#define DC_VALUE                    ( ( 3 * ALIEN_CREDITS_PER_KILL ) / 4 )
#define DC_BAT_PWR                  25000

#define ARMOURY_BP                  10
#define ARMOURY_BT                  10000
#define ARMOURY_HEALTH              HBHM(420000)
#define ARMOURY_SPLASHDAMAGE        HBDM(50000)
#define ARMOURY_SPLASHRADIUS        100
#define ARMOURY_VALUE               ( ( 9 * ALIEN_CREDITS_PER_KILL ) / 4 )
#define ARMOURY_BAT_PWR             25000

#define REACTOR_BP                  0
#define REACTOR_BT                  20000
#define REACTOR_HEALTH              HBHM(1000000)
#define REACTOR_SPLASHDAMAGE        HBDM(200000)
#define REACTOR_SPLASHRADIUS        300
#define REACTOR_ATTACK_RANGE        100.0f
#define REACTOR_ATTACK_REPEAT       1000
#define REACTOR_ATTACK_DAMAGE       HBDM(40000)
#define REACTOR_ATTACK_DCC_REPEAT   1000
#define REACTOR_ATTACK_DCC_RANGE    150.0f
#define REACTOR_ATTACK_DCC_DAMAGE   HBDM(40000)
#define REACTOR_VALUE               ( 3 * ALIEN_CREDITS_PER_KILL )

#define REPEATER_BP                 4
#define REPEATER_BT                 10000
#define REPEATER_HEALTH             HBHM(250000)
#define REPEATER_SPLASHDAMAGE       HBDM(50000)
#define REPEATER_SPLASHRADIUS       100
#define REPEATER_VALUE              ( ( 3 * ALIEN_CREDITS_PER_KILL ) / 2 )

/*
 * HUMAN misc
 */

#define HUMAN_SPRINT_MODIFIER       1.2f
#define HUMAN_JOG_MODIFIER          1.0f
#define HUMAN_BACK_MODIFIER         0.8f
#define HUMAN_SIDE_MODIFIER         0.9f
#define HUMAN_DODGE_SIDE_MODIFIER   2.9f
#define HUMAN_DODGE_SLOWED_MODIFIER 0.9f
#define HUMAN_DODGE_UP_MODIFIER     0.5f
#define HUMAN_DODGE_TIMEOUT         500
#define HUMAN_LAND_FRICTION         3.0f

#define STAMINA_STOP_RESTORE        30
#define STAMINA_WALK_RESTORE        15
#define STAMINA_RUN_RESTORE         7
#define STAMINA_MEDISTAT_RESTORE    30 // stacked on STOP or WALK
#define STAMINA_SPRINT_TAKE         6
#define STAMINA_JUMP_TAKE           264
#define STAMINA_JUMP_RESTORE_DELAY  1000 // amount of time after a jump before stamina can be restored off of the ground
#define STAMINA_DODGE_TAKE          250
#define STAMINA_MAX                 1000
#define STAMINA_BREATHING_LEVEL     0
#define STAMINA_SLOW_LEVEL          -500
#define STAMINA_BLACKOUT_LEVEL      -800

#define HUMAN_SPAWN_REPEAT_TIME     10000
#define HUMAN_SPAWN_PROTECTION_TIME 5000
#define HUMAN_REGEN_DAMAGE_TIME     3000 //msec since damage before dcc repairs

#define HUMAN_LIFE_SUPPORT_TIME     150000 //amount of time humans can survive without any telenodes/reactor

#define HUMAN_MAX_CREDITS           ALIEN_MAX_CREDITS
#define HUMAN_TK_SUICIDE_PENALTY    150

#define HUMAN_BUILDER_SCOREINC      50       // builders receive this many points every 10 seconds
#define ALIEN_BUILDER_SCOREINC      AVM(100)  // builders receive this many points every 10 seconds

/*
 * Misc
 */

#define MIN_FALL_DISTANCE           30.0f //the fall distance at which fall damage kicks in
#define MAX_FALL_DISTANCE           120.0f //the fall distance at which maximum damage is dealt
#define AVG_FALL_DISTANCE           ((MIN_FALL_DISTANCE+MAX_FALL_DISTANCE)/2.0f)

#define TELEPORT_PROTECTION_TIME    3000 // amount of time one is protected fromtargting after teleporting

#define BUNNY_HOP_DELAY             300  // minimum miliseconds delay between bunny hops
#define DOUBLE_JUMP_MAX_TIME        400  // maximum miliseconds after a jump that
                                         // a higher "double jump" can be made

#define CHARGE_STAMINA_MAX          10000
#define CHARGE_STAMINA_RESTORE      15

#define DEFAULT_FREEKILL_PERIOD     "20" //seconds
#define FREEKILL_ALIEN              ALIEN_CREDITS_PER_KILL
#define FREEKILL_HUMAN              LEVEL0_VALUE

#define DEFAULT_ALIEN_BUILDPOINTS          "250"
#define DEFAULT_ALIEN_BUILDPOINTS_RESERVE  "200"
#define DEFAULT_ALIEN_QUEUE_TIME           "2000"
#define DEFAULT_ALIEN_STAGE2_THRESH        "12000"
#define DEFAULT_ALIEN_STAGE3_THRESH        "24000"
#define DEFAULT_ALIEN_MAX_STAGE            "2"
#define DEFAULT_HUMAN_BUILDPOINTS          "250"
#define DEFAULT_HUMAN_BUILDPOINTS_RESERVE  "200"
#define DEFAULT_HUMAN_QUEUE_TIME           "3000"
#define DEFAULT_HUMAN_REPEATER_BUILDPOINTS "20"
#define DEFAULT_HUMAN_REPEATER_QUEUE_TIME  "2000"
#define DEFAULT_HUMAN_REPEATER_MAX_ZONES   "500"
#define DEFAULT_HUMAN_STAGE2_THRESH        "6000"
#define DEFAULT_HUMAN_STAGE3_THRESH        "12000"
#define DEFAULT_HUMAN_MAX_STAGE            "2"

#define DAMAGE_FRACTION_FOR_KILL    0.5f //how much damage players (versus structures) need to
                                         //do to increment the stage kill counters
                                         
#define MAXIMUM_BUILD_TIME          20000 // used for pie timer
