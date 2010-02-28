/********************************************************************** 
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
#include <config.h>
#endif

#include <assert.h>
#include <stdarg.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* common */
#include "diptreaty.h"
#include "game.h"
#include "government.h"
#include "movement.h"
#include "packets.h"
#include "player.h"
#include "tech.h"
#include "unitlist.h"

/* scripting */
#include "script.h"

/* server */
#include "aiiface.h"
#include "citytools.h"
#include "cityturn.h"
#include "connecthand.h"
#include "diplhand.h"
#include "gamehand.h"
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "sernet.h"
#include "settlers.h"
#include "srv_main.h"
#include "stdinhand.h"
#include "spaceship.h"
#include "spacerace.h"
#include "techtools.h"
#include "unittools.h"
#include "voting.h"

/* ai */
#include "advdiplomacy.h"
#include "advmilitary.h"
#include "aidata.h"

static void package_player_common(struct player *plr,
                                  struct packet_player_info *packet);

static void package_player_info(struct player *plr,
                                struct packet_player_info *packet,
                                struct player *receiver,
                                enum plr_info_level min_info_level);
static enum plr_info_level player_info_level(struct player *plr,
					     struct player *receiver);

/* Used by shuffle_players() and shuffled_player(). */
static int shuffled_order[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];

/**************************************************************************
  Send end-of-turn notifications relevant to specified dests.
  If dest is NULL, do all players, sending to pplayer->connections.
**************************************************************************/
void send_player_turn_notifications(struct conn_list *dest)
{
  if (dest) {
    conn_list_iterate(dest, pconn) {
      struct player *pplayer = pconn->playing;

      if (NULL != pplayer) {
	city_list_iterate(pplayer->cities, pcity) {
	  send_city_turn_notifications(pconn->self, pcity);
	}
	city_list_iterate_end;
      }
    }
    conn_list_iterate_end;
  }
  else {
    players_iterate(pplayer) {
      city_list_iterate(pplayer->cities, pcity) {
	send_city_turn_notifications(pplayer->connections, pcity);
      } city_list_iterate_end;
    } players_iterate_end;
  }

  send_global_city_turn_notifications(dest);
}

/**************************************************************************
  Murder a player in cold blood.

  Called from srv_main kill_dying_players() and edit packet handler
  handle_edit_player_remove().
**************************************************************************/
void kill_player(struct player *pplayer)
{
  bool palace;

  pplayer->is_alive = FALSE;

  /* Remove shared vision from dead player to friends. */
  players_iterate(aplayer) {
    if (gives_shared_vision(pplayer, aplayer)) {
      remove_shared_vision(pplayer, aplayer);
    }
  } players_iterate_end;
    
  cancel_all_meetings(pplayer);

  /* Show entire map for players who are *not* in a team. */
  if (pplayer->team->players == 1) {
    map_know_and_see_all(pplayer);
  }

  if (!is_barbarian(pplayer)) {
    notify_player(NULL, NULL, E_DESTROYED, FTC_SERVER_INFO, NULL,
                  _("The %s are no more!"),
                  nation_plural_for_player(pplayer));
  }

  /* Transfer back all cities not originally owned by player to their
     rightful owners, if they are still around */
  palace = game.info.savepalace;
  game.info.savepalace = FALSE; /* moving it around is dumb */
  city_list_iterate(pplayer->cities, pcity) {
    if (pcity->original != pplayer && pcity->original->is_alive) {
      /* Transfer city to original owner, kill all its units outside of
         a radius of 3, give verbose messages of every unit transferred,
         and raze buildings according to raze chance (also removes palace) */
      transfer_city(pcity->original, pcity, 3, TRUE, TRUE, TRUE);
    }
  } city_list_iterate_end;

  /* Remove all units that are still ours */
  unit_list_iterate_safe(pplayer->units, punit) {
    wipe_unit(punit);
  } unit_list_iterate_safe_end;

  /* Destroy any remaining cities */
  city_list_iterate(pplayer->cities, pcity) {
    remove_city(pcity);
  } city_list_iterate_end;
  game.info.savepalace = palace;

  /* Remove ownership of tiles */
  whole_map_iterate(ptile) {
    if (tile_owner(ptile) == pplayer) {
      map_claim_ownership(ptile, NULL, NULL);
    }
  } whole_map_iterate_end;

  /* Ensure this dead player doesn't win with a spaceship.
   * Now that would be truly unbelievably dumb - Per */
  spaceship_init(&pplayer->spaceship);
  send_spaceship_info(pplayer, NULL);

  send_player_info_c(pplayer, game.est_connections);
}

/**************************************************************************
  Handle a client or AI request to change the tax/luxury/science rates.
  This function does full sanity checking.
**************************************************************************/
void handle_player_rates(struct player *pplayer,
			 int tax, int luxury, int science)
{
  int maxrate;

  if (S_S_RUNNING != server_state()) {
    freelog(LOG_ERROR, "received player_rates packet from %s before start",
	    player_name(pplayer));
    notify_player(pplayer, NULL, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
		  _("Cannot change rates before game start."));
    return;
  }
	
  if (tax + luxury + science != 100) {
    return;
  }
  if (tax < 0 || tax > 100 || luxury < 0 || luxury > 100 || science < 0
      || science > 100) {
    return;
  }
  maxrate = get_player_bonus(pplayer, EFT_MAX_RATES);
  if (tax > maxrate || luxury > maxrate || science > maxrate) {
    const char *rtype;

    if (tax > maxrate) {
      rtype = _("Tax");
    } else if (luxury > maxrate) {
      rtype = _("Luxury");
    } else {
      rtype = _("Science");
    }

    notify_player(pplayer, NULL, E_BAD_COMMAND, FTC_SERVER_INFO, NULL,
		  _("%s rate exceeds the max rate for %s."),
                  rtype,
                  government_name_for_player(pplayer));
  } else {
    pplayer->economic.tax = tax;
    pplayer->economic.luxury = luxury;
    pplayer->economic.science = science;

    city_refresh_for_player(pplayer);
    send_player_info(pplayer, pplayer);
  }
}

/**************************************************************************
  Finish the revolution and set the player's government.  Call this as soon
  as the player has set a target_government and the revolution_finishes
  turn has arrived.
**************************************************************************/
static void finish_revolution(struct player *pplayer)
{
  struct government *government = pplayer->target_government;

  if (pplayer->target_government == game.government_during_revolution
      || pplayer->target_government == NULL) {
    /* More descriptive assert than just assert(FALSE) */
    assert(pplayer->target_government != game.government_during_revolution
           && pplayer->target_government != NULL);
    return;
  }
  if (pplayer->revolution_finishes > game.info.turn) {
    /* More descriptive assert than just assert(FALSE) */
    assert(pplayer->revolution_finishes <= game.info.turn);
    return;
  }

  pplayer->government = government;
  pplayer->target_government = NULL;

  freelog(LOG_DEBUG,
	  "Revolution finished for %s.  Government is %s.  Revofin %d (%d).",
	  player_name(pplayer),
	  government_rule_name(government),
	  pplayer->revolution_finishes, game.info.turn);
  notify_player(pplayer, NULL, E_REVOLT_DONE, FTC_SERVER_INFO, NULL,
                _("%s now governs the %s as a %s."), 
                player_name(pplayer), 
                nation_plural_for_player(pplayer),
                government_name_translation(government));

  if (!pplayer->ai_data.control) {
    /* Keep luxuries if we have any.  Try to max out science. -GJW */
    int max = get_player_bonus(pplayer, EFT_MAX_RATES);

    pplayer->economic.science
      = MIN(100 - pplayer->economic.luxury, max);
    pplayer->economic.tax
      = MIN(100 - pplayer->economic.luxury - pplayer->economic.science, max);
    pplayer->economic.luxury
      = 100 - pplayer->economic.science - pplayer->economic.tax;
  }

  check_player_max_rates(pplayer);
  city_refresh_for_player(pplayer);
  send_player_info(pplayer, pplayer);
}

/**************************************************************************
  Called by the client or AI to change government.
**************************************************************************/
void handle_player_change_government(struct player *pplayer, int government)
{
  int turns;
  struct government *gov = government_by_number(government);

  if (!gov || !can_change_to_government(pplayer, gov)) {
    return;
  }

  freelog(LOG_DEBUG,
	  "Government changed for %s.  Target government is %s; "
	  "old %s.  Revofin %d, Turn %d.",
	  player_name(pplayer),
	  government_rule_name(gov),
	  government_rule_name(government_of_player(pplayer)),
	  pplayer->revolution_finishes, game.info.turn);

  /* Set revolution_finishes value. */
  if (pplayer->revolution_finishes > 0) {
    /* Player already has an active revolution.  Note that the finish time
     * may be in the future (we're waiting for it to finish), the current
     * turn (it just finished - but isn't reset until the end of the turn)
     * or even in the past (if the player is in anarchy and hasn't chosen
     * a government). */
    turns = pplayer->revolution_finishes - game.info.turn;
  } else if ((pplayer->ai_data.control && !ai_handicap(pplayer, H_REVOLUTION))
	     || get_player_bonus(pplayer, EFT_NO_ANARCHY)) {
    /* AI players without the H_REVOLUTION handicap can skip anarchy */
    turns = 0;
  } else if (game.info.revolution_length == 0) {
    turns = myrand(5) + 1;
  } else {
    turns = game.info.revolution_length;
  }

  pplayer->government = game.government_during_revolution;
  pplayer->target_government = gov;
  pplayer->revolution_finishes = game.info.turn + turns;

  freelog(LOG_DEBUG,
	  "Revolution started for %s.  Target government is %s.  "
	  "Revofin %d (%d).",
	  player_name(pplayer),
	  government_rule_name(pplayer->target_government),
	  pplayer->revolution_finishes, game.info.turn);

  /* Now see if the revolution is instantaneous. */
  if (turns <= 0
      && pplayer->target_government != game.government_during_revolution) {
    finish_revolution(pplayer);
    return;
  } else if (turns > 0) {
    notify_player(pplayer, NULL, E_REVOLT_START, FTC_SERVER_INFO, NULL,
                  /* TRANS: this is a message event so don't make it
                   * too long. */
                  PL_("The %s have incited a revolt! "
                      "%d turn of anarchy will ensue! "
                      "Target government is %s.",
                      "The %s have incited a revolt! "
                      "%d turns of anarchy will ensue! "
                      "Target government is %s.",
                      turns),
                  nation_plural_for_player(pplayer),
                  turns,
                  government_name_translation(pplayer->target_government));
  } else {
    assert(pplayer->target_government == game.government_during_revolution);
    notify_player(pplayer, NULL, E_REVOLT_START, FTC_SERVER_INFO, NULL,
                  _("Revolution: returning to anarchy."));
  }

  check_player_max_rates(pplayer);
  city_refresh_for_player(pplayer);
  send_player_info(pplayer, pplayer);

  freelog(LOG_DEBUG,
	  "Government change complete for %s.  Target government is %s; "
	  "now %s.  Turn %d; revofin %d.",
	  player_name(pplayer),
	  government_rule_name(pplayer->target_government),
	  government_rule_name(government_of_player(pplayer)),
	  game.info.turn, pplayer->revolution_finishes);
}

/**************************************************************************
  See if the player has finished their revolution.  This function should
  be called at the beginning of a player's phase.
**************************************************************************/
void update_revolution(struct player *pplayer)
{
  /* The player's revolution counter is stored in the revolution_finishes
   * field.  This value has the following meanings:
   *   - If negative (-1), then the player is not in a revolution.  In this
   *     case the player should never be in anarchy.
   *   - If positive, the player is in the middle of a revolution.  In this
   *     case the value indicates the turn in which the revolution finishes.
   *     * If this value is > than the current turn, then the revolution is
   *       in progress.  In this case the player should always be in anarchy.
   *     * If the value is == to the current turn, then the revolution is
   *       finished.  The player may now choose a government.  However the
   *       value isn't reset until the end of the turn.  If the player has
   *       chosen a government by the end of the turn, then the revolution is
   *       over and the value is reset to -1.
   *     * If the player doesn't pick a government then the revolution
   *       continues.  At this point the value is <= to the current turn,
   *       and the player can leave the revolution at any time.  The value
   *       is reset at the end of any turn when a non-anarchy government is
   *       chosen.
   */
  freelog(LOG_DEBUG, "Update revolution for %s.  Current government %s, "
	  "target %s, revofin %d, turn %d.",
	  player_name(pplayer),
	  government_rule_name(government_of_player(pplayer)),
	  pplayer->target_government
	  ? government_rule_name(pplayer->target_government)
	  : "(none)",
	  pplayer->revolution_finishes, game.info.turn);
  if (government_of_player(pplayer) == game.government_during_revolution
      && pplayer->revolution_finishes <= game.info.turn) {
    if (pplayer->target_government != game.government_during_revolution) {
      /* If the revolution is over and a target government is set, go into
       * the new government. */
      freelog(LOG_DEBUG, "Update: finishing revolution for %s.",
	      player_name(pplayer));
      finish_revolution(pplayer);
    } else {
      /* If the revolution is over but there's no target government set,
       * alert the player. */
      notify_player(pplayer, NULL, E_REVOLT_DONE, NULL, NULL,
                    _("You should choose a new government from the "
                      "government menu."));
    }
  } else if (government_of_player(pplayer) != game.government_during_revolution
	     && pplayer->revolution_finishes < game.info.turn) {
    /* Reset the revolution counter.  If the player has another revolution
     * they'll have to re-enter anarchy. */
    freelog(LOG_DEBUG, "Update: resetting revofin for %s.",
	    player_name(pplayer));
    pplayer->revolution_finishes = -1;
    send_player_info(pplayer, pplayer);
  }
}

/**************************************************************************
The following checks that government rates are acceptable for the present
form of government. Has to be called when switching governments or when
toggling from AI to human.
**************************************************************************/
void check_player_max_rates(struct player *pplayer)
{
  struct player_economic old_econ = pplayer->economic;

  pplayer->economic = player_limit_to_max_rates(pplayer);
  if (old_econ.tax > pplayer->economic.tax) {
    notify_player(pplayer, NULL, E_NEW_GOVERNMENT, FTC_SERVER_INFO, NULL,
		  _("Tax rate exceeded the max rate; adjusted."));
  }
  if (old_econ.science > pplayer->economic.science) {
    notify_player(pplayer, NULL, E_NEW_GOVERNMENT, FTC_SERVER_INFO, NULL,
		  _("Science rate exceeded the max rate; adjusted."));
  }
  if (old_econ.luxury > pplayer->economic.luxury) {
    notify_player(pplayer, NULL, E_NEW_GOVERNMENT, FTC_SERVER_INFO, NULL,
		  _("Luxury rate exceeded the max rate; adjusted."));
  }
}

/**************************************************************************
  After the alliance is breaken, we need to do two things:
  - Inform clients that they cannot see units inside the former's ally
    cities
  - Remove units stacked together
**************************************************************************/
void update_players_after_alliance_breakup(struct player* pplayer,
                                          struct player* pplayer2)
{
  /* The client needs updated diplomatic state, because it is used
   * during calculation of new states of occupied flags in cities */
   send_player_info(pplayer, NULL);
   send_player_info(pplayer2, NULL);
   remove_allied_visibility(pplayer, pplayer2);
   remove_allied_visibility(pplayer2, pplayer);    
   resolve_unit_stacks(pplayer, pplayer2, TRUE);
}


/**************************************************************************
  Handles a player cancelling a "pact" with another player.

  packet.id is id of player we want to cancel a pact with
  packet.val1 is a special value indicating what kind of treaty we want
    to break. If this is CLAUSE_VISION we break shared vision. If it is
    a pact treaty type, we break one pact level. If it is CLAUSE_LAST
    we break _all_ treaties and go straight to war.
**************************************************************************/
void handle_diplomacy_cancel_pact(struct player *pplayer,
				  int other_player_id,
				  enum clause_type clause)
{
  enum diplstate_type old_type;
  enum diplstate_type new_type;
  enum dipl_reason diplcheck;
  bool repeat = FALSE;
  struct player *pplayer2 = valid_player_by_number(other_player_id);

  if (NULL == pplayer2) {
    return;
  }

  old_type = pplayer->diplstates[other_player_id].type;

  if (clause == CLAUSE_VISION) {
    if (!gives_shared_vision(pplayer, pplayer2)) {
      return;
    }
    remove_shared_vision(pplayer, pplayer2);
    notify_player(pplayer2, NULL, E_TREATY_BROKEN, FTC_SERVER_INFO, NULL,
                  _("%s no longer gives us shared vision!"),
                  player_name(pplayer));
    return;
  }

  diplcheck = pplayer_can_cancel_treaty(pplayer, pplayer2);

  /* The senate may not allow you to break the treaty.  In this case you
   * must first dissolve the senate then you can break it. */
  if (diplcheck == DIPL_SENATE_BLOCKING) {
    notify_player(pplayer, NULL, E_TREATY_BROKEN, FTC_SERVER_INFO, NULL,
                  _("The senate will not allow you to break treaty "
                    "with the %s.  You must either dissolve the senate "
                    "or wait until a more timely moment."),
                  nation_plural_for_player(pplayer2));
    return;
  }

  if (diplcheck != DIPL_OK) {
    return;
  }

  reject_all_treaties(pplayer);
  reject_all_treaties(pplayer2);
  /* else, breaking a treaty */

  /* check what the new status will be */
  switch(old_type) {
  case DS_NO_CONTACT: /* possible if someone declares war on our ally */
  case DS_ARMISTICE:
  case DS_CEASEFIRE:
  case DS_PEACE:
    new_type = DS_WAR;
    break;
  case DS_ALLIANCE:
    new_type = DS_ARMISTICE;
    break;
  default:
    freelog(LOG_ERROR, "non-pact diplstate in handle_player_cancel_pact");
    assert(FALSE);
    return;
  }

  /* do the change */
  pplayer->diplstates[player_index(pplayer2)].type =
    pplayer2->diplstates[player_index(pplayer)].type =
    new_type;
  pplayer->diplstates[player_index(pplayer2)].turns_left =
    pplayer2->diplstates[player_index(pplayer)].turns_left =
    16;

  /* If the old state was alliance, the players' units can share tiles
     illegally, and we need to call resolve_unit_stacks() */
  if (old_type == DS_ALLIANCE) {
    update_players_after_alliance_breakup(pplayer, pplayer2);
  }

  /* if there's a reason to cancel the pact, do it without penalty */
  /* FIXME: in the current implementation if you break more than one
   * treaty simultaneously it may partially succed: the first treaty-breaking
   * will happen but the second one will fail. */
  if (get_player_bonus(pplayer, EFT_HAS_SENATE) > 0 && !repeat) {
    if (pplayer->diplstates[player_index(pplayer2)].has_reason_to_cancel > 0) {
      notify_player(pplayer, NULL, E_TREATY_BROKEN, FTC_SERVER_INFO, NULL,
                    _("The senate passes your bill because of the "
                      "constant provocations of the %s."),
                    nation_plural_for_player(pplayer2));
    } else if (new_type == DS_WAR) {
      notify_player(pplayer, NULL, E_TREATY_BROKEN, FTC_SERVER_INFO, NULL,
                    _("The senate refuses to break treaty with the %s, "
                      "but you have no trouble finding a new senate."),
                    nation_plural_for_player(pplayer2));
    }
  }
  if (new_type == DS_WAR) {
    call_incident(INCIDENT_WAR, pplayer, pplayer2);
  }
  pplayer->diplstates[player_index(pplayer2)].has_reason_to_cancel = 0;

  send_player_info(pplayer, NULL);
  send_player_info(pplayer2, NULL);

  if (old_type == DS_ALLIANCE) {
    /* Inform clients about units that have been hidden.  Units in cities
     * and transporters are visible to allies but not visible once the
     * alliance is broken.  We have to call this after resolve_unit_stacks
     * because that function may change units' locations.  It also sends
     * out new city info packets to tell the client about occupied cities,
     * so it should also come after the send_player_info calls above. */
    remove_allied_visibility(pplayer, pplayer2);
    remove_allied_visibility(pplayer2, pplayer);
  }

  /* 
   * Refresh all cities which have a unit of the other side within
   * city range. 
   */
  city_map_update_all_cities_for_player(pplayer);
  city_map_update_all_cities_for_player(pplayer2);
  sync_cities();

  notify_player(pplayer, NULL, E_TREATY_BROKEN, FTC_SERVER_INFO, NULL,
                _("The diplomatic state between the %s "
                  "and the %s is now %s."),
                nation_plural_for_player(pplayer),
                nation_plural_for_player(pplayer2),
                diplstate_text(new_type));
  notify_player(pplayer2, NULL, E_TREATY_BROKEN, FTC_SERVER_INFO, NULL,
                _(" %s canceled the diplomatic agreement! "
                  "The diplomatic state between the %s and the %s "
                  "is now %s."),
                player_name(pplayer),
                nation_plural_for_player(pplayer2),
                nation_plural_for_player(pplayer),
                diplstate_text(new_type));

  /* Check fall-out of a war declaration. */
  players_iterate(other) {
    if (other->is_alive && other != pplayer && other != pplayer2
        && new_type == DS_WAR && pplayers_allied(pplayer2, other)
        && pplayers_allied(pplayer, other)) {
      if (!players_on_same_team(pplayer, other)) {
        /* If an ally declares war on another ally, break off your alliance
         * to the aggressor. This prevents in-alliance wars, which are not
         * permitted. */
        notify_player(other, NULL, E_TREATY_BROKEN, FTC_SERVER_INFO, NULL,
                      _("%s has attacked your ally %s! "
                        "You cancel your alliance to the aggressor."),
                      player_name(pplayer),
                      player_name(pplayer2));
        other->diplstates[player_index(pplayer)].has_reason_to_cancel = 1;
        handle_diplomacy_cancel_pact(other, player_number(pplayer),
                                     CLAUSE_ALLIANCE);
      } else {
        /* We are in the same team as the agressor; we cannot break 
         * alliance with him. We trust our team mate and break alliance
         * with the attacked player */
        notify_player(other, NULL, E_TREATY_BROKEN, FTC_SERVER_INFO, NULL,
                      _("Your team mate %s declared war on %s. "
                        "You are obligated to cancel alliance with %s."),
                      player_name(pplayer),
                      nation_plural_for_player(pplayer2),
                      player_name(pplayer2));
        handle_diplomacy_cancel_pact(other, player_number(pplayer2), CLAUSE_ALLIANCE);
      }
    }
  } players_iterate_end;
}

/**************************************************************************
  Send information about a player slot, i.e. a possibly uninitialized
  player ('used' is FALSE). This should be used for example after a player
  is removed by server_remove_player(). For used initialized player slots
  this function has the same effect as send_player_info_c() (albeit you
  cannot have 'src' equal to NULL). If 'dest' is NULL, then it is set
  to game.est_connections.
**************************************************************************/
void send_player_slot_info_c(struct player *src, struct conn_list *dest)
{
  struct packet_player_info info;

  if (!src) {
    return;
  }

  if (!dest) {
    dest = game.est_connections;
  }

  if (player_slot_is_used(src)) {
    package_player_common(src, &info);
  }

  conn_list_iterate(dest, pconn) {
    if (player_slot_is_used(src)) {
      if (NULL == pconn->playing && pconn->observer) {
        /* Global observer. */
        package_player_info(src, &info, pconn->playing, INFO_FULL);
      } else if (NULL != pconn->playing) {
        /* Players (including regular observers) */
        package_player_info(src, &info, pconn->playing, INFO_MINIMUM);
      } else {
        package_player_info(src, &info, NULL, INFO_MINIMUM);
      }
      send_packet_player_info(pconn, &info);
    } else {
      dsend_packet_player_remove(pconn, player_number(src));
    }
  } conn_list_iterate_end;
}

/**************************************************************************
  Send information about player slot 'src', or all valid (i.e. used and
  initialized) players if 'src' is NULL, to specified clients 'dest'.
  If 'dest' is NULL, it is treated as game.est_connections.

  Note: package_player_info contains incomplete info if it has NULL as a
        dest arg and and info is < INFO_EMBASSY.
  NB: If 'src' is NULL (meaning send information about all players) this
  function will not send info for unused players, i.e. player slots with
  the 'used' field equal to FALSE. Use send_player_slot_info_c for that.
**************************************************************************/
void send_player_info_c(struct player *src, struct conn_list *dest)
{
  if (src != NULL) {
    send_player_slot_info_c(src, dest);
    return;
  }

  players_iterate(pplayer) {
    send_player_slot_info_c(pplayer, dest);
  } players_iterate_end;
}

/**************************************************************************
  Convenience form of send_player_info_c.
  Send information about player src, or all players if src is NULL,
  to specified players dest (that is, to dest->connections).
  As convenience to old code, dest may be NULL meaning send to
  game.est_connections.  
**************************************************************************/
void send_player_info(struct player *src, struct player *dest)
{
  send_player_info_c(src, (dest ? dest->connections : game.est_connections));
}

/**************************************************************************
 Package player information that is always sent.
**************************************************************************/
static void package_player_common(struct player *plr,
                                  struct packet_player_info *packet)
{
  int i;

  packet->playerno = player_number(plr);
  sz_strlcpy(packet->name, player_name(plr));
  sz_strlcpy(packet->username, plr->username);
  packet->nation = plr->nation ? nation_number(plr->nation) : -1;
  packet->is_male=plr->is_male;
  packet->team = plr->team ? team_number(plr->team) : -1;
  packet->is_ready = plr->is_ready;
  packet->was_created = plr->was_created;
  if (city_styles != NULL) {
    packet->city_style = city_style_of_player(plr);
  } else {
    packet->city_style = 0;
  }

  packet->is_alive=plr->is_alive;
  packet->is_connected=plr->is_connected;
  packet->ai = plr->ai_data.control;
  packet->ai_skill_level = plr->ai_data.control ? plr->ai_data.skill_level : 0;
  for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    packet->love[i] = plr->ai_data.love[i];
  }
  packet->barbarian_type = plr->ai_data.barbarian_type;

  packet->phase_done = plr->phase_done;
  packet->nturns_idle=plr->nturns_idle;

  for (i = 0; i < B_LAST/*improvement_count()*/; i++) {
    packet->wonders[i] = plr->wonders[i];
  }
  packet->science_cost = plr->ai_data.science_cost;
}

/**************************************************************************
  Package player info depending on info_level. We send everything to
  plr's connections, we send almost everything to players with embassy
  to plr, we send a little to players we are in contact with and almost
  nothing to everyone else.

  Receiver may be NULL in which cases dummy values are sent for some
  fields.
**************************************************************************/
static void package_player_info(struct player *plr,
                                struct packet_player_info *packet,
                                struct player *receiver,
                                enum plr_info_level min_info_level)
{
  int i;
  enum plr_info_level info_level;
  enum plr_info_level highest_team_level;
  struct player_research* research = get_player_research(plr);
  struct government *pgov = NULL;

  if (receiver) {
    info_level = player_info_level(plr, receiver);
    info_level = MAX(min_info_level, info_level);
  } else {
    info_level = min_info_level;
  }

  /* We need to send all tech info for all players on the same
   * team, even if they are not in contact yet; otherwise we will
   * overwrite team research or confuse the client. */
  highest_team_level = info_level;
  players_iterate(aplayer) {
    if (players_on_same_team(plr, aplayer) && receiver) {
      highest_team_level = MAX(highest_team_level,
                               player_info_level(aplayer, receiver));
    }
  } players_iterate_end;

  /* Only send score if we have contact */
  if (info_level >= INFO_MEETING) {
    packet->score = plr->score.game;
  } else {
    packet->score = 0;
  }

  if (info_level >= INFO_MEETING) {
    packet->gold = plr->economic.gold;
    pgov = government_of_player(plr);
  } else {
    packet->gold = 0;
    pgov = game.government_during_revolution;
  }
  packet->government = pgov ? government_number(pgov) : -1;
   
  /* Send diplomatic status of the player to everyone they are in
   * contact with. */
  if (info_level >= INFO_EMBASSY
      || (receiver
	  && receiver->diplstates[player_index(plr)].contact_turns_left > 0)) {
    packet->target_government = plr->target_government
                                ? government_number(plr->target_government)
                                : -1;
    memset(&packet->embassy, 0, sizeof(packet->embassy));
    players_iterate(pother) {
      packet->embassy[player_index(pother)]
	= BV_ISSET(plr->embassy, player_index(pother));
    } players_iterate_end;
    packet->gives_shared_vision = plr->gives_shared_vision;
    for(i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      packet->diplstates[i].type       = plr->diplstates[i].type;
      packet->diplstates[i].turns_left = plr->diplstates[i].turns_left;
      packet->diplstates[i].contact_turns_left = 
         plr->diplstates[i].contact_turns_left;
      packet->diplstates[i].has_reason_to_cancel = plr->diplstates[i].has_reason_to_cancel;
    }
  } else {
    packet->target_government = packet->government;
    memset(&packet->embassy, 0, sizeof(packet->embassy));
    if (receiver && player_has_embassy(plr, receiver)) {
      packet->embassy[player_index(receiver)] = TRUE;
    }
    if (!receiver || !gives_shared_vision(plr, receiver)) {
      packet->gives_shared_vision = 0;
    } else {
      packet->gives_shared_vision = 1 << player_index(receiver);
    }

    for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
      packet->diplstates[i].type       = DS_WAR;
      packet->diplstates[i].turns_left = 0;
      packet->diplstates[i].has_reason_to_cancel = 0;
      packet->diplstates[i].contact_turns_left = 0;
    }
    /* We always know the player's relation to us */
    if (receiver) {
      int p_no = player_index(receiver);

      packet->diplstates[p_no].type       = plr->diplstates[p_no].type;
      packet->diplstates[p_no].turns_left = plr->diplstates[p_no].turns_left;
      packet->diplstates[p_no].contact_turns_left = 
         plr->diplstates[p_no].contact_turns_left;
      packet->diplstates[p_no].has_reason_to_cancel =
	plr->diplstates[p_no].has_reason_to_cancel;
    }
  }

  /* Make absolutely sure - in case you lose your embassy! */
  if (info_level >= INFO_EMBASSY 
      || (receiver
	  && pplayer_get_diplstate(plr, receiver)->type == DS_TEAM)) {
    packet->bulbs_last_turn = plr->bulbs_last_turn;
  } else {
    packet->bulbs_last_turn = 0;
  }

  /* Send most civ info about the player only to players who have an
   * embassy. */
  if (highest_team_level >= INFO_EMBASSY) {
    advance_index_iterate(A_FIRST, i) {
      packet->inventions[i] = 
        research->inventions[i].state + '0';
    } advance_index_iterate_end;
    packet->tax             = plr->economic.tax;
    packet->science         = plr->economic.science;
    packet->luxury          = plr->economic.luxury;
    packet->bulbs_researched = research->bulbs_researched;
    packet->techs_researched = research->techs_researched;
    packet->researching = research->researching;
    packet->future_tech = research->future_tech;
    packet->revolution_finishes = plr->revolution_finishes;
  } else {
    advance_index_iterate(A_FIRST, i) {
      packet->inventions[i] = '0';
    } advance_index_iterate_end;
    packet->tax             = 0;
    packet->science         = 0;
    packet->luxury          = 0;
    packet->bulbs_researched= 0;
    packet->techs_researched= 0;
    packet->researching     = A_UNKNOWN;
    packet->future_tech     = 0;
    packet->revolution_finishes = -1;
  }

  /* We have to inform the client that the other players also know
   * A_NONE. */
  packet->inventions[A_NONE] = research->inventions[A_NONE].state + '0';
  packet->inventions[advance_count()] = '\0';
#ifdef DEBUG
  freelog(LOG_VERBOSE, "Player%d inventions:%s",
          player_number(plr),
          packet->inventions);
#endif

  if (info_level >= INFO_FULL
      || (receiver
	  && plr->diplstates[player_index(receiver)].type == DS_TEAM)) {
    packet->tech_goal       = research->tech_goal;
  } else {
    packet->tech_goal       = A_UNSET;
  }

  /* 
   * This may be an odd time to check these values but we can be sure
   * to have a consistent state here.
   */
  assert(S_S_RUNNING != server_state()
         || A_UNSET == research->researching
	 || is_future_tech(research->researching)
	 || (A_NONE != research->researching
	     && valid_advance_by_number(research->researching)));
  assert(A_UNSET == research->tech_goal
	 || (A_NONE != research->tech_goal
	     && valid_advance_by_number(research->tech_goal)));
}

/**************************************************************************
  ...
**************************************************************************/
static enum plr_info_level player_info_level(struct player *plr,
					     struct player *receiver)
{
  if (S_S_RUNNING > server_state()) {
    return INFO_MINIMUM;
  }
  if (plr == receiver) {
    return INFO_FULL;
  }
  if (receiver && player_has_embassy(receiver, plr)) {
    return INFO_EMBASSY;
  }
  if (receiver && could_intel_with_player(receiver, plr)) {
    return INFO_MEETING;
  }
  return INFO_MINIMUM;
}

/**************************************************************************
  Convenience function to return "reply" destination connection list
  for player: pplayer->current_conn if set, else pplayer->connections.
**************************************************************************/
struct conn_list *player_reply_dest(struct player *pplayer)
{
  return (pplayer->current_conn ?
	  pplayer->current_conn->self :
	  pplayer->connections);
}

/**************************************************************************
  Call first_contact function if such is defined for player
**************************************************************************/
static void call_first_contact(struct player *pplayer, struct player *aplayer)
{
  if (pplayer->ai->funcs.first_contact) {
    pplayer->ai->funcs.first_contact(pplayer, aplayer);
  }
}

/****************************************************************************
  Initialize ANY newly-created player on the server.

  The initmap option is used because we don't want to initialize the map
  before the x and y sizes have been determined.  This should generally
  be FALSE in pregame.

  The needs_team options should be set for players who should be assigned
  a team.  They will be put on their own newly-created team.
****************************************************************************/
void server_player_init(struct player *pplayer,
			bool initmap, bool needs_team)
{
  pplayer->ai = get_ai_type(AI_DEFAULT);

  if (initmap) {
    player_map_allocate(pplayer);
  }
  if (needs_team) {
    team_add_player(pplayer, find_empty_team());
  }
  ai_data_init(pplayer);
}

/********************************************************************** 
  Creates a new, uninitialized, used player slot. You should probably
  call server_player_init() to initialize it, and send_player_info()
  later to tell clients about it.
  
  May return NULL if creation was not possible.
***********************************************************************/
struct player *server_create_player(void)
{
  struct player *pplayer = NULL;

  player_slots_iterate(pslot) {
    if (!player_slot_is_used(pslot)) {
      pplayer = pslot;
      break;
    }
  } player_slots_iterate_end;

  if (!pplayer) {
    return NULL;
  }

  player_slot_set_used(pplayer, TRUE);
  set_player_count(player_count() + 1);

  return pplayer;
}

/********************************************************************** 
  This function does _not_ close any connections attached to this
  player. The function cut_connection() is used for that. Be sure
  to send_player_slot_info_c() afterwards to tell clients that the
  player slot has become unused.
***********************************************************************/
void server_remove_player(struct player *pplayer)
{
  if (!pplayer || !player_slot_is_used(pplayer)) {
    return;
  }

  freelog(LOG_NORMAL, _("Removing player %s."), player_name(pplayer));

  notify_conn(pplayer->connections, NULL, E_CONNECTION,
              FTC_SERVER_INFO, NULL,
	      _("You've been removed from the game!"));

  notify_conn(game.est_connections, NULL, E_CONNECTION,
              FTC_SERVER_INFO, NULL,
	      _("%s has been removed from the game."),
	      player_name(pplayer));

  if (is_barbarian(pplayer)) {
    server.nbarbarians--;
  }

  /* Note it is ok to remove the _current_ item in a list_iterate. */
  conn_list_iterate(pplayer->connections, pconn) {
    connection_detach(pconn);
  } conn_list_iterate_end;

  team_remove_player(pplayer);
  game_remove_player(pplayer);
  player_init(pplayer);
  player_map_free(pplayer);
  player_slot_set_used(pplayer, FALSE);
  set_player_count(player_count() - 1);

  aifill(game.info.aifill);

  send_updated_vote_totals(NULL);
}

/**************************************************************************
  Update contact info.
**************************************************************************/
void make_contact(struct player *pplayer1, struct player *pplayer2,
		  struct tile *ptile)
{
  int player1 = player_index(pplayer1);
  int player2 = player_index(pplayer2);

  if (pplayer1 == pplayer2
      || !pplayer1->is_alive
      || !pplayer2->is_alive) {
    return;
  }
  if (get_player_bonus(pplayer1, EFT_NO_DIPLOMACY) == 0
      && get_player_bonus(pplayer2, EFT_NO_DIPLOMACY) == 0) {
    pplayer1->diplstates[player2].contact_turns_left = game.info.contactturns;
    pplayer2->diplstates[player1].contact_turns_left = game.info.contactturns;
  }
  if (pplayer_get_diplstate(pplayer1, pplayer2)->type == DS_NO_CONTACT) {
    pplayer1->diplstates[player2].type = DS_WAR;
    pplayer2->diplstates[player1].type = DS_WAR;
    pplayer1->diplstates[player2].first_contact_turn = game.info.turn;
    pplayer2->diplstates[player1].first_contact_turn = game.info.turn;
    notify_player(pplayer1, ptile, E_FIRST_CONTACT, FTC_SERVER_INFO, NULL,
                  _("You have made contact with the %s, ruled by %s."),
                  nation_plural_for_player(pplayer2),
                  player_name(pplayer2));
    notify_player(pplayer2, ptile, E_FIRST_CONTACT, FTC_SERVER_INFO, NULL,
                  _("You have made contact with the %s, ruled by %s."),
                  nation_plural_for_player(pplayer1),
                  player_name(pplayer1));
    if (pplayer1->ai_data.control) {
      call_first_contact(pplayer1, pplayer2);
    }
    if (pplayer2->ai_data.control && !pplayer1->ai_data.control) {
      call_first_contact(pplayer2, pplayer1);
    }
    send_player_info(pplayer1, pplayer2);
    send_player_info(pplayer2, pplayer1);
    send_player_info(pplayer1, pplayer1);
    send_player_info(pplayer2, pplayer2);

    /* Check for new love-love-hate triangles and resolve them */
    players_iterate(pplayer3) {
      if (pplayer1 != pplayer3 && pplayer2 != pplayer3 && pplayer3->is_alive
          && pplayers_allied(pplayer1, pplayer3)
          && pplayers_allied(pplayer2, pplayer3)) {
        notify_player(pplayer3, NULL, E_TREATY_BROKEN, FTC_SERVER_INFO, NULL,
                      _("%s and %s meet and go to instant war. You cancel your alliance "
                        "with both."),
                      player_name(pplayer1),
                      player_name(pplayer2));
        pplayer3->diplstates[player_index(pplayer1)].has_reason_to_cancel = TRUE;
        pplayer3->diplstates[player_index(pplayer2)].has_reason_to_cancel = TRUE;
        handle_diplomacy_cancel_pact(pplayer3, player_number(pplayer1), CLAUSE_ALLIANCE);
        handle_diplomacy_cancel_pact(pplayer3, player_number(pplayer2), CLAUSE_ALLIANCE);
      }
    } players_iterate_end;

    return;
  } else {
    assert(pplayer_get_diplstate(pplayer2, pplayer1)->type != DS_NO_CONTACT);
  }
  if (player_has_embassy(pplayer1, pplayer2)
      || player_has_embassy(pplayer2, pplayer1)) {
    return; /* Avoid sending too much info over the network */
  }
  send_player_info(pplayer1, pplayer1);
  send_player_info(pplayer2, pplayer2);
}

/**************************************************************************
  Check if we make contact with anyone.
**************************************************************************/
void maybe_make_contact(struct tile *ptile, struct player *pplayer)
{
  square_iterate(ptile, 1, tile1) {
    struct city *pcity = tile_city(tile1);
    if (pcity) {
      make_contact(pplayer, city_owner(pcity), ptile);
    }
    unit_list_iterate_safe(tile1->units, punit) {
      make_contact(pplayer, unit_owner(punit), ptile);
    } unit_list_iterate_safe_end;
  } square_iterate_end;
}

/**************************************************************************
  Shuffle or reshuffle the player order, storing in static variables above.
**************************************************************************/
void shuffle_players(void)
{
  /* shuffled_order is defined global */
  int n = player_slot_count();
  int i;

  freelog(LOG_DEBUG, "shuffle_players: creating shuffled order");

  for (i = 0; i < n; i++) {
    shuffled_order[i] = i;
  }

  /* randomize it */
  array_shuffle(shuffled_order, n);

#ifdef DEBUG
  for (i = 0; i < n; i++) {
    freelog(LOG_DEBUG, "shuffled_order[%d] = %d", i, shuffled_order[i]);
  }
#endif
}

/**************************************************************************
  Initialize the shuffled players list (as from a loaded savegame).
**************************************************************************/
void set_shuffled_players(int *shuffled_players)
{
  int i;

  freelog(LOG_DEBUG, "set_shuffled_players: loading shuffled array %p",
          shuffled_players);

  for (i = 0; i < player_slot_count(); i++) {
    shuffled_order[i] = shuffled_players[i];
    freelog(LOG_DEBUG, "shuffled_order[%d] = %d", i, shuffled_order[i]);
  }
}

/**************************************************************************
  Returns the i'th shuffled player, or NULL.

  NB: You should never need to call this function directly.
**************************************************************************/
struct player *shuffled_player(int i)
{
  struct player *pplayer;

  pplayer = valid_player_by_number(shuffled_order[i]);
  freelog(LOG_DEBUG, "shuffled_player(%d) = %d (%p %s)",
          i, shuffled_order[i], pplayer, player_name(pplayer));
  return pplayer;
}

/**************************************************************************
  Returns how much two nations looks good in the same game.
  Negative return value means that we really really don't want these
  nations together.
**************************************************************************/
static int nations_match(struct nation_type* n1, struct nation_type* n2,
                         bool ignore_conflicts)
{
  int i, sum = 0;
  
  /* Scottish is a good civil war nation for British */
  if (!ignore_conflicts) {
  
    for (i = 0; i < n1->num_conflicts; i++) {
      if (n1->conflicts_with[i] == n2) {
        return -1;
      }
    }

    for (i = 0; i < n2->num_conflicts; i++) {
      if (n2->conflicts_with[i] == n1) {
        return -1;
      }
    }
  }
  for (i = 0; i < n1->num_groups; i++) {
    if (is_nation_in_group(n2, n1->groups[i])) {
      sum += n1->groups[i]->match;
    }
  }
  return sum;
}

/****************************************************************************
  This function return one of the nations available from the
  NO_NATION_SELECTED-terminated choices list. If no available nations in this
  file were found, return a random nation. If no nations are available, die.

  choices may be NULL; if so it's ignored.
  If only_available is set choose only nations that have is_available bit set.
****************************************************************************/
struct nation_type *pick_a_nation(struct nation_type **choices,
                                  bool ignore_conflicts,
                                  bool only_available,
                                  enum barbarian_type barb_type)
{
  enum {
    UNAVAILABLE, AVAILABLE, PREFERRED, UNWANTED
  } nations_used[nation_count()], looking_for;
  int match[nation_count()], pick;
  int num_nations_avail = 0, pref_nations_avail = 0;

  /* Values of nations_used: 
   * 0: not available - nation is already used or is a special nation
   * 1: available - we can use this nation
   * 2: preferred - we can use this nation and it is on the choices list 
   * 3: unwanted - we can used this nation, but we really don't want to
   */
  nations_iterate(pnation) {
    if (pnation->player
        || (only_available && !pnation->is_available)
        || (barb_type != nation_barbarian_type(pnation))
        || (barb_type == NOT_A_BARBARIAN && !is_nation_playable(pnation))) {
      /* Nation is unplayable or already used: don't consider it. */
      nations_used[nation_index(pnation)] = UNAVAILABLE;
      match[nation_index(pnation)] = 0;
      continue;
    }

    nations_used[nation_index(pnation)] = AVAILABLE;

    match[nation_index(pnation)] = 1;
    players_iterate(pplayer) {
      if (pplayer->nation != NO_NATION_SELECTED) {
        int x = nations_match(pnation, nation_of_player(pplayer), ignore_conflicts);
	if (x < 0) {
	  nations_used[nation_index(pnation)] = UNWANTED;
	  match[nation_index(pnation)] = 1;
	  break;
	} else {
	  match[nation_index(pnation)] += x * 100;
	}
      }
    } players_iterate_end;

    num_nations_avail += match[nation_index(pnation)];
  } nations_iterate_end;

  /* Mark as prefered those nations which are on the choices list and
   * which are AVAILABLE, but no UNWANTED */
  for (; choices && *choices != NO_NATION_SELECTED; choices++) {
    if (nations_used[nation_index(*choices)] == AVAILABLE) {
      pref_nations_avail += match[nation_index(*choices)];
      nations_used[nation_index(*choices)] = PREFERRED;
    }
  }
  
  nations_iterate(pnation) {
    if (nations_used[nation_index(pnation)] == UNWANTED) {
      nations_used[nation_index(pnation)] = AVAILABLE;
    }
  } nations_iterate_end;

  assert(num_nations_avail > 0);
  assert(pref_nations_avail >= 0);

  if (pref_nations_avail == 0) {
    pick = myrand(num_nations_avail);
    looking_for = AVAILABLE; /* Use any available nation. */
  } else {
    pick = myrand(pref_nations_avail);
    looking_for = PREFERRED; /* Use a preferred nation only. */
  }

  nations_iterate(pnation) {
    if (nations_used[nation_index(pnation)] == looking_for) {
      pick -= match[nation_index(pnation)];

      if (pick < 0) {
	return pnation;
      }
    }
  } nations_iterate_end;

  assert(0);
  return NO_NATION_SELECTED;
}

/****************************************************************************
  Called when something is changed; this resets everyone's readiness.
****************************************************************************/
void reset_all_start_commands(void)
{
  if (S_S_INITIAL != server_state()) {
    return;
  }
  players_iterate(pplayer) {
    if (pplayer->is_ready) {
      pplayer->is_ready = FALSE;
      send_player_info_c(pplayer, game.est_connections);
    }
  } players_iterate_end;
}

/**********************************************************************
This function creates a new player and copies all of it's science
research etc.  Players are both thrown into anarchy and gold is
split between both players.
                               - Kris Bubendorfer 
***********************************************************************/
static struct player *split_player(struct player *pplayer)
{
  struct player_research *new_research, *old_research;
  struct player *cplayer;
  struct nation_type **civilwar_nations;

  /* make a new player, or not */
  cplayer = server_create_player();
  if (!cplayer) {
    return NULL;
  }

  server_player_init(cplayer, TRUE, TRUE);

  /* select a new name and nation for the copied player. */
  /* Rebel will always be an AI player */
  civilwar_nations = get_nation_civilwar(nation_of_player(pplayer));
  player_set_nation(cplayer, pick_a_nation(civilwar_nations, TRUE, FALSE,
                                           NOT_A_BARBARIAN));
  pick_random_player_name(nation_of_player(cplayer), cplayer->name);

  sz_strlcpy(cplayer->username, ANON_USER_NAME);
  cplayer->is_connected = FALSE;
  cplayer->government = nation_of_player(cplayer)->init_government;
  assert(cplayer->revolution_finishes < 0);
  cplayer->capital = TRUE;

  /* cplayer is not yet part of players_iterate which goes only
     to player_count(). */
  players_iterate(other_player) {
    if (get_player_bonus(other_player, EFT_NO_DIPLOMACY)) {
      cplayer->diplstates[player_index(other_player)].type = DS_WAR;
      other_player->diplstates[player_index(cplayer)].type = DS_WAR;
    } else {
      cplayer->diplstates[player_index(other_player)].type = DS_NO_CONTACT;
      other_player->diplstates[player_index(cplayer)].type = DS_NO_CONTACT;
    }

    cplayer->diplstates[player_index(other_player)].has_reason_to_cancel = 0;
    cplayer->diplstates[player_index(other_player)].turns_left = 0;
    cplayer->diplstates[player_index(other_player)].contact_turns_left = 0;
    other_player->diplstates[player_index(cplayer)].has_reason_to_cancel = 0;
    other_player->diplstates[player_index(cplayer)].turns_left = 0;
    other_player->diplstates[player_index(cplayer)].contact_turns_left = 0;
    
    /* Send so that other_player sees updated diplomatic info;
     * pplayer will be sent later anyway
     */
    if (other_player != pplayer) {
      send_player_info(other_player, other_player);
    }
  } players_iterate_end;

  /* Split the resources */
  cplayer->economic.gold = pplayer->economic.gold;
  cplayer->economic.gold /= 2;
  pplayer->economic.gold -= cplayer->economic.gold;

  /* Copy the research */
  new_research = get_player_research(cplayer);
  old_research = get_player_research(pplayer);

  new_research->bulbs_researched = 0;
  new_research->techs_researched = old_research->techs_researched;
  new_research->researching = old_research->researching;
  new_research->tech_goal = old_research->tech_goal;

  advance_index_iterate(A_NONE, i) {
    new_research->inventions[i] = old_research->inventions[i];
  } advance_index_iterate_end;
  cplayer->phase_done = TRUE; /* Have other things to think
				 about - paralysis */
  BV_CLR_ALL(cplayer->embassy);   /* all embassies destroyed */

  /* Do the ai */

  cplayer->ai_data.control = TRUE;
  cplayer->ai_data.prev_gold = pplayer->ai_data.prev_gold;
  cplayer->ai_data.maxbuycost = pplayer->ai_data.maxbuycost;
  cplayer->ai_data.handicaps = pplayer->ai_data.handicaps;
  cplayer->ai_data.warmth = pplayer->ai_data.warmth;
  cplayer->ai_data.frost = pplayer->ai_data.frost;
  set_ai_level_direct(cplayer, game.info.skill_level);

  advance_index_iterate(A_NONE, i) {
    cplayer->ai_data.tech_want[i] = pplayer->ai_data.tech_want[i];
  } advance_index_iterate_end;
  
  /* change the original player */
  if (government_of_player(pplayer) != game.government_during_revolution) {
    pplayer->target_government = pplayer->government;
    pplayer->government = game.government_during_revolution;
    pplayer->revolution_finishes = game.info.turn + 1;
  }
  get_player_research(pplayer)->bulbs_researched = 0;
  BV_CLR_ALL(pplayer->embassy);   /* all embassies destroyed */

  /* give splitted player the embassies to his team mates back, if any */
  if (pplayer->team) {
    players_iterate(pdest) {
      if (pplayer->team == pdest->team
          && pplayer != pdest) {
        establish_embassy(pplayer, pdest);
      }
    } players_iterate_end;
  }

  pplayer->economic = player_limit_to_max_rates(pplayer);

  /* copy the maps */

  give_map_from_player_to_player(pplayer, cplayer);

  /* Not sure if this is necessary, but might be a good idea
   * to avoid doing some ai calculations with bogus data. */
  ai_data_phase_init(cplayer, TRUE);
  assess_danger_player(cplayer);
  if (pplayer->ai_data.control) {
    assess_danger_player(pplayer);
  }

  return cplayer;
}

/********************************************************************** 
civil_war_triggered:
 * The capture of a capital is not a sure fire way to throw
and empire into civil war.  Some governments are more susceptible 
than others, here are the base probabilities:
Anarchy   	90%
Despotism 	80%
Monarchy  	70%
Fundamentalism  60% (Only in civ2 ruleset)
Communism 	50%
Republic  	40%
Democracy 	30%
 * In addition each city in revolt adds 5%, each city in rapture 
subtracts 5% from the probability of a civil war.  
 * If you have at least 1 turns notice of the impending loss of 
your capital, you can hike luxuries up to the hightest value,
and by this reduce the chance of a civil war.  In fact by
hiking the luxuries to 100% under Democracy, it is easy to
get massively negative numbers - guaranteeing imunity from
civil war.  Likewise, 3 revolting cities under despotism
guarantees a civil war.
 * This routine calculates these probabilities and returns true
if a civil war is triggered.
                                   - Kris Bubendorfer 
***********************************************************************/
bool civil_war_triggered(struct player *pplayer)
{
  /* Get base probabilities */

  int dice = myrand(100); /* Throw the dice */
  int prob = get_player_bonus(pplayer, EFT_CIVIL_WAR_CHANCE);

  /* Now compute the contribution of the cities. */
  
  city_list_iterate(pplayer->cities, pcity)
    if (city_unhappy(pcity)) {
      prob += 5;
    }
    if (city_celebrating(pcity)) {
      prob -= 5;
    }
  city_list_iterate_end;

  freelog(LOG_VERBOSE, "Civil war chance for %s: prob %d, dice %d",
	  player_name(pplayer), prob, dice);
  
  return(dice < prob);
}

/**********************************************************************
Capturing a nation's capital is a devastating blow.  This function
creates a new AI player, and randomly splits the original players
city list into two.  Of course this results in a real mix up of 
teritory - but since when have civil wars ever been tidy, or civil.

Embassies:  All embassies with other players are lost.  Other players
            retain their embassies with pplayer.
 * Units:      Units inside cities are assigned to the new owner
            of the city.  Units outside are transferred along 
            with the ownership of their supporting city.
            If the units are in a unit stack with non rebel units,
            then whichever units are nearest an allied city
            are teleported to that city.  If the stack is a 
            transport at sea, then all rebel units on the 
            transport are teleported to their nearest allied city.

Cities:     Are split randomly into 2.  This results in a real
            mix up of teritory - but since when have civil wars 
            ever been tidy, or for any matter civil?
 *
One caveat, since the spliting of cities is random, you can
conceive that this could result in either the original player
or the rebel getting 0 cities.  To prevent this, the hack below
ensures that each side gets roughly half, which ones is still 
determined randomly.
                                   - Kris Bubendorfer
***********************************************************************/
void civil_war(struct player *pplayer)
{
  int i, j;
  struct player *cplayer;

  /* It is possible that this function gets called after pplayer
   * died. Player pointers are safe even after death. */
  if (!pplayer->is_alive) {
    return;
  }

  if (player_count() >= MAX_NUM_PLAYERS) {
    /* No space to make additional player */
    freelog(LOG_NORMAL,
            _("Could not throw %s into civil war - too many players"),
            nation_plural_for_player(pplayer));
    return;
  }
  if (normal_player_count() >= server.playable_nations) {
    /* No nation for additional player */
    freelog(LOG_NORMAL,
            _("Could not throw %s into civil war - no available nations"),
            nation_plural_for_player(pplayer));
    return;
  }

  cplayer = split_player(pplayer);

  /* Before units, cities, so clients know name of new nation
   * (for debugging etc).
   */
  send_player_info(cplayer,  NULL);
  send_player_info(pplayer,  NULL); 
  
  /* Now split the empire */

  freelog(LOG_VERBOSE,
	  "%s civil war; created AI %s",
	  nation_rule_name(nation_of_player(pplayer)),
	  nation_rule_name(nation_of_player(cplayer)));
  notify_player(pplayer, NULL, E_CIVIL_WAR, FTC_SERVER_INFO, NULL,
                _("Your nation is thrust into civil war."));

  notify_player(pplayer, NULL, E_FIRST_CONTACT, FTC_SERVER_INFO, NULL,
                /* TRANS: <leader> ... the Poles. */
                _("%s is the rebellious leader of the %s."),
                player_name(cplayer),
                nation_plural_for_player(cplayer));

  i = city_list_size(pplayer->cities)/2;   /* number to flip */
  j = city_list_size(pplayer->cities);	    /* number left to process */
  city_list_iterate(pplayer->cities, pcity) {
    if (!is_capital(pcity)) {
      if (i >= j || (i > 0 && myrand(2) == 1)) {
	/* Transfer city and units supported by this city to the new owner

	 We do NOT resolve stack conflicts here, but rather later.
	 Reason: if we have a transporter from one city which is carrying
	 a unit from another city, and both cities join the rebellion. We
	 resolved stack conflicts for each city we would teleport the first
	 of the units we met since the other would have another owner */
	transfer_city(cplayer, pcity, -1, FALSE, FALSE, FALSE);
	freelog(LOG_VERBOSE, "%s declares allegiance to the %s.",
		city_name(pcity),
		nation_rule_name(nation_of_player(cplayer)));
	notify_player(pplayer, pcity->tile, E_CITY_LOST,
                      FTC_SERVER_INFO, NULL,
                      /* TRANS: <city> ... the Poles. */
                      _("%s declares allegiance to the %s."),
                      city_link(pcity),
                      nation_plural_for_player(cplayer));
	i--;
      }
    }
    j--;
  } city_list_iterate_end;

  resolve_unit_stacks(pplayer, cplayer, FALSE);

  i = city_list_size(cplayer->cities);

  notify_player(NULL, NULL, E_CIVIL_WAR, FTC_SERVER_INFO, NULL,
		/* TRANS: ... Danes ... Poles ... <7> cities. */
		PL_("Civil war partitions the %s;"
		    " the %s now hold %d city.",
		    "Civil war partitions the %s;"
		    " the %s now hold %d cities.",
		    i),
		nation_plural_for_player(pplayer),
		nation_plural_for_player(cplayer),
		i);
}

/**************************************************************************
 The client has send as a chunk of the attribute block.
**************************************************************************/
void handle_player_attribute_chunk(struct player *pplayer,
				   struct packet_player_attribute_chunk
				   *chunk)
{
  generic_handle_player_attribute_chunk(pplayer, chunk);
}

/**************************************************************************
 The client request an attribute block.
**************************************************************************/
void handle_player_attribute_block(struct player *pplayer)
{
  send_attribute_block(pplayer, pplayer->current_conn);
}

/**************************************************************************
...
(Hmm, how should "turn done" work for multi-connected non-observer players?)
**************************************************************************/
void handle_player_phase_done(struct player *pplayer,
			      int turn)
{
  if (turn != game.info.turn) {
    /* If this happens then the player actually pressed turn-done on a
     * previous turn but we didn't receive it until now.  The player
     * probably didn't actually mean to end their turn! */
    return;
  }
  pplayer->phase_done = TRUE;

  check_for_full_turn_done();

  send_player_info(pplayer, NULL);
}

/**************************************************************************
  Return the number of barbarian players.
**************************************************************************/
int barbarian_count(void)
{
  return server.nbarbarians;
}

/**************************************************************************
  Return the number of non-barbarian players.
**************************************************************************/
int normal_player_count(void)
{
  return player_count() - server.nbarbarians;
}
