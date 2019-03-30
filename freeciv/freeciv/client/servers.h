/**********************************************************************
 Freeciv - Copyright (C) 1996-2005 - Freeciv Development Team
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__SERVERS_H
#define FC__SERVERS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility */
#include "fcthread.h"

#define SERVER_LAN_PORT 4555
#define SERVER_LAN_TTL 1
#define SERVER_LAN_VERSION 2

struct server
{
  char *host;
  int port;
  char *capability;
  char *patches;
  char *version;
  char *state;
  char *topic;
  char *message;

  struct players
  {
    char *name;
    char *type;
    char *host;
    char *nation;
  } *players;

  int nplayers;
  int humans;
};

#define SPECLIST_TAG server
#define SPECLIST_TYPE struct server
#include "speclist.h"

#define server_list_iterate(serverlist, pserver) \
  TYPED_LIST_ITERATE(struct server, serverlist, pserver)
#define server_list_iterate_end  LIST_ITERATE_END

struct server_scan;

struct srv_list
{
  struct server_list *servers;
  fc_mutex mutex;
};

enum server_scan_type {
  SERVER_SCAN_LOCAL, /* Local servers, detected through a LAN scan */
  SERVER_SCAN_GLOBAL, /* Global servers, read from the metaserver */
  SERVER_SCAN_LAST
};

typedef void (*ServerScanErrorFunc)(struct server_scan *scan,
				    const char *message);

struct server_scan *server_scan_begin(enum server_scan_type type,
				      ServerScanErrorFunc error_func);
enum server_scan_type server_scan_get_type(const struct server_scan *scan);
enum server_scan_status {
  SCAN_STATUS_ERROR = 0,
  SCAN_STATUS_WAITING,
  SCAN_STATUS_PARTIAL,
  SCAN_STATUS_DONE,
  SCAN_STATUS_ABORT
};
enum server_scan_status server_scan_poll(struct server_scan *scan);
struct srv_list *
server_scan_get_list(struct server_scan *scan);
void server_scan_finish(struct server_scan *scan);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__SERVERS_H */
