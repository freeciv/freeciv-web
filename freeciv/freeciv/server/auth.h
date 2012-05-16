/**********************************************************************
 Freeciv - Copyright (C) 2005 - M.C. Kaufman
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__AUTH_H
#define FC__AUTH_H

#include "shared.h"

struct connection;

bool is_guest_name(const char *name);
void get_unique_guest_name(char *name);

bool authenticate_user(struct connection *pconn, char *username);
void process_authentication_status(struct connection *pconn);
bool handle_authentication_reply(struct connection *pc, char *password);

bool auth_init(const char *conf_file);
void auth_free(void);

#endif /* FC__AUTH_H */
