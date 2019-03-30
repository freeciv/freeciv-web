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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>

/* utility */
#include "log.h"
#include "mem.h"
#include "support.h"
#include "timing.h"

/* common */
#include "ai.h"
#include "city.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "unitlist.h"

/* common/aicore */
#include "citymap.h"
#include "path_finding.h"
#include "pf_tools.h"

/* server */
#include "citytools.h"
#include "maphand.h"
#include "plrhand.h"
#include "srv_log.h"
#include "unithand.h"
#include "unittools.h"

/* server/advisors */
#include "advbuilding.h"
#include "advdata.h"
#include "advgoto.h"
#include "advtools.h"
#include "infracache.h"

/* ai */
#include "handicaps.h"

#include "autosettlers.h"

/* This factor is multiplied on when calculating the want.  This is done
 * to avoid rounding errors in comparisons when looking for the best
 * possible work.  However before returning the final want we have to
 * divide by it again.  This loses accuracy but is needed since the want
 * values are used for comparison by the AI in trying to calculate the
 * goodness of building worker units. */
#define WORKER_FACTOR 1024

struct settlermap {
  int enroute; /* unit ID of settler en route to this tile */
  int eta; /* estimated number of turns until enroute arrives */
};

action_id as_actions_transform[MAX_NUM_ACTIONS];
action_id as_actions_extra[MAX_NUM_ACTIONS];
Activity_type_id as_activities_rmextra[ACTIVITY_LAST];

static struct timer *as_timer = NULL;

/**********************************************************************//**
  Free resources allocated for autosettlers system
**************************************************************************/
void adv_settlers_free(void)
{
  timer_destroy(as_timer);
  as_timer = NULL;
}

/**********************************************************************//**
  Initialize advisor systems.
**************************************************************************/
void advisors_init(void)
{
  int i = 0;

  as_actions_transform[i++] = ACTION_IRRIGATE_TF;
  as_actions_transform[i++] = ACTION_MINE_TF;
  as_actions_transform[i++] = ACTION_TRANSFORM_TERRAIN;
  as_actions_transform[i++] = ACTION_NONE;

  i = 0;
  as_actions_extra[i++] = ACTION_IRRIGATE;
  as_actions_extra[i++] = ACTION_MINE;
  as_actions_extra[i++] = ACTION_ROAD;
  as_actions_extra[i++] = ACTION_BASE;
  as_actions_extra[i++] = ACTION_NONE;

  i = 0;
  as_activities_rmextra[i++] = ACTIVITY_POLLUTION;
  as_activities_rmextra[i++] = ACTIVITY_FALLOUT;
  /* We could have ACTIVITY_PILLAGE here, but currently we don't */
  as_activities_rmextra[i++] = ACTIVITY_LAST;
}

/**********************************************************************//**
  Calculate the attractiveness of building a road/rail at the given tile.

  This calculates the overall benefit of connecting the civilization; this
  is independent from the local tile (trade) bonus granted by the road.
**************************************************************************/
adv_want adv_settlers_road_bonus(struct tile *ptile, struct road_type *proad)
{
#define MAX_DEP_ROADS 5

  int bonus = 0, i;
  bool potential_road[12], real_road[12], is_slow[12];
  int dx[12] = {-1,  0,  1, -1, 1, -1, 0, 1,  0, -2, 2, 0};
  int dy[12] = {-1, -1, -1,  0, 0,  1, 1, 1, -2,  0, 0, 2};
  int x, y;
  int rnbr;
  struct road_type *pdep_roads[MAX_DEP_ROADS];
  int dep_rnbr[MAX_DEP_ROADS];
  int dep_count = 0;
  struct extra_type *pextra;

  if (proad == NULL) {
    return 0;
  }

  rnbr = road_number(proad);
  pextra = road_extra_get(proad);

  road_deps_iterate(&(pextra->reqs), pdep) {
    if (dep_count < MAX_DEP_ROADS) {
      pdep_roads[dep_count] = pdep;
      dep_rnbr[dep_count++] = road_number(pdep);
    }
  } road_deps_iterate_end;

  index_to_map_pos(&x, &y, tile_index(ptile));
  for (i = 0; i < 12; i++) {
    struct tile *tile1 = map_pos_to_tile(&(wld.map), x + dx[i], y + dy[i]);

    if (!tile1) {
      real_road[i] = FALSE;
      potential_road[i] = FALSE;
      is_slow[i] = FALSE; /* FIXME: should be TRUE? */
    } else {
      int build_time = terrain_extra_build_time(tile_terrain(tile1), ACTIVITY_GEN_ROAD, pextra);
      int j;

      real_road[i] = tile_has_road(tile1, proad);
      potential_road[i] = real_road[i];
      for (j = 0 ; !potential_road[i] && j < dep_count ; j++) {
        potential_road[i] = tile_has_road(tile1, pdep_roads[j]);
      }

      /* If TRUE, this value indicates that this tile does not need
       * a road connector.  This is set for terrains which cannot have
       * road or where road takes "too long" to build. */
      is_slow[i] = (build_time == 0 || build_time > 5);

      if (!real_road[i]) {
	unit_list_iterate(tile1->units, punit) {
          if (punit->activity == ACTIVITY_GEN_ROAD) {
            /* If a road, or its dependency is being built here, consider as if it's already
	     * built. */
            int build_rnbr;

            fc_assert(punit->activity_target != NULL);

            build_rnbr = road_number(extra_road_get(punit->activity_target));

            if (build_rnbr == rnbr) {
              real_road[i] = TRUE;
              potential_road[i] = TRUE;
            }
            for (j = 0 ; !potential_road[i] && j < dep_count ; j++) {
              if (build_rnbr == dep_rnbr[j]) {
                potential_road[i] = TRUE;
              }
            }
          }
	} unit_list_iterate_end;
      }
    }
  }

  if (current_topo_has_flag(TF_HEX)) {
    /* On hex map, road is always a benefit */
    bonus += 20; /* Later divided by 20 */

    /* Road is more valuable when even longer road around does not exist. */
    for (i = 0; i < 12; i++) {
      if (!real_road[i]) {
        bonus += 3;
      }
    }

    /* Scale down the bonus. */
    bonus /= 20;
  } else {
    /*
     * Consider the following tile arrangement (numbered in hex):
     *
     *   8
     *  012
     * 93 4A
     *  567
     *   B
     *
     * these are the tiles defined by the (dx,dy) arrays above.
     *
     * Then the following algorithm is supposed to determine if it's a good
     * idea to build a road here.  Note this won't work well for hex maps
     * since the (dx,dy) arrays will not cover the same tiles.
     *
     * FIXME: if you can understand the algorithm below please rewrite this
     * explanation!
     */
    if (potential_road[0]
        && !real_road[1] && !real_road[3]
        && (!real_road[2] || !real_road[8])
        && (!is_slow[2] || !is_slow[4] || !is_slow[7]
            || !is_slow[6] || !is_slow[5])) {
      bonus++;
    }
    if (potential_road[2]
        && !real_road[1] && !real_road[4]
        && (!real_road[7] || !real_road[10])
        && (!is_slow[0] || !is_slow[3] || !is_slow[7]
            || !is_slow[6] || !is_slow[5])) {
      bonus++;
    }
    if (potential_road[5]
        && !real_road[6] && !real_road[3]
        && (!real_road[5] || !real_road[11])
        && (!is_slow[2] || !is_slow[4] || !is_slow[7]
            || !is_slow[1] || !is_slow[0])) {
      bonus++;
    }
    if (potential_road[7]
        && !real_road[6] && !real_road[4]
        && (!real_road[0] || !real_road[9])
        && (!is_slow[2] || !is_slow[3] || !is_slow[0]
            || !is_slow[1] || !is_slow[5])) {
      bonus++;
    }

    /*   A
     *  B*B
     *  CCC
     *
     * We are at tile *.  If tile A has a road, and neither B tile does, and
     * one C tile is a valid destination, then we might want a road here.
     *
     * Of course the same logic applies if you rotate the diagram.
     */
    if (potential_road[1] && !real_road[4] && !real_road[3]
        && (!is_slow[5] || !is_slow[6] || !is_slow[7])) {
      bonus++;
    }
    if (potential_road[3] && !real_road[1] && !real_road[6]
        && (!is_slow[2] || !is_slow[4] || !is_slow[7])) {
      bonus++;
    }
    if (potential_road[4] && !real_road[1] && !real_road[6]
        && (!is_slow[0] || !is_slow[3] || !is_slow[5])) {
      bonus++;
    }
    if (potential_road[6] && !real_road[4] && !real_road[3]
        && (!is_slow[0] || !is_slow[1] || !is_slow[2])) {
      bonus++;
    }
  }

  return bonus;
}

/**********************************************************************//**
  Compares the best known tile improvement action with improving ptile
  with activity act.  Calculates the value of improving the tile by
  discounting the total value by the time it would take to do the work
  and multiplying by some factor.
**************************************************************************/
static void consider_settler_action(const struct player *pplayer, 
                                    enum unit_activity act,
                                    struct extra_type *target,
                                    adv_want extra,
                                    int new_tile_value, int old_tile_value,
                                    bool in_use, int delay,
                                    adv_want *best_value,
                                    int *best_old_tile_value,
                                    int *best_extra,
                                    bool *improve_worked,
                                    int *best_delay,
                                    enum unit_activity *best_act,
                                    struct extra_type **best_target,
                                    struct tile **best_tile,
                                    struct tile *ptile)
{
  bool improves;
  int total_value = 0, base_value = 0;
  int old_improvement_value;

  fc_assert(act != ACTIVITY_LAST);

  if (extra < 0) {
    extra = 0;
  }

  if (new_tile_value > old_tile_value) {
    improves = TRUE;
  } else if (new_tile_value == old_tile_value && extra > 0) {
    improves = TRUE;
  } else {
    improves = FALSE;
  }

  /* find the present value of the future benefit of this action */
  if (improves || extra > 0) {
    if (!(*improve_worked) && !in_use) {
      /* Going to improve tile that is not yet in use.
       * Getting the best possible total for next citizen to work on is more
       * important than amount tile gets improved. */
      if (improves && (new_tile_value > *best_value
                       || (new_tile_value == *best_value && old_tile_value < *best_old_tile_value))) {
        *best_value = new_tile_value;
        *best_old_tile_value = old_tile_value;
        *best_extra = extra;
        *best_act = act;
        *best_target = target;
        *best_tile = ptile;
        *best_delay = delay;
      }

      return;
    }

    /* At least one of the previous best or current tile is in use
     * Prefer the tile that gets improved more, regarless of the resulting total */

    base_value = new_tile_value - old_tile_value;
    total_value = base_value * WORKER_FACTOR;
    if (!in_use) {
      total_value /= 2;
    }
    total_value += extra * WORKER_FACTOR;

    /* use factor to prevent rounding errors */
    total_value = amortize(total_value, delay);

    if (*improve_worked) {
      old_improvement_value = *best_value;
    } else {
      /* Convert old best_value to improvement value compatible with in_use
       * tile value */
      old_improvement_value = amortize((*best_value - *best_old_tile_value) * WORKER_FACTOR / 2,
                                       *best_delay);
    }

    if (total_value > old_improvement_value
        || (total_value == old_improvement_value
            && old_tile_value > *best_old_tile_value)) {
      if (in_use) {
        *best_value = total_value;
        *improve_worked = TRUE;
      } else {
        *best_value = new_tile_value;
        *improve_worked = FALSE;
      }
      *best_old_tile_value = old_tile_value;
      *best_extra = extra;
      *best_act = act;
      *best_target = target;
      *best_tile = ptile;
      *best_delay = delay;
    }
  }
}

/**********************************************************************//**
  Don't enter in enemy territories.
**************************************************************************/
static enum tile_behavior
autosettler_tile_behavior(const struct tile *ptile,
                          enum known_type known,
                          const struct pf_parameter *param)
{
  const struct player *owner = tile_owner(ptile);

  if (NULL != owner && !pplayers_allied(owner, param->owner)) {
    return TB_IGNORE;
  }
  return TB_NORMAL;
}

/**********************************************************************//**
  Finds tiles to improve, using punit.

  The returned value is the goodness of the best tile and action found.  If
  this return value is > 0, then best_tile indicates the tile chosen,
  bestact indicates the activity it wants to do, and path (if not NULL)
  indicates the path to follow for the unit.  If 0 is returned
  then there are no worthwhile activities available.

  completion_time is the time that would be taken by punit to travel to
  and complete work at best_tile

  state contains, for each tile, the unit id of the worker en route,
  and the eta of this worker (if any).  This information
  is used to possibly displace this previously assigned worker.
  if this array is NULL, workers are never displaced.
**************************************************************************/
adv_want settler_evaluate_improvements(struct unit *punit,
                                       enum unit_activity *best_act,
                                       struct extra_type **best_target,
                                       struct tile **best_tile,
                                       struct pf_path **path,
                                       struct settlermap *state)
{
  const struct player *pplayer = unit_owner(punit);
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_position pos;
  int oldv;             /* Current value of consideration tile. */
  int best_oldv = 9999; /* oldv of best target so far; compared if
                         * newv == best_newv; not initialized to zero,
                         * so that newv = 0 activities are not chosen. */
  adv_want best_newv = 0;
  bool improve_worked = FALSE;
  int best_extra = 0;
  int best_delay = 0;

  /* closest worker, if any, headed towards target tile */
  struct unit *enroute = NULL;

  pft_fill_unit_parameter(&parameter, punit);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  parameter.get_TB = autosettler_tile_behavior;
  pfm = pf_map_new(&parameter);

  city_list_iterate(pplayer->cities, pcity) {
    struct tile *pcenter = city_tile(pcity);

    /* try to work near the city */
    city_tile_iterate_index(city_map_radius_sq_get(pcity), pcenter, ptile,
                            cindex) {
      bool consider = TRUE;
      bool in_use = (tile_worked(ptile) == pcity);

      if (!in_use && !city_can_work_tile(pcity, ptile)) {
        /* Don't risk bothering with this tile. */
        continue;
      }

      if (!adv_settler_safe_tile(pplayer, punit, ptile)) {
        /* Too dangerous place */
        continue;
      }

      /* Do not go to tiles that already have workers there. */
      unit_list_iterate(ptile->units, aunit) {
        if (unit_owner(aunit) == pplayer
            && aunit->id != punit->id
            && unit_has_type_flag(aunit, UTYF_SETTLERS)) {
          consider = FALSE;
        }
      } unit_list_iterate_end;

      if (!consider) {
        continue;
      }

      if (state) {
        enroute = player_unit_by_number(pplayer,
                                        state[tile_index(ptile)].enroute);
      }

      if (pf_map_position(pfm, ptile, &pos)) {
        int eta = FC_INFINITY, inbound_distance = FC_INFINITY, turns;

        if (enroute) {
          eta = state[tile_index(ptile)].eta;
          inbound_distance = real_map_distance(ptile, unit_tile(enroute));
        }

        /* Only consider this tile if we are closer in time and space to
         * it than our other worker (if any) travelling to the site. */
        if ((enroute && enroute->id == punit->id)
            || pos.turn < eta
            || (pos.turn == eta
                && (real_map_distance(ptile, unit_tile(punit))
                    < inbound_distance))) {

          if (enroute) {
            UNIT_LOG(LOG_DEBUG, punit,
                     "Considering (%d, %d) because we're closer "
                     "(%d, %d) than %d (%d, %d)",
                     TILE_XY(ptile), pos.turn,
                     real_map_distance(ptile, unit_tile(punit)),
                     enroute->id, eta, inbound_distance);
          }

          oldv = city_tile_value(pcity, ptile, 0, 0);

          /* Now, consider various activities... */
          as_transform_action_iterate(act) {
            struct extra_type *target = NULL;
            enum extra_cause cause =
                activity_to_extra_cause(action_id_get_activity(act));
            enum extra_rmcause rmcause =
                activity_to_extra_rmcause(action_id_get_activity(act));

            if (cause != EC_NONE) {
              target = next_extra_for_tile(ptile, cause, pplayer,
                                           punit);
            } else if (rmcause != ERM_NONE) {
              target = prev_extra_in_tile(ptile, rmcause, pplayer,
                                          punit);
            }

            if (adv_city_worker_act_get(pcity, cindex,
                                        action_id_get_activity(act)) >= 0
                && action_prob_possible(
                  action_speculate_unit_on_tile(act,
                                                punit, unit_home(punit),
                                                ptile,
                                                parameter.omniscience,
                                                ptile, target))) {
              int base_value =
                  adv_city_worker_act_get(pcity, cindex,
                                          action_id_get_activity(act));

              turns = pos.turn
                  + get_turns_for_activity_at(punit,
                                              action_id_get_activity(act),
                                              ptile, target);
              if (pos.moves_left == 0) {
                /* We need moves left to begin activity immediately. */
                turns++;
              }

              consider_settler_action(pplayer,
                                      action_id_get_activity(act),
                                      target, 0.0, base_value,
                                      oldv, in_use, turns,
                                      &best_newv, &best_oldv, &best_extra,
                                      &improve_worked,
                                      &best_delay, best_act, best_target,
                                      best_tile, ptile);

            } /* endif: can the worker perform this action */
          } as_transform_action_iterate_end;

          extra_type_iterate(pextra) {
            enum unit_activity act = ACTIVITY_LAST;
            enum unit_activity eval_act = ACTIVITY_LAST;
            int base_value;
            bool removing = tile_has_extra(ptile, pextra);

            if (removing) {
              as_rmextra_activity_iterate(try_act) {
                if (is_extra_removed_by_action(pextra, try_act)) {
                  /* We do not even evaluate actions we can't do.
                   * Removal is not considered prerequisite for anything */
                  if (auto_settlers_speculate_can_act_at(punit, try_act,
                          parameter.omniscience, pextra, ptile)) {
                    act = try_act;
                    eval_act = act;
                    break;
                  }
                }
              } as_rmextra_activity_iterate_end;
            } else {
              as_extra_action_iterate(try_act) {
                if (is_extra_caused_by_action(pextra,
                        action_id_get_activity(try_act))) {
                  eval_act = action_id_get_activity(try_act);
                  if (action_prob_possible(
                        action_speculate_unit_on_tile(try_act,
                                                      punit,
                                                      unit_home(punit),
                                                      ptile,
                                                      parameter.omniscience,
                                                      ptile, pextra))) {
                    act = action_id_get_activity(try_act);
                    break;
                  }
                }
              } as_extra_action_iterate_end;
            }

            if (eval_act == ACTIVITY_LAST) {
              /* No activity can provide (or remove) the extra */
              continue;
            }

            if (removing) {
              base_value = adv_city_worker_rmextra_get(pcity, cindex, pextra);
            } else {
              base_value = adv_city_worker_extra_get(pcity, cindex, pextra);
            }

            if (base_value >= 0) {
              adv_want extra;
              struct road_type *proad;

              turns = pos.turn + get_turns_for_activity_at(punit, eval_act,
                                                           ptile, pextra);
              if (pos.moves_left == 0) {
                /* We need moves left to begin activity immediately. */
                turns++;
              }

              proad = extra_road_get(pextra);

              if (proad != NULL && road_provides_move_bonus(proad)) {
                int mc_multiplier = 1;
                int mc_divisor = 1;
                int old_move_cost = tile_terrain(ptile)->movement_cost * SINGLE_MOVE;

                /* Here 'old' means actually 'without the evaluated': In case of
                 * removal activity it's the value after the removal. */

                extra_type_by_cause_iterate(EC_ROAD, pold) {
                  if (tile_has_extra(ptile, pold) && pold != pextra) {
                    struct road_type *po_road = extra_road_get(pold);

                    /* This ignores the fact that new road may be native to units that
                     * old road is not. */
                    if (po_road->move_cost < old_move_cost) {
                      old_move_cost = po_road->move_cost;
                    }
                  }
                } extra_type_by_cause_iterate_end;

                if (proad->move_cost < old_move_cost) {
                  if (proad->move_cost >= terrain_control.move_fragments) {
                    mc_divisor = proad->move_cost / terrain_control.move_fragments;
                  } else {
                    if (proad->move_cost == 0) {
                      mc_multiplier = 2;
                    } else {
                      mc_multiplier = 1 - proad->move_cost;
                    }
                    mc_multiplier += old_move_cost;
                  }
                }

                extra = adv_settlers_road_bonus(ptile, proad) * mc_multiplier / mc_divisor;

              } else {
                extra = 0;
              }

              if (extra_has_flag(pextra, EF_GLOBAL_WARMING)) {
                extra -= pplayer->ai_common.warmth;
              }
              if (extra_has_flag(pextra, EF_NUCLEAR_WINTER)) {
                extra -= pplayer->ai_common.frost;
              }

              if (removing) {
                extra = -extra;
              }

              if (act != ACTIVITY_LAST) {
                consider_settler_action(pplayer, act, pextra, extra, base_value,
                                        oldv, in_use, turns,
                                        &best_newv, &best_oldv, &best_extra,
                                        &improve_worked,
                                        &best_delay, best_act, best_target,
                                        best_tile, ptile);
              } else {
                fc_assert(!removing);

                road_deps_iterate(&(pextra->reqs), pdep) {
                  struct extra_type *dep_tgt;

                  dep_tgt = road_extra_get(pdep);

                  if (action_prob_possible(
                        action_speculate_unit_on_tile(ACTION_ROAD,
                                                      punit, unit_home(punit), ptile,
                                                      parameter.omniscience,
                                                      ptile, dep_tgt))) {
                    /* Consider building dependency road for later upgrade to target extra.
                     * Here we set value to be sum of dependency
                     * road and target extra values, which increases want, and turns is sum
                     * of dependency and target build turns, which decreases want. This can
                     * result in either bigger or lesser want than when checkin dependency
                     * road for the sake of itself when its turn in extra_type_iterate() is. */
                    int dep_turns = turns + get_turns_for_activity_at(punit,
                                                                      ACTIVITY_GEN_ROAD,
                                                                      ptile,
                                                                      dep_tgt);
                    int dep_value = base_value + adv_city_worker_extra_get(pcity, cindex, dep_tgt);

                    consider_settler_action(pplayer, ACTIVITY_GEN_ROAD, dep_tgt, extra,
                                            dep_value,
                                            oldv, in_use, dep_turns,
                                            &best_newv, &best_oldv, &best_extra,
                                            &improve_worked,
                                            &best_delay, best_act, best_target,
                                            best_tile, ptile);
                  }
                } road_deps_iterate_end;

                base_deps_iterate(&(pextra->reqs), pdep) {
                  struct extra_type *dep_tgt;

                  dep_tgt = base_extra_get(pdep);
                  if (action_prob_possible(
                        action_speculate_unit_on_tile(ACTION_BASE,
                                                      punit, unit_home(punit), ptile,
                                                      parameter.omniscience,
                                                      ptile, dep_tgt))) {
                    /* Consider building dependency base for later upgrade to
                     * target extra. See similar road implementation above for
                     * extended commentary. */
                    int dep_turns = turns + get_turns_for_activity_at(punit,
                                                                      ACTIVITY_BASE,
                                                                      ptile,
                                                                      dep_tgt);
                    int dep_value = base_value + adv_city_worker_extra_get(pcity,
                                                                           cindex,
                                                                           dep_tgt);

                    consider_settler_action(pplayer, ACTIVITY_BASE, dep_tgt,
                                            0.0, dep_value, oldv, in_use,
                                            dep_turns, &best_newv, &best_oldv,
                                            &best_extra, &improve_worked,
                                            &best_delay,
                                            best_act, best_target,
                                            best_tile, ptile);
                  }
                } base_deps_iterate_end;
              }
            }
          } extra_type_iterate_end;
        } /* endif: can we arrive sooner than current worker, if any? */
      } /* endif: are we travelling to a legal destination? */
    } city_tile_iterate_index_end;
  } city_list_iterate_end;

  if (!improve_worked) {
    /* best_newv contains total value of improved tile. Check amount of improvement
     * instead. */
    best_newv = amortize((best_newv - best_oldv + best_extra) * WORKER_FACTOR, best_delay);
  }
  best_newv /= WORKER_FACTOR;

  best_newv = MAX(best_newv, 0); /* sanity */

  if (best_newv > 0) {
    log_debug("Settler %d@(%d,%d) wants to %s at (%d,%d) with desire " ADV_WANT_PRINTF,
              punit->id, TILE_XY(unit_tile(punit)),
              get_activity_text(*best_act), TILE_XY(*best_tile), best_newv);
  } else {
    /* Fill in dummy values.  The callers should check if the return value
     * is > 0 but this will avoid confusing them. */
    *best_act = ACTIVITY_IDLE;
    *best_tile = NULL;
  }

  if (path) {
    *path = *best_tile ? pf_map_path(pfm, *best_tile) : NULL;
  }

  pf_map_destroy(pfm);

  return best_newv;
}

/**********************************************************************//**
  Return best city request to fulfill.
**************************************************************************/
struct city *settler_evaluate_city_requests(struct unit *punit,
                                            struct worker_task **best_task,
                                            struct pf_path **path,
                                            struct settlermap *state)
{
  const struct player *pplayer = unit_owner(punit);
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_position pos;
  int best_value = -1;
  struct worker_task *best = NULL;
  struct city *taskcity = NULL;
  int dist = FC_INFINITY;

  pft_fill_unit_parameter(&parameter, punit);
  parameter.omniscience = !has_handicap(pplayer, H_MAP);
  parameter.get_TB = autosettler_tile_behavior;
  pfm = pf_map_new(&parameter);

  /* Have nearby cities requests? */
  city_list_iterate(pplayer->cities, pcity) {
    worker_task_list_iterate(pcity->task_reqs, ptask) {
      bool consider = TRUE;

      /* Do not go to tiles that already have workers there. */
      unit_list_iterate(ptask->ptile->units, aunit) {
        if (unit_owner(aunit) == pplayer
            && aunit->id != punit->id
            && unit_has_type_flag(aunit, UTYF_SETTLERS)) {
          consider = FALSE;
        }
      } unit_list_iterate_end;

      if (consider
          && auto_settlers_speculate_can_act_at(punit, ptask->act,
                                                parameter.omniscience,
                                                ptask->tgt, ptask->ptile)) {
        /* closest worker, if any, headed towards target tile */
        struct unit *enroute = NULL;

        if (state) {
          enroute = player_unit_by_number(pplayer,
                                          state[tile_index(ptask->ptile)].enroute);
        }

        if (pf_map_position(pfm, ptask->ptile, &pos)) {
          int value = (ptask->want + 1) * 10 / (pos.turn + 1);

          if (value > best_value) {
            int eta = FC_INFINITY, inbound_distance = FC_INFINITY;

            if (enroute) {
              eta = state[tile_index(ptask->ptile)].eta;
              inbound_distance = real_map_distance(ptask->ptile, unit_tile(enroute));
            }

            /* Only consider this tile if we are closer in time and space to
             * it than our other worker (if any) travelling to the site. */
            if (pos.turn < dist
                && ((enroute && enroute->id == punit->id)
                    || pos.turn < eta
                    || (pos.turn == eta
                        && (real_map_distance(ptask->ptile, unit_tile(punit))
                            < inbound_distance)))) {
              dist = pos.turn;
              best = ptask;
              best_value = value;
              taskcity = pcity;
            }
          }
        }
      }
    } worker_task_list_iterate_end;
  } city_list_iterate_end;

  *best_task = best;

  if (path != NULL) {
    *path = best ? pf_map_path(pfm, best->ptile) : NULL;
  }

  pf_map_destroy(pfm);

  return taskcity;
}

/**********************************************************************//**
  Find some work for our settlers and/or workers.
**************************************************************************/
#define LOG_SETTLER LOG_DEBUG
void auto_settler_findwork(struct player *pplayer, 
                           struct unit *punit,
                           struct settlermap *state,
                           int recursion)
{
  struct worker_task *best_task;
  enum unit_activity best_act;
  struct tile *best_tile = NULL;
  struct extra_type *best_target;
  struct pf_path *path = NULL;
  struct city *taskcity;

  /* time it will take worker to complete its given task */
  int completion_time = 0;

  if (recursion > unit_list_size(pplayer->units)) {
    fc_assert(recursion <= unit_list_size(pplayer->units));
    adv_unit_new_task(punit, AUT_NONE, NULL);
    set_unit_activity(punit, ACTIVITY_IDLE);
    send_unit_info(NULL, punit);
    return; /* avoid further recursion. */
  }

  CHECK_UNIT(punit);

  fc_assert_ret(pplayer && punit);
  fc_assert_ret(unit_is_cityfounder(punit)
                || unit_has_type_flag(punit, UTYF_SETTLERS));

  /* Have nearby cities requests? */

  taskcity = settler_evaluate_city_requests(punit, &best_task, &path, state);

  if (taskcity != NULL) {
    if (path != NULL) {
      completion_time = pf_path_last_position(path)->turn;
    }

    adv_unit_new_task(punit, AUT_AUTO_SETTLER, best_tile);

    best_target = best_task->tgt;

    if (auto_settler_setup_work(pplayer, punit, state, recursion,
                                path, best_task->ptile, best_task->act,
                                &best_target, completion_time)) {
      clear_worker_task(taskcity, best_task);
    }

    if (path != NULL) {
      pf_path_destroy(path);
    }

    return;
  }

  /*** Try find some work ***/

  if (unit_has_type_flag(punit, UTYF_SETTLERS)) {
    TIMING_LOG(AIT_WORKERS, TIMER_START);
    settler_evaluate_improvements(punit, &best_act, &best_target,
                                  &best_tile, &path, state);
    if (path) {
      completion_time = pf_path_last_position(path)->turn;
    }
    TIMING_LOG(AIT_WORKERS, TIMER_STOP);

    adv_unit_new_task(punit, AUT_AUTO_SETTLER, best_tile);

    auto_settler_setup_work(pplayer, punit, state, recursion, path,
                            best_tile, best_act, &best_target,
                            completion_time);

    if (NULL != path) {
      pf_path_destroy(path);
    }
  }
}

/**********************************************************************//**
  Setup our settler to do the work it has found. Returns TRUE if
  started actual work.
**************************************************************************/
bool auto_settler_setup_work(struct player *pplayer, struct unit *punit,
                             struct settlermap *state, int recursion,
                             struct pf_path *path,
                             struct tile *best_tile,
                             enum unit_activity best_act,
                             struct extra_type **best_target,
                             int completion_time)
{
  /* Run the "autosettler" program */
  if (punit->server.adv->task == AUT_AUTO_SETTLER) {
    struct pf_map *pfm = NULL;
    struct pf_parameter parameter;
    bool working = FALSE;
    struct unit *displaced;

    if (!best_tile) {
      UNIT_LOG(LOG_DEBUG, punit, "giving up trying to improve terrain");
      return FALSE; /* We cannot do anything */
    }

    /* Mark the square as taken. */
    displaced = player_unit_by_number(pplayer,
                                      state[tile_index(best_tile)].enroute);

    if (displaced) {
      fc_assert(state[tile_index(best_tile)].enroute == displaced->id);
      fc_assert(state[tile_index(best_tile)].eta > completion_time
                || (state[tile_index(best_tile)].eta == completion_time
                    && (real_map_distance(best_tile, unit_tile(punit))
                        < real_map_distance(best_tile,
                                            unit_tile(displaced)))));
      UNIT_LOG(displaced->server.debug ? LOG_AI_TEST : LOG_DEBUG, punit,
               "%d (%d,%d) has displaced %d (%d,%d) for worksite %d,%d",
               punit->id, completion_time,
               real_map_distance(best_tile, unit_tile(punit)),
               displaced->id, state[tile_index(best_tile)].eta,
               real_map_distance(best_tile, unit_tile(displaced)),
               TILE_XY(best_tile));
    }

    state[tile_index(best_tile)].enroute = punit->id;
    state[tile_index(best_tile)].eta = completion_time;

    if (displaced) {
      struct tile *goto_tile = punit->goto_tile;
      int saved_id = punit->id;
      struct tile *old_pos = unit_tile(punit);

      displaced->goto_tile = NULL;
      auto_settler_findwork(pplayer, displaced, state, recursion + 1);
      if (NULL == player_unit_by_number(pplayer, saved_id)) {
        /* Actions of the displaced settler somehow caused this settler
         * to die. (maybe by recursively giving control back to this unit)
         */
        return FALSE;
      }
      if (goto_tile != punit->goto_tile || old_pos != unit_tile(punit)
          || punit->activity != ACTIVITY_IDLE) {
        /* Actions of the displaced settler somehow caused this settler
         * to get a new job, or to already move toward current job.
         * (A displaced B, B displaced C, C displaced A)
         */
        UNIT_LOG(LOG_DEBUG, punit,
                 "%d itself acted due to displacement recursion. "
                 "Was going from (%d, %d) to (%d, %d). "
                 "Now heading from (%d, %d) to (%d, %d).",
                 punit->id,
                 TILE_XY(old_pos), TILE_XY(goto_tile),
                 TILE_XY(unit_tile(punit)), TILE_XY(punit->goto_tile));
        return FALSE;
      }
    }

    UNIT_LOG(LOG_DEBUG, punit,
             "is heading to do %s(%s) at (%d, %d)",
             unit_activity_name(best_act),
             best_target && *best_target ? extra_rule_name(*best_target) : "-",
             TILE_XY(best_tile));

    if (!path) {
      pft_fill_unit_parameter(&parameter, punit);
      parameter.omniscience = !has_handicap(pplayer, H_MAP);
      parameter.get_TB = autosettler_tile_behavior;
      pfm = pf_map_new(&parameter);
      path = pf_map_path(pfm, best_tile);
    }

    if (path) {
      bool alive;

      alive = adv_follow_path(punit, path, best_tile);

      if (alive && same_pos(unit_tile(punit), best_tile)
	  && punit->moves_left > 0) {
	/* Reached destination and can start working immediately */
        if (activity_requires_target(best_act)) {
          unit_activity_handling_targeted(punit, best_act, best_target);
        } else {
          unit_activity_handling(punit, best_act);
        }
        send_unit_info(NULL, punit); /* FIXME: probably duplicate */

        UNIT_LOG(LOG_DEBUG, punit,
                 "reached its worksite and started work");
        working = TRUE;
      } else if (alive) {
        UNIT_LOG(LOG_DEBUG, punit,
                 "didn't start work yet; got to (%d, %d) with "
                 "%d move frags left", TILE_XY(unit_tile(punit)),
                 punit->moves_left);
      }
    } else {
      UNIT_LOG(LOG_DEBUG, punit,
               "does not find path (%d, %d) -> (%d, %d)",
               TILE_XY(unit_tile(punit)), TILE_XY(best_tile));
    }

    if (pfm) {
      pf_map_destroy(pfm);
    }

    return working;
  }

  return FALSE;
}
#undef LOG_SETTLER

/**********************************************************************//**
  Do we consider tile safe for autosettler to work?
**************************************************************************/
bool adv_settler_safe_tile(const struct player *pplayer, struct unit *punit,
                           struct tile *ptile)
{
  unit_list_iterate(ptile->units, defender) {
    if (is_military_unit(defender)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  if (is_square_threatened(pplayer, ptile, !has_handicap(pplayer, H_FOG))) {
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Run through all the players settlers and let those on ai.control work 
  automagically.
**************************************************************************/
void auto_settlers_player(struct player *pplayer) 
{
  struct settlermap *state;

  state = fc_calloc(MAP_INDEX_SIZE, sizeof(*state));

  as_timer = timer_renew(as_timer, TIMER_CPU, TIMER_DEBUG);
  timer_start(as_timer);

  if (is_ai(pplayer)) {
    /* Set up our city map. */
    citymap_turn_init(pplayer);
  }

  whole_map_iterate(&(wld.map), ptile) {
    state[tile_index(ptile)].enroute = -1;
    state[tile_index(ptile)].eta = FC_INFINITY;    
  } whole_map_iterate_end;

  /* Initialize the infrastructure cache, which is used shortly. */
  initialize_infrastructure_cache(pplayer);

  /* An extra consideration for the benefit of cleaning up pollution/fallout.
   * This depends heavily on the calculations in update_environmental_upset.
   * Aside from that it's more or less a WAG that simply grows incredibly
   * large as an environmental disaster approaches. */
  pplayer->ai_common.warmth
    = (WARMING_FACTOR * game.info.heating / ((game.info.warminglevel + 1) / 2)
       + game.info.globalwarming);
  pplayer->ai_common.frost
    = (COOLING_FACTOR * game.info.cooling / ((game.info.coolinglevel + 1) / 2)
       + game.info.nuclearwinter);

  log_debug("Warmth = %d, game.globalwarming=%d",
            pplayer->ai_common.warmth, game.info.globalwarming);
  log_debug("Frost = %d, game.nuclearwinter=%d",
            pplayer->ai_common.frost, game.info.nuclearwinter);

  /* Auto-settle with a settler unit if it's under AI control (e.g. human
   * player auto-settler mode) or if the player is an AI.  But don't
   * auto-settle with a unit under orders even for an AI player - these come
   * from the human player and take precedence. */
  unit_list_iterate_safe(pplayer->units, punit) {
    if ((punit->ai_controlled || is_ai(pplayer))
        && (unit_type_get(punit)->adv.worker
            || unit_is_cityfounder(punit))
        && !unit_has_orders(punit)
        && punit->moves_left > 0) {
      log_debug("%s %s at (%d, %d) is ai controlled.",
                nation_rule_name(nation_of_player(pplayer)),
                unit_rule_name(punit),
                TILE_XY(unit_tile(punit)));
      if (punit->activity == ACTIVITY_SENTRY) {
        unit_activity_handling(punit, ACTIVITY_IDLE);
      }
      if (punit->activity == ACTIVITY_GOTO && punit->moves_left > 0) {
        unit_activity_handling(punit, ACTIVITY_IDLE);
      }
      if (punit->activity != ACTIVITY_IDLE) {
        if (!is_ai(pplayer)) {
          if (!adv_settler_safe_tile(pplayer, punit, unit_tile(punit))) {
            unit_activity_handling(punit, ACTIVITY_IDLE);
          }
        } else {
          CALL_PLR_AI_FUNC(settler_cont, pplayer, pplayer, punit, state);
        }
      }
      if (punit->activity == ACTIVITY_IDLE) {
        if (!is_ai(pplayer)) {
          auto_settler_findwork(pplayer, punit, state, 0);
        } else {
          CALL_PLR_AI_FUNC(settler_run, pplayer, pplayer, punit, state);
        }
      }
    }
  } unit_list_iterate_safe_end;
  /* Reset auto settler state for the next run. */
  if (is_ai(pplayer)) {
    CALL_PLR_AI_FUNC(settler_reset, pplayer, pplayer);
  }

  if (timer_in_use(as_timer)) {

#ifdef LOG_TIMERS
    log_verbose("%s autosettlers consumed %g milliseconds.",
                nation_rule_name(nation_of_player(pplayer)),
                1000.0 * timer_read_seconds(as_timer));
#else
    log_verbose("%s autosettlers finished",
                nation_rule_name(nation_of_player(pplayer)));
#endif

  }

  FC_FREE(state);
}

/**********************************************************************//**
  Change unit's advisor task.
**************************************************************************/
void adv_unit_new_task(struct unit *punit, enum adv_unit_task task,
                       struct tile *ptile)
{
  if (punit->server.adv->task == task) {
    /* Already that task */
    return;
  }

  punit->server.adv->task = task;

  CALL_PLR_AI_FUNC(unit_task, unit_owner(punit), punit, task, ptile);
}

/**********************************************************************//**
  Returns TRUE iff the unit can do the targeted activity at the given
  location.

  Some activities aren't actions. They are therefore unable to use
  action_speculate_*(). Activities that are actions will use the actor's
  current tile when evaluating distance requirements etc if used with
  can_unit_do_activity_targeted_at(). This makes them return the wrong
  result. They must therefore use action_speculate_*(). Call the function
  that gives the best speculative evaluation for the specified activity.

  FIXME: Get rid of the need for this.
**************************************************************************/
bool auto_settlers_speculate_can_act_at(const struct unit *punit,
                                        enum unit_activity activity,
                                        bool omniscient_cheat,
                                        struct extra_type *target,
                                        const struct tile *ptile)
{
  struct terrain *pterrain = tile_terrain(ptile);

  switch (activity) {
  case ACTIVITY_MINE:
    if (pterrain->mining_result != pterrain
        && pterrain->mining_result != T_NONE) {
      return action_prob_possible(action_speculate_unit_on_tile(
                                    ACTION_MINE_TF,
                                    punit, unit_home(punit), ptile,
                                    omniscient_cheat,
                                    ptile, target));
    } else if (pterrain->mining_result == pterrain) {
      return action_prob_possible(action_speculate_unit_on_tile(
                                    ACTION_MINE,
                                    punit, unit_home(punit), ptile,
                                    omniscient_cheat,
                                    ptile, target));
    } else {
      return FALSE;
    }

  case ACTIVITY_IRRIGATE:
    if (pterrain->irrigation_result != pterrain
        && pterrain->irrigation_result != T_NONE) {
      return action_prob_possible(action_speculate_unit_on_tile(
                                    ACTION_IRRIGATE_TF,
                                    punit, unit_home(punit), ptile,
                                    omniscient_cheat,
                                    ptile, target));
    } else if (pterrain->irrigation_result == pterrain) {
      return action_prob_possible(action_speculate_unit_on_tile(
                                    ACTION_IRRIGATE,
                                    punit, unit_home(punit), ptile,
                                    omniscient_cheat,
                                    ptile, target));
    } else {
      return FALSE;
    }

  case ACTIVITY_FORTIFYING:
    return action_prob_possible(action_speculate_unit_on_self(
                                  ACTION_FORTIFY,
                                  punit, unit_home(punit), ptile,
                                  omniscient_cheat));

  case ACTIVITY_BASE:
    return action_prob_possible(action_speculate_unit_on_tile(
                                  ACTION_BASE,
                                  punit, unit_home(punit), ptile,
                                  omniscient_cheat,
                                  ptile, target));

  case ACTIVITY_GEN_ROAD:
    return action_prob_possible(action_speculate_unit_on_tile(
                                  ACTION_ROAD,
                                  punit, unit_home(punit), ptile,
                                  omniscient_cheat,
                                  ptile, target));

  case ACTIVITY_PILLAGE:
    return action_prob_possible(action_speculate_unit_on_tile(
                                  ACTION_PILLAGE,
                                  punit, unit_home(punit), ptile,
                                  omniscient_cheat,
                                  ptile, target));

  case ACTIVITY_TRANSFORM:
    return action_prob_possible(action_speculate_unit_on_tile(
                                  ACTION_TRANSFORM_TERRAIN,
                                  punit, unit_home(punit), ptile,
                                  omniscient_cheat,
                                  ptile, target));

  case ACTIVITY_CONVERT:
    return action_prob_possible(action_speculate_unit_on_self(
                                  ACTION_CONVERT,
                                  punit, unit_home(punit), ptile,
                                  omniscient_cheat));
  case ACTIVITY_IDLE:
  case ACTIVITY_GOTO:
  case ACTIVITY_POLLUTION:
  case ACTIVITY_FALLOUT:
  case ACTIVITY_FORTIFIED:
  case ACTIVITY_SENTRY:
  case ACTIVITY_EXPLORE:
  case ACTIVITY_OLD_ROAD:
  case ACTIVITY_OLD_RAILROAD:
  case ACTIVITY_FORTRESS:
  case ACTIVITY_AIRBASE:
  case ACTIVITY_PATROL_UNUSED:
  case ACTIVITY_LAST:
  case ACTIVITY_UNKNOWN:
    return can_unit_do_activity_targeted_at(punit, activity, target, ptile);
  }

  fc_assert(FALSE);
  return FALSE;
}
