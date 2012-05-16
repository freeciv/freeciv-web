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

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Viewport.h>

#include "events.h"
#include "fcintl.h"
#include "game.h"
#include "packets.h"
#include "player.h"
#include "shared.h"
#include "support.h"

#include "chatline.h"
#include "cityrep.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "options.h"

#include "optiondlg.h"

static Widget option_dialog_shell;

/******************************************************************/
void create_option_dialog(void);

void option_ok_command_callback(Widget w, XtPointer client_data, 
			        XtPointer call_data);
void option_cancel_command_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data);

/****************************************************************
... 
*****************************************************************/
void popup_option_dialog(void)
{
  char valstr[64];

  create_option_dialog();

  client_options_iterate(o) {
    switch (o->type) {
    case COT_BOOLEAN:
      XtVaSetValues((Widget) o->gui_data, XtNstate, *(o->boolean.pvalue),
		    XtNlabel, *(o->boolean.pvalue) ? _("Yes") : _("No"), NULL);
      break;
    case COT_INTEGER:
      my_snprintf(valstr, sizeof(valstr), "%d", *(o->integer.pvalue));
      XtVaSetValues((Widget) o->gui_data, XtNstring, valstr, NULL);
      break;
    case COT_STRING:
      XtVaSetValues((Widget) o->gui_data,
		    o->string.val_accessor ? "label" : XtNstring,
		    o->string.pvalue, NULL);
      break;
    case COT_FONT:
      /* FIXME */
      break;
    }
  } client_options_iterate_end;

  xaw_set_relative_position(toplevel, option_dialog_shell, 25, 25);
  XtPopup(option_dialog_shell, XtGrabNone);
  XtSetSensitive(main_form, FALSE);
}



/****************************************************************
  Callback to change the entry for a string option that has
  a fixed list of possible entries.
*****************************************************************/
static void stropt_change_callback(Widget w,
				   XtPointer client_data,
				   XtPointer call_data)
{
  char* val = (char*)client_data;

  XtVaSetValues(XtParent(XtParent(w)), "label", val, NULL);
}

/****************************************************************
...
*****************************************************************/
void create_option_dialog(void)
{
  Widget option_form, option_label;
  Widget option_viewport, option_scrollform;
  Widget option_ok_command, option_cancel_command;
  Widget prev_widget, longest_label = 0;
  size_t longest_len = 0;
  Dimension width;
  
  option_dialog_shell =
    I_T(XtCreatePopupShell("optionpopup", transientShellWidgetClass,
			   toplevel, NULL, 0));

  option_form = XtVaCreateManagedWidget("optionform", 
				        formWidgetClass, 
				        option_dialog_shell, NULL);   

  option_label =
    I_L(XtVaCreateManagedWidget("optionlabel", labelWidgetClass, 
				option_form, NULL));

  option_viewport =
    XtVaCreateManagedWidget("optionviewport", viewportWidgetClass,
			    option_form, NULL);

  option_scrollform =
    XtVaCreateManagedWidget("optionscrollform", formWidgetClass,
			    option_viewport, NULL);   

  prev_widget = NULL; /* init the prev-Widget */
  client_options_iterate(o) {
    const char *descr = _(o->description);
    size_t len = strlen(descr);

    /* 
     * Remember widget so we can reset the vertical position; need to
     * do this because labels and toggles etc have different heights.
     */
    o->gui_data = (void *) prev_widget;

    if (prev_widget) {
      prev_widget = 
	XtVaCreateManagedWidget("label", labelWidgetClass, option_scrollform,
				XtNlabel, descr,
				XtNfromVert, prev_widget,
				NULL);
    } else {
      prev_widget = 
	XtVaCreateManagedWidget("label", labelWidgetClass, option_scrollform,
				XtNlabel, descr,
				NULL);
    }

    /* 
     * The addition of a scrollbar screws things up. There must be a
     * better way to do this.
     */
    XtVaGetValues(prev_widget, XtNwidth, &width, NULL);
    XtVaSetValues(prev_widget, XtNwidth, width + 15, NULL);

    if (len > longest_len) {
      longest_len = len;
      longest_label = prev_widget;
    }
  } client_options_iterate_end;

  XtVaGetValues(longest_label, XtNwidth, &width, NULL);
  XtVaSetValues(option_label, XtNwidth, width + 15, NULL);

  client_options_iterate(o) {
    /* 
     * At the start of the loop o->gui_data will contain the widget
     * which is above the label widget which is associated with this
     * option.
     */
    switch (o->type) {
    case COT_BOOLEAN:
      if (o->gui_data) {
	prev_widget =
	  XtVaCreateManagedWidget("toggle", toggleWidgetClass,
				  option_scrollform,
				  XtNfromHoriz, option_label,
				  XtNfromVert, o->gui_data,
				  NULL);
      } else {
	prev_widget =
	  XtVaCreateManagedWidget("toggle", toggleWidgetClass,
				  option_scrollform,
				  XtNfromHoriz, option_label,
				  NULL);
      }
      XtAddCallback(prev_widget, XtNcallback, toggle_callback, NULL);
      break;
    case COT_STRING:
      if (o->string.val_accessor) {
	int i;
	const char **vals = (*o->string.val_accessor)();
	Widget popupmenu;

	if (o->gui_data) {
	  prev_widget =
	    XtVaCreateManagedWidget(o->name, menuButtonWidgetClass,
				    option_scrollform,
				    XtNfromHoriz, option_label,
				    XtNfromVert, o->gui_data,
				    NULL);
	} else {
	  prev_widget =
	    XtVaCreateManagedWidget(o->name, menuButtonWidgetClass,
				    option_scrollform,
				    XtNfromHoriz, option_label,
				    NULL);
	}

	popupmenu = XtVaCreatePopupShell("menu",
					 simpleMenuWidgetClass,
					 prev_widget,
					 NULL);

	for (i = 0; vals[i]; i++) {
	  Widget entry = XtVaCreateManagedWidget(vals[i], smeBSBObjectClass,
						 popupmenu, NULL);
	  XtAddCallback(entry, XtNcallback, stropt_change_callback,
			(XtPointer)(vals[i]));
	}

	if (i == 0) {
          /* We could disable this if there was just one possible choice,
             too, but for values that are uninitialized (empty) this
             would be confusing. */
	  XtSetSensitive(prev_widget, FALSE);
	}

	/* There should be another way to set width of menu button */
	XtVaSetValues(prev_widget, XtNwidth, 120 ,NULL);

	break;
      }
      /* else fall through */
    case COT_INTEGER:
      if (o->gui_data) {
	prev_widget =
	  XtVaCreateManagedWidget("input", asciiTextWidgetClass,
				  option_scrollform,
				  XtNfromHoriz, option_label,
				  XtNfromVert, o->gui_data,
				  NULL);
      } else {
	prev_widget =
	  XtVaCreateManagedWidget("input", asciiTextWidgetClass,
				  option_scrollform,
				  XtNfromHoriz, option_label,
				  NULL);
      }
      break;
    case COT_FONT:
      /* FIXME */
      break;
    }

    /* store the final widget */
    o->gui_data = (void *) prev_widget;
  } client_options_iterate_end;

  option_ok_command =
    I_L(XtVaCreateManagedWidget("optionokcommand", commandWidgetClass,
				option_form, XtNfromVert, option_viewport,
				NULL));
  
  option_cancel_command =
    I_L(XtVaCreateManagedWidget("optioncancelcommand", commandWidgetClass,
				option_form, XtNfromVert, option_viewport,
				NULL));
	
  XtAddCallback(option_ok_command, XtNcallback, 
		option_ok_command_callback, NULL);
  XtAddCallback(option_cancel_command, XtNcallback, 
		option_cancel_command_callback, NULL);

  XtRealizeWidget(option_dialog_shell);

  xaw_horiz_center(option_label);
}


/**************************************************************************
 Changes the label of the toggle widget to Yes/No depending on the state of
 the toggle.
**************************************************************************/
void toggle_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
  Boolean b;

  XtVaGetValues(w, XtNstate, &b, NULL);
  XtVaSetValues(w, XtNlabel, b ? _("Yes") : _("No"), NULL);
}

/**************************************************************************
...
**************************************************************************/
void option_ok_command_callback(Widget w, XtPointer client_data, 
			       XtPointer call_data)
{
  Boolean b;
  int val;
  XtPointer dp;

  client_options_iterate(o) {
    switch (o->type) {
    case COT_BOOLEAN:
      b = *(o->boolean.pvalue);
      XtVaGetValues((Widget) o->gui_data, XtNstate, o->boolean.pvalue, NULL);
      if (b != *(o->boolean.pvalue) && o->change_callback) {
	(o->change_callback)(o);
      }
      break;
    case COT_INTEGER:
      val = *(o->integer.pvalue);
      XtVaGetValues(o->gui_data, XtNstring, &dp, NULL);
      sscanf(dp, "%d", o->integer.pvalue);
      if (val != *(o->integer.pvalue) && o->change_callback) {
	(o->change_callback)(o);
      }
      break;
    case COT_STRING:
      XtVaGetValues(o->gui_data,
		    o->string.val_accessor ? "label" : XtNstring,
		    &dp, NULL);
      if (strcmp(o->string.pvalue, dp)) {
	mystrlcpy(o->string.pvalue, dp, o->string.size);
	if (o->change_callback) {
	  (o->change_callback)(o);
	}
      }
      break;
    case COT_FONT:
      /* FIXME */
      break;
    }
  } client_options_iterate_end;

  XtSetSensitive(main_form, TRUE);
  XtDestroyWidget(option_dialog_shell);
}

/**************************************************************************
...
**************************************************************************/
void option_cancel_command_callback(Widget w, XtPointer client_data, 
				    XtPointer call_data)
{
  XtSetSensitive(main_form, TRUE);
  XtDestroyWidget(option_dialog_shell);
}
