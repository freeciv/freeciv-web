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

#ifdef FREECIV_JSON_CONNECTION

#include "fc_prehdrs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include <jansson.h>

/* utility */
#include "capability.h"
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* commmon */
#include "dataio.h"
#include "game.h"
#include "events.h"
#include "map.h"

#include "packets_json.h"

/* 
 * Valid values are 0, 1 and 2. For 2 you have to set generate_stats
 * to 1 in generate_packets.py.
 */
#define PACKET_SIZE_STATISTICS 0

extern const char *const packet_functional_capability;

#define SPECHASH_TAG packet_handler
#define SPECHASH_ASTR_KEY_TYPE
#define SPECHASH_IDATA_TYPE struct packet_handlers *
#define SPECHASH_IDATA_FREE (packet_handler_hash_data_free_fn_t) free
#include "spechash.h"

/**********************************************************************//**
  Read and return a packet from the connection 'pc'. The type of the
  packet is written in 'ptype'. On error, the connection is closed and
  the function returns NULL.
**************************************************************************/
void *get_packet_from_connection_json(struct connection *pc,
                                      enum packet_type *ptype)
{
  int len_read;
  int whole_packet_len;
  struct {
    enum packet_type type;
    int itype;
  } utype;
  struct data_in din;
  void *data;
  void *(*receive_handler)(struct connection *);
  json_error_t error;
  json_t *pint;

  if (!pc->used) {
    return NULL;		/* connection was closed, stop reading */
  }
  
  if (pc->buffer->ndata < data_type_size(pc->packet_header.length)) {
    /* Not got enough for a length field yet */
    return NULL;
  }

  dio_input_init(&din, pc->buffer->data, pc->buffer->ndata);
  dio_get_uint16_raw(&din, &len_read);

  /* The non-compressed case */
  whole_packet_len = len_read;

  if ((unsigned)whole_packet_len > pc->buffer->ndata) {
    return NULL;		/* not all data has been read */
  }

  /*
   * At this point the packet is a plain uncompressed one. These have
   * to have to be at least the header bytes in size.
   */
  if (whole_packet_len < (data_type_size(pc->packet_header.length))) {
    log_verbose("The packet stream is corrupt. The connection "
                "will be closed now.");
    connection_close(pc, _("decoding error"));
    return NULL;
  }

  /*
   * The server tries to parse as JSON the first packet that it gets on a
   * connection. If it is a valid JSON packet, the connection is switched
   * to JSON mode.
   */
  if (is_server() && pc->server.last_request_id_seen == 0) {
    /* Try to parse JSON packet. Note that json string has '\0' */
    pc->json_packet = json_loadb((char*)pc->buffer->data + 2, whole_packet_len - 3, 0, &error);

    /* Set the connection mode */
    pc->json_mode = (pc->json_packet != NULL);
  }

  if (pc->json_mode) {
    /* Parse JSON packet. Note that json string has '\0' */
    pc->json_packet = json_loadb((char*)pc->buffer->data + 2, whole_packet_len - 3, 0, &error);

    /* Log errors before we scrap the data */
    if (!pc->json_packet) {
      log_error("ERROR: Unable to parse packet: %s", pc->buffer->data + 2);
      log_error("%s", error.text);
    }

    log_packet_json("Json in: %s", pc->buffer->data + 2);

    /* Shift remaining data to the front */
    pc->buffer->ndata -= whole_packet_len;
    memmove(pc->buffer->data, pc->buffer->data + whole_packet_len, pc->buffer->ndata);

    if (!pc->json_packet) {
      return NULL;
    }

    pint = json_object_get(pc->json_packet, "pid");

    if (!pint) {
      log_error("ERROR: Unable to get packet type.");
      return NULL;
    }

    json_int_t packet_type = json_integer_value(pint);
    utype.type = packet_type;
  } else {
    dio_get_type_raw(&din, pc->packet_header.type, &utype.itype);
    utype.type = utype.itype;
  }

  if (utype.type < 0
      || utype.type >= PACKET_LAST
      || (receive_handler = pc->phs.handlers->receive[utype.type]) == NULL) {
    log_verbose("Received unsupported packet type %d (%s). The connection "
                "will be closed now.",
                utype.type, packet_name(utype.type));
    connection_close(pc, _("unsupported packet type"));
    return NULL;
  }

  log_packet("got packet type=(%s) len=%d from %s",
             packet_name(utype.type), whole_packet_len,
             is_server() ? pc->username : "server");

  *ptype = utype.type;

  if (pc->incoming_packet_notify) {
    pc->incoming_packet_notify(pc, utype.type, whole_packet_len);
  }

#if PACKET_SIZE_STATISTICS 
  {
    static struct {
      int counter;
      int size;
    } packets_stats[PACKET_LAST];
    static int packet_counter = 0;

    int packet_type = utype.itype;
    int size = whole_packet_len;

    if (!packet_counter) {
      int i;

      for (i = 0; i < PACKET_LAST; i++) {
	packets_stats[i].counter = 0;
	packets_stats[i].size = 0;
      }
    }

    packets_stats[packet_type].counter++;
    packets_stats[packet_type].size += size;

    packet_counter++;
    if (packet_counter % 100 == 0) {
      int i, sum = 0;

      log_test("Received packets:");
      for (i = 0; i < PACKET_LAST; i++) {
	if (packets_stats[i].counter == 0)
	  continue;
	sum += packets_stats[i].size;
        log_test("  [%-25.25s %3d]: %6d packets; %8d bytes total; "
                 "%5d bytes/packet average",
                 packet_name(i), i, packets_stats[i].counter,
                 packets_stats[i].size,
                 packets_stats[i].size / packets_stats[i].counter);
      }
      log_test("received %d bytes in %d packets;average size "
               "per packet %d bytes",
               sum, packet_counter, sum / packet_counter);
    }
  }
#endif /* PACKET_SIZE_STATISTICS */
  data = receive_handler(pc);
  if (!data) {
    connection_close(pc, _("incompatible packet contents"));
    return NULL;
  } else {
    return data;
  }
}

#endif /* FREECIV_JSON_CONNECTION */
