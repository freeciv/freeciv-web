/***********************************************************************
 Freeciv - Copyright (C) 2002 - The Freeciv Project
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

/* common */
#include "game.h"
#include "government.h"
#include "map.h"
#include "multipliers.h"
#include "research.h"

/* server */
#include "cityturn.h"
#include "plrhand.h"

/* server/advisors */
#include "advdata.h"

/* ai/default */
#include "aicity.h"
#include "aiferry.h"
#include "aiplayer.h"
#include "aisettler.h"
#include "aiunit.h"
#include "daidiplomacy.h"
#include "daieffects.h"

#include "aidata.h"

static void dai_diplomacy_new(struct ai_type *ait,
                              const struct player *plr1,
                              const struct player *plr2);
static void dai_diplomacy_defaults(struct ai_type *ait,
                                   const struct player *plr1,
                                   const struct player *plr2);
static void dai_diplomacy_destroy(struct ai_type *ait,
                                  const struct player *plr1,
                                  const struct player *plr2);

/************************************************************************//**
  Initialize ai data structure
****************************************************************************/
void dai_data_init(struct ai_type *ait, struct player *pplayer)
{
  struct ai_plr *ai = def_ai_player_data(pplayer, ait);

  ai->phase_initialized = FALSE;

  ai->last_num_continents = -1;
  ai->last_num_oceans = -1;

  ai->diplomacy.player_intel_slots
    = fc_calloc(player_slot_count(),
                sizeof(*ai->diplomacy.player_intel_slots));
  player_slots_iterate(pslot) {
    const struct ai_dip_intel **player_intel_slot
      = ai->diplomacy.player_intel_slots + player_slot_index(pslot);
    *player_intel_slot = NULL;
  } player_slots_iterate_end;

  players_iterate(aplayer) {
    /* create ai diplomacy states for all other players */
    dai_diplomacy_new(ait, pplayer, aplayer);
    dai_diplomacy_defaults(ait, pplayer, aplayer);
    /* create ai diplomacy state of this player */
    if (aplayer != pplayer) {
      dai_diplomacy_new(ait, aplayer, pplayer);
      dai_diplomacy_defaults(ait, aplayer, pplayer);
    }
  } players_iterate_end;

  ai->diplomacy.strategy = WIN_OPEN;
  ai->diplomacy.timer = 0;
  ai->diplomacy.love_coeff = 4; /* 4% */
  ai->diplomacy.love_incr = MAX_AI_LOVE * 3 / 100;
  ai->diplomacy.req_love_for_peace = MAX_AI_LOVE / 8;
  ai->diplomacy.req_love_for_alliance = MAX_AI_LOVE / 4;

  ai->settler = NULL;

  /* Initialise autosettler. */
  dai_auto_settler_init(ai);
}

/************************************************************************//**
  Deinitialize ai data structure
****************************************************************************/
void dai_data_close(struct ai_type *ait, struct player *pplayer)
{
  struct ai_plr *ai = def_ai_player_data(pplayer, ait);

  /* Finish the phase if it's open - free resources related to
   * open/finish cycle */
  dai_data_phase_finished(ait, pplayer);

  /* Free autosettler. */
  dai_auto_settler_free(ai);

  if (ai->diplomacy.player_intel_slots != NULL) {
    players_iterate(aplayer) {
      /* destroy the ai diplomacy states of this player with others ... */
      dai_diplomacy_destroy(ait, pplayer, aplayer);
      /* and of others with this player. */
      if (aplayer != pplayer) {
        dai_diplomacy_destroy(ait, aplayer, pplayer);
      }
    } players_iterate_end;
    free(ai->diplomacy.player_intel_slots);
  }
}

/************************************************************************//**
  Return whether data phase is currently open. Data phase is open
  between dai_data_phase_begin() and dai_data_phase_finished() calls.
****************************************************************************/
bool is_ai_data_phase_open(struct ai_type *ait, struct player *pplayer)
{
  struct ai_plr *ai = def_ai_player_data(pplayer, ait);

  return ai->phase_initialized;
}

/************************************************************************//**
  Make and cache lots of calculations needed for other functions.
****************************************************************************/
void dai_data_phase_begin(struct ai_type *ait, struct player *pplayer,
                          bool is_new_phase)
{
  struct ai_plr *ai = def_ai_player_data(pplayer, ait);
  bool caller_closes;

  /* Note that this refreshes advisor data if needed. ai_plr_data_get()
     is expected to refresh advisor data if needed, and ai_plr_data_get()
     depends on this call
     ai_plr_data_get()->ai_data_phase_begin()->adv_data_get() to do it.
     If you change this, you may need to adjust ai_plr_data_get() also. */
  struct adv_data *adv;

  if (ai->phase_initialized) {
    return;
  }

  ai->phase_initialized = TRUE;

  adv = adv_data_get(pplayer, &caller_closes);

  /* Store current number of known continents and oceans so we can compare
     against it later in order to see if ai data needs refreshing. */
  ai->last_num_continents = adv->num_continents;
  ai->last_num_oceans = adv->num_oceans;

  /*** Diplomacy ***/
  if (is_ai(pplayer) && !is_barbarian(pplayer) && is_new_phase) {
    dai_diplomacy_begin_new_phase(ait, pplayer);
  }

  /* Set per-player variables. We must set all players, since players
   * can be created during a turn, and we don't want those to have
   * invalid values. */
  players_iterate(aplayer) {
    struct ai_dip_intel *adip = dai_diplomacy_get(ait, pplayer, aplayer);

    adip->is_allied_with_enemy = NULL;
    adip->at_war_with_ally = NULL;
    adip->is_allied_with_ally = NULL;

    players_iterate(check_pl) {
      if (check_pl == pplayer
          || check_pl == aplayer
          || !check_pl->is_alive) {
        continue;
      }
      if (pplayers_allied(aplayer, check_pl)
          && player_diplstate_get(pplayer, check_pl)->type == DS_WAR) {
       adip->is_allied_with_enemy = check_pl;
      }
      if (pplayers_allied(pplayer, check_pl)
          && player_diplstate_get(aplayer, check_pl)->type == DS_WAR) {
        adip->at_war_with_ally = check_pl;
      }
      if (pplayers_allied(aplayer, check_pl)
          && pplayers_allied(pplayer, check_pl)) {
        adip->is_allied_with_ally = check_pl;
      }
    } players_iterate_end;
  } players_iterate_end;

  /*** Statistics ***/

  ai->stats.workers = fc_calloc(adv->num_continents + 1, sizeof(int));
  ai->stats.ocean_workers = fc_calloc(adv->num_oceans + 1, sizeof(int));
  unit_list_iterate(pplayer->units, punit) {
    struct tile *ptile = unit_tile(punit);

    if (unit_has_type_flag(punit, UTYF_SETTLERS)) {
      if (is_ocean_tile(ptile)) {
        ai->stats.ocean_workers[(int)-tile_continent(ptile)]++;
      } else {
        ai->stats.workers[(int)tile_continent(ptile)]++;
      }
    }
  } unit_list_iterate_end;

  BV_CLR_ALL(ai->stats.diplomat_reservations);
  unit_list_iterate(pplayer->units, punit) {
    if ((unit_can_do_action(punit, ACTION_SPY_POISON)
         || unit_can_do_action(punit, ACTION_SPY_POISON_ESC)
         || unit_can_do_action(punit, ACTION_SPY_SABOTAGE_CITY)
         || unit_can_do_action(punit, ACTION_SPY_SABOTAGE_CITY_ESC)
         || unit_can_do_action(punit, ACTION_SPY_TARGETED_SABOTAGE_CITY)
         || unit_can_do_action(punit, ACTION_SPY_TARGETED_SABOTAGE_CITY_ESC)
         || unit_can_do_action(punit, ACTION_SPY_INCITE_CITY)
         || unit_can_do_action(punit, ACTION_SPY_INCITE_CITY_ESC)
         || unit_can_do_action(punit, ACTION_ESTABLISH_EMBASSY)
         || unit_can_do_action(punit, ACTION_ESTABLISH_EMBASSY_STAY)
         || unit_can_do_action(punit, ACTION_SPY_STEAL_TECH)
         || unit_can_do_action(punit, ACTION_SPY_STEAL_TECH_ESC)
         || unit_can_do_action(punit, ACTION_SPY_TARGETED_STEAL_TECH)
         || unit_can_do_action(punit, ACTION_SPY_TARGETED_STEAL_TECH_ESC)
         || unit_can_do_action(punit, ACTION_SPY_INVESTIGATE_CITY)
         || unit_can_do_action(punit, ACTION_INV_CITY_SPEND)
         || unit_can_do_action(punit, ACTION_SPY_STEAL_GOLD)
         || unit_can_do_action(punit, ACTION_SPY_STEAL_GOLD_ESC)
         || unit_can_do_action(punit, ACTION_STEAL_MAPS)
         || unit_can_do_action(punit, ACTION_STEAL_MAPS_ESC)
         || unit_can_do_action(punit, ACTION_SPY_NUKE_ESC)
         || unit_can_do_action(punit, ACTION_SPY_NUKE))
        && def_ai_unit_data(punit, ait)->task == AIUNIT_ATTACK) {

      fc_assert_msg(punit->goto_tile != NULL, "No target city for spy action");

      if (punit->goto_tile != NULL) {
        struct city *pcity = tile_city(punit->goto_tile);

        if (pcity != NULL) {
          /* Heading somewhere on a mission, reserve target. */
          BV_SET(ai->stats.diplomat_reservations, pcity->id);
        }
      }
    }
  } unit_list_iterate_end;

  aiferry_init_stats(ait, pplayer);

  /*** Interception engine ***/

  /* We are tracking a unit if punit->server.ai->cur_pos is not NULL. If we
   * are not tracking, start tracking by setting cur_pos. If we are,
   * fill prev_pos with previous cur_pos. This way we get the 
   * necessary coordinates to calculate a probable trajectory. */
  players_iterate_alive(aplayer) {
    if (aplayer == pplayer) {
      continue;
    }
    unit_list_iterate(aplayer->units, punit) {
      struct unit_ai *unit_data = def_ai_unit_data(punit, ait);

      if (!unit_data->cur_pos) {
        /* Start tracking */
        unit_data->cur_pos = &unit_data->cur_struct;
        unit_data->prev_pos = NULL;
      } else {
        unit_data->prev_struct = unit_data->cur_struct;
        unit_data->prev_pos = &unit_data->prev_struct;
      }
      *unit_data->cur_pos = unit_tile(punit);
    } unit_list_iterate_end;
  } players_iterate_alive_end;

  if (caller_closes) {
    adv_data_phase_done(pplayer);
  }
}

/************************************************************************//**
  Clean up ai data after phase finished.
****************************************************************************/
void dai_data_phase_finished(struct ai_type *ait, struct player *pplayer)
{
  struct ai_plr *ai = def_ai_player_data(pplayer, ait);

  if (!ai->phase_initialized) {
    return;
  }

  free(ai->stats.workers);
  ai->stats.workers = NULL;

  free(ai->stats.ocean_workers);
  ai->stats.ocean_workers = NULL;

  ai->phase_initialized = FALSE;
}

/************************************************************************//**
  Get current default ai data related to player.
  If close is set, data phase will be opened even if it's currently closed,
  and the boolean will be set accordingly to tell caller that phase needs
  closing.
****************************************************************************/
struct ai_plr *dai_plr_data_get(struct ai_type *ait, struct player *pplayer,
                                bool *caller_closes)
{
  struct ai_plr *ai = def_ai_player_data(pplayer, ait);

  fc_assert_ret_val(ai != NULL, NULL);

  /* This assert really is required. See longer comment
     in adv_data_get() for equivalent code. */
#if defined(FREECIV_DEBUG) || defined(IS_DEVEL_VERSION)
  fc_assert(caller_closes != NULL || ai->phase_initialized);
#endif

  if (caller_closes != NULL) {
    *caller_closes = FALSE;
  }

  if (ai->last_num_continents != wld.map.num_continents
      || ai->last_num_oceans != wld.map.num_oceans) {
    /* We have discovered more continents, recalculate! */

    /* See adv_data_get() */
    if (ai->phase_initialized) {
      dai_data_phase_finished(ait, pplayer);
      dai_data_phase_begin(ait, pplayer, FALSE);
    } else {
      /* wrong order */
      log_debug("%s ai data phase closed when dai_plr_data_get() called",
                player_name(pplayer));
      dai_data_phase_begin(ait, pplayer, FALSE);
      if (caller_closes != NULL) {
        *caller_closes = TRUE;
      } else {
        dai_data_phase_finished(ait, pplayer);
      }
    }
  } else {
    if (!ai->phase_initialized && caller_closes != NULL) {
      dai_data_phase_begin(ait, pplayer, FALSE);
      *caller_closes = TRUE;
    }
  }

  return ai;
}

/************************************************************************//**
  Allocate new ai diplomacy slot
****************************************************************************/
static void dai_diplomacy_new(struct ai_type *ait,
                              const struct player *plr1,
                              const struct player *plr2)
{
  struct ai_dip_intel *player_intel;

  fc_assert_ret(plr1 != NULL);
  fc_assert_ret(plr2 != NULL);

  const struct ai_dip_intel **player_intel_slot
    = def_ai_player_data(plr1, ait)->diplomacy.player_intel_slots
      + player_index(plr2);

  fc_assert_ret(*player_intel_slot == NULL);

  player_intel = fc_calloc(1, sizeof(*player_intel));
  *player_intel_slot = player_intel;
}

/************************************************************************//**
  Set diplomacy data between two players to its default values.
****************************************************************************/
static void dai_diplomacy_defaults(struct ai_type *ait,
                                   const struct player *plr1,
                                   const struct player *plr2)
{
  struct ai_dip_intel *player_intel = dai_diplomacy_get(ait, plr1, plr2);

  fc_assert_ret(player_intel != NULL);

  /* pseudorandom value */
  player_intel->spam = (player_index(plr1) + player_index(plr2)) % 5;
  player_intel->countdown = -1;
  player_intel->war_reason = DAI_WR_NONE;
  player_intel->distance = 1;
  player_intel->ally_patience = 0;
  player_intel->asked_about_peace = 0;
  player_intel->asked_about_alliance = 0;
  player_intel->asked_about_ceasefire = 0;
  player_intel->warned_about_space = 0;
}

/************************************************************************//**
  Returns diplomatic state type between two players
****************************************************************************/
struct ai_dip_intel *dai_diplomacy_get(struct ai_type *ait,
                                       const struct player *plr1,
                                       const struct player *plr2)
{
  fc_assert_ret_val(plr1 != NULL, NULL);
  fc_assert_ret_val(plr2 != NULL, NULL);

  const struct ai_dip_intel **player_intel_slot
    = def_ai_player_data(plr1, ait)->diplomacy.player_intel_slots
      + player_index(plr2);

  fc_assert_ret_val(player_intel_slot != NULL, NULL);

  return (struct ai_dip_intel *) *player_intel_slot;
}

/************************************************************************//**
  Free resources allocated for diplomacy information between two players.
****************************************************************************/
static void dai_diplomacy_destroy(struct ai_type *ait,
                                  const struct player *plr1,
                                  const struct player *plr2)
{
  fc_assert_ret(plr1 != NULL);
  fc_assert_ret(plr2 != NULL);

  const struct ai_dip_intel **player_intel_slot
    = def_ai_player_data(plr1, ait)->diplomacy.player_intel_slots
      + player_index(plr2);

  if (*player_intel_slot != NULL) {
    free(dai_diplomacy_get(ait, plr1, plr2));
  }

  *player_intel_slot = NULL;
}

/************************************************************************//**
  Adjust multiplier values.
****************************************************************************/
void dai_adjust_policies(struct ai_type *ait, struct player *pplayer)
{
  bool needs_back_rearrange = FALSE; 
  struct adv_data *adv;

  adv = adv_data_get(pplayer, NULL);

  multipliers_iterate(ppol) {
    if (multiplier_can_be_changed(ppol, pplayer)) {
      int orig_value = 0;
      int mp_val = player_multiplier_value(pplayer, ppol);
      int pidx = multiplier_index(ppol);
      bool better_found = FALSE;

      city_list_iterate(pplayer->cities, pcity) {
        orig_value += dai_city_want(pplayer, pcity, adv, NULL);
      } city_list_iterate_end;

      /* Consider reducing policy value */
      if (mp_val > ppol->start) {
        int new_value = 0;

        pplayer->multipliers[pidx] = MAX(mp_val - ppol->step, ppol->start);

        city_list_iterate(pplayer->cities, acity) {
          auto_arrange_workers(acity);
        } city_list_iterate_end;

        city_list_iterate(pplayer->cities, pcity) {
          new_value += dai_city_want(pplayer, pcity, adv, NULL);
        } city_list_iterate_end;

        if (new_value > orig_value) {
          /* This is step to right direction, leave it in effect. */
          pplayer->multipliers_target[pidx] = pplayer->multipliers[pidx];

          needs_back_rearrange = FALSE;
          better_found = TRUE;
        }
      }

      /* Consider increasing policy value */
      if (!better_found && mp_val < ppol->stop) {
        int new_value = 0;

        pplayer->multipliers[pidx] = MIN(mp_val + ppol->step, ppol->stop);

        city_list_iterate(pplayer->cities, acity) {
          auto_arrange_workers(acity);
        } city_list_iterate_end;

        city_list_iterate(pplayer->cities, pcity) {
          new_value += dai_city_want(pplayer, pcity, adv, NULL);
        } city_list_iterate_end;

        if (new_value > orig_value) {
          /* This is step to right direction, leave it in effect. */
          pplayer->multipliers_target[pidx] = pplayer->multipliers[pidx];

          needs_back_rearrange = FALSE;
          better_found = TRUE;
        }
      }

      if (!better_found) {
        /* Restore original multiplier value */
        pplayer->multipliers[pidx] = mp_val;
        needs_back_rearrange = TRUE;
      }
    }
  } multipliers_iterate_end;

  if (needs_back_rearrange) {
    city_list_iterate(pplayer->cities, acity) {
      auto_arrange_workers(acity);
    } city_list_iterate_end;
  }
}

/************************************************************************//**
  Set value of the government.
****************************************************************************/
void dai_gov_value(struct ai_type *ait, struct player *pplayer,
                   struct government *gov, adv_want *val, bool *override)
{
  int dist;
  int bonus = 0; /* in percentage */
  int revolution_turns;
  struct universal source = { .kind = VUT_GOVERNMENT, .value.govern = gov };
  struct adv_data *adv;
  int turns = 9999; /* TODO: Set to correct value */
  int nplayers;
  const struct research *presearch;

  /* Use default handling of no-cities case */
  if (city_list_size(pplayer->cities) == 0) {
    *override = FALSE;
    return;
  }

  adv = adv_data_get(pplayer, NULL);
  nplayers = normal_player_count();
  presearch = research_get(pplayer);

  pplayer->government = gov;
  /* Ideally we should change tax rates here, but since
   * this is a rather big CPU operation, we'd rather not. */
  check_player_max_rates(pplayer);
  city_list_iterate(pplayer->cities, acity) {
    auto_arrange_workers(acity);
  } city_list_iterate_end;
  city_list_iterate(pplayer->cities, pcity) {
    bool capital;

    *val += dai_city_want(pplayer, pcity, adv, NULL);
    capital = is_capital(pcity);

    effect_list_iterate(get_req_source_effects(&source), peffect) {
      bool present = TRUE;
      bool active = TRUE;

      requirement_vector_iterate(&peffect->reqs, preq) {
        /* Check if all the requirements for the currently evaluated effect
         * are met, except for having the tech that we are evaluating.
         * TODO: Consider requirements that could be met later. */
        if (VUT_GOVERNMENT == preq->source.kind
            && preq->source.value.govern == gov) {
          present = preq->present;
          continue;
        }
        if (!is_req_active(pplayer, NULL, pcity, NULL, NULL, NULL, NULL,
                           NULL, NULL, NULL, preq, RPT_POSSIBLE)) {
          active = FALSE;
          break; /* presence doesn't matter for inactive effects. */
        }

      } requirement_vector_iterate_end;

      if (active) {
        adv_want v1;

        v1 = dai_effect_value(pplayer, gov, adv, pcity, capital,
                              turns, peffect, 1,
                              nplayers);

        if (!present) {
          /* Tech removes the effect */
          *val -= v1;
        } else {
          *val += v1;
        }
      }
    } effect_list_iterate_end;
  } city_list_iterate_end;

  revolution_turns = get_player_bonus(pplayer, EFT_REVOLUTION_UNHAPPINESS);
  if (revolution_turns > 0) {
    bonus -= 6 / revolution_turns;
  }

  *val += (*val * bonus) / 100;

  /* FIXME: handle reqs other than technologies. */
  dist = 0;
  requirement_vector_iterate(&gov->reqs, preq) {
    if (VUT_ADVANCE == preq->source.kind) {
      dist += MAX(1, research_goal_unknown_techs(presearch,
                                                 advance_number(preq->source.value.advance)));
    }
  } requirement_vector_iterate_end;
  *val = amortize(*val, dist);

  *override = TRUE;
}
