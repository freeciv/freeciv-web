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
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>

#include "fcintl.h"
#include "game.h"
#include "map.h"
#include "mem.h"
#include "packets.h"
#include "player.h"

#include "chatline.h"
#include "citydlg.h"
#include "gui_main.h"
#include "gui_stuff.h"
#include "mapview.h"
#include "options.h"

#include "messagewin.h"

static Widget meswin_dialog_shell;
static Widget meswin_form;
static Widget meswin_label;
static Widget meswin_list;
static Widget meswin_viewport;
static Widget meswin_close_command;
static Widget meswin_goto_command;
static Widget meswin_popcity_command;
static bool meswin_dialog_shell_is_raised;

static void create_meswin_dialog(bool raise);
static void meswin_scroll_down(void);
static void meswin_close_callback(Widget w, XtPointer client_data,
				  XtPointer call_data);
static void meswin_list_callback(Widget w, XtPointer client_data,
				 XtPointer call_data);
static void meswin_goto_callback(Widget w, XtPointer client_data,
				 XtPointer call_data);
static void meswin_popcity_callback(Widget w, XtPointer client_data,
				    XtPointer call_data);

static char *dummy_message_list[] = {
  "                                                        ", 0 };

#define N_MSG_VIEW 24 		/* max before scrolling happens */

/****************************************************************
popup the dialog 10% inside the main-window 
*****************************************************************/
void popup_meswin_dialog(bool raise)
{
  int updated = 0;
  
  meswin_dialog_shell_is_raised = raise;

  if(!meswin_dialog_shell) {
    create_meswin_dialog(raise);
    updated = 1;		/* create_ calls update_ */
  }

  if (raise) {
    XtSetSensitive(main_form, FALSE);
  }

  xaw_set_relative_position(toplevel, meswin_dialog_shell, 25, 25);
  XtPopup(meswin_dialog_shell, XtGrabNone);
  if(!updated) 
    update_meswin_dialog();
  
  /* Is this necessary here? 
   * from popup_city_report_dialog():
   * force refresh of viewport so the scrollbar is added.
   * Buggy sun athena requires this
   */
  XtVaSetValues(meswin_viewport, XtNforceBars, True, NULL);
  
  meswin_scroll_down();
}

/****************************************************************
  Closes the message window dialog.
*****************************************************************/
void popdown_meswin_dialog(void)
{
  if (meswin_dialog_shell) {
    if (meswin_dialog_shell_is_raised) {
      XtSetSensitive(main_form, TRUE);
    }
    XtDestroyWidget(meswin_dialog_shell);
    meswin_dialog_shell = 0;
  }
}

/****************************************************************
...
*****************************************************************/
bool is_meswin_open(void)
{
  return meswin_dialog_shell != NULL;
}

/* This is set when we are creating the message window and it is not popped
 * up yet.  We can't scroll, since Xaw may crash - see PR#2794. */
static bool creating = FALSE;

/****************************************************************
...
*****************************************************************/
static void create_meswin_dialog(bool raise)
{
  creating = TRUE;

  meswin_dialog_shell =
    I_IN(I_T(XtCreatePopupShell("meswinpopup",
				raise ? transientShellWidgetClass
				: topLevelShellWidgetClass,
				toplevel, NULL, 0)));

  meswin_form = XtVaCreateManagedWidget("meswinform", 
				       formWidgetClass, 
				       meswin_dialog_shell, NULL);

  meswin_label =
    I_L(XtVaCreateManagedWidget("meswinlabel", labelWidgetClass, 
				meswin_form, NULL));
  
  meswin_viewport = XtVaCreateManagedWidget("meswinviewport", 
					    viewportWidgetClass, 
					    meswin_form, 
					    NULL);
   
  meswin_list = XtVaCreateManagedWidget("meswinlist", 
					listWidgetClass,
					meswin_viewport,
					XtNlist,
					(XtArgVal)dummy_message_list,
					NULL);
					 
  meswin_close_command =
    I_L(XtVaCreateManagedWidget("meswinclosecommand", commandWidgetClass,
				meswin_form, NULL));

  meswin_goto_command =
    I_L(XtVaCreateManagedWidget("meswingotocommand", commandWidgetClass,
				meswin_form,
				XtNsensitive, False,
				NULL));

  meswin_popcity_command =
    I_L(XtVaCreateManagedWidget("meswinpopcitycommand", commandWidgetClass,
				meswin_form,
				XtNsensitive, False,
				NULL));

  XtAddCallback(meswin_list, XtNcallback, meswin_list_callback, 
		NULL);
	       
  XtAddCallback(meswin_close_command, XtNcallback, meswin_close_callback, 
		NULL);

  XtAddCallback(meswin_goto_command, XtNcallback, meswin_goto_callback, 
		NULL);
  
  XtAddCallback(meswin_popcity_command, XtNcallback, meswin_popcity_callback, 
		NULL);
  
  real_update_meswin_dialog();

  XtRealizeWidget(meswin_dialog_shell);
  
  XSetWMProtocols(display, XtWindow(meswin_dialog_shell), 
		  &wm_delete_window, 1);
  XtOverrideTranslations(meswin_dialog_shell,
      XtParseTranslationTable("<Message>WM_PROTOCOLS: msg-close-messages()"));

  creating = FALSE;
}

/**************************************************************************
 This scrolls the messages window down to the bottom.
 NOTE: it seems this must not be called until _after_ meswin_dialog_shell
 is ...? realized, popped up, ... something.
 Its a toss-up whether we _should_ scroll the window down:
 Against: user will likely want to read from the top and scroll down manually.
 For: if we don't scroll down, new messages which appear at the bottom
 (including combat results etc) will be easily missed.
**************************************************************************/
static void meswin_scroll_down(void)
{
  Dimension height;
  int pos;
  
  if (!meswin_dialog_shell || creating) {
    return;
  }
  if (get_num_messages() <= N_MSG_VIEW) {
    return;
  }
  
  XtVaGetValues(meswin_list, XtNheight, &height, NULL);
  pos = (((double) (get_num_messages() - 1)) / get_num_messages()) * height;
  XawViewportSetCoordinates(meswin_viewport, 0, pos);
}

/**************************************************************************
...
**************************************************************************/
void real_update_meswin_dialog(void)
{
  Dimension height, iheight, width, oldheight, newheight;
  int i, num = get_num_messages();

  XawFormDoLayout(meswin_form, False);

  XtVaGetValues(meswin_viewport, XtNheight, &oldheight, NULL);

  if (num == 0) {
    XawListChange(meswin_list, dummy_message_list, 1, 0, True);
  } else {
    /* strings will not be freed */
    static char **strings = NULL;

    strings = fc_realloc(strings, num * sizeof(char *));

    for (i = 0; i < num; i++) {
      strings[i] = get_message(i)->descr;
    }

    XawListChange(meswin_list, strings, num, 0, True);
  }

  /* Much of the following copied from city_report_dialog_update() */
  XtVaGetValues(meswin_list, XtNlongest, &i, NULL);
  width = i + 10;
  /* I don't know the proper way to set the width of this viewport widget.
     Someone who knows is more than welcome to fix this */
  XtVaSetValues(meswin_viewport, XtNwidth, width + 15, NULL);
  XtVaSetValues(meswin_label, XtNwidth, width + 15, NULL);

  /* Seems have to do this here so we get the correct height below. */
  XawFormDoLayout(meswin_form, True);

  if (num <= N_MSG_VIEW) {
    XtVaGetValues(meswin_list, XtNheight, &height, NULL);
    if ((oldheight == 0) || (num == 0)) {
      XtVaSetValues(meswin_viewport, XtNheight, height, NULL);
    } else {
      XtVaGetValues(meswin_form, XtNheight, &newheight, NULL);
      newheight = newheight + height - oldheight;
      XtVaSetValues(meswin_form, XtNheight, newheight, NULL);
    }
  } else {
    XtVaGetValues(meswin_list, XtNheight, &height, NULL);
    XtVaGetValues(meswin_list, XtNinternalHeight, &iheight, NULL);
    height -= (iheight * 2);
    height /= num;
    height *= N_MSG_VIEW;
    height += (iheight * 2);
    if (height != oldheight) {
      if (oldheight == 0) {
	XtVaSetValues(meswin_viewport, XtNheight, height, NULL);
      } else {
	XtVaGetValues(meswin_form, XtNheight, &newheight, NULL);
	newheight = newheight + height - oldheight;
	XtVaSetValues(meswin_form, XtNheight, newheight, NULL);
      }
    }
    meswin_scroll_down();
  }

  XtSetSensitive(meswin_goto_command, FALSE);
  XtSetSensitive(meswin_popcity_command, FALSE);
}

/**************************************************************************
...
**************************************************************************/
static void meswin_list_callback(Widget w, XtPointer client_data,
				 XtPointer call_data)
{
  XawListReturnStruct *ret = XawListShowCurrent(meswin_list);

  if ((ret->list_index != XAW_LIST_NONE) && (get_num_messages() != 0)) {
    struct message *message = get_message(ret->list_index);

    XtSetSensitive(meswin_goto_command, message->location_ok ? True : False);
    XtSetSensitive(meswin_popcity_command, message->city_ok ? True : False);
  } else {
    XtSetSensitive(meswin_goto_command, False);
    XtSetSensitive(meswin_popcity_command, False);
  }
}

/**************************************************************************
...
**************************************************************************/
static void meswin_close_callback(Widget w, XtPointer client_data,
				  XtPointer call_data)
{
  popdown_meswin_dialog();
}

/****************************************************************
...
*****************************************************************/
void meswin_msg_close(Widget w)
{
  meswin_close_callback(w, NULL, NULL);
}

/**************************************************************************
...
**************************************************************************/
static void meswin_goto_callback(Widget w, XtPointer client_data,
				 XtPointer call_data)
{
  XawListReturnStruct *ret = XawListShowCurrent(meswin_list);

  if (ret->list_index != XAW_LIST_NONE) {
    meswin_goto(ret->list_index);
  }
}

/**************************************************************************
...
**************************************************************************/
static void meswin_popcity_callback(Widget w, XtPointer client_data,
				    XtPointer call_data)
{
  XawListReturnStruct *ret = XawListShowCurrent(meswin_list);

  if (ret->list_index != XAW_LIST_NONE) {
    meswin_popup_city(ret->list_index);
  }
}
