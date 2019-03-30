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
#ifndef FC__PLRDLG_G_H
#define FC__PLRDLG_G_H

#include "plrdlg_common.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, popup_players_dialog, bool raise)
GUI_FUNC_PROTO(void, real_players_dialog_update, void*)

/* Actually defined in update_queue.c */
void players_dialog_update(void);

#endif  /* FC__PLRDLG_G_H */
