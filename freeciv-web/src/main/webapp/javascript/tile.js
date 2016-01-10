/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2015  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

/**************************************************************************
  Returns true iff the specified tile has the extra with the specified
  extra number.
**************************************************************************/
function tile_has_extra(ptile, extra)
{
  if (ptile['extras'] == null) {
    return false;
  }

  return ptile['extras'].isSet(extra);
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
