/********************************************************************** 
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
#ifndef FC__UNIT_H
#define FC__UNIT_H

#include "fc_types.h"
#include "base.h"
#include "terrain.h"		/* enum tile_special_type */
#include "unittype.h"
#include "vision.h"

/* Changing this enum will break network compatability. */
enum unit_orders {
  ORDER_MOVE = 0,
  ORDER_ACTIVITY = 1,
  ORDER_FULL_MP = 2,
  ORDER_BUILD_CITY = 3,
  ORDER_DISBAND = 4,
  ORDER_BUILD_WONDER = 5,
  ORDER_TRADEROUTE = 6,
  ORDER_HOMECITY = 7,
  /* and plenty more for later... */
  ORDER_LAST
};

enum unit_focus_status {
  FOCUS_AVAIL, FOCUS_WAIT, FOCUS_DONE  
};

/* Changing this enum will break network compatability. */
enum diplomat_actions {
  DIPLOMAT_MOVE = 0,	/* move onto city square - only for allied cities */
  DIPLOMAT_EMBASSY = 1,
  DIPLOMAT_BRIBE = 2,
  DIPLOMAT_INCITE = 3,
  DIPLOMAT_INVESTIGATE = 4,
  DIPLOMAT_SABOTAGE = 5,
  DIPLOMAT_STEAL = 6,
  SPY_POISON = 7, 
  SPY_SABOTAGE_UNIT = 8,
  DIPLOMAT_ANY_ACTION   /* leave this one last */
};

enum ai_unit_task { AIUNIT_NONE, AIUNIT_AUTO_SETTLER, AIUNIT_BUILD_CITY,
                    AIUNIT_DEFEND_HOME, AIUNIT_ATTACK, AIUNIT_ESCORT, 
                    AIUNIT_EXPLORE, AIUNIT_RECOVER, AIUNIT_HUNTER };

enum goto_move_restriction {
  GOTO_MOVE_ANY,
  GOTO_MOVE_CARDINAL_ONLY, /* No diagonal moves.  */
  GOTO_MOVE_STRAIGHTEST
};

enum goto_route_type {
  ROUTE_GOTO, ROUTE_PATROL
};

enum unit_move_result {
  MR_OK,
  MR_DEATH,
  MR_PAUSE,
  MR_BAD_TYPE_FOR_CITY_TAKE_OVER,
  MR_NO_WAR, /* Can't move here without declaring war. */
  MR_PEACE, /* Can't move here because of a peace treaty. */
  MR_ZOC,
  MR_BAD_ACTIVITY,
  MR_BAD_DESTINATION,
  MR_BAD_MAP_POSITION,
  MR_DESTINATION_OCCUPIED_BY_NON_ALLIED_CITY,
  MR_DESTINATION_OCCUPIED_BY_NON_ALLIED_UNIT,
  MR_NO_TRANSPORTER_CAPACITY,
  MR_TRIREME,
};

enum add_build_city_result {
  AB_BUILD_OK,			/* Unit OK to build city */
  AB_ADD_OK,			/* Unit OK to add to city */
  AB_NOT_BUILD_LOC,		/* City is not allowed to be built at
				   this location */
  AB_NOT_ADDABLE_UNIT,		/* Unit is not one that can be added
				   to cities */
  AB_NOT_BUILD_UNIT,		/* Unit is not one that can build
				   cities */
  AB_NO_MOVES_BUILD,		/* Unit does not have moves left to
				   build a city */
  AB_NO_MOVES_ADD,		/* Unit does not have moves left to
				   add to city */
  AB_NOT_OWNER,			/* Owner of unit is not owner of
				   city */
  AB_TOO_BIG,			/* City is too big to be added to */
  AB_NO_SPACE			/* Adding takes city past limit */
};

enum unit_upgrade_result {
  UR_OK,
  UR_NO_UNITTYPE,
  UR_NO_MONEY,
  UR_NOT_IN_CITY,
  UR_NOT_CITY_OWNER,
  UR_NOT_ENOUGH_ROOM
};
    
struct unit_ai {
  bool control; /* 0: not automated    1: automated */
  enum ai_unit_task ai_role;
  /* The following are unit ids or special indicator values (<=0) */
  int ferryboat; /* the ferryboat assigned to us */
  int passenger; /* the unit assigned to this ferryboat */
  int bodyguard; /* the unit bodyguarding us */
  int charge; /* the unit this unit is bodyguarding */

  struct tile *prev_struct, *cur_struct;
  struct tile **prev_pos, **cur_pos;

  int target; /* target we hunt */
  int hunted; /* if a player is hunting us, set by that player */
  bool done;  /* we are done controlling this unit this turn */
};

struct unit_order {
  enum unit_orders order;
  enum unit_activity activity;  /* Only valid for ORDER_ACTIVITY. */
  Base_type_id base;            /* Only valid for activity ACTIVITY_BASE */
  enum direction8 dir;          /* Only valid for ORDER_MOVE. */
};

struct unit {
  struct unit_type *utype; /* Cannot be NULL. */
  struct tile *tile;
  struct player *owner; /* Cannot be NULL. */
  int id;
  int homecity;

  int upkeep[O_LAST]; /* unit upkeep with regards to the homecity */

  int moves_left;
  int hp;
  int veteran;
  int fuel;
  int birth_turn;
  struct unit_ai ai;
  enum unit_activity activity;
  struct tile *goto_tile; /* May be NULL. */

  /* The amount of work that has been done on the current activity.  This
   * is counted in turns but is multiplied by ACTIVITY_COUNT (which allows
   * fractional values in some cases). */
  int activity_count;

  enum tile_special_type activity_target;
  Base_type_id           activity_base;
  enum unit_focus_status focus_status;
  int ord_map, ord_city;
  /* ord_map and ord_city are the order index of this unit in tile.units
     and city.units_supported; they are only used for save/reload */
  bool debug;
  bool moved;
  bool paradropped;

  /* This value is set if the unit is done moving for this turn. This
   * information is used by the client.  The invariant is:
   *   - If the unit has no more moves, it's done moving.
   *   - If the unit is on a goto but is waiting, it's done moving.
   *   - Otherwise the unit is not done moving. */
  bool done_moving;

  int transported_by;
  int occupy; /* number of units that occupy transporter */

  /* The battlegroup ID: defined by the client but stored by the server. */
#define MAX_NUM_BATTLEGROUPS (4)
#define BATTLEGROUP_NONE (-1)
  int battlegroup;

  struct {
    /* Equivalent to pcity->client.color.  Only for F_CITIES units. */
    bool colored;
    int color_index;
  } client;
  struct {
    struct vision *vision;
  } server;

  bool has_orders;
  struct {
    int length, index;
    bool repeat;	/* The path is to be repeated on completion. */
    bool vigilant;	/* Orders should be cleared if an enemy is met. */
    struct unit_order *list;
  } orders;
};

bool is_real_activity(enum unit_activity activity);

/* Iterates over the types of unit activity. */
#define activity_type_iterate(act)					    \
{									    \
  Activity_type_id act;			         			    \
									    \
  for (act = 0; act < ACTIVITY_LAST; act++) {				    \
    if (is_real_activity(act)) {

#define activity_type_iterate_end     					    \
    }									    \
  }									    \
}

bool diplomat_can_do_action(const struct unit *pdiplomat,
			    enum diplomat_actions action,
			    const struct tile *ptile);
bool is_diplomat_action_available(const struct unit *pdiplomat,
				  enum diplomat_actions action,
				  const struct tile *ptile);

bool unit_can_help_build_wonder(const struct unit *punit,
				const struct city *pcity);
bool unit_can_help_build_wonder_here(const struct unit *punit);
bool unit_can_est_traderoute_here(const struct unit *punit);
bool unit_can_airlift_to(const struct unit *punit, const struct city *pcity);
bool unit_has_orders(const struct unit *punit);

bool could_unit_load(const struct unit *pcargo, const struct unit *ptrans);
bool can_unit_load(const struct unit *punit, const struct unit *ptrans);
bool can_unit_unload(const struct unit *punit, const struct unit *ptrans);
bool can_unit_paradrop(const struct unit *punit);
bool can_unit_bombard(const struct unit *punit);
bool can_unit_change_homecity_to(const struct unit *punit,
				 const struct city *pcity);
bool can_unit_change_homecity(const struct unit *punit);
const char *get_activity_text(enum unit_activity activity);
bool can_unit_continue_current_activity(struct unit *punit);
bool can_unit_do_activity(const struct unit *punit,
			  enum unit_activity activity);
bool can_unit_do_activity_targeted(const struct unit *punit,
				   enum unit_activity activity,
				   enum tile_special_type target,
                                   Base_type_id base);
bool can_unit_do_activity_targeted_at(const struct unit *punit,
				      enum unit_activity activity,
				      enum tile_special_type target,
				      const struct tile *ptile,
                                      Base_type_id base);
bool can_unit_do_activity_base(const struct unit *punit,
                               Base_type_id base);
void set_unit_activity(struct unit *punit, enum unit_activity new_activity);
void set_unit_activity_targeted(struct unit *punit,
				enum unit_activity new_activity,
				enum tile_special_type new_target,
                                Base_type_id base);
void set_unit_activity_base(struct unit *punit,
                            Base_type_id base);
int get_activity_rate(const struct unit *punit);
int get_activity_rate_this_turn(const struct unit *punit);
int get_turns_for_activity_at(const struct unit *punit,
			      enum unit_activity activity,
			      const struct tile *ptile);
bool can_unit_do_autosettlers(const struct unit *punit); 
bool is_unit_activity_on_tile(enum unit_activity activity,
			      const struct tile *ptile);
bv_special get_unit_tile_pillage_set(const struct tile *ptile);
bool is_attack_unit(const struct unit *punit);
bool is_military_unit(const struct unit *punit);           /* !set !dip !cara */
bool is_diplomat_unit(const struct unit *punit);
bool is_square_threatened(const struct player *pplayer,
			  const struct tile *ptile);
bool is_field_unit(const struct unit *punit);              /* ships+aero */
bool is_hiding_unit(const struct unit *punit);
#define COULD_OCCUPY(punit) \
  (uclass_has_flag(unit_class(punit), UCF_CAN_OCCUPY_CITY) \
   && is_military_unit(punit))
bool can_unit_add_to_city (const struct unit *punit);
bool can_unit_build_city (const struct unit *punit);
bool can_unit_add_or_build_city (const struct unit *punit);
enum add_build_city_result test_unit_add_or_build_city(const struct unit *
						       punit);
bool kills_citizen_after_attack(const struct unit *punit);

struct astring; /* Forward declaration. */
void unit_activity_astr(const struct unit *punit, struct astring *astr);
void unit_upkeep_astr(const struct unit *punit, struct astring *astr);
const char *unit_activity_text(const struct unit *punit);

int get_transporter_capacity(const struct unit *punit);

struct player *unit_owner(const struct unit *punit);
struct tile *unit_tile(const struct unit *punit);

struct unit *is_allied_unit_tile(const struct tile *ptile,
				 const struct player *pplayer);
struct unit *is_enemy_unit_tile(const struct tile *ptile,
				const struct player *pplayer);
struct unit *is_non_allied_unit_tile(const struct tile *ptile,
				     const struct player *pplayer);
struct unit *is_non_attack_unit_tile(const struct tile *ptile,
				     const struct player *pplayer);
struct unit *unit_occupies_tile(const struct tile *ptile,
				const struct player *pplayer);

bool is_my_zoc(const struct player *unit_owner, const struct tile *ptile);
bool unit_being_aggressive(const struct unit *punit);
bool unit_type_really_ignores_zoc(const struct unit_type *punittype);

bool is_build_or_clean_activity(enum unit_activity activity);

struct unit *create_unit_virtual(struct player *pplayer, struct city *pcity,
                                 struct unit_type *punittype,
				 int veteran_level);
void destroy_unit_virtual(struct unit *punit);
bool unit_is_virtual(const struct unit *punit);
void free_unit_orders(struct unit *punit);

int get_transporter_occupancy(const struct unit *ptrans);
struct unit *find_transporter_for_unit(const struct unit *pcargo);

enum unit_upgrade_result test_unit_upgrade(const struct unit *punit,
					   bool is_free);
enum unit_upgrade_result get_unit_upgrade_info(char *buf, size_t bufsz,
					       const struct unit *punit);
bool test_unit_transform(const struct unit *punit);

bool is_losing_hp(const struct unit *punit);
bool unit_type_is_losing_hp(const struct player *pplayer,
                            const struct unit_type *punittype);

bool unit_alive(int id);

#endif  /* FC__UNIT_H */
