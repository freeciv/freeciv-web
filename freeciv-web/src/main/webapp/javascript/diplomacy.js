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

var CLAUSE_ADVANCE = 0;
var CLAUSE_GOLD = 1;
var CLAUSE_MAP = 2;
var CLAUSE_SEAMAP = 3;
var CLAUSE_CITY = 4;
var CLAUSE_CEASEFIRE = 5;
var CLAUSE_PEACE = 6;
var CLAUSE_ALLIANCE = 7;
var CLAUSE_VISION = 8;
var CLAUSE_EMBASSY = 9;

var diplomacy_request_queue = [];
var diplomacy_clause_map = {};
var active_diplomacy_meeting_id = null;

/**************************************************************************
 ...
**************************************************************************/
function diplomacy_init_meeting_req(counterpart)
{
  var packet = {"pid" : packet_diplomacy_init_meeting_req,
	        "counterpart" : counterpart};
  send_request(JSON.stringify(packet));
}

/**************************************************************************
 ...
**************************************************************************/
function refresh_diplomacy_request_queue()
{
  if (diplomacy_request_queue.length > 0) {
    var next = diplomacy_request_queue[0];
    if (next != null && next != active_diplomacy_meeting_id) {
        active_diplomacy_meeting_id = next;
      	show_diplomacy_dialog(active_diplomacy_meeting_id);
	show_diplomacy_clauses();
    }
  }
}

/**************************************************************************
 ...
**************************************************************************/
function show_diplomacy_dialog(counterpart)
{
 if (cardboard_vr_enabled) return;
 var pplayer = players[counterpart];
 $.ajax({
   url: "/webclient/diplomacy_meeting.hbs?ts=" + ts,
   dataType: "html",
   cache: true,
   async: false
 }).fail(function() {
   swal("Unable to load diplomacy meeting dialog template.");
 }).done(function( data ) {
   create_diplomacy_dialog(pplayer, Handlebars.compile(data));
 });
}

/**************************************************************************
 ...
**************************************************************************/
function accept_treaty_req()
{
  var packet = {"pid" : packet_diplomacy_accept_treaty_req,
	         "counterpart" : active_diplomacy_meeting_id};
  send_request(JSON.stringify(packet));
}

/**************************************************************************
 ...
**************************************************************************/
function accept_treaty(counterpart, I_accepted, other_accepted)
{
  if (active_diplomacy_meeting_id == counterpart
      && I_accepted == true && other_accepted == true) {
    $("#diplomacy_dialog").remove(); //close the dialog.

    if (diplomacy_request_queue.indexOf(counterpart) >= 0) {
      diplomacy_request_queue.splice(diplomacy_request_queue.indexOf(counterpart), 1);
    }
  } else if (active_diplomacy_meeting_id == counterpart) {

  var agree_sprite = get_treaty_agree_thumb_up();
  var disagree_sprite = get_treaty_disagree_thumb_down();


  var agree_self_html = "<div id='agreement' style='float:left; background: transparent url("
           + agree_sprite['image-src']
           + "); background-position:-" + agree_sprite['tileset-x'] + "px -"
	   + agree_sprite['tileset-y']
           + "px;  width: " + agree_sprite['width'] + "px;height: "
	   + agree_sprite['height'] + "px; margin: 5px; '>"
           + "</div>";

  var disagree_self_html = "<div id='agreement' style='float:left; background: transparent url("
           + disagree_sprite['image-src']
           + "); background-position:-" + disagree_sprite['tileset-x'] + "px -"
	   + disagree_sprite['tileset-y']
           + "px;  width: " + disagree_sprite['width'] + "px;height: "
	   + disagree_sprite['height'] + "px; margin: 5px; '>"
           + "</div>";
    if (I_accepted == true) {
      $("#agree_self").html(agree_self_html);
    } else {
      $("#agree_self").html(disagree_self_html);
    }

    if (other_accepted) {
      $("#agree_counterpart").html(agree_self_html);
    } else {
      $("#agree_counterpart").html(disagree_self_html);
    }
  }

  setTimeout(refresh_diplomacy_request_queue, 1000);

}

/**************************************************************************
 ...
**************************************************************************/
function cancel_meeting_req()
{
  var packet = {"pid" : packet_diplomacy_cancel_meeting_req,
	        "counterpart" : active_diplomacy_meeting_id};
  send_request(JSON.stringify(packet));
}

/**************************************************************************
 ...
**************************************************************************/
function create_clause_req(giver, type, value)
{

  if (type == CLAUSE_CEASEFIRE || type == CLAUSE_PEACE || type == CLAUSE_ALLIANCE) {
    // eg. creating peace treaty requires removing ceasefire first.
    var clauses = diplomacy_clause_map[active_diplomacy_meeting_id];
    for (var i = 0; i < clauses.length; i++) {
      var clause = clauses[i];
      if (clause['type'] == CLAUSE_CEASEFIRE || clause['type'] == CLAUSE_PEACE|| clause['type'] == CLAUSE_ALLIANCE) {
        remove_clause_req(i);
      }
    }
  }

  var packet = {"pid" : packet_diplomacy_create_clause_req,
                "counterpart" : active_diplomacy_meeting_id,
                "giver" : giver,
                "type" : type,
                "value" : value};
  send_request(JSON.stringify(packet));
}


/**************************************************************************
 ...
**************************************************************************/
function cancel_meeting(counterpart)
{
  if (active_diplomacy_meeting_id == counterpart) {
    $("#diplomacy_dialog").remove(); //close the dialog.
    active_diplomacy_meeting_id = null;
  }

  if (diplomacy_request_queue.indexOf(counterpart) >= 0) {
    diplomacy_request_queue.splice(diplomacy_request_queue.indexOf(counterpart), 1);
  }

  setTimeout(refresh_diplomacy_request_queue, 1000);
}

/**************************************************************************
 ...
**************************************************************************/
function show_diplomacy_clauses()
{
  if (active_diplomacy_meeting_id != null) {
    var clauses = diplomacy_clause_map[active_diplomacy_meeting_id];
    var diplo_html = "";
    for (var i = 0; i < clauses.length; i++) {
      var clause = clauses[i];
      var diplo_str = client_diplomacy_clause_string(clause['counterpart'],
 		          clause['giver'],
                  clause['type'],
                  clause['value']);
      diplo_html += "<a href='#' onclick='remove_clause_req(" + i + ");'>" + diplo_str + "</a><br>";

    }

    $("#diplomacy_messages").html(diplo_html);
  }

}

/**************************************************************************
 ...
**************************************************************************/
function remove_clause_req(clause_no)
{
  var clauses = diplomacy_clause_map[active_diplomacy_meeting_id];
  var clause = clauses[clause_no];

  var packet = {"pid" : packet_diplomacy_remove_clause_req,
	            "counterpart" : clause['counterpart'],
                "giver": clause['giver'],
                "type" : clause['type'],
                "value": clause['value'] };
  send_request(JSON.stringify(packet));
}

/**************************************************************************
 ...
**************************************************************************/
function remove_clause(remove_clause_obj)
{
  var clause_list = diplomacy_clause_map[remove_clause_obj['counterpart']];
  for (var i = 0; i < clause_list.length; i++) {
    var check_clause = clause_list[i];
    if (remove_clause_obj['counterpart'] == check_clause['counterpart']
	&& remove_clause_obj['giver'] == check_clause['giver']
        && remove_clause_obj['type'] == check_clause['type']) {

      clause_list.splice(i, 1);
      break;
    }
  }

  show_diplomacy_clauses();
}

/**************************************************************************
 ...
**************************************************************************/
function client_diplomacy_clause_string(counterpart, giver, type, value)
{

  var pplayer = null;
  var nation = null;

  pplayer = players[giver];
  nation = nations[pplayer['nation']]['adjective'];

  switch (type) {
  case CLAUSE_ADVANCE:
    var ptech = techs[value];
    return "The " + nation + " give " + ptech['name'];
  case CLAUSE_CITY:
    var pcity = cities[value];

    if (pcity != null) {
      return "The " + nation + " give " + decodeURIComponent(pcity['name']);
    } else {
      return "The " + nation + " give unknown city.";
    }
    break;

  case CLAUSE_GOLD:
    if (giver == client.conn.playing['playerno']) {
      $("#self_gold").val(value);
    } else {
      $("#counterpart_gold").val(value);
    }
    return "The " + nation + " give " + value + " gold";
  case CLAUSE_MAP:
    return "The " + nation + " give their worldmap";
  case CLAUSE_SEAMAP:
    return "The " + nation + " give their seamap";
  case CLAUSE_CEASEFIRE:
    return "The parties agree on a cease-fire";
  case CLAUSE_PEACE:
    return "The parties agree on a peace";
  case CLAUSE_ALLIANCE:
    return "The parties create an alliance";
  case CLAUSE_VISION:
    return "The " + nation + " give shared vision";
  case CLAUSE_EMBASSY:
    return "The " + nation + " give an embassy";
  }

  return "";
}



/**************************************************************************
 ...
**************************************************************************/
function diplomacy_cancel_treaty(player_id)
{
  var packet = {"pid" : packet_diplomacy_cancel_pact,
	        "other_player_id" : player_id,
                "clause" : DS_CEASEFIRE};
  send_request(JSON.stringify(packet));

  update_nation_screen();

  setTimeout(update_nation_screen, 500);
  setTimeout(update_nation_screen, 1500);
}



/**************************************************************************
 ...
**************************************************************************/
function create_diplomacy_dialog(counterpart, template) {
  var pplayer = client.conn.playing;

  // reset diplomacy_dialog div.
  $("#dialog").remove();
  $("#diplomacy_dialog").remove();
  $("#self-items").remove();
  $("#counterpart-items").remove();
  $(".positionHelper").remove();
  $("#game_page").append(template({
    self: meeting_template_data(pplayer, counterpart),
    counterpart: meeting_template_data(counterpart, pplayer)
  }));

  var title = "Diplomacy: " + counterpart['name']
		 + " of the " + nations[counterpart['nation']]['adjective'];

  $("#diplomacy_dialog").attr("title", title);
  $("#diplomacy_dialog").dialog({
			bgiframe: true,
			modal: false,
			width: is_small_screen() ? "90%" : "50%",
			height: 500,
			buttons: {
				"Accept treaty": function() {
				        accept_treaty_req();
				},
				"Cancel meeting" : function() {
				        cancel_meeting_req();
				}
			},
			close: function() {
			     cancel_meeting_req();
			}
		}).dialogExtend({
           "minimizable" : true,
           "closable" : true,
           "icons" : {
             "minimize" : "ui-icon-circle-minus",
             "restore" : "ui-icon-bullet"
           }});

  $("#diplomacy_dialog").dialog('open');
  $(".ui-dialog").css("overflow", "visible");

  var nation = nations[pplayer['nation']];
  if (nation['customized']) {
    meeting_paint_custom_flag(nation, document.getElementById('flag_self'));
  }
  nation = nations[counterpart['nation']];
  if (nation['customized']) {
    meeting_paint_custom_flag(nation, document.getElementById('flag_counterpart'));
  }

  /* setup fg-menu */
  $('#hierarchy_self').fgmenu({
    content: $('#self-items').html(),
    flyOut: true
  });

  $('#hierarchy_counterpart').fgmenu({
    content: $('#counterpart-items').html(),
    flyOut: true
  });

  if (game_info.trading_gold) {
    $("#self_gold").attr({
       "max" : pplayer['gold'],
       "min" : 0
    });

    $("#counterpart_gold").attr({
       "max" : counterpart['gold'],
       "min" : 0
    });

    var wto;
    $("#counterpart_gold").change(function() {
      clearTimeout(wto);
      wto = setTimeout(function() {
        meeting_gold_change_req(counterpart['playerno'],
                                parseFloat($("#counterpart_gold").val()));
      }, 500);
    });

    $("#self_gold").change(function() {
      clearTimeout(wto);
      wto = setTimeout(function() {
        meeting_gold_change_req(pplayer['playerno'],
                                parseFloat($("#self_gold").val()));
      }, 500);
    });

  } else {
    $("#self_gold").prop("disabled", true).parent().hide();
    $("#counterpart_gold").prop("disabled", true).parent().hide();
  }

  $("#diplomacy_dialog").parent().css("z-index", 1000);
}

function meeting_paint_custom_flag(nation, flag_canvas)
{
  var tag = "f." + nation['graphic_str'];
  var flag_canvas_ctx = flag_canvas.getContext("2d");
  flag_canvas_ctx.scale(1.5, 1.5);
  flag_canvas_ctx.drawImage(sprites[tag], 0, 0);
}

/**************************************************************************
 Request update of gold clause
**************************************************************************/
function meeting_gold_change_req(giver, gold)
{
  var clauses = diplomacy_clause_map[active_diplomacy_meeting_id];
  if (clauses != null) {
    for (var i = 0; i < clauses.length; i++) {
      var clause = clauses[i];
      if (clause['giver'] == giver && clause['type'] == CLAUSE_GOLD) {
        if (clause['value'] == gold) return;
        remove_clause_req(i);
      }
    }
  }

  if (gold != 0) {
    var packet = {"pid" : packet_diplomacy_create_clause_req,
                  "counterpart" : active_diplomacy_meeting_id,
                  "giver" : giver,
                  "type" : CLAUSE_GOLD,
                  "value" : gold};
    send_request(JSON.stringify(packet));
  }
}

/**************************************************************************
 Build data object for the dialog template.
**************************************************************************/
function meeting_template_data(giver, taker)
{
  var data = {};
  var nation = nations[giver['nation']];

  if (!nation['customized']) {
    data.flag = nation['graphic_str'] + "-web" + get_tileset_file_extention();
  }

  data.adjective = nation['adjective'];
  data.name = giver['name'];
  data.pid = giver['playerno'];

  var all_clauses = [];

  var clauses = [];
  clauses.push({type: CLAUSE_MAP, value: 1, name: 'World-map'});
  clauses.push({type: CLAUSE_SEAMAP, value: 1, name: 'Sea-map'});
  all_clauses.push({title: 'Maps...', clauses: clauses});

  if (game_info.trading_tech) {
    clauses = [];
    for (var tech_id in techs) {
      if (player_invention_state(giver, tech_id) == TECH_KNOWN
          && (player_invention_state(taker, tech_id) == TECH_UNKNOWN
              || player_invention_state(taker, tech_id) == TECH_PREREQS_KNOWN)) {
        var ptech = techs[tech_id];
        clauses.push({
          type: CLAUSE_ADVANCE,
          value: tech_id,
          name: techs[tech_id]['name']
        });
      }
    }
    if (clauses.length > 0) {
      all_clauses.push({title: 'Advances...', clauses: clauses});
    }
  }

  if (game_info.trading_city && !is_longturn()) {
    clauses = [];
    for (var city_id in cities) {
      var pcity = cities[city_id];
      if (city_owner(pcity) == giver
          && !does_city_have_improvement(pcity, "Palace")) {
        clauses.push({
          type: CLAUSE_CITY,
          value: city_id,
          name: decodeURIComponent(pcity['name'])
        });
      }
    }
    if (clauses.length > 0) {
      all_clauses.push({title: 'Cities...', clauses: clauses});
    }
  }

  all_clauses.push({type: CLAUSE_VISION, value: 1, name: 'Give shared vision'});
  all_clauses.push({type: CLAUSE_EMBASSY, value: 1, name: 'Give embassy'});

  if (giver == client.conn.playing) {
    all_clauses.push({
      title: 'Pacts...',
      clauses: [
        {type: CLAUSE_CEASEFIRE, value: 1, name: 'Cease-fire'},
        {type: CLAUSE_PEACE, value: 1, name: 'Peace'},
        {type: CLAUSE_ALLIANCE, value: 1, name: 'Alliance'}
      ]
    });
  }

  data.clauses = all_clauses;

  return data;
}

