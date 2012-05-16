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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdlib.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_PWD_H
#include <pwd.h>
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
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WINSOCK
#include <winsock.h>
#endif

/* utility */
#include "fcintl.h"
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "rand.h" /* myrand() */
#include "registry.h"
#include "support.h"

/* common */
#include "capstr.h"
#include "dataio.h"
#include "game.h"
#include "packets.h"
#include "version.h"

/* client */
#include "client_main.h"
#include "servers.h"

#include "gui_main_g.h"

struct server_scan {
  enum server_scan_type type;
  ServerScanErrorFunc error_func;

  struct server_list *servers;
  int sock;

  /* Only used for metaserver */
  struct {
    enum {
      META_CONNECTING,
      META_WAITING,
      META_DONE
    } state;
    char name[MAX_LEN_ADDR];
    int port;
    const char *urlpath;
    FILE *fp; /* temp file */
  } meta;
};

extern enum announce_type announce;

/**************************************************************************
 The server sends a stream in a registry 'ini' type format.
 Read it using secfile functions and fill the server_list structs.
**************************************************************************/
static struct server_list *parse_metaserver_data(fz_FILE *f)
{
  struct server_list *server_list;
  struct section_file the_file, *file = &the_file;
  int nservers, i, j;

  server_list = server_list_new();

  /* This call closes f. */
  if (!section_file_load_from_stream(file, f)) {
    return server_list;
  }

  nservers = secfile_lookup_int_default(file, 0, "main.nservers");

  for (i = 0; i < nservers; i++) {
    char *host, *port, *version, *state, *message, *nplayers;
    int n;
    struct server *pserver = (struct server*)fc_malloc(sizeof(struct server));

    host = secfile_lookup_str_default(file, "", "server%d.host", i);
    pserver->host = mystrdup(host);

    port = secfile_lookup_str_default(file, "", "server%d.port", i);
    pserver->port = atoi(port);

    version = secfile_lookup_str_default(file, "", "server%d.version", i);
    pserver->version = mystrdup(version);

    state = secfile_lookup_str_default(file, "", "server%d.state", i);
    pserver->state = mystrdup(state);

    message = secfile_lookup_str_default(file, "", "server%d.message", i);
    pserver->message = mystrdup(message);

    nplayers = secfile_lookup_str_default(file, "0", "server%d.nplayers", i);
    n = atoi(nplayers);
    pserver->nplayers = n;

    if (n > 0) {
      pserver->players = fc_malloc(n * sizeof(*pserver->players));
    } else {
      pserver->players = NULL;
    }
      
    for (j = 0; j < n; j++) {
      char *name, *nation, *type, *host;

      name = secfile_lookup_str_default(file, "", 
                                        "server%d.player%d.name", i, j);
      pserver->players[j].name = mystrdup(name);

      type = secfile_lookup_str_default(file, "",
                                        "server%d.player%d.type", i, j);
      pserver->players[j].type = mystrdup(type);

      host = secfile_lookup_str_default(file, "", 
                                        "server%d.player%d.host", i, j);
      pserver->players[j].host = mystrdup(host);

      nation = secfile_lookup_str_default(file, "",
                                          "server%d.player%d.nation", i, j);
      pserver->players[j].nation = mystrdup(nation);
    }

    server_list_append(server_list, pserver);
  }

  section_file_free(file);
  return server_list;
}

/*****************************************************************
  Returns an uname like string.
*****************************************************************/
static void my_uname(char *buf, size_t len)
{
#ifdef HAVE_UNAME
  {
    struct utsname un;

    uname(&un);
    my_snprintf(buf, len,
		"%s %s [%s]",
		un.sysname,
		un.release,
		un.machine);
  }
#else /* ! HAVE_UNAME */
  /* Fill in here if you are making a binary without sys/utsname.h and know
     the OS name, release number, and machine architechture */
#ifdef WIN32_NATIVE
  {
    char cpuname[16];
    char *osname;
    SYSTEM_INFO sysinfo;
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    switch (osvi.dwPlatformId) {
    case VER_PLATFORM_WIN32s:
      osname = "Win32s";
      break;

    case VER_PLATFORM_WIN32_WINDOWS:
      osname = "Win32";

      if (osvi.dwMajorVersion == 4) {
	switch (osvi.dwMinorVersion) {
	case  0: osname = "Win95";    break;
	case 10: osname = "Win98";    break;
	case 90: osname = "WinME";    break;
	default:			    break;
	}
      }
      break;

    case VER_PLATFORM_WIN32_NT:
      osname = "WinNT";

      if (osvi.dwMajorVersion == 5) {
	switch (osvi.dwMinorVersion) {
	case 0: osname = "Win2000";   break;
	case 1: osname = "WinXP";	    break;
	default:			    break;
	}
      }
      break;

    default:
      osname = osvi.szCSDVersion;
      break;
    }

    GetSystemInfo(&sysinfo); 
    switch (sysinfo.wProcessorArchitecture) {
      case PROCESSOR_ARCHITECTURE_INTEL:
	{
	  unsigned int ptype;
	  if (sysinfo.wProcessorLevel < 3) /* Shouldn't happen. */
	    ptype = 3;
	  else if (sysinfo.wProcessorLevel > 9) /* P4 */
	    ptype = 6;
	  else
	    ptype = sysinfo.wProcessorLevel;
	  
	  my_snprintf(cpuname, sizeof(cpuname), "i%d86", ptype);
	}
	break;

      case PROCESSOR_ARCHITECTURE_MIPS:
	sz_strlcpy(cpuname, "mips");
	break;

      case PROCESSOR_ARCHITECTURE_ALPHA:
	sz_strlcpy(cpuname, "alpha");
	break;

      case PROCESSOR_ARCHITECTURE_PPC:
	sz_strlcpy(cpuname, "ppc");
	break;
#if 0
      case PROCESSOR_ARCHITECTURE_IA64:
	sz_strlcpy(cpuname, "ia64");
	break;
#endif
      default:
	sz_strlcpy(cpuname, "unknown");
	break;
    }
    my_snprintf(buf, len,
		"%s %ld.%ld [%s]",
		osname, osvi.dwMajorVersion, osvi.dwMinorVersion,
		cpuname);
  }
#else
  my_snprintf(buf, len,
              "unknown unknown [unknown]");
#endif
#endif /* HAVE_UNAME */
}

/****************************************************************************
  Send the request string to the metaserver.
****************************************************************************/
static void meta_send_request(struct server_scan *scan)
{
  const char *capstr;
  char str[MAX_LEN_PACKET];
  char machine_string[128];

  my_uname(machine_string, sizeof(machine_string));

  capstr = fc_url_encode(our_capability);

  my_snprintf(str, sizeof(str),
    "POST %s HTTP/1.1\r\n"
    "Host: %s:%d\r\n"
    "User-Agent: Freeciv/%s %s %s\r\n"
    "Connection: close\r\n"
    "Content-Type: application/x-www-form-urlencoded; charset=\"utf-8\"\r\n"
    "Content-Length: %lu\r\n"
    "\r\n"
    "client_cap=%s\r\n",
    scan->meta.urlpath,
    scan->meta.name, scan->meta.port,
    VERSION_STRING, client_string, machine_string,
    (unsigned long) (strlen("client_cap=") + strlen(capstr)),
    capstr);

  if (fc_writesocket(scan->sock, str, strlen(str)) != strlen(str)) {
    /* Even with non-blocking this shouldn't fail. */
    scan->error_func(scan, fc_strerror(fc_get_errno()));
    return;
  }

  scan->meta.state = META_WAITING;
}

/****************************************************************************
  Read the request string (or part of it) from the metaserver.
****************************************************************************/
static void meta_read_response(struct server_scan *scan)
{
  char buf[4096];
  int result;

  if (!scan->meta.fp) {
#ifdef WIN32_NATIVE
    char filename[MAX_PATH];

    GetTempPath(sizeof(filename), filename);
    cat_snprintf(filename, sizeof(filename), "fctmp%d", myrand(1000));

    scan->meta.fp = fopen(filename, "w+b");
#else
    scan->meta.fp = tmpfile();
#endif

    if (!scan->meta.fp) {
      scan->error_func(scan, _("Could not open temp file."));
    }
  }

  while (1) {
    result = fc_readsocket(scan->sock, buf, sizeof(buf));

    if (result < 0) {
      if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
	/* Keep waiting. */
	return;
      }
      scan->error_func(scan, fc_strerror(fc_get_errno()));
      return;
    } else if (result == 0) {
      fz_FILE *f;
      char str[4096];

      /* We're done! */
      rewind(scan->meta.fp);

      f = fz_from_stream(scan->meta.fp);
      assert(f != NULL);

      /* skip HTTP headers */
      /* XXX: TODO check for magic Content-Type: text/x-ini -vasc */
      while (fz_fgets(str, sizeof(str), f) && strcmp(str, "\r\n") != 0) {
	/* nothing */
      }

      /* XXX: TODO check for magic Content-Type: text/x-ini -vasc */

      /* parse HTTP message body */
      scan->servers = parse_metaserver_data(f);
      scan->meta.state = META_DONE;

      /* 'f' (hence 'meta.fp') was closed in parse_metaserver_data(). */
      scan->meta.fp = NULL;

      return;
    } else {
      if (fwrite(buf, 1, result, scan->meta.fp) != result) {
	scan->error_func(scan, fc_strerror(fc_get_errno()));
      }
    }
  }
}

/****************************************************************************
  Begin a metaserver scan for servers.  This just initiates the connection
  to the metaserver; later get_meta_server_list should be called whenever
  the socket has data pending to read and parse it.

  Returns FALSE on error (in which case errbuf will contain an error
  message).
****************************************************************************/
static bool begin_metaserver_scan(struct server_scan *scan)
{
  union fc_sockaddr addr;
  int s;

  scan->meta.urlpath = fc_lookup_httpd(scan->meta.name, &scan->meta.port,
				       metaserver);
  if (!scan->meta.urlpath) {
    scan->error_func(scan,
                     _("Invalid $http_proxy or metaserver value, must "
                       "start with 'http://'"));
    return FALSE;
  }

  if (!net_lookup_service(scan->meta.name, scan->meta.port, &addr, FALSE)) {
    scan->error_func(scan, _("Failed looking up metaserver's host"));
    return FALSE;
  }
  
  if ((s = socket(addr.saddr.sa_family, SOCK_STREAM, 0)) == -1) {
    scan->error_func(scan, fc_strerror(fc_get_errno()));
    return FALSE;
  }

  fc_nonblock(s);
  
  if (fc_connect(s, &addr.saddr, sockaddr_size(&addr)) == -1) {
    if (errno == EINPROGRESS) {
      /* With non-blocking sockets this is the expected result. */
      scan->meta.state = META_CONNECTING;
      scan->sock = s;
    } else {
      fc_closesocket(s);
      scan->error_func(scan, fc_strerror(fc_get_errno()));
      return FALSE;
    }
  } else {
    /* Instant connection?  Whoa. */
    scan->sock = s;
    scan->meta.state = META_CONNECTING;
    meta_send_request(scan);
  }

  return TRUE;
}

/**************************************************************************
  Check for data received from the metaserver.
**************************************************************************/
static enum server_scan_status
get_metaserver_list(struct server_scan *scan)
{
  struct timeval tv;
  fd_set sockset;

  if (!scan || scan->sock < 0) {
    return SCAN_STATUS_ERROR;
  }

  tv.tv_sec = 0;
  tv.tv_usec = 0;
  FD_ZERO(&sockset);
  FD_SET(scan->sock, &sockset);

  switch (scan->meta.state) {
  case META_CONNECTING:
    if (fc_select(scan->sock + 1, NULL, &sockset, NULL, &tv) < 0) {
      scan->error_func(scan, fc_strerror(fc_get_errno()));
    } else if (FD_ISSET(scan->sock, &sockset)) {
      meta_send_request(scan);
    }
    /* Keep waiting. */
    return SCAN_STATUS_WAITING;
    break;
  case META_WAITING:
    if (fc_select(scan->sock + 1, &sockset, NULL, NULL, &tv) < 0) {
      scan->error_func(scan, fc_strerror(fc_get_errno()));
    } else if (FD_ISSET(scan->sock, &sockset)) {
      meta_read_response(scan);
      return SCAN_STATUS_PARTIAL;
    }
    /* Keep waiting. */
    return SCAN_STATUS_WAITING;
    break;
  case META_DONE:
    return SCAN_STATUS_DONE;
    break;
  default:
    break;
  }

  assert(0);
  return SCAN_STATUS_ERROR;
}

/**************************************************************************
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

  server_list_free(server_list);
}

/**************************************************************************
  Broadcast an UDP package to all servers on LAN, requesting information 
  about the server. The packet is send to all Freeciv servers in the same
  multicast group as the client.
**************************************************************************/
static bool begin_lanserver_scan(struct server_scan *scan)
{
  union fc_sockaddr addr;
  struct data_out dout;
  int sock, opt = 1;
#ifndef HAVE_WINSOCK
  unsigned char buffer[MAX_LEN_PACKET];
#else  /* HAVE_WINSOCK */
  char buffer[MAX_LEN_PACKET];
#endif /* HAVE_WINSOCK */
  struct ip_mreq mreq4;
  const char *group;
  size_t size;
  int family;

#ifdef IPV6_SUPPORT
  struct ipv6_mreq mreq6;
#endif

#ifndef HAVE_WINSOCK
  unsigned char ttl;
#endif

  if (announce == ANNOUNCE_NONE) {
    /* Succeeded in doing nothing */
    return TRUE;
  }

#ifdef IPV6_SUPPORT
  if (announce == ANNOUNCE_IPV6) {
    family = AF_INET6;
  } else
#endif
  {
    family = AF_INET;
  }

  /* Create a socket for broadcasting to servers. */
  if ((sock = socket(family, SOCK_DGRAM, 0)) < 0) {
    freelog(LOG_ERROR, "socket failed: %s", fc_strerror(fc_get_errno()));
    return FALSE;
  }

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                 (char *)&opt, sizeof(opt)) == -1) {
    freelog(LOG_ERROR, "SO_REUSEADDR failed: %s", fc_strerror(fc_get_errno()));
  }

  /* Set the UDP Multicast group IP address. */
  group = get_multicast_group(announce == ANNOUNCE_IPV6);
  memset(&addr, 0, sizeof(addr));

#ifndef IPV6_SUPPORT
  {
#ifdef HAVE_INET_ATON
    inet_aton(group, &addr.saddr_in4.sin_addr);
#else  /* HAVE_INET_ATON */
    addr.saddr_in4.sin_addr.s_addr = inet_addr(group);
#endif /* HAVE_INET_ATON */
#else  /* IPv6 support */
  if (family == AF_INET6) {
    addr.saddr.sa_family = AF_INET6;
    inet_pton(AF_INET6, group, &addr.saddr_in6.sin6_addr);
    addr.saddr_in6.sin6_port = htons(SERVER_LAN_PORT);
  } else {
    inet_pton(AF_INET, group, &addr.saddr_in4.sin_addr);
#endif /* IPv6 support */
    addr.saddr.sa_family = AF_INET;
    addr.saddr_in4.sin_port = htons(SERVER_LAN_PORT);
  }

/* this setsockopt call fails on Windows 98, so we stick with the default
 * value of 1 on Windows, which should be fine in most cases */
#ifndef HAVE_WINSOCK
  /* Set the Time-to-Live field for the packet  */
  ttl = SERVER_LAN_TTL;
  if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char*)&ttl, 
                 sizeof(ttl))) {
    freelog(LOG_ERROR, "setsockopt failed: %s", fc_strerror(fc_get_errno()));
    return FALSE;
  }
#endif /* HAVE_WINSOCK */

  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, 
                 sizeof(opt))) {
    freelog(LOG_ERROR, "setsockopt failed: %s", fc_strerror(fc_get_errno()));
    return FALSE;
  }

  dio_output_init(&dout, buffer, sizeof(buffer));
  dio_put_uint8(&dout, SERVER_LAN_VERSION);
  size = dio_output_used(&dout);
 

  if (sendto(sock, buffer, size, 0, &addr.saddr,
             sockaddr_size(&addr)) < 0) {
    /* This can happen when there's no network connection - it should
     * give an in-game message. */
    freelog(LOG_ERROR, "lanserver scan sendto failed: %s",
	    fc_strerror(fc_get_errno()));
    return FALSE;
  } else {
    freelog(LOG_DEBUG, ("Sending request for server announcement on LAN."));
  }

  fc_closesocket(sock);

  /* Create a socket for listening for server packets. */
  if ((scan->sock = socket(family, SOCK_DGRAM, 0)) < 0) {
    scan->error_func(scan, fc_strerror(fc_get_errno()));
    return FALSE;
  }

  fc_nonblock(scan->sock);

  if (setsockopt(scan->sock, SOL_SOCKET, SO_REUSEADDR,
                 (char *)&opt, sizeof(opt)) == -1) {
    freelog(LOG_ERROR, "SO_REUSEADDR failed: %s", fc_strerror(fc_get_errno()));
  }
                                                                               
  memset(&addr, 0, sizeof(addr));

#ifdef IPV6_SUPPORT
  if (family == AF_INET6) {
    addr.saddr.sa_family = AF_INET6;
    addr.saddr_in6.sin6_port = htons(SERVER_LAN_PORT + 1);
    addr.saddr_in6.sin6_addr = in6addr_any;
  } else
#endif /* IPv6 support */
  {
    addr.saddr.sa_family = AF_INET;
    addr.saddr_in4.sin_port = htons(SERVER_LAN_PORT + 1);
    addr.saddr_in4.sin_addr.s_addr = htonl(INADDR_ANY);
  }

  if (bind(scan->sock, &addr.saddr, sockaddr_size(&addr)) < 0) {
    scan->error_func(scan, fc_strerror(fc_get_errno()));
    return FALSE;
  }

#ifndef IPV6_SUPPORT
  {
#ifdef HAVE_INET_ATON
    inet_aton(group, &mreq4.imr_multiaddr);
#else  /* HAVE_INET_ATON */
    mreq4.imr_multiaddr.s_addr = inet_addr(group);
#endif /* HAVE_INET_ATON */
#else  /* IPv6 support */
  if (family == AF_INET6) {
    inet_pton(AF_INET6, group, &mreq6.ipv6mr_multiaddr.s6_addr);
    mreq6.ipv6mr_interface = 0; /* TODO: Interface selection */

    if (setsockopt(scan->sock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
                   (const char*)&mreq6, sizeof(mreq6)) < 0) {
      scan->error_func(scan, fc_strerror(fc_get_errno()));
    }
  } else {
    inet_pton(AF_INET, group, &mreq4.imr_multiaddr.s_addr);
#endif /* IPv6 support */
    mreq4.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(scan->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   (const char*)&mreq4, sizeof(mreq4)) < 0) {
      scan->error_func(scan, fc_strerror(fc_get_errno()));
      return FALSE;
    }
  }

  scan->servers = server_list_new();

  return TRUE;
}

/**************************************************************************
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
  char message[1024];
  bool found_new = FALSE;

  while (1) {
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

    dio_get_uint8(&din, &type);
    if (type != SERVER_LAN_VERSION) {
      continue;
    }
    dio_get_string(&din, servername, sizeof(servername));
    dio_get_string(&din, portstr, sizeof(portstr));
    port = atoi(portstr);
    dio_get_string(&din, version, sizeof(version));
    dio_get_string(&din, status, sizeof(status));
    dio_get_string(&din, players, sizeof(players));
    dio_get_string(&din, message, sizeof(message));

    if (!mystrcasecmp("none", servername)) {
      bool nameinfo = FALSE;
#ifdef IPV6_SUPPORT
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
        } else {
          inet_ntop(AF_INET, &fromend.saddr_in4.sin_addr, dst, sizeof(dst));;
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
    server_list_iterate(scan->servers, aserver) {
      if (0 == mystrcasecmp(aserver->host, servername)
          && aserver->port == port) {
	duplicate = TRUE;
	break;
      } 
    } server_list_iterate_end;
    if (duplicate) {
      continue;
    }

    freelog(LOG_DEBUG,
            ("Received a valid announcement from a server on the LAN."));
    
    pserver = fc_malloc(sizeof(*pserver));
    pserver->host = mystrdup(servername);
    pserver->port = port;
    pserver->version = mystrdup(version);
    pserver->state = mystrdup(status);
    pserver->nplayers = atoi(players);
    pserver->message = mystrdup(message);
    pserver->players = NULL;
    found_new = TRUE;

    server_list_prepend(scan->servers, pserver);
  }

  if (found_new) {
    return SCAN_STATUS_PARTIAL;
  }
  return SCAN_STATUS_WAITING;
}

/****************************************************************************
  Creates a new server scan and returns it, or NULL if impossible.

  Depending on 'type' the scan will look for either local or internet
  games.

  error_func provides a callback to be used in case of error; this
  callback probably should call server_scan_finish.

  NB: You must call server_scan_finish() when you are done with the
  scan to free the memory and resources allocated by it.
****************************************************************************/
struct server_scan *server_scan_begin(enum server_scan_type type,
				      ServerScanErrorFunc error_func)
{
  struct server_scan *scan;
  bool ok = FALSE;

  scan = fc_calloc(1, sizeof(*scan));
  scan->type = type;
  scan->error_func = error_func;
  scan->sock = -1;

  switch (type) {
  case SERVER_SCAN_GLOBAL:
    ok = begin_metaserver_scan(scan);
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

/****************************************************************************
  A simple query function to determine the type of a server scan (previously
  allocated in server_scan_begin).
****************************************************************************/
enum server_scan_type server_scan_get_type(const struct server_scan *scan)
{
  if (!scan) {
    return SERVER_SCAN_LAST;
  }
  return scan->type;
}

/****************************************************************************
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
****************************************************************************/
enum server_scan_status server_scan_poll(struct server_scan *scan)
{
  if (!scan) {
    return SCAN_STATUS_ERROR;
  }

  switch (scan->type) {
  case SERVER_SCAN_GLOBAL:
    return get_metaserver_list(scan);
    break;
  case SERVER_SCAN_LOCAL:
    return get_lan_server_list(scan);
    break;
  default:
    break;
  }

  return SCAN_STATUS_ERROR;
}

/**************************************************************************
  Returns the server_list currently held by the scan (may be NULL).
**************************************************************************/
const struct server_list *
server_scan_get_list(const struct server_scan *scan)
{
  if (!scan) {
    return NULL;
  }
  return scan->servers;
}

/**************************************************************************
  Closes the socket listening on the scan, frees the list of servers, and
  frees the memory allocated for 'scan' by server_scan_begin().
**************************************************************************/
void server_scan_finish(struct server_scan *scan)
{
  if (!scan) {
    return;
  }

  if (scan->sock >= 0) {
    fc_closesocket(scan->sock);
    scan->sock = -1;
  }

  if (scan->servers) {
    delete_server_list(scan->servers);
    scan->servers = NULL;
  }

  if (scan->meta.fp) {
    fclose(scan->meta.fp);
    scan->meta.fp = NULL;
  }

  free(scan);
}
