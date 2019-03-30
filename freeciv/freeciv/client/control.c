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
#include "astring.h"
#include "bitvector.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "timing.h"

/* common */
#include "combat.h"
#include "game.h"
#include "map.h"
#include "movement.h"
#include "unitlist.h"

/* common/aicore */
#include "path_finding.h"

/* client/include */
#include "chatline_g.h"
#include "citydlg_g.h"
#include "dialogs_g.h"
#include "gui_main_g.h"
#include "mapctrl_g.h"
#include "mapview_g.h"
#include "menu_g.h"

/* client */
#include "audio.h"
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "editor.h"
#include "goto.h"
#include "options.h"
#include "overview_common.h"
#include "tilespec.h"
#include "update_queue.h"

#include "control.h"


struct client_disband_unit_data {
  int unit_id;
  int alt;
};

/* Ways to disband a unit. Sorted by preference. Starts with the worst. */
/* TODO: Should other actions that consumes the unit be considered?
 * Join City may be an appealing alternative. Perhaps it should be a
 * user configurable client option? */
static int disband_unit_alternatives[3] = {
  ACTION_DISBAND_UNIT,
  ACTION_RECYCLE_UNIT,
  ACTION_HELP_WONDER,
};

/* gui-dep code may adjust depending on tile size etc: */
int num_units_below = MAX_NUM_UNITS_BELOW;

/* current_focus points to the current unit(s) in focus */
static struct unit_list *current_focus = NULL;

/* The previously focused unit(s).  Focus can generally be recalled
 * with keypad 5 (or the equivalent). */
static struct unit_list *previous_focus = NULL;

/* The priority unit(s) for unit_focus_advance(). */
static struct unit_list *urgent_focus_queue = NULL;

/* These should be set via set_hover_state() */
enum cursor_hover_state hover_state = HOVER_NONE;
enum unit_activity connect_activity;
struct extra_type *connect_tgt;

action_id goto_last_action;
int goto_last_sub_tgt;
enum unit_orders goto_last_order; /* Last order for goto */

static struct tile *hover_tile = NULL;
static struct unit_list *battlegroups[MAX_NUM_BATTLEGROUPS];

/* Current moving unit. */
static struct unit *punit_moving = NULL;

/* units involved in current combat */
static struct unit *punit_attacking = NULL;
static struct unit *punit_defending = NULL;

/* The ID of the unit that currently is in the action selection process.
 *
 * The action selection process begins when the client asks the server what
 * actions a unit can take. It ends when the last follow up question is
 * answered.
 *
 * No common client code using client supports more than one action
 * selection process at once. The interface between the common client code
 * and the clients would have to change before that could happen. (See
 * action_selection_actor_unit() etc)
 */
static int action_selection_in_progress_for = IDENTITY_NUMBER_ZERO;

/*
 * This variable is TRUE iff a NON-AI controlled unit was focused this
 * turn.
 */
bool non_ai_unit_focus;

static void key_unit_clean(enum unit_activity act, enum extra_rmcause rmcause);

/*************************************************************************/

static struct unit *quickselect(struct tile *ptile,
                                enum quickselect_type qtype);

/**********************************************************************//**
  Called only by client_game_init() in client/client_main.c
**************************************************************************/
void control_init(void)
{
  int i;

  current_focus = unit_list_new();
  previous_focus = unit_list_new();
  urgent_focus_queue = unit_list_new();

  for (i = 0; i < MAX_NUM_BATTLEGROUPS; i++) {
    battlegroups[i] = unit_list_new();
  }
  hover_tile = NULL;
}

/**********************************************************************//**
  Called only by client_game_free() in client/client_main.c
**************************************************************************/
void control_free(void)
{
  int i;

  unit_list_destroy(current_focus);
  current_focus = NULL;
  unit_list_destroy(previous_focus);
  previous_focus = NULL;
  unit_list_destroy(urgent_focus_queue);
  urgent_focus_queue = NULL;

  for (i = 0; i < MAX_NUM_BATTLEGROUPS; i++) {
    unit_list_destroy(battlegroups[i]);
    battlegroups[i] = NULL;
  }

  clear_hover_state();
  free_client_goto();
}

/**********************************************************************//**
  Returns list of units currently in focus.
**************************************************************************/
struct unit_list *get_units_in_focus(void)
{
  return current_focus;
}

/**********************************************************************//**
  Return the number of units currently in focus (0 or more).
**************************************************************************/
int get_num_units_in_focus(void)
{
  return (NULL != current_focus ? unit_list_size(current_focus) : 0);
}

/**********************************************************************//**
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

/**********************************************************************//**
  Store a priority focus unit.
**************************************************************************/
void unit_focus_urgent(struct unit *punit)
{
  unit_list_append(urgent_focus_queue, punit);
}

/**********************************************************************//**
  Do various updates required when the set of units in focus changes.
**************************************************************************/
static void focus_units_changed(void)
{
  update_unit_info_label(get_units_in_focus());
  menus_update();
  /* Notify the GUI */
  real_focus_units_changed();
}

/**********************************************************************//**
  Called when a unit is killed; this removes it from the control lists.
**************************************************************************/
void control_unit_killed(struct unit *punit)
{
  int i;

  goto_unit_killed(punit);

  unit_list_remove(get_units_in_focus(), punit);
  if (get_num_units_in_focus() < 1) {
    clear_hover_state();
  }

  unit_list_remove(previous_focus, punit);
  unit_list_remove(urgent_focus_queue, punit);

  for (i = 0; i < MAX_NUM_BATTLEGROUPS; i++) {
    unit_list_remove(battlegroups[i], punit);
  }

  focus_units_changed();
}

/**********************************************************************//**
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
      unit_list_remove(battlegroups[punit->battlegroup], punit);
    }
    punit->battlegroup = battlegroup;
  }
}

/**********************************************************************//**
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

/**********************************************************************//**
  Enter the given hover state.

    activity => The connect activity (ACTIVITY_IRRIGATE, etc.)
    order => The last order (ORDER_PERFORM_ACTION, ORDER_LAST, etc.)
**************************************************************************/
void set_hover_state(struct unit_list *punits, enum cursor_hover_state state,
		     enum unit_activity activity,
                     struct extra_type *tgt,
                     int last_sub_tgt,
                     action_id action,
                     enum unit_orders order)
{
  fc_assert_ret((punits && unit_list_size(punits) > 0)
                || state == HOVER_NONE);
  fc_assert_ret(state == HOVER_CONNECT || activity == ACTIVITY_LAST);
  fc_assert_ret(state == HOVER_GOTO || order == ORDER_LAST);
  fc_assert_ret(state == HOVER_GOTO || action == ACTION_NONE);
  exit_goto_state();
  hover_state = state;
  connect_activity = activity;
  if (tgt) {
    connect_tgt = tgt;
  } else {
    connect_tgt = NULL;
  }
  goto_last_order = order;
  goto_last_action = action;
  goto_last_sub_tgt = last_sub_tgt;
}

/**********************************************************************//**
  Clear current hover state (go to HOVER_NONE).
**************************************************************************/
void clear_hover_state(void)
{
  set_hover_state(NULL, HOVER_NONE,
                  ACTIVITY_LAST, NULL,
                  -1, ACTION_NONE, ORDER_LAST);
}

/**********************************************************************//**
  Returns TRUE iff the client should ask the server about what actions a
  unit can perform.
**************************************************************************/
bool should_ask_server_for_actions(const struct unit *punit)
{
  return (punit->action_decision_want == ACT_DEC_ACTIVE
          /* The player is interested in getting a pop up for a mere
           * arrival. */
          || (punit->action_decision_want == ACT_DEC_PASSIVE
              && gui_options.popup_actor_arrival));
}

/**********************************************************************//**
  Returns TRUE iff it is OK to ask the server about what actions a unit
  can perform.
**************************************************************************/
static bool can_ask_server_for_actions(void)
{
  /* OK as long as no other unit already asked and aren't done yet. */
  return (action_selection_in_progress_for == IDENTITY_NUMBER_ZERO
          && action_selection_actor_unit() == IDENTITY_NUMBER_ZERO);
}

/**********************************************************************//**
  Ask the server about what actions punit may be able to perform against
  it's stored target tile.

  The server's reply will pop up the action selection dialog unless no
  alternatives exists.
**************************************************************************/
static void ask_server_for_actions(struct unit *punit)
{
  fc_assert_ret(punit);
  fc_assert_ret(punit->action_decision_tile);

  /* Only one action selection dialog at a time is supported. */
  fc_assert_msg(action_selection_in_progress_for == IDENTITY_NUMBER_ZERO,
                "Unit %d started action selection before unit %d was done",
                action_selection_in_progress_for, punit->id);
  action_selection_in_progress_for = punit->id;

  dsend_packet_unit_get_actions(&client.conn,
                                punit->id,
                                IDENTITY_NUMBER_ZERO,
                                tile_index(punit->action_decision_tile),
                                EXTRA_NONE,
                                TRUE);
}

/**********************************************************************//**
  Return TRUE iff this unit is in focus.
**************************************************************************/
bool unit_is_in_focus(const struct unit *punit)
{
  return unit_list_search(get_units_in_focus(), punit) != NULL;
}

/**********************************************************************//**
  Return TRUE iff a unit on this tile is in focus.
**************************************************************************/
struct unit *get_focus_unit_on_tile(const struct tile *ptile)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (unit_tile(punit) == ptile) {
      return punit;
    }
  } unit_list_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Return head of focus units list.
**************************************************************************/
struct unit *head_of_units_in_focus(void)
{
  return unit_list_get(current_focus, 0);
}

/**********************************************************************//**
  Finds a single focus unit that we can center on.  May return NULL.
**************************************************************************/
static struct tile *find_a_focus_unit_tile_to_center_on(void)
{
  struct unit *punit;

  if (NULL != (punit = get_focus_unit_on_tile(get_center_tile_mapcanvas()))) {
    return unit_tile(punit);
  } else if (get_num_units_in_focus() > 0) {
    return unit_tile(head_of_units_in_focus());
  } else {
    return NULL;
  }
}

/**********************************************************************//**
  Center on the focus unit, if off-screen and auto_center_on_unit is true.
**************************************************************************/
void auto_center_on_focus_unit(void)
{
  struct tile *ptile = find_a_focus_unit_tile_to_center_on();

  if (ptile && gui_options.auto_center_on_unit &&
      !tile_visible_and_not_on_border_mapcanvas(ptile)) {
    center_tile_mapcanvas(ptile);
  }
}

/**********************************************************************//**
  Add unit to list of units currently in focus.
**************************************************************************/
static void current_focus_append(struct unit *punit)
{
  unit_list_append(current_focus, punit);

  punit->client.focus_status = FOCUS_AVAIL;
  refresh_unit_mapcanvas(punit, unit_tile(punit), TRUE, FALSE);

  if (should_ask_server_for_actions(punit)
      && can_ask_server_for_actions()) {
    ask_server_for_actions(punit);
  }

  if (gui_options.unit_selection_clears_orders) {
    clear_unit_orders(punit);
  }
}

/**********************************************************************//**
  Remove focus from unit.
**************************************************************************/
static void current_focus_remove(struct unit *punit)
{
  /* Close the action selection dialog if the actor unit lose focus. */
  if (action_selection_actor_unit() == punit->id) {
    action_selection_close();
  }

  unit_list_remove(current_focus, punit);
  refresh_unit_mapcanvas(punit, unit_tile(punit), TRUE, FALSE);
}

/**********************************************************************//**
  Clear all orders for the given unit.
**************************************************************************/
void clear_unit_orders(struct unit *punit)
{
  if (!punit) {
    return;
  }

  if (punit->activity != ACTIVITY_IDLE
      || punit->ai_controlled)  {
    punit->ai_controlled = FALSE;
    refresh_unit_city_dialogs(punit);
    request_new_unit_activity(punit, ACTIVITY_IDLE);
  } else if (unit_has_orders(punit)) {
    /* Clear the focus unit's orders. */
    request_orders_cleared(punit);
  }
}

/**********************************************************************//**
  Sets the focus unit directly.  The unit given will be given the
  focus; if NULL the focus will be cleared.

  This function is called for several reasons.  Sometimes a fast-focus
  happens immediately as a result of a client action.  Other times it
  happens because of a server-sent packet that wakes up a unit.
**************************************************************************/
void unit_focus_set(struct unit *punit)
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

  /* Close the action selection dialog if the actor unit lose focus. */
  unit_list_iterate(current_focus, punit_old) {
    if (action_selection_actor_unit() == punit_old->id) {
      action_selection_close();
    }
  } unit_list_iterate_end;

  /* Redraw the old focus unit (to fix blinking or remove the selection
   * circle). */
  unit_list_iterate(current_focus, punit_old) {
    refresh_unit_mapcanvas(punit_old, unit_tile(punit_old), TRUE, FALSE);
  } unit_list_iterate_end;
  unit_list_clear(current_focus);

  if (!can_client_change_view()) {
    /* This function can be called to set the focus to NULL when
     * disconnecting.  In this case we don't want any other actions! */
    fc_assert(punit == NULL);
    return;
  }

  if (NULL != punit) {
    current_focus_append(punit);
    auto_center_on_focus_unit();
  }

  if (focus_changed) {
    clear_hover_state();
    focus_units_changed();
  }
}

/**********************************************************************//**
  Adds this unit to the list of units in focus.
**************************************************************************/
void unit_focus_add(struct unit *punit)
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
    clear_hover_state();
  }

  current_focus_append(punit);
  focus_units_changed();
}

/**********************************************************************//**
  Removes this unit from the list of units in focus.
**************************************************************************/
void unit_focus_remove(struct unit *punit)
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

  if (!unit_is_in_focus(punit)) {
    return;
  }

  if (hover_state != HOVER_NONE) {
    /* Can't continue with current goto if set of focus units
     * change. Cancel it. */
    clear_hover_state();
  }

  current_focus_remove(punit);
  if (get_num_units_in_focus() > 0) {
    focus_units_changed();
  } else {
    unit_focus_advance();
  }
}

/**********************************************************************//**
  The only difference is that here we draw the "cross".
**************************************************************************/
void unit_focus_set_and_select(struct unit *punit)
{
  unit_focus_set(punit);
  if (punit) {
    put_cross_overlay_tile(unit_tile(punit));
  }
}

/**********************************************************************//**
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
      ptile = unit_tile(pfirst);
    }
  }

  iterate_outward(&(wld.map), ptile, FC_INFINITY, ptile2) {
    unit_list_iterate(ptile2->units, punit) {
      if ((!unit_is_in_focus(punit) || accept_current)
          && unit_owner(punit) == client.conn.playing
          && punit->client.focus_status == FOCUS_AVAIL
          && punit->activity == ACTIVITY_IDLE
          && !unit_has_orders(punit)
          && (punit->moves_left > 0 || unit_type_get(punit)->move_rate == 0)
          && !punit->done_moving
          && !punit->ai_controlled) {
        return punit;
      }
    } unit_list_iterate_end;
  } iterate_outward_end;

  return NULL;
}

/**********************************************************************//**
  This function may be called from packhand.c, via unit_focus_update(),
  as a result of packets indicating change in activity for a unit. Also
  called when user press the "Wait" command.
 
  FIXME: Add feature to focus only units of a certain category.
**************************************************************************/
void unit_focus_advance(void)
{
  struct unit *candidate = NULL;
  const int num_units_in_old_focus = get_num_units_in_focus();

  if (NULL == client.conn.playing
      || !is_player_phase(client.conn.playing, game.info.phase)
      || !can_client_change_view()) {
    unit_focus_set(NULL);
    return;
  }

  clear_hover_state();

  unit_list_iterate(get_units_in_focus(), punit) {
    /* 
     * Is the unit which just lost focus a non-AI unit? If yes this
     * enables the auto end turn. 
     */
    if (!punit->ai_controlled) {
      non_ai_unit_focus = TRUE;
      break;
    }
  } unit_list_iterate_end;

  if (unit_list_size(urgent_focus_queue) > 0) {
    /* Try top of the urgent list. */
    struct tile *focus_tile = (get_num_units_in_focus() > 0
                               ? unit_tile(head_of_units_in_focus())
                               : NULL);

    unit_list_both_iterate(urgent_focus_queue, plink, punit) {
      if ((ACTIVITY_IDLE != punit->activity
           || unit_has_orders(punit))
          /* This isn't an action decision needed because of an
           * ORDER_ACTION_MOVE located in the middle of an order. */
          && !should_ask_server_for_actions(punit)) {
        /* We have assigned new orders to this unit since, remove it. */
        unit_list_erase(urgent_focus_queue, plink);
      } else if (NULL == focus_tile
                 || focus_tile == unit_tile(punit)) {
        /* Use the first one found */
        candidate = punit;
        break;
      } else if (NULL == candidate) {
        candidate = punit;
      }
    } unit_list_both_iterate_end;

    if (NULL != candidate) {
      unit_list_remove(urgent_focus_queue, candidate);

      /* Autocenter on Wakeup, regardless of the local option
       * "auto_center_on_unit". */
      if (!tile_visible_and_not_on_border_mapcanvas(unit_tile(candidate))) {
        center_tile_mapcanvas(unit_tile(candidate));
      }
    }
  }

  if (NULL == candidate) {
    candidate = find_best_focus_candidate(FALSE);

    if (!candidate) {
      /* Try for "waiting" units. */
      unit_list_iterate(client.conn.playing->units, punit) {
        if (punit->client.focus_status == FOCUS_WAIT) {
          punit->client.focus_status = FOCUS_AVAIL;
        }
      } unit_list_iterate_end;
      candidate = find_best_focus_candidate(FALSE);

      if (!candidate) {
        /* Accept current focus unit as last resort. */
        candidate = find_best_focus_candidate(TRUE);
      }
    }
  }

  unit_focus_set(candidate);

  /* 
   * Handle auto-turn-done mode: If a unit was in focus (did move),
   * but now none are (no more to move) and there was at least one
   * non-AI unit this turn which was focused, then fake a Turn Done
   * keypress.
   */
  if (gui_options.auto_turn_done
      && num_units_in_old_focus > 0
      && get_num_units_in_focus() == 0
      && non_ai_unit_focus) {
    key_end_turn();
  }
}

/**********************************************************************//**
  If there is no unit currently in focus, or if the current unit in
  focus should not be in focus, then get a new focus unit.
  We let GOTO-ing units stay in focus, so that if they have moves left
  at the end of the goto, then they are still in focus.
**************************************************************************/
void unit_focus_update(void)
{
  if (NULL == client.conn.playing || !can_client_change_view()) {
    return;
  }

  if (!can_ask_server_for_actions()) {
    fc_assert(get_num_units_in_focus() > 0);

    /* An actor unit is asking the player what to do. Don't steal his
     * focus. */
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
	&& !punit->ai_controlled) {
      return;
    }
  } unit_list_iterate_end;

  unit_focus_advance();
}

/**********************************************************************//**
  Return a pointer to a visible unit, if there is one.
**************************************************************************/
struct unit *find_visible_unit(struct tile *ptile)
{
  struct unit *panyowned = NULL, *panyother = NULL, *ptptother = NULL;

  /* If no units here, return nothing. */
  if (unit_list_size(ptile->units) == 0) {
    return NULL;
  }

  /* If a unit is attacking we should show that on top */
  if (punit_attacking && same_pos(unit_tile(punit_attacking), ptile)) {
    unit_list_iterate(ptile->units, punit) {
      if (punit == punit_attacking) {
        return punit;
      }
    } unit_list_iterate_end;
  }

  /* If a unit is defending we should show that on top */
  if (punit_defending && same_pos(unit_tile(punit_defending), ptile)) {
    unit_list_iterate(ptile->units, punit) {
      if (punit == punit_defending) {
        return punit;
      }
    } unit_list_iterate_end;
  }

  /* If the unit in focus is at this tile, show that on top */
  unit_list_iterate(get_units_in_focus(), punit) {
    if (punit != punit_moving && unit_tile(punit) == ptile) {
      return punit;
    }
  } unit_list_iterate_end;

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
      if (!unit_transported(punit)) {
        if (get_transporter_capacity(punit) > 0) {
	  return punit;
        } else if (!panyowned) {
	  panyowned = punit;
        }
      }
    } else if (!ptptother && !unit_transported(punit)) {
      if (get_transporter_capacity(punit) > 0) {
	ptptother = punit;
      } else if (!panyother) {
	panyother = punit;
      }
    }
  unit_list_iterate_end;

  return (panyowned ? panyowned : (ptptother ? ptptother : panyother));
}

/**********************************************************************//**
  Blink the active unit (if necessary).  Return the time until the next
  blink (in seconds).
**************************************************************************/
double blink_active_unit(void)
{
  static struct timer *blink_timer = NULL;
  const double blink_time = get_focus_unit_toggle_timeout(tileset);

  if (get_num_units_in_focus() > 0) {
    if (!blink_timer || timer_read_seconds(blink_timer) > blink_time) {
      toggle_focus_unit_state(tileset);

      /* If we lag, we don't try to catch up.  Instead we just start a
       * new blink_time on every update. */
      blink_timer = timer_renew(blink_timer, TIMER_USER, TIMER_ACTIVE);
      timer_start(blink_timer);

      unit_list_iterate(get_units_in_focus(), punit) {
	/* We flush to screen directly here.  This is most likely faster
	 * since these drawing operations are all small but may be spread
	 * out widely. */
	refresh_unit_mapcanvas(punit, unit_tile(punit), FALSE, TRUE);
      } unit_list_iterate_end;
    }

    return blink_time - timer_read_seconds(blink_timer);
  }

  return blink_time;
}

/**********************************************************************//**
  Blink the turn done button (if necessary).  Return the time until the next
  blink (in seconds).
**************************************************************************/
double blink_turn_done_button(void)
{
  static struct timer *blink_timer = NULL;
  const double blink_time = 0.5; /* half-second blink interval */

  if (NULL != client.conn.playing
      && client.conn.playing->is_alive
      && !client.conn.playing->phase_done
      && is_player_phase(client.conn.playing, game.info.phase)) {
    if (!blink_timer || timer_read_seconds(blink_timer) > blink_time) {
      int is_waiting = 0, is_moving = 0;
      bool blocking_mode;
      struct option *opt;

      opt = optset_option_by_name(server_optset, "turnblock");
      if (opt != NULL) {
        blocking_mode = option_bool_get(opt);
      } else {
        blocking_mode = FALSE;
      }

      players_iterate_alive(pplayer) {
        if ((pplayer->is_connected || blocking_mode)
            && is_player_phase(pplayer, game.info.phase)) {
          if (pplayer->phase_done) {
            is_waiting++;
          } else {
            is_moving++;
          }
        }
      } players_iterate_alive_end;

      if (is_moving == 1 && is_waiting > 0) {
	update_turn_done_button(FALSE);	/* stress the slow player! */
      }
      blink_timer = timer_renew(blink_timer, TIMER_USER, TIMER_ACTIVE);
      timer_start(blink_timer);
    }
    return blink_time - timer_read_seconds(blink_timer);
  }

  return blink_time;
}

/**********************************************************************//**
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
    unit_list_iterate(unit_tile(punit)->units, aunit) {
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
      for (; i < num_units_below; i++) {
	set_unit_icon(i, NULL);
      }
    }
  } else {
    for (i = -1; i < num_units_below; i++) {
      set_unit_icon(i, NULL);
    }
    set_unit_icons_more_arrow(FALSE);
  }
}

/**********************************************************************//**
  Adjusts way combatants are displayed suitable for combat.
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

/**********************************************************************//**
  The action selection process is no longer in progres for the specified
  unit. It is safe to let another unit enter it.
**************************************************************************/
void action_selection_no_longer_in_progress(const int old_actor_id)
{
  /* IDENTITY_NUMBER_ZERO is accepted for cases where the unit is gone
   * without a trace. */
  fc_assert_msg(old_actor_id == action_selection_in_progress_for
                || old_actor_id == IDENTITY_NUMBER_ZERO,
                "Decision taken for %d but selection is for %d.",
                old_actor_id, action_selection_in_progress_for);

  /* Stop objecting to allowing the next unit to ask. */
  action_selection_in_progress_for = IDENTITY_NUMBER_ZERO;

  /* Clean up any client specific assumptions. */
  action_selection_no_longer_in_progress_gui_specific(old_actor_id);
}

/**********************************************************************//**
  Have the server record that a decision no longer is wanted for the
  specified unit.
**************************************************************************/
void action_decision_clear_want(const int old_actor_id)
{
  struct unit *old;

  if ((old = game_unit_by_number(old_actor_id))) {
    /* Have the server record that a decision no longer is wanted. */
    dsend_packet_unit_sscs_set(&client.conn, old_actor_id,
                               USSDT_UNQUEUE, IDENTITY_NUMBER_ZERO);
  }
}

/**********************************************************************//**
  Move on to the next unit in focus that needs an action decision.
**************************************************************************/
void action_selection_next_in_focus(const int old_actor_id)
{
  struct unit *old;

  old = game_unit_by_number(old_actor_id);

  /* Go to the next unit in focus that needs a decision. */
  unit_list_iterate(get_units_in_focus(), funit) {
    if (old != funit && should_ask_server_for_actions(funit)) {
      ask_server_for_actions(funit);
      return;
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Request that the player makes a decision for the specified unit.
**************************************************************************/
void action_decision_request(struct unit *actor_unit)
{
  fc_assert_ret(actor_unit);
  fc_assert_ret(actor_unit->action_decision_tile);

  if (!unit_is_in_focus(actor_unit)) {
    /* Getting feed back may be urgent. A unit standing next to an enemy
     * could be killed while waiting. */
    unit_focus_urgent(actor_unit);
  } else if (can_client_issue_orders()
             && can_ask_server_for_actions()) {
    /* No need to wait. The actor unit is in focus. No other actor unit
     * is currently asking about action selection. */
    ask_server_for_actions(actor_unit);
  }
}

/**********************************************************************//**
  Do a goto with an order at the end (or ORDER_LAST).
**************************************************************************/
void request_unit_goto(enum unit_orders last_order,
                       action_id act_id, int sub_tgt_id)
{
  struct unit_list *punits = get_units_in_focus();

  fc_assert_ret(act_id == ACTION_NONE
                || last_order == ORDER_PERFORM_ACTION);

  if (unit_list_size(punits) == 0) {
    return;
  }

  if (last_order == ORDER_PERFORM_ACTION) {
    /* An action has been specified. */
    fc_assert_ret(action_id_exists(act_id));

    /* The order system doesn't support actions that can be done to a
     * target that isn't at or next to the actor unit's tile.
     *
     * Full explanation in handle_unit_orders(). */
    fc_assert_ret(!action_id_distance_inside_max(act_id, 2));

    unit_list_iterate(punits, punit) {
      if (!unit_can_do_action(punit, act_id)) {
        /* This unit can't perform the action specified in the last
         * order. */

        struct astring astr = ASTRING_INIT;

        if (role_units_translations(&astr,
                                    action_id_get_role(act_id),
                                    TRUE)) {
          /* ...but other units can perform it. */

          create_event(unit_tile(punit), E_BAD_COMMAND, ftc_client,
                        /* TRANS: Only Nuclear or ICBM can do Explode
                         * Nuclear. */
                       _("Only %s can do %s."),
                       astr_str(&astr),
                       action_id_name_translation(act_id));

          astr_free(&astr);
        } else {
          create_event(unit_tile(punit), E_BAD_COMMAND, ftc_client,
                       /* TRANS: Spy can't do Explode Nuclear. */
                       _("%s can't do %s."),
                       unit_name_translation(punit),
                       action_id_name_translation(act_id));
        }

        return;
      }
    } unit_list_iterate_end;
  }

  if (hover_state != HOVER_GOTO) {
    set_hover_state(punits, HOVER_GOTO, ACTIVITY_LAST, NULL,
                    sub_tgt_id, act_id, last_order);
    enter_goto_state(punits);
    create_line_at_mouse_pos();
    update_unit_info_label(punits);
    control_mouse_cursor(NULL);
  } else {
    fc_assert_ret(goto_is_active());
    goto_add_waypoint();
  }
}

/**********************************************************************//**
  Return TRUE if at least one of the units can do an attack at the tile.
**************************************************************************/
static bool can_units_attack_at(struct unit_list *punits,
				const struct tile *ptile)
{
  unit_list_iterate(punits, punit) {
    if (is_attack_unit(punit)
        && can_unit_attack_tile(punit, ptile)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/**********************************************************************//**
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

  if (!gui_options.enable_cursor_changes) {
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
    /* Determine if the goto is valid, invalid, nuke or will attack. */
    if (is_valid_goto_destination(ptile)) {
      if (goto_last_action == ACTION_NUKE) {
        /* Goto results in nuclear attack. */
        mouse_cursor_type = CURSOR_NUKE;
      } else if (can_units_attack_at(active_units, ptile)) {
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
  case HOVER_PARADROP:
    /* FIXME: check for invalid tiles. */
    mouse_cursor_type = CURSOR_PARADROP;
    break;
  case HOVER_ACT_SEL_TGT:
    /* Select a tile to target / find targets on. */
    mouse_cursor_type = CURSOR_SELECT;
    break;
  };

  update_mouse_cursor(mouse_cursor_type);
}

/**********************************************************************//**
  Return TRUE if there are any units doing the activity on the tile.
**************************************************************************/
static bool is_activity_on_tile(struct tile *ptile,
				enum unit_activity activity)
{
  unit_list_iterate(ptile->units, punit) {
    if (punit->activity == activity) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/**********************************************************************//**
  Fill orders to build recursive roads. This modifies ptile, so virtual
  copy of the real tile should be passed.
**************************************************************************/
int check_recursive_road_connect(struct tile *ptile, const struct extra_type *pextra,
                                 const struct unit *punit, const struct player *pplayer, int rec)
{
  int activity_mc = 0;
  struct terrain *pterrain = tile_terrain(ptile);

  if (rec > MAX_EXTRA_TYPES) {
    return -1;
  }

  if (!is_extra_caused_by(pextra, EC_ROAD)) {
    return -1;
  }

  extra_deps_iterate(&(pextra->reqs), pdep) {
    if (!tile_has_extra(ptile, pdep)) {
      int single_mc;

      single_mc = check_recursive_road_connect(ptile, pdep, punit, pplayer, rec + 1);

      if (single_mc < 0) {
        return -1;
      }

      activity_mc += single_mc;
    }
  } extra_deps_iterate_end;

  /* Can build road after that? */
  if (punit != NULL) {
    if (!can_build_road(extra_road_get(pextra), punit, ptile)) {
      return -1;
    }
  } else if (pplayer != NULL) {
    if (!player_can_build_road(extra_road_get(pextra), pplayer, ptile)) {
      return -1;
    }
  }

  tile_add_extra(ptile, pextra);

  activity_mc += terrain_extra_build_time(pterrain, ACTIVITY_GEN_ROAD, pextra);

  return activity_mc;
}

/*******************************************************************//**
  Can tile be irrigated by given unit? Unit can be NULL to check if
  any settler type unit of any player can irrigate.
***********************************************************************/
static bool can_be_irrigated(const struct tile *ptile,
                             const struct unit *punit)
{
  struct terrain* pterrain = tile_terrain(ptile);
  struct universal for_unit = { .kind = VUT_UTYPE,
                                .value.utype = unit_type_get(punit)};
  struct universal for_tile = { .kind = VUT_TERRAIN,
                                .value.terrain = tile_terrain(ptile)};

  if (T_UNKNOWN == pterrain) {
    return FALSE;
  }

  return univs_have_action_enabler(ACTION_IRRIGATE, &for_unit, &for_tile);
}

/**********************************************************************//**
  Return whether the unit can connect with given activity (or with
  any activity if activity arg is set to ACTIVITY_IDLE)

  This function is client-specific.
**************************************************************************/
bool can_unit_do_connect(struct unit *punit,
                         enum unit_activity activity,
                         struct extra_type *tgt) 
{
  struct tile *ptile = unit_tile(punit);
  struct terrain *pterrain = tile_terrain(ptile);
  struct road_type *proad = NULL;

  /* HACK: This code duplicates that in
   * can_unit_do_activity_targeted_at(). The general logic here is that
   * the connect is allowed if both:
   * (1) the unit can do that activity type, in general
   * (2) either
   *     (a) the activity has already been completed at this tile
   *     (b) it can be done by the unit at this tile. */
  switch (activity) {
  case ACTIVITY_GEN_ROAD:
    {
      struct tile *vtile;
      int build_time;

      fc_assert(is_extra_caused_by(tgt, EC_ROAD));

      proad = extra_road_get(tgt);

      if (tile_has_road(ptile, proad)) {
        /* This tile has road, can unit build road to other tiles too? */
        return are_reqs_active(NULL, NULL, NULL, NULL, NULL,
                               punit, unit_type_get(punit), NULL, NULL, NULL,
                               &tgt->reqs, RPT_POSSIBLE);
      }

      /* To start connect, unit must be able to build road to this
       * particular tile. */
      vtile = tile_virtual_new(ptile);
      build_time = check_recursive_road_connect(vtile, tgt, punit, NULL, 0);
      tile_virtual_destroy(vtile);

      return build_time >= 0;
    }

  case ACTIVITY_IRRIGATE:
    /* Special case for irrigation: only irrigate to make S_IRRIGATION,
     * never to transform tiles. */
    if (!unit_has_type_flag(punit, UTYF_SETTLERS)) {
      return FALSE;
    }
    if (tile_has_extra(ptile, tgt)) {
      return are_reqs_active(NULL, NULL, NULL, NULL, NULL,
                             punit, unit_type_get(punit), NULL, NULL, NULL,
                             &tgt->reqs, RPT_POSSIBLE);
    }

    return pterrain == pterrain->irrigation_result
      && can_be_irrigated(ptile, punit)
      && can_build_extra(tgt, punit, ptile)
      && !is_activity_on_tile(ptile,
                              ACTIVITY_MINE);
  default:
    break;
  }

  return FALSE;
}

/**********************************************************************//**
  Prompt player for entering destination point for unit connect
  (e.g. connecting with roads)
**************************************************************************/
void request_unit_connect(enum unit_activity activity,
                          struct extra_type *tgt)
{
  struct unit_list *punits = get_units_in_focus();

  if (!can_units_do_connect(punits, activity, tgt)) {
    return;
  }

  if (hover_state != HOVER_CONNECT || connect_activity != activity
      || (connect_tgt != tgt
          && (activity == ACTIVITY_GEN_ROAD
              || activity == ACTIVITY_IRRIGATE))) {
    set_hover_state(punits, HOVER_CONNECT,
                    activity, tgt, -1, ACTION_NONE, ORDER_LAST);
    enter_goto_state(punits);
    create_line_at_mouse_pos();
    update_unit_info_label(punits);
    control_mouse_cursor(NULL);
  } else {
    fc_assert_ret(goto_is_active());
    goto_add_waypoint();
  }
}

/**********************************************************************//**
  Returns one of the unit of the transporter which can have focus next.
**************************************************************************/
struct unit *request_unit_unload_all(struct unit *punit)
{
  struct tile *ptile = unit_tile(punit);
  struct unit *plast = NULL;

  if (get_transporter_capacity(punit) == 0) {
    create_event(unit_tile(punit), E_BAD_COMMAND, ftc_client,
                 _("Only transporter units can be unloaded."));
    return NULL;
  }

  unit_list_iterate(ptile->units, pcargo) {
    if (unit_transport_get(pcargo) == punit) {
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

/**********************************************************************//**
  Send unit airlift request to server.
**************************************************************************/
void request_unit_airlift(struct unit *punit, struct city *pcity)
{
  request_do_action(ACTION_AIRLIFT, punit->id, pcity->id, 0, "");
}

/**********************************************************************//**
  Return-and-recover for a particular unit.  This sets the unit to GOTO
  the nearest city.
**************************************************************************/
void request_unit_return(struct unit *punit)
{
  struct pf_path *path;

  if ((path = path_to_nearest_allied_city(punit))) {
    int turns = pf_path_last_position(path)->turn;
    int max_hp = unit_type_get(punit)->hp;

    if (punit->hp + turns *
        (get_unit_bonus(punit, EFT_UNIT_RECOVER)
         - (max_hp * unit_class_get(punit)->hp_loss_pct / 100))
	< max_hp) {
      struct unit_order order;

      order.order = ORDER_ACTIVITY;
      order.dir = DIR8_ORIGIN;
      order.activity = ACTIVITY_SENTRY;
      order.sub_target = -1;
      order.action = ACTION_NONE;
      send_goto_path(punit, path, &order);
    } else {
      send_goto_path(punit, path, NULL);
    }
    pf_path_destroy(path);
  }
}

/**********************************************************************//**
  Wakes all owned sentried units on tile.
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

/**********************************************************************//**
  (RP:) un-sentry all my own sentried units on punit's tile
**************************************************************************/
void request_unit_wakeup(struct unit *punit)
{
  wakeup_sentried_units(unit_tile(punit));
}

/**************************************************************************
  Defines specific hash tables needed for request_unit_select().
**************************************************************************/
#define SPECHASH_TAG unit_type
#define SPECHASH_IKEY_TYPE struct unit_type *
#define SPECHASH_IDATA_TYPE void *
#include "spechash.h"

#define SPECHASH_TAG continent
#define SPECHASH_INT_KEY_TYPE
#define SPECHASH_IDATA_TYPE void *
#include "spechash.h"

/**********************************************************************//**
  Select all units based on the given list of units and the selection modes.
**************************************************************************/
void request_unit_select(struct unit_list *punits,
                         enum unit_select_type_mode seltype,
                         enum unit_select_location_mode selloc)
{
  const struct player *pplayer;
  const struct tile *ptile;
  struct unit *punit_first;
  struct tile_hash *tile_table;
  struct unit_type_hash *type_table;
  struct continent_hash *cont_table;

  if (!can_client_change_view() || !punits
      || unit_list_size(punits) < 1) {
    return;
  }

  punit_first = unit_list_get(punits, 0);

  if (seltype == SELTYPE_SINGLE) {
    unit_focus_set(punit_first);
    return;
  }

  pplayer = unit_owner(punit_first);
  tile_table = tile_hash_new();
  type_table = unit_type_hash_new();
  cont_table = continent_hash_new();

  unit_list_iterate(punits, punit) {
    if (seltype == SELTYPE_SAME) {
      unit_type_hash_insert(type_table, unit_type_get(punit), NULL);
    }

    ptile = unit_tile(punit);
    if (selloc == SELLOC_TILE) {
      tile_hash_insert(tile_table, ptile, NULL);
    } else if (selloc == SELLOC_CONT) {
      continent_hash_insert(cont_table, tile_continent(ptile), NULL);
    }
  } unit_list_iterate_end;

  if (selloc == SELLOC_TILE) {
    tile_hash_iterate(tile_table, hash_tile) {
      unit_list_iterate(hash_tile->units, punit) {
        if (unit_owner(punit) != pplayer) {
          continue;
        }
        if (seltype == SELTYPE_SAME
            && !unit_type_hash_lookup(type_table, unit_type_get(punit), NULL)) {
          continue;
        }
        unit_focus_add(punit);
      } unit_list_iterate_end;
    } tile_hash_iterate_end;
  } else {
    unit_list_iterate(pplayer->units, punit) {
      ptile = unit_tile(punit);
      if ((seltype == SELTYPE_SAME
           && !unit_type_hash_lookup(type_table, unit_type_get(punit), NULL))
          || (selloc == SELLOC_CONT
              && !continent_hash_lookup(cont_table, tile_continent(ptile),
                                        NULL))) {
        continue;
      }

      unit_focus_add(punit);
    } unit_list_iterate_end;
  }

  tile_hash_destroy(tile_table);
  unit_type_hash_destroy(type_table);
  continent_hash_destroy(cont_table);
}

/**********************************************************************//**
  Request an actor unit to do a specific action.
  - action    : The action to be requested.
  - actor_id  : The unit ID of the actor unit.
  - target_id : The ID of the target unit, city or tile.
  - sub_tgt   : The sub target. Only some actions take a sub target. The
                sub target kind depends on the action. Example sub targets
                are the technology to steal from a city, the extra to
                pillage at a tile and the building to sabotage in a city.
  - name      : Used by ACTION_FOUND_CITY to specify city name.
**************************************************************************/
void request_do_action(action_id action, int actor_id,
                       int target_id, int sub_tgt, const char *name)
{
  dsend_packet_unit_do_action(&client.conn,
                              actor_id, target_id, sub_tgt, name,
                              action);
}

/**********************************************************************//**
  Request data for follow up questions about an action the unit can
  perform.
  - action : The action more details .
  - actor_id : The unit ID of the acting unit.
  - target_id : The ID of the target unit or city.
**************************************************************************/
void request_action_details(action_id action, int actor_id,
                            int target_id)
{
  dsend_packet_unit_action_query(&client.conn,
                                 actor_id, target_id, action,
                                 /* Users that need the answer in the
                                  * background should send the packet them
                                  * self. At least for now. */
                                 TRUE);
}

/**********************************************************************//**
  Player pressed 'b' or otherwise instructed unit to build or add to city.
  If the unit can build a city, we popup the appropriate dialog.
  Otherwise, we just send a packet to the server.
  If this action is not appropriate, the server will respond
  with an appropriate message.  (This is to avoid duplicating
  all the server checks and messages here.)
**************************************************************************/
void request_unit_build_city(struct unit *punit)
{
  struct city *pcity;

  if ((pcity = tile_city(unit_tile(punit)))) {
    /* Try to join the city. */
    request_do_action(ACTION_JOIN_CITY, punit->id, pcity->id, 0, "");
  } else {
    /* The reply will trigger a dialog to name the new city. */
    dsend_packet_city_name_suggestion_req(&client.conn, punit->id);
  }
}

/**********************************************************************//**
  Order a unit to move to a neighboring tile without performing an action.

  Does nothing it the destination tile isn't next to the tile where the
  unit currently is located.
**************************************************************************/
void request_unit_non_action_move(struct unit *punit,
                                  struct tile *dest_tile)
{
  struct packet_unit_orders p;
  int dir;

  dir = get_direction_for_step(&(wld.map), unit_tile(punit), dest_tile);

  if (dir == -1) {
    /* The unit isn't located next to the destination tile. */
    return;
  }

  memset(&p, 0, sizeof(p));

  p.repeat = FALSE;
  p.vigilant = FALSE;

  p.unit_id = punit->id;
  p.src_tile = tile_index(unit_tile(punit));
  p.dest_tile = tile_index(dest_tile);

  p.length = 1;
  p.orders[0] = ORDER_MOVE;
  p.dir[0] = dir;
  p.activity[0] = ACTIVITY_LAST;
  p.sub_target[0] = -1;
  p.action[0] = ACTION_NONE;

  send_packet_unit_orders(&client.conn, &p);
}

/**********************************************************************//**
  This function is called whenever the player pressed an arrow key.

  We do NOT take into account that punit might be a caravan or a diplomat
  trying to move into a city, or a diplomat going into a tile with a unit;
  the server will catch those cases and send the client a package to pop up
  a dialog. (the server code has to be there anyway as goto's are entirely
  in the server)
**************************************************************************/
void request_move_unit_direction(struct unit *punit, int dir)
{
  struct packet_unit_orders p;
  struct tile *dest_tile;

  /* Catches attempts to move off map */
  dest_tile = mapstep(&(wld.map), unit_tile(punit), dir);
  if (!dest_tile) {
    return;
  }

  if (!can_unit_exist_at_tile(&(wld.map), punit, dest_tile)) {
    if (request_transport(punit, dest_tile)) {
      return;
    }
  }

  /* The goto system isn't used to send the order because that would
   * prevent direction movement from overriding it.
   * Example of a situation when overriding the goto system is useful:
   * The goto system creates a longer path to make a move legal. The player
   * wishes to order the illegal move so the server will explain why the
   * short move is illegal. */

  memset(&p, 0, sizeof(p));

  p.repeat = FALSE;
  p.vigilant = FALSE;

  p.unit_id = punit->id;
  p.src_tile = tile_index(unit_tile(punit));
  p.dest_tile = tile_index(dest_tile);

  p.length = 1;
  p.orders[0] = ORDER_ACTION_MOVE;
  p.dir[0] = dir;
  p.activity[0] = ACTIVITY_LAST;
  p.sub_target[0] = -1;
  p.action[0] = ACTION_NONE;

  send_packet_unit_orders(&client.conn, &p);
}

/**********************************************************************//**
  Send request for unit activity changing to server. If activity has
  target, use request_new_unit_activity_targeted() instead.
**************************************************************************/
void request_new_unit_activity(struct unit *punit, enum unit_activity act)
{
  request_new_unit_activity_targeted(punit, act, NULL);
}

/**********************************************************************//**
  Send request for unit activity changing to server. This is for
  activities that are targeted to certain special or base type.
**************************************************************************/
void request_new_unit_activity_targeted(struct unit *punit,
					enum unit_activity act,
					struct extra_type *tgt)
{
  if (!can_client_issue_orders()) {
    return;
  }

  if (tgt == NULL) {
    dsend_packet_unit_change_activity(&client.conn, punit->id, act, EXTRA_NONE);
  } else {
    dsend_packet_unit_change_activity(&client.conn, punit->id, act, extra_index(tgt));
  }
}

/**********************************************************************//**
  Destroy the client disband unit data.
**************************************************************************/
static void client_disband_unit_data_destroy(void *p)
{
  struct client_disband_unit_data *data = p;

  free(data);
}

/**********************************************************************//**
  Try to disband a unit using actions ordered by preference.
**************************************************************************/
static void do_disband_alternative(void *p)
{
  struct unit *punit;
  struct city *pcity;
  struct tile *ptile;
  int last_request_id_used;
  struct client_disband_unit_data *next;
  struct client_disband_unit_data *data = p;
  const int act = disband_unit_alternatives[data->alt];

  fc_assert_ret(can_client_issue_orders());

  /* Fetch the unit to get rid of. */
  punit = player_unit_by_number(client_player(), data->unit_id);

  if (punit == NULL) {
    /* Success! It is gone. */
    return;
  }

  if (data->alt == -1) {
    /* All alternatives have been tried. */
    create_event(unit_tile(punit), E_BAD_COMMAND, ftc_client,
                  /* TRANS: Unable to get rid of Leader. */
                 _("Unable to get rid of %s."),
                 unit_name_translation(punit));
    return;
  }

  /* Prepare the data for the next try in case this try fails. */
  next = fc_malloc(sizeof(struct client_disband_unit_data));
  next->unit_id = data->unit_id;
  next->alt = data->alt - 1;

  /* Latest request ID before trying to send a request. */
  last_request_id_used = client.conn.client.last_request_id_used;

  /* Send a request to the server unless it is known to be pointless. */
  switch (action_id_get_target_kind(act)) {
  case ATK_CITY:
    if ((pcity = tile_city(unit_tile(punit)))
        && action_prob_possible(action_prob_vs_city(punit, act, pcity))) {
      request_do_action(act, punit->id, pcity->id, 0, "");
    }
    break;
  case ATK_UNIT:
    if (action_prob_possible(action_prob_vs_unit(punit, act, punit))) {
      request_do_action(act, punit->id, punit->id, 0, "");
    }
    break;
  case ATK_UNITS:
    if ((ptile = unit_tile(punit))
        && action_prob_possible(action_prob_vs_units(punit, act, ptile))) {
      request_do_action(act, punit->id, ptile->index, 0, "");
    }
    break;
  case ATK_TILE:
    if ((ptile = unit_tile(punit))
        && action_prob_possible(action_prob_vs_tile(punit, act, ptile, NULL))) {
      request_do_action(act, punit->id, ptile->index, 0, "");
    }
    break;
  case ATK_SELF:
    if (action_prob_possible(action_prob_self(punit, act))) {
      request_do_action(act, punit->id, punit->id, 0, "");
    }
    break;
  case ATK_COUNT:
    fc_assert(action_id_get_target_kind(act) != ATK_COUNT);
    break;
  }

  if (last_request_id_used != client.conn.client.last_request_id_used) {
    /* A request was sent. */

    /* Check if it worked. Move on if it didn't. */
    update_queue_connect_processing_finished_full
        (client.conn.client.last_request_id_used,
         do_disband_alternative, next,
         client_disband_unit_data_destroy);
  } else {
    /* No request was sent. */

    /* Move on. */
    do_disband_alternative(next);

    /* Won't be freed by anyone else. */
    client_disband_unit_data_destroy(next);
  }
}

/**********************************************************************//**
  Send request to disband unit to server.
**************************************************************************/
void request_unit_disband(struct unit *punit)
{
  struct client_disband_unit_data *data;

  /* Set up disband data. Start at the end of the array. */
  data = fc_malloc(sizeof(struct client_disband_unit_data));
  data->unit_id = punit->id;
  data->alt = 2;

  /* Begin. */
  do_disband_alternative(data);

  /* Won't be freed by anyone else. */
  client_disband_unit_data_destroy(data);
}

/**********************************************************************//**
  Send request to change unit homecity to server.
**************************************************************************/
void request_unit_change_homecity(struct unit *punit)
{
  struct city *pcity=tile_city(unit_tile(punit));

  if (pcity) {
    request_do_action(ACTION_HOME_CITY, punit->id, pcity->id, 0, "");
  }
}

/**********************************************************************//**
  Send request to upgrade unit to server.
**************************************************************************/
void request_unit_upgrade(struct unit *punit)
{
  struct city *pcity=tile_city(unit_tile(punit));

  if (pcity) {
    request_do_action(ACTION_UPGRADE_UNIT, punit->id, pcity->id, 0, "");
  }
}

/**********************************************************************//**
  Sends unit convert packet.
**************************************************************************/
void request_unit_convert(struct unit *punit)
{
  request_new_unit_activity(punit, ACTIVITY_CONVERT);
}

/**********************************************************************//**
  Call to request (from the server) that the settler unit is put into
  autosettler mode.
**************************************************************************/
void request_unit_autosettlers(const struct unit *punit)
{
  if (punit && can_unit_do_autosettlers(punit)) {
    dsend_packet_unit_autosettlers(&client.conn, punit->id);
  } else if (punit) {
    create_event(unit_tile(punit), E_BAD_COMMAND, ftc_client,
                 _("Only settler units can be put into auto mode."));
  }
}

/**********************************************************************//**
  Send a request to the server that the cargo be loaded into the transporter.

  If ptransporter is NULL a suitable transporter will be chosen.
**************************************************************************/
void request_unit_load(struct unit *pcargo, struct unit *ptrans,
                       struct tile *ptile)
{
  if (!ptrans) {
    ptrans = transporter_for_unit(pcargo);
  }

  if (ptrans
      && can_client_issue_orders()
      && could_unit_load(pcargo, ptrans)) {
    dsend_packet_unit_load(&client.conn, pcargo->id, ptrans->id,
                           ptile->index);

    /* Sentry the unit.  Don't request_unit_sentry since this can give a
     * recursive loop. */
    /* FIXME: Should not sentry if above loading fails (transport moved away,
     *        or filled already in server side) */
    dsend_packet_unit_change_activity(&client.conn, pcargo->id,
                                      ACTIVITY_SENTRY, EXTRA_NONE);
  }
}

/**********************************************************************//**
  Send a request to the server that the cargo be unloaded from its current
  transporter.
**************************************************************************/
void request_unit_unload(struct unit *pcargo)
{
  struct unit *ptrans = unit_transport_get(pcargo);

  if (can_client_issue_orders()
      && ptrans
      && can_unit_unload(pcargo, ptrans)
      && can_unit_survive_at_tile(&(wld.map), pcargo, unit_tile(pcargo))) {
    dsend_packet_unit_unload(&client.conn, pcargo->id, ptrans->id);

    if (unit_owner(pcargo) == client.conn.playing
        && pcargo->activity == ACTIVITY_SENTRY) {
      /* Activate the unit. */
      dsend_packet_unit_change_activity(&client.conn, pcargo->id,
                                        ACTIVITY_IDLE, EXTRA_NONE);
    }
  }
}

/**********************************************************************//**
  Send request to do caravan action - establishing traderoute or
  helping in wonder building - to server.
**************************************************************************/
void request_unit_caravan_action(struct unit *punit, action_id action)
{
  struct city *target_city;

  if (!((target_city = tile_city(unit_tile(punit))))) {
    return;
  }

  if (action == ACTION_TRADE_ROUTE) {
    request_do_action(ACTION_TRADE_ROUTE, punit->id,
                      target_city->id, 0, "");
  } else if (action == ACTION_HELP_WONDER) {
    request_do_action(ACTION_HELP_WONDER, punit->id,
                      target_city->id, 0, "");
  } else {
    log_error("request_unit_caravan_action() Bad action (%d)", action);
  }
}

/**********************************************************************//**
  Have the player select what tile to paradrop to. Once selected a
  paradrop request will be sent to server.
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
      offender = unit_tile(punit);
    }
  } unit_list_iterate_end;
  if (can) {
    create_event(unit_tile(unit_list_get(punits, 0)), E_BEGINNER_HELP,
                 ftc_client,
                 /* TRANS: paradrop target tile. */
                 _("Click on a tile to paradrop to it."));

    set_hover_state(punits, HOVER_PARADROP, ACTIVITY_LAST, NULL,
                    -1, ACTION_NONE, ORDER_LAST);
    update_unit_info_label(punits);
  } else {
    create_event(offender, E_BAD_COMMAND, ftc_client,
                 _("Only paratrooper units can do this."));
  }
}

/**********************************************************************//**
  Either start new patrol route planning, or add waypoint to current one.
**************************************************************************/
void request_unit_patrol(void)
{
  struct unit_list *punits = get_units_in_focus();

  if (unit_list_size(punits) == 0) {
    return;
  }

  if (hover_state != HOVER_PATROL) {
    set_hover_state(punits, HOVER_PATROL, ACTIVITY_LAST, NULL,
                    -1, ACTION_NONE, ORDER_LAST);
    update_unit_info_label(punits);
    enter_goto_state(punits);
    create_line_at_mouse_pos();
  } else {
    fc_assert_ret(goto_is_active());
    goto_add_waypoint();
  }
}

/**********************************************************************//**
  Try to sentry unit.
**************************************************************************/
void request_unit_sentry(struct unit *punit)
{
  if (punit->activity != ACTIVITY_SENTRY
      && can_unit_do_activity(punit, ACTIVITY_SENTRY)) {
    request_new_unit_activity(punit, ACTIVITY_SENTRY);
  }
}

/**********************************************************************//**
  Try to fortify unit.
**************************************************************************/
void request_unit_fortify(struct unit *punit)
{
  if (punit->activity != ACTIVITY_FORTIFYING
      && can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
    request_new_unit_activity(punit, ACTIVITY_FORTIFYING);
  }
}

/**********************************************************************//**
  Send pillage request to server.
**************************************************************************/
void request_unit_pillage(struct unit *punit)
{
  if (!game.info.pillage_select) {
    /* Leave choice up to the server */
    request_new_unit_activity_targeted(punit, ACTIVITY_PILLAGE, NULL);
  } else {
    bv_extras pspossible;
    int count = 0;

    BV_CLR_ALL(pspossible);
    extra_type_iterate(potential) {
      if (can_unit_do_activity_targeted(punit, ACTIVITY_PILLAGE,
                                        potential)) {
        BV_SET(pspossible, extra_index(potential));
        count++;
      }
    } extra_type_iterate_end;

    if (count > 1) {
      popup_pillage_dialog(punit, pspossible);
    } else {
      /* Should be only one choice... */
      struct extra_type *target = get_preferred_pillage(pspossible);

      if (target != NULL) {
        request_new_unit_activity_targeted(punit, ACTIVITY_PILLAGE, target);
      }
    }
  }
}

/**********************************************************************//**
  Toggle display of city outlines on the map
**************************************************************************/
void request_toggle_city_outlines(void) 
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_city_outlines = !gui_options.draw_city_outlines;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of worker output of cities on the map
**************************************************************************/
void request_toggle_city_output(void)
{
  if (!can_client_change_view()) {
    return;
  }
  
  gui_options.draw_city_output = !gui_options.draw_city_output;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of grid lines on the map
**************************************************************************/
void request_toggle_map_grid(void) 
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_map_grid ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of national borders on the map
**************************************************************************/
void request_toggle_map_borders(void) 
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_borders ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of native tiles on the map
**************************************************************************/
void request_toggle_map_native(void) 
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_native ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of city full bar.
**************************************************************************/
void request_toggle_city_full_bar(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_full_citybar ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of city names
**************************************************************************/
void request_toggle_city_names(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_city_names ^= 1;
  update_map_canvas_visible();
}
 
/**********************************************************************//**
  Toggle display of city growth (turns-to-grow)
**************************************************************************/
void request_toggle_city_growth(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_city_growth ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of city productions
**************************************************************************/
void request_toggle_city_productions(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_city_productions ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of city buycost
**************************************************************************/
void request_toggle_city_buycost(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_city_buycost ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of city trade routes
**************************************************************************/
void request_toggle_city_trade_routes(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_city_trade_routes ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of terrain
**************************************************************************/
void request_toggle_terrain(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_terrain ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of coastline
**************************************************************************/
void request_toggle_coastline(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_coastline ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of roads and rails
**************************************************************************/
void request_toggle_roads_rails(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_roads_rails ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of irrigation
**************************************************************************/
void request_toggle_irrigation(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_irrigation ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of mines
**************************************************************************/
void request_toggle_mines(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_mines ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of bases
**************************************************************************/
void request_toggle_bases(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_fortress_airbase ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of resources
**************************************************************************/
void request_toggle_resources(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_specials ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of huts
**************************************************************************/
void request_toggle_huts(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_huts ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of pollution
**************************************************************************/
void request_toggle_pollution(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_pollution ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of cities
**************************************************************************/
void request_toggle_cities(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_cities ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of units
**************************************************************************/
void request_toggle_units(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_units ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of unit solid background.
**************************************************************************/
void request_toggle_unit_solid_bg(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.solid_color_behind_units ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of unit shields.
**************************************************************************/
void request_toggle_unit_shields(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_unit_shields ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of focus unit
**************************************************************************/
void request_toggle_focus_unit(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_focus_unit ^= 1;
  update_map_canvas_visible();
}

/**********************************************************************//**
  Toggle display of fog of war
**************************************************************************/
void request_toggle_fog_of_war(void)
{
  if (!can_client_change_view()) {
    return;
  }

  gui_options.draw_fog_of_war ^= 1;
  update_map_canvas_visible();
  refresh_overview_canvas();
}

/**********************************************************************//**
  Center to focus unit.
**************************************************************************/
void request_center_focus_unit(void)
{
  struct tile *ptile = find_a_focus_unit_tile_to_center_on();

  if (ptile) {
    center_tile_mapcanvas(ptile);
  }
}

/**********************************************************************//**
  Set units in list to waiting focus. If they are current focus units,
  advance focus.
**************************************************************************/
void request_units_wait(struct unit_list *punits)
{
  unit_list_iterate(punits, punit) {
    punit->client.focus_status = FOCUS_WAIT;
  } unit_list_iterate_end;
  if (punits == get_units_in_focus()) {
    unit_focus_advance();
  }
}

/**********************************************************************//**
  Set focus units to FOCUS_DONE state.
**************************************************************************/
void request_unit_move_done(void)
{
  if (get_num_units_in_focus() > 0) {
    enum unit_focus_status new_status = FOCUS_DONE;
    unit_list_iterate(get_units_in_focus(), punit) {
      /* If any of the focused units are busy, keep all of them
       * in focus; another tap of the key will dismiss them */
      if (punit->activity != ACTIVITY_IDLE) {
        new_status = FOCUS_WAIT;
      }
    } unit_list_iterate_end;
    unit_list_iterate(get_units_in_focus(), punit) {
      clear_unit_orders(punit);
      punit->client.focus_status = new_status;
    } unit_list_iterate_end;
    if (new_status == FOCUS_DONE) {
      unit_focus_advance();
    }
  }
}

/**********************************************************************//**
  Called to have the client move a unit from one location to another,
  updating the graphics if necessary.  The caller must redraw the target
  location after the move.
**************************************************************************/
void do_move_unit(struct unit *punit, struct unit *target_unit)
{
  struct tile *src_tile = unit_tile(punit);
  struct tile *dst_tile = unit_tile(target_unit);
  bool was_teleported, do_animation;
  bool in_focus = unit_is_in_focus(punit);

  was_teleported = !is_tiles_adjacent(src_tile, dst_tile);
  do_animation = (!was_teleported && gui_options.smooth_move_unit_msec > 0);

  if (!was_teleported
      && punit->activity != ACTIVITY_SENTRY
      && !unit_transported(punit)) {
    audio_play_sound(unit_type_get(punit)->sound_move,
                     unit_type_get(punit)->sound_move_alt);
  }

  if (unit_owner(punit) == client.conn.playing
      && gui_options.auto_center_on_unit
      && !unit_has_orders(punit)
      && punit->activity != ACTIVITY_GOTO
      && punit->activity != ACTIVITY_SENTRY
      && ((gui_options.auto_center_on_automated == TRUE
           && punit->ai_controlled == TRUE)
          || (punit->ai_controlled == FALSE))
      && !tile_visible_and_not_on_border_mapcanvas(dst_tile)) {
    center_tile_mapcanvas(dst_tile);
  }

  if (hover_state != HOVER_NONE && in_focus) {
    /* Cancel current goto/patrol/connect/nuke command. */
    clear_hover_state();
    update_unit_info_label(get_units_in_focus());
  }

  unit_list_remove(src_tile->units, punit);

  if (!unit_transported(punit)) {
    /* Mark the unit as moving unit, then find_visible_unit() won't return
     * it. It is especially useful to don't draw many times the unit when
     * refreshing the canvas. */
    punit_moving = punit;

    /* We have to refresh the tile before moving.  This will draw
     * the tile without the unit (because it was unlinked above). */
    refresh_unit_mapcanvas(punit, src_tile, TRUE, FALSE);

    if (gui_options.auto_center_on_automated == FALSE
        && punit->ai_controlled == TRUE) {
      /* Dont animate automatic units */
    } else if (do_animation) {
      int dx, dy;

      /* For the duration of the animation the unit exists at neither
       * tile. */
      map_distance_vector(&dx, &dy, src_tile, dst_tile);
      move_unit_map_canvas(punit, src_tile, dx, dy);
    }
  }

  unit_tile_set(punit, dst_tile);
  unit_list_prepend(dst_tile->units, punit);

  if (!unit_transported(punit)) {
    /* For find_visible_unit(), see above. */
    punit_moving = NULL;

    refresh_unit_mapcanvas(punit, dst_tile, TRUE, FALSE);
  }

  /* With the "full" city bar we have to update the city bar when units move
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

  if (in_focus) {
    menus_update();
  }
}

/**********************************************************************//**
  An action selection dialog for the selected units against the specified
  tile is wanted.
**************************************************************************/
static void do_unit_act_sel_vs(struct tile *ptile)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (utype_may_act_at_all(unit_type_get(punit))) {
      /* Have the server record that an action decision is wanted for
       * this unit against this tile. */
      dsend_packet_unit_sscs_set(&client.conn, punit->id,
                                 USSDT_QUEUE, tile_index(ptile));
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handles everything when the user clicked a tile
**************************************************************************/
void do_map_click(struct tile *ptile, enum quickselect_type qtype)
{
  struct city *pcity = tile_city(ptile);
  struct unit_list *punits = get_units_in_focus();
  bool maybe_goto = FALSE;

  if (hover_state != HOVER_NONE) {
    switch (hover_state) {
    case HOVER_NONE:
      break;
    case HOVER_GOTO:
      do_unit_goto(ptile);
      break;
    case HOVER_PARADROP:
      unit_list_iterate(punits, punit) {
	do_unit_paradrop_to(punit, ptile);
      } unit_list_iterate_end;
      break;
    case HOVER_CONNECT:
      do_unit_connect(ptile, connect_activity, connect_tgt);
      break;
    case HOVER_PATROL:
      do_unit_patrol_to(ptile);
      break;	
    case HOVER_ACT_SEL_TGT:
      do_unit_act_sel_vs(ptile);
      break;
    }

    clear_hover_state();
    update_unit_info_label(get_units_in_focus());
  }

  /* Bypass stack or city popup if quickselect is specified. */
  else if (qtype != SELECT_POPUP && qtype != SELECT_APPEND) {
    struct unit *qunit = quickselect(ptile, qtype);
    if (qunit) {
      unit_focus_set_and_select(qunit);
      maybe_goto = gui_options.keyboardless_goto;
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
    maybe_goto = gui_options.keyboardless_goto;
  }
  else if (unit_list_size(ptile->units) == 1
           && !get_transporter_occupancy(unit_list_get(ptile->units, 0))) {
    struct unit *punit = unit_list_get(ptile->units, 0);

    if (unit_owner(punit) == client.conn.playing) {
      if (can_unit_do_activity(punit, ACTIVITY_IDLE)) {
        maybe_goto = gui_options.keyboardless_goto;
	if (qtype == SELECT_APPEND) {
	  unit_focus_add(punit);
	} else {
	  unit_focus_set_and_select(punit);
	}
      }
    } else if (pcity) {
      /* Don't hide the unit in the city. */
      unit_select_dialog_popup(ptile);
    }
  } else if (unit_list_size(ptile->units) > 0) {
    /* The stack list is always popped up, even if it includes enemy units.
     * If the server doesn't want the player to know about them it shouldn't
     * tell him!  The previous behavior would only pop up the stack if you
     * owned a unit on the tile.  This gave cheating clients an advantage,
     * and also showed you allied units if (and only if) you had a unit on
     * the tile (inconsistent). */
    unit_select_dialog_popup(ptile);
  }

  /* See mapctrl_common.c */
  keyboardless_goto_start_tile = maybe_goto ? ptile : NULL;
  keyboardless_goto_button_down = maybe_goto;
  keyboardless_goto_active = FALSE;
}

/**********************************************************************//**
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

  fc_assert_ret_val(qtype > SELECT_POPUP, NULL);

  if (qtype == SELECT_FOCUS) {
    return head_of_units_in_focus();
  }

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
    else if (utype_move_type(unit_type_get(punit)) == UMT_SEA) {
      if (punit->moves_left > 0) {
        if (!panymovesea) {
          panymovesea = punit;
        }
      } else if (!panysea) {
          panysea = punit;
      }
    }
  } else if (qtype == SELECT_LAND) {
    if (utype_move_type(unit_type_get(punit)) == UMT_LAND) {
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
    else if (utype_move_type(unit_type_get(punit)) == UMT_SEA) {
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

/**********************************************************************//**
  Finish the goto mode and let the units stored in goto_map_list move
  to a given location.
**************************************************************************/
void do_unit_goto(struct tile *ptile)
{
  if (hover_state != HOVER_GOTO) {
    return;
  }

  if (is_valid_goto_draw_line(ptile)) {
    send_goto_route();
  } else {
    create_event(ptile, E_BAD_COMMAND, ftc_client,
                 _("Didn't find a route to the destination!"));
  }
}

/**********************************************************************//**
  Paradrop to a location.
**************************************************************************/
void do_unit_paradrop_to(struct unit *punit, struct tile *ptile)
{
  request_do_action(ACTION_PARADROP, punit->id, tile_index(ptile), 0 , "");
}

/**********************************************************************//**
  Patrol to a location.
**************************************************************************/
void do_unit_patrol_to(struct tile *ptile)
{
  if (is_valid_goto_draw_line(ptile)
      && !is_non_allied_unit_tile(ptile, client.conn.playing)) {
    send_patrol_route();
  } else {
    create_event(ptile, E_BAD_COMMAND, ftc_client,
                 _("Didn't find a route to the destination!"));
  }

  clear_hover_state();
}

/**********************************************************************//**
  "Connect" to the given location.
**************************************************************************/
void do_unit_connect(struct tile *ptile,
		     enum unit_activity activity,
                     struct extra_type *tgt)
{
  if (is_valid_goto_draw_line(ptile)) {
    send_connect_route(activity, tgt);
  } else {
    create_event(ptile, E_BAD_COMMAND, ftc_client,
                 _("Didn't find a route to the destination!"));
  }

  clear_hover_state();
}

/**********************************************************************//**
  The 'Escape' key.
**************************************************************************/
void key_cancel_action(void)
{
  cancel_tile_hiliting();

  switch (hover_state) {
  case HOVER_GOTO:
  case HOVER_PATROL:
  case HOVER_CONNECT:
    if (goto_pop_waypoint()) {
      break;
    }
    /* else fall through: */
  case HOVER_PARADROP:
  case HOVER_ACT_SEL_TGT:
    clear_hover_state();
    update_unit_info_label(get_units_in_focus());

    keyboardless_goto_button_down = FALSE;
    keyboardless_goto_active = FALSE;
    keyboardless_goto_start_tile = NULL;
    break;
  case HOVER_NONE:
    break;
  };
}

/**********************************************************************//**
  Center the mapview on the player's capital, or print a failure message.
**************************************************************************/
void key_center_capital(void)
{
  struct city *capital = player_capital(client_player());

  if (capital)  {
    /* Center on the tile, and pop up the crosshair overlay. */
    center_tile_mapcanvas(capital->tile);
    put_cross_overlay_tile(capital->tile);
  } else {
    create_event(NULL, E_BAD_COMMAND, ftc_client,
                 _("Oh my! You seem to have no capital!"));
  }
}

/**********************************************************************//**
  Handle user 'end turn' input.
**************************************************************************/
void key_end_turn(void)
{
  send_turn_done();
}

/**********************************************************************//**
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
      unit_focus_set(punit);
    } else {
      unit_focus_add(punit);
    }
    i++;
  } unit_list_iterate_safe_end;
}

/**********************************************************************//**
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

/**********************************************************************//**
  Handle use 'build city' input.
**************************************************************************/
void key_unit_build_city(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_build_city(punit);
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'help build wonder' input
**************************************************************************/
void key_unit_build_wonder(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (unit_can_do_action(punit, ACTION_HELP_WONDER)) {
      request_unit_caravan_action(punit, ACTION_HELP_WONDER);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user pressing key for 'Connect' command
**************************************************************************/
void key_unit_connect(enum unit_activity activity,
                      struct extra_type *tgt)
{
  request_unit_connect(activity, tgt);
}

/**********************************************************************//**
  Handle user 'Do...' input
**************************************************************************/
void key_unit_action_select(void)
{
  struct tile *ptile;

  if (!can_ask_server_for_actions()) {
    /* Looks like another action selection dialog is open. */
    return;
  }

  unit_list_iterate(get_units_in_focus(), punit) {
    if (utype_may_act_at_all(unit_type_get(punit))
        && (ptile = unit_tile(punit))) {
      /* Have the server record that an action decision is wanted for this
       * unit. */
      dsend_packet_unit_sscs_set(&client.conn, punit->id,
                                 USSDT_QUEUE, tile_index(ptile));
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Have the user select what action the unit(s) in focus should perform to
  the targets at the tile the user will specify by clicking on it.

  Will stop asking for a target tile and have each actor unit act against
  its own tile if called twice.
**************************************************************************/
void key_unit_action_select_tgt(void)
{
  struct unit_list *punits = get_units_in_focus();

  if (hover_state == HOVER_ACT_SEL_TGT) {
    /* The 2nd key press means that the actor should target its own
     * tile. */
    key_unit_action_select();

    /* Target tile selected. Clean up hover state. */
    clear_hover_state();
    update_unit_info_label(punits);

    return;
  }

  create_event(unit_tile(unit_list_get(punits, 0)), E_BEGINNER_HELP,
               ftc_client,
               /* TRANS: "Do..." action selection dialog target. */
               _("Click on a tile to act against it. "
                 "Press 'd' again to act against own tile."));

  set_hover_state(punits, HOVER_ACT_SEL_TGT, ACTIVITY_LAST, NULL,
                  EXTRA_NONE, ACTION_NONE, ORDER_LAST);
}

/**********************************************************************//**
  Handle user 'unit done' input
**************************************************************************/
void key_unit_done(void)
{
  request_unit_move_done();
}

/**********************************************************************//**
  Handle user 'unit goto' input
**************************************************************************/
void key_unit_goto(void)
{
  request_unit_goto(ORDER_LAST, ACTION_NONE, -1);
}

/**********************************************************************//**
  Handle user 'paradrop' input
**************************************************************************/
void key_unit_paradrop(void)
{
  request_unit_paradrop(get_units_in_focus());
}

/**********************************************************************//**
  Handle user 'patrol' input
**************************************************************************/
void key_unit_patrol(void)
{
  request_unit_patrol();
}

/**********************************************************************//**
  Handle user 'establish traderoute' input
**************************************************************************/
void key_unit_trade_route(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    /* TODO: Is falling back on ACTION_MARKETPLACE if not able to establish
     * a trade route trade a good idea or an unplecant surprice? */
    if (unit_can_do_action(punit, ACTION_TRADE_ROUTE)) {
      request_unit_caravan_action(punit, ACTION_TRADE_ROUTE);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'unload all' input
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
      punit->client.focus_status = FOCUS_WAIT;
    } unit_list_iterate_end;
    unit_focus_set(pnext_focus);
  }
}

/**********************************************************************//**
  Handle user 'wait' input
**************************************************************************/
void key_unit_wait(void)
{
  request_units_wait(get_units_in_focus());
}

/**********************************************************************//**
  Handle user 'wakeup others' input
**************************************************************************/
void key_unit_wakeup_others(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_wakeup(punit);
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'build base of class airbase' input
**************************************************************************/
void key_unit_airbase(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    struct base_type *pbase =
      get_base_by_gui_type(BASE_GUI_AIRBASE, punit, unit_tile(punit));

    if (pbase) {
      struct extra_type *pextra = base_extra_get(pbase);

      request_new_unit_activity_targeted(punit, ACTIVITY_BASE, pextra);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'autoexplore' input
**************************************************************************/
void key_unit_auto_explore(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_EXPLORE)) {
      request_new_unit_activity(punit, ACTIVITY_EXPLORE);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
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

/**********************************************************************//**
  Unit convert key pressed or respective menu entry selected.
**************************************************************************/
void key_unit_convert(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_convert(punit);
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'clean fallout' input
**************************************************************************/
void key_unit_fallout(void)
{
  key_unit_clean(ACTIVITY_FALLOUT, ERM_CLEANFALLOUT);
}

/**********************************************************************//**
  Handle user 'fortify' input
**************************************************************************/
void key_unit_fortify(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
      request_new_unit_activity(punit, ACTIVITY_FORTIFYING);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'build base of class fortress' input
**************************************************************************/
void key_unit_fortress(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    struct base_type *pbase =
      get_base_by_gui_type(BASE_GUI_FORTRESS, punit, unit_tile(punit));

    if (pbase) {
      struct extra_type *pextra = base_extra_get(pbase);

      request_new_unit_activity_targeted(punit, ACTIVITY_BASE, pextra);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'change homecity' input
**************************************************************************/
void key_unit_homecity(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    request_unit_change_homecity(punit);
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user extra building input of given type
**************************************************************************/
static void key_unit_extra(enum unit_activity act, enum extra_cause cause)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    struct extra_type *tgt = next_extra_for_tile(unit_tile(punit),
                                                 cause,
                                                 unit_owner(punit),
                                                 punit);

    if (can_unit_do_activity_targeted(punit, act, tgt)) {
      request_new_unit_activity_targeted(punit, act, tgt);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user extra cleaning input of given type
**************************************************************************/
static void key_unit_clean(enum unit_activity act, enum extra_rmcause rmcause)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    struct extra_type *tgt = prev_extra_in_tile(unit_tile(punit),
                                                rmcause,
                                                unit_owner(punit),
                                                punit);

    if (tgt != NULL
        && can_unit_do_activity_targeted(punit, act, tgt)) {
      request_new_unit_activity_targeted(punit, act, tgt);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'irrigate' input
**************************************************************************/
void key_unit_irrigate(void)
{
  key_unit_extra(ACTIVITY_IRRIGATE, EC_IRRIGATION);
}

/**********************************************************************//**
  Handle user 'build mine' input
**************************************************************************/
void key_unit_mine(void)
{
  key_unit_extra(ACTIVITY_MINE, EC_MINE);
}

/**********************************************************************//**
  Handle user 'pillage' input
**************************************************************************/
void key_unit_pillage(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_PILLAGE)) {
      request_unit_pillage(punit);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'clean pollution' input
**************************************************************************/
void key_unit_pollution(void)
{
  key_unit_clean(ACTIVITY_POLLUTION, ERM_CLEANPOLLUTION);
}

/**********************************************************************//**
  Handle user 'build road or railroad' input
**************************************************************************/
void key_unit_road(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    struct extra_type *tgt = next_extra_for_tile(unit_tile(punit),
                                                 EC_ROAD,
                                                 unit_owner(punit),
                                                 punit);

    if (tgt != NULL
        && can_unit_do_activity_targeted(punit, ACTIVITY_GEN_ROAD, tgt)) {
      request_new_unit_activity_targeted(punit, ACTIVITY_GEN_ROAD, tgt);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'sentry' input
**************************************************************************/
void key_unit_sentry(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_SENTRY)) {
      request_new_unit_activity(punit, ACTIVITY_SENTRY);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Handle user 'transform unit' input
**************************************************************************/
void key_unit_transform(void)
{
  unit_list_iterate(get_units_in_focus(), punit) {
    if (can_unit_do_activity(punit, ACTIVITY_TRANSFORM)) {
      request_new_unit_activity(punit, ACTIVITY_TRANSFORM);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Assign all focus units to this battlegroup.
**************************************************************************/
void key_unit_assign_battlegroup(int battlegroup, bool append)
{
  if (NULL != client.conn.playing && can_client_issue_orders()
      && battlegroups >= 0 && battlegroup < MAX_NUM_BATTLEGROUPS) {
    if (!append) {
      unit_list_iterate_safe(battlegroups[battlegroup], punit) {
	if (!unit_is_in_focus(punit)) {
	  punit->battlegroup = BATTLEGROUP_NONE;
          dsend_packet_unit_sscs_set(&client.conn, punit->id,
                                     USSDT_BATTLE_GROUP,
                                     BATTLEGROUP_NONE);
	  refresh_unit_mapcanvas(punit, unit_tile(punit), TRUE, FALSE);
	  unit_list_remove(battlegroups[battlegroup], punit);
	}
      } unit_list_iterate_safe_end;
    }
    unit_list_iterate(get_units_in_focus(), punit) {
      if (punit->battlegroup != battlegroup) {
	if (punit->battlegroup >= 0
	    && punit->battlegroup < MAX_NUM_BATTLEGROUPS) {
	  unit_list_remove(battlegroups[punit->battlegroup], punit);
	}
	punit->battlegroup = battlegroup;
        dsend_packet_unit_sscs_set(&client.conn, punit->id,
                                   USSDT_BATTLE_GROUP,
                                   battlegroup);
	unit_list_append(battlegroups[battlegroup], punit);
	refresh_unit_mapcanvas(punit, unit_tile(punit), TRUE, FALSE);
      }
    } unit_list_iterate_end;
    unit_list_iterate(battlegroups[battlegroup], punit) {
      unit_focus_add(punit);
    } unit_list_iterate_end;
  }
}

/**********************************************************************//**
  Bring this battlegroup into focus.
**************************************************************************/
void key_unit_select_battlegroup(int battlegroup, bool append)
{
  if (NULL != client.conn.playing && can_client_change_view()
      && battlegroups >= 0 && battlegroup < MAX_NUM_BATTLEGROUPS) {
    int i = 0;

    if (unit_list_size(battlegroups[battlegroup]) == 0 && !append) {
      unit_focus_set(NULL);
      return;
    }

    /* FIXME: this is very inefficient and can be improved. */
    unit_list_iterate(battlegroups[battlegroup], punit) {
      if (i == 0 && !append) {
	unit_focus_set(punit);
      } else {
	unit_focus_add(punit);
      }
      i++;
    } unit_list_iterate_end;
  }
}

/**********************************************************************//**
  Toggle drawing of city outlines.
**************************************************************************/
void key_city_outlines_toggle(void)
{
  request_toggle_city_outlines();
}

/**********************************************************************//**
  Toggle drawing of city output produced by workers of the city.
**************************************************************************/
void key_city_output_toggle(void)
{
  request_toggle_city_output();
}

/**********************************************************************//**
  Handle user 'toggle map grid' input
**************************************************************************/
void key_map_grid_toggle(void)
{
  request_toggle_map_grid();
}

/**********************************************************************//**
  Toggle map borders on the mapview on/off based on a keypress.
**************************************************************************/
void key_map_borders_toggle(void)
{
  request_toggle_map_borders();
}

/**********************************************************************//**
  Toggle native tiles on the mapview on/off based on a keypress.
**************************************************************************/
void key_map_native_toggle(void)
{
  request_toggle_map_native();
}

/**********************************************************************//**
  Toggle the "Draw the city bar" option.
**************************************************************************/
void key_city_full_bar_toggle(void)
{
  request_toggle_city_full_bar();
}

/**********************************************************************//**
  Handle user 'toggle city names display' input
**************************************************************************/
void key_city_names_toggle(void)
{
  request_toggle_city_names();
}

/**********************************************************************//**
  Toggles the "show city growth turns" option by passing off the
  request to another function...
**************************************************************************/
void key_city_growth_toggle(void)
{
  request_toggle_city_growth();
}

/**********************************************************************//**
  Toggles the showing of the buy cost of the current production in the
  city descriptions.
**************************************************************************/
void key_city_buycost_toggle(void)
{
  request_toggle_city_buycost();
}

/**********************************************************************//**
  Handle user 'toggle city production display' input
**************************************************************************/
void key_city_productions_toggle(void)
{
  request_toggle_city_productions();
}

/**********************************************************************//**
  Handle client request to toggle drawing of trade route information
  by the city name for cities visible on the main map view.
**************************************************************************/
void key_city_trade_routes_toggle(void)
{
  request_toggle_city_trade_routes();
}

/**********************************************************************//**
  Handle user 'toggle terrain display' input
**************************************************************************/
void key_terrain_toggle(void)
{
  request_toggle_terrain();
}

/**********************************************************************//**
  Handle user 'toggle coastline display' input
**************************************************************************/
void key_coastline_toggle(void)
{
  request_toggle_coastline();
}

/**********************************************************************//**
  Handle user 'toggle road/railroad display' input
**************************************************************************/
void key_roads_rails_toggle(void)
{
  request_toggle_roads_rails();
}

/**********************************************************************//**
  Handle user 'toggle irrigation display' input
**************************************************************************/
void key_irrigation_toggle(void)
{
  request_toggle_irrigation();
}

/**********************************************************************//**
  Handle user 'toggle mine display' input
**************************************************************************/
void key_mines_toggle(void)
{
  request_toggle_mines();
}

/**********************************************************************//**
  Handle user 'toggle bases display' input
**************************************************************************/
void key_bases_toggle(void)
{
  request_toggle_bases();
}

/**********************************************************************//**
  Handle user 'toggle resources display' input
**************************************************************************/
void key_resources_toggle(void)
{
  request_toggle_resources();
}

/**********************************************************************//**
  Handle user 'toggle huts display' input
**************************************************************************/
void key_huts_toggle(void)
{
  request_toggle_huts();
}

/**********************************************************************//**
  Handle user 'toggle pollution display' input
**************************************************************************/
void key_pollution_toggle(void)
{
  request_toggle_pollution();
}

/**********************************************************************//**
  Handle user 'toggle cities display' input
**************************************************************************/
void key_cities_toggle(void)
{
  request_toggle_cities();
}

/**********************************************************************//**
  Handle user 'toggle units display' input
**************************************************************************/
void key_units_toggle(void)
{
  request_toggle_units();
}

/**********************************************************************//**
  Toggle the "Solid unit background color" option.
**************************************************************************/
void key_unit_solid_bg_toggle(void)
{
  request_toggle_unit_solid_bg();
}

/**********************************************************************//**
  Toggle the "Draw shield graphics for units" option.
**************************************************************************/
void key_unit_shields_toggle(void)
{
  request_toggle_unit_shields();
}

/**********************************************************************//**
  Handle user 'toggle key units display' input
**************************************************************************/
void key_focus_unit_toggle(void)
{
  request_toggle_focus_unit();
}

/**********************************************************************//**
  Handle user 'toggle fog of war display' input
**************************************************************************/
void key_fog_of_war_toggle(void)
{
  request_toggle_fog_of_war();
}

/**********************************************************************//**
  Toggle editor mode in the server.
**************************************************************************/
void key_editor_toggle(void)
{
  dsend_packet_edit_mode(&client.conn, !game.info.is_edit_mode);
}

/**********************************************************************//**
  Recalculate borders.
**************************************************************************/
void key_editor_recalculate_borders(void)
{
  send_packet_edit_recalculate_borders(&client.conn);
}

/**********************************************************************//**
  Send a request to the server to toggle fog-of-war for the current
  player (only applies in edit mode).
**************************************************************************/
void key_editor_toggle_fogofwar(void)
{
  if (client_has_player()) {
    dsend_packet_edit_toggle_fogofwar(&client.conn, client_player_number());
  }
}

/**********************************************************************//**
  All units ready to build city to the tile should now proceed.
**************************************************************************/
void finish_city(struct tile *ptile, const char *name)
{
  unit_list_iterate(ptile->units, punit) {
    if (punit->client.asking_city_name) {
      /* Unit will disappear only in case city building still success.
       * Cancel city building status just in case something has changed
       * to prevent city building in the meanwhile and unit will remain
       * alive. */
      punit->client.asking_city_name = FALSE;
      request_do_action(ACTION_FOUND_CITY, punit->id, ptile->index,
                        0, name);
    }
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Do not build city after all. Cancel city building mark from all units
  prepared for it.
**************************************************************************/
void cancel_city(struct tile *ptile)
{
  unit_list_iterate(ptile->units, punit) {
    punit->client.asking_city_name = FALSE;
  } unit_list_iterate_end;
}
