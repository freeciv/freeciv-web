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

#include "dialogs_g.h"

HWND popup_message_dialog(HWND parent, char *dialogname,
			  const char *text, ...);
void destroy_message_dialog(HWND dlg);
void popup_revolution_dialog(struct government *gov);
void message_dialog_button_set_sensitive(HWND dlg,int id,int state);
BOOL unitselect_init(HINSTANCE hInstance);
#endif  /* FC__DIALOGS_H */
