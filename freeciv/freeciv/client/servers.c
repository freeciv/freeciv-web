/***********************************************************************
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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include "fc_prehdrs.h"

#include <errno.h>
#include <stdlib.h>

#ifdef FREECIV_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* dependencies */
#include "cvercmp.h"

/* utility */
#include "fcintl.h"
#include "fcthread.h"
#include "log.h"
#include "mem.h"
#include "netfile.h"
#include "netintf.h"
#include "rand.h" /* fc_rand() */
#include "registry.h"
#include "support.h"

/* common */
#include "capstr.h"
#include "dataio.h"
#include "game.h"
#include "packets.h"
#include "version.h"

/* client */
#include "chatline_common.h"
#include "chatline_g.h"
#include "client_main.h"
#include "servers.h"

#include "gui_main_g.h"

struct server_scan {
  enum server_scan_type type;
  ServerScanErrorFunc error_func;

  struct srv_list srvrs;
  int sock;

  /* Only used for metaserver */
  struct {
    enum server_scan_status status;

    fc_thread thr;
    fc_mutex mutex;

    const char *urlpath;
    struct netfile_write_cb_data mem;
  } meta;
};

extern enum announce_type announce;

static bool begin_metaserver_scan(struct server_scan *scan);
static void delete_server_list(struct server_list *server_list);

/**********************************************************************//**
  The server sends a stream in a registry 'ini' type format.
  Read it using secfile functions and fill the server_list structs.
**************************************************************************/
static struct server_list *parse_metaserver_data(fz_FILE *f)
{
  struct server_list *server_list;
  struct section_file *file;
  int nservers, i, j;
  const char *latest_ver;
  const char *comment;

  /* This call closes f. */
  if (!(file = secfile_from_stream(f, TRUE))) {
    return NULL;
  }

  latest_ver = secfile_lookup_str_default(file, NULL, "versions." FOLLOWTAG);
  comment = secfile_lookup_str_default(file, NULL, "version_comments." FOLLOWTAG);

  if (latest_ver != NULL) {
    const char *my_comparable = fc_comparable_version();
    char vertext[2048];

    log_verbose("Metaserver says latest '" FOLLOWTAG "' version is '%s'; we have '%s'",
                latest_ver, my_comparable);
    if (cvercmp_greater(latest_ver, my_comparable)) {
      const char *const followtag = "?vertag:" FOLLOWTAG;
      fc_snprintf(vertext, sizeof(vertext),
                  /* TRANS: Type is version tag name like "stable", "S2_4",
                   * "win32" (which can also be localised -- msgids start
                   * '?vertag:') */
                  _("Latest %s release of Freeciv is %s, this is %s."),
                  Q_(followtag), latest_ver, my_comparable);

      version_message(vertext);
    } else if (comment == NULL) {
      fc_snprintf(vertext, sizeof(vertext),
                  _("There is no newer %s release of Freeciv available."),
                  FOLLOWTAG);

      version_message(vertext);
    }
  }

  if (comment != NULL) {
    log_verbose("Mesaserver comment about '" FOLLOWTAG "': %s", comment);
    version_message(comment);
  }

  server_list = server_list_new();
  nservers = secfile_lookup_int_default(file, 0, "main.nservers");

  for (i = 0; i < nservers; i++) {
    const char *host, *port, *version, *state, *message, *nplayers, *nhumans;
    int n;
    struct server *pserver = (struct server*)fc_malloc(sizeof(struct server));

    host = secfile_lookup_str_default(file, "", "server%d.host", i);
    pserver->host = fc_strdup(host);

    port = secfile_lookup_str_default(file, "", "server%d.port", i);
    pserver->port = atoi(port);

    version = secfile_lookup_str_default(file, "", "server%d.version", i);
    pserver->version = fc_strdup(version);

    state = secfile_lookup_str_default(file, "", "server%d.state", i);
    pserver->state = fc_strdup(state);

    message = secfile_lookup_str_default(file, "", "server%d.message", i);
    pserver->message = fc_strdup(message);

    nplayers = secfile_lookup_str_default(file, "0", "server%d.nplayers", i);
    n = atoi(nplayers);
    pserver->nplayers = n;

    nhumans = secfile_lookup_str_default(file, "-1", "server%d.humans", i);
    n = atoi(nhumans);
    pserver->humans = n;

    if (pserver->nplayers > 0) {
      pserver->players = fc_malloc(pserver->nplayers * sizeof(*pserver->players));
    } else {
      pserver->players = NULL;
    }

    for (j = 0; j < pserver->nplayers ; j++) {
      const char *name, *nation, *type, *plrhost;

      name = secfile_lookup_str_default(file, "", 
                                        "server%d.player%d.name", i, j);
      pserver->players[j].name = fc_strdup(name);

      type = secfile_lookup_str_default(file, "",
                                        "server%d.player%d.type", i, j);
      pserver->players[j].type = fc_strdup(type);

      plrhost = secfile_lookup_str_default(file, "", 
                                           "server%d.player%d.host", i, j);
      pserver->players[j].host = fc_strdup(plrhost);

      nation = secfile_lookup_str_default(file, "",
                                          "server%d.player%d.nation", i, j);
      pserver->players[j].nation = fc_strdup(nation);
    }

    server_list_append(server_list, pserver);
  }

  secfile_destroy(file);

  return server_list;
}

/**********************************************************************//**
  Read the reply string from the metaserver.
**************************************************************************/
static bool meta_read_response(struct server_scan *scan)
{
  fz_FILE *f;
  char str[4096];
  struct server_list *srvrs;

  f = fz_from_memory(scan->meta.mem.mem, scan->meta.mem.size, TRUE);
  if (NULL == f) {
    fc_snprintf(str, sizeof(str),
                _("Failed to read the metaserver data from %s."),
                metaserver);
    scan->error_func(scan, str);

    return FALSE;
  }

  /* parse message body */
  fc_allocate_mutex(&scan->srvrs.mutex);
  srvrs = parse_metaserver_data(f);
  scan->srvrs.servers = srvrs;
  fc_release_mutex(&scan->srvrs.mutex);

  /* 'f' (hence 'meta.mem.mem') was closed in parse_metaserver_data(). */
  scan->meta.mem.mem = NULL;

  if (NULL == srvrs) {
    fc_snprintf(str, sizeof(str),
                _("Failed to parse the metaserver data from %s:\n"
                  "%s."),
                metaserver, secfile_error());
    scan->error_func(scan, str);

    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Metaserver scan thread entry point
**************************************************************************/
static void metaserver_scan(void *arg)
{
  struct server_scan *scan = arg;

  if (!begin_metaserver_scan(scan)) {
    fc_allocate_mutex(&scan->meta.mutex);
    scan->meta.status = SCAN_STATUS_ERROR;
  } else {
    if (!meta_read_response(scan)) {
      fc_allocate_mutex(&scan->meta.mutex);
      scan->meta.status = SCAN_STATUS_ERROR;
    } else {
      fc_allocate_mutex(&scan->meta.mutex);
      if (scan->meta.status == SCAN_STATUS_WAITING) {
        scan->meta.status = SCAN_STATUS_DONE;
      }
    }
  }

  fc_release_mutex(&scan->meta.mutex);
}

/**********************************************************************//**
  Begin a metaserver scan for servers.

  Returns FALSE on error (in which case errbuf will contain an error
  message).
**************************************************************************/
static bool begin_metaserver_scan(struct server_scan *scan)
{
  struct netfile_post *post;
  bool retval = TRUE;

  post = netfile_start_post();
  netfile_add_form_str(post, "client_cap", our_capability);

  if (!netfile_send_post(metaserver, post, NULL, &scan->meta.mem, NULL)) {
    scan->error_func(scan, _("Error connecting to metaserver"));
    retval = FALSE;
  }

  netfile_close_post(post);

  return retval;
}

/**********************************************************************//**
  Frees everything associated with a server list including
  the server list itself (so the server_list is no longer
  valid after calling this function)
**************************************************************************/
static void delete_server_list(struct server_list *server_list)
{
  if (!server_list) {
    return;
  }

  server_list_iterate(server_list, ptmp) {
    int i;
    int n = ptmp->nplayers;

    free(ptmp->host);
    free(ptmp->version);
    free(ptmp->state);
    free(ptmp->message);

    if (ptmp->players) {
      for (i = 0; i < n; i++) {
        free(ptmp->players[i].name);
        free(ptmp->players[i].type);
        free(ptmp->players[i].host);
        free(ptmp->players[i].nation);
      }
      free(ptmp->players);
    }

    free(ptmp);
  } server_list_iterate_end;

  server_list_destroy(server_list);
}

/**********************************************************************//**
  Broadcast an UDP package to all servers on LAN, requesting information
  about the server. The packet is send to all Freeciv servers in the same
  multicast group as the client.
**************************************************************************/
static bool begin_lanserver_scan(struct server_scan *scan)
{
  union fc_sockaddr addr;
  struct raw_data_out dout;
  int send_sock, opt = 1;
#ifndef FREECIV_HAVE_WINSOCK
  unsigned char buffer[MAX_LEN_PACKET];
#else  /* FREECIV_HAVE_WINSOCK */
  char buffer[MAX_LEN_PACKET];
#endif /* FREECIV_HAVE_WINSOCK */
#ifdef HAVE_IP_MREQN
  struct ip_mreqn mreq4;
#else
  struct ip_mreq mreq4;
#endif
  const char *group;
  size_t size;
  int family;

#ifdef FREECIV_IPV6_SUPPORT
  struct ipv6_mreq mreq6;
#endif

#ifndef FREECIV_HAVE_WINSOCK
  unsigned char ttl;
#endif

  if (announce == ANNOUNCE_NONE) {
    /* Succeeded in doing nothing */
    return TRUE;
  }

#ifdef FREECIV_IPV6_SUPPORT
  if (announce == ANNOUNCE_IPV6) {
    family = AF_INET6;
  } else
#endif /* IPv6 support */
  {
    family = AF_INET;
  }

  /* Set the UDP Multicast group IP address. */
  group = get_multicast_group(announce == ANNOUNCE_IPV6);

  /* Create a socket for listening for server packets. */
  if ((scan->sock = socket(family, SOCK_DGRAM, 0)) < 0) {
    char errstr[2048];

    fc_snprintf(errstr, sizeof(errstr),
                _("Opening socket to listen LAN announcements failed:\n%s"),
                fc_strerror(fc_get_errno()));
    scan->error_func(scan, errstr);

    return FALSE;
  }

  fc_nonblock(scan->sock);

  if (setsockopt(scan->sock, SOL_SOCKET, SO_REUSEADDR,
                 (char *)&opt, sizeof(opt)) == -1) {
    log_error("SO_REUSEADDR failed: %s", fc_strerror(fc_get_errno()));
  }

  memset(&addr, 0, sizeof(addr));

#ifdef FREECIV_IPV6_SUPPORT
  if (family == AF_INET6) {
    addr.saddr.sa_family = AF_INET6;
    addr.saddr_in6.sin6_port = htons(SERVER_LAN_PORT + 1);
    addr.saddr_in6.sin6_addr = in6addr_any;
  } else
#endif /* IPv6 support */
  if (family == AF_INET) {
    addr.saddr.sa_family = AF_INET;
    addr.saddr_in4.sin_port = htons(SERVER_LAN_PORT + 1);
    addr.saddr_in4.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    /* This is not only error situation worth assert() This
     * is error situation that has check (with assert) against
     * earlier already. */
    fc_assert(FALSE);

    return FALSE;
  }

  if (bind(scan->sock, &addr.saddr, sockaddr_size(&addr)) < 0) {
    char errstr[2048];

    fc_snprintf(errstr, sizeof(errstr),
                _("Binding socket to listen LAN announcements failed:\n%s"),
                fc_strerror(fc_get_errno()));
    scan->error_func(scan, errstr);

    return FALSE;
  }

#ifdef FREECIV_IPV6_SUPPORT
  if (family == AF_INET6) {
    inet_pton(AF_INET6, group, &mreq6.ipv6mr_multiaddr.s6_addr);
    mreq6.ipv6mr_interface = 0; /* TODO: Interface selection */

    if (setsockopt(scan->sock, IPPROTO_IPV6, FC_IPV6_ADD_MEMBERSHIP,
                   (const char*)&mreq6, sizeof(mreq6)) < 0) {
      char errstr[2048];

      fc_snprintf(errstr, sizeof(errstr),
                  _("Adding membership for IPv6 LAN announcement group failed:\n%s"),
                fc_strerror(fc_get_errno()));
      scan->error_func(scan, errstr);
    }
  } else
#endif /* IPv6 support */
  {
    fc_inet_aton(group, &mreq4.imr_multiaddr, FALSE);
#ifdef HAVE_IP_MREQN
    mreq4.imr_address.s_addr = htonl(INADDR_ANY);
    mreq4.imr_ifindex = 0;
#else
     mreq4.imr_interface.s_addr = htonl(INADDR_ANY);
#endif

    if (setsockopt(scan->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   (const char*)&mreq4, sizeof(mreq4)) < 0) {
      char errstr[2048];

      fc_snprintf(errstr, sizeof(errstr),
                  _("Adding membership for IPv4 LAN announcement group failed:\n%s"),
                  fc_strerror(fc_get_errno()));
      scan->error_func(scan, errstr);

      return FALSE;
    }
  }

  /* Create a socket for broadcasting to servers. */
  if ((send_sock = socket(family, SOCK_DGRAM, 0)) < 0) {
    log_error("socket failed: %s", fc_strerror(fc_get_errno()));
    return FALSE;
  }

  memset(&addr, 0, sizeof(addr));

#ifdef FREECIV_IPV6_SUPPORT
  if (family == AF_INET6) {
    addr.saddr.sa_family = AF_INET6;
    inet_pton(AF_INET6, group, &addr.saddr_in6.sin6_addr);
    addr.saddr_in6.sin6_port = htons(SERVER_LAN_PORT);
  } else
#endif /* IPv6 Support */
  if (family == AF_INET) {
    fc_inet_aton(group, &addr.saddr_in4.sin_addr, FALSE);
    addr.saddr.sa_family = AF_INET;
    addr.saddr_in4.sin_port = htons(SERVER_LAN_PORT);
  } else {
    fc_assert(FALSE);

    log_error("Unsupported address family in begin_lanserver_scan()");

    return FALSE;
  }

/* this setsockopt call fails on Windows 98, so we stick with the default
 * value of 1 on Windows, which should be fine in most cases */
#ifndef FREECIV_HAVE_WINSOCK
  /* Set the Time-to-Live field for the packet  */
  ttl = SERVER_LAN_TTL;
  if (setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&ttl, 
                 sizeof(ttl))) {
    log_error("setsockopt failed: %s", fc_strerror(fc_get_errno()));
    return FALSE;
  }
#endif /* FREECIV_HAVE_WINSOCK */

  if (setsockopt(send_sock, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, 
                 sizeof(opt))) {
    log_error("setsockopt failed: %s", fc_strerror(fc_get_errno()));
    return FALSE;
  }

  dio_output_init(&dout, buffer, sizeof(buffer));
  dio_put_uint8_raw(&dout, SERVER_LAN_VERSION);
  size = dio_output_used(&dout);
 

  if (sendto(send_sock, buffer, size, 0, &addr.saddr,
             sockaddr_size(&addr)) < 0) {
    /* This can happen when there's no network connection - it should
     * give an in-game message. */
    log_error("lanserver scan sendto failed: %s",
              fc_strerror(fc_get_errno()));
    return FALSE;
  } else {
    log_debug("Sending request for server announcement on LAN.");
  }

  fc_closesocket(send_sock);

  fc_allocate_mutex(&scan->srvrs.mutex);
  scan->srvrs.servers = server_list_new();
  fc_release_mutex(&scan->srvrs.mutex);

  return TRUE;
}

/**********************************************************************//**
  Listens for UDP packets broadcasted from a server that responded
  to the request-packet sent from the client.
**************************************************************************/
static enum server_scan_status
get_lan_server_list(struct server_scan *scan)
{
  socklen_t fromlen;
  union fc_sockaddr fromend;
  char msgbuf[128];
  int type;
  struct data_in din;
  char servername[512];
  char portstr[256];
  int port;
  char version[256];
  char status[256];
  char players[256];
  char humans[256];
  char message[1024];
  bool found_new = FALSE;

  while (TRUE) {
    struct server *pserver;
    bool duplicate = FALSE;

    dio_input_init(&din, msgbuf, sizeof(msgbuf));
    fromlen = sizeof(fromend);

    /* Try to receive a packet from a server.  No select loop is needed;
     * we just keep on reading until recvfrom returns -1. */
    if (recvfrom(scan->sock, msgbuf, sizeof(msgbuf), 0,
		 &fromend.saddr, &fromlen) < 0) {
      break;
    }

    dio_get_uint8_raw(&din, &type);
    if (type != SERVER_LAN_VERSION) {
      continue;
    }
    dio_get_string_raw(&din, servername, sizeof(servername));
    dio_get_string_raw(&din, portstr, sizeof(portstr));
    port = atoi(portstr);
    dio_get_string_raw(&din, version, sizeof(version));
    dio_get_string_raw(&din, status, sizeof(status));
    dio_get_string_raw(&din, players, sizeof(players));
    dio_get_string_raw(&din, humans, sizeof(humans));
    dio_get_string_raw(&din, message, sizeof(message));

    if (!fc_strcasecmp("none", servername)) {
      bool nameinfo = FALSE;
#ifdef FREECIV_IPV6_SUPPORT
      char dst[INET6_ADDRSTRLEN];
      char host[NI_MAXHOST], service[NI_MAXSERV];

      if (!getnameinfo(&fromend.saddr, fromlen, host, NI_MAXHOST,
                       service, NI_MAXSERV, NI_NUMERICSERV)) {
        nameinfo = TRUE;
      }
      if (!nameinfo) {
        if (fromend.saddr.sa_family == AF_INET6) {
          inet_ntop(AF_INET6, &fromend.saddr_in6.sin6_addr,
                    dst, sizeof(dst));
        } else if (fromend.saddr.sa_family == AF_INET) {
          inet_ntop(AF_INET, &fromend.saddr_in4.sin_addr, dst, sizeof(dst));;
        } else {
	  fc_assert(FALSE);

	  log_error("Unsupported address family in get_lan_server_list()");

	  fc_snprintf(dst, sizeof(dst), "Unknown");
	}
      }
#else  /* IPv6 support */
      const char *dst = NULL;
      struct hostent *from;
      const char *host = NULL;

      from = gethostbyaddr((char *) &fromend.saddr_in4.sin_addr,
			   sizeof(fromend.saddr_in4.sin_addr), AF_INET);
      if (from) {
        host = from->h_name;
        nameinfo = TRUE;
      }
      if (!nameinfo) {
        dst = inet_ntoa(fromend.saddr_in4.sin_addr);
      }
#endif /* IPv6 support */

      sz_strlcpy(servername, nameinfo ? host : dst);
    }

    /* UDP can send duplicate or delayed packets. */
    fc_allocate_mutex(&scan->srvrs.mutex);
    server_list_iterate(scan->srvrs.servers, aserver) {
      if (0 == fc_strcasecmp(aserver->host, servername)
          && aserver->port == port) {
	duplicate = TRUE;
	break;
      }
    } server_list_iterate_end;

    if (duplicate) {
      fc_release_mutex(&scan->srvrs.mutex);
      continue;
    }

    log_debug("Received a valid announcement from a server on the LAN.");

    pserver = fc_malloc(sizeof(*pserver));
    pserver->host = fc_strdup(servername);
    pserver->port = port;
    pserver->version = fc_strdup(version);
    pserver->state = fc_strdup(status);
    pserver->nplayers = atoi(players);
    pserver->humans = atoi(humans);
    pserver->message = fc_strdup(message);
    pserver->players = NULL;
    found_new = TRUE;

    server_list_prepend(scan->srvrs.servers, pserver);
    fc_release_mutex(&scan->srvrs.mutex);
  }

  if (found_new) {
    return SCAN_STATUS_PARTIAL;
  }
  return SCAN_STATUS_WAITING;
}

/**********************************************************************//**
  Creates a new server scan and returns it, or NULL if impossible.

  Depending on 'type' the scan will look for either local or internet
  games.

  error_func provides a callback to be used in case of error; this
  callback probably should call server_scan_finish.

  NB: You must call server_scan_finish() when you are done with the
  scan to free the memory and resources allocated by it.
**************************************************************************/
struct server_scan *server_scan_begin(enum server_scan_type type,
                                      ServerScanErrorFunc error_func)
{
  struct server_scan *scan;
  bool ok = FALSE;

  scan = fc_calloc(1, sizeof(*scan));
  scan->type = type;
  scan->error_func = error_func;
  scan->sock = -1;
  fc_init_mutex(&scan->srvrs.mutex);

  switch (type) {
  case SERVER_SCAN_GLOBAL:
    {
      int thr_ret;

      fc_init_mutex(&scan->meta.mutex);
      scan->meta.status = SCAN_STATUS_WAITING;
      thr_ret = fc_thread_start(&scan->meta.thr, metaserver_scan, scan);
      if (thr_ret) {
        ok = FALSE;
      } else {
        ok = TRUE;
      }
    }
    break;
  case SERVER_SCAN_LOCAL:
    ok = begin_lanserver_scan(scan);
    break;
  default:
    break;
  }

  if (!ok) {
    server_scan_finish(scan);
    scan = NULL;
  }

  return scan;
}

/**********************************************************************//**
  A simple query function to determine the type of a server scan (previously
  allocated in server_scan_begin).
**************************************************************************/
enum server_scan_type server_scan_get_type(const struct server_scan *scan)
{
  if (!scan) {
    return SERVER_SCAN_LAST;
  }
  return scan->type;
}

/**********************************************************************//**
  A function to query servers of the server scan. This will check any
  pending network data and update the server list.

  The return value indicates the status of the server scan:
    SCAN_STATUS_ERROR   - The scan failed and should be aborted.
    SCAN_STATUS_WAITING - The scan is in progress (continue polling).
    SCAN_STATUS_PARTIAL - The scan received some data, with more expected.
                          Get the servers with server_scan_get_list(), and
                          continue polling.
    SCAN_STATUS_DONE    - The scan received all data it expected to receive.
                          Get the servers with server_scan_get_list(), and
                          stop calling this function.
    SCAN_STATUS_ABORT   - The scan has been aborted
**************************************************************************/
enum server_scan_status server_scan_poll(struct server_scan *scan)
{
  if (!scan) {
    return SCAN_STATUS_ERROR;
  }

  switch (scan->type) {
  case SERVER_SCAN_GLOBAL:
    {
      enum server_scan_status status;

      fc_allocate_mutex(&scan->meta.mutex);
      status = scan->meta.status;
      fc_release_mutex(&scan->meta.mutex);

      return status;
    }
    break;
  case SERVER_SCAN_LOCAL:
    return get_lan_server_list(scan);
    break;
  default:
    break;
  }

  return SCAN_STATUS_ERROR;
}

/**********************************************************************//**
  Returns the srv_list currently held by the scan (may be NULL).
**************************************************************************/
struct srv_list *
server_scan_get_list(struct server_scan *scan)
{
  if (!scan) {
    return NULL;
  }

  return &scan->srvrs;
}

/**********************************************************************//**
  Closes the socket listening on the scan, frees the list of servers, and
  frees the memory allocated for 'scan' by server_scan_begin().
**************************************************************************/
void server_scan_finish(struct server_scan *scan)
{
  if (!scan) {
    return;
  }

  if (scan->type == SERVER_SCAN_GLOBAL) {
    /* Signal metaserver scan thread to stop */
    fc_allocate_mutex(&scan->meta.mutex);
    scan->meta.status = SCAN_STATUS_ABORT;
    fc_release_mutex(&scan->meta.mutex);

    /* Wait thread to stop */
    fc_thread_wait(&scan->meta.thr);
    fc_destroy_mutex(&scan->meta.mutex);

    /* This mainly duplicates code from below "else" block.
     * That's intentional, since they will be completely different in future versions.
     * We are better prepared for that by having them separately already. */
    if (scan->sock >= 0) {
      fc_closesocket(scan->sock);
      scan->sock = -1;
    }

    if (scan->srvrs.servers) {
      fc_allocate_mutex(&scan->srvrs.mutex);
      delete_server_list(scan->srvrs.servers);
      scan->srvrs.servers = NULL;
      fc_release_mutex(&scan->srvrs.mutex);
    }

    if (scan->meta.mem.mem) {
      FC_FREE(scan->meta.mem.mem);
      scan->meta.mem.mem = NULL;
    }
  } else {
    if (scan->sock >= 0) {
      fc_closesocket(scan->sock);
      scan->sock = -1;
    }

    if (scan->srvrs.servers) {
      delete_server_list(scan->srvrs.servers);
      scan->srvrs.servers = NULL;
    }
  }

  fc_destroy_mutex(&scan->srvrs.mutex);

  free(scan);
}
