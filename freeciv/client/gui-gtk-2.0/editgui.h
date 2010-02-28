/********************************************************************** 
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__EDITGUI_H
#define FC__EDITGUI_H

#include <gtk/gtk.h>

#include "editor.h"
#include "editgui_g.h"

struct tool_value_selector;

struct editbar {
  /* The widget holding the entire edit bar. */
  GtkWidget *widget;

  GtkTooltips *tooltips;
  GtkSizeGroup *size_group;

  GtkWidget *mode_buttons[NUM_EDITOR_TOOL_MODES];
  GtkWidget *tool_buttons[NUM_EDITOR_TOOL_TYPES];
  GtkWidget *player_properties_button;
  struct tool_value_selector *tool_selectors[NUM_EDITOR_TOOL_TYPES];

  GtkListStore *player_pov_store;
  GtkWidget *player_pov_combobox;
};

gboolean handle_edit_mouse_button_press(GdkEventButton *ev);
gboolean handle_edit_mouse_button_release(GdkEventButton *ev);
gboolean handle_edit_mouse_move(GdkEventMotion *ev);
gboolean handle_edit_key_press(GdkEventKey *ev);
gboolean handle_edit_key_release(GdkEventKey *ev);

struct editinfobox {
  GtkWidget *widget;

  GtkTooltips *tooltips;

  GtkWidget *mode_image;
  GtkWidget *mode_label;

  GtkWidget *tool_label;
  GtkWidget *tool_value_label;
  GtkWidget *tool_image;

  GtkWidget *size_hbox;
  GtkWidget *size_spin_button;
  GtkWidget *count_hbox;
  GtkWidget *count_spin_button;

  GtkListStore *tool_applied_player_store;
  GtkWidget *tool_applied_player_combobox;
};

void editgui_create_widgets(void);
struct editbar *editgui_get_editbar(void);
struct editinfobox *editgui_get_editinfobox(void);

#endif  /* FC__EDITGUI_H */
