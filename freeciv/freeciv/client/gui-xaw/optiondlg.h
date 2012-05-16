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
#ifndef FC__OPTIONDLG_H
#define FC__OPTIONDLG_H

#include <X11/Intrinsic.h>

void popup_option_dialog(void);
void toggle_callback(Widget w, XtPointer client_data, XtPointer call_data);

#endif  /* FC__OPTIONDLG_H */
