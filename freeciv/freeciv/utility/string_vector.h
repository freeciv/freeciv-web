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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "support.h"            /* bool type. */

struct astring;

struct strvec;                  /* Opaque. */

struct strvec *strvec_new(void);
void strvec_destroy(struct strvec *psv);

void strvec_reserve(struct strvec *psv, size_t reserve);
void strvec_store(struct strvec *psv, const char *const *vec, size_t size);
void strvec_from_str(struct strvec *psv,  char separator, const char *str);
void strvec_clear(struct strvec *psv);
void strvec_remove_empty(struct strvec *psv);
void strvec_remove_duplicate(struct strvec *psv,
                             int (*cmp_func) (const char *, const char *));
void strvec_copy(struct strvec *dest, const struct strvec *src);
void strvec_sort(struct strvec *psv, int (*sort_func) (const char *const *,
                                                       const char *const *));

void strvec_prepend(struct strvec *psv, const char *string);
void strvec_append(struct strvec *psv, const char *string);
void strvec_insert(struct strvec *psv, size_t svindex, const char *string);
bool strvec_set(struct strvec *psv, size_t svindex, const char *string);
bool strvec_remove(struct strvec *psv, size_t svindex);

size_t strvec_size(const struct strvec *psv);
bool are_strvecs_equal(const struct strvec *stv1,
                       const struct strvec *stv2);
const char *const *strvec_data(const struct strvec *psv);
bool strvec_index_valid(const struct strvec *psv, size_t svindex);
const char *strvec_get(const struct strvec *psv, size_t svindex);
void strvec_to_str(const struct strvec *psv, char separator,
                   char *buf, size_t buf_len);
const char *strvec_to_or_list(const struct strvec *psv,
                              struct astring *astr);
const char *strvec_to_and_list(const struct strvec *psv,
                               struct astring *astr);

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

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__STRING_VECTOR_H */
