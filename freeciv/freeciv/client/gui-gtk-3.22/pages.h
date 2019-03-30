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
#ifndef FC__PAGES_H
#define FC__PAGES_H

#include <gtk/gtk.h>

/* utility */
#include "support.h"            /* bool type */

/* client */
#include "pages_g.h"

extern GtkWidget *start_message_area;

GtkWidget *create_main_page(void);
GtkWidget *create_start_page(void);
GtkWidget *create_scenario_page(void);
GtkWidget *create_load_page(void);
GtkWidget *create_network_page(void);

GtkWidget *create_statusbar(void);
void append_network_statusbar(const char *text, bool force);

void save_game_dialog_popup(void);
void save_scenario_dialog_popup(void);
void save_mapimg_dialog_popup(void);
void mapimg_client_save(const char *filename);

void ai_fill_changed_by_server(int aifill);

void destroy_server_scans(void);

#endif  /* FC__PAGES_H */
