/***********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Team
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

#include <stdarg.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* common */
#include "aisupport.h"
#include "city.h"
#include "diptreaty.h"
#include "events.h"
#include "game.h"
#include "packets.h"
#include "player.h"
#include "nation.h"
#include "research.h"
#include "spaceship.h"
#include "tech.h"
#include "unitlist.h"

/* server */
#include "citytools.h"
#include "diplhand.h"
#include "maphand.h"
#include "notify.h"
#include "srv_log.h"

/* server/advisors */
#include "advdata.h"
#include "advtools.h"

/* ai */
#include "aitraits.h"
#include "handicaps.h"

/* ai/default */
#include "aicity.h"
#include "aidata.h"
#include "ailog.h"
#include "aiplayer.h"
#include "aiunit.h"
#include "aitools.h"
#include "daimilitary.h"

#include "daidiplomacy.h"

#define LOG_DIPL LOG_DEBUG
#define LOG_DIPL2 LOG_DEBUG

/* One hundred thousand. Basically a number of gold that no player is
 * ever likely to have, but not so big that we get integer overflows. */
#define BIG_NUMBER 100000

/* turn this off when we don't want functions to message players */
static bool diplomacy_verbose = TRUE;

/* turns to wait after contact before taking aim for war */
#define TURNS_BEFORE_TARGET 15

static void dai_incident_war(struct player *violator, struct player *victim);
static void dai_incident_diplomat(struct player *violator, struct player *victim);
static void dai_incident_nuclear(struct player *violator, struct player *victim);
static void dai_incident_nuclear_not_target(struct player *violator,
                                           struct player *victim);
static void dai_incident_nuclear_self(struct player *violator,
				      struct player *victim);
static void dai_incident_pillage(struct player *violator, struct player *victim);
static void clear_old_treaty(struct player *pplayer, struct player *aplayer);

/******************************************************************//**
  Send a diplomatic message. Use this instead of notify directly
  because we may want to highligh/present these messages differently
  in the future.
**********************************************************************/
static void dai_diplo_notify(struct player *pplayer, const char *text, ...)
{
  if (diplomacy_verbose) {
    va_list ap;
    struct conn_list *dest = pplayer->connections;
    struct packet_chat_msg packet;

    va_start(ap, text);
    vpackage_event(&packet, NULL, E_DIPLOMACY, ftc_chat_private, text, ap);
    va_end(ap);

    lsend_packet_chat_msg(dest, &packet);
    /* Add to the event cache. */
    event_cache_add_for_player(&packet, pplayer);
  }
}

/******************************************************************//**
  This is your typical human reaction. Convert lack of love into 
  lust for gold.
**********************************************************************/
static int greed(int missing_love)
{
  if (missing_love > 0) {
    return 0;
  } else {
    /* Don't change the operation order here.
     * We do not want integer overflows */
    return -((missing_love * MAX_AI_LOVE) / 1000) *
           ((missing_love * MAX_AI_LOVE) / 1000) / 10;
  }
}

/******************************************************************//**
  Convert clause into diplomatic state
**********************************************************************/
static enum diplstate_type pact_clause_to_diplstate_type(enum clause_type type)
{
  switch (type) {
  case CLAUSE_ALLIANCE:
    return DS_ALLIANCE;
  case CLAUSE_PEACE:
    return DS_PEACE;
  case CLAUSE_CEASEFIRE:
    return DS_CEASEFIRE;
  default:
    log_error("Invalid diplomatic clause %d.", type)
    return DS_WAR;
  }
}

/******************************************************************//**
  How much is a tech worth to player measured in gold
**********************************************************************/
static int dai_goldequiv_tech(struct ai_type *ait,
                              struct player *pplayer, Tech_type_id tech)
{
  int bulbs, tech_want, worth;
  struct research *presearch = research_get(pplayer);
  enum tech_state state = research_invention_state(presearch, tech);
  struct ai_plr *plr_data = def_ai_player_data(pplayer, ait);

  if (TECH_KNOWN == state
      || !research_invention_gettable(presearch, tech,
                                      game.info.tech_trade_allow_holes)) {
    return 0;
  }
  bulbs = research_goal_bulbs_required(presearch, tech) * 3;
  tech_want = MAX(plr_data->tech_want[tech], 0) / MAX(game.info.turn, 1);
  worth = bulbs + tech_want;
  if (TECH_PREREQS_KNOWN == state) {
    worth /= 2;
  }
  return worth;
}

/******************************************************************//**
  Avoid giving pplayer's vision to non-allied player through aplayer
  (shared vision is transitive).
**********************************************************************/
static bool shared_vision_is_safe(struct player* pplayer,
                                  struct player* aplayer)
{
  if (pplayer->team && pplayer->team == aplayer->team) {
    return TRUE;
  }
  players_iterate_alive(eplayer) {
    if (eplayer == pplayer || eplayer == aplayer) {
      continue;
    }
    if (gives_shared_vision(aplayer, eplayer)) {
      enum diplstate_type ds = player_diplstate_get(pplayer, eplayer)->type;

      if (ds != DS_NO_CONTACT && ds != DS_ALLIANCE) {
        return FALSE;
      }
    }
  } players_iterate_alive_end;
  return TRUE;
}

/******************************************************************//**
  Checks if player1 can agree on ceasefire with player2
  This function should only be used for ai players
**********************************************************************/
static bool dai_players_can_agree_on_ceasefire(struct ai_type *ait,
                                               struct player* player1,
                                               struct player* player2)
{
  return (player1->ai_common.love[player_index(player2)] > - (MAX_AI_LOVE * 4 / 10)
          && dai_diplomacy_get(ait, player1, player2)->countdown == -1);
}

/******************************************************************//**
  Calculate a price of a tech.
  Note that both AI players always evaluate the tech worth symetrically
  This eases tech exchange.
  is_dangerous returns ig the giver is afraid of giving that tech
  (the taker should evaluate it normally, but giver should never give that)
**********************************************************************/
static int compute_tech_sell_price(struct ai_type *ait,
                                   struct player *giver, struct player *taker,
                                   int tech_id, bool *is_dangerous)
{
    int worth;
    
    worth = dai_goldequiv_tech(ait, taker, tech_id);
    
    *is_dangerous = FALSE;
    
    /* Share and expect being shared brotherly between allies */
    if (pplayers_allied(giver, taker)) {
      worth /= 2;
    }
    if (players_on_same_team(giver, taker)) {
      return 0;
    }

    /* Do not bother wanting a tech that we already have. */
    if (research_invention_state(research_get(taker), tech_id)
        == TECH_KNOWN) {
      return 0;
    }

    /* Calculate in tech leak to our opponents, guess 50% chance */
    players_iterate_alive(eplayer) {
      if (eplayer == giver
          || eplayer == taker
          || research_invention_state(research_get(eplayer),
                                      tech_id) == TECH_KNOWN) {
        continue;
      }

      /* Don't risk it falling into enemy hands */
      if (pplayers_allied(taker, eplayer) &&
          adv_is_player_dangerous(giver, eplayer)) {
        *is_dangerous = TRUE;
      }
      
      if (pplayers_allied(taker, eplayer) &&
          !pplayers_allied(giver, eplayer)) {
        /* Taker can enrichen his side with this tech */
        worth += dai_goldequiv_tech(ait, eplayer, tech_id) / 4;
      }
    } players_iterate_alive_end;
    return worth;
}

/******************************************************************//**
  Returns an enemy player to 'us' allied with 'them' if there is one.
**********************************************************************/
static const struct player *
get_allied_with_enemy_player(const struct player *us,
                             const struct player *them)
{
  players_iterate_alive(aplayer) {
    if (aplayer != us
        && aplayer != them
        && pplayers_allied(them, aplayer)
        && player_diplstate_get(us, aplayer)->type == DS_WAR) {
      return aplayer;
    }
  } players_iterate_alive_end;
  return NULL;
}

/******************************************************************//**
  Evaluate gold worth of a single clause in a treaty. Note that it
  sometimes matter a great deal who is giving what to whom, and
  sometimes (such as with treaties) it does not matter at all.
  ds_after means a pact offered in the same treaty or current diplomatic
  state
**********************************************************************/
static int dai_goldequiv_clause(struct ai_type *ait,
                                struct player *pplayer,
                                struct player *aplayer,
                                struct Clause *pclause,
                                bool verbose,
                                enum diplstate_type ds_after)
{
  bool close_here;
  struct ai_plr *ai;
  int worth = 0; /* worth for pplayer of what aplayer gives */
  bool give = (pplayer == pclause->from);
  struct player *giver;
  const struct player *penemy;
  struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, aplayer);
  bool is_dangerous;

  ai = dai_plr_data_get(ait, pplayer, &close_here);

  fc_assert_ret_val(pplayer != aplayer, 0);

  diplomacy_verbose = verbose;
  ds_after = MAX(ds_after, player_diplstate_get(pplayer, aplayer)->type);
  giver = pclause->from;

  switch (pclause->type) {
  case CLAUSE_ADVANCE:
    if (give) {
      worth -= compute_tech_sell_price(ait, pplayer, aplayer, pclause->value,
                                       &is_dangerous);
      if (is_dangerous) {
        worth = -BIG_NUMBER;
      }
    } else if (research_invention_state(research_get(pplayer),
                                        pclause->value) != TECH_KNOWN) {
      worth += compute_tech_sell_price(ait, aplayer, pplayer, pclause->value,
                                       &is_dangerous);

      if (game.info.tech_upkeep_style != TECH_UPKEEP_NONE) {
        /* Consider the upkeep costs! Thus, one can not get an AI player by
         * - given AI lots of techs for gold/cities etc.
         * - AI losses tech due to high upkeep. 
         * FIXME: Is there a better way for this? */
        struct research *research = research_get(pplayer);
        int limit = MAX(1, player_tech_upkeep(pplayer)
                           / research->techs_researched);

        if (pplayer->server.bulbs_last_turn < limit) {
          worth /= 2;
        }
      }
    }
    DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "%s clause worth %d",
              advance_rule_name(advance_by_number(pclause->value)), worth);
    break;

  case CLAUSE_ALLIANCE:
  case CLAUSE_PEACE:
  case CLAUSE_CEASEFIRE:
    /* Don't do anything in away mode */
    if (has_handicap(pplayer, H_AWAY)) {
      dai_diplo_notify(aplayer, _("*%s (AI)* In away mode AI can't sign such a treaty."),
                       player_name(pplayer));
      worth = -BIG_NUMBER;
      break;
    }

    /* This guy is allied to one of our enemies. Only accept
     * ceasefire. */
    if ((penemy = get_allied_with_enemy_player(pplayer, aplayer))
        && pclause->type != CLAUSE_CEASEFIRE) {
      dai_diplo_notify(aplayer, _("*%s (AI)* First break alliance with %s, %s."),
                       player_name(pplayer), player_name(penemy),
                       player_name(aplayer));
      worth = -BIG_NUMBER;
      break;
    }

    /* He was allied with an enemy at the begining of the turn. */
    if (adip->is_allied_with_enemy
        && pclause->type != CLAUSE_CEASEFIRE) {
      dai_diplo_notify(aplayer, _("*%s (AI)* I would like to see you keep your "
                                  "distance from %s for some time, %s."),
                       player_name(pplayer), player_name(adip->is_allied_with_enemy),
                       player_name(aplayer));
      worth = -BIG_NUMBER;
      break;
    }

    /* Steps of the treaty ladder */
    if (pclause->type == CLAUSE_PEACE) {
      const struct player_diplstate *ds
        = player_diplstate_get(pplayer, aplayer);

      if (!pplayers_non_attack(pplayer, aplayer)) {
        dai_diplo_notify(aplayer, _("*%s (AI)* Let us first cease hostilities, %s."),
                         player_name(pplayer),
                         player_name(aplayer));
        worth = -BIG_NUMBER;
      } else if (ds->type == DS_CEASEFIRE && ds->turns_left > 4) {
        dai_diplo_notify(aplayer, _("*%s (AI)* I wish to see you keep the current "
                                    "ceasefire for a bit longer first, %s."),
                         player_name(pplayer), 
                         player_name(aplayer));
        worth = -BIG_NUMBER;
      } else if (adip->countdown >= 0 || adip->countdown < -1) {
        worth = -BIG_NUMBER; /* but say nothing */
      } else {
        worth = greed(pplayer->ai_common.love[player_index(aplayer)]
                      - ai->diplomacy.req_love_for_peace);
      }
    } else if (pclause->type == CLAUSE_ALLIANCE) {
      if (!pplayers_in_peace(pplayer, aplayer)) {
        worth = greed(pplayer->ai_common.love[player_index(aplayer)]
                      - ai->diplomacy.req_love_for_peace);
      }
      if (adip->countdown >= 0 || adip->countdown < -1) {
        worth = -BIG_NUMBER; /* but say nothing */
      } else {
        worth += greed(pplayer->ai_common.love[player_index(aplayer)]
                       - ai->diplomacy.req_love_for_alliance);
      }
      if (pplayer->ai_common.love[player_index(aplayer)] < MAX_AI_LOVE / 10) {
        dai_diplo_notify(aplayer, _("*%s (AI)* I simply do not trust you with an "
                                    "alliance yet, %s."),
                         player_name(pplayer),
                         player_name(aplayer));
        worth = -BIG_NUMBER;
      }
      DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "ally clause worth %d", worth);
    } else {
      if (is_ai(pplayer) && is_ai(aplayer)
          && dai_players_can_agree_on_ceasefire(ait, pplayer, aplayer)) {
        worth = 0;
      } else {
        int turns = game.info.turn;

        turns -= player_diplstate_get(pplayer, aplayer)->first_contact_turn;
        if (turns < TURNS_BEFORE_TARGET) {
          worth = 0; /* show some good faith */
          break;
        } else {
          worth = greed(pplayer->ai_common.love[player_index(aplayer)]);
          DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "ceasefire worth=%d love=%d "
                    "turns=%d", worth,
                    pplayer->ai_common.love[player_index(aplayer)],
                    turns);
        }
      }
    }

    /* Let's all hold hands in one happy family! */
    if (adip->is_allied_with_ally) {
      worth /= 2;
      break;
    }

    DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "treaty clause worth %d", worth);
  break;

  case CLAUSE_GOLD:
    if (give) {
      worth -= pclause->value;
    } else {
      worth += pclause->value * (100 - game.server.diplgoldcost) / 100;
    }
    break;

  case CLAUSE_SEAMAP:
    if (!give || ds_after == DS_ALLIANCE) {
      /* Useless to us - we're omniscient! And allies get it for free! */
      worth = 0;
    } else {
      /* Very silly algorithm 1: Sea map more worth if enemy has more
         cities. Reasoning is he has more use of seamap for settling
         new areas the more cities he has already. */
      worth -= 15 * city_list_size(aplayer->cities);
      /* Don't like him? Don't give him! */
      worth = MIN(pplayer->ai_common.love[player_index(aplayer)] * 7, worth);
      /* Make maps from novice player cheap */
      if (has_handicap(pplayer, H_DIPLOMACY)) {
        worth /= 2;
      }
    }
    DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "seamap clause worth %d",
              worth);
    break;

  case CLAUSE_MAP:
    if (!give || ds_after == DS_ALLIANCE) {
      /* Useless to us - we're omniscient! And allies get it for free! */
      worth = 0;
    } else {
      /* Very silly algorithm 2: Land map more worth the more cities
         we have, since we expose all of these to the enemy. */
      worth -= 40 * MAX(city_list_size(pplayer->cities), 1);
      /* Inflate numbers if not peace */
      if (!pplayers_in_peace(pplayer, aplayer)) {
        worth *= 2;
      }
      /* Don't like him? Don't give him! */
      worth = MIN(pplayer->ai_common.love[player_index(aplayer)] * 10, worth);
      /* Make maps from novice player cheap */
      if (has_handicap(pplayer, H_DIPLOMACY)) {
        worth /= 6;
      }
    }
    DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "landmap clause worth %d",
              worth);
    break;

  case CLAUSE_CITY: {
    struct city *offer = city_list_find_number(pclause->from->cities,
                                               pclause->value);

    if (!offer || city_owner(offer) != giver) {
      /* City destroyed or taken during negotiations */
      dai_diplo_notify(aplayer, _("*%s (AI)* I do not know the city you mention."),
                       player_name(pplayer));
      worth = 0;
      DIPLO_LOG(ait, LOG_DEBUG, pplayer, aplayer, "city destroyed during negotiations");
    } else if (give) {
      /* AI must be crazy to trade away its cities */
      worth -= city_gold_worth(offer);
      if (is_capital(offer)) {
        worth = -BIG_NUMBER; /* Never! Ever! */
      } else {
        worth *= 15;
      }
      if (aplayer == offer->original) {
        /* Let them buy back their own city cheaper. */
        worth /= 2;
      }
    } else {
      worth = city_gold_worth(offer);
    }
    if (offer != NULL) {
      DIPLO_LOG(ait, LOG_DEBUG, pplayer, aplayer, "worth of %s is %d", 
                city_name_get(offer), worth);
    }
    break;
  }

  case CLAUSE_VISION:
    if (give) {
      if (ds_after == DS_ALLIANCE) {
        if (!shared_vision_is_safe(pplayer, aplayer)) {
          dai_diplo_notify(aplayer,
                           _("*%s (AI)* Sorry, sharing vision with you "
                             "is not safe."),
                           player_name(pplayer));
	  worth = -BIG_NUMBER;
	} else {
          worth = 0;
	}
      } else {
        /* so out of the question */
        worth = -BIG_NUMBER;
      }
    } else {
      worth = 0; /* We are omniscient, so... */
    }
    break;

  case CLAUSE_EMBASSY:
    if (give) {
      if (ds_after == DS_ALLIANCE) {
        worth = 0;
      } else if (ds_after == DS_PEACE) {
        worth = -5 * game.info.turn;
      } else {
        worth = MIN(-50 * game.info.turn
                    + pplayer->ai_common.love[player_index(aplayer)], 
                    -5 * game.info.turn);
      }
    } else if (game.info.tech_leakage == TECH_LEAKAGE_EMBASSIES) {
      worth = game.info.tech_leak_pct / 10;
    } else {
      worth = 0; /* We don't need no stinkin' embassy, do we? */
    }
    DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "embassy clause worth %d",
              worth);
    break;
  case CLAUSE_COUNT:
    fc_assert(pclause->type != CLAUSE_COUNT);
    break;
  } /* end of switch */

  if (close_here) {
    dai_data_phase_finished(ait, pplayer);
  }

  diplomacy_verbose = TRUE;
  return worth;
}

/******************************************************************//**
  pplayer is AI player, aplayer is the other player involved, treaty
  is the treaty being considered. It is all a question about money :-)
**********************************************************************/
void dai_treaty_evaluate(struct ai_type *ait, struct player *pplayer,
                         struct player *aplayer, struct Treaty *ptreaty)
{
  int total_balance = 0;
  bool only_gifts = TRUE;
  enum diplstate_type ds_after =
    player_diplstate_get(pplayer, aplayer)->type;
  int given_cities = 0;

  clause_list_iterate(ptreaty->clauses, pclause) {
    if (is_pact_clause(pclause->type)) {
      ds_after = pact_clause_to_diplstate_type(pclause->type);
    }
    if (pclause->type == CLAUSE_CITY && pclause->from == pplayer) {
	given_cities++;
    }    
  } clause_list_iterate_end;
  
  /* Evaluate clauses */
  clause_list_iterate(ptreaty->clauses, pclause) {
    const struct research *presearch = research_get(pplayer);

    total_balance +=
      dai_goldequiv_clause(ait, pplayer, aplayer, pclause, TRUE, ds_after);

    if (pclause->type != CLAUSE_GOLD && pclause->type != CLAUSE_MAP
        && pclause->type != CLAUSE_SEAMAP && pclause->type != CLAUSE_VISION
        && (pclause->type != CLAUSE_ADVANCE 
            || game.info.tech_cost_style != TECH_COST_CIV1CIV2
            || pclause->value == research_get(pplayer)->tech_goal
            || pclause->value == research_get(pplayer)->researching
            || research_goal_tech_req(presearch, presearch->tech_goal,
                                      pclause->value))) {
      /* We accept the above list of clauses as gifts, even if we are
       * at war. We do not accept tech or cities since these can be used
       * against us, unless we know that we want this tech anyway, or
       * it doesn't matter due to tech cost style. */
      only_gifts = FALSE;
    }
  } clause_list_iterate_end;

  /* If we are at war, and no peace is offered, then no deal, unless
   * it is just gifts, in which case we gratefully accept. */
  if (ds_after == DS_WAR && !only_gifts) {
    DIPLO_LOG(ait, LOG_DIPL2, pplayer, aplayer, "no peace offered, must refuse");
    return;
  }

  if (given_cities > 0
      && city_list_size(pplayer->cities) - given_cities <= 2) {
    /* always keep at least two cities */
    DIPLO_LOG(ait, LOG_DIPL2, pplayer, aplayer, "cannot give last cities");
    return;
  }

  /* Accept if balance is good */
  if (total_balance >= 0) {
    handle_diplomacy_accept_treaty_req(pplayer, player_number(aplayer));
    DIPLO_LOG(ait, LOG_DIPL2, pplayer, aplayer, "balance was good: %d", 
              total_balance);
  } else {
    /* AI complains about the treaty which was proposed, unless the AI
     * made the proposal. */
    if (pplayer != ptreaty->plr0) {
      dai_diplo_notify(aplayer, _("*%s (AI)* This deal was not very good for us, %s!"),
                       player_name(pplayer),
                       player_name(aplayer));
    }
    DIPLO_LOG(ait, LOG_DIPL2, pplayer, aplayer, "balance was bad: %d", 
              total_balance);
  }
}

/******************************************************************//**
  Comments to player from AI on clauses being agreed on. Does not
  alter any state.
**********************************************************************/
static void dai_treaty_react(struct ai_type *ait,
                             struct player *pplayer,
                             struct player *aplayer,
                             struct Clause *pclause)
{
  struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, aplayer);

  switch (pclause->type) {
    case CLAUSE_ALLIANCE:
      if (adip->is_allied_with_ally) {
        dai_diplo_notify(aplayer, _("*%s (AI)* Welcome into our alliance %s!"),
                         player_name(pplayer),
                         player_name(aplayer));
      } else {
        dai_diplo_notify(aplayer, _("*%s (AI)* Yes, may we forever stand united, %s."),
                         player_name(pplayer),
                         player_name(aplayer));
      }
      DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "become allies");
      break;
    case CLAUSE_PEACE:
      dai_diplo_notify(aplayer, _("*%s (AI)* Yes, peace in our time!"),
                       player_name(pplayer));
      DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "sign peace treaty");
      break;
    case CLAUSE_CEASEFIRE:
      dai_diplo_notify(aplayer, _("*%s (AI)* Agreed. No more hostilities, %s."),
                       player_name(pplayer),
                       player_name(aplayer));
      DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "sign ceasefire");
      break;
    default:
      break;
  }
}

/******************************************************************//**
  This function is called when a treaty has been concluded, to deal
  with followup issues like comments and relationship (love) changes.

  pplayer is AI player, aplayer is the other player involved, ptreaty
  is the treaty accepted.
**********************************************************************/
void dai_treaty_accepted(struct ai_type *ait, struct player *pplayer,
                         struct player *aplayer, struct Treaty *ptreaty)
{
  bool close_here;
  struct ai_plr *ai;
  int total_balance = 0;
  bool gift = TRUE;
  enum diplstate_type ds_after =
    player_diplstate_get(pplayer, aplayer)->type;

  ai = dai_plr_data_get(ait, pplayer, &close_here);

  fc_assert_ret(pplayer != aplayer);

  clause_list_iterate(ptreaty->clauses, pclause) {
    if (is_pact_clause(pclause->type)) {
      ds_after = pact_clause_to_diplstate_type(pclause->type);
    }
  } clause_list_iterate_end;

  /* Evaluate clauses */
  clause_list_iterate(ptreaty->clauses, pclause) {
    int balance =
      dai_goldequiv_clause(ait, pplayer, aplayer, pclause, TRUE, ds_after);

    total_balance += balance;
    gift = (gift && (balance >= 0));
    dai_treaty_react(ait, pplayer, aplayer, pclause);
    if (is_pact_clause(pclause->type)
        && dai_diplomacy_get(ait, pplayer, aplayer)->countdown != -1) {
      /* Cancel a countdown towards war if we just agreed to peace... */
      DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "countdown nullified");
      dai_diplomacy_get(ait, pplayer, aplayer)->countdown = -1;
    }
  } clause_list_iterate_end;

  /* Rather arbitrary algorithm to increase our love for a player if
   * he or she offers us gifts. It is only a gift if _all_ the clauses
   * are beneficial to us. */
  if (total_balance > 0 && gift) {
    int i = total_balance / ((city_list_size(pplayer->cities) * 10) + 1);

    i = MIN(i, ai->diplomacy.love_incr * 150) * 10;
    pplayer->ai_common.love[player_index(aplayer)] += i;
    DIPLO_LOG(ait, LOG_DIPL2, pplayer, aplayer, "gift increased love by %d", i);
  }

  if (close_here) {
    dai_data_phase_finished(ait, pplayer);
  }
}

/******************************************************************//**
  Calculate our desire to go to war against aplayer.  We want to
  attack a player that is easy to beat and will yield a nice profit.

  This function is full of hardcoded constants by necessity.  They are
  not #defines since they are not used anywhere else.
**********************************************************************/
static int dai_war_desire(struct ai_type *ait, struct player *pplayer,
                          struct player *target)
{
  struct ai_plr *ai = dai_plr_data_get(ait, pplayer, NULL);
  struct adv_data *adv = adv_data_get(pplayer, NULL);
  int want = 0, fear = 0, distance = 0, settlers = 0, cities = 0;
  struct player_spaceship *ship = &target->spaceship;

  city_list_iterate(target->cities, pcity) {
    want += 100; /* base city want */
    want += city_size_get(pcity) * 20;
    want += pcity->surplus[O_SHIELD] * 8;
    want += pcity->surplus[O_TRADE] * 6;

    /* FIXME: This might be buggy if it ignores unmet UnitClass reqs. */
    fear += get_city_bonus(pcity, EFT_DEFEND_BONUS);

    city_built_iterate(pcity, pimprove) {
      int cost = impr_build_shield_cost(pcity, pimprove);

      want += cost;
      if (improvement_obsolete(pplayer, pimprove, pcity)) {
        continue;
      }
      if (is_great_wonder(pimprove)) {
        want += cost * 2;
      } else if (is_small_wonder(pimprove)) {
        want += cost;
      }
    } city_built_iterate_end;
  } city_list_iterate_end;
  unit_list_iterate(target->units, punit) {
    struct unit_type *ptype = unit_type_get(punit);

    fear += ATTACK_POWER(ptype);

    /* Fear enemy expansionism */
    if (unit_is_cityfounder(punit)) {
      want += 100;
    }
  } unit_list_iterate_end;
  unit_list_iterate(pplayer->units, punit) {
    struct unit_type *ptype = unit_type_get(punit);

    fear -= ATTACK_POWER(ptype) / 2;

    /* Our own expansionism reduces want for war */
    if (unit_is_cityfounder(punit)) {
      want -= 200;
      settlers++;
    }
  } unit_list_iterate_end;
  city_list_iterate(pplayer->cities, pcity) {
    if (VUT_UTYPE == pcity->production.kind
        && utype_is_cityfounder(pcity->production.value.utype)) {
      want -= 150;
      settlers++;
    }
    cities++;
  } city_list_iterate_end;

  /* Modify by settler/cities ratio to prevent early wars when
   * we should be expanding. This will eliminate want if we 
   * produce settlers in all cities (ie full expansion). */
  want -= abs(want) / MAX(cities - settlers, 1);

  /* Calculate average distances to other player's empire. */
  distance = player_distance_to_player(pplayer, target);
  dai_diplomacy_get(ait, pplayer, target)->distance = distance;

  /* Worry a bit if the other player has extreme amounts of wealth
   * that can be used in cities to quickly buy an army. */
  fear += (target->economic.gold / 5000) * city_list_size(target->cities);

  /* Tech lead is worrisome. FIXME: Only consider 'military' techs. */
  fear += MAX(research_get(target)->techs_researched
              - research_get(pplayer)->techs_researched, 0) * 100;

  /* Spacerace loss we will not allow! */
  if (ship->state >= SSHIP_STARTED) {
    want *= 2;
  }
  if (adv->dipl.spacerace_leader == target) {
    ai->diplomacy.strategy = WIN_CAPITAL;
    return BIG_NUMBER; /* do NOT amortize this number! */
  }

  /* Modify by which treaties we would break to other players, and what
   * excuses we have to do so. FIXME: We only consider immediate
   * allies, but we might trigger a wider chain reaction. */
  players_iterate_alive(eplayer) {
    bool cancel_excuse =
      player_diplstate_get(pplayer, eplayer)->has_reason_to_cancel != 0;
    enum diplstate_type ds = player_diplstate_get(pplayer, eplayer)->type;

    if (eplayer == pplayer) {
      continue;
    }

    /* Remember: pplayers_allied() returns true when target == eplayer */
    if (!cancel_excuse && pplayers_allied(target, eplayer)) {
      if (ds == DS_ARMISTICE) {
        want -= abs(want) / 10; /* 10% off */
      } else if (ds == DS_CEASEFIRE) {
        want -= abs(want) / 7; /* 15% off */
      } else if (ds == DS_PEACE) {
        want -= abs(want) / 5; /* 20% off */
      } else if (ds == DS_ALLIANCE) {
        want -= abs(want) / 3; /* 33% off */
      }
    }
  } players_iterate_alive_end;

  /* Modify by love. Increase the divisor to make ai go to war earlier */
  want -= MAX(0, want * pplayer->ai_common.love[player_index(target)] 
                 / (2 * MAX_AI_LOVE));

  /* Make novice AI more peaceful with human players */
  if (has_handicap(pplayer, H_DIPLOMACY) && is_human(target)) {
    want /= 2;
  }

  /* Amortize by distance */
  want = amortize(want, distance);

  if (pplayers_allied(pplayer, target)) {
    want /= 4;
  }

  DIPLO_LOG(ait, LOG_DEBUG, pplayer, target, "War want %d, war fear %d",
            want, fear);
  return (want - fear);
}

/******************************************************************//**
  Suggest a treaty.
  pplayer is the (AI) player suggesting the treaty.
  If to_pplayer, then aplayer is giver in the clause, else pplayer is.
**********************************************************************/
static void dai_diplomacy_suggest(struct player *pplayer, 
				  struct player *aplayer,
				  enum clause_type what,
                                  bool to_pplayer,
				  int value)
{
  if (!could_meet_with_player(pplayer, aplayer)) {
    log_base(LOG_DIPL2, "%s tries to do diplomacy to %s without contact",
             player_name(pplayer), player_name(aplayer));
    return;
  }

  handle_diplomacy_init_meeting_req(pplayer, player_number(aplayer));
  handle_diplomacy_create_clause_req(pplayer, player_number(aplayer),
                                     player_number(to_pplayer ? aplayer
                                                              : pplayer),
                                     what, value);
}

/******************************************************************//**
  What to do when we first meet. pplayer is the AI player.
**********************************************************************/
void dai_diplomacy_first_contact(struct ai_type *ait, struct player *pplayer,
                                 struct player *aplayer)
{
  bool wants_ceasefire = FALSE;

  /* Randomize initial love */
  pplayer->ai_common.love[player_index(aplayer)] += 2 - fc_rand(5);

  if (is_ai(pplayer)
      && player_diplstate_get(pplayer, aplayer)->type == DS_WAR
      && could_meet_with_player(pplayer, aplayer)) {
    if (has_handicap(pplayer, H_CEASEFIRE)) {
      fc_assert(!has_handicap(pplayer, H_AWAY));
      wants_ceasefire = TRUE;
    } else if (!has_handicap(pplayer, H_AWAY)) {
      struct Clause clause;

      clause.from = pplayer;
      clause.value = 0;
      clause.type = CLAUSE_CEASEFIRE;

      if (dai_goldequiv_clause(ait, pplayer, aplayer, &clause,
                               FALSE, DS_CEASEFIRE) > 0) {
        wants_ceasefire = TRUE;
      }
    }
  }

  if (wants_ceasefire) {
    dai_diplo_notify(aplayer,
                     _("*%s (AI)* Greetings %s! May we suggest a ceasefire "
                       "while we get to know each other better?"),
                     player_name(pplayer),
                     player_name(aplayer));
    clear_old_treaty(pplayer, aplayer);
    dai_diplomacy_suggest(pplayer, aplayer, CLAUSE_CEASEFIRE, FALSE, 0);
  } else {
    dai_diplo_notify(aplayer,
                     _("*%s (AI)* I found you %s! Now make it worth my "
                       "letting you live, or be crushed."),
                     player_name(pplayer),
                     player_name(aplayer));
  }
}

/******************************************************************//**
  Calculate our diplomatic predispositions here. Don't do anything.

  Only ever called for AI players and never for barbarians.

  This is called at the start of a new AI phase.  It's not called when
  a game is loaded.  So everything calculated here should be put into
  the savegame.
**********************************************************************/
void dai_diplomacy_begin_new_phase(struct ai_type *ait, struct player *pplayer)
{
  struct ai_plr *ai = dai_plr_data_get(ait, pplayer, NULL);
  struct adv_data *adv = adv_data_get(pplayer, NULL);
  int war_desire[player_slot_count()];
  int best_desire = 0;
  struct player *best_target = NULL;

  fc_assert_ret(is_ai(pplayer));

  if (!pplayer->is_alive) {
    return; /* duh */
  }

  memset(war_desire, 0, sizeof(war_desire));

  /* Calculate our desires, and find desired war target */
  players_iterate_alive(aplayer) {
    /* We don't hate ourselves, those we don't know or team members. */
    if (aplayer == pplayer
        || NEVER_MET(pplayer, aplayer)
        || players_on_same_team(pplayer, aplayer)) {
      continue;
    }
    war_desire[player_index(aplayer)] = dai_war_desire(ait, pplayer, aplayer);
    if (war_desire[player_index(aplayer)] > best_desire) {
      best_desire = war_desire[player_index(aplayer)];
      best_target = aplayer;
    }
  } players_iterate_alive_end;

  /* Time to make love. If we've been wronged, hold off that love
   * for a while. Also, cool our head each turn with love_coeff. */
  players_iterate_alive(aplayer) {
    struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, aplayer);
    int amount = 0;

    if (pplayer == aplayer) {
      continue;
    }

    if ((pplayers_non_attack(pplayer, aplayer)
         || pplayers_allied(pplayer, aplayer))
        && player_diplstate_get(pplayer, aplayer)->has_reason_to_cancel == 0
        && adip->countdown == -1
        && !adip->is_allied_with_enemy
        && !adip->at_war_with_ally
        && aplayer != best_target
        && adip->ally_patience >= 0) {
      amount += ai->diplomacy.love_incr / 2;
      if (pplayers_allied(pplayer, aplayer)) {
        amount += ai->diplomacy.love_incr / 3;
      }
      /* Increase love by each enemy he is at war with */
      players_iterate(eplayer) {
        if (WAR(eplayer, aplayer) && WAR(pplayer, eplayer)) {
          amount += ai->diplomacy.love_incr / 4;
        }
      } players_iterate_end;
      pplayer->ai_common.love[player_index(aplayer)] += amount;
      DIPLO_LOG(ait, LOG_DEBUG, pplayer, aplayer, "Increased love by %d", amount);
    } else if (WAR(pplayer, aplayer)) {
      amount -= ai->diplomacy.love_incr / 2;
      pplayer->ai_common.love[player_index(aplayer)] += amount;
      DIPLO_LOG(ait, LOG_DEBUG, pplayer, aplayer, "%d love lost to war", amount);
    } else if (player_diplstate_get(pplayer, aplayer)->has_reason_to_cancel
               != 0) {
      /* Provoked in time of peace */
      if (pplayer->ai_common.love[player_index(aplayer)] > 0) {
        amount -= pplayer->ai_common.love[player_index(aplayer)] / 2;
      }
      amount -= ai->diplomacy.love_incr * 6;
      pplayer->ai_common.love[player_index(aplayer)] += amount;
      DIPLO_LOG(ait, LOG_DEBUG, pplayer, aplayer, "Provoked! %d love lost!",
                amount);
    }
    if (pplayer->ai_common.love[player_index(aplayer)] > MAX_AI_LOVE * 8 / 10
        && !pplayers_allied(pplayer, aplayer)) {
      int love_change = ai->diplomacy.love_incr / 3;

      /* Upper levels of AI trust and love is reserved for allies. */
      pplayer->ai_common.love[player_index(aplayer)] -= love_change;
      DIPLO_LOG(ait, LOG_DEBUG, pplayer, aplayer, "%d love lost from excess",
                love_change);
    }
    amount = 0;

    /* Reduce love due to units in our territory.
     * AI is so naive, that we have to count it even if players are allied */
    amount -= MIN(player_in_territory(pplayer, aplayer) * (MAX_AI_LOVE / 200),
                  ai->diplomacy.love_incr 
                  * ((adip->is_allied_with_enemy != NULL) + 1));
    pplayer->ai_common.love[player_index(aplayer)] += amount;
    if (amount != 0) {
      DIPLO_LOG(ait, LOG_DEBUG, pplayer, aplayer, "%d love lost due to units inside "
                "our borders", amount);
    }

    /* Increase the love if aplayer has got a building that makes 
     * us love him more. Typically it's Eiffel Tower */
    if (!NEVER_MET(pplayer, aplayer)) {
      pplayer->ai_common.love[player_index(aplayer)] +=
        get_player_bonus(aplayer, EFT_GAIN_AI_LOVE) * MAX_AI_LOVE / 1000;
    }
  } players_iterate_alive_end;

  /* Can we win by space race? */
  if (adv->dipl.spacerace_leader == pplayer) {
    log_base(LOG_DIPL2, "%s going for space race victory!",
             player_name(pplayer));
    ai->diplomacy.strategy = WIN_SPACE; /* Yes! */
  } else {
    if (ai->diplomacy.strategy == WIN_SPACE) {
       ai->diplomacy.strategy = WIN_OPEN;
    }
  }

  players_iterate(aplayer) {
    int *love = &pplayer->ai_common.love[player_index(aplayer)];

    if (aplayer == best_target && best_desire > 0) {
      int reduction = MIN(best_desire, MAX_AI_LOVE / 20);

      *love -= reduction;
      DIPLO_LOG(ait, LOG_DEBUG, pplayer, aplayer, "Wants war, reducing "
                "love by %d ", reduction);
    }

    /* Edge love towards zero */
    *love -= *love * ((double)ai->diplomacy.love_coeff / 100.0);

    /* ai love should always be in range [-MAX_AI_LOVE..MAX_AI_LOVE] */
    *love = MAX(-MAX_AI_LOVE, MIN(MAX_AI_LOVE, *love));
  } players_iterate_end;
}

/******************************************************************//**
  Find two techs that can be exchanged and suggest that
**********************************************************************/
static void suggest_tech_exchange(struct ai_type *ait,
                                  struct player *player1,
                                  struct player *player2)
{
  struct research *presearch1 = research_get(player1);
  struct research *presearch2 = research_get(player2);
  int worth[advance_count()];
  bool is_dangerous;
    
  worth[A_NONE] = 0;

  advance_index_iterate(A_FIRST, tech) {
    if (research_invention_state(presearch1, tech)
        == TECH_KNOWN) {
      if (research_invention_state(presearch2, tech) != TECH_KNOWN
          && research_invention_gettable(presearch2, tech, game.info.tech_trade_allow_holes)) {
        worth[tech] = -compute_tech_sell_price(ait, player1, player2, tech,
	                                       &is_dangerous);
	if (is_dangerous) {
	  /* don't try to exchange */
	  worth[tech] = 0;
	}
      } else {
        worth[tech] = 0;
      }
    } else {
      if (research_invention_state(presearch2, tech) == TECH_KNOWN
          && research_invention_gettable(presearch1, tech,
                                         game.info.tech_trade_allow_holes)) {
        worth[tech] = compute_tech_sell_price(ait, player2, player1, tech,
	                                      &is_dangerous);
	if (is_dangerous) {
	  /* don't try to exchange */
	  worth[tech] = 0;
	}
      } else {
        worth[tech] = 0;
      }
    }
  } advance_index_iterate_end;
    
  advance_index_iterate(A_FIRST, tech) {
    if (worth[tech] <= 0) {
      continue;
    }
    advance_index_iterate(A_FIRST, tech2) {
      int diff;

      if (worth[tech2] >= 0) {
        continue;
      }
      /* tech2 is given by player1, tech is given by player2 */
      diff = worth[tech] + worth[tech2];
      if ((diff > 0 && player1->economic.gold >= diff)
          || (diff < 0 && player2->economic.gold >= -diff)
	  || diff == 0) {
        dai_diplomacy_suggest(player1, player2, CLAUSE_ADVANCE, FALSE, tech2);
	dai_diplomacy_suggest(player1, player2, CLAUSE_ADVANCE, TRUE, tech);
	if (diff > 0) {
          dai_diplomacy_suggest(player1, player2, CLAUSE_GOLD, FALSE, diff);
	} else if (diff < 0) {
          dai_diplomacy_suggest(player1, player2, CLAUSE_GOLD, TRUE, -diff);
	}
	return;
      }
    } advance_index_iterate_end;
  } advance_index_iterate_end;
}

/******************************************************************//**
  Clear old clauses from the treaty between players
**********************************************************************/
static void clear_old_treaty(struct player *pplayer, struct player *aplayer)
{
  struct Treaty *old_treaty = find_treaty(pplayer, aplayer);

  if (old_treaty != NULL) {
    /* Remove existing clauses */
    clause_list_iterate(old_treaty->clauses, pclause) {
      dlsend_packet_diplomacy_remove_clause(aplayer->connections,
                                            player_number(pplayer),
                                            player_number(pclause->from),
                                            pclause->type, pclause->value);
      free(pclause);
    } clause_list_iterate_end;
    clause_list_destroy(old_treaty->clauses);
    old_treaty->clauses = clause_list_new();
  }
}

/******************************************************************//**
  Offer techs and stuff to other player and ask for techs we need.
**********************************************************************/
static void dai_share(struct ai_type *ait, struct player *pplayer,
                      struct player *aplayer)
{
  struct research *presearch = research_get(pplayer);
  struct research *aresearch = research_get(aplayer);
  bool gives_vision;

  clear_old_treaty(pplayer, aplayer);

  /* Only share techs with team mates */
  if (presearch != aresearch
      && players_on_same_team(pplayer, aplayer)) {
    advance_index_iterate(A_FIRST, idx) {
      if (research_invention_state(presearch, idx) != TECH_KNOWN
          && research_invention_state(aresearch, idx) == TECH_KNOWN
          && research_invention_gettable(presearch, idx,
                                         game.info.tech_trade_allow_holes)) {
       dai_diplomacy_suggest(pplayer, aplayer, CLAUSE_ADVANCE, TRUE, idx);
      } else if (research_invention_state(presearch, idx) == TECH_KNOWN
                 && research_invention_state(aresearch, idx) != TECH_KNOWN
                 && research_invention_gettable(aresearch, idx,
                                                game.info.tech_trade_allow_holes)) {
        dai_diplomacy_suggest(pplayer, aplayer, CLAUSE_ADVANCE, FALSE, idx);
      }
    } advance_index_iterate_end;
  }

  /* Only give shared vision if safe. Only ask for shared vision if fair. */
  gives_vision = gives_shared_vision(pplayer, aplayer);
  if (!gives_vision
      && shared_vision_is_safe(pplayer, aplayer)) {
    dai_diplomacy_suggest(pplayer, aplayer, CLAUSE_VISION, FALSE, 0);
    gives_vision = TRUE;
  }
  if (gives_vision
      && !gives_shared_vision(aplayer, pplayer)
      && (is_human(aplayer)
          || shared_vision_is_safe(aplayer, pplayer))) {
    dai_diplomacy_suggest(pplayer, aplayer, CLAUSE_VISION, TRUE, 0);
  }

  if (!player_has_embassy(pplayer, aplayer)) {
    dai_diplomacy_suggest(pplayer, aplayer, CLAUSE_EMBASSY, TRUE, 0);
  }
  if (!player_has_embassy(aplayer, pplayer)) {
    dai_diplomacy_suggest(pplayer, aplayer, CLAUSE_EMBASSY, FALSE, 0);
  }

  if (!has_handicap(pplayer, H_DIPLOMACY) || is_human(aplayer)) {
    suggest_tech_exchange(ait, pplayer, aplayer);
  }
}

/******************************************************************//**
  Go to war.  Explain to target why we did it, and set countdown to
  some negative value to make us a bit stubborn to avoid immediate
  reversal to ceasefire.
**********************************************************************/
static void dai_go_to_war(struct ai_type *ait, struct player *pplayer,
                          struct player *target, enum war_reason reason)
{
  struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, target);

  fc_assert_ret(pplayer != target);
  fc_assert_ret(target->is_alive);

  switch (reason) {
  case DAI_WR_SPACE:
    dai_diplo_notify(target, _("*%s (AI)* Space will never be yours. "),
                     player_name(pplayer));
    adip->countdown = -10;
    break;
  case DAI_WR_BEHAVIOUR:
    dai_diplo_notify(target,
                     _("*%s (AI)* I have tolerated your vicious antics "
                       "long enough! To war!"),
                     player_name(pplayer));
    adip->countdown = -20;
    break;
  case DAI_WR_NONE:
    dai_diplo_notify(target, _("*%s (AI)* Peace in ... some other time."),
                     player_name(pplayer));
    adip->countdown = -10;
    break;
  case DAI_WR_HATRED:
    dai_diplo_notify(target,
                     _("*%s (AI)* Finally I get around to you! Did "
                       "you really think you could get away with your crimes?"),
                     player_name(pplayer));
    adip->countdown = -20;
    break;
  case DAI_WR_EXCUSE:
    dai_diplo_notify(target,
                     _("*%s (AI)* Your covert hostilities brought "
                       "this war upon you!"),
                     player_name(pplayer));
    adip->countdown = -20;
    break;
  case DAI_WR_ALLIANCE:
    if (adip->at_war_with_ally) {
      dai_diplo_notify(target,
                       _("*%s (AI)* Your aggression against %s was "
                         "your last mistake!"),
                       player_name(pplayer),
                       player_name(adip->at_war_with_ally));
      adip->countdown = -3;
    } else {
      /* Ooops! */
      DIPLO_LOG(ait, LOG_DIPL, pplayer, target, "Wanted to declare war "
                "for his war against an ally, but can no longer find "
                "this ally!  War declaration aborted.");
      adip->countdown = -1;
      return;
    }
    break;
  }

  fc_assert_ret(adip->countdown < 0);

  if (gives_shared_vision(pplayer, target)) {
    remove_shared_vision(pplayer, target);
  }

  /* Check for Senate obstruction.  If so, dissolve it. */
  if (pplayer_can_cancel_treaty(pplayer, target) == DIPL_SENATE_BLOCKING) {
    handle_player_change_government(pplayer, 
                                    game.info.government_during_revolution_id);
  }

  /* This will take us straight to war. */
  while (player_diplstate_get(pplayer, target)->type != DS_WAR) {
    if (pplayer_can_cancel_treaty(pplayer, target) != DIPL_OK) {
      DIPLO_LOG(ait, LOG_ERROR, pplayer, target, "Wanted to cancel treaty but "
                "was unable to.");
      return;
    }
    handle_diplomacy_cancel_pact(pplayer, player_number(target), clause_type_invalid());
  }

  /* Throw a tantrum */
  if (pplayer->ai_common.love[player_index(target)] > 0) {
    pplayer->ai_common.love[player_index(target)] = -1;
  }
  pplayer->ai_common.love[player_index(target)] -= MAX_AI_LOVE / 8;

  fc_assert(!gives_shared_vision(pplayer, target));
  DIPLO_LOG(ait, LOG_DIPL, pplayer, target, "war declared");
}

/******************************************************************//**
  Do diplomatic actions. Must be called only after calculate function
  above has been run for _all_ AI players.

  Only ever called for AI players and never for barbarians.

  When the adip->coundown variable is set to a positive value, it
  counts down to a declaration of war. When it is set to a value
  smaller than -1, it is a countup towards a "neutral" stance of -1,
  in which time the AI will refuse to make treaties. This is to make
  the AI more stubborn.
**********************************************************************/
void static war_countdown(struct ai_type *ait, struct player *pplayer,
                          struct player *target,
                          int countdown, enum war_reason reason)
{
  struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, target);

  DIPLO_LOG(ait, LOG_DIPL, pplayer, target, "countdown to war in %d", countdown);

  /* Otherwise we're resetting an existing countdown, which is very bad */
  fc_assert_ret(adip->countdown == -1);

  adip->countdown = countdown;
  adip->war_reason = reason;

  players_iterate_alive(ally) {
    if (!pplayers_allied(pplayer, ally) 
        || ally == target
        || NEVER_MET(pplayer, ally)) {
      continue;
    }

    switch (reason) {
    case DAI_WR_SPACE:
      dai_diplo_notify(ally,
                       PL_("*%s (AI)* We will be launching an all-out war "
                           "against %s in %d turn to stop the spaceship "
                           "launch.",
                           "*%s (AI)* We will be launching an all-out war "
                           "against %s in %d turns to stop the spaceship "
                           "launch.",
                           countdown),
                       player_name(pplayer),
                       player_name(target),
                       countdown);
      dai_diplo_notify(ally,
                       _("*%s (AI)* Your aid in this matter will be expected. "
                         "Long live our glorious alliance!"),
                       player_name(pplayer));
      break;
    case DAI_WR_BEHAVIOUR:
    case DAI_WR_EXCUSE:
      dai_diplo_notify(ally,
                       PL_("*%s (AI)* %s has grossly violated his treaties "
                           "with us for own gain.  We will answer in force in "
                           "%d turn and expect you to honor your alliance "
                           "with us and do likewise!",
                           "*%s (AI)* %s has grossly violated his treaties "
                           "with us for own gain.  We will answer in force in "
                           "%d turns and expect you to honor your alliance "
                           "with us and do likewise!", countdown),
                       player_name(pplayer),
                       player_name(target),
                       countdown);
      break;
    case DAI_WR_NONE:
      dai_diplo_notify(ally,
                       PL_("*%s (AI)* We intend to pillage and plunder the rich "
                           "civilization of %s. We declare war in %d turn.",
                           "*%s (AI)* We intend to pillage and plunder the rich "
                           "civilization of %s. We declare war in %d turns.",
                           countdown), 
                       player_name(pplayer),
                       player_name(target),
                       countdown);
      dai_diplo_notify(ally,
                       _("*%s (AI)* If you want a piece of the loot, feel "
                         "free to join in the action!"),
                       player_name(pplayer));
      break;
    case DAI_WR_HATRED:
      dai_diplo_notify(ally,
                       PL_("*%s (AI)* We have had it with %s. Let us tear this "
                           "pathetic civilization apart. We declare war in "
                           "%d turn.",
                           "*%s (AI)* We have had it with %s. Let us tear this "
                           "pathetic civilization apart. We declare war in "
                           "%d turns.",
                           countdown),
                       player_name(pplayer),
                       player_name(target),
                       countdown);
      dai_diplo_notify(ally,
                       _("*%s (AI)* As our glorious allies, we expect your "
                         "help in this war."),
                       player_name(pplayer));
      break;
    case DAI_WR_ALLIANCE:
      if (WAR(ally, target)) {
        dai_diplo_notify(ally,
                         PL_("*%s (AI)* We will honor our alliance and declare "
                             "war on %s in %d turn.  Hold on - we are coming!",
                             "*%s (AI)* We will honor our alliance and declare "
                             "war on %s in %d turns.  Hold on - we are coming!",
                             countdown),
                         player_name(pplayer),
                         player_name(target),
                         countdown);
      } else if (adip->at_war_with_ally) {
        dai_diplo_notify(ally,
                         PL_("*%s (AI)* We will honor our alliance with %s and "
                             "declare war on %s in %d turns.  We expect you to "
                             "do likewise.",
                             "*%s (AI)* We will honor our alliance with %s and "
                             "declare war on %s in %d turns.  We expect you to "
                             "do likewise.",
                             countdown),
                         player_name(pplayer), 
                         player_name(adip->at_war_with_ally),
                         player_name(target),
                         countdown);
      } else {
        fc_assert(FALSE); /* Huh? */
      }
      break;
    }
  } players_iterate_alive_end;
}

/******************************************************************//**
  Do diplomatic actions. Must be called only after calculate function
  above has been run for _all_ AI players.

  Only ever called for AI players.
**********************************************************************/
void dai_diplomacy_actions(struct ai_type *ait, struct player *pplayer)
{
  struct ai_plr *ai = dai_plr_data_get(ait, pplayer, NULL);
  bool need_targets = TRUE;
  struct player *target = NULL;
  int most_hatred = MAX_AI_LOVE;
  int war_threshold;
  int aggr;
  float aggr_sr;
  float max_sr;

  fc_assert_ret(is_ai(pplayer));

  if (!pplayer->is_alive) {
    return;
  }

  if (get_player_bonus(pplayer, EFT_NO_DIPLOMACY) > 0) {
    /* Diplomacy disabled for this player */
    return;
  }

  /*** If we are greviously insulted, go to war immediately. ***/

  players_iterate(aplayer) {
    if (pplayer->ai_common.love[player_index(aplayer)] < 0
        && player_diplstate_get(pplayer, aplayer)->has_reason_to_cancel >= 2
        && dai_diplomacy_get(ait, pplayer, aplayer)->countdown == -1) {
      DIPLO_LOG(ait, LOG_DIPL2, pplayer, aplayer, "Plans war in revenge");
      war_countdown(ait, pplayer, aplayer, map_size_checked(),
                    DAI_WR_BEHAVIOUR);
    }
  } players_iterate_end;

  /*** Stop other players from winning by space race ***/

  if (ai->diplomacy.strategy != WIN_SPACE) {
    struct adv_data *adv = adv_data_get(pplayer, NULL);

    players_iterate_alive(aplayer) {
      struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, aplayer);
      struct player_spaceship *ship = &aplayer->spaceship;

      if (aplayer == pplayer
          || adip->countdown >= 0  /* already counting down to war */
          || ship->state == SSHIP_NONE
          || players_on_same_team(pplayer, aplayer)
          || pplayers_at_war(pplayer, aplayer)) {
        continue;
      }
      /* A spaceship victory is always one single player's or team's victory */
      if (aplayer->spaceship.state == SSHIP_LAUNCHED
          && adv->dipl.spacerace_leader == aplayer
          && pplayers_allied(pplayer, aplayer)) {
        dai_diplo_notify(aplayer,
                         _("*%s (AI)* Your attempt to conquer space for "
                           "yourself alone betrays your true intentions, and I "
                           "will have no more of our alliance!"),
                         player_name(pplayer));
	handle_diplomacy_cancel_pact(pplayer, player_number(aplayer),
				     CLAUSE_ALLIANCE);
        if (gives_shared_vision(pplayer, aplayer)) {
          remove_shared_vision(pplayer, aplayer);
        }
        /* Never forgive this */
        pplayer->ai_common.love[player_index(aplayer)] = -MAX_AI_LOVE;
      } else if (ship->state == SSHIP_STARTED 
		 && adip->warned_about_space == 0) {
        pplayer->ai_common.love[player_index(aplayer)] -= MAX_AI_LOVE / 10;
        adip->warned_about_space = 10 + fc_rand(6);
        dai_diplo_notify(aplayer,
                         _("*%s (AI)* Your attempt to unilaterally "
                           "dominate outer space is highly offensive."),
                         player_name(pplayer));
        dai_diplo_notify(aplayer,
                         _("*%s (AI)* If you do not stop constructing your "
                           "spaceship, I may be forced to take action!"),
                         player_name(pplayer));
      }
      if (aplayer->spaceship.state == SSHIP_LAUNCHED
          && aplayer == adv->dipl.spacerace_leader) {
        /* This means war!!! */
        pplayer->ai_common.love[player_index(aplayer)] -= MAX_AI_LOVE / 2;
        DIPLO_LOG(ait, LOG_DIPL, pplayer, aplayer, "plans war due to spaceship");
        war_countdown(ait, pplayer, aplayer, 4 + map_size_checked(),
                      DAI_WR_SPACE);
      }
    } players_iterate_alive_end;
  }

  /*** Declare war against somebody if we are out of targets ***/

  players_iterate_alive(aplayer) {
    int turns; /* turns since contact */

    if (NEVER_MET(pplayer, aplayer)) {
      continue;
    }
    turns = game.info.turn;
    turns -= player_diplstate_get(pplayer, aplayer)->first_contact_turn;
    if (WAR(pplayer, aplayer)) {
      need_targets = FALSE;
    } else if (pplayer->ai_common.love[player_index(aplayer)] < most_hatred
               && turns > TURNS_BEFORE_TARGET) {
      most_hatred = pplayer->ai_common.love[player_index(aplayer)];
      target = aplayer;
    }
  } players_iterate_alive_end;

  aggr = ai_trait_get_value(TRAIT_AGGRESSIVE, pplayer);
  max_sr = TRAIT_MAX_VALUE_SR;
  aggr_sr = sqrt(aggr);

  war_threshold = (MAX_AI_LOVE * (0.70 + aggr_sr / max_sr / 2.0)) - MAX_AI_LOVE;

  if (need_targets && target && most_hatred < war_threshold
      && dai_diplomacy_get(ait, pplayer, target)->countdown == -1) {
    enum war_reason war_reason;

    if (pplayers_allied(pplayer, target)) {
      DIPLO_LOG(ait, LOG_DEBUG, pplayer, target, "Plans war against an ally!");
    }
    if (player_diplstate_get(pplayer, target)->has_reason_to_cancel > 0) {
      /* We have good reason */
      war_reason = DAI_WR_EXCUSE;
    } else if (pplayer->ai_common.love[player_index(target)] < 0) {
      /* We have a reason of sorts from way back, maybe? */
      war_reason = DAI_WR_HATRED;
    } else {
      /* We have no legimitate reason... So what? */
      war_reason = DAI_WR_NONE;
    }
    DIPLO_LOG(ait, LOG_DEBUG, pplayer, target, "plans war for spoils");
    war_countdown(ait, pplayer, target, 4 + map_size_checked(), war_reason);
  }

  /*** Declare war - against enemies of allies ***/

  players_iterate_alive(aplayer) {
    struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, aplayer);

    if (adip->at_war_with_ally
        && adip->countdown == -1
        && !adip->is_allied_with_ally
        && !pplayers_at_war(pplayer, aplayer)
        && (player_diplstate_get(pplayer, aplayer)->type != DS_CEASEFIRE
            || fc_rand(5) < 1)) {
      DIPLO_LOG(ait, LOG_DEBUG, pplayer, aplayer, "plans war to help ally %s",
                player_name(adip->at_war_with_ally));
      war_countdown(ait, pplayer, aplayer, 2 + map_size_checked(),
                    DAI_WR_ALLIANCE);
    }
  } players_iterate_alive_end;

  /*** Actually declare war (when we have moved units into position) ***/

  players_iterate(aplayer) {
    struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, aplayer);

    if (!aplayer->is_alive) {
      adip->countdown = -1;
      continue;
    }
    if (adip->countdown > 0) {
      adip->countdown--;
    } else if (adip->countdown == 0) {
      if (!WAR(pplayer, aplayer)) {
        DIPLO_LOG(ait, LOG_DIPL2, pplayer, aplayer, "Declaring war!");
        dai_go_to_war(ait, pplayer, aplayer, adip->war_reason);
      }
    } else if (adip->countdown < -1) {
      /* negative countdown less than -1 is war stubbornness */
      adip->countdown++;
    }
  } players_iterate_end;

  /*** Try to make peace with everyone we love ***/

  players_iterate_alive(aplayer) {
    if (get_player_bonus(aplayer, EFT_NO_DIPLOMACY) <= 0
        && diplomacy_possible(pplayer, aplayer)) {
      enum diplstate_type ds = player_diplstate_get(pplayer, aplayer)->type;
      struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, aplayer);
      struct Clause clause;

      /* Meaningless values, but rather not have them unset. */
      clause.from = pplayer;
      clause.value = 0;

      /* Remove shared vision if we are not allied or it is no longer safe. */
      if (gives_shared_vision(pplayer, aplayer)) {
        if (!pplayers_allied(pplayer, aplayer)) {
          remove_shared_vision(pplayer, aplayer);
        } else if (!shared_vision_is_safe(pplayer, aplayer)) {
          dai_diplo_notify(aplayer,
                           _("*%s (AI)* Sorry, sharing vision with you "
                             "is no longer safe."),
                           player_name(pplayer));
          remove_shared_vision(pplayer, aplayer);
        }
      }

      /* No peace to enemies of our allies... or pointless peace. */
      if (is_barbarian(aplayer)
          || aplayer == pplayer
          || aplayer == target     /* no mercy */
          || adip->countdown >= 0
          || !could_meet_with_player(pplayer, aplayer)) {
        continue;
      }

      /* Spam control */
      adip->asked_about_peace = MAX(adip->asked_about_peace - 1, 0);
      adip->asked_about_alliance = MAX(adip->asked_about_alliance - 1, 0);
      adip->asked_about_ceasefire = MAX(adip->asked_about_ceasefire - 1, 0);
      adip->warned_about_space = MAX(adip->warned_about_space - 1, 0);
      adip->spam = MAX(adip->spam - 1, 0);
      if (adip->spam > 0 && is_ai(aplayer)) {
        /* Don't spam */
        continue;
      }

      /* Canvass support from existing friends for our war, and try to
       * make friends with enemies. Then we wait some turns until next time
       * we spam them with our gibbering chatter. */
      if (adip->spam <= 0) {
        if (!pplayers_allied(pplayer, aplayer)) {
          adip->spam = fc_rand(4) + 3; /* Bugger allies often. */
        } else {
          adip->spam = fc_rand(8) + 6; /* Others are less important. */
        }
      }

      switch (ds) {
      case DS_TEAM:
        dai_share(ait, pplayer, aplayer);
        break;
      case DS_ALLIANCE:
        /* See if our allies are diligently declaring war on our enemies... */
        if (adip->at_war_with_ally) {
          break;
        }
        target = NULL;
        players_iterate(eplayer) {
          if (WAR(pplayer, eplayer)
              && !pplayers_at_war(aplayer, eplayer)) {
            target = eplayer;
            break;
          }
        } players_iterate_end;

        if ((players_on_same_team(pplayer, aplayer)
             || pplayer->ai_common.love[player_index(aplayer)] > MAX_AI_LOVE / 2)) {
          /* Share techs only with team mates and allies we really like. */
          dai_share(ait, pplayer, aplayer);
        }
        if (!target || !target->is_alive) {
          adip->ally_patience = 0;
          break; /* No need to nag our ally */
        }

        if (adip->spam <= 0) {
          /* Count down patience toward AI player (one that can have spam > 0) 
           * at the same speed as toward human players. */
          if (adip->ally_patience == 0) {
            dai_diplo_notify(aplayer,
                             _("*%s (AI)* Greetings our most trustworthy "
                               "ally. We call upon you to destroy our enemy, %s."), 
                             player_name(pplayer),
                             player_name(target));
            adip->ally_patience--;
          } else if (adip->ally_patience == -1) {
            if (fc_rand(5) == 1) {
              dai_diplo_notify(aplayer,
                               _("*%s (AI)* Greetings ally, I see you have not yet "
                                 "made war with our enemy, %s. Why do I need to remind "
                                 "you of your promises?"),
                               player_name(pplayer),
                               player_name(target));
              adip->ally_patience--;
            }
          } else {
            if (fc_rand(5) == 1) {
              dai_diplo_notify(aplayer,
                               _("*%s (AI)* Dishonored one, we made a pact of "
                                 "alliance, and yet you remain at peace with our mortal "
                                 "enemy, %s! This is unacceptable; our alliance is no "
                                 "more!"),
                               player_name(pplayer),
                               player_name(target));
              DIPLO_LOG(ait, LOG_DIPL2, pplayer, aplayer, "breaking useless alliance");
              /* to peace */
              handle_diplomacy_cancel_pact(pplayer, player_number(aplayer),
                                           CLAUSE_ALLIANCE);
              pplayer->ai_common.love[player_index(aplayer)] =
                MIN(pplayer->ai_common.love[player_index(aplayer)], 0);
              if (gives_shared_vision(pplayer, aplayer)) {
                remove_shared_vision(pplayer, aplayer);
              }
              fc_assert(!gives_shared_vision(pplayer, aplayer));
            }
          }
        }
        break;

      case DS_PEACE:
        clause.type = CLAUSE_ALLIANCE;
        if (adip->at_war_with_ally
            || (is_human(aplayer) && adip->asked_about_alliance > 0)
            || dai_goldequiv_clause(ait, pplayer, aplayer, &clause,
                                    FALSE, DS_ALLIANCE) < 0) {
          break;
        }
        clear_old_treaty(pplayer, aplayer);
        dai_diplomacy_suggest(pplayer, aplayer, CLAUSE_ALLIANCE, FALSE, 0);
        adip->asked_about_alliance = is_human(aplayer) ? 13 : 0;
        dai_diplo_notify(aplayer,
                         _("*%s (AI)* Greetings friend, may we suggest "
                           "making a common cause and join in an alliance?"), 
                         player_name(pplayer));
        break;

      case DS_CEASEFIRE:
        clause.type = CLAUSE_PEACE;
        if (adip->at_war_with_ally
            || (is_human(aplayer) && adip->asked_about_peace > 0)
            || dai_goldequiv_clause(ait, pplayer, aplayer, &clause,
                                    FALSE, DS_PEACE) < 0) {
          break;
        }
        clear_old_treaty(pplayer, aplayer);
        dai_diplomacy_suggest(pplayer, aplayer, CLAUSE_PEACE, FALSE, 0);
        adip->asked_about_peace = is_human(aplayer) ? 12 : 0;
        dai_diplo_notify(aplayer,
                         _("*%s (AI)* Greetings neighbor, may we suggest "
                           "more peaceful relations?"),
                         player_name(pplayer));
        break;

      case DS_NO_CONTACT: /* but we do have embassy! weird. */
      case DS_WAR:
        clause.type = CLAUSE_CEASEFIRE;
        if ((is_human(aplayer) && adip->asked_about_ceasefire > 0)
            || dai_goldequiv_clause(ait, pplayer, aplayer, &clause,
                                    FALSE, DS_CEASEFIRE) < 0) {
          break; /* Fight until the end! */
        }
        clear_old_treaty(pplayer, aplayer);
        dai_diplomacy_suggest(pplayer, aplayer, CLAUSE_CEASEFIRE, FALSE, 0);
        adip->asked_about_ceasefire = is_human(aplayer) ? 9 : 0;
        dai_diplo_notify(aplayer,
                         _("*%s (AI)* We grow weary of this constant "
                           "bloodshed. May we suggest a cessation of hostilities?"), 
                         player_name(pplayer));
        break;

      case DS_ARMISTICE:
        break;
      default:
        fc_assert_msg(FALSE, "Unknown pact type %d.", ds);
        break;
      }
    }
  } players_iterate_alive_end;
}

/******************************************************************//**
  Are we going to be declaring war in a few turns time?  If so, go
  on a war footing, and try to buy out as many units as possible.
**********************************************************************/
bool dai_on_war_footing(struct ai_type *ait, struct player *pplayer)
{
  players_iterate(plr) {
    if (dai_diplomacy_get(ait, pplayer, plr)->countdown >= 0) {
      return TRUE;
    }
  } players_iterate_end;

  return FALSE;
}

/******************************************************************//**
  Handle incident caused by violator
**********************************************************************/
/* AI attitude call-backs */
void dai_incident(struct ai_type *ait, enum incident_type type,
                  struct player *violator, struct player *victim)
{
  switch(type) {
    case INCIDENT_DIPLOMAT:
      dai_incident_diplomat(violator, victim);
      break;
    case INCIDENT_WAR:
      dai_incident_war(violator, victim);
      break;
    case INCIDENT_PILLAGE:
      dai_incident_pillage(violator, victim);
      break;
    case INCIDENT_NUCLEAR:
      dai_incident_nuclear(violator, victim);
      break;
    case INCIDENT_NUCLEAR_NOT_TARGET:
      dai_incident_nuclear_not_target(violator, victim);
      break;
    case INCIDENT_NUCLEAR_SELF:
      dai_incident_nuclear_self(violator, victim);
      break;
    case INCIDENT_LAST:
      /* Assert that always fails, but with meaningfull message */
      fc_assert(type != INCIDENT_LAST);
      break;
  }
}

/******************************************************************//**
  Nuclear strike against victim. Victim may be NULL.
**********************************************************************/
static void dai_incident_nuclear(struct player *violator, struct player *victim)
{
  if (!is_ai(victim)) {
    return;
  }

  if (violator == victim) {
    return;
  }

  if (victim != NULL) {
    victim->ai_common.love[player_index(violator)] -= 3 * MAX_AI_LOVE / 10;
  }
}

/******************************************************************//**
  Nuclear strike against someone else.
**********************************************************************/
static void dai_incident_nuclear_not_target(struct player *violator,
                                            struct player *victim)
{
  if (!is_ai(victim)) {
    return;
  }

  victim->ai_common.love[player_index(violator)] -= MAX_AI_LOVE / 10;
}

/******************************************************************//**
  Somebody else than victim did nuclear strike against self.
**********************************************************************/
static void dai_incident_nuclear_self(struct player *violator,
                                      struct player *victim)
{
  if (!is_ai(victim)) {
    return;
  }

  victim->ai_common.love[player_index(violator)] -= MAX_AI_LOVE / 20;
}

/******************************************************************//**
  Diplomat caused an incident.
**********************************************************************/
static void dai_incident_diplomat(struct player *violator,
                                  struct player *victim)
{
  players_iterate(pplayer) {
    if (!is_ai(pplayer)) {
      continue;
    }

    if (pplayer != violator) {
      /* Dislike backstabbing bastards */
      pplayer->ai_common.love[player_index(violator)] -= MAX_AI_LOVE / 100;
      if (victim == pplayer) {
        pplayer->ai_common.love[player_index(violator)] -= MAX_AI_LOVE / 7;
      }
    }
  } players_iterate_end;
}

/******************************************************************//**
  War declared against a player.  We apply a penalty because this
  means he is seen as untrustworthy, especially if past relations
  with the victim have been cordial (betrayal).

  Reasons for war and other mitigating circumstances are checked
  in calling code.
**********************************************************************/
static void dai_incident_war(struct player *violator, struct player *victim)
{
  players_iterate(pplayer) {
    if (!is_ai(pplayer)) {
      continue;
    }

    if (pplayer != violator) {
      /* Dislike backstabbing bastards */
      pplayer->ai_common.love[player_index(violator)] -= MAX_AI_LOVE / 30;
      if (player_diplstate_get(violator, victim)->max_state == DS_PEACE) {
        /* Extra penalty if they once had a peace treaty */
        pplayer->ai_common.love[player_index(violator)] -= MAX_AI_LOVE / 30;
      } else if (player_diplstate_get(violator, victim)->max_state
                 == DS_ALLIANCE) {
        /* Extra penalty if they once had an alliance */
        pplayer->ai_common.love[player_index(violator)] -= MAX_AI_LOVE / 10;
      }
      if (victim == pplayer) {
        pplayer->ai_common.love[player_index(violator)] = 
          MIN(pplayer->ai_common.love[player_index(violator)] - MAX_AI_LOVE / 3, -1);
        /* Scream for help!! */
        players_iterate_alive(ally) {
          if (!pplayers_allied(pplayer, ally)) {
            continue;
          }
          dai_diplo_notify(ally,
                           _("*%s (AI)* We have been savagely attacked by "
                             "%s, and we need your help! Honor our glorious "
                             "alliance and your name will never be forgotten!"),
                           player_name(victim),
                           player_name(violator));
        } players_iterate_alive_end;
      }
    }
  } players_iterate_end;
}

/******************************************************************//**
  Violator pillaged something on victims territory
**********************************************************************/
static void dai_incident_pillage(struct player *violator, struct player *victim)
{
  if (violator == victim) {
    return;
  }
  if (victim == NULL) {
    return;
  }
  victim->ai_common.love[player_index(violator)] -= MAX_AI_LOVE / 20;
}
