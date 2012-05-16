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
#ifndef FC__MAPCTRL_H
#define FC__MAPCTRL_H

#include "mapctrl_g.h"
void map_handle_rbut(int x, int y);
void map_handle_lbut(int x, int y);
void map_handle_move(int window_x, int window_y);
void center_on_unit(void);
void init_mapwindow(void);
void overview_handle_rbut(int x, int y);
void indicator_handle_but(int x);
#endif  /* FC__MAPCTRL_H */
