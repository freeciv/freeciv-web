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

#ifndef FC__API_SIGNAL_BASE_H
#define FC__API_SIGNAL_BASE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "support.h"

struct lua_State;

void api_signal_connect(lua_State *L, const char *signal_name,
                        const char *callback_name);
void api_signal_remove(lua_State *L, const char *signal_name,
                       const char *callback_name);
bool api_signal_defined(lua_State *L, const char *signal_name,
                        const char *callback_name);

const char *api_signal_callback_by_index(lua_State *L,
                                         const char *signal_name, int sindex);

const char *api_signal_by_index(lua_State *L, int sindex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__API_SIGNAL_BASE_H */
