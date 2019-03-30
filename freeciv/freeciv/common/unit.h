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
#ifndef FC__UNIT_H
#define FC__UNIT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "bitvector.h"

/* common */
#include "base.h"
#include "fc_types.h"
#include "terrain.h"		/* enum tile_special_type */
#include "unittype.h"
#include "vision.h"
#include "world_object.h"

struct road_type;
struct unit_move_data; /* Actually defined in "server/unittools.c". */

/* Changing this enum will break network compatibility.
 * Different orders take different parameters; see struct unit_order. */
enum unit_orders {
  /* Move without performing any action (dir) */
  ORDER_MOVE = 0,
  /* Perform activity (activity, extra) */
  ORDER_ACTIVITY = 1,
  /* Pause to regain movement points (no parameters) */
  ORDER_FULL_MP = 2,
  /* Move; if necessary prompt for action/target when order executed (dir) */
  ORDER_ACTION_MOVE = 3,
  /* Perform pre-specified action (action, target, extra, dir) */
  ORDER_PERFORM_ACTION = 4,
  /* and plenty more for later... */
  ORDER_LAST
};

enum unit_focus_status {
  FOCUS_AVAIL, FOCUS_WAIT, FOCUS_DONE  
};

enum goto_route_type {
  ROUTE_GOTO, ROUTE_PATROL
};

enum unit_upgrade_result {
  UU_OK,
  UU_NO_UNITTYPE,
  UU_NO_MONEY,
  UU_NOT_IN_CITY,
  UU_NOT_CITY_OWNER,
  UU_NOT_ENOUGH_ROOM,
  UU_NOT_TERRAIN,         /* The upgraded unit could not survive. */
  UU_UNSUITABLE_TRANSPORT /* Can't upgrade inside current transport. */
};

enum unit_airlift_result {
  /* Codes treated as success: */
  AR_OK,                /* This will definitely work */
  AR_OK_SRC_UNKNOWN,    /* Source city's airlift capability is unknown */
  AR_OK_DST_UNKNOWN,    /* Dest city's airlift capability is unknown */
  /* Codes treated as failure: */
  AR_NO_MOVES,          /* Unit has no moves left */
  AR_WRONG_UNITTYPE,    /* Can't airlift this type of unit */
  AR_OCCUPIED,          /* Can't airlift units with passengers */
  AR_NOT_IN_CITY,       /* Unit not in a city */
  AR_BAD_SRC_CITY,      /* Can't airlift from this src city */
  AR_BAD_DST_CITY,      /* Can't airlift to this dst city */
  AR_SRC_NO_FLIGHTS,    /* No flights available from src */
  AR_DST_NO_FLIGHTS     /* No flights available to dst */
};

struct unit_adv {
  enum adv_unit_task task;
};

struct unit_order {
  enum unit_orders order;
  enum unit_activity activity;  /* Only valid for ORDER_ACTIVITY. */
  /* Valid for ORDER_ACTIVITY and ORDER_PERFORM_ACTION. Validity and meaning
   * depends on 'action' or 'activity'. The meaning can be building, extra,
   * tech, ... */
  int sub_target;
  /* Only valid for ORDER_PERFORM_ACTION */
  int action;
  /* Valid for ORDER_MOVE, ORDER_ACTION_MOVE and ORDER_PERFORM_ACTION. */
  enum direction8 dir;
};

/* Used in the network protocol */
#define SPECENUM_NAME unit_ss_data_type
/* The player wants to be reminded to ask what actions the unit can perform
 * to a certain target tile. */
#define SPECENUM_VALUE0 USSDT_QUEUE
/* The player no longer wants the reminder to ask what actions the unit can
 * perform to a certain target tile. */
#define SPECENUM_VALUE1 USSDT_UNQUEUE
/* The player wants to record that the unit now belongs to the specified
 * battle group. */
#define SPECENUM_VALUE2 USSDT_BATTLE_GROUP
#include "specenum_gen.h"

struct unit;
struct unit_list;

struct unit {
  struct unit_type *utype; /* Cannot be NULL. */
  struct tile *tile;
  int refcount;
  enum direction8 facing;
  struct player *owner; /* Cannot be NULL. */
  struct player *nationality;
  int id;
  int homecity;

  int upkeep[O_LAST]; /* unit upkeep with regards to the homecity */

  int moves_left;
  int hp;
  int veteran;
  int fuel;

  struct tile *goto_tile; /* May be NULL. */

  enum unit_activity activity;

  /* The amount of work that has been done on the current activity.  This
   * is counted in turns but is multiplied by ACTIVITY_FACTOR (which allows
   * fractional values in some cases). */
  int activity_count;

  struct extra_type *activity_target;

  /* Previous activity, so it can be resumed without loss of progress
   * if the user changes their mind during a turn. */
  enum unit_activity changed_from;
  int changed_from_count;
  struct extra_type *changed_from_target;

  bool ai_controlled; /* 0: not automated; 1: automated */
  bool moved;
  bool paradropped;

  /* This value is set if the unit is done moving for this turn. This
   * information is used by the client.  The invariant is:
   *   - If the unit has no more moves, it's done moving.
   *   - If the unit is on a goto but is waiting, it's done moving.
   *   - Otherwise the unit is not done moving. */
  bool done_moving;

  struct unit *transporter;   /* This unit is transported by ... */
  struct unit_list *transporting; /* This unit transports ... */

  struct goods_type *carrying;

  /* The battlegroup ID: defined by the client but stored by the server. */
#define MAX_NUM_BATTLEGROUPS (4)
#define BATTLEGROUP_NONE (-1)
  int battlegroup;

  bool has_orders;
  struct {
    int length, index;
    bool repeat;   /* The path is to be repeated on completion. */
    bool vigilant; /* Orders should be cleared if an enemy is met. */
    struct unit_order *list;
  } orders;

  /* The unit may want the player to choose an action. */
  enum action_decision action_decision_want;
  struct tile *action_decision_tile;

  bool stay; /* Unit is prohibited from moving */

  union {
    struct {
      /* Only used at the client (the server is omniscient; ./client/). */

      enum unit_focus_status focus_status;

      int transported_by; /* Used for unit_short_info packets where we can't
                           * be sure that the information about the
                           * transporter is known. */
      bool occupied;      /* TRUE if at least one cargo on the transporter. */

      /* Equivalent to pcity->client.color. Only for cityfounder units. */
      bool colored;
      int color_index;

      bool asking_city_name;

      /* Used in a follow up question about a selected action. */
      struct act_prob *act_prob_cache;
    } client;

    struct {
      /* Only used in the server (./ai/ and ./server/). */

      bool debug;

      struct unit_adv *adv;
      void *ais[FREECIV_AI_MOD_LAST];
      int birth_turn;

      /* ord_map and ord_city are the order index of this unit in tile.units
       * and city.units_supported; they are only used for save/reload */
      int ord_map;
      int ord_city;

      struct vision *vision;
      time_t action_timestamp;
      int action_turn;
      struct unit_move_data *moving;

      /* The unit is in the process of dying. */
      bool dying;

      /* Call back to run on unit removal. */
      void (*removal_callback)(struct unit *punit);

      /* The upkeep that actually was payed. */
      int upkeep_payed[O_LAST];
    } server;
  };
};

#ifdef FREECIV_DEBUG
#define CHECK_UNIT(punit)                                                   \
  (fc_assert(punit != NULL),                                                \
   fc_assert(unit_type_get(punit) != NULL),                                 \
   fc_assert(unit_owner(punit) != NULL),                                    \
   fc_assert(player_by_number(player_index(unit_owner(punit)))              \
             == unit_owner(punit)),                                         \
   fc_assert(game_unit_by_number(punit->id) != NULL))
#else  /* FREECIV_DEBUG */
#define CHECK_UNIT(punit) /* Do nothing */
#endif /* FREECIV_DEBUG */

void setup_real_activities_array(void);

extern Activity_type_id real_activities[ACTIVITY_LAST];

#define activity_type_list_iterate(_act_list_, _act_)                        \
{                                                                            \
  int _act_i_;                                                               \
  for (_act_i_ = 0; _act_list_[_act_i_] != ACTIVITY_LAST; _act_i_++) {       \
    Activity_type_id _act_ = _act_list_[_act_i_];

#define activity_type_list_iterate_end                                       \
  }                                                                          \
}

/* Iterates over the types of unit activity. */
#define activity_type_iterate(_act_)					    \
{									    \
  activity_type_list_iterate(real_activities, _act_)

#define activity_type_iterate_end                                           \
  activity_type_list_iterate_end                                            \
}

bool unit_can_help_build_wonder_here(const struct unit *punit);
bool unit_can_est_trade_route_here(const struct unit *punit);
enum unit_airlift_result
    test_unit_can_airlift_to(const struct player *restriction,
                             const struct unit *punit,
                             const struct city *pdest_city);
bool unit_can_airlift_to(const struct unit *punit, const struct city *pcity);
bool unit_has_orders(const struct unit *punit);

bool could_unit_load(const struct unit *pcargo, const struct unit *ptrans);
bool can_unit_load(const struct unit *punit, const struct unit *ptrans);
bool can_unit_unload(const struct unit *punit, const struct unit *ptrans);
bool can_unit_paradrop(const struct unit *punit);
bool can_unit_change_homecity_to(const struct unit *punit,
				 const struct city *pcity);
bool can_unit_change_homecity(const struct unit *punit);
const char *get_activity_text(enum unit_activity activity);
bool can_unit_continue_current_activity(struct unit *punit);
bool can_unit_do_activity(const struct unit *punit,
			  enum unit_activity activity);
bool can_unit_do_activity_targeted(const struct unit *punit,
				   enum unit_activity activity,
                                   struct extra_type *target);
bool can_unit_do_activity_targeted_at(const struct unit *punit,
				      enum unit_activity activity,
				      struct extra_type *target,
				      const struct tile *ptile);
void set_unit_activity(struct unit *punit, enum unit_activity new_activity);
void set_unit_activity_targeted(struct unit *punit,
				enum unit_activity new_activity,
                                struct extra_type *new_target);
void set_unit_activity_base(struct unit *punit,
                            Base_type_id base);
void set_unit_activity_road(struct unit *punit,
                            Road_type_id road);
int get_activity_rate(const struct unit *punit);
int get_activity_rate_this_turn(const struct unit *punit);
int get_turns_for_activity_at(const struct unit *punit,
			      enum unit_activity activity,
			      const struct tile *ptile,
                              struct extra_type *tgt);
bool activity_requires_target(enum unit_activity activity);
bool can_unit_do_autosettlers(const struct unit *punit); 
bool is_unit_activity_on_tile(enum unit_activity activity,
			      const struct tile *ptile);
bv_extras get_unit_tile_pillage_set(const struct tile *ptile);
bool is_attack_unit(const struct unit *punit);
bool is_military_unit(const struct unit *punit);           /* !set !dip !cara */
bool unit_can_do_action(const struct unit *punit,
                        const action_id act_id);
bool is_square_threatened(const struct player *pplayer,
			  const struct tile *ptile, bool omniscient);
bool is_field_unit(const struct unit *punit);              /* ships+aero */
bool is_hiding_unit(const struct unit *punit);
bool unit_can_add_or_build_city(const struct unit *punit);

bool kills_citizen_after_attack(const struct unit *punit);

struct astring; /* Forward declaration. */
void unit_activity_astr(const struct unit *punit, struct astring *astr);
void unit_upkeep_astr(const struct unit *punit, struct astring *astr);
const char *unit_activity_text(const struct unit *punit);

int get_transporter_capacity(const struct unit *punit);

#define unit_home(_pu_) (game_city_by_number((_pu_)->homecity))
#define unit_owner(_pu) ((_pu)->owner)
#define unit_tile(_pu) ((_pu)->tile)
struct player *unit_nationality(const struct unit *punit);
void unit_tile_set(struct unit *punit, struct tile *ptile);

struct unit *is_allied_unit_tile(const struct tile *ptile,
				 const struct player *pplayer);
struct unit *is_enemy_unit_tile(const struct tile *ptile,
				const struct player *pplayer);
struct unit *is_non_allied_unit_tile(const struct tile *ptile,
				     const struct player *pplayer);
struct unit *is_other_players_unit_tile(const struct tile *ptile,
					const struct player *pplayer);
struct unit *is_non_attack_unit_tile(const struct tile *ptile,
				     const struct player *pplayer);
struct unit *unit_occupies_tile(const struct tile *ptile,
				const struct player *pplayer);

bool is_my_zoc(const struct player *unit_owner, const struct tile *ptile,
               const struct civ_map *zmap);
bool unit_being_aggressive(const struct unit *punit);
bool unit_type_really_ignores_zoc(const struct unit_type *punittype);

bool is_build_activity(enum unit_activity activity, const struct tile *ptile);
bool is_clean_activity(enum unit_activity activity);
bool is_tile_activity(enum unit_activity activity);

struct unit *unit_virtual_create(struct player *pplayer, struct city *pcity,
                                 struct unit_type *punittype,
				 int veteran_level);
void unit_virtual_destroy(struct unit *punit);
bool unit_is_virtual(const struct unit *punit);
void free_unit_orders(struct unit *punit);

int get_transporter_occupancy(const struct unit *ptrans);
struct unit *transporter_for_unit(const struct unit *pcargo);
struct unit *transporter_for_unit_at(const struct unit *pcargo,
                                     const struct tile *ptile);

enum unit_upgrade_result unit_upgrade_test(const struct unit *punit,
                                           bool is_free);
enum unit_upgrade_result unit_upgrade_info(const struct unit *punit,
                                           char *buf, size_t bufsz);
bool unit_can_convert(const struct unit *punit);

bool is_losing_hp(const struct unit *punit);
bool unit_type_is_losing_hp(const struct player *pplayer,
                            const struct unit_type *punittype);

bool unit_is_alive(int id);

void *unit_ai_data(const struct unit *punit, const struct ai_type *ai);
void unit_set_ai_data(struct unit *punit, const struct ai_type *ai,
                      void *data);

int unit_bribe_cost(struct unit *punit, struct player *briber);

bool unit_transport_load(struct unit *pcargo, struct unit *ptrans,
                         bool force);
bool unit_transport_unload(struct unit *pcargo);
struct unit *unit_transport_get(const struct unit *pcargo);
bool unit_transported(const struct unit *pcargo);
struct unit_list *unit_transport_cargo(const struct unit *ptrans);
bool unit_transport_check(const struct unit *pcargo,
                          const struct unit *ptrans);
bool unit_contained_in(const struct unit *pcargo, const struct unit *ptrans);
int unit_cargo_depth(const struct unit *pcargo);
int unit_transport_depth(const struct unit *ptrans);

bool unit_is_cityfounder(const struct unit *punit);

/* Iterate all transporters carrying '_pcargo', directly or indirectly. */
#define unit_transports_iterate(_pcargo, _ptrans) {                         \
  struct unit *_ptrans;                                                     \
  for (_ptrans = unit_transport_get(_pcargo); NULL != _ptrans;              \
       _ptrans = unit_transport_get(_ptrans)) {
#define unit_transports_iterate_end }}

struct cargo_iter;
size_t cargo_iter_sizeof(void) fc__attribute((const));

struct iterator *cargo_iter_init(struct cargo_iter *iter,
                                 const struct unit *ptrans);
#define unit_cargo_iterate(_ptrans, _pcargo)                                \
  generic_iterate(struct cargo_iter, struct unit *, _pcargo,                \
                  cargo_iter_sizeof, cargo_iter_init, _ptrans)
#define unit_cargo_iterate_end generic_iterate_end

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__UNIT_H */
