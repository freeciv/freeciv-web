/***********************************************************************
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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <freeciv_config.h>

/***********************************************************************
  Common network interface.
***********************************************************************/

#ifdef FREECIV_HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef FREECIV_HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef FREECIV_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef FREECIV_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef FREECIV_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef FREECIV_HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef FREECIV_HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif

/* utility */
#include "ioz.h"
#include "net_types.h"
#include "support.h"   /* bool type */

#ifdef FD_ZERO
#define FC_FD_ZERO FD_ZERO
#else
#define FC_FD_ZERO(p) memset((void *)(p), 0, sizeof(*(p)))
#endif

#ifdef IPV6_ADD_MEMBERSHIP
#define FC_IPV6_ADD_MEMBERSHIP IPV6_ADD_MEMBERSHIP
#else
#define FC_IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif

#ifndef FREECIV_HAVE_SOCKLEN_T
typedef int socklen_t;
#endif  /* FREECIV_HAVE_SOCKLEN_T */

union fc_sockaddr {
  struct sockaddr saddr;
  struct sockaddr_in saddr_in4;
#ifdef FREECIV_IPV6_SUPPORT
  struct sockaddr_in6 saddr_in6;
#endif /* FREECIV_IPV6_SUPPORT */
};

/* get 'struct sockaddr_list' and related functions: */
#define SPECLIST_TAG fc_sockaddr
#define SPECLIST_TYPE union fc_sockaddr
#include "speclist.h"

#define fc_sockaddr_list_iterate(sockaddrlist, paddr) \
    TYPED_LIST_ITERATE(union fc_sockaddr, sockaddrlist, paddr)
#define fc_sockaddr_list_iterate_end  LIST_ITERATE_END

#ifdef FREECIV_MSWINDOWS
typedef TIMEVAL fc_timeval;
#else  /* FREECIV_MSWINDOWS */
typedef struct timeval fc_timeval;
#endif /* FREECIV_MSWINDOWS */

int fc_connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
int fc_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
              fc_timeval *timeout);
int fc_readsocket(int sock, void *buf, size_t size);
int fc_writesocket(int sock, const void *buf, size_t size);
void fc_closesocket(int sock);

void fc_nonblock(int sockfd);
struct fc_sockaddr_list *net_lookup_service(const char *name, int port,
					    enum fc_addr_family family);
bool fc_inet_aton(const char *cp, struct in_addr *inp, bool addr_none_ok);
fz_FILE *fc_querysocket(int sock, void *buf, size_t size);
int find_next_free_port(int starting_port, int highest_port,
                        enum fc_addr_family family,
                        char *net_interface, bool not_avail_ok);

void sockaddr_debug(union fc_sockaddr *addr, enum log_level lvl);
int sockaddr_size(union fc_sockaddr *addr);
bool sockaddr_ipv6(union fc_sockaddr *addr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__NETINTF_H */
