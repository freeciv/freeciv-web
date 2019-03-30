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
#ifndef FC__FCINTL_H
#define FC__FCINTL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "freeciv_config.h"

#ifdef FREECIV_HAVE_LOCALE_H
#include <locale.h>
#endif

#include "shared.h" /* bool */

#ifdef FREECIV_ENABLE_NLS

/* Include libintl.h only if nls enabled.
 * It defines some wrapper macros that
 * we don't want defined when nls is disabled. */
#ifdef FREECIV_HAVE_LIBINTL_H
#include <libintl.h>
#endif

/* Core freeciv */
#define _(String) gettext(String)
#define DG_(domain, String) dgettext(domain, String)
#define N_(String) String
#define Q_(String) skip_intl_qualifier_prefix(gettext(String))
#define PL_(String1, String2, n) ngettext((String1), (String2), (n))

/* Ruledit */
#define R__(String) dgettext("freeciv-ruledit", String)
#define RQ_(String) skip_intl_qualifier_prefix(dgettext("freeciv-ruledit", String))

#else  /* FREECIV_ENABLE_NLS */

/* Core freeciv */
#define _(String) (String)
#define DG_(domain, String) (String)
#define N_(String) String
#define Q_(String) skip_intl_qualifier_prefix(String)
#define PL_(String1, String2, n) ((n) == 1 ? (String1) : (String2))
#define C_(String) capitalized_string(String)

/* Ruledit */
#define R__(String) (String)
#define RQ_(String) skip_intl_qualifier_prefix(String)

#undef textdomain
#undef bindtextdomain

#define textdomain(Domain)
#define bindtextdomain(Package, Directory)

#endif /* FREECIV_ENABLE_NLS */

/* This provides an untranslated version of Q_ that allows the caller to
 * get access to the original string.  This may be needed for comparisons,
 * for instance. */
#define Qn_(String) skip_intl_qualifier_prefix(String)

const char *skip_intl_qualifier_prefix(const char *str);
char *capitalized_string(const char *str);
void free_capitalized(char *str);
void capitalization_opt_in(bool opt_in);
bool is_capitalization_enabled(void);

const char *get_locale_dir(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__FCINTL_H */
