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
#ifndef FC__VERSION_H
#define FC__VERSION_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if !defined(FC__FREECIV_CONFIG_H) && !defined(FC_CONFIG_H)
#error Files including versions.h should also include freeciv_config.h directly
#endif

/* This is only used in version.c, and only if IS_BETA_VERSION is true.
 * The month[] array is defined in version.c (index: 1 == Jan, 2 == Feb, ...).
 */
#ifndef NEXT_RELEASE_MONTH
#define NEXT_RELEASE_MONTH	(month[FREECIV_RELEASE_MONTH])
#endif

/* version informational strings */
const char *freeciv_name_version(void);
const char *word_version(void);
const char *fc_git_revision(void);
const char *fc_comparable_version(void);
const char *freeciv_datafile_version(void);

const char *freeciv_motto(void);

/* If returns NULL, not a beta version. */
const char *beta_message(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__VERSION_H */
