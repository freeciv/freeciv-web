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

#include <X11/Intrinsic.h>

#include "pages_g.h"

void popup_start_page(void);
void popdown_start_page(void);
void create_start_page(void);
void update_start_page(void);
void start_page_msg_close(Widget w);

#endif  /* FC__PAGES_H */
