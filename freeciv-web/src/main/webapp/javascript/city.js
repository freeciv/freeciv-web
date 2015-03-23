/********************************************************************** 
 Freeciv - Copyright (C) 2009 - Andreas Røsdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/


var cities = {};
var city_rules = {};

var active_city = null;
var worklist_dialog_active = false;
var selected_value = -1;
var selected_kind = -1;

var FEELING_BASE = 0;		/* before any of the modifiers below */
var FEELING_LUXURY = 1;		/* after luxury */
var FEELING_EFFECT = 2;		/* after building effects */
var FEELING_NATIONALITY = 3;  	/* after citizen nationality effects */
var FEELING_MARTIAL = 4;	/* after units enforce martial order */
var FEELING_FINAL = 5;		/* after wonders (final result) */

var MAX_LEN_WORKLIST = 64;

var INCITE_IMPOSSIBLE_COST = (1000 * 1000 * 1000);

var city_tab_index = 0;

/**************************************************************************
 ...
**************************************************************************/
function city_tile(pcity)
{
  if (pcity == null) return null;
  
  return index_to_tile(pcity['tile'])
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
 
 delete cities[pcity_id];
 
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

  city_worklist_dialog(pcity);
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

  $("#city_dialog").attr("title", decodeURIComponent(pcity['name']));
  $("#city_dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "98%" : "80%",
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

  set_city_mapview_active();
  center_tile_mapcanvas(city_tile(pcity));
  update_map_canvas(0, 0, mapview['store_width'], mapview['store_height']);

  $("#city_size").html("Population: " + numberWithCommas(city_population(pcity)*1000) + "<br>" 
                       + "Size: " + pcity['size'] + "<br>"
                       + "Granary: " + pcity['food_stock'] + "/" + pcity['granary_size'] + "<br>"
                       + "Change in: " + city_turns_to_growth_text(pcity));
  
  if (pcity['production_kind'] == VUT_UTYPE) {
    var punit_type = unit_types[pcity['production_value']];
    $("#city_production_overview").html("Producing: " + punit_type['name']);   
    turns_to_complete = city_turns_to_build(pcity, punit_type, true); 
  }

  if (pcity['production_kind'] == VUT_IMPROVEMENT) {
    var improvement = improvements[pcity['production_value']];
    $("#city_production_overview").html("Producing: " + improvement['name']);
    turns_to_complete = city_turns_to_build(pcity, improvement, true);
    if (improvement['name'] == "Coinage") {
      turns_to_complete = FC_INFINITY;
    }
  }
  if (turns_to_complete != FC_INFINITY) {
    $("#city_production_turns_overview").html("Turns to completion: " + turns_to_complete);
  } else {
    $("#city_production_turns_overview").html("-");
  }
  
  var improvements_html = "";
  for (var z = 0; z < pcity['improvements'].length; z ++) {
    if (pcity['improvements'][z] == 1) {
       var sprite = get_improvement_image_sprite(improvements[z]);
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
      var punit = punits[r];
      var ptype = unit_type(punit);   
      var sprite = get_unit_image_sprite(punit);
      if (sprite == null) {
         console.log("Missing sprite for " + punit);
         continue;
       }
      
      present_units_html = present_units_html +
       "<div id='game_unit_list_item' style='cursor:pointer;cursor:hand; background: transparent url(" 
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
    for (var r = 0; r < sunits.length; r++) {
      var punit = sunits[r];
      var ptype = unit_type(punit);   
      var sprite = get_unit_image_sprite(punit);
      if (sprite == null) {
         console.log("Missing sprite for " + punit);
         continue;
       }
      
      supported_units_html = supported_units_html +
       "<div id='game_unit_list_item' style='cursor:pointer;cursor:hand; background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           + " onclick='city_dialog_activate_unit(units[" + punit['id'] + "]);'"
           +"></div>";
    }
    $("#city_supported_units_list").html(supported_units_html);
  }


  $("#city_food").html(pcity['prod'][0]);
  $("#city_prod").html(pcity['prod'][1]);
  $("#city_trade").html(pcity['prod'][2]);
  $("#city_gold").html(pcity['prod'][3]);
  $("#city_luxury").html(pcity['prod'][4]);
  $("#city_science").html(pcity['prod'][5]);

  $("#city_corruption").html(pcity['waste'][O_TRADE]);
  $("#city_waste").html(pcity['waste'][O_SHIELD]);
  $("#city_pollution").html(pcity['pollution']);

  /* Handle citizens and specialists */
  var specialist_html = "";
  var citizen_types = ["angry", "unhappy", "content", "happy"];
  for (var s = 0; s < citizen_types.length; s++) {
    for (var i = 0; i < pcity['ppl_' + citizen_types[s]][FEELING_FINAL]; i ++) {
     var sprite = get_specialist_image_sprite("citizen." + citizen_types[s] + "_" 
         + (i % 2));
     specialist_html = specialist_html +
     "<div class='specialist_item' style='background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           +" title='One " + citizen_types[s] + " citizen'></div>";
    }
  }

  for (var i = 0; i < pcity['specialists_size']; i++) {
    var spec_type_name = specialists[i]['plural_name'];
    var spec_gfx_key = "specialist." + specialists[i]['rule_name'] + "_0";
    for (var s = 0; s < pcity['specialists'][i]; s++) {
     var sprite = get_specialist_image_sprite(spec_gfx_key);
     specialist_html = specialist_html +
     "<div class='specialist_item' style='cursor:pointer;cursor:hand; background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           + " onclick='city_change_specialist(" + pcity['id'] + "," + specialists[i]['id'] + ");'"
           +" title='" + spec_type_name + " (click to change)'></div>";

    }
  }
  specialist_html += "<div style='clear: both;'></div>";
  $("#specialist_panel").html(specialist_html);

  $('#disbandable_city').off();
  $('#disbandable_city').prop('checked', pcity['disbandable_city']);
  $('#disbandable_city').click(function() {
    var packet = {"pid" : packet_city_disbandable_req, "city_id" : active_city['id']};
    send_request(JSON.stringify(packet));
    city_tab_index = 3;
  });

  city_tab_index = 0;

  if (is_small_screen()) {
   $(".ui-tabs-anchor").css("padding", "2px");
  }
}

/**************************************************************************
...
**************************************************************************/
function change_city_production_dialog()
{

  var pcity = active_city;
  var turns_to_complete = FC_INFINITY;

  // reset dialog page.
  $("#dialog").remove();
  $("<div id='dialog'></div>").appendTo("div#game_page");

  var dhtml = " <div id='city_production_change'></div>" +
	"  <div id='city_production_turns_change'></div>" +
	"  <div id='city_change_production'>" +
	"  <div id='city_production_choices'>" +
	"  </div>" +
	"  </div>"



  $("#dialog").html(dhtml);

  var production_list = generate_production_list(pcity);

  var production_html = "";
  for (var a = 0; a < production_list.length; a++) {
    var current_prod = (pcity['production_kind'] == production_list[a]['kind'] 
	&& pcity['production_value'] == production_list[a]['value']);
    var sprite = production_list[a]['sprite'];  
    if (sprite == null) {
      console.log("Missing sprite for " + production_list[a]['value']);
      continue;
    }

    production_html = production_html 
     + "<div class='production_list_item' style='" + (current_prod ? "background-color:#777777; text:#000000; border: 1px solid #ffffff;" : "") + "'" 
     + " onclick='send_city_change(" + pcity['id'] + "," + production_list[a]['kind'] + "," + production_list[a]['value'] + ")' "
     + " title=\"" + production_list[a]['helptext'] + "\">"
     + "<div class='production_list_item_sub' style=' background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;'"
           +"></div>"
     + production_list[a]['text'] + "</div>";
  }
  $("#city_production_choices").html(production_html);


  if (pcity['production_kind'] == VUT_UTYPE) {
    var punit_type = unit_types[pcity['production_value']];
    $("#city_production").html("Producing: " + punit_type['name']);   
    turns_to_complete = city_turns_to_build(pcity, punit_type, true); 
  }

  if (pcity['production_kind'] == VUT_IMPROVEMENT) {
    var improvement = improvements[pcity['production_value']];
    $("#city_production").html("Producing: " + improvement['name']);
    turns_to_complete = city_turns_to_build(pcity, improvement, true);
    if (improvement['name'] == "Coinage") {
      turns_to_complete = FC_INFINITY;
    }
  }
  if (turns_to_complete != FC_INFINITY) {
    $("#city_production_turns_change").html("Turns to completion: " + turns_to_complete);
  } else {
    $("#city_production_turns_change").html("-");
  }

  $("#dialog").attr("title", "Change city production");
  $("#dialog").dialog({
			bgiframe: true,
			modal: true,
			width: is_small_screen() ? "95%" : "60%",
			buttons: {
				"Buy": function() {
						request_city_buy();
						$("#dialog").dialog('close');
				},
				"Close": function() {
						$("#dialog").dialog('close');

				}
			}
		});

  $("#dialog").dialog('open');		
  
  if (!city_can_buy(pcity)) $(".ui-dialog-buttonpane button:contains('Buy')").button("disable");	

}

/**************************************************************************
...
**************************************************************************/
function generate_production_list(pcity)
{

  var production_list = [];
  for (unit_type_id in unit_types) {
    var punit_type = unit_types[unit_type_id];
    
    /* FIXME: web client doesn't support unit flags yet, so this is a hack: */
    if (punit_type['name'] == "Barbarian Leader" || punit_type['name'] == "Leader") continue;
    
    if (can_city_build_unit_now(pcity, punit_type) == true) { 
      production_list.push({"kind": VUT_UTYPE, "value" : punit_type['id'], 
                            "text" : punit_type['name'],
	                    "helptext" : punit_type['helptext'],
                            "rule_name" : punit_type['rule_name'],
                            "build_cost" : punit_type['build_cost'],
                            "sprite" : get_unit_type_image_sprite(punit_type)});
    }
  }

  for (improvement_id in improvements) {
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
  return (pcity['can_build_improvement'][pimprove['id']] == "1"); 
}


/**************************************************************************
  Return TRUE iff the city has this building in it.
**************************************************************************/
function city_has_building(pcity,
		                   pimprove)
{
  /* TODO: implement. */
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
    buy_price_string = punit_type['name'] + " costs " + pcity['buy_gold_cost'] + " gold.";
    buy_question_string = "Buy " + punit_type['name'] + " for " + pcity['buy_gold_cost'] + " gold?";
  } else {
    var improvement = improvements[pcity['production_value']];
    buy_price_string = improvement['name'] + " costs " + pcity['buy_gold_cost'] + " gold.";
    buy_question_string = "Buy " + improvement['name'] + " for " + pcity['buy_gold_cost'] + " gold?";
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

  for (var z = 0; z < pcity['improvements'].length; z++) {
    if (pcity['improvements'][z] == 1 && improvements[z] != null
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
		  	      + "<input id='city_name_req' type='text' value='" 
			      + suggested_name + "'>"));
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
						  swal("City name is invalid");
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
  keyboard_input=false;
}

/**************************************************************************
.. 
**************************************************************************/
function next_city()
{
  if (!client.conn.playing) return;

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
    var pcity = cities[city_id];
    if (is_next && city_owner(pcity).playerno == client.conn.playing.playerno) {
      show_city_dialog(pcity)
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
  for (city_id in cities) {
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
		  	      + "<input id='city_name_req' type='text' value='" 
			      + active_city['name'] + "'>"));
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
  if (active_city == null || active_city['trade'] == null) return;

  var msg = "";
  for (var i = 0; i < active_city['trade'].length; i++) {
    var tcity_id = active_city['trade'][i];
    if (tcity_id == 0) continue;
    var tcity = cities[tcity_id];
    msg += tcity['name'] + " (" + active_city['trade_value'][i] + ")" + "<br>";
  }
  if (msg == "") msg = "No traderoutes.";
  $("#city_traderoutes_tab").html(msg);

}

/**************************************************************************
 Populates data to the production tab in the city dialog.
**************************************************************************/
function city_worklist_dialog(pcity) 
{
  if (pcity == null) return;

  var universals_list = [];
  if (pcity['worklist'] != null && pcity['worklist'] != "-") {
    var work_list = pcity['worklist'].split(";");
    for (var i = 0; i < work_list.length; i++) {
      var work_item = work_list[i].split(",");
      var kind = work_item[0];
      var value = work_item[1];
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
    var sprite = universal['sprite'];
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
  var production_html = "<table class='worklist_table'><tr><td>Type</td><td>Name</td><td>Cost</td></tr>";
  for (var a = 0; a < production_list.length; a++) {
    var sprite = production_list[a]['sprite'];  
    if (sprite == null) {
      console.log("Missing sprite for " + production_list[a]['value']);
      continue;
    }
    var kind = production_list[a]['kind'];
    var value = production_list[a]['value'];

    production_html += "<tr class='prod_choice_list_item kindvalue_item' data-value='" + value + "' data-kind='" + kind + "'>"
     + "<td><div class='production_list_item_sub' title=\"" + production_list[a]['helptext'] + "\" style=' background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;'"
           +"></div></td>"
     + "<td class='prod_choice_name'>" + production_list[a]['text'] + "</td>"
     + "<td class='prod_choice_cost'>" + production_list[a]['build_cost'] + "</td></tr>";
  }
  production_html += "</table>";

  $("#worklist_production_choices").html(production_html);

  keyboard_input=false;
  worklist_dialog_active = true;
  var headline = "";
  var turns_to_complete = "";
  if (pcity['production_kind'] == VUT_UTYPE) {
    var punit_type = unit_types[pcity['production_value']];
    headline += "Producing: " + punit_type['name'];   
    turns_to_complete = city_turns_to_build(pcity, punit_type, true); 
  }

  if (pcity['production_kind'] == VUT_IMPROVEMENT) {
    var improvement = improvements[pcity['production_value']];
    headline += "Producing: " + improvement['name'];
    turns_to_complete = city_turns_to_build(pcity, improvement, true);
    if (improvement['name'] == "Coinage") {
      turns_to_complete = FC_INFINITY;
    }
  }
  if (turns_to_complete != FC_INFINITY) {
    headline += " - Turns to completion: " + turns_to_complete;
  }

  $("#worklist_dialog_headline").html(headline);

  $(".production_list_item_sub").tooltip();
 
  $(".worklist_table").selectable({ filter: "tr", 
       selected: function( event, ui ) {handle_worklist_select(ui); }}); 

  $(".kindvalue_item").dblclick(function() {
    var value = parseFloat(jQuery(this).data('value'));
    var kind = parseFloat(jQuery(this).data('kind'));
    if (selected_kind >= 0 && selected_value >= 0) {
      send_city_worklist_add(pcity['id'], kind, value);
    }
  });
  $(".button").button();

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
  if (pcity['worklist'].split(";").length >= MAX_LEN_WORKLIST) {
    return;
  } 

  var city_message = {"pid": packet_city_worklist_add, 
                      "city_id" : city_id,
                      "kind" : kind,
                      "value" : value};
  send_request(JSON.stringify(city_message));
  city_tab_index = 1;
}

/**************************************************************************
...
**************************************************************************/
function send_city_worklist_remove(city_id, index) 
{
  var city_message = {"pid": packet_city_worklist_remove, 
                      "city_id" : city_id,
                      "index" : index};
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
