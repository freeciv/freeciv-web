/********************************************************************** 
Freeciv - Copyright (C) 2003 - The Freeciv Project
   This program is free software; you can redistribute it and / or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, 
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/ 
#ifndef FC__CONNECTDLG_COMMON_H
#define FC__CONNECTDLG_COMMON_H

#include "shared.h"

#if defined(HAVE_WORKING_FORK) || defined(WIN32_NATIVE)
# define CLIENT_CAN_LAUNCH_SERVER
#endif

bool client_start_server(void);
void client_kill_server(bool force);

bool is_server_running(void);
bool can_client_access_hack(void);

void send_client_wants_hack(const char *filename);
void send_start_saved_game(void);
void send_save_game(char *filename);

void set_ruleset(const char *ruleset);

extern char leader_name[MAX_LEN_NAME];
extern char *current_filename;

#endif  /* FC__CONNECTDLG_COMMON_H */ 
