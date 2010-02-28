/**********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__API_INTL_H
#define FC__API_INTL_H

const char *api_intl__(const char *untranslated);
const char *api_intl_N_(const char *untranslated);
const char *api_intl_Q_(const char *untranslated);
const char *api_intl_PL_(const char *singular, const char *plural, int n);

#endif /* FC__API_INTL_H */
