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
#ifndef FC__SANITYCHECK_H
#define FC__SANITYCHECK_H

#include "fc_types.h"

#if ((IS_BETA_VERSION || IS_DEVEL_VERSION) && !defined NDEBUG) \
  || defined DEBUG
#  define SANITY_CHECKING
#endif

#ifdef SANITY_CHECKING

#  define sanity_check_city_all(x) real_sanity_check_city_all(x, __FILE__, __LINE__)
void real_sanity_check_city_all(struct city *pcity, const char *file, int line);

#  define sanity_check_city(x) real_sanity_check_city(x, __FILE__, __LINE__)
void real_sanity_check_city(struct city *pcity, const char *file, int line);

#  define sanity_check() real_sanity_check(__FILE__, __LINE__)
void real_sanity_check(const char *file, int line);

#  define sanity_check_feelings(x) real_sanity_check_feelings(x, __FILE__, __LINE__)
void real_sanity_check_feelings(const struct city *pcity,
                                const char *file, int line);

#else /* SANITY_CHECKING */

#  define sanity_check_city_all(x) (void)0
#  define sanity_check_city(x) (void)0
#  define sanity_check() (void)0
#  define sanity_check_feelings(x) (void)0

#endif /* SANITY_CHECKING */


#endif  /* FC__SANITYCHECK_H */
