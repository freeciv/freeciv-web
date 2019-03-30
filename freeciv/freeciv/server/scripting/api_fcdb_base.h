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

#ifndef FC__API_FCDB_BASE_H
#define FC__API_FCDB_BASE_H

/* common/scriptcore */
#include "luascript_types.h"

/* server */
#include "fcdb.h"

struct lua_State;

const char *api_fcdb_option(lua_State *L, const char *type);

#endif /* FC__API_FCDB_BASE_H */

