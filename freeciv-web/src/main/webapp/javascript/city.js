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


var cities = {};
var city_rules = {};
var city_trade_routes = {};

var goods = {};

var active_city = null;
var worklist_dialog_active = false;
var selected_value = -1;
var selected_kind = -1;

/* The city_options enum. */
var CITYO_DISBAND      = 0;
var CITYO_NEW_EINSTEIN = 1;
var CITYO_NEW_TAXMAN   = 2;
var CITYO_LAST         = 3;

var FEELING_BASE = 0;		/* before any of the modifiers below */
var FEELING_LUXURY = 1;		/* after luxury */
var FEELING_EFFECT = 2;		/* after building effects */
var FEELING_NATIONALITY = 3;  	/* after citizen nationality effects */
var FEELING_MARTIAL = 4;	/* after units enforce martial order */
var FEELING_FINAL = 5;		/* after wonders (final result) */

var MAX_LEN_WORKLIST = 64;

var INCITE_IMPOSSIBLE_COST = (1000 * 1000 * 1000);

var city_tab_index = 0;
var city_prod_clicks = 0;

/**************************************************************************
 ...
**************************************************************************/
function city_tile(pcity)
{
  if (pcity == null) return null;

  return index_to_tile(pcity['tile']);
}

/**************************************************************************
 ...
**************************************************************************/
function city_owner_player_id(pcity)
{
  if (pcity == null) return null;
  return pcity['owner'];
}

/**************************************************************************
 ...
**************************************************************************/
function city_owner(pcity)
{
  return players[city_owner_player_id(pcity)];
}

/**************************************************************************
 Removes a city from the game
**************************************************************************/
function remove_city(pcity_id)
{
  if (pcity_id == null) return;
  var ptile = city_tile(cities[pcity_id]);
  delete cities[pcity_id];
  if (renderer == RENDERER_WEBGL) update_city_position(ptile);

}

/**************************************************************************
 ...
**************************************************************************/
function is_city_center(city, tile) {
  return (city['tile'] == tile['index']);
}

/**************************************************************************
 ...
**************************************************************************/
function is_free_worked(city, tile) {
  return (city['tile'] == tile['index']);
}


/**************************************************************************
 ...
**************************************************************************/
function show_city_dialog_by_id(pcity_id)
{
  show_city_dialog(cities[pcity_id]);
}

/**************************************************************************
 Show the city dialog, by loading a Handlebars template, and filling it
 with data about the city.
**************************************************************************/
function show_city_dialog(pcity)
{
  var turns_to_complete = FC_INFINITY;
  var sprite;
  var punit;
  var ptype;

  if (active_city != pcity || active_city == null) city_prod_clicks = 0;
  active_city = pcity;

  if (pcity == null) return;

  // reset dialog page.
  $("#city_dialog").remove();
  $("<div id='city_dialog'></div>").appendTo("div#game_page");

  var city_data = {};

  $.ajax({
    url: "/webclient/city.hbs?ts=" + ts,
    dataType: "html",
    cache: true,
    async: false
  }).fail(function() {
    console.error("Unable to load city dialog template.");
  }).done(function( data ) {
    var template=Handlebars.compile(data);
    $("#city_dialog").html(template(city_data));
  });

  $("#city_canvas").click(city_mapview_mouse_click);

  show_city_traderoutes();

  var dialog_buttons = {};
  if (!is_small_screen()) {
    dialog_buttons = $.extend(dialog_buttons,
      {
       "Previous city" : function() {
         previous_city();
       },
       "Next city" : function() {
         next_city();
       }
     });
   } else {
       dialog_buttons = $.extend(dialog_buttons,
         {
          "Next" : function() {
            next_city();
          }
        });
   }

   dialog_buttons = $.extend(dialog_buttons,
   {
     "Buy" : function() {
       request_city_buy();
      },
      "Rename" : function() {
       rename_city();
      },
      "Close": function() {
       close_city_dialog();
      }
    });

  $("#city_dialog").attr("title", decodeURIComponent(pcity['name'])
                         + " (" + pcity['size'] + ")");
  $("#city_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "98%" : "80%",
                        height: is_small_screen() ? $(window).height() + 10 : $(window).height() - 80,
                        close : function(){
                          close_city_dialog();
                        },
			buttons: dialog_buttons
                     });

  $("#city_dialog").dialog('open');
  $("#game_text_input").blur();

  /* prepare city dialog for small screens. */
  if (!is_small_screen()) {
    $("#city_tabs-5").remove();
    $("#city_tabs-6").remove();
    $(".extra_tabs_small").remove();
  } else {
    $("#city_tabs-4").remove();
    $(".extra_tabs_big").remove();
    var units_element = $("#city_improvements_panel").detach();
    $('#city_units_tab').append(units_element);
   }

  $("#city_tabs").tabs({ active: city_tab_index});

  $(".citydlg_tabs").height(is_small_screen() ? $(window).height() - 110 : $(window).height() - 225);

  city_worklist_dialog(pcity);

  var orig_renderer = renderer;
  renderer = RENDERER_2DCANVAS;
  set_city_mapview_active();
  center_tile_mapcanvas(city_tile(pcity));
  update_map_canvas(0, 0, mapview['store_width'], mapview['store_height']);
  renderer = orig_renderer;

  $("#city_size").html("Population: " + numberWithCommas(city_population(pcity)*1000) + "<br>"
                       + "Size: " + pcity['size'] + "<br>"
                       + "Granary: " + pcity['food_stock'] + "/" + pcity['granary_size'] + "<br>"
                       + "Change in: " + city_turns_to_growth_text(pcity));

  var prod_type = get_city_production_type_sprite(pcity);
  $("#city_production_overview").html("Producing: " + (prod_type != null ? prod_type['type']['name'] : "None"));

  turns_to_complete = get_city_production_time(pcity);

  if (turns_to_complete != FC_INFINITY) {
    $("#city_production_turns_overview").html("Turns to completion: " + turns_to_complete);
  } else {
    $("#city_production_turns_overview").html("-");
  }

  var improvements_html = "";
  for (var z = 0; z < ruleset_control.num_impr_types; z ++) {
    if (pcity['improvements'] != null && pcity['improvements'].isSet(z)) {
       sprite = get_improvement_image_sprite(improvements[z]);
       if (sprite == null) {
         console.log("Missing sprite for improvement " + z);
         continue;
       }

      improvements_html = improvements_html +
       "<div id='city_improvement_element'><div style='background: transparent url("
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y']
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           + "title=\"" + improvements[z]['helptext'] + "\" "
	   + "onclick='city_sell_improvement(" + z + ");'>"
           +"</div>" + improvements[z]['name'] + "</div>";
    }
  }
  $("#city_improvements_list").html(improvements_html);

  var punits = tile_units(city_tile(pcity));
  if (punits != null) {
    var present_units_html = "";
    for (var r = 0; r < punits.length; r++) {
      punit = punits[r];
      ptype = unit_type(punit);
      sprite = get_unit_image_sprite(punit);
      if (sprite == null) {
         console.log("Missing sprite for " + punit);
         continue;
       }

      present_units_html = present_units_html +
       "<div class='game_unit_list_item' title='" + get_unit_city_info(punit)
           + "' style='cursor:pointer;cursor:hand; background: transparent url("
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y']
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           + " onclick='city_dialog_activate_unit(units[" + punit['id'] + "]);'"
           +"></div>";
    }
    $("#city_present_units_list").html(present_units_html);
  }

  var sunits = get_supported_units(pcity);
  if (sunits != null) {
    var supported_units_html = "";
    for (var t = 0; t < sunits.length; t++) {
      punit = sunits[t];
      ptype = unit_type(punit);
      sprite = get_unit_image_sprite(punit);
      if (sprite == null) {
         console.log("Missing sprite for " + punit);
         continue;
       }

      supported_units_html = supported_units_html +
       "<div class='game_unit_list_item' title='" + get_unit_city_info(punit)
           + "' style='cursor:pointer;cursor:hand; background: transparent url("
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y']
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           + " onclick='city_dialog_activate_unit(units[" + punit['id'] + "]);'"
           +"></div>";
    }
    $("#city_supported_units_list").html(supported_units_html);
  }
  $(".game_unit_list_item").tooltip();

  if ('prod' in pcity && 'surplus' in pcity) {
    var food_txt = pcity['prod'][O_FOOD] + " ( ";
    if (pcity['surplus'][O_FOOD] > 0) food_txt += "+";
    food_txt += pcity['surplus'][O_FOOD] + ")";

    var shield_txt = pcity['prod'][O_SHIELD] + " ( ";
    if (pcity['surplus'][O_SHIELD] > 0) shield_txt += "+";
    shield_txt += pcity['surplus'][O_SHIELD] + ")";

    var trade_txt = pcity['prod'][O_TRADE] + " ( ";
    if (pcity['surplus'][O_TRADE] > 0) trade_txt += "+";
    trade_txt += pcity['surplus'][O_TRADE] + ")";

    var gold_txt = pcity['prod'][O_GOLD] + " ( ";
    if (pcity['surplus'][O_GOLD] > 0) gold_txt += "+";
    gold_txt += pcity['surplus'][O_GOLD] + ")";

    var luxury_txt = pcity['prod'][O_LUXURY];
    var science_txt = pcity['prod'][O_SCIENCE];

    $("#city_food").html(food_txt);
    $("#city_prod").html(shield_txt);
    $("#city_trade").html(trade_txt);
    $("#city_gold").html(gold_txt);
    $("#city_luxury").html(luxury_txt);
    $("#city_science").html(science_txt);

    $("#city_corruption").html(pcity['waste'][O_TRADE]);
    $("#city_waste").html(pcity['waste'][O_SHIELD]);
    $("#city_pollution").html(pcity['pollution']);
  }

  /* Handle citizens and specialists */
  var specialist_html = "";
  var citizen_types = ["angry", "unhappy", "content", "happy"];
  for (var s = 0; s < citizen_types.length; s++) {
    if (pcity['ppl_' + citizen_types[s]] == null) continue;
    for (var i = 0; i < pcity['ppl_' + citizen_types[s]][FEELING_FINAL]; i ++) {
      sprite = get_specialist_image_sprite("citizen." + citizen_types[s] + "_"
         + (i % 2));
      specialist_html = specialist_html +
      "<div class='specialist_item' style='background: transparent url("
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y']
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           +" title='One " + citizen_types[s] + " citizen'></div>";
    }
  }

  for (var u = 0; u < pcity['specialists_size']; u++) {
    var spec_type_name = specialists[u]['plural_name'];
    var spec_gfx_key = "specialist." + specialists[u]['rule_name'] + "_0";
    for (var j = 0; j < pcity['specialists'][u]; j++) {
      sprite = get_specialist_image_sprite(spec_gfx_key);
      specialist_html = specialist_html +
      "<div class='specialist_item' style='cursor:pointer;cursor:hand; background: transparent url("
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y']
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           + " onclick='city_change_specialist(" + pcity['id'] + "," + specialists[u]['id'] + ");'"
           +" title='" + spec_type_name + " (click to change)'></div>";

    }
  }
  specialist_html += "<div style='clear: both;'></div>";
  $("#specialist_panel").html(specialist_html);

  $('#disbandable_city').off();
  $('#disbandable_city').prop('checked',
                              pcity['city_options'] != null && pcity['city_options'].isSet(CITYO_DISBAND));
  $('#disbandable_city').click(function() {
    var options = pcity['city_options'];
    var packet = {
      "pid"     : packet_city_options_req,
      "city_id" : active_city['id'],
      "options" : options.raw
    };

    /* Change the option value referred to by the packet. */
    if ($('#disbandable_city').prop('checked')) {
      options.set(CITYO_DISBAND);
    } else {
      options.unset(CITYO_DISBAND);
    }

    /* Send the (now updated) city options. */
    send_request(JSON.stringify(packet));

    city_tab_index = 3;
  });

  city_tab_index = 0;

  if (is_small_screen()) {
   $(".ui-tabs-anchor").css("padding", "2px");
  }
}


/**************************************************************************
 Returns the name and sprite of the current city production.
**************************************************************************/
function get_city_production_type_sprite(pcity)
{
  if (pcity == null) return null; 
  if (pcity['production_kind'] == VUT_UTYPE) {
    var punit_type = unit_types[pcity['production_value']];
    return {"type":punit_type,"sprite":get_unit_type_image_sprite(punit_type)};
  }

  if (pcity['production_kind'] == VUT_IMPROVEMENT) {
    var improvement = improvements[pcity['production_value']];
    return {"type":improvement,"sprite":get_improvement_image_sprite(improvement)};
  }

  return null;
}

/**************************************************************************
 Returns the name of the current city production.
**************************************************************************/
function get_city_production_type(pcity)
{
  if (pcity == null) return null; 
  if (pcity['production_kind'] == VUT_UTYPE) {
    var punit_type = unit_types[pcity['production_value']];
    return punit_type;
  }

  if (pcity['production_kind'] == VUT_IMPROVEMENT) {
    var improvement = improvements[pcity['production_value']];
    return improvement;
  }

  return null;
}


/**************************************************************************
 Returns the number of turns to complete current city production. 
**************************************************************************/
function get_city_production_time(pcity)
{
  if (pcity == null) return FC_INFINITY;
 
  if (pcity['production_kind'] == VUT_UTYPE) {
    var punit_type = unit_types[pcity['production_value']];
    return city_turns_to_build(pcity, punit_type, true);
  }

  if (pcity['production_kind'] == VUT_IMPROVEMENT) {
    var improvement = improvements[pcity['production_value']];
    if (improvement['name'] == "Coinage") {
      return FC_INFINITY;
    }

    return city_turns_to_build(pcity, improvement, true);
  }

  return FC_INFINITY;

}

/**************************************************************************
...
**************************************************************************/
function generate_production_list(pcity)
{
  var production_list = [];
  for (var unit_type_id in unit_types) {
    var punit_type = unit_types[unit_type_id];

    /* FIXME: web client doesn't support unit flags yet, so this is a hack: */
    if (punit_type['name'] == "Barbarian Leader" || punit_type['name'] == "Leader") continue;

    if (can_city_build_unit_now(pcity, punit_type) == true) {
      production_list.push({"kind": VUT_UTYPE, "value" : punit_type['id'],
                            "text" : punit_type['name'],
	                    "helptext" : punit_type['helptext'],
                            "rule_name" : punit_type['rule_name'],
                            "build_cost" : punit_type['build_cost'],
                            "unit_details" : punit_type['attack_strength'] + ", " 
                                             + punit_type['defense_strength'] + ", " 
                                             + punit_type['firepower'],
                            "sprite" : get_unit_type_image_sprite(punit_type)});
    }
  }

  for (var improvement_id in improvements) {
    var pimprovement = improvements[improvement_id];
    if (can_city_build_improvement_now(pcity, pimprovement)) {
      var build_cost = pimprovement['build_cost'];
      if (pimprovement['name'] == "Coinage") build_cost = "-";
      production_list.push({"kind": VUT_IMPROVEMENT,
                            "value" : pimprovement['id'],
                            "text" : pimprovement['name'],
	                    "helptext" : pimprovement['helptext'],
                            "rule_name" : pimprovement['rule_name'],
                            "build_cost" : build_cost,
                            "unit_details" : "-",
                            "sprite" : get_improvement_image_sprite(pimprovement) });
    }
  }
  return production_list;
}

/**************************************************************************
  Return whether given city can build given unit, ignoring whether unit
  is obsolete.
**************************************************************************/
function can_city_build_unit_direct(pcity, punittype)
{
  /* TODO: implement*/
  return true;
}


/**************************************************************************
  Return whether given city can build given unit; returns FALSE if unit is
  obsolete.
**************************************************************************/
function can_city_build_unit_now(pcity, punittype)
{
  return (pcity != null && pcity['can_build_unit'] != null
          && pcity['can_build_unit'][punittype['id']] == "1");
}


/**************************************************************************
  Return whether given city can build given building; returns FALSE if
  the building is obsolete.
**************************************************************************/
function can_city_build_improvement_now(pcity, pimprove)
{
  return (pcity != null && pcity['can_build_improvement'] != null
          && pcity['can_build_improvement'][pimprove['id']] == "1");
}


/**************************************************************************
  Return TRUE iff the city has this building in it.
**************************************************************************/
function city_has_building(pcity, improvement_id)
{
  for (var z = 0; z < ruleset_control.num_impr_types; z ++) {
    if (pcity['improvements'] != null && pcity['improvements'].isSet(z) && z == improvement_id) {
      return true;
    }
  }
  return false;
}


/**************************************************************************
 Calculates the turns which are needed to build the requested
 improvement in the city.  GUI Independent.
**************************************************************************/
function city_turns_to_build(pcity,
							 target,
			                 include_shield_stock)
{
  var city_shield_surplus = pcity['last_turns_shield_surplus'];
  var city_shield_stock = include_shield_stock ? pcity['shield_stock'] : 0;
  var cost = universal_build_shield_cost(target);

  /*if (target.kind == VUT_IMPROVEMENT
      && is_great_wonder(target.value.building)
      && !great_wonder_is_available(target.value.building)) {
    return FC_INFINITY;
  }*/

  if (include_shield_stock == true && (pcity['shield_stock'] >= cost)) {
    return 1;
  } else if (pcity['last_turns_shield_surplus'] > 0) {
    return Math.floor((cost - city_shield_stock - 1) / city_shield_surplus + 1);
  } else {
    return FC_INFINITY;
  }
}

/**************************************************************************
 Show buy production in city dialog
**************************************************************************/
function request_city_buy()
{
  var pcity = active_city;
  var pplayer = client.conn.playing;

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");
  var buy_price_string = "";
  var buy_question_string = "";

  if (pcity['production_kind'] == VUT_UTYPE) {
    var punit_type = unit_types[pcity['production_value']];
    if (punit_type != null) {
      buy_price_string = punit_type['name'] + " costs " + pcity['buy_gold_cost'] + " gold.";
      buy_question_string = "Buy " + punit_type['name'] + " for " + pcity['buy_gold_cost'] + " gold?";
    }
  } else {
    var improvement = improvements[pcity['production_value']];
    if (improvement != null) {
      buy_price_string = improvement['name'] + " costs " + pcity['buy_gold_cost'] + " gold.";
      buy_question_string = "Buy " + improvement['name'] + " for " + pcity['buy_gold_cost'] + " gold?";
    }
  }

  var treasury_text = "<br>Treasury contains " + pplayer['gold'] + " gold.";

  if (pcity['buy_gold_cost'] > pplayer['gold']) {
    show_dialog_message("Buy It!",
      buy_price_string + treasury_text);
    return;
  }

  var dhtml = buy_question_string + treasury_text;


  $("#dialog").html(dhtml);

  $("#dialog").attr("title", "Buy It!");
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "95%" : "50%",
			buttons: {
				"Yes": function() {
						send_city_buy();
						$("#dialog").dialog('close');
				},
				"No": function() {
						$("#dialog").dialog('close');

				}
			}
		});

  $("#dialog").dialog('open');


}


/**************************************************************************
 Buy whatever is being built in the city.
**************************************************************************/
function send_city_buy()
{
  if (active_city != null) {
    var packet = {"pid" : packet_city_buy, "city_id" : active_city['id']};
    send_request(JSON.stringify(packet));
  }
}

/**************************************************************************
 Change city production.
**************************************************************************/
function send_city_change(city_id, kind, value)
{
  var packet = {"pid" : packet_city_change, "city_id" : city_id,
                "production_kind": kind, "production_value" : value};
  send_request(JSON.stringify(packet));


}

/**************************************************************************
 Close dialog.
**************************************************************************/
function close_city_dialog()
{
  $("#city_dialog").dialog('close');
  set_default_mapview_active();
  if (active_city != null) {
    setup_window_size ();
    center_tile_mapcanvas(city_tile(active_city));
    active_city = null;

  }
  keyboard_input=true;
  worklist_dialog_active = false;
}

/**************************************************************************
 The city map has been clicked.
**************************************************************************/
function do_city_map_click(ptile)
{
  var packet = null;
  var c_tile = index_to_tile(active_city['tile']);
  if (ptile['worked'] == active_city['id']) {
    packet = {"pid" : packet_city_make_specialist,
	         "city_id" : active_city['id'],
                 "worker_x" : ptile['x'],
		 "worker_y" : ptile['y']};
  } else {
    packet = {"pid" : packet_city_make_worker,
	         "city_id" : active_city['id'],
                 "worker_x" : ptile['x'],
		 "worker_y" : ptile['y']};
  }
  send_request(JSON.stringify(packet));

}

/**************************************************************************
..
**************************************************************************/
function does_city_have_improvement(pcity, improvement_name)
{
  if (pcity == null || pcity['improvements'] == null) return false;

  for (var z = 0; z < ruleset_control.num_impr_types; z++) {
    if (pcity['improvements'] != null && pcity['improvements'].isSet(z) && improvements[z] != null
        && improvements[z]['name'] == improvement_name) {
      return true;
    }
  }
  return false;
}

/**************************************************************************
  Shows the Request city name dialog to the user.
**************************************************************************/
function city_name_dialog(suggested_name, unit_id) {
  // reset dialog page.
  $("#city_name_dialog").remove();
  $("<div id='city_name_dialog'></div>").appendTo("div#game_page");

  $("#city_name_dialog").html($("<div>What should we call our new city?</div>"
                              + "<input id='city_name_req' type='text'>"));

  /* A suggested city name can contain an apostrophe ("'"). That character
   * is also used for single quotes. It shouldn't be added unescaped to a
   * string that later is interpreted as HTML. */
  /* TODO: Forbid city names containing an apostrophe or make sure that all
   * JavaScript using city names handles it correctly. Look for places
   * where a city name string is added to a string that later is
   * interpreted as HTML. Avoid the situation by directly using JavaScript
   * like below or by escaping the string. */
  $("#city_name_req").attr("value", suggested_name);

  $("#city_name_dialog").attr("title", "Build New City");
  $("#city_name_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: "300",
			close: function() {
				keyboard_input=true;
			},
			buttons: [	{
					text: "Cancel",
				        click: function() {
						$("#city_name_dialog").remove();
        					keyboard_input=true;
					}
				},{
					text: "Ok",
				        click: function() {
						var name = $("#city_name_req").val();
						if (name.length == 0 || name.length >= MAX_LEN_NAME - 4
						    || encodeURIComponent(name).length  >= MAX_LEN_NAME - 4) {
						  swal("City name is invalid. Please try a different shorter name.");
						  return;
						}

                        var actor_unit = game_find_unit_by_number(unit_id);

                        var packet = {"pid" : packet_unit_do_action,
                                                              "actor_id" : unit_id,
                                                              "target_id": actor_unit['tile'],
                                                              "value" : 0,
                                                              "name" : encodeURIComponent(name),
                                                              "action_type": ACTION_FOUND_CITY};
						send_request(JSON.stringify(packet));
						$("#city_name_dialog").remove();
						keyboard_input=true;
					}
					}
				]
		});

  $("#city_name_req").attr('maxlength', MAX_LEN_NAME);

  $("#city_name_dialog").dialog('open');

  $('#city_name_dialog').keyup(function(e) {
    if (e.keyCode == 13) {
    	var name = $("#city_name_req").val();

        var actor_unit = game_find_unit_by_number(unit_id);

        var packet = {"pid" : packet_unit_do_action,
                      "actor_id" : unit_id,
                      "target_id": actor_unit['tile'],
                      "value" : 0,
                      "name" : encodeURIComponent(name),
                      "action_type": ACTION_FOUND_CITY};
	send_request(JSON.stringify(packet));
	$("#city_name_dialog").remove();
        keyboard_input=true;
    }
  });
  blur_input_on_touchdevice();
  keyboard_input=false;
}

/**************************************************************************
..
**************************************************************************/
function next_city()
{
  if (!client.conn.playing) return;
  var city_id;

  var is_next = false;
  for (city_id in cities) {
    var pcity = cities[city_id];
    if (is_next && city_owner(pcity).playerno == client.conn.playing.playerno) {
      show_city_dialog(pcity);
      return;
    }
    if (active_city['id'] == city_id) {
      is_next = true;
    }
  }

  for (city_id in cities) {
    var scity = cities[city_id];
    if (is_next && city_owner(scity).playerno == client.conn.playing.playerno) {
      show_city_dialog(scity);
      return;
    }
  }
}

/**************************************************************************
..
**************************************************************************/
function previous_city()
{
  if (!client.conn.playing) return;

  var prev_city = null;
  for (var city_id in cities) {
    var next_city = cities[city_id];

    if (prev_city != null && active_city['id'] == city_id
        && city_owner(next_city).playerno == client.conn.playing.playerno) {
      show_city_dialog(prev_city);
      return;
    }
    if (city_owner(next_city).playerno == client.conn.playing.playerno) {
      prev_city = next_city;
    }
  }
  if (prev_city != null) {
    show_city_dialog(prev_city);
  }


}

/**************************************************************************
..
**************************************************************************/
function city_sell_improvement(improvement_id)
{
  var agree=confirm("Are you sure you want to sell this building?");
  if (agree) {

    var packet = {"pid" : packet_city_sell, "city_id" : active_city['id'],
                  "build_id": improvement_id};
    send_request(JSON.stringify(packet));
  }

}

/**************************************************************************
  Create text describing city growth.
**************************************************************************/
function city_turns_to_growth_text(pcity)
{
  var turns = pcity['granary_turns'];

  if (turns == 0) {
    return "blocked";
  } else if (turns > 1000000) {
    return "never";
  } else if (turns < 0) {
    return "Starving in " + Math.abs(turns) + " turns";
  } else {
    return turns + " turns";
  }
}

/**************************************************************************
  Converts from coordinate offset from city center (dx, dy),
  to index in the city_info['food_output'] packet.
**************************************************************************/
function get_city_dxy_to_index(dx, dy)
{
  city_tile_map = {};
  city_tile_map[" 00"] = 0;
  city_tile_map[" 10"] = 1;
  city_tile_map[" 01"] = 2;
  city_tile_map[" 0-1"] = 3;
  city_tile_map[" -10"] = 4;
  city_tile_map[" 11"] = 5;
  city_tile_map[" 1-1"] = 6;
  city_tile_map[" -11"] = 7;
  city_tile_map[" -1-1"] = 8;
  city_tile_map[" 20"] = 9;
  city_tile_map[" 02"] = 10;
  city_tile_map[" 0-2"] = 11;
  city_tile_map[" -20"] = 12;
  city_tile_map[" 21"] = 13;
  city_tile_map[" 2-1"] = 14;
  city_tile_map[" 12"] = 15;
  city_tile_map[" 1-2"] = 16;
  city_tile_map[" -12"] = 17;
  city_tile_map[" -1-2"] = 18;
  city_tile_map[" -21"] = 19;
  city_tile_map[" -2-1"] = 20;

  return city_tile_map[" "+dx+""+dy];

}


/**************************************************************************
...
**************************************************************************/
function city_change_specialist(city_id, from_specialist_id)
{
  var city_message = {"pid": packet_city_change_specialist,
                      "city_id" : city_id,
                      "from" : from_specialist_id,
                      "to" : (from_specialist_id + 1) % 3};
  send_request(JSON.stringify(city_message));
}


/**************************************************************************
...  (simplified)
**************************************************************************/
function city_can_buy(pcity)
{
  var improvement = improvements[pcity['production_value']];

  return (!pcity['did_buy'] && pcity['turn_founded'] != game_info['turn']
          && improvement['name'] != "Coinage");

}

/**************************************************************************
 Returns how many thousand citizen live in this city.
**************************************************************************/
function city_population(pcity)
{
  /*  Sum_{i=1}^{n} i  ==  n*(n+1)/2  */
  return pcity['size'] * (pcity['size'] + 1) * 5;
}


/**************************************************************************
 Rename a city
**************************************************************************/
function rename_city()
{
  if (active_city == null) return;

  // reset dialog page.
  $("#city_name_dialog").remove();
  $("<div id='city_name_dialog'></div>").appendTo("div#game_page");

  $("#city_name_dialog").html($("<div>What should we call this city?</div>"
                                + "<input id='city_name_req' type='text'>"));
  /* The city name can contain an apostrophe ("'"). That character is also
   * used for single quotes. It shouldn't be added unescaped to a
   * string that later is interpreted as HTML. */
  $("#city_name_req").attr("value", active_city['name']);
  $("#city_name_dialog").attr("title", "Rename City");
  $("#city_name_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: "300",
			close: function() {
				keyboard_input=true;
			},
			buttons: [	{
					text: "Cancel",
				        click: function() {
						$("#city_name_dialog").remove();
        					keyboard_input=true;
					}
				},{
					text: "Ok",
				        click: function() {
						var name = $("#city_name_req").val();
						if (name.length == 0 || name.length >= MAX_LEN_NAME - 4
						    || encodeURIComponent(name).length  >= MAX_LEN_NAME - 4) {
						  swal("City name is invalid");
						  return;
						}

						var packet = {"pid" : packet_city_rename, "name" : encodeURIComponent(name), "city_id" : active_city['id'] };
						send_request(JSON.stringify(packet));
						$("#city_name_dialog").remove();
						keyboard_input=true;
					}
					}
				]
		});
  $("#city_name_req").attr('maxlength', MAX_LEN_NAME);

  $("#city_name_dialog").dialog('open');

  $('#city_name_dialog').keyup(function(e) {
    if (e.keyCode == 13) {
      var name = $("#city_name_req").val();
      var packet = {"pid" : packet_city_rename, "name" : encodeURIComponent(name), "city_id" : active_city['id'] };
      send_request(JSON.stringify(packet));
      $("#city_name_dialog").remove();
      keyboard_input=true;
    }
  });
  keyboard_input=false;


}

/**************************************************************************
 Shows traderoutes of active city
**************************************************************************/
function show_city_traderoutes()
{
  var msg;
  var routes;

  if (active_city == null) {
    /* No city to show. */
    return;
  }

  routes = city_trade_routes[active_city['id']];

  if (active_city['traderoute_count'] != 0
      && routes == null) {
    /* This city is supposed to have trade routes. It doesn't.  */
    console.log("Can't find the trade routes " + active_city['name']
                + " is said to have");
    return;
  }

  msg = "";
  for (var i = 0; i < active_city['traderoute_count']; i++) {
    var tcity_id;
    var tcity;
    var good;

    tcity_id = routes[i]['partner'];

    if (tcity_id == 0 || tcity_id == null) {
      continue;
    }

    good = goods[routes[i]['goods']];
    if (good == null) {
      console.log("Missing good type " + routes[i]['goods']);
      good = {'name': "Unknown"};
    }

    tcity = cities[tcity_id];
    if (tcity == null) continue;
    msg += good['name'] + " trade with " + tcity['name'];
    msg += " gives " + routes[i]['value'] + " gold each turn." + "<br>";
  }

  if (msg == "") {
    msg = "No traderoutes.";
    msg += " (Open the Manual, select Economy and then Trade ";
    msg += "if you want to learn more about trade and trade routes.)";
  }

  $("#city_traderoutes_tab").html(msg);

}

/**************************************************************************
 Populates data to the production tab in the city dialog.
**************************************************************************/
function city_worklist_dialog(pcity)
{
  if (pcity == null) return;
  var universals_list = [];
  var kind;
  var value;

  if (pcity['worklist'] != null && pcity['worklist'].length != 0) {
    var work_list = pcity['worklist'];
    for (var i = 0; i < work_list.length; i++) {
      var work_item = work_list[i];
      kind = work_item['kind'];
      value = work_item['value'];
      if (kind == null || value == null || work_item.length == 0) continue;
      if (kind == VUT_IMPROVEMENT) {
        var pimpr = improvements[value];
	var build_cost = pimpr['build_cost'];
	if (pimpr['name'] == "Coinage") build_cost = "-";
	universals_list.push({"name" : pimpr['name'],
		"helptext" : pimpr['helptext'],
		"build_cost" : build_cost,
		"sprite" : get_improvement_image_sprite(pimpr)});
      } else if (kind == VUT_UTYPE) {
        var putype = unit_types[value];
        universals_list.push({"name" : putype['name'],
		"helptext" : putype['helptext'],
		"build_cost" : putype['build_cost'],
		"sprite" : get_unit_type_image_sprite(putype)});
      } else {
        console.log("unknown kind: " + kind);
      }
    }
  }

  var worklist_html = "<table class='worklist_table'><tr><td>Type</td><td>Name</td><td>Cost</td></tr>";
  for (var j = 0; j < universals_list.length; j++) {
    var universal = universals_list[j];
    sprite = universal['sprite'];
    if (sprite == null) {
      console.log("Missing sprite for " + universal[j]['name']);
      continue;
    }

    worklist_html += "<tr class='prod_choice_list_item' onclick='send_city_worklist_remove("
     + pcity['id'] + "," + j + ")' "
     + " title=\"" + universal['helptext'] + "\">"
     + "<td><div class='production_list_item_sub' style=' background: transparent url("
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y']
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;'"
           +"></div></td>"
     + "<td class='prod_choice_name'>" + universal['name'] + "</td>"
     + "<td class='prod_choice_cost'>" + universal['build_cost'] + "</td></tr>";
  }
  worklist_html += "</table>";
  $("#city_current_worklist").html(worklist_html);

  var production_list = generate_production_list(pcity);
  var production_html = "<table class='worklist_table'><tr><td>Type</td><td>Name</td><td title='Attack/Defense/Firepower'>Info</td><td>Cost</td></tr>";
  for (var a = 0; a < production_list.length; a++) {
    sprite = production_list[a]['sprite'];
    if (sprite == null) {
      console.log("Missing sprite for " + production_list[a]['value']);
      continue;
    }
    kind = production_list[a]['kind'];
    value = production_list[a]['value'];

    production_html += "<tr class='prod_choice_list_item kindvalue_item' data-value='" + value + "' data-kind='" + kind + "'>"
     + "<td><div class='production_list_item_sub' title=\"" + production_list[a]['helptext'] + "\" style=' background: transparent url("
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y']
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;'"
           +"></div></td>"
     + "<td class='prod_choice_name'>" + production_list[a]['text'] + "</td>"
     + "<td class='prod_choice_name'>" + production_list[a]['unit_details'] + "</td>"
     + "<td class='prod_choice_cost'>" + production_list[a]['build_cost'] + "</td></tr>";
  }
  production_html += "</table>";

  $("#worklist_production_choices").html(production_html);

  keyboard_input=false;
  worklist_dialog_active = true;
  var turns_to_complete = get_city_production_time(pcity);
  var prod_type = get_city_production_type_sprite(pcity);
  var prod_img_html = "";
  if (prod_type != null) { 
    sprite = prod_type['sprite'];
    prod_img_html = "<div style='background: transparent url("
           + sprite['image-src']
           + ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y']
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float: left; '>"
           +"</div>";
  }

  var headline = prod_img_html + "<div id='prod_descr'>" 
    + (is_small_screen() ? " " : " Production: ") 
    + (prod_type != null ? prod_type['type']['name'] : "None");

  if (turns_to_complete != FC_INFINITY) {
    headline += ", turns: " + turns_to_complete;
  }

  $("#worklist_dialog_headline").html(headline + "</div>");

  $(".production_list_item_sub").tooltip();

  if (!is_touch_device()) {
    $(".worklist_table").selectable({ filter: "tr",
       selected: function( event, ui ) {handle_worklist_select(ui); }});

    $(".kindvalue_item").dblclick(function() {
      value = parseFloat($(this).data('value'));
      kind = parseFloat($(this).data('kind'));
      if (selected_kind >= 0 && selected_value >= 0) {
        send_city_worklist_add(pcity['id'], kind, value);
      }
    });
  } else {
    $(".kindvalue_item").click(function() {
      value = parseFloat($(this).data('value'));
      kind = parseFloat($(this).data('kind'));
      if (city_prod_clicks == 0) {
        send_city_change(pcity['id'], kind, value);
      } else {
        send_city_worklist_add(pcity['id'], kind, value);
      }
      city_tab_index = 1;
      city_prod_clicks += 1;

    });
    $("#prod_buttons").html("<x-small>Click to change production, next clicks will add to worklist on mobile.</x-small>");
  }

  $(".button").button();
  $("#city_current_worklist").height($("#city_production_tab").height() - 150);
  $("#worklist_production_choices").height($("#city_production_tab").height() - 100);
}

/**************************************************************************
 ...
**************************************************************************/
function handle_worklist_select( ui )
{
  selected_value = parseFloat($(ui.selected).data("value"));
  selected_kind = parseFloat($(ui.selected).data("kind"));
}

/**************************************************************************
...
**************************************************************************/
function send_city_worklist_add(city_id, kind, value)
{
  var pcity = cities[city_id];
  if (pcity['worklist'].length >= MAX_LEN_WORKLIST) {
    return;
  }

  pcity['worklist'].push({"kind" : kind, "value" : value});

  var city_message = {"pid"      : packet_city_worklist,
                      "city_id"  : city_id,
                      "worklist" : pcity['worklist']};
  send_request(JSON.stringify(city_message));
  city_tab_index = 1;
}

/**************************************************************************
...
**************************************************************************/
function send_city_worklist_remove(city_id, index)
{
  var pcity = cities[city_id];

  pcity['worklist'].splice(index, 1);

  var city_message = {"pid"      : packet_city_worklist,
                      "city_id"  : city_id,
                      "worklist" : pcity['worklist']};
  send_request(JSON.stringify(city_message));
  city_tab_index = 1;
}

/**************************************************************************
...
**************************************************************************/
function city_change_production()
{
  if (selected_kind != -1 && selected_value != -1) {
    send_city_change(active_city['id'], selected_kind, selected_value);
    city_tab_index = 1;
  }
}


/**************************************************************************
...
**************************************************************************/
function city_add_to_worklist()
{
  if (selected_kind != -1 && selected_value != -1) {
    send_city_worklist_add(active_city['id'], selected_kind, selected_value);
    city_tab_index = 1;
  }

}

/**************************************************************************
 Updates the Cities tab when clicked, populating the table.
**************************************************************************/
function update_city_screen()
{
  if (observing) return;

  var city_list_html = "<table class='tablesorter' id='city_table' border=0 cellspacing=0>"
        + "<thead><td>Name</td><td>Population</td><td>Size</td><td>State</td>"
        + "<td>Granary</td><td>Grows In</td><td>Producing</td>"  
        + "<td>Surplus<br>Food/Prod/Trade</td><td>Economy<br>Gold/Luxury/Science</td></thead>";
  var count = 0;
  for (var city_id in cities){
    var pcity = cities[city_id];
    if (city_owner(pcity).playerno == client.conn.playing.playerno) {
      count++; 
      var prod_type = get_city_production_type(pcity);
      var turns_to_complete_str;
      if (get_city_production_time(pcity) == FC_INFINITY) {
        turns_to_complete_str = "-"; //client does not know how long production will take yet.
      } else {
        turns_to_complete_str = get_city_production_time(pcity) + " turns";
      }

      city_list_html += "<tr id='cities_row' onclick='javascript:show_city_dialog_by_id(" + pcity['id'] + ");'><td>" 
              + pcity['name'] + "</td><td>" + numberWithCommas(city_population(pcity)*1000) +
              "</td><td>" + pcity['size'] + "</td><td>" + get_city_state(pcity) + "</td><td>" + pcity['food_stock'] + "/" + pcity['granary_size'] +
              "</td><td>" + city_turns_to_growth_text(pcity) + "</td>" + 
              "<td>" + prod_type['name'] + " (" + turns_to_complete_str + ")" +
              "</td><td>" + pcity['surplus'][O_FOOD] + "/" + pcity['surplus'][O_SHIELD] + "/" + pcity['surplus'][O_TRADE] + "</td>" +
              "<td>" + pcity['prod'][O_GOLD] + "/" + pcity['prod'][O_LUXURY] + "/" + pcity['prod'][O_SCIENCE] + "<td>"; 

      city_list_html += "</tr>";
    }

  }


  city_list_html += "</table>";
  $("#cities_list").html(city_list_html);

  if (count == 0) {
    $("#city_table").html("You have no cities. Build new cities with the Settlers unit.");
  }

  $('#cities_scroll').css("height", $(window).height() - 200);

  $("#city_table").tablesorter({theme:"dark"});
}

/**************************************************************************
 Returns the city state: Celebrating, Disorder og Peace.
**************************************************************************/
function get_city_state(pcity) 
{
  if (pcity == null) return;

  if (pcity['was_happy'] && pcity['size'] >= 3) {
    return "Celebrating";
  } else if (pcity['uhappy']) {
    return "Disorder";
  } else {
    return "Peace";
  }
}
