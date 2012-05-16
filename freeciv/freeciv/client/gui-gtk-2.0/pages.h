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
#ifndef FC__PAGES_H
#define FC__PAGES_H

#include <gtk/gtk.h>

#include "shared.h"		/* bool type */

#include "pages_g.h"

extern GtkWidget *start_message_area;
extern GtkWidget *take_button, *ready_button, *nation_button;

GtkWidget *create_main_page(void);
GtkWidget *create_start_page(void);
GtkWidget *create_scenario_page(void);
GtkWidget *create_load_page(void);
GtkWidget *create_network_page(void);
GtkWidget *create_nation_page(void);

GtkWidget *create_statusbar(void);
void append_network_statusbar(const char *text, bool force);
void popup_save_dialog(bool scenario);

#endif  /* FC__PAGES_H */

