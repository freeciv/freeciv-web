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

#include <string.h>

/* utility */
#include "log.h"
#include "mem.h"

/* common */
#include "game.h"
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "pf_tools.h"
#include "road.h"
#include "unit.h"
#include "unitlist.h"

/* client/include */
#include "client_main.h"
#include "control.h"
#include "mapview_g.h"

/* client */
#include "goto.h"
#include "mapctrl_common.h"

#define LOG_GOTO_PATH           LOG_DEBUG
#define log_goto_path           log_debug
#define log_goto_packet         log_debug

/*
 * The whole path is separated by waypoints into parts.  Each part has its
 * own starting position and requires its own map.  When the unit is unable
 * to move, end_tile equals start_tile and path is NULL.
 */
struct part {
  struct tile *start_tile, *end_tile;
  int end_moves_left, end_fuel_left;
  struct pf_path *path;
  struct pf_map *map;
};

struct goto_map {
  struct unit *focus;
  struct part *parts;
  int num_parts;
  union {
    struct {
      int initial_turns;
    } connect;
    struct {
      struct pf_path *return_path;
    } patrol;
  };
  struct pf_parameter template;
};

/* get 'struct goto_map_list' and related functions: */
#define SPECLIST_TAG goto_map
#define SPECLIST_TYPE struct goto_map
#include "speclist.h"
#define goto_map_list_iterate(gotolist, pgoto)				\
  TYPED_LIST_ITERATE(struct goto_map, gotolist, pgoto)
#define goto_map_list_iterate_end					\
  LIST_ITERATE_END

/* Iterate over goto maps, assumes no dead units. */
#define goto_map_unit_iterate(gotolist, pgoto, punit)			\
  goto_map_list_iterate(gotolist, pgoto) {				\
    struct unit *punit = goto_map_unit(pgoto);

#define goto_map_unit_iterate_end					\
  } goto_map_list_iterate_end;

static struct goto_map_list *goto_maps = NULL;
static bool goto_warned = FALSE;

static void reset_last_part(struct goto_map *goto_map);
static void remove_last_part(struct goto_map *goto_map);
static void fill_parameter_part(struct pf_parameter *param,
                                const struct goto_map *goto_map,
                                const struct part *p);

/****************************************************************************
  Various stuff for the goto routes
****************************************************************************/
static struct tile *goto_destination = NULL;

/************************************************************************//**
  Create a new goto map.
****************************************************************************/
static struct goto_map *goto_map_new(struct unit *punit)
{
  struct goto_map *goto_map = fc_malloc(sizeof(*goto_map));

  goto_map->focus = punit;
  goto_map->parts = NULL;
  goto_map->num_parts = 0;

  return goto_map;
}

/************************************************************************//**
  Free an existing goto map.
****************************************************************************/
static void goto_map_free(struct goto_map *goto_map)
{
  if (NULL != goto_map->parts) {
    while (goto_map->num_parts > 0) {
      remove_last_part(goto_map);
    }
    free(goto_map->parts);
  }
  if (hover_state == HOVER_PATROL && goto_map->patrol.return_path) {
    pf_path_destroy(goto_map->patrol.return_path);
  }
  free(goto_map);
}

/************************************************************************//**
  Returns the unit associated with the goto map.
****************************************************************************/
static struct unit *goto_map_unit(const struct goto_map *goto_map)
{
  struct unit *punit = goto_map->focus;

  fc_assert(punit != NULL);
  fc_assert(unit_is_in_focus(punit));
  fc_assert(punit == player_unit_by_number(client_player(), punit->id));
  return punit;
}

/************************************************************************//**
  Called only by handle_map_info() in client/packhand.c.
****************************************************************************/
void init_client_goto(void)
{
  free_client_goto();

  goto_maps = goto_map_list_new();
}

/************************************************************************//**
  Called above, and by control_done() in client/control.c.
****************************************************************************/
void free_client_goto(void)
{
  if (NULL != goto_maps) {
    goto_map_list_iterate(goto_maps, goto_map) {
      goto_map_free(goto_map);
    } goto_map_list_iterate_end;
    goto_map_list_destroy(goto_maps);
    goto_maps = NULL;
  }

  goto_destination = NULL;
  goto_warned = FALSE;
}

/************************************************************************//**
  Determines if a goto to the destination tile is allowed.
****************************************************************************/
bool is_valid_goto_destination(const struct tile *ptile) 
{
  return (NULL != goto_destination && ptile == goto_destination);
}

/************************************************************************//**
  Show goto lines.
****************************************************************************/
static void goto_path_redraw(const struct pf_path *new_path,
                             const struct pf_path *old_path)
{
  int start_index = 0;
  int i;

  fc_assert_ret(new_path != NULL);

  if (old_path != NULL) {
    /* We had a path drawn already. Determine how much of it we can reuse
     * in drawing the new path. */
    for (i = 0; i < new_path->length - 1 && i < old_path->length - 1; i++) {
      struct pf_position *old_pos = old_path->positions + i;
      struct pf_position *new_pos = new_path->positions + i;

      if (old_pos->dir_to_next_pos != new_pos->dir_to_next_pos
          || old_pos->tile != new_pos->tile) {
        break;
      }
    }
    start_index = i;

    /* Erase everything we cannot reuse. */
    for (; i < old_path->length - 1; i++) {
      struct pf_position *pos = old_path->positions + i;

      if (is_valid_dir(pos->dir_to_next_pos)) {
        mapdeco_remove_gotoline(pos->tile, pos->dir_to_next_pos);
      } else {
        fc_assert(pos->tile == (pos + 1)->tile);
      }
    }
  }

  /* Draw the new path. */
  for (i = start_index; i < new_path->length - 1; i++) {
    struct pf_position *pos = new_path->positions + i;

    if (is_valid_dir(pos->dir_to_next_pos)) {
      mapdeco_add_gotoline(pos->tile, pos->dir_to_next_pos);
    } else {
      fc_assert(pos->tile == (pos + 1)->tile);
    }
  }
}

/************************************************************************//**
  Remove goto lines.
****************************************************************************/
static void goto_path_undraw(const struct pf_path *path)
{
  int i;

  fc_assert_ret(path != NULL);

  for (i = 0; i < path->length - 1; i++) {
    struct pf_position *pos = path->positions + i;

    if (is_valid_dir(pos->dir_to_next_pos)) {
      mapdeco_remove_gotoline(pos->tile, pos->dir_to_next_pos);
    } else {
      fc_assert(pos->tile == (pos + 1)->tile);
    }
  }
}

/************************************************************************//**
  Change the destination of the last part to the given location.
  If a path cannot be found, the destination is set to the start.
  Return TRUE when the new path is valid.
****************************************************************************/
static bool update_last_part(struct goto_map *goto_map,
			     struct tile *ptile)
{
  struct pf_path *old_path, *new_path;
  struct part *p = &goto_map->parts[goto_map->num_parts - 1];

  old_path = p->path;
  if (old_path != NULL && pf_path_last_position(old_path)->tile == ptile) {
    /* Nothing to update. */
    return TRUE;
  }

  log_debug("update_last_part(%d,%d) old (%d,%d)-(%d,%d)",
            TILE_XY(ptile), TILE_XY(p->start_tile), TILE_XY(p->end_tile));
  new_path = pf_map_path(p->map, ptile);

  if (!new_path) {
    log_goto_path("  no path found");

    if (p->start_tile == ptile) {
      /* This mean we cannot reach the start point.  It is probably,
       * a path-finding bug, but don't make infinite recursion. */

      if (!goto_warned) {
        log_error("No path found to reach the start point.");
        goto_warned = TRUE;
      }

      if (old_path != NULL) {
        goto_path_undraw(old_path);
        pf_path_destroy(old_path);
        p->path = NULL;
      }
      if (hover_state == HOVER_PATROL
          && goto_map->patrol.return_path != NULL) {
        goto_path_undraw(goto_map->patrol.return_path);
        pf_path_destroy(goto_map->patrol.return_path);
        goto_map->patrol.return_path = NULL;
      }
      return FALSE;
    }

    reset_last_part(goto_map);
    return FALSE;
  }

  log_goto_path("  path found:");
  pf_path_print(new_path, LOG_GOTO_PATH);

  p->path = new_path;
  p->end_tile = ptile;
  p->end_moves_left = pf_path_last_position(new_path)->moves_left;
  p->end_fuel_left = pf_path_last_position(new_path)->fuel_left;

  if (hover_state == HOVER_PATROL) {
    struct pf_parameter parameter;
    struct pf_map *pfm;
    struct pf_path *return_path;

    fill_parameter_part(&parameter, goto_map, p);
    pfm = pf_map_new(&parameter);
    return_path = pf_map_path(pfm, goto_map->parts[0].start_tile);
    pf_map_destroy(pfm);

    if (return_path == NULL) {
      log_goto_path("  no return path found");

      if (p->start_tile == ptile) {
        /* This mean we cannot reach the start point.  It is probably,
         * a path-finding bug, but don't make infinite recursion. */

        if (!goto_warned) {
          log_error("No path found to reach the start point.");
          goto_warned = TRUE;
        }

        if (old_path != NULL) {
          goto_path_undraw(old_path);
          pf_path_destroy(old_path);
        }
        pf_path_destroy(new_path);
        p->path = NULL;
        if (goto_map->patrol.return_path != NULL) {
          goto_path_undraw(goto_map->patrol.return_path);
          pf_path_destroy(goto_map->patrol.return_path);
          goto_map->patrol.return_path = NULL;
        }
        return FALSE;
      }

      p->path = old_path;
      p->end_tile = pf_path_last_position(old_path)->tile;
      p->end_moves_left = pf_path_last_position(old_path)->moves_left;
      p->end_fuel_left = pf_path_last_position(old_path)->fuel_left;
      pf_path_destroy(new_path);
      reset_last_part(goto_map);
      return FALSE;
    }

    log_goto_path("  returned path found:");
    pf_path_print(return_path, LOG_GOTO_PATH);

    if (goto_map->patrol.return_path != NULL) {
      /* We cannot re-use old path because:
       * 1- the start tile isn't the same.
       * 2- the turn number neither (impossible to do in backward mode). */
      goto_path_undraw(goto_map->patrol.return_path);
      pf_path_destroy(goto_map->patrol.return_path);
    }
    goto_path_redraw(return_path, NULL);
    goto_map->patrol.return_path = return_path;
  }

  goto_path_redraw(new_path, old_path);
  pf_path_destroy(old_path);

  log_goto_path("To (%d,%d) part %d: total_MC: %d",
                TILE_XY(ptile), goto_map->num_parts,
                pf_path_last_position(hover_state == HOVER_PATROL
                                      ? goto_map->patrol.return_path
                                      : new_path)->total_MC);

  return TRUE;
}

/************************************************************************//**
  Change the drawn path to a size of 0 steps by setting it to the
  start position.
****************************************************************************/
static void reset_last_part(struct goto_map *goto_map)
{
  struct part *p = &goto_map->parts[goto_map->num_parts - 1];

  if (NULL != p->path) {
    /* Otherwise no need to update */
    update_last_part(goto_map, p->start_tile);
  }
}

/************************************************************************//**
  Fill pathfinding parameter for adding a new part.
****************************************************************************/
static void fill_parameter_part(struct pf_parameter *param,
                                const struct goto_map *goto_map,
                                const struct part *p)
{
  *param = goto_map->template;

  if (p->start_tile == p->end_tile) {
    /* Copy is enough, we didn't move last part. */
    fc_assert(p->path->length == 1);
    return;
  }

  param->start_tile = p->end_tile;
  param->moves_left_initially = p->end_moves_left;
  param->fuel_left_initially = p->end_fuel_left;
  if (can_exist_at_tile(&(wld.map), param->utype, param->start_tile)) {
    param->transported_by_initially = NULL;
  } else {
    const struct unit *transporter =
        transporter_for_unit_at(goto_map_unit(goto_map), param->start_tile);

    param->transported_by_initially = (transporter != NULL
                                       ? unit_type_get(transporter) : NULL);
  }
}

/************************************************************************//**
  Add a part. Depending on the num of already existing parts the start
  of the new part is either the unit position (for the first part) or
  the destination of the last part (not the first part).
****************************************************************************/
static void add_part(struct goto_map *goto_map)
{
  struct part *p;
  struct pf_parameter parameter;
  struct unit *punit = goto_map_unit(goto_map);

  goto_map->num_parts++;
  goto_map->parts =
      fc_realloc(goto_map->parts,
                 goto_map->num_parts * sizeof(*goto_map->parts));
  p = &goto_map->parts[goto_map->num_parts - 1];

  if (goto_map->num_parts == 1) {
    /* first part */
    p->start_tile = unit_tile(punit);
    parameter = goto_map->template;
  } else {
    struct part *prev = &goto_map->parts[goto_map->num_parts - 2];

    p->start_tile = prev->end_tile;
    fill_parameter_part(&parameter, goto_map, prev);
   }
  p->path = NULL;
  p->end_tile = p->start_tile;
  parameter.start_tile = p->start_tile;
  p->map = pf_map_new(&parameter);
}

/************************************************************************//**
  Remove the last part, erasing the corresponding path segment.
****************************************************************************/
static void remove_last_part(struct goto_map *goto_map)
{
  struct part *p = &goto_map->parts[goto_map->num_parts - 1];

  fc_assert_ret(goto_map->num_parts >= 1);

  reset_last_part(goto_map);
  if (p->path) {
    /* We do not always have a path */
    pf_path_destroy(p->path);
  }
  pf_map_destroy(p->map);
  goto_map->num_parts--;
}

/************************************************************************//**
  Inserts a waypoint at the end of the current goto line.
****************************************************************************/
bool goto_add_waypoint(void)
{
  bool duplicate_of_last = TRUE;

  fc_assert_ret_val(goto_is_active(), FALSE);
  if (NULL == goto_destination) {
    /* Not a valid position. */
    return FALSE;
  }

  goto_map_list_iterate(goto_maps, goto_map) {
    const struct part *last_part = &goto_map->parts[goto_map->num_parts - 1];

    if (last_part->path == NULL) {
      /* The current part has zero length. */
      return FALSE;
    }
    if (last_part->start_tile != last_part->end_tile) {
      duplicate_of_last = FALSE;
    }
  } goto_map_list_iterate_end;
  if (duplicate_of_last) {
    return FALSE;
  }

  goto_map_list_iterate(goto_maps, goto_map) {
    add_part(goto_map);
  } goto_map_list_iterate_end;

  refresh_tile_mapcanvas(goto_destination, FALSE, FALSE);
  return TRUE;
}

/************************************************************************//**
  Returns whether there were any waypoint popped (we don't remove the
  initial position)
****************************************************************************/
bool goto_pop_waypoint(void)
{
  bool popped = FALSE;

  fc_assert_ret_val(goto_is_active(), FALSE);
  goto_map_list_iterate(goto_maps, goto_map) {
    struct part *p = &goto_map->parts[goto_map->num_parts - 1];
    struct tile *end_tile = p->end_tile;

    if (goto_map->num_parts == 1) {
      /* we don't have any waypoint but the start pos. */
      continue;
    }
    popped = TRUE;

    remove_last_part(goto_map);

    /* 
     * Set the end position of the previous part (now the last) to the
     * end position of the last part (now gone). I.e. redraw a line to
     * the mouse position. 
     */
    update_last_part(goto_map, end_tile);
  } goto_map_list_iterate_end;
  return popped;
}

/************************************************************************//**
  PF callback to get the path with the minimal number of steps (out of 
  all shortest paths).
****************************************************************************/
static int get_EC(const struct tile *ptile, enum known_type known,
                  const struct pf_parameter *param)
{
  return 1;
}

/************************************************************************//**
  PF callback to prohibit going into the unknown.  Also makes sure we 
  don't plan our route through enemy city/tile.
****************************************************************************/
static enum tile_behavior get_TB_aggr(const struct tile *ptile,
                                      enum known_type known,
                                      const struct pf_parameter *param)
{
  if (known == TILE_UNKNOWN) {
    if (!gui_options.goto_into_unknown) {
      return TB_IGNORE;
    }
  } else if (is_non_allied_unit_tile(ptile, param->owner)
	     || is_non_allied_city_tile(ptile, param->owner)) {
    /* Can attack but can't count on going through */
    return TB_DONT_LEAVE;
  }
  return TB_NORMAL;
}

/************************************************************************//**
  PF callback for caravans. Caravans doesn't go into the unknown and
  don't attack enemy units but enter enemy cities.
****************************************************************************/
static enum tile_behavior get_TB_caravan(const struct tile *ptile,
                                         enum known_type known,
                                         const struct pf_parameter *param)
{
  if (known == TILE_UNKNOWN) {
    if (!gui_options.goto_into_unknown) {
      return TB_IGNORE;
    }
  } else if (is_non_allied_city_tile(ptile, param->owner)) {
    /* Units that can establish a trade route, enter a market place or
     * establish an embassy can travel to, but not through, enemy cities.
     * FIXME: ACTION_HELP_WONDER units cannot.  */
    return TB_DONT_LEAVE;
  } else if (is_non_allied_unit_tile(ptile, param->owner)) {
    /* Note this must be below the city check. */
    return TB_IGNORE;
  }

  /* Includes empty, allied, or allied-city tiles. */
  return TB_NORMAL;
}

/************************************************************************//**
  Return the number of MP needed to do the connect activity at this
  position.  A negative number means it's impossible.
****************************************************************************/
static int get_activity_time(const struct tile *ptile,
			     const struct player *pplayer)
{
  struct terrain *pterrain = tile_terrain(ptile);
  int activity_mc = 0;

  fc_assert_ret_val(hover_state == HOVER_CONNECT, -1);
 
  switch (connect_activity) {
  case ACTIVITY_IRRIGATE:
    if (pterrain->irrigation_time == 0) {
      return -1;
    }
    extra_type_iterate(pextra) {
      if (BV_ISSET(connect_tgt->conflicts, extra_index(pextra))
          && tile_has_extra(ptile, pextra)) {
        /* Don't replace old extras. */
        return -1;
      }
    } extra_type_iterate_end;

    if (tile_has_extra(ptile, connect_tgt)) {
      break;
    }

    activity_mc = pterrain->irrigation_time;
    break;
  case ACTIVITY_GEN_ROAD:
    fc_assert(is_extra_caused_by(connect_tgt, EC_ROAD));

    if (!tile_has_extra(ptile, connect_tgt)) {
      struct tile *vtile;
      int single_mc;

      vtile = tile_virtual_new(ptile);
      single_mc = check_recursive_road_connect(vtile, connect_tgt, NULL, pplayer, 0);
      tile_virtual_destroy(vtile);

      if (single_mc < 0) {
        return -1;
      }

      activity_mc += single_mc;
    }
    break;
  default:
    log_error("Invalid connect activity: %d.", connect_activity);
  }

  return activity_mc;
}

/************************************************************************//**
  When building a road or a railroad, we don't want to go next to
  nonallied cities
****************************************************************************/
static bool is_non_allied_city_adjacent(const struct player *pplayer,
                                        const struct tile *ptile)
{
  adjc_iterate(&(wld.map), ptile, tile1) {
    if (is_non_allied_city_tile(tile1, pplayer)) {
      return TRUE;
    }
  } adjc_iterate_end;

  return FALSE;
}

/************************************************************************//**
  PF jumbo callback for the cost of a connect by road.
  In road-connect mode we are concerned with
  (1) the number of steps of the resulting path
  (2) (the tie-breaker) time to build the path (travel plus activity time).
  In rail-connect the priorities are reversed.

  param->data should contain the result of get_activity_rate(punit).
****************************************************************************/
static int get_connect_road(const struct tile *src_tile, enum direction8 dir,
                            const struct tile *dest_tile,
                            int src_cost, int src_extra,
                            int *dest_cost, int *dest_extra,
                            const struct pf_parameter *param)
{
  int activity_time, move_cost, moves_left;
  int total_cost, total_extra;
  struct road_type *proad;

  if (tile_get_known(dest_tile, param->owner) == TILE_UNKNOWN) {
    return -1;
  }

  activity_time = get_activity_time(dest_tile, param->owner);  
  if (activity_time < 0) {
    return -1;
  }

  move_cost = param->get_MC(src_tile, PF_MS_NATIVE, dest_tile, PF_MS_NATIVE,
                            param);
  if (move_cost == PF_IMPOSSIBLE_MC) {
    return -1;
  }

  if (is_non_allied_city_adjacent(param->owner, dest_tile)) {
    /* We don't want to build roads to enemies plus get ZoC problems */
    return -1;
  }

  fc_assert(connect_activity == ACTIVITY_GEN_ROAD);

  proad = extra_road_get(connect_tgt);

  if (proad == NULL) {
    /* No suitable road type available */
    return -1;
  }

  /* Ok, the move is possible.  What are the costs? */

  /* Extra cost here is the final length of the road */
  total_extra = src_extra + 1;

  /* Special cases: get_MC function doesn't know that we would have built
   * a road (railroad) on src tile by that time.
   * We assume that settler building the road can also travel it. */
  if (tile_has_road(dest_tile, proad)) {
    move_cost = proad->move_cost;
  }

  move_cost = MIN(move_cost, param->move_rate);
  total_cost = src_cost;
  moves_left = param->move_rate - (src_cost % param->move_rate);
  if (moves_left < move_cost) {
    total_cost += moves_left;
  } else {
    total_cost += move_cost;
  }

  /* Now need to include the activity cost.  If we have moves left, they
   * will count as a full turn towards the activity time */
  moves_left = param->move_rate - (total_cost % param->move_rate);
  if (activity_time > 0) {
    int speed = FC_PTR_TO_INT(param->data);

    activity_time = ((activity_time * ACTIVITY_FACTOR)
                     + (speed - 1)) / speed;
    activity_time--;
    total_cost += moves_left;
  }
  total_cost += activity_time * param->move_rate;

  /* Now we determine if we have found a better path.  When building
   * road type with positive move_cost, we care most about the length
   * of the result.  When building road type with move_cost 0, we
   * care most about construction time. */

  /* *dest_cost == -1 means we haven't reached dest until now */

  if (*dest_cost != -1) {
    if (proad->move_cost > 0) {
      if (total_extra > *dest_extra 
          || (total_extra == *dest_extra && total_cost >= *dest_cost)) {
        /* No, this path is worse than what we already have */
        return -1;
      }
    } else {
      if (total_cost > *dest_cost 
          || (total_cost == *dest_cost && total_extra >= *dest_extra)) {
        return -1;
      }
    }
  }

  /* Ok, we found a better path! */  
  *dest_cost = total_cost;
  *dest_extra = total_extra;
  
  return (proad->move_cost > 0 ? 
	  total_extra * PF_TURN_FACTOR + total_cost : 
	  total_cost * PF_TURN_FACTOR + total_extra);
}

/************************************************************************//**
  PF jumbo callback for the cost of a connect by irrigation.
  Here we are only interested in how long it will take to irrigate the path.

  param->data should contain the result of get_activity_rate(punit) / 10.
****************************************************************************/
static int get_connect_irrig(const struct tile *src_tile,
                             enum direction8 dir,
                             const struct tile *dest_tile,
                             int src_cost, int src_extra,
                             int *dest_cost, int *dest_extra,
                             const struct pf_parameter *param)
{
  int activity_time, move_cost, moves_left, total_cost;

  if (tile_get_known(dest_tile, param->owner) == TILE_UNKNOWN) {
    return -1;
  }

  activity_time = get_activity_time(dest_tile, param->owner);  
  if (activity_time < 0) {
    return -1;
  }

  if (!is_cardinal_dir(dir)) {
    return -1;
  }

  move_cost = param->get_MC(src_tile, PF_MS_NATIVE, dest_tile, PF_MS_NATIVE,
                            param);
  if (move_cost == PF_IMPOSSIBLE_MC) {
    return -1;
  }

  if (is_non_allied_city_adjacent(param->owner, dest_tile)) {
    /* We don't want to build irrigation for enemies plus get ZoC problems */
    return -1;
  }

  /* Ok, the move is possible.  What are the costs? */

  move_cost = MIN(move_cost, param->move_rate);
  total_cost = src_cost;
  moves_left = param->move_rate - (src_cost % param->move_rate);
  if (moves_left < move_cost) {
    total_cost += moves_left;
  } else {
    total_cost += move_cost;
  }

  /* Now need to include the activity cost.  If we have moves left, they
   * will count as a full turn towards the activity time */
  moves_left = param->move_rate - (total_cost % param->move_rate);
  if (activity_time > 0) {
    int speed = FC_PTR_TO_INT(param->data);

    activity_time = ((activity_time * ACTIVITY_FACTOR)
                     + (speed - 1)) / speed;
    activity_time--;
    total_cost += moves_left;
  }
  total_cost += activity_time * param->move_rate;

  /* *dest_cost == -1 means we haven't reached dest until now */
  if (*dest_cost != -1 && total_cost > *dest_cost) {
    return -1;
  }

  /* Ok, we found a better path! */  
  *dest_cost = total_cost;
  *dest_extra = 0;
  
  return total_cost;
}

/************************************************************************//**
  PF callback to prohibit going into the unknown (conditionally).  Also
  makes sure we don't plan to attack anyone.
****************************************************************************/
static enum tile_behavior
no_fights_or_unknown_goto(const struct tile *ptile,
                          enum known_type known,
                          const struct pf_parameter *p)
{
  if (known == TILE_UNKNOWN && gui_options.goto_into_unknown) {
    /* Special case allowing goto into the unknown. */
    return TB_NORMAL;
  }

  return no_fights_or_unknown(ptile, known, p);
}

/************************************************************************//**
  Fill the PF parameter with the correct client-goto values.
  See also goto_fill_parameter_full().
****************************************************************************/
static void goto_fill_parameter_base(struct pf_parameter *parameter,
                                     const struct unit *punit)
{
  pft_fill_unit_parameter(parameter, punit);

  fc_assert(parameter->get_EC == NULL);
  fc_assert(parameter->get_TB == NULL);
  fc_assert(parameter->get_MC != NULL);
  fc_assert(parameter->start_tile == unit_tile(punit));
  fc_assert(parameter->omniscience == FALSE);

  parameter->get_EC = get_EC;
  if (utype_acts_hostile(unit_type_get(punit))) {
    parameter->get_TB = get_TB_aggr;
  } else if (utype_may_act_at_all(unit_type_get(punit))
             && !utype_acts_hostile(unit_type_get(punit))) {
    parameter->get_TB = get_TB_caravan;
  } else {
    parameter->get_TB = no_fights_or_unknown_goto;
  }
}

/************************************************************************//**
  Fill the PF parameter with the correct client-goto values.
  The storage behind "connect_speed" must remain valid for the lifetime
  of the pf_map.

  Note that you must call this function only if the 'hover_state' is set,
  and we are in goto mode (usually, this function is only called from
  enter_goto_state()).

  See also goto_fill_parameter_base().
****************************************************************************/
static void goto_fill_parameter_full(struct goto_map *goto_map,
                                     const struct unit *punit)
{
  struct pf_parameter *parameter = &goto_map->template;

  goto_fill_parameter_base(parameter, punit);

  fc_assert_ret(goto_is_active());

  switch (hover_state) {
  case HOVER_CONNECT:
    {
      int activity_initial;
      int speed;

      if (connect_activity == ACTIVITY_IRRIGATE) {
        parameter->get_costs = get_connect_irrig;
      } else {
        parameter->get_costs = get_connect_road;
      }
      parameter->get_moves_left_req = NULL;

      speed = get_activity_rate(punit);
      parameter->data = FC_INT_TO_PTR(speed);

      /* Take into account the activity time at the origin */
      activity_initial = get_activity_time(unit_tile(punit),
                                           unit_owner(punit));

      if (activity_initial > 0) {
        /* First action is activity */
        parameter->moves_left_initially = parameter->move_rate;
        /* Number of turns, rounding up */
        goto_map->connect.initial_turns =
            ((activity_initial * ACTIVITY_FACTOR + (speed - 1)) / speed);
        if (punit->moves_left == 0) {
          goto_map->connect.initial_turns++;
        }
      } else {
        goto_map->connect.initial_turns = 0;
      }
    }
    break;
  case HOVER_GOTO:
  case HOVER_PATROL:
    if (goto_last_action == ACTION_NUKE) {
      /* We only want targets reachable immediatly... */
      parameter->move_rate = 0;
      /* ...then we don't need to deal with dangers or refuel points. */
      parameter->is_pos_dangerous = NULL;
      parameter->get_moves_left_req = NULL;
    } else {
      goto_map->patrol.return_path = NULL;
    }
    break;
  case HOVER_NONE:
  case HOVER_PARADROP:
  case HOVER_ACT_SEL_TGT:
    fc_assert_msg(hover_state != HOVER_NONE, "Goto with HOVER_NONE?");
    fc_assert_msg(hover_state != HOVER_PARADROP,
                  "Goto with HOVER_PARADROP?");
    fc_assert_msg(hover_state != HOVER_ACT_SEL_TGT,
                  "Goto with HOVER_ACT_SEL_TGT?");
    break;
  };
}

/************************************************************************//**
  Enter the goto state: activate, prepare PF-template and add the
  initial part.
****************************************************************************/
void enter_goto_state(struct unit_list *punits)
{
  fc_assert_ret(!goto_is_active());

  /* Can't have selection rectangle and goto going on at the same time. */
  cancel_selection_rectangle();

  unit_list_iterate(punits, punit) {
    struct goto_map *goto_map = goto_map_new(punit);

    goto_map_list_append(goto_maps, goto_map);

    goto_fill_parameter_full(goto_map, punit);
    add_part(goto_map);
  } unit_list_iterate_end;
  goto_warned = FALSE;
}

/************************************************************************//**
  Tidy up and deactivate goto state.
****************************************************************************/
void exit_goto_state(void)
{
  if (!goto_is_active()) {
    return;
  }

  goto_map_list_iterate(goto_maps, goto_map) {
    goto_map_free(goto_map);
  } goto_map_list_iterate_end;
  goto_map_list_clear(goto_maps);

  goto_destination = NULL;
  goto_warned = FALSE;
}

/************************************************************************//**
  Called from control_unit_killed() in client/control.c
****************************************************************************/
void goto_unit_killed(struct unit *punit)
{
  if (!goto_is_active()) {
    return;
  }

  goto_map_unit_iterate(goto_maps, goto_map, ptest) {
    if (ptest == punit) {
      goto_map_free(goto_map);
      /* Still safe using goto_map pointer, as it is not used for 
       * dereferencing, only as value. */
      goto_map_list_remove(goto_maps, goto_map);
      /* stop now, links are gone! */
      break;
    }
  } goto_map_unit_iterate_end;
}

/************************************************************************//**
  Is goto state active?
****************************************************************************/
bool goto_is_active(void)
{
  return (NULL != goto_maps && 0 != goto_map_list_size(goto_maps));
}

/************************************************************************//**
  Return the path length (in turns).
  WARNING: not useful for determining paths of scattered groups.
****************************************************************************/
bool goto_get_turns(int *min, int *max)
{
  fc_assert_ret_val(min != NULL, FALSE);
  fc_assert_ret_val(max != NULL, FALSE);

  *min = FC_INFINITY;
  *max = -1;

  if (!goto_is_active()) {
    return FALSE;
  }
  if (NULL == goto_destination) {
    /* Not a valid position. */
    return FALSE;
  }

  if (hover_state == HOVER_CONNECT) {
    /* In connect mode, we want to know the turn number the activity will
     * be finished. */
    int activity_time = get_activity_time(goto_destination, client_player());

    goto_map_list_iterate(goto_maps, goto_map) {
      bool moved = FALSE;
      int turns = goto_map->connect.initial_turns;
      int i;

      for (i = 0; i < goto_map->num_parts; i++) {
        const struct pf_path *path = goto_map->parts[i].path;

        turns += pf_path_last_position(goto_map->parts[i].path)->turn;
        if (!moved && path->length > 1) {
          moved = TRUE;
        }
      }

      if (moved && activity_time > 0) {
        turns++;
      }

      if (turns < *min) {
        *min = turns;
      }
      if (turns > *max) {
        *max = turns;
      }
    } goto_map_list_iterate_end;
  } else {
    /* In other modes, we want to know the turn number to reach the tile. */
    goto_map_list_iterate(goto_maps, goto_map) {
      int turns = 0;
      int i;

      for (i = 0; i < goto_map->num_parts; i++) {
        turns += pf_path_last_position(goto_map->parts[i].path)->turn;
      }
      if (hover_state == HOVER_PATROL
          && goto_map->patrol.return_path != NULL) {
        turns += pf_path_last_position(goto_map->patrol.return_path)->turn;
      }

      if (turns < *min) {
        *min = turns;
      }
      if (turns > *max) {
        *max = turns;
      }
    } goto_map_list_iterate_end;
  }

  return TRUE;
}

/************************************************************************//**
  Returns the state of 'ptile': turn number to print, and whether 'ptile'
  is a waypoint.
****************************************************************************/
bool goto_tile_state(const struct tile *ptile, enum goto_tile_state *state,
                     int *turns, bool *waypoint)
{
  fc_assert_ret_val(ptile != NULL, FALSE);
  fc_assert_ret_val(turns != NULL, FALSE);
  fc_assert_ret_val(waypoint != NULL, FALSE);

  if (!goto_is_active()) {
    return FALSE;
  }

  *state = -1;
  *turns = -1;
  *waypoint = FALSE;

  if (hover_state == HOVER_CONNECT) {
    /* In connect mode, we want to know the turn number the activity will
     * be finished. */
    int activity_time;

    if (tile_get_known(ptile, client_player()) == TILE_UNKNOWN) {
      return FALSE; /* We never connect on unknown tiles. */
    }

    activity_time = get_activity_time(ptile, client_player());

    goto_map_list_iterate(goto_maps, goto_map) {
      const struct pf_path *path;
      const struct pf_position *pos = NULL; /* Keep compiler happy! */
      int map_turns = goto_map->connect.initial_turns;
      int turns_for_map = -2;
      bool moved = FALSE;
      int i, j;

      for (i = 0; i < goto_map->num_parts; i++) {
        if (i > 0 && goto_map->parts[i].start_tile == ptile) {
          *waypoint = TRUE;
        }

        path = goto_map->parts[i].path;
        if (path == NULL) {
          continue;
        }

        for (j = 0; j < path->length; j++) {
          pos = path->positions + j;
          if (!moved && j > 0) {
            moved = TRUE;
          }
          if (pos->tile != ptile) {
            continue;
          }
          if (activity_time > 0) {
            if (map_turns + pos->turn + moved > turns_for_map) {
              turns_for_map = map_turns + pos->turn + moved;
            }
          } else if (pos->moves_left == 0) {
            if (map_turns + pos->turn > turns_for_map) {
              turns_for_map = map_turns + pos->turn + moved;
            }
          }
        }
        map_turns += pos->turn;
      }

      if (ptile == goto_destination) {
        fc_assert_ret_val(pos != NULL, FALSE);
        if (moved && activity_time > 0) {
          map_turns++;
        }
        if (map_turns > *turns) {
          *state = (activity_time > 0 || pos->moves_left == 0
                    ? GTS_EXHAUSTED_MP : GTS_MP_LEFT);
          *turns = map_turns;
        } else if (map_turns == *turns
                   && *state == GTS_MP_LEFT
                   && (activity_time > 0 || pos->moves_left == 0)) {
          *state = GTS_EXHAUSTED_MP;
        }
      } else {
        if (activity_time > 0) {
          if (turns_for_map > *turns) {
            *state = GTS_TURN_STEP;
            *turns = turns_for_map;
          }
        } else {
          if (turns_for_map + 1 > *turns) {
            *state = GTS_TURN_STEP;
            *turns = turns_for_map + 1;
          }
        }
      }
    } goto_map_list_iterate_end;
  } else {
    bool mark_on_map = FALSE;
    /* In other modes, we want to know the turn number to reach the tile. */
    goto_map_list_iterate(goto_maps, goto_map) {
      const struct tile *destination;
      const struct pf_path *path;
      const struct pf_position *pos = NULL; /* Keep compiler happy! */
      const struct pf_position *last_pos = NULL;
      int map_turns = 0;
      int turns_for_map = -2;
      int i, j;

      for (i = 0; i < goto_map->num_parts; i++) {
        if (i > 0 && goto_map->parts[i].start_tile == ptile) {
          *waypoint = TRUE;
        }

        path = goto_map->parts[i].path;
        if (path == NULL) {
          continue;
        }
        last_pos = path->positions;
        for (j = 0; j < path->length; j++) {
          pos = path->positions + j;
          /* turn to reach was increased in that step */
          if (pos->turn != last_pos->turn
              && pos->tile == ptile) {
            mark_on_map = TRUE;
          }
          if (pos->moves_left == 0 && last_pos->moves_left != 0
              && pos->tile == ptile) {
            mark_on_map = TRUE;
          }
          if (pos->tile == ptile
              /* End turn case. */
              && (pos->moves_left == 0
                  /* Waiting case. */
                  || (j < path->length - 1 && (pos + 1)->tile == ptile))
              && map_turns + pos->turn > turns_for_map) {
            turns_for_map = map_turns + pos->turn;
          }
          last_pos = pos;
        }
        map_turns += pos->turn;
      }

      if (hover_state == HOVER_PATROL
          && goto_map->patrol.return_path != NULL) {
        path = goto_map->patrol.return_path;
        for (j = 0; j < path->length; j++) {
          pos = path->positions + j;
          if (pos->tile == ptile
              /* End turn case. */
              && (pos->moves_left == 0
                  /* Waiting case. */
                  || (j < path->length - 1 && (pos + 1)->tile == ptile))
              && map_turns + pos->turn > turns_for_map) {
            turns_for_map = map_turns + pos->turn;
          }
        }
        map_turns += pos->turn;
        destination = pos->tile;
      } else {
        destination = goto_destination;
      }

      if (ptile == destination) {
        fc_assert_ret_val(pos != NULL, FALSE);
        if (map_turns > *turns) {
          mark_on_map = TRUE;
          *state = (pos->moves_left == 0 ? GTS_EXHAUSTED_MP : GTS_MP_LEFT);
          *turns = map_turns;
        } else if (map_turns == *turns
                   && *state == GTS_MP_LEFT
                   && pos->moves_left == 0) {
          *state = GTS_EXHAUSTED_MP;
        }
      } else {
        if (turns_for_map > *turns) {
          *state = GTS_TURN_STEP;
          *turns = turns_for_map;
        }
      }
    } goto_map_list_iterate_end;
    return mark_on_map;
  }

  return (*turns != -1 || *waypoint);
}

/************************************************************************//**
  Puts a line to dest_tile on the map according to the current
  goto_map.
  If there is no route to the dest then don't draw anything.
****************************************************************************/
bool is_valid_goto_draw_line(struct tile *dest_tile)
{
  fc_assert_ret_val(goto_is_active(), FALSE);
  if (NULL == dest_tile) {
    return FALSE;
  }

  /* assume valid destination */
  goto_destination = dest_tile;

  goto_map_list_iterate(goto_maps, goto_map) {
    if (!update_last_part(goto_map, dest_tile)) {
      goto_destination = NULL;
    }
  } goto_map_list_iterate_end;

  /* Update goto data in info label. */
  update_unit_info_label(get_units_in_focus());
  return (NULL != goto_destination);
}

/************************************************************************//**
  Send a packet to the server to request that the current orders be
  cleared.
****************************************************************************/
void request_orders_cleared(struct unit *punit)
{
  struct packet_unit_orders p;

  if (!can_client_issue_orders()) {
    return;
  }

  /* Clear the orders by sending an empty orders path. */
  log_goto_packet("Clearing orders for unit %d.", punit->id);
  p.unit_id = punit->id;
  p.src_tile = tile_index(unit_tile(punit));
  p.repeat = p.vigilant = FALSE;
  p.length = 0;
  p.dest_tile = tile_index(unit_tile(punit));
  send_packet_unit_orders(&client.conn, &p);
}

/************************************************************************//**
  Send a path as a goto or patrol route to the server.
****************************************************************************/
static void send_path_orders(struct unit *punit, struct pf_path *path,
                             bool repeat, bool vigilant,
                             enum unit_orders orders,
                             struct unit_order *final_order)
{
  struct packet_unit_orders p;
  int i;
  struct tile *old_tile;

  fc_assert_ret(path != NULL);
  fc_assert_ret_msg(unit_tile(punit) == path->positions[0].tile,
                    "Unit %d has moved without goto cancelation.",
                    punit->id);

  if (path->length == 1 && final_order == NULL) {
    return; /* No path at all, no need to spam the server. */
  }

  memset(&p, 0, sizeof(p));
  p.unit_id = punit->id;
  p.src_tile = tile_index(unit_tile(punit));
  p.repeat = repeat;
  p.vigilant = vigilant;

  log_goto_packet("Orders for unit %d:", punit->id);

  /* We skip the start position. */
  p.length = path->length - 1;
  fc_assert(p.length < MAX_LEN_ROUTE);
  old_tile = path->positions[0].tile;

  log_goto_packet("  Repeat: %d. Vigilant: %d. Length: %d",
                  p.repeat, p.vigilant, p.length);

  /* If the path has n positions it takes n-1 steps. */
  for (i = 0; i < path->length - 1; i++) {
    struct tile *new_tile = path->positions[i + 1].tile;

    if (same_pos(new_tile, old_tile)) {
      p.orders[i] = ORDER_FULL_MP;
      p.dir[i] = DIR8_ORIGIN;
      p.activity[i] = ACTIVITY_LAST;
      p.sub_target[i] = -1;
      p.action[i] = ACTION_NONE;
      log_goto_packet("  packet[%d] = wait: %d,%d", i, TILE_XY(old_tile));
    } else {
      p.orders[i] = orders;
      p.dir[i] = get_direction_for_step(&(wld.map), old_tile, new_tile);
      p.activity[i] = ACTIVITY_LAST;
      p.sub_target[i] = -1;
      p.action[i] = ACTION_NONE;
      log_goto_packet("  packet[%d] = move %s: %d,%d => %d,%d",
                      i, dir_get_name(p.dir[i]),
                      TILE_XY(old_tile), TILE_XY(new_tile));
    }
    old_tile = new_tile;
  }

  if (p.orders[i - 1] == ORDER_MOVE
      && (is_non_allied_city_tile(old_tile, client_player()) != NULL
          || is_non_allied_unit_tile(old_tile, client_player()) != NULL)) {
    /* Won't be able to perform a regular move to the target tile... */
    if (!final_order) {
      /* ...and no final order exists. Choose what to do when the unit gets
       * there. */
      p.orders[i - 1] = ORDER_ACTION_MOVE;
    } else {
      /* ...and a final order exist. Can't assume an action move. Did the
       * caller hope that the situation would change before the unit got
       * there? */

      /* It's currently illegal to walk into tiles with non allied units or
       * cities. Some actions causes the actor to enter the target tile but
       * that is a part of the action it self, not a regular pre action
       * move. */
      log_verbose("unit or city blocks the path of your %s",
                  unit_rule_name(punit));
    }
  }

  if (final_order) {
    /* Append the final order after moving to the target tile. */
    p.orders[i] = final_order->order;
    p.dir[i] = final_order->dir;
    p.activity[i] = (final_order->order == ORDER_ACTIVITY)
      ? final_order->activity : ACTIVITY_LAST;
    p.sub_target[i] = final_order->sub_target;
    p.action[i] = final_order->action;
    p.length++;
  }

  p.dest_tile = tile_index(old_tile);

  send_packet_unit_orders(&client.conn, &p);
}

/************************************************************************//**
  Send an arbitrary goto path for the unit to the server.
****************************************************************************/
void send_goto_path(struct unit *punit, struct pf_path *path,
		    struct unit_order *final_order)
{
  send_path_orders(punit, path, FALSE, FALSE, ORDER_MOVE, final_order);
}

/************************************************************************//**
  Send orders for the unit to move it to the arbitrary tile.  Returns
  FALSE if no path is found.
****************************************************************************/
bool send_goto_tile(struct unit *punit, struct tile *ptile)
{
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_path *path;

  goto_fill_parameter_base(&parameter, punit);
  pfm = pf_map_new(&parameter);
  path = pf_map_path(pfm, ptile);
  pf_map_destroy(pfm);

  if (path) {
    send_goto_path(punit, path, NULL);
    pf_path_destroy(path);
    return TRUE;
  } else {
    return FALSE;
  }
}

/************************************************************************//**
  Send orders for the unit to move it to the arbitrary tile and attack
  everything it approaches. Returns FALSE if no path is found.
****************************************************************************/
bool send_attack_tile(struct unit *punit, struct tile *ptile)
{
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_path *path;

  goto_fill_parameter_base(&parameter, punit);
  parameter.move_rate = 0;
  parameter.is_pos_dangerous = NULL;
  parameter.get_moves_left_req = NULL;
  pfm = pf_map_new(&parameter);
  path = pf_map_path(pfm, ptile);
  pf_map_destroy(pfm);

  if (path) {
   send_path_orders(punit, path, false, false, ORDER_ACTION_MOVE, NULL);
   pf_path_destroy(path);
   return TRUE;
  }
  return FALSE;
}

/************************************************************************//**
  Send the current patrol route (i.e., the one generated via HOVER_STATE)
  to the server.
****************************************************************************/
void send_patrol_route(void)
{
  fc_assert_ret(goto_is_active());
  goto_map_unit_iterate(goto_maps, goto_map, punit) {
    int i;
    struct pf_path *path = NULL;
    struct part *last_part = &goto_map->parts[goto_map->num_parts - 1];

    if (NULL == last_part->path) {
      /* Cannot move there */
      continue;
    }

    for (i = 0; i < goto_map->num_parts; i++) {
      path = pf_path_concat(path, goto_map->parts[i].path);
    }
    path = pf_path_concat(path, goto_map->patrol.return_path);

    send_path_orders(punit, path, TRUE, TRUE, ORDER_MOVE, NULL);

    pf_path_destroy(path);
  } goto_map_unit_iterate_end;
}

/************************************************************************//**
  Fill orders to build recursive roads.
****************************************************************************/
static bool order_recursive_roads(struct tile *ptile, struct extra_type *pextra,
                                 struct packet_unit_orders *p, int rec)
{
  if (rec > MAX_EXTRA_TYPES) {
    return FALSE;
  }

  if (!is_extra_caused_by(pextra, EC_ROAD)) {
    return FALSE;
  }

  extra_deps_iterate(&(pextra->reqs), pdep) {
    if (!tile_has_extra(ptile, pdep)) {
      if (!order_recursive_roads(ptile, pdep, p, rec + 1)) {
        return FALSE;
      }
    }
  } extra_deps_iterate_end;

  p->orders[p->length] = ORDER_ACTIVITY;
  p->dir[p->length] = DIR8_ORIGIN;
  p->activity[p->length] = ACTIVITY_GEN_ROAD;
  p->sub_target[p->length] = extra_index(pextra);
  p->action[p->length] = ACTION_NONE;
  p->length++;

  return TRUE;
}

/************************************************************************//**
  Send the current connect route (i.e., the one generated via HOVER_STATE)
  to the server.
****************************************************************************/
void send_connect_route(enum unit_activity activity,
                        struct extra_type *tgt)
{
  fc_assert_ret(goto_is_active());
  goto_map_unit_iterate(goto_maps, goto_map, punit) {
    int i;
    struct packet_unit_orders p;
    struct tile *old_tile;
    struct pf_path *path = NULL;
    struct part *last_part = &goto_map->parts[goto_map->num_parts - 1];

    if (NULL == last_part->path) {
      /* Cannot move there */
      continue;
    }

    memset(&p, 0, sizeof(p));

    for (i = 0; i < goto_map->num_parts; i++) {
      path = pf_path_concat(path, goto_map->parts[i].path);
    }

    p.unit_id = punit->id;
    p.src_tile = tile_index(unit_tile(punit));
    p.repeat = FALSE;
    p.vigilant = FALSE; /* Should be TRUE? */

    p.length = 0;
    old_tile = path->positions[0].tile;

    for (i = 0; i < path->length; i++) {
      switch (activity) {
      case ACTIVITY_IRRIGATE:
	if (!tile_has_extra(old_tile, tgt)) {
	  /* Assume the unit can irrigate or we wouldn't be here. */
	  p.orders[p.length] = ORDER_ACTIVITY;
          p.dir[p.length] = DIR8_ORIGIN;
	  p.activity[p.length] = ACTIVITY_IRRIGATE;
          p.sub_target[p.length] = extra_index(tgt);
          p.action[p.length] = ACTION_NONE;
	  p.length++;
	}
	break;
      case ACTIVITY_GEN_ROAD:
        order_recursive_roads(old_tile, tgt, &p, 0);
        break;
      default:
        log_error("Invalid connect activity: %d.", activity);
        break;
      }

      if (i != path->length - 1) {
        struct tile *new_tile = path->positions[i + 1].tile;

        fc_assert(!same_pos(new_tile, old_tile));

        p.orders[p.length] = ORDER_MOVE;
        p.dir[p.length] = get_direction_for_step(&(wld.map),
                                                 old_tile, new_tile);
        p.activity[p.length] = ACTIVITY_LAST;
        p.sub_target[p.length] = -1;
        p.action[p.length] = ACTION_NONE;
        p.length++;

        old_tile = new_tile;
      }
    }

    p.dest_tile = tile_index(old_tile);

    send_packet_unit_orders(&client.conn, &p);
  } goto_map_unit_iterate_end;
}

/************************************************************************//**
  Returns TRUE if the order preferably should be performed from an
  adjacent tile.

  This function doesn't care if a direction is required or just possible.
  Use order_demands_direction() for that.
****************************************************************************/
static bool order_wants_direction(enum unit_orders order, action_id act_id,
                                  struct tile *tgt_tile)
{
  switch (order) {
  case ORDER_MOVE:
  case ORDER_ACTION_MOVE:
    /* Not only is it legal. It is mandatory. A move is always done in a
     * direction. */
    return TRUE;
  case ORDER_PERFORM_ACTION:
    if (!action_id_distance_accepted(act_id, 0)) {
      /* Always illegal to do to a target on the actor's own tile. */
      return TRUE;
    }

    if (!action_id_distance_accepted(act_id, 1)) {
      /* Always illegal to perform to a target on a neighbor tile. */
      return FALSE;
    }

    if (is_non_allied_city_tile(tgt_tile, client_player()) != NULL
        || is_non_allied_unit_tile(tgt_tile, client_player()) != NULL) {
      /* Won't be able to move to the target tile to perform the action on
       * top of it. */
      /* TODO: detect situations where it also would be illegal to perform
       * the action from the neighbor tile. */
      return TRUE;
    }

    return FALSE;
  default:
    return FALSE;
  }
}

/************************************************************************//**
  Returns TRUE if it is certain that the order must be performed from an
  adjacent tile.
****************************************************************************/
static bool order_demands_direction(enum unit_orders order, action_id act_id)
{
  switch (order) {
  case ORDER_MOVE:
  case ORDER_ACTION_MOVE:
    /* A move is always done in a direction. */
    return TRUE;
  case ORDER_PERFORM_ACTION:
    if (!action_id_distance_accepted(act_id, 0)) {
      /* Always illegal to do to a target on the actor's own tile. */
      return TRUE;
    }

    return FALSE;
  default:
    return FALSE;
  }
}

/************************************************************************//**
  Send the current goto route (i.e., the one generated via
  HOVER_STATE) to the server.  The route might involve more than one
  part if waypoints were used.  FIXME: danger paths are not supported.
****************************************************************************/
void send_goto_route(void)
{
  fc_assert_ret(goto_is_active());
  goto_map_unit_iterate(goto_maps, goto_map, punit) {
    int i;
    struct tile *tgt_tile;
    struct pf_path *path = NULL;
    struct part *last_part = &goto_map->parts[goto_map->num_parts - 1];

    if (NULL == last_part->path) {
      /* Cannot move there */
      continue;
    }

    for (i = 0; i < goto_map->num_parts; i++) {
      path = pf_path_concat(path, goto_map->parts[i].path);
    }

    clear_unit_orders(punit);

    tgt_tile = pf_path_last_position(path)->tile;

    /* Make the last move in a plain goto try to pop up the action
     * selection dialog rather than moving to the last tile if it contains
     * a domestic, allied or team mate city, unit or unit stack. This can,
     * in cases where the action requires movement left, save a turn. */
    /* TODO: Should this be a client option? */
    if (goto_last_order == ORDER_LAST
        && ((is_allied_city_tile(tgt_tile, client_player())
             || is_allied_unit_tile(tgt_tile, client_player()))
            && (can_utype_do_act_if_tgt_diplrel(unit_type_get(punit),
                                                ACTION_ANY,
                                                DRO_FOREIGN,
                                                FALSE)
                || can_utype_do_act_if_tgt_diplrel(unit_type_get(punit),
                                                   ACTION_ANY,
                                                   DS_ALLIANCE,
                                                   TRUE)
                || can_utype_do_act_if_tgt_diplrel(unit_type_get(punit),
                                                   ACTION_ANY,
                                                   DS_TEAM,
                                                   TRUE)))) {
      /* Try to pop up the action selection dialog before moving to the
       * target tile. */
      goto_last_order = ORDER_ACTION_MOVE;
    }

    if (goto_last_order == ORDER_LAST) {
      send_goto_path(punit, path, NULL);
    } else if (path->length > 1
               || !order_demands_direction(goto_last_order,
                                           goto_last_action)) {
      struct unit_order order;
      int last_order_dir;
      struct tile *on_tile;

      if (path->length > 1
          && ((on_tile = path->positions[path->length - 2].tile))
          && order_wants_direction(goto_last_order, goto_last_action,
                                   tgt_tile)
          && !same_pos(on_tile, tgt_tile)) {
        /* The last order prefers to handle the last direction it self.
         * There exists a tile before the target tile to do it from. */

        /* Give the last path direction to the final order. */
        last_order_dir = get_direction_for_step(&(wld.map), on_tile, tgt_tile);

        /* The last path direction is now spent. */
        pf_path_backtrack(path, on_tile);
      } else {
        fc_assert(!order_demands_direction(goto_last_order,
                                           goto_last_action));

        /* Target the tile the actor is standing on. */
        last_order_dir = DIR8_ORIGIN;
      }

      order.order = goto_last_order;
      order.dir = last_order_dir;
      order.activity = ACTIVITY_LAST;
      order.sub_target = goto_last_sub_tgt;
      order.action = goto_last_action;

      /* ORDER_ACTIVITY would require real activity */
      fc_assert(goto_last_order != ORDER_ACTIVITY);

      send_goto_path(punit, path, &order);
    }
    pf_path_destroy(path);
  } goto_map_unit_iterate_end;
}

/************************************************************************//**
  Find the path to the nearest (fastest to reach) allied city for the
  unit, or NULL if none is reachable.
****************************************************************************/
struct pf_path *path_to_nearest_allied_city(struct unit *punit)
{
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_path *path = NULL;

  if (is_allied_city_tile(unit_tile(punit), unit_owner(punit))) {
    /* We're already on a city - don't go anywhere. */
    return NULL;
  }

  goto_fill_parameter_base(&parameter, punit);
  pfm = pf_map_new(&parameter);

  pf_map_tiles_iterate(pfm, ptile, FALSE) {
    if (is_allied_city_tile(ptile, unit_owner(punit))) {
      path = pf_map_path(pfm, ptile);
      break;
    }
  } pf_map_tiles_iterate_end;

  pf_map_destroy(pfm);

  return path;
}

/************************************************************************//**
  Finds penultimate tile on path for given unit going to ptile
****************************************************************************/
struct tile *tile_before_end_path(struct unit *punit, struct tile *ptile)
{
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct tile *dtile;
  struct pf_path *path;

  goto_fill_parameter_base(&parameter, punit);
  parameter.move_rate = 0;
  parameter.is_pos_dangerous = NULL;
  parameter.get_moves_left_req = NULL;
  pfm = pf_map_new(&parameter);
  path = pf_map_path(pfm, ptile);
  if (path == NULL) {
    return NULL;
  }
  if (path->length < 2) {
    dtile = NULL;
  } else {
    dtile = path->positions[path->length - 2].tile;
  }
  pf_map_destroy(pfm);

  return dtile;
}
