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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "player.h"

/* gui-win32 */
#include "gui_main.h"
#include "gui_stuff.h"
#include "dialogs.h"
#include "mapview.h"
#include "finddlg.h"

extern HWND root_window;
extern HINSTANCE freecivhinst;
static HWND finddialog;

/**************************************************************************

**************************************************************************/
static void update_find_dialog()
{
  ListBox_ResetContent(GetDlgItem(finddialog,ID_FINDCITY_LIST));

  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      HWND hList = GetDlgItem(finddialog,ID_FINDCITY_LIST);
      int id = ListBox_AddString(hList,city_name(pcity));

      ListBox_SetItemData(hList,id,player_number(pplayer));
    } city_list_iterate_end;
  } players_iterate_end;
}

/**************************************************************************

**************************************************************************/
static LONG CALLBACK find_city_proc(HWND hWnd,
				    UINT message,
				    WPARAM wParam,
				    LPARAM lParam)
{
  int id;
  switch(message)
    {                  
    case WM_CREATE: 
      break;
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;
    case WM_SIZE:
    case WM_GETMINMAXINFO:
    case WM_DESTROY:
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case ID_FINDCITY_CENTER:
	  id=ListBox_GetCurSel(GetDlgItem(hWnd,ID_FINDCITY_LIST));
	  if (id!=LB_ERR)
	    {
	      char cityname[512];
	      struct city *pcity;    
	      ListBox_GetText(GetDlgItem(hWnd,ID_FINDCITY_LIST),id,
			      cityname);
	      if ((pcity=game_find_city_by_name(cityname)))
		{
		  center_tile_mapcanvas(pcity->tile);
		}
	      DestroyWindow(hWnd);
	      finddialog=NULL;
	    }
	  break;
	case ID_FINDCITY_CANCEL:
	  DestroyWindow(hWnd);
	  finddialog=NULL;
	  break;
	}
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
    }
  return 0;
}

/**************************************************************************

**************************************************************************/
void
popup_find_dialog(void)
{
  if (!finddialog) {
    struct fcwin_box *vbox;
    struct fcwin_box *hbox;
    finddialog=fcwin_create_layouted_window(find_city_proc,_("Find City"),
					    WS_OVERLAPPEDWINDOW,
					    CW_USEDEFAULT,CW_USEDEFAULT,
					    root_window,NULL,
					    REAL_CHILD,
					    NULL);
    vbox=fcwin_vbox_new(finddialog,FALSE);
    fcwin_box_add_static(vbox,_("Select a city:"),0,SS_LEFT,FALSE,FALSE,5);
    fcwin_box_add_list(vbox,20,ID_FINDCITY_LIST,LBS_HASSTRINGS | LBS_STANDARD,
		       TRUE,TRUE,15);
    hbox=fcwin_hbox_new(finddialog,TRUE);
    fcwin_box_add_button(hbox,_("Center"),ID_FINDCITY_CENTER,0,TRUE,TRUE,25);
    fcwin_box_add_button(hbox,_("Cancel"),ID_FINDCITY_CANCEL,0,TRUE,TRUE,25);
    fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
    fcwin_set_box(finddialog,vbox);
  }
  update_find_dialog();
  ShowWindow(finddialog,SW_SHOWNORMAL);
}
