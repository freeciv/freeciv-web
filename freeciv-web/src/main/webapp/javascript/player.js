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



var players = {};
var research_data = {};

var MAX_NUM_PLAYERS = 30;

var MAX_AI_LOVE = 1000;

var DS_ARMISTICE = 0;
var DS_WAR = 1;
var DS_CEASEFIRE = 2;
var DS_PEACE = 3;
var DS_ALLIANCE = 4;
var DS_NO_CONTACT = 5;
var DS_TEAM = 6;
var DS_LAST = 7;

/* The plr_flag_id enum. */
var PLRF_AI = 0;
var PLRF_SCENARIO_RESERVED = 1;
var PLRF_COUNT = 2;

function valid_player_by_number(playerno)
{
  /*  TODO:
  pslot = player_slot_by_number(player_id);
  if (!player_slot_is_used(pslot)) {
    return NULL;  */

  return players[playerno];
}

function player_by_number(playerno)
{
  return players[playerno];
}


function player_by_name(pname)
{
  for (var player_id in players) {
    var pplayer = players[player_id];
    if (pname == pplayer['name']) return pplayer;
  }
  return null;
}


/***************************************************************
 If the specified player owns the unit with the specified id,
 return pointer to the unit struct.  Else return NULL.
 Uses fast idex_lookup_city.

 pplayer may be NULL in which case all units registered to
 hash are considered - even those not currently owned by any
 player. Callers expect this behavior.
***************************************************************/
function player_find_unit_by_id(pplayer, unit_id)
{
  var punit = idex_lookup_unit(unit_id);

  if (punit == null) return null;

  if (pplayer != null || (unit_owner(punit) == pplayer)) {
    /* Correct owner */
    return punit;
  }

  return NULL;

}

/**************************************************************************
  Return the player index.

  Currently same as player_number(), paired with player_count()
  indicates use as an array index.
**************************************************************************/
function player_index(pplayer)
{
  return player_number(pplayer);
}

/**************************************************************************
  Return the player index/number/id.
**************************************************************************/
function player_number(player)
{
  return player['playerno'];
}


/**************************************************************************
  ...
**************************************************************************/
function get_diplstate_text(state_id)
{
  if (DS_ARMISTICE == state_id) {
    return "Armistice";
  } else if (DS_WAR == state_id) {
    return "War";
  } else if (DS_CEASEFIRE == state_id) {
    return "Ceasefire";
  } else if (DS_PEACE == state_id) {
    return "Peace";
  } else if (DS_ALLIANCE == state_id) {
    return "Alliance";
  } else if (DS_NO_CONTACT == state_id) {
    return "No contact";
  } else if (DS_TEAM == state_id) {
    return "Team";
  } else {
    return "Unknown state";
  }

}

/**************************************************************************
  ...
**************************************************************************/
function get_ai_level_text(player)
{
  var ai_level = player['ai_skill_level'];
  if (ai_level == 0) {
    return "Away";
  } else if (ai_level == 1) {
    return "Handicapped";
  } else if (ai_level == 2) {
    return "Novice";
  } else if (ai_level == 3) {
    return "Easy";
  } else if (ai_level == 4) {
    return "Normal";
  } else if (ai_level == 5) {
    return "Hard";
  } else if (ai_level == 6) {
    return "Cheating";
  } else if (ai_level == 7) {
    return "Experimental";
  }

  return "Unknown";

}

/**************************************************************************
  Returns the research object related to the given player.
**************************************************************************/
function research_get(pplayer)
{
  if (player == null) return null;

  if (game_info['team_pooled_research']) {
    return research_data[pplayer['team']];
  } else {
    return research_data[pplayer['playerno']];
  }

}

/**************************************************************************
  returns true if the given player has the given wonder (improvement)
**************************************************************************/
function player_has_wonder(playerno, improvement_id)
{
  for (var city_id in cities) {
    var pcity = cities[city_id];
    if (city_owner(pcity).playerno == playerno && city_has_building(pcity, improvement_id)) {
      return true;
    }
  }
  return false;
}
