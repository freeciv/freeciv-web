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
 Show the intelligence report dialog, with data depending on the
 stablishment of an embassy.
**************************************************************************/
function show_intelligence_report_dialog()
{
  if (selected_player == -1) return;
  var pplayer = players[selected_player];

  if (client_is_observer()
      || client.conn.playing.real_embassy[selected_player]) {
    show_intelligence_report_embassy(pplayer);
  } else {
    show_intelligence_report_hearsay(pplayer);
  }
}

/**************************************************************************
 Show the intelligence report dialog when there's no embassy.
**************************************************************************/
function show_intelligence_report_hearsay(pplayer)
{
  var msg = "Ruler " + pplayer['name'] + "<br>";
  if (pplayer['government'] > 0) {
    msg += "Government: " + governments[pplayer['government']]['name'] + "<br>";
  }

  if (pplayer['gold'] > 0) {
      msg += "Gold: " + pplayer['gold'] + "<br>";
    }

  if (pplayer['researching'] != null && pplayer['researching'] > 0 && techs[pplayer['researching']] != null) {
    msg += "Researching: " + techs[pplayer['researching']]['name'] + "<br>";
  }


  msg += "<br><br>Establishing an embassy will show a detailed intelligence report."

  show_dialog_message("Intelligence report for " + pplayer['name'],
      msg);

}

/**************************************************************************
 Show the intelligence report dialog when there's an embassy.
**************************************************************************/
function show_intelligence_report_embassy(pplayer)
{
  // reset dialog page.
  $("#intel_dialog").remove();
  $("<div id='intel_dialog'></div>").appendTo("div#game_page");

  var intel_data = {
    ruler: pplayer['name'],
    government: governments[pplayer['government']]['name'],
    capital: '(not implemeted yet)', // TODO
    gold: pplayer['gold'],
    tax: pplayer['tax'] + '%',
    science: pplayer['science'] + '%',
    luxury: pplayer['luxury'] + '%',
    researching: '(Unknown)',
    dipl: [],
    tech: []
  };

  // TODO: future techs
  var research = research_get(pplayer);
  if (research !== undefined) {
    var researching = techs[research['researching']];
    if (researching !== undefined) {
      intel_data['researching'] = researching['name'] + ' ('
                                + research['bulbs_researched'] + '/'
                                + research['researching_cost'] + ')';
    } else {
      intel_data['researching'] = '(Nothing)';
    }
    var myresearch = client_is_observer()
                     ? null
                     : research_get(client.conn.playing)['inventions'];
    for (var tech_id in techs) {
      if (research['inventions'][tech_id] == TECH_KNOWN) {
        intel_data['tech'].push({
          name: techs[tech_id]['name'],
          who: (myresearch != null && myresearch[tech_id] == TECH_KNOWN)
                           ? 'both' : 'them'
        });
      }
    }
  }

  if (pplayer['diplstates'] !== undefined) {
    pplayer['diplstates'].forEach(function (st, i) {
      if (st['state'] !== DS_NO_CONTACT && i !== pplayer['playerno']) {
        var dplst = intel_data['dipl'][st['state']];
        if (dplst === undefined) {
          dplst = {
            state: get_diplstate_text(st['state']),
            nations: []
          };
          intel_data['dipl'][st['state']] = dplst;
        }
        dplst['nations'].push(nations[players[i]['nation']]['adjective']);
      }
    });
  }

  $("#intel_dialog").html(Handlebars.templates['intel'](intel_data));
  $("#intel_dialog").dialog({
			bgiframe: true,
			modal: true,
			title: "Foreign Intelligence: "
                             + nations[pplayer['nation']]['adjective']
                             + " Empire",
                        width: "auto"
                     });

  $("#intel_dialog").dialog('open');
  $("#intel_tabs").tabs();
  $("#game_text_input").blur();

}

