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
#ifndef FC__GGZSERVER_H
#define FC__GGZSERVER_H

#ifdef GGZ_SERVER

#include "shared.h"

#include "player.h"

extern bool with_ggz;

void ggz_initialize(void);
void input_from_ggz(int socket);
int get_ggz_socket(void);

void ggz_report_victor(const struct player *winner);
void ggz_report_victory(void);

void ggz_game_saved(const char *filename);

#else

#  define with_ggz FALSE
#  define ggz_initialize() (void)0
#  define ggz_report_victor(pplayer) (void)0
#  define ggz_report_victory() (void)0
#  define ggz_game_saved(filename) (void)0

#endif

#endif  /* FC__GGZSERVER_H */
