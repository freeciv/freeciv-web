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

/* utility */
#include "log.h"

/* common */
#include "city.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "tile.h"
#include "unit.h"
#include "workertask.h"

/* server */
#include "citytools.h"

/* server/advisors */
#include "autosettlers.h"
#include "infracache.h"

/* ai/threaded */
#include "taimsg.h"

#include "taicity.h"

/* Task Want Multiplier */
#define TWMP 100

struct tai_worker_task_req
{
  int city_id;
  struct worker_task task;
};

enum tai_worker_task_limitation {
  TWTL_CURRENT_UNITS,
  TWTL_BUILDABLE_UNITS
};

static bool tai_city_worker_task_select(struct ai_type *ait,
                                        struct player *pplayer, struct city *pcity,
                                        struct worker_task *task,
                                        enum tai_worker_task_limitation limit);

/**********************************************************************//**
  Create worker request for the city. Only tasks that existing units can
  do are created.
**************************************************************************/
void tai_city_worker_requests_create(struct ai_type *ait,
                                     struct player *pplayer, struct city *pcity)
{
  struct worker_task task;

  if (tai_city_worker_task_select(ait, pplayer, pcity, &task, TWTL_CURRENT_UNITS)) {
    struct tai_worker_task_req *data = fc_malloc(sizeof(*data));

    data->city_id = pcity->id;
    data->task.ptile = task.ptile;
    data->task.act = task.act;
    data->task.tgt = task.tgt;
    data->task.want = task.want;

    tai_send_req(TAI_REQ_WORKER_TASK, pplayer, data);
  }
}

struct tai_tile_state
{
  int uw_max_base; /* Value for the city working the tile */
  int uw_max;      /* With road connectivity bonus */
  int worst_worked;
  int orig_worst_worked;
  int old_worst_worked;
  int *wants;
};

static int dummy_wants[U_LAST];

/**********************************************************************//**
  Select worker task suitable for the tile.
**************************************************************************/
static void tai_tile_worker_task_select(struct player *pplayer,
                                        struct city *pcity, struct tile *ptile,
                                        int cindex, struct unit_list *units,
                                        struct worker_task *worked,
                                        struct worker_task *unworked,
                                        struct tai_tile_state *state,
                                        enum tai_worker_task_limitation limit)
{
  int orig_value;
  bool potential_worst_worked = FALSE;

  if (!city_can_work_tile(pcity, ptile)) {
    return;
  }

  orig_value = city_tile_value(pcity, ptile, 0, 0);

  if (tile_worked(ptile) == pcity
      && orig_value < state->worst_worked) {
    state->worst_worked = orig_value;
    state->orig_worst_worked = orig_value;
    potential_worst_worked = TRUE;
  }

  as_transform_action_iterate(act) {
    bool consider = TRUE;
    bool possible = FALSE;
    struct extra_type *tgt = NULL;
    enum extra_cause cause;
    enum extra_rmcause rmcause;

    /* Do not request activities that already are under way. */
    unit_list_iterate(ptile->units, punit) {
      if (unit_owner(punit) == pplayer
          && unit_has_type_flag(punit, UTYF_SETTLERS)
          && punit->activity == action_id_get_activity(act)) {
        consider = FALSE;
        break;
      }
    } unit_list_iterate_end;

    if (!consider) {
      continue;
    }

    cause = activity_to_extra_cause(action_id_get_activity(act));
    rmcause = activity_to_extra_rmcause(action_id_get_activity(act));

    unit_list_iterate(units, punit) {
      if (cause != EC_NONE) {
        tgt = next_extra_for_tile(ptile, cause, pplayer, punit);
      } else if (rmcause != ERM_NONE) {
        tgt = prev_extra_in_tile(ptile, rmcause, pplayer, punit);
      }

      if (action_prob_possible(
            action_speculate_unit_on_tile(act,
                                          punit, unit_home(punit), ptile,
                                          TRUE,
                                          ptile, tgt))) {
        possible = TRUE;
        break;
      }
    } unit_list_iterate_end;

    if (possible) {
      int value = adv_city_worker_act_get(pcity, cindex,
                                          action_id_get_activity(act));

      if (tile_worked(ptile) == pcity) {
        if ((value - orig_value) * TWMP > worked->want) {
          worked->want       = TWMP * (value - orig_value);
          worked->ptile      = ptile;
          worked->act        = action_id_get_activity(act);
          worked->tgt        = NULL;
          worked->tgt        = NULL;
          if (limit == TWTL_BUILDABLE_UNITS) {
            unit_list_iterate(units, punit) {
              if (action_prob_possible(
                    action_speculate_unit_on_tile(act,
                                                  punit, unit_home(punit),
                                                  ptile,
                                                  TRUE,
                                                  ptile, tgt))) {
                state->wants[utype_index(unit_type_get(punit))] += worked->want;
              }
            } unit_list_iterate_end;
          }         
        }
        if (value > state->old_worst_worked) {
          /* After improvement it would not be the worst */
          potential_worst_worked = FALSE;
        } else {
          state->worst_worked = value;
        }
      } else {
        if (value > orig_value && value > state->uw_max) {
          state->uw_max   = value;
          state->uw_max_base = value;
          unworked->want  = TWMP * (value - orig_value);
          unworked->ptile = ptile;
          unworked->act   = action_id_get_activity(act);
          unworked->tgt   = NULL;
          if (limit == TWTL_BUILDABLE_UNITS) {
            unit_list_iterate(units, punit) {
              if (action_prob_possible(
                    action_speculate_unit_on_tile(act,
                                                  punit, unit_home(punit),
                                                  ptile,
                                                  TRUE,
                                                  ptile, tgt))) {
                state->wants[utype_index(unit_type_get(punit))] += unworked->want;
              }
            } unit_list_iterate_end;
          }
        }
      }
    }
  } as_transform_action_iterate_end;

  extra_type_iterate(tgt) {
    enum unit_activity act = ACTIVITY_LAST;
    bool removing = tile_has_extra(ptile, tgt);

    unit_list_iterate(units, punit) {
      if (removing) {
        as_rmextra_activity_iterate(try_act) {
          if (is_extra_removed_by_action(tgt, try_act)
              && auto_settlers_speculate_can_act_at(punit, try_act, TRUE,
                                                    tgt, ptile)) {
            act = try_act;
            break;
          }
        } as_rmextra_activity_iterate_end;
      } else {
        as_extra_action_iterate(try_act) {
          if (is_extra_caused_by_action(tgt,
                                        action_id_get_activity(try_act))
              && action_prob_possible(
                action_speculate_unit_on_tile(try_act,
                                              punit,
                                              unit_home(punit), ptile,
                                              TRUE,
                                              ptile, tgt))) {
            act = action_id_get_activity(try_act);
            break;
          }
        } as_extra_action_iterate_end;
      }
    } unit_list_iterate_end;

    if (act != ACTIVITY_LAST) {
      adv_want base_value;
      int value;
      adv_want extra;
      bool consider = TRUE;
      struct road_type *proad;

      /* Do not request activities that already are under way. */
      unit_list_iterate(ptile->units, punit) {
        if (unit_owner(punit) == pplayer
            && unit_has_type_flag(punit, UTYF_SETTLERS)
            && punit->activity == act) {
          consider = FALSE;
          break;
        }
      } unit_list_iterate_end;

      if (!consider) {
        continue;
      }

      proad = extra_road_get(tgt);

      if (removing) {
        base_value = adv_city_worker_rmextra_get(pcity, cindex, tgt);
      } else {
        base_value = adv_city_worker_extra_get(pcity, cindex, tgt);
      }

      if (proad != NULL && road_provides_move_bonus(proad)) {
        int old_move_cost;
        int mc_multiplier = 1;
        int mc_divisor = 1;

        /* Here 'old' means actually 'without the evaluated': In case of
         * removal activity it's the value after the removal. */
        old_move_cost = tile_terrain(ptile)->movement_cost * SINGLE_MOVE;

        extra_type_by_cause_iterate(EC_ROAD, poe) {
          struct road_type *pold = extra_road_get(poe);

          if (tile_has_extra(ptile, poe) && poe != tgt) {
            if (road_provides_move_bonus(pold)
                && pold->move_cost < old_move_cost) {
              old_move_cost = pold->move_cost;
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

        if (removing) {
          extra = -extra;
        }
      } else {
        extra = 0;
      }

      value = base_value + extra;

      if (tile_worked(ptile) == pcity) {
        if ((value - orig_value) * TWMP > worked->want) {
          worked->want  = TWMP * (value - orig_value);
          worked->ptile = ptile;
          worked->act   = act;
          worked->tgt   = tgt;
          if (limit == TWTL_BUILDABLE_UNITS) {
            unit_list_iterate(units, punit) {
              if (auto_settlers_speculate_can_act_at(punit, act, TRUE,
                                                     tgt, ptile)) {
                state->wants[utype_index(unit_type_get(punit))] += worked->want;
              }
            } unit_list_iterate_end;
          }
        }
        if (value > state->old_worst_worked) {
          /* After improvement it would not be the worst */
          potential_worst_worked = FALSE;
        } else {
          state->worst_worked = value;
        }
      } else {
        if (value > orig_value && value > state->uw_max) {
          state->uw_max   = value;
          state->uw_max_base = base_value;
          unworked->want  = TWMP * (value - orig_value);
          unworked->ptile = ptile;
          unworked->act   = act;
          unworked->tgt   = tgt;
          if (limit == TWTL_BUILDABLE_UNITS) {
            unit_list_iterate(units, punit) {
              if (auto_settlers_speculate_can_act_at(punit, act, TRUE,
                                                     tgt, ptile)) {
                state->wants[utype_index(unit_type_get(punit))] += unworked->want;
              }
            } unit_list_iterate_end;
          }
        }
      }
    }
  } extra_type_iterate_end;

  if (potential_worst_worked) {
    /* Would still be worst worked even if we improved *it*. */
    state->old_worst_worked = state->worst_worked;
  }
}

/**********************************************************************//**
  Select worker task suitable for the city.
**************************************************************************/
static bool tai_city_worker_task_select(struct ai_type *ait,
                                        struct player *pplayer, struct city *pcity,
                                        struct worker_task *task,
                                        enum tai_worker_task_limitation limit)
{
  struct worker_task *selected;
  struct worker_task worked = { .ptile = NULL, .want = 0, .act = ACTIVITY_IDLE, .tgt = NULL };
  struct worker_task unworked = { .ptile = NULL, .want = 0, .act = ACTIVITY_IDLE, .tgt = NULL };
  struct tai_tile_state state = { .uw_max = 0, .uw_max_base = 0, .worst_worked = FC_INFINITY,
                                  .orig_worst_worked = 0, .old_worst_worked = FC_INFINITY };
  struct unit_list *units = NULL;

  switch (limit) {
  case TWTL_CURRENT_UNITS:
    units = pplayer->units;
    state.wants = NULL;
    break;
  case TWTL_BUILDABLE_UNITS:
    units = unit_list_new();
    unit_type_iterate(ptype) {
      if (can_city_build_unit_now(pcity, ptype)) {
        unit_list_append(units, unit_virtual_create(pplayer, pcity, ptype, 0));
      }
    } unit_type_iterate_end;
    state.wants = dummy_wants;
    break;
  }

  city_tile_iterate_index(city_map_radius_sq_get(pcity), city_tile(pcity),
                          ptile, cindex) {
    tai_tile_worker_task_select(pplayer, pcity, ptile, cindex, units, &worked, &unworked,
                                &state, limit);
  } city_tile_iterate_end;

  if (worked.ptile == NULL
      || (state.old_worst_worked < state.uw_max
          && (state.uw_max - state.orig_worst_worked) * TWMP > worked.want)
      || (state.uw_max - state.uw_max_base * TWMP > worked.want)) {
    /* It's better to improve best yet unworked tile and take it to use after that,
       than to improve already worked tile. OR it's more important to
       improve road connectivity outside worked tiles than improve worked tiles */
    selected = &unworked;
  } else {
    selected = &worked;
  }

  if (limit == TWTL_BUILDABLE_UNITS) {
    unit_list_iterate(units, punit) {
      unit_virtual_destroy(punit);
    } unit_list_iterate_end;
    unit_list_destroy(units);
  }

  if (selected->ptile != NULL) {
    struct extra_type *target = NULL;

    if (selected->tgt == NULL) {
      enum extra_cause cause = activity_to_extra_cause(selected->act);

      if (cause != EC_NONE) {
        target = next_extra_for_tile(selected->ptile, cause, pplayer, NULL);
      } else {
        enum extra_rmcause rmcause = activity_to_extra_rmcause(selected->act);

        if (rmcause != ERM_NONE) {
          target = prev_extra_in_tile(selected->ptile, rmcause, pplayer, NULL);
        }
      }
    } else {
      target = selected->tgt;
    }

    task->ptile = selected->ptile;
    task->act = selected->act;
    task->tgt = target;
    task->want = selected->want;

    return TRUE;
  }

  return FALSE;
}

/**********************************************************************//**
  Receive message from thread to main thread.
**************************************************************************/
void tai_req_worker_task_rcv(struct tai_req *req)
{
  struct tai_worker_task_req *data = (struct tai_worker_task_req *)req->data;
  struct city *pcity;

  pcity = game_city_by_number(data->city_id);

  if (pcity != NULL && city_owner(pcity) == req->plr) {
    /* City has not been lost meanwhile */
    struct worker_task *ptask = worker_task_list_get(pcity->task_reqs, 0);

    if (ptask == NULL) {
      ptask = fc_malloc(sizeof(struct worker_task));
      worker_task_init(ptask);
      worker_task_list_append(pcity->task_reqs, ptask);
    }

    log_debug("%s storing req for act %d at (%d,%d)",
              pcity->name, data->task.act, TILE_XY(data->task.ptile));
    ptask->ptile = data->task.ptile;
    ptask->act   = data->task.act;
    ptask->tgt   = data->task.tgt;
    ptask->want  = data->task.want;

    /* Send info to observers */
    package_and_send_worker_tasks(pcity);
  }

  free(data);
}
