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
#ifndef FC__STRING_VECTOR_H
#define FC__STRING_VECTOR_H

#include "shared.h"             /* bool type. */

struct strvec;                  /* Opaque. */

struct strvec *strvec_new(void);
void strvec_destroy(struct strvec *psv);

void strvec_reserve(struct strvec *psv, size_t reserve);
void strvec_store(struct strvec *psv, const char *const *vec, size_t size);
void strvec_clear(struct strvec *psv);
void strvec_remove_empty(struct strvec *psv);
void strvec_copy(struct strvec *dest, const struct strvec *src);

void strvec_prepend(struct strvec *psv, const char *string);
void strvec_append(struct strvec *psv, const char *string);
void strvec_insert(struct strvec *psv, size_t index, const char *string);
bool strvec_set(struct strvec *psv, size_t index, const char *string);
bool strvec_remove(struct strvec *psv, size_t index);

size_t strvec_size(const struct strvec *psv);
const char *const *strvec_data(const struct strvec *psv);
bool strvec_index_valid(const struct strvec *psv, size_t index);
const char *strvec_get(const struct strvec *psv, size_t index);

/* Iteration macro. */
#define strvec_iterate(psv, str) \
{ \
  size_t _id; \
  const char *str; \
  for (_id = 0; _id < strvec_size((psv)); _id++) { \
    str = strvec_get((psv), _id);

#define strvec_iterate_end \
  } \
}

#endif /* FC__STRING_VECTOR_H */
