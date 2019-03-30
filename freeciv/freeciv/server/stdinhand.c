/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef FREECIV_HAVE_LIBREADLINE
#include <readline/readline.h>
#endif

/* utility */
#include "astring.h"
#include "bitvector.h"
#include "fc_cmdline.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "registry.h"
#include "support.h"            /* fc__attribute, bool type, etc. */
#include "timing.h"
#include "section_file.h"

/* common */
#include "capability.h"
#include "events.h"
#include "fc_types.h" /* LINE_BREAK */
#include "featured_text.h"
#include "game.h"
#include "map.h"
#include "mapimg.h"
#include "packets.h"
#include "player.h"
#include "research.h"
#include "rgbcolor.h"
#include "srvdefs.h"
#include "unitlist.h"
#include "version.h"

/* server */
#include "aiiface.h"
#include "citytools.h"
#include "connecthand.h"
#include "diplhand.h"
#include "gamehand.h"
#include "mapgen.h"
#include "maphand.h"
#include "meta.h"
#include "notify.h"
#include "plrhand.h"
#include "report.h"
#include "ruleset.h"
#include "sanitycheck.h"
#include "savegame.h"
#include "score.h"
#include "sernet.h"
#include "settings.h"
#include "srv_log.h"
#include "srv_main.h"
#include "techtools.h"
#include "voting.h"

/* server/scripting */
#include "script_server.h"
#include "script_fcdb.h"

/* ai */
#include "difficulty.h"
#include "handicaps.h"

#include "stdinhand.h"

#define OPTION_NAME_SPACE 25

static enum cmdlevel default_access_level = ALLOW_BASIC;
static enum cmdlevel first_access_level = ALLOW_BASIC;

static time_t *time_duplicate(const time_t *t);

/* 'struct kick_hash' and related functions. */
#define SPECHASH_TAG kick
#define SPECHASH_ASTR_KEY_TYPE
#define SPECHASH_IDATA_TYPE time_t *
#define SPECHASH_UDATA_TYPE time_t
#define SPECHASH_IDATA_COPY time_duplicate
#define SPECHASH_IDATA_FREE (kick_hash_data_free_fn_t) free
#define SPECHASH_UDATA_TO_IDATA(t) (&(t))
#define SPECHASH_IDATA_TO_UDATA(p) (NULL != p ? *p : 0)
#include "spechash.h"

static struct kick_hash *kick_table_by_addr = NULL;
static struct kick_hash *kick_table_by_user = NULL;


static bool cut_client_connection(struct connection *caller, char *name,
                                  bool check);
static bool show_help(struct connection *caller, char *arg);
static bool show_list(struct connection *caller, char *arg);
static void show_colors(struct connection *caller);
static bool set_ai_level_named(struct connection *caller, const char *name,
                               const char *level_name, bool check);
static bool set_ai_level(struct connection *caller, const char *name,
                         enum ai_level level, bool check);
static bool away_command(struct connection *caller, bool check);
static bool set_rulesetdir(struct connection *caller, char *str, bool check,
                           int read_recursion);
static bool show_command(struct connection *caller, char *str, bool check);
static bool show_settings(struct connection *caller,
                          enum command_id called_as,
                          char *str, bool check);
static void show_settings_one(struct connection *caller, enum command_id cmd,
                              struct setting *pset);
static void show_ruleset_info(struct connection *caller, enum command_id cmd,
                              bool check, int read_recursion);
static void show_mapimg(struct connection *caller, enum command_id cmd);
static bool set_command(struct connection *caller, char *str, bool check);

static bool create_command(struct connection *caller, const char *str,
                           bool check);
static bool end_command(struct connection *caller, char *str, bool check);
static bool surrender_command(struct connection *caller, char *str, bool check);
static bool handle_stdin_input_real(struct connection *caller, char *str,
                                    bool check, int read_recursion);
static bool read_init_script_real(struct connection *caller,
                                  char *script_filename, bool from_cmdline,
                                  bool check, int read_recursion);
static bool reset_command(struct connection *caller, char *arg, bool check,
                          int read_recursion);
static bool default_command(struct connection *caller, char *arg, bool check);
static bool lua_command(struct connection *caller, char *arg, bool check);
static bool kick_command(struct connection *caller, char *name, bool check);
static bool delegate_command(struct connection *caller, char *arg,
                             bool check);
static const char *delegate_player_str(struct player *pplayer, bool observer);
static bool aicmd_command(struct connection *caller, char *arg, bool check);
static bool fcdb_command(struct connection *caller, char *arg, bool check);
static const char *fcdb_accessor(int i);
static char setting_status(struct connection *caller,
                           const struct setting *pset);
static bool player_name_check(const char* name, char *buf, size_t buflen);
static bool playercolor_command(struct connection *caller,
                                char *str, bool check);
static bool mapimg_command(struct connection *caller, char *arg, bool check);
static const char *mapimg_accessor(int i);

static void show_delegations(struct connection *caller);

static const char horiz_line[] =
"------------------------------------------------------------------------------";

/**********************************************************************//**
  Are we operating under a restricted security regime?  For now
  this does not do much.
**************************************************************************/
static bool is_restricted(struct connection *caller)
{
  return (caller && caller->access_level != ALLOW_HACK);
}

/**********************************************************************//**
  Check the player name. Returns TRUE if the player name is valid else
  an error message is saved in 'buf'.
**************************************************************************/
static bool player_name_check(const char* name, char *buf, size_t buflen)
{
  size_t len = strlen(name);

  if (len == 0) {
    fc_snprintf(buf, buflen, _("Can't use an empty name."));
    return FALSE;
  } else if (len > MAX_LEN_NAME-1) {
    fc_snprintf(buf, buflen, _("That name exceeds the maximum of %d chars."),
               MAX_LEN_NAME-1);
    return FALSE;
  } else if (fc_strcasecmp(name, ANON_PLAYER_NAME) == 0
             || fc_strcasecmp(name, "Observer") == 0) {
    fc_snprintf(buf, buflen, _("That name is not allowed."));
    /* "Observer" used to be illegal and we keep it that way for now. */
    /* FIXME: This disallows anonymous player name as it appears in English,
     * but not one in any other language that the user may see. */
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Convert a named command into an id.
  If accept_ambiguity is true, return the first command in the
  enum list which matches, else return CMD_AMBIGUOUS on ambiguity.
  (This is a trick to allow ambiguity to be handled in a flexible way
  without importing notify_player() messages inside this routine - rp)
**************************************************************************/
static enum command_id command_named(const char *token, bool accept_ambiguity)
{
  enum m_pre_result result;
  int ind;

  result = match_prefix(command_name_by_number, CMD_NUM, 0,
                        fc_strncasecmp, NULL, token, &ind);

  if (result < M_PRE_AMBIGUOUS) {
    return ind;
  } else if (result == M_PRE_AMBIGUOUS) {
    return accept_ambiguity ? ind : CMD_AMBIGUOUS;
  } else {
    return CMD_UNRECOGNIZED;
  }
}

/**********************************************************************//**
  Initialize stuff related to this code module.
**************************************************************************/
void stdinhand_init(void)
{
  fc_assert(NULL == kick_table_by_addr);
  kick_table_by_addr = kick_hash_new();

  fc_assert(NULL == kick_table_by_user);
  kick_table_by_user = kick_hash_new();
}

/**********************************************************************//**
  Update stuff every turn that is related to this code module. Run this
  on turn end.
**************************************************************************/
void stdinhand_turn(void)
{
  /* Nothing at the moment. */
}

/**********************************************************************//**
  Deinitialize stuff related to this code module.
**************************************************************************/
void stdinhand_free(void)
{
  fc_assert(NULL != kick_table_by_addr);
  if (NULL != kick_table_by_addr) {
    kick_hash_destroy(kick_table_by_addr);
    kick_table_by_addr = NULL;
  }

  fc_assert(NULL != kick_table_by_user);
  if (NULL != kick_table_by_user) {
    kick_hash_destroy(kick_table_by_user);
    kick_table_by_user = NULL;
  }
}

/**********************************************************************//**
  Whether the caller can use the specified command. caller == NULL means
  console.
**************************************************************************/
static bool may_use(struct connection *caller, enum command_id cmd)
{
  if (!caller) {
    return TRUE;  /* on the console, everything is allowed */
  }
  return (caller->access_level >= command_level(command_by_number(cmd)));
}

/**********************************************************************//**
  Whether the caller cannot use any commands at all.
  caller == NULL means console.
**************************************************************************/
static bool may_use_nothing(struct connection *caller)
{
  if (!caller) {
    return FALSE;  /* on the console, everything is allowed */
  }
  return (ALLOW_NONE == conn_get_access(caller));
}

/**********************************************************************//**
  Return the status of the setting (changeable, locked, fixed).
  caller == NULL means console.
**************************************************************************/
static char setting_status(struct connection *caller,
                           const struct setting *pset)
{
  /* first check for a ruleset lock as this is included in
   * setting_is_changeable() */
  if (setting_locked(pset)) {
    /* setting is locked by the ruleset */
    return '!';
  }

  if (setting_is_changeable(pset, caller, NULL, 0)) {
    /* setting can be changed */
    return '+';
  }

  /* setting is fixed */
  return ' ';
}

/**********************************************************************//**
  feedback related to server commands
  caller == NULL means console.
  No longer duplicate all output to console.

  This lowlevel function takes a single line; prefix is prepended to line.
**************************************************************************/
static void cmd_reply_line(enum command_id cmd, struct connection *caller,
			   enum rfc_status rfc_status, const char *prefix,
			   const char *line)
{
  const char *cmdname = cmd < CMD_NUM
                        ? command_name_by_number(cmd)
                        : cmd == CMD_AMBIGUOUS
                          /* TRANS: ambiguous command */
                          ? _("(ambiguous)")
                          : cmd == CMD_UNRECOGNIZED
                            /* TRANS: unrecognized command */
                            ? _("(unknown)")
                            : "(?!?)";  /* this case is a bug! */

  if (caller) {
    notify_conn(caller->self, NULL, E_SETTING, ftc_command,
                "/%s: %s%s", cmdname, prefix, line);
    /* cc: to the console - testing has proved it's too verbose - rp
    con_write(rfc_status, "%s/%s: %s%s", caller->name, cmdname, prefix, line);
    */
  } else {
    con_write(rfc_status, "%s%s", prefix, line);
  }

  if (rfc_status == C_OK) {
    struct packet_chat_msg packet;

    package_event(&packet, NULL, E_SETTING, ftc_server, "%s", line);
    conn_list_iterate(game.est_connections, pconn) {
      /* Do not tell caller, since he was told above! */
      if (caller != pconn) {
        send_packet_chat_msg(pconn, &packet);
      }
    } conn_list_iterate_end;
    event_cache_add_for_all(&packet);

    if (NULL != caller) {
      /* Echo to the console. */
      log_normal("%s", line);
    }
  }
}

/**********************************************************************//**
  va_list version which allow embedded newlines, and each line is sent
  separately. 'prefix' is prepended to every line _after_ the first line.
**************************************************************************/
static void vcmd_reply_prefix(enum command_id cmd, struct connection *caller,
                              enum rfc_status rfc_status, const char *prefix,
                              const char *format, va_list ap)
{
  char buf[4096];
  char *c0, *c1;

  fc_vsnprintf(buf, sizeof(buf), format, ap);

  c0 = buf;
  while ((c1 = strstr(c0, "\n"))) {
    *c1 = '\0';
    cmd_reply_line(cmd, caller, rfc_status, (c0 == buf ? "" : prefix), c0);
    c0 = c1 + 1;
  }

  cmd_reply_line(cmd, caller, rfc_status, (c0 == buf ? "" : prefix), c0);
}

/**********************************************************************//**
  var-args version of above
  duplicate declaration required for attribute to work...
**************************************************************************/
static void cmd_reply_prefix(enum command_id cmd, struct connection *caller,
			     enum rfc_status rfc_status, const char *prefix,
			     const char *format, ...)
     fc__attribute((__format__ (__printf__, 5, 6)));
static void cmd_reply_prefix(enum command_id cmd, struct connection *caller,
			     enum rfc_status rfc_status, const char *prefix,
			     const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vcmd_reply_prefix(cmd, caller, rfc_status, prefix, format, ap);
  va_end(ap);
}

/**********************************************************************//**
  var-args version as above, no prefix
**************************************************************************/
void cmd_reply(enum command_id cmd, struct connection *caller,
               enum rfc_status rfc_status, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vcmd_reply_prefix(cmd, caller, rfc_status, "", format, ap);
  va_end(ap);
}

/**********************************************************************//**
  Command specific argument parsing has detected that player argument
  is invalid. This function is common handling for that situation.
**************************************************************************/
static void cmd_reply_no_such_player(enum command_id cmd,
				     struct connection *caller,
				     const char *name,
				     enum m_pre_result match_result)
{
  switch(match_result) {
  case M_PRE_EMPTY:
    cmd_reply(cmd, caller, C_SYNTAX,
	      _("Name is empty, so cannot be a player."));
    break;
  case M_PRE_LONG:
    cmd_reply(cmd, caller, C_SYNTAX,
	      _("Name is too long, so cannot be a player."));
    break;
  case M_PRE_AMBIGUOUS:
    cmd_reply(cmd, caller, C_FAIL,
	      _("Player name prefix '%s' is ambiguous."), name);
    break;
  case M_PRE_FAIL:
    cmd_reply(cmd, caller, C_FAIL,
	      _("No player by the name of '%s'."), name);
    break;
  default:
    cmd_reply(cmd, caller, C_FAIL,
	      _("Unexpected match_result %d (%s) for '%s'."),
	      match_result, _(m_pre_description(match_result)), name);
    log_error("Unexpected match_result %d (%s) for '%s'.",
              match_result, m_pre_description(match_result), name);
  }
}

/**********************************************************************//**
  Command specific argument parsing has detected that connection argument
  is invalid. This function is common handling for that situation.
**************************************************************************/
static void cmd_reply_no_such_conn(enum command_id cmd,
				   struct connection *caller,
				   const char *name,
				   enum m_pre_result match_result)
{
  switch(match_result) {
  case M_PRE_EMPTY:
    cmd_reply(cmd, caller, C_SYNTAX,
	      _("Name is empty, so cannot be a connection."));
    break;
  case M_PRE_LONG:
    cmd_reply(cmd, caller, C_SYNTAX,
	      _("Name is too long, so cannot be a connection."));
    break;
  case M_PRE_AMBIGUOUS:
    cmd_reply(cmd, caller, C_FAIL,
	      _("Connection name prefix '%s' is ambiguous."), name);
    break;
  case M_PRE_FAIL:
    cmd_reply(cmd, caller, C_FAIL,
	      _("No connection by the name of '%s'."), name);
    break;
  default:
    cmd_reply(cmd, caller, C_FAIL,
	      _("Unexpected match_result %d (%s) for '%s'."),
	      match_result, _(m_pre_description(match_result)), name);
    log_error("Unexpected match_result %d (%s) for '%s'.",
              match_result, m_pre_description(match_result), name);
  }
}

/**********************************************************************//**
  Start sending game info to metaserver.
**************************************************************************/
static void open_metaserver_connection(struct connection *caller,
                                       bool persistent)
{
  server_open_meta(persistent);
  if (send_server_info_to_metaserver(META_INFO)) {
    cmd_reply(CMD_METACONN, caller, C_OK,
              _("Open metaserver connection to [%s]."),
              meta_addr_port());
  }
}

/**********************************************************************//**
  Stop sending game info to metaserver.
**************************************************************************/
static void close_metaserver_connection(struct connection *caller)
{
  if (send_server_info_to_metaserver(META_GOODBYE)) {
    server_close_meta();
    cmd_reply(CMD_METACONN, caller, C_OK,
              _("Close metaserver connection to [%s]."),
              meta_addr_port());
  }
}

/**********************************************************************//**
  Handle metaconnection command.
**************************************************************************/
static bool metaconnection_command(struct connection *caller, char *arg, 
                                   bool check)
{
  bool persistent = FALSE;

  if ((*arg == '\0')
      || (!strcmp(arg, "?"))) {
    if (is_metaserver_open()) {
      cmd_reply(CMD_METACONN, caller, C_COMMENT,
                _("Metaserver connection is open."));
    } else {
      cmd_reply(CMD_METACONN, caller, C_COMMENT,
                _("Metaserver connection is closed."));
    }
    return TRUE;
  }

  if (!fc_strcasecmp(arg, "p")
      || !fc_strcasecmp(arg, "persistent")) {
    persistent = TRUE;
  }

  if (persistent
      || !fc_strcasecmp(arg, "u")
      || !fc_strcasecmp(arg, "up")) {
    if (!is_metaserver_open()) {
      if (!check) {
        open_metaserver_connection(caller, persistent);
      }
    } else {
      cmd_reply(CMD_METACONN, caller, C_METAERROR,
                _("Metaserver connection is already open."));
      return FALSE;
    }
  } else if (!fc_strcasecmp(arg, "d")
             || !fc_strcasecmp(arg, "down")) {
    if (is_metaserver_open()) {
      if (!check) {
        close_metaserver_connection(caller);
      }
    } else {
      cmd_reply(CMD_METACONN, caller, C_METAERROR,
                _("Metaserver connection is already closed."));
      return FALSE;
    }
  } else {
    cmd_reply(CMD_METACONN, caller, C_METAERROR,
              _("Argument must be 'u', 'up', 'd', 'down', 'p', 'persistent', or '?'."));
    return FALSE;
  }
  return TRUE;
}

/**********************************************************************//**
  Handle metapatches command.
**************************************************************************/
static bool metapatches_command(struct connection *caller, 
                                char *arg, bool check)
{
  if (check) {
    return TRUE;
  }

  set_meta_patches_string(arg);

  if (is_metaserver_open()) {
    send_server_info_to_metaserver(META_INFO);
    cmd_reply(CMD_METAPATCHES, caller, C_OK,
              _("Metaserver patches string set to '%s'."), arg);
  } else {
    cmd_reply(CMD_METAPATCHES, caller, C_OK,
              _("Metaserver patches string set to '%s', "
                "not reporting to metaserver."), arg);
  }

  return TRUE;
}

/**********************************************************************//**
  Handle metamessage command.
**************************************************************************/
static bool metamessage_command(struct connection *caller, 
                                char *arg, bool check)
{
  struct setting *pset;

  if (is_longturn() && S_S_RUNNING == server_state()) {
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  set_user_meta_message_string(arg);
  if (is_metaserver_open()) {
    send_server_info_to_metaserver(META_INFO);
    cmd_reply(CMD_METAMESSAGE, caller, C_OK,
              _("Metaserver message string set to '%s'."), arg);
  } else {
    cmd_reply(CMD_METAMESSAGE, caller, C_OK,
              _("Metaserver message string set to '%s', "
                "not reporting to metaserver."), arg);
  }

  /* Metamessage is also a setting. */
  pset = setting_by_name("metamessage");
  setting_changed(pset);
  send_server_setting(NULL, pset);

  return TRUE;
}

/**********************************************************************//**
  Handle metaserver command.
**************************************************************************/
static bool metaserver_command(struct connection *caller, char *arg,
                               bool check)
{
  if (check) {
    return TRUE;
  }
  close_metaserver_connection(caller);

  sz_strlcpy(srvarg.metaserver_addr, arg);

  cmd_reply(CMD_METASERVER, caller, C_OK,
            _("Metaserver is now [%s]."), meta_addr_port());
  return TRUE;
}

/**********************************************************************//**
  Returns the serverid
**************************************************************************/
static bool show_serverid(struct connection *caller, char *arg)
{
  cmd_reply(CMD_SRVID, caller, C_COMMENT, _("Server id: %s"), srvarg.serverid);

  return TRUE;
}

/**********************************************************************//**
  For command "save foo";
  Save the game, with filename=arg, provided server state is ok.
**************************************************************************/
static bool save_command(struct connection *caller, char *arg, bool check)
{
  if (!check) {
    save_game(arg, "User request", FALSE);
  }
  return TRUE;
}

/**********************************************************************//**
  For command "scensave foo";
  Save the game, with filename=arg, provided server state is ok.
**************************************************************************/
static bool scensave_command(struct connection *caller, char *arg, bool check)
{
  if (is_restricted(caller)) {
    cmd_reply(CMD_SAVE, caller, C_FAIL,
              _("You cannot save games manually on this server."));
    return FALSE;
  }
  if (!check) {
    save_game(arg, "Scenario", TRUE);
  }
  return TRUE;
}

/**********************************************************************//**
  Handle ai player ai toggling.
**************************************************************************/
void toggle_ai_player_direct(struct connection *caller, struct player *pplayer)
{
  fc_assert_ret(pplayer != NULL);

  if (is_longturn() && S_S_RUNNING == server_state()) {
    return;
  }

  if (is_human(pplayer)) {
    cmd_reply(CMD_AITOGGLE, caller, C_OK,
	      _("%s is now under AI control."),
	      player_name(pplayer));
    player_set_to_ai_mode(pplayer,
                          !ai_level_is_valid(pplayer->ai_common.skill_level)
                          ? game.info.skill_level
                          : pplayer->ai_common.skill_level);
    fc_assert(is_ai(pplayer));
  } else {
    cmd_reply(CMD_AITOGGLE, caller, C_OK,
	      _("%s is now under human control."),
	      player_name(pplayer));
    player_set_under_human_control(pplayer);
    fc_assert(is_human(pplayer));
  }
}

/**********************************************************************//**
  Handle aitoggle command.
**************************************************************************/
static bool toggle_ai_command(struct connection *caller, char *arg, bool check)
{
  enum m_pre_result match_result;
  struct player *pplayer;

  if (is_longturn() && S_S_RUNNING == server_state()) {
    return FALSE;
  }

  pplayer = player_by_name_prefix(arg, &match_result);

  if (!pplayer) {
    cmd_reply_no_such_player(CMD_AITOGGLE, caller, arg, match_result);
    return FALSE;
  } else if (!check) {
    toggle_ai_player_direct(caller, pplayer);
    send_player_info_c(pplayer, game.est_connections);
  }
  return TRUE;
}

/**********************************************************************//**
  Creates a named AI player. The function can be called before the start
  of the game (see creat_command_pregame()) and for a running game
  (see creat_command_newcomer(). In the later case, first free player slots
  are used before the slots of dead players are (re)used.
**************************************************************************/
static bool create_command(struct connection *caller, const char *str,
                           bool check)
{
  enum rfc_status status;
  char buf[MAX_LEN_CONSOLE_LINE];

  /* 2 legal arguments, and extra space for stuffing illegal part */
  char *arg[3];
  int ntokens;
  const char *ai_type_name;

  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 3, TOKEN_DELIMITERS);

  if (ntokens == 1) {
    ai_type_name = default_ai_type_name();
  } else if (ntokens == 2) {
    ai_type_name = arg[1];
  } else {
    cmd_reply(CMD_CREATE, caller, C_SYNTAX,
              _("Wrong number of arguments to create command."));
    free_tokens(arg, ntokens);
    return FALSE;
  }

  if (game_was_started()) {
    status = create_command_newcomer(arg[0], ai_type_name, check,
                                     NULL, NULL, buf, sizeof(buf));
  } else {
    status = create_command_pregame(arg[0], ai_type_name, check,
                                    NULL, buf, sizeof(buf));
  }

  free_tokens(arg, ntokens);

  if (status != C_OK) {
    /* No player created. */
    cmd_reply(CMD_CREATE, caller, status, "%s", buf);
    return FALSE;
  }

  if (strlen(buf) > 0) {
    /* Send a notification. */
    cmd_reply(CMD_CREATE, caller, C_OK, "%s", buf);
  }

  return TRUE;
}

/**********************************************************************//**
  Try to add a player to a running game in the following order:

  1. Try to reuse the slot of a dead player with the username 'name'.
  2. Try to reuse the slot of a dead player.
  3. Try to use an empty player slot.

  If 'pnation' is defined this nation is used for the new player.
**************************************************************************/
enum rfc_status create_command_newcomer(const char *name,
                                        const char *ai,
                                        bool check,
                                        struct nation_type *pnation,
                                        struct player **newplayer,
                                        char *buf, size_t buflen)
{
  struct player *pplayer = NULL;
  struct research *presearch;
  bool new_slot = FALSE;

  /* Check player name. */
  if (!player_name_check(name, buf, buflen)) {
    return C_SYNTAX;
  }

  /* Check first if we can replace a player with
   * [1a] - the same username. */
  pplayer = player_by_user(name);
  if (pplayer && pplayer->is_alive) {
    fc_snprintf(buf, buflen,
                _("A living user already exists by that name."));
    return C_BOUNCE;
  }

  /* [1b] - the same player name. */
  pplayer = player_by_name(name);
  if (pplayer && pplayer->is_alive) {
    fc_snprintf(buf, buflen,
                _("A living player already exists by that name."));
    return C_BOUNCE;
  }

  if (pnation) {
    if (!nation_is_in_current_set(pnation)) {
      fc_snprintf(buf, buflen,
                  _("Can't create player, requested nation %s not in "
                    "current nation set."),
                  nation_plural_translation(pnation));
      return C_FAIL;
    }
    players_iterate(aplayer) {
      if (0 > nations_match(pnation, nation_of_player(aplayer), FALSE)) {
        fc_snprintf(buf, buflen,
                    _("Can't create players, nation %s conflicts with %s."),
                    nation_plural_for_player(aplayer),
                    nation_plural_for_player(pplayer));
        return C_FAIL;
      }
    } players_iterate_end;
  } else {
    /* Try to find a nation. */
    pnation = pick_a_nation(NULL, FALSE, TRUE, NOT_A_BARBARIAN);
    if (pnation == NO_NATION_SELECTED) {
      fc_snprintf(buf, buflen,
                  _("Can't create players, no nations available."));
      return C_FAIL;
    }
  }

  if (check) {
    /* All code below will change the game state. */

    /* Return an empty string. */
    buf[0] = '\0';

    return C_OK;
  }

  if (pplayer) {
    /* [1] Replace a player. 'pplayer' was set above. */
    fc_snprintf(buf, buflen,
                _("%s is replacing dead player %s as an AI-controlled "
                  "player."), name, player_name(pplayer));
    /* remove player and thus free a player slot */
    server_remove_player(pplayer);
    pplayer = NULL;
  } else if (player_count() == player_slot_count()) {
    /* [2] All player slots are used; try to remove a dead player. */
    players_iterate(aplayer) {
      if (!aplayer->is_alive) {
        fc_snprintf(buf, buflen,
                    _("%s is replacing dead player %s as an AI-controlled "
                      "player."), name, player_name(aplayer));
        /* remove player and thus free a player slot */
        server_remove_player(aplayer);
      }
    } players_iterate_end;
  } else {
    /* [3] An empty player slot must be used for the new player. */
    new_slot = TRUE;
  }

  if (new_slot) {
    if (normal_player_count() == game.server.max_players) {

      fc_assert(game.server.max_players < MAX_NUM_PLAYERS);

      game.server.max_players++;
      log_debug("Increased 'maxplayers' for creation of a new player.");
    }
  }

  /* Create the new player. */
  pplayer = server_create_player(-1, ai, NULL, FALSE);
  if (!pplayer) {
    fc_snprintf(buf, buflen, _("Failed to create new player %s."), name);
    return C_FAIL;
  }

  if (new_slot) {
    /* 'buf' must be set if a new player slot is used. */
    fc_snprintf(buf, buflen, _("New player %s created."), name);
  }

  /* We have a player; now initialise all needed data. */
  (void) aifill(game.info.aifill);

  /* Initialise player. */
  server_player_init(pplayer, TRUE, TRUE);

  player_nation_defaults(pplayer, pnation, FALSE);
  pplayer->government = pplayer->target_government =
    init_government_of_nation(pnation);
  /* Find a color for the new player. */
  assign_player_colors();

  /* TRANS: keep one space at the beginning of the string. */
  cat_snprintf(buf, buflen, _(" Nation of the new player: %s."),
               nation_rule_name(pnation));

  presearch = research_get(pplayer);
  init_tech(presearch, TRUE);
  give_initial_techs(presearch, 0);

  server_player_set_name(pplayer, name);
  sz_strlcpy(pplayer->username, _(ANON_USER_NAME));
  pplayer->unassigned_user = TRUE;

  pplayer->was_created = TRUE; /* must use /remove explicitly to remove */
  set_as_ai(pplayer);
  set_ai_level_directer(pplayer, game.info.skill_level);

  CALL_PLR_AI_FUNC(gained_control, pplayer, pplayer);

  send_player_info_c(pplayer, NULL);
  /* Send updated diplstate information to all players. */
  send_player_diplstate_c(NULL, NULL);
  /* Send research info after player info, else the client will complain
   * about invalid team. */
  send_research_info(presearch, NULL);
  (void) send_server_info_to_metaserver(META_INFO);

  if (newplayer != NULL) {
    *newplayer = pplayer;
  }
  return C_OK;
}

/**********************************************************************//**
  Create player in pregame.
**************************************************************************/
enum rfc_status create_command_pregame(const char *name,
                                       const char *ai,
                                       bool check,
                                       struct player **newplayer,
                                       char *buf, size_t buflen)
{
  char leader_name[MAX_LEN_NAME]; /* Must be in whole function scope */
  struct player *pplayer = NULL;
  bool rand_name = FALSE;

  if (name[0] == '\0') {
    int filled = 1;

    do {
      fc_snprintf(leader_name, sizeof(leader_name), "%s*%d", ai, filled++);
    } while (player_by_name(leader_name));

    name = leader_name;
    rand_name = TRUE;
  }

  if (!player_name_check(name, buf, buflen)) {
    return C_SYNTAX;
  }

  if (NULL != player_by_name(name)) {
    fc_snprintf(buf, buflen,
                _("A player already exists by that name."));
    return C_BOUNCE;
  }
  if (NULL != player_by_user(name)) {
    fc_snprintf(buf, buflen,
                _("A user already exists by that name."));
    return C_BOUNCE;
  }

  /* Search for first uncontrolled player */
  pplayer = find_uncontrolled_player();

  if (NULL == pplayer) {
    /* Check that we are not going over max players setting */
    if (normal_player_count() >= game.server.max_players) {
      fc_snprintf(buf, buflen,
                  _("Can't add more players, server is full."));
      return C_FAIL;
    }
    /* Check that we have nations available */
    if (normal_player_count() >= server.playable_nations) {
      if (nation_set_count() > 1) {
        fc_snprintf(buf, buflen,
                    _("Can't add more players, not enough playable nations "
                      "in current nation set (see 'nationset' setting)."));
      } else {
        fc_snprintf(buf, buflen,
                    _("Can't add more players, not enough playable nations."));
      }
      return C_FAIL;
    }
  }

  if (pplayer) {
    struct ai_type *ait = ai_type_by_name(ai);

    if (ait == NULL) {
      fc_snprintf(buf, buflen,
                  _("There is no AI type %s."), ai);
      return C_FAIL;
    }
  }

  if (check) {
    /* All code below will change the game state. */

    /* Return an empty string. */
    buf[0] = '\0';

    return C_OK;
  }

  if (pplayer) {
    fc_snprintf(buf, buflen,
                /* TRANS: <name> replacing <name> ... */
                _("%s replacing %s as an AI-controlled player."),
                name, player_name(pplayer));

    team_remove_player(pplayer);
    pplayer->ai = ai_type_by_name(ai);
  } else {
    /* add new player */
    pplayer = server_create_player(-1, ai, NULL, FALSE);
    /* pregame so no need to assign_player_colors() */
    if (!pplayer) {
      fc_snprintf(buf, buflen,
                  _("Failed to create new player %s."), name);
      return C_GENFAIL;
    }

    fc_snprintf(buf, buflen,
                _("%s has been added as an AI-controlled player (%s)."),
                name, ai_name(pplayer->ai));
  }
  server_player_init(pplayer, FALSE, TRUE);

  server_player_set_name(pplayer, name);
  sz_strlcpy(pplayer->username, _(ANON_USER_NAME));
  pplayer->unassigned_user = TRUE;

  pplayer->was_created = TRUE; /* must use /remove explicitly to remove */
  pplayer->random_name = rand_name;
  set_as_ai(pplayer);
  set_ai_level_directer(pplayer, game.info.skill_level);
  CALL_PLR_AI_FUNC(gained_control, pplayer, pplayer);
  send_player_info_c(pplayer, game.est_connections);

  (void) aifill(game.info.aifill);
  reset_all_start_commands(TRUE);
  (void) send_server_info_to_metaserver(META_INFO);

  if (newplayer != NULL) {
    *newplayer = pplayer;
  }
  return C_OK;
}

/**********************************************************************//**
  Handle remove command.
**************************************************************************/
static bool remove_player_command(struct connection *caller, char *arg,
                                  bool check)
{
  enum m_pre_result match_result;
  struct player *pplayer;
  char name[MAX_LEN_NAME];

  pplayer = player_by_name_prefix(arg, &match_result);

  if (NULL == pplayer) {
    cmd_reply_no_such_player(CMD_REMOVE, caller, arg, match_result);
    return FALSE;
  }

  if (game_was_started() && caller && caller->access_level < ALLOW_ADMIN) {
    cmd_reply(CMD_REMOVE, caller, C_FAIL,
              _("Command level '%s' or greater needed to remove a player "
                "once the game has started."), cmdlevel_name(ALLOW_ADMIN));
    return FALSE;
  }
  if (check) {
    return TRUE;
  }

  sz_strlcpy(name, player_name(pplayer));
  server_remove_player(pplayer);
  if (!caller || caller->used) {     /* may have removed self */
    cmd_reply(CMD_REMOVE, caller, C_OK,
	      _("Removed player %s from the game."), name);
  }
  (void) aifill(game.info.aifill);
  return TRUE;
}

/**********************************************************************//**
  Main entry point for the read command.
**************************************************************************/
static bool read_command(struct connection *caller, char *arg, bool check,
                         int read_recursion)
{
  return read_init_script_real(caller, arg, FALSE, check, read_recursion);
}

/**********************************************************************//**
  Main entry point for reading an init script.
**************************************************************************/
bool read_init_script(struct connection *caller, char *script_filename,
                      bool from_cmdline, bool check)
{
  return read_init_script_real(caller, script_filename, from_cmdline,
                               check, 0);
}

/**********************************************************************//**
  Returns FALSE iff there was an error.

  Security: We will look for a file with mandatory extension '.serv',
  and on public servers we will not look outside the data directories.
  As long as the user cannot create files with arbitrary names in the
  root of the data directories, this should ensure that we will not be 
  tricked into loading non-approved content. The script is read with the
  permissions of the caller, so it will in any case not lead to elevated
  permissions unless there are other bugs.
**************************************************************************/
static bool read_init_script_real(struct connection *caller,
                                  char *script_filename, bool from_cmdline,
                                  bool check, int read_recursion)
{
  FILE *script_file;
  const char extension[] = ".serv";
  char serv_filename[strlen(extension) + strlen(script_filename) + 2];
  char tilde_filename[4096];
  const char *real_filename;

  /* check recursion depth */
  if (read_recursion > GAME_MAX_READ_RECURSION) {
    log_error("Error: recursive calls to read!");
    return FALSE;
  }

  /* abuse real_filename to find if we already have a .serv extension */
  real_filename = script_filename + strlen(script_filename) 
                  - MIN(strlen(extension), strlen(script_filename));
  if (strcmp(real_filename, extension) != 0) {
    fc_snprintf(serv_filename, sizeof(serv_filename), "%s%s", 
                script_filename, extension);
  } else {
    sz_strlcpy(serv_filename, script_filename);
  }

  if (is_restricted(caller) && !from_cmdline) {
    if (!is_safe_filename(serv_filename)) {
      cmd_reply(CMD_READ_SCRIPT, caller, C_FAIL,
                _("Name \"%s\" disallowed for security reasons."),
                serv_filename);
      return FALSE;
    }
    sz_strlcpy(tilde_filename, serv_filename);
  } else {
    interpret_tilde(tilde_filename, sizeof(tilde_filename), serv_filename);
  }

  real_filename = fileinfoname(get_data_dirs(), tilde_filename);
  if (!real_filename) {
    if (is_restricted(caller) && !from_cmdline) {
      cmd_reply(CMD_READ_SCRIPT, caller, C_FAIL,
                _("No command script found by the name \"%s\"."), 
                serv_filename);
      return FALSE;
    }
    /* File is outside data directories */
    real_filename = tilde_filename;
  }

  log_testmatic_alt(LOG_NORMAL, _("Loading script file '%s'."), real_filename);

  if (is_reg_file_for_access(real_filename, FALSE)
      && (script_file = fc_fopen(real_filename, "r"))) {
    char buffer[MAX_LEN_CONSOLE_LINE];

    /* the size is set as to not overflow buffer in handle_stdin_input */
    while (fgets(buffer, MAX_LEN_CONSOLE_LINE - 1, script_file)) {
      /* Execute script contents with same permissions as caller */
      handle_stdin_input_real(caller, buffer, check, read_recursion + 1);
    }
    fclose(script_file);

    show_ruleset_info(caller, CMD_READ_SCRIPT, check, read_recursion);

    return TRUE;
  } else {
    cmd_reply(CMD_READ_SCRIPT, caller, C_FAIL,
              _("Cannot read command line scriptfile '%s'."), real_filename);
    if (NULL != caller) {
      log_error(_("Could not read script file '%s'."), real_filename);
    }
    return FALSE;
  }
}

/**********************************************************************//**
  Write current settings to new init script.

  (Should this take a 'caller' argument for output? --dwp)
**************************************************************************/
static void write_init_script(char *script_filename)
{
  char real_filename[1024], buf[256];
  FILE *script_file;
  
  interpret_tilde(real_filename, sizeof(real_filename), script_filename);

  if (is_reg_file_for_access(real_filename, TRUE)
      && (script_file = fc_fopen(real_filename, "w"))) {
    fprintf(script_file,
	"#FREECIV SERVER COMMAND FILE, version %s\n", VERSION_STRING);
    fputs("# These are server options saved from a running freeciv-server.\n",
	script_file);

    /* first rulesetdir. Setting rulesetdir resets the settings to their
     * default value, so they would be lost if placed before this.  */
    fprintf(script_file, "rulesetdir %s\n", game.server.rulesetdir);

    /* some state info from commands (we can't save everything) */

    fprintf(script_file, "cmdlevel %s new\n",
            cmdlevel_name(default_access_level));

    fprintf(script_file, "cmdlevel %s first\n",
            cmdlevel_name(first_access_level));

    fprintf(script_file, "%s\n",
            ai_level_cmd(game.info.skill_level));

    if (*srvarg.metaserver_addr != '\0' &&
	((0 != strcmp(srvarg.metaserver_addr, DEFAULT_META_SERVER_ADDR)))) {
      fprintf(script_file, "metaserver %s\n", meta_addr_port());
    }

    if (0 != strcmp(get_meta_patches_string(), default_meta_patches_string())) {
      fprintf(script_file, "metapatches %s\n", get_meta_patches_string());
    }
    if (0 != strcmp(get_meta_message_string(), default_meta_message_string())) {
      fprintf(script_file, "metamessage %s\n", get_meta_message_string());
    }

    /* then, the 'set' option settings */

    settings_iterate(SSET_ALL, pset) {
      fprintf(script_file, "set %s \"%s\"\n", setting_name(pset),
              setting_value_name(pset, FALSE, buf, sizeof(buf)));
    } settings_iterate_end;

    fclose(script_file);

  } else {
    log_error(_("Could not write script file '%s'."), real_filename);
  }
}

/**********************************************************************//**
  Generate init script from settings currently in use
**************************************************************************/
static bool write_command(struct connection *caller, char *arg, bool check)
{
  if (is_restricted(caller)) {
    cmd_reply(CMD_WRITE_SCRIPT, caller, C_FAIL,
              _("You cannot use the write command on this server"
              " for security reasons."));
    return FALSE;
  } else if (!check) {
    write_init_script(arg);
  }
  return TRUE;
}

/**********************************************************************//**
  Set ptarget's cmdlevel to level if caller is allowed to do so
**************************************************************************/
static bool set_cmdlevel(struct connection *caller,
                         struct connection *ptarget,
                         enum cmdlevel level)
{
  /* Only ever call me for specific connection. */
  fc_assert_ret_val(ptarget != NULL, FALSE);

  if (caller && ptarget->access_level > caller->access_level) {
    /*
     * This command is intended to be used at ctrl access level
     * and thus this if clause is needed.
     * (Imagine a ctrl level access player that wants to change
     * access level of a hack level access player)
     * At the moment it can be used only by hack access level 
     * and thus this clause is never used.
     */
    cmd_reply(CMD_CMDLEVEL, caller, C_FAIL,
              _("Cannot decrease command access level '%s' "
                "for connection '%s'; you only have '%s'."),
              cmdlevel_name(ptarget->access_level),
              ptarget->username,
              cmdlevel_name(caller->access_level));
    return FALSE;
  } else {
    conn_set_access(ptarget, level, TRUE);
    cmd_reply(CMD_CMDLEVEL, caller, C_OK,
              _("Command access level set to '%s' for connection %s."),
              cmdlevel_name(level), ptarget->username);
    return TRUE;
  }
}

/**********************************************************************//**
  Returns true if there is at least one established connection.
**************************************************************************/
static bool a_connection_exists(void)
{
  return conn_list_size(game.est_connections) > 0;
}

/**********************************************************************//**
  Return whether first access level is already taken.
**************************************************************************/
static bool is_first_access_level_taken(void)
{
  conn_list_iterate(game.est_connections, pconn) {
    if (pconn->access_level >= first_access_level) {
      return TRUE;
    }
  }
  conn_list_iterate_end;
  return FALSE;
}

/**********************************************************************//**
  Return access level for next connection
**************************************************************************/
enum cmdlevel access_level_for_next_connection(void)
{
  if ((first_access_level > default_access_level)
			&& !a_connection_exists()) {
    return first_access_level;
  } else {
    return default_access_level;
  }
}

/**********************************************************************//**
  Check if first access level is available and if it is, notify
  connections about it.
**************************************************************************/
void notify_if_first_access_level_is_available(void)
{
  if (first_access_level > default_access_level
      && !is_first_access_level_taken()) {
    notify_conn(NULL, NULL, E_SETTING, ftc_any,
                _("Anyone can now become game organizer "
                  "'%s' by issuing the 'first' command."),
                cmdlevel_name(first_access_level));
  }
}

/**********************************************************************//**
  Change command access level for individual player, or all, or new.
**************************************************************************/
static bool cmdlevel_command(struct connection *caller, char *str, bool check)
{
  char *arg[2];
  int ntokens;
  bool ret = FALSE;
  enum m_pre_result match_result;
  enum cmdlevel level;
  struct connection *ptarget;

  ntokens = get_tokens(str, arg, 2, TOKEN_DELIMITERS);

  if (ntokens == 0) {
    /* No argument supplied; list the levels */
    cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT, horiz_line);
    cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT,
              _("Command access levels in effect:"));
    cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT, horiz_line);
    conn_list_iterate(game.est_connections, pconn) {
      cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT, "cmdlevel %s %s",
                cmdlevel_name(conn_get_access(pconn)), pconn->username);
    } conn_list_iterate_end;
    cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT,
              _("Command access level for new connections: %s"),
              cmdlevel_name(default_access_level));
    cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT,
              _("Command access level for first player to take it: %s"),
              cmdlevel_name(first_access_level));
    cmd_reply(CMD_CMDLEVEL, caller, C_COMMENT, horiz_line);
    return TRUE;
  }

  /* A level name was supplied; set the level. */
  level = cmdlevel_by_name(arg[0], fc_strcasecmp);
  if (!cmdlevel_is_valid(level)) {
    const char *cmdlevel_names[CMDLEVEL_COUNT];
    struct astring astr = ASTRING_INIT;
    int i = 0;

    for (level = cmdlevel_begin(); level != cmdlevel_end();
         level = cmdlevel_next(level)) {
      cmdlevel_names[i++] = cmdlevel_name(level);
    }
    cmd_reply(CMD_CMDLEVEL, caller, C_SYNTAX,
              /* TRANS: comma and 'or' separated list of access levels */
              _("Command access level must be one of %s."),
              astr_build_or_list(&astr, cmdlevel_names, i));
    astr_free(&astr);
    goto CLEAN_UP;
  } else if (caller && level > conn_get_access(caller)) {
    cmd_reply(CMD_CMDLEVEL, caller, C_FAIL,
              _("Cannot increase command access level to '%s';"
                " you only have '%s' yourself."),
              arg[0], cmdlevel_name(conn_get_access(caller)));
    goto CLEAN_UP;
  }

  if (check) {
    return TRUE;                /* looks good */
  }

  if (ntokens == 1) {
    /* No playername supplied: set for all connections */
    conn_list_iterate(game.est_connections, pconn) {
      if (pconn != caller) {
        (void) set_cmdlevel(caller, pconn, level);
      }
    } conn_list_iterate_end;

    /* Set the caller access level at last, because it could make the
     * previous operations impossible if set before. */
    if (caller) {
      (void) set_cmdlevel(caller, caller, level);
    }

    /* Set default access for new connections. */
    default_access_level = level;
    cmd_reply(CMD_CMDLEVEL, caller, C_OK,
              _("Command access level set to '%s' for new players."),
              cmdlevel_name(level));
    /* Set default access for first connection. */
    first_access_level = level;
    cmd_reply(CMD_CMDLEVEL, caller, C_OK,
              _("Command access level set to '%s' "
                "for first player to grab it."),
              cmdlevel_name(level));

    ret = TRUE;

  } else if (fc_strcasecmp(arg[1], "new") == 0) {
    default_access_level = level;
    cmd_reply(CMD_CMDLEVEL, caller, C_OK,
              _("Command access level set to '%s' for new players."),
              cmdlevel_name(level));
    if (level > first_access_level) {
      first_access_level = level;
      cmd_reply(CMD_CMDLEVEL, caller, C_OK,
                _("Command access level set to '%s' "
                  "for first player to grab it."),
                cmdlevel_name(level));
    }

    ret = TRUE;

  } else if (fc_strcasecmp(arg[1], "first") == 0) {
    first_access_level = level;
    cmd_reply(CMD_CMDLEVEL, caller, C_OK,
              _("Command access level set to '%s' "
                "for first player to grab it."),
              cmdlevel_name(level));
    if (level < default_access_level) {
      default_access_level = level;
      cmd_reply(CMD_CMDLEVEL, caller, C_OK,
                _("Command access level set to '%s' for new players."),
                cmdlevel_name(level));
    }

    ret = TRUE;

  } else if ((ptarget = conn_by_user_prefix(arg[1], &match_result))) {
    if (set_cmdlevel(caller, ptarget, level)) {
      ret = TRUE;
    }
  } else {
    cmd_reply_no_such_conn(CMD_CMDLEVEL, caller, arg[1], match_result);
  }

CLEAN_UP:
  free_tokens(arg, ntokens);
  return ret;
}

/**********************************************************************//**
  This special command to set the command access level is not included into
  cmdlevel_command because of its lower access level: it can be used
  to promote one's own connection to 'first come' cmdlevel if that isn't
  already taken.
**************************************************************************/
static bool firstlevel_command(struct connection *caller, bool check)
{
  if (!caller) {
    cmd_reply(CMD_FIRSTLEVEL, caller, C_FAIL,
	_("The 'first' command makes no sense from the server command line."));
    return FALSE;
  } else if (caller->access_level >= first_access_level) {
    cmd_reply(CMD_FIRSTLEVEL, caller, C_FAIL,
	_("You already have command access level '%s' or better."),
		cmdlevel_name(first_access_level));
    return FALSE;
  } else if (is_first_access_level_taken()) {
    cmd_reply(CMD_FIRSTLEVEL, caller, C_FAIL,
	_("Someone else is already game organizer."));
    return FALSE;
  } else if (!check) {
    conn_set_access(caller, first_access_level, FALSE);
    cmd_reply(CMD_FIRSTLEVEL, caller, C_OK,
              _("Connection %s has opted to become the game organizer."),
              caller->username);
  }
  return TRUE;
}

/**********************************************************************//**
  Adjust default command level on game start.
**************************************************************************/
void set_running_game_access_level(void)
{
  if (default_access_level > ALLOW_BASIC) {
    notify_conn(NULL, NULL, E_SETTING, ftc_server,
                _("Default cmdlevel lowered to 'basic' on game start."));
    default_access_level = ALLOW_BASIC;
  }
}

/**********************************************************************//**
  Returns possible parameters for the commands that take server options
  as parameters (CMD_EXPLAIN and CMD_SET).
**************************************************************************/
static const char *optname_accessor(int i)
{
  return setting_name(setting_by_number(i));
}

#ifdef FREECIV_HAVE_LIBREADLINE
/**********************************************************************//**
  Returns possible parameters for the /show command.
**************************************************************************/
static const char *olvlname_accessor(int i)
{
  if (i == 0) {
    return "rulesetdir";
  } else if (i < OLEVELS_NUM+1) {
    return sset_level_name(i-1);
  } else {
    return optname_accessor(i-OLEVELS_NUM-1);
  }
}
#endif /* FREECIV_HAVE_LIBREADLINE */

/**********************************************************************//**
  Set timeout options.
**************************************************************************/
static bool timeout_command(struct connection *caller, char *str, bool check)
{
  char buf[MAX_LEN_CONSOLE_LINE];
  char *arg[4];
  int i = 0, ntokens;
  int *timeouts[4];

  timeouts[0] = &game.server.timeoutint;
  timeouts[1] = &game.server.timeoutintinc;
  timeouts[2] = &game.server.timeoutinc;
  timeouts[3] = &game.server.timeoutincmult;

  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 4, TOKEN_DELIMITERS);

  for (i = 0; i < ntokens; i++) {
    if (!str_to_int(arg[i], timeouts[i])) {
      cmd_reply(CMD_TIMEOUT, caller, C_FAIL, _("Invalid argument %d."),
                i + 1);
    }
    free(arg[i]);
  }

  if (ntokens == 0) {
    cmd_reply(CMD_TIMEOUT, caller, C_SYNTAX, _("Usage:\n%s"),
              command_synopsis(command_by_number(CMD_TIMEOUT)));
    return FALSE;
  } else if (check) {
    return TRUE;
  }

  cmd_reply(CMD_TIMEOUT, caller, C_OK, _("Dynamic timeout set to "
					 "%d %d %d %d"),
	    game.server.timeoutint, game.server.timeoutintinc,
	    game.server.timeoutinc, game.server.timeoutincmult);

  /* if we set anything here, reset the counter */
  game.server.timeoutcounter = 1;
  return TRUE;
}

/**********************************************************************//**
  Find option level number by name.
**************************************************************************/
static enum sset_level lookup_option_level(const char *name)
{
  enum sset_level i;

  for (i = SSET_ALL; i < OLEVELS_NUM; i++) {
    if (0 == fc_strcasecmp(name, sset_level_name(i))) {
      return i;
    }
  }

  return SSET_NONE;
}

/* Special return values of lookup options */
#define LOOKUP_OPTION_NO_RESULT   (-1)
#define LOOKUP_OPTION_AMBIGUOUS   (-2)
#define LOOKUP_OPTION_LEVEL_NAME  (-3)
#define LOOKUP_OPTION_RULESETDIR  (-4)

/**********************************************************************//**
  Find option index by name. Return index (>=0) on success, else returned
  - LOOKUP_OPTION_NO_RESULT   if no suitable options were found
  - LOOKUP_OPTION_AMBIGUOUS   if several matches were found
  - LOOKUP_OPTION_LEVEL_NAME  if it is an option level
  - LOOKUP_OPTION_RULESETDIR  if the argument is rulesetdir (special case)
**************************************************************************/
static int lookup_option(const char *name)
{
  enum m_pre_result result;
  int ind;

  /* Check for option levels, first off */
  if (lookup_option_level(name) != SSET_NONE) {
    return LOOKUP_OPTION_LEVEL_NAME;
  }

  result = match_prefix(optname_accessor, settings_number(),
                        0, fc_strncasecmp, NULL, name, &ind);
  if (M_PRE_AMBIGUOUS > result) {
    return ind;
  } else if (M_PRE_AMBIGUOUS == result) {
    return LOOKUP_OPTION_AMBIGUOUS;
  } else if ('\0' != name[0]
             && 0 == fc_strncasecmp("rulesetdir", name, strlen(name))) {
    return LOOKUP_OPTION_RULESETDIR;
  } else {
    return LOOKUP_OPTION_NO_RESULT;
  }
}

/**********************************************************************//**
  Show the caller detailed help for the single OPTION given by id.
  help_cmd is the command the player used.
  Only show option values for options which the caller can SEE.
**************************************************************************/
static void show_help_option(struct connection *caller,
                             enum command_id help_cmd, int id)
{
  char val_buf[256], def_buf[256];
  struct setting *pset = setting_by_number(id);
  const char *sethelp;

  if (setting_short_help(pset)) {
    cmd_reply(help_cmd, caller, C_COMMENT,
              /* TRANS: <untranslated name> - translated short help */
              _("Option: %s  -  %s"), setting_name(pset),
              _(setting_short_help(pset)));
  } else {
    cmd_reply(help_cmd, caller, C_COMMENT,
              /* TRANS: <untranslated name> */
              _("Option: %s"), setting_name(pset));
  }

  sethelp = setting_extra_help(pset, FALSE);
  if (strlen(sethelp) > 0) {
    char *help = fc_strdup(sethelp);

    fc_break_lines(help, LINE_BREAK);
    cmd_reply(help_cmd, caller, C_COMMENT, _("Description:"));
    cmd_reply_prefix(help_cmd, caller, C_COMMENT, "  ", "  %s", help);
    FC_FREE(help);
  }
  cmd_reply(help_cmd, caller, C_COMMENT,
            _("Status: %s"), (setting_is_changeable(pset, NULL, NULL, 0)
                              ? _("changeable") : _("fixed")));

  if (setting_is_visible(pset, caller)) {
    setting_value_name(pset, TRUE, val_buf, sizeof(val_buf));
    setting_default_name(pset, TRUE, def_buf, sizeof(def_buf));

    switch (setting_type(pset)) {
    case SST_INT:
      cmd_reply(help_cmd, caller, C_COMMENT, "%s %s, %s %d, %s %s, %s %d",
                _("Value:"), val_buf,
                _("Minimum:"), setting_int_min(pset),
                _("Default:"), def_buf,
                _("Maximum:"), setting_int_max(pset));
      break;
    case SST_ENUM:
      {
        int i;
        const char *value;

        cmd_reply(help_cmd, caller, C_COMMENT, _("Possible values:"));
        for (i = 0; (value = setting_enum_val(pset, i, FALSE)); i++) {
          cmd_reply(help_cmd, caller, C_COMMENT, "- %s: \"%s\"",
                    value, setting_enum_val(pset, i, TRUE));
        }
      }
      /* Fall through. */
    case SST_BOOL:
    case SST_STRING:
      cmd_reply(help_cmd, caller, C_COMMENT, "%s %s, %s %s",
                _("Value:"), val_buf, _("Default:"), def_buf);
      break;
    case SST_BITWISE:
      {
        int i;
        const char *value;

        cmd_reply(help_cmd, caller, C_COMMENT,
                  _("Possible values (option can take any number of these):"));
        for (i = 0; (value = setting_bitwise_bit(pset, i, FALSE)); i++) {
          cmd_reply(help_cmd, caller, C_COMMENT, "- %s: \"%s\"",
                    value, setting_bitwise_bit(pset, i, TRUE));
        }
        cmd_reply(help_cmd, caller, C_COMMENT, "%s %s",
                  _("Value:"), val_buf);
        cmd_reply(help_cmd, caller, C_COMMENT, "%s %s",
                  _("Default:"), def_buf);
      }
      break;
    case SST_COUNT:
      fc_assert(setting_type(pset) != SST_COUNT);
      break;
    }
  }
}

/**********************************************************************//**
  Show the caller list of OPTIONS.
  help_cmd is the command the player used.
  Only show options which the caller can SEE.
**************************************************************************/
static void show_help_option_list(struct connection *caller,
				  enum command_id help_cmd)
{
  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
  cmd_reply(help_cmd, caller, C_COMMENT,
	    _("Explanations are available for the following server options:"));
  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
  if (!caller && con_get_style()) {
    settings_iterate(SSET_ALL, pset) {
      cmd_reply(help_cmd, caller, C_COMMENT, "%s", setting_name(pset));
    } settings_iterate_end
  } else {
    char buf[MAX_LEN_CONSOLE_LINE];
    int j = 0;
    buf[0] = '\0';

    settings_iterate(SSET_ALL, pset) {
      if (setting_is_visible(pset, caller)) {
        cat_snprintf(buf, sizeof(buf), "%-19s", setting_name(pset));
        if ((++j % 4) == 0) {
          cmd_reply(help_cmd, caller, C_COMMENT, "%s", buf);
          buf[0] = '\0';
        }
      }
    } settings_iterate_end;

    if (buf[0] != '\0') {
      cmd_reply(help_cmd, caller, C_COMMENT, "%s", buf);
    }
  }
  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
}

/**********************************************************************//**
  Handle explain command
**************************************************************************/
static bool explain_option(struct connection *caller, char *str, bool check)
{
  int cmd;

  remove_leading_trailing_spaces(str);

  if (*str != '\0') {
    cmd = lookup_option(str);
    if (cmd >= 0 && cmd < settings_number()) {
      show_help_option(caller, CMD_EXPLAIN, cmd);
    } else if (cmd == LOOKUP_OPTION_NO_RESULT
               || cmd == LOOKUP_OPTION_LEVEL_NAME
               || cmd == LOOKUP_OPTION_RULESETDIR) {
      cmd_reply(CMD_EXPLAIN, caller, C_FAIL,
                _("No explanation for that yet."));
      return FALSE;
    } else if (cmd == LOOKUP_OPTION_AMBIGUOUS) {
      cmd_reply(CMD_EXPLAIN, caller, C_FAIL, _("Ambiguous option name."));
      return FALSE;
    } else {
      log_error("Unexpected case %d in %s line %d", cmd, __FILE__,
                __FC_LINE__);
      return FALSE;
    }
  } else {
    show_help_option_list(caller, CMD_EXPLAIN);
  }
  return TRUE;
}

/**********************************************************************//**
  Send a message to all players
**************************************************************************/
static bool wall(char *str, bool check)
{
  if (!check) {
    notify_conn(NULL, NULL, E_MESSAGE_WALL, ftc_server_prompt,
                _("Server Operator: %s"), str);
  }
  return TRUE;
}

/**********************************************************************//**
  Set message to send to all new connections
**************************************************************************/
static bool connectmsg_command(struct connection *caller, char *str,
                               bool check)
{
  unsigned int bufsize = sizeof(game.server.connectmsg);

  if (is_restricted(caller)) {
    return FALSE;
  }
  if (!check) {
    int i;
    int c = 0;

    for (i = 0; c < bufsize -1 && str[i] != '\0'; i++) {
      if (str[i] ==  '\\') {
        i++;

        if (str[i] == 'n') {
          game.server.connectmsg[c++] = '\n';
        } else {
          game.server.connectmsg[c++] = str[i];
        }
      } else {
        game.server.connectmsg[c++] = str[i];
      }
    }

    game.server.connectmsg[c++] = '\0';

    if (c == bufsize) {
      /* Truncated */
      cmd_reply(CMD_CONNECTMSG, caller, C_WARNING,
		_("Connectmsg truncated to %u bytes."), bufsize);
    }
  }
  return TRUE;
}

/**********************************************************************//**
  Translate an AI level back to its CMD_* value.
  If we just used /set ailevel <num> we wouldn't have to do this - rp
**************************************************************************/
static enum command_id cmd_of_level(enum ai_level level)
{
  switch (level) {
    case AI_LEVEL_AWAY         : return CMD_AWAY;
    case AI_LEVEL_HANDICAPPED  : return CMD_HANDICAPPED;
    case AI_LEVEL_NOVICE       : return CMD_NOVICE;
    case AI_LEVEL_EASY         : return CMD_EASY;
    case AI_LEVEL_NORMAL       : return CMD_NORMAL;
    case AI_LEVEL_HARD         : return CMD_HARD;
    case AI_LEVEL_CHEATING     : return CMD_CHEATING;
#ifdef FREECIV_DEBUG
    case AI_LEVEL_EXPERIMENTAL : return CMD_EXPERIMENTAL;
#endif /* FREECIV_DEBUG */
    case AI_LEVEL_COUNT        : return CMD_NORMAL;
  }
  log_error("Unknown AI level variant: %d.", level);
  return CMD_NORMAL;
}

/**********************************************************************//**
  Set an AI level from the server prompt.
**************************************************************************/
void set_ai_level_direct(struct player *pplayer, enum ai_level level)
{
  set_ai_level_directer(pplayer, level);
  send_player_info_c(pplayer, NULL);
  cmd_reply(cmd_of_level(level), NULL, C_OK,
	_("Player '%s' now has AI skill level '%s'."),
	player_name(pplayer),
	ai_level_translated_name(level));
  
}

/**********************************************************************//**
  Handle a user command to set an AI level.
**************************************************************************/
static bool set_ai_level_named(struct connection *caller, const char *name,
                               const char *level_name, bool check)
{
  enum ai_level level = ai_level_by_name(level_name, fc_strcasecmp);

  return set_ai_level(caller, name, level, check);
}

/**********************************************************************//**
  Set AI level
**************************************************************************/
static bool set_ai_level(struct connection *caller, const char *name,
                         enum ai_level level, bool check)
{
  enum m_pre_result match_result;
  struct player *pplayer;

  fc_assert_ret_val(level > 0 && level < 11, FALSE);

  pplayer = player_by_name_prefix(name, &match_result);

  if (pplayer) {
    if (is_ai(pplayer)) {
      if (check) {
        return TRUE;
      }
      set_ai_level_directer(pplayer, level);
      send_player_info_c(pplayer, NULL);
      cmd_reply(cmd_of_level(level), caller, C_OK,
		_("Player '%s' now has AI skill level '%s'."),
		player_name(pplayer),
		ai_level_translated_name(level));
    } else {
      cmd_reply(cmd_of_level(level), caller, C_FAIL,
		_("%s is not controlled by the AI."),
		player_name(pplayer));
      return FALSE;
    }
  } else if (match_result == M_PRE_EMPTY) {
    if (check) {
      return TRUE;
    }
    players_iterate(cplayer) {
      if (is_ai(cplayer)) {
        set_ai_level_directer(cplayer, level);
        send_player_info_c(cplayer, NULL);
        cmd_reply(cmd_of_level(level), caller, C_OK,
                  _("Player '%s' now has AI skill level '%s'."),
                  player_name(cplayer),
                  ai_level_translated_name(level));
      }
    } players_iterate_end;
    game.info.skill_level = level;
    send_game_info(NULL);
    cmd_reply(cmd_of_level(level), caller, C_OK,
              _("Default AI skill level set to '%s'."),
              ai_level_translated_name(level));
  } else {
    cmd_reply_no_such_player(cmd_of_level(level), caller, name, match_result);
    return FALSE;
  }
  return TRUE;
}

/**********************************************************************//**
  Set user to away mode.
**************************************************************************/
static bool away_command(struct connection *caller, bool check)
{
  struct player *pplayer;

  if (caller == NULL) {
    cmd_reply(CMD_AWAY, caller, C_FAIL, _("This command is client only."));
    return FALSE;
  }

  if (!conn_controls_player(caller)) {
    /* This happens for detached or observer connections. */
    cmd_reply(CMD_AWAY, caller, C_FAIL,
              _("Only players may use the away command."));
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  pplayer = conn_get_player(caller);
  if (is_human(pplayer)) {
    cmd_reply(CMD_AWAY, caller, C_OK,
              _("%s set to away mode."), player_name(pplayer));
    player_set_to_ai_mode(pplayer, AI_LEVEL_AWAY);
    fc_assert(!is_human(pplayer));
  } else {
    cmd_reply(CMD_AWAY, caller, C_OK,
              _("%s returned to game."), player_name(pplayer));
    player_set_under_human_control(pplayer);
    fc_assert(is_human(pplayer));
  }

  send_player_info_c(caller->playing, game.est_connections);

  return TRUE;
}

/**********************************************************************//**
  Show changed settings and ruleset summary.
**************************************************************************/
static void show_ruleset_info(struct connection *caller, enum command_id cmd,
                              bool check, int read_recursion)
{
  char *show_arg = "changed";

  /* show changed settings only at the top level of recursion */
  if (read_recursion != 0) {
    return;
  }

  show_settings(caller, cmd, show_arg, check);

  if (game.ruleset_summary != NULL) {
    char *translated = fc_strdup(_(game.ruleset_summary));

    fc_break_lines(translated, LINE_BREAK);
    cmd_reply(cmd, caller, C_COMMENT, "%s", translated);
    cmd_reply(cmd, caller, C_COMMENT, horiz_line);
    free(translated);
  }
}

/**********************************************************************//**
  /show command: show settings and their values.
**************************************************************************/
static bool show_command(struct connection *caller, char *str, bool check)
{
  return show_settings(caller, CMD_SHOW, str, check);
}

/**********************************************************************//**
  Print a summary of the settings and their values. Note that most values
  are at most 4 digits, except seeds, which we let overflow their columns,
  plus a sign character. Only show options which the caller can SEE.
**************************************************************************/
static bool show_settings(struct connection *caller,
                          enum command_id called_as,
                          char *str, bool check)
{
  int cmd;
  enum sset_level level = SSET_ALL;
  size_t clen = 0;

  remove_leading_trailing_spaces(str);
  if (str[0] != '\0') {
    /* In "/show forests", figure out that it's the forests option we're
     * looking at. */
    cmd = lookup_option(str);
    if (cmd >= 0) {
      /* Ignore levels when a particular option is specified. */
      level = SSET_NONE;

      if (!setting_is_visible(setting_by_number(cmd), caller)) {
        cmd_reply(called_as, caller, C_FAIL,
                  _("Sorry, you do not have access to view option '%s'."),
                  str);
        return FALSE;
      }
    }

    /* Valid negative values for 'cmd' are defined as LOOKUP_OPTION_*. */
    switch (cmd) {
    case LOOKUP_OPTION_NO_RESULT:
      cmd_reply(called_as, caller, C_FAIL, _("Unknown option '%s'."), str);
      return FALSE;
    case LOOKUP_OPTION_AMBIGUOUS:
      /* Allow ambiguous: show all matching. */
      clen = strlen(str);
      break;
    case LOOKUP_OPTION_LEVEL_NAME:
      /* Option level. */
      level = lookup_option_level(str);
      break;
    case LOOKUP_OPTION_RULESETDIR:
      /* Ruleset. */
      cmd_reply(called_as, caller, C_COMMENT,
                _("Current ruleset directory is \"%s\""),
                game.server.rulesetdir);
      return TRUE;
    }
  } else {
    /* to indicate that no command was specified */
    cmd = LOOKUP_OPTION_NO_RESULT;
    /* Use vital level by default. */
    level = SSET_VITAL;
  }

  fc_assert_ret_val(cmd >= 0 || cmd == LOOKUP_OPTION_AMBIGUOUS
                    || cmd == LOOKUP_OPTION_LEVEL_NAME
                    || cmd == LOOKUP_OPTION_NO_RESULT, FALSE);

#define cmd_reply_show(string)                                               \
  cmd_reply(called_as, caller, C_COMMENT, "%s", string)

  {
    const char *heading = NULL;
    switch(level) {
      case SSET_NONE:
        break;
      case SSET_CHANGED:
        heading = _("All options with non-default values");
        break;
      case SSET_ALL:
        heading = _("All options");
        break;
      case SSET_VITAL:
        heading = _("Vital options");
        break;
      case SSET_SITUATIONAL:
        heading = _("Situational options");
        break;
      case SSET_RARE:
        heading = _("Rarely used options");
        break;
      case SSET_LOCKED:
        heading = _("Options locked by the ruleset");
        break;
      case OLEVELS_NUM:
        /* nothing */
        break;
    }
    if (heading) {
      cmd_reply_show(horiz_line);
      cmd_reply_show(heading);
    }
  }
  cmd_reply_show(horiz_line);
  cmd_reply_show(_("In the column '##' the status of the option is shown:"));
  cmd_reply_show(_(" - a '!' means the option is locked by the ruleset."));
  cmd_reply_show(_(" - a '+' means you may change the option."));
  cmd_reply_show(_(" - a '~' means that option follows default value."));
  cmd_reply_show(_(" - a '=' means the value is same as default."));
  cmd_reply_show(horiz_line);
  cmd_reply(called_as, caller, C_COMMENT, _("%-*s ## value (min, max)"),
            OPTION_NAME_SPACE, _("Option"));
  cmd_reply_show(horiz_line);

  /* Update changed and locked levels. */
  settings_list_update();

  switch(level) {
  case SSET_NONE:
    /* Show _one_ setting. */
    fc_assert_ret_val(0 <= cmd, FALSE);
    {
      struct setting *pset = setting_by_number(cmd);

      show_settings_one(caller, called_as, pset);
    }
    break;
  case SSET_CHANGED:
  case SSET_ALL:
  case SSET_VITAL:
  case SSET_SITUATIONAL:
  case SSET_RARE:
  case SSET_LOCKED:
    settings_iterate(level, pset) {
      if (!setting_is_visible(pset, caller)) {
        continue;
      }

      if (LOOKUP_OPTION_AMBIGUOUS == cmd
          && 0 != fc_strncasecmp(setting_name(pset), str, clen)) {
          continue;
      }

      show_settings_one(caller, called_as, pset);
    } settings_iterate_end;
    break;
  case OLEVELS_NUM:
    /* nothing */
    break;
  }

  cmd_reply_show(horiz_line);
  /* Only emit this additional help for bona fide 'show' command */
  if (called_as == CMD_SHOW) {
    cmd_reply_show(_("A help text for each option is available via 'help "
                     "<option>'."));
    cmd_reply_show(horiz_line);
    if (level == SSET_VITAL) {
      cmd_reply_show(_("Try 'show situational' or 'show rare' to show "
                       "more options.\n"
                       "Try 'show changed' to show settings with "
                       "non-default values.\n"
                       "Try 'show locked' to show settings locked "
                       "by the ruleset."));
      cmd_reply_show(horiz_line);
    }
  }
  return TRUE;
#undef cmd_reply_show
}

/**********************************************************************//**
  Show one setting.

  Each option value will be displayed as:

  [OPTION_NAME_SPACE length for name] ## [value] ([min], [max])

  where '##' is a combination of ' ', '!' or '+' followed by ' ', '*', or '=' with
   - '!': the option is locked by the ruleset
   - '+': you may change the option
   - '~': the option follows default value
   - '=': the value is same as default
**************************************************************************/
static void show_settings_one(struct connection *caller, enum command_id cmd,
                              struct setting *pset)
{
  char buf[MAX_LEN_CONSOLE_LINE] = "", value[MAX_LEN_CONSOLE_LINE] = "";
  bool is_changed;
  static char prefix[OPTION_NAME_SPACE + 4 + 1] = "";
  char defaultness;

  fc_assert_ret(pset != NULL);

  is_changed = setting_non_default(pset);
  setting_value_name(pset, TRUE, value, sizeof(value));

  /* Wrap long option values, such as bitwise options */
  fc_break_lines(value, LINE_BREAK - (sizeof(prefix)-1));

  if (prefix[0] == '\0') {
    memset(prefix, ' ', sizeof(prefix)-1);
  }

  if (is_changed) {
    /* Emphasizes the changed option. */
    /* Apply tags to each line fragment. */
    size_t startpos = 0;
    char *nl;
    do {
      nl = strchr(value + startpos, '\n');
      featured_text_apply_tag(value, buf, sizeof(buf), TTT_COLOR,
                              startpos, nl ? nl - value : FT_OFFSET_UNSET,
                              ftc_changed);
      sz_strlcpy(value, buf);
      if (nl) {
        char *p = strchr(nl, '\n');
        fc_assert_action(p != NULL, break);
        startpos = p + 1 - value;
      }
    } while (nl);
  }

  if (SST_INT == setting_type(pset)) {
    /* Add the range. */
    cat_snprintf(value, sizeof(value), " (%d, %d)",
                 setting_int_min(pset), setting_int_max(pset));
  }

  if (setting_get_setdef(pset) == SETDEF_INTERNAL) {
    defaultness = '~';
  } else if (is_changed) {
    defaultness = ' ';
  } else {
    defaultness = '=';
  }

  cmd_reply_prefix(cmd, caller, C_COMMENT, prefix, "%-*s %c%c %s",
                   OPTION_NAME_SPACE, setting_name(pset),
                   setting_status(caller, pset), defaultness,
                   value);
}

/**********************************************************************//**
  Handle team command
**************************************************************************/
static bool team_command(struct connection *caller, char *str, bool check)
{
  struct player *pplayer;
  enum m_pre_result match_result;
  char buf[MAX_LEN_CONSOLE_LINE];
  char *arg[2];
  int ntokens = 0, i;
  bool res = FALSE;
  struct team_slot *tslot;

  if (game_was_started()) {
    cmd_reply(CMD_TEAM, caller, C_SYNTAX,
              _("Cannot change teams once game has begun."));
    return FALSE;
  }

  if (str != NULL || strlen(str) > 0) {
    sz_strlcpy(buf, str);
    ntokens = get_tokens(buf, arg, 2, TOKEN_DELIMITERS);
  }
  if (ntokens != 2) {
    cmd_reply(CMD_TEAM, caller, C_SYNTAX,
              _("Undefined argument.  Usage:\n%s"),
              command_synopsis(command_by_number(CMD_TEAM)));
    goto cleanup;
  }

  pplayer = player_by_name_prefix(arg[0], &match_result);
  if (pplayer == NULL) {
    cmd_reply_no_such_player(CMD_TEAM, caller, arg[0], match_result);
    goto cleanup;
  }

  tslot = team_slot_by_rule_name(arg[1]);
  if (NULL == tslot) {
    int teamno;

    if (str_to_int(arg[1], &teamno)) {
      tslot = team_slot_by_number(teamno);
    }
  }
  if (NULL == tslot) {
    cmd_reply(CMD_TEAM, caller, C_SYNTAX,
              _("No such team %s.  Please give a "
              "valid team name or number."), arg[1]);
    goto cleanup;
  }

  if (is_barbarian(pplayer)) {
    /* This can happen if we change team settings on a loaded game. */
    cmd_reply(CMD_TEAM, caller, C_SYNTAX, _("Cannot team a barbarian."));
    goto cleanup;
  }
  if (!check) {
    team_add_player(pplayer, team_new(tslot));
    send_player_info_c(pplayer, NULL);
    cmd_reply(CMD_TEAM, caller, C_OK, _("Player %s set to team %s."),
              player_name(pplayer),
              team_slot_name_translation(tslot));
  }
  res = TRUE;

  cleanup:
  for (i = 0; i < ntokens; i++) {
    free(arg[i]);
  }
  return res;
}

/**********************************************************************//**
  List all running votes. Moved from /vote command.
**************************************************************************/
static void show_votes(struct connection *caller)
{
  int count = 0;
  const char *title;

  if (vote_list != NULL) {
    vote_list_iterate(vote_list, pvote) {
      if (NULL != caller && !conn_can_see_vote(caller, pvote)) {
        continue;
      }
      /* TRANS: "Vote" or "Teamvote" is voting-as-a-process. Used as
       * part of a sentence. */
      title = vote_is_team_only(pvote) ? _("Teamvote") : _("Vote");
      cmd_reply(CMD_VOTE, caller, C_COMMENT,
                /* TRANS: "[Vote|Teamvote] 3 \"proposed change\" (needs ..." */
                _("%s %d \"%s\" (needs %0.0f%%%s): %d for, "
                  "%d against, and %d abstained out of %d players."),
                title, pvote->vote_no, pvote->cmdline,
                MIN(100, pvote->need_pc * 100 + 1),
                pvote->flags & VCF_NODISSENT ? _(" no dissent") : "",
                pvote->yes, pvote->no, pvote->abstain, count_voters(pvote));
      count++;
    } vote_list_iterate_end;
  }

  if (count == 0) {
    cmd_reply(CMD_VOTE, caller, C_COMMENT,
              _("There are no votes going on."));
  }
}

/**********************************************************************//**
  Vote command argument definitions.
**************************************************************************/
static const char *const vote_args[] = {
  "yes",
  "no",
  "abstain",
  NULL
};
static const char *vote_arg_accessor(int i)
{
  return vote_args[i];
}

/**********************************************************************//**
  Make or participate in a vote.
**************************************************************************/
static bool vote_command(struct connection *caller, char *str,
                         bool check)
{
  char buf[MAX_LEN_CONSOLE_LINE];
  char *arg[2];
  int ntokens = 0, i = 0, which = -1;
  enum m_pre_result match_result;
  struct vote *pvote = NULL;
  bool res = FALSE;

  if (check) {
    /* This should never happen, since /vote must always be
     * set to ALLOW_BASIC or less. But just in case... */
    return FALSE;
  }

  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 2, TOKEN_DELIMITERS);

  if (ntokens == 0) {
    show_votes(caller);
    goto CLEANUP;
  } else if (!conn_can_vote(caller, NULL)) {
    cmd_reply(CMD_VOTE, caller, C_FAIL,
              _("You are not allowed to use this command."));
    goto CLEANUP;
  }

  match_result = match_prefix(vote_arg_accessor, VOTE_NUM, 0,
                              fc_strncasecmp, NULL, arg[0], &i);

  if (match_result == M_PRE_AMBIGUOUS) {
    cmd_reply(CMD_VOTE, caller, C_SYNTAX,
              _("The argument \"%s\" is ambiguous."), arg[0]);
    goto CLEANUP;
  } else if (match_result > M_PRE_AMBIGUOUS) {
    /* Failed */
    cmd_reply(CMD_VOTE, caller, C_SYNTAX,
              _("Undefined argument.  Usage:\n%s"),
              command_synopsis(command_by_number(CMD_VOTE)));
    goto CLEANUP;
  }

  if (ntokens == 1) {
    /* Applies to last vote */
    if (vote_number_sequence > 0 && get_vote_by_no(vote_number_sequence)) {
      which = vote_number_sequence;
    } else {
      int num_votes = vote_list_size(vote_list);
      if (num_votes == 0) {
        cmd_reply(CMD_VOTE, caller, C_FAIL, _("There are no votes running."));
      } else {
        /* TRANS: "vote" as a process */
        cmd_reply(CMD_VOTE, caller, C_FAIL, _("No legal last vote (%d %s)."),
                  num_votes, PL_("other vote running", "other votes running",
                                 num_votes));
      }
      goto CLEANUP;
    }
  } else {
    if (!str_to_int(arg[1], &which)) {
      cmd_reply(CMD_VOTE, caller, C_SYNTAX, _("Value must be an integer."));
      goto CLEANUP;
    }
  }

  if (!(pvote = get_vote_by_no(which))) {
    /* TRANS: "vote" as a process */
    cmd_reply(CMD_VOTE, caller, C_FAIL, _("No such vote (%d)."), which);
    goto CLEANUP;
  }

  if (!conn_can_vote(caller, pvote)) {
    cmd_reply(CMD_VOTE, caller, C_FAIL,
              _("You are not allowed to vote on that."));
    goto CLEANUP;
  }

  if (i == VOTE_YES) {
    cmd_reply(CMD_VOTE, caller, C_COMMENT, _("You voted for \"%s\""),
              pvote->cmdline);
    connection_vote(caller, pvote, VOTE_YES);
  } else if (i == VOTE_NO) {
    cmd_reply(CMD_VOTE, caller, C_COMMENT, _("You voted against \"%s\""),
              pvote->cmdline);
    connection_vote(caller, pvote, VOTE_NO);
  } else if (i == VOTE_ABSTAIN) {
    cmd_reply(CMD_VOTE, caller, C_COMMENT,
              _("You abstained from voting on \"%s\""), pvote->cmdline);
    connection_vote(caller, pvote, VOTE_ABSTAIN);
  } else {
    /* Must never happen. */
    fc_assert_action(FALSE, goto CLEANUP);
  }

  res = TRUE;

CLEANUP:
  free_tokens(arg, ntokens);
  return res;
}

/**********************************************************************//**
  Cancel a vote... /cancelvote <vote number>|all.
**************************************************************************/
static bool cancelvote_command(struct connection *caller,
                               char *arg, bool check)
{
  struct vote *pvote = NULL;
  int vote_no;

  if (check) {
    /* This should never happen anyway, since /cancelvote
     * is set to ALLOW_BASIC in both pregame and while the
     * game is running. */
    return FALSE;
  }

  remove_leading_trailing_spaces(arg);

  if (arg[0] == '\0') {
    if (caller == NULL) {
      /* Server prompt */
      cmd_reply(CMD_CANCELVOTE, caller, C_SYNTAX,
                /* TRANS: "vote" as a process */
                _("Missing argument <vote number> or "
                  "the string \"all\"."));
      return FALSE;
    }
    /* The caller cancel his/her own vote. */
    if (!(pvote = get_vote_by_caller(caller))) {
      cmd_reply(CMD_CANCELVOTE, caller, C_FAIL,
                _("You don't have any vote going on."));
      return FALSE;
    }
  } else if (fc_strcasecmp(arg, "all") == 0) {
    /* Cancel all votes (needs some privileges). */
    if (vote_list_size(vote_list) == 0) {
      cmd_reply(CMD_CANCELVOTE, caller, C_FAIL,
                _("There isn't any vote going on."));
      return FALSE;
    } else if (!caller || conn_get_access(caller) >= ALLOW_ADMIN) {
      clear_all_votes();
      notify_conn(NULL, NULL, E_VOTE_ABORTED, ftc_server,
                  /* TRANS: "votes" as a process */
                  _("All votes have been removed."));
      return TRUE;
    } else {
      cmd_reply(CMD_CANCELVOTE, caller, C_FAIL,
                _("You are not allowed to use this command."));
      return FALSE;
    }
  } else if (str_to_int(arg, &vote_no)) {
    /* Cancel one particular vote (needs some privileges if the vote
     * is not owned). */
    if (!(pvote = get_vote_by_no(vote_no))) {
      cmd_reply(CMD_CANCELVOTE, caller, C_FAIL,
                /* TRANS: "vote" as a process */
                _("No such vote (%d)."), vote_no);
      return FALSE;
    } else if (caller && conn_get_access(caller) < ALLOW_ADMIN
               && caller->id != pvote->caller_id) {
      cmd_reply(CMD_CANCELVOTE, caller, C_FAIL,
                /* TRANS: "vote" as a process */
                _("You are not allowed to cancel this vote (%d)."),
                vote_no);
      return FALSE;
    }
  } else {
    cmd_reply(CMD_CANCELVOTE, caller, C_SYNTAX,
              /* TRANS: "vote" as a process */
              _("Usage: /cancelvote [<vote number>|all]"));
    return FALSE;
  }

  fc_assert_ret_val(NULL != pvote, FALSE);

  if (caller) {
    notify_team(conn_get_player(vote_get_caller(pvote)),
                NULL, E_VOTE_ABORTED, ftc_server,
                /* TRANS: "vote" as a process */
                _("%s has canceled the vote \"%s\" (number %d)."),
                caller->username, pvote->cmdline, pvote->vote_no);
  } else {
    /* Server prompt */
    notify_team(conn_get_player(vote_get_caller(pvote)),
                NULL, E_VOTE_ABORTED, ftc_server,
                /* TRANS: "vote" as a process */
                _("The vote \"%s\" (number %d) has been canceled."),
                pvote->cmdline, pvote->vote_no);
  }
  /* Make it after, prevent crashs about a free pointer (pvote). */
  remove_vote(pvote);

  return TRUE;
}

/**********************************************************************//**
  Turn on selective debugging.
**************************************************************************/
static bool debug_command(struct connection *caller, char *str,
                          bool check)
{
  char buf[MAX_LEN_CONSOLE_LINE];
  char *arg[3];
  int ntokens = 0, i;

  if (game.info.is_new_game) {
    cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
              _("Can only use this command once game has begun."));
    return FALSE;
  }
  if (check) {
    return TRUE; /* whatever! */
  }

  if (str != NULL && strlen(str) > 0) {
    sz_strlcpy(buf, str);
    ntokens = get_tokens(buf, arg, 3, TOKEN_DELIMITERS);
  } else {
    ntokens = 0;
  }

  if (ntokens > 0 && strcmp(arg[0], "diplomacy") == 0) {
    struct player *pplayer;
    enum m_pre_result match_result;

    if (ntokens != 2) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
                _("Undefined argument.  Usage:\n%s"),
                command_synopsis(command_by_number(CMD_DEBUG)));
      goto cleanup;
    }
    pplayer = player_by_name_prefix(arg[1], &match_result);
    if (pplayer == NULL) {
      cmd_reply_no_such_player(CMD_DEBUG, caller, arg[1], match_result);
      goto cleanup;
    }
    if (BV_ISSET(pplayer->server.debug, PLAYER_DEBUG_DIPLOMACY)) {
      BV_CLR(pplayer->server.debug, PLAYER_DEBUG_DIPLOMACY);
      cmd_reply(CMD_DEBUG, caller, C_OK, _("%s diplomacy no longer debugged"), 
                player_name(pplayer));
    } else {
      BV_SET(pplayer->server.debug, PLAYER_DEBUG_DIPLOMACY);
      cmd_reply(CMD_DEBUG, caller, C_OK, _("%s diplomacy debugged"), 
                player_name(pplayer));
      /* TODO: print some info about the player here */
    } 
  } else if (ntokens > 0 && strcmp(arg[0], "tech") == 0) {
    struct player *pplayer;
    enum m_pre_result match_result;

    if (ntokens != 2) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
                _("Undefined argument.  Usage:\n%s"),
                command_synopsis(command_by_number(CMD_DEBUG)));
      goto cleanup;
    }
    pplayer = player_by_name_prefix(arg[1], &match_result);
    if (pplayer == NULL) {
      cmd_reply_no_such_player(CMD_DEBUG, caller, arg[1], match_result);
      goto cleanup;
    }
    if (BV_ISSET(pplayer->server.debug, PLAYER_DEBUG_TECH)) {
      BV_CLR(pplayer->server.debug, PLAYER_DEBUG_TECH);
      cmd_reply(CMD_DEBUG, caller, C_OK, _("%s tech no longer debugged"), 
                player_name(pplayer));
    } else {
      BV_SET(pplayer->server.debug, PLAYER_DEBUG_TECH);
      cmd_reply(CMD_DEBUG, caller, C_OK, _("%s tech debugged"), 
                player_name(pplayer));
      /* TODO: print some info about the player here */
    }
  } else if (ntokens > 0 && strcmp(arg[0], "info") == 0) {
    int cities = 0, players = 0, units = 0, citizen_count = 0;

    players_iterate(plr) {
      players++;
      city_list_iterate(plr->cities, pcity) {
        cities++;
        citizen_count += city_size_get(pcity);
      } city_list_iterate_end;
      units += unit_list_size(plr->units);
    } players_iterate_end;
    log_normal(_("players=%d cities=%d citizens=%d units=%d"),
               players, cities, citizen_count, units);
    notify_conn(game.est_connections, NULL, E_AI_DEBUG, ftc_log,
                _("players=%d cities=%d citizens=%d units=%d"),
                players, cities, citizen_count, units);
  } else if (ntokens > 0 && strcmp(arg[0], "city") == 0) {
    int x, y;
    struct tile *ptile;
    struct city *pcity;

    if (ntokens != 3) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
                _("Undefined argument.  Usage:\n%s"),
                command_synopsis(command_by_number(CMD_DEBUG)));
      goto cleanup;
    }
    if (!str_to_int(arg[1], &x) || !str_to_int(arg[2], &y)) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("Value 2 & 3 must be integer."));
      goto cleanup;
    }
    if (!(ptile = map_pos_to_tile(&(wld.map), x, y))) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("Bad map coordinates."));
      goto cleanup;
    }
    pcity = tile_city(ptile);
    if (!pcity) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("No city at this coordinate."));
      goto cleanup;
    }
    if (pcity->server.debug) {
      pcity->server.debug = FALSE;
      cmd_reply(CMD_DEBUG, caller, C_OK, _("%s no longer debugged"),
                city_name_get(pcity));
    } else {
      pcity->server.debug = TRUE;
      CITY_LOG(LOG_NORMAL, pcity, "debugged");
    }
  } else if (ntokens > 0 && strcmp(arg[0], "units") == 0) {
    int x, y;
    struct tile *ptile;

    if (ntokens != 3) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
                _("Undefined argument.  Usage:\n%s"),
                command_synopsis(command_by_number(CMD_DEBUG)));
      goto cleanup;
    }
    if (!str_to_int(arg[1], &x) || !str_to_int(arg[2], &y)) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("Value 2 & 3 must be integer."));
      goto cleanup;
    }
    if (!(ptile = map_pos_to_tile(&(wld.map), x, y))) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("Bad map coordinates."));
      goto cleanup;
    }
    unit_list_iterate(ptile->units, punit) {
      if (punit->server.debug) {
        punit->server.debug = FALSE;
        cmd_reply(CMD_DEBUG, caller, C_OK, _("%s %s no longer debugged."),
                  nation_adjective_for_player(unit_owner(punit)),
                  unit_name_translation(punit));
      } else {
        punit->server.debug = TRUE;
        UNIT_LOG(LOG_NORMAL, punit, "%s %s debugged.",
                 nation_rule_name(nation_of_unit(punit)),
                 unit_name_translation(punit));
      }
    } unit_list_iterate_end;
  } else if (ntokens > 0 && strcmp(arg[0], "timing") == 0) {
    TIMING_RESULTS();
  } else if (ntokens > 0 && strcmp(arg[0], "ferries") == 0) {
    if (game.server.debug[DEBUG_FERRIES]) {
      game.server.debug[DEBUG_FERRIES] = FALSE;
      cmd_reply(CMD_DEBUG, caller, C_OK, _("Ferry system is no longer "
                "in debug mode."));
    } else {
      game.server.debug[DEBUG_FERRIES] = TRUE;
      cmd_reply(CMD_DEBUG, caller, C_OK, _("Ferry system in debug mode."));
    }
  } else if (ntokens > 0 && strcmp(arg[0], "unit") == 0) {
    int id;
    struct unit *punit;

    if (ntokens != 2) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
              _("Undefined argument.  Usage:\n%s"),
              command_synopsis(command_by_number(CMD_DEBUG)));
      goto cleanup;
    }
    if (!str_to_int(arg[1], &id)) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("Value 2 must be integer."));
      goto cleanup;
    }
    if (!(punit = game_unit_by_number(id))) {
      cmd_reply(CMD_DEBUG, caller, C_SYNTAX, _("Unit %d does not exist."), id);
      goto cleanup;
    }
    if (punit->server.debug) {
      punit->server.debug = FALSE;
      cmd_reply(CMD_DEBUG, caller, C_OK, _("%s %s no longer debugged."),
                nation_adjective_for_player(unit_owner(punit)),
                unit_name_translation(punit));
    } else {
      punit->server.debug = TRUE;
      UNIT_LOG(LOG_NORMAL, punit, "%s %s debugged.",
               nation_rule_name(nation_of_unit(punit)),
               unit_name_translation(punit));
    }
  } else {
    cmd_reply(CMD_DEBUG, caller, C_SYNTAX,
              _("Undefined argument.  Usage:\n%s"),
              command_synopsis(command_by_number(CMD_DEBUG)));
  }
  cleanup:
  for (i = 0; i < ntokens; i++) {
    free(arg[i]);
  }
  return TRUE;
}

/**********************************************************************//**
  Helper to validate an argument referring to a server setting.
  Sends error message and returns NULL on failure.
**************************************************************************/
static struct setting *validate_setting_arg(enum command_id cmd,
                                            struct connection *caller,
                                            char *arg)
{
  int opt = lookup_option(arg);

  if (opt < 0) {
    switch (opt) {
    case LOOKUP_OPTION_NO_RESULT:
    case LOOKUP_OPTION_LEVEL_NAME:
      cmd_reply(cmd, caller, C_SYNTAX, _("Option '%s' not recognized."), arg);
      break;
    case LOOKUP_OPTION_AMBIGUOUS:
      cmd_reply(cmd, caller, C_SYNTAX, _("Ambiguous option name."));
      break;
    case LOOKUP_OPTION_RULESETDIR:
      cmd_reply(cmd, caller, C_SYNTAX,
                /* TRANS: 'rulesetdir' is the command. Do not translate. */
                _("Use the '%srulesetdir' command to change the ruleset "
                  "directory."), caller ? "/" : "");
      break;
    default:
      fc_assert(opt >= LOOKUP_OPTION_RULESETDIR);
      break;
    }
    return NULL;
  }

  return setting_by_number(opt);
}

/**********************************************************************//**
  Handle set command
**************************************************************************/
static bool set_command(struct connection *caller, char *str, bool check)
{
  char *args[2];
  int val, nargs;
  struct setting *pset;
  bool do_update;
  char reject_msg[256] = "";
  bool ret = FALSE;

  /* '=' is also a valid delimiter for this function. */
  nargs = get_tokens(str, args, ARRAY_SIZE(args), TOKEN_DELIMITERS "=");

  if (nargs < 2) {
    cmd_reply(CMD_SET, caller, C_SYNTAX,
              _("Undefined argument.  Usage:\n%s"),
              command_synopsis(command_by_number(CMD_SET)));
    goto cleanup;
  }

  pset = validate_setting_arg(CMD_SET, caller, args[0]);

  if (!pset) {
    /* Reason already reported. */
    goto cleanup;
  }

  if (!setting_is_changeable(pset, caller, reject_msg, sizeof(reject_msg))
      && !check) {
    cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
    goto cleanup;
  }

  do_update = FALSE;

  switch (setting_type(pset)) {
  case SST_BOOL:
    if (check) {
      if (!setting_is_changeable(pset, caller, reject_msg,
                                 sizeof(reject_msg))
          || (!setting_bool_validate(pset, args[1], caller,
                                     reject_msg, sizeof(reject_msg)))) {
        cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
        goto cleanup;
      }
    } else if (setting_bool_set(pset, args[1], caller,
                                reject_msg, sizeof(reject_msg))) {
      do_update = TRUE;
    } else {
      cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
      goto cleanup;
    }
    break;

  case SST_INT:
    if (!str_to_int(args[1], &val)) {
      cmd_reply(CMD_SET, caller, C_SYNTAX,
                _("The parameter %s should only contain +- and 0-9."),
                setting_name(pset));
      goto cleanup;
    }
    if (check) {
      if (!setting_is_changeable(pset, caller, reject_msg,
                                 sizeof(reject_msg))
          || !setting_int_validate(pset, val, caller, reject_msg,
                                   sizeof(reject_msg))) {
        cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
        goto cleanup;
      }
    } else {
      if (setting_int_set(pset, val, caller, reject_msg,
                          sizeof(reject_msg))) {
        do_update = TRUE;
      } else {
        cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
        goto cleanup;
      }
    }
    break;

  case SST_STRING:
    if (check) {
      if (!setting_is_changeable(pset, caller, reject_msg,
                                 sizeof(reject_msg))
          || !setting_str_validate(pset, args[1], caller, reject_msg,
                                   sizeof(reject_msg))) {
        cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
        goto cleanup;
      }
    } else {
      if (setting_str_set(pset, args[1], caller, reject_msg,
          sizeof(reject_msg))) {
        do_update = TRUE;
      } else {
        cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
        goto cleanup;
      }
    }
    break;

  case SST_ENUM:
    if (check) {
      if (!setting_is_changeable(pset, caller, reject_msg,
                                 sizeof(reject_msg))
          || (!setting_enum_validate(pset, args[1], caller,
                                     reject_msg, sizeof(reject_msg)))) {
        cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
        goto cleanup;
      }
    } else if (setting_enum_set(pset, args[1], caller,
                                reject_msg, sizeof(reject_msg))) {
      do_update = TRUE;
    } else {
      cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
      goto cleanup;
    }
    break;

  case SST_BITWISE:
    if (check) {
      if (!setting_is_changeable(pset, caller, reject_msg,
                                 sizeof(reject_msg))
          || (!setting_bitwise_validate(pset, args[1], caller,
                                        reject_msg, sizeof(reject_msg)))) {
        cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
        goto cleanup;
      }
    } else if (setting_bitwise_set(pset, args[1], caller,
                                   reject_msg, sizeof(reject_msg))) {
      do_update = TRUE;
    } else {
      cmd_reply(CMD_SET, caller, C_FAIL, "%s", reject_msg);
      goto cleanup;
    }
    break;

  case SST_COUNT:
    fc_assert(setting_type(pset) != SST_COUNT);
    goto cleanup;
    break;
  }

  ret = TRUE;   /* Looks like a success. */

  if (!check && do_update) {
    /* Send only to connections able to see that. */
    char buf[256];
    struct packet_chat_msg packet;

    package_event(&packet, NULL, E_SETTING, ftc_server,
                  _("Console: '%s' has been set to %s."), setting_name(pset),
                  setting_value_name(pset, TRUE, buf, sizeof(buf)));
    conn_list_iterate(game.est_connections, pconn) {
      if (setting_is_visible(pset, pconn)) {
        send_packet_chat_msg(pconn, &packet);
      }
    } conn_list_iterate_end;
    /* Notify the console. */
    con_write(C_OK, "%s", packet.message);

    setting_changed(pset);
    setting_action(pset);
    send_server_setting(NULL, pset);
    /* 
     * send any modified game parameters to the clients -- if sent
     * before S_S_RUNNING, triggers a popdown_races_dialog() call
     * in client/packhand.c#handle_game_info() 
     */
    send_game_info(NULL);
    reset_all_start_commands(FALSE);
    send_server_info_to_metaserver(META_INFO);
  }

  cleanup:
  free_tokens(args, nargs);
  return ret;
}

/**********************************************************************//**
  Check game.allow_take and fcdb if enabled for permission to take or
  observe a player.

  NB: If this function returns FALSE, then callers expect that 'msg' will
  be filled in with a NULL-terminated string containing the reason.
**************************************************************************/
static bool is_allowed_to_take(struct connection *requester,
                               struct connection *taker,
                               struct player *pplayer, bool will_obs,
                               char *msg, size_t msg_len)
{
  const char *allow;

  if (!pplayer && !will_obs) {
    /* Auto-taking a new player */

    if (game_was_started()) {
      fc_strlcpy(msg, _("You cannot take a new player at this time."),
                msg_len);
      return FALSE;
    }

    if (normal_player_count() >= game.server.max_players) {
      fc_snprintf(msg, msg_len,
                  /* TRANS: Do not translate "maxplayers". */
                  PL_("You cannot take a new player because "
                      "the maximum of %d player has already "
                      "been reached (maxplayers setting).",
                      "You cannot take a new player because "
                      "the maximum of %d players has already "
                      "been reached (maxplayers setting).",
                      game.server.max_players),
                  game.server.max_players);
      return FALSE;
    }

    if (player_count() >= player_slot_count()) {
      fc_strlcpy(msg, _("You cannot take a new player because there "
                       "are no free player slots."),
                msg_len);
      return FALSE;
    }

    return TRUE;

  }

#ifdef HAVE_FCDB
  if (srvarg.fcdb_enabled) {
    bool ok = FALSE;

    if (script_fcdb_call("user_take", requester, taker, pplayer, will_obs,
			 &ok) && ok) {
      return TRUE;
    }
  }
#endif

  if (!pplayer && will_obs) {
    /* Global observer. */
    if (!(allow = strchr(game.server.allow_take,
                         (game.info.is_new_game ? 'O' : 'o')))) {
      fc_strlcpy(msg, _("Sorry, one can't observe globally in this game."),
                msg_len);
      return FALSE;
    }
  } else if (is_barbarian(pplayer)) {
    if (!(allow = strchr(game.server.allow_take, 'b'))) {
      if (will_obs) {
        fc_strlcpy(msg,
                   _("Sorry, one can't observe barbarians in this game."),
                   msg_len);
      } else {
        fc_strlcpy(msg, _("Sorry, one can't take barbarians in this game."),
                   msg_len);
      }
      return FALSE;
    }
  } else if (!pplayer->is_alive) {
    if (!(allow = strchr(game.server.allow_take, 'd'))) {
      if (will_obs) {
        fc_strlcpy(msg,
                   _("Sorry, one can't observe dead players in this game."),
                   msg_len);
      } else {
        fc_strlcpy(msg,
                   _("Sorry, one can't take dead players in this game."),
                   msg_len);
      }
      return FALSE;
    }
  } else if (is_ai(pplayer)) {
    if (!(allow = strchr(game.server.allow_take,
                         (game.info.is_new_game ? 'A' : 'a')))) {
      if (will_obs) {
        fc_strlcpy(msg,
                   _("Sorry, one can't observe AI players in this game."),
                   msg_len);
      } else {
        fc_strlcpy(msg, _("Sorry, one can't take AI players in this game."),
                  msg_len);
      }
      return FALSE;
    }
  } else { 
    if (!(allow = strchr(game.server.allow_take,
                         (game.info.is_new_game ? 'H' : 'h')))) {
      if (will_obs) {
        fc_strlcpy(msg,
                   _("Sorry, one can't observe human players in this game."),
                   msg_len);
      } else {
        fc_strlcpy(msg,
                   _("Sorry, one can't take human players in this game."),
                   msg_len);
      }
      return FALSE;
    }
  }

  allow++;

  if (will_obs && (*allow == '2' || *allow == '3')) {
    fc_strlcpy(msg, _("Sorry, one can't observe in this game."), msg_len);
    return FALSE;
  }

  if (!will_obs && *allow == '4') {
    fc_strlcpy(msg, _("Sorry, one can't take players in this game."),
              MAX_LEN_MSG);
    return FALSE;
  }

  if (!will_obs && pplayer->is_connected
      && (*allow == '1' || *allow == '3')) {
    fc_strlcpy(msg, _("Sorry, one can't take players already "
                      "connected in this game."), msg_len);
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Observe another player. If we were already attached, detach
  (see connection_detach()). The console and those with ALLOW_HACK can
  use the two-argument command and force others to observe.
**************************************************************************/
static bool observe_command(struct connection *caller, char *str, bool check)
{
  int i = 0, ntokens = 0;
  char buf[MAX_LEN_CONSOLE_LINE], *arg[2], msg[MAX_LEN_MSG];  
  bool is_newgame = !game_was_started();
  enum m_pre_result result;
  struct connection *pconn = NULL;
  struct player *pplayer = NULL;
  bool res = FALSE;
  
  /******** PART I: fill pconn and pplayer ********/

  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 2, TOKEN_DELIMITERS);
  
  /* check syntax, only certain syntax if allowed depending on the caller */
  if (!caller && ntokens < 1) {
    cmd_reply(CMD_OBSERVE, caller, C_SYNTAX, _("Usage:\n%s"),
              command_synopsis(command_by_number(CMD_OBSERVE)));
    goto end;
  } 

  if (ntokens == 2 && (caller && caller->access_level != ALLOW_HACK)) {
    cmd_reply(CMD_OBSERVE, caller, C_SYNTAX,
              _("Only the player name form is allowed."));
    goto end;
  }

  /* match connection if we're console, match a player if we're not */
  if (ntokens == 1) {
    if (!caller && !(pconn = conn_by_user_prefix(arg[0], &result))) {
      cmd_reply_no_such_conn(CMD_OBSERVE, caller, arg[0], result);
      goto end;
    } else if (caller
               && !(pplayer = player_by_name_prefix(arg[0], &result))) {
      cmd_reply_no_such_player(CMD_OBSERVE, caller, arg[0], result);
      goto end;
    }
  }

  /* get connection name then player name */
  if (ntokens == 2) {
    if (!(pconn = conn_by_user_prefix(arg[0], &result))) {
      cmd_reply_no_such_conn(CMD_OBSERVE, caller, arg[0], result);
      goto end;
    }
    if (!(pplayer = player_by_name_prefix(arg[1], &result))) {
      cmd_reply_no_such_player(CMD_OBSERVE, caller, arg[1], result);
      goto end;
    }
  }

  /* if we can't force other connections to observe, assign us to be pconn. */
  if (!pconn) {
    pconn = caller;
  }

  /* if we have no pplayer, it means that we want to be a global observer */

  /******** PART II: do the observing ********/

  /* check allowtake for permission */
  if (!is_allowed_to_take(caller, pconn, pplayer, TRUE, msg, sizeof(msg))) {
    cmd_reply(CMD_OBSERVE, caller, C_FAIL, "%s", msg);
    goto end;
  }

  /* observing your own player (during pregame) makes no sense. */
  if (NULL != pplayer
      && pplayer == pconn->playing
      && !pconn->observer
      && is_newgame
      && !pplayer->was_created) {
    cmd_reply(CMD_OBSERVE, caller, C_FAIL, 
              _("%s already controls %s. Using 'observe' would remove %s"),
              pconn->username,
              player_name(pplayer),
              player_name(pplayer));
    goto end;
  }

  /* attempting to observe a player you're already observing should fail. */
  if (pplayer == pconn->playing && pconn->observer) {
    if (pplayer) {
      cmd_reply(CMD_OBSERVE, caller, C_FAIL,
		_("%s is already observing %s."),  
		pconn->username,
		player_name(pplayer));
    } else {
      cmd_reply(CMD_OBSERVE, caller, C_FAIL,
		_("%s is already observing."),  
		pconn->username);
    }
    goto end;
  }

  res = TRUE; /* all tests passed */
  if (check) {
    goto end;
  }

  /* if the connection is already attached to a player,
   * unattach and cleanup old player (rename, remove, etc) */
  if (TRUE) {
    char name[MAX_LEN_NAME];

    if (pplayer) {
      /* if pconn->playing is removed, we'll lose pplayer */
      sz_strlcpy(name, player_name(pplayer));
    }

    connection_detach(pconn, TRUE);

    if (pplayer) {
      /* find pplayer again, the pointer might have been changed */
      pplayer = player_by_name(name);
    }
  } 

  /* attach pconn to new player as an observer or as global observer */
  if ((res = connection_attach(pconn, pplayer, TRUE))) {
    if (pplayer) {
      cmd_reply(CMD_OBSERVE, caller, C_OK, _("%s now observes %s"),
                pconn->username,
                player_name(pplayer));
    } else {
      cmd_reply(CMD_OBSERVE, caller, C_OK, _("%s now observes"),
                pconn->username);
    }
  }

  end:;
  /* free our args */
  for (i = 0; i < ntokens; i++) {
    free(arg[i]);
  }
  return res;
}

/**********************************************************************//**
  Take over a player. If a connection already has control of that player, 
  disallow it. 

  If there are two arguments, treat the first as the connection name and the
  second as the player name (only hack and the console can do this).
  Otherwise, there should be one argument, that being the player that the 
  caller wants to take.
**************************************************************************/
static bool take_command(struct connection *caller, char *str, bool check)
{
  int i = 0, ntokens = 0;
  char buf[MAX_LEN_CONSOLE_LINE], *arg[2], msg[MAX_LEN_MSG];
  bool is_newgame = !game_was_started();
  enum m_pre_result match_result;
  struct connection *pconn = caller;
  struct player *pplayer = NULL;
  bool res = FALSE;

  /******** PART I: fill pconn and pplayer ********/

  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 2, TOKEN_DELIMITERS);

  /* check syntax */
  if (!caller && ntokens != 2) {
    cmd_reply(CMD_TAKE, caller, C_SYNTAX, _("Usage:\n%s"),
              command_synopsis(command_by_number(CMD_TAKE)));
    goto end;
  }

  if (caller && caller->access_level != ALLOW_HACK && ntokens != 1) {
    cmd_reply(CMD_TAKE, caller, C_SYNTAX,
              _("Only the player name form is allowed."));
    goto end;
  }

  if (ntokens == 0) {
    cmd_reply(CMD_TAKE, caller, C_SYNTAX, _("Usage:\n%s"),
              command_synopsis(command_by_number(CMD_TAKE)));
    goto end;
  }

  if (ntokens == 2) {
    if (!(pconn = conn_by_user_prefix(arg[i], &match_result))) {
      cmd_reply_no_such_conn(CMD_TAKE, caller, arg[i], match_result);
      goto end;
    }
    i++; /* found a conn, now reference the second argument */
  }

  if (strcmp(arg[i], "-") == 0) {
    if (!is_newgame) {
      cmd_reply(CMD_TAKE, caller, C_FAIL,
                _("You cannot issue \"/take -\" when "
                  "the game has already started."));
      goto end;
    }

    /* Find first uncontrolled player. This will return NULL if there is
     * no free players at the moment. Later call to
     * connection_attach() will create new player for such NULL
     * cases. */
    pplayer = find_uncontrolled_player();
    if (pplayer) {
      /* Make it human! */
      set_as_human(pplayer);
    }
  } else if (!(pplayer = player_by_name_prefix(arg[i], &match_result))) {
    cmd_reply_no_such_player(CMD_TAKE, caller, arg[i], match_result);
    goto end;
  }

  /******** PART II: do the attaching ********/

  /* Take not possible if the player is involved in a delegation (either
   * it's being controlled, or it's been put aside by the delegate). */
  if (player_delegation_active(pplayer)) {
    cmd_reply(CMD_TAKE, caller, C_FAIL, _("A delegation is active for player "
                                          "'%s'. /take not possible."),
              player_name(pplayer));
    goto end;
  }

  /* check allowtake for permission */
  if (!is_allowed_to_take(caller, pconn, pplayer, FALSE, msg, sizeof(msg))) {
    cmd_reply(CMD_TAKE, caller, C_FAIL, "%s", msg);
    goto end;
  }

  /* taking your own player makes no sense. */
  if ((NULL != pplayer && !pconn->observer && pplayer == pconn->playing)
   || (NULL == pplayer && !pconn->observer && NULL != pconn->playing)) {
    cmd_reply(CMD_TAKE, caller, C_FAIL, _("%s already controls %s."),
              pconn->username,
              player_name(pconn->playing));
    goto end;
  }

  /* Make sure there is free player slot if there is need to
   * create new player. This is necessary for previously
   * detached connections only. Others can reuse the slot
   * they first release. */
  if (!pplayer && !pconn->playing
      && (normal_player_count() >= game.server.max_players
          || normal_player_count() >= server.playable_nations)) {
    cmd_reply(CMD_TAKE, caller, C_FAIL,
              _("There is no free player slot for %s."),
              pconn->username);
    goto end;
  }
  fc_assert_action(player_count() <= player_slot_count(), goto end);

  res = TRUE;
  if (check) {
    goto end;
  }

  /* If the player is controlled by another user, forcibly detach
   * the user. */
  if (pplayer && pplayer->is_connected) {
    if (NULL == caller) {
       notify_conn(NULL, NULL, E_CONNECTION, ftc_server,
                  _("Reassigned nation to %s by server console."),
                  pconn->username);
    } else {
      notify_conn(NULL, NULL, E_CONNECTION, ftc_server,
                  _("Reassigned nation to %s by %s."),
                  pconn->username,
                  caller->username);
    }

    /* We are reassigning this nation, so we need to detach the current
     * user to set a new one. */
    conn_list_iterate(pplayer->connections, aconn) {
      if (!aconn->observer) {
        connection_detach(aconn, FALSE);
      }
    } conn_list_iterate_end;
  }

  /* if the connection is already attached to another player,
   * unattach and cleanup old player (rename, remove, etc)
   * We may have been observing the player we now want to take */
  if (NULL != pconn->playing || pconn->observer) {
    char name[MAX_LEN_NAME];

    if (pplayer) {
      /* if pconn->playing is removed, we'll lose pplayer */
      sz_strlcpy(name, player_name(pplayer));
    }

    connection_detach(pconn, TRUE);

    if (pplayer) {
      /* find pplayer again; the pointer might have been changed */
      pplayer = player_by_name(name);
    }
  }

  /* Now attach to new player */
  if ((res = connection_attach(pconn, pplayer, FALSE))) {
    /* Successfully attached */
    pplayer = pconn->playing; /* In case pplayer was NULL. */

    /* inform about the status before changes */
    cmd_reply(CMD_TAKE, caller, C_OK, _("%s now controls %s (%s, %s)."),
              pconn->username,
              player_name(pplayer),
              is_barbarian(pplayer)
              ? _("Barbarian")
              : is_ai(pplayer)
              ? _("AI")
              : _("Human"),
              pplayer->is_alive
              ? _("Alive")
              : _("Dead"));
  } else {
    cmd_reply(CMD_TAKE, caller, C_FAIL,
              _("%s failed to attach to any player."),
              pconn->username);
  }

  end:;
  /* free our args */
  for (i = 0; i < ntokens; i++) {
    free(arg[i]);
  }
  return res;
}

/**********************************************************************//**
  Detach from a player. if that player wasn't /created and you were
  controlling the player, remove it (and then detach any observers as well).

  If called for a global observer connection (where pconn->playing is NULL)
  then it will correctly detach from observing mode.
**************************************************************************/
static bool detach_command(struct connection *caller, char *str, bool check)
{
  int i = 0, ntokens = 0;
  char buf[MAX_LEN_CONSOLE_LINE], *arg[1];
  enum m_pre_result match_result;
  struct connection *pconn = NULL;
  struct player *pplayer = NULL;
  bool res = FALSE;

  sz_strlcpy(buf, str);
  ntokens = get_tokens(buf, arg, 1, TOKEN_DELIMITERS);

  if (!caller && ntokens == 0) {
    cmd_reply(CMD_DETACH, caller, C_SYNTAX, _("Usage:\n%s"),
              command_synopsis(command_by_number(CMD_DETACH)));
    goto end;
  }

  /* match the connection if the argument was given */
  if (ntokens == 1 
      && !(pconn = conn_by_user_prefix(arg[0], &match_result))) {
    cmd_reply_no_such_conn(CMD_DETACH, caller, arg[0], match_result);
    goto end;
  }

  /* if no argument is given, the caller wants to detach himself */
  if (!pconn) {
    pconn = caller;
  }

  /* if pconn and caller are not the same, only continue 
   * if we're console, or we have ALLOW_HACK */
  if (pconn != caller  && caller && caller->access_level != ALLOW_HACK) {
    cmd_reply(CMD_DETACH, caller, C_FAIL, 
                _("You can not detach other users."));
    goto end;
  }

  pplayer = pconn->playing;

  /* must have someone to detach from... */
  if (!pplayer && !pconn->observer) {
    cmd_reply(CMD_DETACH, caller, C_FAIL, 
              _("%s is not attached to any player."), pconn->username);
    goto end;
  }

  res = TRUE;
  if (check) {
    goto end;
  }

  if (pplayer) {
    cmd_reply(CMD_DETACH, caller, C_OK, _("%s detaching from %s"),
              pconn->username, player_name(pplayer));
  } else {
    cmd_reply(CMD_DETACH, caller, C_OK, _("%s no longer observing."),
              pconn->username);
  }

  /* Actually do the detaching. */
  connection_detach(pconn, TRUE);

  /* The user explicitly wanted to detach, so if a player is marked for him,
   * reset its username. */
  players_iterate(aplayer) {
    if (0 == strncmp(aplayer->username, pconn->username, MAX_LEN_NAME)) {
      sz_strlcpy(aplayer->username, _(ANON_USER_NAME));
      aplayer->unassigned_user = TRUE;
      send_player_info_c(aplayer, NULL);
    }
  } players_iterate_end;

  check_for_full_turn_done();

  end:
  fc_assert_ret_val(ntokens <= 1, FALSE);

  /* free our args */
  for (i = 0; i < ntokens; i++) {
    free(arg[i]);
  }
  return res;
}

/**********************************************************************//**
  Loads a file, complete with access checks and error messages sent back
  to the caller on failure.

  * caller is the connection requesting the load, or NULL for a
    command-line load.  Error messages are sent back to the caller and
    an access check is done to make sure they are allowed to load.

  * filename is simply the name of the file to be loaded.  This may be
    approximate; the function will look for appropriate suffixes and will
    check in the various directories to see if the file is found.

  * if check is set then only a test run is done and no actual loading
    is attempted.

  * The return value is true if the load succeeds, or would succeed;
    false if there's an error in the file or file name.  Some errors
    in loading however could be unrecoverable (if the save game is
    legitimate but has inconsistencies) and would lead to a broken server
    afterwards.
**************************************************************************/
bool load_command(struct connection *caller, const char *filename, bool check,
                  bool cmdline_load)
{
  struct timer *loadtimer, *uloadtimer;
  struct section_file *file;
  char arg[MAX_LEN_PATH];
  struct conn_list *global_observers;

  if (!filename || filename[0] == '\0') {
    cmd_reply(CMD_LOAD, caller, C_FAIL, _("Usage:\n%s"),
              command_synopsis(command_by_number(CMD_LOAD)));
    return FALSE;
  }
  if (S_S_INITIAL != server_state()) {
    cmd_reply(CMD_LOAD, caller, C_FAIL,
              _("Cannot load a game while another is running."));
    dlsend_packet_game_load(game.est_connections, TRUE, filename);
    return FALSE;
  }
  if (!is_safe_filename(filename) && is_restricted(caller)) {
    cmd_reply(CMD_LOAD, caller, C_FAIL,
              _("Name \"%s\" disallowed for security reasons."),
              filename);
    return FALSE;
  }

  {
    char websave[300];
    struct strvec *webdirs = strvec_new();

    if (caller != NULL) {
      fc_snprintf(websave, sizeof(websave), "%s/%s", srvarg.saves_pathname, caller->username);
      strvec_append(webdirs, websave);
    }
    fc_snprintf(websave, sizeof(websave), "%s/pbem", srvarg.saves_pathname);
    strvec_append(webdirs, websave);

    /* it is a normal savegame or maybe a scenario */
    char testfile[MAX_LEN_PATH];
    const struct strvec *pathes[] = {
      get_save_dirs(), get_scenario_dirs(), webdirs, NULL
    };
    const char *exts[] = {
      "sav", "gz", "bz2", "xz", "sav.gz", "sav.bz2", "sav.xz", NULL
    };
    const char **ext, *found = NULL;
    const struct strvec **path;

    if (cmdline_load) {
      /* Allow plain names being loaded with '--file' option, but not otherwise
       * (no loading of arbitrary files by unauthorized users)
       * Iterate through ALL paths to check for file with plain name before
       * looking any path with an extension, i.e., prefer plain name file
       * in later directory over file with extension in name in earlier
       * directory. */
      for (path = pathes; !found && *path; path++) {
        found = fileinfoname(*path, filename);
        if (found != NULL) {
          sz_strlcpy(arg, found);
        }
      }
    }

    for (path = pathes; !found && *path; path++) {
      for (ext = exts; !found && *ext; ext++) {
        fc_snprintf(testfile, sizeof(testfile), "%s.%s", filename, *ext);
        found = fileinfoname(*path, testfile);
        if (found != NULL) {
          sz_strlcpy(arg, found);
        }
      }
    }

    if (is_restricted(caller) && !found) {
      cmd_reply(CMD_LOAD, caller, C_FAIL, _("Cannot find savegame or "
                "scenario with the name \"%s\"."), filename);
      return FALSE;
    }

    if (!found) {
      sz_strlcpy(arg, filename);
    }
  }

  /* attempt to parse the file */

  if (!(file = secfile_load(arg, FALSE))) {
    log_error("Error loading savefile '%s': %s", arg, secfile_error());
    cmd_reply(CMD_LOAD, caller, C_FAIL, _("Could not load savefile: %s"),
              arg);
    dlsend_packet_game_load(game.est_connections, TRUE, arg);
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  /* Detach current players, before we blow them away. */
  global_observers = conn_list_new();
  conn_list_iterate(game.est_connections, pconn) {
    if (pconn->playing != NULL) {
      connection_detach(pconn, TRUE);
    } else if (pconn->observer) {
      conn_list_append(global_observers, pconn);
      connection_detach(pconn, TRUE);
    }
  } conn_list_iterate_end;

  player_info_freeze();

  /* Now free all game data. */
  server_game_free();

  /* Keep old ruleset value. Scenario file will either use the old value,
   * or to initialize new value itself. */
  server_game_init(TRUE);

  loadtimer = timer_new(TIMER_CPU, TIMER_ACTIVE);
  timer_start(loadtimer);
  uloadtimer = timer_new(TIMER_USER, TIMER_ACTIVE);
  timer_start(uloadtimer);

  sz_strlcpy(srvarg.load_filename, arg);

  savegame_load(file);
  secfile_check_unused(file);
  secfile_destroy(file);

  log_verbose("Load time: %g seconds (%g apparent)",
              timer_read_seconds(loadtimer), timer_read_seconds(uloadtimer));
  timer_destroy(loadtimer);
  timer_destroy(uloadtimer);

  sanity_check();

  log_verbose("load_command() does send_rulesets()");
  conn_list_compression_freeze(game.est_connections);
  send_rulesets(game.est_connections);
  send_server_settings(game.est_connections);
  send_scenario_info(game.est_connections);
  send_scenario_description(game.est_connections);
  send_game_info(game.est_connections);
  conn_list_compression_thaw(game.est_connections);

  /* Send information about the new players. */
  player_info_thaw();
  send_player_diplstate_c(NULL, NULL);

  /* Everything seemed to load ok; spread the good news. */
  dlsend_packet_game_load(game.est_connections, TRUE, srvarg.load_filename);

  /* Attach connections to players. Currently, this applies only 
   * to connections that have the same username as a player. */
  conn_list_iterate(game.est_connections, pconn) {
    players_iterate(pplayer) {
      if (strcmp(pconn->username, pplayer->username) == 0) {
        connection_attach(pconn, pplayer, FALSE);
        break;
      }
    } players_iterate_end;
  } conn_list_iterate_end;

  /* Reattach global observers. */
  conn_list_iterate(global_observers, pconn) {
    if (NULL == pconn->playing) {
      /* May have been assigned to a player before. */
      connection_attach(pconn, NULL, TRUE);
    }
  } conn_list_iterate_end;
  conn_list_destroy(global_observers);

  (void) aifill(game.info.aifill);

  achievements_iterate(pach) {
    players_iterate(pplayer) {
      struct packet_achievement_info pack;

      pack.id = achievement_index(pach);
      pack.gained = achievement_player_has(pach, pplayer);
      pack.first = (pach->first == pplayer);

      lsend_packet_achievement_info(pplayer->connections, &pack);
    } players_iterate_end;
  } achievements_iterate_end;

  cmd_reply(CMD_DEFAULT, caller, C_OK, _("Load complete"));

  return TRUE;
}

/**********************************************************************//**
  Load rulesets from a given ruleset directory.

  Security: There are some rudimentary checks in load_rulesets() to see
  if this directory really is a viable ruleset directory. For public
  servers, we check against directory redirection (is_safe_filename) and
  other bad stuff in the directory name, and will only use directories
  inside the data directories.
**************************************************************************/
static bool set_rulesetdir(struct connection *caller, char *str, bool check,
                           int read_recursion)
{
  char filename[512];
  const char *pfilename;

  if (NULL == str || '\0' == str[0]) {
    cmd_reply(CMD_RULESETDIR, caller, C_SYNTAX,
              _("You must provide a ruleset name. Use \"/show ruleset\" to "
                "see what is the current ruleset."));
    return FALSE;
  }
  if (game_was_started() || !map_is_empty()) {
    cmd_reply(CMD_RULESETDIR, caller, C_FAIL,
              _("This setting can't be modified after the game has started."));
    return FALSE;
  }

  if (strcmp(str, game.server.rulesetdir) == 0) {
    cmd_reply(CMD_RULESETDIR, caller, C_COMMENT,
              _("Ruleset directory is already \"%s\""), str);
    return FALSE;
  }

  if (is_restricted(caller)
      && (!is_safe_filename(str) || strchr(str, '.'))) {
    cmd_reply(CMD_RULESETDIR, caller, C_SYNTAX,
              _("Name \"%s\" disallowed for security reasons."),
              str);
    return FALSE;
  }

  fc_snprintf(filename, sizeof(filename), "%s", str);
  pfilename = fileinfoname(get_data_dirs(), filename);
  if (!pfilename) {
    cmd_reply(CMD_RULESETDIR, caller, C_SYNTAX,
             _("Ruleset directory \"%s\" not found"), str);
    return FALSE;
  }

  if (!check) {
    bool success = TRUE;
    char old[512];

    sz_strlcpy(old, game.server.rulesetdir);
    log_verbose("set_rulesetdir() does load_rulesets() with \"%s\"", str);
    sz_strlcpy(game.server.rulesetdir, str);

    /* load the ruleset (and game settings defined in the ruleset) */
    player_info_freeze();
    if (!load_rulesets(old, FALSE, NULL, TRUE, FALSE)) {
      success = FALSE;

      /* While loading of the requested ruleset failed, we might
       * have changed ruleset from third one to default. Handle
       * rest of the ruleset changing accordingly. */
    }

    if (game.est_connections) {
      /* Now that the rulesets are loaded we immediately send updates to any
       * connected clients. */
      send_rulesets(game.est_connections);
    }
    /* show ruleset summary and list changed values */
    show_ruleset_info(caller, CMD_RULESETDIR, check, read_recursion);
    player_info_thaw();

    if (success) {
      cmd_reply(CMD_RULESETDIR, caller, C_OK, 
                _("Ruleset directory set to \"%s\""), str);
    } else {
      cmd_reply(CMD_RULESETDIR, caller, C_SYNTAX,
                _("Failed loading rulesets from directory \"%s\", using \"%s\""),
                str, game.server.rulesetdir);
    }

    return success;
  }

  return TRUE;
}

/**********************************************************************//**
  /ignore command handler.
**************************************************************************/
static bool ignore_command(struct connection *caller, char *str, bool check)
{
  char buf[128];
  struct conn_pattern *ppattern;

  if (NULL == caller) {
    cmd_reply(CMD_IGNORE, caller, C_FAIL,
              _("That would be rather silly, since you are not a player."));
    return FALSE;
  }

  ppattern = conn_pattern_from_string(str, CPT_USER, buf, sizeof(buf));
  if (NULL == ppattern) {
    cmd_reply(CMD_IGNORE, caller, C_SYNTAX,
              _("%s. Try /help ignore"), buf);
    return FALSE;
  }

  if (check) {
    conn_pattern_destroy(ppattern);
    return TRUE;
  }

  conn_pattern_to_string(ppattern, buf, sizeof(buf));
  conn_pattern_list_append(caller->server.ignore_list, ppattern);
  cmd_reply(CMD_IGNORE, caller, C_COMMENT,
            _("Added pattern %s as entry %d to your ignore list."),
            buf, conn_pattern_list_size(caller->server.ignore_list));

  return TRUE;
}

/**********************************************************************//**
  /unignore command handler.
**************************************************************************/
static bool unignore_command(struct connection *caller,
                             char *str, bool check)
{
  char buf[128], *c;
  int first, last, n;

  if (!caller) {
    cmd_reply(CMD_IGNORE, caller, C_FAIL,
              _("That would be rather silly, since you are not a player."));
    return FALSE;
  }

  sz_strlcpy(buf, str);
  remove_leading_trailing_spaces(buf);

  n = conn_pattern_list_size(caller->server.ignore_list);
  if (n == 0) {
    cmd_reply(CMD_UNIGNORE, caller, C_FAIL, _("Your ignore list is empty."));
    return FALSE;
  }

  /* Parse the range. */
  if ('\0' == buf[0]) {
    cmd_reply(CMD_UNIGNORE, caller, C_SYNTAX,
              _("Missing range. Try /help unignore."));
    return FALSE;
  } else if ((c = strchr(buf, '-'))) {
    *c++ = '\0';
    if ('\0' == buf[0]) {
      first = 1;
    } else if (!str_to_int(buf, &first)) {
      *--c = '-';
      cmd_reply(CMD_UNIGNORE, caller, C_SYNTAX,
                _("\"%s\" is not a valid range. Try /help unignore."), buf);
      return FALSE;
    }
    if ('\0' == *c) {
      last = n;
    } else if (!str_to_int(c, &last)) {
      *--c = '-';
      cmd_reply(CMD_UNIGNORE, caller, C_SYNTAX,
                _("\"%s\" is not a valid range. Try /help unignore."), buf);
      return FALSE;
    }
  } else {
    if (!str_to_int(buf, &first)) {
      cmd_reply(CMD_UNIGNORE, caller, C_SYNTAX,
                _("\"%s\" is not a valid range. Try /help unignore."), buf);
      return FALSE;
    }
    last = first;
  }

  if (!(1 <= first && first <= last && last <= n)) {
    if (first == last) {
      cmd_reply(CMD_UNIGNORE, caller, C_FAIL,
                _("Invalid entry number: %d."), first);
    } else {
      cmd_reply(CMD_UNIGNORE, caller, C_FAIL,
                _("Invalid range: %d to %d."), first, last);
    }
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  n = 1;
  conn_pattern_list_iterate(caller->server.ignore_list, ppattern) {
    if (first <= n) {
      conn_pattern_to_string(ppattern, buf, sizeof(buf));
      cmd_reply(CMD_UNIGNORE, caller, C_COMMENT,
                _("Removed pattern %s (entry %d) from your ignore list."),
                buf, n);
      conn_pattern_list_remove(caller->server.ignore_list, ppattern);
    }
    n++;
    if (n > last) {
      break;
    }
  } conn_pattern_list_iterate_end;

  return TRUE;
}

/**********************************************************************//**
  /playercolor command handler.
**************************************************************************/
static bool playercolor_command(struct connection *caller,
                                char *str, bool check)
{
  enum m_pre_result match_result;
  struct player *pplayer;
  struct rgbcolor *prgbcolor = NULL;
  int ntokens = 0;
  char *token[2];
  bool ret = TRUE;

  ntokens = get_tokens(str, token, 2, TOKEN_DELIMITERS);

  if (ntokens != 2) {
    cmd_reply(CMD_PLAYERCOLOR, caller, C_SYNTAX,
              _("Two arguments needed. See '/help playercolor'."));
    ret = FALSE;
    goto cleanup;
  }

  pplayer = player_by_name_prefix(token[0], &match_result);

  if (!pplayer) {
    cmd_reply_no_such_player(CMD_PLAYERCOLOR, caller, token[0], match_result);
    ret = FALSE;
    goto cleanup;
  }

  {
    const char *reason;
    if (!player_color_changeable(pplayer, &reason)) {
      cmd_reply(CMD_PLAYERCOLOR, caller, C_FAIL, "%s", reason);
      ret = FALSE;
      goto cleanup;
    }
  }

  if (0 == fc_strcasecmp(token[1], "reset")) {
    if (!game_was_started()) {
      prgbcolor = NULL;
    } else {
      cmd_reply(CMD_PLAYERCOLOR, caller, C_FAIL,
                _("Can only unset player color before game starts."));
      ret = FALSE;
      goto cleanup;
    }
  } else if (!rgbcolor_from_hex(&prgbcolor, token[1])) {
    cmd_reply(CMD_PLAYERCOLOR, caller, C_SYNTAX,
              _("Invalid player color definition. See '/help playercolor'."));
    ret = FALSE;
    goto cleanup;
  }

  if (prgbcolor != NULL) {
    players_iterate(pother) {
      if (pother != pplayer && pother->rgb != NULL
          && rgbcolors_are_equal(pother->rgb, prgbcolor)) {
        cmd_reply(CMD_PLAYERCOLOR, caller, C_WARNING,
                  /* TRANS: "... [c0ffee] for Caesar ... to Hammurabi." */
                  _("Warning: new color [%s] for %s is identical to %s."),
                  player_color_ftstr(pother), player_name(pplayer),
                  player_name(pother));
      }
    } players_iterate_end;
  }

  if (check) {
    goto cleanup;
  }

  server_player_set_color(pplayer, prgbcolor);
  cmd_reply(CMD_PLAYERCOLOR, caller, C_OK,
            _("Color of player %s set to [%s]."), player_name(pplayer),
            player_color_ftstr(pplayer));

 cleanup:

  rgbcolor_destroy(prgbcolor);
  free_tokens(token, ntokens);

  return ret;
}

/**********************************************************************//**
  Handle quit command
**************************************************************************/
static bool quit_game(struct connection *caller, bool check)
{
  if (!check) {
    cmd_reply(CMD_QUIT, caller, C_OK, _("Goodbye."));
    server_quit();
  }
  return TRUE;
}

/**********************************************************************//**
  Main entry point for "command input".
**************************************************************************/
bool handle_stdin_input(struct connection *caller, char *str)
{
  return handle_stdin_input_real(caller, str, FALSE, 0);
}

/**********************************************************************//**
  Handle "command input", which could really come from stdin on console,
  or from client chat command, or read from file with -r, etc.
  caller == NULL means console, str is the input, which may optionally
  start with SERVER_COMMAND_PREFIX character.

  If check is TRUE, then do nothing, just check syntax.
**************************************************************************/
static bool handle_stdin_input_real(struct connection *caller, char *str,
                                    bool check, int read_recursion)
{
  char full_command[MAX_LEN_CONSOLE_LINE];
  char command[MAX_LEN_CONSOLE_LINE], arg[MAX_LEN_CONSOLE_LINE];
  char *cptr_s, *cptr_d;
  enum command_id cmd;
  enum cmdlevel level;

  /* Remove leading and trailing spaces, and server command prefix. */
  cptr_s = str = skip_leading_spaces(str);
  if ('\0' == *cptr_s || '#' == *cptr_s) {
    /* This appear to be a comment or blank line. */
    return FALSE;
  }

  if (SERVER_COMMAND_PREFIX == *cptr_s) {
    /* Commands may be prefixed with SERVER_COMMAND_PREFIX, even when
     * given on the server command line. */
    cptr_s++;
    remove_leading_spaces(cptr_s);
    if ('\0' == *cptr_s) {
      /* This appear to be a blank line. */
      return FALSE;
    }
  }
  remove_trailing_spaces(cptr_s);

  /* notify to the server console */
  if (!check && caller) {
    con_write(C_COMMENT, "%s: '%s'", caller->username, str);
  }

  /* if the caller may not use any commands at all, don't waste any time */
  if (may_use_nothing(caller)) {
    cmd_reply(CMD_HELP, caller, C_FAIL,
	_("Sorry, you are not allowed to use server commands."));
     return FALSE;
  }

  /* copy the full command, in case we need it for voting purposes. */
  sz_strlcpy(full_command, cptr_s);

  /*
   * cptr_s points now to the beginning of the real command. It has
   * skipped leading whitespace, the SERVER_COMMAND_PREFIX and any
   * other non-alphanumeric characters.
   */
  for (cptr_d = command; *cptr_s != '\0' && fc_isalnum(*cptr_s)
       && cptr_d < command + sizeof(command) - 1; cptr_s++, cptr_d++) {
    *cptr_d = *cptr_s;
  }
  *cptr_d = '\0';

  /* cptr_s now contains the arguments. */
  sz_strlcpy(arg, skip_leading_spaces(cptr_s));

  cmd = command_named(command, FALSE);
  if (cmd == CMD_AMBIGUOUS) {
    cmd = command_named(command, TRUE);
    cmd_reply(cmd, caller, C_SYNTAX,
	_("Warning: '%s' interpreted as '%s', but it is ambiguous."
	  "  Try '%shelp'."),
	command, command_name_by_number(cmd), caller?"/":"");
  } else if (cmd == CMD_UNRECOGNIZED) {
    cmd_reply(cmd, caller, C_SYNTAX, _("Unknown command '%s%s'. "
                                       " Try '%shelp'."),
              caller ? "/" : "", command, caller ? "/" : "");
    return FALSE;
  }

  level = command_level(command_by_number(cmd));

  /* Special savegame handling for Freeciv-web. */
  if (strncmp(game.server.meta_info.type, "pbem", 4) == 0 
      && cmd == CMD_SAVE && caller) {
    char pbemfile[200];
    fc_snprintf(pbemfile, sizeof(pbemfile), "%s/pbem/pbem-%s-%u", srvarg.saves_pathname, caller->username, (unsigned)time(NULL));
    sz_strlcpy(arg, pbemfile);
  } else if (cmd == CMD_SAVE && caller) {
    char savefile[300];
    char buffer[30];
    time_t timer;
    struct tm* tm_info;

    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 30, "%Y-%m-%d-%H_%M", tm_info);
    fc_snprintf(savefile, sizeof(savefile), "%s/%s/%s_T%u_%s", srvarg.saves_pathname, 
                caller->username, caller->username, game.info.turn, buffer);
    sz_strlcpy(arg, savefile);

    /* create sub-directory from players username in saves directory. */
    char savedir[300];
    fc_snprintf(savedir, sizeof(savedir), "%s/%s", srvarg.saves_pathname, caller->username);
    make_dir(savedir);
  }

  if (conn_can_vote(caller, NULL) && level == ALLOW_CTRL
      && conn_get_access(caller) == ALLOW_BASIC && !check
      && !vote_would_pass_immediately(caller, cmd)) {
    struct vote *vote;
    bool caller_had_vote = (NULL != get_vote_by_caller(caller));

    /* Check if the vote command would succeed. If we already have a vote
     * going, cancel it in favour of the new vote command. You can only
     * have one vote at a time. This is done by vote_new(). */
    if (handle_stdin_input_real(caller, full_command, TRUE,
                                read_recursion + 1)
        && (vote = vote_new(caller, arg, cmd))) {
      char votedesc[MAX_LEN_CONSOLE_LINE];
      const struct player *teamplr;
      const char *what;
      struct ft_color color;

      if (caller_had_vote) {
        cmd_reply(CMD_VOTE, caller, C_COMMENT,
                  /* TRANS: "vote" as a process */
                  _("Your new vote canceled your previous vote."));
      }

      describe_vote(vote, votedesc, sizeof(votedesc));

      if (vote_is_team_only(vote)) {
        /* TRANS: "vote" as a process */
        what = _("New teamvote");
        teamplr = conn_get_player(caller);
        color = ftc_vote_team;
      } else {
        /* TRANS: "vote" as a process */
        what = _("New vote");
        teamplr = NULL;
        color = ftc_vote_public;
      }
      notify_team(teamplr, NULL, E_VOTE_NEW, color,
                  /* TRANS: "[New vote|New teamvote] (number 3)
                   * by fred: proposed change" */
                  _("%s (number %d) by %s: %s"), what,
                  vote->vote_no, caller->username, votedesc);

      /* Vote on your own suggestion. */
      connection_vote(caller, vote, VOTE_YES);
      return TRUE;

    } else {
      cmd_reply(CMD_VOTE, caller, C_FAIL,
                /* TRANS: "vote" as a process */
                _("Your new vote (\"%s\") was not "
                  "legal or was not recognized."), full_command);
      return FALSE;
    }
  }

  if (caller
      && !((check || vote_would_pass_immediately(caller, cmd))
           && conn_get_access(caller) >= ALLOW_BASIC
           && level == ALLOW_CTRL)
      && conn_get_access(caller) < level) {
    cmd_reply(cmd, caller, C_FAIL,
	      _("You are not allowed to use this command."));
    return FALSE;
  }

  if (!check) {
    struct conn_list *echo_list = NULL;
    bool echo_list_allocated = FALSE;

    switch (command_echo(command_by_number(cmd))) {
    case CMD_ECHO_NONE:
      break;
    case CMD_ECHO_ADMINS:
      conn_list_iterate(game.est_connections, pconn) {
        if (ALLOW_ADMIN <= conn_get_access(pconn)) {
          if (NULL == echo_list) {
            echo_list = conn_list_new();
            echo_list_allocated = TRUE;
          }
          conn_list_append(echo_list, pconn);
        }
      } conn_list_iterate_end;
      break;
    case CMD_ECHO_ALL:
      echo_list = game.est_connections;
      break;
    }

    if (NULL != echo_list) {
      if (caller) {
        notify_conn(echo_list, NULL, E_SETTING, ftc_any,
                    "%s: '%s %s'", caller->username, command, arg);
      } else {
        notify_conn(echo_list, NULL, E_SETTING, ftc_server_prompt,
                    "%s: '%s %s'", _("(server prompt)"), command, arg);
      }
      if (echo_list_allocated) {
        conn_list_destroy(echo_list);
      }
    }
  }

  switch(cmd) {
  case CMD_REMOVE:
    return remove_player_command(caller, arg, check);
  case CMD_SAVE:
    return save_command(caller, arg, check);
  case CMD_SCENSAVE:
    return scensave_command(caller, arg, check);
  case CMD_LOAD:
    return load_command(caller, arg, check, FALSE);
  case CMD_METAPATCHES:
    return metapatches_command(caller, arg, check);
  case CMD_METAMESSAGE:
    return metamessage_command(caller, arg, check);
  case CMD_METACONN:
    return metaconnection_command(caller, arg, check);
  case CMD_METASERVER:
    return metaserver_command(caller, arg, check);
  case CMD_HELP:
    return show_help(caller, arg);
  case CMD_SRVID:
    return show_serverid(caller, arg);
  case CMD_LIST:
    return show_list(caller, arg);
  case CMD_AITOGGLE:
    return toggle_ai_command(caller, arg, check);
  case CMD_TAKE:
    return take_command(caller, arg, check);
  case CMD_OBSERVE:
    return observe_command(caller, arg, check);
  case CMD_DETACH:
    return detach_command(caller, arg, check);
  case CMD_CREATE:
    return create_command(caller, arg, check);
  case CMD_AWAY:
    return away_command(caller, check);
  case CMD_HANDICAPPED:
  case CMD_NOVICE:
  case CMD_EASY:
  case CMD_NORMAL:
  case CMD_HARD:
  case CMD_CHEATING:
#ifdef FREECIV_DEBUG
  case CMD_EXPERIMENTAL:
#endif
    return set_ai_level_named(caller, arg, command_name_by_number(cmd), check);
  case CMD_QUIT:
    return quit_game(caller, check);
  case CMD_CUT:
    return cut_client_connection(caller, arg, check);
  case CMD_SHOW:
    return show_command(caller, arg, check);
  case CMD_EXPLAIN:
    return explain_option(caller, arg, check);
  case CMD_DEBUG:
    return debug_command(caller, arg, check);
  case CMD_SET:
    return set_command(caller, arg, check);
  case CMD_TEAM:
    return team_command(caller, arg, check);
  case CMD_RULESETDIR:
    return set_rulesetdir(caller, arg, check, read_recursion);
  case CMD_WALL:
    return wall(arg, check);
  case CMD_CONNECTMSG:
    return connectmsg_command(caller, arg, check);
  case CMD_VOTE:
    return vote_command(caller, arg, check);
  case CMD_CANCELVOTE:
    return cancelvote_command(caller, arg, check);
  case CMD_READ_SCRIPT:
    return read_command(caller, arg, check, read_recursion);
  case CMD_WRITE_SCRIPT:
    return write_command(caller, arg, check);
  case CMD_RESET:
    return reset_command(caller, arg, check, read_recursion);
  case CMD_DEFAULT:
    return default_command(caller, arg, check);
  case CMD_LUA:
    return lua_command(caller, arg, check);
  case CMD_KICK:
    return kick_command(caller, arg, check);
  case CMD_DELEGATE:
    return delegate_command(caller, arg, check);
  case CMD_AICMD:
    return aicmd_command(caller, arg, check);
  case CMD_FCDB:
    return fcdb_command(caller, arg, check);
  case CMD_MAPIMG:
    return mapimg_command(caller, arg, check);
  case CMD_RFCSTYLE:	/* see console.h for an explanation */
    if (!check) {
      con_set_style(!con_get_style());
    }
    return TRUE;
  case CMD_CMDLEVEL:
    return cmdlevel_command(caller, arg, check);
  case CMD_FIRSTLEVEL:
    return firstlevel_command(caller, check);
  case CMD_TIMEOUT:
    return timeout_command(caller, arg, check);
  case CMD_START_GAME:
    return start_command(caller, check, FALSE);
  case CMD_END_GAME:
    return end_command(caller, arg, check);
  case CMD_SURRENDER:
    return surrender_command(caller, arg, check);
  case CMD_IGNORE:
    return ignore_command(caller, arg, check);
  case CMD_UNIGNORE:
    return unignore_command(caller, arg, check);
  case CMD_PLAYERCOLOR:
    return playercolor_command(caller, arg, check);
  case CMD_NUM:
  case CMD_UNRECOGNIZED:
  case CMD_AMBIGUOUS:
    break;
  }
  /* should NEVER happen! */
  log_error("Unknown command variant: %d.", cmd);
  return FALSE;
}

/**********************************************************************//**
  End the game immediately in a draw.
**************************************************************************/
static bool end_command(struct connection *caller, char *str, bool check)
{
  if (S_S_RUNNING == server_state()) {
    if (check) {
      return TRUE;
    }
    notify_conn(game.est_connections, NULL, E_GAME_END, ftc_server,
                _("Game is over."));
    set_server_state(S_S_OVER);
    force_end_of_sniff = TRUE;
    cmd_reply(CMD_END_GAME, caller, C_OK,
              _("Ending the game. The server will restart once all clients "
                "have disconnected."));
    return TRUE;
  } else {
    cmd_reply(CMD_END_GAME, caller, C_FAIL, 
              _("Cannot end the game: no game running."));
    return FALSE;
  }
}

/**********************************************************************//**
  Concede the game.  You still continue playing until all but one player
  or team remains un-conceded.
**************************************************************************/
static bool surrender_command(struct connection *caller, char *str, bool check)
{
  struct player *pplayer;

  if (caller == NULL || !conn_controls_player(caller)) {
    cmd_reply(CMD_SURRENDER, caller, C_FAIL,
              _("You are not allowed to use this command."));
    return FALSE;
  }

  if (S_S_RUNNING != server_state()) {
    cmd_reply(CMD_SURRENDER, caller, C_FAIL, _("You cannot surrender now."));
    return FALSE;
  }

  pplayer = conn_get_player(caller);
  if (player_status_check(pplayer, PSTATUS_SURRENDER)) {
    cmd_reply(CMD_SURRENDER, caller, C_FAIL,
              _("You have already conceded the game."));
    return FALSE;
  }

  if (check) {
    return TRUE;
  }

  notify_conn(game.est_connections, NULL, E_GAME_END, ftc_server,
              _("%s has conceded the game and can no longer win."),
              player_name(pplayer));
  player_status_add(pplayer, PSTATUS_SURRENDER);
  return TRUE;
}

/* Define the possible arguments to the reset command */
#define SPECENUM_NAME reset_args
#define SPECENUM_VALUE0     RESET_GAME
#define SPECENUM_VALUE0NAME "game"
#define SPECENUM_VALUE1     RESET_RULESET
#define SPECENUM_VALUE1NAME "ruleset"
#define SPECENUM_VALUE2     RESET_SCRIPT
#define SPECENUM_VALUE2NAME "script"
#define SPECENUM_VALUE3     RESET_DEFAULT
#define SPECENUM_VALUE3NAME "default"
#include "specenum_gen.h"

/**********************************************************************//**
  Returns possible parameters for the reset command.
**************************************************************************/
static const char *reset_accessor(int i)
{
  i = CLIP(0, i, reset_args_max());
  return reset_args_name((enum reset_args) i);
}

/**********************************************************************//**
  Reload the game settings from the ruleset and reload the init script if
  one was used.
**************************************************************************/
static bool reset_command(struct connection *caller, char *arg, bool check,
                          int read_recursion)
{
  enum m_pre_result result;
  int ind;

  /* match the argument */
  result = match_prefix(reset_accessor, reset_args_max() + 1, 0,
                        fc_strncasecmp, NULL, arg, &ind);

  switch (result) {
  case M_PRE_EXACT:
  case M_PRE_ONLY:
    /* we have a match */
    break;
  case M_PRE_AMBIGUOUS:
  case M_PRE_EMPTY:
    /* use 'ruleset' [1] if the game was not started; else use 'game' [2] */
    if (S_S_INITIAL == server_state() && game.info.is_new_game) {
      cmd_reply(CMD_RESET, caller, C_WARNING,
                _("Guessing argument 'ruleset'."));
      ind = RESET_RULESET;
    } else {
      cmd_reply(CMD_RESET, caller, C_WARNING,
                _("Guessing argument 'game'."));
      ind = RESET_GAME;
    }
    break;
  case M_PRE_LONG:
  case M_PRE_FAIL:
  case M_PRE_LAST:
    cmd_reply(CMD_RESET, caller, C_FAIL,
              _("The valid arguments are: 'game', 'ruleset', 'script' "
                "or 'default'."));
    return FALSE;
    break;
  }

  if (check) {
    return TRUE;
  }

  switch (ind) {
  case RESET_GAME:
    if (!game.info.is_new_game) {
      if (settings_game_reset()) {
        cmd_reply(CMD_RESET, caller, C_OK,
                  _("Reset all settings to the values at the game start."));
      } else {
        cmd_reply(CMD_RESET, caller, C_FAIL,
                  _("No saved settings from the game start available."));
        return FALSE;
      }
    } else {
      cmd_reply(CMD_RESET, caller, C_FAIL, _("No game started..."));
      return FALSE;
    }
    break;

  case RESET_RULESET:
    /* Restore game settings save in game.ruleset. */
    if (reload_rulesets_settings()) {
      cmd_reply(CMD_RESET, caller, C_OK,
                _("Reset all settings to ruleset values."));
    } else {
      cmd_reply(CMD_RESET, caller, C_FAIL,
                _("Failed to reset settings to ruleset values."));
    }
    break;

  case RESET_SCRIPT:
    cmd_reply(CMD_RESET, caller, C_OK,
              _("Reset all settings and rereading the server start "
                "script."));
    settings_reset();
    /* load initial script */
    if (NULL != srvarg.script_filename
        && !read_init_script_real(NULL, srvarg.script_filename, TRUE, FALSE,
                                  read_recursion + 1)) {
      if (NULL != caller) {
        cmd_reply(CMD_RESET, caller, C_FAIL,
                  _("Could not read script file '%s'."),
                  srvarg.script_filename);
      }
      return FALSE;
    }
    break;

  case RESET_DEFAULT:
    cmd_reply(CMD_RESET, caller, C_OK,
              _("Reset all settings to default values."));
    settings_reset();
    break;
  }

  send_server_settings(game.est_connections);
  cmd_reply(CMD_RESET, caller, C_OK, _("Settings re-initialized."));

  /* show ruleset summary and list changed values */
  show_ruleset_info(caller, CMD_RESET, check, read_recursion);

  return TRUE;
}

/**********************************************************************//**
  Set a setting to its default value
**************************************************************************/
static bool default_command(struct connection *caller, char *arg, bool check)
{
  struct setting *pset;
  char reject_msg[256] = "";

  pset = validate_setting_arg(CMD_DEFAULT, caller, arg);

  if (!pset) {
    /* Reason already reported. */
    return FALSE;
  }

  if (!setting_is_changeable(pset, caller, reject_msg, sizeof(reject_msg))) {
    cmd_reply(CMD_DEFAULT, caller, C_FAIL, "%s", reject_msg);

    return FALSE;
  }

  if (!check) {
    setting_set_to_default(pset);
    cmd_reply(CMD_DEFAULT, caller, C_OK,
              _("Option '%s' reset to default value, and will track any "
                "default changes."), arg);
  }

  return TRUE;
}

/* Define the possible arguments to the delegation command */
#define SPECENUM_NAME lua_args
#define SPECENUM_VALUE0     LUA_CMD
#define SPECENUM_VALUE0NAME "cmd"
#define SPECENUM_VALUE1     LUA_FILE
#define SPECENUM_VALUE1NAME "file"
#define SPECENUM_VALUE2     LUA_UNSAFE_CMD
#define SPECENUM_VALUE2NAME "unsafe-cmd"
#define SPECENUM_VALUE3     LUA_UNSAFE_FILE
#define SPECENUM_VALUE3NAME "unsafe-file"
#include "specenum_gen.h"

/**********************************************************************//**
  Returns possible parameters for the reset command.
**************************************************************************/
static const char *lua_accessor(int i)
{
  i = CLIP(0, i, lua_args_max());
  return lua_args_name((enum lua_args) i);
}

/**********************************************************************//**
  Evaluate a line of lua script or a lua script file.
**************************************************************************/
static bool lua_command(struct connection *caller, char *arg, bool check)
{
  FILE *script_file;
  const char extension[] = ".lua", *real_filename = NULL;
  char luafile[4096], tilde_filename[4096];
  char *tokens[1], *luaarg = NULL;
  int ntokens, ind;
  enum m_pre_result result;
  bool ret = FALSE;

  ntokens = get_tokens(arg, tokens, 1, TOKEN_DELIMITERS);

  if (ntokens > 0) {
    /* match the argument */
    result = match_prefix(lua_accessor, lua_args_max() + 1, 0,
                          fc_strncasecmp, NULL, tokens[0], &ind);

    switch (result) {
    case M_PRE_EXACT:
    case M_PRE_ONLY:
      /* We have a match */
      luaarg = arg + strlen(lua_args_name(ind));
      luaarg = skip_leading_spaces(luaarg);
      break;
    case M_PRE_EMPTY:
      /* Nothing. */
      break;
    case M_PRE_AMBIGUOUS:
    case M_PRE_LONG:
    case M_PRE_FAIL:
    case M_PRE_LAST:
      /* Fall back to depreciated 'lua <script command>' syntax. */
      cmd_reply(CMD_LUA, caller, C_SYNTAX,
                _("Fall back to old syntax '%slua <script command>'."),
                caller ? "/" : "");
      ind = LUA_CMD;
      luaarg = arg;
      break;
    }
  }

  if (luaarg == NULL) {
    cmd_reply(CMD_LUA, caller, C_FAIL,
              _("No lua command or lua script file. See '%shelp lua'."),
              caller ? "/" : "");
    ret = TRUE;
    goto cleanup;
  }

  switch (ind) {
  case LUA_CMD:
    /* Nothing to check. */
    break;
  case LUA_UNSAFE_CMD:
    if (is_restricted(caller)) {
      cmd_reply(CMD_LUA, caller, C_FAIL,
                _("You aren't allowed to run unsafe Lua code."));
      ret = FALSE;
      goto cleanup;
    }
    break;
  case LUA_UNSAFE_FILE:
    if (is_restricted(caller)) {
      cmd_reply(CMD_LUA, caller, C_FAIL,
                _("You aren't allowed to run unsafe Lua code."));
      ret = FALSE;
      goto cleanup;
    }
    /* Fall through. */
  case LUA_FILE:
    /* Abuse real_filename to find if we already have a .lua extension. */
    real_filename = luaarg + strlen(luaarg) - MIN(strlen(extension),
                                                  strlen(luaarg));
    if (strcmp(real_filename, extension) != 0) {
      fc_snprintf(luafile, sizeof(luafile), "%s%s", luaarg, extension);
    } else {
      sz_strlcpy(luafile, luaarg);
    }

    if (is_restricted(caller)) {
      if (!is_safe_filename(luafile)) {
        cmd_reply(CMD_LUA, caller, C_FAIL,
                  _("Freeciv script '%s' disallowed for security reasons."),
                  luafile);
        ret = FALSE;
        goto cleanup;;
      }
      sz_strlcpy(tilde_filename, luafile);
    } else {
      interpret_tilde(tilde_filename, sizeof(tilde_filename), luafile);
    }

    real_filename = fileinfoname(get_data_dirs(), tilde_filename);
    if (!real_filename) {
      if (is_restricted(caller)) {
        cmd_reply(CMD_LUA, caller, C_FAIL,
                  _("No Freeciv script found by the name '%s'."),
                  tilde_filename);
        ret = FALSE;
        goto cleanup;
      }
      /* File is outside data directories */
      real_filename = tilde_filename;
    }
    break;
  }

  if (check) {
    ret = TRUE;
    goto cleanup;
  }

  switch (ind) {
  case LUA_CMD:
    ret = script_server_do_string(caller, luaarg);
    break;
  case LUA_UNSAFE_CMD:
    ret = script_server_unsafe_do_string(caller, luaarg);
    break;
  case LUA_FILE:
    cmd_reply(CMD_LUA, caller, C_COMMENT,
              _("Loading Freeciv script file '%s'."), real_filename);

    if (is_reg_file_for_access(real_filename, FALSE)
        && (script_file = fc_fopen(real_filename, "r"))) {
      ret = script_server_do_file(caller, real_filename);
      goto cleanup;
    } else {
      cmd_reply(CMD_LUA, caller, C_FAIL,
                _("Cannot read Freeciv script '%s'."), real_filename);
      ret = FALSE;
      goto cleanup;
    }
    break;
  case LUA_UNSAFE_FILE:
    cmd_reply(CMD_LUA, caller, C_COMMENT,
              _("Loading Freeciv script file '%s'."), real_filename);

    if (is_reg_file_for_access(real_filename, FALSE)
        && (script_file = fc_fopen(real_filename, "r"))) {
      ret = script_server_unsafe_do_file(caller, real_filename);
      goto cleanup;
    } else {
      cmd_reply(CMD_LUA, caller, C_FAIL,
                _("Cannot read Freeciv script '%s'."), real_filename);
      ret = FALSE;
      goto cleanup;
    }
    break;
  }

 cleanup:
  free_tokens(tokens, ntokens);
  return ret;
}

/* Define the possible arguments to the delegation command */
#define SPECENUM_NAME delegate_args
#define SPECENUM_VALUE0     DELEGATE_CANCEL
#define SPECENUM_VALUE0NAME "cancel"
#define SPECENUM_VALUE1     DELEGATE_RESTORE
#define SPECENUM_VALUE1NAME "restore"
#define SPECENUM_VALUE2     DELEGATE_SHOW
#define SPECENUM_VALUE2NAME "show"
#define SPECENUM_VALUE3     DELEGATE_TAKE
#define SPECENUM_VALUE3NAME "take"
#define SPECENUM_VALUE4     DELEGATE_TO
#define SPECENUM_VALUE4NAME "to"
#include "specenum_gen.h"

/**********************************************************************//**
  Returns possible parameters for the 'delegate' command.
**************************************************************************/
static const char *delegate_accessor(int i)
{
  i = CLIP(0, i, delegate_args_max());
  return delegate_args_name((enum delegate_args) i);
}

/**********************************************************************//**
  Handle delegation of control.
**************************************************************************/
static bool delegate_command(struct connection *caller, char *arg,
                             bool check)
{
  char *tokens[3];
  int ntokens, ind = delegate_args_invalid();
  enum m_pre_result result;
  bool player_specified = FALSE; /* affects messages only */
  bool ret = FALSE;
  const char *username = NULL;
  struct player *dplayer = NULL;

  if (!game_was_started()) {
    cmd_reply(CMD_DELEGATE, caller, C_FAIL, _("Game not started - "
                                              "cannot delegate yet."));
    return FALSE;
  }

  ntokens = get_tokens(arg, tokens, 3, TOKEN_DELIMITERS);

  if (ntokens > 0) {
    /* match the argument */
    result = match_prefix(delegate_accessor, delegate_args_max() + 1, 0,
                          fc_strncasecmp, NULL, tokens[0], &ind);

    switch (result) {
    case M_PRE_EXACT:
    case M_PRE_ONLY:
      /* we have a match */
      break;
    case M_PRE_EMPTY:
      if (caller) {
        /* Use 'delegate show' as default. */
        ind = DELEGATE_SHOW;
      }
      break;
    case M_PRE_AMBIGUOUS:
    case M_PRE_LONG:
    case M_PRE_FAIL:
    case M_PRE_LAST:
      ind = delegate_args_invalid();
      break;
    }
  } else {
    if (caller) {
      /* Use 'delegate show' as default. */
      ind = DELEGATE_SHOW;
    }
  }

  if (!delegate_args_is_valid(ind)) {
    char buf[256] = "";
    enum delegate_args valid_args;

    for (valid_args = delegate_args_begin();
         valid_args != delegate_args_end();
         valid_args = delegate_args_next(valid_args)) {
      cat_snprintf(buf, sizeof(buf), "'%s'",
                   delegate_args_name(valid_args));
      if (valid_args != delegate_args_max()) {
        cat_snprintf(buf, sizeof(buf), ", ");
      }
    }

    cmd_reply(CMD_DELEGATE, caller, C_SYNTAX,
              /* TRANS: do not translate the command 'delegate'. */
              _("Valid arguments for 'delegate' are: %s."), buf);
    ret =  FALSE;
    goto cleanup;
  }

  /* Get the data (player, username for delegation) and validate it. */
  switch (ind) {
  case DELEGATE_CANCEL:
    /* delegate cancel [player] */
    if (ntokens > 1) {
      if (!caller || conn_get_access(caller) >= ALLOW_ADMIN) {
        player_specified = TRUE;
        dplayer = player_by_name_prefix(tokens[1], &result);
        if (!dplayer) {
          cmd_reply_no_such_player(CMD_DELEGATE, caller, tokens[1], result);
          ret = FALSE;
          goto cleanup;
        }
      } else {
        cmd_reply(CMD_DELEGATE, caller, C_SYNTAX,
                  _("Command level '%s' or greater needed to modify "
                    "others' delegations."), cmdlevel_name(ALLOW_ADMIN));
        ret = FALSE;
        goto cleanup;
      }
    } else {
      dplayer = conn_get_player(caller);
      if (!dplayer) {
        cmd_reply(CMD_DELEGATE, caller, C_SYNTAX,
                  _("Please specify a player for whom delegation should "
                    "be canceled."));
        ret = FALSE;
        goto cleanup;
      }
    }
    break;
  case DELEGATE_RESTORE:
    /* delegate restore */
    if (!caller) {
      cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                _("You can't switch players from the console."));
      ret = FALSE;
      goto cleanup;
    }
    break;
  case DELEGATE_SHOW:
    /* delegate show [player] */
    if (ntokens > 1) {
      player_specified = TRUE;
      dplayer = player_by_name_prefix(tokens[1], &result);
      if (!dplayer) {
        cmd_reply_no_such_player(CMD_DELEGATE, caller, tokens[1], result);
        ret = FALSE;
        goto cleanup;
      }
    } else {
      dplayer = conn_get_player(caller);
      if (!dplayer) {
        cmd_reply(CMD_DELEGATE, caller, C_SYNTAX,
                  _("Please specify a player for whom the delegation should "
                    "be shown."));
        ret = FALSE;
        goto cleanup;
      }
    }
    break;
  case DELEGATE_TAKE:
    /* delegate take <player> */
    if (!caller) {
      cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                _("You can't switch players from the console."));
      ret = FALSE;
      goto cleanup;
    }
    if (ntokens > 1) {
      player_specified = TRUE;
      dplayer = player_by_name_prefix(tokens[1], &result);
      if (!dplayer) {
        cmd_reply_no_such_player(CMD_DELEGATE, caller, tokens[1], result);
        ret = FALSE;
        goto cleanup;
      }
    } else {
      cmd_reply(CMD_DELEGATE, caller, C_SYNTAX,
                _("Please specify a player to take control of."));
      ret = FALSE;
      goto cleanup;
    }
    break;
  case DELEGATE_TO:
    break;
  }
   /* All checks done to this point will give pretty much the same result at
   * any time. Checks after this point are more likely to vary over time. */
  if (check) {
    ret = TRUE;
    goto cleanup;
  }

  switch (ind) {
  case DELEGATE_TO:
    /* delegate to <username> [player] */
    if (ntokens > 1) {
      username = tokens[1];
    } else {
      cmd_reply(CMD_DELEGATE, caller, C_SYNTAX,
                _("Please specify a user to whom control is to be delegated."));
      ret = FALSE;
      break;
    }
    if (ntokens > 2) {
      player_specified = TRUE;
      dplayer = player_by_name_prefix(tokens[2], &result);
      if (!dplayer) {
        cmd_reply_no_such_player(CMD_DELEGATE, caller, tokens[2], result);
        ret = FALSE;
        break;
      }
#ifndef HAVE_FCDB
      if (caller && conn_get_access(caller) < ALLOW_ADMIN) {
#else
      if (caller && conn_get_access(caller) < ALLOW_ADMIN
          && !(srvarg.fcdb_enabled
               && script_fcdb_call("user_delegate_to", caller, dplayer,
                                   username, &ret) && ret)) {
#endif
        cmd_reply(CMD_DELEGATE, caller, C_SYNTAX,
                  _("Command level '%s' or greater or special permission "
                    "needed to modify others' delegations."),
                  cmdlevel_name(ALLOW_ADMIN));
        ret = FALSE;
        break;
      }
    } else {
      dplayer = conn_controls_player(caller) ? conn_get_player(caller) : NULL;
      if (!dplayer) {
        cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                  _("You do not control a player."));
        ret = FALSE;
        break;
      }
    }

    /* Delegate control of player to another user. */
    fc_assert_ret_val(dplayer, FALSE);
    fc_assert_ret_val(username != NULL, FALSE);

    /* Forbid delegation of players already controlled by a delegate, and
     * those 'put aside' by a delegate.
     * For the former, if player is already under active delegate control,
     * we wouldn't handle the revocation that would be necessary if their
     * delegation changed; and the authority granted to delegates does not
     * include the ability to sub-delegate.
     * For the latter, allowing control of the 'put aside' player to be
     * delegated would break the invariant that whenever a user is connected,
     * they are attached to 'their' player. */
    if (player_delegation_active(dplayer)) {
      if (!player_delegation_get(dplayer)) {
        /* Attempting to change a 'put aside' player. Must be admin 
         * or console. */
        fc_assert(player_specified);
        cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                  _("Can't delegate control of '%s' belonging to %s while "
                    "they are controlling another player."),
                    player_name(dplayer), dplayer->username);
      } else if (player_specified) {
        /* Admin or console attempting to change a controlled player. */
        cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                  _("Can't change delegation of '%s' while controlled by "
                    "delegate %s."), player_name(dplayer), dplayer->username);
      } else {
        /* Caller must be the delegate. Give more specific message.
         * (We don't know if they thought they were delegating their
         * original or delegated player, but we don't allow either.) */
        cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                  _("You can't delegate control while you are controlling "
                    "a delegated player yourself."));
      }
      ret = FALSE;
      break;
    }

    /* Forbid delegation to player's original owner
     * (from above test we know that dplayer->username is the original now) */
    if (fc_strcasecmp(dplayer->username, username) == 0) {
      if (player_specified) {
        /* Probably admin or console. */
        cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                  /* TRANS: don't translate 'delegate cancel' */
                  _("%s already owns '%s', so cannot also be delegate. "
                    "Use '%sdelegate cancel' to cancel an existing "
                    "delegation."),
                  username, player_name(dplayer), caller?"/":"");
      } else {
        /* Player not specified on command line, so they must have been trying
         * to delegate control to themself. Give more specific message. */
        cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                  /* TRANS: don't translate '/delegate cancel' */
                  _("You can't delegate control to yourself. "
                    "Use '/delegate cancel' to cancel an existing "
                    "delegation."));
      }
      ret = FALSE;
      break;
    }

    /* FIXME: if control was already delegated to someone else, that
     * delegation is implicitly canceled. Perhaps we should tell someone. */

    player_delegation_set(dplayer, username);
    cmd_reply(CMD_DELEGATE, caller, C_OK,
              _("Control of player '%s' delegated to user %s."),
              player_name(dplayer), username);
    ret = TRUE;
    break;

  case DELEGATE_SHOW:
    /* Show delegations. */
    fc_assert_ret_val(dplayer, FALSE);

    if (player_delegation_get(dplayer) == NULL) {
      /* No delegation set. */
      cmd_reply(CMD_DELEGATE, caller, C_COMMENT,
                _("No delegation defined for '%s'."),
                player_name(dplayer));
    } else {
      cmd_reply(CMD_DELEGATE, caller, C_COMMENT,
                _("Control of player '%s' delegated to user %s."),
                player_name(dplayer), player_delegation_get(dplayer));
    }
    ret = TRUE;
    break;

  case DELEGATE_CANCEL:
    if (player_delegation_get(dplayer) == NULL) {
      /* No delegation set. */
      cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                _("No delegation defined for '%s'."),
                player_name(dplayer));
      ret = FALSE;
      break;
    }

    if (player_delegation_active(dplayer)) {
      /* Delegation is currently in use. Forcibly break connection. */
      struct connection *pdelegate;
      /* (Can only happen if admin/console issues this command, as owner
       * will end use by their mere presence.) */
      fc_assert(player_specified);
      pdelegate = conn_by_user(player_delegation_get(dplayer));
      fc_assert_ret_val(pdelegate != NULL, FALSE);
      if (!connection_delegate_restore(pdelegate)) {
        /* Should never happen. Generic failure message. */
        log_error("Failed to restore %s's connection as %s during "
                  "'delegate cancel'.", pdelegate->username,
                  delegate_player_str(pdelegate->server.delegation.playing,
                                      pdelegate->server.delegation.observer));
        cmd_reply(CMD_DELEGATE, caller, C_FAIL, _("Unexpected failure."));
        ret = FALSE;
        break;
      }
      notify_conn(pdelegate->self, NULL, E_CONNECTION, ftc_server,
                  _("Your delegated control of player '%s' was canceled."),
                  player_name(dplayer));
    }

    player_delegation_set(dplayer, NULL);
    cmd_reply(CMD_DELEGATE, caller, C_OK, _("Delegation of '%s' canceled."),
              player_name(dplayer));
    ret = TRUE;
    break;

  case DELEGATE_TAKE:
    /* Try to take another player. */
    fc_assert_ret_val(dplayer, FALSE);
    fc_assert_ret_val(caller, FALSE);

    if (caller->server.delegation.status) {
      cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                /* TRANS: don't translate '/delegate restore'. */
                _("You are already controlling a delegated player. "
                  "Use '/delegate restore' to relinquish control of your "
                  "current player first."));
      ret = FALSE;
      break;
    }

    /* Don't allow 'put aside' players to be delegated; the invariant is
     * that while the owning user is connected to the server, they are
     * in sole control of 'their' player. */
    if (conn_controls_player(caller)
        && player_delegation_get(conn_get_player(caller)) != NULL) {
      cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                /* TRANS: don't translate '/delegate cancel'. */
                _("Can't take player while you have delegated control "
                  "yourself. Use '/delegate cancel' to cancel your own "
                  "delegation first."));
      ret = FALSE;
      break;
    }

    /* Taking your own player makes no sense. */
    if (conn_controls_player(caller)
        && dplayer == conn_get_player(caller)) {
      cmd_reply(CMD_DELEGATE, caller, C_FAIL, _("You already control '%s'."),
                player_name(conn_get_player(caller)));
      ret = FALSE;
      break;
    }

    if (!player_delegation_get(dplayer)
        || fc_strcasecmp(player_delegation_get(dplayer), caller->username) != 0) {
      cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                _("Control of player '%s' has not been delegated to you."),
                player_name(dplayer));
      ret = FALSE;
      break;
    }

    /* If the player is controlled by another user, fail. */
    if (dplayer->is_connected) {
      cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                _("Another user already controls player '%s'."),
                player_name(dplayer));
      ret = FALSE;
      break;
    }

    if (!connection_delegate_take(caller, dplayer)) {
      /* Should never happen. Generic failure message. */
      log_error("%s failed to take control of '%s' during 'delegate take'.",
                caller->username, player_name(dplayer));
      cmd_reply(CMD_DELEGATE, caller, C_FAIL, _("Unexpected failure."));
      ret = FALSE;
      break;
    }

    cmd_reply(CMD_DELEGATE, caller, C_OK,
              _("%s is now controlling player '%s'."), caller->username,
              player_name(conn_get_player(caller)));
    ret = TRUE;
    break;

  case DELEGATE_RESTORE:
    /* Delegate user relinquishes control of delegated player, returning to 
     * previous view (e.g. observer) if any. */
    fc_assert_ret_val(caller, FALSE);

    if (!caller->server.delegation.status) {
      cmd_reply(CMD_DELEGATE, caller, C_FAIL,
                _("You are not currently controlling a delegated player."));
      ret = FALSE;
      break;
    }

    if (!connection_delegate_restore(caller)) {
      /* Should never happen. Generic failure message. */
      log_error("Failed to restore %s's connection as %s during "
                "'delegate restore'.", caller->username,
                delegate_player_str(caller->server.delegation.playing,
                                    caller->server.delegation.observer));
      cmd_reply(CMD_DELEGATE, caller, C_FAIL, _("Unexpected failure."));
      ret = FALSE;
      break;
    }

    cmd_reply(CMD_DELEGATE, caller, C_OK,
              /* TRANS: "<user> is now connected to <player>" where <player>
               * can also be "global observer" or "nothing" */
              _("%s is now connected as %s."), caller->username,
              delegate_player_str(conn_get_player(caller), caller->observer));
    ret = TRUE;
    break;
  }

 cleanup:
  free_tokens(tokens, ntokens);
  return ret;
}

/**********************************************************************//**
  Return static string describing what a connection is connected to.
**************************************************************************/
static const char *delegate_player_str(struct player *pplayer, bool observer)
{
  static struct astring buf;

  if (pplayer) {
    if (observer) {
      astr_set(&buf, _("%s (observer)"), player_name(pplayer));
    } else {
      astr_set(&buf, "%s", player_name(pplayer));
    }
  } else if (observer) {
    astr_set(&buf, "%s", _("global observer"));
  } else {
    /* TRANS: in place of player name or "global observer" */
    astr_set(&buf, "%s", _("nothing"));
  }

  return astr_str(&buf);
}

/* Define the possible arguments to the mapimg command */
/* map image layers */
#define SPECENUM_NAME mapimg_args
#define SPECENUM_VALUE0     MAPIMG_COLORTEST
#define SPECENUM_VALUE0NAME "colortest"
#define SPECENUM_VALUE1     MAPIMG_CREATE
#define SPECENUM_VALUE1NAME "create"
#define SPECENUM_VALUE2     MAPIMG_DEFINE
#define SPECENUM_VALUE2NAME "define"
#define SPECENUM_VALUE3     MAPIMG_DELETE
#define SPECENUM_VALUE3NAME "delete"
#define SPECENUM_VALUE4     MAPIMG_SHOW
#define SPECENUM_VALUE4NAME "show"
#define SPECENUM_COUNT      MAPIMG_COUNT
#include "specenum_gen.h"

/**********************************************************************//**
  Returns possible parameters for the mapimg command.
**************************************************************************/
static const char *mapimg_accessor(int i)
{
  i = CLIP(0, i, mapimg_args_max());
  return mapimg_args_name((enum mapimg_args) i);
}

/**********************************************************************//**
  Handle mapimg command
**************************************************************************/
static bool mapimg_command(struct connection *caller, char *arg, bool check)
{
  enum m_pre_result result;
  int ind, ntokens, id;
  char *token[2];
  bool ret = TRUE;

  ntokens = get_tokens(arg, token, 2, TOKEN_DELIMITERS);

  if (ntokens > 0) {
    /* match the argument */
    result = match_prefix(mapimg_accessor, MAPIMG_COUNT, 0,
                          fc_strncasecmp, NULL, token[0], &ind);

    switch (result) {
    case M_PRE_EXACT:
    case M_PRE_ONLY:
      /* we have a match */
      break;
    case M_PRE_AMBIGUOUS:
      cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                _("Ambiguous 'mapimg' command."));
      ret =  FALSE;
      goto cleanup;
      break;
    case M_PRE_EMPTY:
      /* use 'show' as default */
      ind = MAPIMG_SHOW;
      break;
    case M_PRE_LONG:
    case M_PRE_FAIL:
    case M_PRE_LAST:
      {
        char buf[256] = "";
        enum mapimg_args valid_args;

        for (valid_args = mapimg_args_begin();
             valid_args != mapimg_args_end();
             valid_args = mapimg_args_next(valid_args)) {
          cat_snprintf(buf, sizeof(buf), "'%s'",
                       mapimg_args_name(valid_args));
          if (valid_args != mapimg_args_max()) {
            cat_snprintf(buf, sizeof(buf), ", ");
          }
        }

        cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                  _("The valid arguments are: %s."), buf);
        ret =  FALSE;
        goto cleanup;
      }
      break;
    }
  } else {
    /* use 'show' as default */
    ind = MAPIMG_SHOW;
  }

  switch (ind) {
  case MAPIMG_DEFINE:
    if (ntokens == 1) {
      cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                _("Missing argument for 'mapimg define'."));
      ret = FALSE;
    } else {
      /* 'mapimg define <mapstr>' */
      if (!mapimg_define(token[1], check)) {
        cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                  _("Can't use definition: %s."), mapimg_error());
        ret = FALSE;
      } else if (check) {
        /* Validated OK, bail out now */
        goto cleanup;
      } else if (game_was_started()
                 && mapimg_isvalid(mapimg_count() - 1) == NULL) {
        /* game was started - error in map image definition check */
        cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                  _("Can't use definition: %s."), mapimg_error());
        ret = FALSE;
      } else {
        char str[MAX_LEN_MAPDEF];

        id = mapimg_count() - 1;

        mapimg_id2str(id, str, sizeof(str));
        cmd_reply(CMD_MAPIMG, caller, C_OK, _("Defined as map image "
                                              "definition %d: '%s'."),
                  id, str);
      }
    }
    break;

  case MAPIMG_DELETE:
    if (ntokens == 1) {
      cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                _("Missing argument for 'mapimg delete'."));
      ret = FALSE;
    } else if (ntokens == 2 && strcmp(token[1], "all") == 0) {
      /* 'mapimg delete all' */
      if (check) {
        goto cleanup;
      }

      while (mapimg_count() > 0) {
        mapimg_delete(0);
      }
      cmd_reply(CMD_MAPIMG, caller, C_OK, _("All map image definitions "
                                            "deleted."));
    } else if (ntokens == 2 && sscanf(token[1], "%d", &id) != 0) {
      /* 'mapimg delete <id>' */
      if (check) {
        goto cleanup;
      }

      if (!mapimg_delete(id)) {
        cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                  _("Couldn't delete definition: %s."), mapimg_error());
        ret = FALSE;
      } else {
        cmd_reply(CMD_MAPIMG, caller, C_OK, _("Map image definition %d "
                                              "deleted."), id);
      }
    } else {
      cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                _("Bad argument for 'mapimg delete': '%s'."), token[1]);
      ret = FALSE;
    }
    break;

  case MAPIMG_SHOW:
    if (ntokens < 2 || (ntokens == 2 && strcmp(token[1], "all") == 0)) {
      /* 'mapimg show' or 'mapimg show all' */
      if (check) {
        goto cleanup;
      }
      show_mapimg(caller, CMD_MAPIMG);
    } else if (ntokens == 2 && sscanf(token[1], "%d", &id) != 0) {
      char str[2048];
      /* 'mapimg show <id>' */
      if (check) {
        goto cleanup;
      }

      if (mapimg_show(id, str, sizeof(str), TRUE)) {
        cmd_reply(CMD_MAPIMG, caller, C_OK, "%s", str);
      } else {
        cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                  _("Couldn't show definition: %s."), mapimg_error());
        ret = FALSE;
      }
    } else {
      cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                _("Bad argument for 'mapimg show': '%s'."), token[1]);
      ret = FALSE;
    }
    break;

  case MAPIMG_COLORTEST:
    if (check) {
      goto cleanup;
    }

    mapimg_colortest(game.server.save_name, NULL);
    cmd_reply(CMD_MAPIMG, caller, C_OK, _("Map color test images saved."));
    break;

  case MAPIMG_CREATE:
    if (ntokens < 2) {
      cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                _("Missing argument for 'mapimg create'."));
      ret = FALSE;
      goto cleanup;
    }

    if (strcmp(token[1], "all") == 0) {
      /* 'mapimg create all' */
      if (check) {
        goto cleanup;
      }

      for (id = 0; id < mapimg_count(); id++) {
        struct mapdef *pmapdef = mapimg_isvalid(id);

        if (pmapdef == NULL
            || !mapimg_create(pmapdef, TRUE, game.server.save_name,
                              srvarg.saves_pathname)) {
          cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                _("Error saving map image %d: %s."), id, mapimg_error());
          ret = FALSE;
        }
      }
    } else if (sscanf(token[1], "%d", &id) != 0) {
      struct mapdef *pmapdef;

      /* 'mapimg create <id>' */
      if (check) {
        goto cleanup;
      }

      pmapdef = mapimg_isvalid(id);
      if (pmapdef == NULL
          || !mapimg_create(pmapdef, TRUE, game.server.save_name,
                            srvarg.saves_pathname)) {
        cmd_reply(CMD_MAPIMG, caller, C_FAIL,
              _("Error saving map image %d: %s."), id, mapimg_error());
        ret = FALSE;
      }
    } else {
      cmd_reply(CMD_MAPIMG, caller, C_FAIL,
                _("Bad argument for 'mapimg create': '%s'."), token[1]);
      ret = FALSE;
    }
    break;
  }

  cleanup:

  free_tokens(token, ntokens);

  return ret;
}

/**********************************************************************//**
  Execute a command in the context of the AI of the player.
**************************************************************************/
static bool aicmd_command(struct connection *caller, char *arg, bool check)
{
  enum m_pre_result match_result;
  struct player *pplayer;
  char *tokens[1], *cmd = NULL;
  int ntokens;
  bool ret = FALSE;

  ntokens = get_tokens(arg, tokens, 1, TOKEN_DELIMITERS);

  if (ntokens < 1) {
    cmd_reply(CMD_AICMD, caller, C_FAIL,
              _("No player given for aicmd."));
    goto cleanup;
  }

  pplayer = player_by_name_prefix(tokens[0], &match_result);

  if (NULL == pplayer) {
    cmd_reply_no_such_player(CMD_AICMD, caller, tokens[0], match_result);
    goto cleanup;
  }

  /* We have a player - extract the command. */
  cmd = arg + strlen(tokens[0]);
  cmd = skip_leading_spaces(cmd);

  if (strlen(cmd) == 0) {
    cmd_reply(CMD_AICMD, caller, C_FAIL,
              _("No command for the AI console defined."));
    goto cleanup;
  }

  if (check) {
    ret = TRUE;
    goto cleanup;
  }

  /* This check is needed to return a message if the function is not defined
   * for the AI of the player. */
  if (pplayer && pplayer->ai) {
    if (pplayer->ai->funcs.player_console) {
      cmd_reply(CMD_AICMD, caller, C_OK,
                _("AI console for player %s. Command: '%s'."),
                player_name(pplayer), cmd);
      CALL_PLR_AI_FUNC(player_console, pplayer, pplayer, cmd);
      ret = TRUE;
    } else {
      cmd_reply(CMD_AICMD, caller, C_FAIL,
                _("No AI console defined for the AI '%s' of player %s."),
                ai_name(pplayer->ai), player_name(pplayer));
    }
  } else {
    cmd_reply(CMD_AICMD, caller, C_FAIL, _("No AI defined for player %s."),
              player_name(pplayer));
  }

 cleanup:
  free_tokens(tokens, ntokens);
  return ret;
}

/* Define the possible arguments to the fcdb command */
#define SPECENUM_NAME fcdb_args
#define SPECENUM_VALUE0     FCDB_RELOAD
#define SPECENUM_VALUE0NAME "reload"
#define SPECENUM_VALUE1     FCDB_LUA
#define SPECENUM_VALUE1NAME "lua"
#define SPECENUM_COUNT      FCDB_COUNT
#include "specenum_gen.h"

/**********************************************************************//**
  Returns possible parameters for the fcdb command.
**************************************************************************/
static const char *fcdb_accessor(int i)
{
  i = CLIP(0, i, fcdb_args_max());
  return fcdb_args_name((enum fcdb_args) i);
}

/**********************************************************************//**
  Handle the freeciv database script module.
**************************************************************************/
static bool fcdb_command(struct connection *caller, char *arg, bool check)
{
  enum m_pre_result result;
  int ind, ntokens;
  char *token[1];
  bool ret = TRUE;
  bool usage = FALSE;

#ifndef HAVE_FCDB
  cmd_reply(CMD_FCDB, caller, C_FAIL,
            _("Freeciv database script deactivated at compile time."));
  return FALSE;
#endif

  if (!srvarg.fcdb_enabled) {
    /* Not supposed to be used. It isn't initialized. */
    cmd_reply(CMD_FCDB, caller, C_FAIL,
              _("Freeciv database script not activated at server start. "
                "See the Freeciv server's --auth command line option."));
    return FALSE;
  }

  ntokens = get_tokens(arg, token, 1, TOKEN_DELIMITERS);

  if (ntokens > 0) {
    /* match the argument */
    result = match_prefix(fcdb_accessor, FCDB_COUNT, 0,
                          fc_strncasecmp, NULL, token[0], &ind);

    switch (result) {
    case M_PRE_EXACT:
    case M_PRE_ONLY:
      /* we have a match */
      break;
    case M_PRE_AMBIGUOUS:
      cmd_reply(CMD_FCDB, caller, C_FAIL,
                _("Ambiguous fcdb command."));
      ret =  FALSE;
      goto cleanup;
      break;
    case M_PRE_EMPTY:
    case M_PRE_LONG:
    case M_PRE_FAIL:
    case M_PRE_LAST:
      usage = TRUE;
      break;
    }
  } else {
    usage = TRUE;
  }

  if (usage) {
    char buf[256] = "";
    enum fcdb_args valid_args;

    for (valid_args = fcdb_args_begin();
        valid_args != fcdb_args_end();
        valid_args = fcdb_args_next(valid_args)) {
      cat_snprintf(buf, sizeof(buf), "'%s'",
                   fcdb_args_name(valid_args));
      if (valid_args != fcdb_args_max()) {
        cat_snprintf(buf, sizeof(buf), ", ");
      }
    }

    cmd_reply(CMD_FCDB, caller, C_FAIL,
              _("The valid arguments are: %s."), buf);
    ret =  FALSE;
    goto cleanup;
  }

  if (check) {
    ret = TRUE;
    goto cleanup;
  }

  switch (ind) {
  case FCDB_RELOAD:
    /* Reload database lua script. */
    script_fcdb_free();
    script_fcdb_init(NULL);
    break;

  case FCDB_LUA:
    /* Skip whitespaces. */
    arg = skip_leading_spaces(arg);
    /* Skip the base argument 'lua'. */
    arg += 3;
    /* Now execute the scriptlet. */
    ret = script_fcdb_do_string(caller, arg);
    break;
  }

  cleanup:

  free_tokens(token, ntokens);

  return ret;
}

/**********************************************************************//**
  Send start command related message
**************************************************************************/
static void start_cmd_reply(struct connection *caller, bool notify, char *msg)
{
  cmd_reply(CMD_START_GAME, caller, C_FAIL, "%s", msg);
  if (notify) {
    notify_conn(NULL, NULL, E_SETTING, ftc_server, "%s", msg);
  }
}

/**********************************************************************//**
  Handle start command. Notify all players about errors if notify set.
**************************************************************************/
bool start_command(struct connection *caller, bool check, bool notify)
{
  int human_players;

  switch (server_state()) {
  case S_S_INITIAL:
    /* Sanity check scenario */
    if (game.info.is_new_game && !check) {
      if (0 < map_startpos_count()
          && game.server.max_players > map_startpos_count()) {
        /* If we load a pre-generated map (i.e., a scenario) it is possible
         * to increase the number of players beyond the number supported by
         * the scenario. The solution is a hack: cut the extra players
         * when the game starts. */
        log_verbose("Reduced maxplayers from %d to %d to fit "
                    "to the number of start positions.",
                    game.server.max_players, map_startpos_count());
        game.server.max_players = map_startpos_count();
      }

      if (normal_player_count() > game.server.max_players) {
        int i;
        struct player *pplayer;

        for (i = player_slot_count() - 1; i >= 0; i--) {
          pplayer = player_by_number(i);
          if (pplayer) {
            server_remove_player(pplayer);
          }
          if (normal_player_count() <= game.server.max_players) {
            break;
          }
        }

        log_verbose("Had to cut down the number of players to the "
                    "number of map start positions, there must be "
                    "something wrong with the savegame or you "
                    "adjusted the maxplayers value.");
      }
    }

    human_players = 0;
    players_iterate(plr) {
      if (is_human(plr)) {
        human_players++;
      }
    } players_iterate_end;

    /* check min_players.
     * Allow continuing of savegames where some of the original
     * players have died */
    if (game.info.is_new_game
        && human_players < game.server.min_players) {
      char buf[512] = "";

      fc_snprintf(buf, sizeof(buf),
                  _("Not enough human players ('minplayers' server setting has value %d); game will not start."),
                  game.server.min_players);
      start_cmd_reply(caller, notify, buf);
      return FALSE;
    } else if (player_count() < 1) {
      /* At least one player required */
      start_cmd_reply(caller, notify,
                      _("No players; game will not start."));
      return FALSE;
    } else if (normal_player_count() > server.playable_nations) {
      if (nation_set_count() > 1) {
        start_cmd_reply(caller, notify,
                        _("Not enough nations in the current nation set "
                          "for all players; game will not start. "
                          "(See 'nationset' setting.)"));
      } else {
        start_cmd_reply(caller, notify,
                        _("Not enough nations for all players; game will "
                          "not start."));
      }
      return FALSE;
    } else if (strlen(game.server.start_units) == 0 && !game.server.start_city) {
      start_cmd_reply(caller, notify,
                      _("Neither 'startcity' nor 'startunits' setting gives "
                        "players anything to start game with; game will "
                        "not start."));
      return FALSE;
    } else if (check) {
      return TRUE;
    } else if (!caller) {
      if (notify) {
        /* Called from handle_player_ready()
         * Last player just toggled ready-status. */
        notify_conn(NULL, NULL, E_SETTING, ftc_game_start,
                    _("All players are ready; starting game."));
      }
      start_game();
      return TRUE;
    } else if (NULL == caller->playing || caller->observer) {
      /* A detached or observer player can't do /start. */
      return TRUE;
    } else {
      /* This might trigger recursive call to start_command() if this is
       * last player who gets ready. In that case caller is NULL. */
      handle_player_ready(caller->playing, player_number(caller->playing), TRUE);
      return TRUE;
    }
  case S_S_OVER:
    start_cmd_reply(caller, notify,
                    /* TRANS: given when /start is invoked during gameover. */
                    _("Cannot start the game: the game is waiting for all clients "
                      "to disconnect."));
    return FALSE;
  case S_S_RUNNING:
    start_cmd_reply(caller, notify,
                    /* TRANS: given when /start is invoked while the game
                     * is running. */
                    _("Cannot start the game: it is already running."));
    return FALSE;
  }
  log_error("Unknown server state variant: %d.", server_state());
  return FALSE;
}

/**********************************************************************//**
  Handle cut command
**************************************************************************/
static bool cut_client_connection(struct connection *caller, char *name, 
                                  bool check)
{
  enum m_pre_result match_result;
  struct connection *ptarget;

  ptarget = conn_by_user_prefix(name, &match_result);

  if (!ptarget) {
    cmd_reply_no_such_conn(CMD_CUT, caller, name, match_result);
    return FALSE;
  } else if (check) {
    return TRUE;
  }

  if (conn_controls_player(ptarget)) {
    /* If we cut the connection, unassign the login name.*/
    sz_strlcpy(ptarget->playing->username, _(ANON_USER_NAME));
    ptarget->playing->unassigned_user = TRUE;
  }

  cmd_reply(CMD_CUT, caller, C_DISCONNECTED,
            _("Cutting connection %s."), ptarget->username);
  connection_close_server(ptarget, _("connection cut"));

  return TRUE;
}


/**********************************************************************//**
  Utility for 'kick_hash' tables.
**************************************************************************/
static time_t *time_duplicate(const time_t *t)
{
  time_t *d = fc_malloc(sizeof(*d));
  *d = *t;
  return d;
}

/**********************************************************************//**
  Returns FALSE if the connection isn't kicked and can connect the server
  normally.
**************************************************************************/
bool conn_is_kicked(struct connection *pconn, int *time_remaining)
{
  time_t time_of_addr_kick, time_of_user_kick;
  time_t now, time_of_kick = 0;

  if (NULL != time_remaining) {
    *time_remaining = 0;
  }

  fc_assert_ret_val(NULL != kick_table_by_addr, FALSE);
  fc_assert_ret_val(NULL != kick_table_by_user, FALSE);
  fc_assert_ret_val(NULL != pconn, FALSE);

  if (kick_hash_lookup(kick_table_by_addr, pconn->server.ipaddr,
                       &time_of_addr_kick)) {
    time_of_kick = time_of_addr_kick;
  }
  if (kick_hash_lookup(kick_table_by_user, pconn->username,
                       &time_of_user_kick)
      && time_of_user_kick > time_of_kick) {
    time_of_kick = time_of_user_kick;
  }

  if (0 == time_of_kick) {
    return FALSE; /* Not found. */
  }

  now = time(NULL);
  if (now - time_of_kick > game.server.kick_time) {
    /* Kick timeout expired. */
    if (0 != time_of_addr_kick) {
      kick_hash_remove(kick_table_by_addr, pconn->server.ipaddr);
    }
    if (0 != time_of_user_kick) {
      kick_hash_remove(kick_table_by_user, pconn->username);
    }
    return FALSE;
  }

  if (NULL != time_remaining) {
    *time_remaining = game.server.kick_time - (now - time_of_kick);
  }
  return TRUE;
}

/**********************************************************************//**
  Kick command handler.
**************************************************************************/
static bool kick_command(struct connection *caller, char *name, bool check)
{
  char ipaddr[FC_MEMBER_SIZEOF(struct connection, server.ipaddr)];
  struct connection *pconn;
  enum m_pre_result match_result;
  time_t now;

  remove_leading_trailing_spaces(name);
  pconn = conn_by_user_prefix(name, &match_result);
  if (NULL == pconn) {
    cmd_reply_no_such_conn(CMD_KICK, caller, name, match_result);
    return FALSE;
  }

  if (NULL != caller && ALLOW_ADMIN > conn_get_access(caller)) {
    const int MIN_UNIQUE_CONNS = 3;
    const char *unique_ipaddr[MIN_UNIQUE_CONNS];
    int i, num_unique_connections = 0;

    if (pconn == caller) {
      cmd_reply(CMD_KICK, caller, C_FAIL, _("You may not kick yourself."));
      return FALSE;
    }

    conn_list_iterate(game.est_connections, aconn) {
      for (i = 0; i < num_unique_connections; i++) {
        if (0 == strcmp(unique_ipaddr[i], aconn->server.ipaddr)) {
          /* Already listed. */
          break;
        }
      }
      if (i >= num_unique_connections) {
        num_unique_connections++;
        if (MIN_UNIQUE_CONNS <= num_unique_connections) {
          /* We have enought already. */
          break;
        }
        unique_ipaddr[num_unique_connections - 1] = aconn->server.ipaddr;
      }
    } conn_list_iterate_end;

    if (MIN_UNIQUE_CONNS > num_unique_connections) {
      cmd_reply(CMD_KICK, caller, C_FAIL,
                _("There must be at least %d unique connections to the "
                  "server for this command to be valid."), MIN_UNIQUE_CONNS);
      return FALSE;
    }
  }

  if (check) {
    return TRUE;
  }

  sz_strlcpy(ipaddr, pconn->server.ipaddr);
  now = time(NULL);
  kick_hash_replace(kick_table_by_addr, ipaddr, now);

  conn_list_iterate(game.all_connections, aconn) {
    if (0 != strcmp(ipaddr, aconn->server.ipaddr)) {
      continue;
    }

    if (conn_controls_player(aconn)) {
      /* Unassign the username. */
      sz_strlcpy(aconn->playing->username, _(ANON_USER_NAME));
      aconn->playing->unassigned_user = TRUE;
    }

    kick_hash_replace(kick_table_by_user, aconn->username, now);

    connection_close_server(aconn, _("kicked"));
  } conn_list_iterate_end;

  return TRUE;
}


/**********************************************************************//**
  Show caller introductory help about the server. help_cmd is the command
  the player used.
**************************************************************************/
static void show_help_intro(struct connection *caller,
                            enum command_id help_cmd)
{
  /* This is formated like extra_help entries for settings and commands: */
  char *help = fc_strdup(
    _("Welcome - this is the introductory help text for the Freeciv "
      "server.\n"
      "\n"
      "Two important server concepts are Commands and Options. Commands, "
      "such as 'help', are used to interact with the server. Some commands "
      "take one or more arguments, separated by spaces. In many cases "
      "commands and command arguments may be abbreviated. Options are "
      "settings which control the server as it is running.\n"
      "\n"
      "To find out how to get more information about commands and options, "
      "use 'help help'.\n"
      "\n"
      "For the impatient, the main commands to get going are:\n"
      "  show   -  to see current options\n"
      "  set    -  to set options\n"
      "  start  -  to start the game once players have connected\n"
      "  save   -  to save the current game\n"
      "  quit   -  to exit"));

  fc_break_lines(help, LINE_BREAK);
  cmd_reply(help_cmd, caller, C_COMMENT, "%s", help);
  FC_FREE(help);
}

/**********************************************************************//**
  Show the caller detailed help for the single COMMAND given by id.
  help_cmd is the command the player used.
**************************************************************************/
static void show_help_command(struct connection *caller,
			      enum command_id help_cmd,
			      enum command_id id)
{
  const struct command *cmd = command_by_number(id);
  
  if (command_short_help(cmd)) {
    cmd_reply(help_cmd, caller, C_COMMENT,
              /* TRANS: <untranslated name> - translated short help */
              _("Command: %s  -  %s"),
              command_name(cmd),
              command_short_help(cmd));
  } else {
    cmd_reply(help_cmd, caller, C_COMMENT,
	      /* TRANS: <untranslated name> */
	      _("Command: %s"),
	      command_name(cmd));
  }
  if (command_synopsis(cmd)) {
    /* line up the synopsis lines: */
    const char *syn = _("Synopsis: ");
    size_t synlen = strlen(syn);
    char prefix[40];

    fc_snprintf(prefix, sizeof(prefix), "%*s", (int) synlen, " ");
    cmd_reply_prefix(help_cmd, caller, C_COMMENT, prefix,
                     "%s%s", syn, command_synopsis(cmd));
  }
  cmd_reply(help_cmd, caller, C_COMMENT,
            _("Level: %s"), cmdlevel_name(command_level(cmd)));
  {
    char *help = command_extra_help(cmd);

    if (help) {
      fc_break_lines(help, LINE_BREAK);
      cmd_reply(help_cmd, caller, C_COMMENT, _("Description:"));
      cmd_reply_prefix(help_cmd, caller, C_COMMENT, "  ", "  %s", help);
      FC_FREE(help);
    }
  }
}

/**********************************************************************//**
  Show the caller list of COMMANDS.
  help_cmd is the command the player used.
**************************************************************************/
static void show_help_command_list(struct connection *caller,
                                   enum command_id help_cmd)
{
  enum command_id i;

  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
  cmd_reply(help_cmd, caller, C_COMMENT,
            _("The following server commands are available:"));
  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
  if (!caller && con_get_style()) {
    for (i = 0; i < CMD_NUM; i++) {
      cmd_reply(help_cmd, caller, C_COMMENT, "%s", command_name_by_number(i));
    }
  } else {
    char buf[MAX_LEN_CONSOLE_LINE];
    int j;

    buf[0] = '\0';
    for (i=0, j=0; i<CMD_NUM; i++) {
      if (may_use(caller, i)) {
        cat_snprintf(buf, sizeof(buf), "%-19s", command_name_by_number(i));
        if ((++j % 4) == 0) {
          cmd_reply(help_cmd, caller, C_COMMENT, "%s", buf);
          buf[0] = '\0';
        }
      }
    }
    if (buf[0] != '\0') {
      cmd_reply(help_cmd, caller, C_COMMENT, "%s", buf);
    }
  }
  cmd_reply(help_cmd, caller, C_COMMENT, horiz_line);
}

/**********************************************************************//**
  Send a reply to the caller listing the matched names from an ambiguous
  prefix.
**************************************************************************/
static void cmd_reply_matches(enum command_id cmd,
                              struct connection *caller,
                              m_pre_accessor_fn_t accessor_fn,
                              int *matches, int num_matches)
{
  char buf[MAX_LEN_MSG];
  const char *src, *end;
  char *dest;
  int i;

  if (accessor_fn == NULL || matches == NULL || num_matches < 1) {
    return;
  }

  dest = buf;
  end = buf + sizeof(buf) - 1;

  for (i = 0; i < num_matches && dest < end; i++) {
    src = accessor_fn(matches[i]);
    if (!src) {
      continue;
    }
    if (dest != buf) {
      *dest++ = ' ';
    }
    while (*src != '\0' && dest < end) {
      *dest++ = *src++;
    }
  }
  *dest = '\0';

  cmd_reply(cmd, caller, C_COMMENT, _("Possible matches: %s"), buf);
}

/**************************************************************************
  Additional 'help' arguments
**************************************************************************/
#define SPECENUM_NAME help_general_args
#define SPECENUM_VALUE0     HELP_GENERAL_COMMANDS
#define SPECENUM_VALUE0NAME "commands"
#define SPECENUM_VALUE1     HELP_GENERAL_OPTIONS
#define SPECENUM_VALUE1NAME "options"
#define SPECENUM_COUNT      HELP_GENERAL_COUNT
#include "specenum_gen.h"

/**************************************************************************
  Unified indices for help arguments:
    CMD_NUM           -  Server commands
    HELP_GENERAL_NUM  -  General help arguments, above
    settings_number() -  Server options 
**************************************************************************/
#define HELP_ARG_NUM (CMD_NUM + HELP_GENERAL_COUNT + settings_number())

/**********************************************************************//**
  Convert unified helparg index to string; see above.
**************************************************************************/
static const char *helparg_accessor(int i)
{
  if (i < CMD_NUM) {
    return command_name_by_number(i);
  }

  i -= CMD_NUM;
  if (i < HELP_GENERAL_COUNT) {
    return help_general_args_name((enum help_general_args) i);
  }

  i -= HELP_GENERAL_COUNT;
  return optname_accessor(i);
}

/**********************************************************************//**
  Handle help command
**************************************************************************/
static bool show_help(struct connection *caller, char *arg)
{
  int matches[64], num_matches = 0;
  enum m_pre_result match_result;
  int ind;

  fc_assert_ret_val(!may_use_nothing(caller), FALSE);
    /* no commands means no help, either */

  match_result = match_prefix_full(helparg_accessor, HELP_ARG_NUM, 0,
                                   fc_strncasecmp, NULL, arg, &ind, matches,
                                   ARRAY_SIZE(matches), &num_matches);

  if (match_result == M_PRE_EMPTY) {
    show_help_intro(caller, CMD_HELP);
    return FALSE;
  }
  if (match_result == M_PRE_AMBIGUOUS) {
    cmd_reply(CMD_HELP, caller, C_FAIL,
              _("Help argument '%s' is ambiguous."), arg);
    cmd_reply_matches(CMD_HELP, caller, helparg_accessor,
                      matches, num_matches);
    return FALSE;
  }
  if (match_result == M_PRE_FAIL) {
    cmd_reply(CMD_HELP, caller, C_FAIL,
              _("No match for help argument '%s'."), arg);
    return FALSE;
  }

  /* other cases should be above */
  fc_assert_ret_val(match_result < M_PRE_AMBIGUOUS, FALSE);
  
  if (ind < CMD_NUM) {
    show_help_command(caller, CMD_HELP, ind);
    return TRUE;
  }
  ind -= CMD_NUM;
  
  if (ind == HELP_GENERAL_OPTIONS) {
    show_help_option_list(caller, CMD_HELP);
    return TRUE;
  }
  if (ind == HELP_GENERAL_COMMANDS) {
    show_help_command_list(caller, CMD_HELP);
    return TRUE;
  }
  ind -= HELP_GENERAL_COUNT;

  if (ind < settings_number()) {
    show_help_option(caller, CMD_HELP, ind);
    return TRUE;
  }
  
  /* should have finished by now */
  log_error("Bug in show_help!");
  return FALSE;
}

/**********************************************************************//**
  List connections; initially mainly for debugging
**************************************************************************/
static void show_connections(struct connection *caller)
{
  char buf[MAX_LEN_CONSOLE_LINE];

  cmd_reply(CMD_LIST, caller, C_COMMENT,
            _("List of connections to server:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);

  if (conn_list_size(game.all_connections) == 0) {
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("<no connections>"));
  } else {
    conn_list_iterate(game.all_connections, pconn) {
      sz_strlcpy(buf, conn_description(pconn));
      if (pconn->established) {
        cat_snprintf(buf, sizeof(buf), " command access level %s",
                     cmdlevel_name(pconn->access_level));
      }
      cmd_reply(CMD_LIST, caller, C_COMMENT, "%s", buf);
    } conn_list_iterate_end;
  }
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

/**********************************************************************//**
  List all delegations of the current game.
**************************************************************************/
static void show_delegations(struct connection *caller)
{
  bool empty = TRUE;

  cmd_reply(CMD_LIST, caller, C_COMMENT, _("List of all delegations:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);

  players_iterate(pplayer) {
    const char *delegate_to = player_delegation_get(pplayer);
    if (delegate_to != NULL) {
      const char *owner =
        player_delegation_active(pplayer) ? pplayer->server.orig_username
                                          : pplayer->username;
      fc_assert(owner);
      cmd_reply(CMD_LIST, caller, C_COMMENT,
                /* TRANS: last %s is either " (active)" or empty string */
                _("%s delegates control over player '%s' to user %s%s."),
                owner, player_name(pplayer), delegate_to,
                player_delegation_active(pplayer) ? _(" (active)") : "");
      empty = FALSE;
    }
  } players_iterate_end;

  if (empty) {
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("No delegations defined."));
  }

  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

/**********************************************************************//**
  Show the ignore list of the 
**************************************************************************/
static bool show_ignore(struct connection *caller)
{
  char buf[128];
  int n = 1;

  if (NULL == caller) {
    cmd_reply(CMD_IGNORE, caller, C_FAIL,
              _("That would be rather silly, since you are not a player."));
    return FALSE;
  }

  if (0 == conn_pattern_list_size(caller->server.ignore_list)) {
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("Your ignore list is empty."));
    return TRUE;
  }

  cmd_reply(CMD_LIST, caller, C_COMMENT, _("Your ignore list:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  conn_pattern_list_iterate(caller->server.ignore_list, ppattern) {
    conn_pattern_to_string(ppattern, buf, sizeof(buf));
    cmd_reply(CMD_LIST, caller, C_COMMENT, "%d: %s", n++, buf);
  } conn_pattern_list_iterate_end;
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);

  return TRUE;
}

/**********************************************************************//**
  Show the list of the players of the game.
**************************************************************************/
void show_players(struct connection *caller)
{
  cmd_reply(CMD_LIST, caller, C_COMMENT, _("List of players:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);

  if (player_count() == 0) {
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("<no players>"));
  } else {
    players_iterate(pplayer) {
      char buf[MAX_LEN_CONSOLE_LINE];
      int n;

      /* Low access level callers don't get to see barbarians in list: */
      if (is_barbarian(pplayer) && caller
          && (caller->access_level < ALLOW_CTRL)) {
        continue;
      }

      /* The output for each player looks like:
       *
       * <Player name> [color]: Team[, Nation][, Username][, Status]
       *   AI/Barbarian/Human[, AI type, skill level][, Connections]
       *     [Details for each connection]
       */

      /* '<Player name> [color]: [Nation][, Username][, Status]' */
      buf[0] = '\0';
      cat_snprintf(buf, sizeof(buf), "%s [%s]: %s", player_name(pplayer),
                   player_color_ftstr(pplayer),
                   team_name_translation(pplayer->team));
      if (!game.info.is_new_game) {
        cat_snprintf(buf, sizeof(buf), ", %s",
                     nation_adjective_for_player(pplayer));
      }
      if (strlen(pplayer->username) > 0
          && strcmp(pplayer->username, "nouser") != 0) {
        cat_snprintf(buf, sizeof(buf), _(", user %s"), pplayer->username);
      }
      if (S_S_INITIAL == server_state() && pplayer->is_connected) {
        if (pplayer->is_ready) {
          sz_strlcat(buf, _(", ready"));
        } else {
          /* Emphasizes this */
          n = strlen(buf);
          featured_text_apply_tag(_(", not ready"),
                                  buf + n, sizeof(buf) - n,
                                  TTT_COLOR, 1, FT_OFFSET_UNSET,
                                  ftc_changed);
        }
      } else if (!pplayer->is_alive) {
        sz_strlcat(buf, _(", Dead"));
      }
      cmd_reply(CMD_LIST, caller, C_COMMENT, "%s", buf);

      /* '  AI/Barbarian/Human[, skill level][, Connections]' */
      buf[0] = '\0';
      if (is_barbarian(pplayer)) {
        sz_strlcat(buf, _("Barbarian"));
      } else if (is_ai(pplayer)) {
        sz_strlcat(buf, _("AI"));
      } else {
        sz_strlcat(buf, _("Human"));
      }
      if (is_ai(pplayer)) {
        cat_snprintf(buf, sizeof(buf), _(", %s"), ai_name(pplayer->ai));
        cat_snprintf(buf, sizeof(buf), _(", difficulty level %s"),
                     ai_level_translated_name(pplayer->ai_common.skill_level));
      }
      n = conn_list_size(pplayer->connections);
      if (n > 0) {
        cat_snprintf(buf, sizeof(buf), 
                     PL_(", %d connection:", ", %d connections:", n), n);
      }
      cmd_reply(CMD_LIST, caller, C_COMMENT, "  %s", buf);

      /* '    [Details for each connection]' */
      conn_list_iterate(pplayer->connections, pconn) {
        fc_snprintf(buf, sizeof(buf),
                    _("%s from %s (command access level %s), "
                      "bufsize=%dkb"), pconn->username, pconn->addr,
                    cmdlevel_name(pconn->access_level),
                    (pconn->send_buffer->nsize >> 10));
        if (pconn->observer) {
          sz_strlcat(buf, _(" (observer mode)"));
        }
        cmd_reply(CMD_LIST, caller, C_COMMENT, "    %s", buf);
      } conn_list_iterate_end;
    } players_iterate_end;
  }
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

/**********************************************************************//**
  List scenarios. We look both in the DATA_PATH and DATA_PATH/scenario
**************************************************************************/
static void show_scenarios(struct connection *caller)
{
  char buf[MAX_LEN_CONSOLE_LINE];
  struct fileinfo_list *files;

  cmd_reply(CMD_LIST, caller, C_COMMENT, _("List of scenarios available:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);

  files = fileinfolist_infix(get_scenario_dirs(), ".sav", TRUE);
  
  fileinfo_list_iterate(files, pfile) {
    struct section_file *sf = secfile_load_section(pfile->fullname, "scenario", TRUE);

    if (secfile_lookup_bool_default(sf, TRUE, "scenario.is_scenario")) {
        fc_snprintf(buf, sizeof(buf), "%s", pfile->name);
        cmd_reply(CMD_LIST, caller, C_COMMENT, "%s", buf);
    }
  } fileinfo_list_iterate_end;
  fileinfo_list_destroy(files);

  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

/**********************************************************************//**
  List nation sets in the current ruleset.
**************************************************************************/
static void show_nationsets(struct connection *caller)
{
  cmd_reply(CMD_LIST, caller, C_COMMENT,
            /* TRANS: don't translate text between '' */
            _("List of nation sets available for 'nationset' option:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);

  nation_sets_iterate(pset) {
    const char *description = nation_set_description(pset);
    int num_nations = 0;
    nations_iterate(pnation) {
      if (is_nation_playable(pnation) && nation_is_in_set(pnation, pset)) {
        num_nations++;
      }
    } nations_iterate_end;
    cmd_reply(CMD_LIST, caller, C_COMMENT,
              /* TRANS: nation set description; %d refers to number of playable
               * nations in set */
              PL_(" %-10s  %s (%d playable)",
                  " %-10s  %s (%d playable)", num_nations),
              nation_set_rule_name(pset), nation_set_name_translation(pset),
              num_nations);
    if (strlen(description) > 0) {
      static const char prefix[] = "   ";
      char *translated = fc_strdup(_(description));
      fc_break_lines(translated, LINE_BREAK);
      cmd_reply_prefix(CMD_LIST, caller, C_COMMENT, prefix, "%s%s",
                       prefix, translated);
    }
  } nation_sets_iterate_end;

  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

/**********************************************************************//**
  Show a list of teams on the command line.
**************************************************************************/
static void show_teams(struct connection *caller)
{
  /* Currently this just lists all teams (typically 32 of them) with their
   * names and # of players on the team.  This could probably be improved. */
  cmd_reply(CMD_LIST, caller, C_COMMENT, _("List of teams:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);

  teams_iterate(pteam) {
    const struct player_list *members = team_members(pteam);

    /* PL_() is needed here because some languages may differentiate
     * between 2 and 3 (although English does not). */
    cmd_reply(CMD_LIST, caller, C_COMMENT,
              /* TRANS: There will always be at least 2 players here. */
              PL_("%2d : '%s' : %d player :",
                  "%2d : '%s' : %d players :",
                  player_list_size(members)),
              team_index(pteam), team_name_translation(pteam),
              player_list_size(members));
    player_list_iterate(members, pplayer) {
      cmd_reply(CMD_LIST, caller, C_COMMENT, " %s", player_name(pplayer));
    } player_list_iterate_end;
  } teams_iterate_end;

  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

/**********************************************************************//**
  Show a list of all map image definitions on the command line.
**************************************************************************/
static void show_mapimg(struct connection *caller, enum command_id cmd)
{
  int id;

  if (mapimg_count() == 0) {
    cmd_reply(cmd, caller, C_OK, _("No map image definitions."));
  } else {
    cmd_reply(cmd, caller, C_COMMENT, _("List of map image definitions:"));
    cmd_reply(cmd, caller, C_COMMENT, horiz_line);
    for (id = 0; id < mapimg_count(); id++) {
      char str[MAX_LEN_MAPDEF] = "";
      mapimg_show(id, str, sizeof(str), FALSE);
      cmd_reply(cmd, caller, C_COMMENT, _("[%2d] %s"), id, str);
    }
    cmd_reply(cmd, caller, C_COMMENT, horiz_line);
  }
}

/**********************************************************************//**
  Show a list of all players with the assigned color.
**************************************************************************/
static void show_colors(struct connection *caller)
{
  cmd_reply(CMD_LIST, caller, C_COMMENT, _("List of player colors:"));
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
  if (player_count() == 0) {
    cmd_reply(CMD_LIST, caller, C_COMMENT, _("<no players>"));
  } else {
    players_iterate(pplayer) {
      cmd_reply(CMD_LIST, caller, C_COMMENT, _("%s (user %s): [%s]"),
                player_name(pplayer), pplayer->username,
                player_color_ftstr(pplayer));
    } players_iterate_end;
  }
  cmd_reply(CMD_LIST, caller, C_COMMENT, horiz_line);
}

/**************************************************************************
  '/list' arguments
**************************************************************************/
#define SPECENUM_NAME list_args
#define SPECENUM_VALUE0     LIST_COLORS
#define SPECENUM_VALUE0NAME "colors"
#define SPECENUM_VALUE1     LIST_CONNECTIONS
#define SPECENUM_VALUE1NAME "connections"
#define SPECENUM_VALUE2     LIST_DELEGATIONS
#define SPECENUM_VALUE2NAME "delegations"
#define SPECENUM_VALUE3     LIST_IGNORE
#define SPECENUM_VALUE3NAME "ignored users"
#define SPECENUM_VALUE4     LIST_MAPIMG
#define SPECENUM_VALUE4NAME "map image definitions"
#define SPECENUM_VALUE5     LIST_PLAYERS
#define SPECENUM_VALUE5NAME "players"
#define SPECENUM_VALUE6     LIST_SCENARIOS
#define SPECENUM_VALUE6NAME "scenarios"
#define SPECENUM_VALUE7     LIST_NATIONSETS
#define SPECENUM_VALUE7NAME "nationsets"
#define SPECENUM_VALUE8     LIST_TEAMS
#define SPECENUM_VALUE8NAME "teams"
#define SPECENUM_VALUE9     LIST_VOTES
#define SPECENUM_VALUE9NAME "votes"
#include "specenum_gen.h"

/**********************************************************************//**
  Returns possible parameters for the list command.
**************************************************************************/
static const char *list_accessor(int i)
{
  i = CLIP(0, i, list_args_max());
  return list_args_name((enum list_args) i);
}

/**********************************************************************//**
  Show list of players or connections, or connection statistics.
**************************************************************************/
static bool show_list(struct connection *caller, char *arg)
{
  enum m_pre_result match_result;
  int ind_int;
  enum list_args ind;

  remove_leading_trailing_spaces(arg);
  match_result = match_prefix(list_accessor, list_args_max() + 1, 0,
                              fc_strncasecmp, NULL, arg, &ind_int);
  ind = ind_int;

  if (match_result > M_PRE_EMPTY) {
    cmd_reply(CMD_LIST, caller, C_SYNTAX,
	      _("Bad list argument: '%s'.  Try '%shelp list'."),
	      arg, (caller?"/":""));
    return FALSE;
  }

  if (match_result == M_PRE_EMPTY) {
    ind = LIST_PLAYERS;
  }

  switch(ind) {
  case LIST_COLORS:
    show_colors(caller);
    return TRUE;
  case LIST_CONNECTIONS:
    show_connections(caller);
    return TRUE;
  case LIST_DELEGATIONS:
    show_delegations(caller);
    return TRUE;
  case LIST_IGNORE:
    return show_ignore(caller);
  case LIST_MAPIMG:
    show_mapimg(caller, CMD_LIST);
    return TRUE;
  case LIST_PLAYERS:
    show_players(caller);
    return TRUE;
  case LIST_SCENARIOS:
    show_scenarios(caller);
    return TRUE;
  case LIST_NATIONSETS:
    show_nationsets(caller);
    return TRUE;
  case LIST_TEAMS:
    show_teams(caller);
    return TRUE;
  case LIST_VOTES:
    show_votes(caller);
    return TRUE;
  }

  cmd_reply(CMD_LIST, caller, C_FAIL,
            "Internal error: ind %d in show_list", ind);
  log_error("Internal error: ind %d in show_list", ind);
  return FALSE;
}

#ifdef FREECIV_HAVE_LIBREADLINE
/********************* RL completion functions ***************************/
/* To properly complete both commands, player names, options and filenames
   there is one array per type of completion with the commands that
   the type is relevant for.
*/

/**********************************************************************//**
  A generalised generator function: text and state are "standard"
  parameters to a readline generator function;
  num is number of possible completions, or -1 if this is not known and 
  index2str should be iterated until it returns NULL;
  index2str is a function which returns each possible completion string
  by index (it may return NULL).
**************************************************************************/
static char *generic_generator(const char *text, int state, int num,
			       const char*(*index2str)(int))
{
  static int list_index, len;
  const char *name = ""; /* dummy non-NULL string */
  char *mytext = local_to_internal_string_malloc(text);

  /* This function takes a string (text) in the local format and must return
   * a string in the local format.  However comparisons are done against
   * names that are in the internal format (UTF-8).  Thus we have to convert
   * the text function from the local to the internal format before doing
   * the comparison, and convert the string we return *back* to the
   * local format when returning it. */

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (state == 0) {
    list_index = 0;
    len = strlen(mytext);
  }

  /* Return the next name which partially matches: */
  while ((num < 0 && name) || (list_index < num)) {
    name = index2str(list_index);
    list_index++;

    if (name != NULL && fc_strncasecmp(name, mytext, len) == 0) {
      free(mytext);
      return internal_to_local_string_malloc(name);
    }
  }
  free(mytext);

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}

/**********************************************************************//**
The valid commands at the root of the prompt.
**************************************************************************/
static char *command_generator(const char *text, int state)
{
  return generic_generator(text, state, CMD_NUM, command_name_by_number);
}

/**********************************************************************//**
The valid arguments to "set" and "explain"
**************************************************************************/
static char *option_generator(const char *text, int state)
{
  return generic_generator(text, state, settings_number(), optname_accessor);
}

/**********************************************************************//**
  The valid arguments to "show"
**************************************************************************/
static char *olevel_generator(const char *text, int state)
{
  return generic_generator(text, state, settings_number() + OLEVELS_NUM + 1,
                           olvlname_accessor);
}

/**********************************************************************//**
  Accessor for values of the enum/bitwise option defined by
  'completion_option'.
**************************************************************************/
static int completion_option;
static const char *option_value_accessor(int idx) {
  const struct setting *pset = setting_by_number(completion_option);
  switch (setting_type(pset)) {
  case SST_ENUM:
    return setting_enum_val(pset, idx, FALSE);
    break;
  case SST_BITWISE:
    return setting_bitwise_bit(pset, idx, FALSE);
    break;
  default:
    fc_assert_ret_val(0, NULL);
  }
}

/**********************************************************************//**
  The valid arguments to "set OPT", where OPT is the enumerated or
  bitwise option previously defined by completion_option
**************************************************************************/
static char *option_value_generator(const char *text, int state)
{
  return generic_generator(text, state, -1, option_value_accessor);
}

/**********************************************************************//**
  Access player name.
**************************************************************************/
static const char *playername_accessor(int idx)
{
  const struct player_slot *pslot = player_slot_by_number(idx);

  if (!player_slot_is_used(pslot)) {
    return NULL;
  }

  return player_name(player_slot_get_player(pslot));
}

/**********************************************************************//**
  The valid playername arguments.
**************************************************************************/
static char *player_generator(const char *text, int state)
{
  return generic_generator(text, state, player_slot_count(),
                           playername_accessor);
}

/**********************************************************************//**
  Access connection user name, from game.all_connections.
**************************************************************************/
static const char *connection_name_accessor(int idx)
{
  return conn_list_get(game.all_connections, idx)->username;
}

/**********************************************************************//**
  The valid connection user name arguments.
**************************************************************************/
static char *connection_generator(const char *text, int state)
{
  return generic_generator(text, state, conn_list_size(game.all_connections),
			   connection_name_accessor);
}

/**********************************************************************//**
  Extra accessor function since cmdlevel_name() takes enum argument, not int.
**************************************************************************/
static const char *cmdlevel_arg1_accessor(int idx)
{
  return cmdlevel_name(idx);
}

/**********************************************************************//**
  The valid first argument to "cmdlevel"
**************************************************************************/
static char *cmdlevel_arg1_generator(const char *text, int state)
{
  return generic_generator(text, state, cmdlevel_max()+1,
                           cmdlevel_arg1_accessor);
}

/**********************************************************************//**
  Accessor for the second argument to "cmdlevel": "first" or "new" or
  a connection name.
**************************************************************************/
static const char *cmdlevel_arg2_accessor(int idx)
{
  return ((idx == 0) ? "first" :
          (idx == 1) ? "new" :
          connection_name_accessor(idx - 2));
}

/**********************************************************************//**
  The valid arguments for the second argument to "cmdlevel".
**************************************************************************/
static char *cmdlevel_arg2_generator(const char *text, int state)
{
  return generic_generator(text, state,
                           /* "first", "new", connection names */
			   2 + conn_list_size(game.all_connections),
			   cmdlevel_arg2_accessor);
}

/**********************************************************************//**
  Accessor for the second argument to "create": ai type name
**************************************************************************/
static const char *aitype_accessor(int idx)
{
  return get_ai_type(idx)->name;
}

/**********************************************************************//**
  The valid arguments for the second argument to "create".
**************************************************************************/
static char *aitype_generator(const char *text, int state)
{
  return generic_generator(text, state, ai_type_get_count(),
                           aitype_accessor);
}

/**********************************************************************//**
  The valid arguments for the argument to "reset".
**************************************************************************/
static char *reset_generator(const char *text, int state)
{
  return generic_generator(text, state, reset_args_max() + 1, reset_accessor);
}

/**********************************************************************//**
  The valid arguments for the argument to "vote".
**************************************************************************/
static char *vote_generator(const char *text, int state)
{
  return generic_generator(text, state, -1, vote_arg_accessor);
}

/**********************************************************************//**
  The valid arguments for the first argument to "delegate".
**************************************************************************/
static char *delegate_generator(const char *text, int state)
{
  return generic_generator(text, state, delegate_args_max() + 1,
                           delegate_accessor);
}

/**********************************************************************//**
  The valid arguments for the first argument to "mapimg".
**************************************************************************/
static char *mapimg_generator(const char *text, int state)
{
  return generic_generator(text, state, mapimg_args_max() + 1,
                           mapimg_accessor);
}

/**********************************************************************//**
  The valid arguments for the argument to "fcdb".
**************************************************************************/
static char *fcdb_generator(const char *text, int state)
{
  return generic_generator(text, state, FCDB_COUNT, fcdb_accessor);
}

/**********************************************************************//**
  The valid arguments for the argument to "lua".
**************************************************************************/
static char *lua_generator(const char *text, int state)
{
  return generic_generator(text, state, lua_args_max() + 1, lua_accessor);
}

/**********************************************************************//**
  The valid first arguments to "help".
**************************************************************************/
static char *help_generator(const char *text, int state)
{
  return generic_generator(text, state, HELP_ARG_NUM, helparg_accessor);
}

/**********************************************************************//**
  The valid first arguments to "list".
**************************************************************************/
static char *list_generator(const char *text, int state)
{
  return generic_generator(text, state, list_args_max() + 1, list_accessor);
}

/**********************************************************************//**
  Generalised version of contains_str_before_start, which searches the
  N'th token in rl_line_buffer (0=first).
**************************************************************************/
static bool contains_token_before_start(int start, int token, const char *arg,
                                        bool allow_fluff)
{
  char *str_itr = rl_line_buffer;
  int arg_len = strlen(arg);

  /* Swallow unwanted tokens and their preceding delimiters */
  while (token--) {
    while (str_itr < rl_line_buffer + start && !fc_isalnum(*str_itr)) {
      str_itr++;
    }
    while (str_itr < rl_line_buffer + start && fc_isalnum(*str_itr)) {
      str_itr++;
    }
  }

  /* Swallow any delimiters before the token we're interested in */
  while (str_itr < rl_line_buffer + start && !fc_isalnum(*str_itr)) {
    str_itr++;
  }

  if (fc_strncasecmp(str_itr, arg, arg_len) != 0) {
    return FALSE;
  }
  str_itr += arg_len;

  if (fc_isalnum(*str_itr)) {
    /* Not a distinct word. */
    return FALSE;
  }

  if (!allow_fluff) {
    for (; str_itr < rl_line_buffer + start; str_itr++) {
      if (fc_isalnum(*str_itr)) {
        return FALSE;
      }
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Returns whether the text between the start of rl_line_buffer and the
  start position is of the form [non-alpha]*cmd[non-alpha]*
  allow_fluff changes the regexp to [non-alpha]*cmd[non-alpha].*
**************************************************************************/
static bool contains_str_before_start(int start, const char *cmd,
                                      bool allow_fluff)
{
  return contains_token_before_start(start, 0, cmd, allow_fluff);
}

/**********************************************************************//**
  Return whether we are completing command name. This can be either
  command itself, or argument to 'help'.
**************************************************************************/
static bool is_command(int start)
{
  char *str_itr;

  if (contains_str_before_start(start, command_name_by_number(CMD_HELP), FALSE))
    return TRUE;

  /* if there is only it is also OK */
  str_itr = rl_line_buffer;
  while (str_itr - rl_line_buffer < start) {
    if (fc_isalnum(*str_itr)) {
      return FALSE;
    }
    str_itr++;
  }
  return TRUE;
}

/**********************************************************************//**
  Number of tokens in rl_line_buffer before start
**************************************************************************/
static int num_tokens(int start)
{
  int res = 0;
  bool alnum = FALSE;
  char *chptr = rl_line_buffer;

  while (chptr - rl_line_buffer < start) {
    if (fc_isalnum(*chptr)) {
      if (!alnum) {
        alnum = TRUE;
        res++;
      }
    } else {
      alnum = FALSE;
    }
    chptr++;
  }

  return res;
}

/**************************************************************************
  Commands that may be followed by a player name
**************************************************************************/
static const int player_cmd[] = {
  CMD_AITOGGLE,
  CMD_HANDICAPPED,
  CMD_NOVICE,
  CMD_EASY,
  CMD_NORMAL,
  CMD_HARD,
  CMD_CHEATING,
#ifdef FREECIV_DEBUG
  CMD_EXPERIMENTAL,
#endif
  CMD_REMOVE,
  CMD_TEAM,
  CMD_PLAYERCOLOR,
  -1
};

/**********************************************************************//**
  Return whether we are completing player name argument.
**************************************************************************/
static bool is_player(int start)
{
  int i = 0;

  while (player_cmd[i] != -1) {
    if (contains_str_before_start(start, command_name_by_number(player_cmd[i]), FALSE)) {
      return TRUE;
    }
    i++;
  }

  return FALSE;
}

/**************************************************************************
  Commands that may be followed by a connection name
**************************************************************************/
static const int connection_cmd[] = {
  CMD_CUT,
  CMD_KICK,
  -1
};

/**********************************************************************//**
  Return whether we are completing connection name argument.
**************************************************************************/
static bool is_connection(int start)
{
  int i = 0;

  while (connection_cmd[i] != -1) {
    if (contains_str_before_start(start,
                                  command_name_by_number(connection_cmd[i]),
                                  FALSE)) {
      return TRUE;
    }
    i++;
  }

  return FALSE;
}

/**********************************************************************//**
  Return whether we are completing cmdlevel command argument 2.
**************************************************************************/
static bool is_cmdlevel_arg2(int start)
{
  return (contains_str_before_start(start, command_name_by_number(CMD_CMDLEVEL), TRUE)
	  && num_tokens(start) == 2);
}

/**********************************************************************//**
  Return whether we are completing cmdlevel command argument.
**************************************************************************/
static bool is_cmdlevel_arg1(int start)
{
  return contains_str_before_start(start, command_name_by_number(CMD_CMDLEVEL), FALSE);
}

/**************************************************************************
  Commands that may be followed by a server option name

  CMD_SHOW is handled by option_level_cmd, which is for both option levels
  and server options
**************************************************************************/
static const int server_option_cmd[] = {
  CMD_EXPLAIN,
  CMD_SET,
  CMD_DEFAULT,
  -1
};

/**********************************************************************//**
  Returns TRUE if the readline buffer string matches a server option at
  the given position.
**************************************************************************/
static bool is_server_option(int start)
{
  int i = 0;

  while (server_option_cmd[i] != -1) {
    if (contains_str_before_start(start, command_name_by_number(server_option_cmd[i]),
				  FALSE)) {
      return TRUE;
    }
    i++;
  }

  return FALSE;
}

/**************************************************************************
  Commands that may be followed by an option level or server option
**************************************************************************/
static const int option_level_cmd[] = {
  CMD_SHOW,
  -1
};

/**********************************************************************//**
  Returns true if the readline buffer string matches an option level or an
  option at the given position.
**************************************************************************/
static bool is_option_level(int start)
{
  int i = 0;

  while (option_level_cmd[i] != -1) {
    if (contains_str_before_start(start, command_name_by_number(option_level_cmd[i]),
				  FALSE)) {
      return TRUE;
    }
    i++;
  }

  return FALSE;
}

/**********************************************************************//**
  Returns TRUE if the readline buffer string is such that we expect an
  enumerated value at the given position. The option for which values
  should be completed is written to opt_p.
**************************************************************************/
static bool is_enum_option_value(int start, int *opt_p)
{
  if (contains_str_before_start(start, command_name_by_number(CMD_SET),
                                TRUE)) {
    settings_iterate(SSET_ALL, pset) {
      if (setting_type(pset) != SST_ENUM
          && setting_type(pset) != SST_BITWISE) {
        continue;
      }
      /* Allow a single token for enum options, multiple for bitwise
       * (the separator | will separate tokens for these purposes) */
      if (contains_token_before_start(start, 1, setting_name(pset),
                                      setting_type(pset) == SST_BITWISE)) {
        *opt_p = setting_number(pset);
        /* Suppress appended space for bitwise options (user may want |) */
        rl_completion_suppress_append = (setting_type(pset) == SST_BITWISE);
        return TRUE;
      }
    } settings_iterate_end;
  }
  return FALSE;
}

/**************************************************************************
  Commands that may be followed by a filename
**************************************************************************/
static const int filename_cmd[] = {
  CMD_LOAD,
  CMD_SAVE,
  CMD_READ_SCRIPT,
  CMD_WRITE_SCRIPT,
  -1
};

/**********************************************************************//**
  Return whether we are completing filename.
**************************************************************************/
static bool is_filename(int start)
{
  int i = 0;

  while (filename_cmd[i] != -1) {
    if (contains_str_before_start(start, command_name_by_number(filename_cmd[i]), FALSE)) {
      return TRUE;
    }
    i++;
  }

  return FALSE;
}

/**********************************************************************//**
  Return whether we are completing second argument for create command
**************************************************************************/
static bool is_create_arg2(int start)
{
  return (contains_str_before_start(start, command_name_by_number(CMD_CREATE), TRUE)
	  && num_tokens(start) == 2);
}

/**********************************************************************//**
  Return whether we are completing argument for reset command
**************************************************************************/
static bool is_reset(int start)
{
  return contains_str_before_start(start,
                                   command_name_by_number(CMD_RESET),
                                   FALSE);
}

/**********************************************************************//**
  Return whether we are completing argument for vote command
**************************************************************************/
static bool is_vote(int start)
{
  return contains_str_before_start(start,
                                   command_name_by_number(CMD_VOTE),
                                   FALSE);
}

/**********************************************************************//**
  Return whether we are completing first argument for delegate command
**************************************************************************/
static bool is_delegate_arg1(int start)
{
  return contains_str_before_start(start,
                                   command_name_by_number(CMD_DELEGATE),
                                   FALSE);
}

/**********************************************************************//**
  Return whether we are completing first argument for mapimg command
**************************************************************************/
static bool is_mapimg(int start)
{
  return contains_str_before_start(start,
                                   command_name_by_number(CMD_MAPIMG),
                                   FALSE);
}

/**********************************************************************//**
  Return whether we are completing argument for fcdb command
**************************************************************************/
static bool is_fcdb(int start)
{
  return contains_str_before_start(start,
                                   command_name_by_number(CMD_FCDB),
                                   FALSE);
}

/**********************************************************************//**
  Return whether we are completing argument for lua command
**************************************************************************/
static bool is_lua(int start)
{
  return contains_str_before_start(start,
                                   command_name_by_number(CMD_LUA),
                                   FALSE);
}

/**********************************************************************//**
  Return whether we are completing help command argument.
**************************************************************************/
static bool is_help(int start)
{
  return contains_str_before_start(start, command_name_by_number(CMD_HELP), FALSE);
}

/**********************************************************************//**
  Return whether we are completing list command argument.
**************************************************************************/
static bool is_list(int start)
{
  return contains_str_before_start(start, command_name_by_number(CMD_LIST), FALSE);
}

/**********************************************************************//**
  Attempt to complete on the contents of TEXT.  START and END bound the
  region of rl_line_buffer that contains the word to complete.  TEXT is
  the word to complete.  We can use the entire contents of rl_line_buffer
  in case we want to do some simple parsing.  Return the array of matches,
  or NULL if there aren't any.
**************************************************************************/
char **freeciv_completion(const char *text, int start, int end)
{
  char **matches = (char **)NULL;

  if (is_help(start)) {
    matches = rl_completion_matches(text, help_generator);
  } else if (is_command(start)) {
    matches = rl_completion_matches(text, command_generator);
  } else if (is_list(start)) {
    matches = rl_completion_matches(text, list_generator);
  } else if (is_cmdlevel_arg2(start)) {
    matches = rl_completion_matches(text, cmdlevel_arg2_generator);
  } else if (is_cmdlevel_arg1(start)) {
    matches = rl_completion_matches(text, cmdlevel_arg1_generator);
  } else if (is_connection(start)) {
    matches = rl_completion_matches(text, connection_generator);
  } else if (is_player(start)) {
    matches = rl_completion_matches(text, player_generator);
  } else if (is_server_option(start)) {
    matches = rl_completion_matches(text, option_generator);
  } else if (is_option_level(start)) {
    matches = rl_completion_matches(text, olevel_generator);
  } else if (is_enum_option_value(start, &completion_option)) {
    matches = rl_completion_matches(text, option_value_generator);
  } else if (is_filename(start)) {
    /* This function we get from readline */
    matches = rl_completion_matches(text, rl_filename_completion_function);
  } else if (is_create_arg2(start)) {
    matches = rl_completion_matches(text, aitype_generator);
  } else if (is_reset(start)) {
    matches = rl_completion_matches(text, reset_generator);
  } else if (is_vote(start)) {
    matches = rl_completion_matches(text, vote_generator);
  } else if (is_delegate_arg1(start)) {
    matches = rl_completion_matches(text, delegate_generator);
  } else if (is_mapimg(start)) {
    matches = rl_completion_matches(text, mapimg_generator);
  } else if (is_fcdb(start)) {
    matches = rl_completion_matches(text, fcdb_generator);
  } else if (is_lua(start)) {
    matches = rl_completion_matches(text, lua_generator);
  } else {
    /* We have no idea what to do */
    matches = NULL;
  }

  /* Don't automatically try to complete with filenames */
  rl_attempted_completion_over = 1;

  return (matches);
}

#endif /* FREECIV_HAVE_LIBREADLINE */
