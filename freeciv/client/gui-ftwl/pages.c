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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "chatline.h"
#include "mapview.h"

#include "pages.h"

static enum client_pages old_page = PAGE_MAIN;

static void show_main_page()
{
  output_window_append(FTC_CLIENT_INFO, NULL,
                       "Connection dialog not yet implemented. Start client "
                       "using the -a option.");
}

/**************************************************************************
  Does a toplevel window page transition.
**************************************************************************/
void set_client_page(enum client_pages page)
{
  if ((page == old_page) && (page != PAGE_MAIN)) {
    return;
  }
  
  old_page = page;    
    
  if (page == PAGE_MAIN) {
    show_main_page();
  } else if (page == PAGE_START) {
    popup_mapcanvas();
  }
  
}

/**************************************************************************
  Returns current client page
**************************************************************************/
enum client_pages get_client_page(void)
{
  /* PORTME */    
  return old_page;
}

/**************************************************************************
  update the start page.
**************************************************************************/
void update_start_page(void)
{
  /* PORTME*/    
}
