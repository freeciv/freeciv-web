/***********************************************************************
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
#ifndef FC__PLRDLG_H
#define FC__PLRDLG_H

/* client */
#include "plrdlg_g.h"

void popdown_players_dialog(void);

/* Misc helper functions */
GdkPixbuf *get_flag(const struct nation_type *pnation);
GdkPixbuf *create_player_icon(const struct player *plr);

extern struct gui_dialog *players_dialog_shell;

#endif  /* FC__PLRDLG_H */
