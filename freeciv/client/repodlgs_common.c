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

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"		/* free */
#include "support.h"		/* my_snprintf */

/* common */
#include "game.h"
#include "government.h"
#include "unitlist.h"

#include "repodlgs_g.h"

/* client */
#include "client_main.h"
#include "connectdlg_common.h"	/* is_server_running */
#include "control.h"
#include "options.h"
#include "repodlgs_common.h"
#include "packhand_gen.h"


char **options_categories = NULL;
struct options_settable *settable_options = NULL;

int num_options_categories = 0;
int num_settable_options = 0;

static bool settable_options_loaded = FALSE;	/* desired options from file */


/****************************************************************
  Fills out the array of struct improvement_entry given by
  entries. The array must be able to hold at least B_LAST entries.
*****************************************************************/
void get_economy_report_data(struct improvement_entry *entries,
			     int *num_entries_used, int *total_cost,
			     int *total_income)
{
  *num_entries_used = 0;
  *total_cost = 0;
  *total_income = 0;

  if (NULL == client.conn.playing) {
    return;
  }

  improvement_iterate(pimprove) {
    if (is_improvement(pimprove)) {
      int count = 0, cost = 0;
      city_list_iterate(client.conn.playing->cities, pcity) {
	if (city_has_building(pcity, pimprove)) {
	  count++;
	  cost += city_improvement_upkeep(pcity, pimprove);
	}
      }
      city_list_iterate_end;

      if (count == 0) {
	continue;
      }

      entries[*num_entries_used].type = pimprove;
      entries[*num_entries_used].count = count;
      entries[*num_entries_used].total_cost = cost;
      entries[*num_entries_used].cost = cost / count;
      (*num_entries_used)++;

      /* Currently there is no building expense under anarchy.  It's
       * not a good idea to hard-code this in the client, but what
       * else can we do? */
      if (government_of_player(client.conn.playing) !=
          game.government_during_revolution) {
        *total_cost += cost;
      }
    }
  } improvement_iterate_end;

  city_list_iterate(client.conn.playing->cities, pcity) {
    *total_income += pcity->prod[O_GOLD];
    if (city_production_has_flag(pcity, IF_GOLD)) {
      *total_income += MAX(0, pcity->surplus[O_SHIELD]);
    }
  } city_list_iterate_end;
}

/******************************************************************
  Returns an array of units with gold_upkeep. Number of units in 
  the array is added to num_entries_used.
******************************************************************/
void get_economy_report_units_data(struct unit_entry *entries,
				   int *num_entries_used, int *total_cost)
{
  int count, cost, partial_cost;

  *num_entries_used = 0;
  *total_cost = 0;

  if (NULL == client.conn.playing) {
    return;
  }

  unit_type_iterate(unittype) {
    cost = utype_upkeep_cost(unittype, client.conn.playing, O_GOLD);

    if (cost == 0) {
      /* Short-circuit all of the following checks. */
      continue;
    }

    count = 0;
    partial_cost = 0;

    city_list_iterate(client.conn.playing->cities, pcity) {
      unit_list_iterate(pcity->units_supported, punit) {
	if (unit_type(punit) == unittype) {
	  count++;
	  partial_cost += punit->upkeep[O_GOLD];
	}

      } unit_list_iterate_end;
    } city_list_iterate_end;

    if (count == 0) {
      continue;
    }

    (*total_cost) += partial_cost;

    entries[*num_entries_used].type = unittype;
    entries[*num_entries_used].count = count;
    entries[*num_entries_used].cost = cost;
    entries[*num_entries_used].total_cost = partial_cost;
    (*num_entries_used)++;

  } unit_type_iterate_end;
}

static int frozen_level = 0;

/******************************************************************
 Turn off updating of reports
*******************************************************************/
void report_dialogs_freeze(void)
{
  frozen_level++;
}

/******************************************************************
 Turn on updating of reports
*******************************************************************/
void report_dialogs_thaw(void)
{
  frozen_level--;
  assert(frozen_level >= 0);
  if (frozen_level == 0) {
    update_report_dialogs();
  }
}

/******************************************************************
 Turn on updating of reports
*******************************************************************/
void report_dialogs_force_thaw(void)
{
  frozen_level = 1;
  report_dialogs_thaw();
}

/******************************************************************
 ...
*******************************************************************/
bool is_report_dialogs_frozen(void)
{
  return frozen_level > 0;
}

/******************************************************************
 initialize settable_options[] and options_categories[]
*******************************************************************/
void settable_options_init(void)
{
  settable_options_loaded = FALSE;

  settable_options = NULL;
  num_settable_options = 0;

  options_categories = NULL;
  num_options_categories = 0;
}

/******************************************************************
 free and clear all settable_option packet strings
*******************************************************************/
static void settable_option_strings_free(struct options_settable *o)
{
  if (NULL != o->name) {
    free(o->name);
    o->name = NULL;
  }
  if (NULL != o->short_help) {
    free(o->short_help);
    o->short_help = NULL;
  }
  if (NULL != o->extra_help) {
    free(o->extra_help);
    o->extra_help = NULL;
  }
  if (NULL != o->strval) {
    free(o->strval);
    o->strval = NULL;
  }
  if (NULL != o->default_strval) {
    free(o->default_strval);
    o->default_strval = NULL;
  }
}

/******************************************************************
 free settable_options[] and options_categories[]
*******************************************************************/
void settable_options_free(void)
{
  int i;

  for (i = 0; i < num_settable_options; i++) {
    struct options_settable *o = &settable_options[i];
    settable_option_strings_free(o);

    /* special handling for non-packet strings */
    if (NULL != o->desired_strval) {
      free(o->desired_strval);
      o->desired_strval = NULL;
    }
  }
  free(settable_options);

  for (i = 0; i < num_options_categories; i++) {
    free(options_categories[i]);
  }
  free(options_categories);

  settable_options_init();
}

/******************************************************************
 reinitialize the struct options_settable: allocate enough
 space for all the options that the server is going to send us.
*******************************************************************/
void handle_options_settable_control(
                               struct packet_options_settable_control *packet)
{
  int i; 

  settable_options_free();

  /* avoid a malloc of size 0 warning */
  if (0 == packet->num_categories
   || 0 == packet->num_settings) {
    return;
  }

  options_categories = fc_calloc(packet->num_categories,
				 sizeof(*options_categories));
  num_options_categories = packet->num_categories;
  
  for (i = 0; i < num_options_categories; i++) {
    options_categories[i] = mystrdup(packet->category_names[i]);
  }

  settable_options = fc_calloc(packet->num_settings,
			       sizeof(*settable_options));
  num_settable_options = packet->num_settings;
}

/******************************************************************
 Fill the settable_options array with an option.
*******************************************************************/
void handle_options_settable(struct packet_options_settable *packet)
{
  struct options_settable *o;
  int i = packet->id;

  if (i < 0 || i > num_settable_options) {
    freelog(LOG_ERROR,
	    "handle_options_settable() bad id %d.",
	    packet->id);
    return;
  }
  o = &settable_options[i];

  o->stype = packet->stype;
  o->scategory = packet->scategory;
  o->sclass = packet->sclass;

  o->val = packet->val;
  o->default_val = packet->default_val;
  if (!settable_options_loaded) {
    o->desired_val = packet->default_val;
    /* desired_val is loaded later */
  }
  o->min = packet->min;
  o->max = packet->max;

  /* ensure packet string fields are NULL for repeat calls */
  settable_option_strings_free(o);

  switch (o->stype) {
  case SSET_BOOL:
    o->min = FALSE;
    o->max = TRUE;				/* server sent FALSE */
    break;
  case SSET_INT:
    break;
  case SSET_STRING:
    o->strval = mystrdup(packet->strval);
    o->default_strval = mystrdup(packet->default_strval);
    /* desired_strval is loaded later */
    break;
  default:
    freelog(LOG_ERROR,
	    "handle_options_settable() bad type %d.",
	    packet->stype);
    return;
  };

  /* only set for valid type */
  o->is_visible = packet->is_visible;
  o->name = mystrdup(packet->name);
  o->short_help = mystrdup(packet->short_help);
  //o->extra_help = mystrdup(packet->extra_help); FIXME: removed for webclient. - Andreas

  /* have no proper final packet, test for the last instead */
  if (i == (num_settable_options - 1) && !settable_options_loaded) {
    /* Only send our private settings if we are running
     * on a forked local server, i.e. started by the
     * client with the "Start New Game" button. */
    load_settable_options(is_server_running());
    settable_options_loaded = TRUE;
  }
}

/****************************************************************************
  Sell all improvements of the given type in all cities.  If "obsolete_only"
  is specified then only those improvements that are replaced will be sold.

  The "message" string will be filled with a GUI-friendly message about
  what was sold.
****************************************************************************/
void sell_all_improvements(struct impr_type *pimprove, bool obsolete_only,
			   char *message, size_t message_sz)
{
  int count = 0, gold = 0;

  if (!can_client_issue_orders()) {
    my_snprintf(message, message_sz, _("You cannot sell improvements."));
    return;
  }

  city_list_iterate(client.conn.playing->cities, pcity) {
    if (!pcity->did_sell && city_has_building(pcity, pimprove)
	&& (!obsolete_only
	    || improvement_obsolete(client.conn.playing, pimprove)
	    || is_building_replaced(pcity, pimprove, RPT_CERTAIN))) {
      count++;
      gold += impr_sell_gold(pimprove);
      city_sell_improvement(pcity, improvement_number(pimprove));
    }
  } city_list_iterate_end;

  if (count > 0) {
    my_snprintf(message, message_sz, _("Sold %d %s for %d gold."),
		count,
		improvement_name_translation(pimprove),
		gold);
  } else {
    my_snprintf(message, message_sz, _("No %s could be sold."),
		improvement_name_translation(pimprove));
  }
}

/****************************************************************************
  Disband all supported units of the given type.  If in_cities_only is
  specified then only units inside our cities will be disbanded.

  The "message" string will be filled with a GUI-friendly message about
  what was sold.
****************************************************************************/
void disband_all_units(struct unit_type *punittype, bool in_cities_only,
		       char *message, size_t message_sz)
{
  int count = 0;

  if (!can_client_issue_orders()) {
    /* TRANS: Obscure observer error. */
    my_snprintf(message, message_sz, _("You cannot disband units."));
    return;
  }

  if (utype_has_flag(punittype, F_UNDISBANDABLE)) {
    my_snprintf(message, message_sz, _("%s cannot be disbanded."),
		utype_name_translation(punittype));
    return;
  }

  city_list_iterate(client.conn.playing->cities, pcity) {
    /* Only supported units are disbanded.  Units with no homecity have no
     * cost and are not disbanded. */
    unit_list_iterate(pcity->units_supported, punit) {
      struct city *incity = tile_city(punit->tile);

      if (unit_type(punit) == punittype
	  && (!in_cities_only
	      || (incity && city_owner(incity) == client.conn.playing))) {
	count++;
	request_unit_disband(punit);
      }
    } unit_list_iterate_end;
  } city_list_iterate_end;

  if (count > 0) {
    my_snprintf(message, message_sz, _("Disbanded %d %s."),
		count,
		utype_name_translation(punittype));
  } else {
    my_snprintf(message, message_sz, _("No %s could be disbanded."),
		utype_name_translation(punittype));
  }
}
