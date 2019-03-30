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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* utility */
#include "fcintl.h"

/* common/scriptcore */
#include "luascript.h"

#include "api_common_intl.h"

/*************************************************************************//**
  Translation helper function.
*****************************************************************************/
const char *api_intl__(lua_State *L, const char *untranslated)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, untranslated, 2, string, "");

  return _(untranslated);
}

/*************************************************************************//**
  Translation helper function.
*****************************************************************************/
const char *api_intl_N_(lua_State *L, const char *untranslated)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, untranslated, 2, string, "");

  return N_(untranslated);
}

/*************************************************************************//**
  Translation helper function.
*****************************************************************************/
const char *api_intl_Q_(lua_State *L, const char *untranslated)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, untranslated, 2, string, "");

  return Q_(untranslated);
}

/*************************************************************************//**
  Translation helper function.
*****************************************************************************/
const char *api_intl_PL_(lua_State *L, const char *singular,
                         const char *plural, int n)
{
  LUASCRIPT_CHECK_STATE(L, NULL);
  LUASCRIPT_CHECK_ARG_NIL(L, singular, 2, string, "");
  LUASCRIPT_CHECK_ARG_NIL(L, plural, 3, string, "");

  return PL_(singular, plural, n);
}
