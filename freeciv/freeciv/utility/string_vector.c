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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "mem.h"
#include "shared.h"
#include "support.h"

#include "string_vector.h"

/* The string vector structure. */
struct strvec {
  char **vec;
  size_t size;
};

/**************************************************************************
  Free a string.
**************************************************************************/
static void string_free(char *string)
{
  if (string) {
    free(string);
  }
}

/**************************************************************************
  Duplicate a string.
**************************************************************************/
static char *string_duplicate(const char *string)
{
  if (string) {
    return mystrdup(string);
  }
  return NULL;
}

/**************************************************************************
  Create a new string vector.
**************************************************************************/
struct strvec *strvec_new(void)
{
  struct strvec *psv = fc_malloc(sizeof(struct strvec));

  psv->vec = NULL;
  psv->size = 0;

  return psv;
}

/**************************************************************************
  Destroy a string vector.
**************************************************************************/
void strvec_destroy(struct strvec *psv)
{
  strvec_clear(psv);
  free(psv);
}

/**************************************************************************
  Set the size of the vector.
**************************************************************************/
void strvec_reserve(struct strvec *psv, size_t reserve)
{
  if (reserve == psv->size) {
    return;
  } else if (reserve == 0) {
    strvec_clear(psv);
    return;
  } else if (!psv->vec) {
    /* Initial reserve */
    psv->vec = fc_calloc(reserve, sizeof(char *));
  } else if (reserve > psv->size) {
    /* Expand the vector. */
    psv->vec = fc_realloc(psv->vec, reserve * sizeof(char *));
    memset(psv->vec + psv->size, 0, (reserve - psv->size) * sizeof(char *));
  } else {
    /* Shrink the vector: free the extra strings. */
    size_t i;

    for (i = psv->size - 1; i >= reserve; i--) {
      string_free(psv->vec[i]);
    }
    psv->vec = fc_realloc(psv->vec, reserve * sizeof(char *));
  }
  psv->size = reserve;
}

/**************************************************************************
  Stores the string vector from a normal vector. If size == -1, it will
  assume it is a NULL terminated vector.
**************************************************************************/
void strvec_store(struct strvec *psv, const char *const *vec, size_t size)
{
  if (size == -1) {
    strvec_clear(psv);
    for (; *vec; vec++) {
      strvec_append(psv, *vec);
    }
  } else {
    size_t i;

    strvec_reserve(psv, size);
    for (i = 0; i < size; i++, vec++) {
      strvec_set(psv, i, *vec);
    }
  }
}

/**************************************************************************
  Remove all strings from the vector.
**************************************************************************/
void strvec_clear(struct strvec *psv)
{
  size_t i;
  char **p;

  if (!psv->vec) {
    return;
  }

  for (i = 0, p = psv->vec; i < psv->size; i++, p++) {
    string_free(*p);
  }
  free(psv->vec);
  psv->vec = NULL;
  psv->size = 0;
}

/**************************************************************************
  Remove all empty strings from the vector and removes all leading and
  trailing spaces.
**************************************************************************/
void strvec_remove_empty(struct strvec *psv)
{
  size_t i;
  char *str;

  if (!psv->vec) {
    return;
  }

  for (i = 0; i < psv->size;) {
    str = psv->vec[i];

    if (!str) {
      strvec_remove(psv, i);
      continue;
    }

    remove_leading_trailing_spaces(str);
    if (str[0] == '\0') {
      strvec_remove(psv, i);
      continue;
    }

    i++;
  }
}

/**************************************************************************
  Copy a string vector.
**************************************************************************/
void strvec_copy(struct strvec *dest, const struct strvec *src)
{
  size_t i;
  char **p;
  char *const *l;

  if (!src->vec) {
    strvec_clear(dest);
    return;
  }

  strvec_reserve(dest, src->size);
  for (i = 0, p = dest->vec, l = src->vec; i < dest->size; i++, p++, l++) {
    string_free(*p);
    *p = string_duplicate(*l);
  }
}

/**************************************************************************
  Insert a string at the start of the vector.
**************************************************************************/
void strvec_prepend(struct strvec *psv, const char *string)
{
  strvec_reserve(psv, psv->size + 1);
  memmove(psv->vec + 1, psv->vec, (psv->size - 1) * sizeof(char *));
  psv->vec[0] = string_duplicate(string);
}

/**************************************************************************
  Insert a string at the end of the vector.
**************************************************************************/
void strvec_append(struct strvec *psv, const char *string)
{
  strvec_reserve(psv, psv->size + 1);
  psv->vec[psv->size - 1] = string_duplicate(string);
}

/**************************************************************************
  Insert a string at the index of the vector.
**************************************************************************/
void strvec_insert(struct strvec *psv, size_t index, const char *string)
{
  if (index <= 0) {
    strvec_prepend(psv, string);
  } else if (index >= psv->size) {
    strvec_append(psv, string);
  } else {
    strvec_reserve(psv, psv->size + 1);
    memmove(psv->vec + index + 1, psv->vec + index,
            (psv->size - index - 1) * sizeof(char *));
    psv->vec[index] = string_duplicate(string);
  }
}

/**************************************************************************
  Replace a string at the index of the vector.
  Returns TRUE if the element has been really set.
**************************************************************************/
bool strvec_set(struct strvec *psv, size_t index, const char *string)
{
  if (strvec_index_valid(psv, index)) {
    string_free(psv->vec[index]);
    psv->vec[index] = string_duplicate(string);
    return TRUE;
  }
  return FALSE;
}

/**************************************************************************
  Remove the string at the index from the vector.
  Returns TRUE if the element has been really removed.
**************************************************************************/
bool strvec_remove(struct strvec *psv, size_t index)
{
  if (!strvec_index_valid(psv, index)) {
    return FALSE;
  }

  if (psv->size == 1) {
    /* It is the last. */
    strvec_clear(psv);
    return TRUE;
  }

  string_free(psv->vec[index]);
  memmove(psv->vec + index, psv->vec + index + 1,
          (psv->size - index - 1) * sizeof(char *));
  psv->vec[psv->size - 1] = NULL; /* Do not attempt to free this data. */
  strvec_reserve(psv, psv->size - 1);

  return TRUE;
}

/**************************************************************************
  Returns the size of the vector.
**************************************************************************/
size_t strvec_size(const struct strvec *psv)
{
  return psv->size;
}

/**************************************************************************
  Returns the datas of the vector.
**************************************************************************/
const char *const *strvec_data(const struct strvec *psv)
{
  return (const char **) psv->vec;
}

/**************************************************************************
  Returns TRUE if the index is valid.
**************************************************************************/
bool strvec_index_valid(const struct strvec *psv, size_t index)
{
  return index >= 0 && index < psv->size;
}

/**************************************************************************
  Returns the string at the index of the vector.
**************************************************************************/
const char *strvec_get(const struct strvec *psv, size_t index)
{
  return strvec_index_valid(psv, index) ? psv->vec[index] : NULL;
}
