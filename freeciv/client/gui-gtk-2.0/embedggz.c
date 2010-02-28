/********************7************************************************** 
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* utility */
#include "rand.h"
#include "shared.h"

#ifdef GGZ_GTK
#  include <ggz-embed.h>
#endif

/* client */
#include "ggz_g.h"

/****************************************************************
  Call ggz_embed_leave_table() if ggz enabled.
*****************************************************************/
void gui_ggz_embed_leave_table(void)
{
#ifdef GGZ_GTK
    ggz_embed_leave_table();
#endif
}

/****************************************************************
  Call ggz_embed_ensure_server() if ggz enabled.
*****************************************************************/
void gui_ggz_embed_ensure_server(void)
{
#ifdef GGZ_GTK
  {
    char buf[128];

    user_username(buf, sizeof(buf));
    cat_snprintf(buf, sizeof(buf), "%d", myrand(100));
    ggz_embed_ensure_server("Pubserver", "freeciv.ggzgamingzone.org",
                           5688, buf);
  }
#endif /* GGZ_GTK */
}
