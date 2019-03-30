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

/* utility */
#include "fcintl.h"
#include "log.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "specialist.h"
#include "unitlist.h"

/* client/include */
#include "citydlg_g.h"
#include "mapview_g.h"

/* client */
#include "citydlg_common.h"
#include "client_main.h"		/* for can_client_issue_orders() */
#include "climap.h"
#include "control.h"
#include "mapview_common.h"
#include "options.h"		/* for concise_city_production */
#include "tilespec.h"		/* for tileset_is_isometric(tileset) */

static int citydlg_map_width, citydlg_map_height;

/**********************************************************************//**
  Return the width of the city dialog canvas.
**************************************************************************/
int get_citydlg_canvas_width(void)
{
  return citydlg_map_width;
}

/**********************************************************************//**
  Return the height of the city dialog canvas.
**************************************************************************/
int get_citydlg_canvas_height(void)
{
  return citydlg_map_height;
}

/**********************************************************************//**
  Calculate the citydlg width and height.
**************************************************************************/
void generate_citydlg_dimensions(void)
{
  int min_x = 0, max_x = 0, min_y = 0, max_y = 0;
  int max_rad = rs_max_city_radius_sq();

  /* use maximum possible squared city radius. */
  city_map_iterate_without_index(max_rad, city_x, city_y) {
    float canvas_x, canvas_y;

    map_to_gui_vector(get_tileset(), 1.0, &canvas_x, &canvas_y,
                      CITY_ABS2REL(city_x), CITY_ABS2REL(city_y));

    min_x = MIN(canvas_x, min_x);
    max_x = MAX(canvas_x, max_x);
    min_y = MIN(canvas_y, min_y);
    max_y = MAX(canvas_y, max_y);
  } city_map_iterate_without_index_end;

  citydlg_map_width = max_x - min_x + tileset_tile_width(get_tileset());
  citydlg_map_height = max_y - min_y + tileset_tile_height(get_tileset());
}

/**********************************************************************//**
  Converts a (cartesian) city position to citymap canvas coordinates.
  Returns TRUE if the city position is valid.
**************************************************************************/
bool city_to_canvas_pos(float *canvas_x, float *canvas_y, int city_x,
                        int city_y, int city_radius_sq)
{
  const int width = get_citydlg_canvas_width();
  const int height = get_citydlg_canvas_height();

  /* The citymap is centered over the center of the citydlg canvas. */
  map_to_gui_vector(tileset, 1.0, canvas_x, canvas_y, CITY_ABS2REL(city_x),
                    CITY_ABS2REL(city_y));
  *canvas_x += (width - tileset_tile_width(tileset)) / 2;
  *canvas_y += (height - tileset_tile_height(tileset)) / 2;

  fc_assert_ret_val(is_valid_city_coords(city_radius_sq, city_x, city_y),
                    FALSE);

  return TRUE;
}

/**********************************************************************//**
  Converts a citymap canvas position to a (cartesian) city coordinate
  position.  Returns TRUE iff the city position is valid.
**************************************************************************/
bool canvas_to_city_pos(int *city_x, int *city_y, int city_radius_sq,
                        int canvas_x, int canvas_y)
{
#ifdef FREECIV_DEBUG
  int orig_canvas_x = canvas_x, orig_canvas_y = canvas_y;
#endif
  const int width = get_citydlg_canvas_width();
  const int height = get_citydlg_canvas_height();

  /* The citymap is centered over the center of the citydlg canvas. */
  canvas_x -= (width - tileset_tile_width(get_tileset())) / 2;
  canvas_y -= (height - tileset_tile_height(get_tileset())) / 2;

  if (tileset_is_isometric(get_tileset())) {
    const int W = tileset_tile_width(get_tileset()),
              H = tileset_tile_height(get_tileset());

    /* Shift the tile left so the top corner of the origin tile is at
       canvas position (0,0). */
    canvas_x -= W / 2;

    /* Perform a pi/4 rotation, with scaling.  See canvas_pos_to_map_pos
       for a full explanation. */
    *city_x = DIVIDE(canvas_x * H + canvas_y * W, W * H);
    *city_y = DIVIDE(canvas_y * W - canvas_x * H, W * H);
  } else {
    *city_x = DIVIDE(canvas_x, tileset_tile_width(get_tileset()));
    *city_y = DIVIDE(canvas_y, tileset_tile_height(get_tileset()));
  }

  /* Add on the offset of the top-left corner to get the final
   * coordinates (like in canvas_to_map_pos). */
  *city_x = CITY_REL2ABS(*city_x);
  *city_y = CITY_REL2ABS(*city_y);

  log_debug("canvas_to_city_pos(pos=(%d,%d))=(%d,%d)@radius=%d",
            orig_canvas_x, orig_canvas_y, *city_x, *city_y, city_radius_sq);

  return is_valid_city_coords(city_radius_sq, *city_x, *city_y);
}

/* Iterate over all known tiles in the city.  This iteration follows the
 * painter's algorithm and can be used for drawing. */
#define citydlg_iterate(pcity, ptile, pedge, pcorner, _x, _y)		\
{									\
  float _x##_0, _y##_0;                                                 \
  int _tile_x, _tile_y;                                                 \
  const int _x##_w = get_citydlg_canvas_width();			\
  const int _y##_h = get_citydlg_canvas_height();			\
  index_to_map_pos(&_tile_x, &_tile_y, tile_index((pcity)->tile));      \
									\
  map_to_gui_vector(tileset, 1.0, &_x##_0, &_y##_0, _tile_x, _tile_y);  \
  _x##_0 -= (_x##_w - tileset_tile_width(tileset)) / 2;			\
  _y##_0 -= (_y##_h - tileset_tile_height(tileset)) / 2;		\
  log_debug("citydlg: %f,%f + %dx%d",					\
	    _x##_0, _y##_0, _x##_w, _y##_h);				\
									\
  gui_rect_iterate_coord(_x##_0, _y##_0, _x##_w, _y##_h,		\
                         ptile, pedge, pcorner, _x##_g, _y##_g, 1.0) {  \
    const int _x = _x##_g - _x##_0;					\
    const int _y = _y##_g - _y##_0;					\
    {

#define citydlg_iterate_end						\
    }									\
  } gui_rect_iterate_coord_end;						\
}

/**********************************************************************//**
  Draw the full city map onto the canvas store.  Works for both isometric
  and orthogonal views.
**************************************************************************/
void city_dialog_redraw_map(struct city *pcity,
			    struct canvas *pcanvas)
{
  struct tileset *tmp;

  tmp = NULL;
  if (unscaled_tileset) {
    tmp = tileset;
    tileset = unscaled_tileset;
  }

  /* First make it all black. */
  canvas_put_rectangle(pcanvas, get_color(tileset, COLOR_MAPVIEW_UNKNOWN),
		       0, 0,
		       get_citydlg_canvas_width(),
		       get_citydlg_canvas_height());

  mapview_layer_iterate(layer) {
    citydlg_iterate(pcity, ptile, pedge, pcorner, canvas_x, canvas_y) {
      struct unit *punit
	= ptile ? get_drawable_unit(tileset, ptile, pcity) : NULL;
      struct city *pcity_draw = ptile ? tile_city(ptile) : NULL;

      put_one_element(pcanvas, 1.0, layer, ptile, pedge, pcorner,
                      punit, pcity_draw, canvas_x, canvas_y, pcity, NULL);
    } citydlg_iterate_end;
  } mapview_layer_iterate_end;

  if (tmp != NULL) {
    tileset = tmp;
  }
}

/**********************************************************************//**
  Return a string describing the cost for the production of the city
  considerung several build slots for units.
**************************************************************************/
char *city_production_cost_str(const struct city *pcity)
{
  static char cost_str[50];
  int cost = city_production_build_shield_cost(pcity);
  int build_slots = city_build_slots(pcity);
  int num_units;

  if (build_slots > 1
      && city_production_build_units(pcity, TRUE, &num_units)) {
    /* the city could build more than one unit of the selected type */
    if (num_units == 0) {
      /* no unit will be finished this turn but one is build */
      num_units++;
    }

    if (build_slots > num_units) {
      /* some build slots for units will be unused */
      fc_snprintf(cost_str, sizeof(cost_str), "{%d*%d}", num_units, cost);
    } else {
      /* maximal number of units will be build */
      fc_snprintf(cost_str, sizeof(cost_str), "[%d*%d]", num_units, cost);
    }
  } else {
    /* nothing special */
    fc_snprintf(cost_str, sizeof(cost_str), "%3d", cost);
  }

  return cost_str;
}

/**********************************************************************//**
  Find the city dialog city production text for the given city, and
  place it into the buffer.  This will check the
  concise_city_production option.  pcity may be NULL; in this case a
  filler string is returned.
**************************************************************************/
void get_city_dialog_production(struct city *pcity,
				char *buffer, size_t buffer_len)
{
  char time_str[50], *cost_str;
  int turns, stock;

  if (pcity == NULL) {
    /*
     * Some GUIs use this to build a "filler string" so that they can
     * properly size the widget to hold the string.  This has some
     * obvious problems; the big one is that we have two forms of time
     * information: "XXX turns" and "never".  Later this may need to
     * be extended to return the longer of the two; in the meantime
     * translators can fudge it by changing this "filler" string. 
     */
    /* TRANS: Use longer of "XXX turns" and "never" */
    fc_strlcpy(buffer, Q_("?filler:XXX/XXX XXX turns"), buffer_len);

    return;
  }

  if (city_production_has_flag(pcity, IF_GOLD)) {
    int gold = MAX(0, pcity->surplus[O_SHIELD]);
    fc_snprintf(buffer, buffer_len, PL_("%3d gold per turn",
                                        "%3d gold per turn", gold), gold);
    return;
  }

  turns = city_production_turns_to_build(pcity, TRUE);
  stock = pcity->shield_stock;
  cost_str = city_production_cost_str(pcity);

  if (turns < FC_INFINITY) {
    if (gui_options.concise_city_production) {
      fc_snprintf(time_str, sizeof(time_str), "%3d", turns);
    } else {
      fc_snprintf(time_str, sizeof(time_str),
                  PL_("%3d turn", "%3d turns", turns), turns);
    }
  } else {
    fc_snprintf(time_str, sizeof(time_str), "%s",
                gui_options.concise_city_production ? "-" : _("never"));
  }

  if (gui_options.concise_city_production) {
    fc_snprintf(buffer, buffer_len, _("%3d/%s:%s"), stock, cost_str,
                time_str);
  } else {
    fc_snprintf(buffer, buffer_len, _("%3d/%s %s"), stock, cost_str,
                time_str);
  }
}


/**********************************************************************//**
 Pretty sprints the info about a production (name, info, cost, turns
 to build) into a single text string.

 This is very similar to get_city_dialog_production_row; the
 difference is that instead of placing the data into an array of
 strings it all goes into one long string.  This means it can be used
 by frontends that do not use a tabled structure, but it also gives
 less flexibility.
**************************************************************************/
void get_city_dialog_production_full(char *buffer, size_t buffer_len,
                                     struct universal *target,
                                     struct city *pcity)
{
  int turns = city_turns_to_build(pcity, target, TRUE);
  int cost = universal_build_shield_cost(pcity, target);

  switch (target->kind) {
  case VUT_IMPROVEMENT:
    fc_strlcpy(buffer,
               city_improvement_name_translation(pcity, target->value.building),
               buffer_len);

    if (improvement_has_flag(target->value.building, IF_GOLD)) {
      cat_snprintf(buffer, buffer_len, " (--) ");
      cat_snprintf(buffer, buffer_len, _("%d/turn"),
                   MAX(0, pcity->surplus[O_SHIELD]));
      return;
    }
    break;
  default:
    universal_name_translation(target, buffer, buffer_len);
    break;
  };
  cat_snprintf(buffer, buffer_len, " (%d) ", cost);

  if (turns < FC_INFINITY) {
    cat_snprintf(buffer, buffer_len,
		 PL_("%d turn", "%d turns", turns),
		 turns);
  } else {
    cat_snprintf(buffer, buffer_len, _("never"));
  }
}

/**********************************************************************//**
  Pretty sprints the info about a production in 4 columns (name, info,
  cost, turns to build). The columns must each have a size of
  column_size bytes.  City may be NULL.
**************************************************************************/
void get_city_dialog_production_row(char *buf[], size_t column_size,
                                    struct universal *target,
                                    struct city *pcity)
{
  universal_name_translation(target, buf[0], column_size);

  switch (target->kind) {
  case VUT_UTYPE:
  {
    struct unit_type *ptype = target->value.utype;

    fc_strlcpy(buf[1], utype_values_string(ptype), column_size);
    fc_snprintf(buf[2], column_size, "(%d)", utype_build_shield_cost(pcity, ptype));
    break;
  }
  case VUT_IMPROVEMENT:
  {
    struct player *pplayer = pcity ? city_owner(pcity) : client.conn.playing;
    struct impr_type *pimprove = target->value.building;

    /* Total & turns left meaningless on capitalization */
    if (improvement_has_flag(pimprove, IF_GOLD)) {
      buf[1][0] = '\0';
      fc_snprintf(buf[2], column_size, "---");
    } else {
      int upkeep = pcity ? city_improvement_upkeep(pcity, pimprove)
                         : pimprove->upkeep;
      /* from city.c city_improvement_name_translation() */
      if (pcity && is_improvement_redundant(pcity, pimprove)) {
        fc_snprintf(buf[1], column_size, "%d*", upkeep);
      } else {
        const char *state = NULL;

        if (is_great_wonder(pimprove)) {
          if (improvement_obsolete(pplayer, pimprove, pcity)) {
            state = _("Obsolete");
          } else if (great_wonder_is_built(pimprove)) {
            state = _("Built");
          } else if (great_wonder_is_destroyed(pimprove)) {
            state = _("Destroyed");
          } else {
            state = _("Great Wonder");
          }
        } else if (is_small_wonder(pimprove)) {
          if (improvement_obsolete(pplayer, pimprove, pcity)) {
            state = _("Obsolete");
          } else if (wonder_is_built(pplayer, target->value.building)) {
            state = _("Built");
          } else {
            state = _("Small Wonder");
          }
        }
        if (state) {
          fc_strlcpy(buf[1], state, column_size);
        } else {
          fc_snprintf(buf[1], column_size, "%d", upkeep);
        }
      }

      fc_snprintf(buf[2], column_size, "%d",
                  impr_build_shield_cost(pcity, pimprove));
    }
    break;
  }
  default:
    buf[1][0] = '\0';
    buf[2][0] = '\0';
    break;
  };

  /* Add the turns-to-build entry in the 4th position */
  if (pcity) {
    if (VUT_IMPROVEMENT == target->kind
        && improvement_has_flag(target->value.building, IF_GOLD)) {
      fc_snprintf(buf[3], column_size, _("%d/turn"),
                  MAX(0, pcity->surplus[O_SHIELD]));
    } else {
      int turns = city_turns_to_build(pcity, target, FALSE);

      if (turns < FC_INFINITY) {
        fc_snprintf(buf[3], column_size, "%d", turns);
      } else {
        fc_snprintf(buf[3], column_size, _("never"));
      }
    }
  } else {
    fc_snprintf(buf[3], column_size, "---");
  }
}

/**********************************************************************//**
  Return text describing the production output.
**************************************************************************/
void get_city_dialog_output_text(const struct city *pcity,
                                 Output_type_id otype,
                                 char *buf, size_t bufsz)
{
  int total = 0;
  int priority;
  int tax[O_LAST];
  struct output_type *output = &output_types[otype];

  buf[0] = '\0';

  cat_snprintf(buf, bufsz,
	       _("%+4d : Citizens\n"), pcity->citizen_base[otype]);
  total += pcity->citizen_base[otype];

  /* Hack to get around the ugliness of add_tax_income. */
  memset(tax, 0, O_LAST * sizeof(*tax));
  add_tax_income(city_owner(pcity), pcity->prod[O_TRADE], tax);
  if (tax[otype] != 0) {
    cat_snprintf(buf, bufsz, _("%+4d : Taxed from trade\n"), tax[otype]);
    total += tax[otype];
  }

  /* Special cases for "bonus" production.  See set_city_production in
   * city.c. */
  if (otype == O_TRADE) {
    trade_routes_iterate(pcity, proute) {
      /* NB: (proute->value == 0) is valid case.  The trade route
       * is established but doesn't give trade surplus. */
      struct city *trade_city = game_city_by_number(proute->partner);
      /* TRANS: Trade partner unknown to client */
      const char *name = trade_city ? city_name_get(trade_city) : _("(unknown)");
      int value = proute->value
        * (100 + get_city_bonus(pcity, EFT_TRADEROUTE_PCT)) / 100;

      switch (proute->dir) {
      case RDIR_BIDIRECTIONAL:
        cat_snprintf(buf, bufsz, _("%+4d : Trading %s with %s\n"), value,
                     goods_name_translation(proute->goods),
                     name);
        break;
      case RDIR_FROM:
        cat_snprintf(buf, bufsz, _("%+4d : Trading %s to %s\n"), value,
                     goods_name_translation(proute->goods),
                     name);
        break;
      case RDIR_TO:
        cat_snprintf(buf, bufsz, _("%+4d : Trading %s from %s\n"), value,
                     goods_name_translation(proute->goods),
                     name);
        break;
      }
      total += value;
    } trade_routes_iterate_end;
  } else if (otype == O_GOLD) {
    int tithes = get_city_tithes_bonus(pcity);

    if (tithes != 0) {
      cat_snprintf(buf, bufsz, _("%+4d : Building tithes\n"), tithes);
      total += tithes;
    }
  }

  for (priority = 0; priority < 2; priority++) {
    enum effect_type eft[] = {EFT_OUTPUT_BONUS, EFT_OUTPUT_BONUS_2};

    {
      int base = total, bonus = 100;
      struct effect_list *plist = effect_list_new();

      (void) get_city_bonus_effects(plist, pcity, output, eft[priority]);

      effect_list_iterate(plist, peffect) {
	char buf2[512];
        int delta;
	int new_total;

	get_effect_req_text(peffect, buf2, sizeof(buf2));

        if (peffect->multiplier) {
          int mul = player_multiplier_effect_value(city_owner(pcity),
                                                   peffect->multiplier);
          
          if (mul == 0) {
            /* Suppress text when multiplier setting suppresses effect
             * (this will also suppress it when the city owner's policy
             * settings are not known to us) */
            continue;
          }
          delta = (peffect->value * mul) / 100;
        } else {
          delta = peffect->value;
        }
        bonus += delta;
	new_total = bonus * base / 100;
	cat_snprintf(buf, bufsz,
                     (delta > 0) ? _("%+4d : Bonus from %s (%+d%%)\n")
                                 : _("%+4d : Loss from %s (%+d%%)\n"),
		     (new_total - total), buf2, delta);
	total = new_total;
      } effect_list_iterate_end;
      effect_list_destroy(plist);
    }
  }

  if (pcity->waste[otype] != 0) {
    int wastetypes[OLOSS_LAST];
    bool breakdown_ok;
    int regular_waste;
    /* FIXME: this will give the wrong answer in rulesets with waste on
     * taxed outputs, such as 'science waste', as total includes tax whereas
     * the equivalent bit in set_city_production() does not */
    if (city_waste(pcity, otype, total, wastetypes) == pcity->waste[otype]) {
      /* Our calculation matches the server's, so we trust our breakdown. */
      if (wastetypes[OLOSS_SIZE] > 0) {
        cat_snprintf(buf, bufsz,
                     _("%+4d : Size penalty\n"), -wastetypes[OLOSS_SIZE]);
      }
      regular_waste = wastetypes[OLOSS_WASTE];
      breakdown_ok = TRUE;
    } else {
      /* Our calculation doesn't match what the server sent. Account it all
       * to corruption/waste. */
      regular_waste = pcity->waste[otype];
      breakdown_ok = FALSE;
    }
    if (regular_waste > 0) {
      char *fmt;
      switch (otype) {
        case O_SHIELD:
        default: /* FIXME other output types? */
          /* TRANS: %s is normally empty, but becomes '?' if client is
           * uncertain about its accounting (should never happen) */
          fmt = _("%+4d : Waste%s\n");
          break;
        case O_TRADE:
          /* TRANS: %s is normally empty, but becomes '?' if client is
           * uncertain about its accounting (should never happen) */
          fmt = _("%+4d : Corruption%s\n");
          break;
      }
      cat_snprintf(buf, bufsz, fmt, -regular_waste, breakdown_ok ? "" : "?");
    }
    total -= pcity->waste[otype];
  }

  if (pcity->unhappy_penalty[otype] != 0) {
    cat_snprintf(buf, bufsz,
		 _("%+4d : Disorder\n"), -pcity->unhappy_penalty[otype]);
    total -= pcity->unhappy_penalty[otype];
  }

  if (pcity->usage[otype] > 0) {
    cat_snprintf(buf, bufsz,
		 _("%+4d : Used\n"), -pcity->usage[otype]);
    total -= pcity->usage[otype];
  }

  /* This should never happen, but if it does, at least acknowledge to
   * the user that we are confused, rather than displaying an incorrect
   * sum. */
  if (total != pcity->surplus[otype]) {
    cat_snprintf(buf, bufsz,
                 /* TRANS: City output that we cannot explain.
                  * Should never happen. */
                 _("%+4d : (unknown)\n"), pcity->surplus[otype] - total);
  }

  cat_snprintf(buf, bufsz,
	       _("==== : Adds up to\n"));
  cat_snprintf(buf, bufsz,
	       _("%4d : Total surplus"), pcity->surplus[otype]);
}

/**********************************************************************//**
  Return text describing the chance for a plague.
**************************************************************************/
void get_city_dialog_illness_text(const struct city *pcity,
                                  char *buf, size_t bufsz)
{
  int illness, ill_base, ill_size, ill_trade, ill_pollution;
  struct effect_list *plist;

  buf[0] = '\0';

  if (!game.info.illness_on) {
    cat_snprintf(buf, bufsz, _("Illness deactivated in ruleset."));
    return;
  }

  illness = city_illness_calc(pcity, &ill_base, &ill_size, &ill_trade,
                              &ill_pollution);

  cat_snprintf(buf, bufsz, _("%+5.1f : Risk from overcrowding\n"),
               ((float)(ill_size) / 10.0));
  cat_snprintf(buf, bufsz, _("%+5.1f : Risk from trade\n"),
               ((float)(ill_trade) / 10.0));
  cat_snprintf(buf, bufsz, _("%+5.1f : Risk from pollution\n"),
               ((float)(ill_pollution) / 10.0));

  plist = effect_list_new();

  (void) get_city_bonus_effects(plist, pcity, NULL, EFT_HEALTH_PCT);

  effect_list_iterate(plist, peffect) {
    char buf2[512];
    int delta;

    get_effect_req_text(peffect, buf2, sizeof(buf2));

    if (peffect->multiplier) {
      int mul = player_multiplier_effect_value(city_owner(pcity),
                                               peffect->multiplier);
      
      if (mul == 0) {
        /* Suppress text when multiplier setting suppresses effect
         * (this will also suppress it when the city owner's policy
         * settings are not known to us) */
        continue;
      }
      delta = (peffect->value * mul) / 100;
    } else {
      delta = peffect->value;
    }

    cat_snprintf(buf, bufsz,
                 (delta > 0) ? _("%+5.1f : Bonus from %s\n")
                             : _("%+5.1f : Risk from %s\n"),
                 -(0.1 * ill_base * delta / 100), buf2);
  } effect_list_iterate_end;
  effect_list_destroy(plist);

  cat_snprintf(buf, bufsz, _("==== : Adds up to\n"));
  cat_snprintf(buf, bufsz, _("%5.1f : Total chance for a plague"),
               ((float)(illness) / 10.0));
}

/**********************************************************************//**
  Return text describing the pollution output.
**************************************************************************/
void get_city_dialog_pollution_text(const struct city *pcity,
				    char *buf, size_t bufsz)
{
  int pollu, prod, pop, mod;

  /* On the server, pollution is calculated before production is deducted
   * for disorder; we need to compensate for that */
  pollu = city_pollution_types(pcity,
                               pcity->prod[O_SHIELD]
                               + pcity->unhappy_penalty[O_SHIELD],
			       &prod, &pop, &mod);
  buf[0] = '\0';

  cat_snprintf(buf, bufsz,
	       _("%+4d : Pollution from shields\n"), prod);
  cat_snprintf(buf, bufsz,
	       _("%+4d : Pollution from citizens\n"), pop);
  cat_snprintf(buf, bufsz,
	       _("%+4d : Pollution modifier\n"), mod);
  cat_snprintf(buf, bufsz,
	       _("==== : Adds up to\n"));
  cat_snprintf(buf, bufsz,
	       _("%4d : Total surplus"), pollu);
}

/**********************************************************************//**
  Return text describing the culture output.
**************************************************************************/
void get_city_dialog_culture_text(const struct city *pcity,
                                  char *buf, size_t bufsz)
{
  struct effect_list *plist;
  int total_performance = 0;

  buf[0] = '\0';

  cat_snprintf(buf, bufsz,
               _("%4d : History\n"), pcity->history);

  plist = effect_list_new();

  (void) get_city_bonus_effects(plist, pcity, NULL, EFT_PERFORMANCE);

  effect_list_iterate(plist, peffect) {
    char buf2[512];
    int mul = 100;
    int value;

    get_effect_req_text(peffect, buf2, sizeof(buf2));

    if (peffect->multiplier) {
      mul = player_multiplier_effect_value(city_owner(pcity),
                                           peffect->multiplier);
      
      if (mul == 0) {
        /* Suppress text when multiplier setting suppresses effect
         * (this will also suppress it when the city owner's policy
         * settings are not known to us) */
        continue;
      }
    }

    value = (peffect->value * mul) / 100;
    cat_snprintf(buf, bufsz, _("%4d : %s\n"), value, buf2);
    total_performance += value;
  } effect_list_iterate_end;
  effect_list_destroy(plist);

  /* This probably ought not to happen in well-designed rulesets, but it's
   * possible for incomplete client knowledge to give an inaccurate
   * breakdown. If it does happen, at least acknowledge to the user that
   * we are confused, rather than displaying an incorrect sum. */
  if (total_performance != pcity->client.culture - pcity->history) {
    cat_snprintf(buf, bufsz,
                 /* TRANS: City culture that we cannot explain.
                  * Unlikely to happen. */
                 _("%4d : (unknown)\n"),
                 pcity->client.culture - pcity->history - total_performance);
  }

  cat_snprintf(buf, bufsz,
	       _("==== : Adds up to\n"));
  cat_snprintf(buf, bufsz,
	       _("%4d : Total culture"), pcity->client.culture);
}

/**********************************************************************//**
  Provide a list of all citizens in the city, in order.  "index"
  should be the happiness index (currently [0..4]; 4 = final
  happiness).  "citizens" should be an array large enough to hold all
  citizens (use MAX_CITY_SIZE to be on the safe side).
**************************************************************************/
int get_city_citizen_types(struct city *pcity, enum citizen_feeling idx,
                           enum citizen_category *categories)
{
  int i = 0, n;

  fc_assert(idx >= 0 && idx < FEELING_LAST);

  for (n = 0; n < pcity->feel[CITIZEN_HAPPY][idx]; n++, i++) {
    categories[i] = CITIZEN_HAPPY;
  }
  for (n = 0; n < pcity->feel[CITIZEN_CONTENT][idx]; n++, i++) {
    categories[i] = CITIZEN_CONTENT;
  }
  for (n = 0; n < pcity->feel[CITIZEN_UNHAPPY][idx]; n++, i++) {
    categories[i] = CITIZEN_UNHAPPY;
  }
  for (n = 0; n < pcity->feel[CITIZEN_ANGRY][idx]; n++, i++) {
    categories[i] = CITIZEN_ANGRY;
  }

  specialist_type_iterate(sp) {
    for (n = 0; n < pcity->specialists[sp]; n++, i++) {
      categories[i] = CITIZEN_SPECIALIST + sp;
    }
  } specialist_type_iterate_end;

  if (city_size_get(pcity) != i) {
    log_error("get_city_citizen_types() %d citizens "
              "not equal %d city size in \"%s\".",
              i, city_size_get(pcity), city_name_get(pcity));
  }
  return i;
}

/**********************************************************************//**
  Rotate the given specialist citizen to the next type of citizen.
**************************************************************************/
void city_rotate_specialist(struct city *pcity, int citizen_index)
{
  enum citizen_category categories[MAX_CITY_SIZE];
  Specialist_type_id from, to;
  int num_citizens = get_city_citizen_types(pcity, FEELING_FINAL, categories);

  if (citizen_index < 0 || citizen_index >= num_citizens
      || categories[citizen_index] < CITIZEN_SPECIALIST) {
    return;
  }
  from = categories[citizen_index] - CITIZEN_SPECIALIST;

  /* Loop through all specialists in order until we find a usable one
   * (or run out of choices). */
  to = from;
  fc_assert(to >= 0 && to < specialist_count());
  do {
    to = (to + 1) % specialist_count();
  } while (to != from && !city_can_use_specialist(pcity, to));

  if (from != to) {
    city_change_specialist(pcity, from, to);
  }
}

/**********************************************************************//**
  Activate all units on the given map tile.
**************************************************************************/
void activate_all_units(struct tile *ptile)
{
  struct unit_list *punit_list = ptile->units;
  struct unit *pmyunit = NULL;

  unit_list_iterate(punit_list, punit) {
    if (unit_owner(punit) == client.conn.playing) {
      /* Activate this unit. */
      pmyunit = punit;
      request_new_unit_activity(punit, ACTIVITY_IDLE);
    }
  } unit_list_iterate_end;
  if (pmyunit) {
    /* Put the focus on one of the activated units. */
    unit_focus_set(pmyunit);
  }
}

/**********************************************************************//**
  Change the production of a given city.  Return the request ID.
**************************************************************************/
int city_change_production(struct city *pcity, struct universal *target)
{
  return dsend_packet_city_change(&client.conn, pcity->id,
                                  target->kind,
                                  universal_number(target));
}

/**********************************************************************//**
  Set the worklist for a given city.  Return the request ID.

  Note that the worklist does NOT include the current production.
**************************************************************************/
int city_set_worklist(struct city *pcity, const struct worklist *pworklist)
{
  return dsend_packet_city_worklist(&client.conn, pcity->id, pworklist);
}

/**********************************************************************//**
  Commit the changes to the worklist for the city.
**************************************************************************/
void city_worklist_commit(struct city *pcity, struct worklist *pwl)
{
  int k;

  /* Update the worklist.  Remember, though -- the current build
     target really isn't in the worklist; don't send it to the server
     as part of the worklist.  Of course, we have to search through
     the current worklist to find the first _now_available_ build
     target (to cope with players who try mean things like adding a
     Battleship to a city worklist when the player doesn't even yet
     have the Map Making tech).  */

  for (k = 0; k < MAX_LEN_WORKLIST; k++) {
    int same_as_current_build;
    struct universal target;

    if (!worklist_peek_ith(pwl, &target, k)) {
      break;
    }

    same_as_current_build = are_universals_equal(&pcity->production, &target);

    /* Very special case: If we are currently building a wonder we
       allow the construction to continue, even if we the wonder is
       finished elsewhere, ie unbuildable. */
    if (k == 0
        && VUT_IMPROVEMENT == target.kind
        && is_wonder(target.value.building)
        && same_as_current_build) {
      worklist_remove(pwl, k);
      break;
    }

    /* If it can be built... */
    if (can_city_build_now(pcity, &target)) {
      /* ...but we're not yet building it, then switch. */
      if (!same_as_current_build) {
        /* Change the current target */
	city_change_production(pcity, &target);
      }

      /* This item is now (and may have always been) the current
         build target.  Drop it out of the worklist. */
      worklist_remove(pwl, k);
      break;
    }
  }

  /* Send the rest of the worklist on its way. */
  city_set_worklist(pcity, pwl);
}

/**********************************************************************//**
  Insert an item into the city's queue.  This function will send new
  production requests to the server but will NOT send the new worklist
  to the server - the caller should call city_set_worklist() if the
  function returns TRUE.

  Note that the queue DOES include the current production.
**************************************************************************/
static bool base_city_queue_insert(struct city *pcity, int position,
                                   struct universal *item)
{
  if (position == 0) {
    struct universal old = pcity->production;

    /* Insert as current production. */
    if (!can_city_build_direct(pcity, item)) {
      return FALSE;
    }

    if (!worklist_insert(&pcity->worklist, &old, 0)) {
      return FALSE;
    }

    city_change_production(pcity, item);
  } else if (position >= 1
             && position <= worklist_length(&pcity->worklist)) {
    /* Insert into middle. */
    if (!can_city_build_later(pcity, item)) {
      return FALSE;
    }
    if (!worklist_insert(&pcity->worklist, item, position - 1)) {
      return FALSE;
    }
  } else {
    /* Insert at end. */
    if (!can_city_build_later(pcity, item)) {
      return FALSE;
    }
    if (!worklist_append(&pcity->worklist, item)) {
      return FALSE;
    }
  }
  return TRUE;
}

/**********************************************************************//**
  Insert an item into the city's queue.

  Note that the queue DOES include the current production.
**************************************************************************/
bool city_queue_insert(struct city *pcity, int position,
                       struct universal *item)
{
  if (base_city_queue_insert(pcity, position, item)) {
    city_set_worklist(pcity, &pcity->worklist);
    return TRUE;
  }
  return FALSE;
}

/**********************************************************************//**
  Clear the queue (all entries except the first one since that can't be
  cleared).

  Note that the queue DOES include the current production.
**************************************************************************/
bool city_queue_clear(struct city *pcity)
{
  worklist_init(&pcity->worklist);

  return TRUE;
}

/**********************************************************************//**
  Insert the worklist into the city's queue at the given position.

  Note that the queue DOES include the current production.
**************************************************************************/
bool city_queue_insert_worklist(struct city *pcity, int position,
				const struct worklist *worklist)
{
  bool success = FALSE;

  if (worklist_length(worklist) == 0) {
    return TRUE;
  }

  worklist_iterate(worklist, target) {
    if (base_city_queue_insert(pcity, position, &target)) {
      if (position > 0) {
	/* Move to the next position (unless position == -1 in which case
	 * we're appending. */
	position++;
      }
      success = TRUE;
    }
  } worklist_iterate_end;

  if (success) {
    city_set_worklist(pcity, &pcity->worklist);
  }

  return success;
}

/**********************************************************************//**
  Get the city current production and the worklist, like it should be.
**************************************************************************/
void city_get_queue(struct city *pcity, struct worklist *pqueue)
{
  worklist_copy(pqueue, &pcity->worklist);

  /* The GUI wants current production to be in the task list, but the
     worklist API wants it out for reasons unknown. Perhaps someone enjoyed
     making things more complicated than necessary? So I dance around it. */

  /* We want the current production to be in the queue. Always. */
  worklist_remove(pqueue, MAX_LEN_WORKLIST - 1);

  worklist_insert(pqueue, &pcity->production, 0);
}

/**********************************************************************//**
  Set the city current production and the worklist, like it should be.
**************************************************************************/
bool city_set_queue(struct city *pcity, const struct worklist *pqueue)
{
  struct worklist copy;
  struct universal target;

  worklist_copy(&copy, pqueue);

  /* The GUI wants current production to be in the task list, but the
     worklist API wants it out for reasons unknown. Perhaps someone enjoyed
     making things more complicated than necessary? So I dance around it. */
  if (worklist_peek(&copy, &target)) {

    if (!city_can_change_build(pcity)
        && !are_universals_equal(&pcity->production, &target)) {
      /* We cannot change production to one from worklist.
       * Do not replace old worklist with new one. */
      return FALSE;
    }

    worklist_advance(&copy);

    city_set_worklist(pcity, &copy);
    city_change_production(pcity, &target);
  } else {
    /* You naughty boy, you can't erase the current production. Nyah! */
    if (worklist_is_empty(&pcity->worklist)) {
      refresh_city_dialog(pcity);
    } else {
      city_set_worklist(pcity, &copy);
    }
  }

  return TRUE;
}

/**********************************************************************//**
  Return TRUE iff the city can buy.
**************************************************************************/
bool city_can_buy(const struct city *pcity)
{
  /* See really_handle_city_buy() in the server.  However this function
   * doesn't allow for error messages.  It doesn't check the cost of
   * buying; that's handled separately (and with an error message). */
  return (can_client_issue_orders()
          && NULL != pcity
          && city_owner(pcity) == client.conn.playing
          && pcity->turn_founded != game.info.turn
          && !pcity->did_buy
          && (VUT_UTYPE == pcity->production.kind
              || !improvement_has_flag(pcity->production.value.building, IF_GOLD))
          && !(VUT_UTYPE == pcity->production.kind
               && pcity->anarchy != 0)
          && pcity->client.buy_cost > 0);
}

/**********************************************************************//**
  Change the production of a given city.  Return the request ID.
**************************************************************************/
int city_sell_improvement(struct city *pcity, Impr_type_id sell_id)
{
  return dsend_packet_city_sell(&client.conn, pcity->id, sell_id);
}

/**********************************************************************//**
  Buy the current production item in a given city.  Return the request ID.
**************************************************************************/
int city_buy_production(struct city *pcity)
{
  return dsend_packet_city_buy(&client.conn, pcity->id);
}

/**********************************************************************//**
  Change a specialist in the given city.  Return the request ID.
**************************************************************************/
int city_change_specialist(struct city *pcity, Specialist_type_id from,
                           Specialist_type_id to)
{
  return dsend_packet_city_change_specialist(&client.conn, pcity->id, from,
                                             to);
}

/**********************************************************************//**
  Toggle a worker<->specialist at the given city tile.  Return the
  request ID.
**************************************************************************/
int city_toggle_worker(struct city *pcity, int city_x, int city_y)
{
  int city_radius_sq;
  struct tile *ptile;

  if (city_owner(pcity) != client_player()) {
    return 0;
  }

  city_radius_sq = city_map_radius_sq_get(pcity);
  fc_assert(is_valid_city_coords(city_radius_sq, city_x, city_y));
  ptile = city_map_to_tile(city_tile(pcity), city_radius_sq,
                           city_x, city_y);
  if (NULL == ptile) {
    return 0;
  }

  if (NULL != tile_worked(ptile) && tile_worked(ptile) == pcity) {
    return dsend_packet_city_make_specialist(&client.conn,
                                             pcity->id, ptile->index);
  } else if (city_can_work_tile(pcity, ptile)) {
    return dsend_packet_city_make_worker(&client.conn,
                                         pcity->id, ptile->index);
  } else {
    return 0;
  }
}

/**********************************************************************//**
  Tell the server to rename the city.  Return the request ID.
**************************************************************************/
int city_rename(struct city *pcity, const char *name)
{
  return dsend_packet_city_rename(&client.conn, pcity->id, name);
}
