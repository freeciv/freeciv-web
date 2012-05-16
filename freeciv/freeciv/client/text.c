/********************************************************************** 
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
#include <config.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <string.h>

/* utility */
#include "astring.h"
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "combat.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "unitlist.h"

/* client */
#include "client_main.h"
#include "climap.h"
#include "climisc.h"

#include "control.h"
#include "goto.h"
#include "text.h"

/****************************************************************************
  Return a (static) string with a tile's food/prod/trade
****************************************************************************/
const char *get_tile_output_text(const struct tile *ptile)
{
  static struct astring str = ASTRING_INIT;
  int i;
  char output_text[O_LAST][16];

  for (i = 0; i < O_LAST; i++) {
    int before_penalty = 0;
    int x = city_tile_output(NULL, ptile, FALSE, i);

    if (NULL != client.conn.playing) {
      before_penalty = get_player_output_bonus(client.conn.playing,
                                               get_output_type(i),
                                               EFT_OUTPUT_PENALTY_TILE);
    }

    if (before_penalty > 0 && x > before_penalty) {
      my_snprintf(output_text[i], sizeof(output_text[i]), "%d(-1)", x);
    } else {
      my_snprintf(output_text[i], sizeof(output_text[i]), "%d", x);
    }
  }
  
  astr_clear(&str);
  astr_add_line(&str, "%s/%s/%s",
                output_text[O_FOOD],
		output_text[O_SHIELD],
		output_text[O_TRADE]);

  return str.str;
}

/****************************************************************************
  For AIs, fill the buffer with their player name prefixed with "AI". For
  humans, just fill it with their username.
****************************************************************************/
static inline void get_full_username(char *buf, int buflen,
                                     const struct player *pplayer)
{
  if (!buf || buflen < 1) {
    return;
  }

  if (!pplayer) {
    buf[0] = '\0';
    return;
  }

  if (pplayer->ai_data.control) {
    /* TRANS: "AI <player name>" */
    my_snprintf(buf, buflen, _("AI %s"), pplayer->name);
  } else {
    mystrlcpy(buf, pplayer->username, buflen);
  }
}

/****************************************************************************
  Fill the buffer with the player's nation name (in adjective form) and
  optionally add the player's team name.
****************************************************************************/
static inline void get_full_nation(char *buf, int buflen,
                                   const struct player *pplayer)
{
  if (!buf || buflen < 1) {
    return;
  }

  if (!pplayer) {
    buf[0] = '\0';
    return;
  }

  if (pplayer->team) {
    /* TRANS: "<nation adjective>, team <team name>" */
    my_snprintf(buf, buflen, _("%s, team %s"),
                nation_adjective_for_player(pplayer),
                team_name_translation(pplayer->team));
  } else {
    mystrlcpy(buf, nation_adjective_for_player(pplayer), buflen);
  }
}

/****************************************************************************
  Text to popup on a middle-click in the mapview.
****************************************************************************/
const char *popup_info_text(struct tile *ptile)
{
  const char *activity_text;
  struct city *pcity = tile_city(ptile);
  struct unit *punit = find_visible_unit(ptile);
  const char *diplo_nation_plural_adjectives[DS_LAST] =
    {Q_("?nation:Neutral"), Q_("?nation:Hostile"),
     Q_("?nation:Neutral"),
     Q_("?nation:Peaceful"), Q_("?nation:Friendly"), 
     Q_("?nation:Mysterious"), Q_("?nation:Friendly(team)")};
  const char *diplo_city_adjectives[DS_LAST] =
    {Q_("?city:Neutral"), Q_("?city:Hostile"),
     Q_("?nation:Neutral"),
     Q_("?city:Peaceful"), Q_("?city:Friendly"), Q_("?city:Mysterious"),
     Q_("?city:Friendly(team)")};
  int infracount;
  bv_special infra;
  static struct astring str = ASTRING_INIT;
  char username[MAX_LEN_NAME + 32];
  char nation[2 * MAX_LEN_NAME + 32];

  astr_clear(&str);
  astr_add_line(&str, _("Location: (%d, %d) [%d]"),
                ptile->x, ptile->y, tile_continent(ptile));
  astr_add_line(&str, _("Native coordinates: (%d, %d)"),
                ptile->nat_x, ptile->nat_y);

  if (client_tile_get_known(ptile) == TILE_UNKNOWN) {
    astr_add(&str, _("Unknown"));
    return str.str;
  }
  astr_add_line(&str, _("Terrain: %s"),  tile_get_info_text(ptile, 0));
  astr_add_line(&str, _("Food/Prod/Trade: %s"),
		get_tile_output_text(ptile));
  if (tile_has_special(ptile, S_HUT)) {
    astr_add_line(&str, _("Minor Tribe Village"));
  }
  if (game.info.borders > 0 && !pcity) {
    struct player *owner = tile_owner(ptile);

    get_full_username(username, sizeof(username), owner);
    get_full_nation(nation, sizeof(nation), owner);

    if (NULL != client.conn.playing && owner == client.conn.playing) {
      astr_add_line(&str, _("Our territory"));
    } else if (NULL != owner && NULL == client.conn.playing) {
      /* TRANS: "Territory of <username> (<nation + team>)" */
      astr_add_line(&str, _("Territory of %s (%s)"), username, nation);
    } else if (NULL != owner) {
      struct player_diplstate *ds = client.conn.playing->diplstates;

      if (ds[player_index(owner)].type == DS_CEASEFIRE) {
	int turns = ds[player_index(owner)].turns_left;

        /* TRANS: "Territory of <username> (<nation + team>)
         * (<number> turn cease-fire)" */
        astr_add_line(&str,
                      PL_("Territory of %s (%s) (%d turn cease-fire)",
                          "Territory of %s (%s) (%d turn cease-fire)",
                          turns),
                      username, nation, turns);
      } else {
	int type = ds[player_index(owner)].type;

        /* TRANS: "Territory of <username>
         * (<nation + team> | <diplomatic state>)" */
        astr_add_line(&str, _("Territory of %s (%s | %s)"),
                      username, nation,
                      diplo_nation_plural_adjectives[type]);
      }
    } else {
      astr_add_line(&str, _("Unclaimed territory"));
    }
  }
  if (pcity) {
    /* Look at city owner, not tile owner (the two should be the same, if
     * borders are in use). */
    struct player *owner = city_owner(pcity);
    int has_improvements = 0;
    struct impr_type *prev_impr = NULL;

    get_full_username(username, sizeof(username), owner);
    get_full_nation(nation, sizeof(nation), owner);

    if (NULL == client.conn.playing || owner == client.conn.playing) {
      /* TRANS: "City: <city name> | <username> (<nation + team>)" */
      astr_add_line(&str, _("City: %s | %s (%s)"),
                    city_name(pcity), username, nation);
    } else {
      struct player_diplstate *ds = client.conn.playing->diplstates;

      if (ds[player_index(owner)].type == DS_CEASEFIRE) {
	int turns = ds[player_index(owner)].turns_left;

        /* TRANS:  "City: <city name> | <username>
         * (<nation + team>, <number> turn cease-fire)" */
        astr_add_line(&str, PL_("City: %s | %s (%s, %d turn cease-fire)",
                                "City: %s | %s (%s, %d turn cease-fire)",
                                turns),
                      city_name(pcity), username, nation, turns);
      } else {
        /* TRANS: "City: <city name> | <username>
         * (<nation + team>, <diplomatic state>)" */
        astr_add_line(&str, _("City: %s | %s (%s, %s)"),
                      city_name(pcity), username, nation,
                      diplo_city_adjectives[ds[player_index(owner)].type]);
      }
    }
    improvement_iterate(pimprove) {
      if (is_improvement_visible(pimprove)
          && city_has_building(pcity, pimprove)) {
        if (has_improvements++ > 1) {
          /* TRANS: continue list, in case comma is not the separator of choice. */
          astr_add(&str, Q_("?clistmore:, %s"),
                   improvement_name_translation(prev_impr));
        } else if (has_improvements == 1) {
          astr_add_line(&str, "  ");
          /* TRANS: previous lines gave other information about the city. */
          astr_add(&str, Q_("?clistbegin: with %s"),
                   improvement_name_translation(pimprove));
        }
        prev_impr = pimprove;
      }
    } improvement_iterate_end;

    if (NULL != prev_impr) {
      if (has_improvements > 2) {
        /* TRANS: This appears with two or more previous entries in the list */
        astr_add(&str, Q_("?clistlast:, and %s"),
		 improvement_name_translation(prev_impr));
      } else if (has_improvements > 1) {
        /* TRANS: This appears with only one previous entry in the list */
        astr_add(&str, Q_("?clistlast: and %s"),
		 improvement_name_translation(prev_impr));
      }
      astr_add(&str, "%s", Q_("?clistend:."));
    }

    unit_list_iterate(get_units_in_focus(), pfocus_unit) {
      struct city *hcity = game_find_city_by_number(pfocus_unit->homecity);

      if (unit_has_type_flag(pfocus_unit, F_TRADE_ROUTE)
	  && can_cities_trade(hcity, pcity)
	  && can_establish_trade_route(hcity, pcity)) {
	/* TRANS: "Trade from Warsaw: 5" */
	astr_add_line(&str, _("Trade from %s: %d"),
		      city_name(hcity),
		      trade_between_cities(hcity, pcity));
      }
    } unit_list_iterate_end;
  }
  infra = get_tile_infrastructure_set(ptile, &infracount);
  if (infracount == 0) {
    base_type_iterate(pbase) {
      if (BV_ISSET(ptile->bases, base_index(pbase))) {
        infracount = 1;
        break;
      }
    } base_type_iterate_end;
  }
  if (infracount > 0) {
    astr_add_line(&str, _("Infrastructure: %s"),
		  get_infrastructure_text(ptile->special, ptile->bases));
  }
  activity_text = concat_tile_activity_text(ptile);
  if (strlen(activity_text) > 0) {
    astr_add_line(&str, _("Activity: %s"), activity_text);
  }
  if (punit && !pcity) {
    struct player *owner = unit_owner(punit);
    struct unit_type *ptype = unit_type(punit);

    get_full_username(username, sizeof(username), owner);
    get_full_nation(nation, sizeof(nation), owner);

    if (NULL == client.conn.playing || owner == client.conn.playing) {
      struct city *pcity = player_find_city_by_id(owner, punit->homecity);

      if (pcity) {
        /* TRANS: "Unit: <unit type> | <username>
         * (<nation + team>, <homecity>)" */
        astr_add_line(&str, _("Unit: %s | %s (%s, %s)"),
                      utype_name_translation(ptype), username,
                      nation, city_name(pcity));
      } else {
        /* TRANS: "Unit: <unit type> | <username> (<nation + team>)" */
        astr_add_line(&str, _("Unit: %s | %s (%s)"),
                      utype_name_translation(ptype), username, nation);
      }
    } else if (NULL != owner) {
      struct player_diplstate *ds = client.conn.playing->diplstates;

      if (ds[player_index(owner)].type == DS_CEASEFIRE) {
	int turns = ds[player_index(owner)].turns_left;

        /* TRANS:  "Unit: <unit type> | <username> (<nation + team>,
         * <number> turn cease-fire)" */
        astr_add_line(&str, PL_("Unit: %s | %s (%s, %d turn cease-fire)",
                                "Unit: %s | %s (%s, %d turn cease-fire)",
                                turns),
                      utype_name_translation(ptype),
                      username, nation, turns);
      } else {
        /* TRANS: "Unit: <unit type> | <username> (<nation + team>,
         * <diplomatic state>)" */
        astr_add_line(&str, _("Unit: %s | %s (%s, %s)"),
                      utype_name_translation(ptype), username, nation,
                      diplo_city_adjectives[ds[player_index(owner)].type]);
      }
    }

    unit_list_iterate(get_units_in_focus(), pfocus_unit) {
      int att_chance = FC_INFINITY, def_chance = FC_INFINITY;
      bool found = FALSE;

      unit_list_iterate(ptile->units, tile_unit) {
	if (unit_owner(tile_unit) != unit_owner(pfocus_unit)) {
	  int att = unit_win_chance(pfocus_unit, tile_unit) * 100;
	  int def = (1.0 - unit_win_chance(tile_unit, pfocus_unit)) * 100;

	  found = TRUE;

	  /* Presumably the best attacker and defender will be used. */
	  att_chance = MIN(att, att_chance);
	  def_chance = MIN(def, def_chance);
	}
      } unit_list_iterate_end;

      if (found) {
	/* TRANS: "Chance to win: A:95% D:46%" */
	astr_add_line(&str, _("Chance to win: A:%d%% D:%d%%"),
		      att_chance, def_chance);	
      }
    } unit_list_iterate_end;

    /* TRANS: A is attack power, D is defense power, FP is firepower,
     * HP is hitpoints (current and max). */
    astr_add_line(&str, _("A:%d D:%d FP:%d HP:%d/%d (%s)"),
		  ptype->attack_strength, 
		  ptype->defense_strength, ptype->firepower, punit->hp, 
		  ptype->hp,
		  _(ptype->veteran[punit->veteran].name));

    if ((NULL == client.conn.playing || owner == client.conn.playing)
	&& unit_list_size(ptile->units) >= 2) {
      /* TRANS: "5 more" units on this tile */
      astr_add(&str, _("  (%d more)"), unit_list_size(ptile->units) - 1);
    }
  } 
  return str.str;
}

/****************************************************************************
  Creates the activity progress text for the given tile.

  This should only be used inside popup_info_text and should eventually be
  made static.
****************************************************************************/
const char *concat_tile_activity_text(struct tile *ptile)
{
  int activity_total[ACTIVITY_LAST];
  int activity_units[ACTIVITY_LAST];
  int base_total[MAX_BASE_TYPES];
  int base_units[MAX_BASE_TYPES];
  int num_activities = 0;
  int remains, turns, i;
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  memset(activity_total, 0, sizeof(activity_total));
  memset(activity_units, 0, sizeof(activity_units));
  memset(base_total, 0, sizeof(base_total));
  memset(base_units, 0, sizeof(base_units));

  unit_list_iterate(ptile->units, punit) {
    if (punit->activity == ACTIVITY_BASE) {
      base_total[punit->activity_base] += punit->activity_count;
      base_total[punit->activity_base] += get_activity_rate_this_turn(punit);
      base_units[punit->activity_base] += get_activity_rate(punit);
    } else {
      activity_total[punit->activity] += punit->activity_count;
      activity_total[punit->activity] += get_activity_rate_this_turn(punit);
      activity_units[punit->activity] += get_activity_rate(punit);
    }
  } unit_list_iterate_end;

  for (i = 0; i < ACTIVITY_LAST; i++) {
    if (i == ACTIVITY_BASE) {
      base_type_iterate(bp) {
        Base_type_id b = base_index(bp);
	if (base_units[b] > 0) {
	  remains = tile_activity_base_time(ptile, b) - base_total[b];
	  if (remains > 0) {
	    turns = 1 + (remains + base_units[b] - 1) / base_units[b];
	  } else {
	    /* base will be finished this turn */
	    turns = 1;
	  }
	  if (num_activities > 0) {
	    astr_add(&str, "/");
	  }
	  astr_add(&str, "%s(%d)", base_name_translation(bp), turns);
	  num_activities++;
	}
      } base_type_iterate_end;
    } else if (is_build_or_clean_activity(i) && activity_units[i] > 0) {
      if (num_activities > 0) {
	astr_add(&str, "/");
      }
      remains = tile_activity_time(i, ptile) - activity_total[i];
      if (remains > 0) {
	turns = 1 + (remains + activity_units[i] - 1) / activity_units[i];
      } else {
	/* activity will be finished this turn */
	turns = 1;
      }
      astr_add(&str, "%s(%d)", get_activity_text(i), turns);
      num_activities++;
    }
  }

  return str.str;
}

#define FAR_CITY_SQUARE_DIST (2*(6*6))

/****************************************************************************
  Returns the text describing the city and its distance.
****************************************************************************/
const char *get_nearest_city_text(struct city *pcity, int sq_dist)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  /* just to be sure */
  if (!pcity) {
    sq_dist = -1;
  }

  astr_add(&str, (sq_dist >= FAR_CITY_SQUARE_DIST)
                 /* TRANS: on own line immediately following \n, ... <city> */
                 ? _("far from %s")
                 : (sq_dist > 0)
                   /* TRANS: on own line immediately following \n, ... <city> */
                   ? _("near %s")
                   : (sq_dist == 0)
                     /* TRANS: on own line immediately following \n, ... <city> */
                     ? _("in %s")
                     : "%s",
                 pcity
                 ? city_name(pcity)
                 : "");

  return str.str;
}

/****************************************************************************
  Returns the unit description.

  FIXME: This function is not re-entrant because it returns a pointer to
  static data.
****************************************************************************/
const char *unit_description(struct unit *punit)
{
  int pcity_near_dist;
  struct city *pcity =
      player_find_city_by_id(unit_owner(punit), punit->homecity);
  struct city *pcity_near = get_nearest_city(punit, &pcity_near_dist);
  struct unit_type *ptype = unit_type(punit);
  static struct astring str = ASTRING_INIT;
  const struct player *pplayer = client_player();

  astr_clear(&str);

  astr_add(&str, "%s", utype_name_translation(ptype));

  if (ptype->veteran[punit->veteran].name[0] != '\0') {
    astr_add(&str, " (%s)", _(ptype->veteran[punit->veteran].name));
  }

  if (pplayer == unit_owner(punit)) {
    unit_upkeep_astr(punit, &str);
  }
  astr_add(&str, "\n");
  unit_activity_astr(punit, &str);

  if (pcity) {
    /* TRANS: on own line immediately following \n, ... <city> */
    astr_add_line(&str, _("from %s"), city_name(pcity));
  } else {
    astr_add(&str, "\n");
  }

  astr_add_line(&str, "%s",
		get_nearest_city_text(pcity_near, pcity_near_dist));
#ifdef DEBUG
  astr_add_line(&str, "Unit ID: %d", punit->id);
#endif

  return str.str;
}

/****************************************************************************
  Return total expected bulbs.
****************************************************************************/
static int get_bulbs_per_turn(int *pours, int *ptheirs)
{
  int ours = 0, theirs = 0;

  if (NULL == client.conn.playing) {
    return 0;
  }

  /* Sum up science */
  players_iterate(pplayer) {
    enum diplstate_type ds = pplayer_get_diplstate(client.conn.playing, pplayer)->type;

    if (pplayer == client.conn.playing) {
      city_list_iterate(pplayer->cities, pcity) {
        ours += pcity->prod[O_SCIENCE];
      } city_list_iterate_end;
    } else if (ds == DS_TEAM) {
      theirs += pplayer->bulbs_last_turn;
    }
  } players_iterate_end;

  if (pours) {
    *pours = ours;
  }
  if (ptheirs) {
    *ptheirs = theirs;
  }
  return ours + theirs;
}

/****************************************************************************
  Returns the text to display in the science dialog.
****************************************************************************/
const char *science_dialog_text(void)
{
  int ours, theirs;
  static struct astring str = ASTRING_INIT;
  char ourbuf[1024] = "", theirbuf[1024] = "";

  astr_clear(&str);

  get_bulbs_per_turn(&ours, &theirs);

  if (NULL == client.conn.playing || (ours == 0 && theirs == 0)) {
    return _("Progress: no research");
  }
  assert(ours >= 0 && theirs >= 0);

  if (A_UNSET == get_player_research(client.conn.playing)->researching) {
    astr_add(&str, _("Progress: no research target"));
  } else {
    int turns_to_advance = ((total_bulbs_required(client.conn.playing) + ours + theirs - 1)
			    / (ours + theirs));

    astr_add(&str, PL_("Progress: %d turn/advance",
		       "Progress: %d turns/advance",
		       turns_to_advance), turns_to_advance);
  }
  my_snprintf(ourbuf, sizeof(ourbuf),
	      PL_("%d bulb/turn", "%d bulbs/turn", ours), ours);
  if (theirs > 0) {
    /* Techpool version */
    /* TRANS: This is appended to "%d bulb/turn" text */
    my_snprintf(theirbuf, sizeof(theirbuf),
		PL_(", %d bulb/turn from team",
		    ", %d bulbs/turn from team", theirs), theirs);
  }
  astr_add(&str, " (%s%s)", ourbuf, theirbuf);
  return str.str;
}

/****************************************************************************
  Get the short science-target text.  This is usually shown directly in
  the progress bar.

     5/28 - 3 turns

  The "percent" value, if given, will be set to the completion percentage
  of the research target (actually it's a [0,1] scale not a percent).
****************************************************************************/
const char *get_science_target_text(double *percent)
{
  struct player_research *research = get_player_research(client.conn.playing);
  static struct astring str = ASTRING_INIT;

  if (!research) {
    return "-";
  }

  astr_clear(&str);
  if (research->researching == A_UNSET) {
    astr_add(&str, _("%d/- (never)"), research->bulbs_researched);
    if (percent) {
      *percent = 0.0;
    }
  } else {
    int total = total_bulbs_required(client.conn.playing);
    int done = research->bulbs_researched;
    int perturn = get_bulbs_per_turn(NULL, NULL);

    if (perturn > 0) {
      int turns = (total - done + perturn - 1) / perturn;

      astr_add(&str, PL_("%d/%d (%d turn)", "%d/%d (%d turns)", turns),
	       done, total, turns);
    } else {
      astr_add(&str, _("%d/%d (never)"), done, total);
    }
    if (percent) {
      *percent = (double)done / (double)total;
      *percent = CLIP(0.0, *percent, 1.0);
    }
  }

  return str.str;
}

/****************************************************************************
  Set the science-goal-label text as if we're researching the given goal.
****************************************************************************/
const char *get_science_goal_text(Tech_type_id goal)
{
  int steps = num_unknown_techs_for_goal(client.conn.playing, goal);
  int bulbs = total_bulbs_required_for_goal(client.conn.playing, goal);
  int bulbs_needed = bulbs, turns;
  int perturn = get_bulbs_per_turn(NULL, NULL);
  char buf1[256], buf2[256], buf3[256];
  struct player_research* research = get_player_research(client.conn.playing);
  static struct astring str = ASTRING_INIT;

  if (!research) {
    return "-";
  }

  astr_clear(&str);

  if (is_tech_a_req_for_goal(client.conn.playing,
			     research->researching, goal)
      || research->researching == goal) {
    bulbs_needed -= research->bulbs_researched;
  }

  my_snprintf(buf1, sizeof(buf1),
	      PL_("%d step", "%d steps", steps), steps);
  my_snprintf(buf2, sizeof(buf2),
	      PL_("%d bulb", "%d bulbs", bulbs), bulbs);
  if (perturn > 0) {
    turns = (bulbs_needed + perturn - 1) / perturn;
    my_snprintf(buf3, sizeof(buf3),
		PL_("%d turn", "%d turns", turns), turns);
  } else {
    my_snprintf(buf3, sizeof(buf3), _("never"));
  }
  astr_add_line(&str, "(%s - %s - %s)", buf1, buf2, buf3);

  return str.str;
}

/****************************************************************************
  Return the text for the label on the info panel.  (This is traditionally
  shown to the left of the mapview.)

  Clicking on this text should bring up the get_info_label_text_popup text.
****************************************************************************/
const char *get_info_label_text(void)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  if (NULL != client.conn.playing) {
    astr_add_line(&str, _("Population: %s"),
		  population_to_text(civ_population(client.conn.playing)));
  }
  astr_add_line(&str, _("Year: %s (T%d)"),
		textyear(game.info.year), game.info.turn);

  if (NULL != client.conn.playing) {
    astr_add_line(&str, _("Gold: %d (%+d)"),
		  client.conn.playing->economic.gold,
		  player_get_expected_income(client.conn.playing));
    astr_add_line(&str, _("Tax: %d Lux: %d Sci: %d"),
		  client.conn.playing->economic.tax,
		  client.conn.playing->economic.luxury,
		  client.conn.playing->economic.science);
  }
  if (game.info.phase_mode == PMT_PLAYERS_ALTERNATE) {
    if (game.info.phase < 0 || game.info.phase >= player_count()) {
      astr_add_line(&str, _("Moving: Nobody"));
    } else {
      astr_add_line(&str, _("Moving: %s"),
                    player_name(player_by_number(game.info.phase)));
    }
  } else if (game.info.phase_mode == PMT_TEAMS_ALTERNATE) {
    if (game.info.phase < 0 || game.info.phase >= team_count()) {
      astr_add_line(&str, _("Moving: Nobody"));
    } else {
      astr_add_line(&str, _("Moving: %s"),
                    team_name_translation(team_by_number(game.info.phase)));
    }
  }
  astr_add_line(&str, _("(Click for more info)"));
  return str.str;
}

/****************************************************************************
  Return the text for the popup label on the info panel.  (This is
  traditionally done as a popup whenever the regular info text is clicked
  on.)
****************************************************************************/
const char *get_info_label_text_popup(void)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  if (NULL != client.conn.playing) {
    astr_add_line(&str, _("%s People"),
		  population_to_text(civ_population(client.conn.playing)));
  }
  astr_add_line(&str, _("Year: %s"), textyear(game.info.year));
  astr_add_line(&str, _("Turn: %d"), game.info.turn);

  if (NULL != client.conn.playing) {
    astr_add_line(&str, _("Gold: %d"),
		  client.conn.playing->economic.gold);
    astr_add_line(&str, _("Net Income: %d"),
		  player_get_expected_income(client.conn.playing));
    /* TRANS: Gold, luxury, and science rates are in percentage values. */
    astr_add_line(&str, _("Tax rates: Gold:%d%% Luxury:%d%% Science:%d%%"),
		  client.conn.playing->economic.tax,
		  client.conn.playing->economic.luxury,
		  client.conn.playing->economic.science);
    astr_add_line(&str, _("Researching %s: %s"),
		  advance_name_researching(client.conn.playing),
		  get_science_target_text(NULL));
  }

  /* These mirror the code in get_global_warming_tooltip and
   * get_nuclear_winter_tooltip. */
  astr_add_line(&str, _("Global warming chance: %d%% (%+d%%/turn)"),
		CLIP(0, (game.info.globalwarming + 1) / 2, 100),
		DIVIDE(game.info.heating - game.info.warminglevel + 1, 2));
  astr_add_line(&str, _("Nuclear winter chance: %d%% (%+d%%/turn)"),
		CLIP(0, (game.info.nuclearwinter + 1) / 2, 100),
		DIVIDE(game.info.cooling - game.info.coolinglevel + 1, 2));

  if (NULL != client.conn.playing) {
    astr_add_line(&str, _("Government: %s"),
		  government_name_for_player(client.conn.playing));
  }

  return str.str;
}

/****************************************************************************
  Return the title text for the unit info shown in the info panel.

  FIXME: this should be renamed.
****************************************************************************/
const char *get_unit_info_label_text1(struct unit_list *punits)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  if (punits) {
    int count = unit_list_size(punits);

    if (count == 1) {
      astr_add(&str, "%s", unit_name_translation(unit_list_get(punits, 0)));
    } else {
      astr_add(&str, PL_("%d unit", "%d units", count), count);
    }
  }
  return str.str;
}

/****************************************************************************
  Return the text body for the unit info shown in the info panel.

  FIXME: this should be renamed.
****************************************************************************/
const char *get_unit_info_label_text2(struct unit_list *punits, int linebreaks)
{
  static struct astring str = ASTRING_INIT;
  int count;

  astr_clear(&str);

  if (!punits) {
    return "";
  }

  count = unit_list_size(punits);

  /* This text should always have the same number of lines if
   * 'linebreaks' has no flags at all. Otherwise the GUI widgets may be
   * confused and try to resize themselves. If caller asks for
   * conditional 'linebreaks', it should take care of these problems
   * itself. */

  /* Line 1. Goto or activity text. */
  if (count > 0 && hover_state != HOVER_NONE) {
    int min, max;

    if (!goto_get_turns(&min, &max)) {
      /* TRANS: Impossible to reach goto target tile */
      astr_add_line(&str, "%s", Q_("?goto:Unreachable"));
    } else if (min == max) {
      astr_add_line(&str, _("Turns to target: %d"), max);
    } else {
      astr_add_line(&str, _("Turns to target: %d to %d"), min, max);
    }
  } else if (count == 1) {
    astr_add_line(&str, "%s",
		  unit_activity_text(unit_list_get(punits, 0)));
  } else if (count > 1) {
    astr_add_line(&str, PL_("%d unit selected",
			    "%d units selected",
			    count),
		  count);
  } else {
    astr_add_line(&str, _("No units selected."));
  }

  /* Lines 2, 3, and 4 vary. */
  if (count == 1) {
    struct unit *punit = unit_list_get(punits, 0);
    struct city *pcity =
	player_find_city_by_id(unit_owner(punit), punit->homecity);
    int infracount;
    bv_special infrastructure =
      get_tile_infrastructure_set(punit->tile, &infracount);

    astr_add_line(&str, "%s", tile_get_info_text(punit->tile, linebreaks));
    if (infracount == 0) {
      base_type_iterate(pbase) {
        if (BV_ISSET(punit->tile->bases, base_index(pbase))) {
          infracount = 1;
          break;
        }
      } base_type_iterate_end;
    }
    if (infracount > 0) {
      astr_add_line(&str, "%s", get_infrastructure_text(infrastructure, punit->tile->bases));
    } else {
      astr_add_line(&str, " ");
    }
    if (pcity) {
      astr_add_line(&str, "%s", city_name(pcity));
    } else {
      astr_add_line(&str, " ");
    }
  } else if (count > 1) {
    int mil = 0, nonmil = 0;
    int types_count[U_LAST], i;
    struct unit_type *top[3];

    memset(types_count, 0, sizeof(types_count));
    unit_list_iterate(punits, punit) {
      if (unit_has_type_flag(punit, F_CIVILIAN)) {
	nonmil++;
      } else {
	mil++;
      }
      types_count[utype_index(unit_type(punit))]++;
    } unit_list_iterate_end;

    top[0] = top[1] = top[2] = NULL;
    unit_type_iterate(utype) {
      if (!top[2]
	  || types_count[utype_index(top[2])] < types_count[utype_index(utype)]) {
	top[2] = utype;

	if (!top[1]
	    || types_count[utype_index(top[1])] < types_count[utype_index(top[2])]) {
	  top[2] = top[1];
	  top[1] = utype;

	  if (!top[0]
	      || types_count[utype_index(top[0])] < types_count[utype_index(utype)]) {
	    top[1] = top[0];
	    top[0] = utype;
	  }
	}
      }
    } unit_type_iterate_end;

    for (i = 0; i < 3; i++) {
      if (top[i] && types_count[utype_index(top[i])] > 0) {
	if (utype_has_flag(top[i], F_CIVILIAN)) {
	  nonmil -= types_count[utype_index(top[i])];
	} else {
	  mil -= types_count[utype_index(top[i])];
	}
	astr_add_line(&str, "%d: %s",
		      types_count[utype_index(top[i])],
		      utype_name_translation(top[i]));
      } else {
	astr_add_line(&str, " ");
      }
    }

    if (nonmil > 0 && mil > 0) {
      astr_add_line(&str, _("Others: %d civil; %d military"), nonmil, mil);
    } else if (nonmil > 0) {
      astr_add_line(&str, _("Others: %d civilian"), nonmil);
    } else if (mil > 0) {
      astr_add_line(&str, _("Others: %d military"), mil);
    } else {
      astr_add_line(&str, " ");
    }
  } else {
    astr_add_line(&str, " ");
    astr_add_line(&str, " ");
    astr_add_line(&str, " ");
  }

  /* Line 5. Debug text. */
#ifdef DEBUG
  if (count == 1) {
    astr_add_line(&str, "(Unit ID %d)", unit_list_get(punits, 0)->id);
  } else {
    astr_add_line(&str, " ");
  }
#endif

  return str.str;
}

/****************************************************************************
  Return text about upgrading these unit lists.

  Returns TRUE iff any units can be upgraded.
****************************************************************************/
bool get_units_upgrade_info(char *buf, size_t bufsz,
			    struct unit_list *punits)
{
  if (unit_list_size(punits) == 0) {
    my_snprintf(buf, bufsz, _("No units to upgrade!"));
    return FALSE;
  } else if (unit_list_size(punits) == 1) {
    return (get_unit_upgrade_info(buf, bufsz, unit_list_get(punits, 0))
	    == UR_OK);
  } else {
    int upgrade_cost = 0;
    int num_upgraded = 0;
    int min_upgrade_cost = FC_INFINITY;

    unit_list_iterate(punits, punit) {
      if (unit_owner(punit) == client.conn.playing
	  && test_unit_upgrade(punit, FALSE) == UR_OK) {
	struct unit_type *from_unittype = unit_type(punit);
	struct unit_type *to_unittype = can_upgrade_unittype(client.conn.playing,
							     unit_type(punit));
	int cost = unit_upgrade_price(unit_owner(punit),
					   from_unittype, to_unittype);

	num_upgraded++;
	upgrade_cost += cost;
	min_upgrade_cost = MIN(min_upgrade_cost, cost);
      }
    } unit_list_iterate_end;
    if (num_upgraded == 0) {
      my_snprintf(buf, bufsz, _("None of these units may be upgraded."));
      return FALSE;
    } else {
      /* This may trigger sometimes if you don't have enough money for
       * a full upgrade.  If you have enough to upgrade at least one, it
       * will do it. */
      my_snprintf(buf, bufsz, PL_("Upgrade %d unit for %d gold?\n"
				  "Treasury contains %d gold.", 
				  "Upgrade %d units for %d gold?\n"
				  "Treasury contains %d gold.",
				  num_upgraded),
		  num_upgraded, upgrade_cost, client.conn.playing->economic.gold);
      return TRUE;
    }
  }
}

/****************************************************************************
  Get a tooltip text for the info panel research indicator.  See
  client_research_sprite().
****************************************************************************/
const char *get_bulb_tooltip(void)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  astr_add_line(&str, _("Shows your progress in "
			"researching the current technology."));

  if (NULL != client.conn.playing) {
    struct player_research *research = get_player_research(client.conn.playing);

    if (research->researching == A_UNSET) {
      astr_add_line(&str, _("no research target."));
    } else {
      /* TRANS: <tech>: <amount>/<total bulbs> */
      astr_add_line(&str, _("%s: %d/%d."),
		    advance_name_researching(client.conn.playing),
		    research->bulbs_researched,
		    total_bulbs_required(client.conn.playing));
    }
  }
  return str.str;
}

/****************************************************************************
  Get a tooltip text for the info panel global warning indicator.  See also
  client_warming_sprite().
****************************************************************************/
const char *get_global_warming_tooltip(void)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  /* This mirrors the logic in update_environmental_upset. */
  astr_add_line(&str, _("Shows the progress of global warming:"));
  astr_add_line(&str, _("Pollution rate: %d%%"),
		DIVIDE(game.info.heating - game.info.warminglevel + 1, 2));
  astr_add_line(&str, _("Chance of catastrophic warming each turn: %d%%"),
		CLIP(0, (game.info.globalwarming + 1) / 2, 100));
  return str.str;
}

/****************************************************************************
  Get a tooltip text for the info panel nuclear winter indicator.  See also
  client_cooling_sprite().
****************************************************************************/
const char *get_nuclear_winter_tooltip(void)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  /* This mirrors the logic in update_environmental_upset. */
  astr_add_line(&str, _("Shows the progress of nuclear winter:"));
  astr_add_line(&str, _("Fallout rate: %d%%"),
		DIVIDE(game.info.cooling - game.info.coolinglevel + 1, 2));
  astr_add_line(&str, _("Chance of catastrophic winter each turn: %d%%"),
		CLIP(0, (game.info.nuclearwinter + 1) / 2, 100));
  return str.str;
}

/****************************************************************************
  Get a tooltip text for the info panel government indicator.  See also
  government_by_number(...)->sprite.
****************************************************************************/
const char *get_government_tooltip(void)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  astr_add_line(&str, _("Shows your current government:"));

  if (NULL != client.conn.playing) {
    astr_add_line(&str, "%s",
		  government_name_for_player(client.conn.playing));
  }
  return str.str;
}

/****************************************************************************
  Returns a description of the given spaceship.  If there is no spaceship
  (pship is NULL) then text with dummy values is returned.
****************************************************************************/
const char *get_spaceship_descr(struct player_spaceship *pship)
{
  struct player_spaceship ship;
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  if (!pship) {
    pship = &ship;
    memset(&ship, 0, sizeof(ship));
  }

  /* TRANS: spaceship text; should have constant width. */
  astr_add_line(&str, _("Population:      %5d"), pship->population);

  /* TRANS: spaceship text; should have constant width. */
  astr_add_line(&str, _("Support:         %5d %%"),
		(int) (pship->support_rate * 100.0));

  /* TRANS: spaceship text; should have constant width. */
  astr_add_line(&str, _("Energy:          %5d %%"),
		(int) (pship->energy_rate * 100.0));

  /* TRANS: spaceship text; should have constant width. */
  astr_add_line(&str, PL_("Mass:            %5d ton",
			  "Mass:            %5d tons",
			  pship->mass), pship->mass);

  if (pship->propulsion > 0) {
    /* TRANS: spaceship text; should have constant width. */
    astr_add_line(&str, _("Travel time:     %5.1f years"),
		  (float) (0.1 * ((int) (pship->travel_time * 10.0))));
  } else {
    /* TRANS: spaceship text; should have constant width. */
    astr_add_line(&str, "%s", _("Travel time:        N/A     "));
  }

  /* TRANS: spaceship text; should have constant width. */
  astr_add_line(&str, _("Success prob.:   %5d %%"),
		(int) (pship->success_rate * 100.0));

  /* TRANS: spaceship text; should have constant width. */
  astr_add_line(&str, _("Year of arrival: %8s"),
		(pship->state == SSHIP_LAUNCHED)
		? textyear((int) (pship->launch_year +
				  (int) pship->travel_time))
		: "-   ");

  return str.str;
}

/****************************************************************************
  Get the text showing the timeout.  This is generally disaplyed on the info
  panel.
****************************************************************************/
const char *get_timeout_label_text(void)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  if (game.info.timeout <= 0) {
    astr_add(&str, "%s", Q_("?timeout:off"));
  } else {
    astr_add(&str, "%s", format_duration(get_seconds_to_turndone()));
  }

  return str.str;
}

/****************************************************************************
  Format a duration, in seconds, so it comes up in minutes or hours if
  that would be more meaningful.

  (7 characters, maximum.  Enough for, e.g., "99h 59m".)
****************************************************************************/
const char *format_duration(int duration)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  if (duration < 0) {
    duration = 0;
  }
  if (duration < 60) {
    astr_add(&str, Q_("?seconds:%02ds"), duration);
  } else if (duration < 3600) {	/* < 60 minutes */
    astr_add(&str, Q_("?mins/secs:%02dm %02ds"), duration / 60, duration % 60);
  } else if (duration < 360000) {	/* < 100 hours */
    astr_add(&str, Q_("?hrs/mns:%02dh %02dm"), duration / 3600, (duration / 60) % 60);
  } else if (duration < 8640000) {	/* < 100 days */
    astr_add(&str, Q_("?dys/hrs:%02dd %02dh"), duration / 86400,
	(duration / 3600) % 24);
  } else {
    astr_add(&str, "%s", Q_("?duration:overflow"));
  }

  return str.str;
}

/****************************************************************************
  Return text giving the ping time for the player.  This is generally used
  used in the playerdlg.  This should only be used in playerdlg_common.c.
****************************************************************************/
const char *get_ping_time_text(const struct player *pplayer)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  conn_list_iterate(pplayer->connections, pconn) {
    if (!pconn->observer
	/* Certainly not needed, but safer. */
	&& 0 == strcmp(pconn->username, pplayer->username)) {
      if (pconn->ping_time != -1) {
	double ping_time_in_ms = 1000 * pconn->ping_time;

	astr_add(&str, _("%6d.%02d ms"), (int) ping_time_in_ms,
		 ((int) (ping_time_in_ms * 100.0)) % 100);
      }
      break;
    }
  } conn_list_iterate_end;

  return str.str;
}

/****************************************************************************
  Return text giving the score of the player. This should only be used 
  in playerdlg_common.c.
****************************************************************************/
const char *get_score_text(const struct player *pplayer)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  if (pplayer->score.game > 0
      || NULL == client.conn.playing
      || pplayer == client.conn.playing) {
    astr_add(&str, "%d", pplayer->score.game);
  } else {
    astr_add(&str, "?");
  }

  return str.str;
}

/****************************************************************************
  Get the title for a "report".  This may include the city, economy,
  military, trade, player, etc., reports.  Some clients may generate the
  text themselves to get a better GUI layout.
****************************************************************************/
const char *get_report_title(const char *report_name)
{
  static struct astring str = ASTRING_INIT;
  const struct player *pplayer = client.conn.playing;

  astr_clear(&str);

  astr_add_line(&str, "%s", report_name);

  if (pplayer != NULL) {
    /* TRANS: <nation adjective> <government name>.
     * E.g. "Polish Republic". */
    astr_add_line(&str, _("?nationgovernment:%s %s"),
                  nation_adjective_for_player(pplayer),
                  government_name_for_player(pplayer));

    astr_add_line(&str, "%s %s: %s",
                  ruler_title_translation(pplayer),
                  player_name(pplayer),
                  textyear(game.info.year));
  } else {
    /* TRANS: "Observer: 1985" */
    astr_add_line(&str, _("Observer: %s"),
		  textyear(game.info.year));
  }
  return str.str;
}

/****************************************************************************
  Describing buildings that affect happiness.
****************************************************************************/
const char *text_happiness_buildings(const struct city *pcity)
{
  char buf[512];
  int faces = 0;
  struct effect_list *plist = effect_list_new();
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  astr_add_line(&str, _("Buildings: "));

  get_city_bonus_effects(plist, pcity, NULL, EFT_MAKE_CONTENT);

  effect_list_iterate(plist, peffect) {
    get_effect_req_text(peffect, buf, sizeof(buf));
    if (faces++ > 0) {
      /* only one comment to translators needed. */
      astr_add(&str, Q_("?clistmore:, %s"), buf);
    } else {
      astr_add(&str, "%s", buf);
    }
  } effect_list_iterate_end;
  effect_list_free(plist);

  if (faces == 0) {
    astr_add(&str, _("None. "));
  } else {
    astr_add(&str, "%s", Q_("?clistend:."));
  }

  return str.str;
}

/****************************************************************************
  Describing wonders that affect happiness.
****************************************************************************/
const char *text_happiness_wonders(const struct city *pcity)
{
  char buf[512];
  int faces = 0;
  struct effect_list *plist = effect_list_new();
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  astr_add_line(&str, _("Wonders: "));
  get_city_bonus_effects(plist, pcity, NULL, EFT_MAKE_HAPPY);
  get_city_bonus_effects(plist, pcity, NULL, EFT_FORCE_CONTENT);
  get_city_bonus_effects(plist, pcity, NULL, EFT_NO_UNHAPPY);

  effect_list_iterate(plist, peffect) {
    get_effect_req_text(peffect, buf, sizeof(buf));
    if (faces++ > 0) {
      /* only one comment to translators needed. */
      astr_add(&str, Q_("?clistmore:, %s"), buf);
    } else {
      astr_add(&str, "%s", buf);
    }
  } effect_list_iterate_end;

  effect_list_free(plist);

  if (faces == 0) {
    astr_add(&str, _("None. "));
  } else {
    astr_add(&str, "%s",  Q_("?clistend:."));
  }

  return str.str;
}

/****************************************************************************
  Describing city factors that affect happiness.
****************************************************************************/
const char *text_happiness_cities(const struct city *pcity)
{
  struct player *pplayer = city_owner(pcity);
  int cities = city_list_size(pplayer->cities);
  int content = get_player_bonus(pplayer, EFT_CITY_UNHAPPY_SIZE);
  int basis = get_player_bonus(pplayer, EFT_EMPIRE_SIZE_BASE);
  int step = get_player_bonus(pplayer, EFT_EMPIRE_SIZE_STEP);
  int excess = cities - basis;
  int penalty = 0;
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  if (excess > 0) {
    if (step > 0)
      penalty = 1 + (excess - 1) / step;
    else
      penalty = 1;
  } else {
    excess = 0;
    penalty = 0;
  }

  astr_add_line(&str,
                _("Cities: %d total, %d over threshold of %d cities."),
              cities, excess, basis);
  astr_add_line(&str,
                /* TRANS: 0-21 content [citizen(s)] ... */
                PL_("%d content before penalty.",
                    "%d content before penalty.",
                    content),
                content);
  astr_add_line(&str,
                /* TRANS: 0-21 unhappy citizen(s). */
                PL_("%d additional unhappy citizen.",
                    "%d additional unhappy citizens.",
                    penalty),
                penalty);
  return str.str;
}

/****************************************************************************
  Describing units that affect happiness.
****************************************************************************/
const char *text_happiness_units(const struct city *pcity)
{
  int mlmax = get_city_bonus(pcity, EFT_MARTIAL_LAW_MAX);
  int uhcfac = get_city_bonus(pcity, EFT_UNHAPPY_FACTOR);
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  if (mlmax > 0) {
    int mleach = get_city_bonus(pcity, EFT_MARTIAL_LAW_EACH);
    if (mlmax == 100) {
      astr_add_line(&str, "%s", _("Unlimited martial law in effect."));
    } else {
      astr_add_line(&str, PL_("%d military unit may impose martial law.",
                              "Up to %d military units may impose martial "
                              "law.", mlmax), mlmax);
    }
    astr_add_line(&str, PL_("Each military unit makes %d "
                            "unhappy citizen content.",
                            "Each military unit makes %d "
                            "unhappy citizens content.",
                            mleach), mleach);
  } else if (uhcfac > 0) {
    astr_add_line(&str,
                  _("Military units in the field may cause unhappiness. "));
  } else {
    astr_add_line(&str,
                  _("Military units have no happiness effect. "));
  }
  return str.str;
}

/****************************************************************************
  Describing luxuries that affect happiness.
****************************************************************************/
const char *text_happiness_luxuries(const struct city *pcity)
{
  static struct astring str = ASTRING_INIT;

  astr_clear(&str);

  astr_add_line(&str,
                _("Luxury: %d total."),
                pcity->prod[O_LUXURY]);
  return str.str;
}
