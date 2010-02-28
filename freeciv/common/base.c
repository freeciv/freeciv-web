/****************************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

/* utility */
#include "fcintl.h"
#include "log.h"

/* common */
#include "game.h"
#include "tile.h"
#include "unit.h"

#include "base.h"

static struct base_type base_types[MAX_BASE_TYPES];

static const char *base_type_flag_names[] = {
  "NoAggressive", "NoStackDeath",
  "DiplomatDefense", "ParadropFrom", "NativeTile"
};

/* This must correspond to enum base_gui_type in base.h */
static const char *base_gui_type_names[] = {
  "Fortress", "Airbase", "Other"
};

/****************************************************************************
  Check if base provides effect
****************************************************************************/
bool base_has_flag(const struct base_type *pbase, enum base_flag_id flag)
{
  return BV_ISSET(pbase->flags, flag);
}

/****************************************************************************
  Is base native to unit class?
****************************************************************************/
bool is_native_base_to_uclass(const struct base_type *pbase,
                              const struct unit_class *pclass)
{
  return BV_ISSET(pbase->native_to, uclass_index(pclass));
}

/****************************************************************************
  Is base native to unit type?
****************************************************************************/
bool is_native_base_to_utype(const struct base_type *pbase,
                             const struct unit_type *punittype)
{
  return is_native_base_to_uclass(pbase, utype_class(punittype));
}

/****************************************************************************
  Base provides base flag for unit? Checks if base provides flag and if
  base is native to unit.
****************************************************************************/
bool base_has_flag_for_utype(const struct base_type *pbase,
                             enum base_flag_id flag,
                             const struct unit_type *punittype)
{
  return base_has_flag(pbase, flag) && is_native_base_to_utype(pbase, punittype);
}

/**************************************************************************
  Return the (translated) name of the base type.
  You don't have to free the return pointer.
**************************************************************************/
const char *base_name_translation(struct base_type *pbase)
{
  if (NULL == pbase->name.translated) {
    /* delayed (unified) translation */
    pbase->name.translated = ('\0' == pbase->name.vernacular[0])
			      ? pbase->name.vernacular
			      : Q_(pbase->name.vernacular);
  }
  return pbase->name.translated;
}

/**************************************************************************
  Return the (untranslated) rule name of the base type.
  You don't have to free the return pointer.
**************************************************************************/
const char *base_rule_name(const struct base_type *pbase)
{
  return Qn_(pbase->name.vernacular);
}

/**************************************************************************
  Returns base type matching rule name or NULL if there is no base type
  with such name.
**************************************************************************/
struct base_type *find_base_type_by_rule_name(const char *name)
{
  const char *qs = Qn_(name);

  base_type_iterate(pbase) {
    if (!mystrcasecmp(base_rule_name(pbase), qs)) {
      return pbase;
    }
  } base_type_iterate_end;

  return NULL;
}

/****************************************************************************
  Is there base of the given type near tile?
****************************************************************************/
bool is_base_near_tile(const struct tile *ptile, const struct base_type *pbase)
{
  adjc_iterate(ptile, adjc_tile) {
    if (tile_has_base(adjc_tile, pbase)) {
      return TRUE;
    }
  } adjc_iterate_end;

  return FALSE;
}

/**************************************************************************
  Can unit build base to given tile?
**************************************************************************/
bool can_build_base(const struct unit *punit, const struct base_type *pbase,
                    const struct tile *ptile)
{
  if (tile_city(ptile)) {
    /* Bases cannot be built inside cities */
    return FALSE;
  }

  if (!pbase->buildable) {
    /* Base type not buildable. */
    return FALSE;
  }

  if (tile_has_base(ptile, pbase)) {
    /* Exist already */
    return FALSE;
  }

  return are_reqs_active(unit_owner(punit), NULL, NULL, ptile,
                         unit_type(punit), NULL, NULL, &pbase->reqs,
                         RPT_CERTAIN);
}

/**************************************************************************
  Convert base flag names to enum; case insensitive;
  returns BF_LAST if can't match.
**************************************************************************/
enum base_flag_id base_flag_from_str(const char *s)
{
  enum base_flag_id i;
  
  assert(ARRAY_SIZE(base_type_flag_names) == BF_LAST);
  
  for(i = 0; i < BF_LAST; i++) {
    if (mystrcasecmp(base_type_flag_names[i], s)==0) {
      return i;
    }
  }
  return BF_LAST;
}
  
/****************************************************************************
  Returns base_type entry for an ID value.
****************************************************************************/
struct base_type *base_by_number(const Base_type_id id)
{
  if (id < 0 || id >= game.control.num_base_types) {
    return NULL;
  }
  return &base_types[id];
}

/**************************************************************************
  Return the base index.
**************************************************************************/
Base_type_id base_number(const struct base_type *pbase)
{
  assert(pbase);
  return pbase->item_number;
}

/**************************************************************************
  Return the base index.

  Currently same as base_number(), paired with base_count()
  indicates use as an array index.
**************************************************************************/
Base_type_id base_index(const struct base_type *pbase)
{
  assert(pbase);
  return pbase - base_types;
}

/**************************************************************************
  Return the number of base_types.
**************************************************************************/
Base_type_id base_count(void)
{
  return game.control.num_base_types;
}

/**************************************************************************
  Return the last item of base_types.
**************************************************************************/
const struct base_type *base_array_last(void)
{
  if (game.control.num_base_types > 0) {
    return &base_types[game.control.num_base_types - 1];
  }
  return NULL;
}

/**************************************************************************
  Return the first item of base_types.
**************************************************************************/
struct base_type *base_array_first(void)
{
  if (game.control.num_base_types > 0) {
    return base_types;
  }
  return NULL;
}

/****************************************************************************
  Initialize base_type structures.
****************************************************************************/
void base_types_init(void)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(base_types); i++) {
    base_types[i].item_number = i;
    requirement_vector_init(&base_types[i].reqs);
  }
}

/****************************************************************************
  Free the memory associated with base types
****************************************************************************/
void base_types_free(void)
{
  base_type_iterate(pbase) {
    requirement_vector_free(&pbase->reqs);
  } base_type_iterate_end;
}

/**************************************************************************
  Convert base gui type names to enum; case insensitive;
  returns BASE_GUI_LAST if can't match.
**************************************************************************/
enum base_gui_type base_gui_type_from_str(const char *s)
{
  enum base_gui_type i;
  
  assert(ARRAY_SIZE(base_gui_type_names) == BASE_GUI_LAST);
  
  for(i = 0; i < BASE_GUI_LAST; i++) {
    if (mystrcasecmp(base_gui_type_names[i], s)==0) {
      return i;
    }
  }
  return BASE_GUI_LAST;
}

/**************************************************************************
  Get best gui_type base for given parameters
**************************************************************************/
struct base_type *get_base_by_gui_type(enum base_gui_type type,
                                       const struct unit *punit,
                                       const struct tile *ptile)
{
  base_type_iterate(pbase) {
    if (type == pbase->gui_type
        && (!punit || can_build_base(punit, pbase, ptile))) {
      return pbase;
    }
  } base_type_iterate_end;

  return NULL;
}

/**************************************************************************
  Can two bases coexist in same tile?
**************************************************************************/
bool can_bases_coexist(const struct base_type *base1, const struct base_type *base2)
{
  if (base1 == base2) {
    return TRUE;
  }

  return !BV_ISSET(base1->conflicts, base_index(base2));
}

/**************************************************************************
  Does this base type claim territory?
**************************************************************************/
bool territory_claiming_base(const struct base_type *pbase)
{
  return pbase->border_sq >= 0;
}
