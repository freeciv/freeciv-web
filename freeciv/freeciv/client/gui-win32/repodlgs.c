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
#include <commctrl.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

/* common */
#include "government.h"
#include "packets.h"
#include "unitlist.h"

/* client */
#include "chatline_common.h"	/* send_chat() */ 
#include "cityrep.h"
#include "client_main.h"
#include "climisc.h"
#include "dialogs.h"
#include "gui_stuff.h"
#include "gui_main.h"
#include "helpdlg.h"
#include "optiondlg.h"    
#include "repodlgs_common.h"
#include "repodlgs.h"
#include "text.h"

static HWND economy_dlg;
static HWND activeunits_dlg;
static HWND science_dlg;

static HWND settable_options_dialog_win;
static HWND tab_ctrl;
static HWND *tab_wnds;
static int num_tabs;
static int *categories;

extern HINSTANCE freecivhinst;
extern HWND root_window;

struct impr_type *economy_improvement_type[B_LAST];
struct unit_type *activeunits_type[U_LAST];

#define ID_OPTIONS_BASE 1000

/**************************************************************************

**************************************************************************/
void
update_report_dialogs(void)
{
  if(is_report_dialogs_frozen()) return;
  activeunits_report_dialog_update();
  economy_report_dialog_update();
  city_report_dialog_update();
  science_dialog_update();    
}

/**************************************************************************

**************************************************************************/
void
science_dialog_update(void)
{
 
  char text[512];

  Tech_type_id tech_id;
  int hist, id, steps;

  if (!science_dlg) return;            
  /* TRANS: Research report title */
  sz_strlcpy(text, get_report_title(_("Research")));
  sz_strlcat(text, science_dialog_text());

  SetWindowText(GetDlgItem(science_dlg, ID_SCIENCE_TOP), text);
  ListBox_ResetContent(GetDlgItem(science_dlg, ID_SCIENCE_LIST));

  advance_index_iterate(A_FIRST, tech_id) {
    if (TECH_KNOWN == player_invention_state(client.conn.playing, tech_id)) {
      id = ListBox_AddString(GetDlgItem(science_dlg, ID_SCIENCE_LIST),
			     advance_name_for_player(client.conn.playing, tech_id));
      ListBox_SetItemData(GetDlgItem(science_dlg,ID_SCIENCE_LIST), id,
			  tech_id);
    }
  } advance_index_iterate_end;

  ComboBox_ResetContent(GetDlgItem(science_dlg, ID_SCIENCE_RESEARCH));

  if (A_UNSET == get_player_research(client.conn.playing)->researching) {
    id = ComboBox_AddString(GetDlgItem(science_dlg, ID_SCIENCE_RESEARCH),
			    advance_name_for_player(client.conn.playing, A_NONE));
    ComboBox_SetItemData(GetDlgItem(science_dlg, ID_SCIENCE_RESEARCH),
			 id, A_NONE);
    ComboBox_SetCurSel(GetDlgItem(science_dlg, ID_SCIENCE_RESEARCH),
		       id);
    text[0] = '\0';
  } else {
    my_snprintf(text, sizeof(text), "%d/%d",
		get_player_research(client.conn.playing)->bulbs_researched,
		total_bulbs_required(client.conn.playing));
  }

  SetWindowText(GetDlgItem(science_dlg, ID_SCIENCE_PROG), text);

  if (!is_future_tech(get_player_research(client.conn.playing)->researching)) {
    advance_index_iterate(A_FIRST, tech_id) {
      if (TECH_PREREQS_KNOWN !=
            player_invention_state(client.conn.playing, tech_id)) {
	continue;
      }

      id = ComboBox_AddString(GetDlgItem(science_dlg, ID_SCIENCE_RESEARCH),
			      advance_name_for_player(client.conn.playing, tech_id));
      ComboBox_SetItemData(GetDlgItem(science_dlg, ID_SCIENCE_RESEARCH),
			   id, tech_id);
      if (tech_id == get_player_research(client.conn.playing)->researching) {
	ComboBox_SetCurSel(GetDlgItem(science_dlg, ID_SCIENCE_RESEARCH),
			   id);
      }
    } advance_index_iterate_end;
  } else {
      tech_id = advance_count() + 1
		+ get_player_research(client.conn.playing)->future_tech;
      id = ComboBox_AddString(GetDlgItem(science_dlg, ID_SCIENCE_RESEARCH),
			      advance_name_for_player(client.conn.playing, tech_id));
      ComboBox_SetItemData(GetDlgItem(science_dlg, ID_SCIENCE_RESEARCH),
			   id, tech_id);
      ComboBox_SetCurSel(GetDlgItem(science_dlg, ID_SCIENCE_RESEARCH),
			 id);
  }
  ComboBox_ResetContent(GetDlgItem(science_dlg,ID_SCIENCE_GOAL));
    hist=0;
  advance_index_iterate(A_FIRST, tech_id) {
    if (player_invention_reachable(client.conn.playing, tech_id)
        && TECH_KNOWN != player_invention_state(client.conn.playing, tech_id)
        && (11 > num_unknown_techs_for_goal(client.conn.playing, tech_id)
	    || tech_id == get_player_research(client.conn.playing)->tech_goal)) {
      id = ComboBox_AddString(GetDlgItem(science_dlg,ID_SCIENCE_GOAL),
			      advance_name_for_player(client.conn.playing, tech_id));
       ComboBox_SetItemData(GetDlgItem(science_dlg,ID_SCIENCE_GOAL),
			 id, tech_id);
      if (tech_id == get_player_research(client.conn.playing)->tech_goal)
 	ComboBox_SetCurSel(GetDlgItem(science_dlg,ID_SCIENCE_GOAL),
 			   id);
       
     }
  } advance_index_iterate_end;

  if (A_UNSET == get_player_research(client.conn.playing)->tech_goal) {
    id = ComboBox_AddString(GetDlgItem(science_dlg, ID_SCIENCE_GOAL),
			    advance_name_for_player(client.conn.playing, A_NONE));
    ComboBox_SetItemData(GetDlgItem(science_dlg, ID_SCIENCE_GOAL),
			 id, A_NONE);
    ComboBox_SetCurSel(GetDlgItem(science_dlg, ID_SCIENCE_GOAL),
		       id);
   }

  steps = num_unknown_techs_for_goal(client.conn.playing,
                            get_player_research(client.conn.playing)->tech_goal);
  my_snprintf(text, sizeof(text),
	      PL_("(%d step)", "(%d steps)", steps), steps);
  SetWindowText(GetDlgItem(science_dlg,ID_SCIENCE_STEPS),text);
  fcwin_redo_layout(science_dlg);
}

/**************************************************************************

**************************************************************************/
static LONG CALLBACK science_proc(HWND hWnd,
				  UINT message,
				  WPARAM wParam,
				  LPARAM lParam)
{
  int to, steps;
  switch(message)
    {
    case WM_CREATE:
      break;
    case WM_COMMAND:
      switch LOWORD(wParam)
	{
	case ID_SCIENCE_CLOSE:
	  DestroyWindow(science_dlg);
	  science_dlg=NULL;
	  break;
	case ID_SCIENCE_RESEARCH:
	  if (HIWORD(wParam)==CBN_SELCHANGE) {
	    to=ComboBox_GetCurSel(GetDlgItem(hWnd,ID_SCIENCE_RESEARCH));
	    if (to!=LB_ERR) {
	      to = ComboBox_GetItemData(GetDlgItem(hWnd,
						   ID_SCIENCE_RESEARCH),
					to);
	      
	      if (IsDlgButtonChecked(hWnd, ID_SCIENCE_HELP)) {
		popup_help_dialog_typed(advance_name_translation(advance_by_number(to)),
					HELP_TECH);
		science_dialog_update();
	      } else {
		dsend_packet_player_research(&client.conn, to);
	      }
	    }
	  }
	  break;
	case ID_SCIENCE_GOAL:
	  if (HIWORD(wParam)==CBN_SELCHANGE) {
	    to=ComboBox_GetCurSel(GetDlgItem(hWnd,ID_SCIENCE_GOAL));
	    if (to!=LB_ERR) {
	      char text[512];

	      to = ComboBox_GetItemData(GetDlgItem(hWnd, ID_SCIENCE_GOAL),
					to);
	      steps = num_unknown_techs_for_goal(client.conn.playing, to);
	      my_snprintf(text, sizeof(text), 
	                  PL_("(%d step)", "(%d steps)", steps),
			  steps);
	      SetWindowText(GetDlgItem(hWnd,ID_SCIENCE_STEPS), text);
	      dsend_packet_player_tech_goal(&client.conn, to);
	    }
	  }
	  break;
	}
      break;
    case WM_CLOSE:
      DestroyWindow(science_dlg);
      break;
    case WM_DESTROY:
      science_dlg=NULL;
      break;
    case WM_SIZE:
      break;
    case WM_GETMINMAXINFO:
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
    }
 return 0; 
}

/**************************************************************************

**************************************************************************/
void
popup_science_dialog(bool raise)
{
  if (!science_dlg)
    {
      struct fcwin_box *vbox;
      struct fcwin_box *hbox;
      science_dlg=fcwin_create_layouted_window(science_proc,
					       /* TRANS: Research report title */
					       _("Research"),
					       WS_OVERLAPPEDWINDOW,
					       CW_USEDEFAULT,CW_USEDEFAULT,
					       root_window,
					       NULL,
					       JUST_CLEANUP,
					       NULL);
      vbox=fcwin_vbox_new(science_dlg,FALSE);
      fcwin_box_add_static(vbox,"",ID_SCIENCE_TOP,SS_CENTER,FALSE,FALSE,15);
      hbox=fcwin_hbox_new(science_dlg,FALSE);
      fcwin_box_add_groupbox(vbox,_("Researching"),hbox,0,FALSE,FALSE,5);
     
      fcwin_box_add_combo(hbox,10,ID_SCIENCE_RESEARCH,
			  CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL,
			  FALSE,FALSE,5);
      fcwin_box_add_static(hbox,"",ID_SCIENCE_PROG,SS_CENTER,TRUE,TRUE,25);
      fcwin_box_add_checkbox(hbox,_("Help"),ID_SCIENCE_HELP,0,
			     FALSE,FALSE,10);
      
      hbox=fcwin_hbox_new(science_dlg,FALSE);
      fcwin_box_add_groupbox(vbox,_("Goal"),hbox,0,FALSE,FALSE,5);
      fcwin_box_add_combo(hbox,10,ID_SCIENCE_GOAL,
			  CBS_DROPDOWNLIST | CBS_SORT | WS_VSCROLL,
			  TRUE,TRUE,15);
      fcwin_box_add_static(hbox,"",ID_SCIENCE_STEPS,SS_CENTER,TRUE,TRUE,15);
 
      fcwin_box_add_list(vbox,15,ID_SCIENCE_LIST,
			 LBS_HASSTRINGS | LBS_STANDARD | 
			 WS_VSCROLL | LBS_NOSEL,
			 TRUE,TRUE,5);
      fcwin_box_add_button(vbox,_("Close"),ID_SCIENCE_CLOSE,0,FALSE,FALSE,5);
      fcwin_set_box(science_dlg,vbox);
    }
  science_dialog_update();
  ShowWindow(science_dlg,SW_SHOWNORMAL);
  if (raise) {
    SetFocus(science_dlg);
  }
}
/**************************************************************************

**************************************************************************/

void
economy_report_dialog_update(void)
{
   
  HWND lv;
  int tax, total, i, entries_used;
  char   buf0 [64];
  char   buf1 [64];
  char   buf2 [64];
  char   buf3 [64];     
  char *row[4];   
  char economy_total[48];
  struct improvement_entry entries[B_LAST];

  if(is_report_dialogs_frozen()) return;      
  if(!economy_dlg) return;
  lv=GetDlgItem(economy_dlg,ID_TRADEREP_LIST);
  SetWindowText(GetDlgItem(economy_dlg, ID_TRADEREP_TOP),
		get_report_title(_("Economy")));
  ListView_DeleteAllItems(lv);
  row[0] = buf0;
  row[1] = buf1;
  row[2] = buf2;
  row[3] = buf3;

  get_economy_report_data(entries, &entries_used, &total, &tax);

  for (i = 0; i < entries_used; i++) {
    struct improvement_entry *p = &entries[i];

    my_snprintf(buf0, sizeof(buf0), "%s", improvement_name_translation(p->type));
    my_snprintf(buf1, sizeof(buf1), "%5d", p->count);
    my_snprintf(buf2, sizeof(buf2), "%5d", p->cost);
    my_snprintf(buf3, sizeof(buf3), "%6d", p->total_cost);

    fcwin_listview_add_row(lv, i, 4, row);

    economy_improvement_type[i] = p->type;
  }
  my_snprintf(economy_total, sizeof(economy_total),
	      _("Income:%6d    Total Costs: %6d"), tax, total);
  SetWindowText(GetDlgItem(economy_dlg,ID_TRADEREP_CASH),economy_total);
  ListView_SetColumnWidth(lv,0,LVSCW_AUTOSIZE);
  for(i=1;i<4;i++) {
    ListView_SetColumnWidth(lv,i,
			    LVSCW_AUTOSIZE_USEHEADER);	
  }
  fcwin_redo_layout(economy_dlg);
}
  
/**************************************************************************

**************************************************************************/
static void economy_dlg_sell(HWND hWnd,int data)
{
  HWND lv = GetDlgItem(hWnd, ID_TRADEREP_LIST);
  char str[1024];
  int row, n = ListView_GetItemCount(lv);

  for (row = 0; row < n; row++) {
    if (ListView_GetItemState(lv,row,LVIS_SELECTED)) {
      sell_all_improvements(economy_improvement_type[row],
			    data == 0, str, sizeof(str));
      ListView_SetItemState(lv,row,0,LVIS_SELECTED);
      popup_notify_dialog(_("Sell-Off:"),_("Results"),str);
    }
  }
}

/**************************************************************************

**************************************************************************/
static LONG CALLBACK economy_proc(HWND hWnd,
				UINT message,
				WPARAM wParam,
				LPARAM lParam) 
{

  switch(message)
    {
  
    case WM_SIZE:
    case WM_GETMINMAXINFO:
    case WM_DESTROY:
      break;
    case WM_CLOSE:
      DestroyWindow(hWnd);
      economy_dlg=NULL;
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
	{
	case ID_TRADEREP_CLOSE:
	  DestroyWindow(hWnd);
	  economy_dlg=NULL;
	  break;
	case ID_TRADEREP_ALL:
	  economy_dlg_sell(hWnd,1);
	  break;
	case ID_TRADEREP_OBSOLETE:
	  economy_dlg_sell(hWnd,0);
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
popup_economy_report_dialog(bool raise)
{
  int i;
  struct fcwin_box *hbox;
  struct fcwin_box *vbox;
  if (!economy_dlg) {
    HWND lv;
    LV_COLUMN lvc;
    economy_dlg=fcwin_create_layouted_window(economy_proc,_("Trade Report"),
					     WS_OVERLAPPEDWINDOW,
					     CW_USEDEFAULT,CW_USEDEFAULT,
					     root_window,
					     NULL,
					     JUST_CLEANUP,
					     NULL);
    vbox=fcwin_vbox_new(economy_dlg,FALSE);
    hbox=fcwin_hbox_new(economy_dlg,TRUE);
    fcwin_box_add_static(vbox,"",ID_TRADEREP_TOP,SS_CENTER,
			  FALSE,FALSE,10);
    lv=fcwin_box_add_listview(vbox,10,ID_TRADEREP_LIST,
			      LVS_REPORT,TRUE,TRUE,10);
    fcwin_box_add_static(vbox,"",ID_TRADEREP_CASH,SS_CENTER,FALSE,FALSE,15);
    fcwin_box_add_button(hbox,_("Close"),ID_TRADEREP_CLOSE,
			 0,TRUE,TRUE,20);
    fcwin_box_add_button(hbox,_("Sell Obsolete"),ID_TRADEREP_OBSOLETE,
			 0,TRUE,TRUE,20);
    fcwin_box_add_button(hbox,_("Sell All"),ID_TRADEREP_ALL,
			 0,TRUE,TRUE,20);
    fcwin_box_add_box(vbox,hbox,FALSE,FALSE,10);
    lvc.pszText=_("Building Name");
    lvc.mask=LVCF_TEXT;
    ListView_InsertColumn(lv,0,&lvc);
    lvc.pszText=_("Count");
    lvc.mask=LVCF_TEXT | LVCF_FMT;
    lvc.fmt=LVCFMT_RIGHT;
    ListView_InsertColumn(lv,1,&lvc);
    lvc.pszText=_("Cost");
    ListView_InsertColumn(lv,2,&lvc);
    lvc.pszText=_("U Total");
    ListView_InsertColumn(lv,3,&lvc);
    ListView_SetColumnWidth(lv,0,LVSCW_AUTOSIZE);
    for(i=1;i<4;i++) {
      ListView_SetColumnWidth(lv,i,LVSCW_AUTOSIZE_USEHEADER);	
    }
    fcwin_set_box(economy_dlg,vbox);
  }
  economy_report_dialog_update();
  
  ShowWindow(economy_dlg,SW_SHOWNORMAL);
  if (raise) {
    SetFocus(economy_dlg);
  }
}
/****************************************************************
...
*****************************************************************/
static void upgrade_callback_yes(HWND w, void * data)
{
  dsend_packet_unit_type_upgrade(&client.conn, (size_t)data);
  destroy_message_dialog(w);
}
 
/****************************************************************
...
*****************************************************************/
static void upgrade_callback_no(HWND w, void * data)
{
  destroy_message_dialog(w);
}
 
/****************************************************************
...
*****************************************************************/              
static LONG CALLBACK activeunits_proc(HWND hWnd,
				      UINT message,
				      WPARAM wParam,
				      LPARAM lParam) 
{
  HWND lv;
  int n,sel,i;
  sel=-1;
  if ((message==WM_COMMAND)||(message==WM_NOTIFY)) {
    /* without totals line */
    lv=GetDlgItem(hWnd,ID_MILITARY_LIST);
    n=ListView_GetItemCount(lv)-1;
    for(i=0;i<n;i++) {
      if (ListView_GetItemState(lv,i,LVIS_SELECTED)) {
	sel=i;
	break;
      }
    } 
  }
  switch(message)
    {
    case WM_CREATE:
    case WM_SIZE:
    case WM_GETMINMAXINFO:
    case WM_DESTROY:
      break;
    case WM_CLOSE:
      DestroyWindow(activeunits_dlg);
      activeunits_dlg=NULL;
      return TRUE;
      break;
      
    case WM_NOTIFY:
      if (sel>=0) {
	CHECK_UNIT_TYPE(activeunits_type[sel]);
	if (can_upgrade_unittype(client.conn.playing,
				 activeunits_type[sel]) != NULL) {
	  EnableWindow(GetDlgItem(activeunits_dlg,ID_MILITARY_UPGRADE),
		       TRUE);
	} else {
	  EnableWindow(GetDlgItem(activeunits_dlg,ID_MILITARY_UPGRADE),
		       FALSE);
	}
      }
      break;
    case WM_COMMAND:    
      switch(LOWORD(wParam))
	{
	case IDCANCEL:
	  DestroyWindow(activeunits_dlg);
	  activeunits_dlg=NULL;
	  break;
	case ID_MILITARY_REFRESH:
	  activeunits_report_dialog_update();           
	  break;
	case ID_MILITARY_UPGRADE:
	  if (sel>=0)
	    {
	      char buf[512];
	      struct unit_type *ut1, *ut2;     

	      ut1 = activeunits_type[sel];
	      CHECK_UNIT_TYPE(ut1);
	      ut2 = can_upgrade_unittype(client.conn.playing, activeunits_type[sel]);
	      my_snprintf(buf, sizeof(buf),
			  _("Upgrade as many %s to %s as possible for %d gold each?\n"
			    "Treasury contains %d gold."),
			  utype_name_translation(ut1),
			  utype_name_translation(ut2),
			  unit_upgrade_price(client.conn.playing, ut1, ut2),
			  client.conn.playing->economic.gold);

	      popup_message_dialog(NULL, 
				   /*"upgradedialog"*/
				   _("Upgrade Obsolete Units"), buf,
				   _("Yes"), upgrade_callback_yes,
				   (void *)(activeunits_type[sel]),
				   _("No"), upgrade_callback_no, 0, 0);
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
#define AU_COL 6
void
activeunits_report_dialog_update(void)
{
  struct repoinfo {
    int active_count;
    int upkeep[O_LAST];
    int building_count;
  };

  if(activeunits_dlg) {
    HWND lv;
    int    i, k, can;

    struct repoinfo unitarray[U_LAST];
    struct repoinfo unittotals;

    char *row[AU_COL];
    char   buf[AU_COL][64];

    SetWindowText(GetDlgItem(activeunits_dlg, ID_MILITARY_TOP),
		  get_report_title(_("Units")));
    lv=GetDlgItem(activeunits_dlg,ID_MILITARY_LIST);
    ListView_DeleteAllItems(lv);
    for (i = 0; i < ARRAY_SIZE(row); i++) {
      row[i] = buf[i];
    }

    memset(unitarray, '\0', sizeof(unitarray));

   city_list_iterate(client.conn.playing->cities, pcity) {
      unit_list_iterate(client.conn.playing->units, punit) {
        Unit_type_id uti = utype_index(unit_type(punit));

        (unitarray[uti].active_count)++;
        if (punit->homecity) {
          output_type_iterate(o) {
            unitarray[uti].upkeep[o] += punit->upkeep[o];
          } output_type_iterate_end;
        }
      } unit_list_iterate_end;
   } city_list_iterate_end;

    city_list_iterate(client.conn.playing->cities, pcity) {
      if (VUT_UTYPE == pcity->production.kind) {
        struct unit_type *punittype = pcity->production.value.utype;
        (unitarray[utype_index(punittype)].building_count)++;
      }
    }
    city_list_iterate_end;

    k = 0;
    memset(&unittotals, '\0', sizeof(unittotals));
    unit_type_iterate(putype) {
      int index = utype_index(putype);
      if ((unitarray[index].active_count > 0)
	  || (unitarray[index].building_count > 0)) {
        can = (can_upgrade_unittype(client.conn.playing, putype) != NULL);
        my_snprintf(buf[0], sizeof(buf[0]), "%s",
                    utype_name_translation(putype));
        my_snprintf(buf[1], sizeof(buf[1]), "%c", can ? '*': '-');
        my_snprintf(buf[2], sizeof(buf[2]), "%3d",
                    unitarray[index].building_count);
        my_snprintf(buf[3], sizeof(buf[3]), "%3d",
                    unitarray[index].active_count);
        my_snprintf(buf[4], sizeof(buf[4]), "%3d",
                    unitarray[index].upkeep[O_SHIELD]);
        my_snprintf(buf[5], sizeof(buf[5]), "%3d",
                    unitarray[index].upkeep[O_FOOD]);
        /* TODO: add upkeep[O_GOLD] here */
        fcwin_listview_add_row(lv,k,AU_COL,row);
        activeunits_type[k]=(unitarray[index].active_count > 0) ? putype : NULL;
        k++;
        unittotals.active_count += unitarray[index].active_count;
        output_type_iterate(o) {
          unittotals.upkeep[0] = unitarray[index].upkeep[o];
        } output_type_iterate_end;
        unittotals.building_count += unitarray[index].building_count;
      }
    } unit_type_iterate_end;

    my_snprintf(buf[0],sizeof(buf[0]),"%s",_("Totals"));
    buf[1][0]='\0';
    my_snprintf(buf[2],sizeof(buf[2]),"%d",unittotals.building_count);
    my_snprintf(buf[3],sizeof(buf[3]),"%d",unittotals.active_count);
    my_snprintf(buf[4],sizeof(buf[4]),"%d",unittotals.upkeep[O_SHIELD]);
    my_snprintf(buf[5],sizeof(buf[5]),"%d",unittotals.upkeep[O_FOOD]);
    /* TODO: add upkeep[O_GOLD] here */
    fcwin_listview_add_row(lv,k,AU_COL,row);
    EnableWindow(GetDlgItem(activeunits_dlg,ID_MILITARY_UPGRADE),FALSE);
    ListView_SetColumnWidth(lv,0,LVSCW_AUTOSIZE);
    for(i=1;i<4;i++) {
      ListView_SetColumnWidth(lv,i,LVSCW_AUTOSIZE_USEHEADER);	
    }
    fcwin_redo_layout(activeunits_dlg);
  }
  
}

/**************************************************************************

**************************************************************************/
void
popup_activeunits_report_dialog(bool raise)
{
  if (!activeunits_dlg)
    {
      HWND lv;
      LV_COLUMN lvc;
      char *headers[] = {
        N_("Unit Type"),
        N_("?Upgradable unit [short]:U"),
        N_("In-Prog"),
        N_("Active"),
        N_("Shield"),
        N_("Food"),
        NULL
      };
      int i;
      struct fcwin_box *vbox;
      struct fcwin_box *hbox;
      activeunits_dlg=fcwin_create_layouted_window(activeunits_proc,
						   _("Units"),
						   WS_OVERLAPPEDWINDOW,
						   CW_USEDEFAULT,
						   CW_USEDEFAULT,
						   root_window,NULL,
						   JUST_CLEANUP,
						   NULL);
      vbox=fcwin_vbox_new(activeunits_dlg,FALSE);
      hbox=fcwin_hbox_new(activeunits_dlg,TRUE);
      fcwin_box_add_static(vbox,get_report_title(_("Units")),
			   ID_MILITARY_TOP,SS_CENTER,
			   FALSE,FALSE,10);
      lv=fcwin_box_add_listview(vbox,10,ID_MILITARY_LIST,
			     LVS_REPORT | LVS_SINGLESEL,
			     TRUE,TRUE,10);
      fcwin_box_add_button(hbox,_("Close"),IDCANCEL,0,
			   TRUE,TRUE,20);
      fcwin_box_add_button(hbox,_("Upgrade"),ID_MILITARY_UPGRADE,0,
			   TRUE,TRUE,20);
      fcwin_box_add_button(hbox,_("Refresh"),ID_MILITARY_REFRESH,0,
			   TRUE,TRUE,20);
      fcwin_box_add_box(vbox,hbox,FALSE,FALSE,10);
      lvc.mask=LVCF_TEXT;
      lvc.pszText=Q_(headers[0]);
      ListView_InsertColumn(lv,0,&lvc);
      for(i=1;i<AU_COL;i++) {
	lvc.mask=LVCF_TEXT | LVCF_FMT;
	lvc.fmt=LVCFMT_RIGHT;
	lvc.pszText=Q_(headers[i]);
	ListView_InsertColumn(lv,i,&lvc);
      }
      for(i=0;i<AU_COL;i++) {
	ListView_SetColumnWidth(lv,i,i?LVSCW_AUTOSIZE_USEHEADER:
				LVSCW_AUTOSIZE);
      }
      fcwin_set_box(activeunits_dlg,vbox);
    
    }
  activeunits_report_dialog_update();
  ShowWindow(activeunits_dlg,SW_SHOWNORMAL);
  if (raise) {
    SetFocus(activeunits_dlg);
  }
}

/****************************************************************
  Show a dialog with player statistics at endgame.
  TODO: Display all statistics in packet_endgame_report.
*****************************************************************/
void popup_endgame_report_dialog(struct packet_endgame_report *packet)
{
  char buffer[150 * MAX_NUM_PLAYERS];
  int i;
 
  buffer[0] = '\0';
  for (i = 0; i < packet->nscores; i++) {
    cat_snprintf(buffer, sizeof(buffer),
                 PL_("%2d: The %s ruler %s scored %d point\n",
                     "%2d: The %s ruler %s scored %d points\n",
                     packet->score[i]),
                 i + 1,
                 nation_adjective_for_player(player_by_number(packet->id[i])),
                 player_name(player_by_number(packet->id[i])),
                 packet->score[i]);
  }
  popup_notify_dialog(_("Final Report:"),
                      _("The Greatest Civilizations in the world."),
                      buffer);
}

/****************************************************************

*****************************************************************/
static void destroy_options_window(void)
{
  int i;
  DestroyWindow(settable_options_dialog_win);
  settable_options_dialog_win = NULL;
  free(categories);
  for (i = 0; i < num_tabs; i++) {
    DestroyWindow(tab_wnds[i]);
    tab_wnds[i] = NULL;
  }
  free(tab_wnds);
}

/****************************************************************

*****************************************************************/
static LONG CALLBACK OptionsWndProc(HWND hWnd,
				    UINT message,
				    WPARAM wParam,
				    LPARAM lParam) 
{
  LPNMHDR nmhdr;

  switch(message) {
    case WM_CLOSE:
      destroy_options_window();
      break;
    case WM_COMMAND:
      if (LOWORD(wParam) == IDOK) {
	int i, tab;
        for (i = 0; i < num_settable_options; i++) {
	  struct options_settable *o = &settable_options[i];

	  tab = categories[o->scategory];
	  if (SSET_BOOL == o->stype) {
	    /* checkbox */
	    int val = Button_GetState(GetDlgItem(tab_wnds[tab],
				      ID_OPTIONS_BASE + i)) == BST_CHECKED;
	    if (val != o->val) {
	      send_chat_printf("/set %s %d", o->name, val);
	    }
	  } else if (SSET_INT == o->stype) {
	    char buf[512];
	    int val;
	    Edit_GetText(GetDlgItem(tab_wnds[tab], ID_OPTIONS_BASE + i), buf,
				    512);
	    val = atoi(buf);
	    if (val != o->val) {
              send_chat_printf("/set %s %d", o->name, val);
	    }
	  } else {
	    char strval[512];
	    Edit_GetText(GetDlgItem(tab_wnds[tab], ID_OPTIONS_BASE + i),
			 strval, 512);
	    if (strcmp(strval, o->strval) != 0) {
              send_chat_printf("/set %s %s", o->name, strval);
	    }
	  }
	}
      }
      if ((LOWORD(wParam) == IDCANCEL) || (LOWORD(wParam) == IDOK)) {
        destroy_options_window();
      }
      break;
    case WM_NOTIFY:
      nmhdr = (LPNMHDR)lParam;
      if (nmhdr->hwndFrom == tab_ctrl) {
	int i, sel;
	sel = TabCtrl_GetCurSel(tab_ctrl);
	for (i = 0; i < num_tabs; i++) {
	  ShowWindow(tab_wnds[i], SW_HIDE);
	}
	if ((sel >= 0) && (sel < num_tabs)) {
	  ShowWindow(tab_wnds[sel], SW_SHOWNORMAL);
	}
      }
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
  }
  return (0);
}

/****************************************************************

*****************************************************************/
static LONG CALLBACK OptionsWndProc2(HWND hWnd,
				     UINT message,
				     WPARAM wParam,
				     LPARAM lParam) 
{
  switch(message) {
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
  }
  return (0);
}

/*************************************************************************
  Server options dialog
*************************************************************************/
static void create_settable_options_dialog(void)
{
  HWND win;
  struct fcwin_box *vbox, *hbox, **tab_boxes;
  char **titles;
  void **user_data;
  WNDPROC *tab_procs;
  bool *used;
  int i, j;

  num_tabs = 0;

  used = fc_calloc(num_options_categories, sizeof(*used));
  categories = fc_calloc(num_options_categories, sizeof(*categories));

  for (i = 0; i < num_settable_options; i++) {
    used[settable_options[i].scategory] = TRUE;
  }

  for(i = 0; i < num_options_categories; i++) {
    if (used[i]) {
      categories[i] = num_tabs;
      num_tabs++;
    }
  }

  titles = fc_malloc(sizeof(titles) * num_tabs);
  tab_procs = fc_malloc(sizeof(tab_procs) * num_tabs);
  tab_wnds = fc_malloc(sizeof(tab_wnds) * num_tabs);
  tab_boxes = fc_malloc(sizeof(tab_boxes) * num_tabs);
  user_data = fc_malloc(sizeof(user_data) * num_tabs);

  j = 0;
  for (i = 0; i < num_options_categories; i++) {
    if (used[i]) {
      titles[j] = _(options_categories[i]);
      tab_procs[j] = OptionsWndProc2;
      j++;
    }
  }

  win = fcwin_create_layouted_window(OptionsWndProc, _("Game Options"),
				     WS_OVERLAPPEDWINDOW, 20, 20,
				     root_window, NULL, REAL_CHILD, NULL);
  settable_options_dialog_win = win;
  vbox = fcwin_vbox_new(win, FALSE);

  /* create a notebook for the options */
  tab_ctrl = fcwin_box_add_tab(vbox, tab_procs, tab_wnds, titles,
			       user_data, num_tabs, 0, 0, TRUE, TRUE, 5);
  j = 0;
  for (i = 0; i < num_options_categories; i++) {
    if (used[i]) {
      tab_boxes[j] = fcwin_vbox_new(tab_wnds[j], FALSE);
      fcwin_set_box(tab_wnds[j], tab_boxes[j]);
      j++;
    }
  }

  for (i = 0; i < num_settable_options; i++) {
    struct options_settable *o = &settable_options[i];

    j = categories[o->scategory];
    hbox = fcwin_hbox_new(tab_wnds[j], FALSE);
    fcwin_box_add_static(hbox, _(o->short_help),
			 0, 0, FALSE, TRUE, 5);
    fcwin_box_add_static(hbox, "", 0, 0, TRUE, TRUE, 0);
    if (SSET_BOOL == o->stype) {
      HWND check;
      /* boolean */
      check = fcwin_box_add_checkbox(hbox, "", ID_OPTIONS_BASE + i, 0, FALSE,
				     TRUE, 5);
      Button_SetCheck(check,
		      o->val ? BST_CHECKED : BST_UNCHECKED);
    } else if (SSET_INT == o->stype) {
      /* integer */
      char buf[80];
      int length;
      my_snprintf(buf, 80, "%d", o->max);
      buf[79] = 0;
      length = strlen(buf);
      my_snprintf(buf, 80, "%d", o->min);
      buf[79] = 0;
      if (length < strlen(buf)) {
        length = strlen(buf);
      }
      my_snprintf(buf, 80, "%d", o->val);
      fcwin_box_add_edit(hbox, buf, length, ID_OPTIONS_BASE + i, 0, FALSE,
			 TRUE, 5);
    } else {
      /* string */
      fcwin_box_add_edit(hbox, o->strval, 40,
			 ID_OPTIONS_BASE + i, 0, FALSE, TRUE, 5);
    }
    fcwin_box_add_box(tab_boxes[j], hbox, FALSE, TRUE, 5);
  }

  hbox = fcwin_hbox_new(win, FALSE);
  fcwin_box_add_button(hbox, _("OK"), IDOK, 0, TRUE, TRUE, 5);
  fcwin_box_add_button(hbox, _("Cancel"), IDCANCEL, 0, TRUE, TRUE, 5);
  fcwin_box_add_box(vbox, hbox, TRUE, TRUE, 5);

  free(used);
  free(titles);
  free(tab_procs);
  free(user_data);
  free(tab_boxes);

  fcwin_set_box(win, vbox);

  for (i = 0; i < num_tabs; i++) {
    fcwin_redo_layout(tab_wnds[i]);
  }
  fcwin_redo_layout(win);
  ShowWindow(win, SW_SHOWNORMAL);
  ShowWindow(tab_wnds[0], SW_SHOWNORMAL);
}

/**************************************************************************
  Show a dialog with the server options.
**************************************************************************/
void popup_settable_options_dialog(void)
{
  if (!settable_options_dialog_win) {
    create_settable_options_dialog();
  }
}
