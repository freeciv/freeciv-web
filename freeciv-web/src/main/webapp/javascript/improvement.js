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

var improvements = {};


var B_LAST = MAX_NUM_ITEMS;


/**************************************************************************
 Returns a list containing improvements which are available from a tech.
**************************************************************************/
function get_improvements_from_tech(tech_id)
{
  var result = [];
  for (var improvement_id in improvements) {
    var pimprovement = improvements[improvement_id];
    var reqs = get_improvement_requirements(improvement_id);
    for (var i = 0; i < reqs.length; i++) {
      if (reqs[i] == tech_id) {
        result.push(pimprovement);
      }
    }
  }
  return result;

}

/**************************************************************************
...
**************************************************************************/
function is_wonder(improvement)
{
  return (improvement['soundtag'][0] == 'w');
}

/**************************************************************************
 returns list of tech ids which are a requirement for the given improvement
**************************************************************************/
function get_improvement_requirements(improvement_id)
{
  var result = [];
  var improvement = improvements[improvement_id];
  if (improvement != null && improvement['reqs'] != null) {
    for (var i = 0; i < improvement['reqs'].length; i++) {
      if (improvement['reqs'][i]['kind'] == 1
          && improvement['reqs'][i]['present']) {
        result.push(improvement['reqs'][i]['value']);
      }
    }
  }

  return result;

}
