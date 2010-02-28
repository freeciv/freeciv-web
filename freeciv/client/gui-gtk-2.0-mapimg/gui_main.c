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

#ifdef AUDIO_SDL
#include "SDL.h"
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBREADLINE
#include <readline/history.h>
#include <readline/readline.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* utility */
#include "fciconv.h"
#include "log.h"
#include "netintf.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* common */
#include "console.h"
#include "game.h"

/* client */
#include "chatline_common.h"
#include "client_main.h"
#include "clinet.h"
#include "editgui_g.h"
#include "ggz_g.h"
#include "options.h"
#include "tilespec.h"

/* client/mapimg */
#include "mapimg.h"  /* MaPfa */
#include "resources.h"

#include "gui_main.h"

const char *client_string = "gui-mapimg";

const char * const gui_character_encoding = "UTF-8";
const bool gui_use_transliteration = FALSE;

static gint timer_id; /* main timer */
static guint input_id;

static gboolean timer_callback(gpointer data);
static void get_net_input(gpointer data, gint fid, GdkInputCondition condition);
static void set_wait_for_writable_socket(struct connection *pc,
                                         bool socket_writable);
static gint idle_callback_wrapper(gpointer data);

static void log_callback_utf8(enum log_level level, const char *message,
                              bool file_too);
static void parse_options(int argc, char **argv);
static void print_usage(const char *argv0);
static void mapimg_main(void);

static bool handle_client_stdin_input(char *str);
static enum client_events client_sniff_all_input(void);

enum client_events {
  C_E_END_OF_TURN_TIMEOUT,
  C_E_OTHERWISE,
  C_E_FORCE_END_OF_SNIFF,
};

#define BUF_SIZE 512
static bool force_end_of_sniff;
static int lastturn = -1;
static bool no_input = FALSE;
static bool readline_initialized = FALSE;
static bool readline_handled_input = FALSE;
static int sock;

/****************************************************************************
  Called by the tileset code to set the font size that should be used to
  draw the city names and productions.
****************************************************************************/
void set_city_names_font_sizes(int my_city_names_font_size,
			       int my_city_productions_font_size)
{
  /* PORTME */
}

/**************************************************************************
  Do any necessary pre-initialization of the UI, if necessary.
**************************************************************************/
void ui_init(void)
{
  log_set_callback(log_callback_utf8);
}

/**************************************************************************
  Entry point for whole freeciv client program.
**************************************************************************/
int main(int argc, char **argv)
{
  return client_main(argc, argv);
}

/**************************************************************************
...
**************************************************************************/
static void log_callback_utf8(enum log_level level, const char *message,
                              bool file_too)
{
  if (!file_too || level <= LOG_FATAL) {
    fc_fprintf(stderr, "%d: %s\n", level, message);
  }
}

/**************************************************************************
 Called while in gtk_main() (which is all of the time)
 TIMER_INTERVAL is now set by real_timer_callback()
**************************************************************************/
static gboolean timer_callback(gpointer data)
{
  double seconds = real_timer_callback();
  static int count = 0;

  log_error("timer_callback(): run %4d", count);
  count++;

  mapimg_main();

  timer_id = g_timeout_add(seconds * 1000, timer_callback, NULL);

  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static void get_net_input(gpointer data, gint fid, GdkInputCondition condition)
{
  input_from_server(fid);
}

/**************************************************************************
...
**************************************************************************/
static void set_wait_for_writable_socket(struct connection *pc,
					 bool socket_writable)
{
  static bool previous_state = FALSE;

  assert(pc == &client.conn);

  if (previous_state == socket_writable)
    return;

  log_debug("set_wait_for_writable_socket(%d)", socket_writable);
  gtk_input_remove(input_id);
  input_id = gtk_input_add_full(client.conn.sock, GDK_INPUT_READ
				| (socket_writable ? GDK_INPUT_WRITE : 0)
				| GDK_INPUT_EXCEPTION,
				get_net_input, NULL, NULL, NULL);
  previous_state = socket_writable;
}

/**************************************************************************
  Main function for mapimg.
**************************************************************************/
static void mapimg_main(void)
{
  /* MaPfa */
  /* use this as starting point for mapimg */
  char mapdef[128];
  char basename[128];
  static int saveturn = -1;
  static bool colortest = TRUE;
  static bool observer = FALSE;
  static int rand;
  int i;

  if (client.conn.used && !client.conn.observer&& ! observer) {
    log_error("update_info_label(): send '/observe'");
    send_chat_printf("/detach");
    send_chat_printf("/observe");
    observer = TRUE;
  }

  if (C_S_RUNNING != client_state()) {
    return;
  }

  if (C_S_OVER == client_state()) {
    client_exit();
  }

log_error("mapimg_main()");

  if (saveturn == -1) {
    rand = myrand(1000);
  }

  if ((client_is_observer() || client_is_global_observer())
      && client_state() == C_S_RUNNING && mapimg_count() == 0) {
    if (!client_is_global_observer()) {
      my_snprintf(mapdef, sizeof(mapdef),
                  "show=plrbv:plrbv=100:zoom=2:map=bcfktu");
    } else {
      my_snprintf(mapdef, sizeof(mapdef),
                  "show=all:zoom=2:map=bcfktu");
    }

    log_error("map definition: %s", mapdef);
    if (!mapimg_define(mapdef, FALSE)) {
      log_error("ERROR: %s\n(map definition: %s)", mapimg_error(), mapdef);
    }
  }

  for (i = 0; i < mapimg_count(); i++) {
    if (!mapimg_isvalid(i)) {
      log_error("map %d: %s", i, mapimg_error());
    } else if (colortest) {
      colortest = FALSE;
      my_snprintf(basename, sizeof(basename), "test");
      if (!mapimg_colortest(basename)) {
        log_error("%s", mapimg_error());
      }
    }
  }

  /* create map images */
  for (i = 0; i < mapimg_count(); i++) {
    if (mapimg_isvalid(i)) {
      if (saveturn != game.info.turn) {
        saveturn = game.info.turn;
        my_snprintf(basename, sizeof(basename), "test%03d-T%03d", rand, game.info.turn);
        if (!mapimg_create(i, basename, TRUE)) {
          log_error("%s", mapimg_error());
        }
      }
    }
  }

  client_sniff_all_input();
}

/*****************************************************************************
...
*****************************************************************************/
static void handle_readline_input_callback(char *line)
{
  char *line_internal;

  if (no_input) {
    return;
  }

  if (line[0] != '\0')
    add_history(line);

  con_prompt_enter();		/* just got an 'Enter' hit */
  line_internal = local_to_internal_string_malloc(line);
  (void) handle_client_stdin_input(line_internal);
  free(line_internal);
  free(line);

  readline_handled_input = TRUE;
}

/*****************************************************************************
  copy of sernet.c:server_sniff_all_input()
*****************************************************************************/
static enum client_events client_sniff_all_input(void)
{
  int max_desc;
  fd_set readfs, writefs, exceptfs;
  struct timeval tv;
#ifdef SOCKET_ZERO_ISNT_STDIN
  char *bufptr;
#endif

  con_prompt_init();

#ifdef HAVE_LIBREADLINE
  {
    if (!no_input && !readline_initialized) {
      rl_initialize();
      rl_callback_handler_install((char *) "> ",
                                  handle_readline_input_callback);

      readline_initialized = TRUE;
      atexit(rl_callback_handler_remove);
    }
  }
#endif /* HAVE_LIBREADLINE */

  while (TRUE) {
    con_prompt_on();                /* accepting new input */

    if (force_end_of_sniff) {
      force_end_of_sniff = FALSE;
      con_prompt_off();
      return C_E_FORCE_END_OF_SNIFF;
    }

    /* Don't wait if timeout == -1 (i.e. on auto games) */
    if (C_S_RUNNING == client_state() && game.info.timeout == -1) {
      return C_E_END_OF_TURN_TIMEOUT;
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    MY_FD_ZERO(&readfs);
    MY_FD_ZERO(&writefs);
    MY_FD_ZERO(&exceptfs);

    if (!no_input) {
#ifdef SOCKET_ZERO_ISNT_STDIN
      my_init_console();
#else /* SOCKET_ZERO_ISNT_STDIN */
#   if !defined(__VMS)
      FD_SET(0, &readfs);
#   endif /* VMS */
#endif /* SOCKET_ZERO_ISNT_STDIN */
    }

    FD_SET(sock, &readfs);
    FD_SET(sock, &exceptfs);
    max_desc = sock;

    con_prompt_off(); /* output doesn't generate a new prompt */

    if (fc_select(max_desc + 1, &readfs, &writefs, &exceptfs, &tv) == 0) {
      if (lastturn != game.info.turn) {
        con_prompt_off();
log_error("lastturn");
        lastturn = game.info.turn;
        return C_E_END_OF_TURN_TIMEOUT;
      }
    }

#ifdef SOCKET_ZERO_ISNT_STDIN
    if (!no_input && (bufptr = my_read_console())) {
      char *bufptr_internal = local_to_internal_string_malloc(bufptr);

      con_prompt_enter();	/* will need a new prompt, regardless */
      handle_client_stdin_input(bufptr_internal);
      free(bufptr_internal);
    }
#else  /* !SOCKET_ZERO_ISNT_STDIN */
    if(!no_input && FD_ISSET(0, &readfs)) {    /* input from server operator */
#ifdef HAVE_LIBREADLINE
      rl_callback_read_char();
      if (readline_handled_input) {
        readline_handled_input = FALSE;
        con_prompt_enter_clear();
      }
      continue;
#else  /* !HAVE_LIBREADLINE */
      ssize_t didget;
      char *buffer = NULL; /* Must be NULL when calling getline() */
      char *buf_internal;

#ifdef HAVE_GETLINE
      size_t len = 0;

      didget = getline(&buffer, &len, stdin);
      if (didget >= 1) {
        buffer[didget-1] = '\0'; /* overwrite newline character */
        didget--;
        log_debug("Got line: \"%s\" (%ld, %ld)", buffer,
                  (long int) didget, (long int) len);
      }
#else  /* HAVE_GETLINE */
      buffer = malloc(BUF_SIZE + 1);

      didget = read(0, buffer, BUF_SIZE);
      if (didget < 0) {
        didget = 0; /* Avoid buffer underrun below. */
      }
      *(buffer+didget)='\0';
#endif /* HAVE_GETLINE */
      if (didget <= 0) {
        handle_stdin_close();
      }

      con_prompt_enter();        /* will need a new prompt, regardless */

      if (didget >= 0) {
        buf_internal = local_to_internal_string_malloc(buffer);
        handle_client_stdin_input(buf_internal);
        free(buf_internal);
      }
      free(buffer);
#endif /* !HAVE_LIBREADLINE */
    }
#endif /* !SOCKET_ZERO_ISNT_STDIN */

/* TODO: input from server */

    break;
  }
  con_prompt_off();

  if (game.info.timeout > 0
      && C_S_RUNNING == client_state()
      && game.server.phase_timer) {
    return C_E_END_OF_TURN_TIMEOUT;
  }

  return C_E_OTHERWISE;
}

/**************************************************************************
  Print extra usage information, including one line help on each option,
  to stderr. 
**************************************************************************/
static bool handle_client_stdin_input(char *str)
{
  con_puts(C_OK, str);

  return TRUE;
}

/**************************************************************************
  Print extra usage information, including one line help on each option,
  to stderr. 
**************************************************************************/
static void print_usage(const char *argv0)
{
  /* add client-specific usage information here */
  fc_fprintf(stderr, _("This client has no special command line options\n\n"));

  /* TRANS: No full stop after the URL, could cause confusion. */
  fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);
}

/**************************************************************************
 search for command line options. right now, it's just help
 semi-useless until we have options that aren't the same across all clients.
**************************************************************************/
static void parse_options(int argc, char **argv)
{
  int i = 1;

  while (i < argc) {
    if (is_option("--help", argv[i])) {
      print_usage(argv[0]);
      exit(EXIT_SUCCESS);
    }
    i++;
  }
}

/**************************************************************************
  The main loop for the UI.  This is called from main(), and when it
  exits the client will exit.
**************************************************************************/
void ui_main(int argc, char *argv[])
{
  /* PORTME */
  fc_fprintf(stderr, "Freeciv rules!\n");

  const gchar *home;

  parse_options(argc, argv);

  /* the locale has already been set in init_nls() and the Win32-specific
   * locale logic in gtk_init() causes problems with zh_CN (see PR#39475) */
  gtk_disable_setlocale();

  /* GTK withdraw gtk options. Process GTK arguments */
  gtk_init(&argc, &argv);

  /* Load resources */
  gtk_rc_parse_string(fallback_resources);

  home = g_get_home_dir();
  if (home) {
    gchar *str;

    str = g_build_filename(home, ".freeciv.rc-2.0", NULL);
    gtk_rc_parse(str);
    g_free(str);
  }

  client_options_iterate(poption) {
    if (COT_FONT == option_type(poption)) {
      /* Force to call the appropriated callback. */
      option_changed(poption);
    }
  } client_options_iterate_end;

  tileset_init(tileset);
  tileset_load_tiles(tileset);
  tileset_use_prefered_theme(tileset);
  mapimg_init(); /* MaPfa */

log_error("try autoconnect: %s@%s:%d", user_name, server_host, server_port);
  /* assumes toplevel showing */
  set_client_state(C_S_DISCONNECTED);

  /* assumes client_state is set */
  timer_id = g_timeout_add(TIMER_INTERVAL, timer_callback, NULL);

log_error("main_loop:start")
log_error("tilespec: %s", tileset_get_name(tileset));
  gtk_main();
log_error("main_loop:end")

  mapimg_free(); /* MaPfa */
  tileset_free_tiles(tileset);
}

/****************************************************************************
  Extra initializers for client options.
****************************************************************************/
void gui_options_extra_init(void)
{
  /* Nothing to do. */
}

/**************************************************************************
  Do any necessary UI-specific cleanup
**************************************************************************/
void ui_exit()
{
  /* PORTME */
}

/**************************************************************************
  Return our GUI type
**************************************************************************/
enum gui_type get_gui_type(void)
{
  return GUI_MAPIMG;
}

/**************************************************************************
 Update the connected users list at pregame state.
**************************************************************************/
void update_conn_list_dialog(void)
{
  /* PORTME */
}

/**************************************************************************
  Make a bell noise (beep).  This provides low-level sound alerts even
  if there is no real sound support.
**************************************************************************/
void sound_bell(void)
{
  /* PORTME */
}

/**************************************************************************
  Wait for data on the given socket.  Call input_from_server() when data
  is ready to be read.

  This function is called after the client succesfully has connected
  to the server.
**************************************************************************/
void add_net_input(int sock)
{
  input_id = gtk_input_add_full(sock, GDK_INPUT_READ | GDK_INPUT_EXCEPTION,
                                get_net_input, NULL, NULL, NULL);
  client.conn.notify_of_writable_data = set_wait_for_writable_socket;
}

/**************************************************************************
  Stop waiting for any server network data.  See add_net_input().

  This function is called if the client disconnects from the server.
**************************************************************************/
void remove_net_input(void)
{
  gtk_input_remove(input_id);
}

/**************************************************************************
  Called to monitor a GGZ socket.
**************************************************************************/
void add_ggz_input(int sock)
{
  /* PORTME */
}

/**************************************************************************
  Called on disconnection to remove monitoring on the GGZ socket.  Only
  call this if we're actually in GGZ mode.
**************************************************************************/
void remove_ggz_input(void)
{
  /* PORTME */
}

/**************************************************************************
  Set one of the unit icons (specified by idx) in the information area
  based on punit.

  punit is the unit the information should be taken from. Use NULL to
  clear the icon.

  idx specified which icon should be modified. Use idx==-1 to indicate
  the icon for the active unit. Or idx in [0..num_units_below-1] for
  secondary (inactive) units on the same tile.
**************************************************************************/
void set_unit_icon(int idx, struct unit *punit)
{
  /* PORTME */
}

/**************************************************************************
  Most clients use an arrow (e.g., sprites.right_arrow) to indicate when
  the units_below will not fit. This function is called to activate or
  deactivate the arrow.

  Is disabled by default.
**************************************************************************/
void set_unit_icons_more_arrow(bool onoff)
{
  /* PORTME */
}

struct callback {
  void (*callback)(void *data);
  void *data;
};

/****************************************************************************
  A wrapper for the callback called through add_idle_callback.
****************************************************************************/
static gint idle_callback_wrapper(gpointer data)
{
  struct callback *cb = data;

  (cb->callback)(cb->data);
  free(cb);
  return 0;
}

/****************************************************************************
  Enqueue a callback to be called during an idle moment.  The 'callback'
  function should be called sometimes soon, and passed the 'data' pointer
  as its data.
****************************************************************************/
void add_idle_callback(void (callback)(void *), void *data)
{
  struct callback *cb = fc_malloc(sizeof(*cb));

  cb->callback = callback;
  cb->data = data;
  gtk_idle_add(idle_callback_wrapper, cb);
}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_tileset_changed(void)
{}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_refresh(void)
{}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_popup_properties(const struct tile_list *tiles, int objtype)
{}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_popdown_all(void)
{}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_notify_object_changed(int objtype, int object_id, bool remove)
{}

/****************************************************************************
  Stub for editor function
****************************************************************************/
void editgui_notify_object_created(int tag, int id)
{}

/****************************************************************************
  Stub for ggz function
****************************************************************************/
void gui_ggz_embed_leave_table(void)
{}

/****************************************************************************
  Stub for ggz function
****************************************************************************/
void gui_ggz_embed_ensure_server(void)
{}


/**************************************************************************
  Updates a gui font style.
**************************************************************************/
void gui_update_font(const char *font_name, const char *font_value)
{
  /* PORTME */
}
