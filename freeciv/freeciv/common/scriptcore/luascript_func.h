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
#ifndef FC__LUASCRIPT_FUNC_H
#define FC__LUASCRIPT_FUNC_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "support.h"

struct fc_lua;

void luascript_func_init(struct fc_lua *fcl);
void luascript_func_free(struct fc_lua *fcl);

bool luascript_func_check(struct fc_lua *fcl,
                          struct strvec *missing_func_required,
                          struct strvec *missing_func_optional);
void luascript_func_add_valist(struct fc_lua *fcl, const char *func_name,
                               bool required, int nargs, int nreturns,
                               va_list args);
void luascript_func_add(struct fc_lua *fcl, const char *func_name,
                        bool required, int nargs, int nreturns, ...);
bool luascript_func_call_valist(struct fc_lua *fcl, const char *func_name,
                                va_list args);
bool luascript_func_call(struct fc_lua *fcl, const char *func_name, ...);

bool luascript_func_is_required(struct fc_lua *fcl, const char *func_name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__LUASCRIPT_FUNC_H */
