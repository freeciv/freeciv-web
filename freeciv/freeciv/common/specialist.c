/********************************************************************** 
 Freeciv - Copyright (C) 2005 - The Freeciv Project
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

#include "city.h"
#include "effects.h"
#include "fcintl.h"
#include "game.h"
#include "specialist.h"

struct specialist specialists[SP_MAX];
int default_specialist;

/****************************************************************************
  Initialize data for specialists.
****************************************************************************/
void specialists_init(void)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(specialists); i++) {
    struct specialist *p = &specialists[i];

    p->item_number = i;

    requirement_vector_init(&p->reqs);
  }
}

/****************************************************************************
  Free data for specialists.
****************************************************************************/
void specialists_free(void)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(specialists); i++) {
    struct specialist *p = &specialists[i];

    requirement_vector_free(&p->reqs);
  }
}

/**************************************************************************
  Return the number of specialist_types.
**************************************************************************/
Specialist_type_id specialist_count(void)
{
  return game.control.num_specialist_types;
}

/****************************************************************************
  Return the specialist index.

  Currently same as specialist_number(), paired with specialist_count()
  indicates use as an array index.
****************************************************************************/
Specialist_type_id specialist_index(const struct specialist *sp)
{
  assert(sp);
  return sp - specialists;
}

/****************************************************************************
  Return the specialist item_number.
****************************************************************************/
Specialist_type_id specialist_number(const struct specialist *sp)
{
  assert(sp);
  return sp->item_number;
}

/****************************************************************************
  Return the specialist pointer for the given number.
****************************************************************************/
struct specialist *specialist_by_number(const Specialist_type_id id)
{
  if (id < 0 || id >= game.control.num_specialist_types) {
    return NULL;
  }
  return &specialists[id];
}

/****************************************************************************
  Return the specialist type with the given (untranslated!) rule name.
  Returns NULL if none match.
****************************************************************************/
struct specialist *find_specialist_by_rule_name(const char *name)
{
  const char *qname = Qn_(name);

  specialist_type_iterate(i) {
    struct specialist *sp = specialist_by_number(i);
    if (0 == mystrcasecmp(specialist_rule_name(sp), qname)) {
      return sp;
    }
  } specialist_type_iterate_end;

  return NULL;
}

/**************************************************************************
  Return the (untranslated) rule name of the specialist type.
  You don't have to free the return pointer.
**************************************************************************/
const char *specialist_rule_name(const struct specialist *sp)
{
  return Qn_(sp->name.vernacular);
}

/**************************************************************************
  Return the (translated) name of the specialist type.
  You don't have to free the return pointer.
**************************************************************************/
const char *specialist_name_translation(struct specialist *sp)
{
  if (NULL == sp->name.translated) {
    /* delayed (unified) translation */
    sp->name.translated = ('\0' == sp->name.vernacular[0])
			  ? sp->name.vernacular
			  : Q_(sp->name.vernacular);
  }
  return sp->name.translated;
}

/**************************************************************************
  Return the (translated) abbreviation of the specialist type.
  You don't have to free the return pointer.
**************************************************************************/
const char *specialist_abbreviation_translation(struct specialist *sp)
{
  if (NULL == sp->abbreviation.translated) {
    /* delayed (unified) translation */
    sp->abbreviation.translated = ('\0' == sp->abbreviation.vernacular[0])
				  ? sp->abbreviation.vernacular
				  : Q_(sp->abbreviation.vernacular);
  }
  return sp->abbreviation.translated;
}

/****************************************************************************
  Return a string showing the number of specialists in the array.

  For instance with a city with (0,3,1) specialists call

    specialists_string(pcity->specialists);

  and you'll get "0/3/1".
****************************************************************************/
const char *specialists_string(const int *specialists)
{
  static char buf[5 * SP_MAX];

  buf[0] = '\0';

  specialist_type_iterate(sp) {
    char *separator = (buf[0] == '\0') ? "" : "/";

    cat_snprintf(buf, sizeof(buf), "%s%d", separator, specialists[sp]);
  } specialist_type_iterate_end;

  return buf;
}

/****************************************************************************
  Return the output for the specialist type with this output type.
****************************************************************************/
int get_specialist_output(const struct city *pcity,
			  Specialist_type_id sp, Output_type_id otype)
{
  struct specialist *pspecialist = &specialists[sp];
  struct output_type *poutput = get_output_type(otype);

  return get_city_specialist_output_bonus(pcity, pspecialist, poutput,
					  EFT_SPECIALIST_OUTPUT);
}
