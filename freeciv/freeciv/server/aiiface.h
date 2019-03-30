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
#ifndef FC__AIIFACE_H
#define FC__AIIFACE_H

#include "ai.h" /* incident_type */

void ai_init(void);

bool load_ai_module(const char *modname);

const char *default_ai_type_name(void);

void call_incident(enum incident_type type, struct player *violator,
                   struct player *victim);
void call_ai_refresh(void);

#endif /* FC__AIIFACE_H */
