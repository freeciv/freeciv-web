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
#ifndef FC__UNIV_VALUE_H
#define FC__UNIV_VALUE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void (*univ_kind_values_cb)(const char *value, bool current, void *data);

bool universal_value_initial(struct universal *src);
void universal_kind_values(struct universal *univ, univ_kind_values_cb cb, void *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__UNIV_VALUE_H */
