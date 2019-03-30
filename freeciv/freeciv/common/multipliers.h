/***********************************************************************
 Freeciv - Copyright (C) 2014 - Sławomir Lach
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__MULTIPLIERS_H
#define FC__MULTIPLIERS_H

#ifdef __cplusplus
extern "C" {
#endif

/* utility */
#include "bitvector.h"
#include "string_vector.h"

/* common */
#include "fc_types.h"
#include "name_translation.h"
#include "requirements.h"

struct multiplier
{
  Multiplier_type_id id;
  struct name_translation name;
  bool ruledit_disabled; /* Does not really exist - hole in multipliers array */
  int start; /* display units */
  int stop;  /* display units */
  int step;  /* display units */
  int def;   /* default value, in display units */
  int offset;
  int factor;
  struct requirement_vector reqs;
  struct strvec *helptext;
};

void multipliers_init(void);
void multipliers_free(void);

Multiplier_type_id multiplier_count(void);
Multiplier_type_id multiplier_index(const struct multiplier *pmul);
Multiplier_type_id multiplier_number(const struct multiplier *pmul);

struct multiplier *multiplier_by_number(Multiplier_type_id id);

const char *multiplier_name_translation(const struct multiplier *pmul);
const char *multiplier_rule_name(const struct multiplier *pmul);
struct multiplier *multiplier_by_rule_name(const char *name);

bool multiplier_can_be_changed(struct multiplier *pmul, struct player *pplayer);

#define multipliers_iterate(_mul_) \
{      \
    Multiplier_type_id _i; \
       for (_i = 0; _i < multiplier_count(); _i++) { \
        struct multiplier *_mul_ = multiplier_by_number(_i);

#define multipliers_iterate_end \
      }  \
}

#define multipliers_re_active_iterate(_mul_) \
  multipliers_iterate(_mul_) {               \
    if (!_mul_->ruledit_disabled) {

#define multipliers_re_active_iterate_end    \
    }                                        \
  } multipliers_iterate_end;

#ifdef __cplusplus
}
#endif

#endif /* FC__MULTIPLIERS_H */
