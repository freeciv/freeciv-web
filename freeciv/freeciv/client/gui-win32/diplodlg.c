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

#include <windows.h>
#include <windowsx.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "government.h"
#include "map.h"
#include "packets.h"
#include "player.h"

/* client */
#include "canvas.h"
#include "chatline.h"
#include "client_main.h"
#include "climisc.h"
#include "diptreaty.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "sprite.h"
#include "tilespec.h"

#include "diplodlg.h"

enum Diplomacy_ids {
  ID_MAP0=100,
  ID_MAP1,
  ID_MAP_SEA,
  ID_MAP_WORLD,
  ID_TECH0,
  ID_TECH1,
  ID_CITY0,
  ID_CITY1,
  ID_GOLD0,
  ID_GOLD1,
  ID_PACT,
  ID_CEASEFIRE,
  ID_PEACE,
  ID_ALLIANCE,
  ID_VISION0,
  ID_VISION1,
  ID_EMBASSY0,
  ID_EMBASSY1,
  ID_ERASE,
  ID_LIST,
  ID_ADVANCES_BASE=1000,
  ID_CITIES_BASE=2000
};

#define MAX_NUM_CLAUSES 64

struct Diplomacy_dialog {
  struct Treaty treaty;
  
  HWND mainwin;
  HWND list;
  HWND gold0_label;
  HWND gold1_label;
  POINT thumb0_pos;
  POINT thumb1_pos;
  struct sprite *thumb0;
  struct sprite *thumb1;
  HMENU menu_shown;
};

#define SPECLIST_TAG dialog
#define SPECLIST_TYPE struct Diplomacy_dialog
#include "speclist.h"

#define dialog_list_iterate(dialoglist, pdialog) \
    TYPED_LIST_ITERATE(struct Diplomacy_dialog, dialoglist, pdialog)
#define dialog_list_iterate_end  LIST_ITERATE_END

static struct dialog_list *dialog_list;
static bool dialog_list_list_has_been_initialised = FALSE;

static struct Diplomacy_dialog *create_diplomacy_dialog(int other_player_id);
static struct Diplomacy_dialog *find_diplomacy_dialog(int other_player_id);
static void popup_diplomacy_dialog(int other_player_id);


/****************************************************************

*****************************************************************/
static void update_diplomacy_dialog(struct Diplomacy_dialog *pdialog)
{
  HDC hdc;
  char buf[64];
  ListBox_ResetContent(pdialog->list);
  
  clause_list_iterate(pdialog->treaty.clauses, pclause) {
    client_diplomacy_clause_string(buf, sizeof(buf), pclause);
    ListBox_AddString(pdialog->list,buf);
  } clause_list_iterate_end;
  
  pdialog->thumb0 = get_treaty_thumb_sprite(tileset, BOOL_VAL(pdialog->treaty.accept0));
  pdialog->thumb1 = get_treaty_thumb_sprite(tileset, BOOL_VAL(pdialog->treaty.accept1));
  hdc=GetDC(pdialog->mainwin);
  draw_sprite(pdialog->thumb0,hdc,pdialog->thumb0_pos.x,pdialog->thumb0_pos.y);
  draw_sprite(pdialog->thumb1,hdc,pdialog->thumb1_pos.x,pdialog->thumb1_pos.y);
  ReleaseDC(pdialog->mainwin,hdc);
}

/****************************************************************

*****************************************************************/
static void diplomacy_dialog_close(struct Diplomacy_dialog *pdialog)
{
  dsend_packet_diplomacy_cancel_meeting_req(&client.conn,
					    player_number(pdialog->treaty.plr1));
  DestroyWindow(pdialog->mainwin);
  
}

/****************************************************************

*****************************************************************/
static void popup_map_menu(struct Diplomacy_dialog *pdialog,int plr)
{
  RECT rc;
  MENUITEMINFO iteminfo;
  HMENU menu;
  menu=CreatePopupMenu();
  AppendMenu(menu,MF_STRING,ID_MAP_SEA,_("Sea-map"));
  iteminfo.dwItemData = plr;
  iteminfo.fMask = MIIM_DATA;
  iteminfo.cbSize = sizeof(MENUITEMINFO);
  SetMenuItemInfo(menu, ID_MAP_SEA, FALSE, &iteminfo);
  AppendMenu(menu,MF_STRING,ID_MAP_WORLD,_("World-map"));
  iteminfo.dwItemData = plr;
  iteminfo.fMask = MIIM_DATA;
  iteminfo.cbSize = sizeof(MENUITEMINFO);
  SetMenuItemInfo(menu, ID_MAP_WORLD, FALSE, &iteminfo);
  GetWindowRect(GetDlgItem(pdialog->mainwin,plr?ID_MAP1:ID_MAP0),&rc);
  TrackPopupMenu(menu,0,rc.left,rc.top,0,pdialog->mainwin,NULL);  
  pdialog->menu_shown=menu;
}

/****************************************************************

*****************************************************************/
static void popup_tech_menu(struct Diplomacy_dialog *pdialog,int plr)
{
  RECT rc;
  HMENU menu;
  MENUITEMINFO iteminfo;
  struct player *plr0 = plr?pdialog->treaty.plr1:pdialog->treaty.plr0;
  struct player *plr1 = plr?pdialog->treaty.plr0:pdialog->treaty.plr1;

  menu=CreatePopupMenu();

  advance_index_iterate(A_FIRST, i) {
    if (player_invention_state(plr0, i) == TECH_KNOWN
        && player_invention_reachable(plr1, i)
        && (player_invention_state(plr1, i) == TECH_UNKNOWN
            || player_invention_state(plr1, i) == TECH_PREREQS_KNOWN)) {
      AppendMenu(menu,MF_STRING,ID_ADVANCES_BASE+i,
		 advance_name_translation(advance_by_number(i)));
      iteminfo.dwItemData = player_number(plr0) * MAX_NUM_ITEMS * MAX_NUM_ITEMS +
			    player_number(plr1) * MAX_NUM_ITEMS + i;
      iteminfo.fMask = MIIM_DATA;
      iteminfo.cbSize = sizeof(MENUITEMINFO);
      SetMenuItemInfo(menu, ID_ADVANCES_BASE+i, FALSE, &iteminfo);
    }
  } advance_index_iterate_end;
  GetWindowRect(GetDlgItem(pdialog->mainwin,plr?ID_TECH1:ID_TECH0),
		&rc);
  TrackPopupMenu(menu,0,rc.left,rc.top,0,pdialog->mainwin,NULL);  
  pdialog->menu_shown=menu;
}

/****************************************************************

*****************************************************************/
static void popup_cities_menu(struct Diplomacy_dialog *pdialog,int plr)
{
  RECT rc;
  HMENU menu;
  MENUITEMINFO iteminfo;
  int i = 0, j = 0;
  int n;
  struct city **city_list_ptrs;
  struct player *plr0;
  struct player *plr1;
  menu=CreatePopupMenu();
  plr0=plr?pdialog->treaty.plr1:pdialog->treaty.plr0;
  plr1=plr?pdialog->treaty.plr0:pdialog->treaty.plr1;
  n=city_list_size(plr0->cities);
  if (n>0) {
    city_list_ptrs = fc_malloc(sizeof(struct city*)*n);
  } else {
    city_list_ptrs = NULL;
  }
  
  city_list_iterate(plr0->cities, pcity) {
    if (!is_capital(pcity)) {
      city_list_ptrs[i] = pcity;
      i++;
    }
  } city_list_iterate_end;

  qsort(city_list_ptrs, i, sizeof(struct city*), city_name_compare);
  
  for(j=0; j<i; j++) {
    AppendMenu(menu,MF_STRING,ID_CITIES_BASE+j,city_name(city_list_ptrs[j]));
    iteminfo.dwItemData=city_list_ptrs[j]->id*1024 + 
      player_number(plr0)*32 + player_number(plr1);
    iteminfo.fMask = MIIM_DATA;
    iteminfo.cbSize = sizeof(MENUITEMINFO);
    SetMenuItemInfo(menu, ID_CITIES_BASE+j, FALSE, &iteminfo);
  }
  free(city_list_ptrs);
  GetWindowRect(GetDlgItem(pdialog->mainwin,plr?ID_CITY1:ID_CITY0),
		&rc);
  TrackPopupMenu(menu,0,rc.left,rc.top,0,pdialog->mainwin,NULL); 
  pdialog->menu_shown=menu;
}

/****************************************************************

*****************************************************************/
static void handle_gold_entry(struct Diplomacy_dialog *pdialog,int plr)
{
  char buf[32];
  HWND edit;
  edit=GetDlgItem(pdialog->mainwin,plr?ID_GOLD1:ID_GOLD0);
  GetWindowText(edit,buf,sizeof(buf));
  if (strchr(buf,'\n')) {
    int amount;
    struct player *pgiver;

    SetWindowText(edit, "");
    pgiver = plr ? pdialog->treaty.plr1 : pdialog->treaty.plr0;
    if (sscanf(buf, "%d", &amount) == 1 && amount >= 0
	&& amount <= pgiver->economic.gold) {
       dsend_packet_diplomacy_create_clause_req(&client.conn,
					player_number(pdialog->treaty.plr1),
					player_number(pgiver),
					CLAUSE_GOLD, amount);
     } else {
       output_window_append(FTC_CLIENT_INFO, NULL,
                            _("Invalid amount of gold specified."));
     }
  }
}

/****************************************************************

*****************************************************************/
static void handle_vision_button(struct Diplomacy_dialog *pdialog,int plr)
{
  struct player *pgiver;
  pgiver=plr?pdialog->treaty.plr1:pdialog->treaty.plr0;

  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   player_number(pgiver), CLAUSE_VISION,
					   0);
}

/****************************************************************

*****************************************************************/
static void handle_embassy_button(struct Diplomacy_dialog *pdialog, int plr)
{
  struct player *pgiver;
  pgiver = plr ? pdialog->treaty.plr1 : pdialog->treaty.plr0;

  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   player_number(pgiver), CLAUSE_EMBASSY,
					   0);
}

/****************************************************************

*****************************************************************/
static void handle_pact_button(struct Diplomacy_dialog *pdialog)
{
  RECT rc;
  MENUITEMINFO iteminfo;
  HMENU menu;
  iteminfo.fMask=MIIM_DATA;
  iteminfo.cbSize=sizeof(MENUITEMINFO);
  menu=CreatePopupMenu();
  AppendMenu(menu,MF_STRING,ID_CEASEFIRE,Q_("?diplomatic_state:Cease-fire"));
  AppendMenu(menu,MF_STRING,ID_PEACE,Q_("?diplomatic_state:Peace"));
  AppendMenu(menu,MF_STRING,ID_ALLIANCE,Q_("?diplomatic_state:Alliance"));
  iteminfo.dwItemData=CLAUSE_CEASEFIRE;
  SetMenuItemInfo(menu,ID_CEASEFIRE,FALSE,&iteminfo);
  iteminfo.dwItemData=CLAUSE_PEACE;
  SetMenuItemInfo(menu,ID_PEACE,FALSE,&iteminfo);  
  iteminfo.dwItemData=CLAUSE_ALLIANCE;
  SetMenuItemInfo(menu,ID_ALLIANCE,FALSE,&iteminfo);
  GetWindowRect(GetDlgItem(pdialog->mainwin,ID_PACT),
		&rc);
  TrackPopupMenu(menu,0,rc.left,rc.top,0,pdialog->mainwin,NULL); 
  pdialog->menu_shown=menu;
}

/****************************************************************

*****************************************************************/
static void give_sea_map(struct Diplomacy_dialog *pdialog, int plr)
{
  struct player *pgiver;
  pgiver=plr?pdialog->treaty.plr1:pdialog->treaty.plr0;
  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   player_number(pgiver), CLAUSE_SEAMAP,
					   0);
}

/****************************************************************

*****************************************************************/
static void give_map(struct Diplomacy_dialog *pdialog, int plr)
{
  struct player *pgiver;
  pgiver=plr?pdialog->treaty.plr1:pdialog->treaty.plr0;
  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   player_number(pgiver), CLAUSE_MAP,
					   0);
}

/****************************************************************

*****************************************************************/
static void handle_cities_menu(struct Diplomacy_dialog *pdialog,int choice)
{
  int value, plrno0, plrno1;
  value = choice/1024;
  choice -= value * 1024;
  plrno0 = choice/32;
  choice -= plrno0 * 32;
  plrno1 = choice;
  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   plrno0, CLAUSE_CITY, value);
}

/****************************************************************

*****************************************************************/
static void handle_advances_menu(struct Diplomacy_dialog *pdialog,int choice)
{
  int plrno0 = choice / (MAX_NUM_ITEMS * MAX_NUM_ITEMS);
#if 0 /* Unneeded. */
  int plrno1 = (choice / MAX_NUM_ITEMS) % MAX_NUM_ITEMS;
#endif
  int value = choice % MAX_NUM_ITEMS;

  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   plrno0, CLAUSE_ADVANCE, value);
}

/****************************************************************

*****************************************************************/
static void diplomacy_dialog_add_pact_clause(struct Diplomacy_dialog *pdialog,
					     int type)
{
  dsend_packet_diplomacy_create_clause_req(&client.conn,
					   player_number(pdialog->treaty.plr1),
					   player_number(pdialog->treaty.plr0),
					   type, 0);
}

/****************************************************************

*****************************************************************/
static void handle_erase_button(struct Diplomacy_dialog *pdialog)
{
  int i=0;
  int row;
  row=ListBox_GetCurSel(pdialog->list);
  if (row==LB_ERR)
    return;
  clause_list_iterate(pdialog->treaty.clauses, pclause) {
    if(i == row) {
      dsend_packet_diplomacy_remove_clause_req(&client.conn,
					       player_number(pdialog->treaty.plr1),
					       player_number(pclause->from),
					       pclause->type, pclause->value);
      return;
    }
    i++;
  } clause_list_iterate_end;
}

/****************************************************************

*****************************************************************/
static void handle_accept_button(struct Diplomacy_dialog *pdialog)
{
  dsend_packet_diplomacy_accept_treaty_req(&client.conn,
					   player_number(pdialog->treaty.plr1));
}

/****************************************************************

*****************************************************************/
static void close_diplomacy_dialog(struct Diplomacy_dialog *pdialog)
{
  DestroyWindow(pdialog->mainwin);
}

/****************************************************************

*****************************************************************/
static LONG CALLBACK diplomacy_proc(HWND dlg,UINT message,WPARAM wParam,LPARAM lParam)
{
  int is_menu;
  int menu_data;

  struct Diplomacy_dialog *pdialog;
  pdialog=(struct Diplomacy_dialog *)fcwin_get_user_data(dlg);
  switch(message) {
  case WM_CREATE:
  case WM_SIZE:
  case WM_GETMINMAXINFO:
    break;
  case WM_CLOSE:
    diplomacy_dialog_close(pdialog);
    break;
  case WM_DESTROY:
    if (pdialog->menu_shown)
      DestroyMenu(pdialog->menu_shown);
    dialog_list_unlink(dialog_list, pdialog);
    free(pdialog);
    break;
  case WM_COMMAND:
    is_menu=0;
    menu_data=0;
    if (pdialog->menu_shown) {
      MENUITEMINFO iteminfo;
      iteminfo.cbSize=sizeof(MENUITEMINFO);
      iteminfo.fMask=MIIM_DATA;
      is_menu=GetMenuItemInfo(pdialog->menu_shown,LOWORD(wParam),FALSE,&iteminfo);
      DestroyMenu(pdialog->menu_shown);
      pdialog->menu_shown=NULL;
      menu_data=iteminfo.dwItemData;
    }
    switch((enum Diplomacy_ids) LOWORD(wParam)) {
    case ID_MAP0:
      popup_map_menu(pdialog,0);
      break;
    case ID_MAP1:
      popup_map_menu(pdialog,1);
      break;
    case ID_MAP_SEA:
      give_sea_map(pdialog,menu_data);
      break;
    case ID_MAP_WORLD:
      give_map(pdialog,menu_data);
      break;
    case ID_TECH0:
      popup_tech_menu(pdialog,0);
      break;
    case ID_TECH1:
      popup_tech_menu(pdialog,1);
      break;
    case ID_CITY0:
      popup_cities_menu(pdialog,0);
      break;
    case ID_CITY1:
      popup_cities_menu(pdialog,1);
      break;
    case ID_GOLD0:
      handle_gold_entry(pdialog,0);
      break;
    case ID_GOLD1:
      handle_gold_entry(pdialog,1);
      break;
    case ID_PEACE:
    case ID_CEASEFIRE:
    case ID_ALLIANCE:
      diplomacy_dialog_add_pact_clause(pdialog,menu_data);
      break;
    case ID_VISION0:
      handle_vision_button(pdialog,0);
      break;
    case ID_VISION1:
      handle_vision_button(pdialog,1);
      break;
    case ID_EMBASSY0:
      handle_embassy_button(pdialog,0);
      break;
    case ID_EMBASSY1:
      handle_embassy_button(pdialog,1);
      break;
    case ID_PACT:
      handle_pact_button(pdialog);
      break;
    case ID_ERASE:
      handle_erase_button(pdialog);
      break;
    case IDCANCEL:
      diplomacy_dialog_close(pdialog);
      break;
    case IDOK:
      handle_accept_button(pdialog);
      break;
    case ID_LIST:
      if (HIWORD(wParam) == LBN_DBLCLK) {
	handle_erase_button(pdialog);
      }
      break;
    default:
      if (LOWORD(wParam)>=ID_CITIES_BASE) {
	handle_cities_menu(pdialog,menu_data);
      } else if (LOWORD(wParam)>=ID_ADVANCES_BASE) {
	handle_advances_menu(pdialog,menu_data);
      }
      
    }
    break;
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc;
      hdc=BeginPaint(dlg,(LPPAINTSTRUCT)&ps);
      draw_sprite(pdialog->thumb0,hdc,
		  pdialog->thumb0_pos.x,pdialog->thumb0_pos.y);
      draw_sprite(pdialog->thumb1,hdc,
		  pdialog->thumb1_pos.x,pdialog->thumb1_pos.y);
      EndPaint(dlg,(LPPAINTSTRUCT)&ps);
    }
    break;
  default:
    return DefWindowProc(dlg,message,wParam,lParam);
  }
  return 0;
}

/****************************************************************

*****************************************************************/
static void thumb_minsize(POINT *minsize, void *data)
{
  minsize->x = get_treaty_thumb_sprite(tileset, FALSE)->width;
  minsize->y = get_treaty_thumb_sprite(tileset, FALSE)->height;
}

/****************************************************************

*****************************************************************/
static void thumb_setsize(RECT *rc, void *data)
{
  POINT *thumb_pos;
  thumb_pos=(POINT *)data;
  thumb_pos->x=rc->left;
  thumb_pos->y=rc->top;
}

/****************************************************************
...
*****************************************************************/
static struct Diplomacy_dialog *create_diplomacy_dialog(int other_player_id)
{
  struct player *plr0 = client.conn.playing;
  struct player *plr1 = player_by_number(other_player_id);

  char buf[512];
  struct fcwin_box *vbox;
  struct fcwin_box *hbox;
  struct fcwin_box *hbox2;
  struct Diplomacy_dialog *pdialog;

  pdialog=fc_malloc(sizeof(struct Diplomacy_dialog));  
  dialog_list_prepend(dialog_list, pdialog);
  pdialog->menu_shown=NULL;
  init_treaty(&pdialog->treaty, plr0, plr1);

  pdialog->mainwin=fcwin_create_layouted_window(diplomacy_proc,
						_("Diplomacy meeting"),
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,CW_USEDEFAULT,
						root_window,NULL,
						FAKE_CHILD,
						pdialog);
  vbox=fcwin_vbox_new(pdialog->mainwin,FALSE);
  my_snprintf(buf, sizeof(buf),
              _("The %s offerings"),
              nation_adjective_for_player(plr0));
  fcwin_box_add_static(vbox,buf,0,SS_LEFT,FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Maps"),ID_MAP0,0,FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Advances"),ID_TECH0,0,FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Cities"),ID_CITY0,0,FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Embassy"), ID_EMBASSY0, 0, FALSE, FALSE, 5);
  
  my_snprintf(buf, sizeof(buf), _("Gold(max %d)"), plr0->economic.gold); 
  pdialog->gold0_label=fcwin_box_add_static(vbox,buf,0,SS_LEFT,FALSE,FALSE,5);
  fcwin_box_add_edit(vbox,"",6,ID_GOLD0,ES_WANTRETURN | ES_MULTILINE | ES_AUTOVSCROLL,
		     FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Give shared vision"),ID_VISION0,0,
		       FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Pacts"),ID_PACT,0,FALSE,FALSE,5);
  hbox=fcwin_hbox_new(pdialog->mainwin,FALSE);
  fcwin_box_add_box(hbox,vbox,FALSE,FALSE,5);
  vbox=fcwin_vbox_new(pdialog->mainwin,FALSE);
    
  my_snprintf(buf, sizeof(buf),
	      _("This Eternal Treaty\n"
		"marks the results of the diplomatic work between\n"
		"The %s %s %s\nand\nThe %s %s %s"),
	      nation_adjective_for_player(plr0),
	      ruler_title_translation(plr0),
	      player_name(plr0),
	      nation_adjective_for_player(plr1),
	      ruler_title_translation(plr1),
	      player_name(plr1));
  fcwin_box_add_static(vbox,buf,0,SS_CENTER,FALSE,FALSE,5);
  pdialog->list=fcwin_box_add_list(vbox,6,ID_LIST,WS_VSCROLL,TRUE,TRUE,5);
  hbox2=fcwin_hbox_new(pdialog->mainwin,FALSE);

  my_snprintf(buf, sizeof(buf), _("%s view:"),
              nation_adjective_for_player(plr0));
  fcwin_box_add_static(hbox2,buf,0,SS_LEFT,FALSE,FALSE,5);
  fcwin_box_add_generic(hbox2,thumb_minsize,thumb_setsize,NULL,
			&pdialog->thumb0_pos,FALSE,FALSE,5);
  
  my_snprintf(buf, sizeof(buf), _("%s view:"),
              nation_adjective_for_player(plr1));
  fcwin_box_add_static(hbox2,buf,0,SS_LEFT,FALSE,FALSE,5);
  fcwin_box_add_generic(hbox2,thumb_minsize,thumb_setsize,NULL,
			&pdialog->thumb1_pos,FALSE,FALSE,5);
  pdialog->thumb0 = get_treaty_thumb_sprite(tileset, FALSE);
  pdialog->thumb1 = get_treaty_thumb_sprite(tileset, FALSE);
  fcwin_box_add_box(vbox,hbox2,FALSE,FALSE,5);

  fcwin_box_add_box(hbox,vbox,TRUE,TRUE,5);
  
  vbox=fcwin_vbox_new(pdialog->mainwin,FALSE);
  my_snprintf(buf, sizeof(buf),
              _("The %s offerings"),
              nation_adjective_for_player(plr1));
  fcwin_box_add_static(vbox,buf,0,SS_LEFT,FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Maps"),ID_MAP1,0,FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Advances"),ID_TECH1,0,FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Cities"),ID_CITY1,0,FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Embassy"), ID_EMBASSY1, 0, FALSE, FALSE, 5);
  
  my_snprintf(buf, sizeof(buf), _("Gold(max %d)"), plr1->economic.gold); 
  pdialog->gold1_label=fcwin_box_add_static(vbox,buf,0,SS_LEFT,FALSE,FALSE,5);
  fcwin_box_add_edit(vbox, "", 6, ID_GOLD1, ES_WANTRETURN | ES_MULTILINE
		     | ES_AUTOVSCROLL, FALSE, FALSE, 5);
  fcwin_box_add_button(vbox,_("Give shared vision"),ID_VISION1,0,
		       FALSE,FALSE,5);
  fcwin_box_add_button(vbox,_("Erase Clause"), ID_ERASE, 0, FALSE, FALSE, 5);

  fcwin_box_add_box(hbox,vbox,FALSE,FALSE,5);
  vbox=fcwin_vbox_new(pdialog->mainwin,FALSE);
  fcwin_box_add_box(vbox,hbox,TRUE,TRUE,5);
  hbox=fcwin_hbox_new(pdialog->mainwin,TRUE);
  
  fcwin_box_add_button(hbox,_("Accept treaty"),IDOK,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Cancel meeting"),IDCANCEL,0,TRUE,TRUE,5);

  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  fcwin_set_box(pdialog->mainwin,vbox);

  update_diplomacy_dialog(pdialog);

  return pdialog;
}

/****************************************************************
...
*****************************************************************/
static struct Diplomacy_dialog *find_diplomacy_dialog(int other_player_id)
{
  struct player *plr0 = client.conn.playing;
  struct player *plr1 = player_by_number(other_player_id);

  if(!dialog_list_list_has_been_initialised) {
    dialog_list = dialog_list_new();
    dialog_list_list_has_been_initialised = TRUE;
  }

  dialog_list_iterate(dialog_list, pdialog) {
    if ((pdialog->treaty.plr0 == plr0 && pdialog->treaty.plr1 == plr1) ||
	(pdialog->treaty.plr0 == plr1 && pdialog->treaty.plr1 == plr0)) {
      return pdialog;
    }
  } dialog_list_iterate_end;

  return NULL;
}


/****************************************************************
...
*****************************************************************/
static void popup_diplomacy_dialog(int other_player_id)
{
  struct Diplomacy_dialog *pdialog;

  if (client.conn.playing->ai_data.control) {
    return;			/* Don't show if we are AI controlled. */
  }

  if (!(pdialog = find_diplomacy_dialog(other_player_id))) {
    pdialog = create_diplomacy_dialog(other_player_id);
  }
  
  ShowWindow(pdialog->mainwin,SW_SHOWNORMAL);
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_accept_treaty(int counterpart, bool I_accepted,
				    bool other_accepted)
{
  struct Diplomacy_dialog *pdialog;
  
  if ((pdialog = find_diplomacy_dialog(counterpart))) {

    pdialog->treaty.accept0 = I_accepted;
    pdialog->treaty.accept1 = other_accepted;

    update_diplomacy_dialog(pdialog);
  }
  
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_init_meeting(int counterpart, int initiated_from)
{
  popup_diplomacy_dialog(counterpart);
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_create_clause(int counterpart, int giver,
				    enum clause_type type, int value)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  add_clause(&pdialog->treaty, player_by_number(giver), type, value);
  update_diplomacy_dialog(pdialog);
}


/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_cancel_meeting(int counterpart, int initiated_from)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  close_diplomacy_dialog(pdialog);
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_remove_clause(int counterpart, int giver,
				    enum clause_type type, int value)
{
  struct Diplomacy_dialog *pdialog = find_diplomacy_dialog(counterpart);

  if (!pdialog) {
    return;
  }

  remove_clause(&pdialog->treaty, player_by_number(giver), type, value);
  update_diplomacy_dialog(pdialog);
}

/**************************************************************************
...
**************************************************************************/
void close_all_diplomacy_dialogs(void)
{
  if (!dialog_list_list_has_been_initialised) {
    return;
  }

  while (dialog_list_size(dialog_list) > 0) {
    close_diplomacy_dialog(dialog_list_get(dialog_list, 0));
  }
}
