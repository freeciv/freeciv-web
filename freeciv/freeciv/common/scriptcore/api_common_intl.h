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

#ifndef FC__API_COMMON_INTL_H
#define FC__API_COMMON_INTL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct lua_State;

const char *api_intl__(lua_State *L, const char *untranslated);
const char *api_intl_N_(lua_State *L, const char *untranslated);
const char *api_intl_Q_(lua_State *L, const char *untranslated);
const char *api_intl_PL_(lua_State *L, const char *singular,
                         const char *plural, int n);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__API_COMMON_INTL_H */
