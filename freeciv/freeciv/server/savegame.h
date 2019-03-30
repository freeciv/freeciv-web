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
#ifndef FC__SAVEGAME_H
#define FC__SAVEGAME_H

/* utility */
#include "support.h"

struct section_file;

void savegame_load(struct section_file *sfile);
void savegame_save(struct section_file *sfile, const char *save_reason,
                   bool scenario);

void save_game(const char *orig_filename, const char *save_reason,
               bool scenario);

void save_system_close(void);

#endif /* FC__SAVEGAME_H */
