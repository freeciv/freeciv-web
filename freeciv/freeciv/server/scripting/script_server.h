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

#ifndef FC__SCRIPT_SERVER_H
#define FC__SCRIPT_SERVER_H

/* utility */
#include "support.h"

/* common/scriptcore */
#include "luascript_types.h"

struct section_file;
struct connection;

void script_server_remove_exported_object(void *object);

/* Script functions. */
bool script_server_init(void);
void script_server_free(void);

bool script_server_do_string(struct connection *caller, const char *str);
bool script_server_do_file(struct connection *caller, const char *filename);
bool script_server_load_file(const char *filename, char **buf);

bool script_server_unsafe_do_string(struct connection *caller,
                                    const char *str);
bool script_server_unsafe_do_file(struct connection *caller,
                                  const char *filename);

/* Script state i/o. */
void script_server_state_load(struct section_file *file);
void script_server_state_save(struct section_file *file);

/* Signals. */
void script_server_signal_emit(const char *signal_name, ...);

/* Functions */
bool script_server_call(const char *func_name, ...);

#endif /* FC__SCRIPT_SERVER_H */

