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

#include <windows.h>
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* common & utility */                      
#include "capability.h"
#include "fcintl.h"
#include "map.h"
#include "mem.h"
#include "player.h"
#include "support.h"
#include "unit.h"

/* client */
#include "chatline.h"
#include "citydlg.h"
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "colors.h"
#include "control.h"
#include "dialogs.h"
#include "graphics.h"
#include "gui_stuff.h"
#include "inputdlg.h"
#include "mapview.h"
#include "menu.h"
#include "overview_common.h"
#include "tilespec.h" 
#include "text.h"

#include "goto.h"
#include "mapctrl.h"
#include "gui_main.h"

extern HCURSOR cursors[];

HWND popit_popup=NULL;
static bool popit_is_orders;
/*************************************************************************

*************************************************************************/
void map_handle_move(int window_x, int window_y)
{
  update_line(window_x, window_y);

  control_mouse_cursor(canvas_pos_to_tile(window_x, window_y));
}

/*************************************************************************

*************************************************************************/
static LONG CALLBACK popit_proc(HWND hwnd,UINT message,
				WPARAM wParam,LPARAM lParam)
{
  struct tile **cross_list;
  switch(message)
    {
    case WM_CREATE:
    case WM_SIZE:
    case WM_GETMINMAXINFO:
      break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
      DestroyWindow(hwnd);
      break;
    case WM_DESTROY:
      cross_list = fcwin_get_user_data(hwnd);
      if (cross_list) {
	  while (*cross_list != NULL) {
	    refresh_tile_mapcanvas(*cross_list, TRUE, TRUE);
	    cross_list++;
	  }
      }
      if (popit_is_orders) {
	update_map_canvas_visible();
      }
      popit_popup = NULL;
      break;
    default:
      return DefWindowProc(hwnd,message,wParam,lParam);
    }
  return 0;
}

/*************************************************************************

*************************************************************************/
static void popit(int x, int y, struct tile *ptile)
{
  HWND popup;
  POINT pt;
  RECT rc;
  struct fcwin_box *vbox;
  static struct tile *cross_list[2+1];
  struct tile **cross_head = cross_list;
  int i;
  struct unit *punit;

  if (popit_popup!=NULL) {
    DestroyWindow(popit_popup);
    popit_popup=NULL;
  }
  
  if (TILE_UNKNOWN == client_tile_get_known(ptile))
    return;
  
  popup=fcwin_create_layouted_window(popit_proc,NULL,WS_POPUP|WS_BORDER,
				     0,0,root_window,NULL,
				     FAKE_CHILD,
				     cross_head);
  vbox=fcwin_vbox_new(popup,FALSE);

  fcwin_box_add_static(vbox, popup_info_text(ptile), 0, SS_LEFT,
		       FALSE, FALSE, 0);

  punit = find_visible_unit(ptile);

  popit_is_orders = show_unit_orders(punit);

  if (punit && punit->goto_tile) {
    *cross_head = punit->goto_tile;
    cross_head++;
  }
  *cross_head = ptile;
  cross_head++;

  *cross_head = NULL;
  for (i = 0; cross_list[i] != NULL; i++) {
    put_cross_overlay_tile(cross_list[i]);
  }

  fcwin_set_box(popup,vbox);

  GetWindowRect(popup,&rc);
  pt.x=x;
  pt.y=y;
  ClientToScreen(map_window,&pt);
  MoveWindow(popup,pt.x+16,pt.y-rc.bottom+rc.top,
	     rc.right-rc.left,rc.bottom-rc.top,FALSE);
  ShowWindow(popup,SW_SHOWNORMAL);
  popit_popup=popup;
} 

/**************************************************************************

**************************************************************************/
static LONG CALLBACK map_wnd_proc(HWND hwnd,UINT message,WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  struct tile *ptile;

  switch(message) {
  case WM_CREATE:
    break;
  case WM_LBUTTONDOWN:
    if (!can_client_change_view()) {
      break;
    }
    SetFocus(root_window);
    if (wParam&MK_SHIFT) {
      adjust_workers_button_pressed(LOWORD(lParam), HIWORD(lParam));
      wakeup_button_pressed(LOWORD(lParam), HIWORD(lParam));
    } else if (wParam & MK_CONTROL
	       && (ptile = canvas_pos_to_tile(LOWORD(lParam),
					      HIWORD(lParam)))) {
      popit(LOWORD(lParam),HIWORD(lParam),ptile);
    } else {
      action_button_pressed(LOWORD(lParam), HIWORD(lParam), SELECT_POPUP);
    }
    break;
  case WM_MBUTTONDOWN:
    if (can_client_change_view()
        && (ptile = canvas_pos_to_tile(LOWORD(lParam),
					      HIWORD(lParam)))) {
      popit(LOWORD(lParam), HIWORD(lParam), ptile);
    }
    break;
  case WM_RBUTTONDOWN:
    if (can_client_change_view()) {
      if (wParam&MK_CONTROL) {
        if ((ptile = canvas_pos_to_tile(LOWORD(lParam), HIWORD(lParam)))) {
          popit(LOWORD(lParam), HIWORD(lParam), ptile);
        }
      } else {
	recenter_button_pressed(LOWORD(lParam), HIWORD(lParam));
      }
    }
    break;
  case WM_SETFOCUS:
  case WM_ACTIVATE:
    SetFocus(root_window);
    break;
  case WM_LBUTTONUP:
  case WM_RBUTTONUP:
  case WM_MBUTTONUP:
    if (popit_popup!=NULL) {
      DestroyWindow(popit_popup);
      popit_popup=NULL;
      SetFocus(root_window);
    }
    break;
  case WM_SETCURSOR:
    switch (hover_state) {
    case HOVER_NONE:
      SetCursor (LoadCursor(NULL, IDC_ARROW));
      break;
    case HOVER_PATROL:
      SetCursor (cursors[CURSOR_PATROL]);
      break;
    case HOVER_GOTO:
    case HOVER_CONNECT:
      SetCursor (cursors[CURSOR_GOTO]);
      break;
    case HOVER_NUKE:
      SetCursor (cursors[CURSOR_NUKE]);
      break;
    case HOVER_PARADROP:
      SetCursor (cursors[CURSOR_PARADROP]);
      break;
    }
    break;
  case WM_MOUSEMOVE:
    if (can_client_change_view()) {
      map_handle_move(LOWORD(lParam),HIWORD(lParam));
    }
    break;
  case WM_PAINT:
    BeginPaint(hwnd, (LPPAINTSTRUCT)&ps);
    map_expose(ps.rcPaint.left, ps.rcPaint.top,
	       ps.rcPaint.right - ps.rcPaint.left,
	       ps.rcPaint.bottom - ps.rcPaint.top);
    EndPaint(hwnd, (LPPAINTSTRUCT)&ps);
    break;
  default:
    return DefWindowProc(hwnd,message,wParam,lParam);
  }
  return 0;
}

/**************************************************************************

**************************************************************************/
void init_mapwindow(void)
{
  WNDCLASS *wndclass;
  wndclass=fc_malloc(sizeof(WNDCLASS));
  wndclass->style=0;
  wndclass->cbClsExtra=0;
  wndclass->cbWndExtra=0;
  wndclass->lpfnWndProc=(WNDPROC) map_wnd_proc;
  wndclass->hIcon=NULL;
  wndclass->hCursor=LoadCursor(NULL,IDC_ARROW);
  wndclass->hInstance=freecivhinst;
  wndclass->hbrBackground=GetStockObject(BLACK_BRUSH);
  wndclass->lpszClassName="freecivmapwindow";
  wndclass->lpszMenuName=(LPSTR)NULL;
  if (!RegisterClass(wndclass))
    exit(EXIT_FAILURE);
}

/**************************************************************************

**************************************************************************/
void overview_handle_rbut(int x, int y)
{
 int xtile, ytile;
 struct tile *ptile;

 if (!can_client_change_view()) {
   return;
 }

 overview_to_map_pos(&xtile, &ytile, x, y);
 ptile = map_pos_to_tile(xtile, ytile);

 center_tile_mapcanvas(ptile);

}

/**************************************************************************

**************************************************************************/
void indicator_handle_but(int i)
{
  int delta = 10;
  int lux_end = client.conn.playing->economic.luxury;
  int sci_end = lux_end + client.conn.playing->economic.science;
#if 0 /* Unneeded. */
  int tax_end = 100; 
#endif
  int luxury = client.conn.playing->economic.luxury;
  int science = client.conn.playing->economic.science;
  int tax = client.conn.playing->economic.tax;
  
  i *= 10;
  if (i < lux_end) {
    luxury -= delta;
    science += delta;
  } else if (i < sci_end) {
    science -= delta;
    tax += delta;
  } else {
    tax -= delta;
    luxury += delta;
  }

  dsend_packet_player_rates(&client.conn, tax, luxury, science);
}

/**************************************************************************

**************************************************************************/
static void name_new_city_callback(HWND w, void *data)
{
  size_t unit_id;
 
  if ((unit_id = (size_t)data)) {
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
void
popup_newcity_dialog(struct unit *punit, char *suggestname)
{
  input_dialog_create(NULL, /*"shellnewcityname"*/_("Build New City"),
                        _("What should we call our new city?"),
                        suggestname,
                        name_new_city_callback, (void *)punit->id,
                        name_new_city_callback, (void *)0);  
}

/**************************************************************************

**************************************************************************/
void center_on_unit(void)
{
   request_center_focus_unit(); 
}

/**************************************************************************

**************************************************************************/
void set_turn_done_button_state(bool state)
{
  EnableWindow(turndone_button,state);
}

/**************************************************************************

**************************************************************************/
void create_line_at_mouse_pos(void)
{
  POINT pos;
  GetCursorPos(&pos);
  ScreenToClient(map_window, &pos);
  update_line(pos.x, pos.y);
}

/**************************************************************************
 The Area Selection rectangle. Called by center_tile_mapcanvas() and
 when the mouse pointer moves.
**************************************************************************/
void update_rect_at_mouse_pos(void)
{
  /* PORTME */
}
