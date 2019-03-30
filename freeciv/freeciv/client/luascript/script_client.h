/*****************************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/

#ifndef FC__SCRIPT_CLIENT_H
#define FC__SCRIPT_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common/scriptcore */
#include "luascript_types.h"

struct section_file;

/* callback invocation function. */
bool script_client_callback_invoke(const char *callback_name, int nargs,
                                   enum api_types *parg_types, va_list args);

void script_client_remove_exported_object(void *object);

/* script functions. */
bool script_client_init(void);
void script_client_free(void);
bool script_client_do_string(const char *str);
bool script_client_do_file(const char *filename);

/* script state i/o. */
void script_client_state_load(struct section_file *file);
void script_client_state_save(struct section_file *file);

/* Signals. */
void script_client_signal_connect(const char *signal_name,
                                  const char *callback_name);
void script_client_signal_emit(const char *signal_name, ...);
const char *script_client_signal_list(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__SCRIPT_CLIENT_H */
