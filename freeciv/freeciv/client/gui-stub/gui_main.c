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

#include <stdio.h>

/* utility */
#include "fc_cmdline.h"
#include "fciconv.h"
#include "log.h"

/* gui main header */
#include "gui_stub.h"

/* client */
#include "gui_cbsetter.h"
#include "client_main.h"
#include "editgui_g.h"
#include "options.h"

#include "gui_main.h"

const char *client_string = "gui-stub";

const char * const gui_character_encoding = "UTF-8";
const bool gui_use_transliteration = FALSE;

/**********************************************************************//**
  Do any necessary pre-initialization of the UI, if necessary.
**************************************************************************/
void gui_ui_init(void)
{
  /* PORTME */
}

/**********************************************************************//**
  Entry point for whole freeciv client program.
**************************************************************************/
int main(int argc, char **argv)
{
  setup_gui_funcs();
  return client_main(argc, argv);
}

/**********************************************************************//**
  Print extra usage information, including one line help on each option,
  to stderr.
**************************************************************************/
static void print_usage(const char *argv0)
{
  /* PORTME */
  /* add client-specific usage information here */
  fc_fprintf(stderr,
             _("This client has no special command line options\n\n"));

  /* TRANS: No full stop after the URL, could cause confusion. */
  fc_fprintf(stderr, _("Report bugs at %s\n"), BUG_URL);
}

/**********************************************************************//**
  Parse and enact any client-specific options.
**************************************************************************/
static void parse_options(int argc, char **argv)
{
  int i = 1;

  while (i < argc) {
    if (is_option("--help", argv[i])) {
      print_usage(argv[0]);
      exit(EXIT_SUCCESS);
    } else {
      fc_fprintf(stderr, _("Unrecognized option: \"%s\"\n"), argv[i]);
      exit(EXIT_FAILURE);
    }

    i++;
  }
}

/**********************************************************************//**
  The main loop for the UI.  This is called from main(), and when it
  exits the client will exit.
**************************************************************************/
void gui_ui_main(int argc, char *argv[])
{
  parse_options(argc, argv);

  /* PORTME */
  fc_fprintf(stderr, "Freeciv rules!\n");

  /* Main loop here */

  start_quitting();
}

/**********************************************************************//**
  Extra initializers for client options.
**************************************************************************/
void gui_options_extra_init(void)
{
  /* Nothing to do. */
}

/**********************************************************************//**
  Do any necessary UI-specific cleanup
**************************************************************************/
void gui_ui_exit()
{
  /* PORTME */
}

/**********************************************************************//**
  Return our GUI type
**************************************************************************/
enum gui_type gui_get_gui_type(void)
{
  return GUI_STUB;
}

/**********************************************************************//**
  Update the connected users list at pregame state.
**************************************************************************/
void gui_real_conn_list_dialog_update(void *unused)
{
  /* PORTME */
}

/**********************************************************************//**
  Make a bell noise (beep).  This provides low-level sound alerts even
  if there is no real sound support.
**************************************************************************/
void gui_sound_bell(void)
{
  /* PORTME */
}

/**********************************************************************//**
  Wait for data on the given socket.  Call input_from_server() when data
  is ready to be read.

  This function is called after the client succesfully has connected
  to the server.
**************************************************************************/
void gui_add_net_input(int sock)
{
  /* PORTME */
}

/**********************************************************************//**
  Stop waiting for any server network data.  See add_net_input().

  This function is called if the client disconnects from the server.
**************************************************************************/
void gui_remove_net_input(void)
{
  /* PORTME */
}

/**********************************************************************//**
  Set one of the unit icons (specified by idx) in the information area
  based on punit.

  punit is the unit the information should be taken from. Use NULL to
  clear the icon.

  idx specified which icon should be modified. Use idx == -1 to indicate
  the icon for the active unit. Or idx in [0..num_units_below - 1] for
  secondary (inactive) units on the same tile.
**************************************************************************/
void gui_set_unit_icon(int idx, struct unit *punit)
{
  /* PORTME */
}

/**********************************************************************//**
  Most clients use an arrow (e.g., sprites.right_arrow) to indicate when
  the units_below will not fit. This function is called to activate or
  deactivate the arrow.

  Is disabled by default.
**************************************************************************/
void gui_set_unit_icons_more_arrow(bool onoff)
{
  /* PORTME */
}

/**********************************************************************//**
  Called when the set of units in focus (get_units_in_focus()) changes.
  Standard updates like update_unit_info_label() are handled in the platform-
  independent code, so some clients will not need to do anything here.
**************************************************************************/
void gui_real_focus_units_changed(void)
{
  /* PORTME */
}

/**********************************************************************//**
  Enqueue a callback to be called during an idle moment.  The 'callback'
  function should be called sometimes soon, and passed the 'data' pointer
  as its data.
**************************************************************************/
void gui_add_idle_callback(void (callback)(void *), void *data)
{
  /* PORTME */

  /* This is a reasonable fallback if it's not ported. */
  log_error("Unimplemented add_idle_callback.");
  (callback)(data);
}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_tileset_changed(void)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_refresh(void)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_popup_properties(const struct tile_list *tiles, int objtype)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_popdown_all(void)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_notify_object_changed(int objtype, int object_id,
                                       bool removal)
{}

/**********************************************************************//**
  Stub for editor function
**************************************************************************/
void gui_editgui_notify_object_created(int tag, int id)
{}

/**********************************************************************//**
  Updates a gui font style.
**************************************************************************/
void gui_gui_update_font(const char *font_name, const char *font_value)
{
  /* PORTME */
}

/**********************************************************************//**
  Insert build information to help
**************************************************************************/
void gui_insert_client_build_info(char *outbuf, size_t outlen)
{
  /* PORTME */
}

/**********************************************************************//**
  Make dynamic adjustments to first-launch default options.
**************************************************************************/
void gui_adjust_default_options(void)
{
  /* Nothing in case of this gui */
}
