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

#ifndef FC__REPODLGS_H
#define FC__REPODLGS_H

/* SDL2 */
#ifdef SDL2_PLAIN_INCLUDE
#include <SDL.h>
#else  /* SDL2_PLAIN_INCLUDE */
#include <SDL2/SDL.h>
#endif /* SDL2_PLAIN_INCLUDE */

/* client/gui-sdl2 */
#include "gui_string.h"

#include "repodlgs_g.h"

enum tech_info_mode {
  FULL_MODE,
  MED_MODE,
  SMALL_MODE
};

SDL_Surface *create_select_tech_icon(utf8_str *pstr, Tech_type_id tech_id,
                                     enum tech_info_mode mode);
void science_report_dialogs_popdown_all(void);
void economy_report_dialog_popdown(void);
void units_report_dialog_popdown(void);

#endif /* FC__REPODLGS_H */
