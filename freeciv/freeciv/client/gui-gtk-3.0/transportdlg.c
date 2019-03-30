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

#include <gtk/gtk.h>

/* utility */
#include "fcintl.h"

/* common */
#include "game.h"
#include "movement.h"
#include "unit.h"

/* client */
#include "control.h"
#include "tilespec.h"

/* client/gui-gtk-3.0 */
#include "gui_main.h"
#include "gui_stuff.h"
#include "sprite.h"
#include "unitselunitdlg.h"

#include "transportdlg.h"

struct transport_radio_cb_data {
  GtkWidget *dlg;
  int tp_id;
};

/************************************************************************//**
  Handle user response to transport dialog.
****************************************************************************/
static void transport_response_callback(GtkWidget *dlg, gint arg)
{
  if (arg == GTK_RESPONSE_YES) {
    struct unit *pcargo =
      game_unit_by_number(GPOINTER_TO_INT(
                            g_object_get_data(G_OBJECT(dlg),
                                              "actor")));

    if (pcargo != NULL) {
      int tp_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dlg),
                                                    "target"));
      struct tile *ptile = g_object_get_data(G_OBJECT(dlg), "tile");

      if (tp_id == 0) {
        /* Load to any */
        request_unit_load(pcargo, NULL, ptile);
      } else {
        struct unit *ptransport = game_unit_by_number(tp_id);

        if (ptransport != NULL) {
          /* Still exist */
          request_unit_load(pcargo, ptransport, ptile);
        }
      }
    }
  }

  gtk_widget_destroy(dlg);
}

/************************************************************************//**
  Handle transport request automatically when there's nothing to
  choose from. Otherwise open up transport dialog for the unit
****************************************************************************/
bool request_transport(struct unit *cargo, struct tile *ptile)
{
  int tcount;
  struct unit_list *potential_transports = unit_list_new();
  struct unit *best_transport = transporter_for_unit_at(cargo, ptile);

  unit_list_iterate(ptile->units, ptransport) {
    if (can_unit_transport(ptransport, cargo)
        && get_transporter_occupancy(ptransport) < get_transporter_capacity(ptransport)) {
      unit_list_append(potential_transports, ptransport);
    }
  } unit_list_iterate_end;

  tcount = unit_list_size(potential_transports);

  if (tcount == 0) {
    fc_assert(best_transport == NULL);
    unit_list_destroy(potential_transports);

    return FALSE; /* Unit was not handled here. */
  } else if (tcount == 1) {
    /* There's exactly one potential transport - use it automatically */
    fc_assert(unit_list_get(potential_transports, 0) == best_transport);
    request_unit_load(cargo, unit_list_get(potential_transports, 0), ptile);

    unit_list_destroy(potential_transports);

    return TRUE;
  }

  return select_tgt_unit(cargo, ptile, potential_transports, best_transport,
                         _("Transport selection"),
                         _("Looking for transport:"),
                         _("Transports available:"),
                         _("Load"),
                         G_CALLBACK(transport_response_callback));
}
