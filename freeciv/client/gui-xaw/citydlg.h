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
#ifndef FC__CITYDLG_H
#define FC__CITYDLG_H

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include "citydlg_g.h"


void citydlg_btn_select_citymap(Widget w, XEvent *event);
void citydlg_key_close(Widget w);
void citydlg_msg_close(Widget w);


#endif  /* FC__CITYDLG_H */
