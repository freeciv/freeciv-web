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
#include <windows.h>
#include <windowsx.h>

#include "fcintl.h"
#include "game.h"
#include "support.h"
#include "options.h"

#include "gui_stuff.h"
#include "gui_main.h"
#include "optiondlg.h"

static HWND option_dialog;

/****************************************************************

*****************************************************************/
static LONG CALLBACK option_proc(HWND dlg,UINT message,
				 WPARAM wParam,LPARAM lParam)
{
  switch(message) {
  case WM_CREATE:
  case WM_SIZE:
  case WM_GETMINMAXINFO:
    break;
  case WM_DESTROY:
    option_dialog=NULL;
    break;
  case WM_COMMAND:
    if (LOWORD(wParam)==IDOK) {
      char dp[512];
      bool b;
      int val;
      
      client_options_iterate(o) {
	switch (o->type) {
	case COT_BOOLEAN:
	  b = *(o->boolean.pvalue);
	  *(o->boolean.pvalue)=(Button_GetCheck((HWND)(o->gui_data))==BST_CHECKED);
	  if (b != *(o->boolean.pvalue) && o->change_callback) {
	    (o->change_callback)(o);
	  }
	  break;
	case COT_INTEGER:
	  val = *(o->integer.pvalue);
	  GetWindowText((HWND)(o->gui_data),dp,sizeof(dp));
	  sscanf(dp, "%d", o->integer.pvalue);
	  if (val != *(o->integer.pvalue) && o->change_callback) {
	    (o->change_callback)(o);
	  }
	  break;
	case COT_STRING:
	  if (!o->gui_data) {
	    break;
	  }
	  GetWindowText((HWND) (o->gui_data), dp,
			sizeof(dp));
	  if (strcmp(dp, o->string.pvalue)) {
	    mystrlcpy(o->string.pvalue, dp, o->string.size);
	    if (o->change_callback) {
	      (o->change_callback)(o);
	    }
	  }
	  break;
        case COT_FONT:
          /* FIXME: */
          break;
	}
      } client_options_iterate_end;
      DestroyWindow(dlg);
    }
    break;
  case WM_CLOSE:
    DestroyWindow(dlg);
    break;
  default:
    return DefWindowProc(dlg,message,wParam,lParam);
  }
  return 0;
}

/****************************************************************
... 
*****************************************************************/
static void create_option_dialog(void)
{
  struct fcwin_box *hbox;
  struct fcwin_box *vbox_labels;
  struct fcwin_box *vbox;
  option_dialog=fcwin_create_layouted_window(option_proc,
					     _("Set local options"),
					     WS_OVERLAPPEDWINDOW,
					     CW_USEDEFAULT,CW_USEDEFAULT,
					     root_window,NULL,
					     JUST_CLEANUP,
					     NULL);

  hbox=fcwin_hbox_new(option_dialog,FALSE);
  vbox=fcwin_vbox_new(option_dialog,TRUE);
  vbox_labels=fcwin_vbox_new(option_dialog,TRUE);
  client_options_iterate(o) {
    switch (o->type) {
    case COT_BOOLEAN:
      fcwin_box_add_static(vbox_labels,_(o->description),
			   0,SS_LEFT,TRUE,TRUE,0);
      o->gui_data=(void *)
	fcwin_box_add_checkbox(vbox," ",0,0,TRUE,TRUE,0);
      break;
    case COT_INTEGER:
      fcwin_box_add_static(vbox_labels,_(o->description),
			   0,SS_LEFT,TRUE,TRUE,0);
      o->gui_data=(void *)
	fcwin_box_add_edit(vbox,"",6,0,0,TRUE,TRUE,0);
      break;
    case COT_STRING:
      fcwin_box_add_static(vbox_labels,_(o->description),
			   0,SS_LEFT,TRUE,TRUE,0);
      if (o->string.val_accessor) {
	const char **vals = (*o->string.val_accessor)();

	if (!vals[0]) {
	  fcwin_box_add_static(vbox, o->string.pvalue, 0, SS_LEFT,
			       TRUE, TRUE, 0);
	  o->gui_data = NULL;
	} else {
	  int j;

	  o->gui_data =
	      fcwin_box_add_combo(vbox, 5, 0,
				  WS_VSCROLL | CBS_DROPDOWNLIST | CBS_SORT,
				  TRUE, TRUE, 0);
	  for (j = 0; vals[j]; j++) {
	    ComboBox_AddString(o->gui_data, vals[j]);
	  }
	}
      } else {
	o->gui_data =
	    (void *) fcwin_box_add_edit(vbox, "", 40, 0, 0, TRUE, TRUE, 0);
	break;
      }
     case COT_FONT:
       /* FIXME: */
       break;
    }
  } client_options_iterate_end;
  fcwin_box_add_box(hbox,vbox_labels,TRUE,TRUE,0);
  fcwin_box_add_box(hbox,vbox,TRUE,TRUE,0);
  vbox=fcwin_vbox_new(option_dialog,FALSE);
  fcwin_box_add_box(vbox,hbox,TRUE,FALSE,10);
  fcwin_box_add_button(vbox,_("Close"),IDOK,0,FALSE,FALSE,5);
  fcwin_set_box(option_dialog,vbox);
}

/****************************************************************
... 
*****************************************************************/
void popup_option_dialog(void)
{
  char valstr[64];

  if (!option_dialog)
    create_option_dialog();

  client_options_iterate(o) {
    switch (o->type) {
    case COT_BOOLEAN:
      Button_SetCheck((HWND)(o->gui_data),
		      (*(o->boolean.pvalue))?BST_CHECKED:BST_UNCHECKED);
      break;
    case COT_INTEGER:
      my_snprintf(valstr, sizeof(valstr), "%d", *(o->integer.pvalue));
      SetWindowText((HWND)(o->gui_data), valstr);
      break;
    case COT_STRING:
      if (!o->gui_data) {
	break;
      }

      if ((o->string.val_accessor) && (o->string.pvalue[0] != 0)) {
	int i =
	    ComboBox_FindStringExact(o->gui_data, 0, o->string.pvalue);

	if (i == CB_ERR) {
	  i = ComboBox_AddString(o->gui_data, o->string.pvalue);
	}
	ComboBox_SetCurSel(o->gui_data, i);
      } 
      SetWindowText((HWND)(o->gui_data), o->string.pvalue);
      break;
    case COT_FONT:
      /* FIXME: */
      break;
    }
  } client_options_iterate_end;
  fcwin_redo_layout(option_dialog);
  ShowWindow(option_dialog,SW_SHOWNORMAL);
}
