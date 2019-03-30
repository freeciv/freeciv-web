/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__UNITTYPE_H
#define FC__UNITTYPE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "bitvector.h"
#include "shared.h"

/* common */
#include "fc_types.h"
#include "name_translation.h"

struct astring;         /* Actually defined in "utility/astring.h". */
struct strvec;          /* Actually defined in "utility/string_vector.h". */

struct ai_type;

/* U_LAST is a value which is guaranteed to be larger than all
 * actual Unit_type_id values. It is used as a flag value;
 * it can also be used for fixed allocations to ensure able
 * to hold full number of unit types.
 * Used in the network protocol. */
#define U_LAST MAX_NUM_UNITS

/* The largest distance a ruleset can allow a unit to paradrop.
 *
 * Remember to make sure that the field type of PACKET_RULESET_UNIT's
 * paratroopers_range field can transfer the new maximum if you increase
 * it.
 *
 * The top value is reserved in case a future Freeciv version wants to
 * implement "no maximum range". It could be used to signal that the unit
 * can paradrop anywhere. Note that the value below it is high enough to
 * give the same effect on all maps inside the current size limits.
 * (No map side can be larger than MAP_MAX_LINEAR_SIZE)
 */
#define UNIT_MAX_PARADROP_RANGE (65535 - 1)

/* Used in the network protocol. */
#define SPECENUM_NAME unit_class_flag_id
#define SPECENUM_VALUE0 UCF_TERRAIN_SPEED
#define SPECENUM_VALUE0NAME N_("?uclassflag:TerrainSpeed")
#define SPECENUM_VALUE1 UCF_TERRAIN_DEFENSE
#define SPECENUM_VALUE1NAME N_("?uclassflag:TerrainDefense")
#define SPECENUM_VALUE2 UCF_DAMAGE_SLOWS
#define SPECENUM_VALUE2NAME N_("?uclassflag:DamageSlows")
/* Can occupy enemy cities */
#define SPECENUM_VALUE3 UCF_CAN_OCCUPY_CITY
#define SPECENUM_VALUE3NAME N_("?uclassflag:CanOccupyCity")
#define SPECENUM_VALUE4 UCF_MISSILE
#define SPECENUM_VALUE4NAME N_("?uclassflag:Missile")
#define SPECENUM_VALUE5 UCF_BUILD_ANYWHERE
#define SPECENUM_VALUE5NAME N_("?uclassflag:BuildAnywhere")
#define SPECENUM_VALUE6 UCF_UNREACHABLE
#define SPECENUM_VALUE6NAME N_("?uclassflag:Unreachable")
/* Can collect ransom from barbarian leader */
#define SPECENUM_VALUE7 UCF_COLLECT_RANSOM
#define SPECENUM_VALUE7NAME N_("?uclassflag:CollectRansom")
/* Is subject to ZOC */
#define SPECENUM_VALUE8 UCF_ZOC
#define SPECENUM_VALUE8NAME N_("?uclassflag:ZOC")
/* Can fortify on land squares */
#define SPECENUM_VALUE9 UCF_CAN_FORTIFY
#define SPECENUM_VALUE9NAME N_("?uclassflag:CanFortify")
#define SPECENUM_VALUE10 UCF_CAN_PILLAGE
#define SPECENUM_VALUE10NAME N_("?uclassflag:CanPillage")
/* Cities can still work tile when enemy unit on it */
#define SPECENUM_VALUE11 UCF_DOESNT_OCCUPY_TILE
#define SPECENUM_VALUE11NAME N_("?uclassflag:DoesntOccupyTile")
/* Can attack against units on non-native tiles */
#define SPECENUM_VALUE12 UCF_ATTACK_NON_NATIVE
#define SPECENUM_VALUE12NAME N_("?uclassflag:AttackNonNative")
/* Kills citizens upon successful attack against a city */
#define SPECENUM_VALUE13 UCF_KILLCITIZEN
#define SPECENUM_VALUE13NAME N_("?uclassflag:KillCitizen")

#define SPECENUM_VALUE14 UCF_USER_FLAG_1
#define SPECENUM_VALUE15 UCF_USER_FLAG_2
#define SPECENUM_VALUE16 UCF_USER_FLAG_3
#define SPECENUM_VALUE17 UCF_USER_FLAG_4
#define SPECENUM_VALUE18 UCF_USER_FLAG_5
#define SPECENUM_VALUE19 UCF_USER_FLAG_6
#define SPECENUM_VALUE20 UCF_USER_FLAG_7
#define SPECENUM_VALUE21 UCF_USER_FLAG_8

/* keep this last */
#define SPECENUM_COUNT UCF_COUNT
#define SPECENUM_NAMEOVERRIDE
#define SPECENUM_BITVECTOR bv_unit_class_flags
#include "specenum_gen.h"

#define UCF_LAST_USER_FLAG UCF_USER_FLAG_8
#define MAX_NUM_USER_UCLASS_FLAGS (UCF_LAST_USER_FLAG                     \
                                   - UCF_USER_FLAG_1 + 1)

/* Used in savegame processing and clients. */
#define SPECENUM_NAME unit_move_type
#define SPECENUM_VALUE0 UMT_LAND
#define SPECENUM_VALUE0NAME "Land"
#define SPECENUM_VALUE1 UMT_SEA
#define SPECENUM_VALUE1NAME "Sea"
#define SPECENUM_VALUE2 UMT_BOTH
#define SPECENUM_VALUE2NAME "Both"
#include "specenum_gen.h"

/* Used in the network protocol. */
BV_DEFINE(bv_unit_classes, UCL_LAST);

enum hut_behavior { HUT_NORMAL, HUT_NOTHING, HUT_FRIGHTEN };

enum move_level { MOVE_NONE, MOVE_PARTIAL, MOVE_FULL };

struct extra_type_list;
struct unit_class_list;

struct unit_class {
  Unit_Class_id item_number;
  struct name_translation name;
  bool ruledit_disabled;
  enum unit_move_type move_type;
  int min_speed;           /* Minimum speed after damage and effects */
  int hp_loss_pct;         /* Percentage of hitpoints lost each turn not in city or airbase */
  int non_native_def_pct;
  enum hut_behavior hut_behavior;
  bv_unit_class_flags flags;

  struct strvec *helptext;

  struct {
    enum move_level land_move;
    enum move_level sea_move;
  } adv;

  struct {
    struct extra_type_list *refuel_bases;
    struct extra_type_list *native_tile_extras;
    struct extra_type_list *bonus_roads;
    struct unit_class_list *subset_movers;
  } cache;
};

/* Unit "special effects" flags:
 * Note this is now an enumerated type, and not power-of-two integers
 * for bits, though unit_type.flags is still a bitfield, and code
 * which uses unit_has_type_flag() without twiddling bits is unchanged.
 * (It is easier to go from i to (1<<i) than the reverse.)
 * See data/default/units.ruleset for documentation of their effects.
 * Change the array *flag_names[] in unittype.c accordingly.
 * Used in the network protocol.
 */
#define SPECENUM_NAME unit_type_flag_id
/* Cannot fortify even if class can */
#define SPECENUM_VALUE0 UTYF_CANT_FORTIFY
/* TRANS: this and following strings are 'unit type flags', which may rarely
 * be presented to the player in ruleset help text */
#define SPECENUM_VALUE0NAME N_("?unitflag:Cant_Fortify")
/* Unit has no ZOC */
#define SPECENUM_VALUE1 UTYF_NOZOC
#define SPECENUM_VALUE1NAME N_("?unitflag:HasNoZOC")
#define SPECENUM_VALUE2 UTYF_IGZOC
/* TRANS: unit type flag (rarely shown): "ignore zones of control" */
#define SPECENUM_VALUE2NAME N_("?unitflag:IgZOC")
#define SPECENUM_VALUE3 UTYF_CIVILIAN
#define SPECENUM_VALUE3NAME N_("?unitflag:NonMil")
#define SPECENUM_VALUE4 UTYF_IGTER
/* TRANS: unit type flag (rarely shown): "ignore terrain" */
#define SPECENUM_VALUE4NAME N_("?unitflag:IgTer")
#define SPECENUM_VALUE5 UTYF_ONEATTACK
#define SPECENUM_VALUE5NAME N_("?unitflag:OneAttack")
#define SPECENUM_VALUE6 UTYF_FIELDUNIT
#define SPECENUM_VALUE6NAME N_("?unitflag:FieldUnit")
/* autoattack: a unit will choose to attack this unit even if defending
 * against it has better odds. */
#define SPECENUM_VALUE7 UTYF_PROVOKING
#define SPECENUM_VALUE7NAME N_("?unitflag:Provoking")
/* Invisible except when adjacent (Submarine) */
#define SPECENUM_VALUE8 UTYF_RESERVED_1
#define SPECENUM_VALUE8NAME "RESERVED"
/* Does not include ability to found cities */
#define SPECENUM_VALUE9 UTYF_SETTLERS
#define SPECENUM_VALUE9NAME N_("?unitflag:Settlers")
#define SPECENUM_VALUE10 UTYF_DIPLOMAT
#define SPECENUM_VALUE10NAME N_("?unitflag:Diplomat")
/* Can't leave the coast */
#define SPECENUM_VALUE11 UTYF_COAST_STRICT
#define SPECENUM_VALUE11NAME N_("?unitflag:CoastStrict")
/* Can 'refuel' at coast - meaningless if fuel value not set */
#define SPECENUM_VALUE12 UTYF_COAST
#define SPECENUM_VALUE12NAME N_("?unitflag:Coast")
/* upkeep can switch from shield to gold */
#define SPECENUM_VALUE13 UTYF_SHIELD2GOLD
#define SPECENUM_VALUE13NAME N_("?unitflag:Shield2Gold")
/* Strong in diplomatic battles. */
#define SPECENUM_VALUE14 UTYF_SPY
#define SPECENUM_VALUE14NAME N_("?unitflag:Spy")
/* Cannot attack vs non-native tiles even if class can */
#define SPECENUM_VALUE15 UTYF_ONLY_NATIVE_ATTACK
#define SPECENUM_VALUE15NAME N_("?unitflag:Only_Native_Attack")
/* Only Fundamentalist government can build these units */
#define SPECENUM_VALUE16 UTYF_FANATIC
#define SPECENUM_VALUE16NAME N_("?unitflag:Fanatic")
/* Losing this unit means losing the game */
#define SPECENUM_VALUE17 UTYF_GAMELOSS
#define SPECENUM_VALUE17NAME N_("?unitflag:GameLoss")
/* A player can only have one unit of this type */
#define SPECENUM_VALUE18 UTYF_UNIQUE
#define SPECENUM_VALUE18NAME N_("?unitflag:Unique")
/* When a transport containing this unit disappears the game will try to
 * rescue units with this flag before it tries to rescue units without
 * it. */
#define SPECENUM_VALUE19 UTYF_EVAC_FIRST
#define SPECENUM_VALUE19NAME N_("?unitflag:EvacuateFirst")
/* Always wins diplomatic contests */
#define SPECENUM_VALUE20 UTYF_SUPERSPY
#define SPECENUM_VALUE20NAME N_("?unitflag:SuperSpy")
/* Has no homecity */
#define SPECENUM_VALUE21 UTYF_NOHOME
#define SPECENUM_VALUE21NAME N_("?unitflag:NoHome")
/* Cannot increase veteran level */
#define SPECENUM_VALUE22 UTYF_NO_VETERAN
#define SPECENUM_VALUE22NAME N_("?unitflag:NoVeteran")
/* Gets double firepower against cities */
#define SPECENUM_VALUE23 UTYF_CITYBUSTER
#define SPECENUM_VALUE23NAME N_("?unitflag:CityBuster")
/* Unit cannot be built (barb leader etc) */
#define SPECENUM_VALUE24 UTYF_NOBUILD
#define SPECENUM_VALUE24NAME N_("?unitflag:NoBuild")
/* Firepower set to 1 when EFT_DEFEND_BONUS applies
 * (for example, land unit attacking city with walls) */
#define SPECENUM_VALUE25 UTYF_BADWALLATTACKER
#define SPECENUM_VALUE25NAME N_("?unitflag:BadWallAttacker")
/* Firepower set to 1 and attackers x2 when in city */
#define SPECENUM_VALUE26 UTYF_BADCITYDEFENDER
#define SPECENUM_VALUE26NAME N_("?unitflag:BadCityDefender")
/* Only barbarians can build this unit */
#define SPECENUM_VALUE27 UTYF_BARBARIAN_ONLY
#define SPECENUM_VALUE27NAME N_("?unitflag:BarbarianOnly")
/* Unit won't lose all its movement when moving from non-native terrain to
 * native terrain even if slow_invasions is turned on. */
#define SPECENUM_VALUE28 UTYF_BEACH_LANDER
#define SPECENUM_VALUE28NAME N_("?unitflag:BeachLander")
/* Unit can't be built in scenarios where founding new cities is prevented. */
#define SPECENUM_VALUE29 UTYF_NEWCITY_GAMES_ONLY
#define SPECENUM_VALUE29NAME N_("?unitflag:NewCityGamesOnly")
/* Can escape when killstack occours */
#define SPECENUM_VALUE30 UTYF_CANESCAPE
#define SPECENUM_VALUE30NAME N_("?unitflag:CanEscape")
/* Can kill escaping units */
#define SPECENUM_VALUE31 UTYF_CANKILLESCAPING
#define SPECENUM_VALUE31NAME N_("?unitflag:CanKillEscaping")

#define SPECENUM_VALUE32 UTYF_USER_FLAG_1
#define SPECENUM_VALUE33 UTYF_USER_FLAG_2
#define SPECENUM_VALUE34 UTYF_USER_FLAG_3
#define SPECENUM_VALUE35 UTYF_USER_FLAG_4
#define SPECENUM_VALUE36 UTYF_USER_FLAG_5
#define SPECENUM_VALUE37 UTYF_USER_FLAG_6
#define SPECENUM_VALUE38 UTYF_USER_FLAG_7
#define SPECENUM_VALUE39 UTYF_USER_FLAG_8
#define SPECENUM_VALUE40 UTYF_USER_FLAG_9
#define SPECENUM_VALUE41 UTYF_USER_FLAG_10
#define SPECENUM_VALUE42 UTYF_USER_FLAG_11
#define SPECENUM_VALUE43 UTYF_USER_FLAG_12
#define SPECENUM_VALUE44 UTYF_USER_FLAG_13
#define SPECENUM_VALUE45 UTYF_USER_FLAG_14
#define SPECENUM_VALUE46 UTYF_USER_FLAG_15
#define SPECENUM_VALUE47 UTYF_USER_FLAG_16
#define SPECENUM_VALUE48 UTYF_USER_FLAG_17
#define SPECENUM_VALUE49 UTYF_USER_FLAG_18
#define SPECENUM_VALUE50 UTYF_USER_FLAG_19
#define SPECENUM_VALUE51 UTYF_USER_FLAG_20
#define SPECENUM_VALUE52 UTYF_USER_FLAG_21
#define SPECENUM_VALUE53 UTYF_USER_FLAG_22
#define SPECENUM_VALUE54 UTYF_USER_FLAG_23
#define SPECENUM_VALUE55 UTYF_USER_FLAG_24
#define SPECENUM_VALUE56 UTYF_USER_FLAG_25
#define SPECENUM_VALUE57 UTYF_USER_FLAG_26
#define SPECENUM_VALUE58 UTYF_USER_FLAG_27
#define SPECENUM_VALUE59 UTYF_USER_FLAG_28
#define SPECENUM_VALUE60 UTYF_USER_FLAG_29
#define SPECENUM_VALUE61 UTYF_USER_FLAG_30
#define SPECENUM_VALUE62 UTYF_USER_FLAG_31
#define SPECENUM_VALUE63 UTYF_USER_FLAG_32
#define SPECENUM_VALUE64 UTYF_USER_FLAG_33
#define SPECENUM_VALUE65 UTYF_USER_FLAG_34
#define SPECENUM_VALUE66 UTYF_USER_FLAG_35
#define SPECENUM_VALUE67 UTYF_USER_FLAG_36
#define SPECENUM_VALUE68 UTYF_USER_FLAG_37
#define SPECENUM_VALUE69 UTYF_USER_FLAG_38
#define SPECENUM_VALUE70 UTYF_USER_FLAG_39
#define SPECENUM_VALUE71 UTYF_USER_FLAG_40
#define SPECENUM_VALUE72 UTYF_USER_FLAG_41
#define SPECENUM_VALUE73 UTYF_USER_FLAG_42
#define SPECENUM_VALUE74 UTYF_USER_FLAG_43
#define SPECENUM_VALUE75 UTYF_USER_FLAG_44
#define SPECENUM_VALUE76 UTYF_USER_FLAG_45
/* Note that first role must have value next to last flag */

#define UTYF_LAST_USER_FLAG UTYF_USER_FLAG_45
#define MAX_NUM_USER_UNIT_FLAGS (UTYF_LAST_USER_FLAG - UTYF_USER_FLAG_1 + 1)
#define SPECENUM_NAMEOVERRIDE
#define SPECENUM_BITVECTOR bv_unit_type_flags
#include "specenum_gen.h"


/* Unit "roles": these are similar to unit flags but differ in that
   they don't represent intrinsic properties or abilities of units,
   but determine which units are used (mainly by the server or AI)
   in various circumstances, or "roles".
   Note that in some cases flags can act as roles, eg, we don't need
   a role for "settlers", because we can just use UTYF_SETTLERS.
   (Now have to consider ACTION_FOUND_CITY too)
   So we make sure flag values and role values are distinct,
   so some functions can use them interchangably.
   See data/classic/units.ruleset for documentation of their effects.
*/
#define L_FIRST (UTYF_LAST_USER_FLAG + 1)

#define SPECENUM_NAME unit_role_id
/* is built first when city established */
#define SPECENUM_VALUE77 L_FIRSTBUILD
#define SPECENUM_VALUE77NAME N_("?unitflag:FirstBuild")
/* initial explorer unit */
#define SPECENUM_VALUE78 L_EXPLORER
#define SPECENUM_VALUE78NAME N_("?unitflag:Explorer")
/* can be found in hut */
#define SPECENUM_VALUE79 L_HUT
#define SPECENUM_VALUE79NAME N_("?unitflag:Hut")
/* can be found in hut, global tech required */
#define SPECENUM_VALUE80 L_HUT_TECH
#define SPECENUM_VALUE80NAME N_("?unitflag:HutTech")
/* is created in Partisan circumstances */
#define SPECENUM_VALUE81 L_PARTISAN
#define SPECENUM_VALUE81NAME N_("?unitflag:Partisan")
/* ok on defense (AI) */
#define SPECENUM_VALUE82 L_DEFEND_OK
#define SPECENUM_VALUE82NAME N_("?unitflag:DefendOk")
/* primary purpose is defense (AI) */
#define SPECENUM_VALUE83 L_DEFEND_GOOD
#define SPECENUM_VALUE83NAME N_("?unitflag:DefendGood")
/* is useful for ferrying (AI) */
#define SPECENUM_VALUE84 L_FERRYBOAT
#define SPECENUM_VALUE84NAME N_("?unitflag:FerryBoat")
/* barbarians unit, land only */
#define SPECENUM_VALUE85 L_BARBARIAN
#define SPECENUM_VALUE85NAME N_("?unitflag:Barbarian")
/* barbarians unit, global tech required */
#define SPECENUM_VALUE86 L_BARBARIAN_TECH
#define SPECENUM_VALUE86NAME N_("?unitflag:BarbarianTech")
/* barbarian boat */
#define SPECENUM_VALUE87 L_BARBARIAN_BOAT
#define SPECENUM_VALUE87NAME N_("?unitflag:BarbarianBoat")
/* what barbarians should build */
#define SPECENUM_VALUE88 L_BARBARIAN_BUILD
#define SPECENUM_VALUE88NAME N_("BarbarianBuild")
/* barbarians build when global tech */
#define SPECENUM_VALUE89 L_BARBARIAN_BUILD_TECH
#define SPECENUM_VALUE89NAME N_("?unitflag:BarbarianBuildTech")
/* barbarian leader */
#define SPECENUM_VALUE90 L_BARBARIAN_LEADER
#define SPECENUM_VALUE90NAME N_("?unitflag:BarbarianLeader")
/* sea raider unit */
#define SPECENUM_VALUE91 L_BARBARIAN_SEA
#define SPECENUM_VALUE91NAME N_("?unitflag:BarbarianSea")
/* sea raider unit, global tech required */
#define SPECENUM_VALUE92 L_BARBARIAN_SEA_TECH
#define SPECENUM_VALUE92NAME N_("?unitflag:BarbarianSeaTech")
/* Startunit: Cities */
#define SPECENUM_VALUE93 L_START_CITIES
#define SPECENUM_VALUE93NAME N_("?unitflag:CitiesStartunit")
/* Startunit: Worker */
#define SPECENUM_VALUE94 L_START_WORKER
#define SPECENUM_VALUE94NAME N_("?unitflag:WorkerStartunit")
/* Startunit: Explorer */
#define SPECENUM_VALUE95 L_START_EXPLORER
#define SPECENUM_VALUE95NAME N_("?unitflag:ExplorerStartunit")
/* Startunit: King */
#define SPECENUM_VALUE96 L_START_KING
#define SPECENUM_VALUE96NAME N_("?unitflag:KingStartunit")
/* Startunit: Diplomat */
#define SPECENUM_VALUE97 L_START_DIPLOMAT
#define SPECENUM_VALUE97NAME N_("?unitflag:DiplomatStartunit")
/* Startunit: Ferryboat */
#define SPECENUM_VALUE98 L_START_FERRY
#define SPECENUM_VALUE98NAME N_("?unitflag:FerryStartunit")
/* Startunit: DefendOk */
#define SPECENUM_VALUE99 L_START_DEFEND_OK
#define SPECENUM_VALUE99NAME N_("?unitflag:DefendOkStartunit")
/* Startunit: DefendGood */
#define SPECENUM_VALUE100 L_START_DEFEND_GOOD
#define SPECENUM_VALUE100NAME N_("?unitflag:DefendGoodStartunit")
/* Startunit: AttackFast */
#define SPECENUM_VALUE101 L_START_ATTACK_FAST
#define SPECENUM_VALUE101NAME N_("?unitflag:AttackFastStartunit")
/* Startunit: AttackStrong */
#define SPECENUM_VALUE102 L_START_ATTACK_STRONG
#define SPECENUM_VALUE102NAME N_("?unitflag:AttackStrongStartunit")
/* AI hunter type unit */
#define SPECENUM_VALUE103 L_HUNTER
#define SPECENUM_VALUE103NAME N_("?unitflag:Hunter")
/* can improve terrain */
#define SPECENUM_VALUE104 L_SETTLERS
#define SPECENUM_VALUE104NAME N_("?unitflag:Settlers")
#define L_LAST (L_SETTLERS + 1)

#include "specenum_gen.h"

#define L_MAX 64 /* Used in the network protocol. */

FC_STATIC_ASSERT(L_LAST - L_FIRST <= L_MAX, too_many_unit_roles);

/* Used in the network protocol. */
BV_DEFINE(bv_unit_type_roles, L_MAX);

/* Used in the network protocol. */
#define SPECENUM_NAME combat_bonus_type
#define SPECENUM_VALUE0 CBONUS_DEFENSE_MULTIPLIER
#define SPECENUM_VALUE0NAME "DefenseMultiplier"
#define SPECENUM_VALUE1 CBONUS_DEFENSE_DIVIDER
#define SPECENUM_VALUE1NAME "DefenseDivider"
#define SPECENUM_VALUE2 CBONUS_FIREPOWER1
#define SPECENUM_VALUE2NAME "Firepower1"
#include "specenum_gen.h"

struct combat_bonus {
  enum unit_type_flag_id  flag;
  enum combat_bonus_type  type;
  int                     value;

  /* Not listed in the help text. */
  bool                    quiet;
};

/* get 'struct combat_bonus_list' and related functions: */
#define SPECLIST_TAG combat_bonus
#define SPECLIST_TYPE struct combat_bonus
#include "speclist.h"

#define combat_bonus_list_iterate(bonuslist, pbonus) \
    TYPED_LIST_ITERATE(struct combat_bonus, bonuslist, pbonus)
#define combat_bonus_list_iterate_end LIST_ITERATE_END

BV_DEFINE(bv_unit_types, U_LAST);

struct veteran_level {
  struct name_translation name; /* level/rank name */
  int power_fact; /* combat/work speed/diplomatic power factor (in %) */
  int move_bonus;
  int raise_chance; /* server only */
  int work_raise_chance; /* server only */
};

struct veteran_system {
  int levels;

  struct veteran_level *definitions;
};

struct unit_type {
  Unit_type_id item_number;
  struct name_translation name;
  bool ruledit_disabled;              /* Does not really exist - hole in improvments array */
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  char sound_move[MAX_LEN_NAME];
  char sound_move_alt[MAX_LEN_NAME];
  char sound_fight[MAX_LEN_NAME];
  char sound_fight_alt[MAX_LEN_NAME];
  int build_cost;			/* Use wrappers to access this. */
  int pop_cost;  /* number of workers the unit contains (e.g., settlers, engineers)*/
  int attack_strength;
  int defense_strength;
  int move_rate;
  int unknown_move_cost; /* See utype_unknown_move_cost(). */

  struct advance *require_advance;	/* may be NULL */
  struct impr_type *need_improvement;	/* may be NULL */
  struct government *need_government;	/* may be NULL */

  int vision_radius_sq;
  int transport_capacity;
  int hp;
  int firepower;
  struct combat_bonus_list *bonuses;

#define U_NOT_OBSOLETED (NULL)
  struct unit_type *obsoleted_by;
  struct unit_type *converted_to;
  int convert_time;
  int fuel;

  bv_unit_type_flags flags;
  bv_unit_type_roles roles;

  int happy_cost;  /* unhappy people in home city */
  int upkeep[O_LAST];

  /* Only valid for ACTION_PARADROP */
  int paratroopers_range;
  int paratroopers_mr_req;
  int paratroopers_mr_sub;

  /* Additional values for the expanded veteran system */
  struct veteran_system *veteran;

  /* Values for bombardment */
  int bombard_rate;

  /* Values for founding cities */
  int city_size;

  int city_slots;

  struct unit_class *uclass;

  bv_unit_classes cargo;

  /* Can attack these classes even if they are otherwise "Unreachable" */
  bv_unit_classes targets;
  /* Can load into these class transports at any location,
   * even if they are otherwise "Unreachable". */
  bv_unit_classes embarks;
  /* Can unload from these class transports at any location,
   * even if they are otherwise "Unreachable". */
  bv_unit_classes disembarks;

  enum vision_layer vlayer;

  struct strvec *helptext;

  struct {
    bool igwall;
    bool worker;
  } adv;

  struct {
    int max_defense_mp; /* Value 0 here does not guarantee that unit never
                         * has CBONUS_DEFENSE_MULTIPLIER, it merely means
                         * that there's no POSITIVE one */
    int defense_mp_bonuses[U_LAST];
  } cache;

  void *ais[FREECIV_AI_MOD_LAST];
};

/* General unit and unit type (matched) routines */
Unit_type_id utype_count(void);
Unit_type_id utype_index(const struct unit_type *punittype);
Unit_type_id utype_number(const struct unit_type *punittype);

struct unit_type *unit_type_get(const struct unit *punit);
struct unit_type *utype_by_number(const Unit_type_id id);

struct unit_type *unit_type_by_rule_name(const char *name);
struct unit_type *unit_type_by_translated_name(const char *name);

const char *unit_rule_name(const struct unit *punit);
const char *utype_rule_name(const struct unit_type *punittype);

const char *unit_name_translation(const struct unit *punit);
const char *utype_name_translation(const struct unit_type *punittype);

const char *utype_values_string(const struct unit_type *punittype);
const char *utype_values_translation(const struct unit_type *punittype);

/* General unit type flag and role routines */
bool unit_has_type_flag(const struct unit *punit, enum unit_type_flag_id flag);

/**************************************************************************
  Return whether the given unit type has the flag.
**************************************************************************/
static inline bool utype_has_flag(const struct unit_type *punittype, int flag)
{
  return BV_ISSET(punittype->flags, flag);
}

bool unit_has_type_role(const struct unit *punit, enum unit_role_id role);
bool utype_has_role(const struct unit_type *punittype, int role);

void user_unit_type_flags_init(void);
void set_user_unit_type_flag_name(enum unit_type_flag_id id, const char *name,
                                  const char *helptxt);
const char *unit_type_flag_helptxt(enum unit_type_flag_id id);

bool unit_can_take_over(const struct unit *punit);
bool utype_can_take_over(const struct unit_type *punittype);

bool utype_can_freely_load(const struct unit_type *pcargotype,
                           const struct unit_type *ptranstype);
bool utype_can_freely_unload(const struct unit_type *pcargotype,
                             const struct unit_type *ptranstype);

bool utype_may_act_at_all(const struct unit_type *putype);
bool utype_can_do_action(const struct unit_type *putype,
                         const action_id act_id);
bool utype_acts_hostile(const struct unit_type *putype);

bool can_unit_act_when_ustate_is(const struct unit_type *punit_type,
                                 const enum ustate_prop prop,
                                 const bool is_there);
bool utype_can_do_act_when_ustate(const struct unit_type *punit_type,
                                  const action_id act_id,
                                  const enum ustate_prop prop,
                                  const bool is_there);

bool utype_can_do_act_if_tgt_citytile(const struct unit_type *punit_type,
                                      const action_id act_id,
                                      const enum citytile_type prop,
                                      const bool is_there);

bool can_utype_do_act_if_tgt_diplrel(const struct unit_type *punit_type,
                                     const action_id act_id,
                                     const int prop,
                                     const bool is_there);

bool utype_may_act_move_frags(struct unit_type *punit_type,
                              const action_id act_id,
                              const int move_fragments);

bool utype_may_act_tgt_city_tile(struct unit_type *punit_type,
                                 const action_id act_id,
                                 const enum citytile_type prop,
                                 const bool is_there);

bool utype_is_consumed_by_action(const struct action *paction,
                                 const struct unit_type *utype);

/* Functions to operate on various flag and roles. */
typedef bool (*role_unit_callback)(struct unit_type *ptype, void *data);

void role_unit_precalcs(void);
void role_unit_precalcs_free(void);
int num_role_units(int role);
struct unit_type *role_units_iterate(int role, role_unit_callback cb, void *data);
struct unit_type *role_units_iterate_backwards(int role, role_unit_callback cb, void *data);
struct unit_type *get_role_unit(int role, int role_index);
struct unit_type *best_role_unit(const struct city *pcity, int role);
struct unit_type *best_role_unit_for_player(const struct player *pplayer,
					    int role);
struct unit_type *first_role_unit_for_player(const struct player *pplayer,
					     int role);
bool role_units_translations(struct astring *astr, int flag, bool alts);

/* General unit class routines */
Unit_Class_id uclass_count(void);
Unit_Class_id uclass_number(const struct unit_class *pclass);
/* Optimised to be identical to uclass_number: the implementation
 * unittype.c is also semantically correct. */
#define uclass_index(_c_) (_c_)->item_number
#ifndef uclass_index
Unit_Class_id uclass_index(const struct unit_class *pclass);
#endif /* uclass_index */

struct unit_class *unit_class_get(const struct unit *punit);
struct unit_class *uclass_by_number(const Unit_Class_id id);
#define utype_class(_t_) (_t_)->uclass
#ifndef utype_class
struct unit_class *utype_class(const struct unit_type *punittype);
#endif /* utype_class */

struct unit_class *unit_class_by_rule_name(const char *s);

const char *uclass_rule_name(const struct unit_class *pclass);
const char *uclass_name_translation(const struct unit_class *pclass);

/**************************************************************************
  Return whether the given unit class has the flag.
**************************************************************************/
static inline bool uclass_has_flag(const struct unit_class *punitclass,
                                   enum unit_class_flag_id flag)
{
  return BV_ISSET(punitclass->flags, flag);
}

void user_unit_class_flags_init(void);
void set_user_unit_class_flag_name(enum unit_class_flag_id id,
                                   const char *name,
                                   const char *helptxt);
const char *unit_class_flag_helptxt(enum unit_class_flag_id id);

/* Ancillary routines */
int unit_build_shield_cost(const struct city *pcity, const struct unit *punit);
int utype_build_shield_cost(const struct city *pcity,
                            const struct unit_type *punittype);
int utype_build_shield_cost_base(const struct unit_type *punittype);
int unit_build_shield_cost_base(const struct unit *punit);

int utype_buy_gold_cost(const struct city *pcity,
                        const struct unit_type *punittype,
                        int shields_in_stock);

const struct veteran_system *
  utype_veteran_system(const struct unit_type *punittype);
int utype_veteran_levels(const struct unit_type *punittype);
const struct veteran_level *
  utype_veteran_level(const struct unit_type *punittype, int level);
const char *utype_veteran_name_translation(const struct unit_type *punittype,
                                           int level);
bool utype_veteran_has_power_bonus(const struct unit_type *punittype);

struct veteran_system *veteran_system_new(int count);
void veteran_system_destroy(struct veteran_system *vsystem);
void veteran_system_definition(struct veteran_system *vsystem, int level,
                               const char *vlist_name, int vlist_power,
                               int vlist_move, int vlist_raise,
                               int vlist_wraise);

int unit_disband_shields(const struct unit *punit);
int utype_disband_shields(const struct unit_type *punittype);

int unit_pop_value(const struct unit *punit);
int utype_pop_value(const struct unit_type *punittype);

enum unit_move_type utype_move_type(const struct unit_type *punittype);
void set_unit_move_type(struct unit_class *puclass);

/* player related unit functions */
int utype_upkeep_cost(const struct unit_type *ut, struct player *pplayer,
                      Output_type_id otype);
int utype_happy_cost(const struct unit_type *ut, const struct player *pplayer);

struct unit_type *can_upgrade_unittype(const struct player *pplayer,
				       struct unit_type *punittype);
int unit_upgrade_price(const struct player *pplayer,
		       const struct unit_type *from,
		       const struct unit_type *to);

bool utype_player_already_has_this_unique(const struct player *pplayer,
                                          const struct unit_type *putype);

bool can_player_build_unit_direct(const struct player *p,
				  const struct unit_type *punittype);
bool can_player_build_unit_later(const struct player *p,
				 const struct unit_type *punittype);
bool can_player_build_unit_now(const struct player *p,
			       const struct unit_type *punittype);

#define utype_fuel(ptype) (ptype)->fuel

bool utype_is_cityfounder(struct unit_type *utype);

/* Initialization and iteration */
void unit_types_init(void);
void unit_types_free(void);
void unit_type_flags_free(void);
void unit_class_flags_free(void);

struct unit_type *unit_type_array_first(void);
const struct unit_type *unit_type_array_last(void);

#define unit_type_iterate(_p)						\
{									\
  struct unit_type *_p = unit_type_array_first();			\
  if (NULL != _p) {							\
    for (; _p <= unit_type_array_last(); _p++) {

#define unit_type_iterate_end						\
    }									\
  }									\
}

#define unit_type_re_active_iterate(_p)                                 \
  unit_type_iterate(_p) {                                               \
    if (!_p->ruledit_disabled) {

#define unit_type_re_active_iterate_end                                 \
    }                                                                   \
  } unit_type_iterate_end;


void *utype_ai_data(const struct unit_type *ptype, const struct ai_type *ai);
void utype_set_ai_data(struct unit_type *ptype, const struct ai_type *ai,
                       void *data);

void unit_type_action_cache_set(struct unit_type *ptype);
void unit_type_action_cache_init(void);

/* Initialization and iteration */
void unit_classes_init(void);
void unit_classes_free(void);

void set_unit_class_caches(struct unit_class *pclass);
void set_unit_type_caches(struct unit_type *ptype);

struct unit_class *unit_class_array_first(void);
const struct unit_class *unit_class_array_last(void);

#define unit_class_iterate(_p)						\
{									\
  struct unit_class *_p = unit_class_array_first();			\
  if (NULL != _p) {							\
    for (; _p <= unit_class_array_last(); _p++) {

#define unit_class_iterate_end						\
    }									\
  }									\
}

#define unit_class_re_active_iterate(_p)                                 \
  unit_class_iterate(_p) {                                               \
    if (!_p->ruledit_disabled) {

#define unit_class_re_active_iterate_end                                 \
    }                                                                    \
  } unit_class_iterate_end;

#define SPECLIST_TAG unit_class
#define SPECLIST_TYPE struct unit_class
#include "speclist.h"

#define unit_class_list_iterate(uclass_list, pclass) \
  TYPED_LIST_ITERATE(struct unit_class, uclass_list, pclass)
#define unit_class_list_iterate_end LIST_ITERATE_END

#define SPECLIST_TAG unit_type
#define SPECLIST_TYPE struct unit_type
#include "speclist.h"

#define unit_type_list_iterate(utype_list, ptype) \
  TYPED_LIST_ITERATE(struct unit_type, utype_list, ptype)
#define unit_type_list_iterate_end LIST_ITERATE_END

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__UNITTYPE_H */
