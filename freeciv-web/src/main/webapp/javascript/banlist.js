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


/* This is a list of banned users of Freeciv-web.
   Note that user accounts can also be disabled by setting activated=0 in the auth DB table.
*/
var banned_users = [];

/**************************************************************************
 Returns false if the text contains a banned user.
**************************************************************************/
function check_text_with_banlist(text)
{
  if (text == null || text.length == 0) return false;
  for (var i = 0; i < banned_users.length; i++) {
    if (text.toLowerCase().indexOf(banned_users[i].toLowerCase()) != -1) return false;
  }
  return true;

}

/**************************************************************************
 Returns false if the text contains a banned user.
**************************************************************************/
function check_text_with_banlist_exact(text)
{
  if (text == null || text.length == 0) return false;
  for (var i = 0; i < banned_users.length; i++) {
    if (text.toLowerCase() == banned_users[i].toLowerCase()) return false;
  }
  return true;

}
