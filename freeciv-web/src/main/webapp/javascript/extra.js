/***********************************************************************
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

var extras = {};
var roads = {};
var bases = {};

var EXTRA_NONE = -1;

var BASE_GUI_FORTRESS = 0;
var BASE_GUI_AIRBASE = 1;

// See handle_ruleset_extra(), where EXTRA_* variables are defines dynamically.

/************************************************************************//**
  Return extras type of given id.
****************************************************************************/
function extra_by_number(id)
{
  if (id == EXTRA_NONE) {
    return null;
  }

  if (id >= 0 && id < ruleset_control['num_extra_types']) {
    return extras[id];
  } else {
    console.log("extra_by_number(): Invalid extra id: " + id);

    return null;
  }
}

/************************************************************************//**
  Who owns extras on tile
****************************************************************************/
function extra_owner(ptile)
{
  return player_by_number(ptile['extras_owner']);
}

/************************************************************************//**
  Is given cause one of the causes for the given extra?
****************************************************************************/
function is_extra_caused_by(pextra, cause)
{
  return pextra.causes.isSet(cause);
}

/************************************************************************//**
  Is given cause one of the removal causes for the given extra?
****************************************************************************/
function is_extra_removed_by(pextra, rmcause)
{
  return pextra.rmcauses.isSet(rmcause);
}

/************************************************************************//**
  Does this extra type claim territory?
****************************************************************************/
function territory_claiming_extra(pextra)
{
  return pextra['base'] && pextra['base']['border_sq'] > -1;
}
