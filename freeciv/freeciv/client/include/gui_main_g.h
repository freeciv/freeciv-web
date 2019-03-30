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

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "fc_types.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, ui_init, void)
GUI_FUNC_PROTO(void, ui_main, int argc, char *argv[])
GUI_FUNC_PROTO(void, ui_exit, void)
GUI_FUNC_PROTO(void, options_extra_init, void)

GUI_FUNC_PROTO(void, real_conn_list_dialog_update, void*)
GUI_FUNC_PROTO(void, sound_bell, void)
GUI_FUNC_PROTO(void, add_net_input, int)
GUI_FUNC_PROTO(void, remove_net_input, void)

GUI_FUNC_PROTO(void, set_unit_icon, int idx, struct unit *punit)
GUI_FUNC_PROTO(void, set_unit_icons_more_arrow, bool onoff)
GUI_FUNC_PROTO(void, real_focus_units_changed, void)

GUI_FUNC_PROTO(void, add_idle_callback, void (callback)(void *), void *data)

GUI_FUNC_PROTO(enum gui_type, get_gui_type, void)
GUI_FUNC_PROTO(void, insert_client_build_info, char *outbuf, size_t outlen)

GUI_FUNC_PROTO(void, gui_update_font, const char *font_name,
               const char *font_value)
GUI_FUNC_PROTO(void, adjust_default_options, void)

extern const char *client_string;

/* Actually defined in update_queue.c */
void conn_list_dialog_update(void);

#endif  /* FC__GUI_MAIN_G_H */
