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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SELECT_H
/* For some platforms this must be below sys/types.h. */
#include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_WINSOCK
#include <winsock.h>
#endif

#include "fcintl.h"
#include "game.h"		/* game.all_connections */
#include "hash.h"
#include "log.h"
#include "mem.h"
#include "netintf.h"
#include "packets.h"
#include "support.h"		/* mystr(n)casecmp */

#include "connection.h"

/* String used for connection.addr and related cases to indicate
 * blank/unknown/not-applicable address:
 */
const char blank_addr_str[] = "---.---.---.---";

/* This is only used by the server.
   If it is set the disconnection of conns is posponed. This is sometimes
   neccesary as removing a random connection while we are iterating through
   a connection list might corrupt the list. */
int delayed_disconnect = 0;


/**************************************************************************
  Command access levels for client-side use; at present, they are only
  used to control access to server commands typed at the client chatline.
  NB: These must match enum cmdlevel_id in common/connection.h.
**************************************************************************/
static const char *levelnames[] = {
  "none",
  "info",
  "basic",
  "ctrl",
  "admin",
  "hack"
};

/**************************************************************************
  Get name associated with given level.  These names are used verbatim in
  commands, so should not be translated here.
**************************************************************************/
const char *cmdlevel_name(enum cmdlevel_id lvl)
{
  assert (lvl >= 0 && lvl < ALLOW_NUM);
  return levelnames[lvl];
}

/**************************************************************************
  Lookup level assocated with token, or ALLOW_UNRECOGNISED if no match.
  Only as many characters as in token need match, so token may be
  abbreviated -- current level names are unique at first character.
  Empty token will match first, i.e. level 'none'.
**************************************************************************/
enum cmdlevel_id cmdlevel_named(const char *token)
{
  enum cmdlevel_id i;
  size_t len = strlen(token);

  for (i = 0; i < ALLOW_NUM; i++) {
    if (strncmp(levelnames[i], token, len) == 0) {
      return i;
    }
  }

  return ALLOW_UNRECOGNIZED;
}


/**************************************************************************
  This callback is used when an error occurs trying to write to the
  connection.  The effect of the callback should be to close the
  connection.  This is here so that the server and client can take
  appropriate (different) actions: server lost a client, client lost
  connection to server.
**************************************************************************/
static CLOSE_FUN close_callback;

/**************************************************************************
  Register the close_callback:
**************************************************************************/
void close_socket_set_callback(CLOSE_FUN fun)
{
  close_callback = fun;
}

/**************************************************************************
  Return the the close_callback.
**************************************************************************/
CLOSE_FUN close_socket_get_callback(void)
{
  return close_callback;
}

/**************************************************************************
...
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

/**************************************************************************
  Read data from socket, and check if a packet is ready.
  Returns:
    -1  :  an error occurred - you should close the socket
    >0  :  number of bytes read
    =0  :  non-blocking sockets only; no data read, would block
**************************************************************************/
int read_socket_data(int sock, struct socket_packet_buffer *buffer)
{
  int didget;

  if (!buffer_ensure_free_extra_space(buffer, MAX_LEN_PACKET)) {
    freelog(LOG_ERROR, "can't grow buffer");
    return -1;
  }

  freelog(LOG_DEBUG, "try reading %d bytes", buffer->nsize - buffer->ndata);
  didget = fc_readsocket(sock, (char *) (buffer->data + buffer->ndata),
			 buffer->nsize - buffer->ndata);

  if (didget > 0) {
    buffer->ndata+=didget;
    freelog(LOG_DEBUG, "didget:%d", didget);
    return didget;
  }
  else if (didget == 0) {
    freelog(LOG_DEBUG, "EOF on socket read");
    return -1;
  }
#ifdef NONBLOCKING_SOCKETS
  else if (errno == EWOULDBLOCK || errno == EAGAIN) {
    freelog(LOG_DEBUG, "EGAIN on socket read");
    return 0;
  }
#endif
  return -1;
}

/**************************************************************************
  write wrapper function -vasc
**************************************************************************/
static int write_socket_data(struct connection *pc,
			     struct socket_packet_buffer *buf, int limit)
{
  int start, nput, nblock;

  if (pc->delayed_disconnect) {
    if (delayed_disconnect > 0) {
      return 0;
    } else {
      if (close_callback) {
	(*close_callback)(pc);
      }
      return -1;
    }
  }

  for (start=0; buf->ndata-start>limit;) {
    fd_set writefs, exceptfs;
    struct timeval tv;

    MY_FD_ZERO(&writefs);
    MY_FD_ZERO(&exceptfs);
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
      if (delayed_disconnect > 0) {
	pc->delayed_disconnect = TRUE;
	return 0;
      } else {
	if (close_callback) {
	  (*close_callback)(pc);
	}
	return -1;
      }
    }

    if (FD_ISSET(pc->sock, &writefs)) {
      nblock=MIN(buf->ndata-start, MAX_LEN_PACKET);
      freelog(LOG_DEBUG,"trying to write %d limit=%d",nblock,limit);
      if((nput=fc_writesocket(pc->sock, 
			      (const char *)buf->data+start, nblock)) == -1) {
#ifdef NONBLOCKING_SOCKETS
	if (errno == EWOULDBLOCK || errno == EAGAIN) {
	  break;
	}
#endif
	if (delayed_disconnect > 0) {
	  pc->delayed_disconnect = TRUE;
	  return 0;
	} else {
	  if (close_callback) {
	    (*close_callback)(pc);
	  }
	  return -1;
	}
      }
      start += nput;
    }
  }

  if (start > 0) {
    buf->ndata -= start;
    memmove(buf->data, buf->data+start, buf->ndata);
    pc->last_write = renew_timer_start(pc->last_write,
				       TIMER_USER, TIMER_ACTIVE);
  }
  return 0;
}


/**************************************************************************
  flush'em
**************************************************************************/
void flush_connection_send_buffer_all(struct connection *pc)
{
  if(pc && pc->used && pc->send_buffer->ndata > 0) {
    write_socket_data(pc, pc->send_buffer, 0);
    if (pc->notify_of_writable_data) {
      pc->notify_of_writable_data(pc, pc->send_buffer
				  && pc->send_buffer->ndata > 0);
    }
  }
}

/**************************************************************************
  flush'em
**************************************************************************/
static void flush_connection_send_buffer_packets(struct connection *pc)
{
  if(pc && pc->used && pc->send_buffer->ndata >= MAX_LEN_PACKET) {
    write_socket_data(pc, pc->send_buffer, MAX_LEN_PACKET-1);
    if (pc->notify_of_writable_data) {
      pc->notify_of_writable_data(pc, pc->send_buffer
				  && pc->send_buffer->ndata > 0);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
static bool add_connection_data(struct connection *pc,
				const unsigned char *data, int len)
{
  if (pc && pc->delayed_disconnect) {
    if (delayed_disconnect > 0) {
      return TRUE;
    } else {
      if (close_callback) {
	(*close_callback)(pc);
      }
      return FALSE;
    }
  }

  if (pc && pc->used) {
    struct socket_packet_buffer *buf;

    buf = pc->send_buffer;

    freelog(LOG_DEBUG, "add %d bytes to %d (space=%d)", len, buf->ndata,
	    buf->nsize);
    if (!buffer_ensure_free_extra_space(buf, len)) {
      if (delayed_disconnect > 0) {
	pc->delayed_disconnect = TRUE;
	return TRUE;
      } else {
	if (close_callback) {
	  (*close_callback) (pc);
	}
	return FALSE;
      }
    }
    memcpy(buf->data + buf->ndata, data, len);
    buf->ndata += len;
    return TRUE;
  }
  return TRUE;
}

/**************************************************************************
  write data to socket
**************************************************************************/
void send_connection_data(struct connection *pc, const unsigned char *data,
			  int len)
{
  if (pc && pc->used) {
    pc->statistics.bytes_send += len;
    /* FI*/
    /*if(pc->send_buffer->do_buffer_sends > 0) {
      flush_connection_send_buffer_packets(pc);
      if (!add_connection_data(pc, data, len)) {
	freelog(LOG_ERROR, "cut connection %s due to huge send buffer (1)",
		conn_description(pc));
      }
      flush_connection_send_buffer_packets(pc);
    }
    else {*/
      flush_connection_send_buffer_all(pc);
      if (!add_connection_data(pc, data, len)) {
	freelog(LOG_ERROR, "cut connection %s due to huge send buffer (2)",
		conn_description(pc));
      }
      flush_connection_send_buffer_all(pc);
    }
  //}
}

/**************************************************************************
  Turn on buffering, using a counter so that calls may be nested.
**************************************************************************/
void connection_do_buffer(struct connection *pc)
{
  if (pc && pc->used) {
    pc->send_buffer->do_buffer_sends++;
  }
}

/**************************************************************************
  Turn off buffering if internal counter of number of times buffering
  was turned on falls to zero, to handle nested buffer/unbuffer pairs.
  When counter is zero, flush any pending data.
**************************************************************************/
void connection_do_unbuffer(struct connection *pc)
{
  if (pc && pc->used) {
    pc->send_buffer->do_buffer_sends--;
    if (pc->send_buffer->do_buffer_sends < 0) {
      freelog(LOG_ERROR, "Too many calls to unbuffer %s!", pc->username);
      pc->send_buffer->do_buffer_sends = 0;
    }
    if(pc->send_buffer->do_buffer_sends == 0)
      flush_connection_send_buffer_all(pc);
  }
}

/**************************************************************************
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

/***************************************************************
  Find connection by exact user name, from game.all_connections,
  case-insensitve.  Returns NULL if not found.
***************************************************************/
struct connection *find_conn_by_user(const char *user_name)
{
  conn_list_iterate(game.all_connections, pconn) {
    if (mystrcasecmp(user_name, pconn->username)==0) {
      return pconn;
    }
  } conn_list_iterate_end;
  return NULL;
}

/***************************************************************
  Like find_conn_by_username(), but allow unambigous prefix
  (ie abbreviation).
  Returns NULL if could not match, or if ambiguous or other
  problem, and fills *result with characterisation of
  match/non-match (see shared.[ch])
***************************************************************/
static const char *connection_accessor(int i) {
  return conn_list_get(game.all_connections, i)->username;
}

struct connection *find_conn_by_user_prefix(const char *user_name,
                                            enum m_pre_result *result)
{
  int ind;

  *result = match_prefix(connection_accessor,
			 conn_list_size(game.all_connections),
			 MAX_LEN_NAME-1, mystrncasequotecmp,
                         effectivestrlenquote, user_name, &ind);
  
  if (*result < M_PRE_AMBIGUOUS) {
    return conn_list_get(game.all_connections, ind);
  } else {
    return NULL;
  }
}

/***************************************************************
  Find connection by id, from game.all_connections.
  Returns NULL if not found.
  Number of connections will always be relatively small given
  current implementation, so linear search should be fine.
***************************************************************/
struct connection *find_conn_by_id(int id)
{
  conn_list_iterate(game.all_connections, pconn) {
    if (pconn->id == id) {
      return pconn;
    }
  }
  conn_list_iterate_end;
  return NULL;
}

/**************************************************************************
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

/**************************************************************************
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

/**************************************************************************
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
    my_snprintf(buffer, sizeof(buffer), _("%s from %s"),
		pconn->username, pconn->addr); 
  } else {
    sz_strlcpy(buffer, "server");
  }
  if (!pconn->established) {
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

/****************************************************************************
  Return TRUE iff the connection is currently allowed to edit.
****************************************************************************/
bool can_conn_edit(const struct connection *pconn)
{
  return can_conn_enable_editing(pconn) && game.info.is_edit_mode;
}

/****************************************************************************
  Return TRUE iff the connection is allowed to start editing.
****************************************************************************/
bool can_conn_enable_editing(const struct connection *pconn)
{
  return pconn->access_level == ALLOW_HACK;
}

/**************************************************************************
  Get next request id. Takes wrapping of the 16 bit wide unsigned int
  into account.
**************************************************************************/
int get_next_request_id(int old_request_id)
{
  int result = old_request_id + 1;

  if ((result & 0xffff) == 0) {
    freelog(LOG_PACKET,
	    "INFORMATION: request_id has wrapped around; "
	    "setting from %d to 2", result);
    result = 2;
  }
  assert(result);
  return result;
}

/**************************************************************************
 ...
**************************************************************************/
void free_compression_queue(struct connection *pc)
{
#ifdef USE_COMPRESSION
  byte_vector_free(&pc->compression.queue);
#endif
}

/**************************************************************************
 ...
**************************************************************************/
static void init_packet_hashs(struct connection *pc)
{
  enum packet_type i;

  pc->phs.sent = fc_malloc(sizeof(*pc->phs.sent) * PACKET_LAST);
  pc->phs.received = fc_malloc(sizeof(*pc->phs.received) * PACKET_LAST);
  pc->phs.variant = fc_malloc(sizeof(*pc->phs.variant) * PACKET_LAST);

  for (i = 0; i < PACKET_LAST; i++) {
    pc->phs.sent[i] = NULL;
    pc->phs.received[i] = NULL;
    pc->phs.variant[i] = -1;
  }
}

/**************************************************************************
 ...
**************************************************************************/
static void free_packet_hashes(struct connection *pc)
{
  int i;

  conn_clear_packet_cache(pc);

  if (pc->phs.sent) {
    for (i = 0; i < PACKET_LAST; i++) {
      if (pc->phs.sent[i] != NULL) {
	hash_free(pc->phs.sent[i]);
	pc->phs.sent[i] = NULL;
      }
    }
    free(pc->phs.sent);
    pc->phs.sent = NULL;
  }

  if (pc->phs.received) {
    for (i = 0; i < PACKET_LAST; i++) {
      if (pc->phs.received[i] != NULL) {
	hash_free(pc->phs.received[i]);
	pc->phs.received[i] = NULL;
      }
    }
    free(pc->phs.received);
    pc->phs.received = NULL;
  }

  if (pc->phs.variant) {
    free(pc->phs.variant);
    pc->phs.variant = NULL;
  }
}

/**************************************************************************
 ...
**************************************************************************/
void connection_common_init(struct connection *pconn)
{
  pconn->established = FALSE;
  pconn->used = TRUE;
  pconn->last_write = NULL;
  pconn->buffer = new_socket_packet_buffer();
  pconn->send_buffer = new_socket_packet_buffer();
  pconn->statistics.bytes_send = 0;

  init_packet_hashs(pconn);

#ifdef USE_COMPRESSION
  byte_vector_init(&pconn->compression.queue);
  pconn->compression.frozen_level = 0;
#endif
}

/**************************************************************************
 ...
**************************************************************************/
void connection_common_close(struct connection *pconn)
{
  if (!pconn->used) {
    freelog(LOG_ERROR, "WARNING: Trying to close already closed connection");
  } else {
    fc_closesocket(pconn->sock);
    pconn->used = FALSE;
    pconn->established = FALSE;

    free_socket_packet_buffer(pconn->buffer);
    pconn->buffer = NULL;

    free_socket_packet_buffer(pconn->send_buffer);
    pconn->send_buffer = NULL;

    if (pconn->last_write) {
      free_timer(pconn->last_write);
      pconn->last_write = NULL;
    }

    free_compression_queue(pconn);
    free_packet_hashes(pconn);
  }
}

/**************************************************************************
 Remove all cached packets from the connection. This resets the
 delta-state.
**************************************************************************/
void conn_clear_packet_cache(struct connection *pc)
{
  int i;

  for (i = 0; i < PACKET_LAST; i++) {
    if (pc->phs.sent != NULL && pc->phs.sent[i] != NULL) {
      struct hash_table *hash = pc->phs.sent[i];
      while (hash_num_entries(hash) > 0) {
	const void *key = hash_key_by_number(hash, 0);
	hash_delete_entry(hash, key);
	free((void *) key);
      }
    }
    if (pc->phs.received != NULL && pc->phs.received[i] != NULL) {
      struct hash_table *hash = pc->phs.received[i];
      while (hash_num_entries(hash) > 0) {
	const void *key = hash_key_by_number(hash, 0);
	hash_delete_entry(hash, key);
	free((void *) key);
      }
    }
  }
}

/**************************************************************************
  Returns TRUE if the given connection is attached to a player which it
  also controls (i.e. not a player observer).
**************************************************************************/
bool conn_controls_player(const struct connection *pconn)
{
  return pconn && pconn->playing && !pconn->observer;
}

/**************************************************************************
  Returns TRUE if the given connection is a global observer.
**************************************************************************/
bool conn_is_global_observer(const struct connection *pconn)
{
  return pconn && !pconn->playing && pconn->observer;
}

/**************************************************************************
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

/**************************************************************************
  Returns the current access level of the given connection.
  NB: If 'pconn' is NULL, this function will return ALLOW_NONE.
**************************************************************************/
enum cmdlevel_id conn_get_access(const struct connection *pconn)
{
  if (!pconn) {
    return ALLOW_NONE; /* Would not want to give hack on error... */
  }
  return pconn->access_level;
}
