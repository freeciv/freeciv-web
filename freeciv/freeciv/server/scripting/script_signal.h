/**********************************************************************
 Freeciv - Copyright (C) 2005 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__SCRIPT_SIGNAL_H
#define FC__SCRIPT_SIGNAL_H

#include <stdarg.h>

enum api_types {
  API_TYPE_INT,
  API_TYPE_BOOL,
  API_TYPE_STRING,

  API_TYPE_PLAYER,
  API_TYPE_CITY,
  API_TYPE_UNIT,
  API_TYPE_TILE,

  API_TYPE_GOVERNMENT,
  API_TYPE_BUILDING_TYPE,
  API_TYPE_NATION_TYPE,
  API_TYPE_UNIT_TYPE,
  API_TYPE_TECH_TYPE,
  API_TYPE_TERRAIN,

  API_TYPE_LAST
};

struct section_file;

const char *get_api_type_name(enum api_types id);

void script_signal_emit(const char *signal_name, int nargs, ...);

void script_signal_create_valist(const char *signal_name,
				 int nargs, va_list args);
void script_signal_create(const char *signal_name, int nargs, ...);

void script_signal_connect(const char *signal_name, const char *callback_name);

void script_signals_init(void);
void script_signals_free(void);

#endif

