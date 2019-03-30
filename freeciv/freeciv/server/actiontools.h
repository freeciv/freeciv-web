/***********************************************************************
 Freeciv - Copyright (C) 1996-2015 - Freeciv Development Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__ACTIONTOOLS_H
#define FC__ACTIONTOOLS_H

/* common */
#include "actions.h"
#include "player.h"
#include "tile.h"
#include "unit.h"

void action_consequence_caught(const struct action *paction,
                               struct player *offender,
                               struct player *victim_player,
                               const struct tile *victim_tile,
                               const char *victim_link);

void action_consequence_success(const struct action *paction,
                                struct player *offender,
                                struct player *victim_player,
                                const struct tile *victim_tile,
                                const char *victim_link);

void action_success_actor_consume(struct action *paction,
                                  int actor_id, struct unit *actor);

struct city *action_tgt_city(struct unit *actor, struct tile *target_tile,
                             bool accept_all_actions);

struct unit *action_tgt_unit(struct unit *actor, struct tile *target_tile,
                             bool accept_all_actions);

struct tile *action_tgt_tile_units(struct unit *actor,
                                   struct tile *target_tile,
                                   bool accept_all_actions);

struct tile *action_tgt_tile(struct unit *actor,
                             struct tile *target_tile,
                             const struct extra_type *target_extra,
                             bool accept_all_actions);

struct extra_type *action_tgt_tile_extra(const struct unit *actor,
                                         const struct tile *target_tile,
                                         bool accept_all_actions);

const struct action_auto_perf *
action_auto_perf_unit_sel(const enum action_auto_perf_cause cause,
                          const struct unit *actor,
                          const struct player *other_player,
                          const struct output_type *output);

const struct action *
action_auto_perf_unit_do(const enum action_auto_perf_cause cause,
                         struct unit *actor,
                         const struct player *other_player,
                         const struct output_type *output,
                         const struct tile *target_tile,
                         const struct city *target_city,
                         const struct unit *target_unit,
                         const struct extra_type *target_extra);

struct act_prob
action_auto_perf_unit_prob(const enum action_auto_perf_cause cause,
                           struct unit *actor,
                           const struct player *other_player,
                           const struct output_type *output,
                           const struct tile *target_tile,
                           const struct city *target_city,
                           const struct unit *target_unit,
                           const struct extra_type *target_extra);

#endif /* FC__ACTIONTOOLS_H */
