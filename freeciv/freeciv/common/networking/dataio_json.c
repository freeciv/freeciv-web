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

#include <curl/curl.h>

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef FREECIV_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

/* utility */
#include "bitvector.h"
#include "capability.h"
#include "log.h"
#include "mem.h"
#include "support.h"

/* common */
#include "events.h"
#include "player.h"
#include "requirements.h"
#include "tech.h"
#include "worklist.h"

#include "dataio.h"

static bool dio_get_bool8_json_internal(json_t *json_packet,
                                        const struct plocation *location,
                                        bool *dest);

/**********************************************************************//**
  Returns a CURL easy handle for name encoding and decoding
**************************************************************************/
static CURL *get_curl(void)
{
  static CURL *curl_easy_handle = NULL;

  if (curl_easy_handle == NULL) {
    curl_easy_handle = curl_easy_init();
  } else {
    /* Reuse the existing CURL easy handle */
    curl_easy_reset(curl_easy_handle);
  }

  return curl_easy_handle;
}

static void plocation_write_data(json_t *item,
                                 const struct plocation *location,
                                 json_t *data);

/**********************************************************************//**
  Helper for plocation_write_data(). Use it in stead of this.
**************************************************************************/
static void plocation_write_field(json_t *item,
                                  const struct plocation *location,
                                  json_t *data)
{
  if (location->sub_location == NULL) {
    json_object_set_new(item, location->name, data);
  } else {
    plocation_write_data(json_object_get(item, location->name),
                         location->sub_location, data);
  }
}

/**********************************************************************//**
  Helper for plocation_write_data(). Use it in stead of this.
**************************************************************************/
static void plocation_write_elem(json_t *item,
                                 const struct plocation *location,
                                 json_t *data)
{
  if (location->sub_location == NULL) {
    json_array_set_new(item, location->number, data);
  } else {
    plocation_write_data(json_array_get(item, location->number),
                         location->sub_location, data);
  }
}

/**********************************************************************//**
  Write the specified JSON data to the given location in the provided
  JSON item.
**************************************************************************/
static void plocation_write_data(json_t *item,
                                 const struct plocation *location,
                                 json_t *data)
{
  switch (location->kind) {
  case PADR_FIELD:
    plocation_write_field(item, location, data);
    return;
  case PADR_ELEMENT:
    plocation_write_elem(item, location, data);
    return;
  default:
    log_error("Unknown packet part location kind.");
    return;
  }
}

static json_t *plocation_read_data(json_t *item,
                                   const struct plocation *location);

/**********************************************************************//**
  Helper for plocation_read_data(). Use it in stead of this.
**************************************************************************/
static json_t *plocation_read_field(json_t *item,
                                    const struct plocation *location)
{
  if (location->sub_location == NULL) {
    return json_object_get(item, location->name);
  } else {
    return plocation_read_data(json_object_get(item, location->name),
                               location->sub_location);
  }
}

/**********************************************************************//**
  Helper for plocation_read_data(). Use it in stead of this.
**************************************************************************/
static json_t *plocation_read_elem(json_t *item,
                                   const struct plocation *location)
{
  if (location->sub_location == NULL) {
    return json_array_get(item, location->number);
  } else {
    return plocation_read_data(json_array_get(item, location->number),
                               location->sub_location);
  }
}

/**********************************************************************//**
  Read JSON data from the given location in the provided JSON item.
**************************************************************************/
static json_t *plocation_read_data(json_t *item,
                                   const struct plocation *location)
{
  switch (location->kind) {
  case PADR_FIELD:
    return plocation_read_field(item, location);
  case PADR_ELEMENT:
    return plocation_read_elem(item, location);
  default:
    log_error("Unknown packet part location kind.");
    return NULL;
  }
}

/**********************************************************************//**
  Insert 8 bit value with json.
**************************************************************************/
void dio_put_uint8_json(struct json_data_out *dout,
                        const struct plocation *location,
                        int value)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, json_integer(value));
  } else {
    dio_put_uint8_raw(&dout->raw, value);
  }
}

/**********************************************************************//**
  Insert 8 bit value with json.
**************************************************************************/
void dio_put_sint8_json(struct json_data_out *dout,
                        const struct plocation *location,
                        int value)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, json_integer(value));
  } else {
    dio_put_sint8_raw(&dout->raw, value);
  }
}

/**********************************************************************//**
  Insert value using 32 bits. May overflow.
**************************************************************************/
void dio_put_uint16_json(struct json_data_out *dout,
                         const struct plocation *location, int value)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, json_integer(value));
  } else {
    dio_put_uint16_raw(&dout->raw, value);
  }
}

/**********************************************************************//**
  Insert value using 32 bits. May overflow.
**************************************************************************/
void dio_put_sint16_json(struct json_data_out *dout,
                         const struct plocation *location, int value)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, json_integer(value));
  } else {
    dio_put_sint16_raw(&dout->raw, value);
  }
}

/**********************************************************************//**
  Insert worklist information.
**************************************************************************/
void dio_put_worklist_json(struct json_data_out *dout,
                           struct plocation *location,
                           const struct worklist *pwl)
{
  if (dout->json) {
    int i;
    const int size = worklist_length(pwl);

    /* Must create the array before instertion. */
    dio_put_farray_json(dout, location, size);

    location->sub_location = plocation_elem_new(0);

    for (i = 0; i < size; i++) {
      const struct universal *pcp = &(pwl->entries[i]);
      json_t *universal = json_object();

      location->sub_location->number = i;

      json_object_set_new(universal, "kind", json_integer(pcp->kind));
      json_object_set_new(universal, "value",
                          json_integer(universal_number(pcp)));

      plocation_write_data(dout->json, location, universal);
    }

    FC_FREE(location->sub_location);
  } else {
    dio_put_worklist_raw(&dout->raw, pwl);
  }
}

/**********************************************************************//**
  Receive uint8 value to dest with json.
**************************************************************************/
static bool dio_get_uint8_json_internal(json_t *json_packet,
                                        const struct plocation *location,
                                        int *dest)
{
  json_t *pint = plocation_read_data(json_packet, location);

  if (!pint) {
    log_error("ERROR: Unable to get uint8 from location: %s", plocation_name(location));
    return FALSE;
  }
  *dest = json_integer_value(pint);

  if (!dest) {
    log_error("ERROR: Unable to get unit8 from location: %s", plocation_name(location));
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Receive uint8 value to dest with json.
**************************************************************************/
bool dio_get_uint8_json(struct connection *pc, struct data_in *din,
                        const struct plocation *location, int *dest)
{
  if (pc->json_mode) {
    return dio_get_uint8_json_internal(pc->json_packet, location, dest);
  } else {
    return dio_get_uint8_raw(din, dest);
  }
}

/**********************************************************************//**
  Receive uint16 value to dest with json.
**************************************************************************/
bool dio_get_uint16_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location, int *dest)
{
  if (pc->json_mode) {
    json_t *pint = plocation_read_data(pc->json_packet, location);

    if (!pint) {
      log_error("ERROR: Unable to get uint16 from location: %s", plocation_name(location));
      return FALSE;
    } 
    *dest = json_integer_value(pint);

    if (!dest) {
      log_error("ERROR: Unable to get unit16 from location: %s", plocation_name(location));
      return FALSE;
    }
  } else {
    return dio_get_uint16_raw(din, dest);
  }

  return TRUE;
}

/**********************************************************************//**
  Receive uint32 value to dest with json.
**************************************************************************/
static bool dio_get_uint32_json_internal(json_t *json_packet,
                                         const struct plocation *location,
                                         int *dest)
{
  json_t *pint = plocation_read_data(json_packet, location);

  if (!pint) {
    log_error("ERROR: Unable to get uint32 from location: %s", plocation_name(location));
    return FALSE;
  }
  *dest = json_integer_value(pint);

  if (!dest) {
    log_error("ERROR: Unable to get unit32 from location: %s", plocation_name(location));
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Receive uint32 value to dest with json.
**************************************************************************/
bool dio_get_uint32_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location, int *dest)
{
  if (pc->json_mode) {
    return dio_get_uint32_json_internal(pc->json_packet, location, dest);
  } else {
    return dio_get_uint32_raw(din, dest);
  }
}

/**********************************************************************//**
  Receive sint32 value to dest with json.
**************************************************************************/
bool dio_get_sint32_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location, int *dest)
{
  if (pc->json_mode) {
    return dio_get_uint32_json_internal(pc->json_packet, location, dest);
  } else {
    return dio_get_sint32_raw(din, dest);
  }
}

/**********************************************************************//**
  Receive worklist information.
**************************************************************************/
bool dio_get_worklist_json(struct connection *pc, struct data_in *din,
                           struct plocation *location,
                           struct worklist *pwl)
{
  if (pc->json_mode) {
    int i, length;

    const json_t *wlist = plocation_read_data(pc->json_packet, location);

    worklist_init(pwl);

    if (!json_is_array(wlist)) {
      log_packet("Not a worklist");
      return FALSE;
    }

    /* Safe. Checked that it was an array above. */
    length = json_array_size(wlist);

    /* A worklist is an array... */
    location->sub_location = plocation_elem_new(0);

    /* ... of universal objects. */
    location->sub_location->sub_location = plocation_field_new("kind");

    for (i = 0; i < length; i++) {
      int value;
      int kind;
      struct universal univ;

      location->sub_location->number = i;

      location->sub_location->sub_location->name = "kind";
      if (!dio_get_uint8_json_internal(pc->json_packet, location, &kind)) {
        log_packet("Corrupt worklist element kind");
        FC_FREE(location->sub_location->sub_location);
        FC_FREE(location->sub_location);
        return FALSE;
      }

      location->sub_location->sub_location->name = "value";
      if (!dio_get_uint8_json_internal(pc->json_packet, location, &value)) {
        log_packet("Corrupt worklist element value");
        FC_FREE(location->sub_location->sub_location);
        FC_FREE(location->sub_location);
        return FALSE;
      }

      /*
       * FIXME: the value returned by universal_by_number() should be checked!
       */
      univ = universal_by_number(kind, value);
      worklist_append(pwl, &univ);
    }

    FC_FREE(location->sub_location->sub_location);
    FC_FREE(location->sub_location);
  } else {
    return dio_get_worklist_raw(din, pwl);
  }

  return TRUE;
}

/**********************************************************************//**
  Receive vector of 8 bit values, terminated by stop_value.
**************************************************************************/
bool dio_get_uint8_vec8_json(struct connection *pc, struct data_in *din,
                             const struct plocation *location,
                             int **values, int stop_value)
{
  if (pc->json_mode) {
    /* TODO: implement */
    log_warn("Received unimplemeted data type uint8_vec8.");
  } else {
    return dio_get_uint8_vec8_raw(din, values, stop_value);
  }

  return TRUE;
}

/**********************************************************************//**
  Receive vector of uint16 values.
**************************************************************************/
bool dio_get_uint16_vec8_json(struct connection *pc, struct data_in *din,
                              const struct plocation *location,
                              int **values,
                              int stop_value)
{
  if (pc->json_mode) {
    /* TODO: implement */
    log_warn("Received unimplemeted data type uint16_vec8.");
  } else {
    return dio_get_uint16_vec8_raw(din, values, stop_value);
  }

  return TRUE;
}

/**********************************************************************//**
  Read a single requirement.
**************************************************************************/
bool dio_get_requirement_json(struct connection *pc, struct data_in *din,
                              const struct plocation *location,
                              struct requirement *preq)
{
  if (pc->json_mode) {
    int kind, range, value;
    bool survives, present, quiet;

    struct plocation *req_field;

    /* Find the requirement object. */
    json_t *requirement = plocation_read_data(pc->json_packet, location);

    if (!requirement) {
      log_error("ERROR: Unable to get requirement from location: %s", plocation_name(location));
      return FALSE;
    }

    /* Find the requirement object fields and translate their values. */
    req_field = plocation_field_new("kind");
    if (!dio_get_uint8_json_internal(requirement, req_field, &kind)) {
      log_error("ERROR: Unable to get part of requirement from location: %s",
                plocation_name(location));
      return FALSE;
    }

    req_field->name = "value";
    if (!dio_get_uint32_json_internal(requirement, req_field, &value)) {
      log_error("ERROR: Unable to get part of requirement from location: %s",
                plocation_name(location));
      return FALSE;
    }

    req_field->name = "range";
    if (!dio_get_uint8_json_internal(requirement, req_field, &range)) {
      log_error("ERROR: Unable to get part of requirement from location: %s",
                plocation_name(location));
      return FALSE;
    }

    req_field->name = "survives";
    if (!dio_get_bool8_json_internal(requirement, req_field, &survives)) {
      log_error("ERROR: Unable to get part of requirement from location: %s",
                plocation_name(location));
      return FALSE;
    }

    req_field->name = "present";
    if (!dio_get_bool8_json_internal(requirement, req_field, &present)) {
      log_error("ERROR: Unable to get part of requirement from location: %s",
                plocation_name(location));
      return FALSE;
    }

    req_field->name = "quiet";
    if (!dio_get_bool8_json_internal(requirement, req_field, &quiet)) {
      log_error("ERROR: Unable to get part of requirement from location: %s",
                plocation_name(location));
      return FALSE;
    }

    FC_FREE(req_field);

    /* Create a requirement with the values sent over the network. */
    *preq = req_from_values(kind, range, survives, present, quiet, value);
  } else {
    return dio_get_requirement_raw(din, preq);
  }

  return TRUE;
}

/**********************************************************************//**
  De-serialize an action probability.
**************************************************************************/
bool dio_get_action_probability_json(struct connection *pc, struct data_in *din,
                                     const struct plocation *location,
                                     struct act_prob *prob)
{
  if (pc->json_mode) {
    struct plocation *ap_field;

    /* Find the action probability object. */
    json_t *action_probability = plocation_read_data(pc->json_packet, location);

    if (!action_probability) {
      log_error("ERROR: Unable to get action probability from location: %s",
                plocation_name(location));
      return FALSE;
    }

    /* Find the action probability object fields and translate their
     * values. */
    ap_field = plocation_field_new("min");
    if (!dio_get_uint8_json_internal(action_probability, ap_field, &prob->min)) {
      log_error("ERROR: Unable to get part of action probability "
                "from location: %s",
                plocation_name(location));
      return FALSE;
    }

    ap_field->name = "max";
    if (!dio_get_uint8_json_internal(action_probability, ap_field, &prob->max)) {
      log_error("ERROR: Unable to get part of action probability "
                "from location: %s",
                plocation_name(location));
      return FALSE;
    }

    FC_FREE(ap_field);
  } else {
    return dio_get_action_probability_raw(din, prob);
  }

  return TRUE;
}

/**********************************************************************//**
  Create an empty field array.
**************************************************************************/
void dio_put_farray_json(struct json_data_out *dout,
                         const struct plocation *location, int size)
{
  if (dout->json) {
    int i;
    json_t *farray = json_array();

    /* Jansson's json_array_set_new() refuses to create array elements so
     * they must be created with the array. */
    for (i = 0; i < size; i++) {
      json_array_append_new(farray, json_null());
    }

    plocation_write_data(dout->json, location, farray);
  } else {
    /* No caller needs this */
  }
}

/**********************************************************************//**
  Insert uint32 value.
**************************************************************************/
void dio_put_uint32_json(struct json_data_out *dout,
                         const struct plocation *location, int value)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, json_integer(value));
  } else {
    dio_put_uint32_raw(&dout->raw, value);
  }
}

/**********************************************************************//**
  Insert sint32 value.
**************************************************************************/
void dio_put_sint32_json(struct json_data_out *dout,
                         const struct plocation *location, int value)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, json_integer(value));
  } else {
    dio_put_sint32_raw(&dout->raw, value);
  }
}

/**********************************************************************//**
  Insert bool value.
**************************************************************************/
void dio_put_bool8_json(struct json_data_out *dout,
                        const struct plocation *location, bool value)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, value ? json_true() : json_false());
  } else {
    dio_put_bool8_raw(&dout->raw, value);
  }
}

/**********************************************************************//**
  Insert bool value.
**************************************************************************/
void dio_put_bool32_json(struct json_data_out *dout,
                         const struct plocation *location, bool value)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, value ? json_true() : json_false());
  } else {
    dio_put_bool32_raw(&dout->raw, value);
  }
}

/**********************************************************************//**
  Insert unsigned floating point value.
**************************************************************************/
void dio_put_ufloat_json(struct json_data_out *dout,
                         const struct plocation *location,
                         float value, int float_factor)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, json_real(value));
  } else {
    dio_put_ufloat_raw(&dout->raw, value, float_factor);
  }
}

/**********************************************************************//**
  Insert signed floating point value.
**************************************************************************/
void dio_put_sfloat_json(struct json_data_out *dout,
                         const struct plocation *location,
                         float value, int float_factor)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, json_real(value));
  } else {
    dio_put_sfloat_raw(&dout->raw, value, float_factor);
  }
}

/**********************************************************************//**
  Insert vector of uint8 values, terminated by stop_value.
**************************************************************************/
void dio_put_uint8_vec8_json(struct json_data_out *dout,
                             const struct plocation *location,
                             int *values, int stop_value)
{
  if (dout->json) {
    /* TODO: implement. */
    log_error("Tried to send unimplemeted data type uint8_vec8.");
  } else {
    dio_put_uint8_vec8_raw(&dout->raw, values, stop_value);
  }
}

/**********************************************************************//**
  Insert vector of uint16 values, terminated by stop_value.
**************************************************************************/
void dio_put_uint16_vec8_json(struct json_data_out *dout,
                              const struct plocation *location, int *values,
                              int stop_value)
{
  if (dout->json) {
    /* TODO: implement. */
    log_error("Tried to send unimplemeted data type uint16_vec8.");
  } else {
    dio_put_uint16_vec8_raw(&dout->raw, values, stop_value);
  }
}

/**********************************************************************//**
  Send block of memory as byte array.
**************************************************************************/
void dio_put_memory_json(struct json_data_out *dout,
                         struct plocation *location,
                         const void *value,
                         size_t size)
{
  if (dout->json) {
    int i;

    dio_put_farray_json(dout, location, size);

    location->sub_location = plocation_elem_new(0);

    for (i = 0; i < size; i++) {
      location->sub_location->number = i;

      dio_put_uint8_json(dout, location,
                         ((unsigned char *)value)[i]);
    }

    FC_FREE(location->sub_location);
  } else {
    dio_put_memory_raw(&dout->raw, value, size);
  }
}

/**********************************************************************//**
  Insert NULL-terminated string.
**************************************************************************/
void dio_put_string_json(struct json_data_out *dout,
                         const struct plocation *location,
                         const char *value)
{
  if (dout->json) {
    plocation_write_data(dout->json, location, json_string(value));
  } else {
    dio_put_string_raw(&dout->raw, value);
  }
}

/**********************************************************************//**
  Encode and write the specified string to the specified location.
**************************************************************************/
void dio_put_estring_json(struct json_data_out *dout,
                          const struct plocation *location,
                          const char *value)
{
  if (dout->json) {
    char *escaped_value;

    /* Let CURL find the length it self by passing 0 */
    escaped_value = curl_easy_escape(get_curl(), value, 0);

    /* Handle as a regular string from now on. */
    dio_put_string_json(dout, location, escaped_value);

    /* CURL's memory management wants to free this it self. */
    curl_free(escaped_value);
  } else {
    dio_put_estring_raw(&dout->raw, value);
  }
}

/**********************************************************************//**
  Insert a single requirement.
**************************************************************************/
void dio_put_requirement_json(struct json_data_out *dout,
                              const struct plocation *location,
                              const struct requirement *preq)
{
  if (dout->json) {
    int kind, range, value;
    bool survives, present, quiet;

    /* Create the requirement object. */
    json_t *requirement = json_object();

    /* Read the requirement values. */
    req_get_values(preq, &kind, &range, &survives, &present, &quiet, &value);

    /* Write the requirement values to the fields of the requirement
     * object. */
    json_object_set_new(requirement, "kind", json_integer(kind));
    json_object_set_new(requirement, "value", json_integer(value));

    json_object_set_new(requirement, "range", json_integer(range));

    json_object_set_new(requirement, "survives", json_boolean(survives));
    json_object_set_new(requirement, "present", json_boolean(present));
    json_object_set_new(requirement, "quiet", json_boolean(quiet));

    /* Put the requirement object in the packet. */
    plocation_write_data(dout->json, location, requirement);
  } else {
    dio_put_requirement_raw(&dout->raw, preq);
  }
}

/**********************************************************************//**
  Serialize an action probability.
**************************************************************************/
void dio_put_action_probability_json(struct json_data_out *dout,
                                     const struct plocation *location,
                                     const struct act_prob *prob)
{
  if (dout->json) {
    /* Create the action probability object. */
    json_t *action_probability = json_object();

    /* Write the action probability values to the fields of the action
     * probability object. */
    json_object_set_new(action_probability, "min", json_integer(prob->min));
    json_object_set_new(action_probability, "max", json_integer(prob->max));

    /* Put the action probability object in the packet. */
    plocation_write_data(dout->json, location, action_probability);
  } else {
    dio_put_action_probability_raw(&dout->raw, prob);
  }
}

/**********************************************************************//**
  Receive bool value.
**************************************************************************/
static bool dio_get_bool8_json_internal(json_t *json_packet,
                                        const struct plocation *location,
                                        bool *dest)
{
  json_t *pbool = plocation_read_data(json_packet, location);

  if (!pbool) {
    log_error("ERROR: Unable to get bool8 from location: %s", plocation_name(location));
    return FALSE;
  } 
  *dest = json_is_true(pbool);

  if (!dest) {
    log_error("ERROR: Unable to get bool from location: %s", plocation_name(location));
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Receive bool value.
**************************************************************************/
bool dio_get_bool8_json(struct connection *pc, struct data_in *din,
                        const struct plocation *location, bool *dest)
{
  if (pc->json_mode) {
    return dio_get_bool8_json_internal(pc->json_packet, location, dest);
  } else {
    return dio_get_bool8_raw(din, dest);
  }
}

/**********************************************************************//**
  Receive bool value.
**************************************************************************/
bool dio_get_bool32_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location, bool *dest)
{
  if (pc->json_mode) {
    json_t *pbool = plocation_read_data(pc->json_packet, location);

    if (!pbool) {
      log_error("ERROR: Unable to get bool32 from location: %s", plocation_name(location));
      return FALSE;
    }
    *dest = json_is_true(pbool);

    if (!dest) {
      log_error("ERROR: Unable to get bool32 from location: %s", plocation_name(location));
      return FALSE;
    }
  } else {
    return dio_get_bool32_raw(din, dest);
  }

  return TRUE;
}

/**********************************************************************//**
  Receive unsigned floating point value.
**************************************************************************/
bool dio_get_ufloat_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location,
                         float *dest, int float_factor)
{
  if (pc->json_mode) {
    json_t *preal = plocation_read_data(pc->json_packet, location);

    if (!preal) {
      log_error("ERROR: Unable to get real from location: %s", plocation_name(location));
      return FALSE;
    }
    *dest = json_real_value(preal);
  } else {
    return dio_get_ufloat_raw(din, dest, float_factor);
  }

  return TRUE;
}

/**********************************************************************//**
  Receive signed floating point value.
**************************************************************************/
bool dio_get_sfloat_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location,
                         float *dest, int float_factor)
{
  if (pc->json_mode) {
    json_t *preal = plocation_read_data(pc->json_packet, location);

    if (!preal) {
      log_error("ERROR: Unable to get real from location: %s", plocation_name(location));
      return FALSE;
    }
    *dest = json_real_value(preal);
  } else {
    return dio_get_sfloat_raw(din, dest, float_factor);
  }

  return TRUE;
}

/**********************************************************************//**
  Receive signed 8 bit value.
**************************************************************************/
bool dio_get_sint8_json(struct connection *pc, struct data_in *din,
                        const struct plocation *location, int *dest)
{
  if (pc->json_mode) {
    json_t *pint = plocation_read_data(pc->json_packet, location);

    if (!pint) {
      log_error("ERROR: Unable to get sint8 from location: %s", plocation_name(location));
      return FALSE;
    }
    *dest = json_integer_value(pint);

    if (!dest) {
      log_error("ERROR: Unable to get sint8 from location: %s", plocation_name(location));
      return FALSE;
    }
  } else {
    return dio_get_sint8_raw(din, dest);
  }

  return TRUE;
}

/**********************************************************************//**
  Receive signed 16 bit value.
**************************************************************************/
bool dio_get_sint16_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location, int *dest)
{
  if (pc->json_mode) {
    json_t *pint = plocation_read_data(pc->json_packet, location);

    if (!pint) {
      log_error("ERROR: Unable to get sint16 from location: %s", plocation_name(location));
      return FALSE;
    }
    *dest = json_integer_value(pint);

    if (!dest) {
      log_error("ERROR: Unable to get sint16 from location: %s", plocation_name(location));
      return FALSE;
    }
  } else {
    return dio_get_sint16_raw(din, dest);
  }

  return TRUE;
}

/**********************************************************************//**
  Receive block of memory as byte array.
**************************************************************************/
bool dio_get_memory_json(struct connection *pc, struct data_in *din,
                         struct plocation *location,
                         void *dest, size_t dest_size)
{
  if (pc->json_mode) {
    int i;

    location->sub_location = plocation_elem_new(0);

    for (i = 0; i < dest_size; i++) {
      int val;

      location->sub_location->number = i;

      if (!dio_get_uint8_json_internal(pc->json_packet, location, &val)) {
        free(location->sub_location);
        return FALSE;
      }
      ((unsigned char *)dest)[i] = val;
    }

    FC_FREE(location->sub_location);
  } else {
    return dio_get_memory_raw(din, dest, dest_size);
  }

  return TRUE;
}

/**********************************************************************//**
  Receive at max max_dest_size bytes long NULL-terminated string.
**************************************************************************/
static bool dio_get_string_json_internal(json_t *json_packet,
                                         const struct plocation *location,
                                         char *dest, size_t max_dest_size)
{
  json_t *pstring = plocation_read_data(json_packet, location);
  const char *result_str;

  if (!pstring) {
    log_error("ERROR: Unable to get string from location: %s", plocation_name(location));
    return FALSE;
  }

  result_str = json_string_value(pstring);

  if (dest
      && !dataio_get_conv_callback(dest, max_dest_size, result_str, strlen(result_str))) {
    log_error("ERROR: Unable to get string from location: %s", plocation_name(location));
    return FALSE;
  }

  return TRUE;
}

/**********************************************************************//**
  Receive at max max_dest_size bytes long NULL-terminated string.
**************************************************************************/
bool dio_get_string_json(struct connection *pc, struct data_in *din,
                         const struct plocation *location,
                         char *dest, size_t max_dest_size)
{
  if (pc->json_mode) {
    return dio_get_string_json_internal(pc->json_packet, location,
                                        dest, max_dest_size);
  } else {
    return dio_get_string_raw(din, dest, max_dest_size);
  }
}

/**********************************************************************//**
  Read and decode the string in the specified location.

  max_dest_size applies to both the encoded and to the decoded string.
**************************************************************************/
bool dio_get_estring_json(struct connection *pc, struct data_in *din,
                          const struct plocation *location,
                          char *dest, size_t max_dest_size)
{
  if (pc->json_mode) {
    char *escaped_value;
    char *unescaped_value;

    /* The encoded string has the same size limit as the decoded string. */
    escaped_value = fc_malloc(max_dest_size);

    if (!dio_get_string_json_internal(pc->json_packet, location,
                                      escaped_value, max_dest_size)) {
      /* dio_get_string_json() has logged this already. */
      return FALSE;
    }

    /* Let CURL find the length it self by passing 0 */
    unescaped_value = curl_easy_unescape(get_curl(), escaped_value, 0, NULL);

    /* Done with the escaped value. */
    FC_FREE(escaped_value);

    /* Copy the unescaped value so CURL can free its own copy. */
    memcpy(dest, unescaped_value,
           /* Don't copy the memory following unescaped_value. */
           MIN(max_dest_size, strlen(unescaped_value) + 1));

    /* CURL's memory management wants to free this it self. */
    curl_free(unescaped_value);

    /* Make sure that the string is terminated. */
    dest[max_dest_size - 1] = '\0';
  } else {
    return dio_get_estring_raw(din, dest, max_dest_size);
  }

  return TRUE;
}

#endif /* FREECIV_JSON_CONNECTION */
