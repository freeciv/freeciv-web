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

#ifndef FC__SCRIPT_FCDB_H
#define FC__SCRIPT_FCDB_H

/* utility */
#include "support.h"            /* fc__attribute() */

/* server */
#include "fcdb.h"

/* fcdb script functions. */
bool script_fcdb_init(const char *fcdb_luafile);
bool script_fcdb_call(const char *func_name, ...);
void script_fcdb_free(void);

bool script_fcdb_do_string(struct connection *caller, const char *str);

#endif /* FC__SCRIPT_FCDB_H */
