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
#ifndef FC__CITYDLG_G_H
#define FC__CITYDLG_G_H

#include "fc_types.h"

#include "citydlg_common.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, real_city_dialog_popup, struct city *pcity)
GUI_FUNC_PROTO(void, popdown_city_dialog, struct city *pcity)
GUI_FUNC_PROTO(void, popdown_all_city_dialogs, void)
GUI_FUNC_PROTO(void, real_city_dialog_refresh, struct city *pcity)
GUI_FUNC_PROTO(void, refresh_unit_city_dialogs, struct unit *punit)
GUI_FUNC_PROTO(bool, city_dialog_is_open, struct city *pcity)

/* Actually defined in update_queue.c */
void popup_city_dialog(struct city *pcity);
void refresh_city_dialog(struct city *pcity);

#endif  /* FC__CITYDLG_G_H */
