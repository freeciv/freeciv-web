/**********************************************************************
    Freeciv-web - the web version of Freeciv. https://www.freeciv.org/
    Copyright (C) 2021  The Freeciv-web project

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


/* All generalized actions. */
var actions = {};

/**********************************************************************//**
  Return the action with the given id.

  Returns NULL if no action with the given id exists.
**************************************************************************/
function action_by_number(act_id)
{
  if (actions[act_id] == undefined) {
    console.log("Asked for non existing action numbered %d", act_id);
    return null;
  }

  return actions[act_id];
}

/**********************************************************************//**
  Returns TRUE iff performing the specified action has the specified
  result.
**************************************************************************/
function action_has_result(paction, result)
{
  if (paction == null || paction['result'] == null) {
    console.log("action_has_result(): bad action");
    console.log(paction);
    return null;
  }

  return paction['result'] == result;
}

/**************************************************************************
  Returns true iff the given action probability belongs to an action that
  may be possible.
**************************************************************************/
function action_prob_possible(aprob)
{
  return 0 < aprob['max'] || action_prob_not_impl(aprob);
}
