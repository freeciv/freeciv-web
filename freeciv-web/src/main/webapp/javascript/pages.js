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



var PAGE_MAIN = 0;		/* Main menu, aka intro page.  */
var PAGE_START = 1;		/* Start new game page.  */
var PAGE_SCENARIO = 2;		/* Start new scenario page. */
var PAGE_LOAD = 3;		/* Load saved game page. */
var PAGE_NETWORK = 4;		/* Connect to network page.  */
var PAGE_NATION = 5;		/* Select a nation page.  */
var PAGE_GAME = 6;		/* In game page. */
var PAGE_GGZ = 7;		/* In game page.  This one must be last. */

var old_page = -1;

function set_client_page(page)
{

  var new_page = page;
  

  /* If the page remains the same, don't do anything. */
  if (old_page == new_page) {
    return;
  }

  switch (old_page) {
  case -1:
    $("#pregame_page").remove(); 
    break;
  case PAGE_SCENARIO:
    break;
  case PAGE_LOAD:
    break;
  case PAGE_NETWORK:
    /*destroy_server_scans();*/
    break;
  case PAGE_GAME:
    /*enable_menus(FALSE);*/
    break;
  default:
    break;
  }

  switch (new_page) {
  case PAGE_MAIN:
    break;
  case PAGE_GGZ:
    break;
  case PAGE_START:
    /*if (is_server_running()) {
      gtk_widget_show(start_options_table);
      update_start_page();
    } else {
      gtk_widget_hide(start_options_table);
    }*/
    break;
  case PAGE_NATION:
    break;
  case PAGE_GAME:
    $("#game_page").show(); 
    /*reset_unit_table();
    enable_menus(TRUE);*/
    break;
  case PAGE_LOAD:
    /*update_load_page();*/
    break;
  case PAGE_SCENARIO:
    /*update_scenario_page();*/
    break;
  case PAGE_NETWORK:
    /*update_network_page();*/
    break;
  }



}

function get_client_page()
{
  return old_page;
}

