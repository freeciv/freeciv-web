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

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "map.h"
#include "specialist.h"
#include "unitlist.h"

/* agents */
#include "cma_fec.h"

/* client */
#include "citydlg_common.h" /* city_production_cost_str() */
#include "options.h"

#include "cityrepdata.h"

/********************************************************************//**
  cr_entry = return an entry (one column for one city) for the city report
  These return ptrs to filled in static strings.
  Note the returned string may not be exactly the right length; that
  is handled later.
************************************************************************/
static const char *cr_entry_cityname(const struct city *pcity,
                                     const void *data)
{
  /* We used to truncate the name to 14 bytes.  This should not be needed
   * in any modern GUI library and may give an invalid string if a
   * multibyte character is clipped. */
  return city_name_get(pcity);
}

/********************************************************************//**
  Translated name of nation who owns this city.
************************************************************************/
static const char *cr_entry_nation(const struct city *pcity,
                                   const void *data)
{
  return nation_adjective_for_player(city_owner(pcity));
}

/********************************************************************//**
  Returns city size written to string. Returned string is statically
  allocated and its contents change when this function is called again.
************************************************************************/
static const char *cr_entry_size(const struct city *pcity,
                                 const void *data)
{
  static char buf[8];

  fc_snprintf(buf, sizeof(buf), "%2d", city_size_get(pcity));
  return buf;
}

/********************************************************************//**
  Returns concise city happiness state written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_hstate_concise(const struct city *pcity,
                                           const void *data)
{
  static char buf[4];
  fc_snprintf(buf, sizeof(buf), "%s",
              (city_celebrating(pcity) ? "*"
               : (city_unhappy(pcity) ? "X" : " ")));
  return buf;
}

/********************************************************************//**
  Returns verbose city happiness state written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_hstate_verbose(const struct city *pcity,
                                           const void *data)
{
  static char buf[32];

  fc_snprintf(buf, sizeof(buf), "%s",
              (city_celebrating(pcity) ? Q_("?city_state:Celebrating")
               : (city_unhappy(pcity) ? Q_("?city_state:Disorder")
                  : Q_("?city_state:Peace"))));
  return buf;
}

/********************************************************************//**
  Returns number of citizens of each happiness state written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_workers(const struct city *pcity,
                                    const void *data)
{
  static char buf[32];

  fc_snprintf(buf, sizeof(buf), "%d/%d/%d/%d",
              pcity->feel[CITIZEN_HAPPY][FEELING_FINAL],
              pcity->feel[CITIZEN_CONTENT][FEELING_FINAL],
              pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL],
              pcity->feel[CITIZEN_ANGRY][FEELING_FINAL]);
  return buf;
}

/********************************************************************//**
  Returns number of happy citizens written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_happy(const struct city *pcity,
				  const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%2d",
              pcity->feel[CITIZEN_HAPPY][FEELING_FINAL]);
  return buf;
}

/********************************************************************//**
  Returns city total culture written to string
************************************************************************/
static const char *cr_entry_culture(const struct city *pcity,
                                    const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->client.culture);
  return buf;
}

/********************************************************************//**
  Returns city history culture value written to string
************************************************************************/
static const char *cr_entry_history(const struct city *pcity,
                                    const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->history);
  return buf;
}

/********************************************************************//**
  Returns city performance culture value written to string
************************************************************************/
static const char *cr_entry_performance(const struct city *pcity,
                                        const void *data)
{
  static char buf[8];

  /*
   * Infer the actual performance component of culture from server-supplied
   * values, rather than using the client's guess at EFT_PERFORMANCE.
   * XXX: if culture ever gets more complicated than history+performance,
   * this will need revising, possibly to use a server-supplied value.
   */
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->client.culture - pcity->history);
  return buf;
}

/********************************************************************//**
  Returns number of content citizens written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_content(const struct city *pcity,
				    const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%2d",
              pcity->feel[CITIZEN_CONTENT][FEELING_FINAL]);
  return buf;
}

/********************************************************************//**
  Returns number of unhappy citizens written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_unhappy(const struct city *pcity,
				    const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%2d",
              pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL]);
  return buf;
}

/********************************************************************//**
  Returns number of angry citizens written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_angry(const struct city *pcity,
				  const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%2d",
              pcity->feel[CITIZEN_ANGRY][FEELING_FINAL]);
  return buf;
}

/********************************************************************//**
  Returns list of specialists written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_specialists(const struct city *pcity,
					const void *data)
{
  return specialists_string(pcity->specialists);
}

/********************************************************************//**
  Returns number of specialists of type given as data written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_specialist(const struct city *pcity,
				       const void *data)
{
  static char buf[8];
  const struct specialist *sp = data;

  fc_snprintf(buf, sizeof(buf), "%2d",
              pcity->specialists[specialist_index(sp)]);
  return buf;
}

/********************************************************************//**
  Returns string with best attack values of units in city.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_attack(const struct city *pcity,
				   const void *data)
{
  static char buf[32];
  int attack_best[4] = {-1, -1, -1, -1}, i;

  unit_list_iterate(pcity->tile->units, punit) {
    /* What about allied units?  Should we just count them? */
    attack_best[3] = unit_type_get(punit)->attack_strength;

    /* Now that the element is appended to the end of the list, we simply
       do an insertion sort. */
    for (i = 2; i >= 0 && attack_best[i] < attack_best[i + 1]; i--) {
      int tmp = attack_best[i];
      attack_best[i] = attack_best[i + 1];
      attack_best[i + 1] = tmp;
    }
  } unit_list_iterate_end;

  buf[0] = '\0';
  for (i = 0; i < 3; i++) {
    if (attack_best[i] >= 0) {
      cat_snprintf(buf, sizeof(buf), "%s%d", (i > 0) ? "/" : "",
		   attack_best[i]);
    } else {
      cat_snprintf(buf, sizeof(buf), "%s-", (i > 0) ? "/" : "");
    }
  }

  return buf;
}

/********************************************************************//**
  Returns string with best defend values of units in city.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_defense(const struct city *pcity,
				    const void *data)
{
  static char buf[32];
  int defense_best[4] = {-1, -1, -1, -1}, i;

  unit_list_iterate(pcity->tile->units, punit) {
    /* What about allied units?  Should we just count them? */
    defense_best[3] = unit_type_get(punit)->defense_strength;

    /* Now that the element is appended to the end of the list, we simply
       do an insertion sort. */
    for (i = 2; i >= 0 && defense_best[i] < defense_best[i + 1]; i--) {
      int tmp = defense_best[i];

      defense_best[i] = defense_best[i + 1];
      defense_best[i + 1] = tmp;
    }
  } unit_list_iterate_end;

  buf[0] = '\0';
  for (i = 0; i < 3; i++) {
    if (defense_best[i] >= 0) {
      cat_snprintf(buf, sizeof(buf), "%s%d", (i > 0) ? "/" : "",
		   defense_best[i]);
    } else {
      cat_snprintf(buf, sizeof(buf), "%s-", (i > 0) ? "/" : "");
    }
  }

  return buf;
}

/********************************************************************//**
  Returns number of supported units written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_supported(const struct city *pcity,
				      const void *data)
{
  static char buf[8];
  int num_supported = unit_list_size(pcity->units_supported);

  fc_snprintf(buf, sizeof(buf), "%2d", num_supported);

  return buf;
}

/********************************************************************//**
  Returns number of present units written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_present(const struct city *pcity,
				    const void *data)
{
  static char buf[8];
  int num_present = unit_list_size(pcity->tile->units);

  fc_snprintf(buf, sizeof(buf), "%2d", num_present);

  return buf;
}

/********************************************************************//**
  Returns string listing amounts of resources.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_resources(const struct city *pcity,
				      const void *data)
{
  static char buf[32];
  fc_snprintf(buf, sizeof(buf), "%d/%d/%d",
              pcity->surplus[O_FOOD],
              pcity->surplus[O_SHIELD],
              pcity->surplus[O_TRADE]);
  return buf;
}

/********************************************************************//**
  Returns food surplus written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_foodplus(const struct city *pcity,
				     const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->surplus[O_FOOD]);
  return buf;
}

/********************************************************************//**
  Returns production surplus written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_prodplus(const struct city *pcity,
				     const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->surplus[O_SHIELD]);
  return buf;
}

/********************************************************************//**
  Returns trade surplus written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_tradeplus(const struct city *pcity,
				      const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->surplus[O_TRADE]);
  return buf;
}

/********************************************************************//**
  Returns string describing resource output.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_output(const struct city *pcity,
				   const void *data)
{
  static char buf[32];
  int goldie = pcity->surplus[O_GOLD];

  fc_snprintf(buf, sizeof(buf), "%3d/%d/%d",
              goldie, pcity->prod[O_LUXURY], pcity->prod[O_SCIENCE]);
  return buf;
}

/********************************************************************//**
  Returns gold surplus written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_gold(const struct city *pcity,
				 const void *data)
{
  static char buf[8];

  if (pcity->surplus[O_GOLD] > 0) {
    fc_snprintf(buf, sizeof(buf), "+%d", pcity->surplus[O_GOLD]);
  } else {
    fc_snprintf(buf, sizeof(buf), "%3d", pcity->surplus[O_GOLD]);
  }
  return buf;
}

/********************************************************************//**
  Returns luxury output written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_luxury(const struct city *pcity,
				   const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->prod[O_LUXURY]);
  return buf;
}

/********************************************************************//**
  Returns science output written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_science(const struct city *pcity,
				    const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->prod[O_SCIENCE]);
  return buf;
}

/********************************************************************//**
  Returns number of turns before city grows written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_growturns(const struct city *pcity,
				      const void *data)
{
  int turns = city_turns_to_grow(pcity);
  char buffer[8];
  static char buf[32];

  if (turns == FC_INFINITY) {
    /* 'never' wouldn't be easily translatable here. */
    fc_snprintf(buffer, sizeof(buffer), "---");
  } else {
    /* Shrinking cities get a negative value. */
    fc_snprintf(buffer, sizeof(buffer), "%4d", turns);
  }
  fc_snprintf(buf, sizeof(buf), "%s (%d/%d)",
              buffer, pcity->food_stock,
              city_granary_size(city_size_get(pcity)));
  return buf;
}

/********************************************************************//**
  Returns pollution output written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_pollution(const struct city *pcity,
				      const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->pollution);
  return buf;
}

/********************************************************************//**
  Returns number and output of trade routes written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_trade_routes(const struct city *pcity,
                                         const void *data)
{
  static char buf[16];
  int num = 0, value = 0;

  trade_routes_iterate(pcity, proute) {
    num++;
    value += proute->value;
  } trade_routes_iterate_end;

  if (0 == num) {
    sz_strlcpy(buf, "0");
  } else {
    fc_snprintf(buf, sizeof(buf), "%d (+%d)", num, value);
  }
  return buf;
}

/********************************************************************//**
  Returns number of build slots written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_build_slots(const struct city *pcity,
                                        const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", city_build_slots(pcity));
  return buf;
}

/********************************************************************//**
  Returns name of current production.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_building(const struct city *pcity,
				     const void *data)
{
  static char buf[192];
  const char *from_worklist =
    worklist_is_empty(&pcity->worklist) ? "" :
    gui_options.concise_city_production ? "+" : _("(worklist)");

  if (city_production_has_flag(pcity, IF_GOLD)) {
    fc_snprintf(buf, sizeof(buf), "%s (%d)%s",
                city_production_name_translation(pcity),
                MAX(0, pcity->surplus[O_SHIELD]), from_worklist);
  } else {
    fc_snprintf(buf, sizeof(buf), "%s (%d/%s)%s",
                city_production_name_translation(pcity),
                pcity->shield_stock,
                city_production_cost_str(pcity),
                from_worklist);
  }

  return buf;
}

/********************************************************************//**
  Returns cost of buying current production and turns to completion
  written to string. Returned string is statically allocated and its
  contents change when this function is called again.
************************************************************************/
static const char *cr_entry_build_cost(const struct city *pcity,
				  const void *data)
{
  char bufone[8];
  char buftwo[8];
  static char buf[32];
  int price;
  int turns;

  if (city_production_has_flag(pcity, IF_GOLD)) {
    fc_snprintf(buf, sizeof(buf), "*");
    return buf;
  }
  price = pcity->client.buy_cost;
  turns = city_production_turns_to_build(pcity, TRUE);

  if (price > 99999) {
    fc_snprintf(bufone, sizeof(bufone), "---");
  } else {
    fc_snprintf(bufone, sizeof(bufone), "%d", price);
  }
  if (turns > 999) {
    fc_snprintf(buftwo, sizeof(buftwo), "--");
  } else {
    fc_snprintf(buftwo, sizeof(buftwo), "%3d", turns);
  }
  fc_snprintf(buf, sizeof(buf), "%s/%s", buftwo, bufone);
  return buf;
}

/********************************************************************//**
  Returns corruption amount written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_corruption(const struct city *pcity,
				       const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", -(pcity->waste[O_TRADE]));
  return buf;
}

/********************************************************************//**
  Returns waste amount written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_waste(const struct city *pcity,
				  const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", -(pcity->waste[O_SHIELD]));
  return buf;
}

/********************************************************************//**
  Returns risk percentage of plague written to string.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_plague_risk(const struct city *pcity,
                                        const void *data)
{
  static char buf[8];
  if (!game.info.illness_on) {
    fc_snprintf(buf, sizeof(buf), " -.-");
  } else {
    fc_snprintf(buf, sizeof(buf), "%4.1f",
                (float)city_illness_calc(pcity, NULL, NULL, NULL, NULL)/10.0);
  }
  return buf;
}

/********************************************************************//**
  Returns number of continent
************************************************************************/
static const char *cr_entry_continent(const struct city *pcity, 
                                      const void *data)
{
  static char buf[8];
  fc_snprintf(buf, sizeof(buf), "%3d", pcity->tile->continent);
  return buf;
}

/********************************************************************//**
  Returns city cma description.
  Returned string is statically allocated and its contents change when
  this function is called again.
************************************************************************/
static const char *cr_entry_cma(const struct city *pcity,
				const void *data)
{
  return cmafec_get_short_descr_of_city(pcity);
}

/* City report options (which columns get shown)
 * To add a new entry, you should just have to:
 * - add a function like those above
 * - add an entry in the base_city_report_specs[] table
 */

/* This generates the function name and the tagname: */
#define FUNC_TAG(var)  cr_entry_##var, #var 

static const struct city_report_spec base_city_report_specs[] = {
  { TRUE, -15, 0, NULL,  N_("?city:Name"), N_("City Name"),
    NULL, FUNC_TAG(cityname) },
  { FALSE, -15, 0, NULL, N_("Nation"),  N_("Nation"),
    NULL, FUNC_TAG(nation) },
  { TRUE,   2, 1, NULL,  N_("?size [short]:Sz"), N_("Size"),
    NULL, FUNC_TAG(size) },
  { TRUE,  -8, 1, NULL,  N_("State"),   N_("Celebrating/Peace/Disorder"),
    NULL, FUNC_TAG(hstate_verbose) },
  { FALSE,  1, 1, NULL,  NULL,          N_("Concise *=Celebrating, X=Disorder"),
    NULL, FUNC_TAG(hstate_concise) },

  { FALSE, 2, 1, NULL, N_("?Happy workers:H"), N_("Workers: Happy"),
    NULL, FUNC_TAG(happy) },
  { FALSE, 2, 1, NULL, N_("?Content workers:C"), N_("Workers: Content"),
    NULL, FUNC_TAG(content) },
  { FALSE, 2, 1, NULL, N_("?Unhappy workers:U"), N_("Workers: Unhappy"),
    NULL, FUNC_TAG(unhappy) },
  { FALSE, 2, 1, NULL, N_("?Angry workers:A"), N_("Workers: Angry"),
    NULL, FUNC_TAG(angry) },
  { TRUE, 10, 1, N_("?city:Workers"),
    N_("?happy/content/unhappy/angry:H/C/U/A"),
    N_("Workers: Happy, Content, Unhappy, Angry"),
    NULL, FUNC_TAG(workers) },

  { FALSE, 8, 1, N_("Best"), N_("attack"),
    N_("Best attacking units"), NULL, FUNC_TAG(attack)},
  { FALSE, 8, 1, N_("Best"), N_("defense"),
    N_("Best defending units"), NULL, FUNC_TAG(defense)},
  { FALSE, 2, 1, N_("Units"),
    /* TRANS: Header "Number of units inside city" */
    N_("?Present (units):Here"),
    N_("Number of units present"), NULL, FUNC_TAG(present) },
  { FALSE, 2, 1, N_("Units"),
    /* TRANS: Header "Number of units supported by given city" */
    N_("?Supported (units):Owned"),
    N_("Number of units supported"), NULL, FUNC_TAG(supported) },

  { /* TRANS: Header "It will take this many turns before city grows" */
    TRUE,  14, 1, N_("?food (population):Grow"),
    N_("?Stock/Target:(Have/Need)"),
    N_("Turns until growth/famine"),
    NULL, FUNC_TAG(growturns) },

  { TRUE,  10, 1, N_("Surplus"), N_("?food/production/trade:F/P/T"),
                                 N_("Surplus: Food, Production, Trade"),
    NULL, FUNC_TAG(resources) },
  { FALSE,  3, 1, NULL, N_("?Food surplus [short]:+F"), N_("Surplus: Food"),
    NULL, FUNC_TAG(foodplus) },
  { FALSE,  3, 1, NULL, N_("?Production surplus [short]:+P"),
    N_("Surplus: Production"), NULL, FUNC_TAG(prodplus) },
  { FALSE,  3, 1, NULL, N_("?Production loss (waste) [short]:-P"),
    N_("Waste"), NULL, FUNC_TAG(waste) },
  { FALSE,  3, 1, NULL, N_("?Trade surplus [short]:+T"), N_("Surplus: Trade"),
    NULL, FUNC_TAG(tradeplus) },
  { FALSE,  3, 1, NULL, N_("?Trade loss (corruption) [short]:-T"),
    N_("Corruption"), NULL, FUNC_TAG(corruption) },

  { TRUE,  10, 1, N_("Economy"), N_("?gold/luxury/science:G/L/S"),
                                 N_("Economy: Gold, Luxuries, Science"),
    NULL, FUNC_TAG(output) },
  { FALSE, 3, 1, NULL, N_("?Gold:G"), N_("Economy: Gold"),
    NULL, FUNC_TAG(gold) },
  { FALSE, 3, 1, NULL, N_("?Luxury:L"), N_("Economy: Luxury"),
    NULL, FUNC_TAG(luxury) },
  { FALSE, 3, 1, NULL, N_("?Science:S"), N_("Economy: Science"),
    NULL, FUNC_TAG(science) },
  { FALSE, 3, 1, NULL, N_("?Culture:Clt"), N_("Culture (History+Performance)"),
    NULL, FUNC_TAG(culture) },
  { FALSE, 3, 1, NULL, N_("?History:Hst"), N_("Culture: History"),
    NULL, FUNC_TAG(history) },
  { FALSE, 3, 1, NULL, N_("?Performance:Prf"), N_("Culture: Performance"),
    NULL, FUNC_TAG(performance) },
  { FALSE, 3, 1, NULL, N_("?Continent:C"), N_("Continent number"),
    NULL, FUNC_TAG(continent) },
  { FALSE,  1, 1, N_("?number_trade_routes:n"), N_("?number_trade_routes:R"),
                  N_("Number (and total value) of trade routes"),
    NULL, FUNC_TAG(trade_routes) },
  { FALSE,  3, 1, NULL, N_("?pollution [short]:Pol"), N_("Pollution"),
    NULL, FUNC_TAG(pollution) },
  { FALSE,  4, 1, N_("?plague risk [short]:Pla"), N_("(%)"), N_("Plague risk"),
    NULL, FUNC_TAG(plague_risk) },
  { FALSE, 15, 1, NULL, N_("?cma:Governor"), N_("Citizen Governor"),
    NULL, FUNC_TAG(cma) },

  /* TRANS: "BS" = "build slots" */
  { FALSE,  3, 1, NULL, N_("BS"), N_("Maximum units buildable per turn"),
    NULL, FUNC_TAG(build_slots) },
  { TRUE,   9, 1, N_("Production"), N_("Turns/Buy"),
  /*N_("Turns or gold to complete production"), future menu needs translation */
    N_("Production"),
    NULL, FUNC_TAG(build_cost) },
  { TRUE,   0, 1, N_("Currently Building"),
    N_("?Stock/Target:(Have/Need)"),
    N_("Currently Building"),
    NULL, FUNC_TAG(building) }
};

struct city_report_spec *city_report_specs;
static int num_creport_cols;

/********************************************************************//**
  Simple wrapper for num_creport_cols()
************************************************************************/
int num_city_report_spec(void)
{
  return num_creport_cols;
}

/********************************************************************//**
  Simple wrapper for city_report_specs.show
************************************************************************/
bool *city_report_spec_show_ptr(int i)
{
  return &(city_report_specs[i].show);
}

/********************************************************************//**
  Simple wrapper for city_report_specs.tagname
************************************************************************/
const char *city_report_spec_tagname(int i)
{
  return city_report_specs[i].tagname;
}

/********************************************************************//**
  Initialize city report data.  This deals with ruleset-depedent
  columns and pre-translates the fields (to make things easier on
  the GUI writers).  Should be called before the GUI starts up.
************************************************************************/
void init_city_report_game_data(void)
{
  static char sp_explanation[SP_MAX][128];
  static char sp_explanations[SP_MAX*128];
  struct city_report_spec *p;
  int i;

  num_creport_cols = ARRAY_SIZE(base_city_report_specs)
                   + specialist_count() + 1;
  city_report_specs
    = fc_realloc(city_report_specs,
		 num_creport_cols * sizeof(*city_report_specs));
  p = &city_report_specs[0];

  fc_snprintf(sp_explanations, sizeof(sp_explanations),
              "%s", _("Specialists: "));
  specialist_type_iterate(sp) {
    struct specialist *s = specialist_by_number(sp);

    p->show = FALSE;
    p->width = 2;
    p->space = 1;
    p->title1 = Q_("?specialist:S");
    p->title2 = specialist_abbreviation_translation(s);
    fc_snprintf(sp_explanation[sp], sizeof(sp_explanation[sp]),
                _("Specialists: %s"), specialist_plural_translation(s));
    cat_snprintf(sp_explanations, sizeof(sp_explanations),
                 "%s%s", (sp == 0) ? "" : ", ",
                 specialist_plural_translation(s));
    p->explanation = sp_explanation[sp];
    p->data = s;
    p->func = cr_entry_specialist;
    p->tagname = specialist_rule_name(s);
    p++;
  } specialist_type_iterate_end;
  
  /* Summary column for all specialists. */
  {
    static char sp_summary[128];

    p->show = FALSE;
    p->width = MAX(7, specialist_count()*2-1);
    p->space = 1;
    p->title1 = _("Special");
    fc_snprintf(sp_summary, sizeof(sp_summary),
                "%s", specialists_abbreviation_string());
    p->title2 = sp_summary;
    p->explanation = sp_explanations;
    p->data = NULL;
    p->func = cr_entry_specialists;
    p->tagname = "specialists";
    p++;
  }

  memcpy(p, base_city_report_specs,
	 sizeof(base_city_report_specs));

  for (i = 0; i < ARRAY_SIZE(base_city_report_specs); i++) {
    if (p->title1) {
      p->title1 = Q_(p->title1);
    }
    if (p->title2) {
      p->title2 = Q_(p->title2);
    }
    p->explanation = _(p->explanation);
    p++;
  }

  fc_assert(NUM_CREPORT_COLS == ARRAY_SIZE(base_city_report_specs)
            + specialist_count() + 1);
}

/************************************************************************
  The following several functions allow intelligent sorting city report
  fields by column.  This doesn't necessarily do the right thing, but
  it's better than sorting alphabetically.

  The GUI gives us two values to compare (as strings).  We try to split
  them into an array of numeric and string fields, then we compare
  lexicographically.  Two numeric fields are compared in the obvious
  way, two character fields are compared alphabetically.  Arbitrarily, a
  numeric field is sorted before a character field (for "justification"
  note that numbers are before letters in the ASCII table).
************************************************************************/

/* A datum is one short string, or one number.
   A datum_vector represents a long string of alternating strings and
   numbers. */
struct datum {
  union {
    float numeric_value;
    char *string_value;
  } val;
  bool is_numeric;
};
#define SPECVEC_TAG datum
#include "specvec.h"

/********************************************************************//**
  Init a datum from a substring.
************************************************************************/
static void init_datum_string(struct datum *dat, const char *left,
                              const char *right)
{
  int len = right - left;

  dat->is_numeric = FALSE;
  dat->val.string_value = fc_malloc(len + 1);
  memcpy(dat->val.string_value, left, len);
  dat->val.string_value[len] = 0;
}

/********************************************************************//**
  Init a datum from a number (a float because we happen to use
  strtof).
************************************************************************/
static void init_datum_number(struct datum *dat, float val)
{
  dat->is_numeric = TRUE;
  dat->val.numeric_value = val;
}

/********************************************************************//**
  Free the data associated with a datum -- that is, free the string if
  it was allocated.
************************************************************************/
static void free_datum(struct datum *dat)
{
  if (!dat->is_numeric) {
    free(dat->val.string_value);
  }
}

/********************************************************************//**
  Compare two data items as described above:
  - numbers in the obvious way
  - strings alphabetically
  - number < string for no good reason
************************************************************************/
static int datum_compare(const struct datum *a, const struct datum *b)
{
  if (a->is_numeric == b->is_numeric) {
    if (a->is_numeric) {
      if (a->val.numeric_value == b->val.numeric_value) {
        return 0;
      } else if (a->val.numeric_value < b->val.numeric_value) {
        return -1;
      } else if (a->val.numeric_value > b->val.numeric_value) {
        return +1;
      } else {
        return 0; /* shrug */
      }
    } else {
      return strcmp(a->val.string_value, b->val.string_value);
    }
  } else {
    if (a->is_numeric) {
      return -1;
    } else {
      return 1;
    }
  }
}

/********************************************************************//**
  Compare two strings of data lexicographically.
************************************************************************/
static int data_compare(const struct datum_vector *a,
                        const struct datum_vector *b)
{
  int i, n;

  n = MIN(a->size, b->size);

  for (i = 0; i < n; i++) {
    int cmp = datum_compare(&a->p[i], &b->p[i]);

    if (cmp != 0) {
      return cmp;
    }
  }

  /* The first n fields match; whoever has more fields goes last.
     If they have equal numbers, the two really are equal. */
  return a->size - b->size;
}

/********************************************************************//**
  Split a string into a vector of datum.
************************************************************************/
static void split_string(struct datum_vector *data, const char *str)
{
  const char *string_start;

  datum_vector_init(data);
  string_start = str;
  while (*str) {
    char *endptr;
    float value;

    errno = 0;
    value = strtof(str, &endptr);
    if (errno != 0 || endptr == str || !isfinite(value)) {
      /* that wasn't a sensible number; go on */
      str++;
    } else {
      /* that was a number, so stop the string we were parsing, add 
         it (unless it's empty), then add the number we just parsed */
      struct datum d;

      if (str != string_start) {
        init_datum_string(&d, string_start, str);
        datum_vector_append(data, d);
      }

      init_datum_number(&d, value);
      datum_vector_append(data, d);

      /* finally, update the string position pointers */
      string_start = str = endptr;
    }
  }

  /* if we have anything leftover then it's a string */
  if (str != string_start) {
    struct datum d;

    init_datum_string(&d, string_start, str);
    datum_vector_append(data, d);
  }
}

/********************************************************************//**
  Free every datum in the vector.
************************************************************************/
static void free_data(struct datum_vector *data)
{
  int i;

  for (i = 0; i < data->size; i++) {
    free_datum(&data->p[i]);
  }
  datum_vector_free(data);
}

/********************************************************************//**
  The real function: split the two strings, and compare them.
************************************************************************/
int cityrepfield_compare(const char *str1, const char *str2)
{
  struct datum_vector data1, data2;
  int retval;

  if (str1 == str2) {
    return 0;
  } else if (NULL == str1) {
    return 1;
  } else if (NULL == str2) {
    return -1;
  }

  split_string(&data1, str1);
  split_string(&data2, str2);

  retval = data_compare(&data1, &data2);

  free_data(&data1);
  free_data(&data2);

  return retval;
}

/********************************************************************//**
  Same as can_city_sell_building(), but with universal argument.
************************************************************************/
bool can_city_sell_universal(const struct city *pcity,
                             const struct universal *target)
{
  return target->kind == VUT_IMPROVEMENT
    && can_city_sell_building(pcity, target->value.building);
}
