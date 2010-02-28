/********************************************************************** 
 Freeciv - Copyright (C) 2004 - A. Gorshenev
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__CLIENT_AGENTS_SIMPLE_HISTORIAN_H
#define FC__CLIENT_AGENTS_SIMPLE_HISTORIAN_H

void simple_historian_init(void);
void simple_historian_done(void);

struct tile* sha_tile_recall(struct tile *ptile);
struct unit* sha_unit_recall(int id);

#endif /* header guard */


