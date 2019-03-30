/**********************************************************************
 Freeciv - Copyright (C) 1996-2004 - The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__PAGES_H
#define FC__PAGES_H

extern "C" {
#include "pages_g.h"
}

// gui-qt
#include "qtg_cxxside.h"

struct player;
struct connection;
struct server_scan;

void create_conn_menu (player*, connection*);
void server_scan_error (server_scan*, const char*);

#endif  /* FC__PAGES_H */
