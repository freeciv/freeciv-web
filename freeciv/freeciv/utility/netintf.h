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

#ifndef FC__NETINTF_H
#define FC__NETINTF_H

/********************************************************************** 
  Common network interface.
***********************************************************************/

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WINSOCK
#include <winsock.h>
#endif

#include "ioz.h"
#include "shared.h"		/* bool type */

/* map symbolic Winsock error names to symbolic errno names */
#ifdef HAVE_WINSOCK
#undef EINTR
#undef EINPROGRESS
#undef EWOULDBLOCK
#undef ECONNRESET
#undef ECONNREFUSED
#define EINTR         WSAEINTR
#define EINPROGRESS   WSAEWOULDBLOCK
#define EWOULDBLOCK   WSAEWOULDBLOCK
#define ECONNRESET    WSAECONNRESET
#define ECONNREFUSED  WSAECONNREFUSED
#endif   

#ifdef FD_ZERO
#define MY_FD_ZERO FD_ZERO
#else
#define MY_FD_ZERO(p) memset((void *)(p), 0, sizeof(*(p)))
#endif

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif

union fc_sockaddr {
  struct sockaddr saddr;
  struct sockaddr_in saddr_in4;
#ifdef IPV6_SUPPORT
  struct sockaddr_in6 saddr_in6;
#endif
};

/* Which protocol will be used for LAN announcements */
enum announce_type {
  ANNOUNCE_NONE,
  ANNOUNCE_IPV4,
  ANNOUNCE_IPV6
};

#define ANNOUNCE_DEFAULT ANNOUNCE_IPV4

int fc_connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
int fc_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
              struct timeval *timeout);
int fc_readsocket(int sock, void *buf, size_t size);
int fc_writesocket(int sock, const void *buf, size_t size);
void fc_closesocket(int sock);
void fc_init_network(void);
void fc_shutdown_network(void);

void fc_nonblock(int sockfd);
bool net_lookup_service(const char *name, int port,
                        union fc_sockaddr *addr, bool force_ipv4);
fz_FILE *fc_querysocket(int sock, void *buf, size_t size);
int find_next_free_port(int starting_port);

const char *fc_lookup_httpd(char *server, int *port, const char *url);
const char *fc_url_encode(const char *txt);

void sockaddr_debug(union fc_sockaddr *addr);
int sockaddr_size(union fc_sockaddr *addr);
bool sockaddr_ipv6(union fc_sockaddr *addr);

#endif  /* FC__NETINTF_H */
