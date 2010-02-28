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
#ifndef FC__PAGES_G_H
#define FC__PAGES_G_H

/**************************************************************************
  Toplevel window pages modes.
**************************************************************************/
enum client_pages {
  PAGE_MAIN,		/* Main menu, aka intro page.  */
  PAGE_START,		/* Start new game page.  */
  PAGE_SCENARIO,	/* Start new scenario page. */
  PAGE_LOAD,		/* Load saved game page. */
  PAGE_NETWORK,		/* Connect to network page.  */
  PAGE_NATION,		/* Select a nation page.  */
  PAGE_GAME,		/* In game page. */
  PAGE_GGZ		/* In game page.  This one must be last. */
};

void set_client_page(enum client_pages page);
enum client_pages get_client_page(void);
void update_start_page(void);

#endif  /* FC__PAGES_G_H */
