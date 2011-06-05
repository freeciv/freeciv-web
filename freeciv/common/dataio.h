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
#ifndef FC__DATAIO_H
#define FC__DATAIO_H

#include "shared.h"		/* bool type */
#include <jansson.h>

struct worklist;
struct player_diplstate;
struct requirement;

struct data_in {
  const void *src;
  size_t src_size, current;
  bool too_short;		/* set to 1 if try to read past end */
  bool bad_string;		/* set to 1 if received too-long string */
  bool bad_bit_string;		/* set to 1 if received bad bit-string */
};

struct data_out {
  void *dest;
  json_t *json;
  size_t dest_size, used, current;
  bool too_short;		/* set to 1 if try to read past end */
};

/* network string conversion */
typedef char *(*DIO_PUT_CONV_FUN) (const char *src, size_t *length);
void dio_set_put_conv_callback(DIO_PUT_CONV_FUN fun);

typedef bool(*DIO_GET_CONV_FUN) (char *dst, size_t ndst,
				 const char *src, size_t nsrc);
void dio_set_get_conv_callback(DIO_GET_CONV_FUN fun);

/* General functions */
void dio_output_init(struct data_out *dout, void *destination,
		     size_t dest_size);
void dio_output_rewind(struct data_out *dout);
size_t dio_output_used(struct data_out *dout);

void dio_input_init(struct data_in *dout, const void *src, size_t src_size);
void dio_input_rewind(struct data_in *din);
size_t dio_input_remaining(struct data_in *din);

/* gets */

void dio_get_uint8(json_t *json_packet, char *key, int *dest);
void dio_get_uint16(json_t *json_packet, char *key, int *dest);
void dio_get_uint32(json_t *json_packet, char *key, int *dest);
void dio_get_uint16_old(struct data_in *din, int *dest);
void dio_get_uint8_old(struct data_in *din, int *dest);

void dio_get_sint8(json_t *json_packet, char *key, int *dest);
void dio_get_sint16(json_t *json_packet, char *key, int *dest);
#define dio_get_sint32(d,v,x) dio_get_uint32(d,v,x)


void dio_get_bool8(json_t *json_packet, char *key, bool *dest);
void dio_get_bool32(json_t *json_packet, char *key, bool *dest);
void dio_get_memory(json_t *json_packet, char *key, void *dest, size_t dest_size);
void dio_get_string(json_t *json_packet, char *key, char *dest, size_t max_dest_size);
void dio_get_string_old(struct data_in *din, char *dest, size_t max_dest_size);
void dio_get_bit_string(json_t *json_packet, char *key, char *dest,
			size_t max_dest_size);
void dio_get_tech_list(json_t *json_packet, char *key, int *dest);
void dio_get_worklist(json_t *json_packet, char *key, struct worklist *pwl);
void dio_get_diplstate(json_t *json_packet, char *key, struct player_diplstate *pds);
void dio_get_requirement(json_t *json_packet, char *key, struct requirement *preq);

void dio_get_uint8_vec8(json_t *json_packet, char *key, int **values, int stop_value);
void dio_get_uint16_vec8(json_t *json_packet, char *key, int **values, int stop_value);

/* Should be a function but we need some macro magic. */
#define DIO_BV_GET(pdin, bv) \
  dio_get_memory((pdin), (bv).vec, sizeof((bv).vec))

/* puts */
void dio_put_uint8(struct data_out *dout, char *key, int value);
void dio_put_uint8_old(struct data_out *dout, int value);
void dio_put_uint16(struct data_out *dout, char *key, int value);
void dio_put_uint32(struct data_out *dout, char *key, int value);
void dio_put_uint16_old(struct data_out *dout, int value);

void dio_put_array_uint8(struct data_out *dout, char *key, int *values, int size);
void dio_put_array_uint32(struct data_out *dout, char *key, int *values, int size);
void dio_put_array_sint8(struct data_out *dout, char *key, int *values, int size);
void dio_put_array_sint16(struct data_out *dout, char *key, int *values, int size);
void dio_put_array_sint32(struct data_out *dout, char *key, int *values, int size);
void dio_put_array_bool8(struct data_out *dout, char *key, bool *values, int size);

#define dio_put_sint8(d,k,v) dio_put_uint8(d,k,v)
#define dio_put_sint16(d,k,v) dio_put_uint16(d,k,v)
#define dio_put_sint32(d,k,v) dio_put_uint32(d,k,v)

void dio_put_bool8(struct data_out *dout, char *key, bool value);
void dio_put_bool32(struct data_out *dout, char *key, bool value);

void dio_put_memory(struct data_out *dout, char *key, const void *value, size_t size);
void dio_put_string(struct data_out *dout, char *key, const char *value);
void dio_put_bit_string(struct data_out *dout, char *key, const char *value);
void dio_put_city_map(struct data_out *dout, char *key, const char *value);
void dio_put_tech_list(struct data_out *dout, char *key, const int *value);
void dio_put_worklist(struct data_out *dout, char *key, const struct worklist *pwl);
void dio_put_diplstate(struct data_out *dout, char *key,
		       const struct player_diplstate *pds, int size);
void dio_put_requirement(struct data_out *dout, char *key, const struct requirement *preq, int size);

void dio_put_uint8_vec8(struct data_out *dout, char *key, int *values, int stop_value);
void dio_put_uint16_vec8(struct data_out *dout, char *key, int *values, int stop_value);
void dio_put_string_old(struct data_out *dout, const char *value);
void dio_put_memory_old(struct data_out *dout, const void *value, size_t size);
void dio_put_string_array(struct data_out *dout, char *key, const char *value, int size);


/* Should be a function but we need some macro magic. */
#define DIO_BV_PUT(pdout, type, bv) \
  dio_put_memory((pdout), type, (bv).vec, sizeof((bv).vec))

#endif  /* FC__PACKETS_H */
