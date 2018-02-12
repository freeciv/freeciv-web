/**********************************************************************
    Freeciv-web - the web version of Freeciv. http://play.freeciv.org/
    Copyright (C) 2018  The Freeciv-web project

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

/* TODO: update after changes in tile or other units.
 * Or at least add a refresh button.
 * Current workaround: cancel and give the order again.
 */

/****************************************************************************
  Ask the player to select a target.
****************************************************************************/
function popup_pillage_selection_dialog(punit)
{
  if (punit == null) return;
  var tgt = get_what_can_unit_pillage_from(punit, null);
  if (tgt.length == 0) return;

  var id = '#pillage_sel_dialog_' + punit['id'];

  $(id).remove();
  $("<div id='pillage_sel_dialog_" + punit['id'] + "'></div>").appendTo("div#game_page");
  $(id).append(document.createTextNode("Your "
              + unit_types[punit['type']]['name']
              + " is waiting for you to select what to pillage."));

  var button_id_prefix = 'pillage_sel_' + punit['id'] + '_';
  var buttons = [];
  for (var i = 0; i < tgt.length; i++) {
    var extra_id = tgt[i];
    if (extra_id == EXTRA_NONE) continue;
    buttons.push({
      id     : button_id_prefix + extra_id,
      'class': 'act_sel_button',
      text   : extras[extra_id]['name'],
      click  : pillage_target_selected
    });
  }
  buttons.push({
    id     : button_id_prefix + 'ANYTHING',
    'class': 'act_sel_button',
    text   : 'Just do something!',
    click  : pillage_target_selected
  });
  buttons.push({
    id     : 'pillage_sel_cancel_' + punit['id'],
    'class': 'act_sel_button',
    text   : 'Cancel',
    click  : function() {$(this).dialog('close');}
  });

  $(id).attr('title', 'Choose Your Target');
  $(id).dialog({
    bgiframe: true,
    modal: false,
    dialogClass: 'act_sel_dialog',
    width: 390,
    buttons: buttons,
    close: function () {$(this).remove();},
    autoOpen: true
  });

  $(id).dialog('open');
}

/****************************************************************************
  Respond to the target selection.
****************************************************************************/
function pillage_target_selected(ev)
{
  var id = ev.target.id;
  var params = id.match(/pillage_sel_(\d*)_([^_]*)/);
  var extra_id = params[2] == 'ANYTHING' ? EXTRA_NONE : parseInt(params[2], 10);
  request_new_unit_activity(units[parseInt(params[1], 10)],
                            ACTIVITY_PILLAGE, extra_id);
  $(this).dialog('close');
}
