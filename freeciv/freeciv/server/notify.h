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
#include "featured_text.h"      /* ftc_*: color pre-definitions. */
#include "packets.h"

/* server */
#include "srv_main.h"           /* enum server_states */

struct research;


void package_chat_msg(struct packet_chat_msg *packet,
                      const struct connection *sender,
                      const struct ft_color color,
                      const char *format, ...)
                      fc__attribute((__format__ (__printf__, 4, 5)));
void vpackage_chat_msg(struct packet_chat_msg *packet,
                       const struct connection *sender,
                       const struct ft_color color,
                       const char *format,
                       va_list vargs);
void package_event(struct packet_chat_msg *packet,
                   const struct tile *ptile,
                   enum event_type event,
                   const struct ft_color color,
                   const char *format, ...)
                   fc__attribute((__format__ (__printf__, 5, 6)));
void vpackage_event(struct packet_chat_msg *packet,
                    const struct tile *ptile,
                    enum event_type event,
                    const struct ft_color color,
                    const char *format,
                    va_list vargs);

void notify_conn(struct conn_list *dest,
                 const struct tile *ptile,
                 enum event_type event,
                 const struct ft_color color,
                 const char *format, ...)
                 fc__attribute((__format__ (__printf__, 5, 6)));
void notify_conn_early(struct conn_list *dest,
                       const struct tile *ptile,
                       enum event_type event,
                       const struct ft_color color,
                       const char *format, ...)
                       fc__attribute((__format__ (__printf__, 5, 6)));
void notify_player(const struct player *pplayer,
                   const struct tile *ptile,
                   enum event_type event,
                   const struct ft_color color,
                   const char *format, ...)
                   fc__attribute((__format__ (__printf__, 5, 6)));
void notify_embassies(const struct player *pplayer,
                      const struct tile *ptile,
                      enum event_type event,
                      const struct ft_color color,
                      const char *format, ...)
                      fc__attribute((__format__ (__printf__, 5, 6)));
void notify_team(const struct player *pplayer,
                 const struct tile *ptile,
                 enum event_type event,
                 const struct ft_color color,
                 const char *format, ...)
                 fc__attribute((__format__ (__printf__, 5, 6)));
void notify_research(const struct research *presearch,
                     const struct player *exclude,
                     enum event_type event,
                     const struct ft_color color,
                     const char *format, ...)
                     fc__attribute((__format__ (__printf__, 5, 6)));
void notify_research_embassies(const struct research *presearch,
                               const struct player *exclude,
                               enum event_type event,
                               const struct ft_color color,
                               const char *format, ...)
                               fc__attribute((__format__ (__printf__, 5, 6)));

/* Event cache. */

/* The type of event target. */
struct event_cache_players;

void event_cache_init(void);
void event_cache_free(void);
void event_cache_clear(void);
void event_cache_remove_old(void);

void event_cache_add_for_all(const struct packet_chat_msg *packet);
void event_cache_add_for_global_observers(const struct packet_chat_msg *packet);
void event_cache_add_for_player(const struct packet_chat_msg *packet,
                                const struct player *pplayer);
struct event_cache_players *
event_cache_player_add(struct event_cache_players *players,
                       const struct player *pplayer)
                       fc__warn_unused_result;
void event_cache_add_for_players(const struct packet_chat_msg *packet,
                                 struct event_cache_players *players);

void send_pending_events(struct connection *pconn, bool include_public);

void event_cache_phases_invalidate(void);

struct section_file;
void event_cache_load(struct section_file *file, const char *section);
void event_cache_save(struct section_file *file, const char *section);

#endif  /* FC__NOTIFY_H */
