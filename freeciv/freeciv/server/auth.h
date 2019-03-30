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

bool auth_user(struct connection *pconn, char *username);
void auth_process_status(struct connection *pconn);
bool auth_handle_reply(struct connection *pconn, char *password);

const char *auth_get_username(struct connection *pconn);
const char *auth_get_ipaddr(struct connection *pconn);
bool auth_set_password(struct connection *pconn, const char *password);
const char *auth_get_password(struct connection *pconn);
bool auth_set_salt(struct connection *pconn, int salt);
int auth_get_salt(struct connection *pconn);

#endif /* FC__AUTH_H */
