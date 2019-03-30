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

/***********************************************************************
  Common network interface.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include "fc_prehdrs.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif 
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif
#ifdef FREECIV_MSWINDOWS
#include <windows.h>	/* GetTempPath */
#endif

/* utility */
#include "bugs.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "support.h"

#include "netintf.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#ifdef HAVE_GETADDRINFO
#ifdef AI_NUMERICSERV
#define FC_AI_NUMERICSERV AI_NUMERICSERV
#else  /* AI_NUMERICSERV */
#define FC_AI_NUMERICSERV 0
#endif /* AI_NUMERICSERV */
#endif /* HAVE_GETADDRINFO */

#ifdef FREECIV_HAVE_WINSOCK
/*********************************************************************//**
  Set errno variable on Winsock error
*************************************************************************/
static void set_socket_errno(void)
{
  int err = WSAGetLastError();

  switch(err) {
    /* these have mappings to symbolic errno names in netintf.h */ 
    case WSAEINTR:
    case WSAEWOULDBLOCK:
    case WSAECONNRESET:
    case WSAECONNREFUSED:
    case WSAETIMEDOUT:
    case WSAECONNABORTED:
      errno = err;
      return;
    default:
      bugreport_request("Missing errno mapping for Winsock error #%d.", err);
 
      errno = 0;
  }
}
#endif /* FREECIV_HAVE_WINSOCK */

/*********************************************************************//**
  Connect a socket to an address
*************************************************************************/
int fc_connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen)
{
  int result;
  
  result = connect(sockfd, serv_addr, addrlen);
  
#ifdef FREECIV_HAVE_WINSOCK
  if (result == -1) {
    set_socket_errno();
  }
#endif /* FREECIV_HAVE_WINSOCK */

  return result;
}

/*********************************************************************//**
  Wait for a number of sockets to change status
*************************************************************************/
int fc_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
              fc_timeval *timeout)
{
  int result;

  result = select(n, readfds, writefds, exceptfds, timeout);

#ifdef FREECIV_HAVE_WINSOCK
  if (result == -1) {
    set_socket_errno();
  }
#endif /* FREECIV_HAVE_WINSOCK */

  return result;
}

/*********************************************************************//**
  Read from a socket.
*************************************************************************/
int fc_readsocket(int sock, void *buf, size_t size)
{
  int result;
  
#ifdef FREECIV_HAVE_WINSOCK
  result = recv(sock, buf, size, 0);
  if (result == -1) {
    set_socket_errno();
  }
#else  /* FREECIV_HAVE_WINSOCK */
  result = read(sock, buf, size);
#endif /* FREECIV_HAVE_WINSOCK */

  return result;
}

/*********************************************************************//**
  Write to a socket.
*************************************************************************/
int fc_writesocket(int sock, const void *buf, size_t size)
{
  int result;
        
#ifdef FREECIV_HAVE_WINSOCK
  result = send(sock, buf, size, 0);
  if (result == -1) {
    set_socket_errno();
  }
#else  /* FREECIV_HAVE_WINSOCK */
#  ifdef MSG_NOSIGNAL
  result = send(sock, buf, size, MSG_NOSIGNAL);
#  else  /* MSG_NOSIGNAL */
  result = write(sock, buf, size);
#  endif /* MSG_NOSIGNAL */
#endif /* FREECIV_HAVE_WINSOCK */

  return result;
}

/*********************************************************************//**
  Close a socket.
*************************************************************************/
void fc_closesocket(int sock)
{
#ifdef FREECIV_HAVE_WINSOCK
  closesocket(sock);
#else
  close(sock);
#endif
}

/*********************************************************************//**
  Initialize network stuff.
*************************************************************************/
void fc_init_network(void)
{
#ifdef FREECIV_HAVE_WINSOCK
  WSADATA wsa;

  if (WSAStartup(MAKEWORD(1, 1), &wsa) != 0) {
    log_error("no usable WINSOCK.DLL: %s", fc_strerror(fc_get_errno()));
  }
#endif /* FREECIV_HAVE_WINSOCK */

  /* broken pipes are ignored. */
#ifdef HAVE_SIGPIPE
  (void) signal(SIGPIPE, SIG_IGN);
#endif
}

/*********************************************************************//**
  Shutdown network stuff.
*************************************************************************/
void fc_shutdown_network(void)
{
#ifdef FREECIV_HAVE_WINSOCK
  WSACleanup();
#endif
}

/*********************************************************************//**
  Set socket to non-blocking.
*************************************************************************/
void fc_nonblock(int sockfd)
{
#ifdef NONBLOCKING_SOCKETS
#ifdef FREECIV_HAVE_WINSOCK
#ifdef __LP64__
  unsigned b = 1;
#else  /* __LP64__ */
  u_long b = 1;
#endif /* __LP64__ */

  ioctlsocket(sockfd, FIONBIO, &b);
#else  /* FREECIV_HAVE_WINSOCK */
#ifdef HAVE_FCNTL
  int f_set;

  if ((f_set = fcntl(sockfd, F_GETFL)) == -1) {
    log_error("fcntl F_GETFL failed: %s", fc_strerror(fc_get_errno()));
  }

  f_set |= O_NONBLOCK;

  if (fcntl(sockfd, F_SETFL, f_set) == -1) {
    log_error("fcntl F_SETFL failed: %s", fc_strerror(fc_get_errno()));
  }
#else  /* HAVE_FCNTL */
#ifdef HAVE_IOCTL
  long value=1;

  if (ioctl(sockfd, FIONBIO, (char*)&value) == -1) {
    log_error("ioctl failed: %s", fc_strerror(fc_get_errno()));
  }
#endif /* HAVE_IOCTL */
#endif /* HAVE_FCNTL */
#endif /* FREECIV_HAVE_WINSOCK */
#else  /* NONBLOCKING_SOCKETS */
  log_debug("NONBLOCKING_SOCKETS not available");
#endif /* NONBLOCKING_SOCKETS */
}

/*********************************************************************//**
  Write information about sockaddr to debug log.
*************************************************************************/
void sockaddr_debug(union fc_sockaddr *addr, enum log_level lvl)
{
#ifdef FREECIV_IPV6_SUPPORT
  char buf[INET6_ADDRSTRLEN] = "Unknown";

  if (addr->saddr.sa_family == AF_INET6) { 
    inet_ntop(AF_INET6, &addr->saddr_in6.sin6_addr, buf, INET6_ADDRSTRLEN);
    log_base(lvl, "Host: %s, Port: %d (IPv6)",
             buf, ntohs(addr->saddr_in6.sin6_port));
    return;
  } else if (addr->saddr.sa_family == AF_INET) {
    inet_ntop(AF_INET, &addr->saddr_in4.sin_addr, buf, INET_ADDRSTRLEN);
    log_base(lvl, "Host: %s, Port: %d (IPv4)",
             buf, ntohs(addr->saddr_in4.sin_port));
    return;
  }
#else  /* IPv6 support */
  if (addr->saddr.sa_family == AF_INET) {
    char *buf;

    buf = inet_ntoa(addr->saddr_in4.sin_addr);

    log_base(lvl, "Host: %s, Port: %d",
             buf, ntohs(addr->saddr_in4.sin_port));

    return;
  }
#endif /* IPv6 support */

  log_error("Unsupported address family in sockaddr_debug()");
}

/*********************************************************************//**
  Gets size of address to fc_sockaddr. IPv6/IPv4 must be selected before
  calling this.
*************************************************************************/
int sockaddr_size(union fc_sockaddr *addr)
{
#ifdef FREECIV_MSWINDOWS
  return sizeof(*addr);
#else
#ifdef FREECIV_IPV6_SUPPORT
  if (addr->saddr.sa_family == AF_INET6) {
    return sizeof(addr->saddr_in6);
  } else
#endif /* FREECIV_IPV6_SUPPORT */
  if (addr->saddr.sa_family == AF_INET) {
    return sizeof(addr->saddr_in4);
  } else {
    fc_assert(FALSE);

    log_error("Unsupported address family in sockaddr_size()");

    return 0;
  }
#endif /* FREECIV_MSWINDOWS */
}

/*********************************************************************//**
  Returns wether address is IPv6 address.
*************************************************************************/
bool sockaddr_ipv6(union fc_sockaddr *addr)
{
#ifdef FREECIV_IPV6_SUPPORT
  if (addr->saddr.sa_family == AF_INET6) {
    return TRUE;
  }
#endif /* IPv6 support */

  return FALSE;
}

#ifdef HAVE_GETADDRINFO
/*********************************************************************//**
  Look up the service at hostname:port using getaddrinfo().
*************************************************************************/
static struct fc_sockaddr_list *net_lookup_getaddrinfo(const char *name,
                                                       int port,
                                                       enum fc_addr_family family)
{
  struct addrinfo hints;
  struct addrinfo *res;
  int err;
  char servname[8];
  int gafam;
  struct fc_sockaddr_list *addrs =
      fc_sockaddr_list_new_full((fc_sockaddr_list_free_fn_t) free);

  switch (family) {
    case FC_ADDR_IPV4:
      gafam = AF_INET;
      break;
    case FC_ADDR_IPV6:
      gafam = AF_INET6;
      break;
    case FC_ADDR_ANY:
#ifndef FREECIV_IPV6_SUPPORT
      gafam = AF_INET;
#else
      gafam = AF_UNSPEC;
#endif
      break;
    default:
      fc_assert(FALSE);

      return addrs;
  }

  /* Convert port to string for getaddrinfo() */
  fc_snprintf(servname, sizeof(servname), "%d", port);

  /* Use getaddrinfo() to lookup IPv6 addresses */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = gafam;
  hints.ai_socktype = SOCK_DGRAM; /* any type that uses sin6_port */
  hints.ai_flags = AI_PASSIVE | FC_AI_NUMERICSERV;
  err = getaddrinfo(name, servname, &hints, &res);

  if (err == 0) {
    struct addrinfo *current = res;

    while (current != NULL) {
      union fc_sockaddr *caddr;

      fc_assert_action(current->ai_addrlen <= sizeof(*caddr), continue);
      caddr = fc_malloc(sizeof(*caddr));
      memcpy(caddr, current->ai_addr, current->ai_addrlen);

      fc_sockaddr_list_append(addrs, caddr);

      current = current->ai_next;
    }
    freeaddrinfo(res);
  }

  return addrs;
}
#endif /* HAVE_GETADDRINFO */

/*********************************************************************//**
  Look up the service at hostname:port.
*************************************************************************/
struct fc_sockaddr_list *net_lookup_service(const char *name, int port,
                                            enum fc_addr_family family)
{
  /* IPv6-enabled Freeciv always has HAVE_GETADDRINFO, IPv4-only Freeciv not
   * necessarily */
#ifdef HAVE_GETADDRINFO
  return net_lookup_getaddrinfo(name, port, family);
#else  /* HAVE_GETADDRINFO */

  struct sockaddr_in *sock4;
  struct hostent *hp;
  struct fc_sockaddr_list *addrs = fc_sockaddr_list_new();
  union fc_sockaddr *result = fc_malloc(sizeof(*result));

  sock4 = &result->saddr_in4;

  fc_assert(family != FC_ADDR_IPV6);

  result->saddr.sa_family = AF_INET;
  sock4->sin_port = htons(port);

  if (!name) {
    sock4->sin_addr.s_addr = htonl(INADDR_ANY);
    fc_sockaddr_list_append(addrs, result);

    return addrs;
  }

  if (fc_inet_aton(name, &sock4->sin_addr, FALSE)) {
    fc_sockaddr_list_append(addrs, result);

    return addrs;
  }

  hp = gethostbyname(name);
  if (!hp || hp->h_addrtype != AF_INET) {
    FC_FREE(result);

    return addrs;
  }

  memcpy(&sock4->sin_addr, hp->h_addr, hp->h_length);
  fc_sockaddr_list_append(addrs, result);

  return addrs;

#endif /* !HAVE_GETADDRINFO */

}

/*********************************************************************//**
  Convert internet IPv4 host address to binary form and store it to inp.
  Return FALSE on failure if possible, i.e., FALSE is guarantee that it
  failed but TRUE is not guarantee that it succeeded.
*************************************************************************/
bool fc_inet_aton(const char *cp, struct in_addr *inp, bool addr_none_ok)
{
#ifdef FREECIV_IPV6_SUPPORT
  /* Use inet_pton() */
  if (!inet_pton(AF_INET, cp, &inp->s_addr)) {
    return FALSE;
  }
#else  /* IPv6 Support */
#ifdef HAVE_INET_ATON
  if (!inet_aton(cp, inp)) {
    return FALSE;
  }
#else  /* HAVE_INET_ATON */
  inp->s_addr = inet_addr(cp);
  if (!addr_none_ok && inp->s_addr == INADDR_NONE) {
    return FALSE;
  }
#endif /* HAVE_INET_ATON */
#endif /* IPv6 Support */

  return TRUE;
}

/*********************************************************************//**
  Writes buf to socket and returns the response in an fz_FILE.
  Use only on blocking sockets.
*************************************************************************/
fz_FILE *fc_querysocket(int sock, void *buf, size_t size)
{
  FILE *fp;

#ifdef HAVE_FDOPEN
  fp = fdopen(sock, "r+b");
  if (fwrite(buf, 1, size, fp) != size) {
    log_error("socket %d: write error", sock);
  }
  fflush(fp);

  /* we don't use fc_closesocket on sock here since when fp is closed
   * sock will also be closed. fdopen doesn't dup the socket descriptor. */
#else  /* HAVE_FDOPEN */
  {
    char tmp[4096];
    int n;

#ifdef FREECIV_MSWINDOWS
    /* tmpfile() in mingw attempts to make a temp file in the root directory
     * of the current drive, which we may not have write access to. */
    {
      char filename[MAX_PATH];

      GetTempPath(sizeof(filename), filename);
      sz_strlcat(filename, "fctmp");

      fp = fc_fopen(filename, "w+b");
    }
#else  /* FREECIV_MSWINDOWS */

    fp = tmpfile();

#endif /* FREECIV_MSWINDOWS */

    if (fp == NULL) {
      return NULL;
    }

    fc_writesocket(sock, buf, size);

    while ((n = fc_readsocket(sock, tmp, sizeof(tmp))) > 0) {
      if (fwrite(tmp, 1, n, fp) != n) {
        log_error("socket %d: write error", sock);
      }
    }
    fflush(fp);

    fc_closesocket(sock);

    rewind(fp);
  }
#endif /* HAVE_FDOPEN */

  return fz_from_stream(fp);
}

/*********************************************************************//**
  Finds the next (lowest) free port.
*************************************************************************/ 
int find_next_free_port(int starting_port, int highest_port,
                        enum fc_addr_family family,
                        char *net_interface, bool not_avail_ok)
{
  int port;
  int s;
  int gafamily;
  bool found = FALSE;

#ifndef FREECIV_IPV6_SUPPORT
  fc_assert(family == FC_ADDR_IPV4 || family == FC_ADDR_ANY);
#endif

  switch (family) {
   case FC_ADDR_IPV4:
     gafamily = AF_INET;
     break;
#ifdef FREECIV_IPV6_SUPPORT
   case FC_ADDR_IPV6:
     gafamily = AF_INET6;
     break;
#endif /* FREECIV_IPV6_SUPPORT */
   case FC_ADDR_ANY:
     gafamily = AF_UNSPEC;
     break;
   default:
     fc_assert(FALSE);
     log_error("Port from unsupported address family requested!");

     return -1;
  }

  for (port = starting_port; !found && highest_port > port; port++) {
    /* HAVE_GETADDRINFO implies IPv6 support */
#ifdef HAVE_GETADDRINFO
    struct addrinfo hints;
    int err;
    char servname[8];
    struct addrinfo *res;

    fc_snprintf(servname, sizeof(servname), "%d", port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = gafamily;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE | FC_AI_NUMERICSERV;

    err = getaddrinfo(net_interface, servname, &hints, &res);
    if (!err) {
      struct addrinfo *current = res;
      bool unusable = FALSE;

      while (current != NULL && !unusable) {
        s = socket(current->ai_family, SOCK_STREAM, 0);

        if (s == -1) {
          log_error("socket(): %s", fc_strerror(fc_get_errno()));
        } else {
          if (bind(s, current->ai_addr, current->ai_addrlen) != 0) {
            if (!not_avail_ok || fc_get_errno() != EADDRNOTAVAIL) {
              unusable = TRUE;
            }
          }
        }
        current = current->ai_next;
        fc_closesocket(s);
      }

      freeaddrinfo(res);

      if (!unusable && res != NULL) {
        found = TRUE;
      }
    }
#else /* HAVE_GETADDRINFO */
    union fc_sockaddr tmp;
    struct sockaddr_in *sock4;

    s = socket(gafamily, SOCK_STREAM, 0);

    sock4 = &tmp.saddr_in4;
    memset(&tmp, 0, sizeof(tmp));
    sock4->sin_family = AF_INET;
    sock4->sin_port = htons(port);
    if (net_interface != NULL) {
      if (!fc_inet_aton(net_interface, &sock4->sin_addr, FALSE)) {
        struct hostent *hp;

        hp = gethostbyname(net_interface);
        if (hp == NULL) {
          log_error("No hostent for %s!", net_interface);

          return -1;
        }
        if (hp->h_addrtype != AF_INET) {
          log_error("Requested IPv4 address for %s, got something else! (%d)",
                    net_interface, hp->h_addrtype);

          return -1;
        }

        memcpy(&sock4->sin_addr, hp->h_addr, hp->h_length);
      }
    } else {
      sock4->sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(s, &tmp.saddr, sockaddr_size(&tmp)) == 0) {
      found = TRUE;
    }

    fc_closesocket(s);
#endif /* HAVE_GETADDRINFO */
  }

  if (!found) {
    log_error("None of the ports %d - %d is available.",
              starting_port, highest_port);

    return -1;
  }

  /* Rollback the last increment from the loop, back to the port
   * number found to be free. */
  port--;

  return port;
}
