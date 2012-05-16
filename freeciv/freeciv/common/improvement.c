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

#include "fcintl.h"
#include "game.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "shared.h" /* ARRAY_SIZE */
#include "support.h"
#include "tech.h"

#include "improvement.h"

static const char *genus_names[IG_LAST] = {
  "GreatWonder", "SmallWonder", "Improvement", "Special"
};

static const char *flag_names[] = {
  "VisibleByOthers", "SaveSmallWonder", "Gold"
};
/* Note that these strings must correspond with the enums in impr_flag_id,
   in common/improvement.h */

/**************************************************************************
All the city improvements:
Use improvement_by_number(id) to access the array.
The improvement_types array is now setup in:
   server/ruleset.c (for the server)
   client/packhand.c (for the client)
**************************************************************************/
static struct impr_type improvement_types[B_LAST];

/**************************************************************************
  Convert impr genus names to enum; case insensitive;
  returns IG_LAST if can't match.
**************************************************************************/
enum impr_genus_id find_genus_by_rule_name(const char *s)
{
  enum impr_genus_id i;

  for(i = 0; i < ARRAY_SIZE(genus_names); i++) {
    if (mystrcasecmp(genus_names[i], s) == 0) {
      break;
    }
  }
  return i;
}

/****************************************************************************
  Initialize building structures.
****************************************************************************/
void improvements_init(void)
{
  int i;

  /* Can't use improvement_iterate() or improvement_by_number() here
   * because num_impr_types isn't known yet. */
  for (i = 0; i < ARRAY_SIZE(improvement_types); i++) {
    struct impr_type *p = &improvement_types[i];

    p->item_number = i;
    requirement_vector_init(&p->reqs);
  }
}

/**************************************************************************
  Frees the memory associated with this improvement.
**************************************************************************/
static void improvement_free(struct impr_type *p)
{
  free(p->helptext);
  p->helptext = NULL;

  requirement_vector_free(&p->reqs);
}

/***************************************************************
 Frees the memory associated with all improvements.
***************************************************************/
void improvements_free()
{
  improvement_iterate(p) {
    improvement_free(p);
  } improvement_iterate_end;
}

/**************************************************************************
  Return the first item of improvements.
**************************************************************************/
struct impr_type *improvement_array_first(void)
{
  if (game.control.num_impr_types > 0) {
    return improvement_types;
  }
  return NULL;
}

/**************************************************************************
  Return the last item of improvements.
**************************************************************************/
const struct impr_type *improvement_array_last(void)
{
  if (game.control.num_impr_types > 0) {
    return &improvement_types[game.control.num_impr_types - 1];
  }
  return NULL;
}

/**************************************************************************
  Return the number of improvements.
**************************************************************************/
Impr_type_id improvement_count(void)
{
  return game.control.num_impr_types;
}

/**************************************************************************
  Return the improvement index.

  Currently same as improvement_number(), paired with improvement_count()
  indicates use as an array index.
**************************************************************************/
Impr_type_id improvement_index(const struct impr_type *pimprove)
{
  assert(pimprove);
  return pimprove - improvement_types;
}

/**************************************************************************
  Return the improvement index.
**************************************************************************/
Impr_type_id improvement_number(const struct impr_type *pimprove)
{
  assert(pimprove);
  return pimprove->item_number;
}

/**************************************************************************
  Returns the improvement type for the given index/ID.  Returns NULL for
  an out-of-range index.
**************************************************************************/
struct impr_type *improvement_by_number(const Impr_type_id id)
{
  if (id < 0 || id >= improvement_count()) {
    return NULL;
  }
  return &improvement_types[id];
}

/**************************************************************************
  Returns pointer when the improvement_type "exists" in this game,
  returns NULL otherwise.

  An improvement_type doesn't exist for any of:
   - the improvement_type has been flagged as removed by setting its
     tech_req to A_LAST; [was not in current 2007-07-27]
   - it is a space part, and the spacerace is not enabled.
**************************************************************************/
struct impr_type *valid_improvement(struct impr_type *pimprove)
{
  if (NULL == pimprove) {
    return NULL;
  }

  if (!game.info.spacerace
      && (building_has_effect(pimprove, EFT_SS_STRUCTURAL)
	  || building_has_effect(pimprove, EFT_SS_COMPONENT)
	  || building_has_effect(pimprove, EFT_SS_MODULE))) {
    /* This assumes that space parts don't have any other effects. */
    return NULL;
  }

  return pimprove;
}

/**************************************************************************
  Returns pointer when the improvement_type "exists" in this game,
  returns NULL otherwise.

  In addition to valid_improvement(), tests for id is out of range.
**************************************************************************/
struct impr_type *valid_improvement_by_number(const Impr_type_id id)
{
  return valid_improvement(improvement_by_number(id));
}

/**************************************************************************
  Return the (translated) name of the given improvement. 
  You don't have to free the return pointer.
**************************************************************************/
const char *improvement_name_translation(struct impr_type *pimprove)
{
  if (NULL == pimprove->name.translated) {
    /* delayed (unified) translation */
    pimprove->name.translated = ('\0' == pimprove->name.vernacular[0])
				? pimprove->name.vernacular
				: Q_(pimprove->name.vernacular);
  }
  return pimprove->name.translated;
}

/****************************************************************************
  Return the (untranslated) rule name of the improvement.
  You don't have to free the return pointer.
****************************************************************************/
const char *improvement_rule_name(const struct impr_type *pimprove)
{
  return Qn_(pimprove->name.vernacular); 
}

/****************************************************************************
  Returns the number of shields it takes to build this improvement.
****************************************************************************/
int impr_build_shield_cost(const struct impr_type *pimprove)
{
  int base = pimprove->build_cost;

  return MAX(base * game.info.shieldbox / 100, 1);
}

/****************************************************************************
  Returns the amount of gold it takes to rush this improvement.
****************************************************************************/
int impr_buy_gold_cost(const struct impr_type *pimprove, int shields_in_stock)
{
  int cost = 0;
  const int missing = impr_build_shield_cost(pimprove) - shields_in_stock;

  if (improvement_has_flag(pimprove, IF_GOLD)) {
    /* Can't buy capitalization. */
    return 0;
  }

  if (missing > 0) {
    cost = 2 * missing;
  }

  if (is_wonder(pimprove)) {
    cost *= 2;
  }
  if (shields_in_stock == 0) {
    cost *= 2;
  }
  return cost;
}

/****************************************************************************
  Returns the amount of gold received when this improvement is sold.
****************************************************************************/
int impr_sell_gold(const struct impr_type *pimprove)
{
  return impr_build_shield_cost(pimprove);
}

/**************************************************************************
...
**************************************************************************/
bool is_wonder(const struct impr_type *pimprove)
{
  return (is_great_wonder(pimprove) || is_small_wonder(pimprove));
}

/**************************************************************************
  Does a linear search of improvement_types[].name.translated
  Returns NULL when none match.
**************************************************************************/
struct impr_type *find_improvement_by_translated_name(const char *name)
{
  improvement_iterate(pimprove) {
    if (0 == strcmp(improvement_name_translation(pimprove), name)) {
      return pimprove;
    }
  } improvement_iterate_end;

  return NULL;
}

/****************************************************************************
  Does a linear search of improvement_types[].name.vernacular
  Returns NULL when none match.
****************************************************************************/
struct impr_type *find_improvement_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  improvement_iterate(pimprove) {
    if (0 == mystrcasecmp(improvement_rule_name(pimprove), qname)) {
      return pimprove;
    }
  } improvement_iterate_end;

  return NULL;
}

/**************************************************************************
 Return TRUE if the impr has this flag otherwise FALSE
**************************************************************************/
bool improvement_has_flag(const struct impr_type *pimprove,
			  enum impr_flag_id flag)
{
  assert(flag >= 0 && flag < IF_LAST);
  return TEST_BIT(pimprove->flags, flag);
}

/**************************************************************************
 Convert flag names to enum; case insensitive;
 returns IF_LAST if can't match.
**************************************************************************/
enum impr_flag_id find_improvement_flag_by_rule_name(const char *s)
{
  enum impr_flag_id i;

  assert(ARRAY_SIZE(flag_names) == IF_LAST);
  
  for(i = 0; i < IF_LAST; i++) {
    if (mystrcasecmp(flag_names[i], s) == 0) {
      return i;
    }
  }
  return IF_LAST;
}

/**************************************************************************
 Return TRUE if the improvement should be visible to others without spying
**************************************************************************/
bool is_improvement_visible(const struct impr_type *pimprove)
{
  return (is_wonder(pimprove)
          || improvement_has_flag(pimprove, IF_VISIBLE_BY_OTHERS));
}

/**************************************************************************
 Returns 1 if the improvement is obsolete, now also works for wonders
**************************************************************************/
bool improvement_obsolete(const struct player *pplayer,
			  const struct impr_type *pimprove)
{
  if (!valid_advance(pimprove->obsolete_by)) {
    return FALSE;
  }

  if (is_great_wonder(pimprove)) {
    /* a great wonder is obsolete, as soon as *any* player researched the
       obsolete tech */
   return game.info.global_advances[advance_index(pimprove->obsolete_by)];
  }

  return (player_invention_state(pplayer, advance_number(pimprove->obsolete_by)) == TECH_KNOWN);
}

/**************************************************************************
  Returns TRUE iff improvement provides units buildable by player
**************************************************************************/
bool impr_provides_buildable_units(const struct player *pplayer,
                                   const struct impr_type *pimprove)
{
  /* Fast check */
  if (! pimprove->allows_units) {
    return FALSE;
  }

  unit_type_iterate(ut) {
    if (ut->need_improvement == pimprove) {
      if (can_player_build_unit_now(pplayer, ut)) {
        return TRUE;
      }
    }
  } unit_type_iterate_end;

  return FALSE;
}

/**************************************************************************
   Whether player can build given building somewhere, ignoring whether it
   is obsolete.
**************************************************************************/
bool can_player_build_improvement_direct(const struct player *p,
					 struct impr_type *pimprove)
{
  bool space_part = FALSE;

  if (!valid_improvement(pimprove)) {
    return FALSE;
  }

  requirement_vector_iterate(&pimprove->reqs, preq) {
    if (preq->range >= REQ_RANGE_PLAYER
        && !is_req_active(p, NULL, NULL, NULL, NULL, NULL, NULL, preq,
                          RPT_CERTAIN)) {
      return FALSE;
    }
  } requirement_vector_iterate_end;

  /* Check for space part construction.  This assumes that space parts have
   * no other effects. */
  if (building_has_effect(pimprove, EFT_SS_STRUCTURAL)) {
    space_part = TRUE;
    if (p->spaceship.structurals >= NUM_SS_STRUCTURALS) {
      return FALSE;
    }
  }
  if (building_has_effect(pimprove, EFT_SS_COMPONENT)) {
    space_part = TRUE;
    if (p->spaceship.components >= NUM_SS_COMPONENTS) {
      return FALSE;
    }
  }
  if (building_has_effect(pimprove, EFT_SS_MODULE)) {
    space_part = TRUE;
    if (p->spaceship.modules >= NUM_SS_MODULES) {
      return FALSE;
    }
  }
  if (space_part &&
      (!get_player_bonus(p, EFT_ENABLE_SPACE) > 0
       || p->spaceship.state >= SSHIP_LAUNCHED)) {
    return FALSE;
  }

  if (is_great_wonder(pimprove)) {
    /* Can't build wonder if already built */
    if (!great_wonder_is_available(pimprove)) {
      return FALSE;
    }
  }

  return TRUE;
}

/**************************************************************************
  Whether player can build given building somewhere immediately.
  Returns FALSE if building is obsolete.
**************************************************************************/
bool can_player_build_improvement_now(const struct player *p,
				      struct impr_type *pimprove)
{
  if (!can_player_build_improvement_direct(p, pimprove)) {
    return FALSE;
  }
  if (improvement_obsolete(p, pimprove)) {
    return FALSE;
  }
  return TRUE;
}

/**************************************************************************
  Whether player can _eventually_ build given building somewhere -- i.e.,
  returns TRUE if building is available with current tech OR will be
  available with future tech.  Returns FALSE if building is obsolete.
**************************************************************************/
bool can_player_build_improvement_later(const struct player *p,
					struct impr_type *pimprove)
{
  if (!valid_improvement(pimprove)) {
    return FALSE;
  }
  if (improvement_obsolete(p, pimprove)) {
    return FALSE;
  }

  /* Check for requirements that aren't met and that are unchanging (so
   * they can never be met). */
  requirement_vector_iterate(&pimprove->reqs, preq) {
    if (preq->range >= REQ_RANGE_PLAYER
	&& is_req_unchanging(preq)
        && !is_req_active(p, NULL, NULL, NULL, NULL, NULL, NULL, preq,
                          RPT_POSSIBLE)) {
      return FALSE;
    }
  } requirement_vector_iterate_end;
  /* FIXME: should check some "unchanging" reqs here - like if there's
   * a nation requirement, we can go ahead and check it now. */

  return TRUE;
}

/**************************************************************************
  Is this building a great wonder?
**************************************************************************/
bool is_great_wonder(const struct impr_type *pimprove)
{
  return (pimprove->genus == IG_GREAT_WONDER);
}

/**************************************************************************
  Is this building a small wonder?
**************************************************************************/
bool is_small_wonder(const struct impr_type *pimprove)
{
  return (pimprove->genus == IG_SMALL_WONDER);
}

/**************************************************************************
  Is this building a regular improvement?
**************************************************************************/
bool is_improvement(const struct impr_type *pimprove)
{
  return (pimprove->genus == IG_IMPROVEMENT);
}

/**************************************************************************
  Returns TRUE if this is a "special" improvement. For example, spaceship
  parts and coinage in the default ruleset are considered special.
**************************************************************************/
bool is_special_improvement(const struct impr_type *pimprove)
{
  return pimprove->genus == IG_SPECIAL;
}

/**************************************************************************
  Build a wonder in the city.
**************************************************************************/
void wonder_built(const struct city *pcity, const struct impr_type *pimprove)
{
  struct player *pplayer;
  int index = improvement_number(pimprove);

  RETURN_IF_FAIL(NULL != pcity);
  RETURN_IF_FAIL(is_wonder(pimprove));

  pplayer = city_owner(pcity);
  pplayer->wonders[index] = pcity->id;

  if (is_great_wonder(pimprove)) {
    game.info.great_wonder_owners[index] = player_number(pplayer);
  }
}

/**************************************************************************
  Remove a wonder from a city and destroy it if it's a great wonder.  To
  transfer a great wonder, use great_wonder_transfer.
**************************************************************************/
void wonder_destroyed(const struct city *pcity,
                      const struct impr_type *pimprove)
{
  struct player *pplayer;
  int index = improvement_number(pimprove);

  RETURN_IF_FAIL(NULL != pcity);
  RETURN_IF_FAIL(is_wonder(pimprove));

  pplayer = city_owner(pcity);
  RETURN_IF_FAIL(pplayer->wonders[index] == pcity->id);
  pplayer->wonders[index] = WONDER_NOT_BUILT;

  if (is_great_wonder(pimprove)) {
    RETURN_IF_FAIL(game.info.great_wonder_owners[index]
                   == player_number(pplayer));
    game.info.great_wonder_owners[index] = WONDER_DESTROYED;
  }
}

/**************************************************************************
  Returns whether the player has built this wonder (small or great).
**************************************************************************/
bool wonder_is_built(const struct player *pplayer,
                     const struct impr_type *pimprove)
{
  RETURN_VAL_IF_FAIL(NULL != pplayer, NULL);
  RETURN_VAL_IF_FAIL(is_wonder(pimprove), NULL);

  return WONDER_BUILT(pplayer->wonders[improvement_index(pimprove)]);
}

/**************************************************************************
  Get the world city with this wonder (small or great).  This doesn't
  always success on the client side.
**************************************************************************/
struct city *find_city_from_wonder(const struct player *pplayer,
                                   const struct impr_type *pimprove)
{
  int city_id = pplayer->wonders[improvement_index(pimprove)];

  RETURN_VAL_IF_FAIL(NULL != pplayer, NULL);
  RETURN_VAL_IF_FAIL(is_wonder(pimprove), NULL);

  if (!WONDER_BUILT(city_id)) {
    return NULL;
  }

#ifdef DEBUG
  if (is_server()) {
    /* On client side, this info is not always known. */
    struct city *pcity = player_find_city_by_id(pplayer, city_id);

    if (NULL == pcity) {
      freelog(LOG_ERROR, "Player %s (nb %d) has outdated wonder info for "
              "%s (nb %d), it points to city nb %d.",
              player_name(pplayer), player_number(pplayer),
              improvement_rule_name(pimprove), improvement_number(pimprove),
              pcity->id);
    } else if (!city_has_building(pcity, pimprove)) {
      freelog(LOG_ERROR, "Player %s (nb %d) has outdated wonder info for "
              "%s (nb %d), the city %s (nb %d) doesn't have this wonder.",
              player_name(pplayer), player_number(pplayer),
              improvement_rule_name(pimprove), improvement_number(pimprove),
              city_name(pcity), pcity->id);
      return NULL;
    }

    return pcity;
  }
#endif /* DEBUG */

  return player_find_city_by_id(pplayer, city_id);
}

/**************************************************************************
  Returns whether this wonder is currently built.
**************************************************************************/
bool great_wonder_is_built(const struct impr_type *pimprove)
{
  RETURN_VAL_IF_FAIL(is_great_wonder(pimprove), FALSE);

  return WONDER_OWNED(game.info.great_wonder_owners
                      [improvement_index(pimprove)]);
}

/**************************************************************************
  Returns whether this wonder has been destroyed.
**************************************************************************/
bool great_wonder_is_destroyed(const struct impr_type *pimprove)
{
  RETURN_VAL_IF_FAIL(is_great_wonder(pimprove), FALSE);

  return (WONDER_DESTROYED
          == game.info.great_wonder_owners[improvement_index(pimprove)]);
}

/**************************************************************************
  Returns whether this wonder can be currently built.
**************************************************************************/
bool great_wonder_is_available(const struct impr_type *pimprove)
{
  RETURN_VAL_IF_FAIL(is_great_wonder(pimprove), FALSE);

  return (WONDER_NOT_OWNED
          == game.info.great_wonder_owners[improvement_index(pimprove)]);
}

/**************************************************************************
  Get the world city with this great wonder.  This doesn't always success
  on the client side.
**************************************************************************/
struct city *find_city_from_great_wonder(const struct impr_type *pimprove)
{
  int player_id = game.info.great_wonder_owners[improvement_index(pimprove)];

  RETURN_VAL_IF_FAIL(is_great_wonder(pimprove), NULL);

  if (WONDER_OWNED(player_id)) {
#ifdef DEBUG
    const struct player *pplayer = player_by_number(player_id);
    struct city *pcity = find_city_from_wonder(pplayer, pimprove);

    if (is_server() && NULL == pcity) {
      freelog(LOG_ERROR, "Game has outdated wonder info for %s (nb %d), "
              "the player %s (nb %d) doesn't have this wonder.",
              improvement_rule_name(pimprove), improvement_number(pimprove),
              player_name(pplayer), player_number(pplayer));
    }

    return pcity;
#else
    return find_city_from_wonder(player_by_number(player_id), pimprove);
#endif /* DEBUG */
  } else {
    return NULL;
  }
}

/**************************************************************************
  Get the player owning this small wonder.  This doesn't always success on
  the client side.
**************************************************************************/
struct player *great_wonder_owner(const struct impr_type *pimprove)
{
  int player_id = game.info.great_wonder_owners[improvement_index(pimprove)];

  RETURN_VAL_IF_FAIL(is_great_wonder(pimprove), NULL);

  if (WONDER_OWNED(player_id)) {
    return player_by_number(player_id);
  } else {
    return NULL;
  }
}

/**************************************************************************
  Returns whether the player has built this small wonder.
**************************************************************************/
bool small_wonder_is_built(const struct player *pplayer,
                           const struct impr_type *pimprove)
{
  RETURN_VAL_IF_FAIL(is_small_wonder(pimprove), FALSE);

  return (NULL != pplayer
          && wonder_is_built(pplayer, pimprove));
}

/**************************************************************************
  Get the player city with this small wonder.
**************************************************************************/
struct city *find_city_from_small_wonder(const struct player *pplayer,
                                         const struct impr_type *pimprove)
{
  RETURN_VAL_IF_FAIL(is_small_wonder(pimprove), NULL);

  if (NULL == pplayer) {
    return NULL; /* Used in some places in the client. */
  } else {
    return find_city_from_wonder(pplayer, pimprove);
  }
}

/**************************************************************************
  Return TRUE iff the improvement can be sold.
**************************************************************************/
bool can_sell_building(struct impr_type *pimprove)
{
  return (valid_improvement(pimprove) && is_improvement(pimprove));
}

/****************************************************************************
  Return TRUE iff the city can sell the given improvement.
****************************************************************************/
bool can_city_sell_building(const struct city *pcity,
			    struct impr_type *pimprove)
{
  return (city_has_building(pcity, pimprove) && is_improvement(pimprove));
}

