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

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "packets.h"

/* client */
#include "repodlgs_common.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, science_report_dialog_popup, bool raise)
GUI_FUNC_PROTO(void, science_report_dialog_redraw, void)
GUI_FUNC_PROTO(void, economy_report_dialog_popup, bool raise)
GUI_FUNC_PROTO(void, units_report_dialog_popup, bool raise)
GUI_FUNC_PROTO(void, endgame_report_dialog_start,
               const struct packet_endgame_report *packet)
GUI_FUNC_PROTO(void, endgame_report_dialog_player,
               const struct packet_endgame_player *packet)

GUI_FUNC_PROTO(void, real_science_report_dialog_update, void *unused)
GUI_FUNC_PROTO(void, real_economy_report_dialog_update, void *unused)
GUI_FUNC_PROTO(void, real_units_report_dialog_update, void *unused)

/* Actually defined in update_queue.c */
void science_report_dialog_update(void);
void economy_report_dialog_update(void);
void units_report_dialog_update(void);

#endif  /* FC__REPODLGS_G_H */
