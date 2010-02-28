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

#include <assert.h>
#include <gtk/gtk.h>

/* common & utility */
#include "combat.h"
#include "fcintl.h"
#include "game.h"
#include "map.h"
#include "player.h"
#include "support.h"
#include "unit.h"

#include "overview_common.h"

/* client */
#include "chatline.h"
#include "citydlg.h"
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "colors.h"
#include "control.h"
#include "dialogs.h"
#include "editgui.h"
#include "editor.h"
#include "graphics.h"
#include "gui_main.h"
#include "inputdlg.h"
#include "mapview.h"
#include "menu.h"
#include "tilespec.h"
#include "cma_core.h"
#include "text.h"

#include "mapctrl.h"

struct tmousepos { int x, y; };
extern gint cur_x, cur_y;

/**************************************************************************
...
**************************************************************************/
static gboolean popit_button_release(GtkWidget *w, GdkEventButton *event)
{
  gtk_grab_remove(w);
  gdk_pointer_ungrab(GDK_CURRENT_TIME);
  gtk_widget_destroy(w);
  return FALSE;
}

/**************************************************************************
  Put the popup on a smart position, after the real size of the widget is
  known: left of the cursor if within the right half of the map, and vice
  versa; displace the popup so as not to obscure it by the mouse cursor;
  stay always within the map if possible. 
**************************************************************************/
static void popupinfo_positioning_callback(GtkWidget *w, GtkAllocation *alloc, 
					   gpointer data)
{
  struct tmousepos *mousepos = data;
  gint x, y;
  struct tile *ptile;

  ptile = canvas_pos_to_tile(mousepos->x, mousepos->y);
  if (tile_to_canvas_pos(&x, &y, ptile)) {
    gint minx, miny, maxx, maxy;

    gdk_window_get_origin(map_canvas->window, &minx, &miny);
    maxx = minx + map_canvas->allocation.width;
    maxy = miny + map_canvas->allocation.height;

    if (x > mapview.width/2) {
      /* right part of the map */
      x += minx;
      y += miny + (tileset_tile_height(tileset) - alloc->height)/2;

      y = CLIP(miny, y, maxy - alloc->height);

      gtk_window_move(GTK_WINDOW(w), x - alloc->width, y);
    } else {
      /* left part of the map */
      x += minx + tileset_tile_width(tileset);
      y += miny + (tileset_tile_height(tileset) - alloc->height)/2;

      y = CLIP(miny, y, maxy - alloc->height);

      gtk_window_move(GTK_WINDOW(w), x, y);
    }
  }
}

/**************************************************************************
  Popup a label with information about the tile, unit, city, when the user
  used the middle mouse button on the map.
**************************************************************************/
static void popit(GdkEventButton *event, struct tile *ptile)
{
  GtkWidget *p;
  static struct tmousepos mousepos;
  struct unit *punit;

  if (TILE_UNKNOWN != client_tile_get_known(ptile)) {
    p=gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_set_app_paintable(p, TRUE);
    gtk_container_set_border_width(GTK_CONTAINER(p), 4);
    gtk_container_add(GTK_CONTAINER(p), gtk_label_new(popup_info_text(ptile)));

    punit = find_visible_unit(ptile);

    if (punit) {
      mapdeco_set_gotoroute(punit);
      if (punit->goto_tile) {
        mapdeco_set_crosshair(punit->goto_tile, TRUE);
      }
    }
    mapdeco_set_crosshair(ptile, TRUE);

    g_signal_connect(p, "destroy",
		     G_CALLBACK(popupinfo_popdown_callback), NULL);

    mousepos.x = event->x;
    mousepos.y = event->y;

    g_signal_connect(p, "size-allocate",
		     G_CALLBACK(popupinfo_positioning_callback), 
		     &mousepos);

    gtk_widget_show_all(p);
    gdk_pointer_grab(p->window, TRUE, GDK_BUTTON_RELEASE_MASK,
		     NULL, NULL, event->time);
    gtk_grab_add(p);

    g_signal_connect_after(p, "button_release_event",
                           G_CALLBACK(popit_button_release), NULL);
  }
}

/**************************************************************************
...
**************************************************************************/
void popupinfo_popdown_callback(GtkWidget *w, gpointer data)
{
  mapdeco_clear_crosshairs();
  mapdeco_clear_gotoroutes();
}

 /**************************************************************************
...
**************************************************************************/
static void name_new_city_callback(GtkWidget * w, gpointer data)
{
  dsend_packet_unit_build_city(&client.conn, GPOINTER_TO_INT(data),
			       input_dialog_get_input(w));
  input_dialog_destroy(w);
}

/**************************************************************************
 Popup dialog where the user choose the name of the new city
 punit = (settler) unit which builds the city
 suggestname = suggetion of the new city's name
**************************************************************************/
void popup_newcity_dialog(struct unit *punit, char *suggestname)
{
  input_dialog_create(GTK_WINDOW(toplevel), /*"shellnewcityname" */
		     _("Build New City"),
		     _("What should we call our new city?"), suggestname,
		     G_CALLBACK(name_new_city_callback),
                     GINT_TO_POINTER(punit->id),
		     G_CALLBACK(name_new_city_callback),
                     GINT_TO_POINTER(0));
}

/**************************************************************************
 Enable or disable the turn done button.
 Should probably some where else.
**************************************************************************/
void set_turn_done_button_state(bool state)
{
  gtk_widget_set_sensitive(turn_done_button, state);
}

/**************************************************************************
 Handle 'Mouse button released'. Because of the quickselect feature,
 the release of both left and right mousebutton can launch the goto.
**************************************************************************/
gboolean butt_release_mapcanvas(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  if (editor_is_active()) {
    return handle_edit_mouse_button_release(ev);
  }

  if (ev->button == 1 || ev->button == 3) {
    release_goto_button(ev->x, ev->y);
  }
  if(ev->button == 3 && (rbutton_down || hover_state != HOVER_NONE))  {
    release_right_button(ev->x, ev->y);
  }

  return TRUE;
}

/**************************************************************************
 Handle all mouse button press on canvas.
 Future feature: User-configurable mouse clicks.
**************************************************************************/
gboolean butt_down_mapcanvas(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  struct city *pcity = NULL;
  struct tile *ptile = NULL;

  if (editor_is_active()) {
    return handle_edit_mouse_button_press(ev);
  }

  if (!can_client_change_view()) {
    return TRUE;
  }

  gtk_widget_grab_focus(map_canvas);
  ptile = canvas_pos_to_tile(ev->x, ev->y);
  pcity = ptile ? tile_city(ptile) : NULL;

  switch (ev->button) {

  case 1: /* LEFT mouse button */

    /* <SHIFT> + <CONTROL> + LMB : Adjust workers. */
    if ((ev->state & GDK_SHIFT_MASK) && (ev->state & GDK_CONTROL_MASK)) {
      adjust_workers_button_pressed(ev->x, ev->y);
    }
    /* <CONTROL> + LMB : Quickselect a sea unit. */
    else if (ev->state & GDK_CONTROL_MASK) {
      action_button_pressed(ev->x, ev->y, SELECT_SEA);
    }
    /* <SHIFT> + LMB: Append focus unit Production. */
    else if (ptile && (ev->state & GDK_SHIFT_MASK)) {
      action_button_pressed(ev->x, ev->y, SELECT_APPEND);
    }
    /* <ALT> + LMB: popit (same as middle-click) */
    /* FIXME: we need a general mechanism for letting freeciv work with
     * 1- or 2-button mice. */
    else if (ptile && (ev->state & GDK_MOD1_MASK)) {
      popit(ev, ptile);
    }
    /* LMB in Area Selection mode. */
    else if(tiles_hilited_cities) {
      if (ptile) {
        toggle_tile_hilite(ptile);
      }
    }
    /* Plain LMB click. */
    else {
      action_button_pressed(ev->x, ev->y, SELECT_POPUP);
    }
    break;

  case 2: /* MIDDLE mouse button */

    /* <CONTROL> + MMB: Wake up sentries. */
    if (ev->state & GDK_CONTROL_MASK) {
      wakeup_button_pressed(ev->x, ev->y);
    }
    /* Plain Middle click. */
    else if (ptile) {
      popit(ev, ptile);
    }
    break;

  case 3: /* RIGHT mouse button */

    /* <CONTROL> + <ALT> + RMB : insert city or tile chat link. */
    /* <CONTROL> + <ALT> + <SHIFT> + RMB : insert unit chat link. */
    if (ptile && (ev->state & GDK_MOD1_MASK)
        && (ev->state & GDK_CONTROL_MASK)) {
      inputline_make_chat_link(ptile, (ev->state & GDK_SHIFT_MASK) != 0);
    }
    /* <SHIFT> + <ALT> + RMB : Show/hide workers. */
    else if ((ev->state & GDK_SHIFT_MASK) && (ev->state & GDK_MOD1_MASK)) {
      overlay_workers_at_city();
    }
    /* <SHIFT + CONTROL> + RMB: Paste Production. */
    else if ((ev->state & GDK_SHIFT_MASK) && (ev->state & GDK_CONTROL_MASK)
             && pcity != NULL) {
      clipboard_paste_production(pcity);
      cancel_tile_hiliting();
    }
    /* <SHIFT> + RMB: Copy Production. */
    else if (ev->state & GDK_SHIFT_MASK) {
      clipboard_copy_production(ptile);
    }
    /* <CONTROL> + RMB : Quickselect a land unit. */
    else if (ev->state & GDK_CONTROL_MASK) {
      action_button_pressed(ev->x, ev->y, SELECT_LAND);
    }
    /* Plain RMB click. */
    else {
      /*  A foolproof user will depress button on canvas,
       *  release it on another widget, and return to canvas
       *  to find rectangle still active.
       */
      if (rectangle_active) {
        release_right_button(ev->x, ev->y);
        return TRUE;
      }
      cancel_tile_hiliting();
      if (hover_state == HOVER_NONE) {
        anchor_selection_rectangle(ev->x, ev->y,
				   (ev->state & GDK_SHIFT_MASK) != 0);
        rbutton_down = TRUE; /* causes rectangle updates */
      }
    }
    break;

  default:
    break;
  }


  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
void create_line_at_mouse_pos(void)
{
  int x, y;

  gdk_window_get_pointer(map_canvas->window, &x, &y, 0);
  if (x >= 0 && y >= 0
      && x < mapview.width && y < mapview.width) {
    update_line(x, y);
  } else {
    gdk_window_get_pointer(overview_canvas->window, &x, &y, 0);
    if (x >= 0 && y >= 0
	&& x < OVERVIEW_TILE_WIDTH * map.xsize
	&& y < OVERVIEW_TILE_HEIGHT * map.ysize) {
      overview_update_line(x, y);
    }
  }
}

/**************************************************************************
 The Area Selection rectangle. Called by center_tile_mapcanvas() and
 when the mouse pointer moves.
**************************************************************************/
void update_rect_at_mouse_pos(void)
{
  int canvas_x, canvas_y;

  if (!rbutton_down) {
    return;
  }

  /* Reading the mouse pos here saves event queueing. */
  gdk_window_get_pointer(map_canvas->window, &canvas_x, &canvas_y, NULL);
  update_selection_rectangle(canvas_x, canvas_y);
}

/**************************************************************************
  Triggered by the mouse moving on the mapcanvas, this function will
  update the mouse cursor and goto lines. 
**************************************************************************/
gboolean move_mapcanvas(GtkWidget *w, GdkEventMotion *ev, gpointer data)
{
  if (!GTK_WIDGET_HAS_FOCUS(map_canvas)) {
    gtk_widget_grab_focus(map_canvas);
  }

  if (editor_is_active()) {
    return handle_edit_mouse_move(ev);
  }

  cur_x = ev->x;
  cur_y = ev->y;
  update_line(ev->x, ev->y);
  update_rect_at_mouse_pos();
  if (keyboardless_goto_button_down && hover_state == HOVER_NONE) {
    maybe_activate_keyboardless_goto(ev->x, ev->y);
  }
  control_mouse_cursor(canvas_pos_to_tile(ev->x, ev->y));

  return TRUE;
}

/**************************************************************************
  This function will reset the mouse cursor if it leaves the map.
**************************************************************************/
gboolean leave_mapcanvas(GtkWidget *widget, GdkEventCrossing *event)
{
  int canvas_x, canvas_y;

  if (gtk_notebook_get_current_page(GTK_NOTEBOOK(top_notebook))
      != gtk_notebook_page_num(GTK_NOTEBOOK(top_notebook), map_widget)) {
    /* Map is not currently topmost tab. Do not use tile specific cursors. */
    update_mouse_cursor(CURSOR_DEFAULT);
    return TRUE;
  }

  /* Bizarrely, this function can be called even when we don't "leave"
   * the map canvas, for instance, it gets called any time the mouse is
   * clicked. */
  gdk_window_get_pointer(map_canvas->window, &canvas_x, &canvas_y, NULL);
  if (map_exists()
      && canvas_x >= 0 && canvas_y >= 0
      && canvas_x < mapview.width && canvas_y < mapview.height) {
    control_mouse_cursor(canvas_pos_to_tile(canvas_x, canvas_y));
  } else {
    update_mouse_cursor(CURSOR_DEFAULT);
  }

  update_unit_info_label(get_units_in_focus());
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
gboolean move_overviewcanvas(GtkWidget *w, GdkEventMotion *ev, gpointer data)
{
  overview_update_line(ev->x, ev->y);
  return TRUE;
}

/**************************************************************************
...
**************************************************************************/
gboolean butt_down_overviewcanvas(GtkWidget *w, GdkEventButton *ev, gpointer data)
{
  int xtile, ytile;

  if (ev->type != GDK_BUTTON_PRESS)
    return TRUE; /* Double-clicks? Triple-clicks? No thanks! */

  overview_to_map_pos(&xtile, &ytile, ev->x, ev->y);

  if (can_client_change_view() && (ev->button == 3)) {
    center_tile_mapcanvas(map_pos_to_tile(xtile, ytile));
  } else if (can_client_issue_orders() && (ev->button == 1)) {
    do_map_click(map_pos_to_tile(xtile, ytile),
		 (ev->state & GDK_SHIFT_MASK) ? SELECT_POPUP : SELECT_APPEND);
  }

  return TRUE;
}

/**************************************************************************
  Best effort to center the map on the currently selected unit(s)
**************************************************************************/
void center_on_unit(void)
{
  request_center_focus_unit();
}

/**************************************************************************
  Shows/hides overlay on the map for the city at this location
**************************************************************************/
void overlay_workers_at_city(void)
{
  int x, y;
  
  gdk_window_get_pointer(map_canvas->window, &x, &y, NULL);
  key_city_overlay(x, y);
}
