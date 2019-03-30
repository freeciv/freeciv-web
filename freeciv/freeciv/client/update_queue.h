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
#ifndef FC__UPDATE_QUEUE_H
#define FC__UPDATE_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void (*uq_callback_t) (void *data);
typedef void (*uq_free_fn_t) (void *data);
#define UQ_FREEDATA(fn) ((uq_free_fn_t) fn)

/* General update queue. */
void update_queue_init(void);
void update_queue_free(void);

void update_queue_freeze(void);
void update_queue_thaw(void);
void update_queue_force_thaw(void);
bool update_queue_is_frozen(void);

void update_queue_processing_started(int request_id);
void update_queue_processing_finished(int request_id);

/* User interface. */
void update_queue_add(uq_callback_t callback, void *data);
void update_queue_add_full(uq_callback_t callback, void *data,
                           uq_free_fn_t free_data_func);
bool update_queue_has_callback(uq_callback_t callback);
bool update_queue_has_callback_full(uq_callback_t callback,
                                    const void **data,
                                    uq_free_fn_t *free_data_func);

void update_queue_connect_processing_started(int request_id,
                                             uq_callback_t callback,
                                             void *data);
void update_queue_connect_processing_started_full(int request_id,
                                                  uq_callback_t callback,
                                                  void *data,
                                                  uq_free_fn_t
                                                  free_data_func);
void update_queue_connect_processing_finished(int request_id,
                                              uq_callback_t callback,
                                              void *data);
void update_queue_connect_processing_finished_full(int request_id,
                                                   uq_callback_t callback,
                                                   void *data,
                                                   uq_free_fn_t
                                                   free_data_func);

bool update_queue_is_switching_page(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__UPDATE_QUEUE_H */
