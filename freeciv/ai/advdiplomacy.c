/********************************************************************** 
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
#include <config.h>
#endif

#include <assert.h>
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
#include "spaceship.h"
#include "tech.h"
#include "unitlist.h"

/* server */
#include "citytools.h"
#include "diplhand.h"
#include "maphand.h"
#include "notify.h"
#include "settlers.h"  /* amortize */

/* ai */
#include "aidata.h"
#include "ailog.h"
#include "aiunit.h"
#include "aitools.h"
#include "advmilitary.h"

#include "advdiplomacy.h"

#define LOG_DIPL LOG_DEBUG
#define LOG_DIPL2 LOG_DEBUG

/* One hundred thousand. Basically a number of gold that no player is
 * ever likely to have, but not so big that we get integer overflows. */
#define BIG_NUMBER 100000

/* This is how much negative AI love we need before we decide to embark
 * on opportunistic war for spoils. */
#define WAR_THRESHOLD -(MAX_AI_LOVE / 8)

/* turn this off when we don't want functions to message players */
static bool diplomacy_verbose = TRUE;

/* turns to wait after contact before taking aim for war */
#define TURNS_BEFORE_TARGET 15

static void ai_incident_war(struct player *violator, struct player *victim);
static void ai_incident_diplomat(struct player *violator, struct player *victim);
static void ai_incident_nuclear(struct player *violator, struct player *victim);
static void ai_incident_pillage(struct player *violator, struct player *victim);

/**********************************************************************
  Send a diplomatic message. Use this instead of notify directly
  because we may want to highligh/present these messages differently
  in the future.
***********************************************************************/
static void notify(struct player *pplayer, const char *text, ...)
{
  if (diplomacy_verbose) {
    va_list ap;
    struct conn_list *dest = pplayer->connections;
    struct packet_chat_msg packet;

    va_start(ap, text);
    vpackage_event(&packet, NULL, E_DIPLOMACY,
                   FTC_PRIVATE_MSG, NULL, text, ap);
    va_end(ap);

    lsend_packet_chat_msg(dest, &packet);
  }
}

/********************************************************************** 
  This is your typical human reaction. Convert lack of love into 
  lust for gold.
***********************************************************************/
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

/****************************************************************************
  Convert clause into diplomatic state
****************************************************************************/
static enum diplstate_type pact_clause_to_diplstate_type(enum clause_type type)
{
  switch(type) {
    case CLAUSE_ALLIANCE:
	return DS_ALLIANCE;
    case CLAUSE_PEACE:
        return DS_PEACE;
    case CLAUSE_CEASEFIRE:
	return DS_CEASEFIRE;
    default:
	assert(0);
	return DS_WAR;
  }
}

/********************************************************************** 
  How much is a tech worth to player measured in gold
***********************************************************************/
static int ai_goldequiv_tech(struct player *pplayer, Tech_type_id tech)
{
  int bulbs, tech_want, worth;
  enum tech_state state = player_invention_state(pplayer, tech);

  if (TECH_KNOWN == state
      || ! player_invention_reachable(pplayer, tech)) {
    return 0;
  }
  bulbs = total_bulbs_required_for_goal(pplayer, tech) * 3;
  tech_want = MAX(pplayer->ai_data.tech_want[tech], 0) / MAX(game.info.turn, 1);
  worth = bulbs + tech_want;
  if (TECH_PREREQS_KNOWN == state) {
    worth /= 2;
  }
  return worth;
}

/************************************************************************
  Avoid giving pplayer's vision to non-allied player through aplayer 
  (shared vision is transitive).
************************************************************************/
static bool shared_vision_is_safe(struct player* pplayer,
                                  struct player* aplayer)
{
  if (pplayer->team && pplayer->team == aplayer->team) {
    return TRUE;
  }
  players_iterate(eplayer) {
    if (eplayer == pplayer || eplayer == aplayer || !eplayer->is_alive) {
      continue;
    }
    if (gives_shared_vision(aplayer, eplayer)) {
      enum diplstate_type ds = pplayer_get_diplstate(pplayer, eplayer)->type;

      if (ds != DS_NO_CONTACT && ds != DS_ALLIANCE) {
        return FALSE;
      }
    }
  } players_iterate_end;
  return TRUE;
}

/********************************************************************** 
  Checks if player1 can agree on ceasefire with player2
  This function should only be used for ai players
**********************************************************************/
static bool ai_players_can_agree_on_ceasefire(struct player* player1,
                                              struct player* player2)
{
  struct ai_data *ai = ai_data_get(player1);

  return (player1->ai_data.love[player_index(player2)] > - (MAX_AI_LOVE * 4 / 10)
          && ai->diplomacy.player_intel[player_index(player2)].countdown == -1);
}

/********************************************************************** 
  Calculate a price of a tech.
  Note that both AI players always evaluate the tech worth symetrically
  This eases tech exchange.
  is_dangerous returns ig the giver is afraid of giving that tech
  (the taker should evaluate it normally, but giver should never give that)
**********************************************************************/
static int compute_tech_sell_price(struct player* giver, struct player* taker,
                                   int tech_id, bool* is_dangerous)
{
    int worth;
    
    worth = ai_goldequiv_tech(taker, tech_id);
    
    *is_dangerous = FALSE;
    
    /* Share and expect being shared brotherly between allies */
    if (pplayers_allied(giver, taker)) {
      worth /= 2;
    }
    if (players_on_same_team(giver, taker)) {
      return 0;
    }

    /* Do not bother wanting a tech that we already have. */
    if (player_invention_state(taker, tech_id) == TECH_KNOWN) {
      return 0;
    }

    /* Calculate in tech leak to our opponents, guess 50% chance */
    players_iterate(eplayer) {
      if (eplayer == giver
          || eplayer == taker
          || !eplayer->is_alive
          || player_invention_state(eplayer, tech_id) == TECH_KNOWN) {
        continue;
      }
      
      /* Don't risk it falling into enemy hands */
      if (pplayers_allied(taker, eplayer) &&
          is_player_dangerous(giver, eplayer)) {
        *is_dangerous = TRUE;
      }
      
      if (pplayers_allied(taker, eplayer) &&
          !pplayers_allied(giver, eplayer)) {
        /* Taker can enrichen his side with this tech */
        worth += ai_goldequiv_tech(eplayer, tech_id) / 4;
      }
    } players_iterate_end;
    return worth;
}

/********************************************************************** 
  Returns an enemy player to 'us' allied with 'them' if there is one.
***********************************************************************/
static const struct player *
get_allied_with_enemy_player(const struct player *us,
                             const struct player *them)
{
  players_iterate(aplayer) {
    if (aplayer != us
        && aplayer != them
        && aplayer->is_alive
        && pplayers_allied(them, aplayer)
        && pplayer_get_diplstate(us, aplayer)->type == DS_WAR) {
      return aplayer;
    }
  } players_iterate_end;
  return NULL;
}

/********************************************************************** 
  Evaluate gold worth of a single clause in a treaty. Note that it
  sometimes matter a great deal who is giving what to whom, and
  sometimes (such as with treaties) it does not matter at all.
  ds_after means a pact offered in the same treaty or current diplomatic
  state
***********************************************************************/
static int ai_goldequiv_clause(struct player *pplayer, 
                               struct player *aplayer,
                               struct Clause *pclause,
                               struct ai_data *ai,
                               bool verbose,
                               enum diplstate_type ds_after)
{
  int worth = 0; /* worth for pplayer of what aplayer gives */
  bool give = (pplayer == pclause->from);
  struct player *giver;
  const struct player *penemy;
  struct ai_dip_intel *adip = &ai->diplomacy.player_intel[player_index(aplayer)];
  bool is_dangerous;

  assert(pplayer != aplayer);
  
  diplomacy_verbose = verbose;
  ds_after = MAX(ds_after, pplayer->diplstates[player_index(aplayer)].type);
  giver = pclause->from;

  switch (pclause->type) {
  case CLAUSE_ADVANCE:
    if (give) {
      worth -= compute_tech_sell_price(pplayer, aplayer, pclause->value,
                                       &is_dangerous);
      if (is_dangerous) {
        return -BIG_NUMBER;
      }
    } else if (player_invention_state(pplayer, pclause->value) != TECH_KNOWN) {
      worth += compute_tech_sell_price(aplayer, pplayer, pclause->value,
                                       &is_dangerous);
    }
    DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "%s clause worth %d",
              advance_name_by_player(pplayer, pclause->value), worth);
    break;

  case CLAUSE_ALLIANCE:
  case CLAUSE_PEACE:
  case CLAUSE_CEASEFIRE:
    /* Don't do anything in away mode */
    if (ai_handicap(pplayer, H_AWAY)) {
      notify(aplayer, _("*%s (AI)* In away mode AI can't sign such a treaty."),
             player_name(pplayer));
      worth = -BIG_NUMBER;
      break;
    }

    /* This guy is allied to one of our enemies. Only accept
     * ceasefire. */
    if ((penemy = get_allied_with_enemy_player(pplayer, aplayer))
        && pclause->type != CLAUSE_CEASEFIRE) {
      notify(aplayer, _("*%s (AI)* First break alliance with %s, %s."),
             player_name(pplayer), player_name(penemy),
             player_name(aplayer));
      worth = -BIG_NUMBER;
      break;
    }

    /* He was allied with an enemy at the begining of the turn. */
    if (adip->is_allied_with_enemy
        && pclause->type != CLAUSE_CEASEFIRE) {
      notify(aplayer, _("*%s (AI)* I would like to see you keep your "
                        "distance from %s for some time, %s."),
             player_name(pplayer), player_name(adip->is_allied_with_enemy),
             player_name(aplayer));
      worth = -BIG_NUMBER;
      break;
    }

    /* Steps of the treaty ladder */
    if (pclause->type == CLAUSE_PEACE) {
      struct player_diplstate *ds = &pplayer->diplstates[player_index(aplayer)];

      if (!pplayers_non_attack(pplayer, aplayer)) {
        notify(aplayer, _("*%s (AI)* Let us first cease hostilies, %s."),
               player_name(pplayer),
               player_name(aplayer));
        worth = -BIG_NUMBER;
      } else if (ds->type == DS_CEASEFIRE && ds->turns_left > 4) {
        notify(aplayer, _("*%s (AI)* I wish to see you keep the current "
               "ceasefire for a bit longer first, %s."),
               player_name(pplayer), 
               player_name(aplayer));
        worth = -BIG_NUMBER;
      } else if (adip->countdown >= 0 || adip->countdown < -1) {
        worth = -BIG_NUMBER; /* but say nothing */
      } else {
        worth = greed(pplayer->ai_data.love[player_index(aplayer)]
                      - ai->diplomacy.req_love_for_peace);
      }
    } else if (pclause->type == CLAUSE_ALLIANCE) {
      if (!pplayers_in_peace(pplayer, aplayer)) {
        worth = greed(pplayer->ai_data.love[player_index(aplayer)]
                      - ai->diplomacy.req_love_for_peace);
      }
      if (adip->countdown >= 0 || adip->countdown < -1) {
        worth = -BIG_NUMBER; /* but say nothing */
      } else {
        worth += greed(pplayer->ai_data.love[player_index(aplayer)]
                       - ai->diplomacy.req_love_for_alliance);
      }
      if (pplayer->ai_data.love[player_index(aplayer)] < MAX_AI_LOVE / 10) {
        notify(aplayer, _("*%s (AI)* I simply do not trust you with an "
               "alliance yet, %s."),
               player_name(pplayer),
               player_name(aplayer));
        worth = -BIG_NUMBER;
      }
      DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "ally clause worth %d", worth);
    } else {
      if (pplayer->ai_data.control && aplayer->ai_data.control &&
         ai_players_can_agree_on_ceasefire(pplayer, aplayer)) {
	 worth = 0;
      } else {
        int turns = game.info.turn;

        turns -= pplayer_get_diplstate(pplayer, aplayer)->first_contact_turn;
        if (turns < TURNS_BEFORE_TARGET) {
          worth = 0; /* show some good faith */
          break;
        } else {
          worth = greed(pplayer->ai_data.love[player_index(aplayer)]);
          DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "ceasefire worth=%d love=%d "
                    "turns=%d", worth,
                    pplayer->ai_data.love[player_index(aplayer)],
                    turns);
        }
      }
    }

    /* Let's all hold hands in one happy family! */
    if (adip->is_allied_with_ally) {
      worth /= 2;
      break;
    }

    DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "treaty clause worth %d", worth);
  break;

  case CLAUSE_GOLD:
    if (give) {
      worth -= pclause->value;
    } else {
      worth += pclause->value;
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
      worth = MIN(pplayer->ai_data.love[player_index(aplayer)] * 7, worth);
      /* Make maps from novice player cheap */
      if (ai_handicap(pplayer, H_DIPLOMACY)) {
        worth /= 2;
      }
    }
    DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "seamap clause worth %d",
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
      worth = MIN(pplayer->ai_data.love[player_index(aplayer)] * 10, worth);
      /* Make maps from novice player cheap */
      if (ai_handicap(pplayer, H_DIPLOMACY)) {
        worth /= 6;
      }
    }
    DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "landmap clause worth %d",
              worth);
    break;

  case CLAUSE_CITY: {
    struct city *offer = city_list_find_id(pclause->from->cities, 
                                           pclause->value);

    if (!offer || city_owner(offer) != giver) {
      /* City destroyed or taken during negotiations */
      notify(aplayer, _("*%s (AI)* I do not know the city you mention."),
             player_name(pplayer));
      worth = 0;
      DIPLO_LOG(LOG_DEBUG, pplayer, aplayer, "city destroyed during negotiations");
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
      DIPLO_LOG(LOG_DEBUG, pplayer, aplayer, "worth of %s is %d", 
		city_name(offer), worth);
    }
    break;
  }

  case CLAUSE_VISION:
    if (give) {
      if (ds_after == DS_ALLIANCE) {
        if (!shared_vision_is_safe(pplayer, aplayer)) {
          notify(aplayer, _("*%s (AI)* Sorry, sharing vision with you "
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
                    + pplayer->ai_data.love[player_index(aplayer)], 
                    -5 * game.info.turn);
      }
    } else {
      worth = 0; /* We don't need no stinkin' embassy, do we? */
    }
    DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "embassy clause worth %d",
              worth);
    break;

  case CLAUSE_LAST:
    break;
  } /* end of switch */

  diplomacy_verbose = TRUE;
  return worth;
}

/********************************************************************** 
  pplayer is AI player, aplayer is the other player involved, treaty
  is the treaty being considered. It is all a question about money :-)
***********************************************************************/
void ai_treaty_evaluate(struct player *pplayer, struct player *aplayer,
                        struct Treaty *ptreaty)
{
  int total_balance = 0;
  bool only_gifts = TRUE;
  struct ai_data *ai = ai_data_get(pplayer);
  enum diplstate_type ds_after =
    pplayer_get_diplstate(pplayer, aplayer)->type;
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
    total_balance +=
      ai_goldequiv_clause(pplayer, aplayer, pclause, ai, TRUE, ds_after);
    
    if (pclause->type != CLAUSE_GOLD && pclause->type != CLAUSE_MAP
        && pclause->type != CLAUSE_SEAMAP && pclause->type != CLAUSE_VISION
        && (pclause->type != CLAUSE_ADVANCE 
            || game.info.tech_cost_style != 0
            || pclause->value == get_player_research(pplayer)->tech_goal
            || pclause->value == get_player_research(pplayer)->researching
            || is_tech_a_req_for_goal(pplayer, pclause->value, 
				get_player_research(pplayer)->tech_goal))) {
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
    DIPLO_LOG(LOG_DIPL2, pplayer, aplayer, "no peace offered, must refuse");
    return;
  }

  if (given_cities > 0
      && city_list_size(pplayer->cities) - given_cities <= 2) {
    /* always keep at least two cities */
    DIPLO_LOG(LOG_DIPL2, pplayer, aplayer, "cannot give last cities");
    return;
  }

  /* Accept if balance is good */
  if (total_balance >= 0) {
    handle_diplomacy_accept_treaty_req(pplayer, player_number(aplayer));
    DIPLO_LOG(LOG_DIPL2, pplayer, aplayer, "balance was good: %d", 
              total_balance);
  } else {
    /* AI complains about the treaty which was proposed, unless the AI
     * made the proposal. */
    if (pplayer != ptreaty->plr0) {
      notify(aplayer, _("*%s (AI)* This deal was not very good for us, %s!"),
           player_name(pplayer),
           player_name(aplayer));
    }
    DIPLO_LOG(LOG_DIPL2, pplayer, aplayer, "balance was bad: %d", 
              total_balance);
  }
}

/********************************************************************** 
  Comments to player from AI on clauses being agreed on. Does not
  alter any state.
***********************************************************************/
static void ai_treaty_react(struct player *pplayer,
                            struct player *aplayer,
                            struct Clause *pclause)
{
  struct ai_data *ai = ai_data_get(pplayer);
  struct ai_dip_intel *adip = &ai->diplomacy.player_intel[player_index(aplayer)];

  switch (pclause->type) {
    case CLAUSE_ALLIANCE:
      if (adip->is_allied_with_ally) {
        notify(aplayer, _("*%s (AI)* Welcome into our alliance %s!"),
               player_name(pplayer),
               player_name(aplayer));
      } else {
        notify(aplayer, _("*%s (AI)* Yes, may we forever stand united, %s."),
               player_name(pplayer),
               player_name(aplayer));
      }
      DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "become allies");
      break;
    case CLAUSE_PEACE:
      notify(aplayer, _("*%s (AI)* Yes, peace in our time!"),
             player_name(pplayer));
      DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "sign peace treaty");
      break;
    case CLAUSE_CEASEFIRE:
      notify(aplayer, _("*%s (AI)* Agreed. No more hostilities, %s."),
             player_name(pplayer),
             player_name(aplayer));
      DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "sign ceasefire");
      break;
    default:
      break;
  }
}

/********************************************************************** 
  This function is called when a treaty has been concluded, to deal
  with followup issues like comments and relationship (love) changes.

  pplayer is AI player, aplayer is the other player involved, ptreaty
  is the treaty accepted.
***********************************************************************/
void ai_treaty_accepted(struct player *pplayer, struct player *aplayer,
                        struct Treaty *ptreaty)
{
  int total_balance = 0;
  bool gift = TRUE;
  struct ai_data *ai = ai_data_get(pplayer);
  enum diplstate_type ds_after =
    pplayer_get_diplstate(pplayer, aplayer)->type;

  assert(pplayer != aplayer);
  
  clause_list_iterate(ptreaty->clauses, pclause) {
    if (is_pact_clause(pclause->type)) {
      ds_after = pact_clause_to_diplstate_type(pclause->type);
    }
  } clause_list_iterate_end;

  /* Evaluate clauses */
  clause_list_iterate(ptreaty->clauses, pclause) {
    int balance =
      ai_goldequiv_clause(pplayer, aplayer, pclause, ai, TRUE, ds_after);

    total_balance += balance;
    gift = (gift && (balance >= 0));
    ai_treaty_react(pplayer, aplayer, pclause);
    if (is_pact_clause(pclause->type)
        && ai->diplomacy.player_intel[player_index(aplayer)].countdown != -1) {
      /* Cancel a countdown towards war if we just agreed to peace... */
      DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "countdown nullified");
      ai->diplomacy.player_intel[player_index(aplayer)].countdown = -1;
    }
  } clause_list_iterate_end;

  /* Rather arbitrary algorithm to increase our love for a player if
   * he or she offers us gifts. It is only a gift if _all_ the clauses
   * are beneficial to us. */
  if (total_balance > 0 && gift) {
    int i = total_balance / ((city_list_size(pplayer->cities) * 10) + 1);

    i = MIN(i, ai->diplomacy.love_incr * 150) * 10;
    pplayer->ai_data.love[player_index(aplayer)] += i;
    DIPLO_LOG(LOG_DIPL2, pplayer, aplayer, "gift increased love by %d", i);
  }
}

/********************************************************************** 
  Calculate our desire to go to war against aplayer.  We want to 
  attack a player that is easy to beat and will yield a nice profit.

  This function is full of hardcoded constants by necessity.  They are
  not #defines since they are not used anywhere else.
***********************************************************************/
static int ai_war_desire(struct player *pplayer, struct player *target,
                         struct ai_data *ai)
{
  int want = 0, fear = 0, distance = 0, settlers = 0, cities = 0;
  struct player_spaceship *ship = &target->spaceship;

  city_list_iterate(target->cities, pcity) {
    want += 100; /* base city want */
    want += pcity->size * 20;
    want += pcity->surplus[O_SHIELD] * 8;
    want += pcity->surplus[O_TRADE] * 6;

    /* FIXME: This might be buggy if it ignores unmet UnitClass reqs. */
    fear += get_city_bonus(pcity, EFT_DEFEND_BONUS);

    city_built_iterate(pcity, pimprove) {
      want += impr_build_shield_cost(pimprove);
      if (improvement_obsolete(pplayer, pimprove)) {
        continue;
      }
      if (is_great_wonder(pimprove)) {
        want += impr_build_shield_cost(pimprove) * 2;
      } else if (is_small_wonder(pimprove)) {
        want += impr_build_shield_cost(pimprove);
      }
    } city_built_iterate_end;
  } city_list_iterate_end;
  unit_list_iterate(target->units, punit) {
    fear += ATTACK_POWER(punit);

    /* Fear enemy expansionism */
    if (unit_has_type_flag(punit, F_CITIES)) {
      want += 100;
    }
  } unit_list_iterate_end;
  unit_list_iterate(pplayer->units, punit) {
    fear -= ATTACK_POWER(punit) / 2;

    /* Our own expansionism reduces want for war */
    if (unit_has_type_flag(punit, F_CITIES)) {
      want -= 200;
      settlers++;
    }
  } unit_list_iterate_end;
  city_list_iterate(pplayer->cities, pcity) {
    if (VUT_UTYPE == pcity->production.kind 
        && utype_has_flag(pcity->production.value.utype, F_CITIES)) {
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
  ai->diplomacy.player_intel[player_index(target)].distance = distance;

  /* Worry a bit if the other player has extreme amounts of wealth
   * that can be used in cities to quickly buy an army. */
  fear += (target->economic.gold / 5000) * city_list_size(target->cities);

  /* Tech lead is worrisome. FIXME: Only consider 'military' techs. */
  fear += MAX(get_player_research(target)->techs_researched
              - get_player_research(pplayer)->techs_researched, 0) * 100;

  /* Spacerace loss we will not allow! */
  if (ship->state >= SSHIP_STARTED) {
    want *= 2;
  }
  if (ai->diplomacy.spacerace_leader == target) {
    ai->diplomacy.strategy = WIN_CAPITAL;
    return BIG_NUMBER; /* do NOT amortize this number! */
  }

  /* Modify by which treaties we would break to other players, and what
   * excuses we have to do so. FIXME: We only consider immediate
   * allies, but we might trigger a wider chain reaction. */
  players_iterate(eplayer) {
    bool cancel_excuse =
	pplayer->diplstates[player_index(eplayer)].has_reason_to_cancel != 0;
    enum diplstate_type ds = pplayer_get_diplstate(pplayer, eplayer)->type;

    if (eplayer == pplayer || !eplayer->is_alive) {
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
  } players_iterate_end;

  /* Modify by love. Increase the divisor to make ai go to war earlier */
  want -= MAX(0, want * pplayer->ai_data.love[player_index(target)] 
                 / (2 * MAX_AI_LOVE));

  /* Make novice AI more peaceful with human players */
  if (ai_handicap(pplayer, H_DIPLOMACY) && !target->ai_data.control) {
    want /= 2;
  }

  /* Amortize by distance */
  want = amortize(want, distance);

  if (pplayers_allied(pplayer, target)) {
    want /= 4;
  }

  DIPLO_LOG(LOG_DEBUG, pplayer, target, "War want %d, war fear %d",
            want, fear);
  return (want - fear);
}

/********************************************************************** 
  Suggest a treaty from pplayer to aplayer
***********************************************************************/
static void ai_diplomacy_suggest(struct player *pplayer, 
                                 struct player *aplayer,
                                 enum clause_type what,
                                 int value)
{
  if (!could_meet_with_player(pplayer, aplayer)) {
    freelog(LOG_DIPL2, "%s tries to do diplomacy to %s without contact",
            player_name(pplayer),
            player_name(aplayer));
    return;
  }

  handle_diplomacy_init_meeting_req(pplayer, player_number(aplayer));
  handle_diplomacy_create_clause_req(pplayer, player_number(aplayer),
				     player_number(pplayer), what, value);
}

/********************************************************************** 
  What to do when we first meet. pplayer is the AI player.
***********************************************************************/
void ai_diplomacy_first_contact(struct player *pplayer,
                                struct player *aplayer)
{
  if (pplayer->ai_data.control && !ai_handicap(pplayer, H_AWAY)) {
    notify(aplayer, _("*%s (AI)* Greetings %s! May we suggest a ceasefire "
           "while we get to know each other better?"),
           player_name(pplayer),
           player_name(aplayer));
    ai_diplomacy_suggest(pplayer, aplayer, CLAUSE_CEASEFIRE, 0);
  }
}

/********************************************************************** 
  Calculate our diplomatic predispositions here. Don't do anything.

  Only ever called for AI players and never for barbarians.

  This is called at the start of a new AI phase.  It's not called when
  a game is loaded.  So everything calculated here should be put into
  the savegame.
***********************************************************************/
void ai_diplomacy_begin_new_phase(struct player *pplayer,
				  struct ai_data *ai)
{
  int war_desire[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
  int best_desire = 0;
  struct player *best_target = NULL;

  memset(war_desire, 0, sizeof(war_desire));

  assert(pplayer->ai_data.control);
  if (!pplayer->is_alive) {
    return; /* duh */
  }

  /* Calculate our desires, and find desired war target */
  players_iterate(aplayer) {
    /* We don't hate ourselves, those we don't know or team members. */
    if (aplayer == pplayer
        || !aplayer->is_alive
        || NEVER_MET(pplayer, aplayer)
        || players_on_same_team(pplayer, aplayer)) {
      continue;
    }
    war_desire[player_index(aplayer)] = ai_war_desire(pplayer, aplayer, ai);
    if (war_desire[player_index(aplayer)] > best_desire) {
      best_desire = war_desire[player_index(aplayer)];
      best_target = aplayer;
    }
  } players_iterate_end;

  /* Time to make love. If we've been wronged, hold off that love
   * for a while. Also, cool our head each turn with love_coeff. */
  players_iterate(aplayer) {
    int a = player_number(aplayer);
    struct ai_dip_intel *adip = &ai->diplomacy.player_intel[a];
    int amount = 0;

    if (pplayer == aplayer || !aplayer->is_alive) {
      continue;
    }
    if ((pplayers_non_attack(pplayer, aplayer) 
         || pplayers_allied(pplayer, aplayer))
        && pplayer->diplstates[a].has_reason_to_cancel == 0
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
      pplayer->ai_data.love[player_index(aplayer)] += amount;
      DIPLO_LOG(LOG_DEBUG, pplayer, aplayer, "Increased love by %d", amount);
    } else if (WAR(pplayer, aplayer)) {
      amount -= ai->diplomacy.love_incr / 2;
      pplayer->ai_data.love[player_index(aplayer)] += amount;
      DIPLO_LOG(LOG_DEBUG, pplayer, aplayer, "%d love lost to war", amount);
    } else if (pplayer->diplstates[a].has_reason_to_cancel != 0) {
      /* Provoked in time of peace */
      if (pplayer->ai_data.love[player_index(aplayer)] > 0) {
        amount -= pplayer->ai_data.love[player_index(aplayer)] / 2;
      }
      amount -= ai->diplomacy.love_incr * 6;
      pplayer->ai_data.love[player_index(aplayer)] += amount;
      DIPLO_LOG(LOG_DEBUG, pplayer, aplayer, "Provoked! %d love lost!",
                amount);
    }
    if (pplayer->ai_data.love[player_index(aplayer)] > MAX_AI_LOVE * 8 / 10
        && !pplayers_allied(pplayer, aplayer)) {
      int amount = ai->diplomacy.love_incr / 3;

      /* Upper levels of AI trust and love is reserved for allies. */
      pplayer->ai_data.love[player_index(aplayer)] -= amount;
      DIPLO_LOG(LOG_DEBUG, pplayer, aplayer, "%d love lost from excess",
                amount);
    }
    amount = 0;

    /* Reduce love due to units in our territory.
     * AI is so naive, that we have to count it even if players are allied */
    amount -= MIN(player_in_territory(pplayer, aplayer) * (MAX_AI_LOVE / 200),
                  ai->diplomacy.love_incr 
                  * ((adip->is_allied_with_enemy != NULL) + 1));
    pplayer->ai_data.love[player_index(aplayer)] += amount;
    if (amount != 0) {
      DIPLO_LOG(LOG_DEBUG, pplayer, aplayer, "%d love lost due to units inside "
                "our borders", amount);
    }

    /* Increase the love if aplayer has got a building that makes 
     * us love him more. Typically it's Eiffel Tower */
    if (!NEVER_MET(pplayer, aplayer)) {
      pplayer->ai_data.love[player_index(aplayer)] +=
        get_player_bonus(aplayer, EFT_GAIN_AI_LOVE) * MAX_AI_LOVE / 1000;
    }
  } players_iterate_end;

  /* Can we win by space race? */
  if (ai->diplomacy.spacerace_leader == pplayer) {
    freelog(LOG_DIPL2, "%s going for space race victory!",
            player_name(pplayer));
    ai->diplomacy.strategy = WIN_SPACE; /* Yes! */
  } else {
    if (ai->diplomacy.strategy == WIN_SPACE) {
       ai->diplomacy.strategy = WIN_OPEN;
    }
  }

  players_iterate(aplayer) {
    int *love = &pplayer->ai_data.love[player_index(aplayer)];

    if (aplayer == best_target && best_desire > 0) {
      int reduction = MIN(best_desire, MAX_AI_LOVE / 20);

      *love -= reduction;
      DIPLO_LOG(LOG_DEBUG, pplayer, aplayer, "Wants war, reducing "
                "love by %d ", reduction);
    }

    /* Massage our numbers to keep love and its opposite on the ground.
     * Gravitate towards zero. */
    *love -= *love * (ai->diplomacy.love_coeff / 100);

    /* ai love should always be in range [-MAX_AI_LOVE..MAX_AI_LOVE] */
    *love = MAX(-MAX_AI_LOVE, MIN(MAX_AI_LOVE, *love));
  } players_iterate_end;
}

/********************************************************************** 
  Find two techs that can be exchanged and suggest that
***********************************************************************/
static void suggest_tech_exchange(struct player* player1,
                                  struct player* player2)
{
  int worth[advance_count()];
  bool is_dangerous;
    
  worth[A_NONE] = 0;

  advance_index_iterate(A_FIRST, tech) {
    if (player_invention_state(player1, tech) == TECH_KNOWN) {
      if (player_invention_state(player2, tech) != TECH_KNOWN
          && player_invention_reachable(player2, tech)) {
        worth[tech] = -compute_tech_sell_price(player1, player2, tech,
	                                       &is_dangerous);
	if (is_dangerous) {
	  /* don't try to exchange */
	  worth[tech] = 0;
	}
      } else {
        worth[tech] = 0;
      }
    } else {
      if (player_invention_state(player2, tech) == TECH_KNOWN
          && player_invention_reachable(player1, tech)) {
        worth[tech] = compute_tech_sell_price(player2, player1, tech,
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
        ai_diplomacy_suggest(player1, player2, CLAUSE_ADVANCE, tech2);
	ai_diplomacy_suggest(player2, player1, CLAUSE_ADVANCE, tech);
	if (diff > 0) {
	  ai_diplomacy_suggest(player1, player2, CLAUSE_GOLD, diff);
	} else if (diff < 0) {
	  ai_diplomacy_suggest(player2, player1, CLAUSE_GOLD, -diff);
	}
	return;
      }
    } advance_index_iterate_end;
  } advance_index_iterate_end;
}

/********************************************************************** 
  Offer techs and stuff to other player and ask for techs we need.
***********************************************************************/
static void ai_share(struct player *pplayer, struct player *aplayer)
{
  bool gives_vision;

  /* Only share techs with team mates */
  if (players_on_same_team(pplayer, aplayer)) {
    advance_index_iterate(A_FIRST, index) {
      if ((player_invention_state(pplayer, index) != TECH_KNOWN)
          && (player_invention_state(aplayer, index) == TECH_KNOWN)
          && player_invention_reachable(pplayer, index)) {
       ai_diplomacy_suggest(aplayer, pplayer, CLAUSE_ADVANCE, index);
      } else if ((player_invention_state(pplayer, index) == TECH_KNOWN)
                 && (player_invention_state(aplayer, index) != TECH_KNOWN)
                 && player_invention_reachable(aplayer, index)) {
        ai_diplomacy_suggest(pplayer, aplayer, CLAUSE_ADVANCE, index);
      }
    } advance_index_iterate_end;
  }

  /* Only give shared vision if safe. Only ask for shared vision if fair. */
  gives_vision = gives_shared_vision(pplayer, aplayer);
  if (!gives_vision
      && shared_vision_is_safe(pplayer, aplayer)) {
    ai_diplomacy_suggest(pplayer, aplayer, CLAUSE_VISION, 0);
    gives_vision = TRUE;
  }
  if (gives_vision
      && !gives_shared_vision(aplayer, pplayer)
      && (!aplayer->ai_data.control
          || shared_vision_is_safe(aplayer, pplayer))) {
    ai_diplomacy_suggest(aplayer, pplayer, CLAUSE_VISION, 0);
  }

  if (!player_has_embassy(pplayer, aplayer)) {
    ai_diplomacy_suggest(aplayer, pplayer, CLAUSE_EMBASSY, 0);
  }
  if (!player_has_embassy(aplayer, pplayer)) {
    ai_diplomacy_suggest(pplayer, aplayer, CLAUSE_EMBASSY, 0);
  }
  
  if (!ai_handicap(pplayer, H_DIPLOMACY) || !aplayer->ai_data.control) {
    suggest_tech_exchange(pplayer, aplayer);
  }
}

/********************************************************************** 
  Go to war.  Explain to target why we did it, and set countdown to
  some negative value to make us a bit stubborn to avoid immediate
  reversal to ceasefire.
***********************************************************************/
static void ai_go_to_war(struct player *pplayer, struct ai_data *ai,
                         struct player *target, enum war_reason reason)
{
  struct ai_dip_intel *adip = &ai->diplomacy.player_intel[player_index(target)];

  assert(pplayer != target);
  assert(target->is_alive);

  switch (reason) {
  case WAR_REASON_SPACE:
    notify(target, _("*%s (AI)* Space will never be yours. "),
           player_name(pplayer));
    adip->countdown = -10;
    break;
  case WAR_REASON_BEHAVIOUR:
    notify(target, _("*%s (AI)* I have tolerated your vicious antics "
           "long enough! To war!"),
           player_name(pplayer));
    adip->countdown = -20;
    break;
  case WAR_REASON_NONE:
    notify(target, _("*%s (AI)* Peace in ... some other time."),
           player_name(pplayer));
    adip->countdown = -10;
    break;
  case WAR_REASON_HATRED:
    notify(target, _("*%s (AI)* Finally I get around to you! Did "
           "you really think you could get away with your crimes?"),
           player_name(pplayer));
    adip->countdown = -20;
    break;
  case WAR_REASON_EXCUSE:
    notify(target, _("*%s (AI)* Your covert hostilities brought "
           "this war upon you!"),
           player_name(pplayer));
    adip->countdown = -20;
    break;
  case WAR_REASON_ALLIANCE:
    if (adip->at_war_with_ally) {
      notify(target, _("*%s (AI)* Your aggression against %s was "
			"your last mistake!"),
	     player_name(pplayer),
	     player_name(adip->at_war_with_ally));
      adip->countdown = -3;
    } else {
      /* Ooops! */
      DIPLO_LOG(LOG_DIPL, pplayer, target, "Wanted to declare war "
                "for his war against an ally, but can no longer find "
                "this ally!  War declaration aborted.");
      adip->countdown = -1;
      return;
    }
    break;
  }

  assert(adip->countdown < 0);

  if (gives_shared_vision(pplayer, target)) {
    remove_shared_vision(pplayer, target);
  }

  /* Check for Senate obstruction.  If so, dissolve it. */
  if (pplayer_can_cancel_treaty(pplayer, target) == DIPL_SENATE_BLOCKING) {
    handle_player_change_government(pplayer, 
                                    game.info.government_during_revolution_id);
  }

  /* This will take us straight to war. */
  while (pplayer_get_diplstate(pplayer, target)->type != DS_WAR) {
    if (pplayer_can_cancel_treaty(pplayer, target) != DIPL_OK) {
      DIPLO_LOG(LOG_ERROR, pplayer, target, "Wanted to cancel treaty but "
                "was unable to.");
      return;
    }
    handle_diplomacy_cancel_pact(pplayer, player_number(target), CLAUSE_LAST);
  }

  /* Throw a tantrum */
  if (pplayer->ai_data.love[player_index(target)] > 0) {
    pplayer->ai_data.love[player_index(target)] = -1;
  }
  pplayer->ai_data.love[player_index(target)] -= MAX_AI_LOVE / 8;

  assert(!gives_shared_vision(pplayer, target));
  DIPLO_LOG(LOG_DIPL, pplayer, target, "war declared");
}

/********************************************************************** 
  Do diplomatic actions. Must be called only after calculate function
  above has been run for _all_ AI players.

  Only ever called for AI players and never for barbarians.

  When the adip->coundown variable is set to a positive value, it 
  counts down to a declaration of war. When it is set to a value 
  smaller than -1, it is a countup towards a "neutral" stance of -1,
  in which time the AI will refuse to make treaties. This is to make
  the AI more stubborn.
***********************************************************************/
void static war_countdown(struct player *pplayer, struct player *target,
                          int countdown, enum war_reason reason)
{
  struct ai_data *ai = ai_data_get(pplayer);
  struct ai_dip_intel *adip = &ai->diplomacy.player_intel[player_index(target)];

  DIPLO_LOG(LOG_DIPL, pplayer, target, "countdown to war in %d", countdown);

  /* Otherwise we're resetting an existing countdown, which is very bad */
  assert(adip->countdown == -1);

  adip->countdown = countdown;
  adip->war_reason = reason;

  players_iterate(ally) {
    if (!pplayers_allied(pplayer, ally) 
        || !ally->is_alive
        || ally == target
        || NEVER_MET(pplayer, ally)) {
      continue;
    }

    switch (reason) {
    case WAR_REASON_SPACE:
      notify(ally, PL_("*%s (AI)* We will be launching an all-out war "
		       "against %s in %d turn to stop the spaceship "
		       "launch.",
		       "*%s (AI)* We will be launching an all-out war "
		       "against %s in %d turns to stop the spaceship "
		       "launch.",
		       countdown),
	     player_name(pplayer),
	     player_name(target),
	     countdown);
      notify(ally, _("*%s (AI)* Your aid in this matter will be expected. "
                     "Long live our glorious alliance!"),
             player_name(pplayer));
      break;
    case WAR_REASON_BEHAVIOUR:
    case WAR_REASON_EXCUSE:
      notify(ally, PL_("*%s (AI)* %s has grossly violated his treaties "
		       "with us for own gain.  We will answer in force in "
		       "%d turn and expect you to honour your alliance "
		       "with us and do likewise!",
		       "*%s (AI)* %s has grossly violated his treaties "
		       "with us for own gain.  We will answer in force in "
		       "%d turns and expect you to honour your alliance "
		       "with us and do likewise!", countdown),
	     player_name(pplayer),
	     player_name(target),
	     countdown);
      break;
    case WAR_REASON_NONE:
      notify(ally, PL_("*%s (AI)* We intend to pillage and plunder the rich "
		       "civilization of %s. We declare war in %d turn.",
		       "*%s (AI)* We intend to pillage and plunder the rich "
		       "civilization of %s. We declare war in %d turns.",
		       countdown), 
	     player_name(pplayer),
	     player_name(target),
	     countdown);
      notify(ally, _("*%s (AI)* If you want a piece of the loot, feel "
                     "free to join in the action!"),
             player_name(pplayer));
      break;
    case WAR_REASON_HATRED:
      notify(ally, PL_("*%s (AI)* We have had it with %s. Let us tear this "
		       "pathetic civilization apart. We declare war in "
		       "%d turn.",
		       "*%s (AI)* We have had it with %s. Let us tear this "
		       "pathetic civilization apart. We declare war in "
		       "%d turns.",
		       countdown),
	     player_name(pplayer),
	     player_name(target),
	     countdown);
      notify(ally, _("*%s (AI)* As our glorious allies, we expect your "
                     "help in this war."),
             player_name(pplayer));
      break;
    case WAR_REASON_ALLIANCE:
      if (WAR(ally, target)) {
        notify(ally, PL_("*%s (AI)* We will honour our alliance and declare "
			 "war on %s in %d turn.  Hold on - we are coming!",
			 "*%s (AI)* We will honour our alliance and declare "
			 "war on %s in %d turns.  Hold on - we are coming!",
			 countdown),
	       player_name(pplayer),
	       player_name(target),
	       countdown);
      } else if (adip->at_war_with_ally) {
        notify(ally, PL_("*%s (AI)* We will honour our alliance with %s and "
			 "declare war on %s in %d turns.  We expect you to "
			 "do likewise.",
			 "*%s (AI)* We will honour our alliance with %s and "
			 "declare war on %s in %d turns.  We expect you to "
			 "do likewise.",
			 countdown),
	       player_name(pplayer), 
	       player_name(adip->at_war_with_ally),
	       player_name(target),
	       countdown);
      } else {
        assert(FALSE); /* Huh? */
      }
      break;
    }
  } players_iterate_end;
}

/********************************************************************** 
  Do diplomatic actions. Must be called only after calculate function
  above has been run for _all_ AI players.

  Only ever called for AI players and never for barbarians.
***********************************************************************/
void ai_diplomacy_actions(struct player *pplayer)
{
  struct ai_data *ai = ai_data_get(pplayer);
  bool need_targets = TRUE;
  struct player *target = NULL;
  int most_hatred = MAX_AI_LOVE;

  assert(pplayer->ai_data.control);
  if (!pplayer->is_alive) {
    return;
  }

  /*** If we are greviously insulted, go to war immediately. ***/

  players_iterate(aplayer) {
    if (pplayer->ai_data.love[player_index(aplayer)] < 0
        && pplayer->diplstates[player_index(aplayer)].has_reason_to_cancel >= 2
        && ai->diplomacy.player_intel[player_index(aplayer)].countdown == -1) {
      DIPLO_LOG(LOG_DIPL2, pplayer, aplayer, "Plans war in revenge");
      war_countdown(pplayer, aplayer, map.server.size, WAR_REASON_BEHAVIOUR);
    }
  } players_iterate_end;

  /*** Stop other players from winning by space race ***/

  if (ai->diplomacy.strategy != WIN_SPACE) {
    players_iterate(aplayer) {
      struct ai_dip_intel *adip =
                         &ai->diplomacy.player_intel[player_index(aplayer)];
      struct player_spaceship *ship = &aplayer->spaceship;

      if (!aplayer->is_alive 
          || aplayer == pplayer
          || adip->countdown >= 0  /* already counting down to war */
          || ship->state == SSHIP_NONE
          || players_on_same_team(pplayer, aplayer)
          || pplayers_at_war(pplayer, aplayer)) {
        continue;
      }
      /* A spaceship victory is always one single player's or team's victory */
      if (aplayer->spaceship.state == SSHIP_LAUNCHED
          && ai->diplomacy.spacerace_leader == aplayer
          && pplayers_allied(pplayer, aplayer)) {
        notify(aplayer, _("*%s (AI)* Your attempt to conquer space for "
               "yourself alone betray your true intentions, and I "
               "will have no more of our alliance!"),
               player_name(pplayer));
	handle_diplomacy_cancel_pact(pplayer, player_number(aplayer),
				     CLAUSE_ALLIANCE);
        if (gives_shared_vision(pplayer, aplayer)) {
          remove_shared_vision(pplayer, aplayer);
        }
        /* Never forgive this */
        pplayer->ai_data.love[player_index(aplayer)] = -(BIG_NUMBER);
      } else if (ship->state == SSHIP_STARTED 
		 && adip->warned_about_space == 0) {
        pplayer->ai_data.love[player_index(aplayer)] -= MAX_AI_LOVE / 10;
        adip->warned_about_space = 10 + myrand(6);
        notify(aplayer, _("*%s (AI)* Your attempt to unilaterally "
               "dominate outer space is highly offensive."),
               player_name(pplayer));
        notify(aplayer, _("*%s (AI)* If you do not stop constructing your "
               "spaceship, I may be forced to take action!"),
               player_name(pplayer));
      }
      if (aplayer->spaceship.state == SSHIP_LAUNCHED
          && aplayer == ai->diplomacy.spacerace_leader) {
        /* This means war!!! */
        pplayer->ai_data.love[player_index(aplayer)] -= MAX_AI_LOVE / 2;
        DIPLO_LOG(LOG_DIPL, pplayer, aplayer, "plans war due to spaceship");
        war_countdown(pplayer, aplayer, 4 + map.server.size, WAR_REASON_SPACE);
      }
    } players_iterate_end;
  }

  /*** Declare war against somebody if we are out of targets ***/

  players_iterate(aplayer) {
    int turns; /* turns since contact */

    if (NEVER_MET(pplayer, aplayer)) {
      continue;
    }
    turns = game.info.turn;
    turns -= pplayer_get_diplstate(pplayer, aplayer)->first_contact_turn;
    if (aplayer->is_alive 
        && WAR(pplayer, aplayer)) {
      need_targets = FALSE;
    } else if (aplayer->is_alive
               && pplayer->ai_data.love[player_index(aplayer)] < most_hatred
               && turns > TURNS_BEFORE_TARGET) {
      most_hatred = pplayer->ai_data.love[player_index(aplayer)];
      target = aplayer;
    }
  } players_iterate_end;
  if (need_targets && target && most_hatred < WAR_THRESHOLD
      && ai->diplomacy.player_intel[player_index(target)].countdown == -1) {
    enum war_reason war_reason;

    if (pplayers_allied(pplayer, target)) {
      DIPLO_LOG(LOG_DEBUG, pplayer, target, "Plans war against an ally!");
    }
    if (pplayer->diplstates[player_index(target)].has_reason_to_cancel > 0) {
      /* We have good reason */
      war_reason = WAR_REASON_EXCUSE;
    } if (pplayer->ai_data.love[player_index(target)] < 0) {
      /* We have a reason of sorts from way back, maybe? */
      war_reason = WAR_REASON_HATRED;
    } else {
      /* We have no legimitate reason... So what? */
      war_reason = WAR_REASON_NONE;
    }
    DIPLO_LOG(LOG_DEBUG, pplayer, target, "plans war for spoils");
    war_countdown(pplayer, target, 4 + map.server.size, war_reason);
  }

  /*** Declare war - against enemies of allies ***/

  players_iterate(aplayer) {
    struct ai_dip_intel *adip = &ai->diplomacy.player_intel[player_index(aplayer)];

    if (aplayer->is_alive
        && adip->at_war_with_ally
        && adip->countdown == -1
        && !adip->is_allied_with_ally
        && !pplayers_at_war(pplayer, aplayer)
	&& (pplayer_get_diplstate(pplayer, aplayer)->type != DS_CEASEFIRE || 
	    myrand(5) < 1)) {
      DIPLO_LOG(LOG_DEBUG, pplayer, aplayer, "plans war to help ally %s",
                player_name(adip->at_war_with_ally));
      war_countdown(pplayer, aplayer, 2 + map.server.size, WAR_REASON_ALLIANCE);
    }
  } players_iterate_end;

  /*** Actually declare war (when we have moved units into position) ***/

  players_iterate(aplayer) {
    struct ai_dip_intel *adip = 
                        &ai->diplomacy.player_intel[player_index(aplayer)];

    if (!aplayer->is_alive) {
      adip->countdown = -1;
      continue;
    }
    if (adip->countdown > 0) {
      adip->countdown--;
    } else if (adip->countdown == 0) {
      if (!WAR(pplayer, aplayer)) {
        DIPLO_LOG(LOG_DIPL2, pplayer, aplayer, "Declaring war!");
        ai_go_to_war(pplayer, ai, aplayer, adip->war_reason);
      }
    } else if (adip->countdown < -1) {
      /* negative countdown less than -1 is war stubbornness */
      adip->countdown++;
    }
  } players_iterate_end;

  /*** Try to make peace with everyone we love ***/

  players_iterate(aplayer) {
    enum diplstate_type ds = pplayer_get_diplstate(pplayer, aplayer)->type;
    struct ai_dip_intel *adip = &ai->diplomacy.player_intel[player_index(aplayer)];
    struct Clause clause;

    /* Meaningless values, but rather not have them unset. */
    clause.from = pplayer;
    clause.value = 0;

    /* Remove shared vision if we are not allied or it is no longer safe. */
    if (gives_shared_vision(pplayer, aplayer)) {
      if (!pplayers_allied(pplayer, aplayer)) {
        remove_shared_vision(pplayer, aplayer);
      } else if (!shared_vision_is_safe(pplayer, aplayer)) {
        notify(aplayer, _("*%s (AI)* Sorry, sharing vision with you "
	                    "is no longer safe."),
	       player_name(pplayer));
	remove_shared_vision(pplayer, aplayer);
      }
    }

    /* No peace to enemies of our allies... or pointless peace. */
    if (is_barbarian(aplayer)
        || aplayer == pplayer
        || aplayer == target     /* no mercy */
        || !aplayer->is_alive
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
    if (adip->spam > 0) {
      /* Don't spam */
      continue;
    }

    /* Canvass support from existing friends for our war, and try to
     * make friends with enemies. Then we wait some turns until next time
     * we spam them with our gibbering chatter. */
    if (!aplayer->ai_data.control) {
      if (!pplayers_allied(pplayer, aplayer)) {
        adip->spam = myrand(4) + 3; /* Bugger allies often. */
      } else {
        adip->spam = myrand(8) + 6; /* Others are less important. */
      }
    }

    switch (ds) {
    case DS_TEAM:
      ai_share(pplayer, aplayer);
      break;
    case DS_ALLIANCE:
      /* See if our allies are diligently declaring war on our enemies... */
      if (adip->at_war_with_ally) {
        break;
      }
      target = NULL;
      players_iterate(eplayer) {
        int e = player_number(eplayer);

        /* Read the countdown check below carefully... Note that we check
         * our ally's intentions directly here. */
        if (WAR(pplayer, eplayer)
            && (ai_data_get(aplayer)->diplomacy.player_intel[e].countdown 
                == -1)
                && !pplayers_at_war(aplayer, eplayer)) {
          target = eplayer;
          break;
        }
      } players_iterate_end;

      if ((players_on_same_team(pplayer, aplayer)
          || pplayer->ai_data.love[player_index(aplayer)] > MAX_AI_LOVE / 2)) {
        /* Share techs only with team mates and allies we really like. */
        ai_share(pplayer, aplayer);
      }
      if (!target || !target->is_alive) {
        adip->ally_patience = 0;
        break; /* No need to nag our ally */
      }

      switch (adip->ally_patience--) {
        case 0:
          notify(aplayer, _("*%s (AI)* Greetings our most trustworthy "
                 "ally. We call upon you to destroy our enemy, %s."), 
                 player_name(pplayer),
                 player_name(target));
          break;
        case -1:
          notify(aplayer, _("*%s (AI)* Greetings ally, I see you have not yet "
                 "made war with our enemy, %s. Why do I need to remind "
                 "you of your promises?"),
                 player_name(pplayer),
                 player_name(target));
          break;
        case -2:
          notify(aplayer, _("*%s (AI)* Dishonoured one, we made a pact of "
                 "alliance, and yet you remain at peace with our mortal "
                 "enemy, %s! This is unacceptable; our alliance is no "
                 "more!"),
                 player_name(pplayer),
                 player_name(target));
          DIPLO_LOG(LOG_DIPL2, pplayer, aplayer, "breaking useless alliance");
	  /* to peace */
	  handle_diplomacy_cancel_pact(pplayer, player_number(aplayer),
				       CLAUSE_ALLIANCE);
          pplayer->ai_data.love[player_index(aplayer)] =
            MIN(pplayer->ai_data.love[player_index(aplayer)], 0);
          if (gives_shared_vision(pplayer, aplayer)) {
            remove_shared_vision(pplayer, aplayer);
          }
          assert(!gives_shared_vision(pplayer, aplayer));
          break;
      }
      break;

    case DS_PEACE:
      clause.type = CLAUSE_ALLIANCE;
      if (adip->at_war_with_ally
          || (!aplayer->ai_data.control && adip->asked_about_alliance > 0)
          || ai_goldequiv_clause(pplayer, aplayer, &clause,
                                 ai, FALSE, DS_ALLIANCE) < 0) {
        break; 
      }
      ai_diplomacy_suggest(pplayer, aplayer, CLAUSE_ALLIANCE, 0);
      adip->asked_about_alliance = !aplayer->ai_data.control ? 13 : 0;
      notify(aplayer, _("*%s (AI)* Greetings friend, may we suggest "
             "making a common cause and join in an alliance?"), 
             player_name(pplayer));
      break;

    case DS_CEASEFIRE:
      clause.type = CLAUSE_PEACE;
      if (adip->at_war_with_ally
          || (!aplayer->ai_data.control && adip->asked_about_peace > 0)
          || ai_goldequiv_clause(pplayer, aplayer, &clause,
                                 ai, FALSE, DS_PEACE) < 0) {
        break;
      }
      ai_diplomacy_suggest(pplayer, aplayer, CLAUSE_PEACE, 0);
      adip->asked_about_peace = !aplayer->ai_data.control ? 12 : 0;
      notify(aplayer, _("*%s (AI)* Greetings neighbour, may we suggest "
             "more peaceful relations?"),
             player_name(pplayer));
      break;

    case DS_NO_CONTACT: /* but we do have embassy! weird. */
    case DS_WAR:
      clause.type = CLAUSE_CEASEFIRE;
      if ((!aplayer->ai_data.control && adip->asked_about_ceasefire > 0)
          || ai_goldequiv_clause(pplayer, aplayer, &clause,
                                 ai, FALSE, DS_CEASEFIRE) < 0) {
        break; /* Fight until the end! */
      }
      ai_diplomacy_suggest(pplayer, aplayer, CLAUSE_CEASEFIRE, 0);
      adip->asked_about_ceasefire = !aplayer->ai_data.control ? 9 : 0;
      notify(aplayer, _("*%s (AI)* We grow weary of this constant "
             "bloodshed. May we suggest a cessation of hostilities?"), 
             player_name(pplayer));
      break;

    case DS_ARMISTICE:
      break;
    default:
      die("Unknown pact type");
      break;
    }
  } players_iterate_end;
}

/********************************************************************** 
  Are we going to be declaring war in a few turns time?  If so, go
  on a war footing, and try to buy out as many units as possible.
***********************************************************************/
bool ai_on_war_footing(struct player *pplayer)
{
  struct ai_data *ai = ai_data_get(pplayer);

  players_iterate(plr) {
    if (ai->diplomacy.player_intel[player_index(plr)].countdown >= 0) {
      return TRUE;
    }
  } players_iterate_end;

  return FALSE;
}

/********************************************************************** 
  Handle incident caused by violator
***********************************************************************/
/* AI attitude call-backs */
void ai_incident(enum incident_type type, struct player *violator,
                 struct player *victim)
{
  switch(type) {
    case INCIDENT_DIPLOMAT:
      ai_incident_diplomat(violator, victim);
      break;
    case INCIDENT_WAR:
      ai_incident_war(violator, victim);
      break;
    case INCIDENT_PILLAGE:
      ai_incident_pillage(violator, victim);
      break;
    case INCIDENT_NUCLEAR:
      ai_incident_nuclear(violator, victim);
      break;
    case INCIDENT_LAST:
      /* Assert that always fails, but with meaningfull message */
      assert(type != INCIDENT_LAST);
      break;
  }
}

/********************************************************************** 
  Nuclear strike. Victim is whoever's territory was hit, and may be
  NULL.
***********************************************************************/
static void ai_incident_nuclear(struct player *violator, struct player *victim)
{
  if (violator == victim) {
    players_iterate(pplayer) {
      if (!pplayer->ai_data.control) {
        continue;
      }

      if (pplayer != violator) {
        pplayer->ai_data.love[player_index(violator)] -= MAX_AI_LOVE / 20;
      }
    } players_iterate_end;
    return;
  } else {
    players_iterate(pplayer) {
      if (!pplayer->ai_data.control) {
        continue;
      }

      if (pplayer != violator) {
        pplayer->ai_data.love[player_index(violator)] -= MAX_AI_LOVE / 10;
        if (victim == pplayer) {
          pplayer->ai_data.love[player_index(violator)] -= MAX_AI_LOVE / 10;
        }
      }
    } players_iterate_end;
  }
}

/********************************************************************** 
  Diplomat caused an incident.
***********************************************************************/
static void ai_incident_diplomat(struct player *violator, struct player *victim)
{
  players_iterate(pplayer) {
    if (!pplayer->ai_data.control) {
      continue;
    }

    if (pplayer != violator) {
      /* Dislike backstabbing bastards */
      pplayer->ai_data.love[player_index(violator)] -= MAX_AI_LOVE / 100;
      if (victim == pplayer) {
        pplayer->ai_data.love[player_index(violator)] -= MAX_AI_LOVE / 7;
      }
    }
  } players_iterate_end;
}

/********************************************************************** 
  War declared against a player.  We apply a penalty because this
  means he is seen as untrustworthy, especially if past relations
  with the victim have been cordial (betrayal).

  Reasons for war and other mitigating circumstances are checked
  in calling code.
***********************************************************************/
static void ai_incident_war(struct player *violator, struct player *victim)
{
  players_iterate(pplayer) {
    if (!pplayer->ai_data.control) {
      continue;
    }

    if (pplayer != violator) {
      /* Dislike backstabbing bastards */
      pplayer->ai_data.love[player_index(violator)] -= MAX_AI_LOVE / 30;
      if (violator->diplstates[player_index(victim)].max_state == DS_PEACE) {
        /* Extra penalty if they once had a peace treaty */
        pplayer->ai_data.love[player_index(violator)] -= MAX_AI_LOVE / 30;
      } else if (violator->diplstates[player_index(victim)].max_state 
                 == DS_ALLIANCE) {
        /* Extra penalty if they once had an alliance */
        pplayer->ai_data.love[player_index(violator)] -= MAX_AI_LOVE / 10;
      }
      if (victim == pplayer) {
        pplayer->ai_data.love[player_index(violator)] = 
          MIN(pplayer->ai_data.love[player_index(violator)] - MAX_AI_LOVE / 3, -1);
        /* Scream for help!! */
        players_iterate(ally) {
          if (!pplayers_allied(pplayer, ally) || !ally->is_alive) {
            continue;
          }
          notify(ally, _("*%s (AI)* We have been savagely attacked by "
                         "%s, and we need your help! Honour our glorious "
                         "alliance and your name will never be forgotten!"),
                 player_name(victim),
                 player_name(violator));
        } players_iterate_end;
      }
    }
  } players_iterate_end;
}

/***************************************************************************
  Violator pillaged something on victims territory
***************************************************************************/
static void ai_incident_pillage(struct player *violator, struct player *victim)
{
  if (violator == victim) {
    return;
  }
  if (victim == NULL) {
    return;
  }
  victim->ai_data.love[player_index(violator)] -= MAX_AI_LOVE / 20;
}
