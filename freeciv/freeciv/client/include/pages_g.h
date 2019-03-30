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
#define SPECENUM_NAME client_pages
#define SPECENUM_VALUE0 PAGE_MAIN       /* Main menu, aka intro page.  */
#define SPECENUM_VALUE1 PAGE_START      /* Start new game page.  */
#define SPECENUM_VALUE2 PAGE_SCENARIO   /* Start new scenario page. */
#define SPECENUM_VALUE3 PAGE_LOAD       /* Load saved game page. */
#define SPECENUM_VALUE4 PAGE_NETWORK    /* Connect to network page.  */
#define SPECENUM_VALUE5 PAGE_GAME       /* In game page. */
#include "specenum_gen.h"

#include "gui_proto_constructor.h"

GUI_FUNC_PROTO(void, real_set_client_page, enum client_pages page)
GUI_FUNC_PROTO(enum client_pages, get_current_client_page, void)
GUI_FUNC_PROTO(void, update_start_page, void)

/* Actually defined in update_queue.c */
void set_client_page(enum client_pages page);
void client_start_server_and_set_page(enum client_pages page);
enum client_pages get_client_page(void);

#endif  /* FC__PAGES_G_H */
