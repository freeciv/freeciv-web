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


var C_S_INITIAL = 0;    /* Client boot, only used once on program start. */
var C_S_PREPARING = 1;  /* Main menu (disconnected) and connected in pregame. */
var C_S_RUNNING = 2;    /* Connected with game in progress. */
var C_S_OVER = 3;       /* Connected with game over. */

var civclient_state = C_S_INITIAL;

function set_client_state(newstate)
{
  var connect_error = (C_S_PREPARING == civclient_state)
      && (C_S_PREPARING == newstate);
  var oldstate = civclient_state;

  if (civclient_state != newstate) {
    /*FIXME: struct player *pplayer = client_player();*/

    /* If changing from pre-game state to _either_ select race
       or running state, then we have finished getting ruleset data.
    */
    /*FIXME:if (C_S_PREPARING == civclient_state
	&& C_S_RUNNING == newstate) {
      audio_stop();		// stop intro sound loop 
    }*/

    civclient_state = newstate;

    switch (civclient_state) {
    case C_S_RUNNING:
      chatbox_text = " ";
      
      show_new_game_message();
      
      /*init_city_report_game_data();
      load_ruleset_specific_options();
      create_event(NULL, E_GAME_START, _("Game started."));
      precalc_tech_data();
      if (pplayer) {
        player_research_update(pplayer);
      }
      role_unit_precalcs();
      boot_help_texts(pplayer);	 //reboot with player 
      can_slide = FALSE;
      update_unit_focus();
      can_slide = TRUE;*/
      set_client_page(PAGE_GAME);
      setup_window_size();

      if (observing) center_tile_mapcanvas(map_pos_to_tile(15,15));

      /*// Find something sensible to display instead of the intro gfx. 
      center_on_something();
      free_intro_radar_sprites();
      agents_game_start();
      editgui_tileset_changed();*/
      break;
    case C_S_OVER:
      //if (C_S_RUNNING == oldstate) {
        /*
         * Extra kludge for end-game handling of the CMA.
         */
        /*if (pplayer && pplayer->cities) {
          city_list_iterate(pplayer->cities, pcity) {
            if (cma_is_city_under_agent(pcity, NULL)) {
              cma_release_city(pcity);
            }
          } city_list_iterate_end;
        }
        popdown_all_city_dialogs();
        popdown_all_game_dialogs();
        set_unit_focus(NULL);*/
      //} else {
        /*init_city_report_game_data();
        precalc_tech_data();
        if (pplayer) {
          player_research_update(pplayer);
        }
        role_unit_precalcs();
        boot_help_texts(pplayer);            // reboot
        set_unit_focus(NULL);
        set_client_page(PAGE_GAME);
        center_on_something();*/
      //}
      break;
    case C_S_PREPARING:
      /*popdown_all_city_dialogs();
      close_all_diplomacy_dialogs();
      popdown_all_game_dialogs();
      clear_notify_window();
      if (C_S_INITIAL != oldstate) {
	client_game_free();
      }
      client_game_init();
      set_unit_focus(NULL);
      if (!client.conn.established && !with_ggz) {
	set_client_page(in_ggz ? PAGE_GGZ : PAGE_MAIN);
      } else {
	set_client_page(PAGE_START);
      }*/
      break;
    default:
      break;
    }
    //update_menus();
  }
  /*if (!client.conn.established && C_S_PREPARING == civclient_state) {
    client_game_free();
    client_game_init();
    gui_server_connect();
    if (auto_connect) {
      if (connect_error) {
	freelog(LOG_FATAL,
		_("There was an error while auto connecting; aborting."));
	exit(EXIT_FAILURE);
      } else {
	start_autoconnecting_to_server();
	auto_connect = FALSE;	// don't try this again 
      }
    } 
  }
  update_turn_done_button_state();
  update_conn_list_dialog();*/


}

function client_state()
{
  return civclient_state;
}


/**************************************************************************
  Return TRUE if the client can change the view; i.e. if the mapview is
  active.  This function should be called each time before allowing the
  user to do mapview actions.
**************************************************************************/
function can_client_change_view()
{
  return ((client.conn.playing != null || client_is_observer())
      && (C_S_RUNNING == client_state()
	      || C_S_OVER == client_state())); 
}

/**************************************************************************
  Webclient does have observer support.
**************************************************************************/
function client_is_observer()
{
  return client.conn['observer'] || observing;
}

/**************************************************************************
  Intro message
**************************************************************************/
function show_new_game_message()
{

  if (observing) {
    /* do nothing. */

  } else if (is_small_screen()) {
    show_dialog_message("Welcome to Freeciv-web", 
      "You lead a civilization. Your task is to conquer the world!\n\
      Good luck, and have a lot of fun!");

  } else  {
    show_dialog_message("Welcome to Freeciv-web", 
      "Welcome to Freeciv-web.  You lead a civilization.  Your\n\
      task is to conquer the world!  You should start by\n\
      exploring the land around you with your explorer,\n\
      and using your settlers to find a good place to build\n\
      a city. To move your units around, either use the number \n\
      pad, or click on the Goto button, then click on the \n\
      destination tile on the map.\n\
      Good luck, and have a lot of fun!");
  } 
}

/**************************************************************************
...
**************************************************************************/
function alert_war(player_no)
{
  var pplayer = players[player_no];
  show_dialog_message("War!", "You are now at war with the " 
	+ nations[pplayer['nation']]['adjective']
    + " leader " + pplayer['name'] + "!");
}
