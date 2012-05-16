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

#ifndef FC__SCRIPT_H
#define FC__SCRIPT_H

#include "script_signal.h"

struct section_file;

/* internal api error function. */
int script_error(const char *fmt, ...);

/* callback invocation function. */
bool script_callback_invoke(const char *callback_name,
			    int nargs, va_list args);

/* script functions. */
bool script_init(void);
void script_free(void);
bool script_do_file(const char *filename);

/* script state i/o. */
void script_state_load(struct section_file *file);
void script_state_save(struct section_file *file);

#endif

