/********************************************************************** 
 Freeciv-web - Copyright (C) 2014 - Andreas Rosdal   andrearo@pvv.ntnu.no
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

var toplevel_menu_items = ["help_terrain", "help_economy", "help_cities", 
    "help_city_improvements", "help_wonders_of_the_world", "help_units", 
    "help_combat", "help_technology"];
var hidden_menu_items = ["help_connecting", "help_languages", "help_governor", 
    "help_chatline", "help_about", "help_worklist_editor"];

/**************************************************************************
 Show the Freeciv-web Help Dialog
**************************************************************************/
function show_help()
{
  $("#tabs-hel").show();
  $("#help_menu").remove();
  $("#help_info_page").remove();
  $("<ul id='help_menu'></ul><div id='help_info_page'></div>").appendTo("#tabs-hel");
  for (sec_id in helpdata_order) {
    var key = helpdata_order[sec_id];
    var sec_name = helpdata[key]['name'];
    if (hidden_menu_items.indexOf(key) > -1) {
      continue;
    } else if (key.indexOf("help_gen") != -1) {
      generate_help_menu(key);
    } else if (toplevel_menu_items.indexOf(key) > -1) {
      generate_help_toplevel(key);
    } else {
      var parent_key = find_parent_help_key(key);
      $("<li id='" + key +  "' data-helptag='" + key +  "'>" 
        + helpdata_tag_to_title(key) + "</li>").appendTo(parent_key);
    }
  }

  $("#help_menu").menu({
    select: function( event, ui ) {handle_help_menu_select(ui); }
  });
 
  show_help_intro(); 
  $("#tabs-hel").css("height", $(window).height() - 60);
  $("#help_info_page").css("max-width", $(window).width() - $("#help_menu").width() - 60);

}

/**************************************************************************
...
**************************************************************************/
function show_help_intro()
{
  $.get( "/docs/help_intro.txt", function( data ) {
      $("#help_info_page").html(data);
  });
}

/**************************************************************************
...
**************************************************************************/
function generate_help_menu(key)
{
  if (key == "help_gen_terrain") {
    for (var terrain_id in terrains) {
      var terrain = terrains[terrain_id];
      $("<li data-helptag='" + key + "_" + terrain['id'] + "'>" 
        + terrain['name'] + "</li>").appendTo("#help_terrain_ul");
    }
  } else if (key == "help_gen_improvements") {
    for (var impr_id in improvements) {
      var improvement = improvements[impr_id];
      if (is_wonder(improvement)) continue;
      $("<li data-helptag='" + key + "_" + improvement['id'] + "'>" 
        + improvement['name'] + "</li>").appendTo("#help_city_improvements_ul");
    }
  } else if (key == "help_gen_wonders") {
    for (var impr_id in improvements) {
      var improvement = improvements[impr_id];
      if (!is_wonder(improvement)) continue;
      $("<li data-helptag='" + key + "_" + improvement['id'] + "'>" 
        + improvement['name'] + "</li>").appendTo("#help_wonders_of_the_world_ul");
    }
  } else if (key == "help_gen_units") {
    for (var i = 0; i < unittype_ids_alphabetic().length; i++) {
      var unit_id = unittype_ids_alphabetic()[i];
      var punit_type = unit_types[unit_id];

       $("<li data-helptag='" + key + "_" + punit_type['id'] + "'>" 
          + punit_type['name'] + "</li>").appendTo("#help_units_ul");
    }
  } else if (key == "help_gen_techs") {
    for (var tech_id in techs) {
      var tech = techs[tech_id];
      if (tech_id == 0) continue;
      $("<li data-helptag='" + key + "_" + tech['id'] + "'>" 
          + tech['name'] + "</li>").appendTo("#help_technology_ul");
    }
  }

}

/**************************************************************************
...
**************************************************************************/
function render_sprite(sprite)
{
  var msg = "<div class='help_unit_image' style=' background: transparent url(" 
           + sprite['image-src'] +
           ");background-position:-" + sprite['tileset-x'] + "px -" + sprite['tileset-y'] 
           + "px;  width: " + sprite['width'] + "px;height: " + sprite['height'] + "px;'"
           +"></div>"
  return msg;
}

/**************************************************************************
...
**************************************************************************/
function generate_help_toplevel(key)
{
  var parent_key = find_parent_help_key(key);
  $("<li id='" + key +  "' data-helptag='" + key +  "'>" 
     + helpdata_tag_to_title(key) + "</li>").appendTo(parent_key);
  $("<ul id='" + key + "_ul' class='help_submenu'></ul>").appendTo("#" + key);

}

/**************************************************************************
...
**************************************************************************/
function find_parent_help_key(key)
{
  if (key == "help_terrain_alterations") {
    return "#help_terrain_ul";
  } else if (key == "help_villages" || key == "help_borders") {  
    return "#help_terrain_ul";
  } else if (key == "help_food" || key == "help_production" || key == "help_trade") {
    return "#help_economy_ul";
  } else if (key == "help_specialists" || key == "help_happiness" || key == "help_pollution" 
        || key == "help_plague" || key == "help_migration") {
    return "#help_cities_ul";    
  } else if (key == "help_combat_example_1" || key == "help_combat_example_2") {
    return "#help_combat_ul";
  } else  {
    return "#help_menu";
  }
}
/**************************************************************************
...
**************************************************************************/
function handle_help_menu_select( ui ) 
{
  var selected_tag = $(ui.item).data("helptag");
  if (selected_tag.indexOf("help_gen") != -1) {
    generate_help_text(selected_tag);
  } else if (selected_tag == "help_copying") {
    $.get( "/docs/LICENSE.txt", function( data ) {
      $("#help_info_page").html("<h1>Freeciv-Web License</h1>" + data.replace(/\n/g, "<br>"));
    });
  } else if (selected_tag == "help_controls") {
    $.get( "/docs/controls.txt", function( data ) {
      $("#help_info_page").html(data.replace(/\n/g, "<br>"));
    });    
  } else {
    var msg = "<h1>" + helpdata_tag_to_title(selected_tag) + "</h1>" + helpdata[selected_tag]['text'];
    $("#help_info_page").html(msg);
  }

  $("#help_info_page").focus();
}


/**************************************************************************
...
**************************************************************************/
function generate_help_text(key)
{
  var msg = "";
  if (key.indexOf("help_gen_terrain") != -1) {
    var terrain = terrains[parseInt(key.replace("help_gen_terrain_", ""))];
    msg = "<h1>" + terrain['name'] + "</h1>" + terrain['helptext'] 
	    + "<br><br>Movement cost: " + terrain['movement_cost']
	    + "<br>Defense bonus: " + terrain['defense_bonus']
	    + "<br>Food/Prod/Trade: " + terrain['output'][0] + "/" 
	    + terrain['output'][1] + "/" + terrain['output'][2];
  } else if (key.indexOf("help_gen_improvements") != -1 || key.indexOf("help_gen_wonders") != -1) {
    var improvement = improvements[parseInt(key.replace("help_gen_wonders_", "").replace("help_gen_improvements_", ""))];
    msg = "<h1>" + improvement['name'] + "</h1>" 
	    + render_sprite(get_improvement_image_sprite(improvement)) + "<br>"
	    + improvement['helptext'] 
            + "<br><br>Cost: " + improvement['build_cost']
            + "<br>Upkeep: " + improvement['upkeep'];
    var req = get_improvement_requirements(improvement['id']);
    if (req != null) {
      msg += "<br>Requirements: " + techs[req]['name'];
    }
    msg += "<br><br><button class='help_button' onclick=\"show_wikipedia_dialog('" + improvement['name'] + "');\">Wikipedia on " 
	    + improvement['name'] +  "</button>";
  } else if (key.indexOf("help_gen_units") != -1) {
    var punit_type = unit_types[parseInt(key.replace("help_gen_units_", ""))];
    msg = "<h1>" + punit_type['name'] + "</h1>" 
	    + render_sprite(get_unit_type_image_sprite(punit_type)) + "<br>"
	    + punit_type['helptext'] 
            + "<br><br>Cost: " + punit_type['build_cost'];
            /*+ "<br>Upkeep: " + improvement['upkeep'];*/
    msg += "<br>Attack: " + punit_type['attack_strength'];
    msg += "<br>Defense: " + punit_type['defense_strength'];
    msg += "<br>Firepower: " + punit_type['firepower'];
    msg += "<br>Hitpoints: " + punit_type['hp'];
    msg += "<br>Vision: " + punit_type['vision_radius_sq'];

    var ireq = get_improvement_requirements(punit_type['impr_requirement']);
    if (ireq != null && techs[ireq] != null) {
      msg += "<br>Building Requirements: " + techs[ireq]['name'];
    }
    var treq = punit_type['tech_requirement'];
    if (treq != null && techs[treq] != null) {
      msg += "<br>Tech Requirements: " + techs[treq]['name'];
    }
    msg += "<br><br><button class='help_button' onclick=\"show_wikipedia_dialog('" + punit_type['name'] + "');\">Wikipedia on " 
	    + punit_type['name'] +  "</button>";
  } else if (key.indexOf("help_gen_techs") != -1) {
    var tech = techs[parseInt(key.replace("help_gen_techs_", ""))];
    msg = "<h1>" + tech['name'] + "</h1>" 
	    + render_sprite(get_technology_image_sprite(tech)) + "<br>"
	    + get_advances_text(tech['id']);
    msg += "<br><br><button class='help_button' onclick=\"show_wikipedia_dialog('" + tech['name'] + "');\">Wikipedia on " 
	    + tech['name'] +  "</button>";
  }

  $("#help_info_page").html(msg);
  $(".help_button").button();
}

/**************************************************************************
...
**************************************************************************/
function helpdata_tag_to_title(tag) 
{
  var result = tag.replace("_of_the_world", "").replace("help_", "").replace("gen_", "").replace("misc_", "").replace(/_/g, " ");
  return to_title_case(result);

}
