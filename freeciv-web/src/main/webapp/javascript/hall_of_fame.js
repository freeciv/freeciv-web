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

var submitted_to_hof = false;

/**************************************************************************
 Submit game to Hall of Fame
**************************************************************************/
function submit_game_to_hall_of_fame()
{
  if (client_is_observer() || client.conn.playing == null || submitted_to_hof) {
    swal("Unable to submit to Hall of Fame!");
    return;
  }
  submitted_to_hof = true;

  var pplayer = client.conn.playing;
  var pnation = nations[pplayer['nation']];

  // #overview_map

  $.ajax({
   type: 'POST',
   url: "/hall_of_fame_post?username=" + username + "&nation=" + nations[pplayer['nation']]['adjective'] + "&score=" + get_score_text(pplayer)
         + "&turn=" + game_info['turn'] + "&imgur_url=null"  ,
   success: function(data, textStatus, request){
       $("#dialog").html("Game submitted to Hall of Fame!");
   }

   }).fail(function() {
    swal("Unable to sumit game to Hall of Fame. Please try again later!");
    $.unblockUI();

  })

}