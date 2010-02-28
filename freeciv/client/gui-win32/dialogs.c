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
#include <stdarg.h>
#include <string.h>

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

/* common & utility */
#include "capability.h"
#include "fcintl.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "log.h"
#include "mem.h"
#include "packets.h"
#include "player.h"
#include "rand.h"
#include "support.h"
#include "unitlist.h"
 
/* client */
#include "client_main.h"
#include "control.h"
#include "tilespec.h"
#include "packhand.h"
#include "text.h"

#include "canvas.h"
#include "chatline.h"
#include "dialogs.h"
#include "gui_stuff.h"
#include "mapctrl.h"
#include "mapview.h"
#include "gui_main.h" 
                             
#define POPUP_MESSAGEDLG_IDBASE 500
#define UNITSELECT_CLOSE 300
#define UNITSELECT_READY_ALL 301
#define UNITSELECT_UNITS_BASE 305
#define MAX_NUM_GOVERNMENTS 10
#define ID_RACESDLG_NATION_TYPE_BASE 3000
#define ID_RACESDLG_LIST 2000
#define ID_RACESDLG_NATION_BASE 2000
#define ID_RACESDLG_STYLE_BASE 1000
#define ID_PILLAGE_BASE 1000

static HWND unit_select_main;
static HWND unit_select_ready;
static HWND unit_select_close;
static HWND unit_select_labels[100];
static HWND unit_select_but[100];
static HBITMAP unit_select_bitmaps[100];
static int unit_select_ids[100];
static int unit_select_no;

static HWND caravan_dialog;
static int caravan_city_id;
static int caravan_unit_id;

static bool is_showing_pillage_dialog = FALSE;
static int unit_to_use_to_pillage;

static HWND spy_tech_dialog;
static int advance_type[A_LAST+1];

static HWND spy_sabotage_dialog;
static int improvement_type[B_LAST+1];

                            
struct message_dialog_button
{
  int id;
  HWND buttwin;
  void *data;
  void (*callback)(HWND , void *);
  struct message_dialog_button *next;
  
};
struct message_dialog {
  HWND label;
  HWND main;
  struct fcwin_box *vbox;
  struct message_dialog_button *firstbutton;
};

static HWND races_dlg;
struct player *races_player;
static HWND races_listview;
/*
static HWND races_class;
static HWND races_legend;
*/
int visible_nations[MAX_NUM_ITEMS];
int selected_leader_sex;
int selected_style;
struct fcwin_box *government_box;
int selected_nation;
int selected_leader;
static int city_style_idx[64];        /* translation table basic style->city_style  */
static int city_style_ridx[64];       /* translation table the other way            */
                               /* they in fact limit the num of styles to 64 */
static int b_s_num; /* number of basic city styles, i.e. those that you can start with */



static HWND diplomat_dialog;
int diplomat_id;
int diplomat_target_id;
static LONG APIENTRY msgdialog_proc(HWND hWnd,
			       UINT message,
			       UINT wParam,
			       LONG lParam);                         
static void populate_nation_listview(struct nation_group* group, HWND listview);


/**************************************************************************

**************************************************************************/
static LONG CALLBACK notify_goto_proc(HWND dlg,UINT message,
				      WPARAM wParam,
				      LPARAM lParam)
{
  switch(message) {
  case WM_DESTROY:
    break;
  case WM_CREATE:
  case WM_SIZE:
  case WM_GETMINMAXINFO:
    break;
  case WM_CLOSE:
    DestroyWindow(dlg);
    break;
  case WM_COMMAND:
    switch(LOWORD(wParam)) {
    case IDOK:
      {
	struct tile *ptile = fcwin_get_user_data(dlg);
	center_tile_mapcanvas(ptile);
      }
    case IDCANCEL:
      DestroyWindow(dlg);
      break;
    }
    break;
  default:
    return DefWindowProc(dlg,message,wParam,lParam);
  }
  return 0;
}

/**************************************************************************
  Popup a dialog to display information about an event that has a
  specific location.  The user should be given the option to goto that
  location.
**************************************************************************/
void popup_notify_goto_dialog(const char *headline, const char *lines,
                              const struct text_tag_list *tags,
                              struct tile *ptile)
{
  struct fcwin_box *hbox;
  struct fcwin_box *vbox;
  HWND dlg;
  dlg=fcwin_create_layouted_window(notify_goto_proc,
				   headline,WS_OVERLAPPEDWINDOW,
				   CW_USEDEFAULT,CW_USEDEFAULT,
				   root_window,NULL,
				   REAL_CHILD,
				   ptile);
  vbox=fcwin_vbox_new(dlg,FALSE);
  fcwin_box_add_static(vbox,lines,0,SS_LEFT,
		       TRUE,TRUE,10);
  hbox=fcwin_hbox_new(dlg,TRUE);
  fcwin_box_add_button(hbox,_("Close"),IDCANCEL,0,TRUE,TRUE,10);
  fcwin_box_add_button(hbox,_("Goto and Close"),IDOK,0,TRUE,TRUE,10);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,10);
  fcwin_set_box(dlg,vbox);
  ShowWindow(dlg,SW_SHOWNORMAL);
}

/**************************************************************************
  Popup a dialog to display connection message from server.
**************************************************************************/
void popup_connect_msg(const char *headline, const char *message)
{
  /* FIXME: Needs proper implementation.
   *        Now just puts to chat window so message is not completely lost. */

  output_window_append(FTC_SERVER_INFO, NULL, message);
}

/**************************************************************************

**************************************************************************/
static LONG CALLBACK notify_proc(HWND hWnd,
				 UINT message,
				 WPARAM wParam,
				 LPARAM lParam)  
{
  switch(message)
    {
    case WM_CREATE:
      break;
    case WM_CLOSE:
      DestroyWindow(hWnd);

      break;
    case WM_COMMAND:
      if (LOWORD(wParam)==ID_NOTIFY_CLOSE)
	DestroyWindow(hWnd);
      break;
    case WM_DESTROY:
      break;
    case WM_GETMINMAXINFO:
      break;
    case WM_SIZE:
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
    }
  return 0;
}

/**************************************************************************
  Popup a generic dialog to display some generic information.
**************************************************************************/
void popup_notify_dialog(const char *caption, const char *headline,
			 const char *lines)
{
  HWND dlg;
  dlg=fcwin_create_layouted_window(notify_proc,caption,WS_OVERLAPPEDWINDOW,
				   CW_USEDEFAULT,CW_USEDEFAULT,
				   root_window,NULL,
				   REAL_CHILD,
				   NULL);
  if (dlg)
    {
      HWND edit;
      struct fcwin_box *vbox;
      char *buf;
      vbox=fcwin_vbox_new(dlg,FALSE);
      fcwin_box_add_static(vbox,headline,ID_NOTIFY_HEADLINE,SS_LEFT,
			   FALSE,FALSE,5);
      
      edit=CreateWindow("EDIT","", ES_WANTRETURN | ES_READONLY | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_CHILD | WS_VISIBLE,0,0,30,30,dlg,
      (HMENU)ID_NOTIFY_MSG,freecivhinst,NULL); 
      fcwin_box_add_win(vbox,edit,TRUE,TRUE,5);
      buf=convertnl2crnl(lines);
      SendMessage(GetDlgItem(dlg,ID_NOTIFY_MSG),
		  WM_SETFONT,(WPARAM) font_12courier, MAKELPARAM(TRUE,0)); 

      SetWindowText(GetDlgItem(dlg,ID_NOTIFY_MSG),buf);
      fcwin_box_add_button(vbox,_("Close"),ID_NOTIFY_CLOSE,WS_VISIBLE,
			   FALSE,FALSE,5);
      fcwin_set_box(dlg,vbox);
      free(buf);
      ShowWindow(dlg,SW_SHOWNORMAL);
    }
}


/**************************************************************************
 WS_GROUP seems not to work here
**************************************************************************/
static void update_radio_buttons(int id)
{  
  /* if (id!=selected_leader_sex)  */
  CheckRadioButton(races_dlg,ID_RACESDLG_MALE,
		   ID_RACESDLG_FEMALE,selected_leader_sex);
  /*  if (id!=selected_style) */
  CheckRadioButton(races_dlg,ID_RACESDLG_STYLE_BASE,
		   ID_RACESDLG_STYLE_BASE+b_s_num-1,selected_style);
}



/**************************************************************************

**************************************************************************/
static void update_nation_info()
{
  /*
  int i;
  char buf[255];
  struct nation_type *nation = nation_by_number(selected_nation);
 
  buf[0] = '\0';
  */

/*
  for (i = 0; i < nation->num_groups; i++) {
    sz_strlcat(buf, nation->groups[i]->name);
    if (i != nation->num_groups - 1) {
      sz_strlcat(buf, ", ");
    }
  }

  SetWindowText(races_class, buf);
  SetWindowText(races_legend, nation->legend);
*/
}


/**************************************************************************

**************************************************************************/
static void select_random_race(HWND hWnd)
{
  selected_nation = myrand(nation_count());
  update_nation_info();
  update_radio_buttons(0);
}

/**************************************************************************

**************************************************************************/
static void select_random_leader(HWND hWnd)
{
  int j,leader_num;
  struct nation_leader *leaders 
    = get_nation_leaders(nation_by_number(selected_nation), &leader_num);

  ComboBox_ResetContent(GetDlgItem(hWnd,ID_RACESDLG_LEADER));
  for (j = 0; j < leader_num; j++) {
    ComboBox_AddString(GetDlgItem(hWnd,ID_RACESDLG_LEADER), leaders[j].name);
  }
  selected_leader=myrand(leader_num);
  ComboBox_SetCurSel(GetDlgItem(hWnd,ID_RACESDLG_LEADER),selected_leader);
  SetWindowText(GetDlgItem(hWnd,ID_RACESDLG_LEADER),
		leaders[selected_leader].name);
  if (leaders[selected_leader].is_male) {
    selected_leader_sex=ID_RACESDLG_MALE;
    CheckRadioButton(hWnd,ID_RACESDLG_MALE,ID_RACESDLG_FEMALE,
		     ID_RACESDLG_MALE);
  } else {
    selected_leader_sex=ID_RACESDLG_FEMALE;
    CheckRadioButton(hWnd,ID_RACESDLG_MALE,ID_RACESDLG_FEMALE,
		     ID_RACESDLG_FEMALE);
  }
}

/**************************************************************************

**************************************************************************/
static void do_select(HWND hWnd)
{
  bool is_male = (selected_leader_sex == ID_RACESDLG_MALE);
  int city_style = city_style_idx[selected_style - ID_RACESDLG_STYLE_BASE];
  char name[MAX_LEN_NAME];
 
  ComboBox_GetText(GetDlgItem(hWnd,ID_RACESDLG_LEADER),
		   name,MAX_LEN_NAME);

  if (strlen(name) == 0) {
    output_window_append(FTC_CLIENT_INFO, NULL,
                         _("You must type a legal name."));
    return;
  }
  dsend_packet_nation_select_req(&client.conn, player_number(races_player),
				 selected_nation, is_male, name, city_style);

  popdown_races_dialog();
}


/**************************************************************************

**************************************************************************/
static LONG CALLBACK racesdlg_proc(HWND hWnd,
				   UINT message,
				   WPARAM wParam,
				   LPARAM lParam)  
{
  static bool name_edited;
  int id, n, sel, i;
  NMHDR *pnmh;

  switch(message)
    {
    case WM_CREATE:
      name_edited=FALSE;
      break;
    case WM_CLOSE:
      popdown_races_dialog();
      break;
    case WM_DESTROY:
      races_dlg=NULL;
      break;
    case WM_SIZE:
      break;
    case WM_GETMINMAXINFO:
      break;
    case WM_NOTIFY:
      pnmh = (LPNMHDR)lParam;
      if (pnmh->idFrom != ID_RACESDLG_LIST) {
	break;
      }
      n = ListView_GetItemCount(races_listview);
      sel = -1;
      for(i = 0;i < n; i++) {
	if (ListView_GetItemState(races_listview, i, LVIS_SELECTED)) {
	  sel = visible_nations[i];
	  break;
	}
      }
      if (sel == -1 || selected_nation == sel) {
	break;
      }
      selected_nation = sel;
      update_nation_info();
      if (!name_edited) {
	select_random_leader(hWnd);
      }
      break;
    case WM_COMMAND:
      id=LOWORD(wParam);
      switch(id)
	{ 
	case ID_RACESDLG_MALE:
	case ID_RACESDLG_FEMALE:
	    selected_leader_sex=id;	    
	    update_radio_buttons(id);
	  break;
	case ID_RACESDLG_LEADER:
	  switch(HIWORD(wParam)) {
	  case CBN_SELCHANGE:
	    name_edited=FALSE;
	    break; 
	  case CBN_EDITCHANGE:
	    name_edited=TRUE;
	    break;
	  }
	  break;
	case ID_RACESDLG_QUIT:
	  client_exit();
	  break;
	case IDCANCEL:
	  popdown_races_dialog();
	  break;
	case IDOK:
	  do_select(hWnd);
	  break;
	default:
	  if (id >= ID_RACESDLG_NATION_TYPE_BASE) {
	    if (id == ID_RACESDLG_NATION_TYPE_BASE) {
	      populate_nation_listview(NULL, races_listview);
	    } else {
	      populate_nation_listview(nation_group_by_number(id - ID_RACESDLG_NATION_TYPE_BASE - 1), races_listview);
	    }
	  }
	  break;
	}
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);      
    }
  return 0;
}

/****************************************************************
...
*****************************************************************/
static void populate_nation_listview(struct nation_group* group, HWND listview)
{
  int n;

  ListView_DeleteAllItems(listview);

  n = 0;
  nations_iterate(pnation) {
    char *strings[1];

    if (!is_nation_playable(pnation) || !pnation->is_available) {
      continue;
    }

    if (group != NULL && !is_nation_in_group(pnation, group)) {
      continue;
    }

    /* FIXME: fcwin_listview_add_row() should be fixed to handle
     *        const strings. Now we just cast const away */
    strings[0] = (char *) nation_adjective_translation(pnation);

    fcwin_listview_add_row(listview, nation_index(pnation), 1, strings);
    visible_nations[n++] = nation_index(pnation);

  } nations_iterate_end;

  ListView_SetColumnWidth(listview, 0, LVSCW_AUTOSIZE);
}

/****************************************************************
...
*****************************************************************/
static void create_races_dialog(struct player *pplayer)
{
  HWND shell;
  struct fcwin_box *shell_box, *left_column, *right_column, *group_box, *right_entry_box, *input_box, *sex_select_box, *bottom_bar, *button_column;
  int i;

  shell =
    fcwin_create_layouted_window(racesdlg_proc, _("What nation will you be?"),
				 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
				 CW_USEDEFAULT, root_window, NULL,
				 REAL_CHILD, NULL);

  races_dlg = shell;
  races_player = pplayer;

  shell_box = fcwin_vbox_new(shell, FALSE);
  fcwin_set_box(shell, shell_box);

  group_box = fcwin_hbox_new(shell, FALSE);
  fcwin_box_add_groupbox(shell_box, _("Select a nation"), group_box, 0, FALSE, FALSE, 0);

  left_column = fcwin_hbox_new(shell, FALSE);
  fcwin_box_add_box(group_box, left_column, FALSE, FALSE, 5);

  right_column = fcwin_vbox_new(shell, FALSE);
  fcwin_box_add_box(group_box, right_column, FALSE, FALSE, 5);
  
  /* left column */
  
  button_column = fcwin_vbox_new(shell, FALSE);
  fcwin_box_add_box(left_column, button_column, FALSE, FALSE, 5);

  for (i = 0; i <= nation_group_count(); i++) {
    struct nation_group* group = (i == 0 ? NULL: nation_group_by_number(i - 1));
    fcwin_box_add_button(button_column, group ? _(group->name) : _("All"), ID_RACESDLG_NATION_TYPE_BASE + i, WS_GROUP, TRUE, TRUE, 0);
  }

  races_listview = fcwin_box_add_listview(left_column, 4, ID_RACESDLG_LIST,
					  LVS_REPORT | LVS_SHOWSELALWAYS
					  | LVS_SINGLESEL, TRUE, TRUE, 0);

  LV_COLUMN lvc;

  lvc.mask = LVCF_TEXT | LVCF_FMT;
  lvc.pszText = _("Nation");
  lvc.fmt = LVCFMT_LEFT;

  ListView_InsertColumn(races_listview, 0, &lvc);

  /* Populate nation list store. */

  populate_nation_listview(NULL, races_listview);

  /* right column */
  right_entry_box = fcwin_hbox_new(shell, FALSE);
  fcwin_box_add_box(right_column, right_entry_box, FALSE, FALSE, 0);

  fcwin_box_add_static(right_entry_box, _("Leader:"), 0, 0, TRUE, TRUE, 5);

  input_box = fcwin_vbox_new(shell, FALSE);
  fcwin_box_add_box(right_entry_box, input_box, FALSE, FALSE, 0);

  fcwin_box_add_combo(input_box, 10, ID_RACESDLG_LEADER, CBS_DROPDOWN
		      | WS_VSCROLL, FALSE, FALSE, 0);

  sex_select_box = fcwin_hbox_new(shell, TRUE);

  fcwin_box_add_radiobutton(sex_select_box, _("Male"), ID_RACESDLG_MALE, 0,
			    TRUE,TRUE, 0);
  fcwin_box_add_radiobutton(sex_select_box, _("Female"), ID_RACESDLG_FEMALE,
			    WS_GROUP, TRUE, TRUE, 0);

  fcwin_box_add_box(input_box, sex_select_box, FALSE, TRUE, 0);

  right_entry_box = fcwin_hbox_new(shell, FALSE);
  fcwin_box_add_box(right_column, right_entry_box, FALSE, FALSE, 0);

  fcwin_box_add_static(right_entry_box, _("City Style:"), 0, 0, TRUE, TRUE, 5);
  input_box = fcwin_vbox_new(shell, FALSE);
  fcwin_box_add_box(right_entry_box, input_box, FALSE, FALSE, 0);

  for(i=0, b_s_num=0; i < game.control.styles_count && i < 64; i++) {
    if (!city_style_has_requirements(&city_styles[i])) {
      city_style_idx[b_s_num] = i;
      city_style_ridx[i] = b_s_num;
      b_s_num++;
    }
  }
  for(i = 0; i < b_s_num; i++) {
    fcwin_box_add_radiobutton(input_box, city_style_name_translation(city_style_idx[i]),
			      ID_RACESDLG_STYLE_BASE + i, 0, TRUE, TRUE, 0);
  }

  bottom_bar = fcwin_hbox_new(shell, FALSE);

  fcwin_box_add_button(bottom_bar, _("OK"), IDOK, WS_GROUP, TRUE, TRUE, 0);
  fcwin_box_add_button(bottom_bar, _("Random"), IDCANCEL, 0, TRUE, TRUE, 0);

  fcwin_box_add_box(shell_box, bottom_bar, FALSE, FALSE, 0);

  fcwin_redo_layout(shell);

  ShowWindow(shell, SW_SHOWNORMAL);
}

/**************************************************************************

**************************************************************************/
void popup_races_dialog(struct player *pplayer)
{
  if (!races_dlg) {
    create_races_dialog(pplayer);
    select_random_race(races_dlg);
    SetFocus(races_dlg);
  }
}

/**************************************************************************

**************************************************************************/
void
popdown_races_dialog(void)
{
  if (races_dlg) {
    DestroyWindow(races_dlg);
    races_dlg = NULL;
    SetFocus(root_window);
  }
}

/**************************************************************************

**************************************************************************/
static int number_of_columns(int n)
{
#if 0
  /* This would require libm, which isn't worth it for this one little
   * function.  Since the number of units is limited to 100 already, the ifs
   * work fine.  */
  double sqrt(); double ceil();
  return ceil(sqrt((double)n/5.0));
#else
  if(n<=5) return 1;
  else if(n<=20) return 2;
  else if(n<=45) return 3;
  else if(n<=80) return 4;
  else return 5;
#endif
}

/**************************************************************************

**************************************************************************/
static int number_of_rows(int n)
{
  int c=number_of_columns(n);
  return (n+c-1)/c;
}                                                                          

/**************************************************************************

**************************************************************************/     
static void popdown_unit_select_dialog(void)
{
  if (unit_select_main)
    DestroyWindow(unit_select_main);
}

/**************************************************************************

**************************************************************************/
static LONG APIENTRY unitselect_proc(HWND hWnd, UINT message,
				     UINT wParam, LONG lParam)
{
  int id;
  int i;
  switch(message)
    {
    case WM_CLOSE:
      popdown_unit_select_dialog();
      return TRUE;
      break;
    case WM_DESTROY:
      for (i=0;i<unit_select_no;i++) {
	DeleteObject(unit_select_bitmaps[i]);
      }
      unit_select_main=NULL;
      break;
    case WM_COMMAND:
      id=LOWORD(wParam);
      switch(id)
	{
	case UNITSELECT_CLOSE:
	  break;
	case UNITSELECT_READY_ALL:
	  for(i=0; i<unit_select_no; i++) {
	    struct unit *punit = player_find_unit_by_id(client.conn.playing,
							unit_select_ids[i]);
	    if(punit) {
	      set_unit_focus(punit);
	    }
	  }  
	  break;
	default:
	  id-=UNITSELECT_UNITS_BASE;
	  if ((id>=0)&&(id<100))
	    {
	      struct unit *punit = player_find_unit_by_id(client.conn.playing,
							  unit_select_ids[id]);
	      if (NULL != punit && unit_owner(punit) == client.conn.playing) {
		set_unit_focus(punit);
	      }   
	    }
	  break;
	}
      popdown_unit_select_dialog();
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
    }
  return FALSE;
}


/**************************************************************************

**************************************************************************/
BOOL unitselect_init(HINSTANCE hInstance)
{
  
  HANDLE hMemory;
  LPWNDCLASS pWndClass;
  BOOL bSuccess;
  hMemory=LocalAlloc(LPTR,sizeof(WNDCLASS));
  if (!hMemory)
    {
      return(FALSE);
    }
  pWndClass=(LPWNDCLASS)LocalLock(hMemory);
  pWndClass->style=0;
  pWndClass->lpfnWndProc=(WNDPROC) unitselect_proc;
  pWndClass->hIcon=NULL;
  pWndClass->hCursor=LoadCursor(NULL,IDC_ARROW);
  pWndClass->hInstance=hInstance;
  pWndClass->hbrBackground=CreateSolidBrush(GetSysColor(4));
  pWndClass->lpszClassName=(LPSTR)"freecivunitselect";
  pWndClass->lpszMenuName=(LPSTR)NULL;
  bSuccess=RegisterClass(pWndClass);
  LocalUnlock(hMemory);
  LocalFree(hMemory);
  return bSuccess;
}

/**************************************************************************

**************************************************************************/
void
popup_unit_select_dialog(struct tile *ptile)
{
  int i,n,r,c;
  int max_width,max_height;
  int win_height,win_width;
  char buffer[512];
  char *but1=_("Ready all");
  char *but2=_("Close");
  HDC hdc;
  POINT pt;
  RECT rc,rc2;
  HBITMAP old;
  HDC unitsel_dc;
  struct unit *unit_list[unit_list_size(ptile->units)];
  
  fill_tile_unit_list(ptile, unit_list);
  
  /* unit select box might already be open; if so, close it */
  if (unit_select_main) {
    popdown_unit_select_dialog ();
  }
  
  GetCursorPos(&pt);
  if (!(unit_select_main=CreateWindow("freecivunitselect",
				      _("Unit Selection"),
				      WS_POPUP | WS_CAPTION | WS_SYSMENU,
				      pt.x+20,
				      pt.y+20,
				      40,40,
				      NULL,
				      NULL,
				      freecivhinst,
				      NULL)))
    return;
  hdc=GetDC(unit_select_main);
  unitsel_dc=CreateCompatibleDC(NULL);
  n = unit_list_size(ptile->units);
  r=number_of_rows(n);
  c=number_of_columns(n);
  max_width=0;
  max_height=0;
  for(i=0;i<n;i++)
    {
      struct unit *punit = unit_list[i];
      struct unit_type *punittemp=unit_type(punit);
      struct city *pcity = player_find_city_by_id(client.conn.playing, punit->homecity);

      unit_select_ids[i]=punit->id;

      my_snprintf(buffer, sizeof(buffer), "%s(%s)\n%s",
		  utype_name_translation(punittemp),
		  pcity ? city_name(pcity) : "",
		  unit_activity_text(punit));
      DrawText(hdc,buffer,strlen(buffer),&rc,DT_CALCRECT);
      if ((rc.right-rc.left)>max_width)
	max_width=rc.right-rc.left;
      if ((rc.bottom-rc.top)>max_height)
	max_height=rc.bottom-rc.top;
      unit_select_bitmaps[i]=CreateCompatibleBitmap(hdc,tileset_full_tile_width(tileset),
						    tileset_full_tile_height(tileset));
    }
  ReleaseDC(unit_select_main,hdc);
  old=SelectObject(unitsel_dc,unit_select_bitmaps[0]);
  max_width+=tileset_full_tile_width(tileset);
  max_width+=4;
  if (max_height<tileset_full_tile_width(tileset))
    {
      max_height=tileset_full_tile_height(tileset);
    }
  max_height+=4;
  for (i=0;i<n;i++)
    {
      struct canvas canvas_store;
      struct unit *punit=unit_list[i];
      struct unit_type *punittemp=unit_type(punit);
      struct city *pcity = player_find_city_by_id(client.conn.playing, punit->homecity);

      canvas_store.type = CANVAS_DC;
      canvas_store.hdc = unitsel_dc;
      canvas_store.bmp = NULL;
      canvas_store.wnd = NULL;
      canvas_store.tmp = NULL;

      my_snprintf(buffer, sizeof(buffer), "%s(%s)\n%s",
		  utype_name_translation(punittemp),
		  pcity ? city_name(pcity) : "",
		  unit_activity_text(punit));
      unit_select_labels[i]=CreateWindow("STATIC",buffer,
					 WS_CHILD | WS_VISIBLE | SS_LEFT,
					 (i/r)*max_width+tileset_full_tile_width(tileset),
					 (i%r)*max_height,
					 max_width-tileset_full_tile_width(tileset),
					 max_height,
					 unit_select_main,
					 NULL,
					 freecivhinst,
					 NULL);
      SelectObject(unitsel_dc,unit_select_bitmaps[i]);
      BitBlt(unitsel_dc,0,0,tileset_full_tile_width(tileset),tileset_full_tile_height(tileset),NULL,
	     0,0,WHITENESS);
      put_unit(punit,&canvas_store,0,0);
      unit_select_but[i]=CreateWindow("BUTTON",NULL,
				      WS_CHILD | WS_VISIBLE | BS_BITMAP,
				      (i/r)*max_width,
				      (i%r)*max_height,
				      tileset_full_tile_width(tileset),
				      tileset_full_tile_height(tileset),
				      unit_select_main,
				      (HMENU)(UNITSELECT_UNITS_BASE+i),
				      freecivhinst,
				      NULL);
      SendMessage(unit_select_but[i],BM_SETIMAGE,(WPARAM)0,
		  (LPARAM)unit_select_bitmaps[i]);
      
    }
  SelectObject(unitsel_dc,old);
  DeleteDC(unitsel_dc);
  unit_select_no=i;
  win_height=r*max_height;
  win_width=c*max_width;
  hdc=GetDC(unit_select_main);
  DrawText(hdc,but1,strlen(but1),&rc,DT_CALCRECT);
  if (rc.right-rc.left>(win_width/2-4))
    win_width=(rc.right-rc.left+4)*2;
  DrawText(hdc,but2,strlen(but2),&rc,DT_CALCRECT);
  if (rc.right-rc.left>(win_width/2-4))
    win_width=(rc.right-rc.left+4)*2;
  ReleaseDC(unit_select_main,hdc);
  unit_select_close=CreateWindow("BUTTON",but2,
				 WS_CHILD | WS_VISIBLE | BS_CENTER,
				 win_width/2,win_height+2,
				 win_width/2-2,rc.bottom-rc.top,
				 unit_select_main,
				 (HMENU)UNITSELECT_CLOSE,
				 freecivhinst,
				 NULL);
  unit_select_ready=CreateWindow("BUTTON",but1,
				 WS_CHILD | WS_VISIBLE | BS_CENTER,
				 2,win_height+2,
				 win_width/2-2,rc.bottom-rc.top,
				 unit_select_main,
				 (HMENU)UNITSELECT_READY_ALL,
				 freecivhinst,
				 NULL);
  win_height+=rc.bottom-rc.top;
  win_height+=4;
  GetWindowRect(unit_select_main,&rc);
  GetClientRect(unit_select_main,&rc2);
  win_height+=rc.bottom-rc.top-rc2.bottom+rc2.top;
  win_width+=rc.right-rc.left+rc2.left-rc2.right;
  MoveWindow(unit_select_main,rc.left,rc.top,win_width,win_height,TRUE);
  ShowWindow(unit_select_main,SW_SHOWNORMAL);
  
}

/**************************************************************************

**************************************************************************/
void races_toggles_set_sensitive(void)
{
#if 0
/* FIXME!! */
  int i;
  BOOL changed;

  for (i = 0; i < nation_count(); i++) {
    EnableWindow(GetDlgItem(races_dlg, ID_RACESDLG_NATION_BASE + i), TRUE);
  }

  changed = FALSE;

  nations_iterate(nation) {
    if (!nation->is_available || nation->player) {
      continue;
    }

    EnableWindow(GetDlgItem(races_dlg, ID_RACESDLG_NATION_BASE + nation_index(nation)),
		 FALSE);

    changed = TRUE;
  } nations_iterate_end;

  if (changed) {
    select_random_race(races_dlg);
  }
#endif
}

/****************************************************************
...
*****************************************************************/
static void revolution_callback_yes(HWND w, void * data)
{
  if ((int)data == -1) {
    start_revolution();
  } else {
    set_government_choice((struct government *)data);
  }
  destroy_message_dialog(w);
}
 
/****************************************************************
...
*****************************************************************/
static void revolution_callback_no(HWND w, void * data)
{
  destroy_message_dialog(w);
}
 
 
 
/****************************************************************
...
*****************************************************************/
void popup_revolution_dialog(struct government *gov)
{
  if (client.conn.playing->revolution_finishes < game.info.turn) {
    popup_message_dialog(NULL, _("Revolution!"),
			 _("You say you wanna revolution?"),
			 _("_Yes"),revolution_callback_yes, gov,
			 _("_No"),revolution_callback_no, 0,
			 0);
  } else {
    if (gov == NULL) {
      start_revolution();
    } else {
      set_government_choice(gov);
    }
  }
}
 
/****************************************************************
...
*****************************************************************/
static void caravan_establish_trade_callback(HWND w, void * data)
{
  dsend_packet_unit_establish_trade(&client.conn, caravan_unit_id);
 
  destroy_message_dialog(w);
  caravan_dialog = 0;
  process_caravan_arrival(NULL);
}
 
               
/****************************************************************
...
*****************************************************************/
static void caravan_help_build_wonder_callback(HWND w, void * data)
{
  dsend_packet_unit_help_build_wonder(&client.conn, caravan_unit_id);
 
  destroy_message_dialog(w);
  caravan_dialog = 0;
  process_caravan_arrival(NULL);
}
 
 
/****************************************************************
...
*****************************************************************/
static void caravan_keep_moving_callback(HWND w, void * data)
{
  destroy_message_dialog(w);
  caravan_dialog = 0;
  process_caravan_arrival(NULL);
}
 
 
 
/****************************************************************
...
*****************************************************************/  
void
popup_caravan_dialog(struct unit *punit,
                          struct city *phomecity, struct city *pdestcity)
{
  char buf[128];
  bool can_establish, can_trade;
  
  my_snprintf(buf, sizeof(buf),
              _("Your caravan from %s reaches the city of %s.\nWhat now?"),
              city_name(phomecity), city_name(pdestcity));
 
  caravan_city_id=pdestcity->id; /* callbacks need these */
  caravan_unit_id=punit->id;
 
  can_trade = can_cities_trade(phomecity, pdestcity);
  can_establish = can_trade
  		  && can_establish_trade_route(phomecity, pdestcity);
  
  caravan_dialog=popup_message_dialog(NULL,
                           /*"caravandialog"*/_("Your Caravan Has Arrived"),
                           buf,
                           (can_establish ? _("Establish _Traderoute") :
  			   _("Enter Marketplace")),caravan_establish_trade_callback, 0,
                           _("Help build _Wonder"),caravan_help_build_wonder_callback, 0,
                           _("_Keep moving"),caravan_keep_moving_callback, 0,
                           0);
 
  if (!can_trade)
    {
      message_dialog_button_set_sensitive(caravan_dialog,0,FALSE);
    }
 
  if(!unit_can_help_build_wonder(punit, pdestcity))
    {
      message_dialog_button_set_sensitive(caravan_dialog,1,FALSE);
    } 
  
}

bool caravan_dialog_is_open( int* unit_id, int* city_id)
{
  return BOOL_VAL(caravan_dialog);
}

/****************************************************************
  Updates caravan dialog
****************************************************************/
void caravan_dialog_update(void)
{
  /* PORT ME */
}

/****************************************************************
...
*****************************************************************/
static void diplomat_investigate_callback(HWND w, void * data)
{
  destroy_message_dialog(w);
  diplomat_dialog=0;
 
  if(game_find_unit_by_number(diplomat_id) &&
     (game_find_city_by_number(diplomat_target_id))) {
    request_diplomat_action(DIPLOMAT_INVESTIGATE, diplomat_id,
			    diplomat_target_id, 0);
  }
 
  process_diplomat_arrival(NULL, 0);
}
/****************************************************************
...
*****************************************************************/
static void diplomat_steal_callback(HWND w, void * data)
{
  destroy_message_dialog(w);
  diplomat_dialog=0;
 
  if(game_find_unit_by_number(diplomat_id) &&
     game_find_city_by_number(diplomat_target_id)) {
    request_diplomat_action(DIPLOMAT_STEAL, diplomat_id,
			    diplomat_target_id, A_UNSET);
  }
 
  process_diplomat_arrival(NULL, 0);
}
/****************************************************************
...
*****************************************************************/
static void diplomat_sabotage_callback(HWND w, void * data)
{
  destroy_message_dialog(w);
  diplomat_dialog=0;
 
  if(game_find_unit_by_number(diplomat_id) &&
     game_find_city_by_number(diplomat_target_id)) {
    request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,
			    diplomat_target_id, -1);
  }
 
  process_diplomat_arrival(NULL, 0);
}                  
/*****************************************************************/
static void diplomat_embassy_callback(HWND w, void * data)
{
  destroy_message_dialog(w);
  diplomat_dialog=0;
 
  if(game_find_unit_by_number(diplomat_id) &&
     (game_find_city_by_number(diplomat_target_id))) {
    request_diplomat_action(DIPLOMAT_EMBASSY, diplomat_id,
			    diplomat_target_id, 0);
  }
 
  process_diplomat_arrival(NULL, 0);
}    
/****************************************************************
...
*****************************************************************/
static void spy_sabotage_unit_callback(HWND w, void * data)
{
  request_diplomat_action(SPY_SABOTAGE_UNIT, diplomat_id,
			  diplomat_target_id, 0);

  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void spy_poison_callback(HWND w, void * data)
{
  destroy_message_dialog(w);
  diplomat_dialog=0;

  if(game_find_unit_by_number(diplomat_id) &&
     (game_find_city_by_number(diplomat_target_id))) {
    request_diplomat_action(SPY_POISON, diplomat_id, diplomat_target_id, 0);
  }

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void create_advances_list(struct player *pplayer,
                                struct player *pvictim, HWND lb)
{
  int j = 0;

  advance_type[j] = -1;
  
  if (pvictim) { /* you don't want to know what lag can do -- Syela */
    advance_index_iterate(A_FIRST, i) {
      if(player_invention_state(pvictim, i)==TECH_KNOWN && 
         (player_invention_state(pplayer, i)==TECH_UNKNOWN || 
          player_invention_state(pplayer, i)==TECH_PREREQS_KNOWN)) {
	ListBox_AddString(lb,advance_name_translation(advance_by_number(i)));
        advance_type[j++] = i;
      }
    } advance_index_iterate_end;
    
    if(j > 0) {
      ListBox_AddString(lb,_("At Spy's Discretion"));
      advance_type[j++] = A_UNSET;
    }
  }
  if(j == 0) {
    ListBox_AddString(lb,_("NONE"));
    j++;
  }
}

#define ID_SPY_LIST 100
/****************************************************************

*****************************************************************/
static LONG CALLBACK spy_tech_proc(HWND dlg,UINT message,WPARAM wParam,
				   LPARAM lParam)
{
  switch(message)
    {
    case WM_CREATE:
    case WM_SIZE:
    case WM_GETMINMAXINFO:
      break;
    case WM_DESTROY:
      spy_tech_dialog=NULL;
      process_diplomat_arrival(NULL, 0);
      break;
    case WM_CLOSE:
      DestroyWindow(dlg);
      break;
    case WM_COMMAND:
      switch(LOWORD(wParam)) {
      case ID_SPY_LIST:
	if (ListBox_GetCurSel(GetDlgItem(dlg,ID_SPY_LIST))!=LB_ERR) {
	  EnableWindow(GetDlgItem(dlg,IDOK),TRUE);
	}
	break;
      case IDOK:
	{
	  int steal_advance;
	  steal_advance=ListBox_GetCurSel(GetDlgItem(dlg,ID_SPY_LIST));
	  if (steal_advance==LB_ERR)
	    break;
	  steal_advance=advance_type[steal_advance];
	  if(game_find_unit_by_number(diplomat_id) && 
	     game_find_city_by_number(diplomat_target_id)) { 
	    request_diplomat_action(DIPLOMAT_STEAL, diplomat_id,
				    diplomat_target_id, steal_advance);
	  }
	  
	  
	}
      case IDCANCEL:
	DestroyWindow(dlg);
	break;
      }
      break;
    default:
      return DefWindowProc(dlg,message,wParam,lParam);
    }
  return 0;
}

/****************************************************************
...
*****************************************************************/
static void spy_steal_popup(HWND w, void * data)
{
  struct city *pvcity = game_find_city_by_number(diplomat_target_id);
  struct player *pvictim = NULL;

  if(pvcity)
    pvictim = city_owner(pvcity);

/* it is concievable that pvcity will not be found, because something
has happened to the city during latency.  Therefore we must initialize
pvictim to NULL and account for !pvictim in create_advances_list. -- Syela */
  
  destroy_message_dialog(w);
  diplomat_dialog=0;
  if(!spy_tech_dialog){
    HWND lb;
    struct fcwin_box *hbox;
    struct fcwin_box *vbox;
    spy_tech_dialog=fcwin_create_layouted_window(spy_tech_proc,
						 _("Steal Technology"),
						 WS_OVERLAPPEDWINDOW,
						 CW_USEDEFAULT,
						 CW_USEDEFAULT,
						 root_window,
						 NULL,
						 REAL_CHILD,
						 NULL);
    hbox=fcwin_hbox_new(spy_tech_dialog,TRUE);
    vbox=fcwin_vbox_new(spy_tech_dialog,FALSE);
    fcwin_box_add_static(vbox,_("Select Advance to Steal"),
			 0,SS_LEFT,FALSE,FALSE,5);
    lb=fcwin_box_add_list(vbox,5,ID_SPY_LIST,WS_VSCROLL,TRUE,TRUE,5);
    fcwin_box_add_button(hbox,_("Close"),IDCANCEL,0,TRUE,TRUE,10);
    fcwin_box_add_button(hbox,_("Steal"),IDOK,0,TRUE,TRUE,10);
    EnableWindow(GetDlgItem(spy_tech_dialog,IDOK),FALSE);
    fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
    create_advances_list(client.conn.playing, pvictim, lb);
    fcwin_set_box(spy_tech_dialog,vbox);
    ShowWindow(spy_tech_dialog,SW_SHOWNORMAL);
  }
 
}

/****************************************************************
 Requests up-to-date list of improvements, the return of
 which will trigger the popup_sabotage_dialog() function.
*****************************************************************/
static void spy_request_sabotage_list(HWND w, void * data)
{
  destroy_message_dialog(w);
  diplomat_dialog=0;

  if (game_find_unit_by_number(diplomat_id)
   && game_find_city_by_number(diplomat_target_id)) {
    request_diplomat_answer(DIPLOMAT_SABOTAGE, diplomat_id,
			    diplomat_target_id, 0);
  }
}


/**************************************************************************

**************************************************************************/
static void diplomat_bribe_yes_callback(HWND w, void * data)
{
  request_diplomat_action(DIPLOMAT_BRIBE, diplomat_id,
			  diplomat_target_id, 0);

  destroy_message_dialog(w);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_bribe_no_callback(HWND w, void * data)
{
  destroy_message_dialog(w);
}



/****************************************************************
...  Ask the server how much the bribe is
*****************************************************************/
static void diplomat_bribe_callback(HWND w, void * data)
{

  destroy_message_dialog(w);
  
  if (game_find_unit_by_number(diplomat_id)
   && game_find_unit_by_number(diplomat_target_id)) { 
    request_diplomat_answer(DIPLOMAT_BRIBE, diplomat_id,
			    diplomat_target_id, 0);
   }

}
 
/****************************************************************
...
*****************************************************************/
void popup_bribe_dialog(struct unit *punit, int cost)
{
  char buf[128];
  if (unit_has_type_flag(punit, F_UNBRIBABLE)) {
    popup_message_dialog(root_window, _("Ooops..."),
                         _("This unit cannot be bribed!"),
                         diplomat_bribe_no_callback, 0, 0);
  } else if (cost <= client.conn.playing->economic.gold) {
    my_snprintf(buf, sizeof(buf),
                _("Bribe unit for %d gold?\nTreasury contains %d gold."), 
                cost, client.conn.playing->economic.gold);
    popup_message_dialog(root_window, /*"diplomatbribedialog"*/_("Bribe Enemy Unit"
), buf,
                        _("_Yes"), diplomat_bribe_yes_callback, 0,
                        _("_No"), diplomat_bribe_no_callback, 0, 0);
  } else {
    my_snprintf(buf, sizeof(buf),
                _("Bribing the unit costs %d gold.\n"
                  "Treasury contains %d gold."), 
                cost, client.conn.playing->economic.gold);
    popup_message_dialog(root_window, /*"diplomatnogolddialog"*/
	    	_("Traitors Demand Too Much!"), buf, _("Darn"),
		diplomat_bribe_no_callback, 0, 0);
  }
}

/****************************************************************

****************************************************************/
static void create_improvements_list(struct player *pplayer,
                                    struct city *pcity, HWND lb)
{
  int j;
  j=0;
  ListBox_AddString(lb,_("City Production"));
  improvement_type[j++] = -1;
  
  improvement_iterate(pimprove) {
    if (pimprove->sabotage > 0) {
      ListBox_AddString(lb,city_improvement_name_translation(pcity, pimprove));
      improvement_type[j++] = improvement_index(pimprove);
    }  
  } improvement_iterate_end;

  if(j > 1) {
    ListBox_AddString(lb,_("At Spy's Discretion"));
    improvement_type[j++] = B_LAST;
    } else {
    improvement_type[0] = B_LAST; 
    /* fake "discretion", since must be production */
  }
  
  
}

/****************************************************************

*****************************************************************/
static LONG CALLBACK spy_sabotage_proc(HWND dlg,UINT message,WPARAM wParam,
				       LPARAM lParam)
{
  switch(message)
    {
    case WM_CREATE:
    case WM_SIZE:
    case WM_GETMINMAXINFO:
      break;
    case WM_DESTROY:
      spy_sabotage_dialog=NULL;
      process_diplomat_arrival(NULL, 0);
      break;
    case WM_CLOSE:
      DestroyWindow(dlg);
      break;
    case WM_COMMAND:
      switch(LOWORD(wParam)) {
      case ID_SPY_LIST:
	if (ListBox_GetCurSel(GetDlgItem(dlg,ID_SPY_LIST))!=LB_ERR) {
	  EnableWindow(GetDlgItem(dlg,IDOK),TRUE);
	}
	break;
      case IDOK:
	{
	  int sabotage_improvement;
	  sabotage_improvement=ListBox_GetCurSel(GetDlgItem(dlg,ID_SPY_LIST));
	  if (sabotage_improvement==LB_ERR)
	    break;
	  sabotage_improvement=improvement_type[sabotage_improvement];
	  if(game_find_unit_by_number(diplomat_id) && 
	     game_find_city_by_number(diplomat_target_id)) { 
	    request_diplomat_action(DIPLOMAT_SABOTAGE, diplomat_id,
				    diplomat_target_id,
				    sabotage_improvement + 1);
	  }
	  
	}
      case IDCANCEL:
	DestroyWindow(dlg);
	break;
      }
      break;
    default:
      return DefWindowProc(dlg,message,wParam,lParam);
    }
  return 0;
}

/****************************************************************
 Pops-up the Spy sabotage dialog, upon return of list of
 available improvements requested by the above function.
*****************************************************************/
void popup_sabotage_dialog(struct city *pcity)
{
  
  if(!spy_sabotage_dialog){
    HWND lb;
    struct fcwin_box *hbox;
    struct fcwin_box *vbox;
    spy_sabotage_dialog=fcwin_create_layouted_window(spy_sabotage_proc,
						     _("Sabotage Improvements"),
						     WS_OVERLAPPEDWINDOW,
						     CW_USEDEFAULT,
						     CW_USEDEFAULT,
						     root_window,
						     NULL,
						     REAL_CHILD,
						     NULL);
    hbox=fcwin_hbox_new(spy_sabotage_dialog,TRUE);
    vbox=fcwin_vbox_new(spy_sabotage_dialog,FALSE);
    fcwin_box_add_static(vbox,_("Select Improvement to Sabotage"),
			 0,SS_LEFT,FALSE,FALSE,5);
    lb=fcwin_box_add_list(vbox,5,ID_SPY_LIST,WS_VSCROLL,TRUE,TRUE,5);
    fcwin_box_add_button(hbox,_("Close"),IDCANCEL,0,TRUE,TRUE,10);
    fcwin_box_add_button(hbox,_("Sabotage"),IDOK,0,TRUE,TRUE,10);
    EnableWindow(GetDlgItem(spy_sabotage_dialog,IDOK),FALSE);
    fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
    create_improvements_list(client.conn.playing, pcity, lb);
    fcwin_set_box(spy_sabotage_dialog,vbox);
    ShowWindow(spy_sabotage_dialog,SW_SHOWNORMAL);
  }
}

/****************************************************************
...
*****************************************************************/
static void diplomat_incite_yes_callback(HWND w, void * data)
{
  request_diplomat_action(DIPLOMAT_INCITE, diplomat_id,
			  diplomat_target_id, 0);

  destroy_message_dialog(w);

  process_diplomat_arrival(NULL, 0);
}

/****************************************************************
...
*****************************************************************/
static void diplomat_incite_no_callback(HWND w, void * data)
{
  destroy_message_dialog(w);

  process_diplomat_arrival(NULL, 0);
}


/****************************************************************
...  Ask the server how much the revolt is going to cost us
*****************************************************************/
static void diplomat_incite_callback(HWND w, void * data)
{
  destroy_message_dialog(w);
  diplomat_dialog = 0;

  if (game_find_unit_by_number(diplomat_id)
   && game_find_city_by_number(diplomat_target_id)) {
    request_diplomat_answer(DIPLOMAT_INCITE, diplomat_id,
			    diplomat_target_id, 0);
  }
}

/****************************************************************
Popup the yes/no dialog for inciting, since we know the cost now
*****************************************************************/
void popup_incite_dialog(struct city *pcity, int cost)
{
  char buf[128];

  if (INCITE_IMPOSSIBLE_COST == cost) {
    my_snprintf(buf, sizeof(buf), _("You can't incite a revolt in %s."),
		city_name(pcity));
    popup_message_dialog(root_window, _("City can't be incited!"), buf,
			 _("Darn"), diplomat_incite_no_callback, 0, 0);
  } else if (cost <= client.conn.playing->economic.gold) {
    my_snprintf(buf, sizeof(buf),
		_("Incite a revolt for %d gold?\nTreasury contains %d gold."), 
		cost, client.conn.playing->economic.gold);
   diplomat_target_id = pcity->id;
   popup_message_dialog(root_window, /*"diplomatrevoltdialog"*/_("Incite a Revolt!"), buf,
		       _("_Yes"), diplomat_incite_yes_callback, 0,
		       _("_No"), diplomat_incite_no_callback, 0, 0);
  } else {
    my_snprintf(buf, sizeof(buf),
		_("Inciting a revolt costs %d gold.\n"
		  "Treasury contains %d gold."), 
		cost, client.conn.playing->economic.gold);
   popup_message_dialog(root_window, /*"diplomatnogolddialog"*/_("Traitors Demand Too Much!"), buf,
		       _("Darn"), diplomat_incite_no_callback, 0, 
		       0);
  }
}


/****************************************************************
...
*****************************************************************/
static void diplomat_cancel_callback(HWND w, void * data)
{
  destroy_message_dialog(w);
  diplomat_dialog=0;

  process_diplomat_arrival(NULL, 0);
}


/****************************************************************
...
*****************************************************************/
void popup_diplomat_dialog(struct unit *punit, struct tile *ptile)
{
  struct city *pcity;
  struct unit *ptunit;
  HWND shl;
  char buf[128];

  diplomat_id=punit->id;

  if ((pcity = tile_city(ptile))){
    /* Spy/Diplomat acting against a city */

    diplomat_target_id=pcity->id;
    my_snprintf(buf, sizeof(buf),
		_("Your %s has arrived at %s.\nWhat is your command?"),
		unit_name_translation(punit),
		city_name(pcity));

    if(!unit_has_type_flag(punit, F_SPY)){
      shl=popup_message_dialog(root_window, /*"diplomatdialog"*/
			       _("Choose Your Diplomat's Strategy"), buf,
         		     _("Establish _Embassy"), diplomat_embassy_callback, 0,
         		     _("_Investigate City"), diplomat_investigate_callback, 0,
         		     _("_Sabotage City"), diplomat_sabotage_callback, 0,
         		     _("Steal _Technology"), diplomat_steal_callback, 0,
         		     _("Incite a _Revolt"), diplomat_incite_callback, 0,
         		     _("_Cancel"), diplomat_cancel_callback, 0,
         		     0);
      
      if (!diplomat_can_do_action(punit, DIPLOMAT_EMBASSY, ptile))
       message_dialog_button_set_sensitive(shl,0,FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INVESTIGATE, ptile))
       message_dialog_button_set_sensitive(shl,1,FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_SABOTAGE, ptile))
       message_dialog_button_set_sensitive(shl,2,FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_STEAL, ptile))
       message_dialog_button_set_sensitive(shl,3,FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INCITE, ptile))
       message_dialog_button_set_sensitive(shl,4,FALSE);
    }else{
       shl = popup_message_dialog(root_window, /*"spydialog"*/
		_("Choose Your Spy's Strategy"), buf,
 		_("Establish _Embassy"), diplomat_embassy_callback, 0,
		_("_Investigate City"), diplomat_investigate_callback, 0,
		_("_Poison City"), spy_poison_callback,0,
 		_("Industrial _Sabotage"), spy_request_sabotage_list, 0,
 		_("Steal _Technology"), spy_steal_popup, 0,
 		_("Incite a _Revolt"), diplomat_incite_callback, 0,
 		_("_Cancel"), diplomat_cancel_callback, 0,
		0);

      if (!diplomat_can_do_action(punit, DIPLOMAT_EMBASSY, ptile))
       message_dialog_button_set_sensitive(shl,0,FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INVESTIGATE, ptile))
       message_dialog_button_set_sensitive(shl,1,FALSE);
      if (!diplomat_can_do_action(punit, SPY_POISON, ptile))
       message_dialog_button_set_sensitive(shl,2,FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_SABOTAGE, ptile))
       message_dialog_button_set_sensitive(shl,3,FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_STEAL, ptile))
       message_dialog_button_set_sensitive(shl,4,FALSE);
      if (!diplomat_can_do_action(punit, DIPLOMAT_INCITE, ptile))
       message_dialog_button_set_sensitive(shl,5,FALSE);
     }

    diplomat_dialog=shl;
   }else{
     if ((ptunit = unit_list_get(ptile->units, 0))){
       /* Spy/Diplomat acting against a unit */

       diplomat_target_id=ptunit->id;

       shl=popup_message_dialog(root_window, /*"spybribedialog"*/_("Subvert Enemy Unit"),
                              (!unit_has_type_flag(punit, F_SPY))?
 			      _("The diplomat is waiting for your command"):
 			      _("The spy is waiting for your command"),
 			      _("_Bribe Enemy Unit"), diplomat_bribe_callback, 0,
 			      _("_Sabotage Enemy Unit"), spy_sabotage_unit_callback, 0,
 			      _("_Cancel"), diplomat_cancel_callback, 0,
 			      0);

       if (!diplomat_can_do_action(punit, DIPLOMAT_BRIBE, ptile))
        message_dialog_button_set_sensitive(shl,0,FALSE);
       if (!diplomat_can_do_action(punit, SPY_SABOTAGE_UNIT, ptile))
        message_dialog_button_set_sensitive(shl,1,FALSE);
    }
  }
}

/****************************************************************
  Returns id of a diplomat currently handled in diplomat dialog
*****************************************************************/
int diplomat_handled_in_diplomat_dialog(void)
{
  if (diplomat_dialog == 0) {
    return -1;
  }
  return diplomat_id;
}

/****************************************************************
  Closes the diplomat dialog
****************************************************************/
void close_diplomat_dialog(void)
{
  if (diplomat_dialog != 0) {
    destroy_message_dialog(diplomat_dialog);
    diplomat_dialog = 0;
  }
}

/**************************************************************************

**************************************************************************/
static LONG CALLBACK pillage_proc(HWND dlg,UINT message,
				  WPARAM wParam,LPARAM lParam)
{
  int id;
  switch(message) {
  case WM_DESTROY:
    is_showing_pillage_dialog=FALSE;
    break;
  case WM_CLOSE:
    DestroyWindow(dlg);
    break;
  case WM_GETMINMAXINFO:
  case WM_SIZE:
    break;
  case WM_COMMAND:
    id=LOWORD(wParam);
    if (id==IDCANCEL) {
      DestroyWindow(dlg);
    } else if (id >= ID_PILLAGE_BASE) {
      struct unit *punit = game_find_unit_by_number(unit_to_use_to_pillage);
      if (punit) {
        Base_type_id pillage_base = -1;
        int what = id - ID_PILLAGE_BASE;

        if (what > S_LAST) {
          pillage_base = what - S_LAST - 1;
          what = S_LAST;
        }

	request_new_unit_activity_targeted(punit,
					   ACTIVITY_PILLAGE,
					   what, pillage_base);
	DestroyWindow(dlg);
      }
    }
    break;
  default:
    return DefWindowProc(dlg,message,wParam,lParam);
  }
  return 0;
}

/**************************************************************************

**************************************************************************/
void popup_pillage_dialog(struct unit *punit,
			  bv_special may_pillage,
                          bv_bases bases)
{
  HWND dlg;
  struct fcwin_box *vbox;
  int what;
  enum tile_special_type prereq;

  if (!is_showing_pillage_dialog) {
    is_showing_pillage_dialog = TRUE;
    unit_to_use_to_pillage = punit->id;
    dlg=fcwin_create_layouted_window(pillage_proc,_("What To Pillage"),
				     WS_OVERLAPPEDWINDOW,
				     CW_USEDEFAULT,CW_USEDEFAULT,
				     root_window,NULL,
				     REAL_CHILD,
				     NULL);
    vbox=fcwin_vbox_new(dlg,FALSE);
    fcwin_box_add_static(vbox,_("Select what to pillage:"),0,SS_LEFT,
			 FALSE,FALSE,10);
    while ((what = get_preferred_pillage(may_pillage, bases)) != S_LAST) {
      bv_special what_bv;
      bv_bases what_base;

      BV_CLR_ALL(what_bv);
      BV_CLR_ALL(what_base);

      if (what > S_LAST) {
        BV_SET(what_base, what - S_LAST - 1);
      } else {
        BV_SET(what_bv, what);
      }

      fcwin_box_add_button(vbox, get_infrastructure_text(what_bv, what_base),
                           ID_PILLAGE_BASE+what,0,TRUE,FALSE,5);

      if (what > S_LAST) {
        BV_CLR(bases, what - S_LAST - 1);
      } else {
        clear_special(&may_pillage, what);
        prereq = get_infrastructure_prereq(what);
        if (prereq != S_LAST) {
          clear_special(&may_pillage, prereq);
        }
      }
    }
    fcwin_box_add_button(vbox,_("Cancel"),IDCANCEL,0,TRUE,FALSE,5);
    fcwin_set_box(dlg,vbox);
    ShowWindow(dlg,SW_SHOWNORMAL);
  }
}
/**************************************************************************

**************************************************************************/
HWND popup_message_dialog(HWND parent, char *dialogname,
			  const char *text, ...)
{
  int idcount;
  va_list args;
  void (*fcb)(HWND, void *);
  void *data;
  char *name;
  struct message_dialog *md;
  struct message_dialog_button *mb;
  md=fc_malloc(sizeof(struct message_dialog));
  if (!(md->main=fcwin_create_layouted_window(msgdialog_proc,
					      dialogname,
					      WS_OVERLAPPED,
					      CW_USEDEFAULT,
					      CW_USEDEFAULT,
					      parent,
					      NULL,
					      REAL_CHILD,
					      NULL)))
    {
      free(md);
      return NULL;
    }
  md->firstbutton=NULL;
  md->vbox=fcwin_vbox_new(md->main,FALSE);
  va_start (args,text);
  md->label=fcwin_box_add_static_default(md->vbox,text,0,SS_LEFT);
  idcount=POPUP_MESSAGEDLG_IDBASE;
  while ((name=va_arg(args, char *)))
    {
      char *replacepos;
      char converted_name[512];
      sz_strlcpy(converted_name,name);
      replacepos=converted_name;
      while ((replacepos=strchr(replacepos,'_')))
	{
	  replacepos[0]='&';          /* replace _ by & */
	  replacepos++;
	}
      fcb=va_arg(args,void *);
      data=va_arg(args,void *);
      mb=fc_malloc(sizeof(struct message_dialog_button));
      mb->buttwin=fcwin_box_add_button_default(md->vbox,converted_name,
					       idcount,0);
      mb->id=idcount;
      mb->data=data;
      mb->callback=fcb;
      idcount++;
      mb->next=md->firstbutton;
      md->firstbutton=mb;
    }
  fcwin_set_user_data(md->main,md);
  fcwin_set_box(md->main,md->vbox);
 
  ShowWindow(md->main,SW_SHOWNORMAL);
  return md->main;
}


void destroy_message_dialog(HWND dlg)
{
  DestroyWindow(dlg);
}

void message_dialog_button_set_sensitive(HWND dlg,int id,int state)
{
  
  EnableWindow(GetDlgItem(dlg, POPUP_MESSAGEDLG_IDBASE+id),state);
  
}

static LONG APIENTRY msgdialog_proc (
                           HWND hWnd,
                           UINT message,
                           UINT wParam,
                           LONG lParam)
{
  struct message_dialog *md;
  struct message_dialog_button *mb;
  int id;
  switch (message)
    {
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;
    case WM_DESTROY:      {
      struct message_dialog *md;
      struct message_dialog_button *mb;
      md=(struct message_dialog *)fcwin_get_user_data(hWnd);
      mb=md->firstbutton;
      while (mb)
	{
	  struct message_dialog_button *mbprev;
	  
	  mbprev=mb;
	  mb=mb->next;
	  free(mbprev);
	}
      free(md);
    }
      break;
    case WM_SIZE:
      break;
    case WM_GETMINMAXINFO:
      break;
    case WM_COMMAND:
      id=LOWORD(wParam);
      md=(struct message_dialog *)fcwin_get_user_data(hWnd);
      mb=md->firstbutton;
      while(mb)
	{
	  if (mb->id==id)
	    {
	      mb->callback(hWnd,mb->data);
	      break;
	    }
	  mb=mb->next;
	}
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
    }
  return (0);  
  
}

/**************************************************************************
  Ruleset (modpack) has suggested loading certain tileset. Confirm from
  user and load.
**************************************************************************/
void popup_tileset_suggestion_dialog(void)
{
}

/**************************************************************************
  Tileset (modpack) has suggested loading certain theme. Confirm from
  user and load.
**************************************************************************/
bool popup_theme_suggestion_dialog(const char *theme_name)
{
  /* Don't load */
  return FALSE;
}
