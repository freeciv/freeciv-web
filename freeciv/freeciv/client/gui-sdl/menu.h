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

/***************************************************************************
                          menu.h  -  description
                             -------------------
    begin                : Wed Sep 04 2002
    copyright            : (C) 2002 by Rafał Bursig
    email                : Rafał Bursig <bursig@poczta.fm>
 ***************************************************************************/

#ifndef FC__MENU_H
#define FC__MENU_H

#include "menu_g.h"

void create_units_order_widgets(void);
void delete_units_order_widgets(void);
void update_order_widgets(void);
void undraw_order_widgets(void);
void free_bcgd_order_widgets(void);
void disable_order_buttons(void);
void enable_order_buttons(void);

#endif				/* FC__MENU_H */
