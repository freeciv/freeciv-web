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

#ifndef FC__HELPDLG_H
#define FC__HELPDLG_H

#include "tech.h"

#include "helpdlg_g.h"

void popup_tech_info(Tech_type_id tech);
void popup_impr_info(Impr_type_id impr);
void popup_unit_info(Unit_type_id unit_id);
void popup_gov_info(int gov);

#endif				/* FC__HELPDLG_H */
