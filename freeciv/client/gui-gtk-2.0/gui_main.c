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

#include <assert.h>
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef GGZ_GTK
#  include <ggz-gtk.h>
#endif

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "dataio.h"
#include "featured_text.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "unitlist.h"
#include "version.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "clinet.h"
#include "colors.h"
#include "connectdlg_common.h"
#include "control.h"
#include "editor.h"
#include "ggzclient.h"
#include "options.h"
#include "text.h"
#include "tilespec.h"

/* gui-gtk-2.0 */
#include "chatline.h"
#include "connectdlg.h"
#include "cma_fe.h"
#include "dialogs.h"
#include "diplodlg.h"
#include "editgui.h"
#include "gotodlg.h"
#include "graphics.h"
#include "gui_stuff.h"
#include "happiness.h"
#include "inteldlg.h"
#include "mapctrl.h"
#include "mapview.h"
#include "menu.h"
#include "messagewin.h"
#include "optiondlg.h"
#include "pages.h"
#include "plrdlg.h"
#include "spaceshipdlg.h"
#include "resources.h"
#include "voteinfo_bar.h"

#include "gui_main.h"

const char *client_string = "gui-gtk-2.0";

GtkWidget *map_canvas;                  /* GtkDrawingArea */
GtkWidget *map_horizontal_scrollbar;
GtkWidget *map_vertical_scrollbar;

GtkWidget *overview_canvas;             /* GtkDrawingArea */
GdkPixmap *overview_canvas_store;       /* this pixmap acts as a backing store 
                                         * for the overview_canvas widget */
int overview_canvas_store_width = 2 * 80;
int overview_canvas_store_height = 2 * 50;

GtkWidget *toplevel;
GdkWindow *root_window;
GtkWidget *toplevel_tabs;
GtkWidget *top_vbox;
GtkWidget *top_notebook, *bottom_notebook, *right_notebook;
GtkWidget *map_widget;
static GtkWidget *bottom_hpaned;

int city_names_font_size = 0, city_productions_font_size = 0;
PangoFontDescription *main_font;
PangoFontDescription *city_productions_font;

GdkGC *civ_gc;
GdkGC *mask_fg_gc;
GdkGC *mask_bg_gc;
GdkGC *fill_bg_gc;
GdkGC *fill_tile_gc;
GdkGC *thin_line_gc;
GdkGC *thick_line_gc;
GdkGC *border_line_gc;
GdkGC *selection_gc;
GdkPixmap *gray50, *gray25, *black50;
GdkPixmap *mask_bitmap;

GtkWidget *main_frame_civ_name;
GtkWidget *main_label_info;

GtkWidget *avbox, *ahbox, *vbox, *conn_box;
GtkWidget* scroll_panel;

GtkTreeStore *connection_list_store;
GtkTreeView *connection_list_view;

GtkWidget *econ_label[10];
GtkWidget *bulb_label;
GtkWidget *sun_label;
GtkWidget *flake_label;
GtkWidget *government_label;
GtkWidget *timeout_label;
GtkWidget *turn_done_button;

GtkWidget *unit_info_label;
GtkWidget *unit_info_box;
GtkWidget *unit_info_frame;

GtkTooltips *main_tips;
GtkWidget *econ_ebox;
GtkWidget *bulb_ebox;
GtkWidget *sun_ebox;
GtkWidget *flake_ebox;
GtkWidget *government_ebox;

const char * const gui_character_encoding = "UTF-8";
const bool gui_use_transliteration = FALSE;

static GtkWidget *main_menubar;
static GtkWidget *unit_pixmap_table;
static GtkWidget *unit_pixmap;
static GtkWidget *unit_pixmap_button;
static GtkWidget *unit_below_pixmap[MAX_NUM_UNITS_BELOW];
static GtkWidget *unit_below_pixmap_button[MAX_NUM_UNITS_BELOW];
static GtkWidget *more_arrow_pixmap;

static int unit_id_top;
static int unit_ids[MAX_NUM_UNITS_BELOW];  /* ids of the units icons in 
                                            * information display: (or 0) */
GtkTextView *main_message_area;
GtkTextBuffer *message_buffer;
static GtkWidget *allied_chat_toggle_button;

static enum Display_color_type display_color_type;  /* practically unused */
static gint timer_id;                               /*       ditto        */
static guint input_id, ggz_input_id;
gint cur_x, cur_y;


static gboolean show_info_button_release(GtkWidget *w, GdkEventButton *ev, gpointer data);
static gboolean show_info_popup(GtkWidget *w, GdkEventButton *ev, gpointer data);

static void end_turn_callback(GtkWidget *w, gpointer data);
static void get_net_input(gpointer data, gint fid, GdkInputCondition condition);
static void set_wait_for_writable_socket(struct connection *pc,
                                         bool socket_writable);

static void print_usage(const char *argv0);
static void parse_options(int argc, char **argv);
static gboolean toplevel_key_press_handler(GtkWidget *w, GdkEventKey *ev, gpointer data);
static gboolean toplevel_key_release_handler(GtkWidget *w, GdkEventKey *ev, gpointer data);
static gboolean mouse_scroll_mapcanvas(GtkWidget *w, GdkEventScroll *ev);

static void tearoff_callback(GtkWidget *b, gpointer data);
static GtkWidget *detached_widget_new(void);
static GtkWidget *detached_widget_fill(GtkWidget *ahbox);

static gboolean select_unit_pixmap_callback(GtkWidget *w, GdkEvent *ev, 
					    gpointer data);
static gboolean quit_dialog_callback(void);

static void allied_chat_button_toggled(GtkToggleButton *button,
                                       gpointer user_data);

/****************************************************************************
  Called by the tileset code to set the font size that should be used to
  draw the city names and productions.
****************************************************************************/
void set_city_names_font_sizes(int my_city_names_font_size,
			       int my_city_productions_font_size)
{
  /* This function may be called before the fonts are allocated.  So we
   * save the values for later. */
  city_names_font_size = my_city_names_font_size;
  city_productions_font_size = my_city_productions_font_size;
  if (main_font) {
    pango_font_description_set_size(main_font,
				    PANGO_SCALE * city_names_font_size);
    pango_font_description_set_size(city_productions_font,
				    PANGO_SCALE * city_productions_font_size);
  }
}

/**************************************************************************
...
**************************************************************************/
static void log_callback_utf8(int level, const char *message, bool file_too)
{
  if (! file_too || level <= LOG_FATAL) {
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

  timer_id = g_timeout_add(seconds * 1000, timer_callback, NULL);

  return FALSE;
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
...
**************************************************************************/
static gboolean toplevel_focus(GtkWidget *w, GtkDirectionType arg)
{
  switch (arg) {
    case GTK_DIR_TAB_FORWARD:
    case GTK_DIR_TAB_BACKWARD:
      
      if (!GTK_WIDGET_CAN_FOCUS(w)) {
	return FALSE;
      }

      if (!gtk_widget_is_focus(w)) {
	gtk_widget_grab_focus(w);
	return TRUE;
      }
      break;

    default:
      break;
  }
  return FALSE;
}

/**************************************************************************
  When the chatline text view is resized, scroll it to the bottom. This
  prevents users from accidentally missing messages when the chatline
  gets scrolled up a small amount and stops scrolling down automatically.
**************************************************************************/
static void main_message_area_size_allocate(GtkWidget *widget,
                                            GtkAllocation *allocation,
                                            gpointer data)
{
  static int old_width = 0, old_height = 0;
  if (allocation->width != old_width
      || allocation->height != old_height) {
    chatline_scroll_to_bottom(TRUE);
    old_width = allocation->width;
    old_height = allocation->height;
  }
}

/**************************************************************************
...
**************************************************************************/
gboolean map_canvas_focus(void)
{
  gtk_window_present(GTK_WINDOW(toplevel));
  gtk_notebook_set_current_page(GTK_NOTEBOOK(top_notebook), 0);
  gtk_widget_grab_focus(map_canvas);
  return TRUE;
}

/**************************************************************************
  In GTK+ keyboard events are recursively propagated from the hierarchy
  parent down to its children. Sometimes this is not what we want.
  E.g. The inputline is active, the user presses the 's' key, we want it
  to be sent to the inputline, but because the main menu is further up
  the hierarchy, it wins and the inputline never gets anything!
  This function ensures an entry widget (like the inputline) always gets
  first dibs at handling a keyboard event.
**************************************************************************/
static gboolean toplevel_handler(GtkWidget *w, GdkEventKey *ev, gpointer data)
{
  GtkWidget *focus;

  focus = gtk_window_get_focus(GTK_WINDOW(toplevel));
  if (focus) {
    if (GTK_IS_ENTRY(focus)
        || (GTK_IS_TEXT_VIEW(focus)
            && gtk_text_view_get_editable(GTK_TEXT_VIEW(focus)))) {
      /* Propagate event to currently focused entry widget. */
      if (gtk_widget_event(focus, (GdkEvent *) ev)) {
	/* Do not propagate event to our children. */
	return TRUE;
      }
    }
  }

  /* Continue propagating event to our children. */
  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static gboolean key_press_map_canvas(GtkWidget *w, GdkEventKey *ev,
                                     gpointer data)
{
  if ((ev->state & GDK_SHIFT_MASK)) {
    switch (ev->keyval) {

    case GDK_Left:
      scroll_mapview(DIR8_WEST);
      return TRUE;

    case GDK_Right:
      scroll_mapview(DIR8_EAST);
      return TRUE;

    case GDK_Up:
      scroll_mapview(DIR8_NORTH);
      return TRUE;

    case GDK_Down:
      scroll_mapview(DIR8_SOUTH);
      return TRUE;

    case GDK_Home:
      key_center_capital();
      return TRUE;

    case GDK_Page_Up:
      g_signal_emit_by_name(main_message_area, "move_cursor",
	                          GTK_MOVEMENT_PAGES, -1, FALSE);
      return TRUE;

    case GDK_Page_Down:
      g_signal_emit_by_name(main_message_area, "move_cursor",
	                          GTK_MOVEMENT_PAGES, 1, FALSE);
      return TRUE;

    default:
      break;
    };
  }

  /* Return here if observer */
  if (client_is_observer()) {
    return FALSE;
  }

  assert(MAX_NUM_BATTLEGROUPS == 4);
  
  if ((ev->state & GDK_CONTROL_MASK)) {
    switch (ev->keyval) {

    case GDK_F1:
      key_unit_assign_battlegroup(0, (ev->state & GDK_SHIFT_MASK));
      return TRUE;

    case GDK_F2:
      key_unit_assign_battlegroup(1, (ev->state & GDK_SHIFT_MASK));
      return TRUE;

    case GDK_F3:
      key_unit_assign_battlegroup(2, (ev->state & GDK_SHIFT_MASK));
      return TRUE;

    case GDK_F4:
      key_unit_assign_battlegroup(3, (ev->state & GDK_SHIFT_MASK));
      return TRUE;

    default:
      break;
    };
  } else if ((ev->state & GDK_SHIFT_MASK)) {
    switch (ev->keyval) {

    case GDK_F1:
      key_unit_select_battlegroup(0, FALSE);
      return TRUE;

    case GDK_F2:
      key_unit_select_battlegroup(1, FALSE);
      return TRUE;

    case GDK_F3:
      key_unit_select_battlegroup(2, FALSE);
      return TRUE;

    case GDK_F4:
      key_unit_select_battlegroup(3, FALSE);
      return TRUE;

    default:
      break;
    };
  }

  switch (ev->keyval) {

  case GDK_KP_Up:
  case GDK_KP_8:
  case GDK_Up:
  case GDK_8:
    key_unit_move(DIR8_NORTH);
    return TRUE;

  case GDK_KP_Page_Up:
  case GDK_KP_9:
  case GDK_Page_Up:
  case GDK_9:
    key_unit_move(DIR8_NORTHEAST);
    return TRUE;

  case GDK_KP_Right:
  case GDK_KP_6:
  case GDK_Right:
  case GDK_6:
    key_unit_move(DIR8_EAST);
    return TRUE;

  case GDK_KP_Page_Down:
  case GDK_KP_3:
  case GDK_Page_Down:
  case GDK_3:
    key_unit_move(DIR8_SOUTHEAST);
    return TRUE;

  case GDK_KP_Down:
  case GDK_KP_2:
  case GDK_Down:
  case GDK_2:
    key_unit_move(DIR8_SOUTH);
    return TRUE;

  case GDK_KP_End:
  case GDK_KP_1:
  case GDK_End:
  case GDK_1:
    key_unit_move(DIR8_SOUTHWEST);
    return TRUE;

  case GDK_KP_Left:
  case GDK_KP_4:
  case GDK_Left:
  case GDK_4:
    key_unit_move(DIR8_WEST);
    return TRUE;

  case GDK_KP_Home:
  case GDK_KP_7:
  case GDK_Home:
  case GDK_7:
    key_unit_move(DIR8_NORTHWEST);
    return TRUE;

  case GDK_KP_Begin:
  case GDK_KP_5: 
  case GDK_5:
    key_recall_previous_focus_unit(); 
    return TRUE;

  case GDK_Escape:
    key_cancel_action();
    return TRUE;

  case GDK_b:
    if (tiles_hilited_cities) {
      buy_production_in_selected_cities();
      return TRUE;
    }
    break;

  default:
    break;
  };

  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static gboolean toplevel_key_release_handler(GtkWidget *w, GdkEventKey *ev,
                                             gpointer data)
{
  /* inputline history code */
  if (!GTK_WIDGET_MAPPED(top_vbox) || inputline_has_focus()) {
    return FALSE;
  }

  if (editor_is_active()) {
    return handle_edit_key_release(ev);
  }

  return FALSE;
}

/**************************************************************************
  Handle a keyboard key press made in the client's toplevel window.
**************************************************************************/
static gboolean toplevel_key_press_handler(GtkWidget *w, GdkEventKey *ev,
                                           gpointer data)
{
  /* inputline history code */
  if (!GTK_WIDGET_MAPPED(top_vbox) || inputline_has_focus()) {
    return FALSE;
  }

  if (editor_is_active()) {
    return handle_edit_key_press(ev);
  }

  if ((ev->state & GDK_SHIFT_MASK)) {
    switch (ev->keyval) {

    case GDK_Return:
    case GDK_KP_Enter:
      key_end_turn();
      return TRUE;

    default:
      break;
    };
  } else {
  }

  switch (ev->keyval) {

  case GDK_apostrophe:
    /* FIXME: should find the correct window, even when detached, from any
     * other window; should scroll to the bottom automatically showing the
     * latest text from other players; MUST NOT make spurious text windows
     * at the bottom of other dialogs.
     */
    gtk_notebook_set_current_page(GTK_NOTEBOOK(bottom_notebook), 0);
    inputline_grab_focus();
    return TRUE;

  default:
    break;
  };

  if (GTK_WIDGET_HAS_FOCUS(map_canvas)) {
    return key_press_map_canvas(w, ev, data);
  }

#if 0
  /* We are focused some other dialog, tab, or widget. */
  if ((ev->state & GDK_CONTROL_MASK)) {
  } else if ((ev->state & GDK_SHIFT_MASK)) {
  } else {
    switch (ev->keyval) {

    case GDK_F4:
      map_canvas_focus();
      return TRUE;

    default:
      break;
    };
  }
#endif

  return FALSE;
}

/**************************************************************************
Mouse/touchpad scrolling over the mapview
**************************************************************************/
static gboolean mouse_scroll_mapcanvas(GtkWidget *w, GdkEventScroll *ev)
{
  int scroll_x, scroll_y, xstep, ystep;

  if (!can_client_change_view()) {
    return FALSE;
  }

  get_mapview_scroll_pos(&scroll_x, &scroll_y);
  get_mapview_scroll_step(&xstep, &ystep);

  switch (ev->direction) {
    case GDK_SCROLL_UP:
      scroll_y -= ystep*2;
      break;
    case GDK_SCROLL_DOWN:
      scroll_y += ystep*2;
      break;
    case GDK_SCROLL_RIGHT:
      scroll_x += xstep*2;
      break;
    case GDK_SCROLL_LEFT:
      scroll_x -= xstep*2;
      break;
    default:
      return FALSE;
  };

  set_mapview_scroll_pos(scroll_x, scroll_y);

  // Emulating mouse move now
  if (!GTK_WIDGET_HAS_FOCUS(map_canvas)) {
    gtk_widget_grab_focus(map_canvas);
  }

  update_line(cur_x, cur_y);
  update_rect_at_mouse_pos();

  if (keyboardless_goto_button_down && hover_state == HOVER_NONE) {
    maybe_activate_keyboardless_goto(cur_x, cur_y);
  }

  control_mouse_cursor(canvas_pos_to_tile(cur_x, cur_y));

  return TRUE;
}

/**************************************************************************
 reattaches the detached widget when the user destroys it.
**************************************************************************/
static void tearoff_destroy(GtkWidget *w, gpointer data)
{
  GtkWidget *p, *b, *box;

  box = GTK_WIDGET(data);
  p = g_object_get_data(G_OBJECT(w), "parent");
  b = g_object_get_data(G_OBJECT(w), "toggle");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(b), FALSE);

  gtk_widget_hide(w);
  gtk_widget_reparent(box, p);
}

/**************************************************************************
 propagates a keypress in a tearoff back to the toplevel window.
**************************************************************************/
static gboolean propagate_keypress(GtkWidget *w, GdkEventKey *ev)
{
  gtk_widget_event(toplevel, (GdkEvent *)ev);
  return FALSE;
}

/**************************************************************************
 callback for the toggle button in the detachable widget: causes the
 widget to detach or reattach.
**************************************************************************/
static void tearoff_callback(GtkWidget *b, gpointer data)
{
  GtkWidget *box = GTK_WIDGET(data);
  GtkWidget *w;

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b))) {
    w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_dialog(w, toplevel);
    gtk_widget_set_name(w, "Freeciv");
    gtk_window_set_title(GTK_WINDOW(w), _("Freeciv"));
    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_MOUSE);
    g_signal_connect(w, "destroy", G_CALLBACK(tearoff_destroy), box);
    g_signal_connect(w, "key_press_event",
	G_CALLBACK(propagate_keypress), NULL);


    g_object_set_data(G_OBJECT(w), "parent", box->parent);
    g_object_set_data(G_OBJECT(w), "toggle", b);
    gtk_widget_reparent(box, w);
    gtk_widget_show(w);
  } else {
    gtk_widget_destroy(box->parent);
  }
}

/**************************************************************************
 create the container for the widget that's able to be detached
**************************************************************************/
static GtkWidget *detached_widget_new(void)
{
  return gtk_hbox_new(FALSE, 2);
}

/**************************************************************************
 creates the toggle button necessary to detach and reattach the widget
 and returns a vbox in which you fill your goodies.
**************************************************************************/
static GtkWidget *detached_widget_fill(GtkWidget *ahbox)
{
  GtkWidget *b, *avbox;

  b = gtk_toggle_button_new();
  gtk_box_pack_start(GTK_BOX(ahbox), b, FALSE, FALSE, 0);
  g_signal_connect(b, "toggled", G_CALLBACK(tearoff_callback), ahbox);

  avbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(ahbox), avbox, TRUE, TRUE, 0);
  return avbox;
}

/**************************************************************************
  Called to build the unit_below pixmap table.  This is the table on the
  left of the screen that shows all of the inactive units in the current
  tile.

  It may be called again if the tileset changes.
**************************************************************************/
static void populate_unit_pixmap_table(void)
{
  int i;
  GtkWidget *table = unit_pixmap_table;
 
  /* 135 below is rough value (could be more intelligent) --dwp */
  num_units_below = 135 / (int) tileset_tile_width(tileset);
  num_units_below = CLIP(1, num_units_below, MAX_NUM_UNITS_BELOW);

  gtk_table_resize(GTK_TABLE(table), 2, num_units_below);

  /* Note, we ref this and other widgets here so that we can unref them
   * in reset_unit_table. */
  unit_pixmap = gtk_pixcomm_new(tileset_unit_width(tileset), tileset_unit_height(tileset));
  gtk_widget_ref(unit_pixmap);
  gtk_pixcomm_clear(GTK_PIXCOMM(unit_pixmap));
  unit_pixmap_button = gtk_event_box_new();
  gtk_widget_ref(unit_pixmap_button);
  gtk_container_add(GTK_CONTAINER(unit_pixmap_button), unit_pixmap);
  gtk_table_attach_defaults(GTK_TABLE(table), unit_pixmap_button, 0, 1, 0, 1);
  g_signal_connect(unit_pixmap_button, "button_press_event",
		   G_CALLBACK(select_unit_pixmap_callback), 
		   GINT_TO_POINTER(-1));

  for (i = 0; i < num_units_below; i++) {
    unit_below_pixmap[i] = gtk_pixcomm_new(tileset_unit_width(tileset),
                                           tileset_unit_height(tileset));
    gtk_widget_ref(unit_below_pixmap[i]);
    unit_below_pixmap_button[i] = gtk_event_box_new();
    gtk_widget_ref(unit_below_pixmap_button[i]);
    gtk_container_add(GTK_CONTAINER(unit_below_pixmap_button[i]),
                      unit_below_pixmap[i]);
    g_signal_connect(unit_below_pixmap_button[i],
		     "button_press_event",
		     G_CALLBACK(select_unit_pixmap_callback),
		     GINT_TO_POINTER(i));
      
    gtk_table_attach_defaults(GTK_TABLE(table), unit_below_pixmap_button[i],
                              i, i + 1, 1, 2);
    gtk_pixcomm_clear(GTK_PIXCOMM(unit_below_pixmap[i]));
  }

  more_arrow_pixmap
    = gtk_image_new_from_pixbuf(sprite_get_pixbuf(get_arrow_sprite(tileset,
						   ARROW_RIGHT)));
  gtk_widget_ref(more_arrow_pixmap);
  gtk_table_attach_defaults(GTK_TABLE(table), more_arrow_pixmap, 4, 5, 1, 2);

  gtk_widget_show_all(table);
}

/**************************************************************************
  Called when the tileset is changed to reset the unit pixmap table.
**************************************************************************/
void reset_unit_table(void)
{
  int i;

  if (unit_pixmap_button) {
    /* Unreference all of the widgets that we're about to reallocate, thus
     * avoiding a memory leak. Remove them from the container first, just
     * to be safe. Note, the widgets are ref'd in
     * populatate_unit_pixmap_table. */
    gtk_container_remove(GTK_CONTAINER(unit_pixmap_table),
                         unit_pixmap_button);
    gtk_widget_unref(unit_pixmap);
    gtk_widget_unref(unit_pixmap_button);
    for (i = 0; i < num_units_below; i++) {
      gtk_container_remove(GTK_CONTAINER(unit_pixmap_table),
                           unit_below_pixmap_button[i]);
      gtk_widget_unref(unit_below_pixmap[i]);
      gtk_widget_unref(unit_below_pixmap_button[i]);
    }
    gtk_container_remove(GTK_CONTAINER(unit_pixmap_table),
                         more_arrow_pixmap);
    gtk_widget_unref(more_arrow_pixmap);
  }

  populate_unit_pixmap_table();

  /* We have to force a redraw of the units.  And we explicitly have
   * to force a redraw of the focus unit, which is normally only
   * redrawn when the focus changes. We also have to force the 'more'
   * arrow to go away, both by expicitly hiding it and telling it to
   * do so (this will be reset immediately afterwards if necessary,
   * but we have to make the *internal* state consistent). */
  gtk_widget_hide(more_arrow_pixmap);
  set_unit_icons_more_arrow(FALSE);
  if (get_num_units_in_focus() == 1) {
    set_unit_icon(-1, head_of_units_in_focus());
  } else {
    set_unit_icon(-1, NULL);
  }
  update_unit_pix_label(get_units_in_focus());
}

/**************************************************************************
  Enable/Disable the game page menu bar.
**************************************************************************/
void enable_menus(bool enable)
{
  if (enable) {
    main_menubar = setup_menus(toplevel);
    gtk_box_pack_start(GTK_BOX(top_vbox), main_menubar, FALSE, FALSE, 0);
    update_menus();
    gtk_widget_show_all(main_menubar);
  } else {
    gtk_widget_destroy(main_menubar);
  }
}

/**************************************************************************
  Workaround for a crash that occurs when a button release event is
  emitted for a notebook with no pages. See PR#40743.
  FIXME: Remove this hack once gtk_notebook_button_release() in
  gtk/gtknotebook.c checks for NULL notebook->cur_page.
**************************************************************************/
static gboolean right_notebook_button_release(GtkWidget *widget,
                                              GdkEventButton *event)
{
  if (event->type != GDK_BUTTON_RELEASE) {
    return FALSE;
  }

  if (!GTK_IS_NOTEBOOK(widget)
      || -1 == gtk_notebook_get_current_page(GTK_NOTEBOOK(widget))) {
    /* Make sure the default gtk handler
     * does NOT get called in this case. */
    return TRUE;
  }

  return FALSE;
}

/**************************************************************************
 do the heavy lifting for the widget setup.
**************************************************************************/
static void setup_widgets(void)
{
  GtkWidget *page, *box, *ebox, *hbox, *sbox, *align, *label;
  GtkWidget *frame, *table, *table2, *paned, *hpaned, *sw, *text;
  GtkWidget *button, *view;
  int i;
  char buf[256];
  struct sprite *sprite;

  GtkWidget *notebook, *statusbar;

  message_buffer = gtk_text_buffer_new(NULL);


  notebook = gtk_notebook_new();

  /* stop mouse wheel notebook page switching. */
  g_signal_connect(notebook, "scroll_event",
		   G_CALLBACK(gtk_true), NULL);

  toplevel_tabs = notebook;
  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
  gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
  box = gtk_vbox_new(FALSE, 4);
  gtk_container_add(GTK_CONTAINER(toplevel), box);
  gtk_box_pack_start(GTK_BOX(box), notebook, TRUE, TRUE, 0);
  statusbar = create_statusbar();
  gtk_box_pack_start(GTK_BOX(box), statusbar, FALSE, FALSE, 0);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
      create_main_page(), NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
      create_start_page(), NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
      create_scenario_page(), NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
      create_load_page(), NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
      create_network_page(), NULL);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
      create_nation_page(), NULL);

  main_tips = gtk_tooltips_new();

  editgui_create_widgets();

  /* *** everything in the top *** */

  page = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(page),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(page),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, NULL);

  top_vbox = gtk_vbox_new(FALSE, 5);
  hbox = gtk_hbox_new(FALSE, 0);
  paned = gtk_vpaned_new();

  if (gui_gtk2_small_display_layout) {
    /* The window is divided into two horizontal panels: overview +
     * civinfo + unitinfo, main view + message window. */
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(page),
                                          top_vbox);
    gtk_box_pack_end(GTK_BOX(top_vbox), hbox, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hbox), paned, TRUE, TRUE, 0);
  } else {
    /* The window is divided into two vertical panes: overview +
     * + civinfo + unitinfo + main view, message window. */
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(page), paned);
    gtk_paned_pack1(GTK_PANED(paned), top_vbox, TRUE, FALSE);
    gtk_box_pack_end(GTK_BOX(top_vbox), hbox, TRUE, TRUE, 0);
  }

#ifdef GGZ_GTK
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
			   ggz_gtk_create_main_area(toplevel), NULL);
#endif

  /* this holds the overview canvas, production info, etc. */
  vbox = gtk_vbox_new(FALSE, 3);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

  /* overview canvas */
  ahbox = detached_widget_new();
  gtk_box_pack_start(GTK_BOX(vbox), ahbox, FALSE, FALSE, 0);
  avbox = detached_widget_fill(ahbox);

  align = gtk_alignment_new(0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(avbox), align, FALSE, FALSE, 0);

  overview_canvas = gtk_drawing_area_new();

  gtk_widget_add_events(overview_canvas, GDK_EXPOSURE_MASK
        			        |GDK_BUTTON_PRESS_MASK
				        |GDK_POINTER_MOTION_MASK);

  gtk_widget_set_size_request(overview_canvas, 160, 100);
  gtk_container_add(GTK_CONTAINER(align), overview_canvas);

  g_signal_connect(overview_canvas, "expose_event",
        	   G_CALLBACK(overview_canvas_expose), NULL);

  g_signal_connect(overview_canvas, "motion_notify_event",
        	   G_CALLBACK(move_overviewcanvas), NULL);

  g_signal_connect(overview_canvas, "button_press_event",
        	   G_CALLBACK(butt_down_overviewcanvas), NULL);

  /* The rest */

  ahbox = detached_widget_new();
  gtk_box_pack_start(GTK_BOX(vbox), ahbox, TRUE, TRUE, 0);
  avbox = detached_widget_fill(ahbox);

  /* Info on player's civilization, when game is running. */
  frame = gtk_frame_new("");
  gtk_box_pack_start(GTK_BOX(avbox), frame, FALSE, FALSE, 0);

  main_frame_civ_name = frame;

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  ebox = gtk_event_box_new();
  gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(ebox, "button_press_event",
                   G_CALLBACK(show_info_popup), NULL);
  gtk_box_pack_start(GTK_BOX(vbox), ebox, FALSE, FALSE, 0);

  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_misc_set_padding(GTK_MISC(label), 2, 2);
  gtk_container_add(GTK_CONTAINER(ebox), label);
  main_label_info = label;

  /* Production status */

  /* make a box so the table will be centered */
  box = gtk_hbox_new(FALSE, 0);
  
  gtk_box_pack_start(GTK_BOX(avbox), box, FALSE, FALSE, 0);

  table = gtk_table_new(3, 10, TRUE);
  gtk_table_set_row_spacing(GTK_TABLE(table), 0, 0);
  gtk_table_set_col_spacing(GTK_TABLE(table), 0, 0);
  gtk_box_pack_start(GTK_BOX(box), table, TRUE, FALSE, 0);

  /* citizens for taxrates */
  ebox = gtk_event_box_new();
  gtk_table_attach_defaults(GTK_TABLE(table), ebox, 0, 10, 0, 1);
  econ_ebox = ebox;
  
  table2 = gtk_table_new(1, 10, TRUE);
  gtk_table_set_row_spacing(GTK_TABLE(table2), 0, 0);
  gtk_table_set_col_spacing(GTK_TABLE(table2), 0, 0);
  gtk_container_add(GTK_CONTAINER(ebox), table2);
  
  for (i = 0; i < 10; i++) {
    ebox = gtk_event_box_new();
    gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);

    gtk_table_attach_defaults(GTK_TABLE(table2), ebox, i, i + 1, 0, 1);

    g_signal_connect(ebox, "button_press_event",
                     G_CALLBACK(taxrates_callback), GINT_TO_POINTER(i));

    sprite = i < 5 ? get_tax_sprite(tileset, O_SCIENCE) : get_tax_sprite(tileset, O_GOLD);
    econ_label[i] = gtk_image_new_from_pixbuf(sprite_get_pixbuf(sprite));
    gtk_container_add(GTK_CONTAINER(ebox), econ_label[i]);
  }

  /* science, environmental, govt, timeout */
  bulb_label
    = gtk_image_new_from_pixbuf(sprite_get_pixbuf(client_research_sprite()));
  sun_label
    = gtk_image_new_from_pixbuf(sprite_get_pixbuf(client_warming_sprite()));
  flake_label
    = gtk_image_new_from_pixbuf(sprite_get_pixbuf(client_cooling_sprite()));
  government_label
    = gtk_image_new_from_pixbuf(sprite_get_pixbuf
				(client_government_sprite()));

  for (i = 0; i < 4; i++) {
    GtkWidget *w;
    
    ebox = gtk_event_box_new();

    switch (i) {
    case 0:
      w = bulb_label;
      bulb_ebox = ebox;
      break;
    case 1:
      w = sun_label;
      sun_ebox = ebox;
      break;
    case 2:
      w = flake_label;
      flake_ebox = ebox; 
      break;
    default:
    case 3:
      w = government_label;
      government_ebox = ebox;
      break;
    }

    gtk_misc_set_alignment(GTK_MISC(w), 0.0, 0.0);
    gtk_misc_set_padding(GTK_MISC(w), 0, 0);
    gtk_container_add(GTK_CONTAINER(ebox), w);
    gtk_table_attach_defaults(GTK_TABLE(table), ebox, i, i + 1, 1, 2);
  }

  timeout_label = gtk_label_new("");

  frame = gtk_frame_new(NULL);
  gtk_table_attach_defaults(GTK_TABLE(table), frame, 4, 10, 1, 2);
  gtk_container_add(GTK_CONTAINER(frame), timeout_label);


  /* turn done */
  turn_done_button = gtk_button_new_with_label(_("Turn Done"));

  gtk_table_attach_defaults(GTK_TABLE(table), turn_done_button, 0, 10, 2, 3);

  g_signal_connect(turn_done_button, "clicked",
                   G_CALLBACK(end_turn_callback), NULL);

  my_snprintf(buf, sizeof(buf), "%s:\n%s", _("Turn Done"), _("Shift+Return"));
  gtk_tooltips_set_tip(main_tips, turn_done_button, buf, "");

  /* Selected unit status */

  unit_info_box = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(avbox), unit_info_box, FALSE, FALSE, 0);

  /* In edit mode the unit_info_box widget is replaced by the
   * editinfobox, so we need to add a ref here so that it is
   * not destroyed when removed from its container.
   * See editinfobox_refresh(). */
  g_object_ref(unit_info_box);

  unit_info_frame = gtk_frame_new("");
  gtk_box_pack_start(GTK_BOX(unit_info_box), unit_info_frame, FALSE, FALSE, 0);
    
  label = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_misc_set_padding(GTK_MISC(label), 2, 2);
  gtk_container_add(GTK_CONTAINER(unit_info_frame), label);
  unit_info_label = label;

  box = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start(GTK_BOX(unit_info_box), box, FALSE, FALSE, 0);

  table = gtk_table_new(0, 0, FALSE);
  gtk_box_pack_start(GTK_BOX(box), table, FALSE, FALSE, 5);

  gtk_table_set_row_spacings(GTK_TABLE(table), 2);
  gtk_table_set_col_spacings(GTK_TABLE(table), 2);

  unit_pixmap_table = table;

  /* Map canvas, editor toolbar, and scrollbars */

  /* The top notebook containing the map view and dialogs. */

  top_notebook = gtk_notebook_new();  
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(top_notebook), GTK_POS_BOTTOM);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(top_notebook), TRUE);

  if (gui_gtk2_small_display_layout) {
    gtk_paned_pack1(GTK_PANED(paned), top_notebook, TRUE, TRUE);
  } else {
    gtk_box_pack_start(GTK_BOX(hbox), top_notebook, TRUE, TRUE, 0);
  }

  map_widget = gtk_table_new(2, 2, FALSE);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), map_widget, TRUE, TRUE, 0);

  gtk_box_pack_start(GTK_BOX(vbox),
                     editgui_get_editbar()->widget,
                     FALSE, FALSE, 4);

  label = gtk_label_new(_("View"));
  gtk_notebook_append_page(GTK_NOTEBOOK(top_notebook), vbox, label);

  frame = gtk_frame_new(NULL);
  gtk_table_attach(GTK_TABLE(map_widget), frame, 0, 1, 0, 1,
                   GTK_EXPAND|GTK_SHRINK|GTK_FILL,
                   GTK_EXPAND|GTK_SHRINK|GTK_FILL, 0, 0);

  map_canvas = gtk_drawing_area_new();
  gtk_widget_set_size_request(map_canvas, 300, 300);
  GTK_WIDGET_SET_FLAGS(map_canvas, GTK_CAN_FOCUS);

  for (i = 0; i < 5; i++) {
    gtk_widget_modify_bg(GTK_WIDGET(overview_canvas), i,
			 &get_color(tileset, COLOR_OVERVIEW_UNKNOWN)->color);
    gtk_widget_modify_bg(GTK_WIDGET(map_canvas), i,
			 &get_color(tileset, COLOR_MAPVIEW_UNKNOWN)->color);
  }

  gtk_widget_add_events(map_canvas, GDK_EXPOSURE_MASK
                                   |GDK_BUTTON_PRESS_MASK
                                   |GDK_BUTTON_RELEASE_MASK
                                   |GDK_KEY_PRESS_MASK
                                   |GDK_POINTER_MOTION_MASK
                                   |GDK_SCROLL_MASK);

  gtk_container_add(GTK_CONTAINER(frame), map_canvas);

  map_horizontal_scrollbar = gtk_hscrollbar_new(NULL);
  gtk_table_attach(GTK_TABLE(map_widget), map_horizontal_scrollbar, 0, 1, 1, 2,
                   GTK_EXPAND|GTK_SHRINK|GTK_FILL, 0, 0, 0);

  map_vertical_scrollbar = gtk_vscrollbar_new(NULL);
  gtk_table_attach(GTK_TABLE(map_widget), map_vertical_scrollbar, 1, 2, 0, 1,
                   0, GTK_EXPAND|GTK_SHRINK|GTK_FILL, 0, 0);

  g_signal_connect(map_canvas, "expose_event",
                   G_CALLBACK(map_canvas_expose), NULL);

  g_signal_connect(map_canvas, "configure_event",
                   G_CALLBACK(map_canvas_configure), NULL);

  g_signal_connect(map_canvas, "motion_notify_event",
                   G_CALLBACK(move_mapcanvas), NULL);

  g_signal_connect(toplevel, "enter_notify_event",
                   G_CALLBACK(leave_mapcanvas), NULL);

  g_signal_connect(map_canvas, "button_press_event",
                   G_CALLBACK(butt_down_mapcanvas), NULL);

  g_signal_connect(map_canvas, "button_release_event",
                   G_CALLBACK(butt_release_mapcanvas), NULL);

  g_signal_connect(map_canvas, "scroll_event",
                   G_CALLBACK(mouse_scroll_mapcanvas), NULL);

  g_signal_connect(toplevel, "key_press_event",
                   G_CALLBACK(toplevel_key_press_handler), NULL);

  g_signal_connect(toplevel, "key_release_event",
                   G_CALLBACK(toplevel_key_release_handler), NULL);

  /* *** The message window -- this is a detachable widget *** */

  sbox = detached_widget_new();
  gtk_paned_pack2(GTK_PANED(paned), sbox, TRUE, TRUE);
  avbox = detached_widget_fill(sbox);

  vbox = gtk_vbox_new(FALSE, 0);
  if (ingame_votebar == NULL) {
    ingame_votebar = voteinfo_bar_new();
  }
  gtk_box_pack_start(GTK_BOX(vbox), ingame_votebar, FALSE, FALSE, 2);
  gtk_box_pack_start(GTK_BOX(avbox), vbox, TRUE, TRUE, 0);

  hpaned = gtk_hpaned_new();
  gtk_box_pack_start(GTK_BOX(vbox), hpaned, TRUE, TRUE, 4);
  bottom_hpaned = hpaned;

  bottom_notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(bottom_notebook), GTK_POS_TOP);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(bottom_notebook), TRUE);
  gtk_paned_pack1(GTK_PANED(hpaned), bottom_notebook, TRUE, TRUE);

  right_notebook = gtk_notebook_new();
  g_object_ref(right_notebook);
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(right_notebook), GTK_POS_TOP);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(right_notebook), TRUE);
  g_signal_connect(right_notebook, "button-release-event",
                   G_CALLBACK(right_notebook_button_release), NULL);
  if (gui_gtk2_split_bottom_notebook) {
    gtk_paned_pack2(GTK_PANED(hpaned), right_notebook, TRUE, TRUE);
  }

  vbox = gtk_vbox_new(FALSE, 0);

  sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(sw),
				      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_AUTOMATIC,
  				 GTK_POLICY_ALWAYS);
  gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);

  label = gtk_label_new(_("Chat"));
  gtk_notebook_append_page(GTK_NOTEBOOK(bottom_notebook), vbox, label);

  text = gtk_text_view_new_with_buffer(message_buffer);
  set_message_buffer_view_link_handlers(text);
  gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
  gtk_container_add(GTK_CONTAINER(sw), text);
  g_signal_connect(text, "size-allocate",
                   G_CALLBACK(main_message_area_size_allocate), NULL);

  gtk_widget_set_name(text, "chatline");

  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD);
  gtk_widget_realize(text);
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 5);

  main_message_area = GTK_TEXT_VIEW(text);

  chat_welcome_message();

  /* the chat line */
  view = inputline_toolkit_view_new();
  gtk_box_pack_start(GTK_BOX(vbox), view, FALSE, FALSE, 3);

  button = gtk_toggle_button_new_with_label(_("Allies Only"));
  gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                               gui_gtk2_allied_chat_only);
  g_signal_connect(button, "toggled",
                   G_CALLBACK(allied_chat_button_toggled), NULL);
  inputline_toolkit_view_append_button(view, button);
  allied_chat_toggle_button = button;

  button = gtk_button_new_with_label(_("Clear links"));
  g_signal_connect(button, "clicked",
                   G_CALLBACK(link_marks_clear_all), NULL);
  inputline_toolkit_view_append_button(view, button);

  /* Other things to take care of */

  gtk_widget_show_all(gtk_bin_get_child(GTK_BIN(toplevel)));

  if (gui_gtk2_enable_tabs) {
    popup_meswin_dialog(FALSE);
  }

  gtk_notebook_set_current_page(GTK_NOTEBOOK(top_notebook), 0);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(bottom_notebook), 0);

  if (!gui_gtk2_map_scrollbars) {
    gtk_widget_hide(map_horizontal_scrollbar);
    gtk_widget_hide(map_vertical_scrollbar);
  }
}

#ifdef GGZ_GTK
/****************************************************************************
  Callback function that's called by the library when a connection is
  established (or lost) to the GGZ server.  The server parameter gives
  the server (or NULL).
****************************************************************************/
static void ggz_connected(GGZServer *server)
{
  in_ggz = (server != NULL);
  set_client_page(in_ggz ? PAGE_GGZ : PAGE_MAIN);
}

/****************************************************************************
  Callback function that's called by the library when we launch a game.  This
  means we now have a connection to a freeciv server so handling can be given
  back to the regular freeciv code.
****************************************************************************/
static void ggz_game_launched(void)
{
  ggz_begin();
}

/****************************************************************************
  Callback function that's invoked when GGZ is exited.
****************************************************************************/
static void ggz_closed(void)
{
  set_client_page(PAGE_MAIN);
}
#endif

/**************************************************************************
 called from main().
**************************************************************************/
void ui_init(void)
{
#ifdef GGZ_GTK
  /* Engine and version match what is provided in civclient.dsc.in and
   * civserver.dsc.in. */
  ggz_gtk_initialize(FALSE,
		     ggz_connected, ggz_game_launched, ggz_closed,
		     "Freeciv", NETWORK_CAPSTRING_MANDATORY, "Pubserver");
#endif

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
  Called from client_main(), is what it's named.
**************************************************************************/
void ui_main(int argc, char **argv)
{
  const gchar *home;
  guint sig;
  GtkStyle *style;

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

  toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(toplevel, "key_press_event",
                   G_CALLBACK(toplevel_handler), NULL);

  gtk_window_set_role(GTK_WINDOW(toplevel), "toplevel");
  gtk_widget_realize(toplevel);
  gtk_widget_set_name(toplevel, "Freeciv");
  root_window = toplevel->window;

  if (fullscreen_mode) {
    gtk_window_fullscreen(GTK_WINDOW(toplevel));
  }
  
  gtk_window_set_title(GTK_WINDOW (toplevel), _("Freeciv"));

  g_signal_connect(toplevel, "delete_event",
      G_CALLBACK(quit_dialog_callback), NULL);

  /* Disable GTK+ cursor key focus movement */
  sig = g_signal_lookup("focus", GTK_TYPE_WIDGET);
  g_signal_handlers_disconnect_matched(toplevel, G_SIGNAL_MATCH_ID, sig,
				       0, 0, 0, 0);
  g_signal_connect(toplevel, "focus", G_CALLBACK(toplevel_focus), NULL);


  display_color_type = get_visual();

  civ_gc = gdk_gc_new(root_window);

  /* font names shouldn't be in spec files! */
  style = gtk_rc_get_style_by_paths(gtk_settings_get_default(),
				    "Freeciv*.city names",
				    NULL, G_TYPE_NONE);
  if (!style) {
    style = gtk_style_new();
  }
  g_object_ref(style);
  main_font = style->font_desc;

  style = gtk_rc_get_style_by_paths(gtk_settings_get_default(),
				    "Freeciv*.city productions",
				    NULL, G_TYPE_NONE);
  if (!style) {
    style = gtk_style_new();
  }
  g_object_ref(style);
  city_productions_font = style->font_desc;

  set_city_names_font_sizes(city_names_font_size, city_productions_font_size);

  fill_bg_gc = gdk_gc_new(root_window);

  /* for isometric view. always create. the tileset can change at run time. */
  thin_line_gc = gdk_gc_new(root_window);
  thick_line_gc = gdk_gc_new(root_window);
  border_line_gc = gdk_gc_new(root_window);
  gdk_gc_set_line_attributes(thin_line_gc, 1,
			     GDK_LINE_SOLID,
			     GDK_CAP_NOT_LAST,
			     GDK_JOIN_MITER);
  gdk_gc_set_line_attributes(thick_line_gc, 2,
			     GDK_LINE_SOLID,
			     GDK_CAP_NOT_LAST,
			     GDK_JOIN_MITER);
  gdk_gc_set_line_attributes(border_line_gc, BORDER_WIDTH,
			     GDK_LINE_ON_OFF_DASH,
			     GDK_CAP_NOT_LAST,
			     GDK_JOIN_MITER);

  fill_tile_gc = gdk_gc_new(root_window);
  gdk_gc_set_fill(fill_tile_gc, GDK_STIPPLED);

  {
    char d1[] = {0x03, 0x0c, 0x03, 0x0c};
    char d2[] = {0x08, 0x02, 0x08, 0x02};
    char d3[] = {0xAA, 0x55, 0xAA, 0x55};

    gray50 = gdk_bitmap_create_from_data(root_window, d1, 4, 4);
    gray25 = gdk_bitmap_create_from_data(root_window, d2, 4, 4);
    black50 = gdk_bitmap_create_from_data(root_window, d3, 4, 4);
  }

  {
    GdkColor pixel;
    
    mask_bitmap = gdk_pixmap_new(root_window, 1, 1, 1);

    mask_fg_gc = gdk_gc_new(mask_bitmap);
    pixel.pixel = 1;
    gdk_gc_set_foreground(mask_fg_gc, &pixel);
    gdk_gc_set_function(mask_fg_gc, GDK_OR);

    mask_bg_gc = gdk_gc_new(mask_bitmap);
    pixel.pixel = 0;
    gdk_gc_set_foreground(mask_bg_gc, &pixel);
  }

  selection_gc = gdk_gc_new(root_window);
  gdk_gc_set_function(selection_gc, GDK_XOR);

  tileset_init(tileset);
  tileset_load_tiles(tileset);

  /* keep the icon of the executable on Windows (see PR#36491) */
#ifndef WIN32_NATIVE
  /* Only call this after tileset_load_tiles is called. */
  gtk_window_set_icon(GTK_WINDOW(toplevel),
		sprite_get_pixbuf(get_icon_sprite(tileset, ICON_FREECIV)));
#endif

  setup_widgets();
  load_cursors();
  cma_fe_init();
  diplomacy_dialog_init();
  happiness_dialog_init();
  intel_dialog_init();
  spaceship_dialog_init();
  chatline_init();
  init_mapcanvas_and_overview();

  tileset_use_prefered_theme(tileset);

  gtk_widget_show(toplevel);

  /* assumes toplevel showing */
  set_client_state(C_S_DISCONNECTED);
  
  /* assumes client_state is set */
  timer_id = g_timeout_add(TIMER_INTERVAL, timer_callback, NULL);

  gtk_main();

  spaceship_dialog_done();
  intel_dialog_done();
  happiness_dialog_done();
  diplomacy_dialog_done();
  cma_fe_done();
  tileset_free_tiles(tileset);
}

/**************************************************************************
  Do any necessary UI-specific cleanup
**************************************************************************/
void ui_exit()
{

}

/**************************************************************************
  Return our GUI type
**************************************************************************/
enum gui_type get_gui_type(void)
{
  return GUI_GTK2;
}

/**************************************************************************
 Update the connected users list at pregame state.
**************************************************************************/
void update_conn_list_dialog(void)
{
  if (client_has_player()) {
    char *text;

    if (client_player()->is_ready) {
      text = _("Not _ready");
    } else {
      int num_unready = 0;

      players_iterate(pplayer) {
        if (!pplayer->ai_data.control && !pplayer->is_ready) {
	  num_unready++;
	}
      } players_iterate_end;

      if (num_unready > 1) {
	text = _("_Ready");
      } else {
	/* We are the last unready player so clicking here will
	 * immediately start the game. */
	text = _("_Start");
      }
    }

    gtk_stockbutton_set_label(ready_button, text);
  } else {
    gtk_stockbutton_set_label(ready_button, _("_Start"));
  }

  /* Sensitive only if client can control player */
  gtk_widget_set_sensitive(ready_button, can_client_control());

  if (!client_has_player()) {
    /* Nation button will attach to player */
    gtk_stockbutton_set_label(nation_button, _("_Take Player"));
    gtk_widget_set_sensitive(nation_button, TRUE);
  } else {
    /* Nation button will go to Nation selection */
    gtk_stockbutton_set_label(nation_button, _("Pick _Nation"));

    /* Sensitive iff client is controlling a player. */
    gtk_widget_set_sensitive(nation_button, game.info.is_new_game
                             && can_client_control());
  }

  if (!client_is_observer()) {
    gtk_stockbutton_set_label(take_button, _("_Observe"));
  } else {
    gtk_stockbutton_set_label(take_button, _("Do not _observe"));
  }

  if (connection_list_view != NULL) {
    GObject *view;
    GtkTreeViewColumn *col;
    bool visible;

    view = G_OBJECT(connection_list_view);
    visible = (with_ggz || in_ggz);

    col = g_object_get_data(view, "record_col");
    if (col != NULL) {
      gtk_tree_view_column_set_visible(col, visible);
    }
    col = g_object_get_data(view, "rating_col");
    if (col != NULL) {
      gtk_tree_view_column_set_visible(col, visible);
    }
  }

  if (client_state() == C_S_PREPARING
      && get_client_page() == PAGE_START
      && connection_list_store != NULL) {
    GtkTreeStore *store;
    GtkTreeIter iter, parent;
    GdkPixbuf *pixbuf;
    bool is_ready;
    const char *nation, *plr_name, *team;
    char user_name[MAX_LEN_NAME + 8], rating_text[128], record_text[128];
    int rating, wins, losses, ties, forfeits;
    enum cmdlevel_id access_level;
    int conn_id;

    /* Clear the old list. */
    store = connection_list_store;
    gtk_tree_store_clear(store);

    /* Insert players into the connection list. */
    players_iterate(pplayer) {
      conn_id = -1;
      access_level = ALLOW_NONE;
      pixbuf = pplayer->nation ? get_flag(pplayer->nation) : NULL;;

      conn_list_iterate(pplayer->connections, pconn) {
        if (pconn->playing == pplayer && !pconn->observer) {
          conn_id = pconn->id;
          access_level = pconn->access_level;
          break;
        }
      } conn_list_iterate_end;

      if (pplayer->ai_data.control && !pplayer->was_created
          && !pplayer->is_connected) {
        /* TRANS: "<Novice AI>" */
        my_snprintf(user_name, sizeof(user_name), _("<%s AI>"),
                    ai_level_name(pplayer->ai_data.skill_level));
      } else {
        sz_strlcpy(user_name, pplayer->username);
        if (access_level > ALLOW_BASIC) {
          sz_strlcat(user_name, "*");
        }
      }

      is_ready = pplayer->ai_data.control ? TRUE : pplayer->is_ready;

      if (pplayer->nation == NO_NATION_SELECTED) {
	nation = _("Random");
        if (pplayer->was_created) {
          plr_name = player_name(pplayer);
        } else {
          plr_name = "";
        }
      } else {
	nation = nation_adjective_for_player(pplayer);
	plr_name = player_name(pplayer);
      }

      team = pplayer->team ? team_name_translation(pplayer->team) : "";

      rating_text[0] = '\0';
      if ((in_ggz || with_ggz)
          && !pplayer->ai_data.control
	  && user_get_rating(pplayer->username, &rating)) {
	my_snprintf(rating_text, sizeof(rating_text), "%d", rating);
      }

      record_text[0] = '\0';
      if ((in_ggz || with_ggz)
          && !pplayer->ai_data.control
	  && user_get_record(pplayer->username,
			       &wins, &losses, &ties, &forfeits)) {
	if (forfeits == 0 && ties == 0) {
	  my_snprintf(record_text, sizeof(record_text), "%d-%d",
		      wins, losses);
	} else if (forfeits == 0) {
	  my_snprintf(record_text, sizeof(record_text), "%d-%d-%d",
		      wins, losses, ties);
	} else {
	  my_snprintf(record_text, sizeof(record_text), "%d-%d-%d-%d",
		      wins, losses, ties, forfeits);
	}
      }

      gtk_tree_store_append(store, &iter, NULL);
      gtk_tree_store_set(store, &iter, 
                         CL_COL_PLAYER_NUMBER, player_number(pplayer),
                         CL_COL_USER_NAME, user_name,
                         CL_COL_READY_STATE, is_ready,
                         CL_COL_PLAYER_NAME, plr_name,
			 CL_COL_FLAG, pixbuf,
                         CL_COL_NATION, nation,
                         CL_COL_TEAM, team,
                         CL_COL_GGZ_RECORD, record_text,
                         CL_COL_GGZ_RATING, rating_text,
                         CL_COL_CONN_ID, conn_id, -1);
      parent = iter;

      /* Insert observers of this player as child nodes. */
      conn_list_iterate(pplayer->connections, pconn) {
        if (pconn->id == conn_id) {
          continue;
        }
        gtk_tree_store_append(store, &iter, &parent);
        gtk_tree_store_set(store, &iter,
                           CL_COL_PLAYER_NUMBER, -1,
                           CL_COL_USER_NAME, pconn->username,
                           CL_COL_TEAM, _("Observer"),
                           CL_COL_CONN_ID, pconn->id, -1);
      } conn_list_iterate_end;

      if (pixbuf) {
	g_object_unref(pixbuf);
      }
    } players_iterate_end;

    /* Finally, insert global observers and detached connections. */
    conn_list_iterate(game.est_connections, pconn) {
      if (pconn->playing != NULL) {
	continue; /* Already listed above. */
      }
      team = pconn->observer ? _("Observer") : _("Detached");
      gtk_tree_store_append(store, &iter, NULL);
      gtk_tree_store_set(store, &iter,
			 CL_COL_PLAYER_NUMBER, -1,
			 CL_COL_USER_NAME, pconn->username,
			 CL_COL_TEAM, team,
			 CL_COL_CONN_ID, pconn->id, -1);
    } conn_list_iterate_end;

    if (connection_list_view != NULL) {
      GtkTreeView *view = connection_list_view;
      gtk_tree_view_expand_all(view);
    }
  }
}

/**************************************************************************
 obvious...
**************************************************************************/
void sound_bell(void)
{
  gdk_beep();
}

/**************************************************************************
  Set one of the unit icons in information area based on punit.
  Use punit==NULL to clear icon.
  Index 'idx' is -1 for "active unit", or 0 to (num_units_below-1) for
  units below.  Also updates unit_ids[idx] for idx>=0.
**************************************************************************/
void set_unit_icon(int idx, struct unit *punit)
{
  GtkWidget *w;
  
  assert(idx >= -1 && idx < num_units_below);

  if (idx == -1) {
    w = unit_pixmap;
    unit_id_top = punit ? punit->id : 0;
  } else {
    w = unit_below_pixmap[idx];
    unit_ids[idx] = punit ? punit->id : 0;
  }

  if (!w) {
    return;
  }

  gtk_pixcomm_freeze(GTK_PIXCOMM(w));

  if (punit) {
    put_unit_gpixmap(punit, GTK_PIXCOMM(w));
  } else {
    gtk_pixcomm_clear(GTK_PIXCOMM(w));
  }
  
  gtk_pixcomm_thaw(GTK_PIXCOMM(w));
}

/**************************************************************************
  Set the "more arrow" for the unit icons to on(1) or off(0).
  Maintains a static record of current state to avoid unnecessary redraws.
  Note initial state should match initial gui setup (off).
**************************************************************************/
void set_unit_icons_more_arrow(bool onoff)
{
  static bool showing = FALSE;

  if (!more_arrow_pixmap) {
    return;
  }

  if (onoff && !showing) {
    gtk_widget_show(more_arrow_pixmap);
    showing = TRUE;
  }
  else if(!onoff && showing) {
    gtk_widget_hide(more_arrow_pixmap);
    showing = FALSE;
  }
}

/**************************************************************************
 callback for clicking a unit icon underneath unit info box.
 these are the units on the same tile as the focus unit.
**************************************************************************/
static gboolean select_unit_pixmap_callback(GtkWidget *w, GdkEvent *ev, 
                                        gpointer data) 
{
  int i = GPOINTER_TO_INT(data);
  struct unit *punit;

  if (i == -1) {
    punit = game_find_unit_by_number(unit_id_top);
    if (punit && unit_is_in_focus(punit)) {
      /* Clicking on the currently selected unit will center it. */
      center_tile_mapcanvas(punit->tile);
    }
    return TRUE;
  }

  if (unit_ids[i] == 0) /* no unit displayed at this place */
    return TRUE;

  punit = game_find_unit_by_number(unit_ids[i]);
  if (NULL != punit && unit_owner(punit) == client.conn.playing) {
    /* Unit shouldn't be NULL but may be owned by an ally. */
    set_unit_focus(punit);
  }

  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
static gboolean show_info_button_release(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  gtk_grab_remove(w);
  gdk_pointer_ungrab(GDK_CURRENT_TIME);
  gtk_widget_destroy(w);
  return FALSE;
}

/**************************************************************************
...
**************************************************************************/
static gboolean show_info_popup(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  if(ev->button == 1) {
    GtkWidget *p;

    p = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_set_app_paintable(p, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(p), 4);
    gtk_window_set_position(GTK_WINDOW(p), GTK_WIN_POS_MOUSE);

    gtk_widget_new(GTK_TYPE_LABEL, "GtkWidget::parent", p,
		   "GtkLabel::label", get_info_label_text_popup(),
				   "GtkWidget::visible", TRUE,
        			   NULL);
    gtk_widget_show(p);

    gdk_pointer_grab(p->window, TRUE, GDK_BUTTON_RELEASE_MASK,
		     NULL, NULL, ev->time);
    gtk_grab_add(p);

    g_signal_connect_after(p, "button_release_event",
                           G_CALLBACK(show_info_button_release), NULL);
  }
  return TRUE;
}

/**************************************************************************
 user clicked "Turn Done" button
**************************************************************************/
static void end_turn_callback(GtkWidget *w, gpointer data)
{
    gtk_widget_set_sensitive(turn_done_button, FALSE);
    user_ended_turn();
}

/**************************************************************************
...
**************************************************************************/
static void get_net_input(gpointer data, gint fid, GdkInputCondition condition)
{
  input_from_server(fid);
}

/**************************************************************************
  Callback for when the GGZ socket has data pending.
**************************************************************************/
static void get_ggz_input(gpointer data, gint fid, GdkInputCondition condition)
{
  input_from_ggz(fid);
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

  freelog(LOG_DEBUG, "set_wait_for_writable_socket(%d)", socket_writable);
  gtk_input_remove(input_id);
  input_id = gtk_input_add_full(client.conn.sock, GDK_INPUT_READ
				| (socket_writable ? GDK_INPUT_WRITE : 0)
				| GDK_INPUT_EXCEPTION,
				get_net_input, NULL, NULL, NULL);
  previous_state = socket_writable;
}

/**************************************************************************
 This function is called after the client succesfully
 has connected to the server
**************************************************************************/
void add_net_input(int sock)
{
  input_id = gtk_input_add_full(sock, GDK_INPUT_READ | GDK_INPUT_EXCEPTION,
				get_net_input, NULL, NULL, NULL);
  client.conn.notify_of_writable_data = set_wait_for_writable_socket;
}

/**************************************************************************
 This function is called if the client disconnects
 from the server
**************************************************************************/
void remove_net_input(void)
{
  gtk_input_remove(input_id);
  gdk_window_set_cursor(root_window, NULL);
}

/**************************************************************************
  Called to monitor a GGZ socket.
**************************************************************************/
void add_ggz_input(int sock)
{
  ggz_input_id = gtk_input_add_full(sock, GDK_INPUT_READ, get_ggz_input,
				    NULL, NULL, NULL);
}

/**************************************************************************
  Called on disconnection to remove monitoring on the GGZ socket.  Only
  call this if we're actually in GGZ mode.
**************************************************************************/
void remove_ggz_input(void)
{
  gtk_input_remove(ggz_input_id);
}

/****************************************************************
  This is the response callback for the dialog with the message:
  Are you sure you want to quit?
****************************************************************/
static void quit_dialog_response(GtkWidget *dialog, gint response)
{
  gtk_widget_destroy(dialog);
  if (response == GTK_RESPONSE_YES) {
    client_exit();
  }
}

/****************************************************************
  Popups the dialog with the message:
  Are you sure you want to quit?
****************************************************************/
void popup_quit_dialog(void)
{
  static GtkWidget *dialog;

  if (!dialog) {
    dialog = gtk_message_dialog_new(NULL,
	0,
	GTK_MESSAGE_WARNING,
	GTK_BUTTONS_YES_NO,
	_("Are you sure you want to quit?"));
    setup_dialog(dialog, toplevel);

    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

    g_signal_connect(dialog, "response", 
	G_CALLBACK(quit_dialog_response), NULL);
    g_signal_connect(dialog, "destroy",
	G_CALLBACK(gtk_widget_destroyed), &dialog);
  }

  gtk_window_present(GTK_WINDOW(dialog));
}

/****************************************************************
  Popups the quit dialog.
****************************************************************/
static gboolean quit_dialog_callback(void)
{
  popup_quit_dialog();
  /* Stop emission of event. */
  return TRUE;
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
  Option callback for the 'gui_gtk2_split_bottom_notebook' option.
****************************************************************************/
static void split_bottom_notebook_callback(struct client_option *op)
{
  popdown_meswin_dialog();
  if (*op->boolean.pvalue) {
    gtk_paned_pack2(GTK_PANED(bottom_hpaned), right_notebook, TRUE, TRUE);
    gtk_widget_show_all(right_notebook);
  } else {
    gtk_container_remove(GTK_CONTAINER(bottom_hpaned), right_notebook);
  }
  popup_meswin_dialog(FALSE);
}

/****************************************************************************
  Option callback for the 'gui_gtk2_allied_chat_only' option.
  This updates the state of the associated toggle button.
****************************************************************************/
static void allied_chat_only_callback(struct client_option *op)
{
  GtkWidget *button;

  button = allied_chat_toggle_button;
  g_return_if_fail(button != NULL);
  g_return_if_fail(GTK_IS_TOGGLE_BUTTON(button));

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                               *op->boolean.pvalue);
}

/****************************************************************************
  Extra initializers for client options.  Here we make set the callback
  for the specific gui-gtk-2.0 options.
****************************************************************************/
void gui_options_extra_init(void)
{
  client_options_iterate(o) {
    if (o->type == COT_BOOLEAN
        && o->boolean.pvalue == &gui_gtk2_allied_chat_only) {
      o->change_callback = allied_chat_only_callback;
    } else if (o->type == COT_BOOLEAN
               && o->boolean.pvalue == &gui_gtk2_split_bottom_notebook) {
      o->change_callback = split_bottom_notebook_callback;
    } else if (o->type == COT_STRING
               && o->string.pvalue) {
    } else {
      gui_update_font_from_option(o);
    }
  } client_options_iterate_end;
}

/**************************************************************************
  Set the chatline buttons to reflect the state of the game and current
  client options. This function should be called on game start.
**************************************************************************/
void refresh_chat_buttons(void)
{
  GtkWidget *button;

  button = allied_chat_toggle_button;
  g_return_if_fail(button != NULL);
  g_return_if_fail(GTK_IS_TOGGLE_BUTTON(button));

  /* Hide the "Allies Only" button for local games. */
  if (is_server_running()) {
    gtk_widget_hide(button);
  } else {
    gtk_widget_show(button);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
                                 gui_gtk2_allied_chat_only);
  }
}

/**************************************************************************
  Handle a toggle of the "Allies Only" chat button.
**************************************************************************/
static void allied_chat_button_toggled(GtkToggleButton *button,
                                       gpointer user_data)
{
  gui_gtk2_allied_chat_only = gtk_toggle_button_get_active(button);
}
