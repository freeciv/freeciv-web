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
#ifndef FC__MAPCTRL_G_H
#define FC__MAPCTRL_G_H

/* utility */
#include "support.h"            /* bool type */

/* common */
#include "fc_types.h"

/* client */
#include "mapctrl_common.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, popup_newcity_dialog, struct unit *punit,
               const char *suggestname)

GUI_FUNC_PROTO(void, set_turn_done_button_state, bool state)

GUI_FUNC_PROTO(void, create_line_at_mouse_pos, void)
GUI_FUNC_PROTO(void, update_rect_at_mouse_pos, void)

#endif  /* FC__MAPCTRL_G_H */
