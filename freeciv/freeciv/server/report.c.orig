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

#include <stdio.h>
#include <string.h>

/* utility */
#include "bitvector.h"
#include "fciconv.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "rand.h"
#include "support.h"

/* common */
#include "achievements.h"
#include "calendar.h"
#include "connection.h"
#include "events.h"
#include "game.h"
#include "government.h"
#include "packets.h"
#include "player.h"
#include "research.h"
#include "specialist.h"
#include "unitlist.h"
#include "version.h"

/* server */
#include "citytools.h"
#include "plrhand.h"
#include "score.h"
#include "srv_main.h"

#include "report.h"


/* data needed for logging civ score */
struct plrdata_slot {
  char *name;
};

struct logging_civ_score {
  FILE *fp;
  int last_turn;
  struct plrdata_slot *plrdata;
};

/* Have to be initialized to value less than -1 so it doesn't seem like report was created at
 * the end of previous turn in the beginning to turn 0. */
struct history_report latest_history_report = { -2 };

static struct logging_civ_score *score_log = NULL;

static void plrdata_slot_init(struct plrdata_slot *plrdata,
                              const char *name);
static void plrdata_slot_replace(struct plrdata_slot *plrdata,
                                 const char *name);
static void plrdata_slot_free(struct plrdata_slot *plrdata);

static void page_conn_etype(struct conn_list *dest, const char *caption,
			    const char *headline, const char *lines,
			    enum event_type event);
enum historian_type {
        HISTORIAN_RICHEST=0, 
        HISTORIAN_ADVANCED=1,
        HISTORIAN_MILITARY=2,
        HISTORIAN_HAPPIEST=3,
        HISTORIAN_LARGEST=4};

#define HISTORIAN_FIRST		HISTORIAN_RICHEST
#define HISTORIAN_LAST 		HISTORIAN_LARGEST

static const char *historian_message[]={
    /* TRANS: year <name> reports ... */
    N_("%s %s reports on the RICHEST Civilizations in the World."),
    /* TRANS: year <name> reports ... */
    N_("%s %s reports on the most ADVANCED Civilizations in the World."),
    /* TRANS: year <name> reports ... */
    N_("%s %s reports on the most MILITARIZED Civilizations in the World."),
    /* TRANS: year <name> reports ... */
    N_("%s %s reports on the HAPPIEST Civilizations in the World."),
    /* TRANS: year <name> reports ... */
    N_("%s %s reports on the LARGEST Civilizations in the World.")
};

static const char *historian_name[]={
    /* TRANS: [year] <name> [reports ...] */
    N_("Herodotus"),
    /* TRANS: [year] <name> [reports ...] */
    N_("Thucydides"),
    /* TRANS: [year] <name> [reports ...] */
    N_("Pliny the Elder"),
    /* TRANS: [year] <name> [reports ...] */
    N_("Livy"),
    /* TRANS: [year] <name> [reports ...] */
    N_("Toynbee"),
    /* TRANS: [year] <name> [reports ...] */
    N_("Gibbon"),
    /* TRANS: [year] <name> [reports ...] */
    N_("Ssu-ma Ch'ien"),
    /* TRANS: [year] <name> [reports ...] */
    N_("Pan Ku")
};

static const char scorelog_magic[] = "#FREECIV SCORELOG2 ";

struct player_score_entry {
  const struct player *player;
  int value;
};

struct city_score_entry {
  struct city *city;
  int value;
};

static int get_population(const struct player *pplayer);
static int get_landarea(const struct player *pplayer);
static int get_settledarea(const struct player *pplayer);
static int get_research(const struct player *pplayer);
static int get_production(const struct player *pplayer);
static int get_economics(const struct player *pplayer);
static int get_pollution(const struct player *pplayer);
static int get_mil_service(const struct player *pplayer);
static int get_culture(const struct player *pplayer);

static const char *area_to_text(int value);
static const char *percent_to_text(int value);
static const char *production_to_text(int value);
static const char *economics_to_text(int value);
static const char *science_to_text(int value);
static const char *mil_service_to_text(int value);
static const char *pollution_to_text(int value);
static const char *culture_to_text(int value);

#define GOOD_PLAYER(p) ((p)->is_alive && !is_barbarian(p))

/*
 * Describes a row.
 */
static struct dem_row {
  const char key;
  const char *name;
  int (*get_value) (const struct player *);
  const char *(*to_text) (int);
  bool greater_values_are_better;
} rowtable[] = {
  {'N', N_("Population"),       get_population,  population_to_text,  TRUE },
  {'A', N_("Land Area"),        get_landarea,    area_to_text,        TRUE },
  {'S', N_("Settled Area"),     get_settledarea, area_to_text,        TRUE },
  {'R', N_("Research Speed"),   get_research,    science_to_text,     TRUE },
  /* TRANS: How literate people are. */
  {'L', N_("?ability:Literacy"), get_literacy,    percent_to_text,     TRUE },
  {'P', N_("Production"),       get_production,  production_to_text,  TRUE },
  {'E', N_("Economics"),        get_economics,   economics_to_text,   TRUE },
  {'M', N_("Military Service"), get_mil_service, mil_service_to_text, FALSE },
  {'O', N_("Pollution"),        get_pollution,   pollution_to_text,   FALSE },
  {'C', N_("Culture"),          get_culture,     culture_to_text,     TRUE }
};

/* Demographics columns. */
enum dem_flag {
  DEM_COL_QUANTITY,
  DEM_COL_RANK,
  DEM_COL_BEST,
  DEM_COL_LAST
};
BV_DEFINE(bv_cols, DEM_COL_LAST);
static struct dem_col {
  char key;
} coltable[] = {{'q'}, {'r'}, {'b'}}; /* Corresponds to dem_flag enum */

/* prime number of entries makes for better scaling */
static const char *ranking[] = {
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Supreme %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Magnificent %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Great %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Glorious %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Excellent %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Eminent %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Distinguished %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Average %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Mediocre %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Ordinary %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Pathetic %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Useless %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Valueless %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Worthless %s"),
  /* TRANS: <#>: The <ranking> Poles */
  N_("%2d: The Wretched %s"),
};

/**********************************************************************//**
  Compare two player score entries. Used as callback for qsort.
**************************************************************************/
static int secompare(const void *a, const void *b)
{
  return (((const struct player_score_entry *)b)->value -
	  ((const struct player_score_entry *)a)->value);
}

/**********************************************************************//**
  Construct Historian Report
**************************************************************************/
static void historian_generic(struct history_report *report,
                              enum historian_type which_news)
{
  int i, j = 0, rank = 0;
  struct player_score_entry size[player_count()];

  report->turn = game.info.turn;
  players_iterate(pplayer) {
    if (GOOD_PLAYER(pplayer)) {
      switch(which_news) {
      case HISTORIAN_RICHEST:
	size[j].value = pplayer->economic.gold;
	break;
      case HISTORIAN_ADVANCED:
	size[j].value
	  = pplayer->score.techs + research_get(pplayer)->future_tech;
	break;
      case HISTORIAN_MILITARY:
	size[j].value = pplayer->score.units;
	break;
      case HISTORIAN_HAPPIEST: 
	size[j].value =
            (((pplayer->score.happy - pplayer->score.unhappy
               - 2 * pplayer->score.angry) * 1000) /
             (1 + total_player_citizens(pplayer)));
	break;
      case HISTORIAN_LARGEST:
	size[j].value = total_player_citizens(pplayer);
	break;
      }
      size[j].player = pplayer;
      j++;
    } /* else the player is dead or barbarian or observer */
  } players_iterate_end;

  qsort(size, j, sizeof(size[0]), secompare);
  report->body[0] = '\0';
  for (i = 0; i < j; i++) {
    if (i > 0 && size[i].value < size[i - 1].value) {
      /* since i < j, only top entry reigns Supreme */
      rank = ((i * ARRAY_SIZE(ranking)) / j) + 1;
    }
    if (rank >= ARRAY_SIZE(ranking)) {
      /* clamp to final entry */
      rank = ARRAY_SIZE(ranking) - 1;
    }
    cat_snprintf(report->body, REPORT_BODYSIZE,
		 _(ranking[rank]),
		 i + 1,
		 nation_plural_for_player(size[i].player));
    fc_strlcat(report->body, "\n", REPORT_BODYSIZE);
  }
  fc_snprintf(report->title, REPORT_TITLESIZE, _(historian_message[which_news]),
              calendar_text(),
              _(historian_name[fc_rand(ARRAY_SIZE(historian_name))]));
}

/**********************************************************************//**
  Send history report of this turn.
**************************************************************************/
void send_current_history_report(struct conn_list *dest)
{
  /* History report is actually constructed at the end of previous turn. */
  if (latest_history_report.turn >= game.info.turn - 1) {
    page_conn_etype(dest, _("Historian Publishes!"),
                    latest_history_report.title, latest_history_report.body,
                    E_BROADCAST_REPORT);
  }
}

/**********************************************************************//**
 Returns the number of wonders the given city has.
**************************************************************************/
static int nr_wonders(struct city *pcity)
{
  int result = 0;

  city_built_iterate(pcity, i) {
    if (is_great_wonder(i)) {
      result++;
    }
  } city_built_iterate_end;

  return result;
}

/**********************************************************************//**
  Send report listing the "best" 5 cities in the world.
**************************************************************************/
void report_top_five_cities(struct conn_list *dest)
{
  const int NUM_BEST_CITIES = 5;
  /* a wonder equals WONDER_FACTOR citizen */
  const int WONDER_FACTOR = 5;
  struct city_score_entry size[NUM_BEST_CITIES];
  int i;
  char buffer[4096];

  for (i = 0; i < NUM_BEST_CITIES; i++) {
    size[i].value = 0;
    size[i].city = NULL;
  }

  shuffled_players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      int value_of_pcity = city_size_get(pcity)
                           + nr_wonders(pcity) * WONDER_FACTOR;

      if (value_of_pcity > size[NUM_BEST_CITIES - 1].value) {
        size[NUM_BEST_CITIES - 1].value = value_of_pcity;
        size[NUM_BEST_CITIES - 1].city = pcity;
        qsort(size, NUM_BEST_CITIES, sizeof(size[0]), secompare);
      }
    } city_list_iterate_end;
  } shuffled_players_iterate_end;

  buffer[0] = '\0';
  for (i = 0; i < NUM_BEST_CITIES; i++) {
    int wonders;

    if (!size[i].city) {
	/* 
	 * pcity may be NULL if there are less then NUM_BEST_CITIES in
	 * the whole game.
	 */
      break;
    }

    if (player_count() > team_count()) {
      /* There exists a team with more than one member. */
      char team_name[2 * MAX_LEN_NAME];

      team_pretty_name(city_owner(size[i].city)->team, team_name,
                       sizeof(team_name));
      cat_snprintf(buffer, sizeof(buffer),
                   /* TRANS:"The French City of Lyon (team 3) of size 18". */
                   _("%2d: The %s City of %s (%s) of size %d, "), i + 1,
                   nation_adjective_for_player(city_owner(size[i].city)),
                   city_name_get(size[i].city), team_name,
                   city_size_get(size[i].city));
    } else {
      cat_snprintf(buffer, sizeof(buffer),
                   _("%2d: The %s City of %s of size %d, "), i + 1,
                   nation_adjective_for_player(city_owner(size[i].city)),
                   city_name_get(size[i].city), city_size_get(size[i].city));
    }

    wonders = nr_wonders(size[i].city);
    if (wonders == 0) {
      cat_snprintf(buffer, sizeof(buffer), _("with no wonders\n"));
    } else {
      cat_snprintf(buffer, sizeof(buffer),
		   PL_("with %d wonder\n", "with %d wonders\n", wonders),
		   wonders);}
  }
  page_conn(dest, _("Traveler's Report:"),
	    _("The Five Greatest Cities in the World!"), buffer);
}

/**********************************************************************//**
  Send report listing all built and destroyed wonders, and wonders
  currently being built.
**************************************************************************/
void report_wonders_of_the_world(struct conn_list *dest)
{
  char buffer[4096];

  buffer[0] = '\0';

  improvement_iterate(i) {
    if (is_great_wonder(i)) {
      struct city *pcity = city_from_great_wonder(i);

      if (pcity) {
        if (player_count() > team_count()) {
          /* There exists a team with more than one member. */
          char team_name[2 * MAX_LEN_NAME];

          team_pretty_name(city_owner(pcity)->team, team_name,
                           sizeof(team_name));
          cat_snprintf(buffer, sizeof(buffer),
                       /* TRANS: "Colossus in Rhodes (Greek, team 2)". */
                       _("%s in %s (%s, %s)\n"),
                       city_improvement_name_translation(pcity, i),
                       city_name_get(pcity),
                       nation_adjective_for_player(city_owner(pcity)),
                       team_name);
        } else {
          cat_snprintf(buffer, sizeof(buffer), _("%s in %s (%s)\n"),
                       city_improvement_name_translation(pcity, i),
                       city_name_get(pcity),
                       nation_adjective_for_player(city_owner(pcity)));
        }
      } else if (great_wonder_is_destroyed(i)) {
        cat_snprintf(buffer, sizeof(buffer), _("%s has been DESTROYED\n"),
                     improvement_name_translation(i));
      }
    }
  } improvement_iterate_end;

  improvement_iterate(i) {
    if (is_great_wonder(i)) {
      players_iterate(pplayer) {
        city_list_iterate(pplayer->cities, pcity) {
          if (VUT_IMPROVEMENT == pcity->production.kind
           && pcity->production.value.building == i) {
            if (player_count() > team_count()) {
              /* There exists a team with more than one member. */
              char team_name[2 * MAX_LEN_NAME];

              team_pretty_name(city_owner(pcity)->team, team_name,
                               sizeof(team_name));
              cat_snprintf(buffer, sizeof(buffer),
                           /* TRANS: "([...] (Roman, team 4))". */
                           _("(building %s in %s (%s, %s))\n"),
                           improvement_name_translation(i), city_name_get(pcity),
                           nation_adjective_for_player(pplayer), team_name);
            } else {
              cat_snprintf(buffer, sizeof(buffer),
                           _("(building %s in %s (%s))\n"),
                           improvement_name_translation(i), city_name_get(pcity),
                           nation_adjective_for_player(pplayer));
            }
          }
        } city_list_iterate_end;
      } players_iterate_end;
    }
  } improvement_iterate_end;

  page_conn(dest, _("Traveler's Report:"),
            _("Wonders of the World"), buffer);
}

/**************************************************************************
 Helper functions which return the value for the given player.
**************************************************************************/

/**********************************************************************//**
  Population of player
**************************************************************************/
static int get_population(const struct player *pplayer)
{
  return pplayer->score.population;
}

/**********************************************************************//**
  Number of citizen units of player
**************************************************************************/
static int get_pop(const struct player *pplayer)
{
  return total_player_citizens(pplayer);
}

/**********************************************************************//**
  Number of citizens of player
**************************************************************************/
static int get_real_pop(const struct player *pplayer)
{
  return 1000 * get_pop(pplayer);
}

/**********************************************************************//**
  Land area controlled by player
**************************************************************************/
static int get_landarea(const struct player *pplayer)
{
    return pplayer->score.landarea;
}

/**********************************************************************//**
  Area settled.
**************************************************************************/
static int get_settledarea(const struct player *pplayer)
{
  return pplayer->score.settledarea;
}

/**********************************************************************//**
  Research speed
**************************************************************************/
static int get_research(const struct player *pplayer)
{
  return pplayer->score.techout;
}

/**********************************************************************//**
  Production of player
**************************************************************************/
static int get_production(const struct player *pplayer)
{
  return pplayer->score.mfg;
}

/**********************************************************************//**
  BNP of player
**************************************************************************/
static int get_economics(const struct player *pplayer)
{
  return pplayer->score.bnp;
}

/**********************************************************************//**
  Pollution of player
**************************************************************************/
static int get_pollution(const struct player *pplayer)
{
  return pplayer->score.pollution;
}

/**********************************************************************//**
  Military service length
**************************************************************************/
static int get_mil_service(const struct player *pplayer)
{
  return (pplayer->score.units * 5000) / (10 + civ_population(pplayer));
}

/**********************************************************************//**
  Number of cities
**************************************************************************/
static int get_cities(const struct player *pplayer)
{
  return pplayer->score.cities;
}

/**********************************************************************//**
  Number of techs
**************************************************************************/
static int get_techs(const struct player *pplayer)
{
  return pplayer->score.techs;
}

/**********************************************************************//**
  Number of military units
**************************************************************************/
static int get_munits(const struct player *pplayer)
{
  int result = 0;

  /* count up military units */
  unit_list_iterate(pplayer->units, punit) {
    if (is_military_unit(punit)) {
      result++;
    }
  } unit_list_iterate_end;

  return result;
}

/**********************************************************************//**
  Number of city building units.
**************************************************************************/
static int get_settlers(const struct player *pplayer)
{
  int result = 0;

  if (!game.scenario.prevent_new_cities) {
    /* count up settlers */
    unit_list_iterate(pplayer->units, punit) {
      if (unit_can_do_action(punit, ACTION_FOUND_CITY)) {
        result++;
      }
    } unit_list_iterate_end;
  }

  return result;
}

/**********************************************************************//**
  Wonder score
**************************************************************************/
static int get_wonders(const struct player *pplayer)
{
  return pplayer->score.wonders;
}

/**********************************************************************//**
  Technology output
**************************************************************************/
static int get_techout(const struct player *pplayer)
{
  return pplayer->score.techout;
}

/**********************************************************************//**
  Literacy score calculated one way. See also get_literacy() to see
  alternative way.
**************************************************************************/
static int get_literacy2(const struct player *pplayer)
{
  return pplayer->score.literacy;
}

/**********************************************************************//**
  Spaceship score
**************************************************************************/
static int get_spaceship(const struct player *pplayer)
{
  return pplayer->score.spaceship;
}

/**********************************************************************//**
  Number of units built
**************************************************************************/
static int get_units_built(const struct player *pplayer)
{
  return pplayer->score.units_built;
}

/**********************************************************************//**
  Number of units killed
**************************************************************************/
static int get_units_killed(const struct player *pplayer)
{
  return pplayer->score.units_killed;
}

/**********************************************************************//**
  Number of units lost
**************************************************************************/
static int get_units_lost(const struct player *pplayer)
{
  return pplayer->score.units_lost;
}

/**********************************************************************//**
  Amount of gold.
**************************************************************************/
static int get_gold(const struct player *pplayer)
{
  return pplayer->economic.gold;
}

/**********************************************************************//**
  Tax rate
**************************************************************************/
static int get_taxrate(const struct player *pplayer)
{
  return pplayer->economic.tax;
}

/**********************************************************************//**
  Science rate
**************************************************************************/
static int get_scirate(const struct player *pplayer)
{
  return pplayer->economic.science;
}

/**********************************************************************//**
  Luxury rate
**************************************************************************/
static int get_luxrate(const struct player *pplayer)
{
  return pplayer->economic.luxury;
}

/**********************************************************************//**
  Number of rioting cities
**************************************************************************/
static int get_riots(const struct player *pplayer)
{
  int result = 0;

  city_list_iterate(pplayer->cities, pcity) {
    if (pcity->anarchy > 0) {
      result++;
    }
  } city_list_iterate_end;

  return result;
}

/**********************************************************************//**
  Number of happy citizens
**************************************************************************/
static int get_happypop(const struct player *pplayer)
{
  return pplayer->score.happy;
}

/**********************************************************************//**
  Number of content citizens
**************************************************************************/
static int get_contentpop(const struct player *pplayer)
{
  return pplayer->score.content;
}

/**********************************************************************//**
  Number of unhappy citizens
**************************************************************************/
static int get_unhappypop(const struct player *pplayer)
{
  return pplayer->score.unhappy;
}

/**********************************************************************//**
  Number of specialists.
**************************************************************************/
static int get_specialists(const struct player *pplayer)
{
  int count = 0;

  specialist_type_iterate(sp) {
    count += pplayer->score.specialists[sp];
  } specialist_type_iterate_end;

  return count;
}

/**********************************************************************//**
  Current government
**************************************************************************/
static int get_gov(const struct player *pplayer)
{
  return (int) government_number(government_of_player(pplayer));
}

/**********************************************************************//**
  Total corruption
**************************************************************************/
static int get_corruption(const struct player *pplayer)
{
  int result = 0;

  city_list_iterate(pplayer->cities, pcity) {
    result += pcity->waste[O_TRADE];
  } city_list_iterate_end;

  return result;
}

/**********************************************************************//**
  Total score
**************************************************************************/
static int get_total_score(const struct player *pplayer)
{
  return pplayer->score.game;
}

/**********************************************************************//**
  Culture score
**************************************************************************/
static int get_culture(const struct player *pplayer)
{
  return pplayer->score.culture;
}

/**********************************************************************//**
  Construct string containing value and its unit.
**************************************************************************/
static const char *value_units(int val, const char *uni)
{
  static char buf[64];

  if (fc_snprintf(buf, sizeof(buf), "%s%s", int_to_text(val), uni) == -1) {
    log_error("String truncated in value_units()!");
  }

  return buf;
}

/**********************************************************************//**
  Helper functions which transform the given value to a string
  depending on the unit.
**************************************************************************/
static const char *area_to_text(int value)
{
  /* TRANS: abbreviation of "square miles" */
  return value_units(value, PL_(" sq. mi.", " sq. mi.", value));
}

/**********************************************************************//**
  Construct string containing value followed by '%'. So value is already
  considered to be in units of 1/100.
**************************************************************************/
static const char *percent_to_text(int value)
{
  return value_units(value, "%");
}

/**********************************************************************//**
  Construct string containing value followed by unit suitable for
  production stats.
**************************************************************************/
static const char *production_to_text(int value)
{
  int clip = MAX(0, value);
  /* TRANS: "M tons" = million tons, so always plural */
  return value_units(clip, PL_(" M tons", " M tons", clip));
}

/**********************************************************************//**
  Construct string containing value followed by unit suitable for
  economics stats.
**************************************************************************/
static const char *economics_to_text(int value)
{
  /* TRANS: "M goods" = million goods, so always plural */
  return value_units(value, PL_(" M goods", " M goods", value));
}

/**********************************************************************//**
  Construct string containing value followed by unit suitable for
  science stats.
**************************************************************************/
static const char *science_to_text(int value)
{
  return value_units(value, PL_(" bulb", " bulbs", value));
}

/**********************************************************************//**
  Construct string containing value followed by unit suitable for
  military service stats.
**************************************************************************/
static const char *mil_service_to_text(int value)
{
  return value_units(value, PL_(" month", " months", value));
}

/**********************************************************************//**
  Construct string containing value followed by unit suitable for
  pollution stats.
**************************************************************************/
static const char *pollution_to_text(int value)
{
  return value_units(value, PL_(" ton", " tons", value));
}

/**********************************************************************//**
  Construct string containing value followed by unit suitable for
  culture stats.
**************************************************************************/
static const char *culture_to_text(int value)
{
  /* TRANS: Unit(s) of culture */
  return value_units(value, PL_(" point", " points", value));
}

/**********************************************************************//**
  Construct one demographics line.
**************************************************************************/
static void dem_line_item(char *outptr, size_t out_size,
                          struct player *pplayer, struct dem_row *prow,
                          bv_cols selcols)
{
  if (NULL != pplayer && BV_ISSET(selcols, DEM_COL_QUANTITY)) {
    const char *text = prow->to_text(prow->get_value(pplayer));

    cat_snprintf(outptr, out_size, " %s", text);
    cat_snprintf(outptr, out_size, "%*s",
                 18 - (int) get_internal_string_length(text), "");
  }

  if (NULL != pplayer && BV_ISSET(selcols, DEM_COL_RANK)) {
    int basis = prow->get_value(pplayer);
    int place = 1;

    players_iterate(other) {
      if (GOOD_PLAYER(other)
	  && ((prow->greater_values_are_better
	       && prow->get_value(other) > basis)
	      || (!prow->greater_values_are_better
	          && prow->get_value(other) < basis))) {
	place++;
      }
    } players_iterate_end;

    cat_snprintf(outptr, out_size, _("(ranked %d)"), place);
  }
   
  if (NULL == pplayer || BV_ISSET(selcols, DEM_COL_BEST)) {
    struct player *best_player = pplayer;
    int best_value = NULL != pplayer ? prow->get_value(pplayer) : 0;

    players_iterate(other) {
      if (GOOD_PLAYER(other)) {
        int value = prow->get_value(other);

        if (!best_player
            || (prow->greater_values_are_better && value > best_value)
            || (!prow->greater_values_are_better && value < best_value)) {
          best_player = other;
          best_value = value;
        }
      }
    } players_iterate_end;

    if (NULL == pplayer
        || (player_has_embassy(pplayer, best_player)
            && (pplayer != best_player))) {
      cat_snprintf(outptr, out_size, "   %s: %s",
		   nation_plural_for_player(best_player),
		   prow->to_text(prow->get_value(best_player)));
    }
  }
}

/**********************************************************************//**
  Verify that a given demography string is valid.  See
  game.demography. If the string is not valid the index of the _first_
  invalid character is return as 'error'.

  Other settings callback functions are in settings.c, but this one uses
  static values from this file so it's done separately.
**************************************************************************/
bool is_valid_demography(const char *demography, int *error)
{
  int len = strlen(demography), i;

  /* We check each character individually to see if it's valid.  This
   * does not check for duplicate entries. */
  for (i = 0; i < len; i++) {
    bool found = FALSE;
    int j;

    /* See if the character is a valid column label. */
    for (j = 0; j < DEM_COL_LAST; j++) {
      if (demography[i] == coltable[j].key) {
	found = TRUE;
	break;
      }
    }

    if (found) {
      continue;
    }

    /* See if the character is a valid row label. */
    for (j = 0; j < ARRAY_SIZE(rowtable); j++) {
      if (demography[i] == rowtable[j].key) {
	found = TRUE;
	break;
      }
    }

    if (!found) {
      if (error != NULL) {
        (*error) = i;
      }
      /* The character is invalid. */
      return FALSE;
    }
  }

  /* Looks like all characters were valid. */
  return TRUE;
}

/**********************************************************************//**
  Send demographics report; what gets reported depends on value of
  demographics server option.  
**************************************************************************/
void report_demographics(struct connection *pconn)
{
  char civbuf[1024];
  char buffer[4096];
  unsigned int i;
  bool anyrows;
  bv_cols selcols;
  int numcols = 0;
  struct player *pplayer = pconn->playing;

  BV_CLR_ALL(selcols);
  fc_assert_ret(ARRAY_SIZE(coltable) == DEM_COL_LAST);
  for (i = 0; i < DEM_COL_LAST; i++) {
    if (strchr(game.server.demography, coltable[i].key)) {
      BV_SET(selcols, i);
      numcols++;
    }
  }

  anyrows = FALSE;
  for (i = 0; i < ARRAY_SIZE(rowtable); i++) {
    if (strchr(game.server.demography, rowtable[i].key)) {
      anyrows = TRUE;
      break;
    }
  }

  if ((!pconn->observer && !pplayer)
      || (pplayer && !pplayer->is_alive)
      || !anyrows
      || numcols == 0) {
    page_conn(pconn->self, _("Demographics Report:"),
              _("Sorry, the Demographics report is unavailable."), "");
    return;
  }

  if (pplayer) {
    fc_snprintf(civbuf, sizeof(civbuf), _("%s %s (%s)"),
                nation_adjective_for_player(pplayer),
                government_name_for_player(pplayer),
                calendar_text());
  } else {
    civbuf[0] = '\0';
  }

  buffer[0] = '\0';
  for (i = 0; i < ARRAY_SIZE(rowtable); i++) {
    if (strchr(game.server.demography, rowtable[i].key)) {
      const char *name = Q_(rowtable[i].name);

      cat_snprintf(buffer, sizeof(buffer), "%s", name);
      cat_snprintf(buffer, sizeof(buffer), "%*s",
                   18 - (int) get_internal_string_length(name), "");
      dem_line_item(buffer, sizeof(buffer), pplayer, &rowtable[i], selcols);
      sz_strlcat(buffer, "\n");
    }
  }

  page_conn(pconn->self, _("Demographics Report:"), civbuf, buffer);
}

/**********************************************************************//**
  Send achievements list
**************************************************************************/
void report_achievements(struct connection *pconn)
{
  char civbuf[1024];
  char buffer[4096];
  struct player *pplayer = pconn->playing;

  if (pplayer == NULL) {
    return;
  }

  fc_snprintf(civbuf, sizeof(civbuf), _("%s %s (%s)"),
              nation_adjective_for_player(pplayer),
              government_name_for_player(pplayer),
              calendar_text());

  buffer[0] = '\0';

  achievements_iterate(pach) {
    if (achievement_player_has(pach, pplayer)) {
      cat_snprintf(buffer, sizeof(buffer), "%s\n",
                   achievement_name_translation(pach));
    }
  } achievements_iterate_end;

  page_conn(pconn->self, _("Achievements List:"), civbuf, buffer);
}

/**********************************************************************//**
  Allocate and initialize plrdata slot.
**************************************************************************/
static void plrdata_slot_init(struct plrdata_slot *plrdata,
                              const char *name)
{
  fc_assert_ret(plrdata->name == NULL);

  plrdata->name = fc_calloc(MAX_LEN_NAME, sizeof(plrdata->name));
  plrdata_slot_replace(plrdata, name);
}

/**********************************************************************//**
  Replace plrdata slot with new one named according to input parameter.
**************************************************************************/
static void plrdata_slot_replace(struct plrdata_slot *plrdata,
                                 const char *name)
{
  fc_assert_ret(plrdata->name != NULL);

  fc_strlcpy(plrdata->name, name, MAX_LEN_NAME);
}

/**********************************************************************//**
  Free resources allocated for plrdata slot.
**************************************************************************/
static void plrdata_slot_free(struct plrdata_slot *plrdata)
{
  if (plrdata->name != NULL) {
    free(plrdata->name);
    plrdata->name = NULL;
  }
}

/**********************************************************************//**
  Reads the whole file denoted by fp. Sets last_turn and id to the
  values contained in the file. Returns the player_names indexed by
  player_no at the end of the log file.

  Returns TRUE iff the file had read successfully.
**************************************************************************/
static bool scan_score_log(char *id)
{
  int line_nr, turn, plr_no, spaces;
  struct plrdata_slot *plrdata;
  char plr_name[MAX_LEN_NAME], line[120], *ptr;

  fc_assert_ret_val(score_log != NULL, FALSE);
  fc_assert_ret_val(score_log->fp != NULL, FALSE);

  score_log->last_turn = -1;
  id[0] = '\0';

  for (line_nr = 1;; line_nr++) {
    if (!fgets(line, sizeof(line), score_log->fp)) {
      if (feof(score_log->fp) != 0) {
        break;
      }
      log_error("[%s:-] Can't read scorelog file header!",
                game.server.scorefile);
      return FALSE;
    }

    ptr = strchr(line, '\n');
    if (!ptr) {
      log_error("[%s:%d] Line too long!", game.server.scorefile, line_nr);
      return FALSE;
    }
    *ptr = '\0';

    if (line_nr == 1) {
      if (strncmp(line, scorelog_magic, strlen(scorelog_magic)) != 0) {
        log_error("[%s:%d] Bad file magic!", game.server.scorefile, line_nr);
        return FALSE;
      }
    }

    if (strncmp(line, "id ", strlen("id ")) == 0) {
      if (strlen(id) > 0) {
        log_error("[%s:%d] Multiple ID entries!", game.server.scorefile,
                  line_nr);
        return FALSE;
      }
      fc_strlcpy(id, line + strlen("id "), MAX_LEN_GAME_IDENTIFIER);
      if (strcmp(id, server.game_identifier) != 0) {
        log_error("[%s:%d] IDs don't match! game='%s' scorelog='%s'",
                  game.server.scorefile, line_nr, server.game_identifier,
                  id);
        return FALSE;
      }
    }

    if (strncmp(line, "turn ", strlen("turn ")) == 0) {
      if (sscanf(line + strlen("turn "), "%d", &turn) != 1) {
        log_error("[%s:%d] Bad line (turn)!", game.server.scorefile,
                  line_nr);
        return FALSE;
      }

      fc_assert_ret_val(turn > score_log->last_turn, FALSE);
      score_log->last_turn = turn;
    }

    if (strncmp(line, "addplayer ", strlen("addplayer ")) == 0) {
      if (3 != sscanf(line + strlen("addplayer "), "%d %d %s",
                      &turn, &plr_no, plr_name)) {
        log_error("[%s:%d] Bad line (addplayer)!",
                  game.server.scorefile, line_nr);
        return FALSE;
      }

      /* Now get the complete player name if there are several parts. */
      ptr = line + strlen("addplayer ");
      spaces = 0;
      while (*ptr != '\0' && spaces < 2) {
        if (*ptr == ' ') {
          spaces++;
        }
        ptr++;
      }
      fc_snprintf(plr_name, sizeof(plr_name), "%s", ptr);
      log_debug("add player '%s' (from line %d: '%s')", plr_name, line_nr,
                line);

      if (0 > plr_no || plr_no >= player_slot_count()) {
        log_error("[%s:%d] Invalid player number: %d!",
                  game.server.scorefile, line_nr, plr_no);
        return FALSE;
      }

      plrdata = score_log->plrdata + plr_no;
      if (plrdata->name != NULL) {
        log_error("[%s:%d] Two names for one player (id %d)!",
                  game.server.scorefile, line_nr, plr_no);
        return FALSE;
      }

      plrdata_slot_init(plrdata, plr_name);
    }

    if (strncmp(line, "delplayer ", strlen("delplayer ")) == 0) {
      if (2 != sscanf(line + strlen("delplayer "), "%d %d",
                      &turn, &plr_no)) {
        log_error("[%s:%d] Bad line (delplayer)!",
                  game.server.scorefile, line_nr);
        return FALSE;
      }

      if (!(plr_no >= 0 && plr_no < player_slot_count())) {
        log_error("[%s:%d] Invalid player number: %d!",
                  game.server.scorefile, line_nr, plr_no);
        return FALSE;
      }

      plrdata = score_log->plrdata + plr_no;
      if (plrdata->name == NULL) {
        log_error("[%s:%d] Trying to remove undefined player (id %d)!",
                  game.server.scorefile, line_nr, plr_no);
        return FALSE;
      }

      plrdata_slot_free(plrdata);
    }
  }

  if (score_log->last_turn == -1) {
    log_error("[%s:-] Scorelog contains no turn!", game.server.scorefile);
    return FALSE;
  }

  if (strlen(id) == 0) {
    log_error("[%s:-] Scorelog contains no ID!", game.server.scorefile);
    return FALSE;
  }

  if (score_log->last_turn + 1 != game.info.turn) {
    log_error("[%s:-] Scorelog doesn't match savegame!",
              game.server.scorefile);
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Initialize score logging system
**************************************************************************/
void log_civ_score_init(void)
{
  if (score_log != NULL) {
    return;
  }

  score_log = fc_calloc(1, sizeof(*score_log));
  score_log->fp = NULL;
  score_log->last_turn = -1;
  score_log->plrdata = fc_calloc(player_slot_count(),
                                 sizeof(*score_log->plrdata));
  player_slots_iterate(pslot) {
    struct plrdata_slot *plrdata = score_log->plrdata
                                   + player_slot_index(pslot);
    plrdata->name = NULL;
  } player_slots_iterate_end;

  latest_history_report.turn = -2;
}

/**********************************************************************//**
  Free resources allocated for score logging system
**************************************************************************/
void log_civ_score_free(void)
{
  if (!score_log) {
    /* nothing to do */
    return;
  }

  if (score_log->fp) {
    fclose(score_log->fp);
    score_log->fp = NULL;
  }

  if (score_log->plrdata) {
    player_slots_iterate(pslot) {
      struct plrdata_slot *plrdata = score_log->plrdata
                                     + player_slot_index(pslot);
      if (plrdata->name != NULL) {
        free(plrdata->name);
      }
    } player_slots_iterate_end;
    free(score_log->plrdata);
  }

  free(score_log);
  score_log = NULL;
}

/**********************************************************************//**
  Create a log file of the civilizations so you can see what was happening.
**************************************************************************/
void log_civ_score_now(void)
{
  enum { SL_CREATE, SL_APPEND, SL_UNSPEC } oper = SL_UNSPEC;
  char id[MAX_LEN_GAME_IDENTIFIER];
  int i = 0;

  /* Add new tags only at end of this list. Maintaining the order of
   * old tags is critical. */
  static const struct {
    char *name;
    int (*get_value) (const struct player *);
  } score_tags[] = {
    {"pop",             get_pop},
    {"bnp",             get_economics},
    {"mfg",             get_production},
    {"cities",          get_cities},
    {"techs",           get_techs},
    {"munits",          get_munits},
    {"settlers",        get_settlers},  /* "original" tags end here */

    {"wonders",         get_wonders},
    {"techout",         get_techout},
    {"landarea",        get_landarea},
    {"settledarea",     get_settledarea},
    {"pollution",       get_pollution},
    {"literacy",        get_literacy2},
    {"spaceship",       get_spaceship}, /* new 1.8.2 tags end here */

    {"gold",            get_gold},
    {"taxrate",         get_taxrate},
    {"scirate",         get_scirate},
    {"luxrate",         get_luxrate},
    {"riots",           get_riots},
    {"happypop",        get_happypop},
    {"contentpop",      get_contentpop},
    {"unhappypop",      get_unhappypop},
    {"specialists",     get_specialists},
    {"gov",             get_gov},
    {"corruption",      get_corruption}, /* new 1.11.5 tags end here */

    {"score",           get_total_score}, /* New 2.1.10 tag end here. */

    {"unitsbuilt",      get_units_built}, /* New tags since 2.3.0. */
    {"unitskilled",     get_units_killed},
    {"unitslost",       get_units_lost},

    {"culture",         get_culture}      /* New tag in 2.6.0. */
  };

  if (!game.server.scorelog) {
    return;
  }

  if (!score_log) {
    return;
  }

  if (!score_log->fp) {
    if (game.info.year == GAME_START_YEAR) {
      oper = SL_CREATE;
    } else {
      score_log->fp = fc_fopen(game.server.scorefile, "r");
      if (!score_log->fp) {
        oper = SL_CREATE;
      } else {
        if (!scan_score_log(id)) {
          goto log_civ_score_disable;
        }
        oper = SL_APPEND;

        fclose(score_log->fp);
        score_log->fp = NULL;
      }
    }

    switch (oper) {
    case SL_CREATE:
      score_log->fp = fc_fopen(game.server.scorefile, "w");
      if (!score_log->fp) {
        log_error("Can't open scorelog file '%s' for creation!",
                  game.server.scorefile);
        goto log_civ_score_disable;
      }
      fprintf(score_log->fp, "%s%s\n", scorelog_magic, VERSION_STRING);
      fprintf(score_log->fp,
              "\n"
              "# For a specification of the format of this see doc/README.scorelog or \n"
              "# <https://raw.githubusercontent.com/freeciv/freeciv/master/doc/README.scorelog>.\n"
              "\n");

      fprintf(score_log->fp, "id %s\n", server.game_identifier);
      for (i = 0; i < ARRAY_SIZE(score_tags); i++) {
        fprintf(score_log->fp, "tag %d %s\n", i, score_tags[i].name);
      }
      break;
    case SL_APPEND:
      score_log->fp = fc_fopen(game.server.scorefile, "a");
      if (!score_log->fp) {
        log_error("Can't open scorelog file '%s' for appending!",
                  game.server.scorefile);
        goto log_civ_score_disable;
      }
      break;
    default:
      log_error("[%s] bad operation %d", __FUNCTION__, (int) oper);
      goto log_civ_score_disable;
    }
  }

  if (game.info.turn > score_log->last_turn) {
    fprintf(score_log->fp, "turn %d %d %s\n", game.info.turn, game.info.year,
            calendar_text());
    score_log->last_turn = game.info.turn;
  }

  player_slots_iterate(pslot) {
    struct plrdata_slot *plrdata = score_log->plrdata
                                   + player_slot_index(pslot);
    if (plrdata->name != NULL
        && player_slot_is_used(pslot)) {
      struct player *pplayer = player_slot_get_player(pslot);

      if (!GOOD_PLAYER(pplayer)) {
        fprintf(score_log->fp, "delplayer %d %d\n", game.info.turn - 1,
                player_number(pplayer));
        plrdata_slot_free(plrdata);
      }
    }
  } player_slots_iterate_end;

  players_iterate(pplayer) {
    struct plrdata_slot *plrdata = score_log->plrdata + player_index(pplayer);
    if (plrdata->name == NULL && GOOD_PLAYER(pplayer)) {
      switch (game.server.scoreloglevel) {
      case SL_HUMANS:
        if (is_ai(pplayer)) {
          break;
        }
      case SL_ALL:
        fprintf(score_log->fp, "addplayer %d %d %s\n", game.info.turn,
              player_number(pplayer), player_name(pplayer));
        plrdata_slot_init(plrdata, player_name(pplayer));
      }
    }
  } players_iterate_end;

  players_iterate(pplayer) {
    struct plrdata_slot *plrdata = score_log->plrdata + player_index(pplayer);

    if (GOOD_PLAYER(pplayer)) {
      switch (game.server.scoreloglevel) {
      case SL_HUMANS:
        if (is_ai(pplayer) && plrdata->name == NULL) {
          /* If a human player toggled into AI mode, don't break. */
          break;
        }
      case SL_ALL:
        if (strcmp(plrdata->name, player_name(pplayer)) != 0) {
          log_debug("player names does not match '%s' != '%s'", plrdata->name,
                  player_name(pplayer));
          fprintf(score_log->fp, "delplayer %d %d\n", game.info.turn - 1,
                  player_number(pplayer));
          fprintf(score_log->fp, "addplayer %d %d %s\n", game.info.turn,
                  player_number(pplayer), player_name(pplayer));
          plrdata_slot_replace(plrdata, player_name(pplayer));
        }
      }
    }
  } players_iterate_end;

  for (i = 0; i < ARRAY_SIZE(score_tags); i++) {
    players_iterate(pplayer) {
      if (!GOOD_PLAYER(pplayer)
          || (game.server.scoreloglevel == SL_HUMANS && is_ai(pplayer))) {
        continue;
      }

      fprintf(score_log->fp, "data %d %d %d %d\n", game.info.turn, i,
              player_number(pplayer), score_tags[i].get_value(pplayer));
    } players_iterate_end;
  }

  fflush(score_log->fp);

  return;

log_civ_score_disable:

  log_civ_score_free();
}

/**********************************************************************//**
  Produce random history report if it's time for one.
**************************************************************************/
void make_history_report(void)
{
  if (player_count() == 1) {
    return;
  }

  if (game.server.scoreturn > game.info.turn) {
    return;
  }

  game.server.scoreturn = (game.info.turn + GAME_DEFAULT_SCORETURN
                           + fc_rand(GAME_DEFAULT_SCORETURN));

  historian_generic(&latest_history_report, game.server.scoreturn
                    % (HISTORIAN_LAST + 1));
  send_current_history_report(game.est_connections);
}

/**********************************************************************//**
  Inform clients about player scores and statistics when the game ends.
  Called only from server/srv_main.c srv_scores()
**************************************************************************/
void report_final_scores(struct conn_list *dest)
{
  static const struct {
    const char *name;
    int (*score) (const struct player *);
  } score_categories[] = {
    { N_("Population\n"),               get_real_pop },
    /* TRANS: "M goods" = million goods */
    { N_("Trade\n(M goods)"),           get_economics },
    /* TRANS: "M tons" = million tons */
    { N_("Production\n(M tons)"),       get_production },
    { N_("Cities\n"),                   get_cities },
    { N_("Technologies\n"),             get_techs },
    { N_("Military Service\n(months)"), get_mil_service },
    { N_("Wonders\n"),                  get_wonders },
    { N_("Research Speed\n(bulbs)"),    get_research },
    /* TRANS: "sq. mi." is abbreviation for "square miles" */
    { N_("Land Area\n(sq. mi.)"),       get_landarea },
    /* TRANS: "sq. mi." is abbreviation for "square miles" */
    { N_("Settled Area\n(sq. mi.)"),    get_settledarea },
    { N_("Literacy\n(%)"),              get_literacy },
    { N_("Culture\n"),                  get_culture },
    { N_("Spaceship\n"),                get_spaceship },
    { N_("Built Units\n"),              get_units_built },
    { N_("Killed Units\n"),             get_units_killed },
    { N_("Unit Losses\n"),              get_units_lost },
  };
  const size_t score_categories_num = ARRAY_SIZE(score_categories);

  int i, j;
  struct player_score_entry size[player_count()];
  struct packet_endgame_report packet;

  fc_assert(score_categories_num <= ARRAY_SIZE(packet.category_name));

  if (!dest) {
    dest = game.est_connections;
  }

  packet.category_num = score_categories_num;
  for (j = 0; j < score_categories_num; j++) {
    sz_strlcpy(packet.category_name[j], score_categories[j].name);
  }

  i = 0;
  players_iterate(pplayer) {
    if (is_barbarian(pplayer) == FALSE) {
      size[i].value = pplayer->score.game;
      size[i].player = pplayer;
      i++;
    }
  } players_iterate_end;

  qsort(size, i, sizeof(size[0]), secompare);

  packet.player_num = i;

  lsend_packet_endgame_report(dest, &packet);

  for (i = 0; i < packet.player_num; i++) {
    struct packet_endgame_player ppacket;
    const struct player *pplayer = size[i].player;

    ppacket.category_num = score_categories_num;
    ppacket.player_id = player_number(pplayer);
    ppacket.score = size[i].value;
    for (j = 0; j < score_categories_num; j++) {
      ppacket.category_score[j] = score_categories[j].score(pplayer);
    }

    ppacket.winner = pplayer->is_winner;

    lsend_packet_endgame_player(dest, &ppacket);
  }
}

/**********************************************************************//**
  This function pops up a non-modal message dialog on the player's desktop
**************************************************************************/
void page_conn(struct conn_list *dest, const char *caption, 
	       const char *headline, const char *lines) {
  page_conn_etype(dest, caption, headline, lines, E_REPORT);
}


/**********************************************************************//**
  This function pops up a non-modal message dialog on the player's desktop

  event == E_REPORT: message should not be ignored by clients watching
                     AI players with ai_popup_windows off. Example:
                     Server Options, Demographics Report, etc.

  event == E_BROADCAST_REPORT: message can safely be ignored by clients
                     watching AI players with ai_popup_windows off. For
                     example: Herodot's report... and similar messages.
**************************************************************************/
static void page_conn_etype(struct conn_list *dest, const char *caption,
                            const char *headline, const char *lines,
                            enum event_type event)
{
  struct packet_page_msg packet;
  int i;
  int len;

  sz_strlcpy(packet.caption, caption);
  sz_strlcpy(packet.headline, headline);
  packet.event = event;
  len = strlen(lines);
  if ((len % (MAX_LEN_CONTENT - 1)) == 0) {
    packet.parts = len / (MAX_LEN_CONTENT - 1);
  } else {
    packet.parts = len / (MAX_LEN_CONTENT - 1) + 1;
  }
  packet.len = len;

  lsend_packet_page_msg(dest, &packet);

  for (i = 0; i < packet.parts; i++) {
    struct packet_page_msg_part part;
    int plen;

    plen = MIN(len, (MAX_LEN_CONTENT - 1));
    strncpy(part.lines, &(lines[(MAX_LEN_CONTENT - 1) * i]), plen);
    part.lines[plen] = '\0';

    lsend_packet_page_msg_part(dest, &part);

    len -= plen;
  }
}

/**********************************************************************//**
  Return current history report
**************************************************************************/
struct history_report *history_report_get(void)
{
  return &latest_history_report;
}
