/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2009-2017  The Freeciv-web project

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


/**************************************************************************
 ...
**************************************************************************/
function show_intelligence_report_dialog()
{
  if (selected_player == -1) return;
  var pplayer = players[selected_player];

  var msg = "Ruler " + pplayer['name'] + "<br>";
  if (pplayer['government'] > 0) {
    msg += "Government: " + governments[pplayer['government']]['name'] + "<br>";
  }

  if (pplayer['gold'] > 0) {
      msg += "Gold: " + pplayer['gold'] + "<br>";
    }

  if (pplayer['researching'] != null) {
    msg += "Researching: " + techs[pplayer['researching']]['name'] + "<br>";
  }


  msg += "<br><br>Establishing an embassy will show a detailed intelligence report."

  show_dialog_message("Intelligence report for " + pplayer['name'],
      msg);

}