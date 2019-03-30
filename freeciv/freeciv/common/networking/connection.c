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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include "fc_prehdrs.h"

#include <errno.h>
#include <string.h>
#include <time.h>

#ifdef FREECIV_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SELECT_H
/* For some platforms this must be below sys/types.h. */
#include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* utility */
#include "fcintl.h"
#include "genhash.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "support.h"            /* fc_str(n)casecmp */

/* common */
#include "game.h"               /* game.all_connections */
#include "packets.h"

#include "connection.h"


static void default_conn_close_callback(struct connection *pconn);

/* String used for connection.addr and related cases to indicate
 * blank/unknown/not-applicable address:
 */
const char blank_addr_str[] = "---.---.---.---";

/****************************************************************************
  This callback is used when an error occurs trying to write to the
  connection. The effect of the callback should be to close the connection.
  This is here so that the server and client can take appropriate
  (different) actions: server lost a client, client lost connection to
  server. Never attempt to call this function directly, call
  connection_close() instead.
****************************************************************************/
static conn_close_fn_t conn_close_callback = default_conn_close_callback;

/************************************************************************//**
  Default 'conn_close_fn_t' to close a connection.
****************************************************************************/
static void default_conn_close_callback(struct connection *pconn)
{
  fc_assert_msg(conn_close_callback != default_conn_close_callback,
                "Closing a socket (%s) before calling "
                "close_socket_set_callback().", conn_description(pconn));
}

/**********************************************************************//**
  Register the close_callback.
**************************************************************************/
void connections_set_close_callback(conn_close_fn_t func)
{
  conn_close_callback = func;
}

/**********************************************************************//**
  Call the conn_close_callback.
**************************************************************************/
void connection_close(struct connection *pconn, const char *reason)
{
  fc_assert_ret(NULL != pconn);

  if (NULL != reason && NULL == pconn->closing_reason) {
    /* NB: we don't overwrite the original reason. */
    pconn->closing_reason = fc_strdup(reason);
  }

  (*conn_close_callback) (pconn);
}

/**********************************************************************//**
  Make sure that there is at least extra_space bytes free space in buffer,
  allocating more memory if needed.
**************************************************************************/
static bool buffer_ensure_free_extra_space(struct socket_packet_buffer *buf,
					   int extra_space)
{
  /* room for more? */
  if (buf->nsize - buf->ndata < extra_space) {
    buf->nsize = buf->ndata + extra_space;

    /* added this check so we don't gobble up too much mem */
    if (buf->nsize > MAX_LEN_BUFFER) {
      return FALSE;
    }
    buf->data = (unsigned char *) fc_realloc(buf->data, buf->nsize);
  }
  return TRUE;
}

/**********************************************************************//**
  Read data from socket, and check if a packet is ready.
  Returns:
    -1  :  an error occurred - you should close the socket
    -2  :  the connection was closed
    >0  :  number of bytes read
    =0  :  non-blocking sockets only; no data read, would block
**************************************************************************/
int read_socket_data(int sock, struct socket_packet_buffer *buffer)
{
  int didget;

  if (!buffer_ensure_free_extra_space(buffer, MAX_LEN_PACKET)) {
    log_error("can't grow buffer");
    return -1;
  }

  log_debug("try reading %d bytes", buffer->nsize - buffer->ndata);
  didget = fc_readsocket(sock, (char *) (buffer->data + buffer->ndata),
			 buffer->nsize - buffer->ndata);

  if (didget > 0) {
    buffer->ndata+=didget;
    log_debug("didget:%d", didget);
    return didget;
  }
  else if (didget == 0) {
    log_debug("EOF on socket read");
    return -2;
  }
#ifdef NONBLOCKING_SOCKETS
  else if (errno == EWOULDBLOCK || errno == EAGAIN) {
    log_debug("EGAIN on socket read");
    return 0;
  }
#endif
  return -1;
}

/**********************************************************************//**
  write wrapper function -vasc
**************************************************************************/
static int write_socket_data(struct connection *pc,
			     struct socket_packet_buffer *buf, int limit)
{
  int start, nput, nblock;

  if (is_server() && pc->server.is_closing) {
    return 0;
  }

  for (start=0; buf->ndata-start>limit;) {
    fd_set writefs, exceptfs;
    fc_timeval tv;

    FC_FD_ZERO(&writefs);
    FC_FD_ZERO(&exceptfs);
    FD_SET(pc->sock, &writefs);
    FD_SET(pc->sock, &exceptfs);

    tv.tv_sec = 0; tv.tv_usec = 0;

    if (fc_select(pc->sock+1, NULL, &writefs, &exceptfs, &tv) <= 0) {
      if (errno != EINTR) {
	break;
      } else {
	/* EINTR can happen sometimes, especially when compiling with -pg.
	 * Generally we just want to run select again. */
	continue;
      }
    }

    if (FD_ISSET(pc->sock, &exceptfs)) {
      connection_close(pc, _("network exception"));
      return -1;
    }

    if (FD_ISSET(pc->sock, &writefs)) {
      nblock=MIN(buf->ndata-start, MAX_LEN_PACKET);
      log_debug("trying to write %d limit=%d", nblock, limit);
      if ((nput = fc_writesocket(pc->sock, 
                                 (const char *)buf->data+start, nblock)) == -1) {
#ifdef NONBLOCKING_SOCKETS
	if (errno == EWOULDBLOCK || errno == EAGAIN) {
	  break;
	}
#endif
        connection_close(pc, _("lagging connection"));
        return -1;
      }
      start += nput;
    }
  }

  if (start > 0) {
    buf->ndata -= start;
    memmove(buf->data, buf->data+start, buf->ndata);
    pc->last_write = timer_renew(pc->last_write, TIMER_USER, TIMER_ACTIVE);
    timer_start(pc->last_write);
  }
  return 0;
}


/**********************************************************************//**
  flush'em
**************************************************************************/
void flush_connection_send_buffer_all(struct connection *pc)
{
  if (pc && pc->used && pc->send_buffer->ndata > 0) {
    write_socket_data(pc, pc->send_buffer, 0);
    if (pc->notify_of_writable_data) {
      pc->notify_of_writable_data(pc, pc->send_buffer
                                  && pc->send_buffer->ndata > 0);
    }
  }
}

/**********************************************************************//**
  flush'em
**************************************************************************/
#ifndef FREECIV_JSON_CONNECTION
static void flush_connection_send_buffer_packets(struct connection *pc)
{
  if (pc && pc->used && pc->send_buffer->ndata >= MAX_LEN_PACKET) {
    write_socket_data(pc, pc->send_buffer, MAX_LEN_PACKET-1);
    if (pc->notify_of_writable_data) {
      pc->notify_of_writable_data(pc, pc->send_buffer
                                  && pc->send_buffer->ndata > 0);
    }
  }
}
#endif /* FREECIV_JSON_CONNECTION */

/**********************************************************************//**
  Add data to send to the connection.
**************************************************************************/
static bool add_connection_data(struct connection *pconn,
                                const unsigned char *data, int len)
{
  struct socket_packet_buffer *buf;

  if (NULL == pconn
      || !pconn->used
      || (is_server() && pconn->server.is_closing)) {
    return TRUE;
  }

  buf = pconn->send_buffer;
  log_debug("add %d bytes to %d (space =%d)", len, buf->ndata, buf->nsize);
  if (!buffer_ensure_free_extra_space(buf, len)) {
    connection_close(pconn, _("buffer overflow"));
    return FALSE;
  }

  memcpy(buf->data + buf->ndata, data, len);
  buf->ndata += len;
  return TRUE;
}

/**********************************************************************//**
  Write data to socket. Return TRUE on success.
**************************************************************************/
bool connection_send_data(struct connection *pconn,
                          const unsigned char *data, int len)
{
  if (NULL == pconn
      || !pconn->used
      || (is_server() && pconn->server.is_closing)) {
    return TRUE;
  }

  pconn->statistics.bytes_send += len;

#ifndef FREECIV_JSON_CONNECTION
  if (0 < pconn->send_buffer->do_buffer_sends) {
    flush_connection_send_buffer_packets(pconn);
    if (!add_connection_data(pconn, data, len)) {
      log_verbose("cut connection %s due to huge send buffer (1)",
                  conn_description(pconn));
      return FALSE;
    }
    flush_connection_send_buffer_packets(pconn);
  } else
#endif /* FREECIV_JSON_CONNECTION */
  {
    flush_connection_send_buffer_all(pconn);
    if (!add_connection_data(pconn, data, len)) {
      log_verbose("cut connection %s due to huge send buffer (2)",
                  conn_description(pconn));
      return FALSE;
    }
    flush_connection_send_buffer_all(pconn);
  }
  return TRUE;
}

/**********************************************************************//**
  Turn on buffering, using a counter so that calls may be nested.
**************************************************************************/
void connection_do_buffer(struct connection *pc)
{
  if (pc && pc->used) {
    pc->send_buffer->do_buffer_sends++;
  }
}

/**********************************************************************//**
  Turn off buffering if internal counter of number of times buffering
  was turned on falls to zero, to handle nested buffer/unbuffer pairs.
  When counter is zero, flush any pending data.
**************************************************************************/
void connection_do_unbuffer(struct connection *pc)
{
  if (NULL == pc || !pc->used || (is_server() && pc->server.is_closing)) {
    return;
  }

  pc->send_buffer->do_buffer_sends--;
  if (0 > pc->send_buffer->do_buffer_sends) {
    log_error("Too many calls to unbuffer %s!", pc->username);
    pc->send_buffer->do_buffer_sends = 0;
  }

  if (0 == pc->send_buffer->do_buffer_sends) {
    flush_connection_send_buffer_all(pc);
  }
}

/**********************************************************************//**
  Convenience functions to buffer/unbuffer a list of connections:
**************************************************************************/
void conn_list_do_buffer(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn)
    connection_do_buffer(pconn);
  conn_list_iterate_end;
}
void conn_list_do_unbuffer(struct conn_list *dest)
{
  conn_list_iterate(dest, pconn)
    connection_do_unbuffer(pconn);
  conn_list_iterate_end;
}

/**********************************************************************//**
  Find connection by exact user name, from game.all_connections,
  case-insensitve.  Returns NULL if not found.
**************************************************************************/
struct connection *conn_by_user(const char *user_name)
{
  conn_list_iterate(game.all_connections, pconn) {
    if (fc_strcasecmp(user_name, pconn->username) == 0) {
      return pconn;
    }
  } conn_list_iterate_end;

  return NULL;
}

/**********************************************************************//**
  Like conn_by_username(), but allow unambigous prefix (i.e. abbreviation).
  Returns NULL if could not match, or if ambiguous or other problem, and
  fills *result with characterisation of match/non-match (see
  "utility/shared.[ch]").
**************************************************************************/
static const char *connection_accessor(int i) {
  return conn_list_get(game.all_connections, i)->username;
}

struct connection *conn_by_user_prefix(const char *user_name,
                                       enum m_pre_result *result)
{
  int ind;

  *result = match_prefix(connection_accessor,
                         conn_list_size(game.all_connections),
                         MAX_LEN_NAME-1, fc_strncasequotecmp,
                         effectivestrlenquote, user_name, &ind);
  
  if (*result < M_PRE_AMBIGUOUS) {
    return conn_list_get(game.all_connections, ind);
  } else {
    return NULL;
  }
}

/**********************************************************************//**
  Find connection by id, from game.all_connections.
  Returns NULL if not found.
  Number of connections will always be relatively small given
  current implementation, so linear search should be fine.
**************************************************************************/
struct connection *conn_by_number(int id)
{
  conn_list_iterate(game.all_connections, pconn) {
    fc_assert_msg(pconn != NULL,
                  "Trying to look at the id of a non existing connection");

    if (pconn->id == id) {
      return pconn;
    }
  }
  conn_list_iterate_end;
  return NULL;
}

/**********************************************************************//**
  Return malloced struct, appropriately initialized.
**************************************************************************/
struct socket_packet_buffer *new_socket_packet_buffer(void)
{
  struct socket_packet_buffer *buf;

  buf = fc_malloc(sizeof(*buf));
  buf->ndata = 0;
  buf->do_buffer_sends = 0;
  buf->nsize = 10*MAX_LEN_PACKET;
  buf->data = (unsigned char *)fc_malloc(buf->nsize);
  return buf;
}

/**********************************************************************//**
  Free malloced struct
**************************************************************************/
static void free_socket_packet_buffer(struct socket_packet_buffer *buf)
{
  if (buf) {
    if (buf->data) {
      free(buf->data);
    }
    free(buf);
  }
}

/**********************************************************************//**
  Return pointer to static string containing a description for this
  connection, based on pconn->name, pconn->addr, and (if applicable)
  pconn->playing->name.  (Also pconn->established and pconn->observer.)

  Note that when pconn is client.conn (connection to server),
  pconn->name and pconn->addr contain empty string, and pconn->playing
  is NULL: in this case return string "server".
**************************************************************************/
const char *conn_description(const struct connection *pconn)
{
  static char buffer[MAX_LEN_NAME*2 + MAX_LEN_ADDR + 128];

  buffer[0] = '\0';

  if (*pconn->username != '\0') {
    fc_snprintf(buffer, sizeof(buffer), _("%s from %s"),
                pconn->username, pconn->addr); 
  } else {
    sz_strlcpy(buffer, "server");
  }
  if (NULL != pconn->closing_reason) {
    /* TRANS: Appending the reason why a connection has closed. */
    cat_snprintf(buffer, sizeof(buffer), _(" (%s)"), pconn->closing_reason);
  } else if (!pconn->established) {
    sz_strlcat(buffer, _(" (connection incomplete)"));
    return buffer;
  }
  if (NULL != pconn->playing) {
    cat_snprintf(buffer, sizeof(buffer), _(" (player %s)"),
		 player_name(pconn->playing));
  }
  if (pconn->observer) {
    sz_strlcat(buffer, _(" (observer)"));
  }
  return buffer;
}

/**********************************************************************//**
  Return TRUE iff the connection is currently allowed to edit.
**************************************************************************/
bool can_conn_edit(const struct connection *pconn)
{
  return (can_conn_enable_editing(pconn)
          && game.info.is_edit_mode
          && (NULL != pconn->playing || pconn->observer));
}

/**********************************************************************//**
  Return TRUE iff the connection is allowed to start editing.
**************************************************************************/
bool can_conn_enable_editing(const struct connection *pconn)
{
  return pconn->access_level == ALLOW_HACK;
}

/**********************************************************************//**
  Get next request id. Takes wrapping of the 16 bit wide unsigned int
  into account.
**************************************************************************/
int get_next_request_id(int old_request_id)
{
  int result = old_request_id + 1;

  if ((result & 0xffff) == 0) {
    log_packet("INFORMATION: request_id has wrapped around; "
               "setting from %d to 2", result);
    result = 2;
  }
  fc_assert(0 != result);
  return result;
}

/**********************************************************************//**
  Free compression queue for given connection.
**************************************************************************/
void free_compression_queue(struct connection *pc)
{
#ifdef USE_COMPRESSION
  byte_vector_free(&pc->compression.queue);
#endif
}

/**********************************************************************//**
  Allocate and initialize packet hashs for given connection.
**************************************************************************/
static void init_packet_hashs(struct connection *pc)
{
  enum packet_type i;

  pc->phs.sent = fc_malloc(sizeof(*pc->phs.sent) * PACKET_LAST);
  pc->phs.received = fc_malloc(sizeof(*pc->phs.received) * PACKET_LAST);
  pc->phs.handlers = packet_handlers_initial();

  for (i = 0; i < PACKET_LAST; i++) {
    pc->phs.sent[i] = NULL;
    pc->phs.received[i] = NULL;
  }
}

/**********************************************************************//**
  Free packet hash resources from given connection.
**************************************************************************/
static void free_packet_hashes(struct connection *pc)
{
  int i;

  if (pc->phs.sent) {
    for (i = 0; i < PACKET_LAST; i++) {
      if (pc->phs.sent[i] != NULL) {
        genhash_destroy(pc->phs.sent[i]);
      }
    }
    free(pc->phs.sent);
    pc->phs.sent = NULL;
  }

  if (pc->phs.received) {
    for (i = 0; i < PACKET_LAST; i++) {
      if (pc->phs.received[i] != NULL) {
        genhash_destroy(pc->phs.received[i]);
      }
    }
    free(pc->phs.received);
    pc->phs.received = NULL;
  }
}

/**********************************************************************//**
  Initialize common part of connection structure. This is used by
  both server and client.
**************************************************************************/
void connection_common_init(struct connection *pconn)
{
  pconn->established = FALSE;
  pconn->used = TRUE;
  packet_header_init(&pconn->packet_header);
  pconn->closing_reason = NULL;
  pconn->last_write = NULL;
  pconn->buffer = new_socket_packet_buffer();
  pconn->send_buffer = new_socket_packet_buffer();
  pconn->statistics.bytes_send = 0;
#ifdef FREECIV_JSON_CONNECTION
  pconn->json_mode = TRUE;
#endif /* FREECIV_JSON_CONNECTION */

  init_packet_hashs(pconn);

#ifdef USE_COMPRESSION
  byte_vector_init(&pconn->compression.queue);
  pconn->compression.frozen_level = 0;
#endif
}

/**********************************************************************//**
   Connection closing part common to server and client.
**************************************************************************/
void connection_common_close(struct connection *pconn)
{
  if (!pconn->used) {
    log_error("WARNING: Trying to close already closed connection");
  } else {
    fc_closesocket(pconn->sock);
    pconn->used = FALSE;
    pconn->established = FALSE;
    if (NULL != pconn->closing_reason) {
      free(pconn->closing_reason);
    }

    free_socket_packet_buffer(pconn->buffer);
    pconn->buffer = NULL;

    free_socket_packet_buffer(pconn->send_buffer);
    pconn->send_buffer = NULL;

    if (pconn->last_write) {
      timer_destroy(pconn->last_write);
      pconn->last_write = NULL;
    }

    free_compression_queue(pconn);
    free_packet_hashes(pconn);
  }
}

/**********************************************************************//**
  Set the network capability string for 'pconn'.
**************************************************************************/
void conn_set_capability(struct connection *pconn, const char *capability)
{
  fc_assert(strlen(capability) < sizeof(pconn->capability));
  sz_strlcpy(pconn->capability, capability);
  pconn->phs.handlers = packet_handlers_get(capability);
}

/**********************************************************************//**
  Remove all is-game-info cached packets from the connection. This resets
  the delta-state partially.
**************************************************************************/
void conn_reset_delta_state(struct connection *pc)
{
  int i;

  for (i = 0; i < PACKET_LAST; i++) {
    if (packet_has_game_info_flag(i)) {
      if (NULL != pc->phs.sent && NULL != pc->phs.sent[i]) {
        genhash_clear(pc->phs.sent[i]);
      }
      if (NULL != pc->phs.received && NULL != pc->phs.received[i]) {
        genhash_clear(pc->phs.received[i]);
      }
    }
  }
}

/**********************************************************************//**
  Freeze the connection. Then the packets sent to it won't be sent
  immediatly, but later, using a compression method. See futher details in
  common/packets.[ch].
**************************************************************************/
void conn_compression_freeze(struct connection *pconn)
{
#ifdef USE_COMPRESSION
  if (0 == pconn->compression.frozen_level) {
    byte_vector_reserve(&pconn->compression.queue, 0);
  }
  pconn->compression.frozen_level++;
#endif /* USE_COMPRESSION */
}

/**********************************************************************//**
  Returns TRUE if the connection is frozen. See also
  conn_compression_freeze().
**************************************************************************/
bool conn_compression_frozen(const struct connection *pconn)
{
#ifdef USE_COMPRESSION
  return 0 < pconn->compression.frozen_level;
#else
  return FALSE;
#endif /* USE_COMPRESSION */
}

/**********************************************************************//**
  Freeze a connection list.
**************************************************************************/
void conn_list_compression_freeze(const struct conn_list *pconn_list)
{
#ifdef USE_COMPRESSION
  conn_list_iterate(pconn_list, pconn) {
    conn_compression_freeze(pconn);
  } conn_list_iterate_end;
#endif /* USE_COMPRESSION */
}

/**********************************************************************//**
  Thaw a connection list.
**************************************************************************/
void conn_list_compression_thaw(const struct conn_list *pconn_list)
{
#ifdef USE_COMPRESSION
  conn_list_iterate(pconn_list, pconn) {
    conn_compression_thaw(pconn);
  } conn_list_iterate_end;
#endif /* USE_COMPRESSION */
}

/**********************************************************************//**
  Returns TRUE if the given connection is attached to a player which it
  also controls (i.e. not a player observer).
**************************************************************************/
bool conn_controls_player(const struct connection *pconn)
{
  return pconn && pconn->playing && !pconn->observer;
}

/**********************************************************************//**
  Returns TRUE if the given connection is a global observer.
**************************************************************************/
bool conn_is_global_observer(const struct connection *pconn)
{
  return pconn && !pconn->playing && pconn->observer;
}

/**********************************************************************//**
  Returns the player that this connection is attached to, or NULL. Note
  that this will return the observed player for connections that are
  observing players.
**************************************************************************/
struct player *conn_get_player(const struct connection *pconn)
{
  if (!pconn) {
    return NULL;
  }
  return pconn->playing;
}

/**********************************************************************//**
  Returns the current access level of the given connection.
  NB: If 'pconn' is NULL, this function will return ALLOW_NONE.
**************************************************************************/
enum cmdlevel conn_get_access(const struct connection *pconn)
{
  if (!pconn) {
    return ALLOW_NONE; /* Would not want to give hack on error... */
  }
  return pconn->access_level;
}


/**************************************************************************
  Connection patterns.
**************************************************************************/
struct conn_pattern {
  enum conn_pattern_type type;
  char *wildcard;
};

/**********************************************************************//**
  Creates a new connection pattern.
**************************************************************************/
struct conn_pattern *conn_pattern_new(enum conn_pattern_type type,
                                      const char *wildcard)
{
  struct conn_pattern *ppattern = fc_malloc(sizeof(*ppattern));

  ppattern->type = type;
  ppattern->wildcard = fc_strdup(wildcard);

  return ppattern;
}

/**********************************************************************//**
  Free a connection pattern.
**************************************************************************/
void conn_pattern_destroy(struct conn_pattern *ppattern)
{
  fc_assert_ret(NULL != ppattern);
  free(ppattern->wildcard);
  free(ppattern);
}

/**********************************************************************//**
  Returns TRUE whether the connection fits the connection pattern.
**************************************************************************/
bool conn_pattern_match(const struct conn_pattern *ppattern,
                        const struct connection *pconn)
{
  const char *test = NULL;

  switch (ppattern->type) {
  case CPT_USER:
    test = pconn->username;
    break;
  case CPT_HOST:
    test =  pconn->addr;
    break;
  case CPT_IP:
    if (is_server()) {
      test =  pconn->server.ipaddr;
    }
    break;
  }

  if (NULL != test) {
    return wildcard_fit_string(ppattern->wildcard, test);
  } else {
    log_error("%s(): Invalid pattern type (%d)",
              __FUNCTION__, ppattern->type);
    return FALSE;
  }
}

/**********************************************************************//**
  Returns TRUE whether the connection fits one of the connection patterns.
**************************************************************************/
bool conn_pattern_list_match(const struct conn_pattern_list *plist,
                             const struct connection *pconn)
{
  conn_pattern_list_iterate(plist, ppattern) {
    if (conn_pattern_match(ppattern, pconn)) {
      return TRUE;
    }
  } conn_pattern_list_iterate_end;
  return FALSE;
}

/**********************************************************************//**
  Put a string reprentation of the pattern in 'buf'.
**************************************************************************/
size_t conn_pattern_to_string(const struct conn_pattern *ppattern,
                              char *buf, size_t buf_len)
{
  return fc_snprintf(buf, buf_len, "<%s=%s>",
                     conn_pattern_type_name(ppattern->type),
                     ppattern->wildcard);
}

/**********************************************************************//**
  Creates a new connection pattern from the string. If the type is not
  specified in 'pattern', then 'prefer' type will be used. If the type
  is needed, then pass conn_pattern_type_invalid() for 'prefer'.
**************************************************************************/
struct conn_pattern *conn_pattern_from_string(const char *pattern,
                                              enum conn_pattern_type prefer,
                                              char *error_buf,
                                              size_t error_buf_len)
{
  enum conn_pattern_type type = conn_pattern_type_invalid();
  const char *p;

  /* Determine pattern type. */
  if ((p = strchr(pattern, '='))) {
    /* Special character to separate the type of the pattern. */
    const size_t pattern_type_len = ++p - pattern;
    char pattern_type[pattern_type_len];

    fc_strlcpy(pattern_type, pattern, pattern_type_len);
    remove_leading_trailing_spaces(pattern_type);
    type = conn_pattern_type_by_name(pattern_type, fc_strcasecmp);
    if (!conn_pattern_type_is_valid(type)) {
      if (NULL != error_buf) {
        fc_snprintf(error_buf, error_buf_len,
                    _("\"%s\" is not a valid pattern type"),
                    pattern_type);
      }
      return NULL;
    }
  } else {
    /* Use 'prefer' type. */
    p = pattern;
    type = prefer;
    if (!conn_pattern_type_is_valid(type)) {
      if (NULL != error_buf) {
        fc_strlcpy(error_buf, _("Missing pattern type"), error_buf_len);
      }
      return NULL;
    }
  }

  /* Remove leading spaces. */
  while (fc_isspace(*p)) {
    p++;
  }

  if ('\0' == *p) {
    if (NULL != error_buf) {
      fc_strlcpy(error_buf, _("Missing pattern"), error_buf_len);
    }
    return NULL;
  }

  return  conn_pattern_new(type, p);
}

/**********************************************************************//**
  Returns TRUE if the connection is valid, i.e. not NULL, not closed, not
  closing, etc.
**************************************************************************/
bool conn_is_valid(const struct connection *pconn)
{
  return (pconn && pconn->used && !pconn->server.is_closing);
}
