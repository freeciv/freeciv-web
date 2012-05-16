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
#ifndef FC__INPUTDLG_H
#define FC__INPUTDLG_H

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>


Widget input_dialog_create(Widget parent, const char *dialogname, 
			   const char *text, const char *postinputtest,
			   XtCallbackProc ok_callback,
			   XtPointer ok_cli_data, 
			   XtCallbackProc cancel_callback,
			   XtPointer cancel_cli_data);

void input_dialog_destroy(Widget button);
char *input_dialog_get_input(Widget button);

void inputdlg_key_ok(Widget w);


#endif  /* FC__INPUTDLG_H */
