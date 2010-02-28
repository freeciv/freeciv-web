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

#ifndef FC__SPECIALIST_H
#define FC__SPECIALIST_H

#include "shared.h"

#include "fc_types.h"
#include "requirements.h"

struct specialist {
  int item_number;
  struct name_translation name;
  struct name_translation abbreviation;

  struct requirement_vector reqs;
};

#define DEFAULT_SPECIALIST default_specialist

extern struct specialist specialists[SP_MAX];
extern int default_specialist;

/* General specialist accessor functions. */
Specialist_type_id specialist_count(void);
Specialist_type_id specialist_index(const struct specialist *sp);
Specialist_type_id specialist_number(const struct specialist *sp);

struct specialist *specialist_by_number(const Specialist_type_id id);
struct specialist *find_specialist_by_rule_name(const char *name);

const char *specialist_rule_name(const struct specialist *sp);
const char *specialist_name_translation(struct specialist *sp);
const char *specialist_abbreviation_translation(struct specialist *sp);

/* Ancillary routines */
const char *specialists_string(const int *specialists);

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

#endif /* FC__SPECIALIST_H */
