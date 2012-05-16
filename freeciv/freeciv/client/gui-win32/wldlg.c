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
#include <commctrl.h>  

/* common & utility */
#include "city.h"
#include "fcintl.h"
#include "game.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "inputdlg.h"
#include "mem.h"
#include "packets.h"
#include "worklist.h"
#include "support.h"
#include "log.h"

/* client */
#include "client_main.h"
#include "climisc.h"

#include "wldlg.h"
#include "citydlg.h"


typedef int wid;

/* 
 * A worklist id (wid) can hold all objects which can be part of a
 * city worklist: improvements (with wonders), units and global
 * worklists. This is achieved by seperation the value set: 
 *  - (wid < B_LAST) denotes a improvement (including wonders)
 *  - (B_LAST <= wid < B_LAST + U_LAST) denotes a unit with the
 *  unit_type_id of (wid - B_LAST)
 *  - (B_LAST + U_LAST<= wid) denotes a global worklist with the id of
 *  (wid - (B_LAST + U_LAST))
 *
 * This used to be used globally but has been moved into the GUI code
 * because it is of limited usefulness.
 */

#define WORKLIST_END (-1)

/**************************************************************************
...
**************************************************************************/
static wid wid_encode(bool is_unit, bool is_worklist, int id)
{
  assert(!is_unit || !is_worklist);

  if (is_unit) {
    return id + B_LAST;
  }
  if (is_worklist) {
    return id + B_LAST + U_LAST;
  }
  return id;
}

/**************************************************************************
...
**************************************************************************/
static bool wid_is_unit(wid wid)
{
  assert(wid != WORKLIST_END);

  return (wid >= B_LAST && wid < B_LAST + U_LAST);
}

/**************************************************************************
...
**************************************************************************/
static bool wid_is_worklist(wid wid)
{
  assert(wid != WORKLIST_END);

  return (wid >= B_LAST + U_LAST);
}

/**************************************************************************
...
**************************************************************************/
static int wid_id(wid wid)
{
  assert(wid != WORKLIST_END);

  if (wid >= B_LAST + U_LAST) {
    return wid - (B_LAST + U_LAST);
  }
  if (wid >= B_LAST) {
    return wid - B_LAST;
  }
  return wid;
}

/**************************************************************************
 Collect the wids of all possible targets of the given city.
**************************************************************************/
static int collect_wids1(wid * dest_wids, struct city *pcity, bool wl_first, 
			 bool advanced_tech)
{
  struct universal targets[MAX_NUM_PRODUCTION_TARGETS];
  struct item items[MAX_NUM_PRODUCTION_TARGETS];
  int item, targets_used, wids_used = 0;

  /* Fill in the global worklists now?                      */
  /* perhaps judicious use of goto would be good here? -mck */
  if (wl_first && client.worklists[0].is_valid && pcity) {
    int i;

    for (i = 0; i < MAX_NUM_WORKLISTS; i++) {
      if (client.worklists[i].is_valid) {
	dest_wids[wids_used] = wid_encode(FALSE, TRUE, i);
	wids_used++;
      }
    }
  }

  /* Fill in improvements and units */
  targets_used = collect_eventually_buildable_targets(targets, pcity, advanced_tech);
  name_and_sort_items(targets, targets_used, items, FALSE, pcity);

  for (item = 0; item < targets_used; item++) {
    struct universal target = items[item].item;

    if (VUT_UTYPE == target.kind) {
      dest_wids[wids_used] = wid_encode(TRUE, FALSE, utype_number(target.value.utype));
    } else {
      dest_wids[wids_used] = wid_encode(FALSE, FALSE, improvement_number(target.value.building));
    }
    wids_used++;
  }

  /* we didn't fill in the global worklists above */
  if (!wl_first && client.worklists[0].is_valid && pcity) {
    int i;

    for (i = 0; i < MAX_NUM_WORKLISTS; i++) {
      if (client.worklists[i].is_valid) {
        dest_wids[wids_used] = wid_encode(FALSE, TRUE, i);
        wids_used++;
      }
    }
  }

  return wids_used;
}


#define COLUMNS                         4
#define BUFFER_SIZE                     100
#define NUM_TARGET_TYPES                3

struct worklist_report {
  HWND win;
  HWND list;
  char worklist_names[MAX_NUM_WORKLISTS][MAX_LEN_NAME];
  char *worklist_names_ptrs[MAX_NUM_WORKLISTS + 1];
  struct worklist *worklist_ptr[MAX_NUM_WORKLISTS];
  int wl_idx;
};
static struct worklist_report *report_dialog;
static int are_worklists_first = 1;

struct worklist_editor {
  HWND win,worklist,avail;
  HWND toggle_show_advanced;
  struct city *pcity;
  struct worklist *pwl;
  void *user_data;
  int changed;
  WorklistOkCallback ok_callback;
  WorklistCancelCallback cancel_callback;
  wid worklist_wids[MAX_LEN_WORKLIST];
  /* maps from slot to wid; last one contains WORKLIST_END */
  wid worklist_avail_wids[B_LAST + U_LAST + MAX_NUM_WORKLISTS + 1];

};

enum wl_report_ids {
  ID_OK=IDOK,
  ID_CANCEL=IDCANCEL,
  ID_LIST=100,
  ID_WORKLIST,
  ID_EDIT,
  ID_RENAME,
  ID_INSERT,
  ID_DELETE,
  ID_FUTURE_TARGETS,
  ID_HELP,
  ID_UP,
  ID_DOWN,
  ID_CLOSE
};

static void global_commit_worklist(struct worklist *pwl, void *data);
static void worklist_copy_to_editor(struct worklist *pwl,
                                    struct worklist_editor *peditor,
                                    int where);
static void worklist_really_insert_item(struct worklist_editor *peditor,
                                        int before, wid wid);
static void worklist_prep(struct worklist_editor *peditor);
static void worklist_list_update(struct worklist_editor *peditor);
static void targets_list_update(struct worklist_editor *peditor);
static struct worklist_editor *create_worklist_editor(struct worklist *pwl,
						      struct city *pcity,
						      void *user_data,
						      WorklistOkCallback ok_cb,
						      WorklistCancelCallback
						      cancel_cb,HWND win);
static void update_worklist_editor(struct worklist_editor *peditor);

/****************************************************************

*****************************************************************/
static void global_list_update(struct worklist_report *preport)
{
  HWND lst;
  int i, n;

  for (i = 0, n = 0; i < MAX_NUM_WORKLISTS; i++) {
    if (client.worklists[i].is_valid) {
      strcpy(preport->worklist_names[n], client.worklists[i].name);
      preport->worklist_names_ptrs[n] = preport->worklist_names[n];
      preport->worklist_ptr[n] = &client.worklists[i];

      n++;
    }
  }

  /* Terminators */
  preport->worklist_names_ptrs[n] = NULL;

  /* now fill the list */

  n = 0;
  lst=GetDlgItem(preport->win,ID_LIST);
  ListBox_ResetContent(lst);
  while (preport->worklist_names_ptrs[n]) {
    ListBox_AddString(lst,preport->worklist_names_ptrs[n]);
    n++;
  }
}

/****************************************************************
  Remove the current worklist.  This request is made by sliding
  up all lower worklists to fill in the slot that's being deleted.
*****************************************************************/
static void global_delete_callback(struct worklist_report *preport, int sel)
{
  int i, j;

  /* Look for the last free worklist */
  for (i = 0; i < MAX_NUM_WORKLISTS; i++)
    if (!client.worklists[i].is_valid)
      break;

  for (j = sel; j < i - 1; j++) {
    worklist_copy(&client.worklists[j],
                  &client.worklists[j + 1]);
  }

  /* The last worklist in the set is no longer valid -- it's been slid up
   * one slot. */
  client.worklists[i-1].is_valid = FALSE;
  strcpy(client.worklists[i-1].name, "\0");

  global_list_update(preport);
}

/****************************************************************

*****************************************************************/
static void global_rename_sub_callback(HWND w, void * data)
{
  struct worklist_report *preport = (struct worklist_report *) data;

  if (preport) {
    strncpy(client.worklists[preport->wl_idx].name,
            input_dialog_get_input(w), MAX_LEN_NAME);
    client.worklists[preport->wl_idx].name[MAX_LEN_NAME - 1] = '\0';

    global_list_update(preport);
  }

  input_dialog_destroy(w);
}

/****************************************************************
  Create a new worklist.
*****************************************************************/
static void global_insert_callback(struct worklist_report *preport)
{
  int j;

  /* Find the next free worklist for this player */

  for (j = 0; j < MAX_NUM_WORKLISTS; j++)
    if (!client.worklists[j].is_valid)
      break;

  /* No more worklist slots free.  (!!!Maybe we should tell the user?) */
  if (j == MAX_NUM_WORKLISTS)
    return;

  /* Validate this slot. */
  worklist_init(&client.worklists[j]);
  client.worklists[j].is_valid = TRUE;
  strcpy(client.worklists[j].name, _("empty worklist"));

  global_list_update(preport);
}

/****************************************************************

*****************************************************************/
static LONG CALLBACK global_wl_proc(HWND hwnd,UINT message,
				    WPARAM wParam,LPARAM lParam)
{
  int sel;
  struct worklist_report *preport;
  preport=fcwin_get_user_data(hwnd);
  assert(preport);
  switch(message) {
  case WM_CREATE:
  case WM_SIZE:
  case WM_GETMINMAXINFO:
    break;
  case WM_DESTROY:
    memset(preport, 0, sizeof(*preport));
    free(preport);
    report_dialog=NULL;
    break;
  case WM_CLOSE:
    DestroyWindow(hwnd);
  case WM_COMMAND:
    sel=ListBox_GetCurSel(GetDlgItem(hwnd,ID_LIST));
    switch((enum wl_report_ids)LOWORD(wParam)) {
    case ID_CLOSE:
      DestroyWindow(hwnd);
      break;
    case ID_RENAME:
      if (sel!=LB_ERR) {
	preport->wl_idx=sel;
	input_dialog_create(hwnd,
			    _("Rename Worklist"),
			    _("What should the new name be?"),
			    client.worklists[preport->wl_idx].name,
			    (void *) global_rename_sub_callback,
			    (void *) preport,
			    (void *) global_rename_sub_callback,
			    NULL);
      }
      break;
    case ID_INSERT:
      global_insert_callback(preport);
      break;
    case ID_DELETE:
      if (sel!=LB_ERR)
	global_delete_callback(preport,sel);
      break;
    case ID_EDIT:
      if (sel!=LB_ERR) {
	struct worklist_window_init init;
	preport->wl_idx = sel;
	init.pwl = preport->worklist_ptr[preport->wl_idx];
	init.pcity = NULL;
	init.parent = hwnd;
	init.user_data = preport;
	init.ok_cb = global_commit_worklist;
	init.cancel_cb = NULL;
	popup_worklist(&init);
      }
      break;
    default:
      break;
    }
    break;
  default:
    return DefWindowProc(hwnd,message,wParam,lParam);
  }
  return 0;
}

/****************************************************************
  Bring up the global worklist report.
*****************************************************************/
void popup_worklists_report(void)
{
  struct fcwin_box *vbox;
  struct fcwin_box *hbox;
  if (report_dialog && report_dialog->win)
    return;
  assert(!report_dialog);
  report_dialog = fc_malloc(sizeof(struct worklist_report));
  report_dialog->win=fcwin_create_layouted_window(global_wl_proc,
						  _("Edit worklists"),
						  WS_OVERLAPPEDWINDOW,
						  CW_USEDEFAULT,
						  CW_USEDEFAULT,
						  root_window,
						  NULL,
						  JUST_CLEANUP,
						  report_dialog);
  vbox=fcwin_vbox_new(report_dialog->win,FALSE);
  hbox=fcwin_hbox_new(report_dialog->win,TRUE);
  fcwin_box_add_static(vbox,_("Available worklists"),0,SS_LEFT,
		       FALSE,FALSE,0);
  fcwin_box_add_list(vbox,7,ID_LIST,0,TRUE,TRUE,0);
  fcwin_box_add_button(hbox,_("Close"),ID_CLOSE,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Edit"),ID_EDIT,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Rename"),ID_RENAME,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Insert"),ID_INSERT,0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Delete"),ID_DELETE,0,TRUE,TRUE,5);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,5);
  fcwin_set_box(report_dialog->win,vbox);
  
  /*  - Update the worklists and clist. */
  global_list_update(report_dialog);
  
  ShowWindow(report_dialog->win,SW_SHOWNORMAL);
}            

/****************************************************************

****************************************************************/
void
update_worklist_report_dialog(void)
{
  if (report_dialog) {
    global_list_update(report_dialog);
  }

}
/****************************************************************
 ...
*****************************************************************/
static void worklist_help(int id, bool is_unit)
{
  if (id >= 0) {
    if (is_unit) {
      popup_help_dialog_typed(utype_name_translation(utype_by_number(id)), HELP_UNIT);
    } else if (is_great_wonder(improvement_by_number(id))) {
      popup_help_dialog_typed(improvement_name_translation(improvement_by_number(id)), HELP_WONDER);
    } else {
      popup_help_dialog_typed(improvement_name_translation(improvement_by_number(id)), HELP_IMPROVEMENT);
    }
  } else
    popup_help_dialog_string(HELP_WORKLIST_EDITOR_ITEM);
}

/****************************************************************

*****************************************************************/
static void worklist_swap_entries(int i, int j,
                                  struct worklist_editor *peditor)
{
  int id = peditor->worklist_wids[i];

  peditor->worklist_wids[i] = peditor->worklist_wids[j];
  peditor->worklist_wids[j] = id;
}


/****************************************************************

****************************************************************/
static void global_commit_worklist(struct worklist *pwl, void *data)
{
  struct worklist_report *preport = (struct worklist_report *) data;
  
  worklist_copy(&client.worklists[preport->wl_idx], pwl);
}

/****************************************************************
 copies a worklist back from the editor
*****************************************************************/
static void copy_editor_to_worklist(struct worklist_editor *peditor,
                                    struct worklist *pwl)
{
  int i, n;

  /* Fill in this worklist with the parameters set in the worklist dialog. */
  worklist_init(pwl);

  n = 0;
  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    if (peditor->worklist_wids[i] == WORKLIST_END) {
      break;
    } else {
      wid wid = peditor->worklist_wids[i];
      struct universal prod =
        universal_by_number(wid_is_unit(wid) ? VUT_UTYPE : VUT_IMPROVEMENT,
                               wid_id(wid));

      assert(!wid_is_worklist(wid));
      worklist_append(pwl, prod);
    }
  }
  strcpy(pwl->name, peditor->pwl->name);
  pwl->is_valid = peditor->pwl->is_valid;
}

/****************************************************************
  User wants to save the worklist.
*****************************************************************/
static void worklist_ok_callback(struct worklist_editor *peditor)
{
  struct worklist wl;

  copy_editor_to_worklist(peditor, &wl);

  /* Invoke the dialog's parent-specified callback */
  if (peditor->ok_callback)
    (*peditor->ok_callback) (&wl, peditor->user_data);

  peditor->changed = 0;
  /*  update_changed_sensitive(peditor); */
  if (peditor->pcity == NULL) 
    DestroyWindow(peditor->win);
}

/****************************************************************
  User cancelled from the Worklist dialog or hit Undo.
*****************************************************************/
static void worklist_no_callback(struct worklist_editor *peditor)
{
  /* Invoke the dialog's parent-specified callback */
  if (peditor->cancel_callback)
    (*peditor->cancel_callback) (peditor->user_data);

  peditor->changed = 0;
  /* update_changed_sensitive(peditor); */

  if (peditor->pcity == NULL) {
    DestroyWindow(peditor->win);
  } else {
    worklist_prep(peditor);
    worklist_list_update(peditor);
  }
}

/****************************************************************

****************************************************************/
static void get_selected(struct worklist_editor *peditor,
			 int *worklist_sel, int *avail_sel)
{
  int i,n;
  n=ListView_GetItemCount(peditor->worklist);
  for(i=0;i<n;i++) {
    if (ListView_GetItemState(peditor->worklist,i,LVIS_SELECTED)) {
      *worklist_sel=i;
      break;
    }
  }
  n=ListView_GetItemCount(peditor->avail);
  for(i=0;i<n;i++) {
    if (ListView_GetItemState(peditor->avail,i,LVIS_SELECTED)) {
      *avail_sel=i;
      break;
    }
  }
}

/****************************************************************
  User asked for help from the Worklist dialog.  If there's 
  something highlighted, bring up the help for that item.  Else,
  bring up help for improvements.
*****************************************************************/
static void targets_help_callback(struct worklist_editor *peditor,
				  int sel)
{
  int id;
  bool is_unit = FALSE;

  if (sel>=0) {
    wid wid =
      peditor->worklist_avail_wids[sel];

    if (wid_is_worklist(wid)) {
      id = -1;
    } else {
      id = wid_id(wid);
      is_unit = wid_is_unit(wid);
    }
  } else {
    id = -1;
  }

  worklist_help(id, is_unit);
}


/****************************************************************
 does the UI work of inserting a target into the worklist
 also inserts a global worklist straight in.
*****************************************************************/
static void worklist_insert_item(struct worklist_editor *peditor,
				 int wl_sel, int avail_sel)
{
  int where, len;
  wid wid;
  
  if (wl_sel<0) {
    where=MAX_LEN_WORKLIST;
  } else {
    where=wl_sel;
  }
  wid = peditor->worklist_avail_wids[avail_sel];
  
  /* target is a global worklist id */
  if (wid_is_worklist(wid)) {
    struct worklist *pwl = &client.worklists[wid_id(wid)];

    worklist_copy_to_editor(pwl, peditor, where);
    where += worklist_length(pwl);
  } else {
    worklist_really_insert_item(peditor, where, wid);
    where++;
  }

  /* Update the list with the actual data */
  worklist_list_update(peditor);
  
  /* How long is the new worklist? */
  for (len = 0; len < MAX_LEN_WORKLIST; len++)
    if (peditor->worklist_wids[len] == WORKLIST_END)
      break;
  
  /* Re-select the item that was previously selected. */
  if (where < len) {
    ListView_SetItemState(peditor->worklist,where,
			  LVIS_SELECTED,LVIS_SELECTED);
  }
}

static void worklist_remove_item(struct worklist_editor *peditor, int row)
{
  int i, j;
  /* Find the last element in the worklist */
  for (i = 0; i < MAX_LEN_WORKLIST; i++)
    if (peditor->worklist_wids[i] == WORKLIST_END)
      break;

  /* Slide all the later elements in the worklist up. */
  for (j = row; j < i; j++) {
    peditor->worklist_wids[j] = peditor->worklist_wids[j + 1];
  }
  
  i--;
  peditor->worklist_wids[i] = WORKLIST_END;
  
  /* Update the list with the actual data */
  worklist_list_update(peditor);
  /* Select the item immediately after the item we just deleted,
     if there is such an item. */
  if (row < i) {
    ListView_SetItemState(peditor->worklist,row,LVIS_SELECTED,LVIS_SELECTED);
  }

}

/****************************************************************

****************************************************************/
LONG CALLBACK worklist_editor_proc(HWND hwnd,UINT message,WPARAM wParam,
				   LPARAM lParam)
{
  int avail_sel;
  int wl_sel;

  struct worklist_editor *peditor;
  peditor=fcwin_get_user_data(hwnd);
  wl_sel=-1;
  avail_sel=-1;
 
  switch(message) {
  case WM_CREATE:
    {
      struct worklist_window_init *init;
      init = fcwin_get_user_data(hwnd);
      peditor = create_worklist_editor(init->pwl, init->pcity, init->user_data,
				       init->ok_cb, init->cancel_cb, hwnd);
      update_worklist_editor(peditor);
    }
    break;
  case WM_SIZE:
  case WM_GETMINMAXINFO:
    break;
  case WM_DESTROY:
    if (peditor) {
      memset(peditor, 0, sizeof(*peditor));
      free(peditor);
    }
    fcwin_set_user_data(hwnd, NULL);
    break;
  case WM_CLOSE:
    DestroyWindow(hwnd);
    break;
  case WM_COMMAND:
    if (peditor == NULL)
      break;
    get_selected(peditor,&wl_sel,&avail_sel);
    switch((enum wl_report_ids)LOWORD(wParam)) {
    case ID_CANCEL:
      worklist_no_callback(peditor);
      break;
    case ID_OK:
      worklist_ok_callback(peditor);   
      break; 
    case ID_FUTURE_TARGETS:
      targets_list_update(peditor);
      break;
    case ID_DELETE:
      if (wl_sel>=0)
	worklist_remove_item(peditor,wl_sel);
      break;
   case ID_UP:
      if (wl_sel>0) {
	worklist_swap_entries(wl_sel,wl_sel - 1, peditor);
	worklist_list_update(peditor);
      }
      break;
    case ID_DOWN:
      if ((wl_sel>=0)&&(wl_sel!=MAX_LEN_WORKLIST-1)&&
	  (peditor->worklist_wids[wl_sel+1] !=WORKLIST_END)) {
	worklist_swap_entries(wl_sel, wl_sel + 1, peditor);
	worklist_list_update(peditor);
      }
      break;
    case ID_HELP:
      targets_help_callback(peditor,avail_sel);
      break;
    default:
      break;
    }
    break;
  case WM_NOTIFY:
    {
      NM_LISTVIEW *nmlv=(NM_LISTVIEW *)lParam;
      if (peditor == NULL)
	break;
      get_selected(peditor,&wl_sel,&avail_sel);
      if ((nmlv->hdr.idFrom==ID_LIST)&&
	  (nmlv->hdr.code==NM_DBLCLK)&&(avail_sel>=0)) {
	worklist_insert_item(peditor,wl_sel,avail_sel);
	
      } else if ((nmlv->hdr.idFrom == ID_WORKLIST)
		 && (nmlv->hdr.code == NM_DBLCLK)
		 && (wl_sel >= 0)) {
	worklist_remove_item(peditor, wl_sel);
      }
    }
    break;
  default:
    return DefWindowProc(hwnd,message,wParam,lParam);
  }
  return 0;
}

/****************************************************************
 Worklist editor
****************************************************************/
static struct worklist_editor *create_worklist_editor(struct worklist *pwl,
						      struct city *pcity,
						      void *user_data,
						      WorklistOkCallback ok_cb,
						      WorklistCancelCallback
						      cancel_cb,HWND win)
{
  struct worklist_editor *peditor;
  struct fcwin_box *hbox;
  struct fcwin_box *vbox;
  struct fcwin_box *hbox2;
  LV_COLUMN lvc;
  int i;

  char *wl_titles[] = { N_("Type"),
			N_("Info"),
			N_("Cost")
  };
  

  char *avail_titles[] = { N_("Type"),
			   N_("Info"),
			   N_("Cost"),
			   N_("Turns")
  };
  
  peditor=fc_malloc(sizeof(struct worklist_editor));
  
  peditor->pcity = pcity;
  peditor->pwl = pwl;
  peditor->user_data = user_data;
  peditor->ok_callback = ok_cb;
  peditor->cancel_callback = cancel_cb;
  peditor->changed = 0;
  
  peditor->win = win;
  fcwin_set_user_data(win, peditor);

  hbox2=fcwin_hbox_new(peditor->win,FALSE);
  vbox=fcwin_vbox_new(peditor->win,FALSE);
  fcwin_box_add_groupbox(hbox2,_("Current worklist"),vbox,SS_LEFT,
		       TRUE,TRUE,0);
  peditor->worklist=fcwin_box_add_listview(vbox, 5, ID_WORKLIST,
					   LVS_REPORT | LVS_SINGLESEL,
					   TRUE,TRUE,0);
  hbox=fcwin_hbox_new(peditor->win,TRUE);
  fcwin_box_add_button(hbox,_("Up"),ID_UP,0,TRUE,TRUE,0);
  fcwin_box_add_button(hbox,_("Down"),ID_DOWN,0,TRUE,TRUE,0);
  fcwin_box_add_button(hbox,_("Remove"),ID_DELETE,0,TRUE,TRUE,0);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,0);
 
  vbox=fcwin_vbox_new(peditor->win,FALSE);
  fcwin_box_add_groupbox(hbox2,_("Available items"),vbox,0,TRUE,TRUE,0);
  peditor->avail=fcwin_box_add_listview(vbox,5,ID_LIST,
					LVS_SINGLESEL | LVS_REPORT,
					TRUE,TRUE,5);
  hbox=fcwin_hbox_new(peditor->win,FALSE);
  peditor->toggle_show_advanced=
    fcwin_box_add_checkbox(hbox,_("Show future targets"),ID_FUTURE_TARGETS,
			   0,TRUE,TRUE,5);
  fcwin_box_add_button(hbox,_("Help"),ID_HELP,0,FALSE,FALSE,5);
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,0);
  vbox=fcwin_vbox_new(peditor->win,FALSE);
  fcwin_box_add_box(vbox,hbox2,TRUE,TRUE,5);
  hbox=fcwin_hbox_new(peditor->win,TRUE);
  if (pcity) {
    fcwin_box_add_button(hbox,_("Undo"), ID_CANCEL,0,TRUE,TRUE,5);
  } else {
    fcwin_box_add_button(hbox,_("Ok"),ID_OK,0,TRUE,TRUE,5);
    fcwin_box_add_button(hbox,_("Cancel"),ID_CANCEL,0,TRUE,TRUE,5);
  }
  fcwin_box_add_box(vbox,hbox,FALSE,FALSE,10);

  lvc.pszText=_(wl_titles[0]);
  lvc.mask=LVCF_TEXT | LVCF_FMT;
  lvc.fmt=LVCFMT_LEFT;
  ListView_InsertColumn(peditor->worklist,0,&lvc);
  for (i=1;i<ARRAY_SIZE(wl_titles);i++) {
    lvc.pszText=_(wl_titles[i]);
    lvc.mask=LVCF_TEXT | LVCF_FMT;
    lvc.fmt=LVCFMT_RIGHT;
    ListView_InsertColumn(peditor->worklist,i,&lvc);
  }
  ListView_SetColumnWidth(peditor->worklist,0,LVSCW_AUTOSIZE);
  for(i=1;i<ARRAY_SIZE(wl_titles);i++) {
    ListView_SetColumnWidth(peditor->worklist,i,
			    LVSCW_AUTOSIZE_USEHEADER); 
  }
  
  lvc.pszText=_(avail_titles[0]);
  lvc.mask=LVCF_TEXT | LVCF_FMT;
  lvc.fmt=LVCFMT_LEFT;
  ListView_InsertColumn(peditor->avail,0,&lvc);  
  
  for (i=1;i<ARRAY_SIZE(avail_titles);i++) {
    lvc.pszText=_(avail_titles[i]);
    lvc.mask=LVCF_TEXT | LVCF_FMT;
    lvc.fmt=LVCFMT_RIGHT;
    ListView_InsertColumn(peditor->avail,i,&lvc);
  }
  
  ListView_SetColumnWidth(peditor->avail,0,LVSCW_AUTOSIZE);
  for(i=1;i<ARRAY_SIZE(wl_titles);i++) {
    ListView_SetColumnWidth(peditor->avail,i,
			    LVSCW_AUTOSIZE_USEHEADER); 
  }
  fcwin_set_box(peditor->win,vbox);
  return peditor;
}     

/****************************************************************
 does the heavy lifting for inserting an item (not a global worklist) 
 into a worklist.
*****************************************************************/
static void worklist_really_insert_item(struct worklist_editor *peditor,
                                        int before, wid wid)
{
  int i, first_free;
  struct universal target =
    universal_by_number(wid_is_unit(wid) ? VUT_UTYPE : VUT_IMPROVEMENT,
                           wid_id(wid));

  assert(!wid_is_worklist(wid));

  /* If this worklist is a city worklist, double check that the city
     really can (eventually) build the target.  We've made sure that
     the list of available targets is okay for this city, but a global
     worklist may try to insert an odd-ball unit or target. */
  if (peditor->pcity
      && !can_city_build_later(peditor->pcity, target)) {
    /* Nope, this city can't build this target, ever.  Don't put it into
       the worklist. */
    return;
  }

  /* Find the first free element in the worklist */
  for (first_free = 0; first_free < MAX_LEN_WORKLIST; first_free++)
    if (peditor->worklist_wids[first_free] == WORKLIST_END)
      break;

  if (first_free >= MAX_LEN_WORKLIST - 1) {
    /* No room left in the worklist! (remember, we need to keep space
       open for the WORKLIST_END sentinel.) */
    return;
  }

  if (first_free < before && before != MAX_LEN_WORKLIST) {
    /* True weirdness. */
    return;
  }

  if (before < MAX_LEN_WORKLIST) {
    /* Slide all the later elements in the worklist down. */
    for (i = first_free; i > before; i--) {
      peditor->worklist_wids[i] = peditor->worklist_wids[i - 1];
    }
  } else {
    /* Append the new id, not insert. */
    before = first_free;
  }
  first_free++;
  peditor->worklist_wids[first_free] = WORKLIST_END;
  peditor->worklist_wids[before] = wid;
}


/*****************************************************************
 copies a worklist to the editor for editing
******************************************************************/
static void worklist_copy_to_editor(struct worklist *pwl,
                                    struct worklist_editor *peditor,
                                    int where)
{
  int i;

  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    struct universal target;

    /* end of list */
    if (!worklist_peek_ith(pwl, &target, i)) {
      break;
    }

    worklist_really_insert_item(peditor, where,
                                wid_encode(VUT_UTYPE == target.kind, FALSE,
					   universal_number(&target)));
    if (where < MAX_LEN_WORKLIST)
      where++;
  }
  /* Terminators */
  while (where < MAX_LEN_WORKLIST) {
    peditor->worklist_wids[where++] = WORKLIST_END;
  }
}


/****************************************************************
 sets aside the first space for "production.value" if in city
*****************************************************************/
static void worklist_prep(struct worklist_editor *peditor)
{
  if (peditor->pcity) {
    peditor->worklist_wids[0] =
        wid_encode(VUT_UTYPE == peditor->pcity->production.kind, FALSE,
                   universal_number(&peditor->pcity->production));
    peditor->worklist_wids[1] = WORKLIST_END;
    worklist_copy_to_editor(&peditor->pcity->worklist, peditor,
                            MAX_LEN_WORKLIST);
  } else {
    peditor->worklist_wids[0] = WORKLIST_END;
    worklist_copy_to_editor(peditor->pwl, peditor, MAX_LEN_WORKLIST);
  }
}

/****************************************************************

*****************************************************************/
static void worklist_list_update(struct worklist_editor *peditor)
{
  int i, n;
  char *row[COLUMNS];
  char buf[COLUMNS][BUFFER_SIZE];

  for (i = 0; i < COLUMNS; i++)
    row[i] = buf[i];

  ListView_DeleteAllItems(peditor->worklist);

  n = 0;

  /* Fill in the rest of the worklist list */
  for (i = 0; n < MAX_LEN_WORKLIST; i++, n++) {
    wid wid = peditor->worklist_wids[i];
    struct universal target;

    if (wid == WORKLIST_END) {
      break;
    }

    assert(!wid_is_worklist(wid));

    target = universal_by_number(wid_is_unit(wid)
                                    ? VUT_UTYPE : VUT_IMPROVEMENT,
                                    wid_id(wid));

    get_city_dialog_production_row(row, BUFFER_SIZE, target,
                                   peditor->pcity);
    fcwin_listview_add_row(peditor->worklist,i,COLUMNS,row);
  }
  
  ListView_SetColumnWidth(peditor->worklist,0,LVSCW_AUTOSIZE);
  for(i=1;i<COLUMNS;i++) {
    ListView_SetColumnWidth(peditor->worklist,i,
			    LVSCW_AUTOSIZE_USEHEADER); 
  }
  
}

/****************************************************************
  Fill in the target arrays in the peditor.
*****************************************************************/
static void targets_list_update(struct worklist_editor *peditor)
{
  int i = 0, wids_used = 0;
  int advanced_tech;
  char *row[COLUMNS];
  char buf[COLUMNS][BUFFER_SIZE];

  /* Is the worklist limited to just the current targets, or */
  /* to any available and future targets?                    */
  advanced_tech = (peditor->toggle_show_advanced &&
                   (Button_GetCheck(peditor->toggle_show_advanced) ==
                   BST_CHECKED));

  wids_used = collect_wids1(peditor->worklist_avail_wids, peditor->pcity,
                            are_worklists_first, advanced_tech);
  peditor->worklist_avail_wids[wids_used] = WORKLIST_END;


  /* fill the gui list */
  for (i = 0; i < COLUMNS; i++)
    row[i] = buf[i];

  ListView_DeleteAllItems(peditor->avail);
 
  for (i = 0;; i++) {
    wid wid = peditor->worklist_avail_wids[i];

    if (wid == WORKLIST_END) {
      break;
    }

    if (wid_is_worklist(wid)) {
      my_snprintf(buf[0], BUFFER_SIZE, "%s",
		  client.worklists[wid_id(wid)].name);
      my_snprintf(buf[1], BUFFER_SIZE, _("Worklist"));
      my_snprintf(buf[2], BUFFER_SIZE, "---");
      my_snprintf(buf[3], BUFFER_SIZE, "---");
    } else {
      struct universal target = 
        universal_by_number(wid_is_unit(wid) ? VUT_UTYPE : VUT_IMPROVEMENT,
                               wid_id(wid));

      get_city_dialog_production_row(row, BUFFER_SIZE, target,
                                     peditor->pcity);
    }
    fcwin_listview_add_row(peditor->avail,i,COLUMNS,row);
  }
  ListView_SetColumnWidth(peditor->avail,0,LVSCW_AUTOSIZE);
  for(i=1;i<COLUMNS;i++) {
    ListView_SetColumnWidth(peditor->avail,i,
			    LVSCW_AUTOSIZE_USEHEADER); 
  }
  fcwin_redo_layout(peditor->win);
}

/****************************************************************
...
*****************************************************************/
static void update_worklist_editor(struct worklist_editor *peditor)
{
  worklist_prep(peditor);
  worklist_list_update(peditor);
  targets_list_update(peditor);
}

/****************************************************************
...
*****************************************************************/
void update_worklist_editor_win(HWND win)
{
  if (fcwin_get_user_data(win) != NULL) {
    update_worklist_editor((struct worklist_editor *)fcwin_get_user_data(win));
  }
}



/****************************************************************

****************************************************************/
void popup_worklist(struct worklist_window_init *init)
{
  HWND win;
  win=fcwin_create_layouted_window(worklist_editor_proc,_("Worklist"),
				   WS_OVERLAPPEDWINDOW,
				   CW_USEDEFAULT,
				   CW_USEDEFAULT,
				   init->parent,
				   NULL,
				   FAKE_CHILD,
				   init);
  ShowWindow(win, SW_SHOWNORMAL); 
}
