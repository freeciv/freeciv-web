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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* server */
#include "notify.h"

#include "api_notify.h"


/**************************************************************************
  Notify players which have embassies with pplayer with the given message.
**************************************************************************/
void api_notify_embassies_msg(Player *pplayer, Tile *ptile, int event,
			      const char *message)
{
  notify_embassies(pplayer, NULL, ptile, event, NULL, NULL, "%s", message);
}

/**************************************************************************
  Notify pplayer of a complex event.
**************************************************************************/
void api_notify_event_msg(Player *pplayer, Tile *ptile, int event,
		     	  const char *message)
{
  notify_player(pplayer, ptile, event, NULL, NULL, "%s", message);
}

