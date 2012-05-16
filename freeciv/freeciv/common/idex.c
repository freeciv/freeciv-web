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

/**************************************************************************
   idex = ident index: a lookup table for quick mapping of unit and city
   id values to unit and city pointers.

   Method: use separate hash tables for each type.
   Means code duplication for city/unit cases, but simplicity advantages.
   Don't have to manage memory at all: store pointers to unit and city
   structs allocated elsewhere, and keys are pointers to id values inside
   the structs.

   Note id values should probably be unsigned int: here leave as plain int
   so can use pointers to pcity->id etc.

   On probable errors, print LOG_ERROR messages and persevere,
   unless IDEX_DIE set.
***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>

#include "city.h"
#include "hash.h"
#include "log.h"
#include "unit.h"

#include "idex.h"


#ifndef IDEX_DIE
#define IDEX_DIE FALSE
#endif
#define LOG_IDEX_ERR (IDEX_DIE ? LOG_FATAL : LOG_ERROR)


/* "Global" data: */
static struct hash_table *idex_city_hash = NULL;
static struct hash_table *idex_unit_hash = NULL;

/**************************************************************************
   Initialize.  Should call this at the start before use.
***************************************************************************/
void idex_init(void)
{
  assert(idex_city_hash == NULL);
  assert(idex_unit_hash == NULL);

  idex_city_hash = hash_new(hash_fval_int, hash_fcmp_int);
  idex_unit_hash = hash_new(hash_fval_int, hash_fcmp_int);
}

/**************************************************************************
   Free the hashs.
***************************************************************************/
void idex_free(void)
{
  hash_free(idex_city_hash);
  idex_city_hash = NULL;

  hash_free(idex_unit_hash);
  idex_unit_hash = NULL;
}

/**************************************************************************
   Register a city into idex, with current pcity->id.
   Call this when pcity created.
***************************************************************************/
void idex_register_city(struct city *pcity)
{
  struct city *old = (struct city *)
    hash_replace(idex_city_hash, &pcity->id, pcity);
  if (old) {
    /* error */
    freelog(LOG_IDEX_ERR, "IDEX: city collision: new %d %p %s, old %d %p %s",
	    pcity->id, (void*)pcity, city_name(pcity),
	    old->id, (void*)old, city_name(old));
    if (IDEX_DIE) {
      die("byebye");
    }
  }
}

/**************************************************************************
   Register a unit into idex, with current punit->id.
   Call this when punit created.
***************************************************************************/
void idex_register_unit(struct unit *punit)
{
  struct unit *old = (struct unit *)
    hash_replace(idex_unit_hash, &punit->id, punit);
  if (old) {
    /* error */
    freelog(LOG_IDEX_ERR, "IDEX: unit collision: new %d %p %s, old %d %p %s",
	    punit->id, (void*)punit,
	    unit_rule_name(punit),
	    old->id, (void*)old,
	    unit_rule_name(old));
    if (IDEX_DIE) {
      die("byebye");
    }
  }
}

/**************************************************************************
   Remove a city from idex, with current pcity->id.
   Call this when pcity deleted.
***************************************************************************/
void idex_unregister_city(struct city *pcity)
{
  struct city *old = (struct city *)
    hash_delete_entry(idex_city_hash, &pcity->id);
  if (!old) {
    /* error */
    freelog(LOG_IDEX_ERR, "IDEX: city unreg missing: %d %p %s",
	    pcity->id, (void*)pcity, city_name(pcity));
    if (IDEX_DIE) {
      die("byebye");
    }
  } else if (old != pcity) {
    /* error */
    freelog(LOG_IDEX_ERR,
	    "IDEX: city unreg mismatch: unreg %d %p %s, old %d %p %s",
	    pcity->id, (void*)pcity, city_name(pcity),
	    old->id, (void*)old, city_name(old));
    if (IDEX_DIE) {
      die("byebye");
    }
  }
}

/**************************************************************************
   Remove a unit from idex, with current punit->id.
   Call this when punit deleted.
***************************************************************************/
void idex_unregister_unit(struct unit *punit)
{
  struct unit *old = (struct unit *)
    hash_delete_entry(idex_unit_hash, &punit->id);
  if (!old) {
    /* error */
    freelog(LOG_IDEX_ERR, "IDEX: unit unreg missing: %d %p %s",
	    punit->id, (void*)punit,
	    unit_rule_name(punit));
    if (IDEX_DIE) {
      die("byebye");
    }
  } else if (old != punit) {
    /* error */
    freelog(LOG_IDEX_ERR,
	    "IDEX: unit unreg mismatch: unreg %d %p %s, old %d %p %s",
	    punit->id, (void*)punit,
	    unit_rule_name(punit),
	    old->id, (void*)old,
	    unit_rule_name(old));
    if (IDEX_DIE) {
      die("byebye");
    }
  }
}

/**************************************************************************
   Lookup city with given id.
   Returns NULL if the city is not registered (which is not an error).
***************************************************************************/
struct city *idex_lookup_city(int id)
{
  return (struct city *)hash_lookup_data(idex_city_hash, &id);
}

/**************************************************************************
   Lookup unit with given id.
   Returns NULL if the unit is not registered (which is not an error).
***************************************************************************/
struct unit *idex_lookup_unit(int id)
{
  return (struct unit *)hash_lookup_data(idex_unit_hash, &id);
}
