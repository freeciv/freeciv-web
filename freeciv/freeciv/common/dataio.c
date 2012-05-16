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
void dio_put_uint8(struct data_out *dout, char *key, int value)
{
  json_object_set_new(dout->json, key, json_integer(value));
}

void dio_put_uint8_old(struct data_out *dout, int value)
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
void dio_put_uint16(struct data_out *dout, char *key, int value)
{
  json_object_set_new(dout->json, key, json_integer(value));
}

/**************************************************************************
...
**************************************************************************/
void dio_put_uint32(struct data_out *dout, char *key, int value)
{
  json_object_set_new(dout->json, key, json_integer(value));
}

/**************************************************************************
...
**************************************************************************/
void dio_put_bool8(struct data_out *dout, char *key, bool value)
{
  json_object_set_new(dout->json, key, value ? json_true() : json_false());
}

/**************************************************************************
...
**************************************************************************/
void dio_put_bool32(struct data_out *dout, char *key, bool value)
{
  json_object_set_new(dout->json, key, value ? json_true() : json_false());
}

/**************************************************************************
...
**************************************************************************/
void dio_put_uint8_vec8(struct data_out *dout, char *key, int *values, int stop_value)
{
 /* TODO: implement. */
}

/**************************************************************************
...
**************************************************************************/
void dio_put_uint16_vec8(struct data_out *dout, char *key, int *values, int stop_value)
{
  /* TODO: implement */

}

/**************************************************************************
...
**************************************************************************/
void dio_put_memory(struct data_out *dout, char *key, const void *value, size_t size)
{
  /* TODO: implement */
}

/**************************************************************************
...
**************************************************************************/
void dio_put_memory_old(struct data_out *dout, const void *value, size_t size)
{
  if (enough_space(dout, size)) {
    memcpy(ADD_TO_POINTER(dout->dest, dout->current), value, size);
    dout->current += size;
  }
}


/**************************************************************************
...
**************************************************************************/
void dio_put_string(struct data_out *dout, char *key, const char *value)
{
  json_object_set_new(dout->json, key, json_string(value));
}

/**************************************************************************
...
**************************************************************************/
void dio_put_string_array(struct data_out *dout, char *key, 
		          const char *value, int size)
{
  int i;

  json_t *array = json_array();
  for (i = 0; i < size; i++) {
    if (value != NULL) {
      json_array_append_new(array, json_string(value + i));
    }
  }
  
  json_object_set_new(dout->json, key, array);
}


void dio_put_string_old(struct data_out *dout, const char *value)
 {
  if (put_conv_callback) {
    size_t length;
    char *buffer;

    if ((buffer = (*put_conv_callback) (value, &length))) {
      dio_put_memory_old(dout, buffer, length + 1);
      free(buffer);
    }
  } else {
    dio_put_memory_old(dout, value, strlen(value) + 1);
  }
}

void dio_put_uint16_old(struct data_out *dout, int value)
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
void dio_put_bit_string(struct data_out *dout, char *key, const char *value)
{
  /* TODO: implement */

}

/**************************************************************************
...
**************************************************************************/
void dio_put_tech_list(struct data_out *dout, char *key, const int *value)
{
  /* TODO: implement */
}

/**************************************************************************
...
**************************************************************************/
void dio_put_worklist(struct data_out *dout, char *key, const struct worklist *pwl)
{
  /* TODO: implement */
}

void dio_put_array_uint8(struct data_out *dout, char *key, int *values, int size)
{
  int i;
  json_t *array = json_array();
  for (i = 0; i < size; i++) {
    json_array_append_new(array, json_integer(values[i]));
  }
  
  json_object_set_new(dout->json, key, array);
}

void dio_put_array_uint32(struct data_out *dout, char *key, int *values, int size)
{
  int i;
  json_t *array = json_array();
  for (i = 0; i < size; i++) {
    json_array_append_new(array, json_integer(values[i]));
  }
  
  json_object_set_new(dout->json, key, array);

}

void dio_put_array_sint8(struct data_out *dout, char *key, int *values, int size)
{
  int i;
  json_t *array = json_array();
  for (i = 0; i < size; i++) {
    json_array_append_new(array, json_integer(values[i]));
  }
  
  json_object_set_new(dout->json, key, array);

}

void dio_put_array_sint16(struct data_out *dout, char *key, int *values, int size)
{
  int i;
  json_t *array = json_array();
  for (i = 0; i < size; i++) {
    json_array_append_new(array, json_integer(values[i]));
  }
  
  json_object_set_new(dout->json, key, array);

}

void dio_put_array_sint32(struct data_out *dout, char *key, int *values, int size)
{
  int i;
  json_t *array = json_array();
  for (i = 0; i < size; i++) {
    json_array_append_new(array, json_integer(values[i]));
  }
  
  json_object_set_new(dout->json, key, array);

}

void dio_put_array_bool8(struct data_out *dout, char *key, bool *values, int size)
{
  int i;
  json_t *array = json_array();
  for (i = 0; i < size; i++) {
    json_array_append_new(array, values[i] ? json_true() : json_false());
  }
  
  json_object_set_new(dout->json, key, array);

}


/**************************************************************************
 Receive uint8 value to dest. In case of failure, value stored to dest
 will be zero. Note that zero is legal value even when there is no failure.
**************************************************************************/
void dio_get_uint8(json_t *json_packet, char *key, int *dest)
{
  json_t *pint = json_object_get(json_packet, key);

  if (!pint) {
    freelog(LOG_ERROR, "ERROR: Unable to get uint8 with key: %s", key);
    return;
  } 
  *dest = json_integer_value(pint);

  if (!dest) {
    freelog(LOG_ERROR, "ERROR: Unable to get unit8 with key: %s", key);
  }

}

/**************************************************************************
 Receive uint8 value to dest. In case of failure, value stored to dest
 will be zero. Note that zero is legal value even when there is no failure.
**************************************************************************/
void dio_get_uint8_old(struct data_in *din, int *dest)
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
void dio_get_uint16_old(struct data_in *din, int *dest)
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
 Receive uint16 value to dest. In case of failure, value stored to dest
 will be zero. Note that zero is legal value even when there is no failure.
**************************************************************************/
void dio_get_uint16(json_t *json_packet, char *key, int *dest)
{
  json_t *pint = json_object_get(json_packet, key);

  if (!pint) {
    freelog(LOG_ERROR, "ERROR: Unable to get uint16 with key: %s", key);
    return;
  } 
  *dest = json_integer_value(pint);

  if (!dest) {
    freelog(LOG_ERROR, "ERROR: Unable to get unit16 with key: %s", key);
  }

}

/**************************************************************************
 Receive uint32 value to dest. In case of failure, value stored to dest
 will be zero. Note that zero is legal value even when there is no failure.
**************************************************************************/
void dio_get_uint32(json_t *json_packet, char *key, int *dest)
{

  json_t *pint = json_object_get(json_packet, key);

  if (!pint) {
    freelog(LOG_ERROR, "ERROR: Unable to get uint32 with key: %s", key);
    return;
  } 
  *dest = json_integer_value(pint);

  if (!dest) {
    freelog(LOG_ERROR, "ERROR: Unable to get unit32 with key: %s", key);
  }

}

/**************************************************************************
...
**************************************************************************/
void dio_get_bool8(json_t *json_packet, char *key, bool * dest)
{
  json_t *pbool = json_object_get(json_packet, key);

  if (!pbool) {
    freelog(LOG_ERROR, "ERROR: Unable to get bool8 with key: %s", key);
    return;
  } 
  *dest = json_is_true(pbool);

  if (!dest) {
    freelog(LOG_ERROR, "ERROR: Unable to get bool with key: %s", key);
  }

}

/**************************************************************************
...
**************************************************************************/
void dio_get_bool32(json_t *json_packet, char *key, bool * dest)
{
 json_t *pbool = json_object_get(json_packet, key);

  if (!pbool) {
    freelog(LOG_ERROR, "ERROR: Unable to get bool32 with key: %s", key);
    return;
  } 
  *dest = json_is_true(pbool);

  if (!dest) {
    freelog(LOG_ERROR, "ERROR: Unable to get bool32 with key: %s", key);
  }

}

/**************************************************************************
...
**************************************************************************/
void dio_get_sint8(json_t *json_packet, char *key, int *dest)
{
  json_t *pint = json_object_get(json_packet, key);

  if (!pint) {
    freelog(LOG_ERROR, "ERROR: Unable to get sint8 with key: %s", key);
    return;
  } 
  *dest = json_integer_value(pint);

  if (!dest) {
    freelog(LOG_ERROR, "ERROR: Unable to get sint8 with key: %s", key);
  }
}

/**************************************************************************
...
**************************************************************************/
void dio_get_sint16(json_t *json_packet, char *key, int *dest)
{
  json_t *pint = json_object_get(json_packet, key);

  if (!pint) {
    freelog(LOG_ERROR, "ERROR: Unable to get sint16 with key: %s", key);
    return;
  } 
  *dest = json_integer_value(pint);

  if (!dest) {
    freelog(LOG_ERROR, "ERROR: Unable to get sint16 with key: %s", key);
  }

}

/**************************************************************************
...
**************************************************************************/
void dio_get_memory(json_t *json_packet, char *key, void *dest, size_t dest_size)
{
  /* TODO: implement */ 
}

/**************************************************************************
...
**************************************************************************/
void dio_get_string(json_t *json_packet, char *key, char *dest, size_t max_dest_size)
{

  json_t *pstring = json_object_get(json_packet, key);

  if (!pstring) {
    freelog(LOG_ERROR, "ERROR: Unable to get string with key: %s", key);
    return;
  } 
  const char *result_str = json_string_value(pstring);

  if (dest && !(*get_conv_callback) (dest, max_dest_size, result_str, strlen(result_str))) {
    freelog(LOG_ERROR, "ERROR: Unable to get string with key: %s", key);
  }

}

/**************************************************************************
...
**************************************************************************/
void dio_get_string_old(struct data_in *din, char *dest, size_t max_dest_size)
{
  char *c;
  size_t ps_len;               /* length in packet, not including null */
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
    din->current += (ps_len + 1);      /* past terminator */
  }
}


/**************************************************************************
...
**************************************************************************/
void dio_get_bit_string(json_t *json_packet, char *key, char *dest,
			size_t max_dest_size)
{
  /* TODO: implement */
}

/**************************************************************************
...
**************************************************************************/
void dio_get_tech_list(json_t *json_packet, char *key, int *dest)
{
 /* TODO: implement*/ 

}

/**************************************************************************
...
**************************************************************************/
void dio_get_worklist(json_t *json_packet, char *key, struct worklist *pwl)
{
  /* TODO: implement */
}

/**************************************************************************
...
**************************************************************************/
void dio_get_uint8_vec8(json_t *json_packet, char *key, int **values, int stop_value)
{
  /* TODO: implement */
}

/**************************************************************************
 Receive vector of uint6 values.
**************************************************************************/
void dio_get_uint16_vec8(json_t *json_packet, char *key, int **values, int stop_value)
{
 /* TODO: implement */
}

/**************************************************************************
  De-serialize a player diplomatic state.
**************************************************************************/
void dio_get_diplstate(json_t *json_packet, char *key, struct player_diplstate *pds)
{
  /* TODO: implement */
}

/**************************************************************************
  Serialize a player diplomatic state.
**************************************************************************/
void dio_put_diplstate(struct data_out *dout, char *key,
		       const struct player_diplstate *pds, int size)
{
  int i;

  json_t *array = json_array();
  for (i = 0; i < size; i++) {
    json_array_append_new(array, json_integer(pds[i].type));
  }
  
  json_object_set_new(dout->json, key, array);
}

/**************************************************************************
  De-serialize a requirement.
**************************************************************************/
void dio_get_requirement(json_t *json_packet, char *key, struct requirement *preq)
{
  /* TODO: implement */
}

/**************************************************************************
  Serialize a requirement.
**************************************************************************/
void dio_put_requirement(struct data_out *dout, char *key, const struct requirement *preq, int size)
{
  /* TODO: implement */
}

