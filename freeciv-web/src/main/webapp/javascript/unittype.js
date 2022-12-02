/**********************************************************************
    Freeciv-web - the web version of Freeciv. https://www.freeciv.org/
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
var unit_classes = {};  /* packet_ruleset_unit_class */

var U_NOT_OBSOLETED = null;

var U_LAST = MAX_NUM_UNITS;

var UCF_TERRAIN_SPEED = 0;
var UCF_TERRAIN_DEFENSE = 1;
var UCF_DAMAGE_SLOWS = 2;
var UCF_CAN_OCCUPY_CITY = 3;
var UCF_BUILD_ANYWHERE = 4;
var UCF_UNREACHABLE = 5;
var UCF_COLLECT_RANSOM = 6;
var UCF_ZOC = 7;
var UCF_DOESNT_OCCUPY_TILE = 8;
var UCF_ATTACK_NON_NATIVE = 9;
var UCF_KILLCITIZEN = 10;
var UCF_HUT_FRIGHTEN = 11;

/**********************************************************************//**
  Return true iff units of the given type can do the specified generalized
  (ruleset defined) action enabler controlled action.

  Note that a specific unit in a specific situation still may be unable to
  perform the specified action.
**************************************************************************/
function utype_can_do_action(putype, action_id)
{
  if (putype == null || putype['utype_actions'] == null) {
    console.log("utype_can_do_action(): bad unit type.");
    return false;
  } else if (action_id >= ACTION_COUNT || action_id < 0) {
    console.log("utype_can_do_action(): invalid action id " + action_id);
    return false;
  }

  return putype['utype_actions'].isSet(action_id);
}

/**********************************************************************//**
  Return TRUE iff units of the given type can do any enabler controlled
  action with the specified action result.

  Note that a specific unit in a specific situation still may be unable to
  perform the specified action.
**************************************************************************/
function utype_can_do_action_result(putype, result)
{
  var act_id;

  if (putype == null || putype['utype_actions'] == null) {
    console.log("utype_can_do_action_result(): bad unit type.");
    return false;
  }

  for (act_id = 0; act_id < ACTION_COUNT; act_id++) {
    var paction = action_by_number(act_id);
    if (action_has_result(paction, result)
        && utype_can_do_action(putype, act_id)) {
      return true;
    }
  }

  return false;
}

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
