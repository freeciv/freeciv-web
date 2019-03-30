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

#include <stdarg.h>
#include <string.h>

/* utility */
#include "capability.h"
#include "log.h"
#include "mem.h"
#include "timing.h"

/* client */
#include "client_main.h"

/* include */
#include "mapctrl_g.h"

/* agents */
#include "cma_core.h"
#include "cma_fec.h"
#include "sha.h"

#include "agents.h"

#define log_request_ids(...)            /* log_test(__VA_ARGS__) */
#define log_todo_lists(...)             /* log_test(__VA_ARGS__) */
#define log_meta_callback(...)          log_debug(__VA_ARGS__)
#define log_debug_freeze(...)           /* log_test(__VA_ARGS__) */

#define MAX_AGENTS			10

struct my_agent;

struct call {
  struct my_agent *agent;
  enum oct { OCT_NEW_TURN, OCT_UNIT, OCT_CITY, OCT_TILE } type;
  enum callback_type cb_type;
  int arg;
};

#define SPECLIST_TAG call
#define SPECLIST_TYPE struct call
#include "speclist.h"

#define call_list_iterate(calllist, pcall) \
    TYPED_LIST_ITERATE(struct call, calllist, pcall)
#define call_list_iterate_end  LIST_ITERATE_END

#define call_list_both_iterate(calllist, plink, pcall) \
    TYPED_LIST_BOTH_ITERATE(struct call_list_link, struct call, \
                            calllist, plink, pcall)
#define call_list_both_iterate_end  LIST_BOTH_ITERATE_END

/*
 * Main data structure. Contains all registered agents and all
 * outstanding calls.
 */
static struct {
  int entries_used;
  struct my_agent {
    struct agent agent;
    int first_outstanding_request_id, last_outstanding_request_id;
    struct {
      struct timer *network_wall_timer;
      int wait_at_network, wait_at_network_requests;
    } stats;
  } entries[MAX_AGENTS];
  struct call_list *calls;
} agents;

static bool initialized = FALSE;
static int frozen_level;
static bool currently_running = FALSE;

/************************************************************************//**
  Return TRUE iff the two agent calls are equal.
****************************************************************************/
static bool calls_are_equal(const struct call *pcall1,
                            const struct call *pcall2)
{
  if (pcall1->agent != pcall2->agent) {
    return FALSE;
  }

  if (pcall1->type != pcall2->type && pcall1->cb_type != pcall2->cb_type) {
    return FALSE;
  }

  switch (pcall1->type) {
  case OCT_UNIT:
  case OCT_CITY:
  case OCT_TILE:
    return (pcall1->arg == pcall2->arg);
  case OCT_NEW_TURN:
    return TRUE;
  }

  log_error("Unsupported call type %d.", pcall1->type);
  return FALSE;
}

/************************************************************************//**
  If the call described by the given arguments isn't contained in
  agents.calls list, add the call to this list.
  Maintains the list in a sorted order.
****************************************************************************/
static void enqueue_call(enum oct type,
                         enum callback_type cb_type,
                         struct my_agent *agent, ...)
{
  va_list ap;
  struct call *pcall2;
  int arg = 0;
  const struct tile *ptile;
  bool added = FALSE;

  va_start(ap, agent);

  if (client_is_observer()) {
    return;
  }

  switch (type) {
  case OCT_UNIT:
  case OCT_CITY:
    arg = va_arg(ap, int);
    break;
  case OCT_TILE:
    ptile = va_arg(ap, const struct tile *);
    arg = tile_index(ptile);
    break;
  case OCT_NEW_TURN:
    /* nothing */
    break;
  }
  va_end(ap);

  pcall2 = fc_malloc(sizeof(struct call));

  pcall2->agent = agent;
  pcall2->type = type;
  pcall2->cb_type = cb_type;
  pcall2->arg = arg;

  /* Ensure list is sorted so that calls to agents with lower levels
   * come first, since that's how we'll want to pop them */
  call_list_both_iterate(agents.calls, plink, pcall) {
    if (calls_are_equal(pcall, pcall2)) {
      /* Already got one like this, discard duplicate. */
      free(pcall2);
      return;
    }
    if (pcall->agent->agent.level - pcall2->agent->agent.level > 0) {
      /* Found a level greater than ours. Can assume that calls_are_equal()
       * will never be true from here on, since list is sorted by level and
       * unequal levels => unequal agents => !calls_are_equal().
       * Insert into list here. */
      call_list_insert_before(agents.calls, pcall2, plink);
      added = TRUE;
      break;
    }
  } call_list_both_iterate_end;

  if (!added) {
    call_list_append(agents.calls, pcall2);
  }

  log_todo_lists("A: adding call");

  /* agents_busy() may have changed */
  update_turn_done_button_state();
}

/************************************************************************//**
  Return an outstanding call. The call is removed from the agents.calls
  list. Returns NULL if there no more outstanding calls.
****************************************************************************/
static struct call *remove_and_return_a_call(void)
{
  struct call *result;

  if (call_list_size(agents.calls) == 0) {
    return NULL;
  }

  result = call_list_front(agents.calls);
  call_list_pop_front(agents.calls);

  log_todo_lists("A: removed call");
  return result;
}

/************************************************************************//**
  Calls an callback of an agent as described in the given call.
****************************************************************************/
static void execute_call(const struct call *call)
{
  switch (call->type) {
  case OCT_NEW_TURN:
    call->agent->agent.turn_start_notify();
    break;
  case OCT_UNIT:
    call->agent->agent.unit_callbacks[call->cb_type] (call->arg);
    break;
  case OCT_CITY:
    call->agent->agent.city_callbacks[call->cb_type] (call->arg);
    break;
  case OCT_TILE:
    call->agent->agent.tile_callbacks[call->cb_type]
      (index_to_tile(&(wld.map), call->arg));
    break;
  }
}

/************************************************************************//**
  Execute all outstanding calls. This method will do nothing if the
  dispatching is frozen (frozen_level > 0). Also call_handle_methods
  will ensure that only one instance is running at any given time.
****************************************************************************/
static void call_handle_methods(void)
{
  if (currently_running) {
    return;
  }
  if (frozen_level > 0) {
    return;
  }
  currently_running = TRUE;

  /*
   * The following should ensure that the methods of agents which have
   * a lower level are called first.
   */
  for (;;) {
    struct call *pcall;

    pcall = remove_and_return_a_call();
    if (!pcall) {
      break;
    }

    execute_call(pcall);
    free(pcall);
  }

  currently_running = FALSE;

  update_turn_done_button_state();
}

/************************************************************************//**
  Increase the frozen_level by one.
****************************************************************************/
static void freeze(void)
{
  if (!initialized) {
    frozen_level = 0;
    initialized = TRUE;
  }
  log_debug_freeze("A: freeze() current level=%d", frozen_level);
  frozen_level++;
}

/************************************************************************//**
  Decrease the frozen_level by one. If the dispatching is not frozen
  anymore (frozen_level == 0) all outstanding calls are executed.
****************************************************************************/
static void thaw(void)
{
  log_debug_freeze("A: thaw() current level=%d", frozen_level);
  frozen_level--;
  fc_assert(frozen_level >= 0);
  if (0 == frozen_level && C_S_RUNNING == client_state()) {
    call_handle_methods();
  }
}

/************************************************************************//**
  Helper.
****************************************************************************/
static struct my_agent *agent_by_name(const char *agent_name)
{
  int i;

  for (i = 0; i < agents.entries_used; i++) {
    if (strcmp(agent_name, agents.entries[i].agent.name) == 0)
      return &agents.entries[i];
  }

  return NULL;
}

/************************************************************************//**
  Returns TRUE iff currently handled packet was caused by the given
  agent.
****************************************************************************/
static bool is_outstanding_request(struct my_agent *agent)
{
  if (agent->first_outstanding_request_id != 0 &&
      client.conn.client.request_id_of_currently_handled_packet != 0 &&
      agent->first_outstanding_request_id <=
      client.conn.client.request_id_of_currently_handled_packet &&
      agent->last_outstanding_request_id >=
      client.conn.client.request_id_of_currently_handled_packet) {
    log_debug("A:%s: ignoring packet; outstanding [%d..%d] got=%d",
              agent->agent.name, agent->first_outstanding_request_id,
              agent->last_outstanding_request_id,
              client.conn.client.request_id_of_currently_handled_packet);
    return TRUE;
  }
  return FALSE;
}

/************************************************************************//**
  Called once per client startup.
****************************************************************************/
void agents_init(void)
{
  agents.entries_used = 0;
  agents.calls = call_list_new();

  /* Add init calls of agents here */
  cma_init();
  cmafec_init();
  /*simple_historian_init();*/
}

/************************************************************************//**
  Free resources allocated for agents framework
****************************************************************************/
void agents_free(void)
{
  int i;

  /* FIXME: doing this will wipe out any presets on disconnect.
   * a proper solution should be to split up the client_free functions 
   * for a simple disconnect and a client quit. for right now, we just
   * let the OS free the memory on exit instead of doing it ourselves. */
  /* cmafec_free(); */

  /*simple_historian_done();*/

  for (;;) {
    struct call *pcall = remove_and_return_a_call();
    if (!pcall) {
      break;
    }

    free(pcall);
  }

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    timer_destroy(agent->stats.network_wall_timer);
  }
  call_list_destroy(agents.calls);
}

/************************************************************************//**
  Registers an agent.
****************************************************************************/
void register_agent(const struct agent *agent)
{
  struct my_agent *priv_agent = &agents.entries[agents.entries_used];

  fc_assert_ret(agents.entries_used < MAX_AGENTS);
  fc_assert_ret(agent->level > 0);

  memcpy(&priv_agent->agent, agent, sizeof(struct agent));

  priv_agent->first_outstanding_request_id = 0;
  priv_agent->last_outstanding_request_id = 0;

  priv_agent->stats.network_wall_timer = timer_new(TIMER_USER, TIMER_ACTIVE);
  priv_agent->stats.wait_at_network = 0;
  priv_agent->stats.wait_at_network_requests = 0;

  agents.entries_used++;
}

/************************************************************************//**
  Called from client/packhand.c.
****************************************************************************/
void agents_disconnect(void)
{
  log_meta_callback("agents_disconnect()");
  initialized = FALSE;
}

/************************************************************************//**
  Called from client/packhand.c.
****************************************************************************/
void agents_processing_started(void)
{
  log_meta_callback("agents_processing_started()");
  freeze();
}

/************************************************************************//**
  Called from client/packhand.c.
****************************************************************************/
void agents_processing_finished(void)
{
  log_meta_callback("agents_processing_finished()");
  thaw();
}

/************************************************************************//**
  Called from client/packhand.c.
****************************************************************************/
void agents_freeze_hint(void)
{
  log_meta_callback("agents_freeze_hint()");
  freeze();
}

/************************************************************************//**
  Called from client/packhand.c.
****************************************************************************/
void agents_thaw_hint(void)
{
  log_meta_callback("agents_thaw_hint()");
  thaw();
}

/************************************************************************//**
  Called from client/packhand.c.
****************************************************************************/
void agents_game_joined(void)
{
  log_meta_callback("agents_game_joined()");
}

/************************************************************************//**
  Called from client/packhand.c.
****************************************************************************/
void agents_game_start(void)
{
  log_meta_callback("agents_game_start()");
  call_handle_methods();
}

/************************************************************************//**
  Called from client/packhand.c.
****************************************************************************/
void agents_before_new_turn(void)
{
  log_meta_callback("agents_before_new_turn()");
}

/************************************************************************//**
  Called from client/packhand.c.
****************************************************************************/
void agents_start_turn(void)
{
  log_meta_callback("agents_start_turn()");
}

/************************************************************************//**
  Called from client/packhand.c. See agents_unit_changed() for a generic
  documentation.
****************************************************************************/
void agents_new_turn(void)
{
  int i;

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (is_outstanding_request(agent)) {
      continue;
    }
    if (agent->agent.turn_start_notify) {
      enqueue_call(OCT_NEW_TURN, CB_LAST, agent);
    }
  }
  /*
   * call_handle_methods() isn't called here because the agents are
   * frozen anyway.
   */
}

/************************************************************************//**
  Called from client/packhand.c. A call is created and added to the
  list of outstanding calls if an agent wants to be informed about this
  event and the change wasn't caused by the agent. We then try (this
  may not be successful in every case since we can be frozen or another
  call_handle_methods may be running higher up on the stack) to execute
  all outstanding calls.
****************************************************************************/
void agents_unit_changed(struct unit *punit)
{
  int i;

  log_debug("A: agents_unit_changed(unit=%d) type=%s pos=(%d,%d) owner=%s",
            punit->id, unit_rule_name(punit), TILE_XY(unit_tile(punit)),
            player_name(unit_owner(punit)));

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (is_outstanding_request(agent)) {
      continue;
    }
    if (agent->agent.unit_callbacks[CB_CHANGE]) {
      enqueue_call(OCT_UNIT, CB_CHANGE, agent, punit->id);
    }
  }
  call_handle_methods();
}

/************************************************************************//**
  Called from client/packhand.c. See agents_unit_changed() for a generic
  documentation.
****************************************************************************/
void agents_unit_new(struct unit *punit)
{
  int i;

  log_debug("A: agents_new_unit(unit=%d) type=%s pos=(%d,%d) owner=%s",
            punit->id, unit_rule_name(punit), TILE_XY(unit_tile(punit)),
            player_name(unit_owner(punit)));

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (is_outstanding_request(agent)) {
      continue;
    }
    if (agent->agent.unit_callbacks[CB_NEW]) {
      enqueue_call(OCT_UNIT, CB_NEW, agent, punit->id);
    }
  }

  call_handle_methods();
}

/************************************************************************//**
  Called from client/packhand.c. See agents_unit_changed() for a generic
  documentation.
****************************************************************************/
void agents_unit_remove(struct unit *punit)
{
  int i;

  log_debug("A: agents_remove_unit(unit=%d) type=%s pos=(%d,%d) owner=%s",
            punit->id, unit_rule_name(punit), TILE_XY(unit_tile(punit)),
            player_name(unit_owner(punit)));

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (is_outstanding_request(agent)) {
      continue;
    }
    if (agent->agent.unit_callbacks[CB_REMOVE]) {
      enqueue_call(OCT_UNIT, CB_REMOVE, agent, punit->id);
    }
  }

  call_handle_methods();
}

/************************************************************************//**
  Called from client/packhand.c. See agents_unit_changed() for a generic
  documentation.
****************************************************************************/
void agents_city_changed(struct city *pcity)
{
  int i;

  log_debug("A: agents_city_changed(city %d=\"%s\") owner=%s",
            pcity->id, city_name_get(pcity),
            nation_rule_name(nation_of_city(pcity)));

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (is_outstanding_request(agent)) {
      continue;
    }
    if (agent->agent.city_callbacks[CB_CHANGE]) {
      enqueue_call(OCT_CITY, CB_CHANGE, agent, pcity->id);
    }
  }

  call_handle_methods();
}

/************************************************************************//**
  Called from client/packhand.c. See agents_unit_changed() for a generic
  documentation.
****************************************************************************/
void agents_city_new(struct city *pcity)
{
  int i;

  log_debug("A: agents_city_new(city %d=\"%s\") pos=(%d,%d) owner=%s",
            pcity->id, city_name_get(pcity), TILE_XY(pcity->tile),
            nation_rule_name(nation_of_city(pcity)));

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (is_outstanding_request(agent)) {
      continue;
    }
    if (agent->agent.city_callbacks[CB_NEW]) {
      enqueue_call(OCT_CITY, CB_NEW, agent, pcity->id);
    }
  }

  call_handle_methods();
}

/************************************************************************//**
  Called from client/packhand.c. See agents_unit_changed() for a generic
  documentation.
****************************************************************************/
void agents_city_remove(struct city *pcity)
{
  int i;

  log_debug("A: agents_city_remove(city %d=\"%s\") pos=(%d,%d) owner=%s",
            pcity->id, city_name_get(pcity), TILE_XY(pcity->tile),
            nation_rule_name(nation_of_city(pcity)));

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (is_outstanding_request(agent)) {
      continue;
    }
    if (agent->agent.city_callbacks[CB_REMOVE]) {
      enqueue_call(OCT_CITY, CB_REMOVE, agent, pcity->id);
    }
  }

  call_handle_methods();
}

/************************************************************************//**
  Called from client/packhand.c. See agents_unit_changed() for a generic
  documentation.
  Tiles got removed because of FOW.
****************************************************************************/
void agents_tile_remove(struct tile *ptile)
{
  int i;

  log_debug("A: agents_tile_remove(tile=(%d, %d))", TILE_XY(ptile));

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (is_outstanding_request(agent)) {
      continue;
    }
    if (agent->agent.tile_callbacks[CB_REMOVE]) {
      enqueue_call(OCT_TILE, CB_REMOVE, agent, ptile);
    }
  }

  call_handle_methods();
}

/************************************************************************//**
  Called from client/packhand.c. See agents_unit_changed() for a generic
  documentation.
****************************************************************************/
void agents_tile_changed(struct tile *ptile)
{
  int i;

  log_debug("A: agents_tile_changed(tile=(%d, %d))", TILE_XY(ptile));

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (is_outstanding_request(agent)) {
      continue;
    }
    if (agent->agent.tile_callbacks[CB_CHANGE]) {
      enqueue_call(OCT_TILE, CB_CHANGE, agent, ptile);
    }
  }

  call_handle_methods();
}

/************************************************************************//**
  Called from client/packhand.c. See agents_unit_changed() for a generic
  documentation.
****************************************************************************/
void agents_tile_new(struct tile *ptile)
{
  int i;

  log_debug("A: agents_tile_new(tile=(%d, %d))", TILE_XY(ptile));

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (is_outstanding_request(agent)) {
      continue;
    }
    if (agent->agent.tile_callbacks[CB_NEW]) {
      enqueue_call(OCT_TILE, CB_NEW, agent, ptile);
    }
  }

  call_handle_methods();
}

/************************************************************************//**
  Called from an agent. This function will return until the last
  request has been processed by the server.
****************************************************************************/
void wait_for_requests(const char *agent_name, int first_request_id,
                       int last_request_id)
{
  struct my_agent *agent = agent_by_name(agent_name);

  log_request_ids("A:%s: wait_for_request(ids=[%d..%d])",
                  agent->agent.name, first_request_id, last_request_id);

  fc_assert_ret(first_request_id != 0 && last_request_id != 0
                && first_request_id <= last_request_id);
  fc_assert_ret(agent->first_outstanding_request_id == 0);
  agent->first_outstanding_request_id = first_request_id;
  agent->last_outstanding_request_id = last_request_id;

  timer_start(agent->stats.network_wall_timer);
  wait_till_request_got_processed(last_request_id);
  timer_stop(agent->stats.network_wall_timer);

  agent->stats.wait_at_network++;
  agent->stats.wait_at_network_requests +=
      (1 + (last_request_id - first_request_id));

  log_request_ids("A:%s: wait_for_request: ids=[%d..%d]; got it",
                  agent->agent.name, first_request_id, last_request_id);

  agent->first_outstanding_request_id = 0;

  log_debug("A:%s: waited %fs in total for network; "
            "requests=%d; waited %d times",
            agent->agent.name,
            timer_read_seconds(agent->stats.network_wall_timer),
            agent->stats.wait_at_network_requests,
            agent->stats.wait_at_network);
}

/************************************************************************//**
  Adds a specific call for the given agent.
****************************************************************************/
void cause_a_unit_changed_for_agent(const char *name_of_calling_agent,
                                    struct unit *punit)
{
  struct my_agent *agent = agent_by_name(name_of_calling_agent);

  fc_assert_ret(agent->agent.unit_callbacks[CB_CHANGE] != NULL);
  enqueue_call(OCT_UNIT, CB_CHANGE, agent, punit->id);
  call_handle_methods();
}

/************************************************************************//**
  Adds a specific call for the given agent.
****************************************************************************/
void cause_a_city_changed_for_agent(const char *name_of_calling_agent,
                                    struct city *pcity)
{
  struct my_agent *agent = agent_by_name(name_of_calling_agent);

  fc_assert_ret(agent->agent.city_callbacks[CB_CHANGE] != NULL);
  enqueue_call(OCT_CITY, CB_CHANGE, agent, pcity->id);
  call_handle_methods();
}

/************************************************************************//**
  Returns TRUE iff some agent is currently busy.
****************************************************************************/
bool agents_busy(void)
{
  int i;

  if (!initialized) {
    return FALSE;
  }

  if (call_list_size(agents.calls) > 0 || frozen_level > 0
      || currently_running) {
    return TRUE;
  }

  for (i = 0; i < agents.entries_used; i++) {
    struct my_agent *agent = &agents.entries[i];

    if (agent->first_outstanding_request_id != 0) {
      return TRUE;
    }
  }
  return FALSE;
}
