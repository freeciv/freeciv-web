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
#ifndef FC__STDINHAND_H
#define FC__STDINHAND_H

#include "connection.h"		/* enum cmdlevel_id */
#include "fc_types.h"

#define SERVER_COMMAND_PREFIX '/'
  /* the character to mark chatlines as server commands */

void stdinhand_init(void);
void stdinhand_turn(void);
void stdinhand_free(void);

bool handle_stdin_input(struct connection *caller, char *str, bool check);
void send_server_settings(struct conn_list *dest);
void set_ai_level_direct(struct player *pplayer, enum ai_level level);
void set_ai_level_directer(struct player *pplayer, enum ai_level level);
bool read_init_script(struct connection *caller, char *script_filename,
                      bool from_cmdline, bool check);
void show_players(struct connection *caller);

bool load_command(struct connection *caller,
		  const char *filename, bool check);
bool start_command(struct connection *caller, bool check, bool notify);

void toggle_ai_player_direct(struct connection *caller,
			     struct player *pplayer);

/* for sernet.c in initing a new connection */
enum cmdlevel_id access_level_for_next_connection(void);

void notify_if_first_access_level_is_available(void);

#ifdef HAVE_LIBREADLINE
#ifdef HAVE_NEWLIBREADLINE
char **freeciv_completion(const char *text, int start, int end);
#else
char **freeciv_completion(char *text, int start, int end);
#endif
#endif

#endif /* FC__STDINHAND_H */
