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

#include <string.h>

#include "game.h"
#include "government.h"
#include "log.h"
#include "player.h"
#include "tech.h"

#include "plrhand.h"
#include "techtools.h"

#include "advmilitary.h"
#include "ailog.h"
#include "aitools.h"

#include "aitech.h"

struct ai_tech_choice {
  Tech_type_id choice;   /* The id of the most needed tech */
  int want;              /* Want of the most needed tech */
  int current_want;      /* Want of the tech which currenlty researched 
			  * or is our current goal */
};

/**************************************************************************
  Massage the numbers provided to us by ai.tech_want into unrecognizable 
  pulp.

  TODO: Write a transparent formula.

  Notes: 1. num_unknown_techs_for_goal returns 0 for known techs, 1 if tech 
  is immediately available etc.
  2. A tech is reachable means we can research it now; tech is available 
  means it's on our tech tree (different nations can have different techs).
  3. ai.tech_want is usually upped by each city, so it is divided by number
  of cities here.
  4. A tech isn't a requirement of itself.
**************************************************************************/
static void ai_select_tech(struct player *pplayer, 
			   struct ai_tech_choice *choice,
			   struct ai_tech_choice *goal)
{
  Tech_type_id newtech, newgoal;
  int num_cities_nonzero = MAX(1, city_list_size(pplayer->cities));
  int values[A_LAST];
  int goal_values[A_LAST];

  memset(values, 0, sizeof(values));
  memset(goal_values, 0, sizeof(goal_values));
  values[A_UNSET] = -1;
  values[A_NONE] = -1;
  goal_values[A_UNSET] = -1;
  goal_values[A_NONE] = -1;

  /* if we are researching future techs, then simply continue with that. 
   * we don't need to do anything below. */
  if (is_future_tech(get_player_research(pplayer)->researching)) {
    if (choice) {
      choice->choice = get_player_research(pplayer)->researching;
      choice->want = 1;
      choice->current_want = 1;
    }
    if (goal) {
      goal->choice = get_player_research(pplayer)->tech_goal;
      goal->want = 1;
      goal->current_want = 1;
    }
    return;
  }  

  /* Fill in values for the techs: want of the tech 
   * + average want of those we will discover en route */
  advance_index_iterate(A_FIRST, i) {
    if (valid_advance_by_number(i)) {
      int steps = num_unknown_techs_for_goal(pplayer, i);

      /* We only want it if we haven't got it (so AI is human after all) */
      if (steps > 0) { 
        values[i] += pplayer->ai_data.tech_want[i];
	advance_index_iterate(A_FIRST, k) {
	  if (is_tech_a_req_for_goal(pplayer, k, i)) {
            values[k] += pplayer->ai_data.tech_want[i] / steps;
	  }
	} advance_index_iterate_end;
      }
    }
  } advance_index_iterate_end;

  /* Fill in the values for the tech goals */
  advance_index_iterate(A_FIRST, i) {
    if (valid_advance_by_number(i)) {
      int steps = num_unknown_techs_for_goal(pplayer, i);

      if (steps == 0) {
	continue;
      }

      goal_values[i] = values[i];      
      advance_index_iterate(A_FIRST, k) {
	if (is_tech_a_req_for_goal(pplayer, k, i)) {
	  goal_values[i] += values[k];
	}
      } advance_index_iterate_end;

      /* This is the best I could do.  It still sometimes does freaky stuff
       * like setting goal to Republic and learning Monarchy, but that's what
       * it's supposed to be doing; it just looks strange. -- Syela */
      goal_values[i] /= steps;
      if (steps < 6) {
	freelog(LOG_DEBUG, "%s: want = %d, value = %d, goal_value = %d",
                advance_name_by_player(pplayer, i),
                pplayer->ai_data.tech_want[i],
		values[i], goal_values[i]);
      }
    }
  } advance_index_iterate_end;

  newtech = A_UNSET;
  newgoal = A_UNSET;
  advance_index_iterate(A_FIRST, i) {
    if (valid_advance_by_number(i)) {
      if (values[i] > values[newtech]
	  && player_invention_reachable(pplayer, i)
	  && player_invention_state(pplayer, i) == TECH_PREREQS_KNOWN) {
	newtech = i;
      }
      if (goal_values[i] > goal_values[newgoal]
	  && player_invention_reachable(pplayer, i)) {
	newgoal = i;
      }
    }
  } advance_index_iterate_end;
#ifdef REALLY_DEBUG_THIS
  advance_index_iterate(A_FIRST, id) {
    if (values[id] > 0
        && player_invention_state(pplayer, id) == TECH_PREREQS_KNOWN) {
      TECH_LOG(LOG_DEBUG, pplayer, advance_by_number(id),
              "turn end want: %d", values[id]);
    }
  } advance_index_iterate_end;
#endif
  if (choice) {
    choice->choice = newtech;
    choice->want = values[newtech] / num_cities_nonzero;
    choice->current_want = 
      values[get_player_research(pplayer)->researching] / num_cities_nonzero;
  }

  if (goal) {
    goal->choice = newgoal;
    goal->want = goal_values[newgoal] / num_cities_nonzero;
    goal->current_want
      = (goal_values[get_player_research(pplayer)->tech_goal]
	 / num_cities_nonzero);
    freelog(LOG_DEBUG,
	    "Goal->choice = %s, goal->want = %d, goal_value = %d, "
	    "num_cities_nonzero = %d",
	    advance_name_by_player(pplayer, goal->choice), goal->want,
	    goal_values[newgoal],
	    num_cities_nonzero);
  }

  /* we can't have this, which will happen in the circumstance 
   * where all ai.tech_wants are negative */
  if (choice && choice->choice == A_UNSET) {
    choice->choice = get_player_research(pplayer)->researching;
  }

  return;
}

/**************************************************************************
  Key AI research function. Disable if we are in a team with human team
  mates in a research pool.
**************************************************************************/
void ai_manage_tech(struct player *pplayer)
{
  struct ai_tech_choice choice, goal;
  struct player_research *research = get_player_research(pplayer);
  /* Penalty for switching research */
  int penalty = (research->got_tech ? 0 : research->bulbs_researched);

  /* If there are humans in our team, they will choose the techs */
  players_iterate(aplayer) {
    const struct player_diplstate *ds = pplayer_get_diplstate(pplayer, aplayer);

    if (ds->type == DS_TEAM) {
      return;
    }
  } players_iterate_end;

  ai_select_tech(pplayer, &choice, &goal);
  if (choice.choice != research->researching) {
    /* changing */
    if ((choice.want - choice.current_want) > penalty &&
	penalty + research->bulbs_researched <=
	total_bulbs_required(pplayer)) {
      TECH_LOG(LOG_DEBUG, pplayer, advance_by_number(choice.choice), 
               "new research, was %s, penalty was %d", 
               advance_name_by_player(pplayer, research->researching),
               penalty);
      choose_tech(pplayer, choice.choice);
    }
  }

  /* crossing my fingers on this one! -- Syela (seems to have worked!) */
  /* It worked, in particular, because the value it sets (research->tech_goal)
   * is practically never used, see the comment for ai_next_tech_goal */
  if (goal.choice != research->tech_goal) {
    freelog(LOG_DEBUG, "%s change goal from %s (want=%d) to %s (want=%d)",
	    player_name(pplayer),
	    advance_name_by_player(pplayer, research->tech_goal), 
	    goal.current_want,
	    advance_name_by_player(pplayer, goal.choice),
	    goal.want);
    choose_tech_goal(pplayer, goal.choice);
  }
}

/**************************************************************************
  Returns the best unit we can build, or NULL if none.  "Best" here
  means last in the unit list as defined in the ruleset.  Assigns tech 
  wants for techs to get better units with given role, but only for the
  cheapest to research "next" unit up the "chain".
**************************************************************************/
struct unit_type *ai_wants_role_unit(struct player *pplayer,
				     struct city *pcity,
				     int role, int want)
{
  int i, n;
  int best_cost = FC_INFINITY;
  struct advance *best_tech = A_NEVER;
  struct unit_type *best_unit = NULL;
  struct unit_type *build_unit = NULL;

  n = num_role_units(role);
  for (i = n - 1; i >= 0; i--) {
    struct unit_type *iunit = get_role_unit(role, i);
    struct advance *itech = iunit->require_advance;

    if (can_city_build_unit_now(pcity, iunit)) {
      build_unit = iunit;
      break;
    } else if (can_city_build_unit_later(pcity, iunit)) {
      int cost = 0;

      if (A_NEVER != itech
       && player_invention_state(pplayer, advance_number(itech)) != TECH_KNOWN) {
        /* See if we want to invent this. */
        cost = total_bulbs_required_for_goal(pplayer, advance_number(itech));
      }
      if (iunit->need_improvement 
          && !can_player_build_improvement_direct(pplayer, iunit->need_improvement)) {
        struct impr_type *building = iunit->need_improvement;

	requirement_vector_iterate(&building->reqs, preq) {
	  if (VUT_ADVANCE == preq->source.kind) {
	    int iimprtech = advance_number(preq->source.value.advance);

	    if (TECH_KNOWN != player_invention_state(pplayer, iimprtech)) {
	      int imprcost = total_bulbs_required_for_goal(pplayer, iimprtech);

	      if (imprcost < cost || cost == 0) {
	        /* If we already have the primary tech (cost==0),
	         * or the building's tech is cheaper,
	         * go for the building's required tech. */
	        itech = preq->source.value.advance;
	        cost = 0;
	      }
	      cost += imprcost;
	    }
	  }
	} requirement_vector_iterate_end;
      }

      if (cost < best_cost
       && player_invention_reachable(pplayer, advance_number(itech))) {
        best_tech = itech;
        best_cost = cost;
        best_unit = iunit;
      }
    }
  }

  if (A_NEVER != best_tech) {
    /* Crank up chosen tech want */
    if (build_unit != NULL) {
      /* We already have a role unit of this kind */
      want /= 2;
    }
    pplayer->ai_data.tech_want[advance_index(best_tech)] += want;
    TECH_LOG(LOG_DEBUG, pplayer, best_tech,
             "+ %d for %s by role",
             want,
             utype_rule_name(best_unit));
  }
  return build_unit;
}
