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
#ifndef FC__DIFFICULTY_H
#define FC__DIFFICULTY_H

void set_ai_level_directer(struct player *pplayer, enum ai_level level);

char *ai_level_help(const char *cmdname);

bool ai_fuzzy(const struct player *pplayer, bool normal_decision);

#endif /* FC__DIFFICULTY_H */
