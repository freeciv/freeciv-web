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

/********************************************************************** 
This module contains various general - mostly highlevel - functions
used throughout the client.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "city.h"
#include "diptreaty.h"
#include "featured_text.h"
#include "game.h"
#include "government.h"
#include "map.h"
#include "packets.h"
#include "spaceship.h"
#include "unitlist.h"

/* include */
#include "chatline_g.h"
#include "citydlg_g.h"
#include "cityrep_g.h"
#include "dialogs_g.h"
#include "gui_main_g.h"
#include "mapview_g.h"

/* client */
#include "client_main.h"
#include "climap.h"
#include "climisc.h"
#include "control.h"
#include "mapctrl_common.h"
#include "mapview_common.h"
#include "messagewin_common.h"
#include "packhand.h"
#include "plrdlg_common.h"
#include "repodlgs_common.h"
#include "tilespec.h"


/**************************************************************************
...
**************************************************************************/
void client_remove_unit(struct unit *punit)
{
  struct city *pcity;
  struct tile *ptile = punit->tile;
  int hc = punit->homecity;
  struct unit old_unit = *punit;
  int old = get_num_units_in_focus();
  bool update;

  freelog(LOG_DEBUG, "removing unit %d, %s %s (%d %d) hcity %d",
	  punit->id,
	  nation_rule_name(nation_of_unit(punit)),
	  unit_rule_name(punit),
	  TILE_XY(punit->tile), hc);

  update = (get_focus_unit_on_tile(punit->tile) != NULL);
  control_unit_killed(punit);
  game_remove_unit(punit);
  punit = NULL;
  if (old > 0 && get_num_units_in_focus() == 0) {
    advance_unit_focus();
  } else if (update) {
    update_unit_pix_label(get_units_in_focus());
    update_unit_info_label(get_units_in_focus());
  }

  pcity = tile_city(ptile);
  if (pcity) {
    if (can_player_see_units_in_city(client.conn.playing, pcity)) {
      pcity->client.occupied =
	(unit_list_size(pcity->tile->units) > 0);
    }

    refresh_city_dialog(pcity);
    freelog(LOG_DEBUG, "map city %s, %s, (%d %d)",
	    city_name(pcity),
	    nation_rule_name(nation_of_city(pcity)),
	    TILE_XY(pcity->tile));
  }

  /* FIXME: this can cause two refreshes to be done? */
  if (NULL != client.conn.playing) {
    pcity = player_find_city_by_id(client.conn.playing, hc);
    if (pcity) {
      refresh_city_dialog(pcity);
      freelog(LOG_DEBUG, "home city %s, %s, (%d %d)",
	      city_name(pcity),
	      nation_rule_name(nation_of_city(pcity)),
	      TILE_XY(pcity->tile));
    }
  }

  refresh_unit_mapcanvas(&old_unit, ptile, TRUE, FALSE);
}

/**************************************************************************
...
**************************************************************************/
void client_remove_city(struct city *pcity)
{
  bool effect_update;
  struct tile *ptile = city_tile(pcity);
  struct city old_city = *pcity;

  freelog(LOG_DEBUG, "client_remove_city() %d, %s",
	  pcity->id,
	  city_name(pcity));

  /* Explicitly remove all improvements, to properly remove any global effects
     and to handle the preservation of "destroyed" effects. */
  effect_update=FALSE;

  city_built_iterate(pcity, pimprove) {
    effect_update = TRUE;
    city_remove_improvement(pcity, pimprove);
  } city_built_iterate_end;

  if (effect_update) {
    /* nothing yet */
  }

  popdown_city_dialog(pcity);
  game_remove_city(pcity);
  city_report_dialog_update();
  refresh_city_mapcanvas(&old_city, ptile, TRUE, FALSE);
}

/**************************************************************************
Change all cities building X to building Y, if possible.  X and Y
could be improvements or units. X and Y are compound ids.
**************************************************************************/
void client_change_all(struct universal from,
		       struct universal to)
{
  int last_request_id = 0;

  if (!can_client_issue_orders()) {
    return;
  }

  create_event(NULL, E_CITY_PRODUCTION_CHANGED, FTC_CLIENT_INFO, NULL,
               _("Changing production of every %s into %s."),
               VUT_UTYPE == from.kind
               ? utype_name_translation(from.value.utype)
               : improvement_name_translation(from.value.building),
               VUT_UTYPE == to.kind
               ? utype_name_translation(to.value.utype)
               : improvement_name_translation(to.value.building));

  connection_do_buffer(&client.conn);
  city_list_iterate (client.conn.playing->cities, pcity) {
    if (are_universals_equal(&pcity->production, &from)
	&& can_city_build_now(pcity, to)) {
      last_request_id = city_change_production(pcity, to);
    }
  } city_list_iterate_end;

  connection_do_unbuffer(&client.conn);
  reports_freeze_till(last_request_id);
}

/***************************************************************************
  Return a string indicating one nation's embassy status with another
***************************************************************************/
const char *get_embassy_status(const struct player *me,
			       const struct player *them)
{
  if (!me || !them
      || me == them
      || !them->is_alive
      || !me->is_alive) {
    return "-";
  }
  if (player_has_embassy(me, them)) {
    if (player_has_embassy(them, me)) {
      return Q_("?embassy:Both");
    } else {
      return Q_("?embassy:Yes");
    }
  } else if (player_has_embassy(them, me)) {
    return Q_("?embassy:With Us");
  } else if (me->diplstates[player_index(them)].contact_turns_left > 0
             || them->diplstates[player_index(me)].contact_turns_left > 0) {
    return Q_("?embassy:Contact");
  } else {
    return Q_("?embassy:No Contact");
  }
}

/***************************************************************************
  Return a string indicating one nation's shaed vision status with another
***************************************************************************/
const char *get_vision_status(const struct player *me,
			      const struct player *them)
{
  if (me && them && gives_shared_vision(me, them)) {
    if (gives_shared_vision(them, me)) {
      return Q_("?vision:Both");
    } else {
      return Q_("?vision:To Them");
    }
  } else if (me && them && gives_shared_vision(them, me)) {
    return Q_("?vision:To Us");
  } else {
    return "";
  }
}

/**************************************************************************
Copy a string that describes the given clause into the return buffer.
**************************************************************************/
void client_diplomacy_clause_string(char *buf, int bufsiz,
				    struct Clause *pclause)
{
  struct city *pcity;

  switch(pclause->type) {
  case CLAUSE_ADVANCE:
    my_snprintf(buf, bufsiz, _("The %s give %s"),
		nation_plural_for_player(pclause->from),
		advance_name_for_player(client.conn.playing, pclause->value));
    break;
  case CLAUSE_CITY:
    pcity = game_find_city_by_number(pclause->value);
    if (pcity) {
      my_snprintf(buf, bufsiz, _("The %s give %s"),
                  nation_plural_for_player(pclause->from),
		  city_name(pcity));
    } else {
      my_snprintf(buf, bufsiz,_("The %s give unknown city."),
                  nation_plural_for_player(pclause->from));
    }
    break;
  case CLAUSE_GOLD:
    my_snprintf(buf, bufsiz, _("The %s give %d gold"),
		nation_plural_for_player(pclause->from),
		pclause->value);
    break;
  case CLAUSE_MAP:
    my_snprintf(buf, bufsiz, _("The %s give their worldmap"),
		nation_plural_for_player(pclause->from));
    break;
  case CLAUSE_SEAMAP:
    my_snprintf(buf, bufsiz, _("The %s give their seamap"),
		nation_plural_for_player(pclause->from));
    break;
  case CLAUSE_CEASEFIRE:
    my_snprintf(buf, bufsiz, _("The parties agree on a cease-fire"));
    break;
  case CLAUSE_PEACE:
    my_snprintf(buf, bufsiz, _("The parties agree on a peace"));
    break;
  case CLAUSE_ALLIANCE:
    my_snprintf(buf, bufsiz, _("The parties create an alliance"));
    break;
  case CLAUSE_VISION:
    my_snprintf(buf, bufsiz, _("The %s give shared vision"),
		nation_plural_for_player(pclause->from));
    break;
  case CLAUSE_EMBASSY:
    my_snprintf(buf, bufsiz, _("The %s give an embassy"),
                nation_plural_for_player(pclause->from));
    break;
  default:
    assert(FALSE);
    if (bufsiz > 0) {
      *buf = '\0';
    }
    break;
  }
}

/**************************************************************************
  Return the sprite for the research indicator.
**************************************************************************/
struct sprite *client_research_sprite(void)
{
  if (NULL != client.conn.playing && can_client_change_view()) {
    int index = 0;

    if (A_UNSET != get_player_research(client.conn.playing)->researching) {
      index = (NUM_TILES_PROGRESS
	       * get_player_research(client.conn.playing)->bulbs_researched)
	/ (total_bulbs_required(client.conn.playing) + 1);
    }

    /* This clipping can be necessary since we can end up with excess
     * research */
    index = CLIP(0, index, NUM_TILES_PROGRESS - 1);
    return get_indicator_sprite(tileset, INDICATOR_BULB, index);
  } else {
    return get_indicator_sprite(tileset, INDICATOR_BULB, 0);
  }
}

/**************************************************************************
  Return the sprite for the global-warming indicator.
**************************************************************************/
struct sprite *client_warming_sprite(void)
{
  if (NULL != client.conn.playing && can_client_change_view()) {
    int index;

    if ((game.info.globalwarming <= 0) &&
	(game.info.heating < (NUM_TILES_PROGRESS / 2))) {
      index = MAX(0, game.info.heating);
    } else {
      index = MIN(NUM_TILES_PROGRESS,
		  (MAX(0, 4 + game.info.globalwarming) / 5) +
		  ((NUM_TILES_PROGRESS / 2) - 1));
    }

    /* The clipping is needed because the above math is a little fuzzy. */
    index = CLIP(0, index, NUM_TILES_PROGRESS - 1);
    return get_indicator_sprite(tileset, INDICATOR_WARMING, index);
  } else {
    return get_indicator_sprite(tileset, INDICATOR_WARMING, 0);
  }
}

/**************************************************************************
  Return the sprite for the global-cooling indicator.
**************************************************************************/
struct sprite *client_cooling_sprite(void)
{
  if (can_client_change_view()) {
    int index;

    if ((game.info.nuclearwinter <= 0) &&
	(game.info.cooling < (NUM_TILES_PROGRESS / 2))) {
      index = MAX(0, game.info.cooling);
    } else {
      index = MIN(NUM_TILES_PROGRESS,
		  (MAX(0, 4 + game.info.nuclearwinter) / 5) +
		  ((NUM_TILES_PROGRESS / 2) - 1));
    }

    /* The clipping is needed because the above math is a little fuzzy. */
    index = CLIP(0, index, NUM_TILES_PROGRESS - 1);
    return get_indicator_sprite(tileset, INDICATOR_COOLING, index);
  } else {
    return get_indicator_sprite(tileset, INDICATOR_COOLING, 0);
  }
}

/**************************************************************************
  Return the sprite for the government indicator.
**************************************************************************/
struct sprite *client_government_sprite(void)
{
  if (NULL != client.conn.playing && can_client_change_view()
      && government_count() > 0) {
    struct government *gov = government_of_player(client.conn.playing);

    return get_government_sprite(tileset, gov);
  } else {
    /* HACK: the UNHAPPY citizen is used for the government
     * when we don't know any better. */
    return get_citizen_sprite(tileset, CITIZEN_UNHAPPY, 0, NULL);
  }
}

/**************************************************************************
Find something sensible to display. This is used to overwrite the
intro gfx.
**************************************************************************/
void center_on_something(void)
{
  struct city *pcity;
  struct unit *punit;

  if (!can_client_change_view()) {
    return;
  }

  can_slide = FALSE;
  if (get_num_units_in_focus() > 0) {
    center_tile_mapcanvas(head_of_units_in_focus()->tile);
  } else if (NULL != client.conn.playing
             && NULL != (pcity = find_palace(client.conn.playing))) {
    /* Else focus on the capital. */
    center_tile_mapcanvas(pcity->tile);
  } else if (NULL != client.conn.playing
             && 0 < city_list_size(client.conn.playing->cities)) {
    /* Just focus on any city. */
    pcity = city_list_get(client.conn.playing->cities, 0);
    assert(pcity != NULL);
    center_tile_mapcanvas(pcity->tile);
  } else if (NULL != client.conn.playing
             && 0 < unit_list_size(client.conn.playing->units)) {
    /* Just focus on any unit. */
    punit = unit_list_get(client.conn.playing->units, 0);
    assert(punit != NULL);
    center_tile_mapcanvas(punit->tile);
  } else {
    struct tile *ctile = native_pos_to_tile(map.xsize / 2, map.ysize / 2);

    /* Just any known tile will do; search near the middle first. */
    /* Iterate outward from the center tile.  We have to give a radius that
     * is guaranteed to be larger than the map will be.  Although this is
     * a misuse of map.xsize and map.ysize (which are native dimensions),
     * it should give a sufficiently large radius. */
    iterate_outward(ctile, map.xsize + map.ysize, ptile) {
      if (client_tile_get_known(ptile) != TILE_UNKNOWN) {
	ctile = ptile;
	break;
      }
    } iterate_outward_end;

    center_tile_mapcanvas(ctile);
  }
  can_slide = TRUE;
}

/****************************************************************************
  Encode a CID for the target production.
****************************************************************************/
cid cid_encode(struct universal target)
{
  return VUT_UTYPE == target.kind
         ? B_LAST + utype_number(target.value.utype)
         : improvement_number(target.value.building);
}

/****************************************************************************
  Encode a CID for the target unit type.
****************************************************************************/
cid cid_encode_unit(struct unit_type *punittype)
{
  struct universal target = {
    .kind = VUT_UTYPE,
    .value = {.utype = punittype}};

  return cid_encode(target);
}

/****************************************************************************
  Encode a CID for the target building.
****************************************************************************/
cid cid_encode_building(struct impr_type *pimprove)
{
  struct universal target = {
    .kind = VUT_IMPROVEMENT,
    .value = {.building = pimprove}
  };

  return cid_encode(target);
}

/****************************************************************************
  Encode a CID for the target city's production.
****************************************************************************/
cid cid_encode_from_city(const struct city *pcity)
{
  return cid_encode(pcity->production);
}

/**************************************************************************
  Decode the CID into a city_production structure.
**************************************************************************/
struct universal cid_decode(cid cid)
{
  struct universal target;

  if (cid >= B_LAST) {
    target.kind = VUT_UTYPE;
    target.value.utype = utype_by_number(cid - B_LAST);
  } else {
    target.kind = VUT_IMPROVEMENT;
    target.value.building = improvement_by_number(cid);
  }

  return target;
}

/****************************************************************************
  Return TRUE if the city supports at least one unit of the given
  production type (returns FALSE if the production is a building).
****************************************************************************/
bool city_unit_supported(const struct city *pcity,
			 struct universal target)
{
  if (VUT_UTYPE == target.kind) {
    struct unit_type *tvtype = target.value.utype;

    unit_list_iterate(pcity->units_supported, punit) {
      if (unit_type(punit) == tvtype)
	return TRUE;
    } unit_list_iterate_end;
  }
  return FALSE;
}

/****************************************************************************
  Return TRUE if the city has present at least one unit of the given
  production type (returns FALSE if the production is a building).
****************************************************************************/
bool city_unit_present(const struct city *pcity,
		       struct universal target)
{
  if (VUT_UTYPE == target.kind) {
    struct unit_type *tvtype = target.value.utype;

    unit_list_iterate(pcity->tile->units, punit) {
      if (unit_type(punit) == tvtype)
	return TRUE;
    }
    unit_list_iterate_end;
  }
  return FALSE;
}

/****************************************************************************
  A TestCityFunc to tell whether the item is a building and is present.
****************************************************************************/
bool city_building_present(const struct city *pcity,
			   struct universal target)
{
  return VUT_IMPROVEMENT == target.kind
      && city_has_building(pcity, target.value.building);
}

/**************************************************************************
  Return the numerical "section" of an item.  This is used for sorting.
**************************************************************************/
static int target_get_section(struct universal target)
{
  if (VUT_UTYPE == target.kind) {
    if (utype_has_flag(target.value.utype, F_CIVILIAN)) {
      return 2;
    } else {
      return 3;
    }
  } else {
    if (improvement_has_flag(target.value.building, IF_GOLD)) {
      return 1;
    } else if (is_small_wonder(target.value.building)) {
      return 4;
    } else if (is_great_wonder(target.value.building)) {
      return 5;
    } else {
      return 0;
    }
  }
}

/**************************************************************************
 Helper for name_and_sort_items.
**************************************************************************/
static int my_cmp(const void *p1, const void *p2)
{
  const struct item *i1 = p1, *i2 = p2;
  int s1 = target_get_section(i1->item);
  int s2 = target_get_section(i2->item);

  if (s1 == s2) {
    return mystrcasecmp(i1->descr, i2->descr);
  }
  return s1 - s2;
}

/**************************************************************************
 Takes an array of compound ids (cids). It will fill out an array of
 struct items and also sort it.

 section 0: normal buildings
 section 1: Capitalization
 section 2: F_CIVILIAN units
 section 3: other units
 section 4: small wonders
 section 5: great wonders
**************************************************************************/
void name_and_sort_items(struct universal *targets, int num_targets,
			 struct item *items,
			 bool show_cost, struct city *pcity)
{
  int i;

  for (i = 0; i < num_targets; i++) {
    struct universal target = targets[i];
    int cost;
    struct item *pitem = &items[i];
    const char *name;

    pitem->item = target;

    if (VUT_UTYPE == target.kind) {
      name = utype_values_translation(target.value.utype);
      cost = utype_build_shield_cost(target.value.utype);
    } else {
      name = city_improvement_name_translation(pcity, target.value.building);
      if (improvement_has_flag(target.value.building, IF_GOLD)) {
	cost = -1;
      } else {
	cost = impr_build_shield_cost(target.value.building);
      }
    }

    if (show_cost) {
      if (cost < 0) {
	my_snprintf(pitem->descr, sizeof(pitem->descr), "%s (XX)", name);
      } else {
	my_snprintf(pitem->descr, sizeof(pitem->descr), "%s (%d)", name, cost);
      }
    } else {
      (void) mystrlcpy(pitem->descr, name, sizeof(pitem->descr));
    }
  }

  qsort(items, num_targets, sizeof(struct item), my_cmp);
}

/**************************************************************************
  Return possible production targets for the current player's cities.

  FIXME: this should probably take a pplayer argument.
**************************************************************************/
int collect_production_targets(struct universal *targets,
			       struct city **selected_cities,
			       int num_selected_cities, bool append_units,
			       bool append_wonders, bool change_prod,
			       TestCityFunc test_func)
{
  cid first = append_units ? B_LAST : 0;
  cid last = (append_units
	      ? utype_count() + B_LAST
	      : improvement_count());
  cid cid;
  int items_used = 0;

  if (NULL == client.conn.playing) {
    return 0;
  }

  for (cid = first; cid < last; cid++) {
    bool append = FALSE;
    struct universal target = cid_decode(cid);

    if (!append_units && (append_wonders != is_wonder(target.value.building))) {
      continue;
    }

    if (!change_prod) {
      city_list_iterate(client.conn.playing->cities, pcity) {
	append |= test_func(pcity, cid_decode(cid));
      }
      city_list_iterate_end;
    } else {
      int i;

      for (i = 0; i < num_selected_cities; i++) {
	append |= test_func(selected_cities[i], cid_decode(cid));
      }
    }

    if (!append)
      continue;

    targets[items_used] = target;
    items_used++;
  }
  return items_used;
}

/**************************************************************************
 Collect the cids of all targets (improvements and units) which are
 currently built in a city.

  FIXME: this should probably take a pplayer argument.
**************************************************************************/
int collect_currently_building_targets(struct universal *targets)
{
  bool mapping[MAX_NUM_PRODUCTION_TARGETS];
  int cids_used = 0;
  cid cid;

  if (NULL == client.conn.playing) {
    return 0;
  }

  memset(mapping, 0, sizeof(mapping));
  city_list_iterate(client.conn.playing->cities, pcity) {
    mapping[cid_encode_from_city(pcity)] = TRUE;
  }
  city_list_iterate_end;

  for (cid = 0; cid < ARRAY_SIZE(mapping); cid++) {
    if (mapping[cid]) {
      targets[cids_used] = cid_decode(cid);
      cids_used++;
    }
  }
  return cids_used;
}

/**************************************************************************
 Collect the cids of all targets (improvements and units) which can
 be build in a city.

  FIXME: this should probably take a pplayer argument.
**************************************************************************/
int collect_buildable_targets(struct universal *targets)
{
  int cids_used = 0;

  if (NULL == client.conn.playing) {
    return 0;
  }

  improvement_iterate(pimprove) {
    if (can_player_build_improvement_now(client.conn.playing, pimprove)) {
      targets[cids_used].kind = VUT_IMPROVEMENT;
      targets[cids_used].value.building = pimprove;
      cids_used++;
    }
  } improvement_iterate_end;

  unit_type_iterate(punittype) {
    if (can_player_build_unit_now(client.conn.playing, punittype)) {
      targets[cids_used].kind = VUT_UTYPE;
      targets[cids_used].value.utype = punittype;
      cids_used++;
    }
  } unit_type_iterate_end

  return cids_used;
}

/**************************************************************************
 Collect the cids of all targets which can be build by this city or
 in general.

  FIXME: this should probably take a pplayer argument.
**************************************************************************/
int collect_eventually_buildable_targets(struct universal *targets,
					 struct city *pcity,
					 bool advanced_tech)
{
  int cids_used = 0;

  if (NULL == client.conn.playing) {
    return 0;
  }

  improvement_iterate(pimprove) {
    bool can_build = can_player_build_improvement_now(client.conn.playing, pimprove);
    bool can_eventually_build =
	can_player_build_improvement_later(client.conn.playing, pimprove);

    /* If there's a city, can the city build the improvement? */
    if (pcity) {
      can_build = can_build && can_city_build_improvement_now(pcity, pimprove);
      can_eventually_build = can_eventually_build &&
	  can_city_build_improvement_later(pcity, pimprove);
    }

    if ((advanced_tech && can_eventually_build) ||
	(!advanced_tech && can_build)) {
      targets[cids_used].kind = VUT_IMPROVEMENT;
      targets[cids_used].value.building = pimprove;
      cids_used++;
    }
  } improvement_iterate_end;

  unit_type_iterate(punittype) {
    bool can_build = can_player_build_unit_now(client.conn.playing, punittype);
    bool can_eventually_build =
	can_player_build_unit_later(client.conn.playing, punittype);

    /* If there's a city, can the city build the unit? */
    if (pcity) {
      can_build = can_build && can_city_build_unit_now(pcity, punittype);
      can_eventually_build = can_eventually_build &&
	  can_city_build_unit_later(pcity, punittype);
    }

    if ((advanced_tech && can_eventually_build) ||
	(!advanced_tech && can_build)) {
      targets[cids_used].kind = VUT_UTYPE;
      targets[cids_used].value.utype = punittype;
      cids_used++;
    }
  } unit_type_iterate_end;

  return cids_used;
}

/**************************************************************************
 Collect the cids of all improvements which are built in the given city.
**************************************************************************/
int collect_already_built_targets(struct universal *targets,
				  struct city *pcity)
{
  int cids_used = 0;

  assert(pcity != NULL);

  city_built_iterate(pcity, pimprove) {
    targets[cids_used].kind = VUT_IMPROVEMENT;
    targets[cids_used].value.building = pimprove;
    cids_used++;
  } city_built_iterate_end;

  return cids_used;
}

/**************************************************************************
...
**************************************************************************/
int num_supported_units_in_city(struct city *pcity)
{
  struct unit_list *plist;

  if (can_player_see_city_internals(client.conn.playing, pcity)) {
    /* Other players don't see inside the city (but observers do). */
    plist = pcity->info_units_supported;
  } else {
    plist = pcity->units_supported;
  }

  return unit_list_size(plist);
}

/**************************************************************************
...
**************************************************************************/
int num_present_units_in_city(struct city *pcity)
{
  struct unit_list *plist;

  if (can_player_see_units_in_city(client.conn.playing, pcity)) {
    /* Other players don't see inside the city (but observers do). */
    plist = pcity->info_units_present;
  } else {
    plist = pcity->tile->units;
  }

  return unit_list_size(plist);
}

/**************************************************************************
  Handles a chat or event message.
**************************************************************************/
void handle_event(const char *featured_text, struct tile *ptile,
		  enum event_type event, int conn_id)
{
  char plain_text[MAX_LEN_MSG];
  struct text_tag_list *tags = text_tag_list_new();
  int where = MW_OUTPUT;	/* where to display the message */
  bool fallback_needed = FALSE; /* we want fallback if actual 'where' is not
                                 * usable */
  bool shown = FALSE;           /* Message displayed somewhere at least */

  if (event >= E_LAST)  {
    /* Server may have added a new event; leave as MW_OUTPUT */
    freelog(LOG_VERBOSE, "Unknown event type %d!", event);
  } else if (event >= 0)  {
    where = messages_where[event];
  }

  /* Get the original text. */
  featured_text_to_plain_text(featured_text, plain_text,
                              sizeof(plain_text), tags);

  /* Display link marks when an user is pointed us something. */
  if (conn_id != -1) {
    text_tag_list_iterate(tags, ptag) {
      if (text_tag_type(ptag) == TTT_LINK) {
        link_mark_add_new(text_tag_link_type(ptag), text_tag_link_id(ptag));
      }
    } text_tag_list_iterate_end;
  }

  /* Maybe highlight our player and user names if someone is talking
   * about us. */
  if (highlight_our_names[0] != '\0'
      && conn_id != -1 && conn_id != client.conn.id) {
    const char *username = client.conn.username;
    size_t userlen = strlen(username);
    const char *playername = ((client_player() && !client_is_observer())
                              ? player_name(client_player()) : NULL);
    size_t playerlen = playername ? strlen(playername) : 0;
    const char *p;

    if (playername && playername[0] == '\0') {
      playername = NULL;
    }

    if (username && username[0] == '\0') {
      username = NULL;
    }

    for (p = plain_text; *p != '\0'; p++) {
      if (username
          && 0 == mystrncasecmp(p, username, userlen)) {
        /* Appends to be sure it will be applied at last. */
        text_tag_list_append(tags, text_tag_new(TTT_COLOR, p - plain_text,
                                                p - plain_text + userlen,
                                                NULL, highlight_our_names));
      } else if (playername
                 && 0 == mystrncasecmp(p, playername, playerlen)) {
        /* Appends to be sure it will be applied at last. */
        text_tag_list_append(tags, text_tag_new(TTT_COLOR, p - plain_text,
                                                p - plain_text + playerlen,
                                                NULL, highlight_our_names));
      }
    }
  }

  /* Popup */
  if (BOOL_VAL(where & MW_POPUP)) {
    /* Popups are usually not shown if player is under AI control.
     * Server operator messages are shown always. */
    if (NULL == client.conn.playing
        || !client.conn.playing->ai_data.control
        || event == E_MESSAGE_WALL) {
      popup_notify_goto_dialog(_("Popup Request"), plain_text, tags, ptile);
      shown = TRUE;
    } else {
      /* Force to chatline so it will be visible somewhere at least.
       * Messages window may still handle this so chatline is not needed
       * after all. */
      fallback_needed = TRUE;
    }
  }

  /* Message window */
  if (BOOL_VAL(where & MW_MESSAGES)) {
    /* When the game isn't running, the messages dialog isn't present. */
    if (C_S_RUNNING <= client_state()) {
      add_notify_window(plain_text, tags, ptile, event);
      shown = TRUE;
    } else {
      /* Force to chatline instead. */
      fallback_needed = TRUE;
    }
  }

  /* Chatline */
  if (BOOL_VAL(where & MW_OUTPUT) || (fallback_needed && !shown)) {
    output_window_event(plain_text, tags, conn_id);
  }

  play_sound_for_event(event);

  /* Free tags */
  text_tag_list_clear_all(tags);
  text_tag_list_free(tags);
}

/**************************************************************************
  Creates a struct packet_generic_message packet and injects it via
  handle_chat_msg.
**************************************************************************/
void create_event(struct tile *ptile, enum event_type event,
                  const char *fg_color, const char *bg_color,
                  const char *format, ...)
{
  va_list ap;
  char message[MAX_LEN_MSG];

  va_start(ap, format);
  my_vsnprintf(message, sizeof(message), format, ap);
  va_end(ap);

  if ((fg_color && fg_color[0] != '\0')
      || (bg_color && bg_color[0] != '\0')) {
    char colored_text[MAX_LEN_MSG];

    featured_text_apply_tag(message, colored_text, sizeof(colored_text),
                            TTT_COLOR, 0, OFFSET_UNSET, fg_color, bg_color);
    handle_event(colored_text, ptile, event, -1);
  } else {
    handle_event(message, ptile, event, -1);
  }
}

/**************************************************************************
  Writes the supplied string into the file civgame.log.
**************************************************************************/
void write_chatline_content(const char *txt)
{
  FILE *fp = fopen("civgame.log", "w");	/* should allow choice of name? */

  output_window_append(FTC_CLIENT_INFO, NULL,
                       _("Exporting output window to civgame.log ..."));
  if (fp) {
    fputs(txt, fp);
    fclose(fp);
    output_window_append(FTC_CLIENT_INFO, NULL, _("Export complete."));
  } else {
    output_window_append(FTC_CLIENT_INFO, NULL,
                         _("Export failed, couldn't write to file."));
  }
}

/**************************************************************************
  Freeze all reports and other GUI elements.
**************************************************************************/
void reports_freeze(void)
{
  freelog(LOG_DEBUG, "reports_freeze");

  meswin_freeze();
  plrdlg_freeze();
  report_dialogs_freeze();
  output_window_freeze();
}

/**************************************************************************
  Freeze all reports and other GUI elements until the given request
  was executed.
**************************************************************************/
void reports_freeze_till(int request_id)
{
  if (request_id != 0) {
    reports_freeze();
    set_reports_thaw_request(request_id);
  }
}

/**************************************************************************
  Thaw all reports and other GUI elements.
**************************************************************************/
void reports_thaw(void)
{
  freelog(LOG_DEBUG, "reports_thaw");

  meswin_thaw();
  plrdlg_thaw();
  report_dialogs_thaw();
  output_window_thaw();
}

/**************************************************************************
  Thaw all reports and other GUI elements unconditionally.
**************************************************************************/
void reports_force_thaw(void)
{
  meswin_force_thaw();
  plrdlg_force_thaw();
  report_dialogs_force_thaw();
  output_window_force_thaw();
}

/**************************************************************************
  Find city nearest to given unit and optionally return squared city
  distance Parameter sq_dist may be NULL. Returns NULL only if no city is
  known. Favors punit owner's cities over other cities if equally distant.
**************************************************************************/
struct city *get_nearest_city(const struct unit *punit, int *sq_dist)
{
  struct city *pcity_near;
  int pcity_near_dist;

  if ((pcity_near = tile_city(punit->tile))) {
    pcity_near_dist = 0;
  } else {
    pcity_near = NULL;
    pcity_near_dist = -1;
    players_iterate(pplayer) {
      city_list_iterate(pplayer->cities, pcity_current) {
        int dist = sq_map_distance(pcity_current->tile, punit->tile);
        if (pcity_near_dist == -1 || dist < pcity_near_dist
	    || (dist == pcity_near_dist
		&& unit_owner(punit) == city_owner(pcity_current))) {
          pcity_near = pcity_current;
          pcity_near_dist = dist;
        }
      } city_list_iterate_end;
    } players_iterate_end;
  }

  if (sq_dist) {
    *sq_dist = pcity_near_dist;
  }

  return pcity_near;
}

/**************************************************************************
  Called when the "Buy" button is pressed in the city report for every
  selected city. Checks for coinage and sufficient funds or request the
  purchase if everything is ok.
**************************************************************************/
void cityrep_buy(struct city *pcity)
{
  int value;

  if (city_production_has_flag(pcity, IF_GOLD)) {
    create_event(pcity->tile, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                 _("You don't buy %s in %s!"),
                 improvement_name_translation(pcity->production.value.building),
                 city_link(pcity));
    return;
  }
  value = city_production_buy_gold_cost(pcity);

  if (city_owner(pcity)->economic.gold >= value) {
    city_buy_production(pcity);
  } else {
    create_event(NULL, E_BAD_COMMAND, FTC_CLIENT_INFO, NULL,
                 _("%s costs %d gold and you only have %d gold."),
                 city_production_name_translation(pcity),
                 value,
                 city_owner(pcity)->economic.gold);
  }
}

void common_taxrates_callback(int i)
{
  int tax_end, lux_end, sci_end, tax, lux, sci;
  int delta = 10;

  if (!can_client_issue_orders()) {
    return;
  }

  lux_end = client.conn.playing->economic.luxury;
  sci_end = lux_end + client.conn.playing->economic.science;
  tax_end = 100;

  lux = client.conn.playing->economic.luxury;
  sci = client.conn.playing->economic.science;
  tax = client.conn.playing->economic.tax;

  i *= 10;
  if (i < lux_end) {
    lux -= delta;
    sci += delta;
  } else if (i < sci_end) {
    sci -= delta;
    tax += delta;
  } else {
    tax -= delta;
    lux += delta;
  }
  dsend_packet_player_rates(&client.conn, tax, lux, sci);
}

/****************************************************************************
  Returns TRUE if any of the units can do the connect activity.
****************************************************************************/
bool can_units_do_connect(struct unit_list *punits,
			  enum unit_activity activity)
{
  unit_list_iterate(punits, punit) {
    if (can_unit_do_connect(punit, activity)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/****************************************************************************
  Determines which color type should be used for unit background.
  This is only guesswork based on unit properties. One should not
  take UNIT_BG_FLYING seriously meaning that unit can fly - custom
  ruleset might have units with similar properties but explains these
  properties by some other means than by flying. 
****************************************************************************/
enum unit_bg_color_type unit_color_type(const struct unit_type *punittype)
{
  struct unit_class *pclass = utype_class(punittype);

  if (pclass->hp_loss_pct > 0) {
    return UNIT_BG_HP_LOSS;
  }

  if (pclass->move_type == LAND_MOVING) {
    return UNIT_BG_LAND;
  }
  if (pclass->move_type == SEA_MOVING) {
    return UNIT_BG_SEA;
  }

  assert(pclass->move_type == BOTH_MOVING);

  if (uclass_has_flag(pclass, UCF_TERRAIN_SPEED)) {
    /* Unit moves on both sea and land by speed determined by terrain */
    return UNIT_BG_AMPHIBIOUS;
  }

  return UNIT_BG_FLYING;
}

/****************************************************************************
  Comparison function used by qsort in buy_production_in_selected_cities().
****************************************************************************/
static int city_buy_cost_compare(const void *a, const void *b)
{
  const struct city *ca, *cb;
  ca = *((const struct city **) a);
  cb = *((const struct city **) b);
  return (city_production_buy_gold_cost(ca)
          - city_production_buy_gold_cost(cb));
}

/****************************************************************************
  For each selected city, buy the current production. The selected cities
  are sorted so production is bought in the cities with lowest cost first.
****************************************************************************/
void buy_production_in_selected_cities(void)
{
  const struct player *pplayer = client_player();
  if (!pplayer || !pplayer->cities
      || city_list_size(pplayer->cities) < 1) {
    return;
  }

  int gold = pplayer->economic.gold;
  if (gold < 1) {
    return;
  }

  const int n = city_list_size(pplayer->cities);
  struct city *cities[n];
  int i, count = 0;

  city_list_iterate(pplayer->cities, pcity) {
    if (!is_city_hilited(pcity) || !city_can_buy(pcity)) {
      continue;
    }
    cities[count++] = pcity;
  } city_list_iterate_end;

  if (count < 1) {
    return;
  }

  qsort(cities, count, sizeof(*cities), city_buy_cost_compare);

  struct connection *pconn = &client.conn;
  connection_do_buffer(pconn);

  for (i = 0; i < count && gold > 0; i++) {
    gold -= city_production_buy_gold_cost(cities[i]);
    city_buy_production(cities[i]);
  }

  connection_do_unbuffer(pconn);
}
