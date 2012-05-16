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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

/* utility */
#include "log.h"
#include "mem.h"

/* common */
#include "map.h"
#include "movement.h"
#include "packets.h"
#include "pf_tools.h"
#include "unit.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "control.h"
#include "mapview_g.h"

#include "goto.h"
#include "mapctrl_common.h"

#define PATH_LOG_LEVEL          LOG_DEBUG
#define PACKET_LOG_LEVEL        LOG_DEBUG

/*
 * The whole path is separated by waypoints into parts.  Each part has its
 * own starting position and requires its own map.  When the unit is unable
 * to move, end_tile equals start_tile.
 */
struct part {
  int start_moves_left, start_fuel_left;
  struct tile *start_tile, *end_tile;
  int end_moves_left, end_fuel_left;
  int time;
  struct pf_path *path;
  struct pf_map *map;
};

struct goto_map {
  struct unit *focus;
  struct part *parts;
  int num_parts;
  int connect_initial;
  int connect_speed;
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
    struct unit *punit = pgoto->focus;

#define goto_map_unit_iterate_end					\
  } goto_map_list_iterate_end;

static struct goto_map_list *goto_maps = NULL;

static void reset_last_part(struct goto_map *goto_map);
static void remove_last_part(struct goto_map *goto_map);


/**************************************************************************
  Various stuff for the goto routes
**************************************************************************/
static struct tile *goto_destination = NULL;

/****************************************************************************
  Create a new goto map.
****************************************************************************/
static struct goto_map *goto_map_new(void)
{
  struct goto_map *goto_map = fc_malloc(sizeof(*goto_map));

  goto_map->focus = NULL;
  goto_map->parts = NULL;
  goto_map->num_parts = 0;
  goto_map->connect_initial = 0;
  goto_map->connect_speed = 0;

  return goto_map;
}

/****************************************************************************
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
  free(goto_map);
}

/********************************************************************** 
  Called only by handle_map_info() in client/packhand.c.
***********************************************************************/
void init_client_goto(void)
{
  free_client_goto();

  goto_maps = goto_map_list_new();
}

/********************************************************************** 
  Called above, and by control_done() in client/control.c.
***********************************************************************/
void free_client_goto(void)
{
  if (NULL != goto_maps) {
    goto_map_list_iterate(goto_maps, goto_map) {
      goto_map_free(goto_map);
    } goto_map_list_iterate_end;
    goto_map_list_free(goto_maps);
    goto_maps = NULL;
  }
}

/**********************************************************************
  Determines if a goto to the destination tile is allowed.
***********************************************************************/
bool is_valid_goto_destination(const struct tile *ptile) 
{
  return (NULL != goto_destination && ptile == goto_destination);
}

/********************************************************************** 
  Change the destination of the last part to the given location.
  If a path cannot be found, the destination is set to the start.
  Return TRUE when the new path is valid.
***********************************************************************/
static bool update_last_part(struct goto_map *goto_map,
			     struct tile *ptile)
{
  struct pf_path *new_path;
  struct part *p = &goto_map->parts[goto_map->num_parts - 1];
  struct tile *old_tile = p->start_tile;
  int i, start_index = 0;

  freelog(LOG_DEBUG, "update_last_part(%d,%d) old (%d,%d)-(%d,%d)",
          TILE_XY(ptile), TILE_XY(p->start_tile), TILE_XY(p->end_tile));
  new_path = pf_map_get_path(p->map, ptile);

  if (!new_path) {
    freelog(PATH_LOG_LEVEL, "  no path found");
    reset_last_part(goto_map);
    return FALSE;
  }

  freelog(PATH_LOG_LEVEL, "  path found:");
  pf_path_print(new_path, PATH_LOG_LEVEL);

  if (p->path) {
    /* We had a path drawn already.  Determine how much of it we can reuse
     * in drawing the new path. */
    for (i = 0; i < MIN(new_path->length, p->path->length) - 1; i++) {
      struct pf_position *a = &p->path->positions[i];
      struct pf_position *b = &new_path->positions[i];

      if (a->dir_to_next_pos != b->dir_to_next_pos
	  || !same_pos(a->tile, b->tile)) {
	break;
      }
    }
    start_index = i;

    /* Erase everything we cannot reuse */
    for (; i < p->path->length - 1; i++) {
      struct pf_position *a = &p->path->positions[i];

      if (is_valid_dir(a->dir_to_next_pos)) {
        mapdeco_remove_gotoline(a->tile, a->dir_to_next_pos);
      } else {
	assert(i < p->path->length - 1
	       && a->tile == p->path->positions[i + 1].tile);
      }
    }
    pf_path_destroy(p->path);
    p->path = NULL;

    old_tile = p->end_tile;
  }

  /* Draw the new path */
  for (i = start_index; i < new_path->length - 1; i++) {
    struct pf_position *a = &new_path->positions[i];

    if (is_valid_dir(a->dir_to_next_pos)) {
      mapdeco_add_gotoline(a->tile, a->dir_to_next_pos);
    } else {
      assert(i < new_path->length - 1
	     && a->tile == new_path->positions[i + 1].tile);
    }
  }
  p->path = new_path;
  p->end_tile = ptile;
  p->end_moves_left = pf_path_get_last_position(p->path)->moves_left;
  p->end_fuel_left = pf_path_get_last_position(p->path)->fuel_left;

  if (hover_state == HOVER_CONNECT) {
    int move_rate = goto_map->template.move_rate;
    int moves = pf_path_get_last_position(p->path)->total_MC;

    p->time = moves / move_rate;
    if (goto_map->connect_initial > 0) {
      p->time += goto_map->connect_initial;
    }
    freelog(PATH_LOG_LEVEL, "To (%d,%d) MC: %d, connect_initial: %d",
	    TILE_XY(ptile), moves, goto_map->connect_initial);
  } else {
    p->time = pf_path_get_last_position(p->path)->turn;
  }

  /* Refresh tiles so turn information is shown. */
  refresh_tile_mapcanvas(old_tile, FALSE, FALSE);
  refresh_tile_mapcanvas(ptile, FALSE, FALSE);
  return TRUE;
}

/********************************************************************** 
  Change the drawn path to a size of 0 steps by setting it to the
  start position.
***********************************************************************/
static void reset_last_part(struct goto_map *goto_map)
{
  struct part *p = &goto_map->parts[goto_map->num_parts - 1];

  if (!same_pos(p->start_tile, p->end_tile)) {
    /* Otherwise no need to update */
    update_last_part(goto_map, p->start_tile);
  }
}

/********************************************************************** 
  Add a part. Depending on the num of already existing parts the start
  of the new part is either the unit position (for the first part) or
  the destination of the last part (not the first part).
***********************************************************************/
static void add_part(struct goto_map *goto_map)
{
  struct part *p;
  struct pf_parameter parameter = goto_map->template;
  struct unit *punit = goto_map->focus;

  if (!punit) {
    return;
  }

  goto_map->num_parts++;
  goto_map->parts =
      fc_realloc(goto_map->parts,
                 goto_map->num_parts * sizeof(*goto_map->parts));
  p = &goto_map->parts[goto_map->num_parts - 1];

  if (goto_map->num_parts == 1) {
    /* first part */
    p->start_tile = punit->tile;
    p->start_moves_left = parameter.moves_left_initially;
    p->start_fuel_left = parameter.fuel_left_initially;
  } else {
    struct part *prev = &goto_map->parts[goto_map->num_parts - 2];

    p->start_tile = prev->end_tile;
    p->start_moves_left = prev->end_moves_left;
    p->start_fuel_left = prev->end_fuel_left;
    parameter.moves_left_initially = p->start_moves_left;
    parameter.fuel_left_initially = p->start_fuel_left;
  }
  p->path = NULL;
  p->end_tile = p->start_tile;
  p->time = 0;
  parameter.start_tile = p->start_tile;
  p->map = pf_map_new(&parameter);
}

/********************************************************************** 
  Remove the last part, erasing the corresponding path segment.
***********************************************************************/
static void remove_last_part(struct goto_map *goto_map)
{
  struct part *p = &goto_map->parts[goto_map->num_parts - 1];

  assert(goto_map->num_parts >= 1);

  reset_last_part(goto_map);
  if (p->path) {
    /* We do not always have a path */
    pf_path_destroy(p->path);
  }
  pf_map_destroy(p->map);
  goto_map->num_parts--;
}

/********************************************************************** 
  Inserts a waypoint at the end of the current goto line.
***********************************************************************/
bool goto_add_waypoint(void)
{
  struct tile *ptile_start = NULL;

  assert(goto_is_active());
  if (NULL == goto_destination) {
    /* Not a valid position. */
    return FALSE;
  }

  goto_map_list_iterate(goto_maps, goto_map) {
    struct part *first_part = &goto_map->parts[0];
    struct part *last_part = &goto_map->parts[goto_map->num_parts - 1];

    if (same_pos(last_part->start_tile, last_part->end_tile)) {
      /* The current part has zero length. */
      return FALSE;
    } else if (NULL == ptile_start) {
      ptile_start = first_part->start_tile;
    } else if (ptile_start != first_part->start_tile) {
      /* Scattered group (not all in same location). */
      return FALSE;
    }
  } goto_map_list_iterate_end;

  goto_map_list_iterate(goto_maps, goto_map) {
    add_part(goto_map);
  } goto_map_list_iterate_end;
  return TRUE;
}

/********************************************************************** 
  Returns whether there were any waypoint popped (we don't remove the
  initial position)
***********************************************************************/
bool goto_pop_waypoint(void)
{
  bool popped = FALSE;

  assert(goto_is_active());
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

/********************************************************************** 
  PF callback to get the path with the minimal number of steps (out of 
  all shortest paths).
***********************************************************************/
static int get_EC(const struct tile *ptile, enum known_type known,
                  const struct pf_parameter *param)
{
  return 1;
}

/********************************************************************** 
  PF callback to prohibit going into the unknown.  Also makes sure we 
  don't plan our route through enemy city/tile.
***********************************************************************/
static enum tile_behavior get_TB_aggr(const struct tile *ptile,
                                      enum known_type known,
                                      const struct pf_parameter *param)
{
  if (known == TILE_UNKNOWN) {
    if (!goto_into_unknown) {
      return TB_IGNORE;
    }
  } else if (is_non_allied_unit_tile(ptile, param->owner)
	     || is_non_allied_city_tile(ptile, param->owner)) {
    /* Can attack but can't count on going through */
    return TB_DONT_LEAVE;
  }
  return TB_NORMAL;
}

/********************************************************************** 
  PF callback for caravans. Caravans doesn't go into the unknown and
  don't attack enemy units but enter enemy cities.
***********************************************************************/
static enum tile_behavior get_TB_caravan(const struct tile *ptile,
                                         enum known_type known,
                                         const struct pf_parameter *param)
{
  if (known == TILE_UNKNOWN) {
    if (!goto_into_unknown) {
      return TB_IGNORE;
    }
  } else if (is_non_allied_city_tile(ptile, param->owner)) {
    /* F_TRADE_ROUTE units can travel to, but not through, enemy cities.
     * FIXME: F_HELP_WONDER units cannot.  */
    return TB_DONT_LEAVE;
  } else if (is_non_allied_unit_tile(ptile, param->owner)) {
    /* Note this must be below the city check. */
    return TB_IGNORE;
  }

  /* Includes empty, allied, or allied-city tiles. */
  return TB_NORMAL;
}

/****************************************************************************
  Return the number of MP needed to do the connect activity at this
  position.  A negative number means it's impossible.
****************************************************************************/
static int get_activity_time(const struct tile *ptile,
			     struct player *pplayer)
{
  struct terrain *pterrain = tile_terrain(ptile);
  int activity_mc = 0;

  assert(hover_state == HOVER_CONNECT);
  assert(terrain_control.may_road);
 
  switch (connect_activity) {
  case ACTIVITY_IRRIGATE:
    if (pterrain->irrigation_time == 0) {
      return -1;
    }
    if (tile_has_special(ptile, S_MINE)) {
      /* Don't overwrite mines. */
      return -1;
    }

    if (tile_has_special(ptile, S_IRRIGATION)) {
      break;
    }

    activity_mc = pterrain->irrigation_time;
    break;
  case ACTIVITY_RAILROAD:
  case ACTIVITY_ROAD:
    if (!tile_has_special(ptile, S_ROAD)) {
      if (pterrain->road_time == 0
	  || (tile_has_special(ptile, S_RIVER)
	      && !player_knows_techs_with_flag(pplayer, TF_BRIDGE))) {
	/* 0 means road is impossible here (??) */
	return -1;
      }
      activity_mc += pterrain->road_time;
    }
    if (connect_activity == ACTIVITY_ROAD 
        || tile_has_special(ptile, S_RAILROAD)) {
      break;
    }
    activity_mc += pterrain->rail_time;
    /* No break */
    break;
  default:
    die("Invalid connect activity.");
  }

  return activity_mc;
}

/****************************************************************************
  When building a road or a railroad, we don't want to go next to 
  nonallied cities
****************************************************************************/
static bool is_non_allied_city_adjacent(struct player *pplayer,
					const struct tile *ptile)
{
  adjc_iterate(ptile, tile1) {
    if (is_non_allied_city_tile(tile1, pplayer)) {
      return TRUE;
    }
  } adjc_iterate_end;
  
  return FALSE;
}

/****************************************************************************
  PF jumbo callback for the cost of a connect by road. 
  In road-connect mode we are concerned with 
  (1) the number of steps of the resulting path
  (2) (the tie-breaker) time to build the path (travel plus activity time).
  In rail-connect the priorities are reversed.

  param->data should contain the result of
  get_activity_rate(punit) / ACTIVITY_FACTOR.
****************************************************************************/
static int get_connect_road(const struct tile *src_tile, enum direction8 dir,
                            const struct tile *dest_tile,
                            int src_cost, int src_extra,
                            int *dest_cost, int *dest_extra,
                            const struct pf_parameter *param)
{
  int activity_time, move_cost, moves_left;
  int total_cost, total_extra;

  if (tile_get_known(dest_tile, param->owner) == TILE_UNKNOWN) {
    return -1;
  }

  activity_time = get_activity_time(dest_tile, param->owner);  
  if (activity_time < 0) {
    return -1;
  }

  move_cost = param->get_MC(src_tile, dir, dest_tile, param);
  if (move_cost == PF_IMPOSSIBLE_MC) {
    return -1;
  }

  if (is_non_allied_city_adjacent(param->owner, dest_tile)) {
    /* We don't want to build roads to enemies plus get ZoC problems */
    return -1;
  }

  /* Ok, the move is possible.  What are the costs? */

  /* Extra cost here is the final length of the road */
  total_extra = src_extra + 1;

  /* Special cases: get_MC function doesn't know that we would have built
   * a road (railroad) on src tile by that time */
  if (tile_has_special(dest_tile, S_ROAD)) {
    move_cost = MOVE_COST_ROAD;
  }
  if (connect_activity == ACTIVITY_RAILROAD
      && tile_has_special(dest_tile, S_RAILROAD)) {
    move_cost = MOVE_COST_RAIL;
  }

  move_cost = MIN(move_cost, param->move_rate);
  total_cost = src_cost;
  moves_left = param->move_rate - (src_cost % param->move_rate);
  if (moves_left < move_cost) {
    /* Emulating TM_WORST_TIME */
    total_cost += moves_left;
  }
  total_cost += move_cost;

  /* Now need to include the activity cost.  If we have moves left, they
   * will count as a full turn towards the activity time */
  moves_left = param->move_rate - (total_cost % param->move_rate);
  if (activity_time > 0) {
    int speed = *(int *)param->data;
    
    activity_time /= speed;
    activity_time--;
    total_cost += moves_left;
  }
  total_cost += activity_time * param->move_rate;

  /* Now we determine if we have found a better path.  When building a road,
   * we care most about the length of the result.  When building a rail, we 
   * care most about the constructions time (assuming MOVE_COST_RAIL == 0) */

  /* *dest_cost==-1 means we haven't reached dest until now */
  if (*dest_cost != -1) {
    if (connect_activity == ACTIVITY_ROAD) {
      if (total_extra > *dest_extra 
	  || (total_extra == *dest_extra && total_cost >= *dest_cost)) {
	/* No, this path is worse than what we already have */
	return -1;
      }
    } else {
      /* assert(connect_activity == ACTIVITY_RAILROAD) */
      if (total_cost > *dest_cost 
	  || (total_cost == *dest_cost && total_cost >= *dest_cost)) {
	return -1;
      }
    }
  }

  /* Ok, we found a better path! */  
  *dest_cost = total_cost;
  *dest_extra = total_extra;
  
  return (connect_activity == ACTIVITY_ROAD ? 
	  total_extra * PF_TURN_FACTOR + total_cost : 
	  total_cost * PF_TURN_FACTOR + total_extra);
}

/****************************************************************************
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

  move_cost = param->get_MC(src_tile, dir, dest_tile, param);
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
    /* Emulating TM_WORST_TIME */
    total_cost += moves_left;
  }
  total_cost += move_cost;

  /* Now need to include the activity cost.  If we have moves left, they
   * will count as a full turn towards the activity time */
  moves_left = param->move_rate - (total_cost % param->move_rate);
  if (activity_time > 0) {
    int speed = *(int *)param->data;
    
    activity_time /= speed;
    activity_time--;
    total_cost += moves_left;
  }
  total_cost += activity_time * param->move_rate;

  /* *dest_cost==-1 means we haven't reached dest until now */
  if (*dest_cost != -1 && total_cost > *dest_cost) {
      return -1;
  }

  /* Ok, we found a better path! */  
  *dest_cost = total_cost;
  *dest_extra = 0;
  
  return total_cost;
}

/********************************************************************** 
  PF callback to prohibit going into the unknown (conditionally).  Also
  makes sure we don't plan to attack anyone.
***********************************************************************/
static enum tile_behavior
no_fights_or_unknown_goto(const struct tile *ptile,
                          enum known_type known,
                          const struct pf_parameter *p)
{
  if (known == TILE_UNKNOWN && goto_into_unknown) {
    /* Special case allowing goto into the unknown. */
    return TB_NORMAL;
  }

  return no_fights_or_unknown(ptile, known, p);
}

/********************************************************************** 
  Fill the PF parameter with the correct client-goto values.
***********************************************************************/
static void fill_client_goto_parameter(struct unit *punit,
				       struct pf_parameter *parameter,
				       int *connect_initial,
				       int *connect_speed)
{
  pft_fill_unit_parameter(parameter, punit);
  assert(parameter->get_EC == NULL);
  parameter->get_EC = get_EC;
  assert(parameter->get_TB == NULL);
  assert(parameter->get_MC != NULL);

  switch (hover_state) {
  case HOVER_CONNECT:
    if (connect_activity == ACTIVITY_IRRIGATE) {
      parameter->get_costs = get_connect_irrig;
    } else {
      parameter->get_costs = get_connect_road;
    }
    parameter->get_moves_left_req = NULL;

    *connect_speed = get_activity_rate(punit) / ACTIVITY_FACTOR;
    parameter->data = connect_speed;

    /* Take into account the activity time at the origin */
    *connect_initial = get_activity_time(punit->tile, unit_owner(punit))
                     / *connect_speed;
    if (*connect_initial > 0) {
      parameter->moves_left_initially = 0;
      if (punit->moves_left == 0) {
	*connect_initial += 1;
      }
    } else {
      /* otherwise moves_left_initially = punit->moves_left (default) */
      *connect_initial = 0;
    }
    break;
  case HOVER_NUKE:
    parameter->get_moves_left_req = NULL; /* nuclear safety? pwah! */
    /* FALLTHRU */
  default:
    *connect_initial = 0;
    break;
  };

  if (is_attack_unit(punit) || is_diplomat_unit(punit)) {
    parameter->get_TB = get_TB_aggr;
  } else if (unit_has_type_flag(punit, F_TRADE_ROUTE)
	     || unit_has_type_flag(punit, F_HELP_WONDER)) {
    parameter->get_TB = get_TB_caravan;
  } else {
    parameter->get_TB = no_fights_or_unknown_goto;
  }

  /* Note that in connect mode the "time" does not correspond to any actual
   * move rate.
   *
   * FIXME: for units traveling across dangerous terrains with partial MP
   * (which is very rare) using TM_BEST_TIME could cause them to die. */
  parameter->turn_mode = TM_BEST_TIME;
  parameter->start_tile = punit->tile;

  /* Omniscience is always FALSE in the client */
  parameter->omniscience = FALSE;
}

/********************************************************************** 
  Enter the goto state: activate, prepare PF-template and add the 
  initial part.
***********************************************************************/
void enter_goto_state(struct unit_list *punits)
{
  assert(!goto_is_active());

  /* Can't have selection rectangle and goto going on at the same time. */
  cancel_selection_rectangle();

  unit_list_iterate(punits, punit) {
    struct goto_map *goto_map = goto_map_new();

    goto_map->focus = punit;

    fill_client_goto_parameter(punit, &goto_map->template,
                               &goto_map->connect_initial,
                               &goto_map->connect_speed);

    add_part(goto_map);

    goto_map_list_append(goto_maps, goto_map);
  } unit_list_iterate_end;
}

/********************************************************************** 
  Tidy up and deactivate goto state.
***********************************************************************/
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
}

/********************************************************************** 
  Called from control_unit_killed() in client/control.c
***********************************************************************/
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
      goto_map_list_unlink(goto_maps, goto_map);
      /* stop now, links are gone! */
      break;
    }
  } goto_map_unit_iterate_end;
}

/********************************************************************** 
  Is goto state active?
***********************************************************************/
bool goto_is_active(void)
{
  return (NULL != goto_maps && 0 != goto_map_list_size(goto_maps));
}

/**************************************************************************
  Return the path length (in turns).
  WARNING: not useful for determining paths of scattered groups.
***************************************************************************/
bool goto_get_turns(int *min, int *max)
{
  if (min) {
    *min = FC_INFINITY;
  }
  if (max) {
    *max = -1;
  }
  if (!goto_is_active()) {
    return FALSE;
  }
  if (NULL == goto_destination) {
    /* Not a valid position. */
    return FALSE;
  }

  goto_map_list_iterate(goto_maps, goto_map) {
    int i, time = 0;

    for (i = 0; i < goto_map->num_parts; i++) {
      time += goto_map->parts[i].time;
    }

    if (min) {
      *min = MIN(*min, time);
    }
    if (max) {
      *max = MAX(*max, time);
    }
  } goto_map_list_iterate_end;
  return TRUE;
}

/********************************************************************** 
  Puts a line to dest_tile on the map according to the current
  goto_map.
  If there is no route to the dest then don't draw anything.
***********************************************************************/
bool is_valid_goto_draw_line(struct tile *dest_tile)
{
  assert(goto_is_active());
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

/****************************************************************************
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
  freelog(PACKET_LOG_LEVEL, "Clearing orders for unit %d.", punit->id);
  p.unit_id = punit->id;
  p.src_x = punit->tile->x;
  p.src_y = punit->tile->y;
  p.repeat = p.vigilant = FALSE;
  p.length = 0;
  p.dest_x = punit->tile->x;
  p.dest_y = punit->tile->y;
  send_packet_unit_orders(&client.conn, &p);
}

/**************************************************************************
  Send a path as a goto or patrol route to the server.
**************************************************************************/
static void send_path_orders(struct unit *punit, struct pf_path *path,
			     bool repeat, bool vigilant,
			     struct unit_order *final_order)
{
  struct packet_unit_orders p;
  int i;
  struct tile *old_tile;

  p.unit_id = punit->id;
  p.src_x = punit->tile->x;
  p.src_y = punit->tile->y;
  p.repeat = repeat;
  p.vigilant = vigilant;

  freelog(PACKET_LOG_LEVEL, "Orders for unit %d:", punit->id);

  /* We skip the start position. */
  p.length = path->length - 1;
  assert(p.length < MAX_LEN_ROUTE);
  old_tile = path->positions[0].tile;

  freelog(PACKET_LOG_LEVEL, "  Repeat: %d.  Vigilant: %d.  Length: %d",
	  p.repeat, p.vigilant, p.length);

  /* If the path has n positions it takes n-1 steps. */
  for (i = 0; i < path->length - 1; i++) {
    struct tile *new_tile = path->positions[i + 1].tile;

    if (same_pos(new_tile, old_tile)) {
      p.orders[i] = ORDER_FULL_MP;
      p.dir[i] = -1;
      freelog(PACKET_LOG_LEVEL, "  packet[%d] = wait: %d,%d",
	      i, TILE_XY(old_tile));
    } else {
      p.orders[i] = ORDER_MOVE;
      p.dir[i] = get_direction_for_step(old_tile, new_tile);
      p.activity[i] = ACTIVITY_LAST;
      freelog(PACKET_LOG_LEVEL, "  packet[%d] = move %s: %d,%d => %d,%d",
 	      i, dir_get_name(p.dir[i]),
	      TILE_XY(old_tile), TILE_XY(new_tile));
      p.activity[i] = ACTIVITY_LAST;
    }
    old_tile = new_tile;
  }

  if (final_order) {
    p.orders[i] = final_order->order;
    p.dir[i] = (final_order->order == ORDER_MOVE) ? final_order->dir : -1;
    p.activity[i] = (final_order->order == ORDER_ACTIVITY)
      ? final_order->activity : ACTIVITY_LAST;
    p.length++;
  }

  p.dest_x = old_tile->x;
  p.dest_y = old_tile->y;

  send_packet_unit_orders(&client.conn, &p);
}

/**************************************************************************
  Send an arbitrary goto path for the unit to the server.
**************************************************************************/
void send_goto_path(struct unit *punit, struct pf_path *path,
		    struct unit_order *final_order)
{
  send_path_orders(punit, path, FALSE, FALSE, final_order);
}

/****************************************************************************
  Send orders for the unit to move it to the arbitrary tile.  Returns
  FALSE if no path is found.
****************************************************************************/
bool send_goto_tile(struct unit *punit, struct tile *ptile)
{
  int dummy1, dummy2;
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_path *path;

  fill_client_goto_parameter(punit, &parameter, &dummy1, &dummy2);
  pfm = pf_map_new(&parameter);
  path = pf_map_get_path(pfm, ptile);
  pf_map_destroy(pfm);

  if (path) {
    send_goto_path(punit, path, NULL);
    pf_path_destroy(path);
    return TRUE;
  } else {
    return FALSE;
  }
}

/**************************************************************************
  Send the current patrol route (i.e., the one generated via HOVER_STATE)
  to the server.
**************************************************************************/
void send_patrol_route(void)
{
  assert(goto_is_active());
  goto_map_unit_iterate(goto_maps, goto_map, punit) {
    int i;
    struct pf_map *pfm;
    struct pf_path *return_path;
    struct pf_path *path = NULL;
    struct pf_parameter parameter = goto_map->template;
    struct part *last_part = &goto_map->parts[goto_map->num_parts - 1];

    if (last_part->end_tile == last_part->start_tile) {
      /* Cannot move there */
      continue;
    }

    parameter.start_tile = last_part->end_tile;
    parameter.moves_left_initially = last_part->end_moves_left;
    parameter.fuel_left_initially = last_part->end_fuel_left;
    pfm = pf_map_new(&parameter);
    return_path = pf_map_get_path(pfm, goto_map->parts[0].start_tile);
    if (!return_path) {
      /* Cannot make a path */
      pf_map_destroy(pfm);
      continue;
    }

    for (i = 0; i < goto_map->num_parts; i++) {
      path = pft_concat(path, goto_map->parts[i].path);
    }
    path = pft_concat(path, return_path);

    pf_map_destroy(pfm);
    pf_path_destroy(return_path);

    send_path_orders(punit, path, TRUE, TRUE, NULL);

    pf_path_destroy(path);
  } goto_map_unit_iterate_end;
}

/**************************************************************************
  Send the current connect route (i.e., the one generated via HOVER_STATE)
  to the server.
**************************************************************************/
void send_connect_route(enum unit_activity activity)
{
  assert(goto_is_active());
  goto_map_unit_iterate(goto_maps, goto_map, punit) {
    int i;
    struct packet_unit_orders p;
    struct tile *old_tile;
    struct pf_path *path = NULL;
    struct part *last_part = &goto_map->parts[goto_map->num_parts - 1];

    if (last_part->end_tile == last_part->start_tile) {
      /* Cannot move there */
      continue;
    }

    memset(&p, 0, sizeof(p));

    for (i = 0; i < goto_map->num_parts; i++) {
      path = pft_concat(path, goto_map->parts[i].path);
    }

    p.unit_id = punit->id;
    p.src_x = punit->tile->x;
    p.src_y = punit->tile->y;
    p.repeat = FALSE;
    p.vigilant = FALSE; /* Should be TRUE? */

    p.length = 0;
    old_tile = path->positions[0].tile;

    for (i = 0; i < path->length; i++) {
      switch (activity) {
      case ACTIVITY_IRRIGATE:
	if (!tile_has_special(old_tile, S_IRRIGATION)) {
	  /* Assume the unit can irrigate or we wouldn't be here. */
	  p.orders[p.length] = ORDER_ACTIVITY;
	  p.activity[p.length] = ACTIVITY_IRRIGATE;
	  p.length++;
	}
	break;
      case ACTIVITY_ROAD:
      case ACTIVITY_RAILROAD:
	if (!tile_has_special(old_tile, S_ROAD)) {
	  /* Assume the unit can build the road or we wouldn't be here. */
	  p.orders[p.length] = ORDER_ACTIVITY;
	  p.activity[p.length] = ACTIVITY_ROAD;
	  p.length++;
	}
	if (activity == ACTIVITY_RAILROAD) {
	  if (!tile_has_special(old_tile, S_RAILROAD)) {
	    /* Assume the unit can build the rail or we wouldn't be here. */
	    p.orders[p.length] = ORDER_ACTIVITY;
	    p.activity[p.length] = ACTIVITY_RAILROAD;
	    p.length++;
	  }
	}
	break;
      default:
	die("Invalid connect activity.");
	break;
      }

      if (i != path->length - 1) {
	struct tile *new_tile = path->positions[i + 1].tile;

	assert(!same_pos(new_tile, old_tile));

	p.orders[p.length] = ORDER_MOVE;
	p.dir[p.length] = get_direction_for_step(old_tile, new_tile);
	p.length++;

	old_tile = new_tile;
      }
    }

    p.dest_x = old_tile->x;
    p.dest_y = old_tile->y;

    send_packet_unit_orders(&client.conn, &p);
  } goto_map_unit_iterate_end;
}

/**************************************************************************
  Send the current goto route (i.e., the one generated via
  HOVER_STATE) to the server.  The route might involve more than one
  part if waypoints were used.  FIXME: danger paths are not supported.
**************************************************************************/
void send_goto_route(void)
{
  assert(goto_is_active());
  goto_map_unit_iterate(goto_maps, goto_map, punit) {
    int i;
    struct pf_path *path = NULL;
    struct part *last_part = &goto_map->parts[goto_map->num_parts - 1];

    if (last_part->end_tile == last_part->start_tile) {
      /* Cannot move there */
      continue;
    }

    for (i = 0; i < goto_map->num_parts; i++) {
      path = pft_concat(path, goto_map->parts[i].path);
    }

    clear_unit_orders(punit);
    if (goto_last_order == ORDER_LAST) {
      send_goto_path(punit, path, NULL);
    } else {
      struct unit_order order;

      order.order = goto_last_order;
      order.dir = -1;
      order.activity = ACTIVITY_LAST;

      /* ORDER_MOVE would require real direction,
       * ORDER_ACTIVITY would require real activity */
      assert(goto_last_order != ORDER_MOVE
	     && goto_last_order != ORDER_ACTIVITY);

      send_goto_path(punit, path, &order);
    }
    pf_path_destroy(path);
  } goto_map_unit_iterate_end;
}

/**************************************************************************
  Find the path to the nearest (fastest to reach) allied city for the
  unit, or NULL if none is reachable.
***************************************************************************/
struct pf_path *path_to_nearest_allied_city(struct unit *punit)
{
  int dummy1, dummy2;
  struct pf_parameter parameter;
  struct pf_map *pfm;
  struct pf_path *path = NULL;

  if (is_allied_city_tile(punit->tile, unit_owner(punit))) {
    /* We're already on a city - don't go anywhere. */
    return NULL;
  }

  fill_client_goto_parameter(punit, &parameter, &dummy1, &dummy2);
  pfm = pf_map_new(&parameter);

  pf_map_iterate_tiles(pfm, ptile, FALSE) {
    if (is_allied_city_tile(ptile, unit_owner(punit))) {
      path = pf_map_get_path(pfm, ptile);
      break;
    }
  } pf_map_iterate_tiles_end;

  pf_map_destroy(pfm);

  return path;
}
