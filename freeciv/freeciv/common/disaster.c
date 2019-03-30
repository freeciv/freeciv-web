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
#include <fc_config.h>
#endif

/* utility */
#include "fcintl.h"

/* common */
#include "fc_types.h"
#include "game.h"
#include "name_translation.h"

#include "disaster.h"

static struct disaster_type disaster_types[MAX_DISASTER_TYPES];

/************************************************************************//**
  Initialize disaster_type structures.
****************************************************************************/
void disaster_types_init(void)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(disaster_types); i++) {
    disaster_types[i].id = i;
    requirement_vector_init(&disaster_types[i].reqs);
  }
}

/************************************************************************//**
  Free the memory associated with disaster types
****************************************************************************/
void disaster_types_free(void)
{
  disaster_type_iterate(pdis) {
    requirement_vector_free(&pdis->reqs);
  } disaster_type_iterate_end;
}

/************************************************************************//**
  Return the disaster id.
****************************************************************************/
Disaster_type_id disaster_number(const struct disaster_type *pdis)
{
  fc_assert_ret_val(NULL != pdis, -1);

  return pdis->id;
}

/************************************************************************//**
  Return the disaster index.

  Currently same as disaster_number(), paired with disaster_count()
  indicates use as an array index.
****************************************************************************/
Disaster_type_id disaster_index(const struct disaster_type *pdis)
{
  fc_assert_ret_val(NULL != pdis, -1);

  return pdis - disaster_types;
}

/************************************************************************//**
  Return the number of disaster_types.
****************************************************************************/
Disaster_type_id disaster_count(void)
{
  return game.control.num_disaster_types;
}

/************************************************************************//**
  Return disaster type of given id.
****************************************************************************/
struct disaster_type *disaster_by_number(Disaster_type_id id)
{
  fc_assert_ret_val(id >= 0 && id < game.control.num_disaster_types, NULL);

  return &disaster_types[id];
}

/************************************************************************//**
  Return translated name of this disaster type.
****************************************************************************/
const char *disaster_name_translation(struct disaster_type *pdis)
{
  return name_translation_get(&pdis->name);
}

/************************************************************************//**
  Return untranslated name of this disaster type.
****************************************************************************/
const char *disaster_rule_name(struct disaster_type *pdis)
{
  return rule_name_get(&pdis->name);
}

/************************************************************************//**
  Check if disaster provides effect
****************************************************************************/
bool disaster_has_effect(const struct disaster_type *pdis,
                         enum disaster_effect_id effect)
{
  return BV_ISSET(pdis->effects, effect);
}

/************************************************************************//**
  Whether disaster can happen in given city.
****************************************************************************/
bool can_disaster_happen(const struct disaster_type *pdis,
                         const struct city *pcity)
{
  return are_reqs_active(city_owner(pcity), NULL, pcity, NULL,
                         city_tile(pcity), NULL, NULL, NULL, NULL, NULL,
                         &pdis->reqs, RPT_POSSIBLE);
}
