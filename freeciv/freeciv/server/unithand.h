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
#ifndef FC__UNITHAND_H
#define FC__UNITHAND_H

#include "unit.h"

#include "hand_gen.h"

/* A category of reasons why an action isn't enabled. */
enum ane_kind {
  /* Explanation: wrong actor unit. */
  ANEK_ACTOR_UNIT,
  /* Explanation: no action target. */
  ANEK_MISSING_TARGET,
  /* Explanation: the action is redundant vs this target. */
  ANEK_BAD_TARGET,
  /* Explanation: bad actor terrain. */
  ANEK_BAD_TERRAIN_ACT,
  /* Explanation: bad target terrain. */
  ANEK_BAD_TERRAIN_TGT,
  /* Explanation: being transported. */
  ANEK_IS_TRANSPORTED,
  /* Explanation: not being transported. */
  ANEK_IS_NOT_TRANSPORTED,
  /* Explanation: transports a cargo unit. */
  ANEK_IS_TRANSPORTING,
  /* Explanation: doesn't transport a cargo unit. */
  ANEK_IS_NOT_TRANSPORTING,
  /* Explanation: actor unit has a home city. */
  ANEK_ACTOR_HAS_HOME_CITY,
  /* Explanation: actor unit has no a home city. */
  ANEK_ACTOR_HAS_NO_HOME_CITY,
  /* Explanation: must declare war first. */
  ANEK_NO_WAR,
  /* Explanation: can't be done to domestic targets. */
  ANEK_DOMESTIC,
  /* Explanation: can't be done to foreign targets. */
  ANEK_FOREIGN,
  /* Explanation: this nation can't act. */
  ANEK_NATION_ACT,
  /* Explanation: this nation can't be targeted. */
  ANEK_NATION_TGT,
  /* Explanation: not enough MP left. */
  ANEK_LOW_MP,
  /* Explanation: can't be done to city centers. */
  ANEK_IS_CITY_CENTER,
  /* Explanation: can't be done to non city centers. */
  ANEK_IS_NOT_CITY_CENTER,
  /* Explanation: can't be done to claimed target tiles. */
  ANEK_TGT_IS_CLAIMED,
  /* Explanation: can't be done to unclaimed target tiles. */
  ANEK_TGT_IS_UNCLAIMED,
  /* Explanation: can't be done because target is too near. */
  ANEK_DISTANCE_NEAR,
  /* Explanation: can't be done because target is too far away. */
  ANEK_DISTANCE_FAR,
  /* Explanation: can't be done to targets that far from the coast line. */
  ANEK_TRIREME_MOVE,
  /* Explanation: can't be done because the actor can't exit its
   * transport. */
  ANEK_DISEMBARK_ACT,
  /* Explanation: actor can't reach unit at target. */
  ANEK_TGT_UNREACHABLE,
  /* Explanation: the action is disabled in this scenario. */
  ANEK_SCENARIO_DISABLED,
  /* Explanation: too close to a city. */
  ANEK_CITY_TOO_CLOSE_TGT,
  /* Explanation: the target city is too big. */
  ANEK_CITY_TOO_BIG,
  /* Explanation: the target city's population limit banned the action. */
  ANEK_CITY_POP_LIMIT,
  /* Explanation: the specified city don't have the needed capacity. */
  ANEK_CITY_NO_CAPACITY,
  /* Explanation: the target unit can't switch sides because it is unique
   * and the actor player already has one. */
  ANEK_TGT_IS_UNIQUE_ACT_HAS,
  /* Explanation: the target tile is unknown. */
  ANEK_TGT_TILE_UNKNOWN,
  /* Explanation: the action is blocked by another action. */
  ANEK_ACTION_BLOCKS,
  /* Explanation not detected. */
  ANEK_UNKNOWN,
};

bool unit_activity_handling(struct unit *punit,
                            enum unit_activity new_activity);
bool unit_activity_handling_targeted(struct unit *punit,
                                     enum unit_activity new_activity,
                                     struct extra_type **new_target);
void unit_change_homecity_handling(struct unit *punit, struct city *new_pcity,
                                   bool rehome);

bool unit_move_handling(struct unit *punit, struct tile *pdesttile,
                        bool igzoc, bool move_diplomat_city,
                        struct unit *embark_to);

void unit_do_action(struct player *pplayer,
                    const int actor_id,
                    const int target_id,
                    const int sub_tgt_id,
                    const char *name,
                    const action_id action_type);

bool unit_perform_action(struct player *pplayer,
                         const int actor_id,
                         const int target_id,
                         const int sub_tgt_id,
                         const char *name,
                         const action_id action_type,
                         const enum action_requester requester);

void illegal_action_msg(struct player *pplayer,
                        const enum event_type event,
                        struct unit *actor,
                        const action_id stopped_action,
                        const struct tile *target_tile,
                        const struct city *target_city,
                        const struct unit *target_unit);

enum ane_kind action_not_enabled_reason(struct unit *punit,
                                        action_id act_id,
                                        const struct tile *target_tile,
                                        const struct city *target_city,
                                        const struct unit *target_unit);

#endif  /* FC__UNITHAND_H */
