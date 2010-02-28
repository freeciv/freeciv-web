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
#ifndef FC__NOTIFY_H
#define FC__NOTIFY_H

#include <stdarg.h>

/* utility */
#include "support.h"            /* fc__attribute */

/* common */
#include "events.h"
#include "fc_types.h"
#include "featured_text.h"      /* FTC_*: color pre-definitions. */
#include "packets.h"

void package_chat_msg(struct packet_chat_msg *packet,
                      const struct connection *sender,
                      const char *fg_color, const char *bg_color,
                      const char *format, ...)
                      fc__attribute((__format__ (__printf__, 5, 6)));
void vpackage_chat_msg(struct packet_chat_msg *packet,
                       const struct connection *sender,
                       const char *fg_color, const char *bg_color,
                       const char *format, va_list vargs);
void vpackage_event(struct packet_chat_msg *packet,
                    const struct tile *ptile, enum event_type event,
                    const char *fg_color, const char *bg_color,
                    const char *format, va_list vargs);

void notify_conn(struct conn_list *dest, const struct tile *ptile,
                 enum event_type event, const char *fg_color,
                 const char *bg_color, const char *format, ...)
                 fc__attribute((__format__ (__printf__, 6, 7)));
void notify_player(const struct player *pplayer, const struct tile *ptile,
                   enum event_type event, const char *fg_color,
                   const char *bg_color, const char *format, ...)
                   fc__attribute((__format__ (__printf__, 6, 7)));
void notify_embassies(const struct player *pplayer,
                      const struct player *exclude,
                      const struct tile *ptile, enum event_type event,
                      const char *fg_color, const char *bg_color,
                      const char *format, ...)
                      fc__attribute((__format__ (__printf__, 7, 8)));
void notify_team(const struct player *pplayer, const struct tile *ptile,
                 enum event_type event, const char *fg_color,
                 const char *bg_color, const char *format, ...)
                 fc__attribute((__format__ (__printf__, 6, 7)));
void notify_research(const struct player *pplayer, enum event_type event,
                     const char *fg_color, const char *bg_color,
                     const char *format, ...)
                     fc__attribute((__format__ (__printf__, 5, 6)));

#endif  /* FC__NOTIFY_H */
