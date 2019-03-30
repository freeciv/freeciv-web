/***********************************************************************
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

#ifndef FC__SPECIALIST_H
#define FC__SPECIALIST_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "shared.h"

/* common */
#include "fc_types.h"
#include "name_translation.h"
#include "requirements.h"

struct specialist {
  int item_number;
  struct name_translation name;
  struct name_translation abbreviation;
  bool ruledit_disabled;

  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];

  struct requirement_vector reqs;

  struct strvec *helptext;
};

#define DEFAULT_SPECIALIST default_specialist

extern struct specialist specialists[SP_MAX];
extern int default_specialist;

/* General specialist accessor functions. */
Specialist_type_id specialist_count(void);
Specialist_type_id specialist_index(const struct specialist *sp);
Specialist_type_id specialist_number(const struct specialist *sp);

struct specialist *specialist_by_number(const Specialist_type_id id);
struct specialist *specialist_by_rule_name(const char *name);
struct specialist *specialist_by_translated_name(const char *name);

const char *specialist_rule_name(const struct specialist *sp);
const char *specialist_plural_translation(const struct specialist *sp);
const char *specialist_abbreviation_translation(const struct specialist *sp);

/* Ancillary routines */
const char *specialists_abbreviation_string(void);
const char *specialists_string(const citizens *specialist_list);

int get_specialist_output(const struct city *pcity,
                          Specialist_type_id sp, Output_type_id otype);

/* Initialization and iteration */
void specialists_init(void);
void specialists_free(void);

/* usually an index to arrays */
#define specialist_type_iterate(sp)					    \
{									    \
  Specialist_type_id sp;						    \
                                                                            \
  for (sp = 0; sp < specialist_count(); sp++) {

#define specialist_type_iterate_end                                         \
  }                                                                         \
}

#define specialist_type_re_active_iterate(_p)                               \
  specialist_type_iterate(_p##_) {                                          \
    struct specialist *_p = specialist_by_number(_p##_);                    \
    if (!_p->ruledit_disabled) {

#define specialist_type_re_active_iterate_end                               \
    }                                                                       \
  } specialist_type_iterate_end;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__SPECIALIST_H */
