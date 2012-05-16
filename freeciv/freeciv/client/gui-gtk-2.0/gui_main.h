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
#ifndef FC__GUI_MAIN_H
#define FC__GUI_MAIN_H

#include <gtk/gtk.h>

#include "gtkpixcomm.h"
#include "gui_main_g.h"

/* network string charset conversion */
gchar *ntoh_str(const gchar *netstr);

extern PangoFontDescription *        main_font;
extern PangoFontDescription *        city_productions_font;

extern GdkGC *          civ_gc;
extern GdkGC *          mask_fg_gc;
extern GdkGC *          mask_bg_gc;
extern GdkGC *          fill_bg_gc;
extern GdkGC *          fill_tile_gc;
extern GdkGC *          thin_line_gc;
extern GdkGC *          thick_line_gc;
extern GdkGC *          border_line_gc;
extern GdkGC *          selection_gc;
extern GdkPixmap *      gray50;
extern GdkPixmap *      gray25;
extern GdkPixmap *      black50;
extern GdkPixmap *      mask_bitmap;
#define single_tile_pixmap (mapview.single_tile->pixmap)
extern GtkTextView *	main_message_area;
extern GtkWidget *      text_scrollbar;
extern GtkWidget *      toplevel;
extern GtkWidget *      top_vbox;
extern GtkWidget *      main_frame_civ_name;
extern GtkWidget *      main_label_info;
extern GtkWidget *      econ_label[10];
extern GtkWidget *      bulb_label;
extern GtkWidget *      sun_label;
extern GtkWidget *      flake_label;
extern GtkWidget *      government_label;
extern GtkTooltips *	main_tips;
extern GtkWidget *	econ_ebox;
extern GtkWidget *	bulb_ebox;
extern GtkWidget *	sun_ebox;
extern GtkWidget *	flake_ebox;
extern GtkWidget *	government_ebox;
extern GtkWidget *      map_canvas;             /* GtkDrawingArea */
extern GtkWidget *      overview_canvas;        /* GtkDrawingArea */
extern GtkWidget *      timeout_label;
extern GtkWidget *      turn_done_button;
extern GtkWidget *      unit_info_box;
extern GtkWidget *      unit_info_label;
extern GtkWidget *      unit_info_frame;
extern GtkWidget *      map_horizontal_scrollbar;
extern GtkWidget *      map_vertical_scrollbar;
extern GdkWindow *      root_window;

extern GtkWidget *	toplevel_tabs;
extern GtkWidget *	top_notebook;
extern GtkWidget *      map_widget;
extern GtkWidget *	bottom_notebook;
extern GtkWidget *	right_notebook;
extern GtkTextBuffer *	message_buffer;

/* NB: Must match creation arugments in
 * create_start_page() in pages.c */
enum connection_list_columns {
  CL_COL_PLAYER_NUMBER = 0,
  CL_COL_USER_NAME,
  CL_COL_READY_STATE,
  CL_COL_PLAYER_NAME,
  CL_COL_FLAG,
  CL_COL_NATION,
  CL_COL_TEAM,
  CL_COL_GGZ_RECORD,
  CL_COL_GGZ_RATING,
  CL_COL_CONN_ID,
  
  CL_NUM_COLUMNS
};
extern GtkTreeStore *connection_list_store;
extern GtkTreeView *connection_list_view;

void enable_menus(bool enable);

gboolean map_canvas_focus(void);

void reset_unit_table(void);
void popup_quit_dialog(void);
void refresh_chat_buttons(void);

/* There simply is no proper header to place this define. Creating one
 * just for this seems overkill. */
#define FC_GTK_DEFAULT_THEME_NAME "Freeciv"

#endif  /* FC__GUI_MAIN_H */
