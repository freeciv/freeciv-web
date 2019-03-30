/***********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Project
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

#include "fc_prehdrs.h"

#include <fcntl.h>
#include <stdio.h>
#include <signal.h>             /* SIGTERM and kill */
#include <string.h>
#include <time.h>

#ifdef FREECIV_MSWINDOWS
#include <windows.h>
#endif

#ifdef FREECIV_HAVE_SYS_TYPES_H
#include <sys/types.h>		/* fchmod */
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>		/* fchmod */
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

/* utility */
#include "astring.h"
#include "capability.h"
#include "deprecations.h"
#include "fciconv.h"
#include "fcintl.h"
#include "ioz.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "rand.h"
#include "registry.h"
#include "shared.h"
#include "support.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "clinet.h"		/* connect_to_server() */
#include "packhand_gen.h"

#include "chatline_common.h"
#include "connectdlg_g.h"
#include "connectdlg_common.h"
#include "tilespec.h"

#define WAIT_BETWEEN_TRIES 100000 /* usecs */ 
#define NUMBER_OF_TRIES 500

#if defined(HAVE_WORKING_FORK) && !defined(FREECIV_MSWINDOWS)
/* We are yet to see FREECIV_MSWINDOWS setup where even HAVE_WORKING_FORK would
 * mean fork() that actually works for us. */
#define HAVE_USABLE_FORK
#endif

#ifdef HAVE_USABLE_FORK
static pid_t server_pid = -1;
#elif FREECIV_MSWINDOWS
HANDLE server_process = INVALID_HANDLE_VALUE;
HANDLE loghandle = INVALID_HANDLE_VALUE;
#endif /* HAVE_USABLE_FORK || FREECIV_MSWINDOWS */
bool server_quitting = FALSE;

static char challenge_fullname[MAX_LEN_PATH];
static bool client_has_hack = FALSE;

int internal_server_port;

/************************************************************************** 
The general chain of events:

Two distinct paths are taken depending on the choice of mode: 

if the user selects the multi- player mode, then a packet_req_join_game
packet is sent to the server. It is either successful or not. The end.

If the user selects a single- player mode (either a new game or a save game)
then:
   1. the packet_req_join_game is sent.
   2. on receipt, if we can join, then a challenge packet is sent to the
      server, so we can get hack level control.
   3. if we can't get hack, then we get dumped to multi- player mode. If
      we can, then:
      a. for a new game, we send a series of packet_generic_message packets
         with commands to start the game.
      b. for a saved game, we send the load command with a
         packet_generic_message, then we send a PACKET_PLAYER_LIST_REQUEST.
         the response to this request will tell us if the game was loaded or
         not. if not, then we send another load command. if so, then we send
         a series of packet_generic_message packets with commands to start
         the game.
**************************************************************************/

/**********************************************************************//**
  Tests if the client has started the server.
**************************************************************************/
bool is_server_running(void)
{
  if (server_quitting) {
    return FALSE;
  }
#ifdef HAVE_USABLE_FORK
  return (server_pid > 0);
#elif FREECIV_MSWINDOWS
  return (server_process != INVALID_HANDLE_VALUE);
#else
  return FALSE; /* We've been unable to start one! */
#endif
}

/**********************************************************************//**
  Returns TRUE if the client has hack access.
**************************************************************************/
bool can_client_access_hack(void)
{
  return client_has_hack;
}

/**********************************************************************//**
  Kills the server if the client has started it.

  If the 'force' parameter is unset, we just do a /quit.  If it's set, then
  we'll send a signal to the server to kill it (use this when the socket
  is disconnected already).
**************************************************************************/
void client_kill_server(bool force)
{
#ifdef HAVE_USABLE_FORK
  if (server_quitting && server_pid > 0) {
    /* Already asked to /quit.
     * If it didn't do that, kill it. */
    if (waitpid(server_pid, NULL, WUNTRACED) <= 0) {
      kill(server_pid, SIGTERM);
      waitpid(server_pid, NULL, WUNTRACED);
    }
    server_pid = -1;
    server_quitting = FALSE;
  }
#elif FREECIV_MSWINDOWS
  if (server_quitting && server_process != INVALID_HANDLE_VALUE) {
    /* Already asked to /quit.
     * If it didn't do that, kill it. */
    TerminateProcess(server_process, 0);
    CloseHandle(server_process);
    if (loghandle != INVALID_HANDLE_VALUE) {
      CloseHandle(loghandle);
    }
    server_process = INVALID_HANDLE_VALUE;
    loghandle = INVALID_HANDLE_VALUE;
    server_quitting = FALSE;
  }
#endif /* FREECIV_MSWINDOWS || HAVE_USABLE_FORK */

  if (is_server_running()) {
    if (client.conn.used && client_has_hack) {
      /* This does a "soft" shutdown of the server by sending a /quit.
       *
       * This is useful when closing the client or disconnecting because it
       * doesn't kill the server prematurely.  In particular, killing the
       * server in the middle of a save can have disastrous results.  This
       * method tells the server to quit on its own.  This is safer from a
       * game perspective, but more dangerous because if the kill fails the
       * server will be left running.
       *
       * Another potential problem is because this function is called atexit
       * it could potentially be called when we're connected to an unowned
       * server.  In this case we don't want to kill it. */
      send_chat("/quit");
      server_quitting = TRUE;
    } else if (force) {
      /* Either we already disconnected, or we didn't get control of the
       * server. In either case, the only thing to do is a "hard" kill of
       * the server. */
#ifdef HAVE_USABLE_FORK
      kill(server_pid, SIGTERM);
      waitpid(server_pid, NULL, WUNTRACED);
      server_pid = -1;
#elif FREECIV_MSWINDOWS
      TerminateProcess(server_process, 0);
      CloseHandle(server_process);
      if (loghandle != INVALID_HANDLE_VALUE) {
	CloseHandle(loghandle);
      }
      server_process = INVALID_HANDLE_VALUE;
      loghandle = INVALID_HANDLE_VALUE;
#endif /* FREECIV_MSWINDOWS || HAVE_USABLE_FORK */
      server_quitting = FALSE;
    }
  }
  client_has_hack = FALSE;
}   

/**********************************************************************//**
  Forks a server if it can. Returns FALSE if we find we
  couldn't start the server.
**************************************************************************/
bool client_start_server(void)
{
#if !defined(HAVE_USABLE_FORK) && !defined(FREECIV_MSWINDOWS)
  /* Can't do much without fork */
  return FALSE;
#else /* HAVE_USABLE_FORK || FREECIV_MSWINDOWS */
  char buf[512];
  int connect_tries = 0;
  char savesdir[MAX_LEN_PATH];
  char scensdir[MAX_LEN_PATH];
  char *storage;
  char *ruleset;

#if !defined(HAVE_USABLE_FORK)
  /* Above also implies that this is FREECIV_MSWINDOWS ->
   * Win32 that can't use fork() */
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  char options[1024];
  char *depr;
  char rsparam[256];
#ifdef FREECIV_DEBUG
  char cmdline1[512];
#ifndef FREECIV_WEB
  char cmdline2[512];
#endif /* FREECIV_WEB */
#endif /* FREECIV_DEBUG */
  char cmdline3[512];
  char cmdline4[512];
  char logcmdline[512];
  char scriptcmdline[512];
  char savefilecmdline[512];
  char savescmdline[512];
  char scenscmdline[512];
#endif /* !HAVE_USABLE_FORK -> FREECIV_MSWINDOWS */

#ifdef FREECIV_IPV6_SUPPORT
  enum fc_addr_family family = FC_ADDR_ANY;
#else
  enum fc_addr_family family = FC_ADDR_IPV4;
#endif /* FREECIV_IPV6_SUPPORT */

  /* only one server (forked from this client) shall be running at a time */
  /* This also resets client_has_hack. */
  client_kill_server(TRUE);

  output_window_append(ftc_client, _("Starting local server..."));

  /* find a free port */
  /* Mitigate the risk of ending up with the port already
   * used by standalone server on Windows where this is known to be buggy
   * by not starting from DEFAULT_SOCK_PORT but from one higher. */
  internal_server_port = find_next_free_port(DEFAULT_SOCK_PORT + 1,
                                             DEFAULT_SOCK_PORT + 1 + 10000,
                                             family, "localhost", TRUE);

  if (internal_server_port < 0) {
    output_window_append(ftc_client, _("Couldn't start the server."));
    output_window_append(ftc_client,
                         _("You'll have to start one manually. Sorry..."));
    return FALSE;
  }

  storage = freeciv_storage_dir();
  if (storage == NULL) {
    output_window_append(ftc_client, _("Cannot find freeciv storage directory"));
    output_window_append(ftc_client,
                         _("You'll have to start server manually. Sorry..."));
    return FALSE;
  }

  ruleset = tileset_what_ruleset(tileset);

#ifdef HAVE_USABLE_FORK
  {
    int argc = 0;
    const int max_nargs = 25;
    char *argv[max_nargs + 1];
    char port_buf[32];
    char dbg_lvl_buf[32]; /* Do not move this inside the block where it gets filled,
                           * it's needed via the argv[x] pointer later on, so must
                           * remain in scope. */

    /* Set up the command-line parameters. */
    fc_snprintf(port_buf, sizeof(port_buf), "%d", internal_server_port);
    fc_snprintf(savesdir, sizeof(savesdir), "%s" DIR_SEPARATOR "saves", storage);
    fc_snprintf(scensdir, sizeof(scensdir), "%s" DIR_SEPARATOR "scenarios", storage);
#ifdef FREECIV_WEB
    argv[argc++] = "freeciv-web";
#else  /* FREECIV_WEB */
    argv[argc++] = "freeciv-server";
#endif /* FREECIV_WEB */
    argv[argc++] = "-p";
    argv[argc++] = port_buf;
    argv[argc++] = "--bind";
    argv[argc++] = "localhost";
    argv[argc++] = "-q";
    argv[argc++] = "1";
    argv[argc++] = "-e";
    argv[argc++] = "--saves";
    argv[argc++] = savesdir;
    argv[argc++] = "--scenarios";
    argv[argc++] = scensdir;
    argv[argc++] = "-A";
    argv[argc++] = "none";
    if (logfile) {
      enum log_level llvl = log_get_level();

      argv[argc++] = "--debug";
      fc_snprintf(dbg_lvl_buf, sizeof(dbg_lvl_buf), "%d", llvl);
      argv[argc++] = dbg_lvl_buf;
      argv[argc++] = "--log";
      argv[argc++] = logfile;
    }
    if (scriptfile) {
      argv[argc++] = "--read";
      argv[argc++] = scriptfile;
    }
    if (savefile) {
      argv[argc++] = "--file";
      argv[argc++] = savefile;
    }
    if (are_deprecation_warnings_enabled()) {
      argv[argc++] = "--warnings";
    }
    if (ruleset != NULL) {
      argv[argc++] = "--ruleset";
      argv[argc++] = ruleset;
    }
    argv[argc] = NULL;
    fc_assert(argc <= max_nargs);

    {
      struct astring srv_cmdline_opts = ASTRING_INIT;
      int i;

      for (i = 1; i < argc; i++) {
        astr_add(&srv_cmdline_opts, i == 1 ? "%s" : " %s", argv[i]);
      }
      log_verbose("Arguments to spawned server: %s",
                  astr_str(&srv_cmdline_opts));
      astr_free(&srv_cmdline_opts);
    }

    server_pid = fork();
    server_quitting = FALSE;

    if (server_pid == 0) {
      int fd;

      /* inside the child */

      /* avoid terminal spam, but still make server output available */ 
      fclose(stdout);
      fclose(stderr);

      /* FIXME: include the port to avoid duplication? */
      if (logfile) {
        fd = open(logfile, O_WRONLY | O_CREAT | O_APPEND, 0644);

        if (fd != 1) {
          dup2(fd, 1);
        }
        if (fd != 2) {
          dup2(fd, 2);
        }
        fchmod(1, 0644);
      }

      /* If it's still attatched to our terminal, things get messed up, 
        but freeciv-server needs *something* */ 
      fclose(stdin);
      fd = open("/dev/null", O_RDONLY);
      if (fd != 0) {
        dup2(fd, 0);
      }

      /* these won't return on success */
#ifdef FREECIV_DEBUG
      /* Search under current directory (what ever that happens to be)
       * only in debug builds. This allows running freeciv directly from build
       * tree, but could be considered security risk in release builds. */
#ifdef FREECIV_WEB
      execvp("./server/freeciv-web", argv);
#else  /* FREECIV_WEB */
      execvp("./fcser", argv);
      execvp("./server/freeciv-server", argv);
#endif /* FREECIV_WEB */
#endif /* FREECIV_DEBUG */
#ifdef FREECIV_WEB
      execvp(BINDIR "/freeciv-web", argv);
      execvp("freeciv-web", argv);
#else  /* FREECIV_WEB */
      execvp(BINDIR "/freeciv-server", argv);
      execvp("freeciv-server", argv);
#endif /* FREECIV_WEB */

      /* This line is only reached if freeciv-server cannot be started, 
       * so we kill the forked process.
       * Calling exit here is dangerous due to X11 problems (async replies) */ 
      _exit(1);
    } 
  }
#else /* HAVE_USABLE_FORK */
#ifdef FREECIV_MSWINDOWS
  if (logfile) {
    loghandle = CreateFile(logfile, GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
			   OPEN_ALWAYS, 0, NULL);
  }

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.hStdOutput = loghandle;
  si.hStdInput = INVALID_HANDLE_VALUE;
  si.hStdError = loghandle;
  si.dwFlags = STARTF_USESTDHANDLES;

  /* Set up the command-line parameters. */
  logcmdline[0] = 0;
  scriptcmdline[0] = 0;
  savefilecmdline[0] = 0;

  /* the server expects command line arguments to be in local encoding */ 
  if (logfile) {
    char *logfile_in_local_encoding =
        internal_to_local_string_malloc(logfile);
    enum log_level llvl = log_get_level();

    fc_snprintf(logcmdline, sizeof(logcmdline), " --debug %d --log %s", llvl,
                logfile_in_local_encoding);
    free(logfile_in_local_encoding);
  }
  if (scriptfile) {
    char *scriptfile_in_local_encoding =
        internal_to_local_string_malloc(scriptfile);

    fc_snprintf(scriptcmdline, sizeof(scriptcmdline),  " --read %s",
                scriptfile_in_local_encoding);
    free(scriptfile_in_local_encoding);
  }
  if (savefile) {
    char *savefile_in_local_encoding =
        internal_to_local_string_malloc(savefile);

    fc_snprintf(savefilecmdline, sizeof(savefilecmdline),  " --file %s",
                savefile_in_local_encoding);
    free(savefile_in_local_encoding);
  }

  fc_snprintf(savesdir, sizeof(savesdir), "%s" DIR_SEPARATOR "saves", storage);
  internal_to_local_string_buffer(savesdir, savescmdline, sizeof(savescmdline));

  fc_snprintf(scensdir, sizeof(scensdir), "%s" DIR_SEPARATOR "scenarios", storage);
  internal_to_local_string_buffer(scensdir, scenscmdline, sizeof(scenscmdline));

  if (are_deprecation_warnings_enabled()) {
    depr = " --warnings";
  } else {
    depr = "";
  }
  if (ruleset != NULL) {
    fc_snprintf(rsparam, sizeof(rsparam), " --ruleset %s", ruleset);
  } else {
    rsparam[0] = '\0';
  }

  fc_snprintf(options, sizeof(options),
              "-p %d --bind localhost -q 1 -e%s%s%s --saves \"%s\" "
              "--scenarios \"%s\" -A none %s%s",
              internal_server_port, logcmdline, scriptcmdline, savefilecmdline,
              savescmdline, scenscmdline, rsparam, depr);
#ifdef FREECIV_DEBUG
#ifdef FREECIV_WEB
  fc_snprintf(cmdline1, sizeof(cmdline1),
              "./server/freeciv-web %s", options);
#else  /* FREECIV_WEB */
  fc_snprintf(cmdline1, sizeof(cmdline1), "./fcser %s", options);
  fc_snprintf(cmdline2, sizeof(cmdline2),
              "./server/freeciv-server %s", options);
#endif /* FREECIV_WEB */
#endif /* FREECIV_DEBUG */
#ifdef FREECIV_WEB
  fc_snprintf(cmdline3, sizeof(cmdline2),
              BINDIR "/freeciv-web %s", options);
  fc_snprintf(cmdline4, sizeof(cmdline3),
              "freeciv-web %s", options);
#else  /* FREECIV_WEB */
  fc_snprintf(cmdline3, sizeof(cmdline3),
              BINDIR "/freeciv-server %s", options);
  fc_snprintf(cmdline4, sizeof(cmdline4),
              "freeciv-server %s", options);
#endif /* FREECIV_WEB */

  if (
#ifdef FREECIV_DEBUG
      !CreateProcess(NULL, cmdline1, NULL, NULL, TRUE,
                     DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
                     NULL, NULL, &si, &pi)
#ifndef FREECIV_WEB
      && !CreateProcess(NULL, cmdline2, NULL, NULL, TRUE,
                        DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
                        NULL, NULL, &si, &pi)
#endif /* FREECIV_WEB */
      &&
#endif /* FREECIV_DEBUG */
      !CreateProcess(NULL, cmdline3, NULL, NULL, TRUE,
                     DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
                     NULL, NULL, &si, &pi)
      && !CreateProcess(NULL, cmdline4, NULL, NULL, TRUE,
                        DETACHED_PROCESS | NORMAL_PRIORITY_CLASS,
                        NULL, NULL, &si, &pi)) {
    log_error("Failed to start server process.");
#ifdef FREECIV_DEBUG
    log_verbose("Tried with commandline: '%s'", cmdline1);
    log_verbose("Tried with commandline: '%s'", cmdline2);
#endif /* FREECIV_DEBUG */
    log_verbose("Tried with commandline: '%s'", cmdline3);
    log_verbose("Tried with commandline: '%s'", cmdline4);
    output_window_append(ftc_client, _("Couldn't start the server."));
    output_window_append(ftc_client,
                         _("You'll have to start one manually. Sorry..."));
    return FALSE;
  }

  log_verbose("Arguments to spawned server: %s", options);

  server_process = pi.hProcess;

#endif /* FREECIV_MSWINDOWS */
#endif /* HAVE_USABLE_FORK */

  /* a reasonable number of tries */ 
  while (connect_to_server(user_name, "localhost", internal_server_port, 
                           buf, sizeof(buf)) == -1) {
    fc_usleep(WAIT_BETWEEN_TRIES);
#ifdef HAVE_USABLE_FORK
#ifndef FREECIV_MSWINDOWS
    if (waitpid(server_pid, NULL, WNOHANG) != 0) {
      break;
    }
#endif /* FREECIV_MSWINDOWS */
#endif /* HAVE_USABLE_FORK */
    if (connect_tries++ > NUMBER_OF_TRIES) {
      log_error("Last error from connect attempts: '%s'", buf);
      break;
    }
  }

  /* weird, but could happen, if server doesn't support new startup stuff
   * capabilities won't help us here... */ 
  if (!client.conn.used) {
    /* possible that server is still running. kill it */ 
    client_kill_server(TRUE);

    log_error("Failed to connect to spawned server!");
    output_window_append(ftc_client, _("Couldn't connect to the server."));
    output_window_append(ftc_client,
                         _("We probably couldn't start it from here."));
    output_window_append(ftc_client,
                         _("You'll have to start one manually. Sorry..."));

    return FALSE;
  }

  /* We set the topology to match the view.
   *
   * When a typical player launches a game, he wants the map orientation to
   * match the tileset orientation.  So if you use an isometric tileset you
   * get an iso-map and for a classic tileset you get a classic map.  In
   * both cases the map wraps in the X direction by default.
   *
   * This works with hex maps too now.  A hex map always has
   * tileset_is_isometric(tileset) return TRUE.  An iso-hex map has
   * tileset_hex_height(tileset) != 0, while a non-iso hex map
   * has tileset_hex_width(tileset) != 0.
   *
   * Setting the option here is a bit of a hack, but so long as the client
   * has sufficient permissions to do so (it doesn't have HACK access yet) it
   * is safe enough.  Note that if you load a savegame the topology will be
   * set but then overwritten during the load.
   *
   * Don't send it now, it will be sent to the server when receiving the
   * server setting infos. */
  {
    char topobuf[16];

    fc_strlcpy(topobuf, "WRAPX", sizeof(topobuf));
    if (tileset_is_isometric(tileset) && 0 == tileset_hex_height(tileset)) {
      fc_strlcat(topobuf, "|ISO", sizeof(topobuf));
    }
    if (0 < tileset_hex_width(tileset) || 0 < tileset_hex_height(tileset)) {
      fc_strlcat(topobuf, "|HEX", sizeof(topobuf));
    }
    desired_settable_option_update("topology", topobuf, FALSE);
  }

  return TRUE;
#endif /* HAVE_USABLE_FORK || FREECIV_MSWINDOWS */
}

/**********************************************************************//**
  Generate a random string.
**************************************************************************/
static void randomize_string(char *str, size_t n)
{
  const char chars[] =
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i;

  for (i = 0; i < n - 1; i++) {
    str[i] = chars[fc_rand(sizeof(chars) - 1)];
  }
  str[i] = '\0';
}

/**********************************************************************//**
  If the client is capable of 'wanting hack', then the server will
  send the client a filename in the packet_join_game_reply packet.

  This function creates the file with a suitably random string in it
  and then sends the string to the server. If the server can open
  and read the string, then the client is given hack access.
**************************************************************************/
void send_client_wants_hack(const char *filename)
{
  if (filename[0] != '\0') {
    struct packet_single_want_hack_req req;
    struct section_file *file;
    const char *sdir = freeciv_storage_dir();

    if (sdir == NULL) {
      return;
    }

    if (!is_safe_filename(filename)) {
      return;
    }

    make_dir(sdir);

    fc_snprintf(challenge_fullname, sizeof(challenge_fullname),
                "%s" DIR_SEPARATOR "%s", sdir, filename);

    /* generate an authentication token */ 
    randomize_string(req.token, sizeof(req.token));

    file = secfile_new(FALSE);
    secfile_insert_str(file, req.token, "challenge.token");
    if (!secfile_save(file, challenge_fullname, 0, FZ_PLAIN)) {
      log_error("Couldn't write token to temporary file: %s",
                challenge_fullname);
    }
    secfile_destroy(file);

    /* tell the server what we put into the file */ 
    send_packet_single_want_hack_req(&client.conn, &req);
  }
}

/**********************************************************************//**
  Handle response (by the server) if the client has got hack or not.
**************************************************************************/
void handle_single_want_hack_reply(bool you_have_hack)
{
  /* remove challenge file */
  if (challenge_fullname[0] != '\0') {
    if (fc_remove(challenge_fullname) == -1) {
      log_error("Couldn't remove temporary file: %s", challenge_fullname);
    }
    challenge_fullname[0] = '\0';
  }

  if (you_have_hack) {
    output_window_append(ftc_client,
                         _("Established control over the server. "
                           "You have command access level 'hack'."));
    client_has_hack = TRUE;
  } else if (is_server_running()) {
    /* only output this if we started the server and we NEED hack */
    output_window_append(ftc_client,
                         _("Failed to obtain the required access "
                           "level to take control of the server. "
                           "Attempting to shut down server."));
    client_kill_server(TRUE);
  }
}

/**********************************************************************//**
  Send server command to save game.
**************************************************************************/
void send_save_game(const char *filename)
{   
  if (filename) {
    send_chat_printf("/save %s", filename);
  } else {
    send_chat("/save");
  }
}

/**********************************************************************//**
  Handle the list of rulesets sent by the server.
**************************************************************************/
void handle_ruleset_choices(const struct packet_ruleset_choices *packet)
{
  char *rulesets[packet->ruleset_count];
  int i;
  size_t suf_len = strlen(RULESET_SUFFIX);

  for (i = 0; i < packet->ruleset_count; i++) {
    size_t len = strlen(packet->rulesets[i]);

    rulesets[i] = fc_strdup(packet->rulesets[i]);

    if (len > suf_len
	&& strcmp(rulesets[i] + len - suf_len, RULESET_SUFFIX) == 0) {
      rulesets[i][len - suf_len] = '\0';
    }
  }
  set_rulesets(packet->ruleset_count, rulesets);

  for (i = 0; i < packet->ruleset_count; i++) {
    free(rulesets[i]);
  }
}

/**********************************************************************//**
  Called by the GUI code when the user sets the ruleset.  The ruleset
  passed in here should match one of the strings given to set_rulesets().
**************************************************************************/
void set_ruleset(const char *ruleset)
{
  char buf[4096];

  fc_snprintf(buf, sizeof(buf), "/read %s%s", ruleset, RULESET_SUFFIX);
  log_debug("Executing '%s'", buf);
  send_chat(buf);
}
