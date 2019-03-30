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

#include <stdarg.h>

/* dependencies/lua */
#include "lua.h"
#include "lualib.h"

/* utility */
#include "string_vector.h"

/* common/scriptcore */
#include "luascript.h"
#include "luascript_types.h"

#include "luascript_func.h"

static struct luascript_func *func_new(bool required, int nargs,
                                       enum api_types *parg_types,
                                       int nreturns,
                                       enum api_types *preturn_types);
static void func_destroy(struct luascript_func *pfunc);

struct luascript_func {
  bool required;                 /* if this function is required */
  int nargs;                     /* number of arguments to pass */
  int nreturns;                  /* number of return values to accept */
  enum api_types *arg_types;     /* argument types */
  enum api_types *return_types;  /* return types */
};

#define SPECHASH_TAG luascript_func
#define SPECHASH_ASTR_KEY_TYPE
#define SPECHASH_IDATA_TYPE struct luascript_func *
#define SPECHASH_IDATA_FREE func_destroy
#include "spechash.h"

#define luascript_func_hash_keys_iterate(phash, key)                         \
  TYPED_HASH_KEYS_ITERATE(char *, phash, key)
#define luascript_func_hash_keys_iterate_end                                 \
  HASH_KEYS_ITERATE_END

/*************************************************************************//**
  Create a new function definition.
*****************************************************************************/
static struct luascript_func *func_new(bool required, int nargs,
                                       enum api_types *parg_types,
                                       int nreturns,
                                       enum api_types *preturn_types)
{
  struct luascript_func *pfunc = fc_malloc(sizeof(*pfunc));

  pfunc->required = required;
  pfunc->nargs = nargs;
  pfunc->arg_types = parg_types;
  pfunc->nreturns = nreturns;
  pfunc->return_types = preturn_types;

  return pfunc;
}

/*************************************************************************//**
  Free a function definition.
*****************************************************************************/
static void func_destroy(struct luascript_func *pfunc)
{
  if (pfunc->arg_types) {
    free(pfunc->arg_types);
    free(pfunc->return_types);
  }
  free(pfunc);
}

/*************************************************************************//**
  Test if all function are defines. If it fails (return value FALSE), the
  missing functions are listed in 'missing_func_required' and
  'missing_func_optional'.
*****************************************************************************/
bool luascript_func_check(struct fc_lua *fcl,
                          struct strvec *missing_func_required,
                          struct strvec *missing_func_optional)
{
  bool ret = TRUE;

  fc_assert_ret_val(fcl, FALSE);
  fc_assert_ret_val(fcl->funcs, FALSE);

  luascript_func_hash_keys_iterate(fcl->funcs, func_name) {
    if (!luascript_check_function(fcl, func_name)) {
      struct luascript_func *pfunc;

      fc_assert_ret_val(luascript_func_hash_lookup(fcl->funcs, func_name,
                                                   &pfunc), FALSE);

      if (pfunc->required) {
        strvec_append(missing_func_required, func_name);
      } else {
        strvec_append(missing_func_optional, func_name);
      }

      ret = FALSE;
    }
  } luascript_func_hash_keys_iterate_end;

  return ret;
}

/*************************************************************************//**
  Add a lua function.
*****************************************************************************/
void luascript_func_add_valist(struct fc_lua *fcl, const char *func_name,
                               bool required, int nargs, int nreturns,
                               va_list args)
{
  struct luascript_func *pfunc;
  enum api_types *parg_types = fc_calloc(nargs, sizeof(*parg_types));
  enum api_types *pret_types = fc_calloc(nreturns, sizeof(*pret_types));
  int i;

  fc_assert_ret(fcl);
  fc_assert_ret(fcl->funcs);

  if (luascript_func_hash_lookup(fcl->funcs, func_name, &pfunc)) {
    luascript_log(fcl, LOG_ERROR, "Function '%s' was already created.",
                  func_name);
    return;
  }

  for (i = 0; i < nargs; i++) {
    *(parg_types + i) = va_arg(args, int);
  }

  for (i = 0; i < nreturns; i++) {
    *(pret_types + i) = va_arg(args, int);
  }

  pfunc = func_new(required, nargs, parg_types, nreturns, pret_types);
  luascript_func_hash_insert(fcl->funcs, func_name, pfunc);
}

/*************************************************************************//**
  Add a lua function.
*****************************************************************************/
void luascript_func_add(struct fc_lua *fcl, const char *func_name,
                        bool required, int nargs, int nreturns, ...)
{
  va_list args;

  va_start(args, nreturns);
  luascript_func_add_valist(fcl, func_name, required, nargs, nreturns, args);
  va_end(args);
}


/*************************************************************************//**
  Free the function definitions.
*****************************************************************************/
void luascript_func_free(struct fc_lua *fcl)
{
  if (fcl && fcl->funcs) {
    luascript_func_hash_destroy(fcl->funcs);
    fcl->funcs = NULL;
  }
}

/*************************************************************************//**
  Initialize the structures needed to save functions definitions.
*****************************************************************************/
void luascript_func_init(struct fc_lua *fcl)
{
  fc_assert_ret(fcl != NULL);

  if (fcl->funcs == NULL) {
    /* Define the prototypes for the needed lua functions. */
    fcl->funcs = luascript_func_hash_new();
  }
}

/*************************************************************************//**
  Call a lua function; return value is TRUE if no errors occurred, otherwise
  FALSE.

  Example call to the lua function 'user_load()':
    success = luascript_func_call(L, "user_load", pconn, &password);
*****************************************************************************/
bool luascript_func_call_valist(struct fc_lua *fcl, const char *func_name,
                                va_list args)
{
  struct luascript_func *pfunc;
  bool success = FALSE;

  fc_assert_ret_val(fcl, FALSE);
  fc_assert_ret_val(fcl->state, FALSE);
  fc_assert_ret_val(fcl->funcs, FALSE);

  if (!luascript_func_hash_lookup(fcl->funcs, func_name, &pfunc)) {
    luascript_log(fcl, LOG_ERROR, "Lua function '%s' does not exist, "
                                  "so cannot be invoked.", func_name);
    return FALSE;
  }

  /* The function name */
  lua_getglobal(fcl->state, func_name);

  if (!lua_isfunction(fcl->state, -1)) {
    if (pfunc->required) {
      /* This function is required. Thus, this is an error. */
      luascript_log(fcl, LOG_ERROR, "Unknown lua function '%s'", func_name);
      lua_pop(fcl->state, 1);
    }
    return FALSE;
  }

  luascript_push_args(fcl, pfunc->nargs, pfunc->arg_types, args);

  /* Call the function with nargs arguments, return 1 results */
  if (luascript_call(fcl, pfunc->nargs, pfunc->nreturns, NULL) == 0) {
    /* Successful call to the script. */
    success = TRUE;

    luascript_pop_returns(fcl, func_name, pfunc->nreturns,
                          pfunc->return_types, args);
  }

  return success;
}

/*************************************************************************//**
  Call a lua function; return value is TRUE if no errors occurred, otherwise
  FALSE.

  Example call to the lua function 'user_load()':
    success = luascript_func_call(L, "user_load", pconn, &password);
*****************************************************************************/
bool luascript_func_call(struct fc_lua *fcl, const char *func_name, ...)
{
  va_list args;
  bool success;

  va_start(args, func_name);
  success = luascript_func_call_valist(fcl, func_name, args);
  va_end(args);

  return success;
}

/*************************************************************************//**
  Return iff the function is required.
*****************************************************************************/
bool luascript_func_is_required(struct fc_lua *fcl, const char *func_name)
{
  struct luascript_func *pfunc;

  fc_assert_ret_val(fcl, FALSE);
  fc_assert_ret_val(fcl->state, FALSE);
  fc_assert_ret_val(fcl->funcs, FALSE);

  if (!luascript_func_hash_lookup(fcl->funcs, func_name, &pfunc)) {
    luascript_log(fcl, LOG_ERROR, "Lua function '%s' does not exist.",
                  func_name);
    return FALSE;
  }

  return pfunc->required;
}
