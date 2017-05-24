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


var requirements = {};


/* Range of requirements.
 * Used in the network protocol.
 * Order is important -- wider ranges should come later -- some code
 * assumes a total order, or tests for e.g. >= REQ_RANGE_PLAYER.
 * Ranges of similar types should be supersets, for example:
 *  - the set of Adjacent tiles contains the set of CAdjacent tiles,
 *    and both contain the center Local tile (a requirement on the local
 *    tile is also within Adjacent range);
 *  - World contains Alliance contains Player (a requirement we ourselves
 *    have is also within Alliance range). */
var REQ_RANGE_LOCAL = 0;
var REQ_RANGE_CADJACENT = 1;
var REQ_RANGE_ADJACENT = 2;
var REQ_RANGE_CITY = 3;
var REQ_RANGE_TRADEROUTE = 4;
var REQ_RANGE_CONTINENT = 5;
var REQ_RANGE_PLAYER = 6;
var REQ_RANGE_TEAM = 7;
var REQ_RANGE_ALLIANCE = 8;
var REQ_RANGE_WORLD = 9;
var REQ_RANGE_COUNT = 10;   /* keep this last */


/****************************************************************************
  Checks the requirement to see if it is active on the given target.

  target gives the type of the target
  (player,city,building,tile) give the exact target
  req gives the requirement itself

  Make sure you give all aspects of the target when calling this function:
  for instance if you have TARGET_CITY pass the city's owner as the target
  player as well as the city itself as the target city.
****************************************************************************/
function is_req_active(target_player,
		   target_city,
		   target_building,
		   target_tile,
		   target_unittype,
		   target_output,
		   target_specialist,
		   req,
           prob_type)
{
  var result = TRI_NO;

  /* Note the target may actually not exist.  In particular, effects that
   * have a VUT_SPECIAL or VUT_TERRAIN may often be passed to this function
   * with a city as their target.  In this case the requirement is simply
   * not met. */
  switch (req['kind']) {
  case VUT_NONE:
    result = TRI_YES;
    break;
  case VUT_ADVANCE:
    /* The requirement is filled if the player owns the tech. */
    result = is_tech_in_range(target_player, req['range'], req['value']);
    break;
  case VUT_GOVERNMENT:
  case VUT_IMPROVEMENT:
  case VUT_TERRAIN:
  case VUT_NATION:
  case VUT_UTYPE:
  case VUT_UTFLAG:
  case VUT_UCLASS:
  case VUT_UCFLAG:
  case VUT_OTYPE:
  case VUT_SPECIALIST:
  case VUT_MINSIZE:
  case VUT_AI_LEVEL:
  case VUT_TERRAINCLASS:
  case VUT_MINYEAR:
  case VUT_TERRAINALTER:
  case VUT_CITYTILE:
  case VUT_GOOD:
  case VUT_TERRFLAG:
  case VUT_NATIONALITY:
  case VUT_BASEFLAG:
  case VUT_ROADFLAG:
  case VUT_EXTRA:
  case VUT_TECHFLAG:
  case VUT_ACHIEVEMENT:
  case VUT_DIPLREL:
  case VUT_MAXTILEUNITS:
  case VUT_STYLE:
  case VUT_MINCULTURE:
  case VUT_UNITSTATE:
  case VUT_MINMOVES:
  case VUT_MINVETERAN:
  case VUT_MINHP:
  case VUT_AGE:
  case VUT_NATIONGROUP:
  case VUT_TOPO:
  case VUT_IMPR_GENUS:
  case VUT_ACTION:
  case VUT_MINTECHS:
  case VUT_EXTRAFLAG:
  case VUT_MINCALFRAG:
  case VUT_SERVERSETTING:
    //FIXME: implement
    console.log("Unimplemented requirement type " + req['kind']);
    break;
  case VUT_COUNT:
    return false;
  default:
    console.log("Unknown requirement type " + req['kind']);
  }

  if (result == TRI_MAYBE) {
    if (prob_type == RPT_POSSIBLE) {
      return true;
    } else {
      return false;
    }
  }
  if (req['present']) {
    return result == TRI_YES;
  } else {
    return result == TRI_NO;
  }
}


/****************************************************************************
  Checks the requirement(s) to see if they are active on the given target.

  target gives the type of the target
  (player,city,building,tile) give the exact target

  reqs gives the requirement vector.
  The function returns TRUE only if all requirements are active.

  Make sure you give all aspects of the target when calling this function:
  for instance if you have TARGET_CITY pass the city's owner as the target
  player as well as the city itself as the target city.
****************************************************************************/
function are_reqs_active(target_player,
		     target_city,
		     target_building,
		     target_tile,
		     target_unittype,
		     target_output,
		     target_specialist,
		     reqs,
             prob_type)
{

  for (var i = 0; i < reqs.length; i++) {
    if (!is_req_active(target_player, target_city, target_building,
		       target_tile, target_unittype, target_output,
		       target_specialist,
		       reqs[i], prob_type)) {
      return false;
    }
  }
  return true;
}


/****************************************************************************
  Is there a source tech within range of the target?
****************************************************************************/
function is_tech_in_range(target_player, range, tech)
{
  switch (range) {
  case REQ_RANGE_PLAYER:
    return ((target_player != null
            && player_invention_state(target_player, tech) == TECH_KNOWN) ?
              TRI_YES :
              TRI_NO);
  case REQ_RANGE_TEAM:
  case REQ_RANGE_ALLIANCE:
  case REQ_RANGE_WORLD:
    /* FIXME: Add support for the above ranges. Freeciv's implementation
     * currently (25th Jan 2017) lives in common/requirements.c */
    console.log("Unimplemented tech requirement range " + range);
    return TRI_MAYBE;
  case REQ_RANGE_LOCAL:
  case REQ_RANGE_CADJACENT:
  case REQ_RANGE_ADJACENT:
  case REQ_RANGE_CITY:
  case REQ_RANGE_TRADEROUTE:
  case REQ_RANGE_CONTINENT:
  case REQ_RANGE_COUNT:
    break;
  }

  console.log("Invalid tech req range " + range);
  return TRI_MAYBE;
}


/**************************************************************************
  Return the number of shields it takes to build this universal.
**************************************************************************/
function universal_build_shield_cost(target)
{
  return target['build_cost'];
}
