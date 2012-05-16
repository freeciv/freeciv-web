/********************************************************************** 
 Freeciv - Copyright (C) 2005 The Freeciv Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/
#ifndef FC__THEMES_COMMON_H
#define FC__THEMES_COMMON_H
#include "options.h"

void init_themes(void);
const char** get_themes_list(void);
bool load_theme(const char* theme_name);
void theme_reread_callback(struct client_option *option);
#endif
