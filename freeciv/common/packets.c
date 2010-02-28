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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#include "capability.h"
#include "dataio.h"
#include "events.h"
#include "fcintl.h"
#include "hash.h"
#include "log.h"
#include "map.h"
#include "mem.h"
#include "support.h"

#include "packets.h"

#ifdef USE_COMPRESSION
#include <zlib.h>
/*
 * Value for the 16bit size to indicate a jumbo packet
 */
#define JUMBO_SIZE		0xffff

/*
 * All values 0<=size<COMPRESSION_BORDER are uncompressed packets.
 */
#define COMPRESSION_BORDER	(16*1024+1)

/*
 * All compressed packets over this size are sent as a jumbo packet.
 */
#define JUMBO_BORDER 		(64*1024-COMPRESSION_BORDER-1)
#endif

#define BASIC_PACKET_LOG_LEVEL	LOG_VERBOSE
#define COMPRESS_LOG_LEVEL	LOG_DEBUG
#define COMPRESS2_LOG_LEVEL	LOG_DEBUG

/* 
 * Valid values are 0, 1 and 2. For 2 you have to set generate_stats
 * to 1 in generate_packets.py.
 */
#define PACKET_SIZE_STATISTICS 0

/********************************************************************** 
 The current packet functions don't handle signed values
 correct. This will probably lead to problems when compiling
 freeciv for a platform which has 64 bit ints. Also 16 and 8
 bits values cannot be signed without using tricks (look in e.g
 receive_packet_city_info() )

  TODO to solve these problems:
    o use the new signed functions where they are necessary
    o change the prototypes of the unsigned functions
       to unsigned int instead of int (but only on the 32
       bit functions, because otherwhere it's not necessary)

  Possibe enhancements:
    o check in configure for ints with size others than 4
    o write real put functions and check for the limit
***********************************************************************/

/**************************************************************************
  It returns the request id of the outgoing packet (or 0 if pc->is_server).
**************************************************************************/
int send_packet_data(struct connection *pc, unsigned char *data, int len)
{
  /* default for the server */
  int result = 0;

  freelog(BASIC_PACKET_LOG_LEVEL, "sending packet type=%s(%d) len=%d",
	  get_packet_name(data[2]), data[2], len);

  if (!pc->is_server) {
    pc->client.last_request_id_used =
	get_next_request_id(pc->client.last_request_id_used);
    result = pc->client.last_request_id_used;
    freelog(BASIC_PACKET_LOG_LEVEL, "sending request %d", result);
  }

  if (pc->outgoing_packet_notify) {
    pc->outgoing_packet_notify(pc, data[2], len, result);
  }

#ifdef USE_COMPRESSION
  if(TRUE) {
    static int stat_size_alone, stat_size_uncompressed, stat_size_compressed,
	stat_size_no_compression;
    static bool compression_level_initialized = FALSE;
    static int compression_level;
    int packet_type = data[2];
    int size = len;

    if (!compression_level_initialized) {
      char *s = getenv("FREECIV_COMPRESSION_LEVEL");
      if (!s || sscanf(s, "%d", &compression_level) != 1
	  || compression_level < -1 || compression_level > 9) {
	compression_level = -1;
      }
      compression_level_initialized = TRUE;
    }

    /* TODO: PACKET_FREEZE_HINT and PACKET_THAW_HINT are meaningful
     * only internally. They should not be sent to connection at all.
     * Freezing could also be handled via separate functions, and
     * not by special packets.
     * Only problem is backward compatibility, so this cannot be
     * changed in stable branch. */
    if (packet_type == PACKET_PROCESSING_STARTED
	|| packet_type == PACKET_FREEZE_HINT) {
      if (pc->compression.frozen_level == 0) {
	byte_vector_reserve(&pc->compression.queue, 0);
      }
      pc->compression.frozen_level++;
    }

    if (pc->compression.frozen_level > 0) {
      size_t old_size = pc->compression.queue.size;

      byte_vector_reserve(&pc->compression.queue, old_size + len);
      memcpy(pc->compression.queue.p + old_size, data, len);
      freelog(COMPRESS2_LOG_LEVEL, "COMPRESS: putting %s into the queue",
	      get_packet_name(packet_type));
    } else {
      stat_size_alone += size;
      freelog(COMPRESS_LOG_LEVEL, "COMPRESS: sending %s alone (%d bytes total)",
	      get_packet_name(packet_type), stat_size_alone);
      send_connection_data(pc, data, len);
    }

    if (packet_type ==
	PACKET_PROCESSING_FINISHED || packet_type == PACKET_THAW_HINT) {
      pc->compression.frozen_level--;
      if (pc->compression.frozen_level == 0) {
	uLongf compressed_size = 12 + pc->compression.queue.size * 1.001;
	int error;
	Bytef compressed[compressed_size];

	error =
	    compress2(compressed, &compressed_size,
		      pc->compression.queue.p, pc->compression.queue.size,
		      compression_level);
	assert(error == Z_OK);
	if (compressed_size + 2 < pc->compression.queue.size) {
	    struct data_out dout;

	  freelog(COMPRESS_LOG_LEVEL,
		  "COMPRESS: compressed %lu bytes to %ld (level %d)",
		  (unsigned long)pc->compression.queue.size, compressed_size,
		  compression_level);
	  stat_size_uncompressed += pc->compression.queue.size;
	  stat_size_compressed += compressed_size;

	  if (compressed_size <= JUMBO_BORDER) {
	    unsigned char header[2];

	    freelog(COMPRESS_LOG_LEVEL, "COMPRESS: sending %ld as normal",
		    compressed_size);

	    dio_output_init(&dout, header, sizeof(header));
	    dio_put_uint16(&dout, 2 + compressed_size + COMPRESSION_BORDER);
	    send_connection_data(pc, header, sizeof(header));
	    send_connection_data(pc, compressed, compressed_size);
	  } else {
	    unsigned char header[6];

	    freelog(COMPRESS_LOG_LEVEL, "COMPRESS: sending %ld as jumbo",
		    compressed_size);
	    dio_output_init(&dout, header, sizeof(header));
	    dio_put_uint16(&dout, JUMBO_SIZE);
	    dio_put_uint32(&dout, 6 + compressed_size);
	    send_connection_data(pc, header, sizeof(header));
	    send_connection_data(pc, compressed, compressed_size);
	  }
	} else {
	  freelog(COMPRESS_LOG_LEVEL,
		  "COMPRESS: would enlarging %lu bytes to %ld; sending uncompressed",
		  (unsigned long)pc->compression.queue.size, compressed_size);
	  send_connection_data(pc, pc->compression.queue.p,
			       pc->compression.queue.size);
	  stat_size_no_compression += pc->compression.queue.size;
	}
      }
    }
    freelog(COMPRESS2_LOG_LEVEL,
	    "COMPRESS: STATS: alone=%d compression-expand=%d compression (before/after) = %d/%d",
	    stat_size_alone, stat_size_no_compression,
	    stat_size_uncompressed, stat_size_compressed);
  }
#else
  send_connection_data(pc, data, len);
#endif

#if PACKET_SIZE_STATISTICS
  {
    static struct {
      int counter;
      int size;
    } packets_stats[PACKET_LAST];
    static int packet_counter = 0;
    static int last_start_turn_seen = -1;
    static bool start_turn_seen = FALSE;

    int packet_type = data[2];
    int size = len;
    bool print = FALSE;
    bool clear = FALSE;

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
    if (packet_type == PACKET_START_TURN
	&& last_start_turn_seen != game.turn) {
	start_turn_seen=TRUE;
      last_start_turn_seen = game.turn;
    }

    if ((packet_type ==
	 PACKET_PROCESSING_FINISHED || packet_type == PACKET_THAW_HINT)
	&& start_turn_seen) {
      start_turn_seen = FALSE;
      print = TRUE;
      clear = TRUE;
    }

    if(print) {
      int i, sum = 0;
      int ll = LOG_DEBUG;

#if PACKET_SIZE_STATISTICS == 2
      delta_stats_report();
#endif
      freelog(ll, "Transmitted packets:");
      freelog(ll, "%8s %8s %8s %s", "Packets", "Bytes",
	      "Byt/Pac", "Name");

      for (i = 0; i < PACKET_LAST; i++) {
	if (packets_stats[i].counter == 0) {
	  continue;
	}
	sum += packets_stats[i].size;
	freelog(ll, "%8d %8d %8d %s(%i)",
		packets_stats[i].counter, packets_stats[i].size,
		packets_stats[i].size / packets_stats[i].counter,
		get_packet_name(i),i);
      }
      freelog(LOG_TEST,
	      "turn=%d; transmitted %d bytes in %d packets;average size "
	      "per packet %d bytes", game.turn, sum, packet_counter,
	      sum / packet_counter);
      freelog(LOG_TEST, "turn=%d; transmitted %d bytes", game.turn,
	      pc->statistics.bytes_send);
    }    
    if (clear) {
      int i;

      for (i = 0; i < PACKET_LAST; i++) {
	packets_stats[i].counter = 0;
	packets_stats[i].size = 0;
      }
      packet_counter = 0;
      pc->statistics.bytes_send = 0;
      delta_stats_reset();
    }
  }
#endif

  return result;
}

/**************************************************************************
presult indicates if there is more packets in the cache. We return result
instead of just testing if the returning package is NULL as we sometimes
return a NULL packet even if everything is OK (receive_packet_goto_route).
**************************************************************************/
void *get_packet_from_connection(struct connection *pc,
				 enum packet_type *ptype, bool * presult)
{
  int len_read;
  int whole_packet_len;
  union {
    enum packet_type type;
    int itype;
  } utype;
  struct data_in din;
#ifdef USE_COMPRESSION
  bool compressed_packet = FALSE;
  int header_size = 0;
#endif

  assert(sizeof(utype.type) == sizeof(utype.itype));

  *presult = FALSE;

  if (!pc->used) {
    return NULL;		/* connection was closed, stop reading */
  }
  
  if (pc->buffer->ndata < 3) {
    return NULL;           /* length and type not read */
  }

  dio_input_init(&din, pc->buffer->data, pc->buffer->ndata);
  dio_get_uint16(&din, &len_read);

  /* The non-compressed case */
  whole_packet_len = len_read;

#ifdef USE_COMPRESSION
  if (len_read == JUMBO_SIZE) {
    compressed_packet = TRUE;
    header_size = 6;
    if (dio_input_remaining(&din) >= 4) {
      dio_get_uint32(&din, &whole_packet_len);
      freelog(COMPRESS_LOG_LEVEL, "COMPRESS: got a jumbo packet of size %d", whole_packet_len);
    } else {
      /* to return NULL below */
      whole_packet_len = 6;
    }
  } else if (len_read >= COMPRESSION_BORDER) {
    compressed_packet = TRUE;
    header_size = 2;
    whole_packet_len = len_read - COMPRESSION_BORDER;
    freelog(COMPRESS_LOG_LEVEL, "COMPRESS: got a normal packet of size %d", whole_packet_len);
  }
#endif

  if ((unsigned)whole_packet_len > pc->buffer->ndata) {
    return NULL;		/* not all data has been read */
  }

#ifdef USE_COMPRESSION
  if (compressed_packet) {
    uLong compressed_size = whole_packet_len - header_size;
    /* 
     * We don't know the decompressed size. We assume a bad case
     * here: an expansion by an factor of 100. 
     */
    unsigned long int decompressed_size = 100 * compressed_size;
    void *decompressed = fc_malloc(decompressed_size);
    int error;
    struct socket_packet_buffer *buffer = pc->buffer;
    
    error =
	uncompress(decompressed, &decompressed_size,
		   ADD_TO_POINTER(buffer->data, header_size), 
		   compressed_size);
    if (error != Z_OK) {
      CLOSE_FUN close_callback = close_socket_get_callback();

      freelog(LOG_ERROR, "Uncompressing of the packet stream failed. "
	      "The connection will be closed now.");
      assert(close_callback);
      (*close_callback) (pc);

      return NULL;
    }

    buffer->ndata -= whole_packet_len;
    /* 
     * Remove the packet with the compressed data and shift all the
     * remaining data to the front. 
     */
    memmove(buffer->data, buffer->data + whole_packet_len, buffer->ndata);

    if (buffer->ndata + decompressed_size > buffer->nsize) {
      buffer->nsize += decompressed_size;
      buffer->data = fc_realloc(buffer->data, buffer->nsize);
    }

    /*
     * Make place for the uncompressed data by moving the remaining
     * data.
     */
    memmove(buffer->data + decompressed_size, buffer->data, buffer->ndata);

    /* 
     * Copy the uncompressed data.
     */
    memcpy(buffer->data, decompressed, decompressed_size);

    free(decompressed);

    buffer->ndata += decompressed_size;
    
    freelog(COMPRESS_LOG_LEVEL, "COMPRESS: decompressed %ld into %ld",
	    compressed_size, decompressed_size);

    return get_packet_from_connection(pc, ptype, presult);
  }
#endif

  /*
   * At this point the packet is a plain uncompressed one. These have
   * to have to be at least 3 bytes in size.
   */
  if (whole_packet_len < 3) {
    CLOSE_FUN close_callback = close_socket_get_callback();

    freelog(LOG_ERROR, "The packet stream is corrupt. The connection "
	    "will be closed now.");
    assert(close_callback);
    (*close_callback) (pc);

    return NULL;
  }

  dio_get_uint8(&din, &utype.itype);

  freelog(BASIC_PACKET_LOG_LEVEL, "got packet type=(%s)%d len=%d",
	  get_packet_name(utype.type), utype.itype, whole_packet_len);

  *ptype = utype.type;
  *presult = TRUE;

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

      freelog(LOG_TEST, "Received packets:");
      for (i = 0; i < PACKET_LAST; i++) {
	if (packets_stats[i].counter == 0)
	  continue;
	sum += packets_stats[i].size;
	freelog(LOG_TEST,
		"  [%-25.25s %3d]: %6d packets; %8d bytes total; "
		"%5d bytes/packet average",
		get_packet_name(i), i, packets_stats[i].counter,
		packets_stats[i].size,
		packets_stats[i].size / packets_stats[i].counter);
      }
      freelog(LOG_TEST,
	      "received %d bytes in %d packets;average size "
	      "per packet %d bytes",
	      sum, packet_counter, sum / packet_counter);
    }
  }
#endif
  return get_packet_from_connection_helper(pc, utype.type);
}

/**************************************************************************
  Remove the packet from the buffer
**************************************************************************/
void remove_packet_from_buffer(struct socket_packet_buffer *buffer)
{
  struct data_in din;
  int len;

  dio_input_init(&din, buffer->data, buffer->ndata);
  dio_get_uint16(&din, &len);
  memmove(buffer->data, buffer->data + len, buffer->ndata - len);
  buffer->ndata -= len;
  freelog(LOG_DEBUG, "remove_packet_from_buffer: remove %d; remaining %d",
	  len, buffer->ndata);
}

/**************************************************************************
  ...
**************************************************************************/
void check_packet(struct data_in *din, struct connection *pc)
{
  size_t rem = dio_input_remaining(din);

  if (din->bad_string || din->bad_bit_string || rem != 0) {
    char from[MAX_LEN_ADDR + MAX_LEN_NAME + 128];
    int type, len;

    assert(pc != NULL);
    my_snprintf(from, sizeof(from), " from %s", conn_description(pc));

    dio_input_rewind(din);
    dio_get_uint16(din, &len);
    dio_get_uint8(din, &type);

    if (din->bad_string) {
      freelog(LOG_ERROR,
	      "received bad string in packet (type %d, len %d)%s",
	      type, len, from);
    }

    if (din->bad_bit_string) {
      freelog(LOG_ERROR,
	      "received bad bit string in packet (type %d, len %d)%s",
	      type, len, from);
    }

    if (din->too_short) {
      freelog(LOG_ERROR, "received short packet (type %d, len %d)%s",
	      type, len, from);
    }

    if (rem > 0) {
      /* This may be ok, eg a packet from a newer version with extra info
       * which we should just ignore */
      freelog(LOG_VERBOSE,
	      "received long packet (type %d, len %d, rem %lu)%s", type,
	      len, (unsigned long)rem, from);
    }
  }
}

/**************************************************************************
 Updates pplayer->attribute_block according to the given packet.
**************************************************************************/
void generic_handle_player_attribute_chunk(struct player *pplayer,
					   const struct
					   packet_player_attribute_chunk
					   *chunk)
{
  freelog(BASIC_PACKET_LOG_LEVEL, "received attribute chunk %u/%u %u",
	  (unsigned int) chunk->offset,
	  (unsigned int) chunk->total_length,
	  (unsigned int) chunk->chunk_length);

  if (chunk->total_length < 0
      || chunk->chunk_length < 0
      || chunk->total_length >= MAX_ATTRIBUTE_BLOCK
      || chunk->offset < 0
      || chunk->offset > chunk->total_length /* necessary check on 32 bit systems */
      || chunk->chunk_length > chunk->total_length
      || chunk->offset + chunk->chunk_length > chunk->total_length
      || (chunk->offset != 0
          && chunk->total_length != pplayer->attribute_block_buffer.length)) {
    /* wrong attribute data */
    if (pplayer->attribute_block_buffer.data) {
      free(pplayer->attribute_block_buffer.data);
      pplayer->attribute_block_buffer.data = NULL;
    }
    pplayer->attribute_block_buffer.length = 0;
    freelog(LOG_ERROR, "Received wrong attribute chunk");
    return;
  }
  /* first one in a row */
  if (chunk->offset == 0) {
    if (pplayer->attribute_block_buffer.data) {
      free(pplayer->attribute_block_buffer.data);
      pplayer->attribute_block_buffer.data = NULL;
    }
    pplayer->attribute_block_buffer.data = fc_malloc(chunk->total_length);
    pplayer->attribute_block_buffer.length = chunk->total_length;
  }
  memcpy((char *) (pplayer->attribute_block_buffer.data) + chunk->offset,
	 chunk->data, chunk->chunk_length);
  
  if (chunk->offset + chunk->chunk_length == chunk->total_length) {
    /* Received full attribute block */
    if (pplayer->attribute_block.data != NULL) {
      free(pplayer->attribute_block.data);
    }
    pplayer->attribute_block.data = pplayer->attribute_block_buffer.data;
    pplayer->attribute_block.length = pplayer->attribute_block_buffer.length;
    
    pplayer->attribute_block_buffer.data = NULL;
    pplayer->attribute_block_buffer.length = 0;
  }
}

/**************************************************************************
 Split the attribute block into chunks and send them over pconn.
**************************************************************************/
void send_attribute_block(const struct player *pplayer,
			  struct connection *pconn)
{
  struct packet_player_attribute_chunk packet;
  int current_chunk, chunks, bytes_left;

  if (!pplayer || !pplayer->attribute_block.data) {
    return;
  }

  assert(pplayer->attribute_block.length > 0 &&
	 pplayer->attribute_block.length < MAX_ATTRIBUTE_BLOCK);

  chunks =
      (pplayer->attribute_block.length - 1) / ATTRIBUTE_CHUNK_SIZE + 1;
  bytes_left = pplayer->attribute_block.length;

  connection_do_buffer(pconn);

  for (current_chunk = 0; current_chunk < chunks; current_chunk++) {
    int size_of_current_chunk = MIN(bytes_left, ATTRIBUTE_CHUNK_SIZE);

    packet.offset = ATTRIBUTE_CHUNK_SIZE * current_chunk;
    packet.total_length = pplayer->attribute_block.length;
    packet.chunk_length = size_of_current_chunk;

    memcpy(packet.data,
	   (char *) (pplayer->attribute_block.data) + packet.offset,
	   packet.chunk_length);
    bytes_left -= packet.chunk_length;

    if (packet.chunk_length < ATTRIBUTE_CHUNK_SIZE) {
      /* Last chunk is not full. Make sure that delta does
       * not use random data. */
      memset(packet.data + packet.chunk_length, 0,
             ATTRIBUTE_CHUNK_SIZE - packet.chunk_length);
    }

    send_packet_player_attribute_chunk(pconn, &packet);
  }

  connection_do_unbuffer(pconn);
}

/**************************************************************************
  This function is a hack to convert the internal form for storing
  connection IDs and map positions into the network form.  It's called
  directly on a packet before it's written to the network.
**************************************************************************/
void pre_send_packet_chat_msg(struct connection *pc,
			      struct packet_chat_msg *packet)
{
  if (packet->conn_id == -1) {
    /* since we can currently only send unsigned ints... */
    packet->conn_id = 255;
  }

  if (packet->x == -1 && packet->y == -1) {
    /* since we can currently only send unsigned ints... */
    assert(!is_normal_map_pos(255, 255));
    packet->x = 255;
    packet->y = 255;
  }
}

/**************************************************************************
  This function is a hack to convert the network form for storing
  connection IDs and map positions into the internal form.  It's
  called directly on a packet after it's been received from the network.
**************************************************************************/
void post_receive_packet_chat_msg(struct connection *pc,
				  struct packet_chat_msg *packet)
{
  if (packet->x == 255 && packet->y == 255) {
    /* unsigned encoding for no position */
    packet->x = -1;
    packet->y = -1;
  }
  if (packet->conn_id == 255) {
    packet->conn_id = -1;
  }
}

/**************************************************************************
  Test and log for sending player attribute_block
**************************************************************************/
void pre_send_packet_player_attribute_chunk(struct connection *pc,
					    struct packet_player_attribute_chunk
					    *packet)
{
  assert(packet->total_length > 0
	 && packet->total_length < MAX_ATTRIBUTE_BLOCK);
  /* 500 bytes header, just to be sure */
  assert(packet->chunk_length > 0
	 && packet->chunk_length < MAX_LEN_PACKET - 500);
  assert(packet->chunk_length <= packet->total_length);
  assert(packet->offset >= 0 && packet->offset < packet->total_length);

  freelog(BASIC_PACKET_LOG_LEVEL, "sending attribute chunk %d/%d %d",
	  packet->offset, packet->total_length, packet->chunk_length);

}

/**************************************************************************
  ...
**************************************************************************/
void post_receive_packet_ruleset_control(struct connection *pc,
                                         struct packet_ruleset_control *packet)
{
  conn_clear_packet_cache(pc);
}

/**************************************************************************
  ...
**************************************************************************/
void post_send_packet_ruleset_control(struct connection *pc,
                                      const struct packet_ruleset_control *packet)
{
  conn_clear_packet_cache(pc);
}
