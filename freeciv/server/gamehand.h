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
#ifndef FC__GAMEHAND_H
#define FC__GAMEHAND_H

struct section_file;
struct connection;
struct conn_list;

void init_new_game(void);
void send_year_to_clients(int year);
void send_game_info(struct conn_list *dest);

void send_scenario_info(struct conn_list *dest);

struct unit_type *crole_to_unit_type(char crole,struct player *pplayer);

int update_timeout(void);
void increase_timeout_because_unit_moved(void);

const char *new_challenge_filename(struct connection *pc);

#endif  /* FC__GAMEHAND_H */
