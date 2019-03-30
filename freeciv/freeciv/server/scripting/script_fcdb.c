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

/* dependencies/tolua */
#include "tolua.h"

/* dependencies/luasql */
#ifdef HAVE_FCDB_MYSQL
#include "ls_mysql.h"
#endif
#ifdef HAVE_FCDB_POSTGRES
#include "ls_postgres.h"
#endif
#ifdef HAVE_FCDB_SQLITE3
#include "ls_sqlite3.h"
#endif

/* utility */
#include "log.h"
#include "md5.h"
#include "registry.h"
#include "string_vector.h"

/* common/scriptcore */
#include "luascript.h"
#include "luascript_types.h"
#include "tolua_common_a_gen.h"
#include "tolua_common_z_gen.h"

/* server */
#include "console.h"
#include "stdinhand.h"

/* server/scripting */
#ifdef HAVE_FCDB
#include "tolua_fcdb_gen.h"
#endif /* HAVE_FCDB */

#include "script_fcdb.h"

#ifdef HAVE_FCDB

#define SCRIPT_FCDB_LUA_FILE "database.lua"

static void script_fcdb_functions_define(void);
static bool script_fcdb_functions_check(const char *fcdb_luafile);

static void script_fcdb_cmd_reply(struct fc_lua *lfcl, enum log_level level,
                                  const char *format, ...)
            fc__attribute((__format__ (__printf__, 3, 4)));

/*************************************************************************//**
  Lua virtual machine state.
*****************************************************************************/
static struct fc_lua *fcl = NULL;

/*************************************************************************//**
  Add fcdb callback functions; these must be defined in the lua script
  'database.lua':

  database_init:
    - test and initialise the database.
  database_free:
    - free the database.

  user_load(Connection pconn):
    - check if the user data was successful loaded from the database.
  user_save(Connection pconn, String password):
    - check if the user data was successful saved in the database.
  user_log(Connection pconn, Bool success):
    - check if the login attempt was successful logged.
  user_delegate_to(Connection pconn, Player pplayer, String delegate):
    - returns Bool, whether pconn is allowed to delegate player to delegate.
  user_take(Connection requester, Connection taker, Player pplayer,
            Bool observer):
    - returns Bool, whether requester is allowed to attach taker to pplayer.

  If an error occurred, the functions return a non-NULL string error message
  as the last return value.
*****************************************************************************/
static void script_fcdb_functions_define(void)
{
  luascript_func_add(fcl, "database_init", TRUE, 0, 0);
  luascript_func_add(fcl, "database_free", TRUE, 0, 0);

  luascript_func_add(fcl, "user_load", TRUE, 1, 1, API_TYPE_CONNECTION,
                     API_TYPE_STRING);
  luascript_func_add(fcl, "user_save", TRUE, 2, 0, API_TYPE_CONNECTION,
                     API_TYPE_STRING);
  luascript_func_add(fcl, "user_log", TRUE, 2, 0, API_TYPE_CONNECTION,
                     API_TYPE_BOOL);
  luascript_func_add(fcl, "user_delegate_to", FALSE, 3, 1,
                     API_TYPE_CONNECTION, API_TYPE_PLAYER, API_TYPE_STRING,
                     API_TYPE_BOOL);
  luascript_func_add(fcl, "user_take", FALSE, 4, 1, API_TYPE_CONNECTION,
                     API_TYPE_CONNECTION, API_TYPE_PLAYER, API_TYPE_BOOL,
                     API_TYPE_BOOL);
}

/*************************************************************************//**
  Check the existence of all needed functions.
*****************************************************************************/
static bool script_fcdb_functions_check(const char *fcdb_luafile)
{
  bool ret = TRUE;
  struct strvec *missing_func_required = strvec_new();
  struct strvec *missing_func_optional = strvec_new();

  if (!luascript_func_check(fcl, missing_func_required,
                            missing_func_optional)) {
    strvec_iterate(missing_func_required, func_name) {
      log_error("Database script '%s' does not define the required function "
                "'%s'.", fcdb_luafile, func_name);
      ret = FALSE;
    } strvec_iterate_end;
    strvec_iterate(missing_func_optional, func_name) {
      log_verbose("Database script '%s' does not define the optional "
                  "function '%s'.", fcdb_luafile, func_name);
    } strvec_iterate_end;
  }

  strvec_destroy(missing_func_required);
  strvec_destroy(missing_func_optional);

  return ret;
}

/*************************************************************************//**
  Send the message via cmd_reply().
*****************************************************************************/
static void script_fcdb_cmd_reply(struct fc_lua *lfcl, enum log_level level,
                                  const char *format, ...)
{
  va_list args;
  enum rfc_status rfc_status = C_OK;
  char buf[1024];

  va_start(args, format);
  fc_vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  switch (level) {
  case LOG_FATAL:
    /* Special case - will quit the server. */
    log_fatal("%s", buf);
    break;
  case LOG_ERROR:
  case LOG_WARN:
    rfc_status = C_WARNING;
    break;
  case LOG_NORMAL:
    rfc_status = C_COMMENT;
    break;
  case LOG_VERBOSE:
    rfc_status = C_LOG_BASE;
    break;
  case LOG_DEBUG:
    rfc_status = C_DEBUG;
    break;
  }

  cmd_reply(CMD_FCDB, lfcl->caller, rfc_status, "%s", buf);
}
#endif /* HAVE_FCDB */

/*************************************************************************//**
  Initialize the scripting state. Returns the status of the freeciv database
  lua state.
*****************************************************************************/
bool script_fcdb_init(const char *fcdb_luafile)
{
#ifdef HAVE_FCDB
  if (fcl != NULL) {
    fc_assert_ret_val(fcl->state != NULL, FALSE);

    return TRUE;
  }

  if (!fcdb_luafile) {
    /* Use default freeciv database lua file. */
    fcdb_luafile = FC_CONF_PATH "/" SCRIPT_FCDB_LUA_FILE;
  }

  fcl = luascript_new(NULL, FALSE);
  if (fcl == NULL) {
    log_error("Error loading the Freeciv database lua definition.");
    return FALSE;
  }

  tolua_common_a_open(fcl->state);
  tolua_fcdb_open(fcl->state);
#ifdef HAVE_FCDB_MYSQL
  luaL_requiref(fcl->state, "ls_mysql", luaopen_luasql_mysql, 1);
  lua_pop(fcl->state, 1);
#endif
#ifdef HAVE_FCDB_POSTGRES
  luaL_requiref(fcl->state, "ls_postgres", luaopen_luasql_postgres, 1);
  lua_pop(fcl->state, 1);
#endif
#ifdef HAVE_FCDB_SQLITE3
  luaL_requiref(fcl->state, "ls_sqlite3", luaopen_luasql_sqlite3, 1);
  lua_pop(fcl->state, 1);
#endif
  tolua_common_z_open(fcl->state);

  luascript_func_init(fcl);

  /* Define the prototypes for the needed lua functions. */
  script_fcdb_functions_define();

  if (luascript_do_file(fcl, fcdb_luafile)
      || !script_fcdb_functions_check(fcdb_luafile)) {
    log_error("Error loading the Freeciv database lua script '%s'.",
              fcdb_luafile);
    script_fcdb_free();
    return FALSE;
  }

  if (!script_fcdb_call("database_init")) {
    log_error("Error connecting to the database");
    script_fcdb_free();
    return FALSE;
  }
#endif /* HAVE_FCDB */

  return TRUE;
}

/*************************************************************************//**
  Call a lua function.

  Example call to the lua function 'user_load()':
    success = script_fcdb_call("user_load", pconn);
*****************************************************************************/
bool script_fcdb_call(const char *func_name, ...)
{
  bool success = TRUE;
#ifdef HAVE_FCDB

  va_list args;
  va_start(args, func_name);

  success = luascript_func_call_valist(fcl, func_name, args);
  va_end(args);
#endif /* HAVE_FCDB */

  return success;
}

/*************************************************************************//**
  Free the scripting data.
*****************************************************************************/
void script_fcdb_free(void)
{
#ifdef HAVE_FCDB
  if (!script_fcdb_call("database_free", 0)) {
    log_error("Error closing the database connection. Continuing anyway...");
  }

  if (fcl) {
    /* luascript_func_free() is called by luascript_destroy(). */
    luascript_destroy(fcl);
    fcl = NULL;
  }
#endif /* HAVE_FCDB */
}

/*************************************************************************//**
  Parse and execute the script in str in the lua instance for the freeciv
  database.
*****************************************************************************/
bool script_fcdb_do_string(struct connection *caller, const char *str)
{
#ifdef HAVE_FCDB
  int status;
  struct connection *save_caller;
  luascript_log_func_t save_output_fct;

  /* Set a log callback function which allows to send the results of the
   * command to the clients. */
  save_caller = fcl->caller;
  save_output_fct = fcl->output_fct;
  fcl->output_fct = script_fcdb_cmd_reply;
  fcl->caller = caller;

  status = luascript_do_string(fcl, str, "cmd");

  /* Reset the changes. */
  fcl->caller = save_caller;
  fcl->output_fct = save_output_fct;

  return (status == 0);
#else
  return TRUE;
#endif /* HAVE_FCDB */
}
