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
#include <stdlib.h>
#include <time.h>

/* dependencies/lua */
#include "lua.h"
#include "lualib.h"

/* utility */
#include "astring.h"
#include "log.h"
#include "registry.h"

/* common/scriptcore */
#include "luascript_func.h"
#include "luascript_signal.h"

#include "luascript.h"

/*****************************************************************************
  Configuration for script execution time limits. Checkinterval is the
  number of executed lua instructions between checking. Disabled if 0.
*****************************************************************************/
#define LUASCRIPT_MAX_EXECUTION_TIME_SEC 5.0
#define LUASCRIPT_CHECKINTERVAL 10000

/* The name used for the freeciv lua struct saved in the lua state. */
#define LUASCRIPT_GLOBAL_VAR_NAME "__fcl"

/*****************************************************************************
  Unsafe Lua builtin symbols that we to remove access to.

  If Freeciv's Lua version changes, you have to check how the set of
  unsafe functions and modules changes in the new version. Update the list of
  loaded libraries in luascript_lualibs, then update the unsafe symbols
  blacklist in luascript_unsafe_symbols.

  Once the variables are updated for the new version, update the value of
  LUASCRIPT_SECURE_LUA_VERSION

  In general, unsafe is all functionality that gives access to:
  * Reading files and running processes
  * Loading lua files or libraries
*****************************************************************************/
#define LUASCRIPT_SECURE_LUA_VERSION1 502
#define LUASCRIPT_SECURE_LUA_VERSION2 503

static const char *luascript_unsafe_symbols_secure[] = {
  "debug",
  "dofile",
  "loadfile",
  NULL
};

static const char *luascript_unsafe_symbols_permissive[] = {
  "debug",
  "dofile",
  "loadfile",
  NULL
};

#if LUA_VERSION_NUM != LUASCRIPT_SECURE_LUA_VERSION1 && LUA_VERSION_NUM != LUASCRIPT_SECURE_LUA_VERSION2
#warning "The script runtime's unsafe symbols information is not up to date."
#warning "This can be a big security hole!"
#endif

/*****************************************************************************
  Lua libraries to load (all default libraries, excluding operating system
  and library loading modules). See linit.c in Lua 5.1 for the default list.
*****************************************************************************/
#if LUA_VERSION_NUM == 502
static luaL_Reg luascript_lualibs_secure[] = {
  /* Using default libraries excluding: package, io and os */
  {"_G", luaopen_base},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_BITLIBNAME, luaopen_bit32},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_DBLIBNAME, luaopen_debug},
  {NULL, NULL}
};
#elif LUA_VERSION_NUM == 503
static luaL_Reg luascript_lualibs_secure[] = {
  /* Using default libraries excluding: package, io, os, and bit32 */
  {"_G", luaopen_base},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_DBLIBNAME, luaopen_debug},
  {NULL, NULL}
};

static luaL_Reg luascript_lualibs_permissive[] = {
  /* Using default libraries excluding: package, io, and bit32 */
  {"_G", luaopen_base},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_UTF8LIBNAME, luaopen_utf8},
  {LUA_MATHLIBNAME, luaopen_math},
  {LUA_DBLIBNAME, luaopen_debug},
  {LUA_OSLIBNAME, luaopen_os},
  {NULL, NULL}
};
#else  /* LUA_VERSION_NUM */
#error "Unsupported lua version"
#endif /* LUA_VERSION_NUM */
 
static int luascript_report(struct fc_lua *fcl, int status, const char *code);
static void luascript_traceback_func_save(lua_State *L);
static void luascript_traceback_func_push(lua_State *L);
static void luascript_exec_check(lua_State *L, lua_Debug *ar);
static void luascript_hook_start(lua_State *L);
static void luascript_hook_end(lua_State *L);
static void luascript_openlibs(lua_State *L, const luaL_Reg *llib);
static void luascript_blacklist(lua_State *L, const char *lsymbols[]);

/*************************************************************************//**
  Report a lua error.
*****************************************************************************/
static int luascript_report(struct fc_lua *fcl, int status, const char *code)
{
  fc_assert_ret_val(fcl, -1);
  fc_assert_ret_val(fcl->state, -1);

  if (status) {
    struct astring str = ASTRING_INIT;
    const char *msg;
    int lineno;

    if (!(msg = lua_tostring(fcl->state, -1))) {
      msg = "(error with no message)";
    }

    /* Add error message. */
    astr_add_line(&str, "lua error:");
    astr_add_line(&str, "\t%s", msg);

    if (code) {
      /* Add lines around the place the parse error is. */
      if (sscanf(msg, "%*[^:]:%d:", &lineno) == 1) {
        const char *begin, *end;
        int i;

        astr_add(&str, "\n");

        i = 1;
        for (begin = code; *begin != '\0';) {
          int len;

          end = strchr(begin, '\n');
          if (end) {
            len = end - begin;
          } else {
            len = strlen(begin);
          }

          if (abs(lineno - i) <= 3) {
            const char *indicator;

            indicator = (lineno == i) ? "-->" : "   ";

            astr_add_line(&str, "\t%s%3d:\t%*.*s",
                indicator, i, len, len, begin);
          }

          i++;

          if (end) {
            begin = end + 1;
          } else {
            break;
          }
        }

        astr_add(&str, "\n");
      }
    }

    luascript_log(fcl, LOG_ERROR, "%s", astr_str(&str));

    astr_free(&str);

    lua_pop(fcl->state, 1);
  }

  return status;
}

/*************************************************************************//**
  Find the debug.traceback function and store in the registry
*****************************************************************************/
static void luascript_traceback_func_save(lua_State *L)
{
  /* Find the debug.traceback function, if available */
  lua_getglobal(L, "debug");
  if (lua_istable(L, -1)) {
    lua_getfield(L, -1, "traceback");
    lua_setfield(L, LUA_REGISTRYINDEX, "freeciv_traceback");
  }
  lua_pop(L, 1);       /* pop debug */
}

/*************************************************************************//**
  Push the traceback function to the stack
*****************************************************************************/
static void luascript_traceback_func_push(lua_State *L)
{
  lua_getfield(L, LUA_REGISTRYINDEX, "freeciv_traceback");
}

/*************************************************************************//**
  Check currently excecuting lua function for execution time limit
*****************************************************************************/
static void luascript_exec_check(lua_State *L, lua_Debug *ar)
{
  lua_Number exec_clock;

  lua_getfield(L, LUA_REGISTRYINDEX, "freeciv_exec_clock");
  exec_clock = lua_tonumber(L, -1);
  lua_pop(L, 1);
  if ((float)(clock() - exec_clock)/CLOCKS_PER_SEC
      > LUASCRIPT_MAX_EXECUTION_TIME_SEC) {
    luaL_error(L, "Execution time limit exceeded in script");
  }
}

/*************************************************************************//**
  Setup function execution guard
*****************************************************************************/
static void luascript_hook_start(lua_State *L)
{
#if LUASCRIPT_CHECKINTERVAL
  /* Store clock timestamp in the registry */
  lua_pushnumber(L, clock());
  lua_setfield(L, LUA_REGISTRYINDEX, "freeciv_exec_clock");
  lua_sethook(L, luascript_exec_check, LUA_MASKCOUNT, LUASCRIPT_CHECKINTERVAL);
#endif
}

/*************************************************************************//**
  Clear function execution guard
*****************************************************************************/
static void luascript_hook_end(lua_State *L)
{
#if LUASCRIPT_CHECKINTERVAL
  lua_sethook(L, luascript_exec_check, 0, 0);
#endif
}

/*************************************************************************//**
  Open lua libraries in the array of library definitions in llib.
*****************************************************************************/
static void luascript_openlibs(lua_State *L, const luaL_Reg *llib)
{
  /* set results to global table */
  for (; llib->func; llib++) {
    luaL_requiref(L, llib->name, llib->func, 1);
    lua_pop(L, 1);  /* remove lib */
  }
}

/*************************************************************************//**
  Remove global symbols from lua state L
*****************************************************************************/
static void luascript_blacklist(lua_State *L, const char *lsymbols[])
{
  int i;

  for (i = 0; lsymbols[i] != NULL; i++) {
    lua_pushnil(L);
    lua_setglobal(L, lsymbols[i]);
  }
}

/*************************************************************************//**
  Internal api error function - varg version.
*****************************************************************************/
int luascript_error(lua_State *L, const char *format, ...)
{
  va_list vargs;
  int ret;

  va_start(vargs, format);
  ret = luascript_error_vargs(L, format, vargs);
  va_end(vargs);

  return ret;
}

/*************************************************************************//**
  Internal api error function.
  Invoking this will cause Lua to stop executing the current context and
  throw an exception, so to speak.
*****************************************************************************/
int luascript_error_vargs(lua_State *L, const char *format, va_list vargs)
{
  fc_assert_ret_val(L != NULL, -1);

  luaL_where(L, 1);
  lua_pushvfstring(L, format, vargs);
  lua_concat(L, 2);

  return lua_error(L);
}

/*************************************************************************//**
  Like script_error, but using a prefix identifying the called lua function:
    bad argument #narg to '<func>': msg
*****************************************************************************/
int luascript_arg_error(lua_State *L, int narg, const char *msg)
{
  return luaL_argerror(L, narg, msg);
}

/*************************************************************************//**
  Initialize the scripting state.
*****************************************************************************/
struct fc_lua *luascript_new(luascript_log_func_t output_fct,
                             bool secured_environment)
{
  struct fc_lua *fcl = fc_calloc(1, sizeof(*fcl));

  fcl->state = luaL_newstate();
  if (!fcl->state) {
    FC_FREE(fcl);
    return NULL;
  }
  fcl->output_fct = output_fct;
  fcl->caller = NULL;

  if (secured_environment) {
    luascript_openlibs(fcl->state, luascript_lualibs_secure);
    luascript_traceback_func_save(fcl->state);
    luascript_blacklist(fcl->state, luascript_unsafe_symbols_secure);
  } else {
    luascript_openlibs(fcl->state, luascript_lualibs_permissive);
    luascript_traceback_func_save(fcl->state);
    luascript_blacklist(fcl->state, luascript_unsafe_symbols_permissive);
  }

  /* Save the freeciv lua struct in the lua state. */
  lua_pushstring(fcl->state, LUASCRIPT_GLOBAL_VAR_NAME);
  lua_pushlightuserdata(fcl->state, fcl);
  lua_settable(fcl->state, LUA_REGISTRYINDEX);

  return fcl;
}

/*************************************************************************//**
  Get the freeciv lua struct from a lua state.
*****************************************************************************/
struct fc_lua *luascript_get_fcl(lua_State *L)
{
  struct fc_lua *fcl;

  fc_assert_ret_val(L, NULL);

  /* Get the freeciv lua struct from the lua state. */
  lua_pushstring(L, LUASCRIPT_GLOBAL_VAR_NAME);
  lua_gettable(L, LUA_REGISTRYINDEX);
  fcl = lua_touserdata(L, -1);

  /* This is an error! */
  fc_assert_ret_val(fcl != NULL, NULL);

  return fcl;
}

/*************************************************************************//**
  Free the scripting data.
*****************************************************************************/
void luascript_destroy(struct fc_lua *fcl)
{
  if (fcl) {
    fc_assert_ret(fcl->caller == NULL);

    /* Free function data. */
    luascript_func_free(fcl);

    /* Free signal data. */
    luascript_signal_free(fcl);

    /* Free lua state. */
    if (fcl->state) {
      lua_gc(fcl->state, LUA_GCCOLLECT, 0); /* Collected garbage */
      lua_close(fcl->state);
    }
    free(fcl);
  }
}

/*************************************************************************//**
  Print a message to the selected output handle.
*****************************************************************************/
void luascript_log(struct fc_lua *fcl, enum log_level level,
                   const char *format, ...)
{
  va_list args;

  va_start(args, format);
  luascript_log_vargs(fcl, level, format, args);
  va_end(args);
}

/*************************************************************************//**
  Print a message to the selected output handle.
*****************************************************************************/
void luascript_log_vargs(struct fc_lua *fcl, enum log_level level,
                         const char *format, va_list args)
{
  char buf[1024];

  fc_assert_ret(fcl);
  fc_assert_ret(0 <= level && level <= LOG_DEBUG);

  fc_vsnprintf(buf, sizeof(buf), format, args);

  if (fcl->output_fct) {
    fcl->output_fct(fcl, level, "%s", buf);
  } else {
    log_base(level, "%s", buf);
  }
}

/*************************************************************************//**
  Pop return values from the Lua stack.
*****************************************************************************/
void luascript_pop_returns(struct fc_lua *fcl, const char *func_name,
                           int nreturns, enum api_types *preturn_types,
                           va_list args)
{
  int i;
  lua_State *L;

  fc_assert_ret(fcl);
  fc_assert_ret(fcl->state);
  L = fcl->state;

  for (i = 0; i < nreturns; i++) {
    enum api_types type = preturn_types[i];

    fc_assert_ret(api_types_is_valid(type));

    switch (type) {
      case API_TYPE_INT:
        {
          int isnum;
          int *pres = va_arg(args, int*);

          *pres = lua_tointegerx(L, -1, &isnum);
          if (!isnum) {
            log_error("Return value from lua function %s is a %s, want int",
                      func_name, lua_typename(L, lua_type(L, -1)));
          }
        }
        break;
      case API_TYPE_BOOL:
        {
          bool *pres = va_arg(args, bool*);
          *pres = lua_toboolean(L, -1);
        }
        break;
      case API_TYPE_STRING:
        {
          char **pres = va_arg(args, char**);

          if (lua_isstring(L, -1)) {
            *pres = fc_strdup(lua_tostring(L, -1));
          }
        }
        break;
      default:
        {
          void **pres = va_arg(args, void**);

          *pres = tolua_tousertype(fcl->state, -1, NULL);
        }
        break;
    }
    lua_pop(L, 1);
  }
}

/*************************************************************************//**
  Push arguments into the Lua stack.
*****************************************************************************/
void luascript_push_args(struct fc_lua *fcl, int nargs,
                         enum api_types *parg_types, va_list args)
{
  int i;

  fc_assert_ret(fcl);
  fc_assert_ret(fcl->state);

  for (i = 0; i < nargs; i++) {
    enum api_types type = parg_types[i];

    fc_assert_ret(api_types_is_valid(type));

    switch (type) {
      case API_TYPE_INT:
        {
          lua_Integer arg = va_arg(args, lua_Integer);
          lua_pushinteger(fcl->state, arg);
        }
        break;
      case API_TYPE_BOOL:
        {
          int arg = va_arg(args, int);
          lua_pushboolean(fcl->state, arg);
        }
        break;
      case API_TYPE_STRING:
        {
          const char *arg = va_arg(args, const char*);
          lua_pushstring(fcl->state, arg);
        }
        break;
      default:
        {
          const char *name;
          void *arg;

          name = api_types_name(type);

          arg = va_arg(args, void*);
          tolua_pushusertype(fcl->state, arg, name);
        }
        break;
    }
  }
}

/*************************************************************************//**
  Return if the function 'funcname' is define in the lua state 'fcl->state'.
*****************************************************************************/
bool luascript_check_function(struct fc_lua *fcl, const char *funcname)
{
  bool defined;

  fc_assert_ret_val(fcl, FALSE);
  fc_assert_ret_val(fcl->state, FALSE);

  lua_getglobal(fcl->state, funcname);
  defined = lua_isfunction(fcl->state, -1);
  lua_pop(fcl->state, 1);

  return defined;
}

/*************************************************************************//**
  Evaluate a Lua function call or loaded script on the stack.
  Return nonzero if an error occurred.

  If available pass the source code string as code, else NULL.

  Will pop function and arguments (1 + narg values) from the stack.
  Will push nret return values to the stack.

  On error, print an error message with traceback. Nothing is pushed to
  the stack.
*****************************************************************************/
int luascript_call(struct fc_lua *fcl, int narg, int nret, const char *code)
{
  int status;
  int base;          /* Index of function to call */
  int traceback = 0; /* Index of traceback function  */

  fc_assert_ret_val(fcl, -1);
  fc_assert_ret_val(fcl->state, -1);

  base = lua_gettop(fcl->state) - narg;

  /* Find the traceback function, if available */
  luascript_traceback_func_push(fcl->state);
  if (lua_isfunction(fcl->state, -1)) {
    lua_insert(fcl->state, base);  /* insert traceback before function */
    traceback = base;
  } else {
    lua_pop(fcl->state, 1);   /* pop non-function traceback */
  }

  luascript_hook_start(fcl->state);
  status = lua_pcall(fcl->state, narg, nret, traceback);
  luascript_hook_end(fcl->state);

  if (status) {
    luascript_report(fcl, status, code);
  }

  if (traceback) {
    lua_remove(fcl->state, traceback);
  }

  return status;
}

/*************************************************************************//**
  lua_dostring replacement with error message showing on errors.
*****************************************************************************/
int luascript_do_string(struct fc_lua *fcl, const char *str, const char *name)
{
  int status;

  fc_assert_ret_val(fcl, -1);
  fc_assert_ret_val(fcl->state, -1);

  status = luaL_loadbuffer(fcl->state, str, strlen(str), name);
  if (status) {
    luascript_report(fcl, status, str);
  } else {
    status = luascript_call(fcl, 0, 0, str);
  }
  return status;
}

/*************************************************************************//**
  Parse and execute the script at filename.
*****************************************************************************/
int luascript_do_file(struct fc_lua *fcl, const char *filename)
{
  int status;

  fc_assert_ret_val(fcl, -1);
  fc_assert_ret_val(fcl->state, -1);

  status = luaL_loadfile(fcl->state, filename);
  if (status) {
    luascript_report(fcl, status, NULL);
  } else {
    status = luascript_call(fcl, 0, 0, NULL);
  }
  return status;
}


/*************************************************************************//**
  Invoke the 'callback_name' Lua function.
*****************************************************************************/
bool luascript_callback_invoke(struct fc_lua *fcl, const char *callback_name,
                               int nargs, enum api_types *parg_types,
                               va_list args)
{
  bool stop_emission = FALSE;

  fc_assert_ret_val(fcl, FALSE);
  fc_assert_ret_val(fcl->state, FALSE);

  /* The function name */
  lua_getglobal(fcl->state, callback_name);

  if (!lua_isfunction(fcl->state, -1)) {
    luascript_log(fcl, LOG_ERROR, "lua error: Unknown callback '%s'",
                  callback_name);
    lua_pop(fcl->state, 1);
    return FALSE;
  }

  luascript_log(fcl, LOG_DEBUG, "lua callback: '%s'", callback_name);

  luascript_push_args(fcl, nargs, parg_types, args);

  /* Call the function with nargs arguments, return 1 results */
  if (luascript_call(fcl, nargs, 1, NULL)) {
    return FALSE;
  }

  /* Shall we stop the emission of this signal? */
  if (lua_isboolean(fcl->state, -1)) {
    stop_emission = lua_toboolean(fcl->state, -1);
  }
  lua_pop(fcl->state, 1);   /* pop return value */

  return stop_emission;
}

/*************************************************************************//**
  Mark any, if exported, full userdata representing 'object' in
  the current script state as 'Nonexistent'.
  This changes the type of the lua variable.
*****************************************************************************/
void luascript_remove_exported_object(struct fc_lua *fcl, void *object)
{
  if (fcl && fcl->state) {
    fc_assert_ret(object != NULL);

    /* The following is similar to tolua_release(..) in src/lib/tolua_map.c */
    /* Find the userdata representing 'object' */
    lua_pushstring(fcl->state,"tolua_ubox");
    /* stack: ubox */
    lua_rawget(fcl->state, LUA_REGISTRYINDEX);
    /* stack: ubox u */
    lua_pushlightuserdata(fcl->state, object);
    /* stack: ubox ubox[u] */
    lua_rawget(fcl->state, -2);

    if (!lua_isnil(fcl->state, -1)) {
      fc_assert(object == tolua_tousertype(fcl->state, -1, NULL));
      /* Change API type to 'Nonexistent' */
      /* stack: ubox ubox[u] mt */
      tolua_getmetatable(fcl->state, "Nonexistent");
      lua_setmetatable(fcl->state, -2);
      /* Set the userdata payload to NULL */
      *((void **)lua_touserdata(fcl->state, -1)) = NULL;
      /* Remove from ubox */
      /* stack: ubox ubox[u] u */
      lua_pushlightuserdata(fcl->state, object);
      /* stack: ubox ubox[u] u nil */
      lua_pushnil(fcl->state);
      lua_rawset(fcl->state, -4);
    }
    lua_pop(fcl->state, 2);
  }
}

/*************************************************************************//**
  Save lua variables to file.
*****************************************************************************/
void luascript_vars_save(struct fc_lua *fcl, struct section_file *file,
                         const char *section)
{
  fc_assert_ret(file);
  fc_assert_ret(fcl);
  fc_assert_ret(fcl->state);

  lua_getglobal(fcl->state, "_freeciv_state_dump");
  if (luascript_call(fcl, 0, 1, NULL) == 0) {
    const char *vars;

    vars = lua_tostring(fcl->state, -1);
    lua_pop(fcl->state, 1);

    if (vars) {
      secfile_insert_str_noescape(file, vars, "%s", section);
    }
  } else {
    /* _freeciv_state_dump in tolua_game.pkg is busted */
    luascript_log(fcl, LOG_ERROR, "lua error: Failed to dump variables");
  }
}

/*************************************************************************//**
  Load lua variables from file.
*****************************************************************************/
void luascript_vars_load(struct fc_lua *fcl, struct section_file *file,
                         const char *section)
{
  const char *vars;

  fc_assert_ret(file);
  fc_assert_ret(fcl);
  fc_assert_ret(fcl->state);

  vars = secfile_lookup_str_default(file, "", "%s", section);
  luascript_do_string(fcl, vars, section);
}
