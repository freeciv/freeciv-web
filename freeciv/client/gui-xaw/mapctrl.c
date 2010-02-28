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
#include <stdio.h>
#include <stdlib.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/AsciiText.h>  
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>

/* common & utility */
#include "climap.h"
#include "fcintl.h"
#include "game.h"
#include "map.h"
#include "city.h"
#include "player.h"
#include "support.h"
#include "unit.h"

/* client */
#include "chatline.h"
#include "citydlg.h"
#include "client_main.h"
#include "colors.h"
#include "control.h"
#include "dialogs.h"
#include "goto.h"
#include "graphics.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "inputdlg.h"
#include "mapview.h"
#include "menu.h"
#include "overview_common.h"
#include "text.h"
#include "tilespec.h"

#include "mapctrl.h"

static void popupinfo_popdown_callback(Widget w, XtPointer client_data, XtPointer call_data);

/**************************************************************************
...
**************************************************************************/
static void name_new_city_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data)
{
  size_t unit_id=(size_t)client_data;
  
  if (unit_id) {
    dsend_packet_unit_build_city(&client.conn, unit_id,
				 input_dialog_get_input(w));
  }
    
  input_dialog_destroy(w);
}

/**************************************************************************
 Popup dialog where the user choose the name of the new city
 punit = (settler) unit which builds the city
 suggestname = suggetion of the new city's name
**************************************************************************/
void popup_newcity_dialog(struct unit *punit, char *suggestname)
{
  input_dialog_create(toplevel, "shellnewcityname",
		      _("What should we call our new city?"), suggestname,
		      name_new_city_callback, INT_TO_XTPOINTER(punit->id),
		      name_new_city_callback, INT_TO_XTPOINTER(0));
}

/**************************************************************************
...
**************************************************************************/
static void popit(int xin, int yin, struct tile *ptile)
{
  Position x, y;
  int dw, dh;
  Dimension w, h, b;
  static struct tile *cross_list[2+1];
  struct tile **cross_head = cross_list;
  int i;
  struct unit *punit;
  char *content;
  static bool is_orders;
  
  if (TILE_UNKNOWN != client_tile_get_known(ptile)) {
    Widget p=XtCreatePopupShell("popupinfo", simpleMenuWidgetClass,
				map_canvas, NULL, 0);
    content = (char *) popup_info_text(ptile);
    /* content is provided to us as a single string with multiple lines,
       but xaw doens't support multi-line labels.  So we break it up.
       We mangle it in the process, but who cares?  It's never going to be
       used again anyway. */
    while (1) {
      char *end = strchr(content, '\n'); 
      if (end) {
	*end='\0';
      }
      XtCreateManagedWidget(content, smeBSBObjectClass, p, NULL, 0);
      if (end) {
	content = end+1;
      } else {
	break;
      }
    }

    punit = find_visible_unit(ptile);
    is_orders = show_unit_orders(punit);
    if (punit && punit->goto_tile) {
      *cross_head = punit->goto_tile;
      cross_head++;
    }
    *cross_head = ptile;
    cross_head++;

    xin /= tileset_tile_width(tileset);
    xin *= tileset_tile_width(tileset);
    yin /= tileset_tile_height(tileset);
    yin *= tileset_tile_height(tileset);
    xin += (tileset_tile_width(tileset) / 2);
    XtTranslateCoords(map_canvas, xin, yin, &x, &y);
    dw = XDisplayWidth (display, screen_number);
    dh = XDisplayHeight (display, screen_number);
    XtRealizeWidget(p);
    XtVaGetValues(p, XtNwidth, &w, XtNheight, &h, XtNborderWidth, &b, NULL);
    w += (2 * b);
    h += (2 * b);
    x -= (w / 2);
    y -= h;
    if ((x + w) > dw) x = dw - w;
    if (x < 0) x = 0;
    if ((y + h) > dh) y = dh - h;
    if (y < 0) y = 0;
    XtVaSetValues(p, XtNx, x, XtNy, y, NULL);

    *cross_head = NULL;
    for (i = 0; cross_list[i]; i++) {
      put_cross_overlay_tile(cross_list[i]);
    }
    XtAddCallback(p,XtNpopdownCallback,popupinfo_popdown_callback,
		  (XtPointer)&is_orders);

    XtPopupSpringLoaded(p);
  }
  
}

/**************************************************************************
...
**************************************************************************/
void popupinfo_popdown_callback(Widget w, XtPointer client_data,
        XtPointer call_data)
{
  bool *full = client_data;

  if (*full) {
    update_map_canvas_visible();
  } else {
    dirty_all();
  }

  XtDestroyWidget(w);
}

/**************************************************************************
(RP:) wake up my own sentried units on the tile that was clicked
**************************************************************************/
void mapctrl_btn_wakeup(XEvent *event)
{
  wakeup_button_pressed(event->xbutton.x, event->xbutton.y);
}

/**************************************************************************
...
**************************************************************************/
void mapctrl_btn_mapcanvas(XEvent *event)
{
  XButtonEvent *ev=&event->xbutton;
  struct tile *ptile = canvas_pos_to_tile(ev->x, ev->y);

  if (!can_client_change_view()) {
    return;
  }

  if (ev->button == Button1 && (ev->state & ControlMask)) {
    action_button_pressed(ev->x, ev->y, SELECT_SEA);
  } else if (ev->button == Button1) {
    action_button_pressed(ev->x, ev->y, SELECT_POPUP);
  } else if (ev->button == Button2 && ptile) {
    popit(ev->x, ev->y, ptile);
  } else if (ev->button == Button3 && (ev->state & ControlMask)) {
    action_button_pressed(ev->x, ev->y, SELECT_LAND);
  } else if (ev->button == Button3) {
    recenter_button_pressed(ev->x, ev->y);
  }
}

/**************************************************************************
...
**************************************************************************/
void create_line_at_mouse_pos(void)
{
  Bool on_same_screen;
  Window root, child;
  int rx, ry, x, y;
  unsigned int mask;

  on_same_screen =
    XQueryPointer(display, XtWindow(map_canvas),
		  &root, &child,
		  &rx, &ry,
		  &x, &y,
		  &mask);

  if (on_same_screen) {
    update_line(x, y);
  }
}

/**************************************************************************
 The Area Selection rectangle. Called by center_tile_mapcanvas() and
 when the mouse pointer moves.
**************************************************************************/
void update_rect_at_mouse_pos(void)
{
  /* PORTME */
}

/**************************************************************************
  Draws the on the map the tiles the given city is using
**************************************************************************/
void mapctrl_key_city_workers(XEvent *event)
{
  key_city_overlay(event->xbutton.x, event->xbutton.y);
}

/**************************************************************************
...
**************************************************************************/
void mapctrl_btn_overviewcanvas(XEvent *event)
{
  int map_x, map_y;
  struct tile *ptile;
  XButtonEvent *ev = &event->xbutton;

  if (!can_client_change_view()) {
    return;
  }

  overview_to_map_pos(&map_x, &map_y, event->xbutton.x, event->xbutton.y);
  ptile = map_pos_to_tile(map_x, map_y);
  if (!ptile) {
    return;
  }

  if(ev->button==Button1)
    do_map_click(ptile, SELECT_POPUP);
  else if(ev->button==Button3)
    center_tile_mapcanvas(ptile);
}

/**************************************************************************
...
**************************************************************************/
void center_on_unit(void)
{
  request_center_focus_unit();
}

/**************************************************************************
 Enable or disable the turn done button.
 Should probably some where else.
**************************************************************************/
void set_turn_done_button_state(bool state)
{
  XtSetSensitive(turn_done_button, state);
}
