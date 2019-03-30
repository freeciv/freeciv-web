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

/***********************************************************************
  Allocated/allocatable strings
  original author: David Pfitzner <dwp@mso.anu.edu.au>

  A common technique is to have some memory dynamically allocated
  (using malloc etc), to avoid compiled-in limits, but only allocate
  enough space as initially needed, and then realloc later if/when
  require more space.  Typically, the realloc is made a bit more than
  immediately necessary, to avoid frequent reallocs if the object
  grows incrementally.  Also, don't usually realloc at all if the
  object shrinks.  This is straightforward, but just requires a bit
  of book-keeping to keep track of how much has been allocated etc.
  This module provides some tools to make this a bit easier.

  This is deliberately simple and light-weight.  The user is allowed
  full access to the struct elements rather than use accessor
  functions etc.

  Note one potential hazard: when the size is increased (astr_reserve()),
  realloc (really fc_realloc) is used, which retains any data which
  was there previously, _but_: any external pointers into the allocated
  memory may then become wild.  So you cannot safely use such external
  pointers into the astring data, except strictly between times when
  the astring size may be changed.

  There are two ways of getting the resulting string as a char *:

   - astr_str() returns a const char *. This should not be modified
     or freed by the caller; the storage remains owned by the
     struct astring, which should be freed with astr_free().

   - astr_to_str() returns a char * and destroys the struct astring.
     Responsibility for freeing the storage becomes the caller's.

  One pattern for using astr_str() is to replace static buffers in
  functions that return a pointer to static storage. Where previously
  you would have had e.g. "static struct buf[128]" with an arbitrary
  size limit, you can have "static struct astring buf", and re-use the
  same astring on subsequent calls; the caller should behave the
  same (only reading the string and not freeing it).

***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include "fc_prehdrs.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"                /* fc_assert */
#include "mem.h"
#include "support.h"            /* fc_vsnprintf, fc_strlcat */

#include "astring.h"

#define str     _private_str_
#define n       _private_n_
#define n_alloc _private_n_alloc_

static const struct astring zero_astr = ASTRING_INIT;
static char *astr_buffer = NULL;
static size_t astr_buffer_alloc = 0;

static inline char *astr_buffer_get(size_t *alloc);
static inline char *astr_buffer_grow(size_t *alloc);
static void astr_buffer_free(void);


/************************************************************************//**
  Returns the astring buffer. Create it if necessary.
****************************************************************************/
static inline char *astr_buffer_get(size_t *alloc)
{
  if (!astr_buffer) {
    astr_buffer_alloc = 65536;
    astr_buffer = fc_malloc(astr_buffer_alloc);
    atexit(astr_buffer_free);
  }

  *alloc = astr_buffer_alloc;
  return astr_buffer;
}

/************************************************************************//**
  Grow the astring buffer.
****************************************************************************/
static inline char *astr_buffer_grow(size_t *alloc)
{
  astr_buffer_alloc *= 2;
  astr_buffer = fc_realloc(astr_buffer, astr_buffer_alloc);

  *alloc = astr_buffer_alloc;
  return astr_buffer;
}

/************************************************************************//**
  Free the astring buffer.
****************************************************************************/
static void astr_buffer_free(void)
{
  free(astr_buffer);
}

/************************************************************************//**
  Initialize the struct.
****************************************************************************/
void astr_init(struct astring *astr)
{
  *astr = zero_astr;
}

/************************************************************************//**
  Free the memory associated with astr, and return astr to same
  state as after astr_init.
****************************************************************************/
void astr_free(struct astring *astr)
{
  if (astr->n_alloc > 0) {
    fc_assert_ret(NULL != astr->str);
    free(astr->str);
  }
  *astr = zero_astr;
}

/************************************************************************//**
  Return the raw string to the caller, and return astr to same state as
  after astr_init().
  Freeing the string's storage becomes the caller's responsibility.
****************************************************************************/
char *astr_to_str(struct astring *astr)
{
  char *str = astr->str;
  *astr = zero_astr;
  return str;
}

/************************************************************************//**
  Check that astr has enough size to hold n, and realloc to a bigger
  size if necessary.  Here n must be big enough to include the trailing
  ascii-null if required.  The requested n is stored in astr->n.
  The actual amount allocated may be larger than n, and is stored
  in astr->n_alloc.
****************************************************************************/
void astr_reserve(struct astring *astr, size_t n)
{
  int n1;
  bool was_null = (astr->n == 0);

  fc_assert_ret(NULL != astr);

  astr->n = n;
  if (n <= astr->n_alloc) {
    return;
  }

  /* Allocated more if this is only a small increase on before: */
  n1 = (3 * (astr->n_alloc + 10)) / 2;
  astr->n_alloc = (n > n1) ? n : n1;
  astr->str = (char *) fc_realloc(astr->str, astr->n_alloc);
  if (was_null) {
    astr_clear(astr);
  }
}

/************************************************************************//**
  Sets the content to the empty string.
****************************************************************************/
void astr_clear(struct astring *astr)
{
  if (astr->n == 0) {
    /* astr_reserve is really astr_size, so we don't want to reduce the
     * size. */
    astr_reserve(astr, 1);
  }
  astr->str[0] = '\0';
}

/************************************************************************//**
  Add the text to the string.
****************************************************************************/
static void astr_vadd(struct astring *astr, size_t at,
                      const char *format, va_list ap)
{
  char *buffer;
  size_t buffer_size;
  size_t new_len;

  buffer = astr_buffer_get(&buffer_size);
  for (;;) {
    new_len = fc_vsnprintf(buffer, buffer_size, format, ap);
    if (new_len < buffer_size && (size_t) -1 != new_len) {
      break;
    }
    buffer = astr_buffer_grow(&buffer_size);
  }

  new_len += at + 1;

  astr_reserve(astr, new_len);
  fc_strlcpy(astr->str + at, buffer, astr->n_alloc - at);
}

/************************************************************************//**
  Set the text to the string.
****************************************************************************/
void astr_set(struct astring *astr, const char *format, ...)
{
  va_list args;

  va_start(args, format);
  astr_vadd(astr, 0, format, args);
  va_end(args);
}


/************************************************************************//**
  Add the text to the string.
****************************************************************************/
void astr_add(struct astring *astr, const char *format, ...)
{
  va_list args;

  va_start(args, format);
  astr_vadd(astr, astr_len(astr), format, args);
  va_end(args);
}

/************************************************************************//**
  Add the text to the string in a new line.
****************************************************************************/
void astr_add_line(struct astring *astr, const char *format, ...)
{
  size_t len = astr_len(astr);
  va_list args;

  va_start(args, format);
  if (0 < len) {
    astr_vadd(astr, len + 1, format, args);
    astr->str[len] = '\n';
  } else {
    astr_vadd(astr, len, format, args);
  }
  va_end(args);
}

/************************************************************************//**
  Replace the spaces by line breaks when the line lenght is over the desired
  one.
****************************************************************************/
void astr_break_lines(struct astring *astr, size_t desired_len)
{
  fc_break_lines(astr->str, desired_len);
}

/************************************************************************//**
  Build a localized string with the given items. Items will be
  "or"-separated.

  See also astr_build_and_list(), strvec_to_or_list().
****************************************************************************/
const char *astr_build_or_list(struct astring *astr,
                               const char *const *items, size_t number)
{
  fc_assert_ret_val(NULL != astr, NULL);
  fc_assert_ret_val(0 < number, NULL);
  fc_assert_ret_val(NULL != items, NULL);

  if (1 == number) {
    /* TRANS: "or"-separated string list with one single item. */
    astr_set(astr, Q_("?or-list-single:%s"), *items);
  } else if (2 == number) {
    /* TRANS: "or"-separated string list with 2 items. */
    astr_set(astr, Q_("?or-list:%s or %s"), items[0], items[1]);
  } else {
    /* Estimate the space we need. */
    astr_reserve(astr, number * 64);
    /* TRANS: start of an "or"-separated string list with more than two
     * items. */
    astr_set(astr, Q_("?or-list:%s"), *items++);
    while (1 < --number) {
      /* TRANS: next elements of an "or"-separated string list with more
       * than two items. */
      astr_add(astr, Q_("?or-list:, %s"), *items++);
    }
    /* TRANS: end of an "or"-separated string list with more than two
     * items. */
    astr_add(astr, Q_("?or-list:, or %s"), *items);
  }

  return astr->str;
}

/************************************************************************//**
  Build a localized string with the given items. Items will be
  "and"-separated.

  See also astr_build_or_list(), strvec_to_and_list().
****************************************************************************/
const char *astr_build_and_list(struct astring *astr,
                                const char *const *items, size_t number)
{
  fc_assert_ret_val(NULL != astr, NULL);
  fc_assert_ret_val(0 < number, NULL);
  fc_assert_ret_val(NULL != items, NULL);

  if (1 == number) {
    /* TRANS: "and"-separated string list with one single item. */
    astr_set(astr, Q_("?and-list-single:%s"), *items);
  } else if (2 == number) {
    /* TRANS: "and"-separated string list with 2 items. */
    astr_set(astr, Q_("?and-list:%s and %s"), items[0], items[1]);
  } else {
    /* Estimate the space we need. */
    astr_reserve(astr, number * 64);
    /* TRANS: start of an "and"-separated string list with more than two
     * items. */
    astr_set(astr, Q_("?and-list:%s"), *items++);
    while (1 < --number) {
      /* TRANS: next elements of an "and"-separated string list with more
       * than two items. */
      astr_add(astr, Q_("?and-list:, %s"), *items++);
    }
    /* TRANS: end of an "and"-separated string list with more than two
     * items. */
    astr_add(astr, Q_("?and-list:, and %s"), *items);
  }

  return astr->str;
}

/************************************************************************//**
  Copy one astring in another.
****************************************************************************/
void astr_copy(struct astring *dest, const struct astring *src)
{
  if (astr_empty(src)) {
    astr_clear(dest);
  } else {
    astr_set(dest, "%s", src->str);
  }
}
