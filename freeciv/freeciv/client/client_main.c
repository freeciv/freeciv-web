/********************************************************************** 
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
#include <config.h>
#endif

#ifdef WIN32_NATIVE
#include <windows.h>	/* LoadLibrary() */
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* utility */
#include "capstr.h"
#include "dataio.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "support.h"
#include "timing.h"

/* common */
#include "ai.h"
#include "diptreaty.h"
#include "game.h"
#include "idex.h"
#include "map.h"
#include "netintf.h"
#include "packets.h"
#include "version.h"

/* include */
#include "chatline_g.h"
#include "citydlg_g.h"
#include "connectdlg_g.h"
#include "dialogs_g.h"
#include "diplodlg_g.h"
#include "editgui_g.h"
#include "graphics_g.h"
#include "gui_main_g.h"
#include "mapctrl_g.h"
#include "mapview_g.h"
#include "menu_g.h"
#include "messagewin_g.h"
#include "pages_g.h"
#include "plrdlg_g.h"
#include "repodlgs_g.h"

/* client */
#include "agents.h"
#include "attribute.h"
#include "audio.h"
#include "cityrepdata.h"
#include "climisc.h"
#include "clinet.h"
#include "cma_core.h"           /* kludge */
#include "connectdlg_common.h"  /* client_kill_server() */
#include "control.h" 
#include "editor.h"
#include "ggzclient.h"
#include "helpdata.h"           /* boot_help_texts() */
#include "mapview_common.h"
#include "options.h"
#include "overview_common.h"
#include "packhand.h"
#include "tilespec.h"
#include "themes_common.h"
#include "voteinfo.h"

#include "client_main.h"

char *logfile = NULL;
char *scriptfile = NULL;
static char tileset_name[512] = "\0";
char sound_plugin_name[512] = "\0";
char sound_set_name[512] = "\0";
char server_host[512] = "\0";
char user_name[512] = "\0";
char password[MAX_LEN_PASSWORD] = "\0";
char metaserver[512] = "\0";
int  server_port = -1;
bool auto_connect = FALSE; /* TRUE = skip "Connect to Freeciv Server" dialog */
bool in_ggz = FALSE;
enum announce_type announce;

struct civclient client;

static enum client_states civclient_state = C_S_INITIAL;

/* TRUE if an end turn request is blocked by busy agents */
bool waiting_for_end_turn = FALSE;

/* 
 * TRUE for the time between sending PACKET_TURN_DONE and receiving
 * PACKET_NEW_YEAR. 
 */
bool turn_done_sent = FALSE;

/*
 * TRUE between receiving PACKET_END_TURN and PACKET_BEGIN_TURN
 */
static bool server_busy = FALSE;

/**************************************************************************
  Convert a text string from the internal to the data encoding, when it
  is written to the network.
**************************************************************************/
static char *put_conv(const char *src, size_t *length)
{
  char *out = internal_to_data_string_malloc(src);

  if (out) {
    *length = strlen(out);
    return out;
  } else {
    *length = 0;
    return NULL;
  }
}

/**************************************************************************
  Convert a text string from the data to the internal encoding when it is
  first read from the network.  Returns FALSE if the destination isn't
  large enough or the source was bad.
**************************************************************************/
static bool get_conv(char *dst, size_t ndst,
		     const char *src, size_t nsrc)
{
  char *out = data_to_internal_string_malloc(src);
  bool ret = TRUE;
  size_t len;

  if (!out) {
    dst[0] = '\0';
    return FALSE;
  }

  len = strlen(out);
  if (ndst > 0 && len >= ndst) {
    ret = FALSE;
    len = ndst - 1;
  }

  memcpy(dst, out, len);
  dst[len] = '\0';
  free(out);

  return ret;
}

/**************************************************************************
  Set up charsets for the client.
**************************************************************************/
static void charsets_init(void)
{
  dio_set_put_conv_callback(put_conv);
  dio_set_get_conv_callback(get_conv);
}

/**************************************************************************
 This is called at program exit.
**************************************************************************/
static void at_exit(void)
{
  client_kill_server(TRUE);
  fc_shutdown_network();
}

/**************************************************************************
  Called only by set_client_state() below.
**************************************************************************/
static void client_game_init(void)
{
  client.conn.playing = NULL;
  client.conn.observer = FALSE;

  game_init();
  attribute_init();
  agents_init();
  control_init();
  link_marks_init();
  voteinfo_queue_init();
}

/**************************************************************************
  Called by set_client_state() and client_exit() below.
**************************************************************************/
static void client_game_free(void)
{
  packhand_free();
  voteinfo_queue_free();
  link_marks_free();
  control_done();
  free_help_texts();
  attribute_free();
  agents_free();
  game_free();

  client.conn.playing = NULL;
  client.conn.observer = FALSE;
}

/**************************************************************************
  Called only by set_client_state() below.  Just free what is needed to
  change view (player target).
**************************************************************************/
static void client_game_reset(void)
{
  packhand_free();
  link_marks_free();
  control_done();
  attribute_free();
  agents_free();

  game_reset();

  attribute_init();
  agents_init();
  control_init();
  link_marks_init();
}

/**************************************************************************
  Entry point for common client code.
**************************************************************************/
int client_main(int argc, char *argv[])
{
  int i, loglevel;
  int ui_options = 0;
  bool ui_separator = FALSE;
  char *option=NULL;
  bool user_tileset = FALSE;

  /* Load win32 post-crash debugger */
#ifdef WIN32_NATIVE
# ifndef NDEBUG
  if (LoadLibrary("exchndl.dll") == NULL) {
#  ifdef DEBUG
    fprintf(stderr, "exchndl.dll could not be loaded, no crash debugger\n");
#  endif
  }
# endif
#endif

  i_am_client(); /* Tell to libfreeciv that we are client */

  /* Ensure that all AIs are initialized to unused state */
  ai_type_iterate(ai) {
    init_ai(ai);
  } ai_type_iterate_end;

  init_nls();
  audio_init();
  init_character_encodings(gui_character_encoding, gui_use_transliteration);

  /* default argument values are set in options.c */
  loglevel=LOG_NORMAL;

  i = 1;

  announce = ANNOUNCE_DEFAULT;

  while (i < argc) {
    if (ui_separator) {
      argv[1 + ui_options] = argv[i];
      ui_options++;
    } else if (is_option("--help", argv[i])) {
      fc_fprintf(stderr, _("Usage: %s [option ...]\n"
			   "Valid options are:\n"), argv[0]);
      fc_fprintf(stderr, _("  -A, --Announce PROTO\tAnnounce game in LAN using protocol PROTO (IPv4/IPv6/none)\n"));
      fc_fprintf(stderr, _("  -a, --autoconnect\tSkip connect dialog\n"));
#ifdef DEBUG
      fc_fprintf(stderr, _("  -d, --debug NUM\tSet debug log level (0 to 4,"
			   " or 4:file1,min,max:...)\n"));
#else
      fc_fprintf(stderr,
		 _("  -d, --debug NUM\tSet debug log level (0 to 3)\n"));
#endif
      fc_fprintf(stderr,
		 _("  -h, --help\t\tPrint a summary of the options\n"));
      fc_fprintf(stderr, _("  -l, --log FILE\tUse FILE as logfile "
			   "(spawned server also uses this)\n"));
      fc_fprintf(stderr, _("  -M, --Meta HOST\t"
			   "Connect to the metaserver at HOST\n"));
      fc_fprintf(stderr, _("  -n, --name NAME\tUse NAME as name\n"));
      fc_fprintf(stderr,
		 _("  -p, --port PORT\tConnect to server port PORT (usually with -a)\n"));
      fc_fprintf(stderr,
		 _("  -P, --Plugin PLUGIN\tUse PLUGIN for sound output %s\n"),
		 audio_get_all_plugin_names());
      fc_fprintf(stderr, _("  -r, --read FILE\tRead startup script FILE "
			   "(for spawned server only)\n"));
      fc_fprintf(stderr,
		 _("  -s, --server HOST\tConnect to the server at HOST (usually with -a)\n"));
      fc_fprintf(stderr,
		 _("  -S, --Sound FILE\tRead sound tags from FILE\n"));
      fc_fprintf(stderr, _("  -t, --tiles FILE\t"
			   "Use data file FILE.tilespec for tiles\n"));
      fc_fprintf(stderr, _("  -v, --version\t\tPrint the version number\n"));
      fc_fprintf(stderr, _("      --\t\t"
			   "Pass any following options to the UI.\n"
			   "\t\t\tTry \"%s -- --help\" for more.\n"), argv[0]);
      fc_fprintf(stderr, "\n");
      /* TRANS: No full stop after the URL, could cause confusion. */
      fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);
      exit(EXIT_SUCCESS);
    } else if (is_option("--version",argv[i])) {
      fc_fprintf(stderr, "%s %s\n", freeciv_name_version(), client_string);
      exit(EXIT_SUCCESS);
    } else if ((option = get_option_malloc("--log", argv, &i, argc))) {
      logfile = option; /* never free()d */
    } else  if ((option = get_option_malloc("--read", argv, &i, argc))) {
      scriptfile = option; /* never free()d */
    } else if ((option = get_option_malloc("--name", argv, &i, argc))) {
      sz_strlcpy(user_name, option);
      free(option);
    } else if ((option = get_option_malloc("--Meta", argv, &i, argc))) {
      sz_strlcpy(metaserver, option);
      free(option);
    } else if ((option = get_option_malloc("--Sound", argv, &i, argc))) {
      sz_strlcpy(sound_set_name, option);
      free(option);
    } else if ((option = get_option_malloc("--Plugin", argv, &i, argc))) {
      sz_strlcpy(sound_plugin_name, option);
      free(option);
    } else if ((option = get_option_malloc("--port",argv,&i,argc))) {
      if (sscanf(option, "%d", &server_port) != 1) {
	fc_fprintf(stderr,
		   _("Invalid port \"%s\" specified with --port option.\n"),
		   option);
	fc_fprintf(stderr, _("Try using --help.\n"));
        exit(EXIT_FAILURE);
      }
      free(option);
    } else if ((option = get_option_malloc("--server", argv, &i, argc))) {
      sz_strlcpy(server_host, option);
      free(option);
    } else if (is_option("--autoconnect", argv[i])) {
      auto_connect = TRUE;
    } else if ((option = get_option_malloc("--debug", argv, &i, argc))) {
      loglevel = log_parse_level_str(option);
      if (loglevel == -1) {
	fc_fprintf(stderr,
		   _("Invalid debug level \"%s\" specified with --debug "
		     "option.\n"), option);
	fc_fprintf(stderr, _("Try using --help.\n"));
        exit(EXIT_FAILURE);
      }
      free(option);
    } else if ((option = get_option_malloc("--tiles", argv, &i, argc))) {
      sz_strlcpy(tileset_name, option);
      free(option);
      user_tileset = TRUE;
    } else if ((option = get_option_malloc("--Announce", argv, &i, argc))) {
      if (!strcasecmp(option, "ipv4")) {
        announce = ANNOUNCE_IPV4;
      } else if (!strcasecmp(option, "none")) {
        announce = ANNOUNCE_NONE;
#ifdef IPV6_SUPPORT
      } else if(!strcasecmp(option, "ipv6")) {
        announce = ANNOUNCE_IPV6;
#endif /* IPv6 support */
      } else {
        fc_fprintf(stderr, _("Invalid announce protocol \"%s\".\n"), option);
        exit(EXIT_FAILURE);
      }
      free(option);
    } else if (is_option("--", argv[i])) {
      ui_separator = TRUE;
    } else {
      fc_fprintf(stderr, _("Unrecognized option: \"%s\"\n"), argv[i]);
      exit(EXIT_FAILURE);
    }
    i++;
  } /* of while */

  /* Remove all options except those intended for the UI. */
  argv[1 + ui_options] = NULL;
  argc = 1 + ui_options;

  /* disallow running as root -- too dangerous */
  dont_run_as_root(argv[0], "freeciv_client");

  log_init(logfile, loglevel, NULL);

  /* after log_init: */

  (void)user_username(default_user_name, MAX_LEN_NAME);
  if (!is_valid_username(default_user_name)) {
    char buf[sizeof(default_user_name)];

    my_snprintf(buf, sizeof(buf), "_%s", default_user_name);
    if (is_valid_username(buf)) {
      sz_strlcpy(default_user_name, buf);
    } else {
      my_snprintf(default_user_name, sizeof(default_user_name),
		  "player%d", myrand(10000));
    }
  }

  /* initialization */

  game.all_connections = conn_list_new();
  game.est_connections = conn_list_new();

  ui_init();
  charsets_init();
  fc_init_network();

  /* register exit handler */ 
  atexit(at_exit);

  init_our_capability();
  chatline_common_init();
  message_options_init();
  init_player_dlg_common();
  init_themes();
  settable_options_init();

  gui_options_extra_init();
  load_general_options();

  if (tileset_name[0] == '\0') {
    sz_strlcpy(tileset_name, default_tileset_name);
  }
  if (sound_set_name[0] == '\0') 
    sz_strlcpy(sound_set_name, default_sound_set_name); 
  if (sound_plugin_name[0] == '\0')
    sz_strlcpy(sound_plugin_name, default_sound_plugin_name); 
  if (server_host[0] == '\0')
    sz_strlcpy(server_host, default_server_host); 
  if (user_name[0] == '\0')
    sz_strlcpy(user_name, default_user_name); 
  if (metaserver[0] == '\0') {
    /* FIXME: Find a cleaner way to achieve this. */
    const char *oldaddr = "http://www.cazfi.net/freeciv/metaserver/";
    if (0 == strcmp(default_metaserver, oldaddr)) {
      freelog(LOG_NORMAL, _("Updating old metaserver address \"%s\"."),
              oldaddr);
      sz_strlcpy(default_metaserver, META_URL);
      freelog(LOG_NORMAL, _("Default metaserver has been set to \"%s\"."),
              META_URL);
    }
    sz_strlcpy(metaserver, default_metaserver);
  }
  if (server_port == -1) server_port = default_server_port;

  /* This seed is not saved anywhere; randoms in the client should
     have cosmetic effects only (eg city name suggestions).  --dwp */
  mysrand(time(NULL));
  helpdata_init();
  boot_help_texts(NULL);

  tilespec_try_read(tileset_name, user_tileset);

  audio_real_init(sound_set_name, sound_plugin_name);
  audio_play_music("music_start", NULL);

  ggz_initialize();

  editor_init();

  /* run gui-specific client */
  ui_main(argc, argv);

  /* termination */
  client_exit();
  
  /* not reached */
  return EXIT_SUCCESS;
}

/**************************************************************************
...
**************************************************************************/
void client_exit(void)
{
  if (client_state() >= C_S_PREPARING) {
    attribute_flush();
    client_remove_all_cli_conn();
  }

  if (save_options_on_exit) {
    save_options();
  }
  
  tileset_free(tileset);
  
  ui_exit();
  
  chatline_common_done();
  message_options_free();
  if (client_state() >= C_S_PREPARING) {
    client_game_free();
  }

  helpdata_done(); /* client_exit() unlinks help text list */
  conn_list_free(game.all_connections);
  conn_list_free(game.est_connections);

  free_nls();

  exit(EXIT_SUCCESS);
}


/**************************************************************************
...
**************************************************************************/
void client_packet_input(void *packet, int type)
{
  if (!client_handle_packet(type, packet)) {
    freelog(LOG_ERROR, "Received unknown packet (type %d) from server!",
	    type);
  }
}

/**************************************************************************
...
**************************************************************************/
void user_ended_turn(void)
{
  send_turn_done();
}

/**************************************************************************
...
**************************************************************************/
void send_turn_done(void)
{
  freelog(LOG_DEBUG, "send_turn_done() turn_done_button_state=%d",
	  get_turn_done_button_state());

  if (!get_turn_done_button_state()) {
    /*
     * The turn done button is disabled but the user may have press
     * the return key.
     */

    if (agents_busy()) {
      waiting_for_end_turn = TRUE;
    }

    return;
  }

  waiting_for_end_turn = FALSE;
  turn_done_sent = TRUE;

  attribute_flush();

  dsend_packet_player_phase_done(&client.conn, game.info.turn);

  update_turn_done_button_state();
}

/**************************************************************************
...
**************************************************************************/
void send_report_request(enum report_type type)
{
  dsend_packet_report_req(&client.conn, type);
}

/**************************************************************************
  ...
**************************************************************************/
void set_client_state(enum client_states newstate)
{
  enum client_states oldstate = civclient_state;
  struct player *pplayer = client_player();

  if (auto_connect && newstate == C_S_DISCONNECTED) {
    if (oldstate == C_S_DISCONNECTED) {
      freelog(LOG_FATAL,
              _("There was an error while auto connecting; aborting."));
        exit(EXIT_FAILURE);
    } else {
      start_autoconnecting_to_server();
      auto_connect = FALSE;     /* Don't try this again. */
    }
  }

  if (C_S_PREPARING == newstate
      && (client_has_player() || client_is_observer())) {
    /* Reset the delta-state. */
    conn_clear_packet_cache(&client.conn);
  }

  if (oldstate == newstate) {
    return;
  }

  civclient_state = newstate;

  switch (newstate) {
  case C_S_INITIAL:
    die("%d is not a valid client state to set", C_S_INITIAL);
    break;

  case C_S_DISCONNECTED:
    popdown_all_city_dialogs();
    close_all_diplomacy_dialogs();
    popdown_all_game_dialogs();
    clear_notify_window();

    if (oldstate > C_S_DISCONNECTED) {
      set_unit_focus(NULL);
      agents_disconnect();
      client_remove_all_cli_conn();
      client_game_free();
    }

    if (!with_ggz) {
      set_client_page(in_ggz ? PAGE_GGZ : PAGE_MAIN);
    }
    break;

  case C_S_PREPARING:
    popdown_all_city_dialogs();
    close_all_diplomacy_dialogs();
    popdown_all_game_dialogs();
    clear_notify_window();

    if (oldstate < C_S_PREPARING) {
      client_game_init();
    } else {
      /* From an upper state means that we didn't quit the server,
       * so a lot of informations are still in effect. */
      client_game_reset();
    }

    set_unit_focus(NULL);

    if (get_client_page() != PAGE_SCENARIO
        && get_client_page() != PAGE_LOAD) {
      set_client_page(PAGE_START);
    }
    break;

  case C_S_RUNNING:
    if (oldstate == C_S_PREPARING) {
      popdown_races_dialog();
      audio_stop();     /* stop intro sound loop. */
    }

    init_city_report_game_data();
    load_ruleset_specific_options();
    create_event(NULL, E_GAME_START, FTC_CLIENT_INFO, NULL,
                 _("Game started."));
    precalc_tech_data();
    if (pplayer) {
      player_research_update(pplayer);
    }
    boot_help_texts(pplayer);   /* reboot with player */
    can_slide = FALSE;
    update_unit_focus();
    can_slide = TRUE;
    set_client_page(PAGE_GAME);
    /* Find something sensible to display instead of the intro gfx. */
    center_on_something();
    free_intro_radar_sprites();
    agents_game_start();
    editgui_tileset_changed();

    refresh_overview_canvas();

    update_info_label();        /* get initial population right */
    update_unit_focus();
    update_unit_info_label(get_units_in_focus());

    if (auto_center_each_turn) {
      center_on_something();
    }
    break;

  case C_S_OVER:
    if (C_S_RUNNING == oldstate) {
      /* Extra kludge for end-game handling of the CMA. */
      if (pplayer && pplayer->cities) {
        city_list_iterate(pplayer->cities, pcity) {
          if (cma_is_city_under_agent(pcity, NULL)) {
            cma_release_city(pcity);
          }
        } city_list_iterate_end;
      }
      popdown_all_city_dialogs();
      popdown_all_game_dialogs();
      set_unit_focus(NULL);
    } else {
      /* From C_S_PREPARING. */
      init_city_report_game_data();
      precalc_tech_data();
      if (pplayer) {
        player_research_update(pplayer);
      }
      role_unit_precalcs();
      boot_help_texts(pplayer);            /* reboot */
      set_unit_focus(NULL);
      set_client_page(PAGE_GAME);
      center_on_something();
    }
    refresh_overview_canvas();

    update_info_label();
    update_unit_focus();
    update_unit_info_label(NULL);

    break;
  }

  update_menus();
  update_turn_done_button_state();
  update_conn_list_dialog();
  if (can_client_change_view()) {
    update_map_canvas_visible();
  }

  /* If turn was going to change, that is now aborted. */
  set_server_busy(FALSE);
}

/**************************************************************************
  ...
**************************************************************************/
enum client_states client_state(void)
{
  return civclient_state;
}

/**************************************************************************
  Remove pconn from all connection lists in client, then free it.
**************************************************************************/
void client_remove_cli_conn(struct connection *pconn)
{
  if (NULL != pconn->playing) {
    conn_list_unlink(pconn->playing->connections, pconn);
  }
  conn_list_unlink(game.all_connections, pconn);
  conn_list_unlink(game.est_connections, pconn);
  if (pconn != &client.conn) {
    free(pconn);
  }
}

/**************************************************************************
  Remove (and free) all connections from all connection lists in client.
  Assumes game.all_connections is properly maintained with all connections.
**************************************************************************/
void client_remove_all_cli_conn(void)
{
  while (conn_list_size(game.all_connections) > 0) {
    struct connection *pconn = conn_list_get(game.all_connections, 0);
    client_remove_cli_conn(pconn);
  }
}

/**************************************************************************
..
**************************************************************************/
void send_attribute_block_request()
{
  send_packet_player_attribute_block(&client.conn);
}

/**************************************************************************
..
**************************************************************************/
void wait_till_request_got_processed(int request_id)
{
  input_from_server_till_request_got_processed(client.conn.sock,
					       request_id);
}

/**************************************************************************
..
**************************************************************************/
bool client_is_observer(void)
{
  return client.conn.established && client.conn.observer;
}

/* Seconds_to_turndone is the number of seconds the server has told us
 * are left.  The timer tells exactly how much time has passed since the
 * server gave us that data. */
static double seconds_to_turndone = 0.0;
static struct timer *turndone_timer;

/* This value shows what value the timeout label is currently showing for
 * the seconds-to-turndone. */
static int seconds_shown_to_turndone;

/**************************************************************************
  Reset the number of seconds to turndone from an "authentic" source.

  The seconds are taken as a double even though most callers will just
  know an integer value.
**************************************************************************/
void set_seconds_to_turndone(double seconds)
{
  if (game.info.timeout > 0) {
    seconds_to_turndone = seconds;
    turndone_timer = renew_timer_start(turndone_timer, TIMER_USER,
				       TIMER_ACTIVE);

    /* Maybe we should do an update_timeout_label here, but it doesn't
     * seem to be necessary. */
    seconds_shown_to_turndone = ceil(seconds) + 0.1;
  }
}

/**************************************************************************
  Return the number of seconds until turn-done.  Don't call this unless
  game.info.timeout != 0.
**************************************************************************/
int get_seconds_to_turndone(void)
{
  if (game.info.timeout > 0) {
    return seconds_shown_to_turndone;
  } else {
    /* This shouldn't happen. */
    return FC_INFINITY;
  }
}

/**************************************************************************
  This function should be called at least once per second.  It does various
  updates (idle animations and timeout updates).  It returns the number of
  seconds until it should be called again.
**************************************************************************/
double real_timer_callback(void)
{
  double time_until_next_call = 1.0;

  voteinfo_queue_check_removed();

  {
    double autoconnect_time = try_to_autoconnect();
    time_until_next_call = MIN(time_until_next_call, autoconnect_time);
  }

  if (C_S_RUNNING != client_state()) {
    return time_until_next_call;
  }

  {
    double blink_time = blink_turn_done_button();

    time_until_next_call = MIN(time_until_next_call, blink_time);
  }

  if (get_num_units_in_focus() > 0) {
    double blink_time = blink_active_unit();

    time_until_next_call = MIN(time_until_next_call, blink_time);
  }

  /* It is possible to have game.info.timeout > 0 but !turndone_timer, in the
   * first moments after the timeout is set. */
  if (game.info.timeout > 0 && turndone_timer) {
    double seconds = seconds_to_turndone - read_timer_seconds(turndone_timer);
    int iseconds = ceil(seconds) + 0.1; /* Turn should end right on 0. */

    if (iseconds < seconds_shown_to_turndone) {
      seconds_shown_to_turndone = iseconds;
      update_timeout_label();
    }

    time_until_next_call = MIN(time_until_next_call,
			       seconds - floor(seconds) + 0.001);
  }

  /* Make sure we wait at least 50 ms, otherwise we may not give any other
   * code time to run. */
  return MAX(time_until_next_call, 0.05);
}

/**************************************************************************
  Returns TRUE iff the client can control player.
**************************************************************************/
bool can_client_control(void)
{
  return (NULL != client.conn.playing
	  && !client_is_observer());
}

/**************************************************************************
  Returns TRUE iff the client can issue orders (such as giving unit
  commands).  This function should be called each time before allowing the
  user to give an order.
**************************************************************************/
bool can_client_issue_orders(void)
{
  return (can_client_control()
	  && C_S_RUNNING == client_state());
}

/**************************************************************************
  Returns TRUE iff the client can do diplomatic meetings with another 
  given player.
**************************************************************************/
bool can_meet_with_player(const struct player *pplayer)
{
  return (can_client_issue_orders()
	  /* && NULL != client.conn.playing (above) */
	  && could_meet_with_player(client.conn.playing, pplayer));
}

/**************************************************************************
  Returns TRUE iff the client can get intelligence from another 
  given player.
**************************************************************************/
bool can_intel_with_player(const struct player *pplayer)
{
  return (client_is_observer()
	  || (NULL != client.conn.playing
	      && could_intel_with_player(client.conn.playing, pplayer)));
}

/**************************************************************************
  Return TRUE if the client can change the view; i.e. if the mapview is
  active.  This function should be called each time before allowing the
  user to do mapview actions.
**************************************************************************/
bool can_client_change_view(void)
{
  return ((NULL != client.conn.playing || client_is_observer())
	  && (C_S_RUNNING == client_state()
	      || C_S_OVER == client_state()));
}

/**************************************************************************
  Sets if server is considered busy. Currently it is considered busy
  between turns.
**************************************************************************/
void set_server_busy(bool busy)
{
  if (busy != server_busy) {
    /* server_busy value will change */
    server_busy = busy;

    /* This may mean that we have to change from or to wait cursor */
    control_mouse_cursor(NULL);
  }
}

/**************************************************************************
  Returns if server is considered busy at the moment
**************************************************************************/
bool is_server_busy(void)
{
  return server_busy;
}

/****************************************************************************
  ...
****************************************************************************/
bool client_is_global_observer(void)
{
  return client.conn.playing == NULL && client.conn.observer == TRUE;
}

/****************************************************************************
  ...
****************************************************************************/
int client_player_number(void)
{
  if (client.conn.playing == NULL) {
    return -1;
  }
  return player_number(client.conn.playing);
}

/****************************************************************************
  Either controlling or observing.
****************************************************************************/
bool client_has_player(void)
{
  return client.conn.playing != NULL;
}

/****************************************************************************
  Either controlling or observing.
****************************************************************************/
struct player *client_player(void)
{
  return client.conn.playing;
}
