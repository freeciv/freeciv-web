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

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/AsciiText.h>  

#include "fcintl.h"

#include "gui_stuff.h"

#include "inputdlg.h"

/****************************************************************
...
*****************************************************************/
char *input_dialog_get_input(Widget button)
{
  XtPointer dp;
  Widget winput;
      
  winput=XtNameToWidget(XtParent(button), "iinput");
  
  XtVaGetValues(winput, XtNstring, &dp, NULL);
 
  return dp;
}


/****************************************************************
...
*****************************************************************/
void input_dialog_destroy(Widget button)
{
  XtSetSensitive(XtParent(XtParent(XtParent(button))), TRUE);

  XtDestroyWidget(XtParent(XtParent(button)));
}

/****************************************************************
...
*****************************************************************/
void inputdlg_key_ok(Widget w)
{
  x_simulate_button_click(XtNameToWidget(XtParent(w), "iokcommand"));
}


/****************************************************************
...
*****************************************************************/
Widget input_dialog_create(Widget parent, const char *dialogname, 
			   const char *text, const char *postinputtest,
			   XtCallbackProc ok_callback,
			   XtPointer ok_cli_data, 
			   XtCallbackProc cancel_callback,
			   XtPointer cancel_cli_data)
{
  Widget shell, form, label, input, ok, cancel;

  XtSetSensitive(parent, FALSE);
  
  I_T(shell=XtCreatePopupShell(dialogname, transientShellWidgetClass,
			       parent, NULL, 0));
  
  form=XtVaCreateManagedWidget("iform", formWidgetClass, shell, NULL);

  /* Caller should i18n the text passed in as desired */
  label=XtVaCreateManagedWidget("ilabel", labelWidgetClass, form, 
				    XtNlabel, (XtArgVal)text, NULL);   

  input=XtVaCreateManagedWidget("iinput",
				asciiTextWidgetClass,
				form,
				XtNfromVert, (XtArgVal)label,
				XtNeditType, XawtextEdit,
				XtNstring, postinputtest,
				NULL);
  
  ok=XtVaCreateManagedWidget("iokcommand", 
			     commandWidgetClass,
			     form,
			     XtNlabel, (XtArgVal)_("Ok"),
			     XtNfromVert, input,
			     NULL);

  cancel=XtVaCreateManagedWidget("icancelcommand", 
				 commandWidgetClass,
				 form,
				 XtNlabel, (XtArgVal)_("Cancel"),
				 XtNfromVert, input,
				 XtNfromHoriz, ok,
				 NULL);
  
  xaw_set_relative_position(parent, shell, 10, 10);
  XtPopup(shell, XtGrabNone);
  
  XtAddCallback(ok, XtNcallback, ok_callback, ok_cli_data);
  XtAddCallback(cancel, XtNcallback, cancel_callback, 
		cancel_cli_data);

  XtSetKeyboardFocus(parent, input);
    
  return shell;
}
