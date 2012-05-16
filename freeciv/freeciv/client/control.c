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

/* utility */
#include "fcintl.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "timing.h"

/* common */
#include "game.h"
#include "map.h"
#include "movement.h"

/* client */
#include "audio.h"
#include "chatline_g.h"
#include "citydlg_g.h"
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "combat.h"
#include "dialogs_g.h"
#include "editor.h"
#include "goto.h"
#include "gui_main_g.h"
#include "mapctrl_g.h"
#include "mapview_g.h"
#include "menu_g.h"
#include "options.h"
#include "overview_common.h"
#include "tilespec.h"
#include "unitlist.h"

#include "control.h"

/* gui-dep code may adjust depending on tile size etc: */
int num_units_below = MAX_NUM_UNITS_BELOW;

/* current_focus points to the current unit(s) in focus */
static struct unit_list *current_focus;

/* The previously focused unit(s).  Focus can generally be recalled
 * with keypad 5 (or the equivalent). 
 * FIXME: this is not reset when the client disconnects. */
static struct unit_list *previous_focus;

/* The priority unit(s) for advance_unit_focus(). */
static struct unit_list *urgent_focus_queue;

/* These should be set via set_hover_state() */
enum cursor_hover_state hover_state = HOVER_NONE;
enum unit_activity connect_activity;
enum unit_orders goto_last_order; /* Last order for goto */

static struct tile *hover_tile = NULL;
static struct unit_list *battlegroups[MAX_NUM_BATTLEGROUPS];

/* units involved in current combat */
static struct unit *punit_attacking = NULL;
static struct unit *punit_defending = NULL;

/* unit arrival lists */
static struct genlist *caravan_arrival_queue;
static struct genlist *diplomat_arrival_queue;

/*
 * This variable is TRUE iff a NON-AI controlled unit was focused this
 * turn.
 */
bool non_ai_unit_focus;

/*************************************************************************/

static struct unit *quickselect(struct tile *ptile,
                                enum quickselect_type qtype);

/**************************************************************************
  Called only by client_game_init() in client/civclient.c
**************************************************************************/
void control_init(void)
{
  int i;

  caravan_arrival_queue = genlist_new();
  diplomat_arrival_queue = genlist_new();

  current_focus = unit_list_new();
  previous_focus = unit_list_new();
  urgent_focus_queue = unit_list_new();

  for (i = 0; i < MAX_NUM_BATTLEGROUPS; i++) {
    battlegroups[i] = unit_list_new();
  }
  hover_tile = NULL;
}

/**************************************************************************
  Called only by client_game_free() in client/civclient.c
**************************************************************************/
void control_done(void)
{
  int i;

  genlist_free(caravan_arrival_queue);
  genlist_free(diplomat_arrival_queue);

  unit_list_free(current_focus);
  unit_list_free(previous_focus);
  unit_list_free(urgent_focus_queue);

  for (i = 0; i < MAX_NUM_BATTLEGROUPS; i++) {
    unit_list_free(battlegroups[i]);
  }
  free_client_goto();
}

/**************************************************************************
...
**************************************************************************/
struct unit_list *get_units_in_focus(void)
{
  return current_focus;
}

/****************************************************************************
  Return the number of units currently in focus (0 or more).
****************************************************************************/
int get_num_units_in_focus(void)
{
  return unit_list_size(current_focus);
}

/**************************************************************************
  Store the focus unit(s).  This is used so that we can return to the
  previously focused unit with an appropriate keypress.
**************************************************************************/
static void store_previous_focus(void)
{
  if (get_num_units_in_focus() > 0) {
    unit_list_clear(previous_focus);
    unit_list_iterate(get_units_in_focus(), punit) {
      unit_list_append(previous_focus, punit);
    } unit_list_iterate_end;
  }
}

/****************************************************************************
  Store a priority focus unit.
****************************************************************************/
void urgent_unit_focus(struct unit *punit)
{
  unit_list_append(urgent_focus_queue, punit);
}

/**************************************************************************
  Called when a unit is killed; this removes it from the control lists.
**************************************************************************/
void control_unit_killed(struct unit *punit)
{
  int i;

  goto_unit_killed(punit);

  unit_list_unlink(get_units_in_focus(), punit);
  if (get_num_units_in_focus() < 1) {
    set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST, ORDER_LAST);
  }
  update_unit_info_label(get_units_in_focus());

  unit_list_unlink(previous_focus, punit);
  unit_list_unlink(urgent_focus_queue, punit);

  for (i = 0; i < MAX_NUM_BATTLEGROUPS; i++) {
    unit_list_unlink(battlegroups[i], punit);
  }
}

/**************************************************************************
  Change the battlegroup for this unit.
**************************************************************************/
void unit_change_battlegroup(struct unit *punit, int battlegroup)
{
  if (battlegroup < 0 || battlegroup >= MAX_NUM_BATTLEGROUPS) {
    battlegroup = BATTLEGROUP_NONE;
  }

  if (punit->battlegroup != battlegroup) {
    if (battlegroup != BATTLEGROUP_NONE) {
      unit_list_append(battlegroups[battlegroup], punit);
    }
    if (punit->battlegroup != BATTLEGROUP_NONE) {
      unit_list_unlink(battlegroups[punit->battlegroup], punit);
    }
    punit->battlegroup = battlegroup;
  }
}

/**************************************************************************
  Call this on new units to enter them in the battlegroup lists.
**************************************************************************/
void unit_register_battlegroup(struct unit *punit)
{
  if (punit->battlegroup < 0 || punit->battlegroup >= MAX_NUM_BATTLEGROUPS) {
    punit->battlegroup = BATTLEGROUP_NONE;
  } else {
    unit_list_append(battlegroups[punit->battlegroup], punit);
  }
}

/**************************************************************************
  Enter the given hover state.

    activity => The connect activity (ACTIVITY_ROAD, etc.)
    order => The last order (ORDER_BUILD_CITY, ORDER_LAST, etc.)
**************************************************************************/
void set_hover_state(struct unit_list *punits, enum cursor_hover_state state,
		     enum unit_activity activity,
		     enum unit_orders order)
{
  assert((punits && unit_list_size(punits) > 0) || state == HOVER_NONE);
  assert(state == HOVER_CONNECT || activity == ACTIVITY_LAST);
  assert(state == HOVER_GOTO || order == ORDER_LAST);
  hover_state = state;
  connect_activity = activity;
  goto_last_order = order;
  exit_goto_state();
}

/****************************************************************************
  Return TRUE iff this unit is in focus.
****************************************************************************/
bool unit_is_in_focus(const struct unit *punit)
{
  return unit_list_search(get_units_in_focus(), punit);
}

/****************************************************************************
  Return TRUE iff a unit on this tile is in focus.
****************************************************************************/
struct unit *get_focus_unit_on_tile(const struct tile *ptile)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (punit->tile == ptile) {
      return punit;
    }
  } unit_list_iterate_end;

  return NULL;
}

/****************************************************************************
  Return head of focus units list.
****************************************************************************/
struct unit *head_of_units_in_focus(void)
{
  return unit_list_get(current_focus, 0);
}

/****************************************************************************
  Finds a single focus unit that we can center on.  May return NULL.
****************************************************************************/
static struct tile *find_a_focus_unit_tile_to_center_on(void)
{
  struct unit *punit;

  if (NULL != (punit = get_focus_unit_on_tile(get_center_tile_mapcanvas()))) {
    return punit->tile;
  } else if (get_num_units_in_focus() > 0) {
    return head_of_units_in_focus()->tile;
  } else {
    return NULL;
  }
}

/**************************************************************************
Center on the focus unit, if off-screen and auto_center_on_unit is true.
**************************************************************************/
void auto_center_on_focus_unit(void)
{
  struct tile *ptile = find_a_focus_unit_tile_to_center_on();

  if (ptile && auto_center_on_unit &&
      !tile_visible_and_not_on_border_mapcanvas(ptile)) {
    center_tile_mapcanvas(ptile);
  }
}

/**************************************************************************
  ...
**************************************************************************/
static void current_focus_append(struct unit *punit)
{
  unit_list_append(current_focus, punit);

  punit->focus_status = FOCUS_AVAIL;
  refresh_unit_mapcanvas(punit, punit->tile, TRUE, FALSE);

  if (unit_selection_clears_orders) {
    clear_unit_orders(punit);
  }
}

/**************************************************************************
  Clear all orders for the given unit.
**************************************************************************/
void clear_unit_orders(struct unit *punit)
{
  if (!punit) {
    return;
  }

  if (unit_has_orders(punit)) {
    /* Clear the focus unit's orders. */
    request_orders_cleared(punit);
  }

  if (punit->activity != ACTIVITY_IDLE || punit->ai.control)  {
    punit->ai.control = FALSE;
    refresh_unit_city_dialogs(punit);
    request_new_unit_activity(punit, ACTIVITY_IDLE);
  }
}

/**************************************************************************
  Sets the focus unit directly.  The unit given will be given the
  focus; if NULL the focus will be cleared.

  This function is called for several reasons.  Sometimes a fast-focus
  happens immediately as a result of a client action.  Other times it
  happens because of a server-sent packet that wakes up a unit.
**************************************************************************/
void set_unit_focus(struct unit *punit)
{
  bool focus_changed = FALSE;

  if (NULL != punit
      && NULL != client.conn.playing
      && unit_owner(punit) != client.conn.playing) {
    /* Callers should make sure this never happens. */
    return;
  }

  /* FIXME: this won't work quite right; for instance activating a
   * battlegroup twice in a row will store the focus erronously.  The only
   * solution would be a set_units_focus() */
  if (!(get_num_units_in_focus() == 1
	&& punit == head_of_units_in_focus())) {
    store_previous_focus();
    focus_changed = TRUE;
  }

  /* Redraw the old focus unit (to fix blinking or remove the selection
   * circle). */
  unit_list_iterate(current_focus, punit_old) {
    refresh_unit_mapcanvas(punit_old, punit_old->tile, TRUE, FALSE);
  } unit_list_iterate_end;
  unit_list_clear(current_focus);

  if (!can_client_change_view()) {
    /* This function can be called to set the focus to NULL when
     * disconnecting.  In this case we don't want any other actions! */
    assert(punit == NULL);
    return;
  }

  if (NULL != punit) {
    current_focus_append(punit);
    auto_center_on_focus_unit();
  }

  if (focus_changed) {
    set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST, ORDER_LAST);
  }

  update_unit_info_label(current_focus);
  update_menus();
}

/**************************************************************************
  Adds this unit to the list of units in focus.
**************************************************************************/
void add_unit_focus(struct unit *punit)
{
  if (NULL != punit
      && NULL != client.conn.playing
      && unit_owner(punit) != client.conn.playing) {
    /* Callers should make sure this never happens. */
    return;
  }

  if (NULL == punit || !can_client_change_view()) {
    return;
  }

  if (unit_is_in_focus(punit)) {
    return;
  }

  if (hover_state != HOVER_NONE) {
    /* Can't continue with current goto if set of focus units
     * change. Cancel it. */
    set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST, ORDER_LAST);
  }

  current_focus_append(punit);
  update_unit_info_label(current_focus);
  update_menus();
}

/**************************************************************************
 The only difference is that here we draw the "cross".
**************************************************************************/
void set_unit_focus_and_select(struct unit *punit)
{
  set_unit_focus(punit);
  if (punit) {
    put_cross_overlay_tile(punit->tile);
  }
}

/**************************************************************************
 Find the nearest available unit for focus, excluding any current unit
 in focus unless "accept_current" is TRUE.  If the current focus unit
 is the only possible unit, or if there is no possible unit, returns NULL.
**************************************************************************/
static struct unit *find_best_focus_candidate(bool accept_current)
{
  struct tile *ptile = get_center_tile_mapcanvas();

  if (!get_focus_unit_on_tile(ptile)) {
    struct unit *pfirst = head_of_units_in_focus();

    if (pfirst) {
      ptile = pfirst->tile;
    }
  }

  iterate_outward(ptile, FC_INFINITY, ptile2) {
    unit_list_iterate(ptile2->units, punit) {
      if ((!unit_is_in_focus(punit) || accept_current)
	  && unit_owner(punit) == client.conn.playing
	  && punit->focus_status == FOCUS_AVAIL
	  && punit->activity == ACTIVITY_IDLE
	  && !unit_has_orders(punit)
	  && punit->moves_left > 0
	  && !punit->done_moving
	  && !punit->ai.control) {
	return punit;
      }
    } unit_list_iterate_end;
  } iterate_outward_end;

  return NULL;
}

/**************************************************************************
 This function may be called from packhand.c, via update_unit_focus(),
 as a result of packets indicating change in activity for a unit. Also
 called when user press the "Wait" command.
 
 FIXME: Add feature to focus only units of a certain category.
**************************************************************************/
void advance_unit_focus(void)
{
  struct unit *candidate = NULL;
  const int num_units_in_old_focus = get_num_units_in_focus();

  if (NULL == client.conn.playing
      || !is_player_phase(client.conn.playing, game.info.phase)
      || !can_client_change_view()) {
    set_unit_focus(NULL);
    return;
  }

  set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST, ORDER_LAST);

  unit_list_iterate(get_units_in_focus(), punit) {
    /* 
     * Is the unit which just lost focus a non-AI unit? If yes this
     * enables the auto end turn. 
     */
    if (!punit->ai.control) {
      non_ai_unit_focus = TRUE;
      break;
    }
  } unit_list_iterate_end;

  if (unit_list_size(urgent_focus_queue) > 0) {
    /* Try top of the urgent list. */
    candidate = unit_list_get(urgent_focus_queue, 0);

    if (get_num_units_in_focus() > 0) {
      /* Rarely, more than one unit on the same tile will be in the list. */
      unit_list_iterate(urgent_focus_queue, punit) {
        if (head_of_units_in_focus()->tile == punit->tile) {
          /* Use the first one found */
          candidate = punit;
          break;
        }
      } unit_list_iterate_end;
    }
    unit_list_unlink(urgent_focus_queue, candidate);

    /* Autocenter on Wakeup, regardless of the local option 
     * "auto_center_on_unit". */
    if (!tile_visible_and_not_on_border_mapcanvas(candidate->tile)) {
      center_tile_mapcanvas(candidate->tile);
    }
  } else {
    candidate = find_best_focus_candidate(FALSE);

    if (!candidate) {
      /* Try for "waiting" units. */
      unit_list_iterate(client.conn.playing->units, punit) {
        if(punit->focus_status == FOCUS_WAIT) {
          punit->focus_status = FOCUS_AVAIL;
        }
      } unit_list_iterate_end;
      candidate = find_best_focus_candidate(FALSE);

      if (!candidate) {
        /* Accept current focus unit as last resort. */
        candidate = find_best_focus_candidate(TRUE);
      }
    }
  }

  set_unit_focus(candidate);

  /* 
   * Handle auto-turn-done mode: If a unit was in focus (did move),
   * but now none are (no more to move) and there was at least one
   * non-AI unit this turn which was focused, then fake a Turn Done
   * keypress.
   */
  if (auto_turn_done
      && num_units_in_old_focus > 0
      && get_num_units_in_focus() == 0
      && non_ai_unit_focus) {
    key_end_turn();
  }
}

/**************************************************************************
 If there is no unit currently in focus, or if the current unit in
 focus should not be in focus, then get a new focus unit.
 We let GOTO-ing units stay in focus, so that if they have moves left
 at the end of the goto, then they are still in focus.
**************************************************************************/
void update_unit_focus(void)
{
  if (NULL == client.conn.playing || !can_client_change_view()) {
    return;
  }

  /* iterate zero times for no units in focus,
   * otherwise quit for any of the conditions. */
  unit_list_iterate(get_units_in_focus(), punit) {
    if ((punit->activity == ACTIVITY_IDLE
	 || punit->activity == ACTIVITY_GOTO
	 || unit_has_orders(punit))
	&& punit->moves_left > 0 
	&& !punit->done_moving
	&& !punit->ai.control) {
      return;
    }
  } unit_list_iterate_end;

  advance_unit_focus();
}

/**************************************************************************
Return a pointer to a visible unit, if there is one.
**************************************************************************/
struct unit *find_visible_unit(struct tile *ptile)
{
  struct unit *panyowned = NULL, *panyother = NULL, *ptptother = NULL;
  struct unit *pfocus;

  /* If no units here, return nothing. */
  if (unit_list_size(ptile->units)==0) {
    return NULL;
  }

  /* If a unit is attacking we should show that on top */
  if (punit_attacking && same_pos(punit_attacking->tile, ptile)) {
    unit_list_iterate(ptile->units, punit)
      if(punit == punit_attacking) return punit;
    unit_list_iterate_end;
  }

  /* If a unit is defending we should show that on top */
  if (punit_defending && same_pos(punit_defending->tile, ptile)) {
    unit_list_iterate(ptile->units, punit)
      if(punit == punit_defending) return punit;
    unit_list_iterate_end;
  }

  /* If the unit in focus is at this tile, show that on top */
  if ((pfocus = get_focus_unit_on_tile(ptile))) {
    return pfocus;
  }

  /* If a city is here, return nothing (unit hidden by city). */
  if (tile_city(ptile)) {
    return NULL;
  }

  /* Iterate through the units to find the best one we prioritize this way:
       1: owned transporter.
       2: any owned unit
       3: any transporter
       4: any unit
     (always return first in stack). */
  unit_list_iterate(ptile->units, punit)
    if (unit_owner(punit) == client.conn.playing) {
      if (punit->transported_by == -1) {
        if (get_transporter_capacity(punit) > 0) {
	  return punit;
        } else if (!panyowned) {
	  panyowned = punit;
        }
      }
    } else if (!ptptother && punit->transported_by == -1) {
      if (get_transporter_capacity(punit) > 0) {
	ptptother = punit;
      } else if (!panyother) {
	panyother = punit;
      }
    }
  unit_list_iterate_end;

  return (panyowned ? panyowned : (ptptother ? ptptother : panyother));
}

/**************************************************************************
  Blink the active unit (if necessary).  Return the time until the next
  blink (in seconds).
**************************************************************************/
double blink_active_unit(void)
{
  static struct timer *blink_timer = NULL;
  const double blink_time = get_focus_unit_toggle_timeout(tileset);

  if (get_num_units_in_focus() > 0) {
    if (!blink_timer || read_timer_seconds(blink_timer) > blink_time) {
      toggle_focus_unit_state(tileset);

      /* If we lag, we don't try to catch up.  Instead we just start a
       * new blink_time on every update. */
      blink_timer = renew_timer_start(blink_timer, TIMER_USER, TIMER_ACTIVE);

      unit_list_iterate(get_units_in_focus(), punit) {
	/* We flush to screen directly here.  This is most likely faster
	 * since these drawing operations are all small but may be spread
	 * out widely. */
	refresh_unit_mapcanvas(punit, punit->tile, FALSE, TRUE);
      } unit_list_iterate_end;
    }

    return blink_time - read_timer_seconds(blink_timer);
  }

  return blink_time;
}

/****************************************************************************
  Blink the turn done button (if necessary).  Return the time until the next
  blink (in seconds).
****************************************************************************/
double blink_turn_done_button(void)
{
  static struct timer *blink_timer = NULL;
  const double blink_time = 0.5; /* half-second blink interval */

  if (NULL != client.conn.playing
      && client.conn.playing->is_alive
      && !client.conn.playing->phase_done) {
    if (!blink_timer || read_timer_seconds(blink_timer) > blink_time) {
      int is_waiting = 0, is_moving = 0;

      players_iterate(pplayer) {
	if (pplayer->is_alive && pplayer->is_connected) {
	  if (pplayer->phase_done) {
	    is_waiting++;
	  } else {
	    is_moving++;
	  }
	}
      } players_iterate_end;

      if (is_moving == 1 && is_waiting > 0) {
	update_turn_done_button(FALSE);	/* stress the slow player! */
      }
      blink_timer = renew_timer_start(blink_timer, TIMER_USER, TIMER_ACTIVE);
    }
    return blink_time - read_timer_seconds(blink_timer);
  }

  return blink_time;
}

/**************************************************************************
  Update unit icons (and arrow) in the information display, for specified
  punit as the active unit and other units on the same square.  In practice
  punit is almost always (or maybe strictly always?) the focus unit.
  
  Static vars store some info on current (ie previous) state, to avoid
  unnecessary redraws; initialise to "flag" values to always redraw first
  time.  In principle we _might_ need more info (eg ai.control, connecting),
  but in practice this is enough?
  
  Used to store unit_ids for below units, to use for callbacks (now done
  inside gui-dep set_unit_icon()), but even with ids here they would not
  be enough information to know whether to redraw -- instead redraw every
  time.  (Could store enough info to know, but is it worth it?)
**************************************************************************/
void update_unit_pix_label(struct unit_list *punitlist)
{
  int i;

  /* Check for any change in the unit's state.  This assumes that a unit's
   * orders cannot be changed directly but must be removed and then reset. */
  if (punitlist && unit_list_size(punitlist) > 0
      && C_S_OVER != client_state()) {
    /* There used to be a complicated and bug-prone check here to see if
     * the unit had actually changed.  This was misguided since the stacked
     * units (below) are redrawn in any case.  Unless we write a general
     * system for unit updates here we might as well just redraw it every
     * time. */
    struct unit *punit = unit_list_get(punitlist, 0);

    set_unit_icon(-1, punit);

    i = 0;			/* index into unit_below_canvas */
    unit_list_iterate(punit->tile->units, aunit) {
      if (aunit != punit) {
	if (i < num_units_below) {
	  set_unit_icon(i, aunit);
	}
	i++;
      }
    }
    unit_list_iterate_end;
    
    if (i > num_units_below) {
      set_unit_icons_more_arrow(TRUE);
    } else {
      set_unit_icons_more_arrow(FALSE);
      for(; i < num_units_below; i++) {
	set_unit_icon(i, NULL);
      }
    }
  } else {
    for(i=-1; i<num_units_below; i++) {
      set_unit_icon(i, NULL);
    }
    set_unit_icons_more_arrow(FALSE);
  }
}

/**************************************************************************
...
**************************************************************************/
void set_units_in_combat(struct unit *pattacker, struct unit *pdefender)
{
  punit_attacking = pattacker;
  punit_defending = pdefender;

  if (unit_is_in_focus(punit_attacking)
      || unit_is_in_focus(punit_defending)) {
    /* If one of the units is the focus unit, make sure hidden-focus is
     * disabled.  We don't just do this as a check later because then
     * with a blinking unit it would just disappear again right after the
     * battle. */
    focus_unit_in_combat(tileset);
  }
}

/**************************************************************************
  Add punit to queue of caravan arrivals, and popup a window for the
  next arrival in the queue, if there is not already a popup, and
  re-checking that a popup is appropriate.
  If punit is NULL, just do for the next arrival in the queue.
**************************************************************************/
void process_caravan_arrival(struct unit *punit)
{
  int *p_id;

  /* caravan_arrival_queue is a list of individually malloc-ed ints with
     punit.id values, for units which have arrived. */

  if (punit) {
    p_id = fc_malloc(sizeof(int));
    *p_id = punit->id;
    genlist_prepend(caravan_arrival_queue, p_id);
  }

  /* There can only be one dialog at a time: */
  if (caravan_dialog_is_open(NULL, NULL)) {
    return;
  }
  
  while (genlist_size(caravan_arrival_queue) > 0) {
    int id;
    
    p_id = genlist_get(caravan_arrival_queue, 0);
    genlist_unlink(caravan_arrival_queue, p_id);
    id = *p_id;
    free(p_id);
    p_id = NULL;
    punit = game_find_unit_by_number(id);

    if (punit && (unit_can_help_build_wonder_here(punit)
		  || unit_can_est_traderoute_here(punit))
	&& (NULL == client.conn.playing
	    || (unit_owner(punit) == client.conn.playing
                && !client.conn.playing->ai_data.control))) {
      struct city *pcity_dest = tile_city(punit->tile);
      struct city *pcity_homecity = game_find_city_by_number(punit->homecity);

      if (pcity_dest && pcity_homecity) {
	popup_caravan_dialog(punit, pcity_homecity, pcity_dest);
	return;
      }
    }
  }
}

/**************************************************************************
  Add punit/pcity to queue of diplomat arrivals, and popup a window for
  the next arrival in the queue, if there is not already a popup, and
  re-checking that a popup is appropriate.
  If punit is NULL, just do for the next arrival in the queue.
**************************************************************************/
void process_diplomat_arrival(struct unit *pdiplomat, int victim_id)
{
  int *p_ids;

  /* diplomat_arrival_queue is a list of individually malloc-ed int[2]s with
     punit.id and pcity.id values, for units which have arrived. */

  if (pdiplomat && victim_id != 0) {
    p_ids = fc_malloc(2*sizeof(int));
    p_ids[0] = pdiplomat->id;
    p_ids[1] = victim_id;
    genlist_prepend(diplomat_arrival_queue, p_ids);
  }

  /* There can only be one dialog at a time: */
  if (diplomat_handled_in_diplomat_dialog() != -1) {
    return;
  }

  while (genlist_size(diplomat_arrival_queue) > 0) {
    int diplomat_id, victim_id;
    struct city *pcity;
    struct unit *punit;

    p_ids = genlist_get(diplomat_arrival_queue, 0);
    genlist_unlink(diplomat_arrival_queue, p_ids);
    diplomat_id = p_ids[0];
    victim_id = p_ids[1];
    free(p_ids);
    p_ids = NULL;
    pdiplomat = player_find_unit_by_id(client.conn.playing, diplomat_id);
    pcity = game_find_city_by_number(victim_id);
    punit = game_find_unit_by_number(victim_id);

    if (!pdiplomat || !unit_has_type_flag(pdiplomat, F_DIPLOMAT))
      continue;

    if (punit
	&& is_diplomat_action_available(pdiplomat, DIPLOMAT_ANY_ACTION,
					punit->tile)
	&& diplomat_can_do_action(pdiplomat, DIPLOMAT_ANY_ACTION,
				  punit->tile)) {
      popup_diplomat_dialog(pdiplomat, punit->tile);
      return;
    } else if (pcity
	       && is_diplomat_action_available(pdiplomat, DIPLOMAT_ANY_ACTION,
					       pcity->tile)
	       && diplomat_can_do_action(pdiplomat, DIPLOMAT_ANY_ACTION,
					 pcity->tile)) {
      popup_diplomat_dialog(pdiplomat, pcity->tile);
      return;
    }
  }
}

/**************************************************************************
  Do a goto with an order at the end (or ORDER_LAST).
**************************************************************************/
void request_unit_goto(enum unit_orders last_order)
{
  struct unit_list *punits = get_units_in_focus();

  if (unit_list_size(punits) == 0) {
    return;
  }

  if (hover_state != HOVER_GOTO) {
    set_hover_state(punits, HOVER_GOTO, ACTIVITY_LAST, last_order);
    enter_goto_state(punits);
    create_line_at_mouse_pos();
    update_unit_info_label(punits);
    control_mouse_cursor(NULL);
  } else {
    assert(goto_is_active());
    goto_add_waypoint();
  }
}

/****************************************************************************
  Return TRUE if at least one of the untis can do an attack at the tile.
****************************************************************************/
static bool can_units_attack_at(struct unit_list *punits,
				const struct tile *ptile)
{
  unit_list_iterate(punits, punit) {
    if (is_attack_unit(punit) && can_unit_attack_tile(punit, ptile)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/**************************************************************************
  Determines which mouse cursor should be used, according to hover_state,
  and the information gathered from the tile which is under the mouse 
  cursor (ptile).
**************************************************************************/
void control_mouse_cursor(struct tile *ptile)
{
  struct unit *punit = NULL;
  struct city *pcity = NULL;
  struct unit_list *active_units = get_units_in_focus();
  enum cursor_type mouse_cursor_type = CURSOR_DEFAULT;

  if (!enable_cursor_changes) {
    return;
  }

  if (C_S_RUNNING != client_state()) {
    update_mouse_cursor(CURSOR_DEFAULT);
    return;
  }

  if (is_server_busy()) {
    /* Server will not accept any commands. */
    update_mouse_cursor(CURSOR_WAIT);
    return;
  }

  if (!ptile) {
    if (hover_tile) {
      /* hover_tile is the tile that was previously under the mouse cursor. */
      ptile = hover_tile;
    } else {
      update_mouse_cursor(CURSOR_DEFAULT);
      return;
    }
  } else {
    hover_tile = ptile;
  }

  punit = find_visible_unit(ptile);
  pcity = ptile ? tile_city(ptile) : NULL;

  switch (hover_state) {
  case HOVER_NONE:
    if (NULL != punit
        && unit_owner(punit) == client_player()) {
      /* Set mouse cursor to select a unit.  */
      mouse_cursor_type = CURSOR_SELECT;
    } else if (NULL != pcity
	       && can_player_see_city_internals(client.conn.playing, pcity)) {
      /* Set mouse cursor to select a city. */
      mouse_cursor_type = CURSOR_SELECT;
    } else {
      /* Set default mouse cursor, because nothing selectable found. */
    }
    break;
  case HOVER_GOTO:
    /* Determine if the goto is valid, invalid or will attack. */
    if (is_valid_goto_destination(ptile)) {
      if (can_units_attack_at(active_units, ptile)) {
        /* Goto results in military attack. */
	mouse_cursor_type = CURSOR_ATTACK;
      } else if (is_enemy_city_tile(ptile, client.conn.playing)) {
        /* Goto results in attack of enemy city. */
	mouse_cursor_type = CURSOR_ATTACK;
      } else {
	mouse_cursor_type = CURSOR_GOTO;
      }
    } else {
      mouse_cursor_type = CURSOR_INVALID;
    }
    break;
  case HOVER_PATROL:
    if (is_valid_goto_destination(ptile)) {
      mouse_cursor_type = CURSOR_PATROL;
    } else {
      mouse_cursor_type = CURSOR_INVALID;
    }
    break;
  case HOVER_CONNECT:
    if (is_valid_goto_destination(ptile)) {
      mouse_cursor_type = CURSOR_GOTO;
    } else {
      mouse_cursor_type = CURSOR_INVALID;
    }
    break;
  case HOVER_NUKE:
    /* FIXME: check for invalid tiles. */
    mouse_cursor_type = CURSOR_NUKE;
    break;
  case HOVER_PARADROP:
    /* FIXME: check for invalid tiles. */
    mouse_cursor_type = CURSOR_PARADROP;
    break;
  };

  update_mouse_cursor(mouse_cursor_type);
}

/**************************************************************************
  Return TRUE if there are any units doing the activity on the tile.
**************************************************************************/
static bool is_activity_on_tile(struct tile *ptile,
				enum unit_activity activity)
{
  unit_list_iterate(ptile->units, punit) {
    if (punit->activity == ACTIVITY_MINE) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/**************************************************************************
  Return whether the unit can connect with given activity (or with
  any activity if activity arg is set to ACTIVITY_IDLE)

  This function is client-specific.
**************************************************************************/
bool can_unit_do_connect(struct unit *punit, enum unit_activity activity) 
{
  struct player *pplayer = unit_owner(punit);
  struct terrain *pterrain = tile_terrain(punit->tile);

  /* HACK: This code duplicates that in
   * can_unit_do_activity_targeted_at(). The general logic here is that
   * the connect is allowed if both:
   * (1) the unit can do that activity type, in general
   * (2) either
   *     (a) the activity has already been completed at this tile
   *     (b) it can be done by the unit at this tile. */
  switch (activity) {
  case ACTIVITY_ROAD:
    return terrain_control.may_road
      && unit_has_type_flag(punit, F_SETTLERS)
      && (tile_has_special(punit->tile, S_ROAD)
	  || (pterrain->road_time != 0
	      && (!tile_has_special(punit->tile, S_RIVER)
		  || player_knows_techs_with_flag(pplayer, TF_BRIDGE))));
  case ACTIVITY_RAILROAD:
    /* There is no check for existing road/rail; the connect is allowed
     * regardless. It is assumed that if you know the TF_RAILROAD flag
     * you must also know the TF_BRIDGE flag. */
    return (terrain_control.may_road
	    && unit_has_type_flag(punit, F_SETTLERS)
	    && player_knows_techs_with_flag(pplayer, TF_RAILROAD));
  case ACTIVITY_IRRIGATE:
    /* Special case for irrigation: only irrigate to make S_IRRIGATION,
     * never to transform tiles. */
    return (terrain_control.may_irrigate
	    && unit_has_type_flag(punit, F_SETTLERS)
	    && (tile_has_special(punit->tile, S_IRRIGATION)
		|| (pterrain == pterrain->irrigation_result
		    && is_water_adjacent_to_tile(punit->tile)
		    && !is_activity_on_tile(punit->tile, ACTIVITY_MINE))));
  default:
    break;
  }

  return FALSE;
}

/**************************************************************************
prompt player for entering destination point for unit connect
(e.g. connecting with roads)
**************************************************************************/
void request_unit_connect(enum unit_activity activity)
{
  struct unit_list *punits = get_units_in_focus();

  if (!can_units_do_connect(punits, activity)) {
    return;
  }

  if (hover_state != HOVER_CONNECT || connect_activity != activity) {
    set_hover_state(punits, HOVER_CONNECT, activity, ORDER_LAST);
    enter_goto_state(punits);
    create_line_at_mouse_pos();
    update_unit_info_label(punits);
    control_mouse_cursor(NULL);
  } else {
    assert(goto_is_active());
    goto_add_waypoint();
  }
}

/**************************************************************************
  Returns one of the unit of the transporter which can have focus next.
**************************************************************************/
struct unit *request_unit_unload_all(struct unit *punit)
{
  struct tile *ptile = punit->tile;
  struct unit *plast = NULL;

  if (get_transporter_capacity(punit) == 0) {
    create_event(punit->tile, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                 _("Only transporter units can be unloaded."));
    return NULL;
  }

  unit_list_iterate(ptile->units, pcargo) {
    if (pcargo->transported_by == punit->id) {
      request_unit_unload(pcargo);

      if (pcargo->activity == ACTIVITY_SENTRY) {
	request_new_unit_activity(pcargo, ACTIVITY_IDLE);
      }

      if (unit_owner(pcargo) == unit_owner(punit)) {
	plast = pcargo;
      }
    }
  } unit_list_iterate_end;

  return plast;
}

/**************************************************************************
...
**************************************************************************/
void request_unit_airlift(struct unit *punit, struct city *pcity)
{
  dsend_packet_unit_airlift(&client.conn, punit->id,pcity->id);
}

/**************************************************************************
  Return-and-recover for a particular unit.  This sets the unit to GOTO
  the nearest city.
**************************************************************************/
void request_unit_return(struct unit *punit)
{
  struct pf_path *path;

  if ((path = path_to_nearest_allied_city(punit))) {
    int turns = pf_path_get_last_position(path)->turn;
    int max_hp = unit_type(punit)->hp;

    if (punit->hp + turns *
        (get_unit_bonus(punit, EFT_UNIT_RECOVER)
         - (max_hp * unit_class(punit)->hp_loss_pct / 100))
	< max_hp) {
      struct unit_order order;

      order.order = ORDER_ACTIVITY;
      order.activity = ACTIVITY_SENTRY;
      send_goto_path(punit, path, &order);
    } else {
      send_goto_path(punit, path, NULL);
    }
    pf_path_destroy(path);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void wakeup_sentried_units(struct tile *ptile)
{
  if (!can_client_issue_orders()) {
    return;
  }
  unit_list_iterate(ptile->units, punit) {
    if (punit->activity == ACTIVITY_SENTRY
	&& unit_owner(punit) == client.conn.playing) {
      request_new_unit_activity(punit, ACTIVITY_IDLE);
    }
  }
  unit_list_iterate_end;
}

/**************************************************************************
(RP:) un-sentry all my own sentried units on punit's tile
**************************************************************************/
void request_unit_wakeup(struct unit *punit)
{
  wakeup_sentried_units(punit->tile);
}

/****************************************************************************
  Select all units based on the given list of units and the selection modes.
****************************************************************************/
void request_unit_select(struct unit_list *punits,
                         enum unit_select_type_mode seltype,
                         enum unit_select_location_mode selloc)
{
  const struct player *pplayer;
  const struct tile *ptile;
  struct unit *punit_first;
  struct hash_table *tile_table, *type_table, *cont_table;

  if (!can_client_change_view() || !punits
      || unit_list_size(punits) < 1) {
    return;
  }

  punit_first = unit_list_get(punits, 0);

  if (seltype == SELTYPE_SINGLE) {
    set_unit_focus(punit_first);
    return;
  }

  pplayer = unit_owner(punit_first);
  tile_table = hash_new(hash_fval_keyval, hash_fcmp_keyval);
  type_table = hash_new(hash_fval_keyval, hash_fcmp_keyval);
  cont_table = hash_new(hash_fval_int, hash_fcmp_int);

  unit_list_iterate(punits, punit) {
    if (seltype == SELTYPE_SAME) {
      hash_insert(type_table, unit_type(punit), NULL);
    }

    ptile = unit_tile(punit);
    if (selloc == SELLOC_TILE) {
      hash_insert(tile_table, ptile, NULL);
    } else if (selloc == SELLOC_CONT) {
      hash_insert(cont_table, &ptile->continent, NULL);
    }
  } unit_list_iterate_end;

  if (selloc == SELLOC_TILE) {
    hash_keys_iterate(tile_table, key) {
      ptile = key;
      unit_list_iterate(ptile->units, punit) {
        if (unit_owner(punit) != pplayer) {
          continue;
        }
        if (seltype == SELTYPE_SAME
            && !hash_key_exists(type_table, unit_type(punit))) {
          continue;
        }
        add_unit_focus(punit);
      } unit_list_iterate_end;
    } hash_keys_iterate_end;
  } else {
    unit_list_iterate(pplayer->units, punit) {
      ptile = unit_tile(punit);
      if ((seltype == SELTYPE_SAME
           && !hash_key_exists(type_table, unit_type(punit)))
          || (selloc == SELLOC_CONT
              && !hash_key_exists(cont_table, &ptile->continent))) {
        continue;
      }

      add_unit_focus(punit);
    } unit_list_iterate_end;
  }

  hash_free(tile_table);
  hash_free(type_table);
  hash_free(cont_table);
}

/**************************************************************************
  Request a diplomat to do a specific action.
  - action : The action to be requested.
  - dipl_id : The unit ID of the diplomatic unit.
  - target_id : The ID of the target unit or city.
  - value : For DIPLOMAT_STEAL or DIPLOMAT_SABOTAGE, the technology
            or building to aim for (spies only).
**************************************************************************/
void request_diplomat_action(enum diplomat_actions action, int dipl_id,
			     int target_id, int value)
{
  dsend_packet_unit_diplomat_action(&client.conn, dipl_id,target_id,value,action);
}

/**************************************************************************
  Query a diplomat about costs and infrastructure.
  - action : The action to be requested.
  - dipl_id : The unit ID of the diplomatic unit.
  - target_id : The ID of the target unit or city.
  - value : For DIPLOMAT_STEAL or DIPLOMAT_SABOTAGE, the technology
            or building to aim for (spies only).
**************************************************************************/
void request_diplomat_answer(enum diplomat_actions action, int dipl_id,
			     int target_id, int value)
{
  dsend_packet_unit_diplomat_query(&client.conn, dipl_id,target_id,value,action);
}

/**************************************************************************
Player pressed 'b' or otherwise instructed unit to build or add to city.
If the unit can build a city, we popup the appropriate dialog.
Otherwise, we just send a packet to the server.
If this action is not appropriate, the server will respond
with an appropriate message.  (This is to avoid duplicating
all the server checks and messages here.)
**************************************************************************/
void request_unit_build_city(struct unit *punit)
{
  if (can_unit_build_city(punit)) {
    dsend_packet_city_name_suggestion_req(&client.conn, punit->id);
    /* the reply will trigger a dialog to name the new city */
  } else {
    char name[] = "";
    dsend_packet_unit_build_city(&client.conn, punit->id, name);
  }
}

/**************************************************************************
  This function is called whenever the player pressed an arrow key.

  We do NOT take into account that punit might be a caravan or a diplomat
  trying to move into a city, or a diplomat going into a tile with a unit;
  the server will catch those cases and send the client a package to pop up
  a dialog. (the server code has to be there anyway as goto's are entirely
  in the server)
**************************************************************************/
void request_move_unit_direction(struct unit *punit, int dir)
{
  struct tile *dest_tile;

  /* Catches attempts to move off map */
  dest_tile = mapstep(punit->tile, dir);
  if (!dest_tile) {
    return;
  }

  if (punit->moves_left > 0) {
    dsend_packet_unit_move(&client.conn, punit->id,
			   dest_tile->x, dest_tile->y);
  } else {
    /* Initiate a "goto" with direction keys for exhausted units. */
    send_goto_tile(punit, dest_tile);
  }
}

/**************************************************************************
...
**************************************************************************/
void request_new_unit_activity(struct unit *punit, enum unit_activity act)
{
  if (!can_client_issue_orders()) {
    return;
  }

  dsend_packet_unit_change_activity(&client.conn, punit->id, act,
				    S_LAST, -1);
}

/**************************************************************************
...
**************************************************************************/
void request_new_unit_activity_targeted(struct unit *punit,
					enum unit_activity act,
					enum tile_special_type tgt,
                                        Base_type_id base)
{
  dsend_packet_unit_change_activity(&client.conn, punit->id, act, tgt,
                                    base);
}

/**************************************************************************
  Request base building activity for unit
**************************************************************************/
void request_new_unit_activity_base(struct unit *punit,
				    const struct base_type *pbase)
{
  if (!can_client_issue_orders()) {
    return;
  }

  dsend_packet_unit_change_activity(&client.conn, punit->id, ACTIVITY_BASE,
				    S_LAST, base_number(pbase));
}

/**************************************************************************
...
**************************************************************************/
void request_unit_disband(struct unit *punit)
{
  dsend_packet_unit_disband(&client.conn, punit->id);
}

/**************************************************************************
...
**************************************************************************/
void request_unit_change_homecity(struct unit *punit)
{
  struct city *pcity=tile_city(punit->tile);
  
  if (pcity) {
    dsend_packet_unit_change_homecity(&client.conn, punit->id, pcity->id);
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_upgrade(struct unit *punit)
{
  struct city *pcity=tile_city(punit->tile);

  if (pcity) {
    dsend_packet_unit_upgrade(&client.conn, punit->id);
  }
}

/**************************************************************************
  Sends unit transform packet.
**************************************************************************/
void request_unit_transform(struct unit *punit)
{
  dsend_packet_unit_transform(&client.conn, punit->id);
}

/****************************************************************************
  Call to request (from the server) that the settler unit is put into
  autosettler mode.
****************************************************************************/
void request_unit_autosettlers(const struct unit *punit)
{
  if (punit && can_unit_do_autosettlers(punit)) {
    dsend_packet_unit_autosettlers(&client.conn, punit->id);
  } else if (punit) {
    create_event(punit->tile, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                 _("Only settler units can be put into auto mode."));
  }
}

/****************************************************************************
  Send a request to the server that the cargo be loaded into the transporter.

  If ptransporter is NULL a transporter will be picked at random.
****************************************************************************/
void request_unit_load(struct unit *pcargo, struct unit *ptrans)
{
  if (!ptrans) {
    ptrans = find_transporter_for_unit(pcargo);
  }

  if (ptrans
      && can_client_issue_orders()
      && can_unit_load(pcargo, ptrans)) {
    dsend_packet_unit_load(&client.conn, pcargo->id, ptrans->id);

    /* Sentry the unit.  Don't request_unit_sentry since this can give a
     * recursive loop. */
    dsend_packet_unit_change_activity(&client.conn, pcargo->id,
				      ACTIVITY_SENTRY, S_LAST, -1);
  }
}

/****************************************************************************
  Send a request to the server that the cargo be unloaded from its current
  transporter.
****************************************************************************/
void request_unit_unload(struct unit *pcargo)
{
  struct unit *ptrans = game_find_unit_by_number(pcargo->transported_by);

  if (can_client_issue_orders()
      && ptrans
      && can_unit_unload(pcargo, ptrans)
      && can_unit_survive_at_tile(pcargo, pcargo->tile)) {
    dsend_packet_unit_unload(&client.conn, pcargo->id, ptrans->id);

    if (unit_owner(pcargo) == client.conn.playing
        && pcargo->activity != ACTIVITY_IDLE) {
      /* Activate the unit. */
      dsend_packet_unit_change_activity(&client.conn, pcargo->id,
                                        ACTIVITY_IDLE, S_LAST, -1);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_caravan_action(struct unit *punit, enum packet_type action)
{
  if (!tile_city(punit->tile)) {
    return;
  }

  if (action == PACKET_UNIT_ESTABLISH_TRADE) {
    dsend_packet_unit_establish_trade(&client.conn, punit->id);
  } else if (action == PACKET_UNIT_HELP_BUILD_WONDER) {
    dsend_packet_unit_help_build_wonder(&client.conn, punit->id);
  } else {
    freelog(LOG_ERROR, "request_unit_caravan_action() Bad action (%d)",
	    action);
  }
}

/**************************************************************************
 Explode nuclear at a tile without enemy units
**************************************************************************/
void request_unit_nuke(struct unit_list *punits)
{
  bool can = FALSE;
  struct tile *offender = NULL;

  if (unit_list_size(punits) == 0) {
    return;
  }
  unit_list_iterate(punits, punit) {
    if (unit_has_type_flag(punit, F_NUCLEAR)) {
      can = TRUE;
      break;
    }
    if (!offender) { /* Take first offender tile/unit */
      offender = punit->tile;
    }
  } unit_list_iterate_end;
  if (can) {
    set_hover_state(punits, HOVER_NUKE, ACTIVITY_LAST, ORDER_LAST);
    update_unit_info_label(punits);
    enter_goto_state(punits);
  } else {
    create_event(offender, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                 _("Only nuclear units can do this."));
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_paradrop(struct unit_list *punits)
{
  bool can = FALSE;
  struct tile *offender = NULL;

  if (unit_list_size(punits) == 0) {
    return;
  }
  unit_list_iterate(punits, punit) {
    if (can_unit_paradrop(punit)) {
      can = TRUE;
      break;
    }
    if (!offender) { /* Take first offender tile/unit */
      offender = punit->tile;
    }
  } unit_list_iterate_end;
  if (can) {
    set_hover_state(punits, HOVER_PARADROP, ACTIVITY_LAST, ORDER_LAST);
    update_unit_info_label(punits);
  } else {
    create_event(offender, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                 _("Only paratrooper units can do this."));
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_patrol(void)
{
  struct unit_list *punits = get_units_in_focus();

  if (unit_list_size(punits) == 0) {
    return;
  }

  if (hover_state != HOVER_PATROL) {
    set_hover_state(punits, HOVER_PATROL, ACTIVITY_LAST, ORDER_LAST);
    update_unit_info_label(punits);
    enter_goto_state(punits);
    create_line_at_mouse_pos();
  } else {
    assert(goto_is_active());
    goto_add_waypoint();
  }
}

/****************************************************************
...
*****************************************************************/
void request_unit_sentry(struct unit *punit)
{
  if(punit->activity!=ACTIVITY_SENTRY &&
     can_unit_do_activity(punit, ACTIVITY_SENTRY))
    request_new_unit_activity(punit, ACTIVITY_SENTRY);
}

/****************************************************************
...
*****************************************************************/
void request_unit_fortify(struct unit *punit)
{
  if(punit->activity!=ACTIVITY_FORTIFYING &&
     can_unit_do_activity(punit, ACTIVITY_FORTIFYING))
    request_new_unit_activity(punit, ACTIVITY_FORTIFYING);
}

/**************************************************************************
...
**************************************************************************/
void request_unit_pillage(struct unit *punit)
{
  bv_special pspossible;
  bv_bases bases;
  struct tile *ptile = punit->tile;
  bv_special pspresent = get_tile_infrastructure_set(ptile, NULL);
  bv_special psworking = get_unit_tile_pillage_set(ptile);
  int count = 0;

  BV_CLR_ALL(pspossible);
  tile_special_type_iterate(spe) {
    if (BV_ISSET(pspresent, spe) && !BV_ISSET(psworking, spe)) {
      BV_SET(pspossible, spe);
      count++;
    }
  } tile_special_type_iterate_end;

  BV_CLR_ALL(bases);
  base_type_iterate(pbase) {
    if (tile_has_base(ptile, pbase)) {
      if (pbase->pillageable) {
        BV_SET(bases, base_index(pbase));
        count++;
      }
    }
  } base_type_iterate_end;

  if (count > 1) {
    popup_pillage_dialog(punit, pspossible, bases);
  } else {
    Base_type_id pillage_base = -1;
    int what = get_preferred_pillage(pspossible, bases);

    if (what > S_LAST) {
      pillage_base = what - S_LAST - 1;
      what = S_LAST;
    }

    request_new_unit_activity_targeted(punit, ACTIVITY_PILLAGE, what,
                                       pillage_base);
  }
}

/**************************************************************************
 Toggle display of city outlines on the map
**************************************************************************/
void request_toggle_city_outlines(void) 
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_outlines = !draw_city_outlines;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of worker output of cities on the map
**************************************************************************/
void request_toggle_city_output(void)
{
  if (!can_client_change_view()) {
    return;
  }
  
  draw_city_output = !draw_city_output;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of grid lines on the map
**************************************************************************/
void request_toggle_map_grid(void) 
{
  if (!can_client_change_view()) {
    return;
  }

  draw_map_grid^=1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of national borders on the map
**************************************************************************/
void request_toggle_map_borders(void) 
{
  if (!can_client_change_view()) {
    return;
  }

  draw_borders ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of city names
**************************************************************************/
void request_toggle_city_names(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_names ^= 1;
  update_map_canvas_visible();
}
 
 /**************************************************************************
 Toggle display of city growth (turns-to-grow)
**************************************************************************/
void request_toggle_city_growth(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_growth ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of city productions
**************************************************************************/
void request_toggle_city_productions(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_productions ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of city buycost
**************************************************************************/
void request_toggle_city_buycost(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_buycost ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of city traderoutes
**************************************************************************/
void request_toggle_city_traderoutes(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_city_traderoutes ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of terrain
**************************************************************************/
void request_toggle_terrain(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_terrain ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of coastline
**************************************************************************/
void request_toggle_coastline(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_coastline ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of roads and rails
**************************************************************************/
void request_toggle_roads_rails(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_roads_rails ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of irrigation
**************************************************************************/
void request_toggle_irrigation(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_irrigation ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of mines
**************************************************************************/
void request_toggle_mines(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_mines ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of fortress and airbase
**************************************************************************/
void request_toggle_fortress_airbase(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_fortress_airbase ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of specials
**************************************************************************/
void request_toggle_specials(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_specials ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of pollution
**************************************************************************/
void request_toggle_pollution(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_pollution ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of cities
**************************************************************************/
void request_toggle_cities(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_cities ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of units
**************************************************************************/
void request_toggle_units(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_units ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of focus unit
**************************************************************************/
void request_toggle_focus_unit(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_focus_unit ^= 1;
  update_map_canvas_visible();
}

/**************************************************************************
 Toggle display of fog of war
**************************************************************************/
void request_toggle_fog_of_war(void)
{
  if (!can_client_change_view()) {
    return;
  }

  draw_fog_of_war ^= 1;
  update_map_canvas_visible();
  refresh_overview_canvas();
}

/**************************************************************************
...
**************************************************************************/
void request_center_focus_unit(void)
{
  struct tile *ptile = find_a_focus_unit_tile_to_center_on();

  if (ptile) {
    center_tile_mapcanvas(ptile);
  }
}

/**************************************************************************
...
**************************************************************************/
void request_units_wait(struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    punit->focus_status = FOCUS_WAIT;
  } unit_list_iterate_end;
  if (punits == get_units_in_focus()) {
    advance_unit_focus();
  }
}

/**************************************************************************
...
**************************************************************************/
void request_unit_move_done(void)
{
  if (get_num_units_in_focus() > 0) {
    unit_list_iterate(get_units_in_focus(), punit) {
      clear_unit_orders(punit);
      punit->focus_status = FOCUS_DONE;
    } unit_list_iterate_end;
    advance_unit_focus();
  }
}

/**************************************************************************
  Called to have the client move a unit from one location to another,
  updating the graphics if necessary.  The caller must redraw the target
  location after the move.
**************************************************************************/
void do_move_unit(struct unit *punit, struct unit *target_unit)
{
  struct tile *src_tile = punit->tile, *dst_tile = target_unit->tile;
  bool was_teleported, do_animation;

  was_teleported = !is_tiles_adjacent(src_tile, dst_tile);
  do_animation = (!was_teleported && smooth_move_unit_msec > 0);

  if (!was_teleported
      && punit->activity != ACTIVITY_SENTRY
      && punit->transported_by == -1) {
    audio_play_sound(unit_type(punit)->sound_move,
		     unit_type(punit)->sound_move_alt);
  }

  unit_list_unlink(src_tile->units, punit);

  if (unit_owner(punit) == client.conn.playing
      && auto_center_on_unit
      && !unit_has_orders(punit)
      && punit->activity != ACTIVITY_GOTO
      && punit->activity != ACTIVITY_SENTRY
      && !tile_visible_and_not_on_border_mapcanvas(dst_tile)) {
    center_tile_mapcanvas(dst_tile);
  }

  /* Set the tile before the movement animation is done, so that everything
   * drawn there will be up-to-date. */
  punit->tile = dst_tile;

  if (punit->transported_by == -1) {
    /* We have to refresh the tile before moving.  This will draw
     * the tile without the unit (because it was unlinked above). */
    refresh_unit_mapcanvas(punit, src_tile, TRUE, FALSE);

    if (do_animation) {
      int dx, dy;

      /* For the duration of the animation the unit exists at neither
       * tile. */
      map_distance_vector(&dx, &dy, src_tile, dst_tile);
      move_unit_map_canvas(punit, src_tile, dx, dy);
    }
  }

  unit_list_prepend(dst_tile->units, punit);

  if (punit->transported_by == -1) {
    refresh_unit_mapcanvas(punit, dst_tile, TRUE, FALSE);
  }

  /* With the "full" citybar we have to update the citybar when units move
   * into or out of a city.  For foreign cities this is handled separately,
   * via the occupied field of the short-city packet. */
  if (NULL != tile_city(src_tile)
      && can_player_see_units_in_city(client.conn.playing, tile_city(src_tile))) {
    update_city_description(tile_city(src_tile));
  }
  if (NULL != tile_city(dst_tile)
      && can_player_see_units_in_city(client.conn.playing, tile_city(dst_tile))) {
    update_city_description(tile_city(dst_tile));
  }

  if (unit_is_in_focus(punit)) {
    update_menus();
  }
}

/**************************************************************************
 Handles everything when the user clicked a tile
**************************************************************************/
void do_map_click(struct tile *ptile, enum quickselect_type qtype)
{
  struct city *pcity = tile_city(ptile);
  struct unit_list *punits = get_units_in_focus();
  bool maybe_goto = FALSE;
  bool possible = FALSE;
  struct tile *offender = NULL;

  if (hover_state != HOVER_NONE) {
    switch (hover_state) {
    case HOVER_NONE:
      die("well; shouldn't get here :)");
    case HOVER_GOTO:
      do_unit_goto(ptile);
      break;
    case HOVER_NUKE:
      unit_list_iterate(punits, punit) {
	if (SINGLE_MOVE * real_map_distance(punit->tile, ptile)
	    <= punit->moves_left) {
	  possible = TRUE;
	  break;
	}
	offender = punit->tile;
      } unit_list_iterate_end;
      if (!possible) {
        create_event(offender, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                     _("Too far for this unit."));
      } else {
        do_unit_goto(ptile);
	if (!pcity) {
	  unit_list_iterate(punits, punit) {
	    /* note that this will be executed by the server after the goto */
	    do_unit_nuke(punit);
	  } unit_list_iterate_end;
	}
      }
      break;
    case HOVER_PARADROP:
      unit_list_iterate(punits, punit) {
	do_unit_paradrop_to(punit, ptile);
      } unit_list_iterate_end;
      break;
    case HOVER_CONNECT:
      do_unit_connect(ptile, connect_activity);
      break;
    case HOVER_PATROL:
      do_unit_patrol_to(ptile);
      break;	
    }

    set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST, ORDER_LAST);
    update_unit_info_label(get_units_in_focus());
  }

  /* Bypass stack or city popup if quickselect is specified. */
  else if (qtype != SELECT_POPUP && qtype != SELECT_APPEND) {
    struct unit *qunit = quickselect(ptile, qtype);
    if (qunit) {
      set_unit_focus_and_select(qunit);
      maybe_goto = keyboardless_goto;
    }
  }
  /* Otherwise use popups. */
  else if (NULL != pcity
           && can_player_see_city_internals(client.conn.playing, pcity)) {
    popup_city_dialog(pcity);
  }
  else if (unit_list_size(ptile->units) == 0
           && NULL == pcity
           && get_num_units_in_focus() > 0) {
    maybe_goto = keyboardless_goto;
  }
  else if (unit_list_size(ptile->units) == 1
           && !unit_list_get(ptile->units, 0)->occupy) {
    struct unit *punit=unit_list_get(ptile->units, 0);

    if (unit_owner(punit) == client.conn.playing) {
      if(can_unit_do_activity(punit, ACTIVITY_IDLE)) {
        maybe_goto = keyboardless_goto;
	if (qtype == SELECT_APPEND) {
	  add_unit_focus(punit);
	} else {
	  set_unit_focus_and_select(punit);
	}
      }
    } else if (pcity) {
      /* Don't hide the unit in the city. */
      popup_unit_select_dialog(ptile);
    }
  }
  else if(unit_list_size(ptile->units) > 0) {
    /* The stack list is always popped up, even if it includes enemy units.
     * If the server doesn't want the player to know about them it shouldn't
     * tell him!  The previous behavior would only pop up the stack if you
     * owned a unit on the tile.  This gave cheating clients an advantage,
     * and also showed you allied units if (and only if) you had a unit on
     * the tile (inconsistent). */
    popup_unit_select_dialog(ptile);
  }

  /* See mapctrl_common.c */
  keyboardless_goto_start_tile = maybe_goto ? ptile : NULL;
  keyboardless_goto_button_down = maybe_goto;
  keyboardless_goto_active = FALSE;
}

/**************************************************************************
 Quickselecting a unit is normally done with <control> left, right click,
 for the current tile. Bypassing the stack popup is quite convenient, 
 and can be tactically important in furious multiplayer games.
**************************************************************************/
static struct unit *quickselect(struct tile *ptile,
                                enum quickselect_type qtype)
{
  int listsize = unit_list_size(ptile->units);
  struct unit *panytransporter = NULL,
              *panymovesea  = NULL, *panysea  = NULL,
              *panymoveland = NULL, *panyland = NULL,
              *panymoveunit = NULL, *panyunit = NULL;

  assert(qtype > SELECT_POPUP);

  if (listsize == 0) {
    return NULL;
  } else if (listsize == 1) {
    struct unit *punit = unit_list_get(ptile->units, 0);
    return (unit_owner(punit) == client.conn.playing) ? punit : NULL;
  }

  /*  Quickselect priorities. Units with moves left
   *  before exhausted. Focus unit is excluded.
   *
   *    SEA:  Transporter
   *          Sea unit
   *          Any unit
   *
   *    LAND: Military land unit
   *          Non-combatant
   *          Sea unit
   *          Any unit
   */

  unit_list_iterate(ptile->units, punit)  {
    if (unit_owner(punit) != client.conn.playing || unit_is_in_focus(punit)) {
      continue;
    }
  if (qtype == SELECT_SEA) {
    /* Transporter. */
    if (get_transporter_capacity(punit)) {
      if (punit->moves_left > 0) {
        return punit;
      } else if (!panytransporter) {
        panytransporter = punit;
      }
    }
    /* Any sea, pref. moves left. */
    else if (is_sailing_unit(punit)) {
      if (punit->moves_left > 0) {
        if (!panymovesea) {
          panymovesea = punit;
        }
      } else if (!panysea) {
          panysea = punit;
      }
    }
  } else if (qtype == SELECT_LAND) {
    if (is_ground_unit(punit))  {
      if (punit->moves_left > 0) {
        if (is_military_unit(punit)) {
          return punit;
        } else if (!panymoveland) {
            panymoveland = punit;
        }
      } else if (!panyland) {
        panyland = punit;
      }
    }
    else if (is_sailing_unit(punit)) {
      if (punit->moves_left > 0) {
        panymovesea = punit;
      } else {
        panysea = punit;
      }
    }
  }
  if (punit->moves_left > 0 && !panymoveunit) {
    panymoveunit = punit;
  }
  if (!panyunit) {
    panyunit = punit;
  }
    } unit_list_iterate_end;

  if (qtype == SELECT_SEA) {
    if (panytransporter) {
      return panytransporter;
    } else if (panymovesea) {
      return panymovesea;
    } else if (panysea) {
      return panysea;
    } else if (panymoveunit) {
      return panymoveunit;
    } else if (panyunit) {
      return panyunit;
    }
  }
  else if (qtype == SELECT_LAND) {
    if (panymoveland) {
      return panymoveland;
    } else if (panyland) {
      return panyland;
    } else if (panymovesea) {
      return panymovesea;
    } else if (panysea) {
      return panysea;
    } else if (panymoveunit) {
      return panymoveunit;
    } else if (panyunit) {
      return panyunit;
    }
  }
  return NULL;
}

/**************************************************************************
 Finish the goto mode and let the units stored in goto_map_list move
 to a given location.
**************************************************************************/
void do_unit_goto(struct tile *ptile)
{
  if (hover_state != HOVER_GOTO && hover_state != HOVER_NUKE) {
    return;
  }

  if (is_valid_goto_draw_line(ptile)) {
    send_goto_route();
  } else {
    create_event(ptile, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                 _("Didn't find a route to the destination!"));
  }
}

/**************************************************************************
Explode nuclear at a tile without enemy units
**************************************************************************/
void do_unit_nuke(struct unit *punit)
{
  dsend_packet_unit_nuke(&client.conn, punit->id);
}

/**************************************************************************
  Paradrop to a location.
**************************************************************************/
void do_unit_paradrop_to(struct unit *punit, struct tile *ptile)
{
  dsend_packet_unit_paradrop_to(&client.conn, punit->id, ptile->x, ptile->y);
}
 
/**************************************************************************
  Patrol to a location.
**************************************************************************/
void do_unit_patrol_to(struct tile *ptile)
{
  if (is_valid_goto_draw_line(ptile)
      && !is_non_allied_unit_tile(ptile, client.conn.playing)) {
    send_patrol_route();
  } else {
    create_event(ptile, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                 _("Didn't find a route to the destination!"));
  }

  set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST, ORDER_LAST);
}
 
/**************************************************************************
  "Connect" to the given location.
**************************************************************************/
void do_unit_connect(struct tile *ptile,
		     enum unit_activity activity)
{
  if (is_valid_goto_draw_line(ptile)) {
    send_connect_route(activity);
  } else {
    create_event(ptile, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                 _("Didn't find a route to the destination!"));
  }

  set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST, ORDER_LAST);
}
 
/**************************************************************************
 The 'Escape' key.
**************************************************************************/
void key_cancel_action(void)
{
  cancel_tile_hiliting();

  switch (hover_state) {
  case HOVER_GOTO:
  case HOVER_PATROL:
  case HOVER_CONNECT:
    if (!goto_pop_waypoint()) {
      set_hover_state(NULL, HOVER_NONE, ACTIVITY_LAST, ORDER_LAST);
      update_unit_info_label(get_units_in_focus());

      keyboardless_goto_button_down = FALSE;
      keyboardless_goto_active = FALSE;
      keyboardless_goto_start_tile = NULL;
    }
    break;
  default:
    break;
  };
}

/**************************************************************************
  Center the mapview on the player's capital, or print a failure message.
**************************************************************************/
void key_center_capital(void)
{
  struct city *capital = find_palace(client.conn.playing);

  if (capital)  {
    /* Center on the tile, and pop up the crosshair overlay. */
    center_tile_mapcanvas(capital->tile);
    put_cross_overlay_tile(capital->tile);
  } else {
    create_event(NULL, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                 _("Oh my! You seem to have no capital!"));
  }
}

/**************************************************************************
...
**************************************************************************/
void key_end_turn(void)
{
  send_turn_done();
}

/**************************************************************************
  Recall the previous focus unit(s).  See store_previous_focus().
**************************************************************************/
void key_recall_previous_focus_unit(void)
{
  int i = 0;

  /* Could use unit_list_copy here instead. Just having safe genlists
   * wouldn't be sufficient since we don't want to skip units already
   * removed from focus... */
  unit_list_iterate_safe(previous_focus, punit) {
    if (i == 0) {
      set_unit_focus(punit);
    } else {
      add_unit_focus(punit);
    }
    i++;
  } unit_list_iterate_safe_end;
}

/**************************************************************************
  Move the focus unit in the given direction.  Here directions are
  defined according to the GUI, so that north is "up" in the interface.
**************************************************************************/
void key_unit_move(enum direction8 gui_dir)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    enum direction8 map_dir = gui_to_map_dir(gui_dir);

    request_move_unit_direction(punit, map_dir);
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_build_city(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_build_city(punit);
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_build_wonder(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (unit_has_type_flag(punit, F_HELP_WONDER)) {
      request_unit_caravan_action(punit, PACKET_UNIT_HELP_BUILD_WONDER);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
handle user pressing key for 'Connect' command
**************************************************************************/
void key_unit_connect(enum unit_activity activity)
{
  request_unit_connect(activity);
}

/**************************************************************************
...
**************************************************************************/
void key_unit_diplomat_actions(void)
{
  struct city *pcity;		/* need pcity->id */
  unit_list_iterate(get_units_in_focus(), punit) {
    if (is_diplomat_unit(punit)
	&& (pcity = tile_city(punit->tile))
	&& diplomat_handled_in_diplomat_dialog() != -1    /* confusing otherwise? */
	&& diplomat_can_do_action(punit, DIPLOMAT_ANY_ACTION,
				  punit->tile)) {
      process_diplomat_arrival(punit, pcity->id);
      return;
      /* FIXME: diplomat dialog for more than one unit at a time. */
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_done(void)
{
  request_unit_move_done();
}

/**************************************************************************
...
**************************************************************************/
void key_unit_goto(void)
{
  request_unit_goto(ORDER_LAST);
}

/**************************************************************************
Explode nuclear at a tile without enemy units
**************************************************************************/
void key_unit_nuke(void)
{
  request_unit_nuke(get_units_in_focus());
}

/**************************************************************************
...
**************************************************************************/
void key_unit_paradrop(void)
{
  request_unit_paradrop(get_units_in_focus());
}

/**************************************************************************
...
**************************************************************************/
void key_unit_patrol(void)
{
  request_unit_patrol();
}

/**************************************************************************
...
**************************************************************************/
void key_unit_traderoute(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (unit_has_type_flag(punit, F_TRADE_ROUTE)) {
      request_unit_caravan_action(punit, PACKET_UNIT_ESTABLISH_TRADE);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_unload_all(void)
{
  struct unit *pnext_focus = NULL, *plast;

  unit_list_iterate(get_units_in_focus(), punit) {
    if ((plast = request_unit_unload_all(punit))) {
      pnext_focus = plast;
    }
  } unit_list_iterate_end;

  if (pnext_focus) {
    unit_list_iterate(get_units_in_focus(), punit) {
      /* Unfocus the ships, and advance the focus to the last unloaded unit.
       * If there is no unit unloaded (which shouldn't happen, but could if
       * the caller doesn't check if the transporter is loaded), the we
       * don't do anything. */
      punit->focus_status = FOCUS_WAIT;
    } unit_list_iterate_end;
    set_unit_focus(pnext_focus);
  }
}

/**************************************************************************
...
**************************************************************************/
void key_unit_wait(void)
{
  request_units_wait(get_units_in_focus());
}

/**************************************************************************
...
***************************************************************************/
void key_unit_wakeup_others(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_wakeup(punit);
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_airbase(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    struct base_type *pbase =
      get_base_by_gui_type(BASE_GUI_AIRBASE, punit, punit->tile);

    if (pbase) {
      request_new_unit_activity_base(punit, pbase);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_auto_explore(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_EXPLORE)) {
      request_new_unit_activity(punit, ACTIVITY_EXPLORE);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
  Call to request (from the server) that the focus unit is put into
  autosettler mode.
**************************************************************************/
void key_unit_auto_settle(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_autosettlers(punit)) {
      request_unit_autosettlers(punit);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_disband(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_disband(punit);
  } unit_list_iterate_end;
}

/**************************************************************************
  Unit transform key pressed or respective menu entry selected.
**************************************************************************/
void key_unit_transform_unit(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_transform(punit);
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_fallout(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_FALLOUT)) {
      request_new_unit_activity(punit, ACTIVITY_FALLOUT);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_fortify(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
      request_new_unit_activity(punit, ACTIVITY_FORTIFYING);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_fortress(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    struct base_type *pbase =
      get_base_by_gui_type(BASE_GUI_FORTRESS, punit, punit->tile);

    if (pbase) {
      request_new_unit_activity_base(punit, pbase);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_homecity(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_change_homecity(punit);
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_irrigate(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_IRRIGATE)) {
      request_new_unit_activity(punit, ACTIVITY_IRRIGATE);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_mine(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_MINE)) {
      request_new_unit_activity(punit, ACTIVITY_MINE);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_pillage(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_PILLAGE)) {
      request_unit_pillage(punit);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_pollution(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_POLLUTION)) {
      request_new_unit_activity(punit, ACTIVITY_POLLUTION);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_road(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_ROAD)) {
      request_new_unit_activity(punit, ACTIVITY_ROAD);
    } else if (can_unit_do_activity(punit, ACTIVITY_RAILROAD)) {
      request_new_unit_activity(punit, ACTIVITY_RAILROAD);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_sentry(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_SENTRY)) {
      request_new_unit_activity(punit, ACTIVITY_SENTRY);
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void key_unit_transform(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_TRANSFORM)) {
      request_new_unit_activity(punit, ACTIVITY_TRANSFORM);
    }
  } unit_list_iterate_end;
}

/****************************************************************************
  Assign all focus units to this battlegroup.
****************************************************************************/
void key_unit_assign_battlegroup(int battlegroup, bool append)
{
  if (NULL != client.conn.playing && can_client_issue_orders()
      && battlegroups >= 0 && battlegroup < MAX_NUM_BATTLEGROUPS) {
    if (!append) {
      unit_list_iterate_safe(battlegroups[battlegroup], punit) {
	if (!unit_is_in_focus(punit)) {
	  punit->battlegroup = BATTLEGROUP_NONE;
	  dsend_packet_unit_battlegroup(&client.conn,
					punit->id, BATTLEGROUP_NONE);
	  refresh_unit_mapcanvas(punit, punit->tile, TRUE, FALSE);
	  unit_list_unlink(battlegroups[battlegroup], punit);
	}
      } unit_list_iterate_safe_end;
    }
    unit_list_iterate(get_units_in_focus(), punit) {
      if (punit->battlegroup != battlegroup) {
	if (punit->battlegroup >= 0
	    && punit->battlegroup < MAX_NUM_BATTLEGROUPS) {
	  unit_list_unlink(battlegroups[punit->battlegroup], punit);
	}
	punit->battlegroup = battlegroup;
	dsend_packet_unit_battlegroup(&client.conn,
				      punit->id, battlegroup);
	unit_list_append(battlegroups[battlegroup], punit);
	refresh_unit_mapcanvas(punit, punit->tile, TRUE, FALSE);
      }
    } unit_list_iterate_end;
    unit_list_iterate(battlegroups[battlegroup], punit) {
      add_unit_focus(punit);
    } unit_list_iterate_end;
  }
}

/****************************************************************************
  Bring this battlegroup into focus.
****************************************************************************/
void key_unit_select_battlegroup(int battlegroup, bool append)
{
  if (NULL != client.conn.playing && can_client_change_view()
      && battlegroups >= 0 && battlegroup < MAX_NUM_BATTLEGROUPS) {
    int i = 0;

    if (unit_list_size(battlegroups[battlegroup]) == 0 && !append) {
      set_unit_focus(NULL);
      return;
    }

    /* FIXME: this is very inefficient and can be improved. */
    unit_list_iterate(battlegroups[battlegroup], punit) {
      if (i == 0 && !append) {
	set_unit_focus(punit);
      } else {
	add_unit_focus(punit);
      }
      i++;
    } unit_list_iterate_end;
  }
}

/**************************************************************************
  Toggle drawing of city outlines.
**************************************************************************/
void key_city_outlines_toggle(void)
{
  request_toggle_city_outlines();
}

/**************************************************************************
 Toggle drawing of city output produced by workers of the city.
**************************************************************************/
void key_city_output_toggle(void)
{
  request_toggle_city_output();
}

/**************************************************************************
...
**************************************************************************/
void key_map_grid_toggle(void)
{
  request_toggle_map_grid();
}

/**************************************************************************
  Toggle map borders on the mapview on/off based on a keypress.
**************************************************************************/
void key_map_borders_toggle(void)
{
  request_toggle_map_borders();
}

/**************************************************************************
...
**************************************************************************/
void key_city_names_toggle(void)
{
  request_toggle_city_names();
}

/**************************************************************************
  Toggles the "show city growth turns" option by passing off the
  request to another function...
**************************************************************************/
void key_city_growth_toggle(void)
{
  request_toggle_city_growth();
}

/**************************************************************************
  Toggles the showing of the buy cost of the current production in the
  city descriptions.
**************************************************************************/
void key_city_buycost_toggle(void)
{
  request_toggle_city_buycost();
}

/**************************************************************************
...
**************************************************************************/
void key_city_productions_toggle(void)
{
  request_toggle_city_productions();
}

/**************************************************************************
  Handle client request to toggle drawing of traderoute information
  by the city name for cities visible on the main map view.
**************************************************************************/
void key_city_traderoutes_toggle(void)
{
  request_toggle_city_traderoutes();
}

/**************************************************************************
...
**************************************************************************/
void key_terrain_toggle(void)
{
  request_toggle_terrain();
}

/**************************************************************************
...
**************************************************************************/
void key_coastline_toggle(void)
{
  request_toggle_coastline();
}

/**************************************************************************
...
**************************************************************************/
void key_roads_rails_toggle(void)
{
  request_toggle_roads_rails();
}

/**************************************************************************
...
**************************************************************************/
void key_irrigation_toggle(void)
{
  request_toggle_irrigation();
}

/**************************************************************************
...
**************************************************************************/
void key_mines_toggle(void)
{
  request_toggle_mines();
}

/**************************************************************************
...
**************************************************************************/
void key_fortress_airbase_toggle(void)
{
  request_toggle_fortress_airbase();
}

/**************************************************************************
...
**************************************************************************/
void key_specials_toggle(void)
{
  request_toggle_specials();
}

/**************************************************************************
...
**************************************************************************/
void key_pollution_toggle(void)
{
  request_toggle_pollution();
}

/**************************************************************************
...
**************************************************************************/
void key_cities_toggle(void)
{
  request_toggle_cities();
}

/**************************************************************************
...
**************************************************************************/
void key_units_toggle(void)
{
  request_toggle_units();
}

/**************************************************************************
...
**************************************************************************/
void key_focus_unit_toggle(void)
{
  request_toggle_focus_unit();
}

/**************************************************************************
...
**************************************************************************/
void key_fog_of_war_toggle(void)
{
  request_toggle_fog_of_war();
}

/**************************************************************************
  Toggle editor mode in the server.
**************************************************************************/
void key_editor_toggle(void)
{
  dsend_packet_edit_mode(&client.conn, !game.info.is_edit_mode);
}

/**************************************************************************
  Recalculate borders.
**************************************************************************/
void key_editor_recalculate_borders(void)
{
  send_packet_edit_recalculate_borders(&client.conn);
}

/**************************************************************************
  Send a request to the server to toggle fog-of-war for the current
  player (only applies in edit mode).
**************************************************************************/
void key_editor_toggle_fogofwar(void)
{
  if (client_has_player()) {
    dsend_packet_edit_toggle_fogofwar(&client.conn, client_player_number());
  }
}
