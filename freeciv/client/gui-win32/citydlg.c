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
#include <windowsx.h>
#include <commctrl.h>
#include <assert.h>

/* utility */
#include "fcintl.h"
#include "genlist.h"
#include "log.h"
#include "mem.h"
#include "packets.h"
#include "shared.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "map.h"
#include "packets.h"
#include "player.h"
#include "unitlist.h"

/* client */
#include "canvas.h"
#include "cityrep.h"
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "cma_fe.h"
#include "colors.h"
#include "control.h"
#include "dialogs.h"
#include "gui_stuff.h"
#include "happiness.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mapview.h"
#include "options.h"
#include "repodlgs.h"
#include "sprite.h"
#include "tilespec.h"
#include "wldlg.h"   

#include "gui_main.h"
#include "citydlg.h"
#include "text.h"

#define NUM_UNITS_SHOWN  12
#define NUM_CITIZENS_SHOWN 25   
#define NUM_INFO_FIELDS 12      /* number of fields in city_info */
#define ID_CITYOPT_RADIO 100
#define ID_CITYOPT_TOGGLE 200
#define NUM_TABS 6
struct city_dialog {
  struct city *pcity;
  HBITMAP map_bmp;
  HBITMAP citizen_bmp;
  HWND mainwindow;
  HWND tab_ctrl;
  HWND tab_childs[NUM_TABS];
  HWND buildings_list;
  HWND buy_but;
  HWND change_but;
  HWND sell_but;
  HWND close_but;
  HWND rename_but;
  HWND activate_but;
  HWND unitlist_but;
  HWND info_label[2][NUM_INFO_FIELDS];
  HWND build_area;
  HWND supported_label;
  HWND present_label;
  HWND trade_label;
  HWND cityopt_dialog;
  struct fcwin_box *opt_winbox;
  int resized;
  struct fcwin_box *listbox_area;
  struct fcwin_box *full_win;
  struct fcwin_box *buttonrow;
  int upper_y;
  POINT map;
  POINT maph; /* Happiness Dialog */
  int map_w;
  int map_h;
  int pop_y;
  int pop_x; /* also supported_x and present_x */
  int supported_y;
  int present_y;
  Impr_type_id sell_id;
  
  int support_unit_ids[NUM_UNITS_SHOWN];
  int present_unit_ids[NUM_UNITS_SHOWN];
  struct universal change_list_targets[MAX_NUM_PRODUCTION_TARGETS];
  int change_list_num_improvements;
  int building_values[B_LAST];
  int is_modal;    
  int id_selected;
  int last_improvlist_seen[B_LAST];
  struct happiness_dlg *happiness;
};


extern HFONT font_8courier;
extern HFONT font_12courier;
extern HFONT font_12arial;
extern HINSTANCE freecivhinst;
extern HWND root_window;

static int city_map_width;
static int city_map_height;
static struct genlist *dialog_list;
static int city_dialogs_have_been_initialised;
struct city_dialog *get_city_dialog(struct city *pcity);
struct city_dialog *create_city_dialog(struct city *pcity);

static void city_dialog_update_improvement_list(struct city_dialog *pdialog);
static void city_dialog_update_supported_units(HDC hdc,struct city_dialog *pdialog);
static void city_dialog_update_present_units(HDC hdc,struct city_dialog *pdialog);
static void city_dialog_update_citizens(HDC hdc,struct city_dialog *pdialog);
static void city_dialog_update_map(HDC hdc,struct city_dialog *pdialog);
static void city_dialog_update_information(HWND *info_label,
                                           struct city_dialog *pdialog);
static void city_dialog_update_building(struct city_dialog *pdialog);
static void city_dialog_update_tradelist(struct city_dialog *pdialog);
static void commit_city_worklist(struct worklist *pwl, void *data);
static void resize_city_dialog(struct city_dialog *pdialog);
static LONG CALLBACK trade_page_proc(HWND win, UINT message,
				     WPARAM wParam, LPARAM lParam);
static LONG CALLBACK citydlg_overview_proc(HWND win, UINT message,
					   WPARAM wParam, LPARAM lParam);
static LONG CALLBACK happiness_proc(HWND win, UINT message,
				    WPARAM wParam, LPARAM lParam);
static LONG APIENTRY citydlg_config_proc(HWND hWnd,
					 UINT message,
					 UINT wParam,
					 LONG lParam);

static WNDPROC tab_procs[NUM_TABS]={citydlg_overview_proc,
				    worklist_editor_proc,
				    happiness_proc,
				    cma_proc,
				    trade_page_proc,citydlg_config_proc};
enum t_tab_pages {
  PAGE_OVERVIEW=0,
  PAGE_WORKLIST,
  PAGE_HAPPINESS,
  PAGE_CMA, 
  PAGE_TRADE,
  PAGE_CONFIG
};




/**************************************************************************
...
**************************************************************************/

void refresh_city_dialog(struct city *pcity)
{
  HDC hdc;
  HDC hdcsrc;
  HBITMAP old;
  struct city_dialog *pdialog;
  if((pdialog=get_city_dialog(pcity))) {
    hdc=GetDC(pdialog->mainwindow);
    city_dialog_update_citizens(hdc,pdialog);
    ReleaseDC(pdialog->mainwindow,hdc);
    hdc=GetDC(pdialog->tab_childs[0]);
    city_dialog_update_supported_units(hdc, pdialog);
    city_dialog_update_present_units(hdc, pdialog);   
    city_dialog_update_map(hdc,pdialog);
    ReleaseDC(pdialog->tab_childs[0],hdc);

    hdc=GetDC(pdialog->tab_childs[PAGE_HAPPINESS]);
    hdcsrc = CreateCompatibleDC(NULL);
    old=SelectObject(hdcsrc,pdialog->map_bmp);
    BitBlt(hdc,pdialog->maph.x,pdialog->maph.y,city_map_width,
	   city_map_height,hdcsrc,0,0,SRCCOPY);
    SelectObject(hdcsrc, old);
    DeleteDC(hdcsrc);
    ReleaseDC(pdialog->tab_childs[PAGE_HAPPINESS],hdc);
    
    city_dialog_update_improvement_list(pdialog);  
    city_dialog_update_information(pdialog->info_label[0], pdialog);
    city_dialog_update_information(pdialog->info_label[1], pdialog);
    city_dialog_update_building(pdialog);
    city_dialog_update_tradelist(pdialog);
    refresh_happiness_box(pdialog->happiness);
    resize_city_dialog(pdialog);
  }
  if (city_owner(pcity) == client.conn.playing) {
    city_report_dialog_update_city(pcity);
    economy_report_dialog_update();   
    if (pdialog != NULL) {
      update_worklist_editor_win(pdialog->tab_childs[PAGE_WORKLIST]);
    }
    refresh_cma_dialog(pcity, REFRESH_ALL);
  }
  else if (pdialog) {
      EnableWindow(pdialog->buy_but,FALSE);
      EnableWindow(pdialog->change_but,FALSE);
      EnableWindow(pdialog->sell_but,FALSE);
      EnableWindow(pdialog->rename_but,FALSE);
      EnableWindow(pdialog->activate_but,FALSE);
      EnableWindow(pdialog->unitlist_but,FALSE);
  }
}


bool city_dialog_is_open(struct city *pcity)
{
  return get_city_dialog(pcity) != NULL;
}

/**************************************************************************
...
**************************************************************************/

void city_dialog_update_improvement_list(struct city_dialog *pdialog)
{
  LV_COLUMN lvc;
  int total, item, targets_used;
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  char buf[100];
  bool changed;

  /* Test if the list improvements of pcity has changed */
  changed = false;
  improvement_iterate(pimprove) {
    int index = improvement_index(pimprove);
    bool has_building = city_has_building(pdialog->pcity, pimprove);
    bool had_building =
      pdialog->last_improvlist_seen[index];

    if ((has_building && !had_building)
        || (!has_building && had_building)) {
      changed = true;
      pdialog->last_improvlist_seen[index] = has_building;
    }
  } improvement_iterate_end;

  if (!changed) {
    return;
  }
  
  targets_used = collect_already_built_targets(targets, pdialog->pcity);
  name_and_sort_items(targets, targets_used, items, FALSE, pdialog->pcity);
   
  ListView_DeleteAllItems(pdialog->buildings_list);
  
  total = 0;
  for (item = 0; item < targets_used; item++) {
    char *strings[2];
    int upkeep;
    int row;
    struct universal target = items[item].item;

    assert(VUT_IMPROVEMENT == target.kind);

    strings[0] = items[item].descr;
    strings[1] = buf;

    /* This takes effects (like Adam Smith's) into account. */
    upkeep = city_improvement_upkeep(pdialog->pcity, target.value.building);

    my_snprintf(buf, sizeof(buf), "%d", upkeep);
   
    row=fcwin_listview_add_row(pdialog->buildings_list,
			   item, 2, strings);
    pdialog->building_values[row] = improvement_index(target.value.building);
    total += upkeep;
  }
  lvc.mask=LVCF_TEXT;
  lvc.pszText=buf;
  my_snprintf(buf, sizeof(buf), _("Upkeep (Total: %d)"), total);
  ListView_SetColumn(pdialog->buildings_list,1,&lvc);
  ListView_SetColumnWidth(pdialog->buildings_list,0,LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(pdialog->buildings_list,1,LVSCW_AUTOSIZE_USEHEADER);
  pdialog->id_selected=-1;
}

/**************************************************************************
...
**************************************************************************/

void city_dialog_update_present_units(HDC hdc,struct city_dialog *pdialog) 
{
  int i;
  struct unit_list *plist;
  struct canvas store;
  
  store.type = CANVAS_DC;
  store.hdc = hdc;
  store.bmp = NULL;
  store.wnd = NULL;
  store.tmp = NULL;
 
  if (city_owner(pdialog->pcity) != client.conn.playing) {
    plist = pdialog->pcity->info_units_present;
  } else {
    plist = pdialog->pcity->tile->units;
  }
  
  for(i = 0; i < NUM_UNITS_SHOWN; i++) {
    RECT rc;
    pdialog->present_unit_ids[i]=0;      
    rc.top=pdialog->present_y;
    rc.left=pdialog->pop_x+i*(tileset_small_sprite_width(tileset)+tileset_tile_width(tileset));
    rc.right=rc.left+tileset_tile_width(tileset);
    rc.bottom=rc.top+tileset_small_sprite_height(tileset)+tileset_tile_height(tileset);
    FillRect(hdc, &rc, 
	     (HBRUSH)GetClassLong(pdialog->mainwindow,GCL_HBRBACKGROUND));
  }

  i = 0;
  
  unit_list_iterate(plist, punit) {
    put_unit(punit, &store,
	     pdialog->pop_x + i * (tileset_small_sprite_width(tileset) + tileset_tile_width(tileset)),
	     pdialog->present_y);
    pdialog->present_unit_ids[i] = punit->id;
    i++;
    if (i == NUM_UNITS_SHOWN) {
      break;
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/

void city_dialog_update_supported_units(HDC hdc, struct city_dialog *pdialog)
{
  int i;
  struct unit_list *plist;
  struct canvas store;
  int free_unhappy;

  store.type = CANVAS_DC;
  store.hdc = hdc;
  store.bmp = NULL;
  store.wnd = NULL;
  store.tmp = NULL;

  free_unhappy = get_city_bonus(pdialog->pcity, EFT_MAKE_CONTENT_MIL);

  if (city_owner(pdialog->pcity) != client.conn.playing) {
    plist = pdialog->pcity->info_units_supported;
  } else {
    plist = pdialog->pcity->units_supported;
  }

  for(i = 0; i < NUM_UNITS_SHOWN; i++) {
    RECT rc;
    pdialog->support_unit_ids[i]=0;
    rc.top=pdialog->supported_y;
    rc.left=pdialog->pop_x+i*(tileset_small_sprite_width(tileset) +
            tileset_tile_width(tileset));
    rc.right=rc.left+tileset_tile_width(tileset);
    rc.bottom=rc.top+tileset_small_sprite_height(tileset) +
              tileset_tile_height(tileset);
    FillRect(hdc, &rc,
             (HBRUSH)GetClassLong(pdialog->mainwindow,GCL_HBRBACKGROUND));
  }

  i = 0;

  unit_list_iterate(plist, punit) {
    int happy_cost = city_unit_unhappiness(punit, &free_unhappy);

    put_unit(punit, &store,
	     pdialog->pop_x + i * (tileset_small_sprite_width(tileset) +
                      tileset_tile_width(tileset)),
	     pdialog->supported_y);
    put_unit_city_overlays(punit, &store,
             pdialog->pop_x + i * (tileset_small_sprite_width(tileset) +
                                   tileset_tile_width(tileset)),
             pdialog->supported_y, punit->upkeep, happy_cost);
    pdialog->support_unit_ids[i] = punit->id;
    i++;
    if (i == NUM_UNITS_SHOWN) {
      break;
    }
  } unit_list_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void city_dialog_update_building(struct city_dialog *pdialog)
{
  char buf2[100], buf[32];
  struct city *pcity=pdialog->pcity;    
  const char *descr = city_production_name_translation(pcity);
  
  EnableWindow(pdialog->buy_but, city_can_buy(pcity));
  EnableWindow(pdialog->sell_but, !pcity->did_sell);

  get_city_dialog_production(pcity, buf, sizeof(buf));
  
  my_snprintf(buf2, sizeof(buf2), "%s\r\n%s", descr, buf);
  SetWindowText(pdialog->build_area, buf2);
  SetWindowText(pdialog->build_area, buf2);
  resize_city_dialog(pdialog);
 
}

/****************************************************************
   ...
+*****************************************************************/
static void city_dialog_update_information(HWND *info_label,
                                           struct city_dialog *pdialog)
{
  int i;
  char buf[NUM_INFO_FIELDS][512];
  struct city *pcity = pdialog->pcity;
  int granaryturns;
  enum { FOOD, SHIELD, TRADE, GOLD, LUXURY, SCIENCE, 
	 GRANARY, GROWTH, CORRUPTION, WASTE, POLLUTION,
         ILLNESS
  };

  /* fill the buffers with the necessary info */

  my_snprintf(buf[FOOD], sizeof(buf[FOOD]), "%2d (%+2d)",
	      pcity->prod[O_FOOD], pcity->surplus[O_FOOD]);
  my_snprintf(buf[SHIELD], sizeof(buf[SHIELD]), "%2d (%+2d)",
	      pcity->prod[O_SHIELD] + pcity->waste[O_SHIELD],
	      pcity->surplus[O_SHIELD]);
  my_snprintf(buf[TRADE], sizeof(buf[TRADE]), "%2d (%+2d)",
	      pcity->surplus[O_TRADE] + pcity->waste[O_TRADE],
	      pcity->surplus[O_TRADE]);
  my_snprintf(buf[GOLD], sizeof(buf[GOLD]), "%2d (%+2d)",
	      pcity->prod[O_GOLD], pcity->surplus[O_GOLD]);
  my_snprintf(buf[LUXURY], sizeof(buf[LUXURY]), "%2d      ",
	      pcity->prod[O_LUXURY]);

  my_snprintf(buf[SCIENCE], sizeof(buf[SCIENCE]), "%2d",
	      pcity->prod[O_SCIENCE]);

  my_snprintf(buf[GRANARY], sizeof(buf[GRANARY]), "%d/%-d",
	      pcity->food_stock, city_granary_size(pcity->size));
	
  granaryturns = city_turns_to_grow(pcity);
  if (granaryturns == 0) {
    my_snprintf(buf[GROWTH], sizeof(buf[GROWTH]), _("blocked"));
  } else if (granaryturns == FC_INFINITY) {
    my_snprintf(buf[GROWTH], sizeof(buf[GROWTH]), _("never"));
  } else {
    /* A negative value means we'll have famine in that many turns.
       But that's handled down below. */
    my_snprintf(buf[GROWTH], sizeof(buf[GROWTH]),
		PL_("%d turn", "%d turns", abs(granaryturns)),
		abs(granaryturns));
  }
  my_snprintf(buf[CORRUPTION], sizeof(buf[CORRUPTION]), "%4d",
              pcity->waste[O_TRADE]);
  my_snprintf(buf[WASTE], sizeof(buf[WASTE]), "%4d",
              pcity->waste[O_SHIELD]);
  my_snprintf(buf[POLLUTION], sizeof(buf[POLLUTION]), "%4d",
              pcity->pollution);
  if (!game.info.illness_on) {
    my_snprintf(buf[ILLNESS], sizeof(buf[ILLNESS]), " -.-");
  } else {
    my_snprintf(buf[ILLNESS], sizeof(buf[ILLNESS]), "%4.1f",
                ((float)pcity->illness / 10.0));
  }

  /* stick 'em in the labels */

  for (i = 0; i < NUM_INFO_FIELDS; i++) {
    SetWindowText(info_label[i], buf[i]);
  }
}

/**************************************************************************
  ...
**************************************************************************/
void city_dialog_update_map(HDC hdc, struct city_dialog *pdialog)
{
  struct canvas window;
  struct canvas store;

  window.type = CANVAS_DC;
  window.hdc = hdc;
  window.bmp = NULL;
  window.wnd = NULL;
  window.tmp = NULL;

  store.type = CANVAS_BITMAP;
  store.hdc = NULL;
  store.bmp = pdialog->map_bmp;
  store.wnd = NULL;
  store.tmp = NULL;

  city_dialog_redraw_map(pdialog->pcity, &store);
  canvas_copy(&window, &store, 0, 0, pdialog->map.x, pdialog->map.y,
	      city_map_width, city_map_height);
}

/**************************************************************************
...
**************************************************************************/


void city_dialog_update_citizens(HDC hdc,struct city_dialog *pdialog)
{
  enum citizen_category citizens[MAX_CITY_SIZE];
  int i;
  RECT rc;
  struct city *pcity=pdialog->pcity;
  int num_citizens = get_city_citizen_types(pcity, FEELING_FINAL, citizens);
  HDC hdcsrc = CreateCompatibleDC(NULL);
  HBITMAP oldbit = SelectObject(hdcsrc,pdialog->citizen_bmp);

  for (i = 0; i < num_citizens && i < NUM_CITIZENS_SHOWN; i++) {
      draw_sprite(get_citizen_sprite(tileset, citizens[i], i, pcity), hdcsrc,
		  tileset_small_sprite_width(tileset) * i, 0);
  }

  if (i<NUM_CITIZENS_SHOWN) {
    rc.left=i*tileset_small_sprite_width(tileset);
    rc.right=NUM_CITIZENS_SHOWN*tileset_small_sprite_width(tileset);
    rc.top=0;
    rc.bottom=tileset_small_sprite_height(tileset);
    FillRect(hdcsrc,&rc,
	     (HBRUSH)GetClassLong(pdialog->mainwindow,GCL_HBRBACKGROUND));
    FrameRect(hdcsrc,&rc,
	      (HBRUSH)GetClassLong(pdialog->mainwindow,GCL_HBRBACKGROUND));
  }
    
  BitBlt(hdc,pdialog->pop_x,pdialog->pop_y,
	 NUM_CITIZENS_SHOWN*tileset_small_sprite_width(tileset),
	 tileset_small_sprite_height(tileset),
	 hdcsrc,0,0,SRCCOPY);
  SelectObject(hdcsrc,oldbit);
  DeleteDC(hdcsrc);
}

/**************************************************************************
...
**************************************************************************/


static void CityDlgClose(struct city_dialog *pdialog)
{
  ShowWindow(pdialog->mainwindow,SW_HIDE);
  
  DestroyWindow(pdialog->mainwindow);
 
}
			       
/**************************************************************************
...
**************************************************************************/


static void map_minsize(POINT *minsize,void * data)
{
  minsize->x=city_map_width;
  minsize->y=city_map_height;
}

/**************************************************************************
...
**************************************************************************/


static void map_setsize(LPRECT setsize,void *data)
{
  POINT *pt;
  pt=(POINT *)data;
  pt->x=setsize->left;
  pt->y=setsize->top;
 
}

/**************************************************************************
...
**************************************************************************/


static void supported_minsize(LPPOINT minsize,void *data)
{
  minsize->y=tileset_small_sprite_height(tileset)+tileset_tile_height(tileset);
  minsize->x=1;  /* just a dummy value */
}

/**************************************************************************
...
**************************************************************************/


static void supported_setsize(LPRECT setsize,void *data)
{
  struct city_dialog *pdialog;
  pdialog=(struct city_dialog *)data;
  pdialog->supported_y=setsize->top;
}

/**************************************************************************
...
**************************************************************************/


static void present_minsize(POINT * minsize,void *data)
{
  minsize->y=tileset_small_sprite_height(tileset)+tileset_tile_height(tileset);
  minsize->x=1;  /* just a dummy value */
}

/**************************************************************************
...
**************************************************************************/


static void present_setsize(LPRECT setsize,void *data)
{
  struct city_dialog *pdialog;
  pdialog=(struct city_dialog *)data;
  pdialog->present_y=setsize->top;
}

/**************************************************************************
...
**************************************************************************/


static void resize_city_dialog(struct city_dialog *pdialog)
{
  if (!pdialog->full_win) return;
  fcwin_redo_layout(pdialog->mainwindow);
}

/**************************************************************************
...
**************************************************************************/
static void upper_set(RECT *rc, void *data)
{
  
}
/**************************************************************************
...
**************************************************************************/
static void upper_min(POINT *min,void *data)
{
  min->x=1;
  min->y=45;
}

/****************************************************************
 used once in the overview page and once in the happiness page
 **info_label points to the info_label in the respective struct
****************************************************************/
static struct fcwin_box *create_city_info_table(HWND owner, HWND *info_label)
{
  int i;
  struct fcwin_box *hbox, *column1, *column2;
  HWND label;

  static const char *output_label[NUM_INFO_FIELDS] = {
    N_("Food:"),
    N_("Prod:"),
    N_("Trade:"),
    N_("Gold:"),
    N_("Luxury:"),
    N_("Science:"),
    N_("Granary:"),
    N_("Change in:"),
    N_("Corruption:"),
    N_("Waste:"),
    N_("Pollution:"),
    N_("Plague Risk:")
  };

  hbox = fcwin_hbox_new(owner, TRUE);
  column1 = fcwin_vbox_new(owner, FALSE);
  column2 = fcwin_vbox_new(owner, FALSE);
  fcwin_box_add_box(hbox, column1, TRUE, TRUE, 0);
  fcwin_box_add_box(hbox, column2, TRUE, TRUE, 0);

  for (i = 0; i < NUM_INFO_FIELDS; i++) {
    int padding;
    switch(i) {
      case 2:
      case 6:
      case 7:
	padding = 10;
	break;
      default:
	padding = 0;
    }
    
    label = fcwin_box_add_static(column1, _(output_label[i]), 0, SS_LEFT,
				 FALSE, FALSE, padding);
    SendMessage(label,
	      WM_SETFONT,(WPARAM) font_12courier,MAKELPARAM(TRUE,0)); 

    label = fcwin_box_add_static(column2, " ", 0, SS_LEFT, FALSE, FALSE, padding);
    info_label[i] = label;
    SendMessage(label,
	      WM_SETFONT,(WPARAM) font_12courier,MAKELPARAM(TRUE,0)); 
  }

  return hbox;
}

/**************************************************************************
...
**************************************************************************/
static void CityDlgCreate(HWND hWnd,struct city_dialog *pdialog)
{
  int ybut;
  int i;
  HDC hdc;
  LV_COLUMN lvc;
  struct fcwin_box *lbupper;
  struct fcwin_box *lbunder;
  struct fcwin_box *upper_row;
  struct fcwin_box *child_vbox;
  struct fcwin_box *left_labels;
  struct fcwin_box *grp_box;
  struct cma_gui_initdata *cmadata = fc_malloc(sizeof(struct cma_gui_initdata));
  struct worklist_window_init wl_init;
  void *user_data[NUM_TABS];
  static char *titles_[NUM_TABS]={N_("City Overview"),N_("Worklist"),
				  N_("Happiness"),  N_("Citizen Governor"), 
				  N_("Trade Routes"),
				  N_("Misc. Settings")};
  static char *titles[NUM_TABS];
  if (titles[0] == NULL) {
    for(i=0;i<NUM_TABS;i++) {
      titles[i]=_(titles_[i]);
    }
  }
  pdialog->mainwindow=hWnd;
  pdialog->map_w=city_map_width;
  pdialog->map_h=city_map_height;
  pdialog->upper_y=45;
  pdialog->pop_y=15;
  pdialog->pop_x=20;
  pdialog->supported_y=pdialog->map.y+pdialog->map_h+12;
  pdialog->present_y=pdialog->supported_y+tileset_tile_height(tileset)+12+4+tileset_small_sprite_height(tileset);
  ybut=pdialog->present_y+tileset_tile_height(tileset)+12+4+tileset_small_sprite_height(tileset);
    
  hdc=GetDC(pdialog->mainwindow);
  pdialog->map_bmp = CreateCompatibleBitmap(hdc, city_map_width,
					    city_map_height);
  pdialog->citizen_bmp=CreateCompatibleBitmap(hdc,
					      NUM_CITIZENS_SHOWN*
					      tileset_small_sprite_width(tileset),
					      tileset_small_sprite_height(tileset));
  ReleaseDC(pdialog->mainwindow,hdc);

  pdialog->full_win=fcwin_vbox_new(hWnd,FALSE);
  fcwin_box_add_generic(pdialog->full_win,upper_min,upper_set,NULL,NULL,
			FALSE,FALSE,0);
  for(i=0;i<NUM_TABS;i++) {
    user_data[i]=pdialog;
  }
  user_data[PAGE_WORKLIST] = &wl_init;
  cmadata->pcity = pdialog->pcity;
  cmadata->pdialog = NULL;
  user_data[PAGE_CMA] = cmadata;
  wl_init.pwl = &pdialog->pcity->worklist;
  wl_init.pcity = pdialog->pcity;
  wl_init.parent = pdialog->mainwindow;
  wl_init.user_data = pdialog;
  wl_init.ok_cb = commit_city_worklist;
  wl_init.cancel_cb = NULL;
  pdialog->tab_ctrl=fcwin_box_add_tab(pdialog->full_win,tab_procs,
				      pdialog->tab_childs,
				      titles,user_data,NUM_TABS,
				      0,0,TRUE,TRUE,5);
  
  upper_row=fcwin_hbox_new(pdialog->tab_childs[0],FALSE);
  left_labels = create_city_info_table(pdialog->tab_childs[0], 
				       pdialog->info_label[0]);
  fcwin_box_add_groupbox(upper_row,_("City info"),left_labels,0,
			 FALSE,FALSE,5);
  
  grp_box=fcwin_vbox_new(pdialog->tab_childs[0],FALSE);
  fcwin_box_add_groupbox(upper_row,_("City map"),grp_box,0,TRUE,FALSE,5);
  fcwin_box_add_generic(grp_box,map_minsize,map_setsize,NULL,&pdialog->map,
			TRUE,FALSE,0);
  
  lbunder=fcwin_hbox_new(pdialog->tab_childs[0],FALSE);
  pdialog->sell_but=fcwin_box_add_button(lbunder,_("Sell"),ID_CITY_SELL,0,
					 TRUE,TRUE,5);
  lbupper=fcwin_hbox_new(pdialog->tab_childs[0], FALSE);
  pdialog->build_area=fcwin_box_add_static(lbupper,"",0,SS_LEFT,
					   TRUE,TRUE,5);
  pdialog->buy_but=fcwin_box_add_button(lbupper,_("Buy"),ID_CITY_BUY,0,
					TRUE,TRUE,5);
  pdialog->change_but=fcwin_box_add_button(lbupper,_("Change"),ID_CITY_CHANGE,
					   0,TRUE,TRUE,5);
  pdialog->listbox_area=fcwin_vbox_new(pdialog->tab_childs[0],FALSE);
  fcwin_box_add_box(pdialog->listbox_area,lbupper,FALSE,FALSE,0);
  pdialog->buildings_list=fcwin_box_add_listview(pdialog->listbox_area,4,
						 ID_CITY_BLIST,LVS_REPORT,
						 TRUE,TRUE,0);
  fcwin_box_add_box(pdialog->listbox_area,lbunder,FALSE,FALSE,0);
  fcwin_box_add_box(upper_row,pdialog->listbox_area,TRUE,TRUE,5);

  child_vbox=fcwin_vbox_new(pdialog->tab_childs[0],FALSE);
  fcwin_box_add_box(child_vbox,upper_row,TRUE,TRUE,0);
  pdialog->supported_label=fcwin_box_add_static_default(child_vbox,
							_("Supported units"),
							-1,SS_LEFT);
  fcwin_box_add_generic(child_vbox,
			supported_minsize,supported_setsize,NULL,pdialog,
			FALSE,FALSE,5);
  pdialog->present_label=fcwin_box_add_static_default(child_vbox,
						      _("Units present"),
						      -1,SS_LEFT);
  fcwin_box_add_generic(child_vbox,
			present_minsize,present_setsize,NULL,pdialog,
			FALSE,FALSE,5);
  pdialog->buttonrow=fcwin_hbox_new(hWnd,TRUE);
  pdialog->close_but=fcwin_box_add_button_default(pdialog->buttonrow,_("Close"),
				       ID_CITY_CLOSE,0);
  pdialog->rename_but=fcwin_box_add_button_default(pdialog->buttonrow,_("Rename"),
					ID_CITY_RENAME,0);
  
  pdialog->activate_but=fcwin_box_add_button_default(pdialog->buttonrow,
					  _("Activate Units"),
					  ID_CITY_ACTIVATE,0);
  pdialog->unitlist_but=fcwin_box_add_button_default(pdialog->buttonrow,
					  _("Unit List"),
					  ID_CITY_UNITLIST,0);
  fcwin_box_add_box(pdialog->full_win,pdialog->buttonrow,FALSE,FALSE,5);
 
  SendMessage(pdialog->supported_label,
	      WM_SETFONT,(WPARAM) font_12courier,MAKELPARAM(TRUE,0)); 
  SendMessage(pdialog->present_label,
	      WM_SETFONT,(WPARAM) font_12courier,MAKELPARAM(TRUE,0)); 
  SendMessage(pdialog->build_area,
	      WM_SETFONT,(WPARAM) font_12courier,MAKELPARAM(TRUE,0));    
  genlist_prepend(dialog_list, pdialog);    
 
  for(i=0; i<NUM_UNITS_SHOWN;i++)
    {
      pdialog->support_unit_ids[i]=-1;  
      pdialog->present_unit_ids[i]=-1;    
    }
  lvc.mask=LVCF_TEXT | LVCF_FMT;
  lvc.pszText=_("Improvement");
  lvc.fmt=LVCFMT_RIGHT;
  ListView_InsertColumn(pdialog->buildings_list,0,&lvc);
  lvc.pszText=_("Upkeep");
  ListView_InsertColumn(pdialog->buildings_list,1,&lvc);
  ListView_SetColumnWidth(pdialog->buildings_list,0,LVSCW_AUTOSIZE);
  ListView_SetColumnWidth(pdialog->buildings_list,1,LVSCW_AUTOSIZE_USEHEADER);

  improvement_iterate(pimprove) {
    pdialog->last_improvlist_seen[improvement_index(pimprove)] = 0;
  } improvement_iterate_end;
  
  pdialog->mainwindow=hWnd;
  fcwin_set_box(pdialog->tab_childs[0],child_vbox);
  fcwin_set_box(pdialog->mainwindow,pdialog->full_win);
  ShowWindow(pdialog->tab_childs[0],SW_SHOWNORMAL);
}

/**************************************************************************
...
**************************************************************************/


static void buy_callback_yes(HWND w, void * data)
{
  struct city_dialog *pdialog = data;

  city_buy_production(pdialog->pcity);
 
  destroy_message_dialog(w);
}
 
 
/****************************************************************
...
*****************************************************************/
static void buy_callback_no(HWND w, void * data)
{
  destroy_message_dialog(w);
}

/**************************************************************************
...
**************************************************************************/
static void buy_callback(struct city_dialog *pdialog)
{
  char buf[512];
  struct city *pcity = pdialog->pcity;
  const char *name = city_production_name_translation(pcity);
  int value = city_production_buy_gold_cost(pcity);
 
  if (value <= client.conn.playing->economic.gold) {
    my_snprintf(buf, sizeof(buf),
            _("Buy %s for %d gold?\nTreasury contains %d gold."),
            name, value, client.conn.playing->economic.gold);
 
    popup_message_dialog(pdialog->mainwindow, /*"buydialog"*/ _("Buy It!"), buf,
                         _("_Yes"), buy_callback_yes, pdialog,
                         _("_No"), buy_callback_no, 0, 0);
  }
  else {
    my_snprintf(buf, sizeof(buf),
            _("%s costs %d gold.\nTreasury contains %d gold."),
            name, value, client.conn.playing->economic.gold);
 
    popup_message_dialog(NULL, /*"buynodialog"*/ _("Buy It!"), buf,
                         _("Darn"), buy_callback_no, 0, 0);
  }      
}

/****************************************************************
...
*****************************************************************/
static void sell_callback_yes(HWND w, void * data)
{
  struct city_dialog *pdialog = data;

  city_sell_improvement(pdialog->pcity, pdialog->sell_id);
 
  destroy_message_dialog(w);
}
 
 
/****************************************************************
...
*****************************************************************/
static void sell_callback_no(HWND w, void * data)
{
  destroy_message_dialog(w);
}
 
 
/****************************************************************
...
*****************************************************************/
static void sell_callback(struct city_dialog *pdialog)
{
  char buf[100];
  if (pdialog->id_selected<0) {
    return;
  }
 
  if (!can_city_sell_building(pdialog->pcity, improvement_by_number(pdialog->id_selected))) {
    return;
  }
  
  pdialog->sell_id = pdialog->id_selected;
  my_snprintf(buf, sizeof(buf), _("Sell %s for %d gold?"),
	      city_improvement_name_translation(pdialog->pcity, improvement_by_number(pdialog->id_selected)),
	      impr_sell_gold(improvement_by_number(pdialog->id_selected)));
  
  popup_message_dialog(pdialog->mainwindow, /*"selldialog" */
		       _("Sell It!"), buf, _("_Yes"),
		       sell_callback_yes, pdialog, _("_No"),
		       sell_callback_no, pdialog, 0);

}


/**************************************************************************
...
**************************************************************************/


static LONG CALLBACK changedlg_proc(HWND hWnd,
				    UINT message,
				    WPARAM wParam,
				    LPARAM lParam) 
{
  int sel,i,n;
  struct universal target;
  struct city_dialog *pdialog;
  pdialog=(struct city_dialog *)fcwin_get_user_data(hWnd);
  switch(message)
    {
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;
    case WM_COMMAND:
      n=ListView_GetItemCount(GetDlgItem(hWnd,ID_PRODCHANGE_LIST));
      sel=-1;
      for(i=0;i<n;i++) {
	if (ListView_GetItemState(GetDlgItem(hWnd,ID_PRODCHANGE_LIST),i,
				  LVIS_SELECTED)) {
	  sel=i;
	  break;
	}
      }
      if (sel>=0) {
	target = pdialog->change_list_targets[sel];
      }
      switch(LOWORD(wParam))
	{
	case ID_PRODCHANGE_CANCEL:
	  DestroyWindow(hWnd);
	  break;
	case ID_PRODCHANGE_HELP:
	  if (sel>=0)
	    {
	      if (VUT_UTYPE == target.kind) {
		popup_help_dialog_typed(utype_name_translation(target.value.utype),
					HELP_UNIT);
	      } else if (is_great_wonder(target.value.building)) {
		popup_help_dialog_typed(improvement_name_translation(target.value.building),
					HELP_WONDER);
	      } else {
		popup_help_dialog_typed(improvement_name_translation(target.value.building),
					HELP_IMPROVEMENT);
	      }                                                                     
	    }
	  else
	    {
	      popup_help_dialog_string(HELP_IMPROVEMENTS_ITEM);  
	    }
	  break;
	case ID_PRODCHANGE_CHANGE:
	  if (sel>=0)
	    {
	      city_change_production(pdialog->pcity, target);
	      DestroyWindow(hWnd);
	    }
	  break;
	}
      break;
    case WM_DESTROY:
    case WM_SIZE:
    case WM_GETMINMAXINFO:
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
    }
  return 0;
}

/**************************************************************************
...
**************************************************************************/

     
static void change_callback(struct city_dialog *pdialog)
{
  HWND dlg;
  
  char *row   [4];
  char  buf   [4][64];

  int i,n;

  dlg=fcwin_create_layouted_window(changedlg_proc,_("Change Production"),
				   WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,pdialog->mainwindow,NULL,REAL_CHILD,pdialog);
  if (dlg)
    {
      struct fcwin_box *hbox;
      struct fcwin_box *vbox;
      HWND lv;
      LV_COLUMN lvc;
      LV_ITEM lvi;
      struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
      struct item items[MAX_NUM_PRODUCTION_TARGETS];
      int targets_used, item;

      vbox=fcwin_vbox_new(dlg,FALSE);
      hbox=fcwin_hbox_new(dlg,TRUE);
      fcwin_box_add_button(hbox,_("Change"),ID_PRODCHANGE_CHANGE,
			   0,TRUE,TRUE,20);
      fcwin_box_add_button(hbox,_("Cancel"),ID_PRODCHANGE_CANCEL,
			   0,TRUE,TRUE,20);
      fcwin_box_add_button(hbox,_("Help"),ID_PRODCHANGE_HELP,
			   0,TRUE,TRUE,20);
      lv=fcwin_box_add_listview(vbox,10,ID_PRODCHANGE_LIST,LVS_REPORT | LVS_SINGLESEL,
				TRUE,TRUE,10);
      fcwin_box_add_box(vbox,hbox,FALSE,FALSE,10);
  
      lvc.pszText=_("Type");
      lvc.mask=LVCF_TEXT;
      ListView_InsertColumn(lv,0,&lvc);
      lvc.pszText=_("Info");
      lvc.mask=LVCF_TEXT | LVCF_FMT;
      lvc.fmt=LVCFMT_RIGHT;
      ListView_InsertColumn(lv,1,&lvc);
      lvc.pszText=_("Cost");
      ListView_InsertColumn(lv,2,&lvc);
      lvc.pszText=_("Turns");
      ListView_InsertColumn(lv,3,&lvc);
      lvi.mask=LVIF_TEXT;
      for(i=0; i<4; i++)
	row[i]=buf[i];

      targets_used =
	 collect_eventually_buildable_targets(targets, pdialog->pcity,
					      FALSE);  
      name_and_sort_items(targets, targets_used, items, FALSE,
			  pdialog->pcity);

      n = 0;
      for (item = 0; item < targets_used; item++) {
	if (can_city_build_now(pdialog->pcity, items[item].item)) {
	  struct universal target = items[item].item;

	  get_city_dialog_production_row(row, sizeof(buf[0]), target,
				       pdialog->pcity);
	  fcwin_listview_add_row(lv,n,4,row);
	  pdialog->change_list_targets[n++] = target;
	}
      }
      
      ListView_SetColumnWidth(lv,0,LVSCW_AUTOSIZE);
      for(i=1;i<4;i++) {
	ListView_SetColumnWidth(lv,i,LVSCW_AUTOSIZE_USEHEADER);	
      }
      fcwin_set_box(dlg,vbox);
      ShowWindow(dlg,SW_SHOWNORMAL);
    }
  
}

/****************************************************************
  Commit the changes to the worklist for the city.
*****************************************************************/
static void commit_city_worklist(struct worklist *pwl, void *data)
{
  struct city_dialog *pdialog = data;

  city_worklist_commit(pdialog->pcity, pwl);
}

/**************************************************************************
...
**************************************************************************/
static void rename_city_callback(HWND w, void * data)
{
  struct city_dialog *pdialog = data;
 
  if (pdialog) {
    city_rename(pdialog->pcity, input_dialog_get_input(w));
  }
 
  input_dialog_destroy(w);
}
 
/****************************************************************
...
*****************************************************************/    
static void rename_callback(struct city_dialog *pdialog)
{
  input_dialog_create(pdialog->mainwindow,
                      /*"shellrenamecity"*/_("Rename City"),
                      _("What should we rename the city to?"),
                      city_name(pdialog->pcity),
                      (void*)rename_city_callback, (void *)pdialog,
                      (void*)rename_city_callback, (void *)0);    
}

/**************************************************************************

**************************************************************************/
static LONG CALLBACK trade_page_proc(HWND win, UINT message,
				     WPARAM wParam, LPARAM lParam)
{
  struct city_dialog *pdialog;
  struct fcwin_box *vbox;
  switch(message) 
    {
    case WM_CREATE:
      pdialog=(struct city_dialog *)fcwin_get_user_data(win);
      vbox=fcwin_vbox_new(win,FALSE);
      pdialog->trade_label=fcwin_box_add_static(vbox," ",
					       0,SS_LEFT,
					       FALSE,FALSE,0);
      fcwin_set_box(win,vbox);
      break;
    case WM_DESTROY:
    case WM_SIZE:
    case WM_GETMINMAXINFO:
    case WM_COMMAND:
      break;
    default:
      return DefWindowProc(win,message,wParam,lParam);
    }
  return 0;
}

/****************************************************************
...
*****************************************************************/
static void city_dialog_update_tradelist(struct city_dialog *pdialog)
{
  int i, x = 0, total = 0;

  char cityname[64], buf[512];

  buf[0] = '\0';

  for (i = 0; i < NUM_TRADEROUTES; i++) {
    if (pdialog->pcity->trade[i]) {
      struct city *pcity;
      x = 1;
      total += pdialog->pcity->trade_value[i];

      if ((pcity = game_find_city_by_number(pdialog->pcity->trade[i]))) {
        sz_strlcpy(cityname, city_name(pcity));
      } else {
        sz_strlcpy(cityname, _("Unknown"));
      }
      cat_snprintf(buf, sizeof(buf), _("Trade with %s gives %d trade.\n"),
		   cityname, pdialog->pcity->trade_value[i]);
    }
  }
  if (!x) {
    cat_snprintf(buf, sizeof(buf), _("No trade routes exist."));
  } else {
    cat_snprintf(buf, sizeof(buf), _("Total trade from trade routes: %d"),
		 total);
  }
  SetWindowText(pdialog->trade_label,buf);
}

/**************************************************************************
...
**************************************************************************/
static void supported_units_activate_close_callback(HWND w, void * data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  destroy_message_dialog(w);

  if (NULL != punit) {
    struct city *pcity =
      player_find_city_by_id(client.conn.playing, punit->homecity);

    set_unit_focus(punit);
    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if (NULL != pdialog) {
        CityDlgClose(pdialog);
      }
    }
  }
}

/**************************************************************************
...
**************************************************************************/
static void activate_callback(struct city_dialog *pdialog)
{
  activate_all_units(pdialog->pcity->tile);
}

/****************************************************************
...
*****************************************************************/
static void show_units_callback(struct city_dialog *pdialog)
{
  struct tile *ptile = pdialog->pcity->tile;
 
  if(unit_list_size(ptile->units))
    popup_unit_select_dialog(ptile);
}

/**************************************************************************
...
**************************************************************************/
static void present_units_disband_callback(HWND w, void *data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_disband(punit);
  }

  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void present_units_homecity_callback(HWND w, void * data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_change_homecity(punit);
  }

  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void present_units_cancel_callback(HWND w, void *data)
{
  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/              
static void present_units_activate_callback(HWND w, void * data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    set_unit_focus(punit);
  }

  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void present_units_activate_close_callback(HWND w, void * data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  destroy_message_dialog(w);

  if (NULL != punit) {
    struct city *pcity = tile_city(punit->tile);

    set_unit_focus(punit);
    if (NULL != pcity) {
      struct city_dialog *pdialog = get_city_dialog(pcity);

      if (NULL != pdialog) {
        CityDlgClose(pdialog);
      }
    }
  }
}

/**************************************************************************
...
**************************************************************************/
static void present_units_sentry_callback(HWND w, void * data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_sentry(punit);
  }

  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void present_units_fortify_callback(HWND w, void * data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_fortify(punit);
  }

  destroy_message_dialog(w);
}

/**************************************************************************
...
**************************************************************************/
static void unitupgrade_callback_yes(HWND w, void * data)
{
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t)data);

  if (NULL != punit) {
    request_unit_upgrade(punit);
  }

  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void unitupgrade_callback_no(HWND w, void * data)
{
  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void upgrade_callback(HWND w, void * data)
{
  char buf[512];
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, (size_t) data);

  if (NULL == punit) {
    return;
  }

  if (get_unit_upgrade_info(buf, sizeof(buf), punit) == UR_OK) {
    popup_message_dialog(NULL,
			 _("Upgrade Obsolete Units"), buf,
			 _("_Yes"),
			 unitupgrade_callback_yes, (void *)(punit->id),
			 _("_No"),
			 unitupgrade_callback_no, 0,
			 NULL);
  } else {
    popup_message_dialog(NULL, _("Upgrade Unit!"), buf,
			 _("Darn"), unitupgrade_callback_no, 0,
			 NULL);
  }

  destroy_message_dialog(w);
}

/**************************************************************************
...
**************************************************************************/
static void city_dlg_click_supported(struct city_dialog *pdialog, int n)
{
  struct city *pcity;
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, pdialog->support_unit_ids[n]);

  if (NULL != punit
      && (pcity = game_find_city_by_number(punit->homecity))) {
    HWND wd = popup_message_dialog(NULL,
           /*"supportunitsdialog"*/ _("Unit Commands"),
           unit_description(punit),
           _("_Activate unit"),
             present_units_activate_callback, punit->id,
           _("Activate unit, _close dialog"),
             supported_units_activate_close_callback, punit->id, /* act+c */
           _("_Disband unit"),
             present_units_disband_callback, punit->id,
           _("_Cancel"),
             present_units_cancel_callback, 0, 0, NULL);

    if (unit_has_type_flag(punit, F_UNDISBANDABLE)) {
      message_dialog_button_set_sensitive(wd, 3, FALSE);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
static void city_dlg_click_present(struct city_dialog *pdialog, int n)
{
  struct city *pcity;
  struct unit *punit =
    player_find_unit_by_id(client.conn.playing, pdialog->present_unit_ids[n]);

  if (NULL != punit
      && (pcity=tile_city(punit->tile))
      && (pdialog=get_city_dialog(pcity))) { /* ??? */
     HWND wd = popup_message_dialog(NULL,
                           /*"presentunitsdialog"*/_("Unit Commands"),
                           unit_description(punit),
                           _("_Activate Unit"),
                             present_units_activate_callback, punit->id,
                           _("Activate Unit, _Close Dialog"),
                             present_units_activate_close_callback, punit->id,
                           _("_Sentry Unit"),
                             present_units_sentry_callback, punit->id,
                           _("_Fortify Unit"),
                             present_units_fortify_callback, punit->id,
                           _("_Disband Unit"),
                             present_units_disband_callback, punit->id,
                           _("Set _Home City"),
                             present_units_homecity_callback, punit->id,
                           _("_Upgrade Unit"),
                             upgrade_callback, punit->id,
                           _("_Cancel"),
                             present_units_cancel_callback, 0,
                           NULL);

     if (punit->activity == ACTIVITY_SENTRY
	 || !can_unit_do_activity(punit, ACTIVITY_SENTRY)) {
       message_dialog_button_set_sensitive(wd,2, FALSE);
     }
     if (punit->activity == ACTIVITY_FORTIFYING
	 || !can_unit_do_activity(punit, ACTIVITY_FORTIFYING)) {
       message_dialog_button_set_sensitive(wd,3, FALSE);
     }
     if (unit_has_type_flag(punit, F_UNDISBANDABLE)) {
       message_dialog_button_set_sensitive(wd,4, FALSE);
     }
     if (punit->homecity == pcity->id) {
       message_dialog_button_set_sensitive(wd,5, FALSE);
     }

     if (NULL == can_upgrade_unittype(client.conn.playing,unit_type(punit))) {
       message_dialog_button_set_sensitive(wd,6, FALSE);
     }
   }
}

/**************************************************************************
...
**************************************************************************/
static void city_dlg_click_citizens(struct city_dialog *pdialog, int n)
{
  city_rotate_specialist(pdialog->pcity, n);
}

/**************************************************************************
...
**************************************************************************/


static void city_dlg_mouse(struct city_dialog *pdialog, int x, int y,
			   bool is_overview)
{
  int xr,yr;
  /* click on citizens */
  
  if ((!is_overview)&&
      (y>=pdialog->pop_y)&&(y<(pdialog->pop_y+tileset_small_sprite_height(tileset))))
    {
      xr=x-pdialog->pop_x;
      if (x>=0)
	{
	  xr/=tileset_small_sprite_width(tileset);
	  if (xr<NUM_CITIZENS_SHOWN)
	    {
	      city_dlg_click_citizens(pdialog,xr);
	      return;
	    }
	}
    }
  
  if (!is_overview)
    return;
  /* click on map */
  if ((y>=pdialog->map.y)&&(y<(pdialog->map.y+pdialog->map_h)))
    {
      if ((x>=pdialog->map.x)&&(x<(pdialog->map.x+pdialog->map_w)))
	{
	  int tile_x,tile_y;
	  xr = x - pdialog->map.x;
	  yr = y - pdialog->map.y;

	  if (canvas_to_city_pos(&tile_x, &tile_y, xr, yr)) {
	    city_toggle_worker(pdialog->pcity, tile_x, tile_y);
	  }
	}
    }
  xr=x-pdialog->pop_x;
  if (xr<0) return;
  if (xr%(tileset_tile_width(tileset)+tileset_small_sprite_width(tileset))>tileset_tile_width(tileset))
    return;
  xr/=(tileset_tile_width(tileset)+tileset_small_sprite_width(tileset));
  
  /* click on present units */
  if ((y>=pdialog->present_y)&&(y<(pdialog->present_y+tileset_tile_height(tileset))))
    {
      city_dlg_click_present(pdialog,xr);
      return;
    }
  if ((y>=pdialog->supported_y)&&
      (y<(pdialog->supported_y+tileset_tile_height(tileset)+tileset_small_sprite_height(tileset))))
    {
      city_dlg_click_supported(pdialog,xr);
      return;
    }
}


/**************************************************************************

**************************************************************************/
static LONG CALLBACK citydlg_overview_proc(HWND win, UINT message,
					   WPARAM wParam, LPARAM lParam)
{
  HBITMAP old;
  HDC hdc;
  HDC hdcsrc;
  PAINTSTRUCT ps;
  int n,i;
  struct city_dialog *pdialog;
  pdialog=fcwin_get_user_data(win);
  if (!pdialog) {
    return DefWindowProc(win,message,wParam,lParam);
  }
  
  switch(message)
    {
    case WM_CREATE:
    case WM_SIZE:
    case WM_GETMINMAXINFO:
    case WM_DESTROY:
      break;
    case WM_PAINT:
      hdc=BeginPaint(win,(LPPAINTSTRUCT)&ps);
      hdcsrc = CreateCompatibleDC(NULL);

      old=SelectObject(hdcsrc,pdialog->map_bmp);
      BitBlt(hdc,pdialog->map.x,pdialog->map.y,
	     pdialog->map_w,pdialog->map_h,
	     hdcsrc,0,0,SRCCOPY);
      SelectObject(hdcsrc,old);
      DeleteDC(hdcsrc);
      city_dialog_update_supported_units(hdc, pdialog);
      city_dialog_update_present_units(hdc, pdialog); 
      EndPaint(win,(LPPAINTSTRUCT)&ps);
      break;
    case WM_LBUTTONDOWN:
      city_dlg_mouse(pdialog,LOWORD(lParam),HIWORD(lParam),TRUE);
      break;
    case WM_COMMAND:
      n=ListView_GetItemCount(pdialog->buildings_list);
      for(i=0;i<n;i++) {
	if (ListView_GetItemState(pdialog->buildings_list,
				  i, LVIS_SELECTED)) {
	  pdialog->id_selected = pdialog->building_values[i];
	  break;
	}
      }
      if (i == n) {
	pdialog->id_selected=-1;
      }
      switch(LOWORD(wParam)) 
	{
	case ID_CITY_BUY:
	  buy_callback(pdialog);
	  break;
	case ID_CITY_SELL:
	  sell_callback(pdialog);
	  break;
	case ID_CITY_CHANGE:
	  change_callback(pdialog);
	  break;
	}
      break;
    default:
      return DefWindowProc(win,message,wParam,lParam);
    }
  return 0;
}

/**************************************************************************
...
**************************************************************************/


static LONG APIENTRY CitydlgWndProc(HWND hWnd, UINT message,
				    UINT wParam, LONG lParam)
{
  HDC hdcsrc;
  HBITMAP old;
  LPNMHDR nmhdr;
  HDC hdc;
  PAINTSTRUCT ps;
  struct city_dialog *pdialog;
  if (message==WM_CREATE)
    {
      CityDlgCreate(hWnd,
		    (struct city_dialog *)
		    fcwin_get_user_data(hWnd));
      return 0;
    }
  pdialog=fcwin_get_user_data(hWnd);
  if (!pdialog) return DefWindowProc(hWnd,message,wParam,lParam);
  switch(message)
    {
    case WM_CLOSE:
      SendMessage(pdialog->tab_childs[PAGE_WORKLIST],WM_COMMAND,IDOK,0);
      CityDlgClose(pdialog);
      break;
    case WM_DESTROY:
      genlist_unlink(dialog_list, pdialog);
      DeleteObject(pdialog->map_bmp);
      DeleteObject(pdialog->citizen_bmp);
      cleanup_happiness_box(pdialog->happiness);
      free(pdialog);
      break;
    case WM_GETMINMAXINFO:
      break;
    case WM_SIZE:
      break;
    case WM_PAINT:
      hdc=BeginPaint(hWnd,(LPPAINTSTRUCT)&ps);
      hdcsrc = CreateCompatibleDC(NULL);
      old=SelectObject(hdcsrc,pdialog->citizen_bmp);
      BitBlt(hdc,pdialog->pop_x,pdialog->pop_y,
	     tileset_small_sprite_width(tileset)*NUM_CITIZENS_SHOWN,tileset_small_sprite_height(tileset),
	     hdcsrc,0,0,SRCCOPY);
      SelectObject(hdcsrc,old);
      DeleteDC(hdcsrc);
      /*
      city_dialog_update_map(hdc,pdialog);
      city_dialog_update_citizens(hdc,pdialog); */
      /*      city_dialog_update_production(hdc,pdialog);
      city_dialog_update_output(hdc,pdialog);             
      city_dialog_update_storage(hdc,pdialog);
      city_dialog_update_pollution(hdc,pdialog);  */ 
      EndPaint(hWnd,(LPPAINTSTRUCT)&ps);
      break;
    case WM_LBUTTONDOWN:
      city_dlg_mouse(pdialog,LOWORD(lParam),HIWORD(lParam),FALSE);
      break;
    case WM_COMMAND:
      switch(LOWORD(wParam))
	{

	case ID_CITY_CLOSE:
	  SendMessage(pdialog->tab_childs[PAGE_WORKLIST],WM_COMMAND,IDOK,0);
	  CityDlgClose(pdialog);
	  break;
	case ID_CITY_RENAME:
	  rename_callback(pdialog);
	  break;
	case ID_CITY_ACTIVATE:
	  activate_callback(pdialog);
	  break;
	case ID_CITY_UNITLIST:
	  show_units_callback(pdialog);
	  break;
	}
      break;
    case WM_NOTIFY:
      nmhdr=(LPNMHDR)lParam;
      if (nmhdr->hwndFrom==pdialog->tab_ctrl) {
	int i,sel;
	sel=TabCtrl_GetCurSel(pdialog->tab_ctrl);
	for(i=0;i<NUM_TABS;i++) {
	  /*  if (i!=sel) */
	  ShowWindow(pdialog->tab_childs[i],SW_HIDE); 
	}
	if ((sel>=0)&&(sel<NUM_TABS))
	  ShowWindow(pdialog->tab_childs[sel],SW_SHOWNORMAL);
	SendMessage(pdialog->tab_childs[PAGE_WORKLIST],WM_COMMAND,IDOK,0);
      }
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
    }
  return (0);
}

/**************************************************************************
...
**************************************************************************/

struct city_dialog *create_city_dialog(struct city *pcity)
{   
  struct city_dialog *pdialog;
  pdialog=fc_malloc(sizeof(struct city_dialog));
  pdialog->pcity=pcity;
  pdialog->resized=0;
  pdialog->cityopt_dialog=NULL;
  pdialog->mainwindow=
    fcwin_create_layouted_window(CitydlgWndProc,city_name(pcity),
			   WS_OVERLAPPEDWINDOW,
			   20,20,
			   root_window,
			   NULL,
			   JUST_CLEANUP,
			   pdialog);


  resize_city_dialog(pdialog);
  refresh_city_dialog(pdialog->pcity); 
  ShowWindow(pdialog->mainwindow,SW_SHOWNORMAL);
  return pdialog;
  
}
/****************************************************************
...
*****************************************************************/
static void initialize_city_dialogs(void)
{
  assert(!city_dialogs_have_been_initialised);

  dialog_list = genlist_new();
  city_map_width = get_citydlg_canvas_width();
  city_map_height = get_citydlg_canvas_height();
  city_dialogs_have_been_initialised=1;
}


/**************************************************************************
...
**************************************************************************/

struct city_dialog *get_city_dialog(struct city *pcity)
{   
  if (!city_dialogs_have_been_initialised) {
    initialize_city_dialogs();
  }

  TYPED_LIST_ITERATE(struct city_dialog, dialog_list, pdialog) {
    if (pdialog->pcity == pcity) {
      return pdialog;
    }
  } LIST_ITERATE_END;

  return 0;
}

/**************************************************************************
...
**************************************************************************/

void popup_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog;
 
  if(!(pdialog=get_city_dialog(pcity))) {
    pdialog=create_city_dialog(pcity); 
  } else {
    SetFocus(pdialog->mainwindow);
  }
}

/**************************************************************************
...
**************************************************************************/


void
popdown_city_dialog(struct city *pcity)
{
  struct city_dialog *pdialog;
  
  if((pdialog=get_city_dialog(pcity)))
     CityDlgClose(pdialog);
}

/**************************************************************************
...
**************************************************************************/


static void popdown_cityopt_dialog(void)
{
}

/**************************************************************************
...
**************************************************************************/


void
popdown_all_city_dialogs(void)
{
  if(!city_dialogs_have_been_initialised) {
    return;
  }
  while(genlist_size(dialog_list)) {
    CityDlgClose(genlist_get(dialog_list,0));
  }
  popdown_cityopt_dialog();     
}

/**************************************************************************
...
**************************************************************************/
void citydlg_tileset_change(void)
{
  if (!city_dialogs_have_been_initialised) {
    initialize_city_dialogs();
  }

  city_map_width = get_citydlg_canvas_width();
  city_map_height = get_citydlg_canvas_height();

  TYPED_LIST_ITERATE(struct city_dialog, dialog_list, pdialog) {
    HDC hdc;

    DeleteObject(pdialog->map_bmp);
    hdc = GetDC(pdialog->mainwindow);
    pdialog->map_bmp = CreateCompatibleBitmap(hdc, city_map_width,
					      city_map_height);
    ReleaseDC(pdialog->mainwindow, hdc);
    pdialog->map_w = city_map_width;
    pdialog->map_h = city_map_height;
    resize_city_dialog(pdialog);
    refresh_city_dialog(pdialog->pcity);
  } LIST_ITERATE_END;
}

/**************************************************************************
...
**************************************************************************/
void refresh_unit_city_dialogs(struct unit *punit)
{
  struct city_dialog *pdialog;
  struct city *pcity_pre = tile_city(punit->tile);
  struct city *pcity_sup =
    player_find_city_by_id(client.conn.playing, punit->homecity);
  
  if(pcity_sup && (pdialog=get_city_dialog(pcity_sup)))     
    {
      HDC hdc;
      hdc=GetDC(pdialog->tab_childs[0]);
      city_dialog_update_supported_units(hdc, pdialog);
      ReleaseDC(pdialog->tab_childs[0],hdc);
    }
  if(pcity_pre && (pdialog=get_city_dialog(pcity_pre)))   
    {
      HDC hdc;
      hdc=GetDC(pdialog->tab_childs[0]);
      city_dialog_update_present_units(hdc, pdialog);
      ReleaseDC(pdialog->tab_childs[0],hdc);
    }
}

/**************************************************************************
...
**************************************************************************/


LONG APIENTRY citydlg_config_proc(HWND hWnd,
				   UINT message,
				   UINT wParam,
				   LONG lParam)
{
  struct city_dialog *pdialog;
  pdialog=(struct city_dialog *)fcwin_get_user_data(hWnd);
  switch (message)
    {
    case WM_CREATE:
      {
	int ncitizen_idx;
	struct fcwin_box *vbox;
	struct city *pcity;
	pdialog=(struct city_dialog *)fcwin_get_user_data(hWnd);
	pcity=pdialog->pcity;
	pdialog->cityopt_dialog=hWnd;
	vbox=fcwin_vbox_new(hWnd,FALSE);
	pdialog->opt_winbox=vbox;
	fcwin_box_add_static_default(vbox,city_name(pdialog->pcity),-1,SS_CENTER);
	fcwin_box_add_static_default(vbox,_("New citizens are"),-1,SS_LEFT);
	fcwin_box_add_radiobutton(vbox,_("Elvises"),ID_CITYOPT_RADIO,
				  WS_GROUP,FALSE,FALSE,5);
	fcwin_box_add_radiobutton(vbox,_("Scientists"),ID_CITYOPT_RADIO+1,
				  0,FALSE,FALSE,5);
	fcwin_box_add_radiobutton(vbox,_("Taxmen"),ID_CITYOPT_RADIO+2,
				  0,FALSE,FALSE,5);
	fcwin_box_add_static_default(vbox," ",-1,SS_LEFT | WS_GROUP);
	fcwin_box_add_checkbox(vbox,_("Disband if build settler at size 1"),
				  ID_CITYOPT_TOGGLE,0,FALSE,FALSE,5);
	fcwin_set_box(hWnd,vbox);
	CheckDlgButton(pdialog->cityopt_dialog, ID_CITYOPT_TOGGLE,
		       is_city_option_set(pcity, CITYO_DISBAND)
		       ? BST_CHECKED : BST_UNCHECKED);
	if (is_city_option_set(pcity, CITYO_NEW_EINSTEIN)) {
	  ncitizen_idx = 1;
	} else if (is_city_option_set(pcity, CITYO_NEW_TAXMAN)) {
	  ncitizen_idx = 2;
	} else {
	  ncitizen_idx = 0; 
	}
	CheckRadioButton(pdialog->cityopt_dialog,
			 ID_CITYOPT_RADIO,ID_CITYOPT_RADIO+2,
			 ncitizen_idx+ID_CITYOPT_RADIO);
      }
      break;
    case WM_DESTROY:
      break;
    case WM_COMMAND:
      if (pdialog) {
	struct city *pcity = pdialog->pcity;
	bv_city_options new_options;

	assert(CITYO_LAST == 3);

	BV_CLR_ALL(new_options);

	if (IsDlgButtonChecked(hWnd, ID_CITYOPT_TOGGLE)) {
	  BV_SET(new_options, CITYO_DISBAND);
	}

	if (IsDlgButtonChecked(hWnd, ID_CITYOPT_RADIO + 1)) {
          BV_SET(new_options, CITYO_NEW_EINSTEIN);
	}
	
	if (IsDlgButtonChecked(hWnd, ID_CITYOPT_RADIO + 2)) {
          BV_SET(new_options, CITYO_NEW_TAXMAN);
	}

	dsend_packet_city_options_req(&client.conn, pcity->id, new_options);
      }
      break;
    default: 
      return DefWindowProc(hWnd,message,wParam,lParam); 
    }
  return 0;
} 

/**************************************************************************

**************************************************************************/
static void create_happiness_page(HWND win,
				  struct city_dialog *pdialog)
{
  struct fcwin_box *hbox;
  struct fcwin_box *vbox;
  struct fcwin_box *info;
  hbox = fcwin_hbox_new(win, FALSE);
  pdialog->happiness = create_happiness_box(pdialog->pcity, hbox, win);
  vbox = fcwin_vbox_new(win, FALSE);
  fcwin_box_add_box(hbox,vbox,FALSE,FALSE,0);
  fcwin_box_add_generic(vbox, map_minsize, map_setsize, NULL, &pdialog->maph,
			TRUE,FALSE,0);
  info = create_city_info_table(win, pdialog->info_label[1]);
  fcwin_box_add_box(vbox,info,FALSE,FALSE,0);
  fcwin_set_box(win,hbox);
}

/**************************************************************************
...
**************************************************************************/
static  LONG CALLBACK happiness_proc(HWND win, UINT message,
				     WPARAM wParam, LPARAM lParam)
{
  int x,y;
  struct city_dialog *pdialog;
  HDC hdc;
  HDC hdcsrc;
  HBITMAP old;
  PAINTSTRUCT ps;
  pdialog=fcwin_get_user_data(win);
  switch(message) 
    {
    case WM_CREATE:
      create_happiness_page(win,pdialog);
      break;
    case WM_SIZE:
    case WM_COMMAND:
    case WM_GETMINMAXINFO:
      break;
    case WM_DESTROY:
      break;
    case WM_LBUTTONDOWN:
      x=LOWORD(lParam);
      y=HIWORD(lParam);
      /* Test if the click is in the map */
      if ((x>=pdialog->maph.x)&&(x<(pdialog->maph.x+pdialog->map_w))&&
	  (y>=pdialog->maph.y)&&(y<(pdialog->maph.y+pdialog->map_h))) {
	int tile_x,tile_y;

	if (canvas_to_city_pos(&tile_x, &tile_y,
			       x-pdialog->maph.x, y-pdialog->maph.y)) {
	  city_toggle_worker(pdialog->pcity, tile_x, tile_y);
	}
      }
      break;
    case WM_PAINT:
      hdc=BeginPaint(win,(LPPAINTSTRUCT)&ps);
      hdcsrc = CreateCompatibleDC(NULL);
      old=SelectObject(hdcsrc,pdialog->map_bmp); 
      BitBlt(hdc,pdialog->maph.x,pdialog->maph.y,
	     pdialog->map_w,pdialog->map_h,
	     hdcsrc,0,0,SRCCOPY);
      SelectObject(hdcsrc,old);
      DeleteDC(hdcsrc);
      repaint_happiness_box(pdialog->happiness,hdc); 
      EndPaint(win,(LPPAINTSTRUCT)&ps);
      break;
    default:
      return DefWindowProc(win,message,wParam,lParam);
    }
  return 0;
}
