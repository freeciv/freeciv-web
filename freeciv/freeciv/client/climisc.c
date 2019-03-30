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

/*********************************************************************** 
  This module contains various general - mostly highlevel - functions
  used throughout the client.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "bitvector.h"
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
#include "mapimg.h"
#include "packets.h"
#include "research.h"
#include "spaceship.h"
#include "unitlist.h"

/* client/include */
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
#include "options.h"
#include "packhand.h"
#include "repodlgs_common.h"
#include "tilespec.h"


/**********************************************************************//**
  Remove unit, client end version
**************************************************************************/
void client_remove_unit(struct unit *punit)
{
  struct city *pcity;
  struct tile *ptile = unit_tile(punit);
  int hc = punit->homecity;
  struct unit old_unit = *punit;
  int old = get_num_units_in_focus();
  bool update;

  log_debug("removing unit %d, %s %s (%d %d) hcity %d",
            punit->id, nation_rule_name(nation_of_unit(punit)),
            unit_rule_name(punit), TILE_XY(unit_tile(punit)), hc);

  update = (get_focus_unit_on_tile(unit_tile(punit)) != NULL);

  /* Check transport status. */
  unit_transport_unload(punit);
  if (get_transporter_occupancy(punit) > 0) {
    unit_list_iterate(unit_transport_cargo(punit), pcargo) {
      /* The server should take care that the unit is on the right terrain. */
      unit_transport_unload(pcargo);
    } unit_list_iterate_end;
  }

  control_unit_killed(punit);
  game_remove_unit(&wld, punit);
  punit = NULL;
  if (old > 0 && get_num_units_in_focus() == 0) {
    unit_focus_advance();
  } else if (update) {
    update_unit_pix_label(get_units_in_focus());
    update_unit_info_label(get_units_in_focus());
  }

  pcity = tile_city(ptile);
  if (NULL != pcity) {
    if (can_player_see_units_in_city(client_player(), pcity)) {
      pcity->client.occupied = (0 < unit_list_size(pcity->tile->units));
      refresh_city_dialog(pcity);
    }

    log_debug("map city %s, %s, (%d %d)",
              city_name_get(pcity), nation_rule_name(nation_of_city(pcity)),
              TILE_XY(city_tile(pcity)));
  }

  if (!client_has_player() || unit_owner(&old_unit) == client_player()) {
    pcity = game_city_by_number(hc);
    if (NULL != pcity) {
      refresh_city_dialog(pcity);
      log_debug("home city %s, %s, (%d %d)",
                city_name_get(pcity), nation_rule_name(nation_of_city(pcity)),
                TILE_XY(city_tile(pcity)));
    }
  }

  refresh_unit_mapcanvas(&old_unit, ptile, TRUE, FALSE);
}

/**********************************************************************//**
  Remove city, client end version.
**************************************************************************/
void client_remove_city(struct city *pcity)
{
  bool effect_update;
  struct tile *ptile = city_tile(pcity);
  struct city old_city = *pcity;

  log_debug("client_remove_city() %d, %s", pcity->id, city_name_get(pcity));

  /* Explicitly remove all improvements, to properly remove any global effects
     and to handle the preservation of "destroyed" effects. */
  effect_update = FALSE;

  city_built_iterate(pcity, pimprove) {
    effect_update = TRUE;
    city_remove_improvement(pcity, pimprove);
  } city_built_iterate_end;

  if (effect_update) {
    /* nothing yet */
  }

  popdown_city_dialog(pcity);
  game_remove_city(&wld, pcity);
  city_report_dialog_update();
  refresh_city_mapcanvas(&old_city, ptile, TRUE, FALSE);
}

/**********************************************************************//**
  Change all cities building X to building Y, if possible.  X and Y
  could be improvements or units. X and Y are compound ids.
**************************************************************************/
void client_change_all(struct universal *from, struct universal *to)
{
  if (!can_client_issue_orders()) {
    return;
  }

  create_event(NULL, E_CITY_PRODUCTION_CHANGED, ftc_client,
               _("Changing production of every %s into %s."),
               VUT_UTYPE == from->kind
               ? utype_name_translation(from->value.utype)
               : improvement_name_translation(from->value.building),
               VUT_UTYPE == to->kind
               ? utype_name_translation(to->value.utype)
               : improvement_name_translation(to->value.building));

  connection_do_buffer(&client.conn);
  city_list_iterate (client.conn.playing->cities, pcity) {
    if (are_universals_equal(&pcity->production, from)
        && can_city_build_now(pcity, to)) {
      city_change_production(pcity, to);
    }
  } city_list_iterate_end;

  connection_do_unbuffer(&client.conn);
}

/**********************************************************************//**
  Return a string indicating one nation's embassy status with another
**************************************************************************/
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
  } else if (player_diplstate_get(me, them)->contact_turns_left > 0
             || player_diplstate_get(them, me)->contact_turns_left > 0) {
    return Q_("?embassy:Contact");
  } else {
    return Q_("?embassy:No Contact");
  }
}

/**********************************************************************//**
  Return a string indicating one nation's shaed vision status with another
**************************************************************************/
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

/**********************************************************************//**
  Copy a string that describes the given clause into the return buffer.
**************************************************************************/
void client_diplomacy_clause_string(char *buf, int bufsiz,
				    struct Clause *pclause)
{
  struct city *pcity;

  switch(pclause->type) {
  case CLAUSE_ADVANCE:
    fc_snprintf(buf, bufsiz, _("The %s give %s"),
                nation_plural_for_player(pclause->from),
                advance_name_translation(advance_by_number(pclause->value)));
    break;
  case CLAUSE_CITY:
    pcity = game_city_by_number(pclause->value);
    if (pcity) {
      fc_snprintf(buf, bufsiz, _("The %s give %s"),
                  nation_plural_for_player(pclause->from),
                  city_name_get(pcity));
    } else {
      fc_snprintf(buf, bufsiz,_("The %s give an unknown city"),
                  nation_plural_for_player(pclause->from));
    }
    break;
  case CLAUSE_GOLD:
    fc_snprintf(buf, bufsiz, PL_("The %s give %d gold",
                                 "The %s give %d gold", pclause->value),
                nation_plural_for_player(pclause->from),
                pclause->value);
    break;
  case CLAUSE_MAP:
    fc_snprintf(buf, bufsiz, _("The %s give their worldmap"),
                nation_plural_for_player(pclause->from));
    break;
  case CLAUSE_SEAMAP:
    fc_snprintf(buf, bufsiz, _("The %s give their seamap"),
                nation_plural_for_player(pclause->from));
    break;
  case CLAUSE_CEASEFIRE:
    fc_snprintf(buf, bufsiz, _("The parties agree on a cease-fire"));
    break;
  case CLAUSE_PEACE:
    fc_snprintf(buf, bufsiz, _("The parties agree on a peace"));
    break;
  case CLAUSE_ALLIANCE:
    fc_snprintf(buf, bufsiz, _("The parties create an alliance"));
    break;
  case CLAUSE_VISION:
    fc_snprintf(buf, bufsiz, _("The %s give shared vision"),
                nation_plural_for_player(pclause->from));
    break;
  case CLAUSE_EMBASSY:
    fc_snprintf(buf, bufsiz, _("The %s give an embassy"),
                nation_plural_for_player(pclause->from));
    break;
  default:
    fc_assert(FALSE);
    if (bufsiz > 0) {
      *buf = '\0';
    }
    break;
  }
}

/**********************************************************************//**
  Return global catastrophe chance and rate of change, scaled to some
  maximum (e.g. 100 gives percentages).
  This mirrors the logic in update_environmental_upset().
**************************************************************************/
static void catastrophe_scaled(int *chance, int *rate, int max,
                               int current, int accum, int level)
{
  /* 20 from factor in update_environmental_upset() */
  int numer = 20 * max;
  int denom = map_num_tiles();

  if (chance) {
    *chance = CLIP(0,
                   (int)((((long)accum * numer) + (denom - 1)) / denom),
                   max);
  }
  if (rate) {
    *rate = DIVIDE(((long)(current - level) * numer) + (denom - 1), denom);
  }
}

/**********************************************************************//**
  Return global warming chance and rate of change, scaled to max.
**************************************************************************/
void global_warming_scaled(int *chance, int *rate, int max)
{
  return catastrophe_scaled(chance, rate, max,
                            game.info.heating, game.info.globalwarming,
                            game.info.warminglevel);
}

/**********************************************************************//**
  Return nuclear winter chance and rate of change, scaled to max.
**************************************************************************/
void nuclear_winter_scaled(int *chance, int *rate, int max)
{
  return catastrophe_scaled(chance, rate, max,
                            game.info.cooling, game.info.nuclearwinter,
                            game.info.coolinglevel);
}

/**********************************************************************//**
  Return the sprite for the research indicator.
**************************************************************************/
struct sprite *client_research_sprite(void)
{
  if (NULL != client.conn.playing && can_client_change_view()) {
    const struct research *presearch = research_get(client_player());
    int idx = 0;

    if (A_UNSET != presearch->researching) {
      idx = (NUM_TILES_PROGRESS * presearch->bulbs_researched
             / (presearch->client.researching_cost + 1));
    }

    /* This clipping can be necessary since we can end up with excess
     * research */
    idx = CLIP(0, idx, NUM_TILES_PROGRESS - 1);
    return get_indicator_sprite(tileset, INDICATOR_BULB, idx);
  } else {
    return get_indicator_sprite(tileset, INDICATOR_BULB, 0);
  }
}

/**********************************************************************//**
  Return the sprite for the global-warming indicator.
**************************************************************************/
struct sprite *client_warming_sprite(void)
{
  int idx;

  if (can_client_change_view()) {
    /* Highest sprite kicks in at about 25% risk */
    global_warming_scaled(&idx, NULL, (NUM_TILES_PROGRESS-1)*4);
    idx = CLIP(0, idx, NUM_TILES_PROGRESS-1);
  } else {
    idx = 0;
  }
  return get_indicator_sprite(tileset, INDICATOR_WARMING, idx);
}

/**********************************************************************//**
  Return the sprite for the global-cooling indicator.
**************************************************************************/
struct sprite *client_cooling_sprite(void)
{
  int idx;

  if (can_client_change_view()) {
    /* Highest sprite kicks in at about 25% risk */
    nuclear_winter_scaled(&idx, NULL, (NUM_TILES_PROGRESS-1)*4);
    idx = CLIP(0, idx, NUM_TILES_PROGRESS-1);
  } else {
    idx = 0;
  }
  return get_indicator_sprite(tileset, INDICATOR_COOLING, idx);
}

/**********************************************************************//**
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

/**********************************************************************//**
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
    center_tile_mapcanvas(unit_tile(head_of_units_in_focus()));
  } else if (client_has_player()
             && NULL != (pcity = player_capital(client_player()))) {
    /* Else focus on the capital. */
    center_tile_mapcanvas(pcity->tile);
  } else if (NULL != client.conn.playing
             && 0 < city_list_size(client.conn.playing->cities)) {
    /* Just focus on any city. */
    pcity = city_list_get(client.conn.playing->cities, 0);
    fc_assert_ret(pcity != NULL);
    center_tile_mapcanvas(pcity->tile);
  } else if (NULL != client.conn.playing
             && 0 < unit_list_size(client.conn.playing->units)) {
    /* Just focus on any unit. */
    punit = unit_list_get(client.conn.playing->units, 0);
    fc_assert_ret(punit != NULL);
    center_tile_mapcanvas(unit_tile(punit));
  } else {
    struct tile *ctile = native_pos_to_tile(&(wld.map), wld.map.xsize / 2, wld.map.ysize / 2);

    /* Just any known tile will do; search near the middle first. */
    /* Iterate outward from the center tile.  We have to give a radius that
     * is guaranteed to be larger than the map will be.  Although this is
     * a misuse of map.xsize and map.ysize (which are native dimensions),
     * it should give a sufficiently large radius. */
    iterate_outward(&(wld.map), ctile, wld.map.xsize + wld.map.ysize, ptile) {
      if (client_tile_get_known(ptile) != TILE_UNKNOWN) {
	ctile = ptile;
	break;
      }
    } iterate_outward_end;

    center_tile_mapcanvas(ctile);
  }
  can_slide = TRUE;
}

/**********************************************************************//**
  Encode a CID for the target production.
**************************************************************************/
cid cid_encode(struct universal target)
{
  return VUT_UTYPE == target.kind
         ? B_LAST + utype_number(target.value.utype)
         : improvement_number(target.value.building);
}

/**********************************************************************//**
  Encode a CID for the target unit type.
**************************************************************************/
cid cid_encode_unit(struct unit_type *punittype)
{
  struct universal target = {
    .kind = VUT_UTYPE,
    .value = {.utype = punittype}};

  return cid_encode(target);
}

/**********************************************************************//**
  Encode a CID for the target building.
**************************************************************************/
cid cid_encode_building(struct impr_type *pimprove)
{
  struct universal target = {
    .kind = VUT_IMPROVEMENT,
    .value = {.building = pimprove}
  };

  return cid_encode(target);
}

/**********************************************************************//**
  Encode a CID for the target city's production.
**************************************************************************/
cid cid_encode_from_city(const struct city *pcity)
{
  return cid_encode(pcity->production);
}

/**********************************************************************//**
  Decode the CID into a city_production structure.
**************************************************************************/
struct universal cid_decode(cid id)
{
  struct universal target;

  if (id >= B_LAST) {
    target.kind = VUT_UTYPE;
    target.value.utype = utype_by_number(id - B_LAST);
  } else {
    target.kind = VUT_IMPROVEMENT;
    target.value.building = improvement_by_number(id);
  }

  return target;
}

/**********************************************************************//**
  Return TRUE if the city supports at least one unit of the given
  production type (returns FALSE if the production is a building).
**************************************************************************/
bool city_unit_supported(const struct city *pcity,
                         const struct universal *target)
{
  if (VUT_UTYPE == target->kind) {
    struct unit_type *tvtype = target->value.utype;

    unit_list_iterate(pcity->units_supported, punit) {
      if (unit_type_get(punit) == tvtype) {
        return TRUE;
      }
    } unit_list_iterate_end;
  }
  return FALSE;
}

/**********************************************************************//**
  Return TRUE if the city has present at least one unit of the given
  production type (returns FALSE if the production is a building).
**************************************************************************/
bool city_unit_present(const struct city *pcity,
                       const struct universal *target)
{
  if (VUT_UTYPE == target->kind) {
    struct unit_type *tvtype = target->value.utype;

    unit_list_iterate(pcity->tile->units, punit) {
      if (unit_type_get(punit) == tvtype) {
        return TRUE;
      }
    }
    unit_list_iterate_end;
  }
  return FALSE;
}

/**********************************************************************//**
  A TestCityFunc to tell whether the item is a building and is present.
**************************************************************************/
bool city_building_present(const struct city *pcity,
                           const struct universal *target)
{
  return VUT_IMPROVEMENT == target->kind
    && city_has_building(pcity, target->value.building);
}

/**********************************************************************//**
  Return the numerical "section" of an item.  This is used for sorting.
**************************************************************************/
static int target_get_section(struct universal target)
{
  if (VUT_UTYPE == target.kind) {
    if (utype_has_flag(target.value.utype, UTYF_CIVILIAN)) {
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

/**********************************************************************//**
  Helper for name_and_sort_items.
**************************************************************************/
static int fc_cmp(const void *p1, const void *p2)
{
  const struct item *i1 = p1, *i2 = p2;
  int s1 = target_get_section(i1->item);
  int s2 = target_get_section(i2->item);

  if (s1 == s2) {
    return fc_strcasecmp(i1->descr, i2->descr);
  }
  return s1 - s2;
}

/**********************************************************************//**
  Takes an array of compound ids (cids). It will fill out an array of
  struct items and also sort it.

  section 0: normal buildings
  section 1: Capitalization
  section 2: UTYF_CIVILIAN units
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
      cost = utype_build_shield_cost(pcity, target.value.utype);
    } else {
      name = city_improvement_name_translation(pcity, target.value.building);
      if (improvement_has_flag(target.value.building, IF_GOLD)) {
        cost = -1;
      } else {
        cost = impr_build_shield_cost(pcity, target.value.building);
      }
    }

    if (show_cost) {
      if (cost < 0) {
        fc_snprintf(pitem->descr, sizeof(pitem->descr), "%s (XX)", name);
      } else {
        fc_snprintf(pitem->descr, sizeof(pitem->descr),
                    "%s (%d)", name, cost);
      }
    } else {
      (void) fc_strlcpy(pitem->descr, name, sizeof(pitem->descr));
    }
  }

  qsort(items, num_targets, sizeof(struct item), fc_cmp);
}

/**********************************************************************//**
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
  cid id;
  int items_used = 0;

  for (id = first; id < last; id++) {
    bool append = FALSE;
    struct universal target = cid_decode(id);

    if (!append_units && (append_wonders != is_wonder(target.value.building))) {
      continue;
    }

    if (!change_prod) {
      if (client_has_player()) {
        city_list_iterate(client_player()->cities, pcity) {
          append |= test_func(pcity, &target);
        } city_list_iterate_end;
      } else {
        cities_iterate(pcity) {
          append |= test_func(pcity, &target);
        } cities_iterate_end;
      }
    } else {
      int i;

      for (i = 0; i < num_selected_cities; i++) {
        append |= test_func(selected_cities[i], &target);
      }
    }

    if (!append)
      continue;

    targets[items_used] = target;
    items_used++;
  }
  return items_used;
}

/**********************************************************************//**
  Collect the cids of all targets (improvements and units) which are
  currently built in a city.

  FIXME: this should probably take a pplayer argument.
**************************************************************************/
int collect_currently_building_targets(struct universal *targets)
{
  bool mapping[MAX_NUM_PRODUCTION_TARGETS];
  int cids_used = 0;
  cid id;

  if (NULL == client.conn.playing) {
    return 0;
  }

  memset(mapping, 0, sizeof(mapping));
  city_list_iterate(client.conn.playing->cities, pcity) {
    mapping[cid_encode_from_city(pcity)] = TRUE;
  }
  city_list_iterate_end;

  for (id = 0; id < ARRAY_SIZE(mapping); id++) {
    if (mapping[id]) {
      targets[cids_used] = cid_decode(id);
      cids_used++;
    }
  }

  return cids_used;
}

/**********************************************************************//**
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

/**********************************************************************//**
  Collect the cids of all targets which can be build by this city or
  in general.
**************************************************************************/
int collect_eventually_buildable_targets(struct universal *targets,
                                         struct city *pcity,
                                         bool advanced_tech)
{
  struct player *pplayer = client_player();
  int cids_used = 0;

  improvement_iterate(pimprove) {
    bool can_build;
    bool can_eventually_build;

    if (NULL != pcity) {
      /* Can the city build? */
      can_build = can_city_build_improvement_now(pcity, pimprove);
      can_eventually_build = can_city_build_improvement_later(pcity,
                                                              pimprove);
    } else if (NULL != pplayer) {
      /* Can our player build? */
      can_build = can_player_build_improvement_now(pplayer, pimprove);
      can_eventually_build = can_player_build_improvement_later(pplayer,
                                                                pimprove);
    } else {
      /* Global observer case: can any player build? */
      can_build = FALSE;
      players_iterate(aplayer) {
        if (can_player_build_improvement_now(aplayer, pimprove)) {
          can_build = TRUE;
          break;
        }
      } players_iterate_end;

      can_eventually_build = FALSE;
      players_iterate(aplayer) {
        if (can_player_build_improvement_later(aplayer, pimprove)) {
          can_eventually_build = TRUE;
          break;
        }
      } players_iterate_end;
    }

    if ((advanced_tech && can_eventually_build)
        || (!advanced_tech && can_build)) {
      targets[cids_used].kind = VUT_IMPROVEMENT;
      targets[cids_used].value.building = pimprove;
      cids_used++;
    }
  } improvement_iterate_end;

  unit_type_iterate(punittype) {
    bool can_build;
    bool can_eventually_build;

    if (NULL != pcity) {
      /* Can the city build? */
      can_build = can_city_build_unit_now(pcity, punittype);
      can_eventually_build = can_city_build_unit_later(pcity, punittype);
    } else if (NULL != pplayer) {
      /* Can our player build? */
      can_build = can_player_build_unit_now(pplayer, punittype);
      can_eventually_build = can_player_build_unit_later(pplayer, punittype);
    } else {
      /* Global observer case: can any player build? */
      can_build = FALSE;
      players_iterate(aplayer) {
        if (can_player_build_unit_now(aplayer, punittype)) {
          can_build = TRUE;
          break;
        }
      } players_iterate_end;

      can_eventually_build = FALSE;
      players_iterate(aplayer) {
        if (can_player_build_unit_later(aplayer, punittype)) {
          can_eventually_build = TRUE;
          break;
        }
      } players_iterate_end;
    }

    if ((advanced_tech && can_eventually_build)
        || (!advanced_tech && can_build)) {
      targets[cids_used].kind = VUT_UTYPE;
      targets[cids_used].value.utype = punittype;
      cids_used++;
    }
  } unit_type_iterate_end;

  return cids_used;
}

/**********************************************************************//**
  Collect the cids of all improvements which are built in the given city.
**************************************************************************/
int collect_already_built_targets(struct universal *targets,
				  struct city *pcity)
{
  int cids_used = 0;

  fc_assert_ret_val(pcity != NULL, 0);

  city_built_iterate(pcity, pimprove) {
    targets[cids_used].kind = VUT_IMPROVEMENT;
    targets[cids_used].value.building = pimprove;
    cids_used++;
  } city_built_iterate_end;

  return cids_used;
}

/**********************************************************************//**
  Returns number of units known to be supported by city. This might not real
  number of units in case of enemy city.
**************************************************************************/
int num_supported_units_in_city(struct city *pcity)
{
  struct unit_list *plist;

  if (can_player_see_city_internals(client.conn.playing, pcity)) {
    /* Other players don't see inside the city (but observers do). */
    plist = pcity->client.info_units_supported;
  } else {
    plist = pcity->units_supported;
  }

  return unit_list_size(plist);
}

/**********************************************************************//**
  Returns number of units known to be in city. This might not real
  number of units in case of enemy city.
**************************************************************************/
int num_present_units_in_city(struct city *pcity)
{
  struct unit_list *plist;

  if (can_player_see_units_in_city(client.conn.playing, pcity)) {
    /* Other players don't see inside the city (but observers do). */
    plist = pcity->client.info_units_present;
  } else {
    plist = pcity->tile->units;
  }

  return unit_list_size(plist);
}

/**********************************************************************//**
  Handles a chat or event message.
**************************************************************************/
void handle_event(const char *featured_text, struct tile *ptile,
                  enum event_type event, int turn, int phase, int conn_id)
{
  char plain_text[MAX_LEN_MSG];
  struct text_tag_list *tags;
  int where = MW_OUTPUT;	/* where to display the message */
  bool fallback_needed = FALSE; /* we want fallback if actual 'where' is not
                                 * usable */
  bool shown = FALSE;           /* Message displayed somewhere at least */

  if (!event_type_is_valid(event))  {
    /* Server may have added a new event; leave as MW_OUTPUT */
    log_verbose("Unknown event type %d!", event);
  } else if (event >= 0)  {
    where = messages_where[event];
  }

  /* Get the original text. */
  featured_text_to_plain_text(featured_text, plain_text,
                              sizeof(plain_text), &tags, conn_id != -1);

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
  if (-1 != conn_id
      && client.conn.id != conn_id
      && ft_color_requested(gui_options.highlight_our_names)) {
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
      if (NULL != username
          && 0 == fc_strncasecmp(p, username, userlen)) {
        struct text_tag *ptag = text_tag_new(TTT_COLOR, p - plain_text,
                                             p - plain_text + userlen,
                                             gui_options.highlight_our_names);

        fc_assert(ptag != NULL);

        if (ptag != NULL) {
          /* Appends to be sure it will be applied at last. */
          text_tag_list_append(tags, ptag);
        }
      } else if (NULL != playername
                 && 0 == fc_strncasecmp(p, playername, playerlen)) {
        struct text_tag *ptag = text_tag_new(TTT_COLOR, p - plain_text,
                                             p - plain_text + playerlen,
                                             gui_options.highlight_our_names);

        fc_assert(ptag != NULL);

        if (ptag != NULL) {
          /* Appends to be sure it will be applied at last. */
          text_tag_list_append(tags, ptag);
        }
      }
    }
  }

  /* Popup */
  if (BOOL_VAL(where & MW_POPUP)) {
    /* Popups are usually not shown if player is under AI control.
     * Server operator messages are shown always. */
    if (NULL == client.conn.playing
        || is_human(client.conn.playing)
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
      meswin_add(plain_text, tags, ptile, event, turn, phase);
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

  if (turn == game.info.turn) {
    play_sound_for_event(event);
  }

  /* Free tags */
  text_tag_list_destroy(tags);
}

/**********************************************************************//**
  Creates a struct packet_generic_message packet and injects it via
  handle_chat_msg.
**************************************************************************/
void create_event(struct tile *ptile, enum event_type event,
                  const struct ft_color color, const char *format, ...)
{
  va_list ap;
  char message[MAX_LEN_MSG];

  va_start(ap, format);
  fc_vsnprintf(message, sizeof(message), format, ap);
  va_end(ap);

  if (ft_color_requested(color)) {
    char colored_text[MAX_LEN_MSG];

    featured_text_apply_tag(message, colored_text, sizeof(colored_text),
                            TTT_COLOR, 0, FT_OFFSET_UNSET, color);
    handle_event(colored_text, ptile, event, game.info.turn, game.info.phase, -1);
  } else {
    handle_event(message, ptile, event, game.info.turn, game.info.phase, -1);
  }
}

/**********************************************************************//**
  Find city nearest to given unit and optionally return squared city
  distance Parameter sq_dist may be NULL. Returns NULL only if no city is
  known. Favors punit owner's cities over other cities if equally distant.
**************************************************************************/
struct city *get_nearest_city(const struct unit *punit, int *sq_dist)
{
  struct city *pcity_near;
  int pcity_near_dist;

  if ((pcity_near = tile_city(unit_tile(punit)))) {
    pcity_near_dist = 0;
  } else {
    pcity_near = NULL;
    pcity_near_dist = -1;
    players_iterate(pplayer) {
      city_list_iterate(pplayer->cities, pcity_current) {
        int dist = sq_map_distance(pcity_current->tile, unit_tile(punit));
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

/**********************************************************************//**
  Called when the "Buy" button is pressed in the city report for every
  selected city. Checks for coinage and sufficient funds or request the
  purchase if everything is ok.
**************************************************************************/
void cityrep_buy(struct city *pcity)
{
  int value;

  if (city_production_has_flag(pcity, IF_GOLD)) {
    create_event(pcity->tile, E_BAD_COMMAND, ftc_client,
                 _("You can't buy %s in %s!"),
                 improvement_name_translation(pcity->production.value.building),
                 city_link(pcity));
    return;
  }
  value = pcity->client.buy_cost;

  if (city_owner(pcity)->economic.gold >= value) {
    city_buy_production(pcity);
  } else {
    /* Split into two to allow localization of two pluralisations. */
    char buf[MAX_LEN_MSG];
    /* TRANS: %s is a production type; this whole string is a sentence
     * fragment that is only ever included in one other string
     * (search comments for this string to find it) */
    fc_snprintf(buf, ARRAY_SIZE(buf), PL_("%s costs %d gold",
                                          "%s costs %d gold", value),
                city_production_name_translation(pcity),
                value);
    create_event(NULL, E_BAD_COMMAND, ftc_client,
                 /* TRANS: %s is a pre-pluralised sentence fragment:
                  * "%s costs %d gold" */
                 PL_("%s and you only have %d gold.",
                     "%s and you only have %d gold.",
                     city_owner(pcity)->economic.gold),
                 buf, city_owner(pcity)->economic.gold);
  }
}

/**********************************************************************//**
  Switch between tax/sci/lux at given slot.
**************************************************************************/
void common_taxrates_callback(int i)
{
  int lux_end, sci_end, tax, lux, sci;
  int delta = 10;

  if (!can_client_issue_orders()) {
    return;
  }

  lux_end = client.conn.playing->economic.luxury;
  sci_end = lux_end + client.conn.playing->economic.science;

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

/**********************************************************************//**
  Returns TRUE if any of the units can do the connect activity.
**************************************************************************/
bool can_units_do_connect(struct unit_list *punits,
			  enum unit_activity activity,
                          struct extra_type *tgt)
{
  unit_list_iterate(punits, punit) {
    if (can_unit_do_connect(punit, activity, tgt)) {
      return TRUE;
    }
  } unit_list_iterate_end;

  return FALSE;
}

/**********************************************************************//**
  Initialize the action probability cache. Shouldn't be kept around
  permanently. Its data is quickly outdated.
**************************************************************************/
void client_unit_init_act_prob_cache(struct unit *punit)
{
  /* A double init would cause a leak. */
  fc_assert_ret(punit->client.act_prob_cache == NULL);

  punit->client.act_prob_cache = (struct act_prob*)fc_malloc(
        NUM_ACTIONS * sizeof(*punit->client.act_prob_cache));
}

/**********************************************************************//**
  Determines which color type should be used for unit background.
  This is only guesswork based on unit properties. One should not
  take UNIT_BG_FLYING seriously meaning that unit can fly - custom
  ruleset might have units with similar properties but explains these
  properties by some other means than by flying. 
**************************************************************************/
enum unit_bg_color_type unit_color_type(const struct unit_type *punittype)
{
  struct unit_class *pclass = utype_class(punittype);

  if (pclass->hp_loss_pct > 0) {
    return UNIT_BG_HP_LOSS;
  }

  if (pclass->move_type == UMT_LAND) {
    return UNIT_BG_LAND;
  }
  if (pclass->move_type == UMT_SEA) {
    return UNIT_BG_SEA;
  }

  fc_assert(pclass->move_type == UMT_BOTH);

  if (uclass_has_flag(pclass, UCF_TERRAIN_SPEED)) {
    /* Unit moves on both sea and land by speed determined by terrain */
    return UNIT_BG_AMPHIBIOUS;
  }

  return UNIT_BG_FLYING;
}

/**********************************************************************//**
  Comparison function used by qsort in buy_production_in_selected_cities().
**************************************************************************/
static int city_buy_cost_compare(const void *a, const void *b)
{
  const struct city *ca, *cb;

  ca = *((const struct city **) a);
  cb = *((const struct city **) b);

  return ca->client.buy_cost - cb->client.buy_cost;
}

/**********************************************************************//**
  For each selected city, buy the current production. The selected cities
  are sorted so production is bought in the cities with lowest cost first.
**************************************************************************/
void buy_production_in_selected_cities(void)
{
  const struct player *pplayer = client_player();
  struct connection *pconn;

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

  pconn = &client.conn;
  connection_do_buffer(pconn);

  for (i = 0; i < count && gold > 0; i++) {
    gold -= cities[i]->client.buy_cost;
    city_buy_production(cities[i]);
  }

  connection_do_unbuffer(pconn);
}

/**********************************************************************//**
  Set focus status of all player units to FOCUS_AVAIL.
**************************************************************************/
void unit_focus_set_status(struct player *pplayer)
{
  unit_list_iterate(pplayer->units, punit) {
    punit->client.focus_status = FOCUS_AVAIL;
  } unit_list_iterate_end;
}

/**********************************************************************//**
  Initialize a player on the client side.
**************************************************************************/
void client_player_init(struct player *pplayer)
{
  vision_layer_iterate(v) {
    pplayer->client.tile_vision[v].vec = NULL;
    pplayer->client.tile_vision[v].bits = 0;
  } vision_layer_iterate_end;
}

/**********************************************************************//**
  Reset the private maps of all players.
**************************************************************************/
void client_player_maps_reset(void)
{
  players_iterate(pplayer) {
    int new_size;

    if (pplayer == client.conn.playing) {
      new_size = MAP_INDEX_SIZE;
    } else {
      /* We don't need (or have) information about players other
       * than user of the client. Allocate just one bit as that's
       * the minimum bitvector size (cannot allocate 0 bits)*/
      new_size = 1;
    }

    vision_layer_iterate(v) {
      dbv_resize(&pplayer->client.tile_vision[v], new_size);
    } vision_layer_iterate_end;

    dbv_resize(&pplayer->tile_known, new_size);
  } players_iterate_end;
}

/**********************************************************************//**
  Create a map image definition on the client.
**************************************************************************/
bool mapimg_client_define(void)
{
  char str[MAX_LEN_MAPDEF];
  char mi_map[MAPIMG_LAYER_COUNT + 1];
  enum mapimg_layer layer;
  int map_pos = 0;

  /* Only one definition allowed. */
  while (mapimg_count() != 0) {
    mapimg_delete(0);
  }

  /* Map image definition: zoom, turns */
  fc_snprintf(str, sizeof(str), "zoom=%d:turns=0:format=%s",
              gui_options.mapimg_zoom, gui_options.mapimg_format);

  /* Map image definition: show */
  if (client_is_global_observer()) {
    cat_snprintf(str, sizeof(str), ":show=all");
    /* use all available knowledge */
    gui_options.mapimg_layer[MAPIMG_LAYER_KNOWLEDGE] = FALSE;
  } else {
    cat_snprintf(str, sizeof(str), ":show=plrid:plrid=%d",
                 player_index(client.conn.playing));
    /* use only player knowledge */
    gui_options.mapimg_layer[MAPIMG_LAYER_KNOWLEDGE] = TRUE;
  }

  /* Map image definition: map */
  for (layer = mapimg_layer_begin(); layer != mapimg_layer_end();
       layer = mapimg_layer_next(layer)) {
    if (gui_options.mapimg_layer[layer]) {
      cat_snprintf(mi_map, sizeof(mi_map), "%s",
                   mapimg_layer_name(layer));
      mi_map[map_pos++] = mapimg_layer_name(layer)[0];
    }
  }
  mi_map[map_pos] = '\0';

  if (map_pos == 0) {
    /* no value set - use dummy setting */
    sz_strlcpy(mi_map, "-");
  }
  cat_snprintf(str, sizeof(str), ":map=%s", mi_map);

  log_debug("client map image definition: %s", str);

  if (!mapimg_define(str, FALSE) || !mapimg_isvalid(0)) {
    /* An error in the definition string or an error validation the string.
     * The error message is available via mapimg_error(). */
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Save map image.
**************************************************************************/
bool mapimg_client_createmap(const char *filename)
{
  struct mapdef *pmapdef;
  char mapimgfile[512];

  if (NULL == filename || '\0' == filename[0]) {
    sz_strlcpy(mapimgfile, gui_options.mapimg_filename);
  } else {
    sz_strlcpy(mapimgfile, filename);
  }

  if (!mapimg_client_define()) {
    return FALSE;
  }

  pmapdef = mapimg_isvalid(0);
  if (!pmapdef) {
    return FALSE;
  }

  return mapimg_create(pmapdef, TRUE, mapimgfile, NULL);
}

/**********************************************************************//**
  Returns the nation set in use.
**************************************************************************/
struct nation_set *client_current_nation_set(void)
{
  struct option *poption = optset_option_by_name(server_optset, "nationset");
  const char *setting_str;

  if (poption == NULL
      || option_type(poption) != OT_STRING
      || (setting_str = option_str_get(poption)) == NULL) {
    setting_str = "";
  }
  return nation_set_by_setting_value(setting_str);
}

/**********************************************************************//**
  Returns Whether 'pnation' is in the current nation set.
**************************************************************************/
bool client_nation_is_in_current_set(const struct nation_type *pnation)
{
  return nation_is_in_set(pnation, client_current_nation_set());
}

/**********************************************************************//**
  Returns the current AI skill level on the server, if the same level is
  currently used for all current AI players and will be for new ones;
  else return ai_level_invalid() to indicate inconsistency.
**************************************************************************/
enum ai_level server_ai_level(void)
{
  enum ai_level lvl = game.info.skill_level;

  players_iterate(pplayer) {
    if (is_ai(pplayer) && pplayer->ai_common.skill_level != lvl) {
      return ai_level_invalid();
    }
  } players_iterate_end;

  if (!is_settable_ai_level(lvl)) {
    return ai_level_invalid();
  }

  return lvl;
}
