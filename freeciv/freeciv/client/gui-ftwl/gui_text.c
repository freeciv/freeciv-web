/********************************************************************** 
 Freeciv - Copyright (C) 2004 - The Freeciv Poject
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
#include "support.h"

/* common */
#include "map.h"
#include "combat.h"
#include "government.h"

/* client */
#include "climisc.h"
#include "client_main.h"
#include "control.h"
#include "goto.h"
#include "text.h"
#include "unitlist.h"

#include "gui_text.h"

/*
 * An individual add(_line) string has to fit into GROW_TMP_SIZE. One buffer
 * of GROW_TMP_SIZE size will be allocated for the entire client.
 */
#define GROW_TMP_SIZE	(1024)

/* 
 * Initial size of the buffer for each function.  It may later be
 * grown as needed.
 */
#define START_SIZE	10

static void real_add_line(char **buffer, size_t * buffer_size,
			  const char *format, ...)
fc__attribute((__format__(__printf__, 3, 4)));
static void real_add(char **buffer, size_t * buffer_size,
		     const char *format, ...)
fc__attribute((__format__(__printf__, 3, 4)));

#define add_line(...) real_add_line(&out,&out_size, __VA_ARGS__)
#define add(...) real_add(&out,&out_size, __VA_ARGS__)

#define INIT			\
  static char *out = NULL;	\
  static size_t out_size = 0;	\
  if (!out) {			\
    out_size = START_SIZE;	\
    out = fc_malloc(out_size);	\
  }				\
  out[0] = '\0';

#define RETURN	return out;

/****************************************************************************
  Formats the parameters and appends them. Grows the buffer if
  required.
****************************************************************************/
static void grow_printf(char **buffer, size_t *buffer_size,
			const char *format, va_list ap)
{
  size_t new_len;
  static char buf[GROW_TMP_SIZE];

  if (my_vsnprintf(buf, sizeof(buf), format, ap) == -1) {
    die("Formatted string bigger than %lu", (unsigned long)sizeof(buf));
  }

  new_len = strlen(*buffer) + strlen(buf) + 1;

  if (new_len > *buffer_size) {
    /* It's important that we grow the buffer geometrically, otherwise the
     * overhead adds up quickly. */
    size_t new_size = MAX(new_len, *buffer_size * 2);

    freelog(LOG_VERBOSE, "expand from %lu to %lu to add '%s'",
	    (unsigned long)*buffer_size, (unsigned long)new_size, buf);

    *buffer_size = new_size;
    *buffer = fc_realloc(*buffer, *buffer_size);
  }
  mystrlcat(*buffer, buf, *buffer_size);
}

/****************************************************************************
  Add a full line of text to the buffer.
****************************************************************************/
static void real_add_line(char **buffer, size_t * buffer_size,
			  const char *format, ...)
{
  va_list args;

  if ((*buffer)[0] != '\0') {
    real_add(buffer, buffer_size, "%s", "\n");
  }

  va_start(args, format);
  grow_printf(buffer, buffer_size, format, args);
  va_end(args);
}

/****************************************************************************
  Add the text to the buffer.
****************************************************************************/
static void real_add(char **buffer, size_t * buffer_size, const char *format,
		     ...)
{
  va_list args;

  va_start(args, format);
  grow_printf(buffer, buffer_size, format, args);
  va_end(args);
}

/****************************************************************************
  Return a short tooltip text for a terrain tile.
****************************************************************************/
const char *mapview_get_terrain_tooltip_text(struct tile *ptile)
{
  int count;
    
  bv_special infrastructure = get_tile_infrastructure_set(ptile, &count);
               
  INIT;

#ifdef DEBUG
  add_line(_("Location: (%d, %d) [%d]"),
	   TILE_XY(ptile),
	   tile_continent(ptile));
#endif /* DEBUG */
  add_line("%s", tile_get_info_text(ptile, 0));
  if (count > 0) {
    add_line("%s",
	     get_infrastructure_text(infrastructure, ptile->bases));
  }
  RETURN;
}

/****************************************************************************
  Calculate the effects of various unit activities.
****************************************************************************/
static void calc_effect(enum unit_activity activity, struct tile *ptile,
			int diff[3])
{
  struct tile backup = *ptile;
  int stats_before[3], stats_after[3];
  /* FIXME: use output_type_iterate! */

  stats_before[0] = city_tile_output(NULL, ptile, FALSE, O_FOOD);
  stats_before[1] = city_tile_output(NULL, ptile, FALSE, O_SHIELD);
  stats_before[2] = city_tile_output(NULL, ptile, FALSE, O_TRADE);

  /* BEWARE UGLY HACK AHEAD */

  switch (activity) {
  case ACTIVITY_ROAD:
  case ACTIVITY_RAILROAD:
  case ACTIVITY_MINE:
  case ACTIVITY_IRRIGATE:
  case ACTIVITY_TRANSFORM:
    tile_apply_activity(ptile, activity);
    break;
  default:
    assert(0);
  }

  stats_after[0] = city_tile_output(NULL, ptile, FALSE, O_FOOD);
  stats_after[1] = city_tile_output(NULL, ptile, FALSE, O_SHIELD);
  stats_after[2] = city_tile_output(NULL, ptile, FALSE, O_TRADE);

  ptile->terrain = backup.terrain;
  ptile->special = backup.special;
  /* hopefully everything is now back in place */

  diff[0] = stats_after[0] - stats_before[0];
  diff[1] = stats_after[1] - stats_before[1];
  diff[2] = stats_after[2] - stats_before[2];
}

/****************************************************************************
  Return a text describing what would be the results from a unit activity.
****************************************************************************/
static const char *format_effect(enum unit_activity activity,
				 struct unit *punit)
{
  char parts[3][25];
  int diff[3];
  int n = 0;
  INIT;

  calc_effect(activity, punit->tile, diff);

  if (diff[0] != 0) {
    my_snprintf(parts[n], sizeof(parts[n]), _("%+d food"), diff[0]);
    n++;
  }

  if (diff[1] != 0) {
    my_snprintf(parts[n], sizeof(parts[n]), _("%+d shield"), diff[1]);
    n++;
  }

  if (diff[2] != 0) {
    my_snprintf(parts[n], sizeof(parts[n]), _("%+d trade"), diff[2]);
    n++;
  }
  if (n == 0) {
    add(_("none"));
  } else if (n == 1) {
    add("%s", parts[0]);
  } else if (n == 2) {
    add("%s %s", parts[0], parts[1]);
  } else if (n == 3) {
    add("%s %s %s", parts[0], parts[1],		parts[2]);
  } else {
    assert(0);
  }
  RETURN;
}

/****************************************************************************
  Return a short text for tooltip describing what a unit action does.
****************************************************************************/
const char *mapview_get_unit_action_tooltip(struct unit *punit,
					    const char *action,
					    const char *shortcut_)
{
  char shortcut[256];
  INIT;

  if (shortcut_) {
    my_snprintf(shortcut, sizeof(shortcut), " (%s)", shortcut_);
  } else {
    my_snprintf(shortcut, sizeof(shortcut), "%s", "");
  }

  if (strcmp(action, "unit_fortifying") == 0) {
    add_line(_("Fortify%s"), shortcut);
    add_line(_("Time: 1 turn, then until changed"));
    add_line(_("Effect: +50%% defense bonus"));
  } else if (strcmp(action, "unit_disband") == 0) {
    add_line(_("Disband%s"), shortcut);
    add_line(_("Time: instantly, unit is destroyed"));
  } else if (strcmp(action, "unit_return_nearest") == 0) {
    add_line(_("Return to nearest city%s"), shortcut);
    add_line(_("Time: unknown"));
  } else if (strcmp(action, "unit_sentry") == 0) {
    add_line(_("Sentry%s"), shortcut);
    add_line(_("Time: instantly, lasts until changed"));
    add_line(_("Effect: Unit wakes up if enemy is near"));
  } else if (strcmp(action, "unit_add_to_city") == 0) {
    add_line(_("Add to city%s"), shortcut);
    add_line(_("Time: instantly, unit is destroyed"));
    add_line(_("Effect: city size +1"));
  } else if (strcmp(action, "unit_build_city") == 0) {
    add_line(_("Build city%s"), shortcut);
    add_line(_("Time: instantly, unit is destroyed"));
    add_line(_("Effect: create a city of size 1"));
  } else if (strcmp(action, "unit_road") == 0) {
    add_line(_("Build road%s"), shortcut);
    add_line(_("Time: %d turns"),
          get_turns_for_activity_at(punit, ACTIVITY_ROAD, punit->tile));
    add_line(_("Effect: %s"),
	     format_effect(ACTIVITY_ROAD, punit));
  } else if (strcmp(action, "unit_irrigate") == 0) {
    add_line(_("Build irrigation%s"),shortcut);
    add_line(_("Time: %d turns"),
      get_turns_for_activity_at(punit, ACTIVITY_IRRIGATE, punit->tile));
    add_line(_("Effect: %s"),
	     format_effect(ACTIVITY_IRRIGATE, punit));
  } else if (strcmp(action, "unit_mine") == 0) {
    add_line(_("Build mine%s"),shortcut);
    add_line(_("Time: %d turns"),
	 get_turns_for_activity_at(punit, ACTIVITY_MINE, punit->tile));
    add_line(_("Effect: %s"),
	     format_effect(ACTIVITY_MINE, punit));
  } else if (strcmp(action, "unit_auto_settler") == 0) {
    add_line(_("Auto-Settle%s"),shortcut);
    add_line(_("Time: unknown"));
    add_line(_("Effect: the computer performs settler activities"));
  } else {
#if 0
  ttype = tile_terrain(punit->tile);
  tinfo = terrain_by_number(ttype);
  if ((tinfo->irrigation_result != T_LAST)
      && (tinfo->irrigation_result != ttype)) {
    my_snprintf(irrtext, sizeof(irrtext), irrfmt,
		terrain_name_translation(tinfo->irrigation_result));
  } else if (tile_has_special(punit->tile, S_IRRIGATION)
	     && player_knows_techs_with_flag(client.conn.playing, TF_FARMLAND)) {
    sz_strlcpy(irrtext, _("Bu_ild Farmland"));
  }
  if ((tinfo->mining_result != T_LAST) && (tinfo->mining_result != ttype)) {
    my_snprintf(mintext, sizeof(mintext), minfmt,
		terrain_name_translation(tinfo->mining_result));
  }
  if ((tinfo->transform_result != T_LAST)
      && (tinfo->transform_result != ttype)) {
    my_snprintf(transtext, sizeof(transtext), transfmt,
		terrain_name_translation(tinfo->transform_result));
  }

  menus_rename("<main>/_Orders/Build _Irrigation", irrtext);
  menus_rename("<main>/_Orders/Build _Mine", mintext);
  menus_rename("<main>/_Orders/Transf_orm Terrain", transtext);
#endif
    add_line("tooltip for action %s isn't written yet",
	     action);
    freelog(LOG_VERBOSE,
	    "warning: get_unit_action_tooltip: unknown action %s",
	    action);
  }
  RETURN;
}

/****************************************************************************
  Get a tooltip for a possible city action.
****************************************************************************/
const char *mapview_get_city_action_tooltip(struct city *pcity,
					    const char *action,
					    const char *shortcut_)
{
  INIT;

  if (strcmp(action, "city_buy") == 0) {
    add_line(_("Buy production"));
    add_line(_("Cost: %d (%d in treasury)"),
	     city_production_buy_gold_cost(pcity),
	     client.conn.playing->economic.gold);
    add_line(_("Producting: %s (%d turns)"),
	     city_production_name_translation(pcity),
	     city_production_turns_to_build(pcity, TRUE));
  } else {
    add_line("tooltip for action %s isn't written yet", action);
    freelog(LOG_VERBOSE,
	    "warning: get_city_action_tooltip: unknown action %s", action);
  }
  RETURN;
}  

/****************************************************************************
  Get a short tooltip for a city.
****************************************************************************/
const char *mapview_get_city_tooltip_text(struct city *pcity)
{
  struct player *owner = city_owner(pcity);
  INIT;

  add_line("%s", city_name(pcity));
  add_line("%s", player_name(owner));
  RETURN;
}

/****************************************************************************
  Get a longer tooltip for a city.
****************************************************************************/
const char *mapview_get_city_info_text(struct city *pcity)
{
  INIT;

  add_line(_("City: %s (%s)"), city_name(pcity),
	   nation_adjective_for_player(city_owner(pcity)));
  if (city_got_citywalls(pcity)) {
    add(_(" with City Walls"));
  }
  RETURN;
}

/****************************************************************************
  Get a tooltip for a unit.
****************************************************************************/
const char *mapview_get_unit_tooltip_text(struct unit *punit)
{
  struct unit_type *ptype = unit_type(punit);
  struct city *pcity =
      player_find_city_by_id(client.conn.playing, punit->homecity);
  INIT;

  add("%s", utype_name_translation(ptype));
  if (ptype->veteran[punit->veteran].name[0] != '\0') {
    add(" (%s)", ptype->veteran[punit->veteran].name);
  }
  add("\n");
  add_line("%s", unit_activity_text(punit));
  if (pcity) {
    add_line("%s", city_name(pcity));
  }
  RETURN;
}

/****************************************************************************
  Get a longer tooltip for a unit.
  FIXME: should use same method as client/text.c popup_info_text()
****************************************************************************/
const char *mapview_get_unit_info_text(struct unit *punit)
{
  struct tile *ptile = punit->tile;
  const char *activity_text = concat_tile_activity_text(ptile);
  INIT;

  if (strlen(activity_text)) {
    add_line(_("Activity: %s"), activity_text);
  }
  if (punit) {
    struct player *owner = unit_owner(punit);
    struct unit_type *ptype = unit_type(punit);

    if (owner == client.conn.playing) {
      struct city *pcity = player_find_city_by_id(owner, punit->homecity);

      if (pcity){
	/* TRANS: "Unit: Musketeers (Polish, Warsaw)" */
	add_line(_("Unit: %s (%s, %s)"),
		 utype_name_translation(ptype),
		 nation_adjective_for_player(owner),
		 city_name(pcity));
      } else {
	/* TRANS: "Unit: Musketeers (Polish)" */
	add_line(_("Unit: %s (%s)"),
		 utype_name_translation(ptype),
		 nation_adjective_for_player(owner));
      }
    } else {
      struct unit *apunit = head_of_units_in_focus();  /* FIXME, need best in stack */

      /* TRANS: "Unit: Musketeers (Polish)" */
      add_line(_("Unit: %s (%s)"),
               utype_name_translation(ptype),
               nation_adjective_for_player(owner));

      if (apunit) {
	/* chance to win when active unit is attacking the selected unit */
	int att_chance = unit_win_chance(apunit, punit) * 100;
	
	/* chance to win when selected unit is attacking the active unit */
	int def_chance = (1.0 - unit_win_chance(punit, apunit)) * 100;
	
	add_line(_("Chance to win: A:%d%% D:%d%%"), att_chance, def_chance);
      }
    }
    add_line(_("A:%d D:%d FP:%d HP:%d/%d%s"),
	     ptype->attack_strength,
	     ptype->defense_strength, ptype->firepower, punit->hp,
	     ptype->hp, punit->veteran ? _(" V") : "");
  } 
  RETURN;
}
