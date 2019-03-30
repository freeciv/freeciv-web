/***********************************************************************
 Freeciv - Copyright (C) 2001 - R. Falke
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* utility */
#include "bugs.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"             /* for MIN() */
#include "specialist.h"
#include "support.h"
#include "timing.h"

/* common */
#include "city.h"
#include "dataio.h"
#include "events.h"
#include "game.h"
#include "government.h"
#include "packets.h"
#include "specialist.h"

/* client */
#include "attribute.h"
#include "client_main.h"
#include "climisc.h"
#include "packhand.h"

/* client/include */
#include "chatline_g.h"
#include "citydlg_g.h"
#include "cityrep_g.h"
#include "messagewin_g.h"

/* client/agents */
#include "agents.h"

#include "cma_core.h"


/*
 * The CMA is an agent. The CMA will subscribe itself to all city
 * events. So if a city changes the callback function city_changed is
 * called. handle_city will be called from city_changed to update the
 * given city. handle_city will call cma_query_result and
 * apply_result_on_server to update the server city state.
 */

/****************************************************************************
 defines, structs, globals, forward declarations
*****************************************************************************/

#define log_apply_result                log_debug
#define log_handle_city                 log_debug
#define log_handle_city2                log_debug
#define log_results_are_equal           log_debug

#define SHOW_TIME_STATS                                 FALSE
#define SHOW_APPLY_RESULT_ON_SERVER_ERRORS              FALSE
#define ALWAYS_APPLY_AT_SERVER                          FALSE

#define SAVED_PARAMETER_SIZE				29

/*
 * Misc statistic to analyze performance.
 */
static struct {
  struct timer *wall_timer;
  int apply_result_ignored, apply_result_applied, refresh_forced;
} stats;


/************************************************************************//**
  Returns TRUE iff the two results are equal. Both results have to be
  results for the given city.
****************************************************************************/
static bool fc_results_are_equal(const struct cm_result *result1,
                                 const struct cm_result *result2)
{
#define T(x) if (result1->x != result2->x) { \
  log_results_are_equal(#x); \
  return FALSE; }

  T(disorder);
  T(happy);

  specialist_type_iterate(sp) {
    T(specialists[sp]);
  } specialist_type_iterate_end;

  output_type_iterate(ot) {
    T(surplus[ot]);
  } output_type_iterate_end;

  fc_assert_ret_val(result1->city_radius_sq == result2->city_radius_sq,
                    FALSE);
  city_map_iterate(result1->city_radius_sq, cindex, x, y) {
    if (is_free_worked_index(cindex)) {
      continue;
    }

    if (result1->worker_positions[cindex]
        != result2->worker_positions[cindex]) {
      log_results_are_equal("worker_positions");
      return FALSE;
    }
  } city_map_iterate_end;

  return TRUE;

#undef T
}


/************************************************************************//**
  Returns TRUE if the city is valid for CMA. Fills parameter if TRUE
  is returned. Parameter can be NULL.
****************************************************************************/
static struct city *check_city(int city_id, struct cm_parameter *parameter)
{
  struct city *pcity = game_city_by_number(city_id);
  struct cm_parameter dummy;

  if (!parameter) {
    parameter = &dummy;
  }

  if (!pcity
      || !cma_get_parameter(ATTR_CITY_CMA_PARAMETER, city_id, parameter)) {
    return NULL;
  }

  if (city_owner(pcity) != client.conn.playing) {
    cma_release_city(pcity);
    return NULL;
  }

  return pcity;
}  

/************************************************************************//**
 Change the actual city setting to the given result. Returns TRUE iff
 the actual data matches the calculated one.
****************************************************************************/
static bool apply_result_on_server(struct city *pcity,
                                   const struct cm_result *result)
{
  int first_request_id = 0, last_request_id = 0, i;
  int city_radius_sq = city_map_radius_sq_get(pcity);
  struct cm_result *current_state = cm_result_new(pcity);;
  bool success;
  struct tile *pcenter = city_tile(pcity);

  fc_assert_ret_val(result->found_a_valid, FALSE);
  cm_result_from_main_map(current_state, pcity);

  if (fc_results_are_equal(current_state, result)
      && !ALWAYS_APPLY_AT_SERVER) {
    stats.apply_result_ignored++;
    return TRUE;
  }

  /* Do checks */
  if (city_size_get(pcity) != cm_result_citizens(result)) {
    log_error("apply_result_on_server(city %d=\"%s\") bad result!",
              pcity->id, city_name_get(pcity));
    cm_print_city(pcity);
    cm_print_result(result);
    return FALSE;
  }

  stats.apply_result_applied++;

  log_apply_result("apply_result_on_server(city %d=\"%s\")",
                   pcity->id, city_name_get(pcity));

  connection_do_buffer(&client.conn);

  /* Remove all surplus workers */
  city_tile_iterate_skip_free_worked(city_radius_sq, pcenter, ptile, idx,
                                     x, y) {
    if (tile_worked(ptile) == pcity
        && !result->worker_positions[idx]) {
      log_apply_result("Removing worker at {%d,%d}.", x, y);

      last_request_id =
        dsend_packet_city_make_specialist(&client.conn,
                                          pcity->id, ptile->index);
      if (first_request_id == 0) {
        first_request_id = last_request_id;
      }
    }
  } city_tile_iterate_skip_free_worked_end;

  /* Change the excess non-default specialists to default. */
  specialist_type_iterate(sp) {
    if (sp == DEFAULT_SPECIALIST) {
      continue;
    }

    for (i = 0; i < pcity->specialists[sp] - result->specialists[sp]; i++) {
      log_apply_result("Change specialist from %d to %d.",
                       sp, DEFAULT_SPECIALIST);
      last_request_id = city_change_specialist(pcity,
					       sp, DEFAULT_SPECIALIST);
      if (first_request_id == 0) {
	first_request_id = last_request_id;
      }
    }
  } specialist_type_iterate_end;

  /* now all surplus people are DEFAULT_SPECIALIST */

  /* Set workers */
  /* FIXME: This code assumes that any toggled worker will turn into a
   * DEFAULT_SPECIALIST! */
  city_tile_iterate_skip_free_worked(city_radius_sq, pcenter, ptile, idx,
                                     x, y) {
    if (NULL == tile_worked(ptile)
     && result->worker_positions[idx]) {
      log_apply_result("Putting worker at {%d,%d}.", x, y);
      fc_assert_action(city_can_work_tile(pcity, ptile), break);

      last_request_id =
        dsend_packet_city_make_worker(&client.conn,
                                      pcity->id, ptile->index);
      if (first_request_id == 0) {
	first_request_id = last_request_id;
      }
    }
  } city_tile_iterate_skip_free_worked_end;

  /* Set all specialists except DEFAULT_SPECIALIST (all the unchanged
   * ones remain as DEFAULT_SPECIALIST). */
  specialist_type_iterate(sp) {
    if (sp == DEFAULT_SPECIALIST) {
      continue;
    }

    for (i = 0; i < result->specialists[sp] - pcity->specialists[sp]; i++) {
      log_apply_result("Changing specialist from %d to %d.",
                       DEFAULT_SPECIALIST, sp);
      last_request_id = city_change_specialist(pcity,
					       DEFAULT_SPECIALIST, sp);
      if (first_request_id == 0) {
	first_request_id = last_request_id;
      }
    }
  } specialist_type_iterate_end;

  if (last_request_id == 0 || ALWAYS_APPLY_AT_SERVER) {
      /*
       * If last_request is 0 no change request was send. But it also
       * means that the results are different or the fc_results_are_equal()
       * test at the start of the function would be true. So this
       * means that the client has other results for the same
       * allocation of citizen than the server. We just send a
       * PACKET_CITY_REFRESH to bring them in sync.
       */
    first_request_id = last_request_id =
	dsend_packet_city_refresh(&client.conn, pcity->id);
    stats.refresh_forced++;
  }

  connection_do_unbuffer(&client.conn);

  if (last_request_id != 0) {
    int city_id = pcity->id;

    wait_for_requests("CMA", first_request_id, last_request_id);
    if (pcity != check_city(city_id, NULL)) {
      log_verbose("apply_result_on_server(city %d) !check_city()!", city_id);
      return FALSE;
    }
  }

  /* Return. */
  cm_result_from_main_map(current_state, pcity);

  success = fc_results_are_equal(current_state, result);
  if (!success) {
    cm_clear_cache(pcity);

#if SHOW_APPLY_RESULT_ON_SERVER_ERRORS
      log_error("apply_result_on_server(city %d=\"%s\") no match!",
                pcity->id, city_name_get(pcity));

      log_test("apply_result_on_server(city %d=\"%s\") have:",
               pcity->id, city_name_get(pcity));
      cm_print_city(pcity);
      cm_print_result(current_state);

      log_test("apply_result_on_server(city %d=\"%s\") want:",
               pcity->id, city_name_get(pcity));
      cm_print_result(result);
#endif /* SHOW_APPLY_RESULT_ON_SERVER_ERRORS */
  }

  cm_result_destroy(current_state);

  log_apply_result("apply_result_on_server() return %d.", (int) success);
  return success;
}

/************************************************************************//**
  Prints the data of the stats struct via log_test(...).
****************************************************************************/
static void report_stats(void)
{
#if SHOW_TIME_STATS
  int total, per_mill;

  total = stats.apply_result_ignored + stats.apply_result_applied;
  per_mill = (stats.apply_result_ignored * 1000) / (total ? total : 1);

  log_test("CMA: apply_result: ignored=%2d.%d%% (%d) "
           "applied=%2d.%d%% (%d) total=%d",
           per_mill / 10, per_mill % 10, stats.apply_result_ignored,
           (1000 - per_mill) / 10, (1000 - per_mill) % 10,
           stats.apply_result_applied, total);
#endif /* SHOW_TIME_STATS */
}

/************************************************************************//**
  Remove governor setting from city.
****************************************************************************/
static void release_city(int city_id)
{
  attr_city_set(ATTR_CITY_CMA_PARAMETER, city_id, 0, NULL);
}

/****************************************************************************
                           algorithmic functions
****************************************************************************/

/************************************************************************//**
  The given city has changed. handle_city ensures that either the city
  follows the set CMA goal or that the CMA detaches itself from the
  city.
****************************************************************************/
static void handle_city(struct city *pcity)
{
  struct cm_result *result = cm_result_new(pcity);
  bool handled;
  int i, city_id = pcity->id;

  log_handle_city("handle_city(city %d=\"%s\") pos=(%d,%d) owner=%s",
                  pcity->id, city_name_get(pcity), TILE_XY(pcity->tile),
                  nation_rule_name(nation_of_city(pcity)));

  log_handle_city2("START handle city %d=\"%s\"",
                   pcity->id, city_name_get(pcity));

  handled = FALSE;
  for (i = 0; i < 5; i++) {
    struct cm_parameter parameter;

    log_handle_city2("  try %d", i);

    if (pcity != check_city(city_id, &parameter)) {
      handled = TRUE;	
      break;
    }

    cm_query_result(pcity, &parameter, result, FALSE);
    if (!result->found_a_valid) {
      log_handle_city2("  no valid found result");

      cma_release_city(pcity);

      create_event(city_tile(pcity), E_CITY_CMA_RELEASE, ftc_client,
                   _("The citizen governor can't fulfill the requirements "
                     "for %s. Passing back control."), city_link(pcity));
      handled = TRUE;
      break;
    } else {
      if (!apply_result_on_server(pcity, result)) {
        log_handle_city2("  doesn't cleanly apply");
        if (pcity == check_city(city_id, NULL) && i == 0) {
          create_event(city_tile(pcity), E_CITY_CMA_RELEASE, ftc_client,
                       _("The citizen governor has gotten confused dealing "
                         "with %s.  You may want to have a look."),
                       city_link(pcity));
        }
      } else {
        log_handle_city2("  ok");
        /* Everything ok */
        handled = TRUE;
        break;
      }
    }
  }

  cm_result_destroy(result);

  if (!handled) {
    fc_assert_ret(pcity == check_city(city_id, NULL));
    log_handle_city2("  not handled");

    create_event(city_tile(pcity), E_CITY_CMA_RELEASE, ftc_client,
                 _("The citizen governor has gotten confused dealing "
                   "with %s.  You may want to have a look."),
                 city_link(pcity));

    cma_release_city(pcity);

    bugreport_request("handle_city() CMA: %s has changed multiple times.",
                      city_name_get(pcity));
  }

  log_handle_city2("END handle city=(%d)", city_id);
}

/************************************************************************//**
  Callback for the agent interface.
****************************************************************************/
static void city_changed(int city_id)
{
  struct city *pcity = game_city_by_number(city_id);

  if (pcity) {
    cm_clear_cache(pcity);
    handle_city(pcity);
  }
}

/************************************************************************//**
  Callback for the agent interface.
****************************************************************************/
static void city_remove(int city_id)
{
  release_city(city_id);
}

/************************************************************************//**
  Callback for the agent interface.
****************************************************************************/
static void new_turn(void)
{
  report_stats();
}

/*************************** public interface ******************************/
/************************************************************************//**
  Initialize city governor code
****************************************************************************/
void cma_init(void)
{
  struct agent self;
  struct timer *timer = stats.wall_timer;

  log_debug("sizeof(struct cm_result)=%d",
            (unsigned int) sizeof(struct cm_result));
  log_debug("sizeof(struct cm_parameter)=%d",
            (unsigned int) sizeof(struct cm_parameter));

  /* reset cache counters */
  memset(&stats, 0, sizeof(stats));

  /* We used to just use timer_new here, but apparently cma_init can be
   * called multiple times per client invocation so that lead to memory
   * leaks. */
  stats.wall_timer = timer_renew(timer, TIMER_USER, TIMER_ACTIVE);

  memset(&self, 0, sizeof(self));
  strcpy(self.name, "CMA");
  self.level = 1;
  self.city_callbacks[CB_CHANGE] = city_changed;
  self.city_callbacks[CB_NEW] = city_changed;
  self.city_callbacks[CB_REMOVE] = city_remove;
  self.turn_start_notify = new_turn;
  register_agent(&self);
}

/************************************************************************//**
  Apply result on server if it's valid
****************************************************************************/
bool cma_apply_result(struct city *pcity, const struct cm_result *result)
{
  fc_assert(!cma_is_city_under_agent(pcity, NULL));
  if (result->found_a_valid) {
    return apply_result_on_server(pcity, result);
  } else
    return TRUE; /* ???????? */
}

/************************************************************************//**
  Put city under governor control
****************************************************************************/
void cma_put_city_under_agent(struct city *pcity,
                              const struct cm_parameter *const parameter)
{
  log_debug("cma_put_city_under_agent(city %d=\"%s\")",
            pcity->id, city_name_get(pcity));

  fc_assert_ret(city_owner(pcity) == client.conn.playing);

  cma_set_parameter(ATTR_CITY_CMA_PARAMETER, pcity->id, parameter);

  cause_a_city_changed_for_agent("CMA", pcity);

  log_debug("cma_put_city_under_agent: return");
}

/************************************************************************//**
  Release city from governor control.
****************************************************************************/
void cma_release_city(struct city *pcity)
{
  release_city(pcity->id);
  refresh_city_dialog(pcity);
  city_report_dialog_update_city(pcity);
}

/************************************************************************//**
  Check whether city is under governor control, and fill parameter if it is.
****************************************************************************/
bool cma_is_city_under_agent(const struct city *pcity,
                             struct cm_parameter *parameter)
{
  struct cm_parameter my_parameter;

  if (!cma_get_parameter(ATTR_CITY_CMA_PARAMETER, pcity->id, &my_parameter)) {
    return FALSE;
  }

  if (parameter) {
    memcpy(parameter, &my_parameter, sizeof(struct cm_parameter));
  }
  return TRUE;
}

/************************************************************************//**
  Get the parameter.

  Don't bother to cm_init_parameter, since we set all the fields anyway.
  But leave the comment here so we can find this place when searching
  for all the creators of a parameter.
****************************************************************************/
bool cma_get_parameter(enum attr_city attr, int city_id,
                       struct cm_parameter *parameter)
{
  size_t len;
  char buffer[SAVED_PARAMETER_SIZE];
  struct data_in din;
  int version, dummy;

  /* Changing this function is likely to break compatability with old
   * savegames that store these values. */

  len = attr_city_get(attr, city_id, sizeof(buffer), buffer);
  if (len == 0) {
    return FALSE;
  }
  fc_assert_ret_val(len == SAVED_PARAMETER_SIZE, FALSE);

  dio_input_init(&din, buffer, len);

  dio_get_uint8_raw(&din, &version);
  fc_assert_ret_val(version == 2, FALSE);

  /* Initialize the parameter (includes some AI-only fields that aren't
   * touched below). */
  cm_init_parameter(parameter);

  output_type_iterate(i) {
    dio_get_sint16_raw(&din, &parameter->minimal_surplus[i]);
    dio_get_sint16_raw(&din, &parameter->factor[i]);
  } output_type_iterate_end;

  dio_get_sint16_raw(&din, &parameter->happy_factor);
  dio_get_uint8_raw(&din, &dummy); /* Dummy value; used to be factor_target. */
  dio_get_bool8_raw(&din, &parameter->require_happy);

  return TRUE;
}

/************************************************************************//**
  Set attribute block for city from parameter.
****************************************************************************/
void cma_set_parameter(enum attr_city attr, int city_id,
                       const struct cm_parameter *parameter)
{
  char buffer[SAVED_PARAMETER_SIZE];
  struct raw_data_out dout;

  /* Changing this function is likely to break compatability with old
   * savegames that store these values. */

  dio_output_init(&dout, buffer, sizeof(buffer));

  dio_put_uint8_raw(&dout, 2);

  output_type_iterate(i) {
    dio_put_sint16_raw(&dout, parameter->minimal_surplus[i]);
    dio_put_sint16_raw(&dout, parameter->factor[i]);
  } output_type_iterate_end;

  dio_put_sint16_raw(&dout, parameter->happy_factor);
  dio_put_uint8_raw(&dout, 0); /* Dummy value; used to be factor_target. */
  dio_put_bool8_raw(&dout, parameter->require_happy);

  fc_assert(dio_output_used(&dout) == SAVED_PARAMETER_SIZE);

  attr_city_set(attr, city_id, SAVED_PARAMETER_SIZE, buffer);
}
