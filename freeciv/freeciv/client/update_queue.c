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
#include "support.h"            /* bool */

/* common */
#include "city.h"
#include "connection.h"
#include "player.h"

/* client/include */
#include "canvas_g.h"
#include "citydlg_g.h"
#include "cityrep_g.h"
#include "dialogs_g.h"
#include "gui_main_g.h"
#include "menu_g.h"
#include "pages_g.h"
#include "plrdlg_g.h"
#include "ratesdlg_g.h"
#include "repodlgs_g.h"

/* client */
#include "client_main.h"
#include "connectdlg_common.h"
#include "options.h"
#include "zoom.h"

#include "update_queue.h"


/* Data type in 'update_queue'. */
struct update_queue_data {
  void *data;
  uq_free_fn_t free_data_func;
};

static void update_queue_data_destroy(struct update_queue_data *pdata);

/* 'struct update_queue_hash' and related functions. */
#define SPECHASH_TAG update_queue
#define SPECHASH_IKEY_TYPE uq_callback_t
#define SPECHASH_IDATA_TYPE struct update_queue_data *
#define SPECHASH_IDATA_FREE update_queue_data_destroy
#include "spechash.h"
#define update_queue_hash_iterate(hash, callback, uq_data)                  \
  TYPED_HASH_ITERATE(uq_callback_t, const struct update_queue_data *,       \
                     hash, callback, uq_data)
#define update_queue_hash_iterate_end HASH_ITERATE_END

/* Type of data listed in 'processing_started_waiting_queue' and
 * 'processing_finished_waiting_queue'. Real type is
 * 'struct waiting_queue_list'. */
struct waiting_queue_data {
  uq_callback_t callback;
  struct update_queue_data *uq_data;
};

/* 'struct waiting_queue_list' and related functions. */
#define SPECLIST_TAG waiting_queue
#define SPECLIST_TYPE struct waiting_queue_data
#include "speclist.h"
#define waiting_queue_list_iterate(list, data)                              \
  TYPED_LIST_ITERATE(struct waiting_queue_data, list, data)
#define waiting_queue_list_iterate_end LIST_ITERATE_END

/* 'struct waiting_queue_hash' and related functions. */
#define SPECHASH_TAG waiting_queue
#define SPECHASH_INT_KEY_TYPE
#define SPECHASH_IDATA_TYPE struct waiting_queue_list *
#define SPECHASH_IDATA_FREE waiting_queue_list_destroy
#include "spechash.h"

static struct update_queue_hash *update_queue = NULL;
static struct waiting_queue_hash *processing_started_waiting_queue = NULL;
static struct waiting_queue_hash *processing_finished_waiting_queue = NULL;
static int update_queue_frozen_level = 0;
static bool update_queue_has_idle_callback = FALSE;

static void update_unqueue(void *data);
static inline void update_queue_push(uq_callback_t callback,
                                     struct update_queue_data *uq_data);

/************************************************************************//**
  Create a new update queue data.
****************************************************************************/
static inline struct update_queue_data *
update_queue_data_new(void *data, uq_free_fn_t free_data_func)
{
  struct update_queue_data *uq_data = fc_malloc(sizeof(*uq_data));

  uq_data->data = data;
  uq_data->free_data_func = free_data_func;
  return uq_data;
}

/************************************************************************//**
  Free a update queue data.
****************************************************************************/
static void update_queue_data_destroy(struct update_queue_data *uq_data)
{
  fc_assert_ret(NULL != uq_data);
  if (NULL != uq_data->free_data_func) {
    uq_data->free_data_func(uq_data->data);
  }
  free(uq_data);
}

/************************************************************************//**
  Create a new waiting queue data.
****************************************************************************/
static inline struct waiting_queue_data *
waiting_queue_data_new(uq_callback_t callback, void *data,
                       uq_free_fn_t free_data_func)
{
  struct waiting_queue_data *wq_data = fc_malloc(sizeof(*wq_data));

  wq_data->callback = callback;
  wq_data->uq_data = update_queue_data_new(data, free_data_func);
  return wq_data;
}

/************************************************************************//**
  Free a waiting queue data.
****************************************************************************/
static void waiting_queue_data_destroy(struct waiting_queue_data *wq_data)
{
  fc_assert_ret(NULL != wq_data);
  if (NULL != wq_data->uq_data) {
    /* May be NULL, see waiting_queue_data_extract(). */
    update_queue_data_destroy(wq_data->uq_data);
  }
  free(wq_data);
}

/************************************************************************//**
  Extract the update_queue_data from the waiting queue data.
****************************************************************************/
static inline struct update_queue_data *
waiting_queue_data_extract(struct waiting_queue_data *wq_data)
{
  struct update_queue_data *uq_data = wq_data->uq_data;

  wq_data->uq_data = NULL;
  return uq_data;
}


/************************************************************************//**
  Initialize the update queue.
****************************************************************************/
void update_queue_init(void)
{
  if (NULL !=  update_queue) {
    /* Already initialized. */
    fc_assert(NULL != processing_started_waiting_queue);
    fc_assert(NULL != processing_finished_waiting_queue);
    return;
  }
  fc_assert(NULL == processing_started_waiting_queue);
  fc_assert(NULL == processing_finished_waiting_queue);

  update_queue = update_queue_hash_new();
  processing_started_waiting_queue = waiting_queue_hash_new();
  processing_finished_waiting_queue = waiting_queue_hash_new();
  update_queue_frozen_level = 0;
  update_queue_has_idle_callback = FALSE;
}

/************************************************************************//**
  Free the update queue.
****************************************************************************/
void update_queue_free(void)
{
  fc_assert(NULL != update_queue);
  fc_assert(NULL != processing_started_waiting_queue);
  fc_assert(NULL != processing_finished_waiting_queue);

  if (NULL != update_queue) {
    update_queue_hash_destroy(update_queue);
    update_queue = NULL;
  }
  if (NULL != processing_started_waiting_queue) {
    waiting_queue_hash_destroy(processing_started_waiting_queue);
    processing_started_waiting_queue = NULL;
  }
  if (NULL != processing_finished_waiting_queue) {
    waiting_queue_hash_destroy(processing_finished_waiting_queue);
    processing_finished_waiting_queue = NULL;
  }

  update_queue_frozen_level = 0;
  update_queue_has_idle_callback = FALSE;
}

/************************************************************************//**
  Freezes the update queue.
****************************************************************************/
void update_queue_freeze(void)
{
  update_queue_frozen_level++;
}

/************************************************************************//**
  Free the update queue.
****************************************************************************/
void update_queue_thaw(void)
{
  update_queue_frozen_level--;
  if (0 == update_queue_frozen_level
      && !update_queue_has_idle_callback
      && NULL != update_queue
      && 0 < update_queue_hash_size(update_queue)) {
    update_queue_has_idle_callback = TRUE;
    add_idle_callback(update_unqueue, NULL);
  } else if (0 > update_queue_frozen_level) {
    log_error("update_queue_frozen_level < 0, reparing...");
    update_queue_frozen_level = 0;
  }
}

/************************************************************************//**
  Free the update queue.
****************************************************************************/
void update_queue_force_thaw(void)
{
  while (update_queue_is_frozen()) {
    update_queue_thaw();
  }
}

/************************************************************************//**
  Free the update queue.
****************************************************************************/
bool update_queue_is_frozen(void)
{
  return (0 < update_queue_frozen_level);
}

/************************************************************************//**
  Moves the instances waiting to the request_id to the callback queue.
****************************************************************************/
static inline void
waiting_queue_execute_pending_requests(struct waiting_queue_hash *hash,
                                       int request_id)
{
  struct waiting_queue_list *list;

  if (NULL == hash || !waiting_queue_hash_lookup(hash, request_id, &list)) {
    return;
  }

  waiting_queue_list_iterate(list, wq_data) {
    update_queue_push(wq_data->callback, waiting_queue_data_extract(wq_data));
  } waiting_queue_list_iterate_end;
  waiting_queue_hash_remove(hash, request_id);
}

/************************************************************************//**
  Moves the instances waiting to the request_id to the callback queue.
****************************************************************************/
void update_queue_processing_started(int request_id)
{
  waiting_queue_execute_pending_requests(processing_started_waiting_queue,
                                         request_id);
}

/************************************************************************//**
  Moves the instances waiting to the request_id to the callback queue.
****************************************************************************/
void update_queue_processing_finished(int request_id)
{
  waiting_queue_execute_pending_requests(processing_finished_waiting_queue,
                                         request_id);
}

/************************************************************************//**
  Unqueue all updates.
****************************************************************************/
static void update_unqueue(void *data)
{
  struct update_queue_hash *hash;

  if (NULL == update_queue) {
    update_queue_has_idle_callback = FALSE;
    return;
  }

  if (update_queue_is_frozen()) {
    /* Cannot update now, let's add it again. */
    update_queue_has_idle_callback = FALSE;
    return;
  }

  /* Replace the old list, don't do recursive stuff, and don't write in the
   * hash table when we are reading it. */
  hash = update_queue;
  update_queue = update_queue_hash_new();
  update_queue_has_idle_callback = FALSE;

  /* Invoke callbacks. */
  update_queue_hash_iterate(hash, callback, uq_data) {
    callback(uq_data->data);
  } update_queue_hash_iterate_end;
  update_queue_hash_destroy(hash);
}

/************************************************************************//**
  Add a callback to the update queue. NB: you can only set a callback
  once. Setting a callback twice will overwrite the previous.
****************************************************************************/
static inline void update_queue_push(uq_callback_t callback,
                                     struct update_queue_data *uq_data)
{
  update_queue_hash_replace(update_queue, callback, uq_data);

  if (!update_queue_has_idle_callback
      && !update_queue_is_frozen()) {
    update_queue_has_idle_callback = TRUE;
    add_idle_callback(update_unqueue, NULL);
  }
}

/************************************************************************//**
  Add a callback to the update queue. NB: you can only set a callback
  once. Setting a callback twice will overwrite the previous.
****************************************************************************/
void update_queue_add(uq_callback_t callback, void *data)
{
  if (NULL != update_queue) {
    update_queue_push(callback, update_queue_data_new(data, NULL));
  }
}

/************************************************************************//**
  Add a callback to the update queue. NB: you can only set a callback
  once. Setting a callback twice will overwrite the previous.
****************************************************************************/
void update_queue_add_full(uq_callback_t callback, void *data,
                           uq_free_fn_t free_data_func)
{
  if (NULL != update_queue) {
    update_queue_push(callback, update_queue_data_new(data, free_data_func));
  }
}

/************************************************************************//**
  Returns whether this callback is listed in the update queue.
****************************************************************************/
bool update_queue_has_callback(uq_callback_t callback)
{
  return (NULL != update_queue
          && update_queue_hash_lookup(update_queue, callback, NULL));
}

/************************************************************************//**
  Returns whether this callback is listed in the update queue and
  get the its data and free function. 'data' and 'free_data_func' can
  be NULL.
****************************************************************************/
bool update_queue_has_callback_full(uq_callback_t callback,
                                    const void **data,
                                    uq_free_fn_t *free_data_func)
{
  if (NULL != update_queue) {
    struct update_queue_data *uq_data;

    if (update_queue_hash_lookup(update_queue, callback, &uq_data)) {
      if (NULL != data) {
        *data = uq_data->data;
      }
      if (NULL != free_data_func) {
        *free_data_func = uq_data->free_data_func;
      }
      return TRUE;
    }
  }
  return FALSE;
}

/************************************************************************//**
  Connects the callback to a network event.
****************************************************************************/
static inline void
waiting_queue_add_pending_request(struct waiting_queue_hash *hash,
                                  int request_id, uq_callback_t callback,
                                  void *data, uq_free_fn_t free_data_func)
{
  if (NULL != hash) {
    struct waiting_queue_list *list;

    if (!waiting_queue_hash_lookup(hash, request_id, &list)) {
      /* The list doesn't exist yet for that request, create it. */
      list = waiting_queue_list_new_full(waiting_queue_data_destroy);
      waiting_queue_hash_insert(hash, request_id, list);
    }
    waiting_queue_list_append(list, waiting_queue_data_new(callback, data,
                                                           free_data_func));
  }
}

/************************************************************************//**
  Connects the callback to the start of the processing (in server side) of
  the request.
****************************************************************************/
void update_queue_connect_processing_started(int request_id,
                                             uq_callback_t callback,
                                             void *data)
{
  waiting_queue_add_pending_request(processing_started_waiting_queue,
                                    request_id, callback, data, NULL);
}

/************************************************************************//**
  Connects the callback to the start of the processing (in server side) of
  the request.
****************************************************************************/
void update_queue_connect_processing_started_full(int request_id,
                                                  uq_callback_t callback,
                                                  void *data,
                                                  uq_free_fn_t
                                                  free_data_func)
{
  waiting_queue_add_pending_request(processing_started_waiting_queue,
                                    request_id, callback, data,
                                    free_data_func);
}

/************************************************************************//**
  Connects the callback to the end of the processing (in server side) of
  the request.
****************************************************************************/
void update_queue_connect_processing_finished(int request_id,
                                              uq_callback_t callback,
                                              void *data)
{
  waiting_queue_add_pending_request(processing_finished_waiting_queue,
                                    request_id, callback, data, NULL);
}

/************************************************************************//**
  Connects the callback to the end of the processing (in server side) of
  the request.
****************************************************************************/
void update_queue_connect_processing_finished_full(int request_id,
                                                   uq_callback_t callback,
                                                   void *data,
                                                   uq_free_fn_t
                                                   free_data_func)
{
  waiting_queue_add_pending_request(processing_finished_waiting_queue,
                                    request_id, callback, data,
                                    free_data_func);
}

/************************************************************************//**
  Set the client page.
****************************************************************************/
static void set_client_page_callback(void *data)
{
  enum client_pages page = FC_PTR_TO_INT(data);

  real_set_client_page(page);

  if (page == PAGE_GAME) {
    if (has_zoom_support()) {
      if (gui_options.zoom_set) {
        zoom_set(gui_options.zoom_default_level);
      } else {
        zoom_1_0();
      }
    }
  }
}

/************************************************************************//**
  Set the client page.
****************************************************************************/
void set_client_page(enum client_pages page)
{
  log_debug("Requested page: %s.", client_pages_name(page));

  update_queue_add(set_client_page_callback, FC_INT_TO_PTR(page));
}

/************************************************************************//**
  Start server and then, set the client page.
****************************************************************************/
void client_start_server_and_set_page(enum client_pages page)
{
  log_debug("Requested server start + page: %s.", client_pages_name(page));

  if (client_start_server()) {
    update_queue_connect_processing_finished(client.conn.client.last_request_id_used,
                                             set_client_page_callback,
                                             FC_INT_TO_PTR(page));
  }
}

/************************************************************************//**
  Returns the next client page.
****************************************************************************/
enum client_pages get_client_page(void)
{
  const void *data;

  if (update_queue_has_callback_full(set_client_page_callback,
                                     &data, NULL)) {
    return FC_PTR_TO_INT(data);
  } else {
    return get_current_client_page();
  }
}

/************************************************************************//**
  Returns whether there's page switching already in progress.
****************************************************************************/
bool update_queue_is_switching_page(void)
{
  return update_queue_has_callback(set_client_page_callback);
}

/************************************************************************//**
  Update the menus.
****************************************************************************/
static void menus_update_callback(void *data)
{
  if (FC_PTR_TO_INT(data)) {
    real_menus_init();
  }
  real_menus_update();
}

/************************************************************************//**
  Request the menus to be initialized and updated.
****************************************************************************/
void menus_init(void)
{
  update_queue_add(menus_update_callback, FC_INT_TO_PTR(TRUE));
}

/************************************************************************//**
  Request the menus to be updated.
****************************************************************************/
void menus_update(void)
{
  if (!update_queue_has_callback(menus_update_callback)) {
    update_queue_add(menus_update_callback, FC_INT_TO_PTR(FALSE));
  }
}

/************************************************************************//**
  Update multipliers/policy dialog.
****************************************************************************/
void multipliers_dialog_update(void)
{
  update_queue_add(real_multipliers_dialog_update, NULL);
}

/************************************************************************//**
  Update cities gui.
****************************************************************************/
static void cities_update_callback(void *data)
{
#ifdef FREECIV_DEBUG
#define NEED_UPDATE(city_update, action)                                    \
  if (city_update & need_update) {                                          \
    action;                                                                 \
    need_update &= ~city_update;                                            \
  }
#else  /* FREECIV_DEBUG */
#define NEED_UPDATE(city_update, action)                                    \
  if (city_update & need_update) {                                          \
    action;                                                                 \
  }
#endif /* FREECIV_DEBUG */

  cities_iterate(pcity) {
    enum city_updates need_update = pcity->client.need_updates;

    if (CU_NO_UPDATE == need_update) {
      continue;
    }

    /* Clear all updates. */
    pcity->client.need_updates = CU_NO_UPDATE;

    NEED_UPDATE(CU_UPDATE_REPORT, real_city_report_update_city(pcity));
    NEED_UPDATE(CU_UPDATE_DIALOG, real_city_dialog_refresh(pcity));
    NEED_UPDATE(CU_POPUP_DIALOG, real_city_dialog_popup(pcity));

#ifdef FREECIV_DEBUG
    if (CU_NO_UPDATE != need_update) {
      log_error("Some city updates not handled "
                "for city %s (id %d): %d left.",
                city_name_get(pcity), pcity->id, need_update);
    }
#endif /* FREECIV_DEBUG */
  } cities_iterate_end;
#undef NEED_UPDATE
}

/************************************************************************//**
  Request the city dialog to be popped up for the city.
****************************************************************************/
void popup_city_dialog(struct city *pcity)
{
  pcity->client.need_updates |= CU_POPUP_DIALOG;
  update_queue_add(cities_update_callback, NULL);
}

/************************************************************************//**
  Request the city dialog to be updated for the city.
****************************************************************************/
void refresh_city_dialog(struct city *pcity)
{
  pcity->client.need_updates |= CU_UPDATE_DIALOG;
  update_queue_add(cities_update_callback, NULL);
}

/************************************************************************//**
  Request the city to be updated in the city report.
****************************************************************************/
void city_report_dialog_update_city(struct city *pcity)
{
  pcity->client.need_updates |= CU_UPDATE_REPORT;
  update_queue_add(cities_update_callback, NULL);
}

/************************************************************************//**
  Update the connection list in the start page.
****************************************************************************/
void conn_list_dialog_update(void)
{
  update_queue_add(real_conn_list_dialog_update, NULL);
}

/************************************************************************//**
  Update the nation report.
****************************************************************************/
void players_dialog_update(void)
{
  update_queue_add(real_players_dialog_update, NULL);
}

/************************************************************************//**
  Update the city report.
****************************************************************************/
void city_report_dialog_update(void)
{
  update_queue_add(real_city_report_dialog_update, NULL);
}

/************************************************************************//**
  Update the science report.
****************************************************************************/
void science_report_dialog_update(void)
{
  update_queue_add(real_science_report_dialog_update, NULL);
}

/************************************************************************//**
  Update the economy report.
****************************************************************************/
void economy_report_dialog_update(void)
{
  update_queue_add(real_economy_report_dialog_update, NULL);
}

/************************************************************************//**
  Update the units report.
****************************************************************************/
void units_report_dialog_update(void)
{
  update_queue_add(real_units_report_dialog_update, NULL);
}

/************************************************************************//**
  Update the units report.
****************************************************************************/
void unit_select_dialog_update(void)
{
  update_queue_add(unit_select_dialog_update_real, NULL);
}
