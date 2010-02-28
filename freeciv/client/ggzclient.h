/********************************************************************** 
 Freeciv - Copyright (C) 2005 - Freeciv Development Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__GGZCLIENT_H
#define FC__GGZCLIENT_H

#include "shared.h"

extern bool with_ggz;

void ggz_initialize(void);

#ifdef GGZ_CLIENT

void ggz_begin(void);
void input_from_ggz(int socket);

bool user_get_rating(const char *name, int *rating);
bool user_get_record(const char *name,
		     int *wins, int *losses, int *ties, int *forfeits);

#else

#  define ggz_begin() (void)0
#  define input_from_ggz(socket) (void)0
#  define user_get_rating(p, r) (FALSE)
#  define user_get_record(p, w, l, t, f) (FALSE)

#endif

#endif  /* FC__GGZCLIENT_H */
