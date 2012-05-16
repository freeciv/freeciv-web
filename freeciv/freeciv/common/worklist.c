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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>
#include <string.h>

#include "log.h"
#include "mem.h"
#include "support.h"

#include "city.h"
#include "requirements.h"
#include "unit.h"
#include "worklist.h"

/****************************************************************
  Initialize a worklist to be empty and have a default name.
  For elements, only really need to set [0], but initialize the
  rest to avoid junk values in savefile.
****************************************************************/
void worklist_init(struct worklist *pwl)
{
  int i;

  pwl->is_valid = TRUE;
  pwl->length = 0;
  strcpy(pwl->name, "a worklist");

  for (i = 0; i < MAX_LEN_WORKLIST; i++) {
    /* just setting the entry to zero: */
    pwl->entries[i].kind = VUT_NONE;
    /* all the union pointers should be in the same place: */ 
    pwl->entries[i].value.building = NULL;
  }
}

/****************************************************************************
  Returns the number of entries in the worklist.  The returned value can
  also be used as the next available worklist index (assuming that
  len < MAX_LEN_WORKLIST).
****************************************************************************/
int worklist_length(const struct worklist *pwl)
{
  assert(pwl->length >= 0 && pwl->length <= MAX_LEN_WORKLIST);
  return pwl->length;
}

/****************************************************************
...
****************************************************************/
bool worklist_is_empty(const struct worklist *pwl)
{
  return !pwl || pwl->length == 0;
}

/****************************************************************
  Fill in the id and is_unit values for the head of the worklist
  if the worklist is non-empty.  Return 1 iff id and is_unit
  are valid.
****************************************************************/
bool worklist_peek(const struct worklist *pwl, struct universal *prod)
{
  return worklist_peek_ith(pwl, prod, 0);
}

/****************************************************************
  Fill in the id and is_unit values for the ith element in the
  worklist. If the worklist has fewer than idx elements,
  return FALSE.
****************************************************************/
bool worklist_peek_ith(const struct worklist *pwl,
		       struct universal *prod, int idx)
{
  /* Out of possible bounds. */
  if (idx < 0 || pwl->length <= idx) {
    prod->kind = VUT_NONE;
    prod->value.building = NULL;
    return FALSE;
  }

  *prod = pwl->entries[idx];

  return TRUE;
}

/****************************************************************
...
****************************************************************/
void worklist_advance(struct worklist *pwl)
{
  worklist_remove(pwl, 0);
}  

/****************************************************************
...
****************************************************************/
void worklist_copy(struct worklist *dst, const struct worklist *src)
{
  assert(sizeof(*dst) == sizeof(*src));
  memcpy(dst, src, sizeof(*dst));
}

/****************************************************************
...
****************************************************************/
void worklist_remove(struct worklist *pwl, int idx)
{
  int i;

  /* Don't try to remove something way outside of the worklist. */
  if (idx < 0 || pwl->length <= idx) {
    return;
  }

  /* Slide everything up one spot. */
  for (i = idx; i < pwl->length - 1; i++) {
    pwl->entries[i] = pwl->entries[i + 1];
  }
  /* just setting the entry to zero: */
  pwl->entries[pwl->length - 1].kind = VUT_NONE;
  /* all the union pointers should be in the same place: */ 
  pwl->entries[pwl->length - 1].value.building = NULL;
  pwl->length--;
}

/****************************************************************************
  Adds the id to the next available slot in the worklist.  'id' is the ID of
  the unit/building to be produced; is_unit specifies whether it's a unit or
  a building.  Returns TRUE if successful.
****************************************************************************/
bool worklist_append(struct worklist *pwl, struct universal prod)
{
  int next_index = worklist_length(pwl);

  if (next_index >= MAX_LEN_WORKLIST) {
    return FALSE;
  }

  pwl->entries[next_index] = prod;
  pwl->length++;

  return TRUE;
}

/****************************************************************************
  Inserts the production at the location idx in the worklist, thus moving
  all subsequent entries down.  'id' specifies the unit/building to
  be produced; is_unit tells whether it's a unit or building.  Returns TRUE
  if successful.
****************************************************************************/
bool worklist_insert(struct worklist *pwl,
		     struct universal prod, int idx)
{
  int new_len = MIN(pwl->length + 1, MAX_LEN_WORKLIST), i;

  if (idx < 0 || idx > pwl->length) {
    return FALSE;
  }

  /* move all active values down an index to get room for new id
   * move from [idx .. len - 1] to [idx + 1 .. len].  Any entries at the
   * end are simply lost. */
  for (i = new_len - 2; i >= idx; i--) {
    pwl->entries[i + 1] = pwl->entries[i];
  }
  
  pwl->entries[idx] = prod;
  pwl->length = new_len;

  return TRUE;
}

/**************************************************************************
  Return TRUE iff the two worklists are equal.
**************************************************************************/
bool are_worklists_equal(const struct worklist *wlist1,
			 const struct worklist *wlist2)
{
  int i;

  if (wlist1->is_valid != wlist2->is_valid) {
    return FALSE;
  }
  if (wlist1->length != wlist2->length) {
    return FALSE;
  }

  for (i = 0; i < wlist1->length; i++) {
    if (!are_universals_equal(&wlist1->entries[i], &wlist2->entries[i])) {
      return FALSE;
    }
  }

  return TRUE;
}

/****************************************************************************
  Load the worklist elements specified by path to the worklist pointed to
  by pwl.

  pwl should be a pointer to an existing worklist.

  path and ... give the prefix to load from, printf-style.
****************************************************************************/
void worklist_load(struct section_file *file, struct worklist *pwl,
		   const char *path, ...)
{
  int i;
  const char* kind;
  const char* name;
  char path_str[1024];
  va_list ap;

  /* The first part of the registry path is taken from the varargs to the
   * function. */
  va_start(ap, path);
  my_vsnprintf(path_str, sizeof(path_str), path, ap);
  va_end(ap);

  worklist_init(pwl);
  pwl->length = secfile_lookup_int_default(file, 0,
					   "%s.wl_length", path_str);
  name = secfile_lookup_str_default(file, "a worklist",
				    "%s.wl_name", path_str);
  sz_strlcpy(pwl->name, name);
  pwl->is_valid = secfile_lookup_bool_default(file, FALSE,
					      "%s.wl_is_valid", path_str);

  for (i = 0; i < pwl->length; i++) {
    kind = secfile_lookup_str_default(file, NULL,
				      "%s.wl_kind%d",
				      path_str, i);
    if (!kind) {
      /* before 2.2.0 unit production was indicated by flag. */
      bool is_unit = secfile_lookup_bool_default(file, FALSE,
						 "%s.wl_is_unit%d",
						 path_str, i);
      kind = universal_kind_name(is_unit ? VUT_UTYPE : VUT_IMPROVEMENT);
    }

    /* We lookup the production value by name.  An invalid entry isn't a
     * fatal error; we just truncate the worklist. */
    name = secfile_lookup_str_default(file, "-", "%s.wl_value%d",
				      path_str, i);
    pwl->entries[i] = universal_by_rule_name(kind, name);
    if (VUT_LAST == pwl->entries[i].kind) {
      freelog(LOG_ERROR, "%s.wl_value%d: unknown \"%s\" \"%s\".",
              path_str, i, kind, name);
      pwl->length = i;
      break;
    }
  }
}

/****************************************************************************
  Save the worklist elements specified by path from the worklist pointed to
  by pwl.

  pwl should be a pointer to an existing worklist.

  path and ... give the prefix to load from, printf-style.
****************************************************************************/
void worklist_save(struct section_file *file, struct worklist *pwl,
                   int max_length, const char *path, ...)
{
  char path_str[1024];
  int i;
  va_list ap;

  /* The first part of the registry path is taken from the varargs to the
   * function. */
  va_start(ap, path);
  my_vsnprintf(path_str, sizeof(path_str), path, ap);
  va_end(ap);

  secfile_insert_int(file, pwl->length, "%s.wl_length", path_str);
  secfile_insert_str(file, pwl->name, "%s.wl_name", path_str);
  secfile_insert_bool(file, pwl->is_valid, "%s.wl_is_valid", path_str);

  for (i = 0; i < pwl->length; i++) {
    struct universal *entry = pwl->entries + i;

    /* before 2.2.0 unit production was indicated by flag. */
    secfile_insert_bool(file, (VUT_UTYPE == entry->kind),
			"%s.wl_is_unit%d", path_str, i);

    secfile_insert_str(file, universal_type_rule_name(entry),
                       "%s.wl_kind%d", path_str, i);
    secfile_insert_str(file, universal_rule_name(entry),
		       "%s.wl_value%d", path_str, i);
  }

  assert(max_length <= MAX_LEN_WORKLIST);

  /* We want to keep savegame in tabular format, so each line has to be
   * of equal length. Fill table up to maximum worklist size. */
  for (i = pwl->length ; i < max_length; i++) {
    /* before 2.2.0 unit production was indicated by flag. */
    secfile_insert_bool(file, false, "%s.wl_is_unit%d", path_str, i);
    secfile_insert_str(file, "", "%s.wl_kind%d", path_str, i);
    secfile_insert_str(file, "", "%s.wl_value%d", path_str, i);
  }
}
