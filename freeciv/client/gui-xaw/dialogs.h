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
#ifndef FC__DIALOGS_H
#define FC__DIALOGS_H

#include <X11/Intrinsic.h>

#include "fcintl.h"

#include "dialogs_g.h"

void popup_revolution_dialog(struct government *pgovernment);
Widget popup_message_dialog(Widget parent, const char *shellname,
			    const char *text, ...);
void destroy_message_dialog(Widget button);

void popup_about_dialog(void);
void racesdlg_key_ok(Widget w);

enum help_type_dialog {HELP_COPYING_DIALOG, HELP_KEYS_DIALOG};

void free_bitmap_destroy_callback(Widget w, XtPointer client_data, 
				  XtPointer call_data);
void destroy_me_callback(Widget w, XtPointer client_data, 
			 XtPointer call_data);

void taxrates_callback(Widget w, XtPointer client_data, XtPointer call_data);

#endif  /* FC__DIALOGS_H */
