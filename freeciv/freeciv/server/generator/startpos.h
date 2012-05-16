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
#ifndef FC__STARTPOS_H
#define FC__STARTPOS_H

enum start_mode {
  MT_DEFAULT,
  MT_SINGLE,
  MT_2or3,
  MT_ALL,
  MT_VARIABLE
};

bool create_start_positions(enum start_mode mode,
			    struct unit_type *initial_unit);

#endif
