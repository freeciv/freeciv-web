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
#ifndef FC__BASE_H
#define FC__BASE_H

#include "fc_types.h"
#include "requirements.h"

/* This must correspond to base_gui_type_names[] in base.c */
enum base_gui_type {
  BASE_GUI_FORTRESS = 0,
  BASE_GUI_AIRBASE,
  BASE_GUI_OTHER,
  BASE_GUI_LAST
};

enum base_flag_id {
  BF_NOT_AGGRESSIVE = 0, /* Unit inside are not considered aggressive
                          * if base is close to city */
  BF_NO_STACK_DEATH,     /* Units inside will not die all at once */
  BF_DIPLOMAT_DEFENSE,   /* Base provides bonus for defending diplomat */
  BF_PARADROP_FROM,      /* Paratroopers can use base for paradrop */
  BF_NATIVE_TILE,        /* Makes tile native terrain for units */
  BF_LAST                /* This has to be last */
};

BV_DEFINE(bv_base_flags, BF_LAST);

struct base_type {
  Base_type_id item_number;
  bool buildable;
  bool pillageable;
  struct name_translation name;
  char graphic_str[MAX_LEN_NAME];
  char graphic_alt[MAX_LEN_NAME];
  char activity_gfx[MAX_LEN_NAME];
  struct requirement_vector reqs;
  enum base_gui_type gui_type;
  int build_time;
  int defense_bonus;
  int border_sq;
  int vision_main_sq;
  int vision_invis_sq;

  bv_unit_classes native_to;
  bv_base_flags flags;
  bv_bases conflicts;
};


/* General base accessor functions. */
Base_type_id base_count(void);
Base_type_id base_index(const struct base_type *pbase);
Base_type_id base_number(const struct base_type *pbase);

struct base_type *base_by_number(const Base_type_id id);

const char *base_rule_name(const struct base_type *pbase);
const char *base_name_translation(struct base_type *pbase);

struct base_type *find_base_type_by_rule_name(const char *name);

bool is_base_near_tile(const struct tile *ptile, const struct base_type *pbase);

/* Functions to operate on a base flag. */
bool base_has_flag(const struct base_type *pbase, enum base_flag_id flag);
bool base_has_flag_for_utype(const struct base_type *pbase,
                             enum base_flag_id flag,
                             const struct unit_type *punittype);
bool is_native_base_to_uclass(const struct base_type *pbase,
                              const struct unit_class *pclass);
bool is_native_base_to_utype(const struct base_type *pbase,
                             const struct unit_type *punittype);

enum base_flag_id base_flag_from_str(const char *s);

/* Ancillary functions */
bool can_build_base(const struct unit *punit, const struct base_type *pbase,
                    const struct tile *ptile);

enum base_gui_type base_gui_type_from_str(const char *s);
struct base_type *get_base_by_gui_type(enum base_gui_type type,
                                       const struct unit *punit,
                                       const struct tile *ptile);

bool can_bases_coexist(const struct base_type *base1, const struct base_type *base2);

bool territory_claiming_base(const struct base_type *pbase);

/* Initialization and iteration */
void base_types_init(void);
void base_types_free(void);

struct base_type *base_array_first(void);
const struct base_type *base_array_last(void);

#define base_type_iterate(_p)						\
{									\
  struct base_type *_p = base_array_first();				\
  if (NULL != _p) {							\
    for (; _p <= base_array_last(); _p++) {

#define base_type_iterate_end						\
    }									\
  }									\
}


#endif  /* FC__BASE_H */
