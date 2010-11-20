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

var I_NEVER	 = (-1);    /* Improvement never built */
var I_DESTROYED = (-2); /* Improvement built and destroyed */



/**************************************************************************
 ...
**************************************************************************/
function city_tile(pcity)
{
  if (pcity == null) return null;
  
  if (pcity['tile'] != null) {
    return pcity['tile'];
  } else {
    return map_pos_to_tile(pcity['x'], pcity['y'])
  }
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
 
 var pcity = cities[city_id];
 if (pcity != null && cities.indexOf(pcity) >= 0) {
   cities.splice(cities.indexOf(pcity), 1);
 }
 
}

/**************************************************************************
 ...
**************************************************************************/
function is_city_center(city, tile) {
  return (city['x'] == tile['x'] && city['y'] == tile['y']);
} 

/**************************************************************************
 ...
**************************************************************************/
function is_free_worked(city, tile) {
  return (city['tile'] == tile);
}


/**************************************************************************
 ...
**************************************************************************/
function show_city_dialog_by_id(pcity_id)
{
  show_city_dialog(cities[pcity_id]);
}

/**************************************************************************
 ...
**************************************************************************/
function show_city_dialog(pcity)
{
  var turns_to_complete = FC_INFINITY;
  active_city = pcity;
  
  if (pcity == null) return;
  
  set_city_mapview_active();
  center_tile_mapcanvas(city_tile(pcity));
  
  $("#tabs-map").addClass("ui-tabs-hide");
  $("#tabs-cit").removeClass("ui-tabs-hide");
  $("#map_tab").removeClass("ui-state-active");
  $("#map_tab").removeClass("ui-tabs-selected");
  $("#map_tab").addClass("ui-state-default");
  
  
  $("#city_heading").html(pcity['name']);
  $("#city_size").html("City size: " + pcity['size']);
  
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
    $("#city_production_turns").html("Turns to completion: " + turns_to_complete);
  } else {
    $("#city_production_turns").html("-");
  }

  
  var improvements_html = "";
  for (var z = 0; z < pcity['improvements'].length; z ++) {
    if (pcity['improvements'][z] == 1) {
       var sprite = get_improvement_image_sprite(improvements[z]);
      
      improvements_html = improvements_html +
       "<div style='background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           + ">"
           +"</div>";
    }
  }
  $("#city_improvements_list").html(improvements_html);
  
  var production_list = [];
  for (unit_type_id in unit_types) {
    var punit_type = unit_types[unit_type_id];
    
    /* FIXME: web client doesn't support unit flags yet, so this is a hack: */
    if (punit_type['name'] == "Barbarian Leader" || punit_type['name'] == "Leader") continue;
    
    if (can_city_build_unit_now(pcity, punit_type) == true) { 
      production_list.push({"kind": VUT_UTYPE, "value" : punit_type['id'], 
                            "text" : punit_type['name'],
                            "sprite" : get_unit_type_image_sprite(punit_type)});
    }
  }
  
  for (improvement_id in improvements) {
    var pimprovement = improvements[improvement_id];
    if (can_city_build_improvement_now(pcity, pimprovement)) {
      production_list.push({"kind": VUT_IMPROVEMENT, 
                            "value" : pimprovement['id'], 
                            "text" : pimprovement['name'],
                            "sprite" : get_improvement_image_sprite(pimprovement) });
    }
  }
  
  var production_html = "";
  for (var a = 0; a < production_list.length; a++) {
    var current_prod = (pcity['production_kind'] == production_list[a]['kind'] && pcity['production_value'] == production_list[a]['value']);
    var sprite = production_list[a]['sprite'];  
    production_html = production_html 
     + "<div style='text-align: center; " + (current_prod ? "background-color:#777777; text:#000000; border: 1px solid #ffffff;" : "") + "'" 
     + " onclick='send_city_change(" + pcity['id'] + "," + production_list[a]['kind'] + "," + production_list[a]['value'] + ")'>"
     
     + "<div id='production_list_item' style='cursor:pointer;cursor:hand; background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;float:left; '"
           +"></div>"
      
     + production_list[a]['text'] + "</div>";
  }
  
  $("#city_production_choices").html(production_html);
  
  var punits = tile_units(city_tile(pcity));
  if (punits != null) {
    var present_units_html = "";
    for (var r = 0; r < punits.length; r++) {
      var punit = punits[r];
      var ptype = unit_type(punit);   
      var sprite = get_unit_image_sprite(punit);
      
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

  $("#city_food").html(pcity['prod'][0]);
  $("#city_prod").html(pcity['prod'][1]);
  $("#city_trade").html(pcity['prod'][2]);
  $("#city_gold").html(pcity['prod'][3]);
  $("#city_luxury").html(pcity['prod'][4]);
  $("#city_science").html(pcity['prod'][5]);

}


/**************************************************************************
  Return whether given city can build given unit, ignoring whether unit 
  is obsolete.
**************************************************************************/
function can_city_build_unit_direct(pcity, punittype)
{
  if (!can_player_build_unit_direct(city_owner(pcity), punittype)) {
    return false;
  }

  /* Check to see if the unit has a building requirement. */
  /*if (punittype->need_improvement
   && !city_has_building(pcity, punittype->need_improvement)) {
    return FALSE;
  }*/

  /* You can't build naval units inland. */
  /*if (!uclass_has_flag(utype_class(punittype), UCF_BUILD_ANYWHERE)
      && !is_native_near_tile(punittype, pcity->tile)) {
    return false;
  }*/
  return true;
}


/**************************************************************************
  Return whether given city can build given unit; returns FALSE if unit is 
  obsolete.
**************************************************************************/
function can_city_build_unit_now(pcity, punittype)
{  
  if (!can_city_build_unit_direct(pcity, punittype)) {
    return false;
  }
  while ((punittype = punittype['obsoleted_by']) != U_NOT_OBSOLETED) {
    if (can_player_build_unit_direct(city_owner(pcity), punittype)) {
	return false;
    }
  }
  return true;
}

/**************************************************************************
  Return whether given city can build given building, ignoring whether
  it is obsolete.
**************************************************************************/
function can_city_build_improvement_direct(pcity, pimprove)
{
  if (!can_player_build_improvement_direct(city_owner(pcity), pimprove)) {
    return false;
  }

  if (city_has_building(pcity, pimprove)) {
    return false;
  }

  /* Does city already have this? */
  for (var z = 0; z < pcity['improvements'].length; z ++) {
    if (pcity['improvements'][z] == 1) {
      if (improvements[z]['name'] == pimprove['name']) {
        return false;
      }
    }
  }

  /*return are_reqs_active(city_owner(pcity), pcity, null,
			 city_tile(pcity), null, null, null,
			 pimprove['reqs'], RPT_CERTAIN);*/
			 
  return true;			 
}


/**************************************************************************
  Return whether given city can build given building; returns FALSE if
  the building is obsolete.
**************************************************************************/
function can_city_build_improvement_now(pcity, pimprove)
{  
  if (!can_city_build_improvement_direct(pcity, pimprove)) {
    return false;
  }
  /*if (improvement_obsolete(city_owner(pcity), pimprove)) {
    return false;
  }*/
  return true;
}


/**************************************************************************
  Return TRUE iff the city has this building in it.
**************************************************************************/
function city_has_building(pcity,
		                   pimprove)
{
  if (pimprove = null) {
    return false;
  }
  /* FIXME: check if city has building.  */
  return false;
  //return (pcity->built[improvement_index(pimprove)].turn > I_NEVER);
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
 Buy whatever is being built in the city.
**************************************************************************/
function send_city_buy()
{
  if (active_city != null) {
    var packet = [{"packet_type" : "city_buy", "city_id" : active_city['id']}];
    send_request (JSON.stringify(packet));
  }
}
/**************************************************************************
 Change city production.
**************************************************************************/
function send_city_change(city_id, kind, value)
{
  var packet = [{"packet_type" : "city_change", "city_id" : city_id, 
                "production_kind": kind, "production_value" : value}];
  send_request (JSON.stringify(packet));

  
  //setTimeout ( "show_city_dialog(cities["+city_id+"]);", 1000);
  
}

/**************************************************************************
 Close dialog.
**************************************************************************/
function close_city_dialog()
{
  $("#tabs-map").removeClass("ui-tabs-hide");
  $("#tabs-cit").addClass("ui-tabs-hide");
  $("#map_tab").addClass("ui-state-active");
  $("#map_tab").addClass("ui-tabs-selected");
  $("#map_tab").removeClass("ui-state-default");
  
  set_default_mapview_active();
}


/**************************************************************************
 The city map has been clicked.
**************************************************************************/
function do_city_map_click(ptile)
{
  var packet = null;
  if (ptile['worked'] == active_city['id']) {
    packet = [{"packet_type" : "city_make_specialist", 
	         "city_id" : active_city['id'], 
                 "worker_x" : ptile['x'] - active_city['x'] + 2, 
		 "worker_y" : ptile['y'] - active_city['y'] + 2}];
  } else {
    packet = [{"packet_type" : "city_make_worker", 
	         "city_id" : active_city['id'], 
                 "worker_x" : ptile['x'] - active_city['x'] + 2, 
		 "worker_y" : ptile['y'] - active_city['y'] + 2}];
  }    
  send_request (JSON.stringify(packet));

}

/**************************************************************************
.. 
**************************************************************************/
function does_city_have_improvement(pcity, improvement_name)
{
  if (pcity == null || pcity['improvements'] == null) return false;

  for (var z = 0; z < pcity['improvements'].length; z ++) {
    if (pcity['improvements'][z] == 1) {
      if (improvements[z]['name'] == improvement_name) {
        return true;
      }
    }
  }
  return false;
}
