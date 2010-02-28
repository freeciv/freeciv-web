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
#ifndef FC__VERSION_H
#define FC__VERSION_H

#ifdef HAVE_CONFIG_H
#ifndef FC_CONFIG_H		/* this should be defined in config.h */
#error Files including versions.h should also include config.h directly
#endif
#endif

/* This is only used in version.c, and only if IS_BETA_VERSION is true.
   The month[] array is defined in version.c (index: 1==Jan, 2==Feb, ...). */
#ifndef NEXT_RELEASE_MONTH
#define NEXT_RELEASE_MONTH	(month[RELEASE_MONTH])
#endif

/* version informational strings */
const char *freeciv_name_version(void);
const char *word_version(void);
const char *fc_svn_revision(void);

const char *freeciv_motto(void);

/* If returns NULL, not a beta version. */
const char *beta_message(void);

#endif  /* FC__VERSION_H */
