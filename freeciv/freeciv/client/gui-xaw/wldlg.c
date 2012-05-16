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

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/AsciiText.h>  
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Toggle.h>     
#include <X11/Xaw/Viewport.h>

/* utility */
#include "fcintl.h"
#include "mem.h"

/* common */
#include "city.h"
#include "packets.h"
#include "worklist.h"

/* client */
#include "client_main.h"
#include "climisc.h"

#include "gui_main.h"
#include "gui_stuff.h"
#include "helpdlg.h"
#include "inputdlg.h"

#include "wldlg.h"

#define WORKLIST_ADVANCED_TARGETS  1
#define WORKLIST_CURRENT_TARGETS   0

/*
  The Worklist Report dialog shows all the global worklists that the
  player has defined.  There can be at most MAX_NUM_WORKLISTS global
  worklists.
  */
struct worklist_report_dialog {
  Widget list;
  struct player *pplr;
  char worklist_names[MAX_NUM_WORKLISTS][MAX_LEN_NAME];
  char *worklist_names_ptrs[MAX_NUM_WORKLISTS+1];
  struct worklist *worklist_ptr[MAX_NUM_WORKLISTS];
  int wl_idx;
};

/*
  The Worklist dialog is the dialog with which the player edits a 
  particular worklist.  The worklist dialog is popped-up from either
  the Worklist Report dialog or from a City dialog.
  */
struct worklist_dialog {
  Widget worklist, avail;
  Widget btn_prepend, btn_insert, btn_delete, btn_up, btn_down;
  Widget toggle_show_advanced;

  Widget shell;

  struct city *pcity;
  struct worklist *pwl;

  void *parent_data;
  WorklistOkCallback ok_callback;
  WorklistCancelCallback cancel_callback;
  
  char *worklist_names_ptrs[MAX_LEN_WORKLIST+1];
  char worklist_names[MAX_LEN_WORKLIST][200];
  int worklist_ids[MAX_LEN_WORKLIST];
  char *worklist_avail_names_ptrs[B_LAST+1+U_LAST+1+MAX_NUM_WORKLISTS+1+1];
  char worklist_avail_names[B_LAST+1+U_LAST+1+MAX_NUM_WORKLISTS+1][200];
  int worklist_avail_ids[B_LAST+1+U_LAST+1+MAX_NUM_WORKLISTS+1];
  int worklist_avail_num_improvements;
  int worklist_avail_num_targets;
};
#define WORKLIST_END (-1)


static Widget worklist_report_shell = NULL;
static struct worklist_report_dialog *report_dialog;


static int uni_id(struct worklist *pwl, int wlinx);

static void worklist_id_to_name(char buf[],
				struct universal production,
				struct city *pcity);

static void rename_worklist_callback(Widget w, XtPointer client_data, 
				     XtPointer call_data);
static void rename_worklist_sub_callback(Widget w, XtPointer client_data, 
					 XtPointer call_data);
static void insert_worklist_callback(Widget w, XtPointer client_data, 
				     XtPointer call_data);
static void delete_worklist_callback(Widget w, XtPointer client_data, 
				     XtPointer call_data);
static void edit_worklist_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data);
static void commit_player_worklist(struct worklist *pwl, void *data);
static void close_worklistreport_callback(Widget w, XtPointer client_data, 
					  XtPointer call_data);
static void populate_worklist_report_list(struct worklist_report_dialog *pdialog);

/* Callbacks for the worklist dialog */
static void worklist_list_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data);
static void worklist_avail_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data);
static void insert_into_worklist(struct worklist_dialog *pdialog, 
				 int before, int id);
static void worklist_prepend_callback(Widget w, XtPointer client_data, 
				     XtPointer call_data);
static void worklist_insert_callback(Widget w, XtPointer client_data, 
				     XtPointer call_data);
static void worklist_insert_common_callback(struct worklist_dialog *pdialog,
					    XawListReturnStruct *retAvail,
					    int where);
static void worklist_delete_callback(Widget w, XtPointer client_data, 
				     XtPointer call_data);
static void worklist_swap_entries(int i, int j, 
				  struct worklist_dialog *pdialog);
static void worklist_up_callback(Widget w, XtPointer client_data, 
				 XtPointer call_data);
static void worklist_down_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data);
static void worklist_ok_callback(Widget w, XtPointer client_data, 
				 XtPointer call_data);
static void worklist_no_callback(Widget w, XtPointer client_data, 
				 XtPointer call_data);
static void worklist_worklist_help_callback(Widget w, XtPointer client_data, 
					  XtPointer call_data);
static void worklist_avail_help_callback(Widget w, XtPointer client_data, 
					  XtPointer call_data);
static void worklist_show_advanced_callback(Widget w, XtPointer client_data, 
					  XtPointer call_data);
static void worklist_help(struct universal production);
static void worklist_populate_worklist(struct worklist_dialog *pdialog);
static void worklist_populate_targets(struct worklist_dialog *pdialog);



/****************************************************************
  Bring up the worklist report.
*****************************************************************/
void popup_worklists_dialog(struct player *pplr) 
{
  Widget wshell, wform, wlabel, wview, button_edit, button_close,
    button_rename, button_insert, button_delete;
/*
  Position x, y;
  Dimension width, height;
*/
  struct worklist_report_dialog *pdialog;
  
  char *dummy_worklist_list[]={ 
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    0
  };


  pdialog = fc_malloc(sizeof(struct worklist_report_dialog));
  pdialog->pplr = pplr;

  /* Report window already open */
  if (worklist_report_shell)
    return;

  I_T(wshell=XtCreatePopupShell("worklistreportdialog", 
				topLevelShellWidgetClass,
				toplevel, NULL, 0));
  
  wform=XtVaCreateManagedWidget("dform", formWidgetClass, wshell, NULL);
  
  I_L(wlabel=XtVaCreateManagedWidget("dlabel", labelWidgetClass, wform, 
				     NULL));

  wview=XtVaCreateManagedWidget("dview", viewportWidgetClass,
				wform,
				XtNfromVert, 
				wlabel,
				NULL);

  pdialog->list=XtVaCreateManagedWidget("dlist", listWidgetClass, 
					wview, 
					XtNforceColumns, 1,
					XtNdefaultColumns,1, 
					XtNlist, 
					(XtArgVal)dummy_worklist_list,
					XtNverticalList, False,
					NULL);

  button_rename = I_L(XtVaCreateManagedWidget("buttonrename",
					      commandWidgetClass,
					      wform,
					      XtNfromVert, 
					      wlabel,
					      XtNfromHoriz, 
					      wview,
					      NULL));

  button_insert = I_L(XtVaCreateManagedWidget("buttoninsertwl",
					      commandWidgetClass,
					      wform,
					      XtNfromVert, 
					      button_rename,
					      XtNfromHoriz, 
					      wview,
					      NULL));


  button_delete = I_L(XtVaCreateManagedWidget("buttondeletewl",
					      commandWidgetClass,
					      wform,
					      XtNfromVert, 
					      button_insert,
					      XtNfromHoriz, 
					      wview,
					      NULL));

  
  button_close = I_L(XtVaCreateManagedWidget("buttonclose",
					     commandWidgetClass,
					     wform,
					     XtNfromVert, 
					     wview,
					     NULL));

  button_edit = I_L(XtVaCreateManagedWidget("buttonedit",
					    commandWidgetClass,
					    wform,
					    XtNfromVert, 
					    wview,
					    XtNfromHoriz,
					    button_close,
					    NULL));

  XtAddCallback(button_rename, XtNcallback, 
		rename_worklist_callback, (XtPointer)pdialog);
  XtAddCallback(button_insert, XtNcallback, 
		insert_worklist_callback, (XtPointer)pdialog);
  XtAddCallback(button_delete, XtNcallback, 
		delete_worklist_callback, (XtPointer)pdialog);
  XtAddCallback(button_edit, XtNcallback, 
		edit_worklist_callback, (XtPointer)pdialog);
  XtAddCallback(button_close, XtNcallback, 
		close_worklistreport_callback, (XtPointer)pdialog);

/*
  XtVaGetValues(toplevel, XtNwidth, &width, XtNheight, &height, NULL);
  XtTranslateCoords(toplevel, (Position) width/8, (Position) height/8,
		    &x, &y);
  XtVaSetValues(wshell, XtNx, x, XtNy, y, NULL);
*/
  
  xaw_set_relative_position(toplevel, wshell, 10, 10);
  XtPopup(wshell, XtGrabNone);

  populate_worklist_report_list(pdialog);

  XawListChange(pdialog->list, pdialog->worklist_names_ptrs, 0, 0, False);

  /* force refresh of viewport so the scrollbar is added.
   * Buggy sun athena requires this */
  XtVaSetValues(wview, XtNforceBars, True, NULL);

  worklist_report_shell = wshell;
  report_dialog = pdialog;
}



/*************************************************************************
  Bring up a dialog box to edit the given worklist.  If pcity is
  non-NULL, then use pcity to determine the set of units and improvements
  that can be made.  Otherwise, just list everything that technology
  will allow.
*************************************************************************/
Widget popup_worklist(struct worklist *pwl, struct city *pcity,
		      Widget parent, void *parent_data,
		      WorklistOkCallback ok_cb,
		      WorklistCancelCallback cancel_cb)
{
  struct worklist_dialog *pdialog;

  Widget cshell, cform;
  Widget aform;
  Widget worklist_label, avail_label, show_advanced_label;
  Widget worklist_view, avail_view;
  Widget button_form, button_ok, button_cancel, button_worklist_help, 
    button_avail_help;

  char *dummy_worklist[]={ 
    "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    0
  };


  pdialog = fc_malloc(sizeof(struct worklist_dialog));

  pdialog->pcity = pcity;
  pdialog->pwl = pwl;
  pdialog->shell = parent;
  pdialog->parent_data = parent_data;
  pdialog->ok_callback = ok_cb;
  pdialog->cancel_callback = cancel_cb;

  XtSetSensitive(parent, False);
  I_T(cshell=XtCreatePopupShell("worklistdialog", transientShellWidgetClass,
				parent, NULL, 0));
  
  cform=XtVaCreateManagedWidget("cform", formWidgetClass, cshell, NULL);
  
  I_L(worklist_label=XtVaCreateManagedWidget("worklistlabel", 
					     labelWidgetClass, 
					     cform, 
					     NULL));

  worklist_view=XtVaCreateManagedWidget("wview", viewportWidgetClass,
					cform,
					XtNfromVert, 
					worklist_label,
					NULL);

  pdialog->worklist=XtVaCreateManagedWidget("wlist", listWidgetClass, 
					    worklist_view, 
					    XtNfromHoriz, NULL,
					    XtNfromVert, NULL,
					    XtNforceColumns, 1,
					    XtNdefaultColumns,1, 
					    XtNlist, 
					    (XtArgVal)dummy_worklist,
					    XtNverticalList, False,
					    NULL);

  button_form=XtVaCreateManagedWidget("dform", formWidgetClass, 
				      cform,
				      XtNborderWidth, 0,
				      XtNfromVert, worklist_label,
				      XtNfromHoriz, pdialog->worklist,
				      NULL);

  pdialog->btn_prepend = I_L(XtVaCreateManagedWidget("buttonprepend",
						    commandWidgetClass,
						    button_form,
						    NULL));


  pdialog->btn_insert = I_L(XtVaCreateManagedWidget("buttoninsert",
						    commandWidgetClass,
						    button_form,
						    XtNfromVert, 
						    pdialog->btn_prepend,
						    NULL));


  pdialog->btn_delete = I_L(XtVaCreateManagedWidget("buttondelete",
						    commandWidgetClass,
						    button_form,
						    XtNfromVert, 
						    pdialog->btn_insert,
						    NULL));


  pdialog->btn_up = I_L(XtVaCreateManagedWidget("buttonup",
						commandWidgetClass,
						button_form,
						XtNfromVert, 
						pdialog->btn_delete,
						NULL));


  pdialog->btn_down = I_L(XtVaCreateManagedWidget("buttondown",
						  commandWidgetClass,
						  button_form,
						  XtNfromVert,
						  pdialog->btn_up,
						  NULL));

  aform = XtVaCreateManagedWidget("aform", formWidgetClass,
				  cform,
				  XtNborderWidth, 0,
				  XtNfromHoriz, button_form,
				  NULL);

  I_L(avail_label=XtVaCreateManagedWidget("availlabel", labelWidgetClass, 
					  aform, 
					  NULL));

  avail_view=XtVaCreateManagedWidget("aview", viewportWidgetClass,
				aform,
				XtNfromVert, 
				avail_label,
				NULL);

  pdialog->avail=XtVaCreateManagedWidget("alist", listWidgetClass, 
					 avail_view, 
					 XtNforceColumns, 1,
					 XtNdefaultColumns,1, 
					 XtNlist, 
					 (XtArgVal)dummy_worklist,
					 XtNverticalList, False,
					 NULL);
  


  button_ok = I_L(XtVaCreateManagedWidget("buttonok",
					  commandWidgetClass,
					  cform,
					  XtNfromVert, 
					  worklist_view,
					  XtNfromHoriz,
					  False,
					  NULL));

  button_cancel = I_L(XtVaCreateManagedWidget("buttoncancel",
					      commandWidgetClass,
					      cform,
					      XtNfromVert, 
					      worklist_view,
					      XtNfromHoriz,
					      button_ok,
					      NULL));

  button_worklist_help = I_L(XtVaCreateManagedWidget("buttonworklisthelp",
						     commandWidgetClass,
						     cform,
						     XtNfromVert, 
						     worklist_view,
						     XtNfromHoriz,
						     button_cancel,
						     NULL));

  button_avail_help = I_L(XtVaCreateManagedWidget("buttonavailhelp",
						   commandWidgetClass,
						   aform,
						   XtNfromVert, 
						   avail_view,
						   NULL));
  
  show_advanced_label = I_L(XtVaCreateManagedWidget("showadvancedlabel",
						    labelWidgetClass,
						    aform,
						    XtNfromVert,
						    avail_view,
						    XtNfromHoriz,
						    button_avail_help,
						    NULL));

  pdialog->toggle_show_advanced = I_L(XtVaCreateManagedWidget("buttonshowadvanced",
						     toggleWidgetClass,
						     aform,
						     XtNfromVert,
						     avail_view,
						     XtNfromHoriz,
						     show_advanced_label,
						     XtNstate,
						     WORKLIST_CURRENT_TARGETS,
						     NULL));
  
  XtAddCallback(pdialog->worklist, XtNcallback, 
		worklist_list_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->avail, XtNcallback, 
		worklist_avail_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->btn_prepend, XtNcallback, 
		worklist_prepend_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->btn_insert, XtNcallback, 
		worklist_insert_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->btn_delete, XtNcallback, 
		worklist_delete_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->btn_up, XtNcallback, 
		worklist_up_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->btn_down, XtNcallback, 
		worklist_down_callback, (XtPointer)pdialog);
  XtAddCallback(button_ok, XtNcallback, 
		worklist_ok_callback, (XtPointer)pdialog);
  XtAddCallback(button_cancel, XtNcallback, 
		worklist_no_callback, (XtPointer)pdialog);
  XtAddCallback(button_worklist_help, XtNcallback, 
		worklist_worklist_help_callback, (XtPointer)pdialog);
  XtAddCallback(button_avail_help, XtNcallback, 
		worklist_avail_help_callback, (XtPointer)pdialog);
  XtAddCallback(pdialog->toggle_show_advanced, XtNcallback, 
		worklist_show_advanced_callback, (XtPointer)pdialog);
  
  XtSetSensitive(pdialog->btn_prepend, False);
  XtSetSensitive(pdialog->btn_insert, False);
  XtSetSensitive(pdialog->btn_delete, False);
  XtSetSensitive(pdialog->btn_up, False);
  XtSetSensitive(pdialog->btn_down, False);
  
  xaw_set_relative_position(parent, cshell, 0, 50);
  XtPopup(cshell, XtGrabNone);

  worklist_populate_worklist(pdialog);

  /* Update the list with the actual data */
  XawListChange(pdialog->worklist, pdialog->worklist_names_ptrs, 
		0, 0, False);

  /* Fill in the list of available build targets,
     and set correct label in show-advanced toggle. */
  worklist_show_advanced_callback(pdialog->toggle_show_advanced,
				  (XtPointer)pdialog, NULL);

  /* force refresh of viewport so the scrollbar is added.
   * Buggy sun athena requires this */
  XtVaSetValues(worklist_view, XtNforceBars, True, NULL);
  XtVaSetValues(avail_view, XtNforceBars, True, NULL);

  return cshell;
}


/****************************************************************

*****************************************************************/
void update_worklist_report_dialog(void)
{
  /* If the worklist report is open, force its contents to be 
     update. */
  if (report_dialog) {
    populate_worklist_report_list(report_dialog);
    XawListChange(report_dialog->list, 
		  report_dialog->worklist_names_ptrs, 0, 0, False);
  }
}

/****************************************************************
  Returns an unique id for element in units + buildings sequence
*****************************************************************/
int uni_id(struct worklist *pwl, int inx)
{
  if ((inx < 0) || (inx >= worklist_length(pwl))) {
    return WORKLIST_END;
  } else if (VUT_UTYPE == pwl->entries[inx].kind) {
    return utype_number(pwl->entries[inx].value.utype) + B_LAST;
  } else {
    return improvement_number(pwl->entries[inx].value.building);
  }
}

/****************************************************************

*****************************************************************/
void worklist_id_to_name(char buf[],
			 struct universal production,
			 struct city *pcity)
{
  if (VUT_UTYPE == production.kind)
    sprintf(buf, "%s (%d)",
	    utype_values_translation(production.value.utype),
	    utype_build_shield_cost(production.value.utype));
  else if (pcity)
    sprintf(buf, "%s (%d)",
	    city_improvement_name_translation(pcity, production.value.building),
	    impr_build_shield_cost(production.value.building));
  else
    sprintf(buf, "%s (%d)",
	    improvement_name_translation(production.value.building),
	    impr_build_shield_cost(production.value.building));
}



/****************************************************************

*****************************************************************/
void rename_worklist_callback(Widget w, XtPointer client_data, 
			      XtPointer call_data)
{
  struct worklist_report_dialog *pdialog;
  XawListReturnStruct *retList;

  pdialog = (struct worklist_report_dialog *)client_data;
  retList = XawListShowCurrent(pdialog->list);

  if (retList->list_index == XAW_LIST_NONE)
    return;

  pdialog->wl_idx = retList->list_index;

  input_dialog_create(worklist_report_shell,
		      "renameworklist",
		      _("What should the new name be?"),
		      client.worklists[pdialog->wl_idx].name,
		      rename_worklist_sub_callback,
		      (XtPointer)pdialog,
		      rename_worklist_sub_callback,
		      (XtPointer)NULL);
}

/****************************************************************

*****************************************************************/
void rename_worklist_sub_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data)
{
  struct worklist_report_dialog *pdialog;

  pdialog = (struct worklist_report_dialog *)client_data;

  if (pdialog) {
    strncpy(client.worklists[pdialog->wl_idx].name,
            input_dialog_get_input(w), MAX_LEN_NAME);
    client.worklists[pdialog->wl_idx].name[MAX_LEN_NAME - 1] = '\0';

    update_worklist_report_dialog();
  }
  
  input_dialog_destroy(w);
}

/****************************************************************
  Create a new worklist.
*****************************************************************/
void insert_worklist_callback(Widget w, XtPointer client_data, 
			      XtPointer call_data)
{
  struct worklist_report_dialog *pdialog;
  int j;

  pdialog = (struct worklist_report_dialog *)client_data;

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

  update_worklist_report_dialog();
}

/****************************************************************
  Remove the current worklist.  This request is made by sliding
  up all lower worklists to fill in the slot that's being deleted.
*****************************************************************/
void delete_worklist_callback(Widget w, XtPointer client_data, 
			      XtPointer call_data)
{
  struct worklist_report_dialog *pdialog;
  XawListReturnStruct *retList;
  int i, j;

  pdialog = (struct worklist_report_dialog *)client_data;
  retList = XawListShowCurrent(pdialog->list);

  if (retList->list_index == XAW_LIST_NONE)
    return;

  /* Look for the last free worklist */
  for (i = 0; i < MAX_NUM_WORKLISTS; i++)
    if (!client.worklists[i].is_valid)
      break;

  for (j = retList->list_index; j < i-1; j++) {
    worklist_copy(&client.worklists[j], 
                  &client.worklists[j+1]);
  }

  /* The last worklist in the set is no longer valid -- it's been slid up
   * one slot. */
  client.worklists[i-1].is_valid = FALSE;
  strcpy(client.worklists[i-1].name, "\0");
  
  update_worklist_report_dialog();
}

/****************************************************************

*****************************************************************/
void edit_worklist_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  struct worklist_report_dialog *pdialog;
  XawListReturnStruct *retList;

  pdialog = (struct worklist_report_dialog *)client_data;
  retList = XawListShowCurrent(pdialog->list);

  if (retList->list_index == XAW_LIST_NONE)
    return;

  pdialog->wl_idx = retList->list_index;

  popup_worklist(pdialog->worklist_ptr[pdialog->wl_idx], NULL, 
		 worklist_report_shell,
		 pdialog, commit_player_worklist, NULL);
}

/****************************************************************
  Commit the changes to the worklist for this player.
*****************************************************************/
void commit_player_worklist(struct worklist *pwl, void *data)
{
  struct worklist_report_dialog *pdialog;

  pdialog = (struct worklist_report_dialog *)data;

  worklist_copy(&client.worklists[pdialog->wl_idx], pwl);
}

/****************************************************************

*****************************************************************/
void close_worklistreport_callback(Widget w, XtPointer client_data, 
				   XtPointer call_data)
{
  XtDestroyWidget(XtParent(XtParent(w)));
  free(report_dialog);
  worklist_report_shell = NULL;
  report_dialog = NULL;
}

/****************************************************************
  Fill in the worklist arrays in the pdialog.
*****************************************************************/
void populate_worklist_report_list(struct worklist_report_dialog *pdialog)
{
  int i, n;

  for (i = 0, n = 0; i < MAX_NUM_WORKLISTS; i++) {
    if (client.worklists[i].is_valid) {
      strcpy(pdialog->worklist_names[n], client.worklists[i].name);
      pdialog->worklist_names_ptrs[n] = pdialog->worklist_names[n];
      pdialog->worklist_ptr[n] = &client.worklists[i];
      n++;
    }
  }
  
  /* Terminators */
  pdialog->worklist_names_ptrs[n] = NULL;
}



/****************************************************************
  User selected one of the worklist items
*****************************************************************/
void worklist_list_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  XawListReturnStruct *ret;
  struct worklist_dialog *pdialog;
  
  pdialog=(struct worklist_dialog *)client_data;
  
  ret = XawListShowCurrent(pdialog->worklist);
  if (ret->list_index == XAW_LIST_NONE) {
    /* Deselected */
    XtSetSensitive(pdialog->btn_delete, False);
    XtSetSensitive(pdialog->btn_up, False);
    XtSetSensitive(pdialog->btn_down, False);
  } else {
    XtSetSensitive(pdialog->btn_delete, True);
    XtSetSensitive(pdialog->btn_up, True);
    XtSetSensitive(pdialog->btn_down, True);
  }
}

/****************************************************************
  User selected one of the available items
*****************************************************************/
void worklist_avail_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  XawListReturnStruct *ret;
  struct worklist_dialog *pdialog;
  
  pdialog=(struct worklist_dialog *)client_data;
  
  ret = XawListShowCurrent(pdialog->avail);
  if (ret->list_index == XAW_LIST_NONE) {
    /* Deselected */
    XtSetSensitive(pdialog->btn_prepend, False);
    XtSetSensitive(pdialog->btn_insert, False);
  } else {
    XtSetSensitive(pdialog->btn_prepend, True);
    XtSetSensitive(pdialog->btn_insert, True);
  }
}

/****************************************************************

*****************************************************************/
void insert_into_worklist(struct worklist_dialog *pdialog,
			  int before, cid cid)
{
  int i, first_free;
  struct universal target = cid_decode(cid);

  /* If this worklist is a city worklist, double check that the city
     really can (eventually) build the target.  We've made sure that
     the list of available targets is okay for this city, but a global
     worklist may try to insert an odd-ball unit or target. */
  if (pdialog->pcity
      && !can_city_build_later(pdialog->pcity, target)) {
    /* Nope, this city can't build this target, ever.  Don't put it into
       the worklist. */
    return;
  }

  /* Find the first free element in the worklist */
  for (first_free = 0; first_free < MAX_LEN_WORKLIST; first_free++)
    if (pdialog->worklist_ids[first_free] == WORKLIST_END)
      break;

  if (first_free >= MAX_LEN_WORKLIST-1)
    /* No room left in the worklist! (remember, we need to keep space
       open for the WORKLIST_END sentinel.) */
    return;

  if (first_free < before && before != MAX_LEN_WORKLIST)
    /* True weirdness. */
    return;

  if (before < MAX_LEN_WORKLIST) {
    /* Slide all the later elements in the worklist down. */
    for (i = first_free; i > before; i--) {
      pdialog->worklist_ids[i] = pdialog->worklist_ids[i-1];
      strcpy(pdialog->worklist_names[i], pdialog->worklist_names[i-1]);
      pdialog->worklist_names_ptrs[i] = pdialog->worklist_names[i];
    }
  } else {
    /* Append the new id, not insert. */
    before = first_free;
  }

  first_free++;
  pdialog->worklist_ids[first_free] = WORKLIST_END;
  pdialog->worklist_names_ptrs[first_free] = NULL;

  pdialog->worklist_ids[before] = cid;
  
  worklist_id_to_name(pdialog->worklist_names[before],
		      target, pdialog->pcity);
  pdialog->worklist_names_ptrs[before] = pdialog->worklist_names[before];
}

/****************************************************************
  Insert the selected build target at the head of the worklist.
*****************************************************************/
void worklist_prepend_callback(Widget w, XtPointer client_data, 
			       XtPointer call_data)
{
  struct worklist_dialog *pdialog = (struct worklist_dialog *)client_data;
  XawListReturnStruct *retAvail = XawListShowCurrent(pdialog->avail);

  worklist_insert_common_callback(pdialog, retAvail, 0);

  if (pdialog->worklist_ids[1] != WORKLIST_END) {
    XtSetSensitive(pdialog->btn_delete, True);
    XtSetSensitive(pdialog->btn_up, True);
    XtSetSensitive(pdialog->btn_down, True);
  }
}

/****************************************************************
  Insert the selected build target into the worklist.
*****************************************************************/
void worklist_insert_callback(Widget w, XtPointer client_data, 
			      XtPointer call_data)
{
  struct worklist_dialog *pdialog = (struct worklist_dialog *)client_data;
  XawListReturnStruct *retList = XawListShowCurrent(pdialog->worklist);
  XawListReturnStruct *retAvail = XawListShowCurrent(pdialog->avail);
  int where;

  /* Insert before some element, or at end? */
  if (retList->list_index == XAW_LIST_NONE)
    where = MAX_LEN_WORKLIST;
  else
    where = retList->list_index;

  worklist_insert_common_callback(pdialog, retAvail, where);
}

/****************************************************************
  Do the actual UI work of inserting a target into the worklist.
*****************************************************************/
void worklist_insert_common_callback(struct worklist_dialog *pdialog,
				     XawListReturnStruct *retAvail,
				     int where)
{
  int target;
  int i, len;
  struct universal production;

  /* Is there anything selected to insert? */
  if (retAvail->list_index == XAW_LIST_NONE)
    return;

  /* Pick out the target and its type. */
  target = pdialog->worklist_avail_ids[retAvail->list_index];

  if (retAvail->list_index >= pdialog->worklist_avail_num_targets) {
    /* target is a global worklist id */
    /* struct player *pplr = city_owner(pdialog->pcity); */
    int wl_idx = pdialog->worklist_avail_ids[retAvail->list_index];
    struct worklist *pwl = &client.worklists[wl_idx];

    for (i = 0; i < MAX_LEN_WORKLIST && uni_id(pwl, i) != WORKLIST_END; i++) {
      insert_into_worklist(pdialog, where, uni_id(pwl, i));
      if (where < MAX_LEN_WORKLIST)
	where++;
    }
  } else if (retAvail->list_index >= 
	     pdialog->worklist_avail_num_improvements) {
    /* target is a unit */
    production.kind = VUT_UTYPE;
    production.value.utype = utype_by_number(target);
    insert_into_worklist(pdialog, where, cid_encode(production));
    where++;
  } else {
    /* target is an improvement or wonder */
    production.kind = VUT_IMPROVEMENT;
    production.value.building = improvement_by_number(target);
    insert_into_worklist(pdialog, where, cid_encode(production));
    where++;
  }

  /* Update the list with the actual data */
  XawListChange(pdialog->worklist, pdialog->worklist_names_ptrs, 
		0, 0, False);

  /* How long is the new worklist? */
  for (len = 0; len < MAX_LEN_WORKLIST; len++)
    if (pdialog->worklist_ids[len] == WORKLIST_END)
      break;

  if (where < len)
    XawListHighlight(pdialog->worklist, where);
}

/****************************************************************
  Remove the selected target in the worklist.
*****************************************************************/
void worklist_delete_callback(Widget w, XtPointer client_data, 
			      XtPointer call_data)
{
  struct worklist_dialog *pdialog = (struct worklist_dialog *)client_data;
  XawListReturnStruct *retList = XawListShowCurrent(pdialog->worklist);
  int i, j, k;

  if (retList->list_index == XAW_LIST_NONE)
    return;

  k = retList->list_index;

  /* Find the last element in the worklist */
  for (i = 0; i < MAX_LEN_WORKLIST; i++)
    if (pdialog->worklist_ids[i] == WORKLIST_END)
      break;

  /* Slide all the later elements in the worklist up. */
  for (j = k; j < i; j++) {
    pdialog->worklist_ids[j] = pdialog->worklist_ids[j+1];
    strcpy(pdialog->worklist_names[j], pdialog->worklist_names[j+1]);
    pdialog->worklist_names_ptrs[j] = pdialog->worklist_names[j];
  }

  i--;
  pdialog->worklist_ids[i] = WORKLIST_END;
  pdialog->worklist_names_ptrs[i] = 0;

  if (i == 0 || k >= i) {
    XtSetSensitive(pdialog->btn_delete, False);
    XtSetSensitive(pdialog->btn_up, False);
    XtSetSensitive(pdialog->btn_down, False);
  }    

  /* Update the list with the actual data */
  XawListChange(pdialog->worklist, pdialog->worklist_names_ptrs, 
		0, 0, False);
  if (k < i)
    XawListHighlight(pdialog->worklist, k);
}


/****************************************************************

*****************************************************************/
void worklist_swap_entries(int i, int j, struct worklist_dialog *pdialog)
{
  int id;
  char name[200];

  id = pdialog->worklist_ids[i];
  strcpy(name, pdialog->worklist_names[i]);

  pdialog->worklist_ids[i] = pdialog->worklist_ids[j];
  strcpy(pdialog->worklist_names[i], pdialog->worklist_names[j]);
  pdialog->worklist_names_ptrs[i] = pdialog->worklist_names[i];

  pdialog->worklist_ids[j] = id;
  strcpy(pdialog->worklist_names[j], name);
  pdialog->worklist_names_ptrs[j] = pdialog->worklist_names[j];
}  

/****************************************************************
  Swap the selected element with its upward neighbor
*****************************************************************/
void worklist_up_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  struct worklist_dialog *pdialog = (struct worklist_dialog *)client_data;
  XawListReturnStruct *retList = XawListShowCurrent(pdialog->worklist);

  if (retList->list_index == XAW_LIST_NONE)
    return;

  if (retList->list_index == 0)
    return;

  worklist_swap_entries(retList->list_index, retList->list_index-1, pdialog);

  XawListChange(pdialog->worklist, pdialog->worklist_names_ptrs, 
		0, 0, False);
  XawListHighlight(pdialog->worklist, retList->list_index-1);
}

/****************************************************************
 Swap the selected element with its downward neighbor
*****************************************************************/
void worklist_down_callback(Widget w, XtPointer client_data, 
			    XtPointer call_data)
{
  struct worklist_dialog *pdialog = (struct worklist_dialog *)client_data;
  XawListReturnStruct *retList = XawListShowCurrent(pdialog->worklist);

  if (retList->list_index == XAW_LIST_NONE)
    return;

  if (retList->list_index == MAX_LEN_WORKLIST-1 ||
      pdialog->worklist_ids[retList->list_index+1] == WORKLIST_END)
    return;

  worklist_swap_entries(retList->list_index, retList->list_index+1, pdialog);

  XawListChange(pdialog->worklist, pdialog->worklist_names_ptrs, 
		0, 0, False);
  XawListHighlight(pdialog->worklist, retList->list_index+1);
}

/****************************************************************
  User wants to save the worklist.
*****************************************************************/
void worklist_ok_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  struct worklist_dialog *pdialog;
  struct worklist wl;
  struct universal production;
  int i;
  
  pdialog=(struct worklist_dialog *)client_data;
  
  /* Fill in this worklist with the parameters set in the worklist 
     dialog. */
  worklist_init(&wl);
  
  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    if (pdialog->worklist_ids[i] == WORKLIST_END) {
      continue;
    } else if (pdialog->worklist_ids[i] >= B_LAST) {
      production.kind = VUT_UTYPE;
      production.value.utype = utype_by_number(pdialog->worklist_ids[i] - B_LAST);
      worklist_append(&wl, production);
    } else if (pdialog->worklist_ids[i] >= 0) {
      production.kind = VUT_IMPROVEMENT;
      production.value.building = improvement_by_number(pdialog->worklist_ids[i]);
      worklist_append(&wl, production);
    } else {
      continue;
    }
  }
  strcpy(wl.name, pdialog->pwl->name);
  wl.is_valid = pdialog->pwl->is_valid;
  
  /* Invoke the dialog's parent-specified callback */
  if (pdialog->ok_callback)
    (*pdialog->ok_callback)(&wl, pdialog->parent_data);

  /* Cleanup. */
  XtDestroyWidget(XtParent(XtParent(w)));
  XtSetSensitive(pdialog->shell, TRUE);
  free(pdialog);
}

/****************************************************************
  User cancelled from the Worklist dialog.
*****************************************************************/
void worklist_no_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  struct worklist_dialog *pdialog;
  
  pdialog=(struct worklist_dialog *)client_data;
  
  /* Invoke the dialog's parent-specified callback */
  if (pdialog->cancel_callback)
    (*pdialog->cancel_callback)(pdialog->parent_data);

  pdialog->worklist = NULL;
  XtDestroyWidget(XtParent(XtParent(w)));
  XtSetSensitive(pdialog->shell, TRUE);
  free(pdialog);
}

/****************************************************************
  User asked for help from the Worklist dialog.  If there's 
  something highlighted, bring up the help for that item.  Else,
  bring up help for improvements.
*****************************************************************/
void worklist_worklist_help_callback(Widget w, XtPointer client_data, 
					    XtPointer call_data)
{
  struct worklist_dialog *pdialog;
  XawListReturnStruct *ret;
  struct universal production;

  pdialog=(struct worklist_dialog *)client_data;

  ret = XawListShowCurrent(pdialog->worklist);
  if(ret->list_index!=XAW_LIST_NONE) {
    cid cid = pdialog->worklist_ids[ret->list_index];
    production = cid_decode(cid);
  } else {
    production.kind = VUT_NONE;
    production.value.building = NULL;
  }

  worklist_help(production);
}

void worklist_avail_help_callback(Widget w, XtPointer client_data, 
					  XtPointer call_data)
{
  struct worklist_dialog *pdialog;
  XawListReturnStruct *ret;
  struct universal production;

  pdialog=(struct worklist_dialog *)client_data;

  ret = XawListShowCurrent(pdialog->avail);
  if(ret->list_index!=XAW_LIST_NONE) {
    if (ret->list_index >= pdialog->worklist_avail_num_targets) {
      /* target is a global worklist id */
      production.kind = VUT_NONE;
      production.value.building = NULL;
    } else {
      production =
        universal_by_number((ret->list_index >= pdialog->worklist_avail_num_improvements)
                               ? VUT_UTYPE : VUT_IMPROVEMENT,
                               pdialog->worklist_avail_ids[ret->list_index]);
    }
  } else {
    production.kind = VUT_NONE;
    production.value.building = NULL;
  }

  worklist_help(production);
}


void worklist_help(struct universal production)
{
  if (production.value.building) {
    if (VUT_UTYPE == production.kind) {
      popup_help_dialog_typed(utype_name_translation(production.value.utype),
					    HELP_UNIT);
    } else if (is_great_wonder(production.value.building)) {
      popup_help_dialog_typed(improvement_name_translation(production.value.building),
						   HELP_WONDER);
    } else {
      popup_help_dialog_typed(improvement_name_translation(production.value.building),
						   HELP_IMPROVEMENT);
    }
  }
  else
    popup_help_dialog_string(HELP_IMPROVEMENTS_ITEM);
}

/**************************************************************************
  Change the label of the Show Advanced toggle.  Also updates the list
  of available targets.
**************************************************************************/
void worklist_show_advanced_callback(Widget w, XtPointer client_data, 
					  XtPointer call_data)
{
  Boolean b;
  struct worklist_dialog *pdialog = (struct worklist_dialog *)client_data;

  XtVaGetValues(w, XtNstate, &b, NULL);
  XtVaSetValues(w, XtNlabel, 
		b==WORKLIST_ADVANCED_TARGETS ? 
		_("Yes") : _("No"), 
		NULL);

  worklist_populate_targets(pdialog);
  XawListChange(pdialog->avail, pdialog->worklist_avail_names_ptrs, 
		0, 0, False);
}



/****************************************************************
  Fill in the worklist arrays in the pdialog.
*****************************************************************/
void worklist_populate_worklist(struct worklist_dialog *pdialog)
{
  int i, n;
  int id;
  struct universal target;

  n = 0;
  if (pdialog->pcity) {
    /* First element is the current build target of the city. */
    id = universal_number(&pdialog->pcity->production);

    worklist_id_to_name(pdialog->worklist_names[n],
			pdialog->pcity->production, pdialog->pcity);

    if (VUT_UTYPE == pdialog->pcity->production.kind)
      id += B_LAST;
    pdialog->worklist_names_ptrs[n] = pdialog->worklist_names[n];
    pdialog->worklist_ids[n] = id;
    n++;
  }

  /* Fill in the rest of the worklist list */
  for (i = 0; n < MAX_LEN_WORKLIST &&
	 uni_id(pdialog->pwl, i) != WORKLIST_END; i++, n++) {
    worklist_peek_ith(pdialog->pwl, &target, i);
    id = uni_id(pdialog->pwl, i);

    worklist_id_to_name(pdialog->worklist_names[n],
			target, pdialog->pcity);

    pdialog->worklist_names_ptrs[n] = pdialog->worklist_names[n];
    pdialog->worklist_ids[n] = id;
  }
  
  /* Terminators */
  pdialog->worklist_names_ptrs[n] = NULL;
  while (n != MAX_LEN_WORKLIST)
    pdialog->worklist_ids[n++] = WORKLIST_END;
}

/****************************************************************
  Fill in the target arrays in the pdialog.
*****************************************************************/
void worklist_populate_targets(struct worklist_dialog *pdialog)
{
  int i, n;
  Boolean b;
  int advanced_tech;
  int can_build, can_eventually_build;
  struct universal production;

  if (!can_client_issue_orders()) {
    return;
  }

  n = 0;

  /* Is the worklist limited to just the current targets, or
     to any available and future targets? */
  XtVaGetValues(pdialog->toggle_show_advanced, XtNstate, &b, NULL);
  if (b == WORKLIST_ADVANCED_TARGETS)
    advanced_tech = True;
  else
    advanced_tech = False;
 
  /*     + First, improvements and Wonders. */
  improvement_iterate(pimprove) {
    /* Can the player (eventually) build this improvement? */
    can_build = can_player_build_improvement_now(client.conn.playing, pimprove);
    can_eventually_build = can_player_build_improvement_later(client.conn.playing, pimprove);

    /* If there's a city, can the city build the improvement? */
    if (pdialog->pcity) {
      can_build = can_build && can_city_build_improvement_now(pdialog->pcity, pimprove);
      can_eventually_build = can_eventually_build &&
	can_city_build_improvement_later(pdialog->pcity, pimprove);
    }
    
    if (( advanced_tech && can_eventually_build) ||
	(!advanced_tech && can_build)) {
      production.kind = VUT_IMPROVEMENT;
      production.value.building = pimprove;
      worklist_id_to_name(pdialog->worklist_avail_names[n],
			  production, pdialog->pcity);
      pdialog->worklist_avail_names_ptrs[n]=pdialog->worklist_avail_names[n];
      pdialog->worklist_avail_ids[n++] = improvement_number(pimprove);
    }
  } improvement_iterate_end;
  pdialog->worklist_avail_num_improvements=n;

  /*     + Second, units. */
  unit_type_iterate(punittype) {
    /* Can the player (eventually) build this improvement? */
    can_build = can_player_build_unit_now(client.conn.playing, punittype);
    can_eventually_build = can_player_build_unit_later(client.conn.playing, punittype);

    /* If there's a city, can the city build the improvement? */
    if (pdialog->pcity) {
      can_build = can_build && can_city_build_unit_now(pdialog->pcity, punittype);
      can_eventually_build = can_eventually_build &&
	can_city_build_unit_later(pdialog->pcity, punittype);
    }

    if (( advanced_tech && can_eventually_build) ||
	(!advanced_tech && can_build)) {
      production.kind = VUT_UTYPE;
      production.value.utype = punittype;
      worklist_id_to_name(pdialog->worklist_avail_names[n],
			  production, pdialog->pcity);
      pdialog->worklist_avail_names_ptrs[n]=pdialog->worklist_avail_names[n];
      pdialog->worklist_avail_ids[n++] = utype_number(punittype);
    }
  } unit_type_iterate_end;

  pdialog->worklist_avail_num_targets=n;

  /*     + Finally, the global worklists. */
  if (pdialog->pcity) {
    /* Now fill in the global worklists. */
    for (i = 0; i < MAX_NUM_WORKLISTS; i++)
      if (client.worklists[i].is_valid) {
	strcpy(pdialog->worklist_avail_names[n], client.worklists[i].name);
	pdialog->worklist_avail_names_ptrs[n]=pdialog->worklist_avail_names[n];
	pdialog->worklist_avail_ids[n++]=i;
      }
  }

  pdialog->worklist_avail_names_ptrs[n]=0;
}
