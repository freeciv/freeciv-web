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
#ifndef FC__CHATLINE_H
#define FC__CHATLINE_H

#include <gtk/gtk.h>

/* include */
#include "chatline_g.h"

void chatline_init(void);

void inputline_make_chat_link(struct tile *ptile, bool unit);
bool inputline_has_focus(void);
void inputline_grab_focus(void);

void set_output_window_text(const char *text);
bool chatline_is_scrolled_to_bottom(void);
void chatline_scroll_to_bottom(bool delayed);

void set_message_buffer_view_link_handlers(GtkWidget *view);

GtkWidget *inputline_toolkit_view_new(void);
void inputline_toolkit_view_append_button(GtkWidget *toolkit_view,
                                          GtkWidget *button);

#endif  /* FC__CHATLINE_H */
