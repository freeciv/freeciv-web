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

#include <stdio.h>
#include <stdlib.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"

/* common */
#include "diptreaty.h"
#include "events.h"
#include "game.h"
#include "map.h"
#include "packets.h"
#include "player.h"
#include "unit.h"

/* ai */
#include "advdiplomacy.h"

/* scripting */
#include "script.h"

/* server */
#include "citytools.h"
#include "cityturn.h"
#include "maphand.h"
#include "plrhand.h"
#include "notify.h"
#include "settlers.h"
#include "techtools.h"
#include "unittools.h"

#include "diplhand.h"

#define SPECLIST_TAG treaty
#define SPECLIST_TYPE struct Treaty
#include "speclist.h"

#define treaty_list_iterate(list, p) \
    TYPED_LIST_ITERATE(struct Treaty, list, p)
#define treaty_list_iterate_end  LIST_ITERATE_END

static struct treaty_list *treaties = NULL;

/* FIXME: Should this be put in a ruleset somewhere? */
#define TURNS_LEFT 16

/**************************************************************************
  Calls treaty_evaluate function if such is set for AI player                           
**************************************************************************/
static void call_treaty_evaluate(struct player *pplayer, struct player *aplayer,
                                 struct Treaty *ptreaty)
{
  if (pplayer->ai_data.control && pplayer->ai->funcs.treaty_evaluate) {
    pplayer->ai->funcs.treaty_evaluate(pplayer, aplayer, ptreaty);
  }
}

/**************************************************************************
  Calls treaty_accepted function if such is set for AI player
**************************************************************************/
static void call_treaty_accepted(struct player *pplayer, struct player *aplayer,
                                 struct Treaty *ptreaty)
{
  if (pplayer->ai_data.control && pplayer->ai->funcs.treaty_accepted) {
    pplayer->ai->funcs.treaty_accepted(pplayer, aplayer, ptreaty);
  }
}

/**************************************************************************
...
**************************************************************************/
void diplhand_init(void)
{
  treaties = treaty_list_new();
}

/**************************************************************************
  Free all the resources allocated by diplhand.
**************************************************************************/
void diplhand_free(void)
{
  free_treaties();

  treaty_list_free(treaties);
  treaties = NULL;
}

/**************************************************************************
  Free all the treaties currently in treaty list.
**************************************************************************/
void free_treaties(void)
{
  /* Free memory allocated for treaties */
  treaty_list_iterate(treaties, pt) {
    clear_treaty(pt);
    free(pt);
  } treaty_list_iterate_end;

  treaty_list_clear(treaties);
}

/**************************************************************************
...
**************************************************************************/
struct Treaty *find_treaty(struct player *plr0, struct player *plr1)
{
  treaty_list_iterate(treaties, ptreaty) {
    if ((ptreaty->plr0 == plr0 && ptreaty->plr1 == plr1) ||
	(ptreaty->plr0 == plr1 && ptreaty->plr1 == plr0)) {
      return ptreaty;
    }
  } treaty_list_iterate_end;

  return NULL;
}

/**************************************************************************
pplayer clicked the accept button. If he accepted the treaty we check the
clauses. If both players have now accepted the treaty we execute the agreed
clauses.
**************************************************************************/
void handle_diplomacy_accept_treaty_req(struct player *pplayer,
					int counterpart)
{
  struct Treaty *ptreaty;
  bool *player_accept, *other_accept;
  enum dipl_reason diplcheck;
  bool worker_refresh_required = FALSE;
  struct player *pother = valid_player_by_number(counterpart);

  if (NULL == pother || pplayer == pother) {
    return;
  }

  ptreaty = find_treaty(pplayer, pother);

  if (!ptreaty) {
    return;
  }

  if (ptreaty->plr0 == pplayer) {
    player_accept = &ptreaty->accept0;
    other_accept = &ptreaty->accept1;
  } else {
    player_accept = &ptreaty->accept1;
    other_accept = &ptreaty->accept0;
  }

  if (!*player_accept) {	/* Tries to accept. */

    /* Check that player who accepts can keep what (s)he promises. */

    clause_list_iterate(ptreaty->clauses, pclause) {
      struct city *pcity = NULL;

      if (pclause->from == pplayer) {
	switch(pclause->type) {
	case CLAUSE_EMBASSY:
          /* Don't use player_has_embassy() here, because it also checks
           * for the embassy effect, and we should always be able to make
           * an embassy. */
          if (BV_ISSET(pother->embassy, player_index(pplayer))) {
            freelog(LOG_ERROR, "%s tried to give embassy to %s, who already "
                    "has an embassy",
                    player_name(pplayer),
                    player_name(pother));
            return;
          }
          break;
	case CLAUSE_ADVANCE:
          if (!player_invention_reachable(pother, pclause->value)) {
	    /* It is impossible to give a technology to a civilization that
	     * can never possess it (the client should enforce this). */
	    freelog(LOG_ERROR, "Treaty: %s can't have tech %s",
                    nation_rule_name(nation_of_player(pother)),
		    advance_name_by_player(pplayer, pclause->value));
	    notify_player(pplayer, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
                          _("The %s can't accept %s."),
                          nation_plural_for_player(pother),
			  advance_name_for_player(pplayer, pclause->value));
	    return;
          }
	  if (player_invention_state(pplayer, pclause->value) != TECH_KNOWN) {
	    freelog(LOG_ERROR,
                    "Nation %s try to give unknown tech %s to nation %s.",
		    nation_rule_name(nation_of_player(pplayer)),
		    advance_name_by_player(pplayer, pclause->value),
		    nation_rule_name(nation_of_player(pother)));
	    notify_player(pplayer, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("You don't have tech %s, you can't accept treaty."),
			  advance_name_for_player(pplayer, pclause->value));
	    return;
	  }
	  break;
	case CLAUSE_CITY:
	  pcity = game_find_city_by_number(pclause->value);
	  if (!pcity) { /* Can't find out cityname any more. */
	    notify_player(pplayer, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("City you are trying to give no longer exists, "
			    "you can't accept treaty."));
	    return;
	  }
	  if (city_owner(pcity) != pplayer) {
	    notify_player(pplayer, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("You are not owner of %s, you can't accept treaty."),
			  city_link(pcity));
	    return;
	  }
	  if (is_capital(pcity)) {
	    notify_player(pplayer, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("Your capital (%s) is requested, "
			    "you can't accept treaty."),
			  city_link(pcity));
	    return;
	  }
	  break;
	case CLAUSE_CEASEFIRE:
          diplcheck = pplayer_can_make_treaty(pplayer, pother, DS_CEASEFIRE);
          if (diplcheck != DIPL_OK) {
            return;
          }
          break;
	case CLAUSE_PEACE:
          diplcheck = pplayer_can_make_treaty(pplayer, pother, DS_PEACE);
          if (diplcheck != DIPL_OK) {
            return;
          }
          break;
	case CLAUSE_ALLIANCE:
          diplcheck = pplayer_can_make_treaty(pplayer, pother, DS_ALLIANCE);
          if (diplcheck == DIPL_ALLIANCE_PROBLEM) {
            notify_player(pplayer, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
                          _("You cannot form an alliance because you are "
                            "at war with an ally of %s."),
                          player_name(pother));
          }
          if (diplcheck != DIPL_OK) {
            return;
          }
          break;
	case CLAUSE_GOLD:
	  if (pplayer->economic.gold < pclause->value) {
	    notify_player(pplayer, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("You don't have enough gold, "
			    "you can't accept treaty."));
	    return;
	  }
	  break;
	default:
	  ; /* nothing */
	}
      }
    } clause_list_iterate_end;
  }

  *player_accept = ! *player_accept;

  dlsend_packet_diplomacy_accept_treaty(pplayer->connections,
					player_number(pother), *player_accept,
					*other_accept);
  dlsend_packet_diplomacy_accept_treaty(pother->connections,
					player_number(pplayer), *other_accept,
					*player_accept);

  if (ptreaty->accept0 && ptreaty->accept1) {
    int nclauses = clause_list_size(ptreaty->clauses);

    dlsend_packet_diplomacy_cancel_meeting(pplayer->connections,
					   player_number(pother),
					   player_number(pplayer));
    dlsend_packet_diplomacy_cancel_meeting(pother->connections,
					   player_number(pplayer),
 					   player_number(pplayer));

    notify_player(pplayer, NULL, E_DIPLOMACY,
                  FTC_SERVER_INFO, NULL,
		  PL_("A treaty containing %d clause was agreed upon.",
		      "A treaty containing %d clauses was agreed upon.",
		      nclauses),
		  nclauses);
    notify_player(pother, NULL, E_DIPLOMACY,
                  FTC_SERVER_INFO, NULL,
		  PL_("A treaty containing %d clause was agreed upon.",
		      "A treaty containing %d clauses was agreed upon.",
		      nclauses),
		  nclauses);

    /* Check that one who accepted treaty earlier still have everything
       (s)he promised to give. */

    clause_list_iterate(ptreaty->clauses, pclause) {
      struct city *pcity;
      if (pclause->from == pother) {
	switch (pclause->type) {
	case CLAUSE_CITY:
	  pcity = game_find_city_by_number(pclause->value);
	  if (!pcity) { /* Can't find out cityname any more. */
	    notify_player(pplayer, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("One of the cities the %s are giving away"
			    " is destroyed! Treaty canceled!"),
			  nation_plural_for_player(pother));
	    notify_player(pother, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("One of the cities the %s are giving away"
			    " is destroyed! Treaty canceled!"),
			  nation_plural_for_player(pother));
	    goto cleanup;
	  }
	  if (city_owner(pcity) != pother) {
	    notify_player(pplayer, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("The %s no longer control %s! "
			    "Treaty canceled!"),
			  nation_plural_for_player(pother),
			  city_name(pcity));
	    notify_player(pother, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("The %s no longer control %s! "
			    "Treaty canceled!"),
			  nation_plural_for_player(pother),
			  city_link(pcity));
	    goto cleanup;
	  }
	  if (is_capital(pcity)) {
	    notify_player(pother, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("Your capital (%s) is requested, "
			    "you can't accept treaty."),
			  city_link(pcity));
	    goto cleanup;
	  }

	  break;
	case CLAUSE_ALLIANCE:
          /* We need to recheck this way since things might have
           * changed. */
          diplcheck = pplayer_can_make_treaty(pplayer, pother, DS_ALLIANCE);
          if (diplcheck != DIPL_OK) {
            goto cleanup;
          }
          break;
  case CLAUSE_PEACE:
          diplcheck = pplayer_can_make_treaty(pplayer, pother, DS_PEACE);
          if (diplcheck != DIPL_OK) {
            goto cleanup;
          }
          break;
  case CLAUSE_CEASEFIRE:
          diplcheck = pplayer_can_make_treaty(pplayer, pother, DS_CEASEFIRE);
          if (diplcheck != DIPL_OK) {
            goto cleanup;
          }
          break;
	case CLAUSE_GOLD:
	  if (pother->economic.gold < pclause->value) {
	    notify_player(pplayer, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("The %s don't have the promised amount "
			    "of gold! Treaty canceled!"),
			  nation_plural_for_player(pother));
	    notify_player(pother, NULL, E_DIPLOMACY,
                          FTC_SERVER_INFO, NULL,
			  _("The %s don't have the promised amount "
			    "of gold! Treaty canceled!"),
			  nation_plural_for_player(pother));
	    goto cleanup;
	  }
	  break;
	default:
	  ; /* nothing */
	}
      }
    } clause_list_iterate_end;

    call_treaty_accepted(pplayer, pother, ptreaty);
    call_treaty_accepted(pother, pplayer, ptreaty);

    clause_list_iterate(ptreaty->clauses, pclause) {
      struct player *pgiver = pclause->from;
      struct player *pdest = (pplayer == pgiver) ? pother : pplayer;
      enum diplstate_type old_diplstate = 
        pgiver->diplstates[player_index(pdest)].type;

      switch (pclause->type) {
      case CLAUSE_EMBASSY:
        establish_embassy(pdest, pgiver); /* sic */
        notify_player(pgiver, NULL, E_TREATY_SHARED_VISION,
                      FTC_SERVER_INFO, NULL,
                      _("You gave an embassy to %s."),
                      player_name(pdest));
        notify_player(pdest, NULL, E_TREATY_SHARED_VISION,
                      FTC_SERVER_INFO, NULL,
                      _("%s allowed you to create an embassy!"),
                      player_name(pgiver));
        break;
      case CLAUSE_ADVANCE:
        /* It is possible that two players open the diplomacy dialog
         * and try to give us the same tech at the same time. This
         * should be handled discreetly instead of giving a core dump. */
        if (player_invention_state(pdest, pclause->value) == TECH_KNOWN) {
	  freelog(LOG_VERBOSE,
                  "Nation %s already know tech %s, that %s want to give them.",
		  nation_rule_name(nation_of_player(pdest)),
		  advance_name_by_player(pplayer, pclause->value),
		  nation_rule_name(nation_of_player(pgiver)));
          break;
        }
	notify_player(pdest, NULL, E_TECH_GAIN,
                      FTC_SERVER_INFO, NULL,
                      _("You are taught the knowledge of %s."),
                      advance_name_for_player(pdest, pclause->value));

	notify_embassies(pdest, pgiver, NULL, E_TECH_GAIN,
                         FTC_SERVER_INFO, NULL,
			 _("The %s have acquired %s from the %s."),
			 nation_plural_for_player(pdest),
			 advance_name_for_player(pdest, pclause->value),
			 nation_plural_for_player(pgiver));

	script_signal_emit("tech_researched", 3,
			   API_TYPE_TECH_TYPE, advance_by_number(pclause->value),
			   API_TYPE_PLAYER, pdest,
			   API_TYPE_STRING, "traded");
	do_dipl_cost(pdest, pclause->value);

	found_new_tech(pdest, pclause->value, FALSE, TRUE);
	break;
      case CLAUSE_GOLD:
	notify_player(pdest, NULL, E_DIPLOMACY,
                      FTC_SERVER_INFO, NULL,
		      _("You get %d gold."), pclause->value);
	pgiver->economic.gold -= pclause->value;
	pdest->economic.gold += pclause->value * (100 - game.info.diplcost) / 100;
	break;
      case CLAUSE_MAP:
	give_map_from_player_to_player(pgiver, pdest);
	notify_player(pdest, NULL, E_DIPLOMACY, FTC_SERVER_INFO, NULL,
		      /* TRANS: ... Polish worldmap. */
		      _("You receive the %s worldmap."),
		      nation_adjective_for_player(pgiver));

        worker_refresh_required = TRUE; /* See CLAUSE_VISION */
	break;
      case CLAUSE_SEAMAP:
	give_seamap_from_player_to_player(pgiver, pdest);
	notify_player(pdest, NULL, E_DIPLOMACY, FTC_SERVER_INFO, NULL,
		      /* TRANS: ... Polish seamap. */
		      _("You receive the %s seamap."),
		      nation_adjective_for_player(pgiver));

        worker_refresh_required = TRUE; /* See CLAUSE_VISION */
	break;
      case CLAUSE_CITY:
	{
	  struct city *pcity = game_find_city_by_number(pclause->value);

	  if (!pcity) {
	    freelog(LOG_ERROR,
		    "Treaty city id %d not found - skipping clause.",
		    pclause->value);
	    break;
	  }

	  notify_player(pdest, pcity->tile, E_CITY_TRANSFER,
                        FTC_SERVER_INFO, NULL,
                        _("You receive city of %s from %s."),
                        city_link(pcity), player_name(pgiver));

	  notify_player(pgiver, pcity->tile, E_CITY_LOST,
                        FTC_SERVER_INFO, NULL,
                        _("You give city of %s to %s."),
                        city_link(pcity), player_name(pdest));

	  transfer_city(pdest, pcity, -1, TRUE, TRUE, FALSE);
	  break;
	}
      case CLAUSE_CEASEFIRE:
	pgiver->diplstates[player_index(pdest)].type=DS_CEASEFIRE;
	pgiver->diplstates[player_index(pdest)].turns_left = TURNS_LEFT;
	pdest->diplstates[player_index(pgiver)].type=DS_CEASEFIRE;
	pdest->diplstates[player_index(pgiver)].turns_left = TURNS_LEFT;
	notify_player(pgiver, NULL, E_TREATY_CEASEFIRE,
                      FTC_SERVER_INFO, NULL,
                      _("You agree on a cease-fire with %s."),
                      player_name(pdest));
	notify_player(pdest, NULL, E_TREATY_CEASEFIRE,
                      FTC_SERVER_INFO, NULL,
                      _("You agree on a cease-fire with %s."),
                      player_name(pgiver));
	if (old_diplstate == DS_ALLIANCE) {
	  update_players_after_alliance_breakup(pgiver, pdest);
	}

        worker_refresh_required = TRUE;
	break;
      case CLAUSE_PEACE:
	pgiver->diplstates[player_index(pdest)].type = DS_ARMISTICE;
	pdest->diplstates[player_index(pgiver)].type = DS_ARMISTICE;
	pgiver->diplstates[player_index(pdest)].turns_left = TURNS_LEFT;
	pdest->diplstates[player_index(pgiver)].turns_left = TURNS_LEFT;
	pgiver->diplstates[player_index(pdest)].max_state = 
          MAX(DS_PEACE, pgiver->diplstates[player_index(pdest)].max_state);
	pdest->diplstates[player_index(pgiver)].max_state = 
          MAX(DS_PEACE, pdest->diplstates[player_index(pgiver)].max_state);
	notify_player(pgiver, NULL, E_TREATY_PEACE, FTC_SERVER_INFO, NULL,
		      /* TRANS: ... the Poles ... Polish territory. */
		      PL_("You agree on an armistice with the %s. In %d turn, "
			  "it will become a peace treaty. Move your "
			  "units out of %s territory.",
			  "You agree on an armistice with the %s. In %d turns, "
			  "it will become a peace treaty. Move your "
			  "units out of %s territory.",
			  TURNS_LEFT),
		      nation_plural_for_player(pdest),
		      TURNS_LEFT,
		      nation_adjective_for_player(pdest));
	notify_player(pdest, NULL, E_TREATY_PEACE, FTC_SERVER_INFO, NULL,
		      /* TRANS: ... the Poles ... Polish territory. */
		      PL_("You agree on an armistice with the %s. In %d turn, "
			  "it will become a peace treaty. Move your "
			  "units out of %s territory.",
			  "You agree on an armistice with the %s. In %d turns, "
			  "it will become a peace treaty. Move your "
			  "units out of %s territory.",
			  TURNS_LEFT),
		      nation_plural_for_player(pgiver),
		      TURNS_LEFT,
		      nation_adjective_for_player(pgiver));
	if (old_diplstate == DS_ALLIANCE) {
	  update_players_after_alliance_breakup(pgiver, pdest);
	}

        worker_refresh_required = TRUE;
	break;
      case CLAUSE_ALLIANCE:
	pgiver->diplstates[player_index(pdest)].type=DS_ALLIANCE;
	pdest->diplstates[player_index(pgiver)].type=DS_ALLIANCE;
	pgiver->diplstates[player_index(pdest)].max_state = 
          MAX(DS_ALLIANCE, pgiver->diplstates[player_index(pdest)].max_state);
	pdest->diplstates[player_index(pgiver)].max_state = 
          MAX(DS_ALLIANCE, pdest->diplstates[player_index(pgiver)].max_state);
	notify_player(pgiver, NULL, E_TREATY_ALLIANCE, FTC_SERVER_INFO, NULL,
                      _("You agree on an alliance with %s."),
                      player_name(pdest));
	notify_player(pdest, NULL, E_TREATY_ALLIANCE, FTC_SERVER_INFO, NULL,
                      _("You agree on an alliance with %s."),
                      player_name(pgiver));

        worker_refresh_required = TRUE;
	break;
      case CLAUSE_VISION:
	give_shared_vision(pgiver, pdest);
	notify_player(pgiver, NULL, E_TREATY_SHARED_VISION,
                      FTC_SERVER_INFO, NULL,
                      _("You give shared vision to %s."),
                      player_name(pdest));
	notify_player(pdest, NULL, E_TREATY_SHARED_VISION,
                      FTC_SERVER_INFO, NULL,
                      _("%s gives you shared vision."),
                      player_name(pgiver));

        /* Yes, shared vision may let us to _know_ tiles
         * within radius of our own city. */
        worker_refresh_required = TRUE;
	break;
      case CLAUSE_LAST:
        freelog(LOG_ERROR, "Received bad clause type");
        break;
      }

    } clause_list_iterate_end;

    /* In theory, we would need refresh only receiving party of
     * CLAUSE_MAP, CLAUSE_SEAMAP and CLAUSE_VISION clauses.
     * It's quite unlikely that there is such a clause going one
     * way but no clauses affecting both parties or going other
     * way. */
    if (worker_refresh_required) {
      city_map_update_all_cities_for_player(pplayer);
      city_map_update_all_cities_for_player(pother);
      sync_cities();
    }

  cleanup:
    treaty_list_unlink(treaties, ptreaty);
    clear_treaty(ptreaty);
    free(ptreaty);
    send_player_info(pplayer, NULL);
    send_player_info(pother, NULL);
  }
}

/****************************************************************************
  Create an embassy. pplayer gets an embassy with aplayer.
****************************************************************************/
void establish_embassy(struct player *pplayer, struct player *aplayer)
{
  /* Establish the embassy. */
  BV_SET(pplayer->embassy, player_index(aplayer));
  send_player_info(pplayer, pplayer);
  send_player_info(pplayer, aplayer);  /* update player dialog with embassy */
  send_player_info(aplayer, pplayer);  /* INFO_EMBASSY level info */
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_remove_clause_req(struct player *pplayer,
					int counterpart, int giver,
					enum clause_type type, int value)
{
  struct Treaty *ptreaty;
  struct player *pgiver = valid_player_by_number(giver);
  struct player *pother = valid_player_by_number(counterpart);

  if (NULL == pother || pplayer == pother || NULL == pgiver) {
    return;
  }

  if (pgiver != pplayer && pgiver != pother) {
    return;
  }
  
  ptreaty = find_treaty(pplayer, pother);

  if (ptreaty && remove_clause(ptreaty, pgiver, type, value)) {
    dlsend_packet_diplomacy_remove_clause(pplayer->connections,
					  player_number(pother), giver, type,
					  value);
    dlsend_packet_diplomacy_remove_clause(pother->connections,
					  player_number(pplayer), giver, type,
					  value);
    call_treaty_evaluate(pplayer, pother, ptreaty);
    call_treaty_evaluate(pother, pplayer, ptreaty);
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_create_clause_req(struct player *pplayer,
					int counterpart, int giver,
					enum clause_type type, int value)
{
  struct Treaty *ptreaty;
  struct player *pgiver = valid_player_by_number(giver);
  struct player *pother = valid_player_by_number(counterpart);

  if (NULL == pother || pplayer == pother || NULL == pgiver) {
    return;
  }

  if (pgiver != pplayer && pgiver != pother) {
    return;
  }

  ptreaty = find_treaty(pplayer, pother);

  if (ptreaty && add_clause(ptreaty, pgiver, type, value)) {
    /* 
     * If we are trading cities, then it is possible that the
     * dest is unaware of it's existence.  We have 2 choices,
     * forbid it, or lighten that area.  If we assume that
     * the giver knows what they are doing, then 2. is the
     * most powerful option - I'll choose that for now.
     *                           - Kris Bubendorfer
     */
    if (type == CLAUSE_CITY) {
      struct city *pcity = game_find_city_by_number(value);

      if (pcity && !map_is_known_and_seen(pcity->tile, pother, V_MAIN))
	give_citymap_from_player_to_player(pcity, pplayer, pother);
    }

    dlsend_packet_diplomacy_create_clause(pplayer->connections,
					  player_number(pother), giver, type,
					  value);
    dlsend_packet_diplomacy_create_clause(pother->connections,
					  player_number(pplayer), giver, type,
					  value);
    call_treaty_evaluate(pplayer, pother, ptreaty);
    call_treaty_evaluate(pother, pplayer, ptreaty);
  }
}

/**************************************************************************
...
**************************************************************************/
static void really_diplomacy_cancel_meeting(struct player *pplayer,
					    struct player *pother)
{
  struct Treaty *ptreaty = find_treaty(pplayer, pother);

  if (ptreaty) {
    dlsend_packet_diplomacy_cancel_meeting(pother->connections,
					   player_number(pplayer),
					   player_number(pplayer));
    notify_player(pother, NULL, E_DIPLOMACY, FTC_SERVER_INFO, NULL,
		  _("%s canceled the meeting!"), 
		  player_name(pplayer));
    /* Need to send to pplayer too, for multi-connects: */
    dlsend_packet_diplomacy_cancel_meeting(pplayer->connections,
					   player_number(pother),
					   player_number(pplayer));
    notify_player(pplayer, NULL, E_DIPLOMACY, FTC_SERVER_INFO, NULL,
		  _("Meeting with %s canceled."), 
		  player_name(pother));
    treaty_list_unlink(treaties, ptreaty);
    clear_treaty(ptreaty);
    free(ptreaty);
  }
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_cancel_meeting_req(struct player *pplayer,
					 int counterpart)
{
  struct player *pother = valid_player_by_number(counterpart);

  if (NULL == pother || pplayer == pother) {
    return;
  }

  really_diplomacy_cancel_meeting(pplayer, pother);
}

/**************************************************************************
...
**************************************************************************/
void handle_diplomacy_init_meeting_req(struct player *pplayer,
				       int counterpart)
{
  struct player *pother = valid_player_by_number(counterpart);

  if (NULL == pother || pplayer == pother) {
    return;
  }

  if (find_treaty(pplayer, pother)) {
    return;
  }

  if (get_player_bonus(pplayer, EFT_NO_DIPLOMACY)
      || get_player_bonus(pother, EFT_NO_DIPLOMACY)) {
    notify_player(pplayer, NULL, E_DIPLOMACY, FTC_SERVER_INFO, NULL,
		  _("Your diplomatic envoy was decapitated!"));
    return;
  }

  if (could_meet_with_player(pplayer, pother)) {
    struct Treaty *ptreaty;

    ptreaty = fc_malloc(sizeof(*ptreaty));
    init_treaty(ptreaty, pplayer, pother);
    treaty_list_prepend(treaties, ptreaty);

    dlsend_packet_diplomacy_init_meeting(pplayer->connections,
					 player_number(pother),
					 player_number(pplayer));
    dlsend_packet_diplomacy_init_meeting(pother->connections,
					 player_number(pplayer),
					 player_number(pplayer));
  }
}

/**************************************************************************
  Send information on any on-going diplomatic meetings for connection's
  player.  For re-connections.
**************************************************************************/
void send_diplomatic_meetings(struct connection *dest)
{
  struct player *pplayer = dest->playing;

  if (!pplayer) {
    return;
  }
  players_iterate(other) {
    struct Treaty *ptreaty = find_treaty(pplayer, other);

    if (ptreaty) {
      assert(pplayer != other);
      dsend_packet_diplomacy_init_meeting(dest, player_number(other),
                                          player_number(pplayer));
      clause_list_iterate(ptreaty->clauses, pclause) {
        dsend_packet_diplomacy_create_clause(dest, 
                                             player_number(other),
                                             player_number(pclause->from),
                                             pclause->type,
                                             pclause->value);
      } clause_list_iterate_end;
      if (ptreaty->plr0 == pplayer) {
        dsend_packet_diplomacy_accept_treaty(dest, player_number(other),
                                             ptreaty->accept0, 
                                             ptreaty->accept1);
      } else {
        dsend_packet_diplomacy_accept_treaty(dest, player_number(other),
                                             ptreaty->accept1, 
                                             ptreaty->accept0);
      }
    }
  } players_iterate_end;
}

/**************************************************************************
...
**************************************************************************/
void cancel_all_meetings(struct player *pplayer)
{
  players_iterate(pplayer2) {
    if (find_treaty(pplayer, pplayer2)) {
      really_diplomacy_cancel_meeting(pplayer, pplayer2);
    }
  } players_iterate_end;
}

/**************************************************************************
  Reject all treaties currently being negotiated
**************************************************************************/
void reject_all_treaties(struct player *pplayer)
{
  struct Treaty* treaty;
  players_iterate(pplayer2) {
    treaty = find_treaty(pplayer, pplayer2);
    if (!treaty) {
      continue;
    }
    treaty->accept0 = FALSE;
    treaty->accept1 = FALSE;
    dlsend_packet_diplomacy_accept_treaty(pplayer->connections,
					  player_number(pplayer2),
					  FALSE,
					  FALSE);
    dlsend_packet_diplomacy_accept_treaty(pplayer2->connections,
                                          player_number(pplayer),
					  FALSE,
					  FALSE);
  } players_iterate_end;
}

