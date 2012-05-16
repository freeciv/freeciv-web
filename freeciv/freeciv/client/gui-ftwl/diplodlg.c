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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "diplodlg.h"

void handle_diplomacy_init_meeting(int counterpart, int initiated_from){}
void handle_diplomacy_cancel_meeting(int counterpart, int initiated_from){}
void handle_diplomacy_create_clause(int counterpart, int giver,
    enum clause_type type, int value){}
void handle_diplomacy_remove_clause(int counterpart, int giver,
    enum clause_type type, int value){}
void handle_diplomacy_accept_treaty(int counterpart, bool I_accepted,
    bool other_accepted){}

void close_all_diplomacy_dialogs(void){}
