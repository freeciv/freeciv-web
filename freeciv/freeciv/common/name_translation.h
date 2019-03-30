/***********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__NAME_TRANSLATION_H
#define FC__NAME_TRANSLATION_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "fcintl.h"
#include "support.h"

/* common */
#include "fc_types.h" /* MAX_LEN_NAME */

/* Don't allow other modules to access directly to the fields. */
#define vernacular _private_vernacular_
#define rulename   _private_rulename_
#define translated _private_translated_

/* Ruleset strings (such as names) are kept in their original vernacular
 * as well as being translated to the current locale. */
struct name_translation {
  const char *translated;               /* String doesn't need freeing. */
  char vernacular[MAX_LEN_NAME];        /* Original string,
                                         * used for comparisons. */
  char rulename[MAX_LEN_NAME];          /* Name used in savefiles etc.
                                           Often the same as 'vernacular'. */
};

/* Inititalization macro. */
#define NAME_INIT { NULL, "\0", "\0" }

/****************************************************************************
  Initializes a name translation structure.
****************************************************************************/
static inline void name_init(struct name_translation *ptrans)
{
  ptrans->vernacular[0] = ptrans->rulename[0] = '\0';
  ptrans->translated = NULL;
}

/****************************************************************************
  Set the untranslated and rule names of the name translation structure.
  If rule_name is NULL, use vernacular_name for it (after removing any i18n
  qualifier).
****************************************************************************/
static inline void names_set(struct name_translation *ptrans,
                             const char *domain,
                             const char *vernacular_name,
                             const char *rule_name)
{
  static const char name_too_long[] = "Name \"%s\" too long; truncating.";

  (void) sz_loud_strlcpy(ptrans->vernacular, vernacular_name, name_too_long);
  (void) sz_loud_strlcpy(ptrans->rulename,
                         rule_name ? rule_name : Qn_(vernacular_name),
                         name_too_long);

  if (ptrans->vernacular[0] != '\0') {
    /* Translate now. */
    if (domain == NULL) {
      ptrans->translated = Q_(ptrans->vernacular);
    } else {
      ptrans->translated = skip_intl_qualifier_prefix(DG_(domain, ptrans->vernacular));
    }
  } else {
    ptrans->translated = ptrans->vernacular;
  }
}

/****************************************************************************
  Set the untranslated name of the name translation structure.
  Assumes the rule name should be based on the vernacular.
****************************************************************************/
static inline void name_set(struct name_translation *ptrans,
                            const char *domain,
                            const char *vernacular_name)
{
  names_set(ptrans, domain, vernacular_name, NULL);
}

/****************************************************************************
  Return the untranslated (vernacular) name of the name translation
  structure.
  Rarely used; you usually want name_translation() or rule_name().
  Note that this does not discard any translation qualifiers! -- if this
  string is to be displayed to the user (unlikely), the caller must call
  Qn_() on it.
****************************************************************************/
static inline const char *
    untranslated_name(const struct name_translation *ptrans)
{
  return ptrans->vernacular;
}

/****************************************************************************
  Return the rule name of the name translation structure.
****************************************************************************/
static inline const char *rule_name_get(const struct name_translation *ptrans)
{
  return ptrans->rulename;
}

/****************************************************************************
  Return the translated name of the name translation structure.
****************************************************************************/
static inline const char *
    name_translation_get(const struct name_translation *ptrans)
{
  return ptrans->translated;
}

/* Don't allow other modules to access directly to the fields. */
#undef vernacular
#undef rulename
#undef translated

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__NAME_TRANSLATION_H */
