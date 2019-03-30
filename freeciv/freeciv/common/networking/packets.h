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
#ifndef FC__PACKETS_H
#define FC__PACKETS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct connection;
struct data_in;

/* utility */
#include "shared.h"		/* MAX_LEN_ADDR */

/* common */
#include "connection.h"		/* struct connection, MAX_LEN_* */
#include "diptreaty.h"
#include "effects.h"
#include "events.h"
#include "improvement.h"	/* bv_imprs */
#include "player.h"
#include "requirements.h"
#include "spaceship.h"
#include "team.h"
#include "tile.h"
#include "traderoutes.h"
#include "unittype.h"
#include "worklist.h"


/* Used in network protocol. */
#define MAX_LEN_MSG             1536
#define MAX_LEN_ROUTE		2000	  /* MAX_LEN_PACKET / 2 - header */

#ifdef FREECIV_WEB
#define web_send_packet(packetname, ...) \
  send_packet_web_ ##packetname( __VA_ARGS__ )
#define web_lsend_packet(packetname, ...) \
  lsend_packet_web_ ##packetname( __VA_ARGS__ )
#else  /* FREECIV_WEB */
#define web_send_packet(packetname, ...)
#define web_lsend_packet(packetname, ...)
#endif /* FREECIV_WEB */

/* The size of opaque (void *) data sent in the network packet.  To avoid
 * fragmentation issues, this SHOULD NOT be larger than the standard
 * ethernet or PPP 1500 byte frame size (with room for headers).
 *
 * Do not spend much time optimizing, you have no idea of the actual dynamic
 * path characteristics between systems, such as VPNs and tunnels.
 *
 * Used in network protocol.
 */
#define ATTRIBUTE_CHUNK_SIZE    (1400)

/* Used in network protocol. */
enum report_type {
  REPORT_WONDERS_OF_THE_WORLD,
  REPORT_TOP_5_CITIES,
  REPORT_DEMOGRAPHIC,
  REPORT_ACHIEVEMENTS
};

/* Used in network protocol. */
enum unit_info_use {
  UNIT_INFO_IDENTITY,
  UNIT_INFO_CITY_SUPPORTED,
  UNIT_INFO_CITY_PRESENT
};

/* Used in network protocol. */
enum authentication_type {
  AUTH_LOGIN_FIRST,   /* request a password for a returning user */
  AUTH_NEWUSER_FIRST, /* request a password for a new user */
  AUTH_LOGIN_RETRY,   /* inform the client to try a different password */
  AUTH_NEWUSER_RETRY  /* inform the client to try a different [new] password */
};

#include "packets_gen.h"

struct packet_handlers {
  union {
    int (*no_packet)(struct connection *pconn);
    int (*packet)(struct connection *pconn, const void *packet);
    int (*force_to_send)(struct connection *pconn, const void *packet,
                         bool force_to_send);
  } send[PACKET_LAST];
  void *(*receive[PACKET_LAST])(struct connection *pconn);
};

void *get_packet_from_connection_raw(struct connection *pc,
                                     enum packet_type *ptype);

#ifdef FREECIV_JSON_CONNECTION
#define get_packet_from_connection(pc, ptype) get_packet_from_connection_json(pc, ptype)
#else
#define get_packet_from_connection(pc, ptype) get_packet_from_connection_raw(pc, ptype)
#endif

void remove_packet_from_buffer(struct socket_packet_buffer *buffer);

void send_attribute_block(const struct player *pplayer,
			  struct connection *pconn);
void generic_handle_player_attribute_chunk(struct player *pplayer,
					   const struct
					   packet_player_attribute_chunk
					   *chunk);
void packet_handlers_fill_initial(struct packet_handlers *phandlers);
void packet_handlers_fill_capability(struct packet_handlers *phandlers,
                                     const char *capability);
const char *packet_name(enum packet_type type);
bool packet_has_game_info_flag(enum packet_type type);

void packet_header_init(struct packet_header *packet_header);
void post_send_packet_server_join_reply(struct connection *pconn,
                                        const struct packet_server_join_reply
                                        *packet);
void post_receive_packet_server_join_reply(struct connection *pconn,
                                           const struct
                                           packet_server_join_reply *packet);

void pre_send_packet_player_attribute_chunk(struct connection *pc,
					    struct packet_player_attribute_chunk
					    *packet);

const struct packet_handlers *packet_handlers_initial(void);
const struct packet_handlers *packet_handlers_get(const char *capability);

void packets_deinit(void);

#ifdef FREECIV_JSON_CONNECTION
#include "packets_json.h"
#else

#define SEND_PACKET_START(packet_type) \
  unsigned char buffer[MAX_LEN_PACKET]; \
  struct raw_data_out dout; \
  \
  dio_output_init(&dout, buffer, sizeof(buffer)); \
  dio_put_type_raw(&dout, pc->packet_header.length, 0); \
  dio_put_type_raw(&dout, pc->packet_header.type, packet_type);

#define SEND_PACKET_END(packet_type) \
  { \
    size_t size = dio_output_used(&dout); \
    \
    dio_output_rewind(&dout); \
    dio_put_type_raw(&dout, pc->packet_header.length, size); \
    fc_assert(!dout.too_short); \
    return send_packet_data(pc, buffer, size, packet_type); \
  }

#define RECEIVE_PACKET_START(packet_type, result) \
  struct data_in din; \
  struct packet_type packet_buf, *result = &packet_buf; \
  \
  dio_input_init(&din, pc->buffer->data, \
                 data_type_size(pc->packet_header.length)); \
  { \
    int size; \
  \
    dio_get_type_raw(&din, pc->packet_header.length, &size); \
    dio_input_init(&din, pc->buffer->data, MIN(size, pc->buffer->ndata)); \
  } \
  dio_input_skip(&din, (data_type_size(pc->packet_header.length) \
                        + data_type_size(pc->packet_header.type)));

#define RECEIVE_PACKET_END(result) \
  if (!packet_check(&din, pc)) { \
    return NULL; \
  } \
  remove_packet_from_buffer(pc->buffer); \
  result = fc_malloc(sizeof(*result)); \
  *result = packet_buf; \
  return result;

#define RECEIVE_PACKET_FIELD_ERROR(field, ...) \
  log_packet("Error on field '" #field "'" __VA_ARGS__); \
  return NULL

#endif /* FREECIV_JSON_PROTOCOL */

int send_packet_data(struct connection *pc, unsigned char *data, int len,
                     enum packet_type packet_type);
bool packet_check(struct data_in *din, struct connection *pc);

/* Utilities to exchange strings and string vectors. */
#define PACKET_STRVEC_SEPARATOR '\3'
#define PACKET_STRVEC_COMPUTE(str, strvec)                                  \
  if (NULL != strvec) {                                                     \
    strvec_to_str(strvec, PACKET_STRVEC_SEPARATOR, str, sizeof(str));       \
  } else {                                                                  \
    str[0] = '\0';                                                          \
  }
#define PACKET_STRVEC_EXTRACT(strvec, str)                                  \
  if ('\0' != str[0]) {                                                     \
    strvec = strvec_new();                                                  \
    strvec_from_str(strvec, PACKET_STRVEC_SEPARATOR, str);                  \
  } else {                                                                  \
    strvec = NULL;                                                          \
  }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__PACKETS_H */
