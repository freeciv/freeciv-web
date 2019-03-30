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
#ifndef FC__UNITTOOLS_H
#define FC__UNITTOOLS_H

#include "fc_types.h"

#include "packets.h"		/* enum unit_info_use */
#include "unitlist.h"

#define SPECENUM_NAME unit_loss_reason
#define SPECENUM_VALUE0 ULR_KILLED
#define SPECENUM_VALUE0NAME "killed"
#define SPECENUM_VALUE1 ULR_RETIRED
#define SPECENUM_VALUE1NAME "retired"
#define SPECENUM_VALUE2 ULR_DISBANDED
#define SPECENUM_VALUE2NAME "disbanded"
#define SPECENUM_VALUE3 ULR_BARB_UNLEASH
#define SPECENUM_VALUE3NAME "barb_unleash"
#define SPECENUM_VALUE4 ULR_CITY_LOST
#define SPECENUM_VALUE4NAME "city_lost"
#define SPECENUM_VALUE5 ULR_STARVED
#define SPECENUM_VALUE5NAME "starved"
#define SPECENUM_VALUE6 ULR_SOLD
#define SPECENUM_VALUE6NAME "sold"
#define SPECENUM_VALUE7 ULR_USED
#define SPECENUM_VALUE7NAME "used"
#define SPECENUM_VALUE8 ULR_EXECUTED
#define SPECENUM_VALUE8NAME "executed"
#define SPECENUM_VALUE9 ULR_ELIMINATED
#define SPECENUM_VALUE9NAME "eliminated"
#define SPECENUM_VALUE10 ULR_EDITOR
#define SPECENUM_VALUE10NAME "editor"
#define SPECENUM_VALUE11 ULR_NONNATIVE_TERR
#define SPECENUM_VALUE11NAME "nonnative_terr"
#define SPECENUM_VALUE12 ULR_PLAYER_DIED
#define SPECENUM_VALUE12NAME "player_died"
#define SPECENUM_VALUE13 ULR_ARMISTICE
#define SPECENUM_VALUE13NAME "armistice"
#define SPECENUM_VALUE14 ULR_SDI
#define SPECENUM_VALUE14NAME "sdi"
#define SPECENUM_VALUE15 ULR_DETONATED
#define SPECENUM_VALUE15NAME "detonated"
#define SPECENUM_VALUE16 ULR_MISSILE
#define SPECENUM_VALUE16NAME "missile"
#define SPECENUM_VALUE17 ULR_NUKE
#define SPECENUM_VALUE17NAME "nuke"
#define SPECENUM_VALUE18 ULR_HP_LOSS
#define SPECENUM_VALUE18NAME "hp_loss"
#define SPECENUM_VALUE19 ULR_FUEL
#define SPECENUM_VALUE19NAME "fuel"
#define SPECENUM_VALUE20 ULR_STACK_CONFLICT
#define SPECENUM_VALUE20NAME "stack_conflict"
#define SPECENUM_VALUE21 ULR_BRIBED
#define SPECENUM_VALUE21NAME "bribed"
#define SPECENUM_VALUE22 ULR_CAPTURED
#define SPECENUM_VALUE22NAME "captured"
#define SPECENUM_VALUE23 ULR_CAUGHT
#define SPECENUM_VALUE23NAME "caught"
#define SPECENUM_VALUE24 ULR_TRANSPORT_LOST
#define SPECENUM_VALUE24NAME "transport_lost"
#include "specenum_gen.h"

/* battle related */
struct unit_type *find_a_unit_type(enum unit_role_id role,
				   enum unit_role_id role_tech);
bool maybe_make_veteran(struct unit *punit);
void notify_unit_experience(struct unit *punit);
void unit_versus_unit(struct unit *attacker, struct unit *defender,
                      int *att_hp, int *def_hp);
void unit_bombs_unit(struct unit *attacker, struct unit *defender,
                     int *att_hp, int *def_hp);
void combat_veterans(struct unit *attacker, struct unit *defender);

/* move check related */
bool is_unit_being_refueled(const struct unit *punit);
bool is_airunit_refuel_point(const struct tile *ptile,
                             const struct player *pplayer,
                             const struct unit *punit);

/* turn update related */
void player_restore_units(struct player *pplayer);
void update_unit_activities(struct player *pplayer);
void execute_unit_orders(struct player *pplayer);
void finalize_unit_phase_beginning(struct player *pplayer);

/* various */
void place_partisans(struct tile *pcenter, struct player *powner,
                     int count, int sq_radius);
bool teleport_unit_to_city(struct unit *punit, struct city *pcity, int move_cost,
			  bool verbose);
void resolve_unit_stacks(struct player *pplayer, struct player *aplayer,
                         bool verbose);
struct unit_list *get_units_seen_via_ally(const struct player *pplayer,
                                          const struct player *aplayer);
void remove_allied_visibility(struct player *pplayer, struct player *aplayer,
                              const struct unit_list *seen_units);
void give_allied_visibility(struct player *pplayer, struct player *aplayer);
int get_unit_vision_at(struct unit *punit, const struct tile *ptile,
                       enum vision_layer vlayer);
void unit_refresh_vision(struct unit *punit);
void unit_list_refresh_vision(struct unit_list *punitlist);
void bounce_unit(struct unit *punit, bool verbose);
bool unit_activity_needs_target_from_client(enum unit_activity activity);
void unit_assign_specific_activity_target(struct unit *punit,
                                          enum unit_activity *activity,
                                          struct extra_type **target);
void unit_forget_last_activity(struct unit *punit);

/* creation/deletion/upgrading */
void transform_unit(struct unit *punit, struct unit_type *to_unit,
                    bool has_to_pay);
struct unit *create_unit(struct player *pplayer, struct tile *ptile,
			 struct unit_type *punittype,
			 int veteran_level, int homecity_id, int moves_left);
struct unit *create_unit_full(struct player *pplayer, struct tile *ptile,
			      struct unit_type *punittype, int veteran_level,
			      int homecity_id, int moves_left, int hp_left,
			      struct unit *ptrans);
void wipe_unit(struct unit *punit, enum unit_loss_reason reason,
               struct player *killer);
void kill_unit(struct unit *pkiller, struct unit *punit, bool vet);

struct unit *unit_change_owner(struct unit *punit, struct player *pplayer,
                               int homecity, enum unit_loss_reason reason)
                               fc__warn_unused_result;

void unit_set_removal_callback(struct unit *punit,
                               void (*callback)(struct unit *punit));
void unit_unset_removal_callback(struct unit *punit);

/* sending to client */
void package_unit(struct unit *punit, struct packet_unit_info *packet);
void package_short_unit(struct unit *punit,
			struct packet_unit_short_info *packet,
                        enum unit_info_use packet_use, int info_city_id);
void send_unit_info(struct conn_list *dest, struct unit *punit);
void send_all_known_units(struct conn_list *dest);
void unit_goes_out_of_sight(struct player *pplayer, struct unit *punit);

/* doing a unit activity */
void do_nuclear_explosion(struct player *pplayer, struct tile *ptile);
bool do_airline(struct unit *punit, struct city *city2);
void do_explore(struct unit *punit);
bool do_paradrop(struct unit *punit, struct tile *ptile);
void unit_transport_load_send(struct unit *punit, struct unit *ptrans);
void unit_transport_unload_send(struct unit *punit);
bool unit_move(struct unit *punit, struct tile *ptile, int move_cost,
               struct unit *embark_to, bool conquer_city_allowed);
bool execute_orders(struct unit *punit, const bool fresh);

bool unit_can_do_action_now(const struct unit *punit);
void unit_did_action(struct unit *punit);

bool unit_can_be_retired(struct unit *punit);

void unit_activities_cancel_all_illegal(const struct tile *ptile);

void unit_get_goods(struct unit *punit);

#endif  /* FC__UNITTOOLS_H */
