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
#ifndef FC__RULESAVE_H
#define FC__RULESAVE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct rule_data
{
  char *nationlist;
  char *nationlist_saved;
};

bool save_ruleset(const char *path, const char *name, struct rule_data *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__RULESAVE_H */
