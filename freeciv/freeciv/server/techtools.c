/***********************************************************************
 Freeciv - Copyright (C) 2005 The Freeciv Team
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

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "shared.h"
#include "support.h"

/* common */
#include "game.h"
#include "government.h"
#include "movement.h"
#include "player.h"
#include "research.h"
#include "tech.h"
#include "unit.h"

/* common/scriptcore */
#include "luascript_types.h"

/* server */
#include "citytools.h"
#include "cityturn.h"
#include "connecthand.h"
#include "gamehand.h"
#include "maphand.h"
#include "notify.h"
#include "plrhand.h"
#include "unittools.h"

/* server/scripting */
#include "script_server.h"

#include "techtools.h"

/* Define this to add information about tech upkeep. */
#undef TECH_UPKEEP_DEBUGGING

static Tech_type_id
pick_random_tech_to_lose(const struct research *presearch);
static Tech_type_id pick_random_tech(const struct research *presearch);
static Tech_type_id pick_cheapest_tech(const struct research *presearch);
static void research_tech_lost(struct research *presearch,
                               Tech_type_id tech);
static void forget_tech_transfered(struct player *pplayer, Tech_type_id tech);

/************************************************************************//**
  Apply a penalty to the research.
****************************************************************************/
void research_apply_penalty(struct research *presearch, Tech_type_id tech,
                            int penalty_percent)
{
  if (game.server.multiresearch) {
    return;
  }
  presearch->bulbs_researched -=
      (research_total_bulbs_required(presearch, tech, FALSE)
       * penalty_percent) / 100;
  presearch->researching_saved = A_UNKNOWN;
}

/************************************************************************//**
  Emit script signal(s) for player/team learning new tech.
  originating_plr is the player whose action caused this; may be NULL, and
  is only used to order the emission of the signals.
****************************************************************************/
void script_tech_learned(struct research *presearch,
                         struct player *originating_plr, struct advance *tech,
                         const char *reason)
{
  /* Emit signal for individual player whose action triggered the
   * tech first */
  if (originating_plr) {
    fc_assert(research_get(originating_plr) == presearch);
    script_server_signal_emit("tech_researched", tech, originating_plr,
                              reason);
  }

  /* Emit signal to remaining research teammates, if any */
  research_players_iterate(presearch, member) {
    if (member != originating_plr) {
      script_server_signal_emit("tech_researched", tech, member, reason);
    }
  } research_players_iterate_end;
}

/************************************************************************//**
  Players have researched a new technology.
****************************************************************************/
static void tech_researched(struct research *research)
{
  char research_name[MAX_LEN_NAME * 2];
  /* Cache researched technology for event signal, because found_new_tech()
   * changes the research target. */
  Tech_type_id tech = research->researching;

  research_pretty_name(research, research_name, sizeof(research_name));
  /* Players will be notified when new tech is chosen. */
  notify_research_embassies
      (research, NULL, E_TECH_EMBASSY, ftc_server,
       _("The %s have researched %s."),
       research_name,
       research_advance_name_translation(research, tech));

  /* Deduct tech cost. */
  research->bulbs_researched -= research_total_bulbs_required(research, tech,
                                                              FALSE);
  advance_index_iterate(A_FIRST, j) {
    if (j == research->researching) {
      research->inventions[j].bulbs_researched_saved
                  = research_total_bulbs_required(research, tech, FALSE);
    }
  } advance_index_iterate_end;


  /* Do all the updates needed after finding new tech. */
  found_new_tech(research, tech, TRUE, TRUE);

  script_tech_learned(research, NULL, advance_by_number(tech), "researched");
}

/************************************************************************//**
  Give technologies to players with EFT_TECH_PARASITE (traditionally from
  the Great Library).
****************************************************************************/
void do_tech_parasite_effect(struct player *pplayer)
{
  struct effect_list *plist = effect_list_new();
  struct astring effects;
  struct research *presearch;
  char research_name[MAX_LEN_NAME * 2];
  const char *advance_name;
  Tech_type_id tech;
  /* Note that two EFT_TECH_PARASITE effects will combine into a single,
   * much worse effect. */
  int mod = get_player_bonus_effects(plist, pplayer, EFT_TECH_PARASITE);
  int num_players;
  int num_techs;

  if (mod <= 0) {
    /* No effect. */
    effect_list_destroy(plist);
    return;
  }

  /* Pick a random technology. */
  tech = A_UNSET;
  num_techs = 0;
  presearch = research_get(pplayer);
  advance_index_iterate(A_FIRST, i) {
    if (!research_invention_gettable(presearch, i,
                                     game.info.tech_parasite_allow_holes)
        || TECH_KNOWN == research_invention_state(presearch, i)) {
      continue;
    }

    num_players = 0;
    players_iterate(aplayer) {
      if (TECH_KNOWN == research_invention_state(research_get(aplayer), i)) {
        if (mod <= ++num_players) {
          if (0 == fc_rand(++num_techs)) {
            tech = i;
          }
          break;
        }
      }
    } players_iterate_end;
  } advance_index_iterate_end;

  if (A_UNSET == tech) {
    /* No tech found. */
    effect_list_destroy(plist);
    return;
  }

  /* Notify. */
  research_pretty_name(presearch, research_name, sizeof(research_name));
  advance_name = research_advance_name_translation(presearch, tech);
  astr_init(&effects);
  get_effect_list_req_text(plist, &effects);

  notify_player(pplayer, NULL, E_TECH_GAIN, ftc_server,
                /* TRANS: Tech from source of an effect
                 * (Great Library) */
                Q_("?fromeffect:%s acquired from %s!"),
                advance_name,
                astr_str(&effects));
  notify_research(presearch, pplayer, E_TECH_GAIN, ftc_server,
                  /* TRANS: Tech from source of an effect
                   * (Great Library) */
                  Q_("?fromeffect:%s acquired from %s's %s!"),
                  advance_name,
                  player_name(pplayer),
                  astr_str(&effects));
  notify_research_embassies(presearch, NULL, E_TECH_EMBASSY, ftc_server,
                            /* TRANS: Tech from source of an effect
                             * (Great Library) */
                            Q_("?fromeffect:The %s have acquired %s from %s."),
                            research_name,
                            advance_name,
                            astr_str(&effects));

  effect_list_destroy(plist);
  astr_free(&effects);

  /* Really get tech. */
  research_apply_penalty(presearch, tech, game.server.freecost);
  found_new_tech(presearch, tech, FALSE, TRUE);

  research_players_iterate(presearch, member) {
    script_server_signal_emit("tech_researched", advance_by_number(tech),
                              member, "stolen");
  } research_players_iterate_end;
}

/************************************************************************//**
  Fill packet fields. Helper for following functions.
****************************************************************************/
static inline void
package_research_info(struct packet_research_info *packet,
                      const struct research *presearch)
{
  packet->id = research_number(presearch);
  packet->techs_researched = presearch->techs_researched;
  packet->future_tech = presearch->future_tech;
  packet->researching = presearch->researching;
  packet->researching_cost =
      (packet->researching != A_UNSET
       ? research_total_bulbs_required(presearch, presearch->researching,
                                       FALSE) : 0);
  packet->bulbs_researched = presearch->bulbs_researched;
  packet->tech_goal = presearch->tech_goal;
  packet->total_bulbs_prod = 0;
  research_players_iterate(presearch, pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      packet->total_bulbs_prod += pcity->surplus[O_SCIENCE];
    } city_list_iterate_end;
  } research_players_iterate_end;
  advance_index_iterate(A_NONE, i) {
    packet->inventions[i] = presearch->inventions[i].state + '0';
  } advance_index_iterate_end;
  packet->inventions[advance_count()] = '\0';
  packet->tech_goal = presearch->tech_goal;
#ifdef FREECIV_DEBUG
  log_verbose("Research nb %d inventions: %s",
              research_number(presearch),
              packet->inventions);
#endif /* FREECIV_DEBUG */
}

/************************************************************************//**
  Send research info for 'presearch' to 'dest'. 'dest' can be NULL to send
  to all established connections.
****************************************************************************/
void send_research_info(const struct research *presearch,
                        const struct conn_list *dest)
{
  struct packet_research_info full_info, restricted_info;
  const struct player *pplayer;

  fc_assert_ret(NULL != presearch);
  if (NULL == dest) {
    dest = game.est_connections;
  }

  /* Packaging */
  package_research_info(&full_info, presearch);
  restricted_info = full_info;
  restricted_info.tech_goal = A_UNSET;
  restricted_info.total_bulbs_prod = 0;

  conn_list_iterate(dest, pconn) {
    pplayer = conn_get_player(pconn);
    if (NULL != pplayer) {
      if (presearch == research_get(pplayer)) {
        /* Case research owner. */
        send_packet_research_info(pconn, &full_info);
      } else {
        /* 'pplayer' may have an embassy for looking to 'presearch'. */
        research_players_iterate(presearch, powner) {
          if (player_has_embassy(pplayer, powner)) {
            send_packet_research_info(pconn, &restricted_info);
            break;
          }
        } research_players_iterate_end;
      }
    } else if (pconn->observer) {
      /* Case global observer. */
      send_packet_research_info(pconn, &full_info);
    }
  } conn_list_iterate_end;
}

/************************************************************************//**
  Players sharing the research have got a new technology (from somewhere).
  'was_discovery' is passed on to upgrade_city_extras. Logging and
  notification is not done here as it depends on how the tech came.
****************************************************************************/
void found_new_tech(struct research *presearch, Tech_type_id tech_found,
                    bool was_discovery, bool saving_bulbs)
{
  int had_embassies[player_slot_count()];
  bool could_switch[player_slot_count()][government_count()];
  bool was_first = FALSE;
  bool bonus_tech_hack = FALSE;
  int i;
  const char *advance_name;
  struct advance *vap = valid_advance_by_number(tech_found);
  struct city *pcity;

  if (!is_future_tech(tech_found)) {

#ifndef FREECIV_NDEBUG
    fc_assert(NULL != vap);
    fc_assert(TECH_KNOWN != research_invention_state(presearch, tech_found));
#endif /* FREECIV_NDEBUG */

    was_first = (!game.info.global_advances[tech_found]);
  }

  /* Assign 'advance_name' before we increase the future tech counter. */
  advance_name = research_advance_name_translation(presearch, tech_found);

  if (was_first && vap) {
    /* Alert the owners of any wonders that have been made obsolete */
    improvement_iterate(pimprove) {
      requirement_vector_iterate(&pimprove->obsolete_by, pobs) {
        if (pobs->source.kind == VUT_ADVANCE
            && pobs->source.value.advance == vap
            && pobs->range >= REQ_RANGE_WORLD
            && pobs->survives
            && is_great_wonder(pimprove)
            && (pcity = city_from_great_wonder(pimprove))) {
          notify_player(city_owner(pcity), NULL, E_WONDER_OBSOLETE, ftc_server,
                        _("Discovery of %s OBSOLETES %s in %s!"), 
                        research_advance_name_translation
                            (research_get(city_owner(pcity)), tech_found),
                        improvement_name_translation(pimprove),
                        city_link(pcity));
        }
      } requirement_vector_iterate_end;
    } improvement_iterate_end;
  }

  if (was_first
      && !is_future_tech(tech_found)
      && advance_has_flag(tech_found, TF_BONUS_TECH)) {
    bonus_tech_hack = TRUE;
  }

  /* Memorize some values before the tech is marked as researched.
   * We will check what has changed later. */
  players_iterate(aplayer) {
    i = player_index(aplayer);

    /* Count EFT_HAVE_EMBASSIES effect for each player. */
    had_embassies[i] = get_player_bonus(aplayer, EFT_HAVE_EMBASSIES);

    if (presearch != research_get(aplayer)) {
      continue;
    }

    /* Memorize for the players sharing the research what government
     * they could switch on. */
    governments_iterate(pgov) {
      could_switch[i][government_index(pgov)]
          = can_change_to_government(aplayer, pgov);
    } governments_iterate_end;
  } players_iterate_end;

  /* got_tech allows us to change research without applying techpenalty
   * (without losing bulbs) */
  if (tech_found == presearch->researching) {
    presearch->got_tech = TRUE;
    presearch->got_tech_multi = TRUE;
  }
  presearch->researching_saved = A_UNKNOWN;
  presearch->techs_researched++;

  /* Mark the tech as known in the research struct and update
   * global_advances array. */
  if (is_future_tech(tech_found)) {
    presearch->future_tech++;
  } else {
    research_invention_set(presearch, tech_found, TECH_KNOWN);
    research_update(presearch);
  }

  /* Inform players about their new tech. */
  send_research_info(presearch, NULL);

  if (was_first) {
    /* Inform all players about new global advances to give them a
     * chance for obsolete buildings. */
    send_game_info(NULL);
  }

  /* Make proper changes for all players. Use shuffled order, in case
   * a script would detect a signal. */
  shuffled_players_iterate(aplayer) {
    i = player_index(aplayer);

    if (presearch == research_get(aplayer)) {
      /* Only for players sharing the research. */
      remove_obsolete_buildings(aplayer);

      /* Give free infrastructure in every city */
      if (tech_found != A_FUTURE) {
        upgrade_all_city_extras(aplayer, was_discovery);

        /* Revealing of extras with visibility_req */
        whole_map_iterate(&(wld.map),ptile) {
          if (map_is_known_and_seen(ptile, aplayer, V_MAIN)) {
            if (update_player_tile_knowledge(aplayer, ptile)) {
              send_tile_info(aplayer->connections, ptile, FALSE);
            }
          }
        } whole_map_iterate_end;
      }

      /* Enhance vision of units if a player-ranged effect has changed. Note
       * that world-ranged effects will not be updated immediately. */
      unit_list_refresh_vision(aplayer->units);

      /* Notify a player about new governments available */
      governments_iterate(pgov) {
        if (!could_switch[i][government_index(pgov)]
            && can_change_to_government(aplayer, pgov)) {
          notify_player(aplayer, NULL, E_NEW_GOVERNMENT, ftc_server,
                        _("Discovery of %s makes the government form %s"
                          " available. You may want to start a revolution."),
                        advance_name,
                        government_name_translation(pgov));
        }
      } governments_iterate_end;
    }

    /* For any player. */
    /* Update all cities in case the tech changed some effects. This is
     * inefficient; it could be optimized if it's found to be a problem.
     * But techs aren't researched that often. */
    city_list_iterate(aplayer->cities, apcity) {
      /* Refresh the city data; this also updates the squared city radius. */
      city_refresh(apcity);
      city_refresh_vision(apcity);
      send_city_info(aplayer, apcity);
    } city_list_iterate_end;

    /* Send all player an updated info of the owner of the Marco Polo
     * Wonder if this wonder has become obsolete. */
    if (0 < had_embassies[i]
        && 0 <= get_player_bonus(aplayer, EFT_HAVE_EMBASSIES)) {
      send_player_all_c(aplayer, aplayer->connections);
      players_iterate(pother_player) {
        if (aplayer != pother_player) {
          send_player_all_c(aplayer, pother_player->connections);
          send_player_all_c(pother_player, aplayer->connections);
        }
      } players_iterate_end;
    }
  } shuffled_players_iterate_end;

  if (tech_found == presearch->tech_goal) {
    presearch->tech_goal = A_UNSET;
  }

  if (tech_found == presearch->researching) {
    /* Try to pick new tech to research. */
    Tech_type_id next_tech = research_goal_step(presearch,
                                                presearch->tech_goal);

    /* As this function can be recursive, we need to print the messages
     * before really picking the new technology. */
    if (A_UNSET != next_tech) {
      notify_research(presearch, NULL, E_TECH_LEARNED, ftc_server,
                      _("Learned %s. Our scientists focus on %s; "
                        "goal is %s."),
                      advance_name,
                      research_advance_name_translation(presearch,
                                                        next_tech),
                      research_advance_name_translation
                          (presearch, presearch->tech_goal));
    } else {
      if (is_future_tech(tech_found)) {
        /* Continue researching future tech. */
        next_tech = A_FUTURE;
      } else {
        /* If there is at least one AI player still alive, then pick
         * a random tech, else keep A_UNSET. */
        research_players_iterate(presearch, aplayer) {
          if (is_ai(aplayer)) {
            next_tech = pick_random_tech(presearch);
            break;
          }
        } research_players_iterate_end;
      }

      if (A_UNSET == next_tech) {
        notify_research(presearch, NULL, E_TECH_LEARNED, ftc_server,
                        _("Learned %s. Scientists "
                          "do not know what to research next."),
                        advance_name);
      } else {
        notify_research(presearch, NULL, E_TECH_LEARNED, ftc_server,
                        _("Learned %s. Scientists choose to research %s."),
                        advance_name,
                        research_advance_name_translation(presearch,
                                                          next_tech));
      }
    }

    if (A_UNSET != next_tech) {
      choose_tech(presearch, next_tech);
    } else {
      presearch->researching = A_UNSET;
    }
  }

  if (!saving_bulbs && presearch->bulbs_researched > 0) {
    presearch->bulbs_researched = 0;
  }

  if (bonus_tech_hack) {
    Tech_type_id additional_tech;
    char research_name[MAX_LEN_NAME * 2];
    const char *radv_name;

    research_pretty_name(presearch, research_name, sizeof(research_name));

    additional_tech = pick_free_tech(presearch);

    radv_name = research_advance_name_translation(presearch, additional_tech);

    give_immediate_free_tech(presearch, additional_tech);
    if (advance_by_number(tech_found)->bonus_message != NULL
        && additional_tech != A_UNSET) {
      notify_research(presearch, NULL, E_TECH_GAIN, ftc_server,
                      _(advance_by_number(tech_found)->bonus_message),
                      radv_name);
    } else if (additional_tech != A_UNSET) {
      /* FIXME: "your" when it was just civilization of one of the players
       * sharing the reseach. */
      notify_research(presearch, NULL, E_TECH_GAIN, ftc_server,
                      _("Great scientists from all the "
                        "world join your civilization: you learn "
                        "%s immediately."), radv_name);
    }
    /* TODO: Ruleset should be able to customize this message too */
    notify_research_embassies(presearch, NULL, E_TECH_EMBASSY, ftc_server,
                              _("%s acquire %s as a result of learning %s."),
                              research_name, radv_name, advance_name);
  }
}

/************************************************************************//**
  Is player about to lose tech?
****************************************************************************/
static bool lose_tech(struct research *research)
{
  if (game.info.techloss_forgiveness < 0) {
    /* Tech loss disabled */
    return FALSE;
  }

  if (research->techs_researched == 0) {
    /* No tech to lose */
    fc_assert(research->future_tech == 0);
    return FALSE;
  }

  /* First check is not for optimization only - it protects
   * research_total_bulbs_required() from getting called before research
   * has even been set to value other than A_UNSET. */
  if (research->bulbs_researched < 0
      && research->bulbs_researched <
         (-research_total_bulbs_required(research, research->researching, FALSE)
          * game.info.techloss_forgiveness / 100)) {
    return TRUE;
  }

  return FALSE;
}

/************************************************************************//**
  Adds the given number of bulbs into the player's tech and (if necessary and
  'check_tech' is TRUE) completes the research. If the total number of bulbs
  is negative due to tech upkeep, one (randomly chosen) tech is lost.

  The caller is responsible for sending updated player information.

  This is called from each city every turn, from caravan revenue, and at the
  end of the phase.
****************************************************************************/
void update_bulbs(struct player *pplayer, int bulbs, bool check_tech)
{
  struct research *research = research_get(pplayer);

  if (!pplayer->is_alive) {
    /* Dead players do not produce research */
    return;
  }

  /* count our research contribution this turn */
  pplayer->server.bulbs_last_turn += bulbs;
  research->bulbs_researched += bulbs;
  advance_index_iterate(A_FIRST, j) {
    if (j == research->researching) {
      research->inventions[j].bulbs_researched_saved = research->bulbs_researched;
    }
  } advance_index_iterate_end;


  do {
    /* If we have a negative number of bulbs we do try to:
     * - reduce the number of future techs;
     * - or lose one random tech.
     * After that the number of bulbs available is incresed based on the
     * value of the lost tech. */
    if (lose_tech(research)) {
      Tech_type_id tech = (research->future_tech > 0
                           ? A_FUTURE : pick_random_tech_to_lose(research));

      if (tech != A_NONE) {
        if (game.server.techloss_restore >= 0) {
          research->bulbs_researched +=
              (research_total_bulbs_required(research, tech, TRUE)
               * game.server.techloss_restore / 100);
        } else {
          research->bulbs_researched = 0;
        }
        research->researching_saved = A_UNKNOWN;

        log_debug("%s: tech loss (%s)",
                  research_rule_name(research),
                  (is_future_tech(tech) ? "Future Tech"
                   : research_advance_rule_name(research, tech)));
        research_tech_lost(research, tech);
        /* Make notification after losing the research, in case it is
         * a future tech (for getting the right tech number). */
        notify_research(research, NULL, E_TECH_LOST, ftc_server,
                        _("Insufficient science output. We lost %s."),
                        research_advance_name_translation(research, tech));
      }
    }

    /* Check for finished research. */
    if (!check_tech
        || research->researching == A_UNSET
        || (research->bulbs_researched
            < research_total_bulbs_required(research, research->researching,
                                            FALSE))) {
      break;
    }

    tech_researched(research);
  } while (research->researching != A_UNSET);
}

/************************************************************************//**
  Choose a random tech for player to lose.
****************************************************************************/
static Tech_type_id
pick_random_tech_to_lose(const struct research *presearch)
{
  bv_techs eligible_techs;
  /* A_NONE included in advance_count(). */
  int eligible = advance_count() - 1;
  int chosen;

  BV_SET_ALL(eligible_techs);

  advance_index_iterate(A_FIRST, i) {
    if (research_invention_state(presearch, i) != TECH_KNOWN) {
      if (BV_ISSET(eligible_techs, i)) {
        eligible--;
        BV_CLR(eligible_techs, i);
      }
    } else {
      /* Knowing this tech may make others ineligible */
      Tech_type_id root = advance_required(i, AR_ROOT);
      /* Never lose techs that are root_req for a currently known tech
       * (including self root_req) */
      if (root != A_NONE && BV_ISSET(eligible_techs, root)) {
        eligible--;
        BV_CLR(eligible_techs, root);
      }
      if (!game.info.tech_loss_allow_holes) {
        /* Ruleset can prevent this kind of tech loss from opening up
         * holes in the tech tree */
        Tech_type_id prereq;
        prereq = advance_required(i, AR_ONE);
        if (prereq != A_NONE && BV_ISSET(eligible_techs, prereq)) {
          eligible--;
          BV_CLR(eligible_techs, prereq);
        }
        prereq = advance_required(i, AR_TWO);
        if (prereq != A_NONE && BV_ISSET(eligible_techs, prereq)) {
          eligible--;
          BV_CLR(eligible_techs, prereq);
        }
      }
    }
  } advance_index_iterate_end;

  if (eligible == 0) {
    /* no researched technology at all */
    return A_NONE;
  }

  chosen = fc_rand(eligible) + 1;

  advance_index_iterate(A_FIRST, i) {
    if (BV_ISSET(eligible_techs, i)) {
      chosen--;
      if (chosen == 0) {
        return i;
      }
    }
  } advance_index_iterate_end;

  /* should never be reached */
  fc_assert_msg(chosen == 0, "internal error (eligible=%d, chosen=%d)",
                eligible, chosen);
  return A_NONE;
}

/************************************************************************//**
  Helper for research_tech_lost().
****************************************************************************/
static inline struct government *
pick_random_government(struct player *pplayer)
{
  struct government *picked = NULL;
  int gov_nb = 0;

  governments_iterate(pgov) {
    if (can_change_to_government(pplayer, pgov) && 0 == fc_rand(++gov_nb)) {
      picked = pgov;
    }
  } governments_iterate_end;
  fc_assert(NULL != picked);
  return picked;
}

/************************************************************************//**
  Remove one tech from the research.
****************************************************************************/
static void research_tech_lost(struct research *presearch, Tech_type_id tech)
{
  char research_name[MAX_LEN_NAME * 2];
  /* Research members will be notified when new tech is chosen. */

  research_pretty_name(presearch, research_name, sizeof(research_name));

  presearch->techs_researched--;
  if (is_future_tech(tech)) {
    presearch->future_tech--;
    research_update(presearch);
    /* Notify after decreasing the future tech counter, to get the right
     * tech number in the message. */
    notify_research_embassies(presearch, NULL, E_TECH_EMBASSY, ftc_server,
                              _("The %s have lost %s."),
                              research_name,
                              research_advance_name_translation(presearch,
                                                                tech));
    /* Inform players about their technology loss. */
    send_research_info(presearch, NULL);
    return;
  }

  fc_assert_ret(valid_advance_by_number(tech));
  notify_research_embassies(presearch, NULL, E_TECH_EMBASSY, ftc_server,
                            /* TRANS: technology loss */
                            _("The %s have lost %s."),
                            research_name,
                            research_advance_name_translation(presearch,
                                                              tech));

  /* Remove technology. */
  research_invention_set(presearch, tech, TECH_UNKNOWN);
  research_update(presearch);
  log_debug("%s lost tech id %d (%s)", research_rule_name(presearch), tech,
            advance_rule_name(advance_by_number(tech)));

  /* Inform players about their technology loss. */
  send_research_info(presearch, NULL);

  research_players_iterate(presearch, pplayer) {
    /* Check government. */
    if (!can_change_to_government(pplayer, government_of_player(pplayer))) {
      /* Lost the technology for the government; switch to random
       * available government. */
      struct government *pgov = pick_random_government(pplayer);

      notify_player(pplayer, NULL, E_NEW_GOVERNMENT, ftc_server,
                    _("The required technology for our government '%s' "
                      "was lost. The citizens have started a "
                      "revolution into '%s'."),
                    government_name_translation(government_of_player
                                                (pplayer)),
                    government_name_translation(pgov));
      handle_player_change_government(pplayer, government_number(pgov));
      send_player_info_c(pplayer, NULL);
    } else if (NULL != pplayer->target_government
               && !can_change_to_government(pplayer,
                                            pplayer->target_government)) {
      /* Lost the technology for the target government; use a random
       * available government as new target government. */
      struct government *pgov = pick_random_government(pplayer);

      notify_player(pplayer, NULL, E_NEW_GOVERNMENT, ftc_server,
                    _("The required technology for our new government "
                      "'%s' was lost. The citizens chose '%s' as new "
                      "target government."),
                    government_name_translation(pplayer->target_government),
                    government_name_translation(pgov));
      pplayer->target_government = pgov;
      send_player_info_c(pplayer, pplayer->connections);
    }

    /* Check all units for valid activities. */
    unit_list_iterate(pplayer->units, punit) {
      if (!can_unit_continue_current_activity(punit)) {
        log_debug("lost technology for activity of unit %s of %s (%d, %d)",
                  unit_name_translation(punit), player_name(pplayer),
                  TILE_XY(unit_tile(punit)));
        set_unit_activity(punit, ACTIVITY_IDLE);
        send_unit_info(NULL, punit);
      }
    } unit_list_iterate_end;

    /* Check city production */
    city_list_iterate(pplayer->cities, pcity) {
      bool update = FALSE;

      if (pcity->production.kind == VUT_UTYPE
          && !can_city_build_unit_now(pcity, pcity->production.value.utype)) {
        notify_player(pplayer, city_tile(pcity),
                      E_CITY_CANTBUILD, ftc_server,
                      _("%s can't build %s. The required technology was "
                        "lost."),
                      city_link(pcity),
                      utype_name_translation(pcity->production.value.utype));
        choose_build_target(pplayer, pcity);
        update = TRUE;
      }

      if (pcity->production.kind == VUT_IMPROVEMENT
          && !can_city_build_improvement_now(pcity,
                                             pcity->production.value.building)) {
        notify_player(pplayer, city_tile(pcity),
                      E_CITY_CANTBUILD, ftc_server,
                      _("%s can't build %s. The required technology was "
                        "lost."),
                      city_link(pcity),
                      improvement_name_translation
                      (pcity->production.value.building));
        choose_build_target(pplayer, pcity);
        update = TRUE;
      }

      if (update) {
        city_refresh(pcity);
        send_city_info(pplayer, pcity);
      }
    } city_list_iterate_end;
  } research_players_iterate_end;
}

/************************************************************************//**
  Returns random researchable tech or A_FUTURE. No side effects.
****************************************************************************/
static Tech_type_id pick_random_tech(const struct research *presearch)
{
  Tech_type_id tech = A_FUTURE;
  int num_techs = 0;

  advance_index_iterate(A_FIRST, i) {
    if (research_invention_state(presearch, i) == TECH_PREREQS_KNOWN) {
      if (fc_rand(++num_techs) == 0) {
        tech = i;
      }
    }
  } advance_index_iterate_end;
  return tech;
}

/************************************************************************//**
  Returns cheapest researchable tech, random among equal cost ones.
****************************************************************************/
static Tech_type_id pick_cheapest_tech(const struct research *presearch)
{
  int cheapest_cost = -1;
  int cheapest_amount = 0;
  Tech_type_id cheapest = A_FUTURE; /* If no real tech is found to be missing */

  advance_index_iterate(A_FIRST, i) {
    if (research_invention_state(presearch, i) == TECH_PREREQS_KNOWN) {
      int cost = research_total_bulbs_required(presearch, i, FALSE);

      if (cost < cheapest_cost || cheapest_cost == -1) {
        cheapest_cost = cost;
        cheapest_amount = 1;
        cheapest = i;
      } else if (cost == cheapest_cost && fc_rand(++cheapest_amount) == 0) {
        cheapest = i;
      }
    }
  } advance_index_iterate_end;

  return cheapest;
}

/************************************************************************//**
  Finds and chooses (sets) a random research target from among all those
  available until presearch->researching != A_UNSET.
  Players may research more than one tech in this function.
  Possible reasons:
  - techpenalty < 100;
  - research.got_tech = TRUE and enough bulbs was saved;
  - research.researching = A_UNSET and enough bulbs was saved.
****************************************************************************/
void choose_random_tech(struct research *research)
{
  do {
    choose_tech(research, pick_random_tech(research));
  } while (research->researching == A_UNSET);
}

/************************************************************************//**
  Called when a player chooses the tech he wants to research (or when
  the server chooses it for him automatically).

  This takes care of all side effects so the research target probably
  shouldn't be changed outside of this function (doing so has been the
  cause of several bugs).
****************************************************************************/
void choose_tech(struct research *research, Tech_type_id tech)
{
  int bulbs_res = 0;

  if (is_future_tech(tech)) {
    if (is_future_tech(research->researching)
        && (research->bulbs_researched
            >= research_total_bulbs_required(research, tech, FALSE))) {
      tech_researched(research);
    }
  } else {
    if (research->researching == tech) {
      return;
    }
    if (research_invention_state(research, tech) != TECH_PREREQS_KNOWN) {
      /* Can't research this. */
      return;
    }
  }

  if (game.server.multiresearch) {
    advance_index_iterate(A_FIRST, j) {
      /* Save old tech research */
      if (j == research->researching) {
        research->inventions[j].bulbs_researched_saved = research->bulbs_researched;
      }
      /* New tech*/
      if (j == tech) {
        bulbs_res = research->inventions[j].bulbs_researched_saved;
      }
    } advance_index_iterate_end;
    research->researching = tech;
    if (research->got_tech_multi == FALSE) {
      research->bulbs_researched = 0;
    }
    research->bulbs_researched = research->bulbs_researched + bulbs_res;
    research->got_tech_multi = FALSE;
    if (research->bulbs_researched
        >= research_total_bulbs_required(research, tech, FALSE)) {
      tech_researched(research);
    }
    return;
  }

  if (!research->got_tech && research->researching_saved == A_UNKNOWN) {
    research->bulbs_researching_saved = research->bulbs_researched;
    research->researching_saved = research->researching;
    /* Subtract a penalty because we changed subject. */
    if (research->bulbs_researched > 0) {
      research->bulbs_researched
        -= ((research->bulbs_researched * game.server.techpenalty) / 100);
      fc_assert(research->bulbs_researched >= 0);
    }
  } else if (tech == research->researching_saved) {
    research->bulbs_researched = research->bulbs_researching_saved;
    research->researching_saved = A_UNKNOWN;
  }
  research->researching = tech;
  if (research->bulbs_researched
      >= research_total_bulbs_required(research, tech, FALSE)) {
    tech_researched(research);
  }
}

/************************************************************************//**
  Called when a player chooses the tech goal he wants to research (or when
  the server chooses it for him automatically).
****************************************************************************/
void choose_tech_goal(struct research *presearch, Tech_type_id tech)
{
  fc_assert_ret(presearch != NULL);

  if (tech == presearch->tech_goal) {
    return;
  }

  /* It's been suggested that if the research target is empty then
   * choose_random_tech() should be called here. */
  presearch->tech_goal = tech;
  notify_research(presearch, NULL, E_TECH_GOAL, ftc_server,
                  _("Technology goal is %s."),
                  research_advance_name_translation(presearch, tech));
}

/************************************************************************//**
  Initializes tech data for the research.
****************************************************************************/
void init_tech(struct research *research, bool update)
{
  research_invention_set(research, A_NONE, TECH_KNOWN);

  advance_index_iterate(A_FIRST, i) {
    research_invention_set(research, i, TECH_UNKNOWN);
  } advance_index_iterate_end;

#ifdef TECH_UPKEEP_DEBUGGING
  /* Print a list of the needed upkeep if 'i' techs are researched.
   * If the ruleset contains self-rooted techs this can not work! */
  {
    bool global_state[A_LAST];
    Tech_type_id tech = A_LAST;

    /* Save the game research state. */
    advance_index_iterate(A_FIRST, i) {
      global_state[i] = game.info.global_advances[i];
    } advance_index_iterate_end;

    research->techs_researched = 1;
    research_update(presearch);

    /* Show research costs. */
    advance_index_iterate(A_NONE, i) {
      log_debug("[research %d] %-25s (ID: %3d) cost: %6d - reachable: %-3s "
                "(now) / %-3s (ever)", research_number(research),
                advance_rule_name(advance_by_number(i)), i,
                research_total_bulbs_required(research, i, FALSE),
                research_invention_gettable(research, i, FALSE)
                ? "yes" : "no",
                research_invention_reachable(research, i) ? "yes" : "no");
    } advance_index_iterate_end;

    /* Update step for step each tech as known and print the upkeep. */
    while (tech != A_NONE) {
      tech = A_NONE;
      advance_index_iterate(A_FIRST, i) {
        if (research_invention_state(research, i) == TECH_PREREQS_KNOWN) {
          /* Found a tech which can be researched. */
          tech = i;
          break;
        }
      } advance_index_iterate_end;

      if (tech != A_NONE) {
        research->inventions[tech].state = TECH_KNOWN;
        research->techs_researched++;

        /* This will change the game state! */
        research_update(research);

        research_players_iterate(research, pplayer) {
          log_debug("[player %d] researched: %-25s (ID: %4d) techs: %3d "
                    "upkeep: %4d", research_number(research),
                    advance_rule_name(advance_by_number(tech)), tech,
                    research->techs_researched, player_tech_upkeep(pplayer));
        } research_players_iterate_end;
      }
    }

    /* Reset the changes done. */
    advance_index_iterate(A_FIRST, i) {
      research_invention_set(research, i, TECH_UNKNOWN);
      game.info.global_advances[i] = global_state[i];
    } advance_index_iterate_end;
  }
#endif /* TECH_UPKEEP_DEBUGGING */

  research->techs_researched = 1;

  if (update) {
    Tech_type_id next_tech;

    /* Mark the reachable techs */
    research_update(research);

    next_tech = research_goal_step(research, research->tech_goal);
    if (A_UNSET != next_tech) {
      choose_tech(research, next_tech);
    } else {
      choose_random_tech(research);
    }
  }
}

/************************************************************************//**
  Gives global (read from the game ruleset file) and nation (read from the
  nation ruleset files) initial techs as specified in the ruleset, and
  random free technologies thanks to the techlevel setting.
****************************************************************************/
void give_initial_techs(struct research *presearch, int num_random_techs)
{
  int i;

  /* Global techs. */
  for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
    if (game.rgame.global_init_techs[i] == A_LAST) {
      break;
    }
    /* Maybe the player already got this tech by an other way (e.g. team). */
    if (research_invention_state(presearch, game.rgame.global_init_techs[i])
        != TECH_KNOWN) {
      found_new_tech(presearch, game.rgame.global_init_techs[i],
                     FALSE, TRUE);
    }
  }

  /* Nation techs. */
  research_players_iterate(presearch, pplayer) {
    const struct nation_type *pnation = nation_of_player(pplayer);

    for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
      if (pnation->init_techs[i] == A_LAST) {
        break;
      }
      /* Maybe the player already got this tech by an other way. */
      if (research_invention_state(presearch, pnation->init_techs[i])
          != TECH_KNOWN) {
        found_new_tech(presearch, pnation->init_techs[i], FALSE, TRUE);
      }
    }
  } research_players_iterate_end;

  /* Random free techs (N.B.: freecost penalty not applied). */
  for (i = 0; i < num_random_techs; i++) {
    found_new_tech(presearch, pick_random_tech(presearch), FALSE, TRUE);
  }
}

/************************************************************************//**
  If victim has a tech which pplayer doesn't have, pplayer will get it.
  The clients will both be notified and the conquer cost
  penalty applied. Used for diplomats and city conquest.
  If preferred is A_UNSET one random tech will be chosen.
  Returns the stolen tech or A_NONE if no tech was found.
****************************************************************************/
Tech_type_id steal_a_tech(struct player *pplayer, struct player *victim,
                          Tech_type_id preferred)
{
  struct research *presearch, *vresearch;
  Tech_type_id stolen_tech = A_NONE;
  const char *advance_name;
  char research_name[MAX_LEN_NAME * 2];

  if (get_player_bonus(victim, EFT_NOT_TECH_SOURCE) > 0) {
    return A_NONE;
  }

  presearch = research_get(pplayer);
  vresearch = research_get(victim);

  if (preferred == A_UNSET) {
    int j = 0;

    advance_index_iterate(A_FIRST, i) {
      if (research_invention_gettable(presearch, i,
                                      game.info.tech_steal_allow_holes)
          && research_invention_state(presearch, i) != TECH_KNOWN
          && research_invention_state(vresearch, i) == TECH_KNOWN) {
        j++;
      }
    } advance_index_iterate_end;
  
    if (j == 0)  {
      /* we've moved on to future tech */
      if (vresearch->future_tech > presearch->future_tech) {
        stolen_tech = A_FUTURE;
      } else {
        return A_NONE;
      }
    } else {
      /* pick random tech */
      j = fc_rand(j) + 1;
      stolen_tech = A_NONE; /* avoid compiler warning */
      advance_index_iterate(A_FIRST, i) {
        if (research_invention_gettable(presearch, i,
                                        game.info.tech_steal_allow_holes)
            && research_invention_state(presearch, i) != TECH_KNOWN
            && research_invention_state(vresearch, i) == TECH_KNOWN) {
	  j--;
        }
        if (j == 0) {
	  stolen_tech = i;
	  break;
        }
      } advance_index_iterate_end;
      fc_assert(stolen_tech != A_NONE);
    }
  } else { /* preferred != A_UNSET */
#ifndef FREECIV_NDEBUG
    if (!is_future_tech(preferred)) {
      fc_assert(NULL != valid_advance_by_number(preferred));
      fc_assert(TECH_KNOWN == research_invention_state(vresearch,
                                                       preferred));
    }
#endif /* FREECIV_NDEBUG */
    stolen_tech = preferred;
  }

  advance_name = research_advance_name_translation(presearch, stolen_tech);
  research_pretty_name(presearch, research_name, sizeof(research_name));
  notify_player(pplayer, NULL, E_MY_DIPLOMAT_THEFT, ftc_server,
                _("You steal %s from the %s."),
                advance_name,
                nation_plural_for_player(victim));
  notify_research(presearch, pplayer, E_TECH_GAIN, ftc_server,
                  _("The %s stole %s from the %s and shared it with you."),
                  nation_plural_for_player(pplayer),
                  advance_name,
                  nation_plural_for_player(victim));

  notify_player(victim, NULL, E_ENEMY_DIPLOMAT_THEFT, ftc_server,
                _("The %s stole %s from you!"),
                nation_plural_for_player(pplayer),
                advance_name);

  notify_research_embassies(presearch, victim, E_TECH_EMBASSY, ftc_server,
                            _("The %s have stolen %s from the %s."),
                            research_name,
                            advance_name,
                            nation_plural_for_player(victim));

  if (tech_transfer(pplayer, victim, stolen_tech)) {
    research_apply_penalty(presearch, stolen_tech, game.server.conquercost);
    found_new_tech(presearch, stolen_tech, FALSE, TRUE);
    script_tech_learned(presearch, pplayer, advance_by_number(stolen_tech),
                        "stolen");
    return stolen_tech;
  }

  return A_NONE;
}

/************************************************************************//**
  Handle incoming research packet. Need to check correctness
  Set the player to be researching the given tech.

  If there are enough accumulated research points, the tech may be
  acquired immediately.
****************************************************************************/
void handle_player_research(struct player *pplayer, int tech)
{
  struct research *research = research_get(pplayer);

  if (tech != A_FUTURE && !valid_advance_by_number(tech)) {
    return;
  }
  
  if (tech != A_FUTURE
      && research_invention_state(research, tech) != TECH_PREREQS_KNOWN) {
    return;
  }

  choose_tech(research, tech);

  /* Notify players sharing the same research. */
  send_research_info(research, NULL);
}

/************************************************************************//**
  Handle incoming player_tech_goal packet
  Called from the network or AI code to set the player's tech goal.
****************************************************************************/
void handle_player_tech_goal(struct player *pplayer, int tech_goal)
{
  struct research *research = research_get(pplayer);

  /* Set the tech goal to a defined state if it is
   * - not a future tech and not a valid goal
   * - not a future tech and not a valid advance
   * - not defined
   * - known (i.e. due to EFT_GIVE_IMM_TECH). */
  if ((tech_goal != A_FUTURE
       && (!valid_advance_by_number(tech_goal)
           || !research_invention_reachable(research, tech_goal)))
      || (tech_goal == A_NONE)
      || (TECH_KNOWN == research_invention_state(research, tech_goal))) {
    tech_goal = A_UNSET;
  }

  choose_tech_goal(research, tech_goal);

  /* Notify players sharing the same research. */
  send_research_info(research, NULL);
}

/************************************************************************//**
  Choose a free tech.
****************************************************************************/
Tech_type_id pick_free_tech(struct research *presearch)
{
  Tech_type_id tech;

  if (game.info.free_tech_method == FTM_CHEAPEST) {
    tech = pick_cheapest_tech(presearch);
  } else if (presearch->researching == A_UNSET
      || game.info.free_tech_method == FTM_RANDOM) {
    tech = pick_random_tech(presearch);
  } else {
    tech = presearch->researching;
  }
  return tech;
}

/************************************************************************//**
  Give an immediate free tech (probably chosen with pick_free_tech()).
  Applies freecost.
****************************************************************************/
void give_immediate_free_tech(struct research *presearch, Tech_type_id tech)
{
  research_apply_penalty(presearch, tech, game.server.freecost);
  found_new_tech(presearch, tech, FALSE, TRUE);
}

/************************************************************************//**
  Let the player forget one tech.
****************************************************************************/
static void forget_tech_transfered(struct player *pplayer, Tech_type_id tech)
{
  struct research *presearch = research_get(pplayer);

  research_tech_lost(presearch, tech);
  /* Make notification after losing the research, in case it is a future
   * tech (for getting the right tech number). */
  notify_player(pplayer, NULL, E_TECH_LOST, ftc_server,
                _("Too bad! You made a mistake transferring the tech %s and "
                  "lost it."),
                research_advance_name_translation(presearch, tech));
  notify_research(presearch, pplayer, E_TECH_LOST, ftc_server,
                  _("Too bad! The %s made a mistake transferring the tech "
                    "%s and lost it."),
                nation_plural_for_player(pplayer),
                research_advance_name_translation(presearch, tech));
}

/************************************************************************//**
  Check if the tech is lost by the donor or receiver. Returns if the
  receiver gets a new tech.
****************************************************************************/
bool tech_transfer(struct player *plr_recv, struct player *plr_donor,
                   Tech_type_id tech)
{
  if (game.server.techlost_donor > 0) {
    struct research *donor_research = research_get(plr_donor);
    bool donor_can_lose = TRUE;

    advance_index_iterate(A_FIRST, i) {
      /* Never let donor lose tech if it's root_req for some other known
       * tech */
      if (research_invention_state(donor_research, i) == TECH_KNOWN
          && (advance_required(i, AR_ROOT) == tech
              || (!game.info.tech_trade_loss_allow_holes
                  && (advance_required(i, AR_ONE) == tech
                      || advance_required(i, AR_TWO) == tech)))) {
        donor_can_lose = FALSE;
        break;
      }
    } advance_index_iterate_end;
    if (donor_can_lose && fc_rand(100) < game.server.techlost_donor) {
      forget_tech_transfered(plr_donor, tech);
    }
  }

  if (fc_rand(100) < game.server.techlost_recv) {
    forget_tech_transfered(plr_recv, tech);
    return FALSE;
  }

  return TRUE;
}
