/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

function client_tile_get_known(ptile)
{

/* TODO:
  if (NULL == client.conn.playing) {
    if (client_is_observer()) {
      return TILE_KNOWN_SEEN;
    } else {
      return TILE_UNKNOWN;
    }
  }
*/

  return tile_get_known(ptile);
}

