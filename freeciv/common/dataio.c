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

/*
 * The DataIO module provides a system independent (endianess and
 * sizeof(int) independent) way to write and read data. It supports
 * multiple datas which are collected in a buffer. It provides
 * recognition of error cases like "not enough space" or "not enough
 * data".
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_WINSOCK
#include <winsock.h>
#endif

#include "capability.h"
#include "events.h"
#include "log.h"
#include "mem.h"
#include "player.h"
#include "requirements.h"
#include "support.h"
#include "tech.h"
#include "worklist.h"

#include "dataio.h"

/**************************************************************************
...
**************************************************************************/
static DIO_PUT_CONV_FUN put_conv_callback = NULL;

/**************************************************************************
...
**************************************************************************/
void dio_set_put_conv_callback(DIO_PUT_CONV_FUN fun)
{
  put_conv_callback = fun;
}

/**************************************************************************
 Returns FALSE if the destination isn't large enough or the source was
 bad.
**************************************************************************/
static bool get_conv(char *dst, size_t ndst, const char *src,
		     size_t nsrc)
{
  size_t len = nsrc;		/* length to copy, not including null */
  bool ret = TRUE;

  if (ndst > 0 && len >= ndst) {
    ret = FALSE;
    len = ndst - 1;
  }

  memcpy(dst, src, len);
  dst[len] = '\0';

  return ret;
}

/**************************************************************************
...
**************************************************************************/
static DIO_GET_CONV_FUN get_conv_callback = get_conv;

/**************************************************************************
...
**************************************************************************/
void dio_set_get_conv_callback(DIO_GET_CONV_FUN fun)
{
  get_conv_callback = fun;
}

/**************************************************************************
  Returns TRUE iff the output has size bytes available.
**************************************************************************/
static bool enough_space(struct data_out *dout, size_t size)
{
  if (ADD_TO_POINTER(dout->current, size) >=
      ADD_TO_POINTER(dout->dest, dout->dest_size)) {
    dout->too_short = TRUE;
    return FALSE;
  } else {
    dout->used = MAX(dout->used, dout->current + size);
    return TRUE;
  }
}

/**************************************************************************
  Returns TRUE iff the input contains size unread bytes.
**************************************************************************/
static bool enough_data(struct data_in *din, size_t size)
{
  if (dio_input_remaining(din) < size) {
    din->too_short = TRUE;
    return FALSE;
  } else {
    return TRUE;
  }
}

/**************************************************************************
  Initializes the output to the given output buffer and the given
  buffer size.
**************************************************************************/
void dio_output_init(struct data_out *dout, void *destination,
		     size_t dest_size)
{
  dout->dest = destination;
  dout->dest_size = dest_size;
  dout->current = 0;
  dout->used = 0;
  dout->too_short = FALSE;
}

/**************************************************************************
  Return the maximum number of bytes used.
**************************************************************************/
size_t dio_output_used(struct data_out *dout)
{
  return dout->used;
}

/**************************************************************************
  Rewinds the stream so that the put-functions start from the
  beginning.
**************************************************************************/
void dio_output_rewind(struct data_out *dout)
{
  dout->current = 0;
}

/**************************************************************************
  Initializes the input to the given input buffer and the given
  number of valid input bytes.
**************************************************************************/
void dio_input_init(struct data_in *din, const void *src, size_t src_size)
{
  din->src = src;
  din->src_size = src_size;
  din->current = 0;
  din->too_short = FALSE;
  din->bad_string = FALSE;
  din->bad_bit_string = FALSE;
}

/**************************************************************************
  Rewinds the stream so that the get-functions start from the
  beginning.
**************************************************************************/
void dio_input_rewind(struct data_in *din)
{
  din->current = 0;
}

/**************************************************************************
  Return the number of unread bytes.
**************************************************************************/
size_t dio_input_remaining(struct data_in *din)
{
  return din->src_size - din->current;
}

/**************************************************************************
...
**************************************************************************/
void dio_put_uint8(struct data_out *dout, int value)
{
  if (enough_space(dout, 1)) {
    uint8_t x = value;

    assert(sizeof(x) == 1);
    memcpy(ADD_TO_POINTER(dout->dest, dout->current), &x, 1);
    dout->current++;
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_put_uint16(struct data_out *dout, int value)
{
  if (enough_space(dout, 2)) {
    uint16_t x = htons(value);

    assert(sizeof(x) == 2);
    memcpy(ADD_TO_POINTER(dout->dest, dout->current), &x, 2);
    dout->current += 2;
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_put_uint32(struct data_out *dout, int value)
{
  if (enough_space(dout, 4)) {
    uint32_t x = htonl(value);

    assert(sizeof(x) == 4);
    memcpy(ADD_TO_POINTER(dout->dest, dout->current), &x, 4);
    dout->current += 4;
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_put_bool8(struct data_out *dout, bool value)
{
  if (value != TRUE && value != FALSE) {
    freelog(LOG_ERROR, "Trying to put a non-boolean: %d", (int) value);
    value = FALSE;
  }

  dio_put_uint8(dout, value ? 1 : 0);
}

/**************************************************************************
...
**************************************************************************/
void dio_put_bool32(struct data_out *dout, bool value)
{
  if (value != TRUE && value != FALSE) {
    freelog(LOG_ERROR, "Trying to put a non-boolean: %d", (int) value);
    value = FALSE;
  }

  dio_put_uint32(dout, value ? 1 : 0);
}

/**************************************************************************
...
**************************************************************************/
void dio_put_uint8_vec8(struct data_out *dout, int *values, int stop_value)
{
  size_t count;

  for (count = 0; values[count] != stop_value; count++) {
    /* nothing */
  }

  if (enough_space(dout, 1 + count)) {
    size_t i;

    dio_put_uint8(dout, count);

    for (i = 0; i < count; i++) {
      dio_put_uint8(dout, values[i]);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_put_uint16_vec8(struct data_out *dout, int *values, int stop_value)
{
  size_t count;

  for (count = 0; values[count] != stop_value; count++) {
    /* nothing */
  }

  if (enough_space(dout, 1 + 2 * count)) {
    size_t i;

    dio_put_uint8(dout, count);

    for (i = 0; i < count; i++) {
      dio_put_uint16(dout, values[i]);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_put_memory(struct data_out *dout, const void *value, size_t size)
{
  if (enough_space(dout, size)) {
    memcpy(ADD_TO_POINTER(dout->dest, dout->current), value, size);
    dout->current += size;
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_put_string(struct data_out *dout, const char *value)
{
  if (put_conv_callback) {
    size_t length;
    char *buffer;

    if ((buffer = (*put_conv_callback) (value, &length))) {
      dio_put_memory(dout, buffer, length + 1);
      free(buffer);
    }
  } else {
    dio_put_memory(dout, value, strlen(value) + 1);
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_put_bit_string(struct data_out *dout, const char *value)
{
  /* Note that size_t is often an unsigned type, so we must be careful
   * with the math when calculating 'bytes'. */
  size_t bits = strlen(value), bytes;
  size_t max = (unsigned short)(-1);

  if (bits > max) {
    freelog(LOG_ERROR, "Bit string too long: %lu bits.", (unsigned long)bits);
    assert(FALSE);
    bits = max;
  }
  bytes = (bits + 7) / 8;

  if (enough_space(dout, bytes + 1)) {
    size_t i;

    dio_put_uint16(dout, bits);

    for (i = 0; i < bits;) {
      int bit, data = 0;

      for (bit = 0; bit < 8 && i < bits; bit++, i++) {
	if (value[i] == '1') {
	  data |= (1 << bit);
	}
      }
      dio_put_uint8(dout, data);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_put_tech_list(struct data_out *dout, const int *value)
{
  int i;

  for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
    dio_put_uint8(dout, value[i]);
    if (value[i] == A_LAST) {
      break;
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_put_worklist(struct data_out *dout, const struct worklist *pwl)
{
  dio_put_bool8(dout, pwl->is_valid);

  if (pwl->is_valid) {
    int i, length = worklist_length(pwl);

    dio_put_uint8(dout, length);
    for (i = 0; i < length; i++) {
      const struct universal *pcp = &(pwl->entries[i]);

      dio_put_uint8(dout, pcp->kind);
      dio_put_uint8(dout, universal_number(pcp));
    }
  }
}

/**************************************************************************
 Receive uint8 value to dest. In case of failure, value stored to dest
 will be zero. Note that zero is legal value even when there is no failure.
**************************************************************************/
void dio_get_uint8(struct data_in *din, int *dest)
{
  if (enough_data(din, 1)) {
    if (dest) {
      uint8_t x;

      assert(sizeof(x) == 1);
      memcpy(&x, ADD_TO_POINTER(din->src, din->current), 1);
      *dest = x;
    }
    din->current++;
  } else if (dest) {
    *dest = 0;
  }
}

/**************************************************************************
 Receive uint16 value to dest. In case of failure, value stored to dest
 will be zero. Note that zero is legal value even when there is no failure.
**************************************************************************/
void dio_get_uint16(struct data_in *din, int *dest)
{
  if (enough_data(din, 2)) {
    if (dest) {
      uint16_t x;

      assert(sizeof(x) == 2);
      memcpy(&x, ADD_TO_POINTER(din->src, din->current), 2);
      *dest = ntohs(x);
    }
    din->current += 2;
  } else if (dest) {
    *dest = 0;
  }
}

/**************************************************************************
 Receive uint32 value to dest. In case of failure, value stored to dest
 will be zero. Note that zero is legal value even when there is no failure.
**************************************************************************/
void dio_get_uint32(struct data_in *din, int *dest)
{
  if (enough_data(din, 4)) {
    if (dest) {
      uint32_t x;

      assert(sizeof(x) == 4);
      memcpy(&x, ADD_TO_POINTER(din->src, din->current), 4);
      *dest = ntohl(x);
    }
    din->current += 4;
  } else if (dest) {
    *dest = 0;
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_get_bool8(struct data_in *din, bool * dest)
{
  int ival;

  dio_get_uint8(din, &ival);

  if (ival != 0 && ival != 1) {
    freelog(LOG_ERROR, "Received value isn't boolean: %d", ival);
    ival = 1;
  }

  *dest = (ival != 0);
}

/**************************************************************************
...
**************************************************************************/
void dio_get_bool32(struct data_in *din, bool * dest)
{
  int ival = 0;

  dio_get_uint32(din, &ival);

  if (ival != 0 && ival != 1) {
    freelog(LOG_ERROR, "Received value isn't boolean: %d", ival);
    ival = 1;
  }

  *dest = (ival != 0);
}

/**************************************************************************
...
**************************************************************************/
void dio_get_sint8(struct data_in *din, int *dest)
{
  int tmp;

  dio_get_uint8(din, &tmp);
  if (dest) {
    if (tmp > 0x7f) {
      tmp -= 0x100;
    }
    *dest = tmp;
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_get_sint16(struct data_in *din, int *dest)
{
  int tmp = 0;

  dio_get_uint16(din, &tmp);
  if (dest) {
    if (tmp > 0x7fff) {
      tmp -= 0x10000;
    }
    *dest = tmp;
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_get_memory(struct data_in *din, void *dest, size_t dest_size)
{
  if (enough_data(din, dest_size)) {
    if (dest) {
      memcpy(dest, ADD_TO_POINTER(din->src, din->current), dest_size);
    }
    din->current += dest_size;
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_get_string(struct data_in *din, char *dest, size_t max_dest_size)
{
  char *c;
  size_t ps_len;		/* length in packet, not including null */
  size_t offset, remaining;

  assert(max_dest_size > 0 || dest == NULL);

  if (!enough_data(din, 1)) {
    dest[0] = '\0';
    return;
  }

  remaining = dio_input_remaining(din);
  c = ADD_TO_POINTER(din->src, din->current);

  /* avoid using strlen (or strcpy) on an (unsigned char*)  --dwp */
  for (offset = 0; offset < remaining && c[offset] != '\0'; offset++) {
    /* nothing */
  }

  if (offset >= remaining) {
    ps_len = remaining;
    din->too_short = TRUE;
    din->bad_string = TRUE;
  } else {
    ps_len = offset;
  }

  if (dest && !(*get_conv_callback) (dest, max_dest_size, c, ps_len)) {
    din->bad_string = TRUE;
  }

  if (!din->too_short) {
    din->current += (ps_len + 1);	/* past terminator */
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_get_bit_string(struct data_in *din, char *dest,
			size_t max_dest_size)
{
  int npack = 0;		/* number claimed in packet */
  int i;			/* iterate the bytes */

  assert(dest != NULL && max_dest_size > 0);

  if (!enough_data(din, 1)) {
    dest[0] = '\0';
    return;
  }

  dio_get_uint16(din, &npack);
  if (npack >= max_dest_size) {
      freelog(LOG_ERROR, "Have size for %lu, got %d",
              (unsigned long)max_dest_size, npack);
    din->bad_bit_string = TRUE;
    dest[0] = '\0';
    return;
  }

  for (i = 0; i < npack;) {
    int bit, byte_value;

    dio_get_uint8(din, &byte_value);
    for (bit = 0; bit < 8 && i < npack; bit++, i++) {
      if (TEST_BIT(byte_value, bit)) {
	dest[i] = '1';
      } else {
	dest[i] = '0';
      }
    }
  }

  dest[npack] = '\0';

  if (din->too_short) {
    din->bad_bit_string = TRUE;
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_get_tech_list(struct data_in *din, int *dest)
{
  int i;

  for (i = 0; i < MAX_NUM_TECH_LIST; i++) {
    dio_get_uint8(din, &dest[i]);
    if (dest[i] == A_LAST) {
      break;
    }
  }

  for (; i < MAX_NUM_TECH_LIST; i++) {
    dest[i] = A_LAST;
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_get_worklist(struct data_in *din, struct worklist *pwl)
{
  dio_get_bool8(din, &pwl->is_valid);

  if (pwl->is_valid) {
    int i, length;

    worklist_init(pwl);

    dio_get_uint8(din, &length);
    for (i = 0; i < length; i++) {
      struct universal prod;
      int identifier;
      int kind;

      dio_get_uint8(din, &kind);
      dio_get_uint8(din, &identifier);

      prod = universal_by_number(kind, identifier);
      worklist_append(pwl, prod);
    }
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_get_uint8_vec8(struct data_in *din, int **values, int stop_value)
{
  int count, inx;

  dio_get_uint8(din, &count);
  if (values) {
    *values = fc_calloc((count + 1), sizeof(**values));
  }
  for (inx = 0; inx < count; inx++) {
    dio_get_uint8(din, values ? &((*values)[inx]) : NULL);
  }
  if (values) {
    (*values)[inx] = stop_value;
  }
}

/**************************************************************************
 Receive vector of uint6 values.
**************************************************************************/
void dio_get_uint16_vec8(struct data_in *din, int **values, int stop_value)
{
  int count, inx;

  dio_get_uint8(din, &count);
  if (values) {
    *values = fc_calloc((count + 1), sizeof(**values));
  }
  for (inx = 0; inx < count; inx++) {
    dio_get_uint16(din, values ? &((*values)[inx]) : NULL);
  }
  if (values) {
    (*values)[inx] = stop_value;
  }
}

/**************************************************************************
  De-serialize a player diplomatic state.
**************************************************************************/
void dio_get_diplstate(struct data_in *din, struct player_diplstate *pds)
{
  int value = 0;

  /* backward compatible order defined for this transaction */
  dio_get_uint8(din, &value);
  pds->type = value;
  dio_get_uint16(din, &pds->turns_left);
  dio_get_uint16(din, &pds->contact_turns_left);
  dio_get_uint8(din, &pds->has_reason_to_cancel);
  dio_get_uint16(din, &pds->first_contact_turn);
  value = 0;
  dio_get_uint8(din, &value);
  pds->max_state = value;
}

/**************************************************************************
  Serialize a player diplomatic state.
**************************************************************************/
void dio_put_diplstate(struct data_out *dout,
		       const struct player_diplstate *pds)
{
  /* backward compatible order defined for this transaction */
  dio_put_uint8(dout, pds->type);
  dio_put_uint16(dout, pds->turns_left);
  dio_put_uint16(dout, pds->contact_turns_left);
  dio_put_uint8(dout, pds->has_reason_to_cancel);
  dio_put_uint16(dout, pds->first_contact_turn);
  dio_put_uint8(dout, pds->max_state);
}

/**************************************************************************
  De-serialize a requirement.
**************************************************************************/
void dio_get_requirement(struct data_in *din, struct requirement *preq)
{
  int type, range, value;
  bool survives, negated;

  dio_get_uint8(din, &type);
  dio_get_sint32(din, &value);
  dio_get_uint8(din, &range);
  dio_get_bool8(din, &survives);
  dio_get_bool8(din, &negated);

  *preq = req_from_values(type, range, survives, negated, value);
}

/**************************************************************************
  Serialize a requirement.
**************************************************************************/
void dio_put_requirement(struct data_out *dout, const struct requirement *preq)
{
  int type, range, value;
  bool survives, negated;

  req_get_values(preq, &type, &range, &survives, &negated, &value);

  dio_put_uint8(dout, type);
  dio_put_sint32(dout, value);
  dio_put_uint8(dout, range);
  dio_put_bool8(dout, survives);
  dio_put_bool8(dout, negated);
}

