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


var unit_types = {};  /* packet_ruleset_unit */

var U_NOT_OBSOLETED = null;

/**************************************************************************
Whether player can build given unit somewhere,
ignoring whether unit is obsolete and assuming the
player has a coastal city.
**************************************************************************/
function can_player_build_unit_direct(p, punittype)
{


  /*if (utype_has_flag(punittype, F_NUCLEAR)
      && !get_player_bonus(p, EFT_ENABLE_NUKE) > 0) {
    return FALSE;
  }*/

  /*if (utype_has_flag(punittype, F_NOBUILD)) {
    return FALSE;
  }*/


  /*if (punittype->need_government
      && punittype->need_government != government_of_player(p)) {
    return FALSE;
  }*/

  if (player_invention_state(p, punittype['tech_requirement']) != TECH_KNOWN) {
    return false;
  }

  /* FIXME: add support for global advances, check for building reqs etc.*/

  return true;
}

/**************************************************************************
...
**************************************************************************/
function get_units_from_tech(tech_id)
{
  var result = [];

  for (var unit_type_id in unit_types) {
    var punit_type = unit_types[unit_type_id];
    if (punit_type['tech_requirement'] == tech_id) {
      result.push(punit_type);
    }
  }
  return result;
}
