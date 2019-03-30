/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "shared.h"

/* common */
#include "calendar.h"
#include "events.h"
#include "game.h"
#include "packets.h"
#include "spaceship.h"

/* server */
#include "plrhand.h"
#include "notify.h"
#include "srv_main.h"

#include "spacerace.h"

/**********************************************************************//**
  Calculate and fill in the derived quantities about the spaceship.
  Data reverse engineered from Civ1. --dwp
  This could be in common, but its better for the client to take
  the values the server calculates, in case things change.
**************************************************************************/
void spaceship_calc_derived(struct player_spaceship *ship)
{
  int i;
  /* these are how many are connected: */
  int fuel = 0;
  int propulsion = 0;
  int habitation = 0;
  int life_support = 0;
  int solar_panels = 0;

  fc_assert_ret(ship->structurals <= NUM_SS_STRUCTURALS);
  fc_assert_ret(ship->components <= NUM_SS_COMPONENTS);
  fc_assert_ret(ship->modules <= NUM_SS_MODULES);
  
  ship->mass = 0;
  ship->support_rate = ship->energy_rate =
    ship->success_rate = ship->travel_time = 0.0;

  for (i = 0; i < NUM_SS_STRUCTURALS; i++) {
    if (BV_ISSET(ship->structure, i)) {
      ship->mass += (i < 6) ? 200 : 100;
      /* s0 to s3 are heavier; actually in Civ1 its a bit stranger
	 than this, but not worth figuring out --dwp */
    }
  }
  for (i = 0; i < ship->fuel; i++) {
    if (BV_ISSET(ship->structure, components_info[i * 2].required)) {
      fuel++;
    }
  }
  for (i = 0; i < ship->propulsion; i++) {
    if (BV_ISSET(ship->structure, components_info[i * 2 + 1].required)) {
      propulsion++;
    }
  }
  for (i = 0; i < ship->habitation; i++) {
    if (BV_ISSET(ship->structure, modules_info[i * 3].required)) {
      habitation++;
    }
  }
  for (i = 0; i < ship->life_support; i++) {
    if (BV_ISSET(ship->structure, modules_info[i * 3 + 1].required)) {
      life_support++;
    }
  }
  for (i = 0; i < ship->solar_panels; i++) {
    if (BV_ISSET(ship->structure, modules_info[i * 3 + 2].required)) {
      solar_panels++;
    }
  }

  ship->mass += 1600 * (habitation + life_support)
    + 400 * (solar_panels + propulsion + fuel);

  ship->population = habitation * 10000;

  if (habitation > 0) {
    ship->support_rate = life_support / (double) habitation;
  }
  if (life_support + habitation > 0) {
    ship->energy_rate = 2.0 * solar_panels / (double)(life_support+habitation);
  }
  if (fuel>0 && propulsion>0) {
    ship->success_rate =
	MIN(ship->support_rate, 1.0) * MIN(ship->energy_rate, 1.0);
  }

  /* The Success% can be less by up to a few % in some cases
     (I think if P != F or if P and/or F too small (eg <= 2?) ?)
     but probably not worth worrying about.
     Actually, the Civ1 manual suggests travel time is relevant. --dwp
  */

  ship->travel_time = ship->mass
    / (200.0 * MIN(propulsion,fuel) + 20.0);

}

/**********************************************************************//**
  Send details of src's spaceship (or spaceships of all players
  if src is NULL) to specified destinations.  If dest is NULL then
  game.est_connections is used.
**************************************************************************/
void send_spaceship_info(struct player *src, struct conn_list *dest)
{
  if (!dest) {
    dest = game.est_connections;
  }

  players_iterate(pplayer) {
    if (!src || pplayer == src) {
      struct packet_spaceship_info info;
      struct player_spaceship *ship = &pplayer->spaceship;
	  
      info.player_num = player_number(pplayer);
      info.sship_state = ship->state;
      info.structurals = ship->structurals;
      info.components = ship->components;
      info.modules = ship->modules;
      info.fuel = ship->fuel;
      info.propulsion = ship->propulsion;
      info.habitation = ship->habitation;
      info.life_support = ship->life_support;
      info.solar_panels = ship->solar_panels;
      info.launch_year = ship->launch_year;
      info.population = ship->population;
      info.mass = ship->mass;
      info.support_rate = ship->support_rate;
      info.energy_rate = ship->energy_rate;
      info.success_rate = ship->success_rate;
      info.travel_time = ship->travel_time;
      info.structure = ship->structure;
	  
      lsend_packet_spaceship_info(dest, &info);
    }
  } players_iterate_end;
}

/**********************************************************************//**
  Handle spaceship launch request.
**************************************************************************/
void handle_spaceship_launch(struct player *pplayer)
{
  struct player_spaceship *ship = &pplayer->spaceship;
  int arrival;

  if (!player_capital(pplayer)) {
    notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                  _("You need to have a capital in order to launch "
                    "your spaceship."));
    return;
  }
  if (ship->state >= SSHIP_LAUNCHED) {
    notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                  _("Your spaceship is already launched!"));
    return;
  }
  if (ship->state != SSHIP_STARTED
      || ship->success_rate == 0.0) {
    notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                  _("Your spaceship can't be launched yet!"));
    return;
  }

  ship->state = SSHIP_LAUNCHED;
  ship->launch_year = game.info.year;
  arrival = ship->launch_year + (int) ship->travel_time;

  notify_player(NULL, NULL, E_SPACESHIP, ftc_server,
                _("The %s have launched a spaceship!  "
                  "It is estimated to arrive at Alpha Centauri in %s."),
                nation_plural_for_player(pplayer),
                textyear(arrival));

  send_spaceship_info(pplayer, NULL);
}

/**********************************************************************//**
  Handle spaceship part placement request
**************************************************************************/
void handle_spaceship_place(struct player *pplayer,
                            enum spaceship_place_type type, int num)
{
  (void) do_spaceship_place(pplayer, ACT_REQ_PLAYER, type, num);
}

/**********************************************************************//**
  Place a spaceship part
**************************************************************************/
bool do_spaceship_place(struct player *pplayer, enum action_requester from,
                        enum spaceship_place_type type, int num)
{
  struct player_spaceship *ship = &pplayer->spaceship;
  
  if (ship->state == SSHIP_NONE) {
    if (from == ACT_REQ_PLAYER) {
      notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                    _("Spaceship action received,"
                      " but you don't have a spaceship!"));
    }

    return FALSE;
  }

  if (ship->state >= SSHIP_LAUNCHED) {
    if (from == ACT_REQ_PLAYER) {
      notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                    _("You can't modify your spaceship after launch!"));
    }

    return FALSE;
  }

  if (type == SSHIP_PLACE_STRUCTURAL) {
    if (num < 0 || num >= NUM_SS_STRUCTURALS
        || BV_ISSET(ship->structure, num)) {
      return FALSE;
    }
    if (num_spaceship_structurals_placed(ship) >= ship->structurals) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("You don't have any unplaced Space Structurals!"));
      }

      return FALSE;
    }
    if (num != 0
        && !BV_ISSET(ship->structure, structurals_info[num].required)) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("That Space Structural would not be connected!"));
      }

      return FALSE;
    }

    BV_SET(ship->structure, num);
    spaceship_calc_derived(ship);
    send_spaceship_info(pplayer, NULL);
    return TRUE;
  }

  if (type == SSHIP_PLACE_FUEL) {
    if (ship->fuel != num - 1) {
      return FALSE;
    }
    if (ship->fuel + ship->propulsion >= ship->components) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("You don't have any unplaced Space Components!"));
      }

      return FALSE;
    }
    if (num > NUM_SS_COMPONENTS/2) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("Your spaceship already has"
                        " the maximum number of Fuel Components!"));
      }

      return FALSE;
    }

    ship->fuel++;
    spaceship_calc_derived(ship);
    send_spaceship_info(pplayer, NULL);
    return TRUE;
  }

  if (type == SSHIP_PLACE_PROPULSION) {
    if (ship->propulsion != num - 1) {
      return FALSE;
    }
    if (ship->fuel + ship->propulsion >= ship->components) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("You don't have any unplaced"
                        " Space Components!"));
      }

      return FALSE;
    }
    if (num > NUM_SS_COMPONENTS/2) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("Your spaceship already has the"
                        " maximum number of Propulsion Components!"));
      }

      return FALSE;
    }

    ship->propulsion++;
    spaceship_calc_derived(ship);
    send_spaceship_info(pplayer, NULL);
    return TRUE;
  }

  if (type == SSHIP_PLACE_HABITATION) {
    if (ship->habitation != num - 1) {
      return FALSE;
    }
    if (ship->habitation + ship->life_support + ship->solar_panels
        >= ship->modules) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("You don't have any unplaced Space Modules!"));
      }

      return FALSE;
    }
    if (num > NUM_SS_MODULES / 3) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("Your spaceship already has the"
                        " maximum number of Habitation Modules!"));
      }

      return FALSE;
    }

    ship->habitation++;
    spaceship_calc_derived(ship);
    send_spaceship_info(pplayer, NULL);
    return TRUE;
  }

  if (type == SSHIP_PLACE_LIFE_SUPPORT) {
    if (ship->life_support != num - 1) {
      return FALSE;
    }
    if (ship->habitation + ship->life_support + ship->solar_panels
        >= ship->modules) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("You don't have any unplaced Space Modules!"));
      }

      return FALSE;
    }
    if (num > NUM_SS_MODULES / 3) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("Your spaceship already has the"
                        " maximum number of Life Support Modules!"));
      }

      return FALSE;
    }

    ship->life_support++;
    spaceship_calc_derived(ship);
    send_spaceship_info(pplayer, NULL);
    return TRUE;
  }

  if (type == SSHIP_PLACE_SOLAR_PANELS) {
    if (ship->solar_panels != num - 1) {
      return FALSE;
    }
    if (ship->habitation + ship->life_support + ship->solar_panels
        >= ship->modules) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("You don't have any unplaced Space Modules!"));
      }

      return FALSE;
    }
    if (num > NUM_SS_MODULES / 3) {
      if (from == ACT_REQ_PLAYER) {
        notify_player(pplayer, NULL, E_SPACESHIP, ftc_server,
                      _("Your spaceship already has the"
                        " maximum number of Solar Panel Modules!"));
      }

      return FALSE;
    }

    ship->solar_panels++;
    spaceship_calc_derived(ship);
    send_spaceship_info(pplayer, NULL);
    return TRUE;
  }

  log_error("Received unknown spaceship place type %d from %s",
            type, player_name(pplayer));
  return FALSE;
}

/**********************************************************************//**
  Handle spaceship arrival.
**************************************************************************/
void spaceship_arrived(struct player *pplayer)
{
  notify_player(NULL, NULL, E_SPACESHIP, ftc_server,
                _("The %s spaceship has arrived at Alpha Centauri."),
                nation_adjective_for_player(pplayer));
  pplayer->spaceship.state = SSHIP_ARRIVED;
}

/**********************************************************************//**
  Handle spaceship loss.
**************************************************************************/
void spaceship_lost(struct player *pplayer)
{
  notify_player(NULL, NULL, E_SPACESHIP, ftc_server,
                _("Without guidance from the capital, the %s "
                  "spaceship is lost!"),
                nation_adjective_for_player(pplayer));
  spaceship_init(&pplayer->spaceship);
  send_spaceship_info(pplayer, NULL);
}

/**********************************************************************//**
  Return arrival year of player's spaceship (fractional, as one spaceship
  may arrive before another in a given year).
  Only meaningful if spaceship has been launched.
**************************************************************************/
double spaceship_arrival(const struct player *pplayer)
{
  const struct player_spaceship *ship = &pplayer->spaceship;

  return ship->launch_year + ship->travel_time;
}

/**********************************************************************//**
  Rank launched player spaceships in order of arrival.
  'result' is an array big enough to hold all the players.
  Returns number of launched spaceships, having filled the start of
  'result' with that many players in order of predicted arrival.
  Uses shuffled player order in case of a tie.
**************************************************************************/
int rank_spaceship_arrival(struct player **result)
{
  int n = 0, i;

  shuffled_players_iterate(pplayer) {
    struct player_spaceship *ship = &pplayer->spaceship;
    
    if (ship->state == SSHIP_LAUNCHED) {
      result[n++] = pplayer;
    }
  } shuffled_players_iterate_end;

  /* An insertion sort will do; n is probably small, and we need a
   * stable sort to preserve the shuffled order for tie-breaking, so can't
   * use qsort() */
  for (i = 1; i < n; i++) {
    int j;
    for (j = i;
         j > 0
         && spaceship_arrival(result[j-1]) > spaceship_arrival(result[j]);
         j--) {
      struct player *tmp = result[j];
      result[j] = result[j-1];
      result[j-1] = tmp;
    }
  }

  return n;
}
