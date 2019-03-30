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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* client */
#include "options.h"

void init_themes(void);
struct strvec;
const struct strvec *get_themes_list(const struct option *poption);
bool load_theme(const char* theme_name);
void theme_reread_callback(struct option *option);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__THEMES_COMMON_H */
