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
#include <string.h>

#include <windows.h>
#include <windowsx.h>

/* common & utility */ 
#include "city.h"
#include "fcintl.h"
#include "genlist.h"
#include "government.h"
#include "map.h"
#include "mem.h"
#include "movement.h"
#include "shared.h"
#include "tech.h"
#include "unit.h"
#include "support.h"
#include "version.h"

/* client */
#include "client_main.h"
#include "climisc.h"
#include "colors.h"
#include "gui_stuff.h"
#include "helpdata.h"
#include "options.h"
#include "sprite.h"
#include "tilespec.h"
                                  
#include "helpdlg.h"
#define ID_HELP_LIST 109
#define ID_HELP_TECH_LINK 110
#define ID_HELP_UNIT_LINK 111
#define ID_HELP_IMPROVEMENT_LINK 112
#define ID_HELP_WONDER_LINK 113

extern HINSTANCE freecivhinst;
extern HWND root_window;
static HWND helpdlg_win;
static HWND helpdlg_list;
static HWND helpdlg_topic;
static HWND helpdlg_text;
static HWND helpdlg_close;
static HWND help_ilabel[6];
static HWND help_ulabel[5][5];
static HWND help_tlabel[4][5];

static POINT unitpos;
static struct unit_type *drawn_unit_type = NULL;

struct fcwin_box *helpdlg_hbox;
static struct fcwin_box *helpdlg_page_vbox;
static void create_help_dialog(void);
static void help_update_dialog(const struct help_item *pitem);
/* static void create_help_page(enum help_page_type type);
 */
static void select_help_item(int item);
static void select_help_item_string(const char *item,
                                    enum help_page_type htype);
char *help_ilabel_name[6] =
{ N_("Cost:"), NULL, N_("Upkeep:"), NULL, N_("Requirement:"), NULL };
 
char *help_wlabel_name[6] =
{ N_("Cost:"), NULL, N_("Requirement:"), NULL, N_("Obsolete by:"), NULL };
 
char *help_ulabel_name[5][5] =
{
    { N_("Cost:"),         NULL, NULL, N_("Attack:"),      NULL },
    { N_("Defense:"),      NULL, NULL, N_("Move:"),        NULL },
    { N_("FirePower:"),    NULL, NULL, N_("Hitpoints:"),   NULL },
    { N_("Basic Upkeep:"), NULL, NULL, N_("Vision:"),      NULL },
    { N_("Requirement:"),  NULL, NULL, N_("Obsolete by:"), NULL }
};
 
char *help_tlabel_name[4][5] =
{
    { N_("Move/Defense:"),   NULL, NULL, N_("Food/Res/Trade:"),   NULL },
    { N_("Resources:"),      NULL, NULL, NULL                 ,   NULL },
    { N_("Road Rslt/Time:"), NULL, NULL, N_("Irrig. Rslt/Time:"), NULL },
    { N_("Mine Rslt/Time:"), NULL, NULL, N_("Trans. Rslt/Time:"), NULL }
};                                 

static void help_draw_unit(HDC hdc, struct unit_type *utype);

/**************************************************************************

**************************************************************************/
static enum help_page_type page_type_from_id(int id)
{
  switch(id){
  case ID_HELP_WONDER_LINK:
    return HELP_WONDER;
  case ID_HELP_IMPROVEMENT_LINK:
    return HELP_IMPROVEMENT;
  case ID_HELP_UNIT_LINK:
    return HELP_UNIT;
  case ID_HELP_TECH_LINK:
    return HELP_TECH;
  }
  return HELP_ANY;
}

/**************************************************************************

**************************************************************************/
static LONG APIENTRY HelpdlgWndProc(HWND hWnd,UINT uMsg,
				    UINT wParam,
				    LONG lParam)
{
  PAINTSTRUCT ps;
  HDC hdc;
  switch (uMsg)
    {
    case WM_CREATE:
      helpdlg_win=hWnd;
      return 0;
    case WM_DESTROY:
      helpdlg_win=NULL;
      break;
    case WM_GETMINMAXINFO:
    
      break;
    case WM_SIZE:
      break;
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;
    case WM_PAINT:
      hdc=BeginPaint(hWnd,(LPPAINTSTRUCT)&ps);
      if (drawn_unit_type)
	help_draw_unit(hdc, drawn_unit_type);
      EndPaint(hWnd,(LPPAINTSTRUCT)&ps);
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case IDCANCEL:
	  DestroyWindow(hWnd);
	  break;
	case ID_HELP_TECH_LINK:
	case ID_HELP_UNIT_LINK:
	case ID_HELP_WONDER_LINK:
	case ID_HELP_IMPROVEMENT_LINK:
	  {
	    char s[128];
	    GetWindowText((HWND)lParam,s,sizeof(s));
	    if (strcmp(s, _("(Never)")) != 0 && strcmp(s, _("None")) != 0
		&& strcmp(s, advance_name_translation(advance_by_number(A_NONE))) != 0)
	      select_help_item_string(s,page_type_from_id(LOWORD(wParam)));
	    
	  }
	  break;
	case ID_HELP_LIST:
	  {
	    if (HIWORD(wParam)==LBN_SELCHANGE)
	      {
		int row;
		const struct help_item *p = NULL;
		
		row=ListBox_GetCurSel(helpdlg_list);
		help_items_iterate(pitem) {
		  if ((row--)==0)
		    {
		      p=pitem;
		      break;
		    }
		}
		help_items_iterate_end;
		
		if (p)
		  help_update_dialog(p);
	      }
	  }
	  break;
	}
      break;
    default:
      return DefWindowProc(hWnd,uMsg,wParam,lParam);
      
    }
  return 0;
}

/**************************************************************************

**************************************************************************/
static void set_help_text(char *txt)
{
  char *newstr;
  if ((txt)&&(strlen(txt)))
    {
      newstr=convertnl2crnl(txt);
      SetWindowText(helpdlg_text,newstr);
      free(newstr);
    }
}



/**************************************************************************

**************************************************************************/
static void edit_minsize(POINT *pt,void *data)
{
  char buf[32768];
  HWND hwnd = (HWND) data;
  HFONT old;
  HFONT font;
  HDC hdc;
  RECT rc;
  old = NULL; /* just silence gcc */
  buf[0] = 0;
  GetWindowText(hwnd, buf, sizeof(buf));
  if (strlen(buf)<10) {
    pt->x = 300;
    pt->y = 300;
    return;
  }
  
  hdc = GetDC(hwnd);
  if ((font = GetWindowFont(hwnd))) {
    old = SelectObject(hdc, font);
  }
  rc.left = 0;
  rc.right = 10000;
  rc.top=0;
  DrawText(hdc, buf, strlen(buf), &rc, DT_CALCRECT);
  pt->x = rc.right - rc.left + 40;
  pt->y = 300;
  if (font) {
    SelectObject(hdc, old);
  }
  ReleaseDC(hwnd, hdc);
}

/**************************************************************************

**************************************************************************/
static void edit_setsize(LPRECT rc, void *data)
{
  HWND hWnd;
  hWnd=(HWND)data;
  MoveWindow(hWnd,rc->left,rc->top,rc->right-rc->left,rc->bottom-rc->top,TRUE);
}

/**************************************************************************

**************************************************************************/
static void create_improvement_page(struct fcwin_box *vbox)
{
  struct fcwin_box *hbox;
  int i;
  hbox=fcwin_hbox_new(helpdlg_win,FALSE);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  for (i=0;i<5;i++)
    {
      help_ilabel[i]=fcwin_box_add_static(hbox,
					     _(help_ilabel_name[i]),0,
					     SS_LEFT,TRUE,TRUE,5);
    }
  help_ilabel[5]=fcwin_box_add_button(hbox,"",ID_HELP_TECH_LINK,0,TRUE,TRUE,5);
}


/**************************************************************************

**************************************************************************/
static void create_wonder_page(struct fcwin_box *vbox)
{
  struct fcwin_box *hbox;
  int i;
  hbox=fcwin_hbox_new(helpdlg_win,FALSE);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  for(i=0;i<5;i++)
    {
      if (i==3)
	help_ilabel[3]=fcwin_box_add_button(hbox,"",
					    ID_HELP_TECH_LINK,
					    0,TRUE,TRUE,5);
      else
	help_ilabel[i]=fcwin_box_add_static(hbox,
					    _(help_wlabel_name[i]),0,
					    SS_LEFT,TRUE,TRUE,5);      
    }
  help_ilabel[5]=fcwin_box_add_button(hbox,"",ID_HELP_TECH_LINK,
				      0,TRUE,TRUE,5);
  
}

/*************************************************************************

**************************************************************************/
static void unit_minsize(POINT *min,void *data)
{
  min->x=tileset_full_tile_width(tileset);
  min->y=tileset_full_tile_height(tileset);
}

/*************************************************************************

**************************************************************************/
static void unit_setsize(RECT *rc,void *data)
{
  unitpos.x=rc->left;
  unitpos.y=rc->top;
  InvalidateRect(helpdlg_win,rc,TRUE);
}

/**************************************************************************

***************************************************************************/
static void create_unit_page(struct fcwin_box *vbox)
{
  int x,y;
  struct fcwin_box *hbox;
  struct fcwin_box *vbox_labels;
  fcwin_box_add_generic(vbox,unit_minsize,unit_setsize,NULL,NULL,FALSE,FALSE,5);
  hbox=fcwin_hbox_new(helpdlg_win,FALSE);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  for(x=0;x<5;x++) {
    vbox_labels=fcwin_vbox_new(helpdlg_win,FALSE);
    fcwin_box_add_box(hbox,vbox_labels,TRUE,TRUE,15);
    for(y=0;y<4;y++) {
      help_ulabel[y][x]=
	fcwin_box_add_static(vbox_labels, _(help_ulabel_name[y][x]), 0,
			     SS_LEFT, FALSE, FALSE, 0);
    }
    if ((x==1)||(x==4))
      help_ulabel[y][x]=
	fcwin_box_add_button(vbox_labels, _(help_ulabel_name[y][x]),
			     x==1?ID_HELP_TECH_LINK:ID_HELP_UNIT_LINK,
			     0,TRUE,TRUE,0);
    else
      help_ulabel[y][x]=
	fcwin_box_add_static(vbox_labels, _(help_ulabel_name[y][x]), 0,
			     SS_LEFT, TRUE, TRUE, 0);
  }
}

/**************************************************************************

**************************************************************************/
static void create_terrain_page(struct fcwin_box *vbox)
{
  int x,y;
  struct fcwin_box *hbox;
  struct fcwin_box *vbox_labels;  
  hbox=fcwin_hbox_new(helpdlg_win,FALSE);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  for(x=0;x<5;x++) {
    vbox_labels=fcwin_vbox_new(helpdlg_win,FALSE);
    fcwin_box_add_box(hbox,vbox_labels,TRUE,TRUE,15);
    for(y=0;y<4;y++) {
      help_tlabel[y][x]=
	fcwin_box_add_static(vbox_labels, _(help_tlabel_name[y][x]), 0,
			     SS_LEFT, FALSE, FALSE, 0);
    }
  }
}

/**************************************************************************

**************************************************************************/
static void create_help_page(enum help_page_type type)
{

  RECT rc;
  GetClientRect(helpdlg_win,&rc);
  InvalidateRect(helpdlg_win,&rc,TRUE);
  fcwin_box_freeitem(helpdlg_hbox,1);
  drawn_unit_type = NULL;
  helpdlg_page_vbox=fcwin_vbox_new(helpdlg_win,FALSE);
  helpdlg_topic=fcwin_box_add_static(helpdlg_page_vbox,
				     "",0,SS_LEFT,FALSE,FALSE,5);
  fcwin_box_add_box(helpdlg_hbox,helpdlg_page_vbox,TRUE,TRUE,5);
  switch(type) {
  case HELP_IMPROVEMENT:
    create_improvement_page(helpdlg_page_vbox);
    break;
  case HELP_WONDER:
    create_wonder_page(helpdlg_page_vbox);
    break;
  case HELP_UNIT:
    create_unit_page(helpdlg_page_vbox);
    break;
  case HELP_TERRAIN:
    create_terrain_page(helpdlg_page_vbox);
    break;
  default:
    break;
  }
  if (type!=HELP_TECH)
    fcwin_box_add_generic(helpdlg_page_vbox,edit_minsize,edit_setsize,NULL,
			  helpdlg_text,TRUE,TRUE,5);
  ShowWindow(helpdlg_text,type==HELP_TECH?SW_HIDE:SW_SHOWNORMAL);
}

/**************************************************************************

**************************************************************************/
void create_help_dialog()
{
  struct fcwin_box *vbox;
  drawn_unit_type = NULL;
  helpdlg_win=fcwin_create_layouted_window(HelpdlgWndProc,
					   _("Freeciv Help Browser"),
					   WS_OVERLAPPEDWINDOW,
					   CW_USEDEFAULT,CW_USEDEFAULT,
					   root_window,
					   NULL,
					   JUST_CLEANUP,
					   NULL);

  helpdlg_hbox=fcwin_hbox_new(helpdlg_win,FALSE);
  vbox=fcwin_vbox_new(helpdlg_win,FALSE);
  helpdlg_topic=fcwin_box_add_static(vbox,"",0,SS_LEFT,
				     FALSE,FALSE,5);
  helpdlg_text=CreateWindow("EDIT","",
			    WS_CHILD | WS_VISIBLE |  ES_READONLY | 
			    ES_WANTRETURN | 
			    ES_AUTOHSCROLL | ES_LEFT | WS_VSCROLL |
			    ES_AUTOVSCROLL | ES_MULTILINE | WS_BORDER,0,0,0,0,
			    helpdlg_win,
			    NULL,
			    freecivhinst,
			    NULL);
  fcwin_box_add_generic(vbox,edit_minsize,edit_setsize,NULL,
			helpdlg_text,TRUE,TRUE,5);
  helpdlg_list=fcwin_box_add_list(helpdlg_hbox,6,ID_HELP_LIST, 
		     LBS_NOTIFY | WS_VSCROLL,
		     FALSE,FALSE,5);
  fcwin_box_add_box(helpdlg_hbox,vbox,TRUE,TRUE,5);
  vbox=fcwin_vbox_new(helpdlg_win,FALSE);
  fcwin_box_add_box(vbox,helpdlg_hbox,TRUE,TRUE,5);
  helpdlg_close=fcwin_box_add_button(vbox,_("Close"),IDCANCEL,0,
		       FALSE,FALSE,5);
  help_items_iterate(pitem)
    ListBox_AddString(helpdlg_list,pitem->topic);
  help_items_iterate_end;
  fcwin_set_box(helpdlg_win,vbox);
  create_help_page(HELP_TEXT);
  
  fcwin_redo_layout(helpdlg_win);
  
}

/**************************************************************************
...
**************************************************************************/
static void help_update_improvement(const struct help_item *pitem,
                                    char *title)
{
  char buf[64000];
  struct impr_type *imp = find_improvement_by_translated_name(title);

  create_help_page(HELP_IMPROVEMENT);

  if (imp  &&  !is_great_wonder(imp)) {
    char req_buf[512];
    int i;

    sprintf(buf, "%d", impr_build_shield_cost(imp));
    SetWindowText(help_ilabel[1], buf);
    sprintf(buf, "%d", imp->upkeep);
    SetWindowText(help_ilabel[3], buf);

    /* FIXME: this should show ranges and all the MAX_NUM_REQS reqs. 
     * Currently it's limited to 1 req but this code is partially prepared
     * to be extended.  Remember MAX_NUM_REQS is a compile-time
     * definition. */
    i = 0;
    requirement_vector_iterate(&imp->reqs, preq) {
      SetWindowText(help_ilabel[5 + i],
		    universal_name_translation(&preq->source, req_buf,
		    sizeof(req_buf)));
      i++;
      break;
    } requirement_vector_iterate_end;
/*    create_tech_tree(help_improvement_tree, 0, imp->tech_req, 3);*/
  }
  else {
    SetWindowText(help_ilabel[1], "0");
    SetWindowText(help_ilabel[3], "0");
    SetWindowText(help_ilabel[5], _("(Never)"));
/*    create_tech_tree(help_improvement_tree, 0, advance_count(), 3);*/
  }
  helptext_building(buf, sizeof(buf), client.conn.playing, pitem->text, imp);
  set_help_text(buf);

}
/**************************************************************************
...
**************************************************************************/
static void help_update_wonder(const struct help_item *pitem,
                               char *title)
{
  char buf[64000];
  struct impr_type *imp = find_improvement_by_translated_name(title);

  create_help_page(HELP_WONDER);

  if (imp  &&  is_great_wonder(imp)) {
    char req_buf[512];
    int i;

    sprintf(buf, "%d", impr_build_shield_cost(imp));
    SetWindowText(help_ilabel[1], buf);

    /* FIXME: this should show ranges and all the MAX_NUM_REQS reqs. 
     * Currently it's limited to 1 req but this code is partially prepared
     * to be extended.  Remember MAX_NUM_REQS is a compile-time
     * definition. */
    i = 0;
    requirement_vector_iterate(&imp->reqs, preq) {
      SetWindowText(help_ilabel[3 + i],
		    universal_name_translation(&preq->source, req_buf,
		    sizeof(req_buf)));
      i++;
      break;
    } requirement_vector_iterate_end;
    if (valid_advance(imp->obsolete_by)) {
      SetWindowText(help_ilabel[5],
                    advance_name_for_player(client.conn.playing,
					    advance_number(imp->obsolete_by)));
    } else {
      SetWindowText(help_ilabel[5], _("None"));
    }

/*    create_tech_tree(help_improvement_tree, 0, imp->tech_req, 3);*/
  }
  else {
    /* can't find wonder */
    SetWindowText(help_ilabel[1], "0");
    SetWindowText(help_ilabel[3], _("(Never)"));
    SetWindowText(help_ilabel[5], _("None"));
/*    create_tech_tree(help_improvement_tree, 0, advance_count(), 3); */
  }
 
  helptext_building(buf, sizeof(buf), client.conn.playing, pitem->text, imp);
  set_help_text(buf);
}                            

/**************************************************************************
...
**************************************************************************/
static void help_update_terrain(const struct help_item *pitem,
				char *title)
{
  char buf[64000];
  struct terrain *pterrain = find_terrain_by_translated_name(title);

  create_help_page(HELP_TERRAIN);

  if (pterrain) {
    sprintf(buf, "%d/%d.%d",
	    pterrain->movement_cost,
	    (int)((pterrain->defense_bonus + 100) / 100),
	    (pterrain->defense_bonus + 100) % 100 / 10);
    SetWindowText (help_tlabel[0][1], buf);

    sprintf(buf, "%d/%d/%d",
	    pterrain->output[O_FOOD],
	    pterrain->output[O_SHIELD],
	    pterrain->output[O_TRADE]);
    SetWindowText(help_tlabel[0][4], buf);

    buf[0] = '\0';
    if (*(pterrain->resources)) {
      struct resource **r;

      for (r = pterrain->resources; *r; r++) {
        sprintf (buf + strlen (buf), " %s,", resource_name_translation(*r));
      }
      buf[strlen (buf) - 1] = '.';
    } else {
      /* TRANS: "Resources: (none)" */
      sprintf (buf + strlen (buf), _("(none)"));
    }
    SetWindowText(help_tlabel[1][1], buf);

    if (pterrain->road_trade_incr > 0) {
      sprintf(buf, _("+%d Trade / %d"),
	      pterrain->road_trade_incr,
	      pterrain->road_time);
    } else if (pterrain->road_time > 0) {
      sprintf(buf, _("no extra / %d"),
	      pterrain->road_time);
    } else {
      strcpy(buf, _("n/a"));
    }
    SetWindowText(help_tlabel[2][1], buf);

    strcpy(buf, _("n/a"));
    if (pterrain->irrigation_result == pterrain) {
      if (pterrain->irrigation_food_incr > 0) {
	sprintf(buf, _("+%d Food / %d"),
		pterrain->irrigation_food_incr,
		pterrain->irrigation_time);
      }
    } else if (pterrain->irrigation_result != T_NONE) {
      sprintf(buf, "%s / %d",
	      terrain_name_translation(pterrain->irrigation_result),
	      pterrain->irrigation_time);
    }
    SetWindowText(help_tlabel[2][4], buf);

    strcpy(buf, _("n/a"));
    if (pterrain->mining_result == pterrain) {
      if (pterrain->mining_shield_incr > 0) {
	sprintf(buf, _("+%d Res. / %d"),
		pterrain->mining_shield_incr,
		pterrain->mining_time);
      }
    } else if (pterrain->mining_result != T_NONE) {
      sprintf(buf, "%s / %d",
	      terrain_name_translation(pterrain->mining_result),
	      pterrain->mining_time);
    }
    SetWindowText(help_tlabel[3][1], buf);

    if (pterrain->transform_result != T_NONE) {
      sprintf(buf, "%s / %d",
	      terrain_name_translation(pterrain->transform_result),
	      pterrain->transform_time);
    } else {
      strcpy(buf, _("n/a"));
    }
    SetWindowText(help_tlabel[3][4], buf);
  }

  helptext_terrain(buf, sizeof(buf), client.conn.playing, pitem->text, pterrain);
  set_help_text(buf);
}

/*************************************************************************

*************************************************************************/
static void help_draw_unit(HDC hdc, struct unit_type *utype)
{
  enum color_std bg_color;
  RECT rc;
  HBRUSH brush;
  rc.top=unitpos.y;
  rc.left=unitpos.x;
  rc.bottom=unitpos.y+tileset_full_tile_height(tileset);
  rc.right=unitpos.x+tileset_full_tile_width(tileset);
  
  /* Give tile a background color, based on the type of unit
   * FIXME: make a new set of colors for this.               */
  switch (unit_color_type(utype)) {
  case UNIT_BG_LAND:
    bg_color = COLOR_OVERVIEW_LAND;
    break;
  case UNIT_BG_SEA:
    bg_color = COLOR_OVERVIEW_OCEAN;
    break;
  case UNIT_BG_HP_LOSS:
  case UNIT_BG_AMPHIBIOUS:
    bg_color = COLOR_OVERVIEW_MY_UNIT;
    break;
  case UNIT_BG_FLYING:
    bg_color = COLOR_OVERVIEW_ENEMY_CITY;
    break;
  default:
    bg_color = COLOR_OVERVIEW_UNKNOWN;
    break;
  }

  brush = brush_alloc(get_color(tileset, bg_color));

  FillRect(hdc, &rc, brush);

  brush_free(brush);
  
  /* Put a picture of the unit in the tile */
  if (utype) {
    struct sprite *sprite = get_unittype_sprite(tileset, utype);
    draw_sprite(sprite, hdc, unitpos.x, unitpos.y);
  }
  
}

/**************************************************************************

**************************************************************************/
static void help_update_unit_type(const struct help_item *pitem,
				  char *title)
{
  char buf[64000];
  struct unit_type *utype = find_unit_type_by_translated_name(title);

  create_help_page(HELP_UNIT);

  drawn_unit_type = utype;

  if (utype) {
    sprintf(buf, "%d", utype_build_shield_cost(utype));
    SetWindowText(help_ulabel[0][1], buf);
    sprintf(buf, "%d", utype->attack_strength);
    SetWindowText(help_ulabel[0][4], buf);
    sprintf(buf, "%d", utype->defense_strength);
    SetWindowText(help_ulabel[1][1], buf);
    sprintf(buf, "%d", utype->move_rate/3);
    SetWindowText(help_ulabel[1][4], buf);
    sprintf(buf, "%d", utype->firepower);
    SetWindowText(help_ulabel[2][1], buf);
    sprintf(buf, "%d", utype->hp);
    SetWindowText(help_ulabel[2][4], buf);
    SetWindowText(help_ulabel[3][1], helptext_unit_upkeep_str(utype));
    sprintf(buf, "%d", utype->vision_radius_sq);
    SetWindowText(help_ulabel[3][4], buf);
    if (A_NEVER == utype->require_advance) {
      SetWindowText(help_ulabel[4][1], _("(Never)"));
    } else {
      SetWindowText(help_ulabel[4][1],
                    advance_name_for_player(client.conn.playing,
				       advance_number(utype->require_advance)));
    }
    /*    create_tech_tree(help_improvement_tree, 0, advance_number(utype->require_advance), 3);*/
    if (U_NOT_OBSOLETED == utype->obsoleted_by) {
      SetWindowText(help_ulabel[4][4], _("None"));
    } else {
      SetWindowText(help_ulabel[4][4],
                    utype_name_translation(utype->obsoleted_by));
    }

    helptext_unit(buf, sizeof(buf), client.conn.playing, pitem->text, utype);
    set_help_text(buf);
  }
  else {
    SetWindowText(help_ulabel[0][1], "0");
    SetWindowText(help_ulabel[0][4], "0");
    SetWindowText(help_ulabel[1][1], "0");
    SetWindowText(help_ulabel[1][4], "0");
    SetWindowText(help_ulabel[2][1], "0");
    SetWindowText(help_ulabel[2][4], "0");
    SetWindowText(help_ulabel[3][1], "0");
    SetWindowText(help_ulabel[3][4], "0");

    SetWindowText(help_ulabel[4][1], _("(Never)"));
/*    create_tech_tree(help_improvement_tree, 0, A_LAST, 3);*/
    SetWindowText(help_ulabel[4][4], _("None"));
    set_help_text(pitem->text);
  }
}

/**************************************************************************

**************************************************************************/
static void help_update_tech(const struct help_item *pitem, char *title)
{
  struct fcwin_box *hbox;
  char buf[64000];
  int i;
  struct advance *padvance = find_advance_by_translated_name(title);

  create_help_page(HELP_TECH);

  if (padvance  &&  !is_future_tech(i = advance_number(padvance))) {
    /*    
	  create_tech_tree(GTK_CTREE(help_tree), i, TECH_TREE_DEPTH,
	  TECH_TREE_EXPANDED_DEPTH, NULL);
    */
    helptext_advance(buf, sizeof(buf), client.conn.playing, pitem->text, i);
    wordwrap_string(buf, 68);
    fcwin_box_add_static(helpdlg_page_vbox,buf,0,SS_LEFT,FALSE,FALSE,5);

    improvement_iterate(pimprove) {
      /* FIXME: need a more general mechanism for this, since this
       * helptext needs to be shown in all possible req source types. */
      requirement_vector_iterate(&pimprove->reqs, req) {
	if (VUT_NONE == req->source.kind) {
	  break;
	}
	if (VUT_IMPROVEMENT == req->source.kind
	 && req->source.value.building == pimprove) {
	  hbox = fcwin_hbox_new(helpdlg_win, FALSE);
	  fcwin_box_add_box(helpdlg_page_vbox, hbox, FALSE, FALSE, 5);
	  fcwin_box_add_static(hbox, _("Allows "), 0, SS_LEFT, FALSE, FALSE,
			       5);
	  fcwin_box_add_button(hbox,
			       improvement_name_translation(pimprove),
			       is_great_wonder(pimprove)
			       ? ID_HELP_WONDER_LINK
			       : ID_HELP_IMPROVEMENT_LINK,
			       0, FALSE, FALSE, 5);
	}
      } requirement_vector_iterate_end;
      if (padvance == pimprove->obsolete_by) {
	hbox=fcwin_hbox_new(helpdlg_win,FALSE);
	fcwin_box_add_box(helpdlg_page_vbox,hbox,FALSE,FALSE,5);
	fcwin_box_add_static(hbox,_("Obsoletes "),0,SS_LEFT,FALSE,FALSE,5);
	fcwin_box_add_button(hbox,
			     improvement_name_translation(pimprove),
			     is_great_wonder(pimprove)
			     ? ID_HELP_WONDER_LINK
			     : ID_HELP_IMPROVEMENT_LINK,
			     0, FALSE, FALSE, 5);
      }
    } improvement_iterate_end;

    unit_type_iterate(punittype) {
      if (padvance != punittype->require_advance) {
	continue;
      }
      hbox=fcwin_hbox_new(helpdlg_win,FALSE);
      fcwin_box_add_box(helpdlg_page_vbox,hbox,FALSE,FALSE,5);
      fcwin_box_add_static(hbox,_("Allows "),0,SS_LEFT,FALSE,FALSE,5);
      fcwin_box_add_button(hbox,
			   utype_name_translation(punittype),
			   ID_HELP_UNIT_LINK,
			   0,FALSE,FALSE,5);
    } unit_type_iterate_end;

    advance_iterate(A_NONE, ptest) {
      if (padvance == advance_requires(ptest, AR_ONE)) {
	if (advance_by_number(A_NONE) == advance_requires(ptest, AR_TWO)) {
	  hbox=fcwin_hbox_new(helpdlg_win,FALSE);
	  fcwin_box_add_box(helpdlg_page_vbox,hbox,FALSE,FALSE,5);
	  fcwin_box_add_static(hbox,_("Allows "),0,SS_LEFT,FALSE,FALSE,5);
	  fcwin_box_add_button(hbox,advance_name_translation(ptest),
			       ID_HELP_TECH_LINK,0,FALSE,FALSE,5);
	} else {
	  hbox=fcwin_hbox_new(helpdlg_win,FALSE);
	  fcwin_box_add_box(helpdlg_page_vbox,hbox,FALSE,FALSE,5);
	  fcwin_box_add_static(hbox,_("Allows "),0,SS_LEFT,FALSE,FALSE,5);
	  fcwin_box_add_button(hbox,advance_name_translation(ptest),
			       ID_HELP_TECH_LINK,0,FALSE,FALSE,5);
	  fcwin_box_add_static(hbox,_(" (with "),0,SS_LEFT,FALSE,FALSE,5);
	  fcwin_box_add_button(hbox,advance_name_translation(advance_requires(ptest, AR_TWO)),
			       ID_HELP_TECH_LINK,0,FALSE,FALSE,5);
	  fcwin_box_add_static(hbox,Q_("?techhelp:)."),
			       0,SS_LEFT,FALSE,FALSE,5);
	}
      }
      if (padvance == advance_requires(ptest, AR_TWO)) {
	hbox=fcwin_hbox_new(helpdlg_win,FALSE);
	fcwin_box_add_box(helpdlg_page_vbox,hbox,FALSE,FALSE,5);
	fcwin_box_add_static(hbox,_("Allows "),0,SS_LEFT,FALSE,FALSE,5);
	fcwin_box_add_button(hbox,advance_name_translation(ptest),
			     ID_HELP_TECH_LINK,0,FALSE,FALSE,5);
	fcwin_box_add_static(hbox,_(" (with "),0,SS_LEFT,FALSE,FALSE,5);
	fcwin_box_add_button(hbox,advance_name_translation(advance_requires(ptest, AR_ONE)),
			     ID_HELP_TECH_LINK,0,FALSE,FALSE,5);
	fcwin_box_add_static(hbox,Q_("?techhelp:)."),
			     0,SS_LEFT,FALSE,FALSE,5);
      }
    } advance_iterate_end;
  }
}

/**************************************************************************
  This is currently just a text page, with special text:
**************************************************************************/
static void help_update_government(const struct help_item *pitem,
                                   char *title)
{
  char buf[64000];
  struct government *gov = find_government_by_translated_name(title);

  if (!gov) {
    strcat(buf, pitem->text);
  } else {
    helptext_government(buf, sizeof(buf), client.conn.playing, pitem->text, gov);
  }
  create_help_page(HELP_TEXT);
  set_help_text(buf);
}
              
/**************************************************************************
...
**************************************************************************/
static void help_update_dialog(const struct help_item *pitem)
{
  char *top;
  /* figure out what kind of item is required for pitem ingo */

  for (top = pitem->topic; *top == ' '; top++) {
    /* nothing */
  }
  SetWindowText(helpdlg_text,"");

  switch(pitem->type) {
  case HELP_IMPROVEMENT:
    help_update_improvement(pitem, top);
    break;
  case HELP_WONDER:
    help_update_wonder(pitem, top);
    break;
  case HELP_UNIT:
    help_update_unit_type(pitem, top);
    break;
  case HELP_TECH:
    help_update_tech(pitem, top);
    break;
  case HELP_TERRAIN:
    help_update_terrain(pitem, top);
    break;
  case HELP_GOVERNMENT:
    help_update_government(pitem, top);
    break;
  case HELP_TEXT:
  default:
    create_help_page(HELP_TEXT);
    set_help_text(pitem->text);
    break;
  }
  SetWindowText(helpdlg_topic,pitem->topic);
  fcwin_redo_layout(helpdlg_win);
  /* set_title_topic(pitem->topic); */
  ShowWindow(helpdlg_win,SW_SHOWNORMAL);
}
/**************************************************************************
...
**************************************************************************/
static void select_help_item(int item)
{
  
  ListBox_SetTopIndex(helpdlg_list,item);
  ListBox_SetCurSel(helpdlg_list,item);
}

/****************************************************************
...
*****************************************************************/
static void select_help_item_string(const char *item,
                                    enum help_page_type htype)
{
  const struct help_item *pitem;
  int idx;

  pitem = get_help_item_spec(item, htype, &idx);
  if(idx==-1) idx = 0;
  select_help_item(idx);
  help_update_dialog(pitem);
}

/**************************************************************************

**************************************************************************/
void
popup_help_dialog_string(const char *item)
{
  popup_help_dialog_typed(_(item), HELP_ANY); 
}

/**************************************************************************

**************************************************************************/
void
popup_help_dialog_typed(const char *item, enum help_page_type htype)
{
  if (!helpdlg_win)
    create_help_dialog();
  select_help_item_string(item,htype);
}

/**************************************************************************

**************************************************************************/
void
popdown_help_dialog(void)
{
  DestroyWindow(helpdlg_win);
}
