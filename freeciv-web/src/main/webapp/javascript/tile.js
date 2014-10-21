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

  var TILE_UNKNOWN = 0;
  var TILE_KNOWN_UNSEEN = 1;
  var TILE_KNOWN_SEEN = 2;


/****************************************************************************
  Return a known_type enumeration value for the tile.

  Note that the client only has known data about its own player.
****************************************************************************/
function tile_get_known(ptile)
{
  if (ptile['known'] == null || ptile['known'] == TILE_UNKNOWN) {
    return TILE_UNKNOWN;
  } else if (ptile['known'] == TILE_KNOWN_UNSEEN) {
    return TILE_KNOWN_UNSEEN;
  } else if (ptile['known'] == TILE_KNOWN_SEEN) {
    return TILE_KNOWN_SEEN;
  }
  
}

function tile_has_extra(ptile, extra)
{
  if (ptile['extras'] == null || ptile['extras'][extra] == null) {
    return false;
  }

  return ptile['extras'][extra] == 1;
}

function tile_resource(tile)
{
  return tile['resource'];
}

function tile_set_resource(tile, resource)
{
  tile['resource'] = resource;
}


function tile_owner(tile)
{
  return tile['owner'];
}

function tile_set_owner(tile, owner, claimer)
{
  tile['owner'] = owner;
}

function tile_worked(tile)
{
  return tile['worked'];
}

function tile_set_worked(ptile, pwork)
{
  ptile['worked'] = pwork;
}


/****************************************************************************
  Return the city on this tile (or NULL), checking for city center.
****************************************************************************/
function tile_city(ptile)
{
  if (ptile == null) return null;

  var city_id = ptile['worked'];
  var pcity = cities[city_id];

  if (pcity != null && is_city_center(pcity, ptile)) {
    return pcity;
  }
  return null;
}
