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
#ifndef FC__CONNECTDLG_G_H
#define FC__CONNECTDLG_G_H

void close_connection_dialog(void);
void really_close_connection_dialog(void);

void gui_server_connect(void);

void gui_set_rulesets(int num_rulesets, char **rulesets);

#endif  /* FC__CONNECTDLG_G_H */
