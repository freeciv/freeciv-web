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
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "city.h"
#include "packets.h"
#include "unit.h"

/* client */
#include "chatline.h"
#include "citydlg.h"
#include "cityrepdata.h"
#include "client_main.h"
#include "climisc.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "optiondlg.h"
#include "options.h"
#include "repodlgs.h"
#include "text.h"

#include "cityrep.h"

extern HINSTANCE freecivhinst;
extern HFONT font_12courier;

int max_changemenu_id;
int max_supportmenu_id;
int max_presentmenu_id;
int max_availablemenu_id;
int max_improvement_id;
static int city_sort_id;
static int city_sort_order;
static HMENU menu_shown; 
static HWND *sort_buttons;
HWND cityrep_list;

#define ID_CITYREP_SORTBASE 6400
#define ID_CHANGE_POPUP_BASE 7000
#define ID_SUPPORTED_POPUP_BASE 8000
#define ID_PRESENT_POPUP_BASE 9000
#define ID_AVAILABLE_POPUP_BASE 10000
#define ID_IMPROVEMENTS_POPUP_BASE 11000
#define ID_CITYREP_POPUP_ALL 6900
#define ID_CITYREP_POPUP_NO 6910
#define ID_CITYREP_POPUP_INVERT 6920
#define ID_CITYREP_POPUP_COASTAL 6930
#define ID_CITYREP_POPUP_ISLAND 6940
#define ID_CITYREP_CONFIG_BASE 6500
#define NEG_VAL(x)  ((x)<0 ? (x) : (-x))     

static RECT list_singlechar;
static HWND hCityRep;
static HWND hChangeAll;
static HWND hCityRepConfig;
extern HWND root_window;
LONG APIENTRY ConfigCityRepProc(HWND hWnd,
				UINT message,
				UINT wParam,
				LONG lParam);
/**************************************************************************

**************************************************************************/
static LONG APIENTRY city_report_proc(HWND hWnd,
				      UINT uMsg,
				      UINT wParam,
				      LONG lParam);

static void popup_city_report_config_dialog()
{
  int i;
  if (!hCityRepConfig)
    {
      hCityRepConfig=
	fcwin_create_layouted_window(ConfigCityRepProc,
				     _("Configure City Report"),
				     WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
				     WS_MINIMIZEBOX,
				     20,20,
				     hCityRep,NULL,
				     REAL_CHILD,
				     NULL);
				  
      ShowWindow(hCityRepConfig,SW_SHOWNORMAL);
    }
  for(i=1; i<NUM_CREPORT_COLS; i++) {
    
    CheckDlgButton(hCityRepConfig,i+ID_CITYREP_CONFIG_BASE,
		   city_report_specs[i].show?BST_CHECKED:BST_UNCHECKED);
  }
			       
}


/**************************************************************************

**************************************************************************/
static void create_cityrep_config_dlg(HWND hWnd)
{
  int i;
  struct fcwin_box *box;
  struct city_report_spec *spec;   
  box=fcwin_vbox_new(hWnd,FALSE);
  fcwin_box_add_static_default(box,_("Set columns shown"),-1,SS_CENTER);
  for(i=0, spec=city_report_specs+i; i<NUM_CREPORT_COLS; i++, spec++) {     
    fcwin_box_add_checkbox(box, spec->explanation,
			   ID_CITYREP_CONFIG_BASE+i,0,FALSE,FALSE,5);
  }
  fcwin_box_add_button(box,_("Close"),IDOK,0,FALSE,FALSE,15);
  fcwin_set_box(hWnd,box);
}

/**************************************************************************

**************************************************************************/
LONG APIENTRY ConfigCityRepProc(HWND hWnd,
				UINT message,
				UINT wParam,
				LONG lParam)
{
  switch(message)
    {
    case WM_CREATE:
      create_cityrep_config_dlg(hWnd);
      return 0;
    case WM_DESTROY:
      hCityRepConfig=NULL;
      break;
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;
    case WM_COMMAND:
      if (LOWORD(wParam)==IDOK)
	{
	  struct city_report_spec *spec;   
	  int i;
	  for(i=0, spec=city_report_specs+i; i<NUM_CREPORT_COLS; i++, spec++) 
	    {   
	      spec->show=IsDlgButtonChecked(hWnd,ID_CITYREP_CONFIG_BASE+i);  
	    }
	  DestroyWindow(hWnd);
	  DestroyWindow(hCityRep);
	  popup_city_report_dialog(FALSE);
	}
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam); 
    }
  return 0;
}

/**************************************************************************

**************************************************************************/
static void
append_impr_or_unit_to_menu_sub(HMENU menu,
				char *nothing_appended_text,
				int append_units,
				int append_wonders,
				int change_prod,
				TestCityFunc test_func,
				int *selitems,
				int selcount, int *idcount)
{
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  int item, targets_used, num_selected_cities = 0;
  struct city **selected_cities = NULL;

  if (change_prod) {
    int j;
    HWND hList = GetDlgItem(hCityRep, ID_CITYREP_LIST);

    num_selected_cities = selcount;
    selected_cities =
	fc_malloc(sizeof(*selected_cities) * num_selected_cities);
    
    for (j = 0; j < num_selected_cities; j++) {
      selected_cities[j] = (struct city *) ListBox_GetItemData(hList,
							       selitems[j]);
    }
  }

  targets_used = collect_production_targets(targets, selected_cities,
			    num_selected_cities, append_units,
			    append_wonders, change_prod, test_func);
  if (selected_cities) {
    free(selected_cities);
  }
  name_and_sort_items(targets, targets_used, items, change_prod, NULL);

  for (item = 0; item < targets_used; item++) {
    MENUITEMINFO iteminfo;

    AppendMenu(menu, MF_STRING, (*idcount), items[item].descr);
    iteminfo.dwItemData = cid_encode(items[item].item);
    iteminfo.fMask = MIIM_DATA;
    iteminfo.cbSize = sizeof(MENUITEMINFO);
    SetMenuItemInfo(menu, (*idcount), FALSE, &iteminfo);
    (*idcount)++;
  }

  if (targets_used == 0) {
    AppendMenu(menu, MF_STRING, -1, nothing_appended_text);
  }
}

/**************************************************************************

**************************************************************************/
static void append_impr_or_unit_to_menu(HMENU menu,
					int change_prod,
					int append_improvements,
					int append_units,
					TestCityFunc test_func,
					int *selitems, 
					int selcount, 
					int *idcount)
{
  if (append_improvements) {
    /* Add all buildings */
    append_impr_or_unit_to_menu_sub(menu, _("No Buildings Available"),
				    FALSE, FALSE, change_prod,
				    test_func,
				    selitems, selcount, idcount);
    /* Add a separator */
    AppendMenu(menu, MF_SEPARATOR, -1, NULL);
  }

  if (append_units) {
    /* Add all units */
    append_impr_or_unit_to_menu_sub(menu, _("No Units Available"),
				    TRUE, FALSE, change_prod,
				    test_func,
				    selitems, selcount, idcount);
  }

  if (append_improvements) {
    if (append_units) {
      /* Add a separator */
	AppendMenu(menu, MF_SEPARATOR, -1, NULL);
    }

    /* Add all wonders */
    append_impr_or_unit_to_menu_sub(menu, _("No Wonders Available"),
				    FALSE, TRUE, change_prod,
				    test_func,
				    selitems, selcount, idcount);
  }
}

/****************************************************************
 Return text line for the column headers for the city report
*****************************************************************/
static void get_city_table_header(char *text[], int n)
{
  struct city_report_spec *spec;
  int i;
  for(i=0, spec=city_report_specs; i<NUM_CREPORT_COLS; i++, spec++) {
    my_snprintf(text[i], n, "%*s\n%*s",
            NEG_VAL(spec->width), spec->title1 ? spec->title1: "",
            NEG_VAL(spec->width), spec->title2 ? spec->title2: "");
  }
}             
                  
/**************************************************************************

**************************************************************************/
static void get_city_text(struct city *pcity, char *buf[], int n)
{
  struct city_report_spec *spec;
  int i;
 
  for(i=0, spec=city_report_specs; i<NUM_CREPORT_COLS; i++, spec++) {
    buf[i][0]='\0';
    if(!spec->show) continue;
    my_snprintf(buf[i], n, "%*s", NEG_VAL(spec->width)-2,
		(spec->func)(pcity, spec->data)); 
  }
}

/**************************************************************************

**************************************************************************/
static void cityrep_center(HWND hWnd)
{
  int selcount;
  int id;
  struct city *pcity;
  selcount=ListBox_GetSelCount(GetDlgItem(hWnd,ID_CITYREP_LIST));
  if (selcount!=1)
    return;
  if (ListBox_GetSelItems(GetDlgItem(hWnd,ID_CITYREP_LIST),1,&id)!=1)
    return;
  pcity=(struct city *)ListBox_GetItemData(GetDlgItem(hWnd,ID_CITYREP_LIST),
					   id);
  center_tile_mapcanvas(pcity->tile);
}

/**************************************************************************

**************************************************************************/
static void cityrep_popup(HWND hWnd)
{
  int cityids[256];
  int selcount;
  int i;
  struct city *pcity;
  selcount=ListBox_GetSelCount(GetDlgItem(hWnd,ID_CITYREP_LIST));
  if (selcount==LB_ERR) return;
  selcount=MIN(256,selcount);
  selcount=ListBox_GetSelItems(GetDlgItem(hWnd,ID_CITYREP_LIST),
			       selcount,&cityids[0]);
  for(i=0;i<selcount;i++)
    {
      pcity=(struct city *)ListBox_GetItemData(GetDlgItem(hWnd,ID_CITYREP_LIST),
				cityids[i]);
      popup_city_dialog(pcity);
    }
}

/**************************************************************************

**************************************************************************/
static void cityrep_do_buy(HWND hWnd)
{
  int cityids[256];
  int i;
  int selcount = ListBox_GetSelCount(GetDlgItem(hWnd, ID_CITYREP_LIST));

  if (selcount == LB_ERR) {
    return;
  }
  selcount = MIN(256, selcount);
  selcount = ListBox_GetSelItems(GetDlgItem(hWnd, ID_CITYREP_LIST),
				 selcount, &cityids[0]);
  for (i = 0; i < selcount; i++) {
    cityrep_buy((struct city *) ListBox_GetItemData(GetDlgItem(hWnd,
							       ID_CITYREP_LIST),
						    cityids[i]));
  }
}
  
#if 0
/**************************************************************************

**************************************************************************/
static void cityrep_refresh(HWND hWnd)
{
    int cityids[256];
  int selcount;
  int i;
  struct city *pcity; 
  struct packet_generic_integer packet;    
  selcount=ListBox_GetSelCount(GetDlgItem(hWnd,ID_CITYREP_LIST));
  if ((selcount==LB_ERR)||(selcount==0))
    {
      packet.value = 0;
      send_packet_generic_integer(&client.conn, PACKET_CITY_REFRESH, &packet);
      return;
    }
  selcount=MIN(256,selcount);
  selcount=ListBox_GetSelItems(GetDlgItem(hWnd,ID_CITYREP_LIST),
			       selcount,&cityids[0]);
  for (i=0;i<selcount;i++)
    {
      pcity=(struct city *)ListBox_GetItemData(GetDlgItem(hWnd,
							  ID_CITYREP_LIST),
					       cityids[i]);
      packet.value = pcity->id;
      send_packet_generic_integer(&client.conn, PACKET_CITY_REFRESH,
				  &packet);      
    }
}
#endif

/**************************************************************************

**************************************************************************/

static void cityrep_change(HWND hWnd)
{    
  int cityids[256];
  int selcount;
  HMENU popup;
  RECT rc;
  selcount=ListBox_GetSelCount(GetDlgItem(hWnd,ID_CITYREP_LIST));

  if ((selcount==LB_ERR)||(selcount==0))
    return;
  selcount=MIN(256,selcount);
  selcount=ListBox_GetSelItems(GetDlgItem(hWnd,ID_CITYREP_LIST),
			       selcount,&cityids[0]);
  popup=CreatePopupMenu();
  max_changemenu_id=ID_CHANGE_POPUP_BASE;
  append_impr_or_unit_to_menu(popup,TRUE,TRUE,TRUE,
			      can_city_build_now,
			      &cityids[0],selcount,&max_changemenu_id);
  GetWindowRect(GetDlgItem(hWnd,ID_CITYREP_CHANGE),&rc);
  menu_shown=popup;
  TrackPopupMenu(popup,0,rc.left,rc.top,0,hWnd,NULL);
}

/**************************************************************************

**************************************************************************/
static void cityrep_select(HWND hWnd)
{
  int cityids; /* We do not need to initialize it because selcount is 0
		  We only have it to satisfy  append_impr_or_unit_to_menu */
  int selcount;
  RECT rc;
  HMENU popup,submenu;
  selcount=0;
  popup=CreatePopupMenu();
  AppendMenu(popup,MF_STRING,ID_CITYREP_POPUP_ALL,_("All Cities"));
  AppendMenu(popup,MF_STRING,ID_CITYREP_POPUP_NO,_("No Cities"));
  AppendMenu(popup,MF_STRING,ID_CITYREP_POPUP_INVERT,_("Invert Selection"));
  AppendMenu(popup,MF_SEPARATOR,0,NULL);
  AppendMenu(popup,MF_STRING,ID_CITYREP_POPUP_COASTAL,_("Coastal Cities"));
  AppendMenu(popup,MF_STRING,ID_CITYREP_POPUP_ISLAND,_("Same Island"));
  AppendMenu(popup,MF_SEPARATOR,0,NULL);  
  submenu=CreatePopupMenu();
  AppendMenu(popup,MF_POPUP,(UINT)submenu,_("Supported Units"));
  max_supportmenu_id=ID_SUPPORTED_POPUP_BASE;

  append_impr_or_unit_to_menu(submenu, FALSE, FALSE, TRUE, 
			      city_unit_supported,&cityids,selcount,
			      &max_supportmenu_id);
  submenu=CreatePopupMenu();
  AppendMenu(popup,MF_POPUP,(UINT)submenu,_("Units Present"));
  
  max_presentmenu_id=ID_PRESENT_POPUP_BASE;
  append_impr_or_unit_to_menu(submenu, FALSE, FALSE, TRUE, 
			      city_unit_present,&cityids,selcount,
			      &max_presentmenu_id); 
  
  submenu=CreatePopupMenu();
  AppendMenu(popup,MF_POPUP,(UINT)submenu,_("Available To Build"));
  max_availablemenu_id=ID_AVAILABLE_POPUP_BASE;
  append_impr_or_unit_to_menu(submenu, FALSE, TRUE, TRUE,
			      can_city_build_now,&cityids,selcount,
			      &max_availablemenu_id);
  
  submenu=CreatePopupMenu(); 
  AppendMenu(popup,MF_POPUP,(UINT)submenu,_("Improvements in City"));
  max_improvement_id=ID_IMPROVEMENTS_POPUP_BASE;
  append_impr_or_unit_to_menu(submenu, FALSE, TRUE, FALSE,
			      city_building_present,
			      &cityids,selcount,
			      &max_improvement_id);
  
  GetWindowRect(GetDlgItem(hWnd,ID_CITYREP_SELECT),&rc);
  menu_shown=popup;
  TrackPopupMenu(popup,0,rc.left,rc.top,0,hWnd,NULL);
}

/**************************************************************************

**************************************************************************/
static void cityrep_change_menu(HWND hWnd, cid cid)
{  
  int cityids[256];
  int selcount, i, last_request_id = 0;
  struct city *pcity;
  struct universal target = cid_production(cid);
  
  selcount=ListBox_GetSelCount(GetDlgItem(hWnd,ID_CITYREP_LIST));
  if (selcount==LB_ERR) return;
  selcount=MIN(256,selcount);
  selcount=ListBox_GetSelItems(GetDlgItem(hWnd,ID_CITYREP_LIST),
			       selcount,&cityids[0]);

  connection_do_buffer(&client.conn);
  for (i = 0; i < selcount; i++) {
    pcity = (struct city *) ListBox_GetItemData(GetDlgItem(hWnd,
							   ID_CITYREP_LIST),
						cityids[i]);
    last_request_id =
      city_change_production(pcity, target);
    ListBox_SetSel(GetDlgItem(hWnd, ID_CITYREP_LIST), FALSE, cityids[i]);
  }

  connection_do_unbuffer(&client.conn);
  reports_freeze_till(last_request_id);
}

/**************************************************************************

**************************************************************************/
static void list_all_select(HWND hLst,int state)
{
  int i;
  int num;
  num=ListBox_GetCount(hLst);
  for (i=0;i<num;i++)
    {
      ListBox_SetSel(hLst,state,i);
    }
}

/**************************************************************************

**************************************************************************/
static void list_invert_select(HWND hLst)
{
  int i;
  int num;
  num=ListBox_GetCount(hLst);
  for (i=0;i<num;i++)
    {
      ListBox_SetSel(hLst,!ListBox_GetSel(hLst,i),i);
    }
}

/**************************************************************************

**************************************************************************/
static void list_coastal_select(HWND hLst)
{
  int num,i;
  struct city *pcity;
  num=ListBox_GetCount(hLst);
  list_all_select(hLst,FALSE);
  for (i=0;i<num;i++)
    {
      pcity=(struct city *)ListBox_GetItemData(hLst,i);
      if (is_ocean_near_tile(pcity->tile)) {
	ListBox_SetSel(hLst,TRUE,i);
      }
    }
}

/**************************************************************************

**************************************************************************/
static void list_sameisland_select(HWND hLst)
{
  struct city *pcity;
  int cityids[256];
  int selcount;
  int i,j;
  int num;
  selcount=ListBox_GetSelCount(hLst);
  if (selcount==LB_ERR) return;
    selcount=MIN(256,selcount);
  selcount=ListBox_GetSelItems(hLst,
			       selcount,&cityids[0]);
  num=ListBox_GetCount(hLst);
  for (i=0;i<num;i++)
    {
      pcity=(struct city *)ListBox_GetItemData(hLst,i);
      for (j=0;j<selcount;j++)
	{
	  struct city *selectedcity;
	  selectedcity=(struct city *)ListBox_GetItemData(hLst,cityids[j]);
          if (tile_continent(pcity->tile)
              == tile_continent(selectedcity->tile))
	    {    
	      ListBox_SetSel(hLst,TRUE,i);
	      break;
	    }
	}
    }
} 

/**************************************************************************

**************************************************************************/
static void list_impr_or_unit_select(HWND hLst,
				     struct universal target,
				     TestCityFunc test_func)
{
  int i,rows;
  list_all_select(hLst,FALSE);
  rows=ListBox_GetCount(hLst);
  for (i=0;i<rows;i++)
    {
      struct city *pcity=(struct city *)ListBox_GetItemData(hLst,i);
      if (test_func(pcity, target))
	ListBox_SetSel(hLst,TRUE,i);
    }
}

/**************************************************************************

**************************************************************************/
static void menu_proc(HWND hWnd,int cmd, DWORD num)
{
  HWND hLst;
  struct universal target = cid_decode(num);
  hLst=GetDlgItem(hWnd,ID_CITYREP_LIST);
  if ((cmd>=ID_CHANGE_POPUP_BASE)&&
      (cmd<max_changemenu_id))
    {
      cityrep_change_menu(hWnd,num);
      max_changemenu_id=0;
    }
  if ((cmd>=ID_SUPPORTED_POPUP_BASE)&&
      (cmd<max_supportmenu_id))
    {
      list_impr_or_unit_select(hLst, target, city_unit_supported);
      max_supportmenu_id=0;
    }
  if ((cmd>=ID_PRESENT_POPUP_BASE)&&
      (cmd<max_presentmenu_id))
    {
      list_impr_or_unit_select(hLst, target, city_unit_present);
      max_presentmenu_id=0;
    }
  if ((cmd>=ID_AVAILABLE_POPUP_BASE)&&
      (cmd<max_availablemenu_id))
    {
      list_impr_or_unit_select(hLst, target, can_city_build_now);
      max_availablemenu_id=0;
    }
  if ((cmd>=ID_IMPROVEMENTS_POPUP_BASE)&&
      (cmd<max_improvement_id))
    {
      list_impr_or_unit_select(hLst, target, city_building_present);
      max_improvement_id=0;
    }
}

/**************************************************************************

**************************************************************************/
static LONG CALLBACK cityrep_changeall_proc(HWND hWnd,
					    UINT message,
					    WPARAM wParam,
					    LPARAM lParam)  
{
  switch (message)
    {
    case WM_CREATE:
      break;
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;
    case WM_SIZE:
    case WM_GETMINMAXINFO:
    case WM_DESTROY:    
      hChangeAll=NULL;
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case ID_PRODCHANGE_CANCEL:
	  DestroyWindow(hWnd);
	  hChangeAll=NULL;
	  break;
	case ID_PRODCHANGE_CHANGE:
	  {
	    int id;
	    int from, to;

	    id=ListBox_GetCurSel(GetDlgItem(hWnd,ID_PRODCHANGE_FROM));
	    if (id==LB_ERR)
	      {
                output_window_append(FTC_CLIENT_INFO, NULL,
                                     _("Select a unit or improvement"
                                       " to change production from."));
		break;        
	      }
	    from=ListBox_GetItemData(GetDlgItem(hWnd,ID_PRODCHANGE_FROM),
				     id);
	    id=ListBox_GetCurSel(GetDlgItem(hWnd,ID_PRODCHANGE_TO));
	    if (id==LB_ERR)
	      {
                output_window_append(FTC_CLIENT_INFO, NULL,
                                     _("Select a unit or improvement"
                                       " to change production to."));
		break;          
	      }
	    to=ListBox_GetItemData(GetDlgItem(hWnd,ID_PRODCHANGE_TO),id);
	    if (from==to) {
              output_window_append(FTC_CLIENT_INFO, NULL,
                                   _("That's the same thing!"));
	      break;
	    }
	    client_change_all(cid_decode(from), cid_decode(to));
	    DestroyWindow(hWnd);
	    hChangeAll=NULL;
	  }
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
static void cityrep_changeall(HWND hWnd)
{ 
  struct fcwin_box *vbox;
  struct fcwin_box *hbox;
  int selid;
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  int targets_used;
  cid selected_cid;
  int id,i;
  HWND hDlg;
  HWND hLst;
  if (hChangeAll)
    return;
  hDlg=fcwin_create_layouted_window(cityrep_changeall_proc,
				    _("Change Production Everywhere"),
				    WS_OVERLAPPEDWINDOW,
				    CW_USEDEFAULT,CW_USEDEFAULT,
				    hCityRep,NULL,
				    REAL_CHILD,
				    NULL);
  hbox=fcwin_hbox_new(hDlg,TRUE);
  vbox=fcwin_vbox_new(hDlg,FALSE);
  fcwin_box_add_static(vbox,_("From:"),0,SS_LEFT,FALSE,FALSE,5);
  fcwin_box_add_list(vbox,10,ID_PRODCHANGE_FROM,
		     WS_VSCROLL | WS_VISIBLE | LBS_HASSTRINGS,
		     TRUE,TRUE,10);
  fcwin_box_add_button(vbox,_("Change"),ID_PRODCHANGE_CHANGE,0,
		       FALSE,FALSE,15);
  fcwin_box_add_box(hbox,vbox,TRUE,TRUE,5);
  vbox=fcwin_vbox_new(hDlg,FALSE);
  fcwin_box_add_static(vbox,_("To:"),0,SS_LEFT,FALSE,FALSE,5);
  fcwin_box_add_list(vbox,10,ID_PRODCHANGE_TO,WS_VSCROLL | 
		     WS_VISIBLE | LBS_HASSTRINGS,
		     TRUE,TRUE,10);
  fcwin_box_add_button(vbox,_("Cancel"),ID_PRODCHANGE_CANCEL,0,
		       FALSE,FALSE,15);
  fcwin_box_add_box(hbox,vbox,TRUE,TRUE,5);

  selected_cid = -1;
  selid = ListBox_GetCurSel(GetDlgItem(hWnd, ID_CITYREP_LIST));
  if (selid != LB_ERR) {
    selected_cid =
	cid_encode_from_city((struct city *)
			     ListBox_GetItemData(GetDlgItem
						 (hWnd, ID_CITYREP_LIST),
						 selid));
  }

  targets_used = collect_currently_building_targets(targets);
  name_and_sort_items(targets, targets_used, items, FALSE, NULL);

  hLst = GetDlgItem(hDlg, ID_PRODCHANGE_FROM);
  for (i = 0; i < targets_used; i++) {
    id = ListBox_AddString(hLst, items[i].descr);
    ListBox_SetItemData(hLst, id, cid_encode(items[i].item));
    if (cid_encode(items[i].item) == selected_cid)
      ListBox_SetCurSel(hLst, id);
  }

  targets_used = collect_buildable_targets(targets);
  name_and_sort_items(targets, targets_used, items, TRUE, NULL);

  hLst = GetDlgItem(hDlg, ID_PRODCHANGE_TO);
  for (i = 0; i < targets_used; i++) {
    id = ListBox_AddString(hLst, items[i].descr);
    ListBox_SetItemData(hLst, id, cid_encode(items[i].item));
  }

  fcwin_set_box(hDlg,hbox);
  hChangeAll=hDlg;
  ShowWindow(hDlg,SW_SHOWNORMAL);
}

/**************************************************************************

**************************************************************************/
static void list_minsize(LPPOINT minsize,void *data)
{
  int i,width_total,width;
  struct city_report_spec *spec;
  HDC hdc;
  HFONT old;
  HWND hWnd;
  hWnd=(HWND)data;
  hdc=GetDC(hWnd);
  old=SelectObject(hdc,font_12courier);
  DrawText(hdc,"X",1,&list_singlechar,DT_CALCRECT);
  SelectObject(hdc,old);
  ReleaseDC(hWnd,hdc);
  width_total=0;
  for(i=0, spec=city_report_specs; i<NUM_CREPORT_COLS; i++, spec++) {    
    if (!spec->show) continue;
    width=spec->width>0?spec->width:-spec->width;
    width = MAX(strlen(spec->title1 ? spec->title1: ""), width);
    width = MAX(strlen(spec->title2 ? spec->title2: ""), width);
    width_total+=width+2;
  }
  minsize->x=(list_singlechar.right-list_singlechar.left)*width_total;
  minsize->y=(list_singlechar.bottom-list_singlechar.top)*8;
  
}

/**************************************************************************

**************************************************************************/
static void list_setsize(LPRECT setsize,void *data)
{
  int i,x,y,button_h,w;  
  struct city_report_spec *spec;   
  x=setsize->left;
  y=setsize->top;
  button_h=(list_singlechar.bottom-list_singlechar.top)*2+5;
  for(i=0, spec=city_report_specs; i<NUM_CREPORT_COLS; i++, spec++) {
    if (!sort_buttons[i]) continue;
    w=spec->width>0?spec->width:-spec->width;
    w = MAX(strlen(spec->title1 ? spec->title1: ""), w);
    w = MAX(strlen(spec->title2 ? spec->title2: ""), w);
    w+=2;
    w*=(list_singlechar.right-list_singlechar.left);
    MoveWindow(sort_buttons[i],x,y,w-1,button_h,TRUE);
    x+=w;
  }   
  MoveWindow(cityrep_list,setsize->left,y+button_h,
	     setsize->right-setsize->left,
	     setsize->bottom-setsize->top-button_h,TRUE);
}

/**************************************************************************

**************************************************************************/
static void list_del(void *data)
{
  int i;
  for (i=0;i<NUM_CREPORT_COLS;i++)
    {
      if (sort_buttons[i])
	{
	  DestroyWindow(sort_buttons[i]);
	  sort_buttons[i]=NULL;
	}
    }
  DestroyWindow(cityrep_list);
}

/**************************************************************************

**************************************************************************/
static void city_report_create(HWND hWnd)
{
  static char **titles;
  static char (*buf)[64];   
  struct city_report_spec *spec;        
  struct fcwin_box *vbox;
  struct fcwin_box *hbox;
  int i;
  hbox=fcwin_hbox_new(hWnd,FALSE);
  fcwin_box_add_button_default(hbox,_("Close"),ID_CITYREP_CLOSE,0);
  fcwin_box_add_button_default(hbox,_("Center"),ID_CITYREP_CENTER,0);
  fcwin_box_add_button_default(hbox,_("Popup"),ID_CITYREP_POPUP,0);
  fcwin_box_add_button_default(hbox,_("Buy"),ID_CITYREP_BUY,0);
  fcwin_box_add_button_default(hbox,_("Change"),ID_CITYREP_CHANGE,0);
  fcwin_box_add_button_default(hbox,_("Change All"),ID_CITYREP_CHANGEALL,0);
  fcwin_box_add_button_default(hbox,_("Refresh"),ID_CITYREP_REFRESH,0);
  fcwin_box_add_button_default(hbox,_("Select"),ID_CITYREP_SELECT,0);
  fcwin_box_add_button_default(hbox,_("Configure"),ID_CITYREP_CONFIG,0);
  vbox=fcwin_vbox_new(hWnd,FALSE);
  fcwin_box_add_static(vbox,get_report_title(_("City Advisor")),
		       ID_CITYREP_TOP,SS_CENTER,FALSE,FALSE,0);
  fcwin_box_add_generic(vbox,list_minsize,list_setsize,list_del,(void *)hWnd,
			TRUE,TRUE,5);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  
  if (titles) {
    free(titles);
  }
  if (buf) {
    free(buf);
  }
  if (sort_buttons) {
    free(sort_buttons);
  }
  
  titles = fc_malloc(sizeof(*titles) * NUM_CREPORT_COLS);
  buf = fc_malloc(sizeof(*buf) * NUM_CREPORT_COLS);
  sort_buttons = fc_malloc(sizeof(*sort_buttons) * NUM_CREPORT_COLS);

  for (i=0;i<NUM_CREPORT_COLS;i++)
    titles[i]=buf[i];
  
  get_city_table_header(titles, sizeof(buf[0]));  
  for(i=0, spec=city_report_specs; i<NUM_CREPORT_COLS; i++, spec++) {         
    if (spec->show) {
      sort_buttons[i]=CreateWindow("BUTTON", titles[i], WS_CHILD | WS_VISIBLE
				   | BS_MULTILINE, 0, 0, 0, 0, hWnd,
				   (HMENU)(ID_CITYREP_SORTBASE+i),
				   freecivhinst, NULL);
      SendMessage(sort_buttons[i], WM_SETFONT, (WPARAM)font_12courier,
		  MAKELPARAM(TRUE,0));
    } else {
      sort_buttons[i] = NULL;
    }
  }

  cityrep_list=CreateWindow("LISTBOX",NULL,WS_CHILD | WS_VISIBLE | 
			    LBS_HASSTRINGS | LBS_NOTIFY | LBS_EXTENDEDSEL | 
			    WS_VSCROLL,
			    0,0,0,0,
			    hWnd,
			    (HMENU)ID_CITYREP_LIST,
			    freecivhinst,
			    NULL);
  SendMessage(cityrep_list,
	      WM_SETFONT,(WPARAM) font_12courier,MAKELPARAM(TRUE,0));
  fcwin_set_box(hWnd,vbox);
}

/**************************************************************************

**************************************************************************/
static LONG APIENTRY city_report_proc(HWND hWnd,
				      UINT uMsg,
				      UINT wParam,
				      LONG lParam)

{
  int selcount;
  int is_menu;
  int cmd_id;
  MENUITEMINFO iteminfo;
  iteminfo.cbSize=sizeof(MENUITEMINFO);
  iteminfo.fMask=MIIM_DATA;
  switch(uMsg)
    {
    case WM_CREATE:
      {
	city_report_create(hWnd);
      }
      break;
    case WM_DESTROY:
      if (menu_shown) {
	DestroyMenu(menu_shown);
	menu_shown=NULL;
      }
      hCityRep=NULL;
      break; 
    case WM_GETMINMAXINFO:
      break;
    case WM_SIZE:
    
      break;
    case WM_COMMAND:
      is_menu=0;
      cmd_id=LOWORD(wParam);
      if (menu_shown)
	{
	  is_menu=GetMenuItemInfo(menu_shown,cmd_id,FALSE,&iteminfo);
	  DestroyMenu(menu_shown);
	  menu_shown=NULL;
	}
      switch (cmd_id)
	{
	  
	case ID_CITYREP_CLOSE:
	  DestroyWindow(hCityRep);
	  break;
	case ID_CITYREP_LIST:
	  selcount=ListBox_GetSelCount(GetDlgItem(hWnd,ID_CITYREP_LIST));
	  if ((selcount==LB_ERR)||(selcount==0))
	    {
	      EnableWindow(GetDlgItem(hWnd,ID_CITYREP_CENTER),FALSE);
	      EnableWindow(GetDlgItem(hWnd,ID_CITYREP_POPUP),FALSE);
	      EnableWindow(GetDlgItem(hWnd,ID_CITYREP_BUY),FALSE);
	      EnableWindow(GetDlgItem(hWnd,ID_CITYREP_CHANGE),FALSE);
	    }
	  else 
	    {
	      EnableWindow(GetDlgItem(hWnd,ID_CITYREP_CENTER),(selcount==1));
	      EnableWindow(GetDlgItem(hWnd,ID_CITYREP_POPUP),TRUE);
	      EnableWindow(GetDlgItem(hWnd,ID_CITYREP_BUY),TRUE);
	      EnableWindow(GetDlgItem(hWnd,ID_CITYREP_CHANGE),TRUE);
	    }
	  break; 
	case ID_CITYREP_CENTER:
	  cityrep_center(hWnd);
	  break;
	case ID_CITYREP_POPUP:
	  cityrep_popup(hWnd);
	  break;
	case ID_CITYREP_BUY:
	  cityrep_do_buy(hWnd);
	  break;
	case ID_CITYREP_CHANGE:
	  cityrep_change(hWnd);
	  break;
	case ID_CITYREP_CHANGEALL:
	  cityrep_changeall(hWnd);
	  break;
	case ID_CITYREP_SELECT:
	  cityrep_select(hWnd);
	  break;
	case ID_CITYREP_CONFIG:
	  popup_city_report_config_dialog();        
	  break;
	case ID_CITYREP_POPUP_ALL:
	  list_all_select(GetDlgItem(hWnd,ID_CITYREP_LIST),TRUE);
	  break;
	case ID_CITYREP_POPUP_NO:
	  list_all_select(GetDlgItem(hWnd,ID_CITYREP_LIST),FALSE);
	  break;
	case ID_CITYREP_POPUP_INVERT:
	  list_invert_select(GetDlgItem(hWnd,ID_CITYREP_LIST));
	  break;
	case ID_CITYREP_POPUP_COASTAL:
	  list_coastal_select(GetDlgItem(hWnd,ID_CITYREP_LIST));
	  break;
	case ID_CITYREP_POPUP_ISLAND:
	  list_sameisland_select(GetDlgItem(hWnd,ID_CITYREP_LIST));
	  break;
	}
      if ((cmd_id>=ID_CITYREP_SORTBASE)&&
	  (cmd_id<(ID_CITYREP_SORTBASE+NUM_CREPORT_COLS)))
      {
	int new_sort_id;
	new_sort_id=cmd_id-ID_CITYREP_SORTBASE;
	if (new_sort_id==city_sort_id)
	  {
	    city_sort_order=!city_sort_order;
	  }
	else
	  {
	    city_sort_order=0;
	    city_sort_id=new_sort_id;
	  }
	city_report_dialog_update();
      }
      if (is_menu)
	{
	  menu_proc(hWnd,cmd_id,iteminfo.dwItemData);
	}
      break;
    case WM_CLOSE:
      DestroyWindow(hCityRep);      
      break;
    default:
      return DefWindowProc(hWnd,uMsg,wParam,lParam);
    }
  return 0;
}


/**************************************************************************

**************************************************************************/
void
popup_city_report_dialog(bool raise)
{
  if (!hCityRep) {
    hCityRep =
      fcwin_create_layouted_window(city_report_proc,_("City Report"),
				   WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX
				    | WS_MAXIMIZEBOX | WS_THICKFRAME,
				   CW_USEDEFAULT, CW_USEDEFAULT,
				   root_window, NULL, JUST_CLEANUP, NULL);
  
    hChangeAll = NULL;
    city_report_dialog_update();
  }

  ShowWindow(hCityRep, SW_SHOWNORMAL);
  if (raise) {
    SetFocus(hCityRep);
  }
}

/**************************************************************************

**************************************************************************/
static int compare_cities(char *rowold[],char *row[])
{
  int tmp;
  tmp=mystrcasecmp(rowold[city_sort_id],row[city_sort_id]);
  return city_sort_order?tmp:-tmp;
}

/**************************************************************************

**************************************************************************/
static void my_cityrep_add_city(HWND hLst,struct city *pcity_new,char *full_row,
				char *row[])
{
  int id;
  int i,n,listcount;  
  struct city_report_spec *spec; 
  char *rowold   [NUM_CREPORT_COLS];
  char  buf   [NUM_CREPORT_COLS][128];
  for (i=0, spec=city_report_specs;i<NUM_CREPORT_COLS;i++, spec++)
    {
      rowold[i] = buf[i];    
    }
  listcount=ListBox_GetCount(hLst);
  for (n=0;n<listcount;n++)
    {
      struct city *pcity;
      pcity=(struct city *)ListBox_GetItemData(hLst,n);
      get_city_text(pcity,rowold,sizeof(buf[0]));
      if (compare_cities(rowold,row)>0)
	{
	  id=ListBox_InsertString(hLst,n,full_row);
	  ListBox_SetItemData(hLst,id,pcity_new);
	  return;
	}
    }
  ListBox_InsertString(hLst,listcount,full_row);
  ListBox_SetItemData(hLst,listcount,pcity_new);

}

/**************************************************************************

**************************************************************************/
void
city_report_dialog_update(void)
{ 
  char *row   [NUM_CREPORT_COLS];
  char  buf   [NUM_CREPORT_COLS][128];
  char full_row[1024];
  int   i;
  struct city_report_spec *spec; 
  if(is_report_dialogs_frozen()) return;    
  if (!hCityRep)
    return;
  SetWindowText(GetDlgItem(hCityRep, ID_CITYREP_TOP),
		get_report_title(_("City Advisor")));
  for (i=0, spec=city_report_specs;i<NUM_CREPORT_COLS;i++, spec++)
    {
      row[i] = buf[i];    
    }
  /* FIXME restore old selection */
  ListBox_ResetContent(GetDlgItem(hCityRep,ID_CITYREP_LIST));
  city_list_iterate(client.conn.playing->cities, pcity) {
    get_city_text(pcity, row, sizeof(buf[0]));
    full_row[0]=0;
    for(i=0; i<NUM_CREPORT_COLS; i++)
      {
	sz_strlcat(full_row,buf[i]);
      }
    my_cityrep_add_city(GetDlgItem(hCityRep,ID_CITYREP_LIST),pcity,
			full_row,row);
 
    
  } city_list_iterate_end;         
}


/**************************************************************************

**************************************************************************/
void
city_report_dialog_update_city(struct city *pcity)
{
  char *row [NUM_CREPORT_COLS];
  char  buf [NUM_CREPORT_COLS][128];
  char full_row[512];
  int   i,nCount;     
  struct city_report_spec *spec; 
  HWND hLst; 
  if(is_report_dialogs_frozen()) return;
  if(!hCityRep) return;     
  hLst=GetDlgItem(hCityRep,ID_CITYREP_LIST);
  for (i=0, spec=city_report_specs;i<NUM_CREPORT_COLS;i++, spec++)
    {
      row[i] = buf[i];
    }   
  get_city_text(pcity, row, sizeof(buf[0]));    
  full_row[0]=0;
  for(i=0; i<NUM_CREPORT_COLS; i++)
    {
      sz_strlcat(full_row,buf[i]);
    } 
  nCount=ListBox_GetCount(hLst);
  for(i=0;i<nCount;i++)
    {
      if (pcity==(struct city *)ListBox_GetItemData(hLst,i))
	{
	  ListBox_DeleteString(hLst,i);
	  ListBox_InsertString(hLst,i,full_row);
	  ListBox_SetItemData(hLst,i,pcity);
	}
    }
}

/****************************************************************
 After a selection rectangle is defined, make the cities that
 are hilited on the canvas exclusively hilited in the
 City List window.
*****************************************************************/
void hilite_cities_from_canvas(void)
{
  /* PORTME */
}

/****************************************************************
 Toggle a city's hilited status.
*****************************************************************/
void toggle_city_hilite(struct city *pcity, bool on_off)
{
  /* PORTME */
}
