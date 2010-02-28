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

#include "shared.h"

#include "iterator.h"


/***********************************************************************
  'next' function implementation for an "invalid" iterator.
***********************************************************************/
static void invalid_iter_next(struct iterator *it)
{
  /* Do nothing. */
}

/***********************************************************************
  'get' function implementation for an "invalid" iterator.
***********************************************************************/
static void *invalid_iter_get(const struct iterator *it)
{
  return NULL;
}

/***********************************************************************
  'valid' function implementation for an "invalid" iterator.
***********************************************************************/
static bool invalid_iter_valid(const struct iterator *it)
{
  return FALSE;
}

/***********************************************************************
  Initializes the iterator vtable so that generic_iterate assumes that
  the iterator is invalid.
***********************************************************************/
struct iterator *invalid_iter_init(struct iterator *it)
{
  it->next = invalid_iter_next;
  it->get = invalid_iter_get;
  it->valid = invalid_iter_valid;
  return it;
}
