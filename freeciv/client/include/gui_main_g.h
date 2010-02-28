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
#ifndef FC__GUI_MAIN_G_H
#define FC__GUI_MAIN_G_H

#include "shared.h"		/* bool type */

#include "fc_types.h"

void set_city_names_font_sizes(int city_names_font_size,
			       int city_productions_font_size);

void ui_init(void);
void ui_main(int argc, char *argv[]);
void ui_exit(void);
void gui_options_extra_init(void);

void update_conn_list_dialog(void);
void sound_bell(void);
void add_net_input(int);
void remove_net_input(void);
void add_ggz_input(int socket);
void remove_ggz_input(void);

void set_unit_icon(int idx, struct unit *punit);
void set_unit_icons_more_arrow(bool onoff);

void add_idle_callback(void (callback)(void *), void *data);

enum gui_type get_gui_type(void);

extern const char *client_string;

#endif  /* FC__GUI_MAIN_G_H */
