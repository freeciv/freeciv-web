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
#ifndef FC__REPODLGS_G_H
#define FC__REPODLGS_G_H

#include "packets.h"         
#include "shared.h"		/* bool type */

#include "repodlgs_common.h"

void update_report_dialogs(void);

void science_dialog_update(void);
void popup_science_dialog(bool raise);
void economy_report_dialog_update(void);
void popup_economy_report_dialog(bool raise);
void activeunits_report_dialog_update(void);
void popup_activeunits_report_dialog(bool raise);
void popup_endgame_report_dialog(struct packet_endgame_report *packet);
void popup_settable_options_dialog(void);

#endif  /* FC__REPODLGS_G_H */
