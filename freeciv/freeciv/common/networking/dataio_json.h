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
#ifndef FC__DATAIO_JSON_H
#define FC__DATAIO_JSON_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <jansson.h>

/* utility */
#include "bitvector.h"
#include "support.h"            /* bool type */

/* common/networking */
#include "connection.h"

struct worklist;
struct requirement;

struct json_data_out {
  struct raw_data_out raw;
  json_t *json;
};

/* gets */
bool dio_get_type_json(struct data_in *din, enum data_type type, int *dest)
    fc__attribute((nonnull (3)));

bool dio_get_uint8_json(struct connection *pc, struct data_in *din,
                        const struct plocation *location, int *dest);
bool dio_get_uint16_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location, int *dest);
bool dio_get_uint32_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location, int *dest);

bool dio_get_sint8_json(struct connection *pc, struct data_in *din,
                        const struct plocation *location, int *dest);
bool dio_get_sint16_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location, int *dest);
bool dio_get_sint32_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location, int *dest);

bool dio_get_bool8_json(struct connection *pc, struct data_in *din,
                        const struct plocation *location, bool *dest);
bool dio_get_bool32_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location, bool *dest);
bool dio_get_ufloat_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location,
                         float *dest, int float_factor);
bool dio_get_sfloat_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location,
                         float *dest, int float_factor);
bool dio_get_memory_json(struct connection *pc, struct data_in *din,
                         struct plocation *location,
                         void *dest, size_t dest_size);
bool dio_get_string_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location,
                         char *dest, size_t max_dest_size);
bool dio_get_estring_json(struct connection *pc, struct data_in *din,
                          const struct plocation *location,
                          char *dest, size_t max_dest_size);
bool dio_get_worklist_json(struct connection *pc, struct data_in *din,
                           struct plocation *location,
                           struct worklist *pwl);
bool dio_get_requirement_json(struct connection *pc, struct data_in *din,
                              const struct plocation *location,
                              struct requirement *preq);
bool dio_get_action_probability_json(struct connection *pc, struct data_in *din,
                                     const struct plocation *location,
                                     struct act_prob *prob);

bool dio_get_uint8_vec8_json(struct connection *pc, struct data_in *din,
                             const struct plocation *location,
                             int **values, int stop_value);
bool dio_get_uint16_vec8_json(struct connection *pc, struct data_in *din,
                              const struct plocation *location,
                              int **values, int stop_value);

/* Should be a function but we need some macro magic. */
#define DIO_BV_GET(pdin, location, bv) \
  dio_get_memory_json(pc, pdin, location,                \
                      (bv).vec, sizeof((bv).vec))

#define DIO_GET(f, d, l, ...) \
  dio_get_##f##_json(pc, d, l, ## __VA_ARGS__)

/* puts */
void dio_put_farray_json(struct json_data_out *dout,
                         const struct plocation *location, int size);

void dio_put_type_json(struct json_data_out *dout, enum data_type type,
                       const struct plocation *location,
                       int value);

void dio_put_uint8_json(struct json_data_out *dout,
                        const struct plocation *location, int value);
void dio_put_sint8_json(struct json_data_out *dout,
                        const struct plocation *location, int value);
void dio_put_uint16_json(struct json_data_out *dout,
                         const struct plocation *location, int value);
void dio_put_sint16_json(struct json_data_out *dout,
                         const struct plocation *location, int value);
void dio_put_uint32_json(struct json_data_out *dout,
                         const struct plocation *location, int value);
void dio_put_sint32_json(struct json_data_out *dout,
                         const struct plocation *location, int value);
void dio_put_bool8_json(struct json_data_out *dout,
                        const struct plocation *location, bool value);
void dio_put_bool32_json(struct json_data_out *dout,
                         const struct plocation *location, bool value);
void dio_put_ufloat_json(struct json_data_out *dout,
                         const struct plocation *location,
                         float value, int float_factor);
void dio_put_sfloat_json(struct json_data_out *dout,
                         const struct plocation *location,
                         float value, int float_factor);

void dio_put_memory_json(struct json_data_out *dout,
                         struct plocation *location,
                         const void *value, size_t size);
void dio_put_string_json(struct json_data_out *dout,
                         const struct plocation *location,
                         const char *value);
void dio_put_estring_json(struct json_data_out *dout,
                          const struct plocation *location,
                          const char *value);
void dio_put_city_map_json(struct json_data_out *dout,
                           const struct plocation *location,
                           const char *value);
void dio_put_worklist_json(struct json_data_out *dout,
                           struct plocation *location,
                           const struct worklist *pwl);
void dio_put_requirement_json(struct json_data_out *dout,
                              const struct plocation *location,
                              const struct requirement *preq);
void dio_put_action_probability_json(struct json_data_out *dout,
                                     const struct plocation *location,
                                     const struct act_prob *prob);

void dio_put_uint8_vec8_json(struct json_data_out *dout,
                             const struct plocation *location,
                             int *values, int stop_value);
void dio_put_uint16_vec8_json(struct json_data_out *dout,
                              const struct plocation *location,
                              int *values, int stop_value);

/* Should be a function but we need some macro magic. */
#define DIO_BV_PUT(pdout, location, bv) \
  dio_put_memory_json((pdout), location, (bv).vec, sizeof((bv).vec))

#define DIO_PUT(f, d, l, ...) \
  dio_put_##f##_json(d, l, ## __VA_ARGS__)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__DATAIO_JSON_H */
