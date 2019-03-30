/***********************************************************************
 Freeciv - Copyright (C) 2001 - R. Falke
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

/* utility */
#include "dataio.h"
#include "fcintl.h"
#include "genhash.h"    /* genhash_val_t */
#include "log.h"
#include "mem.h"

/* common */
#include "packets.h"

/* client */
#include "client_main.h"

#include "attribute.h"

#define log_attribute           log_debug

enum attribute_serial {
  A_SERIAL_FAIL,
  A_SERIAL_OK,
  A_SERIAL_OLD,
};

struct attr_key {
  int key, id, x, y;
};

static genhash_val_t attr_key_val(const struct attr_key *pkey);
static bool attr_key_comp(const struct attr_key *pkey1,
                          const struct attr_key *pkey2);
static struct attr_key *attr_key_dup(const struct attr_key *pkey);
static void attr_key_destroy(struct attr_key *pkey);

/* 'struct attribute_hash' and related functions. */
#define SPECHASH_TAG attribute
#define SPECHASH_IKEY_TYPE struct attr_key *
#define SPECHASH_IDATA_TYPE void *
#define SPECHASH_IKEY_VAL attr_key_val
#define SPECHASH_IKEY_COMP attr_key_comp
#define SPECHASH_IKEY_COPY attr_key_dup
#define SPECHASH_IKEY_FREE attr_key_destroy
#define SPECHASH_IDATA_FREE free
#include "spechash.h"
#define attribute_hash_values_iterate(hash, pvalue)                         \
  TYPED_HASH_DATA_ITERATE(void *, hash, pvalue)
#define attribute_hash_values_iterate_end HASH_DATA_ITERATE_END
#define attribute_hash_iterate(hash, pkey, pvalue)                          \
  TYPED_HASH_ITERATE(const struct attr_key *, void *, hash, pkey, pvalue)
#define attribute_hash_iterate_end HASH_ITERATE_END


static struct attribute_hash *attribute_hash = NULL;

/************************************************************************//**
  Hash function for attribute_hash.
****************************************************************************/
static genhash_val_t attr_key_val(const struct attr_key *pkey)
{
  return (genhash_val_t) pkey->id ^ pkey->x ^ pkey->y ^ pkey->key;
}

/************************************************************************//**
  Compare-function for the keys in the hash table.
****************************************************************************/
static bool attr_key_comp(const struct attr_key *pkey1,
                          const struct attr_key *pkey2)
{
  return pkey1->key == pkey2->key
      && pkey1->id  == pkey2->id
      && pkey1->x   == pkey2->x
      && pkey1->y   == pkey2->y;
}

/************************************************************************//**
  Duplicate an attribute key.
****************************************************************************/
static struct attr_key *attr_key_dup(const struct attr_key *pkey)
{
  struct attr_key *pnew_key = fc_malloc(sizeof(*pnew_key));

  *pnew_key = *pkey;
  return pnew_key;
}

/************************************************************************//**
  Free an attribute key.
****************************************************************************/
static void attr_key_destroy(struct attr_key *pkey)
{
  fc_assert_ret(NULL != pkey);
  free(pkey);
}

/************************************************************************//**
  Initializes the attribute module.
****************************************************************************/
void attribute_init(void)
{
  fc_assert(NULL == attribute_hash);
  attribute_hash = attribute_hash_new();
}

/************************************************************************//**
  Frees the attribute module.
****************************************************************************/
void attribute_free(void)
{
  fc_assert_ret(NULL != attribute_hash);
  attribute_hash_destroy(attribute_hash);
  attribute_hash = NULL;
}

/************************************************************************//**
  Serialize an attribute hash for network/storage.
****************************************************************************/
static enum attribute_serial
serialize_hash(const struct attribute_hash *hash,
               void **pdata, int *pdata_length)
{
  /*
   * Layout of version 2:
   *
   * struct {
   *   uint32 0;   always != 0 in version 1
   *   uint8 2;
   *   uint32 entries;
   *   uint32 total_size_in_bytes;
   * } preamble;
   * 
   * struct {
   *   uint32 value_size;
   *   char key[], char value[];
   * } body[entries];
   */
  const size_t entries = attribute_hash_size(hash);
  int total_length, value_lengths[entries];
  void *result;
  struct raw_data_out dout;
  int i;

  /*
   * Step 1: loop through all keys and fill value_lengths and calculate
   * the total_length.
   */
  /* preamble */
  total_length = 4 + 1 + 4 + 4;
  /* body */
  total_length += entries * (4 + 4 + 4 + 2 + 2); /* value_size + key */
  i = 0;
  attribute_hash_values_iterate(hash, pvalue) {
    struct data_in din;

    dio_input_init(&din, pvalue, 4);
    dio_get_uint32_raw(&din, &value_lengths[i]);

    total_length += value_lengths[i];
    i++;
  } attribute_hash_values_iterate_end;

  /*
   * Step 2: allocate memory.
   */
  result = fc_malloc(total_length);
  dio_output_init(&dout, result, total_length);

  /*
   * Step 3: fill out the preamble.
   */
  dio_put_uint32_raw(&dout, 0);
  dio_put_uint8_raw(&dout, 2);
  dio_put_uint32_raw(&dout, attribute_hash_size(hash));
  dio_put_uint32_raw(&dout, total_length);

  /*
   * Step 4: fill out the body.
   */
  i = 0;
  attribute_hash_iterate(hash, pkey, pvalue) {
    dio_put_uint32_raw(&dout, value_lengths[i]);

    dio_put_uint32_raw(&dout, pkey->key);
    dio_put_uint32_raw(&dout, pkey->id);
    dio_put_sint16_raw(&dout, pkey->x);
    dio_put_sint16_raw(&dout, pkey->y);

    dio_put_memory_raw(&dout, ADD_TO_POINTER(pvalue, 4), value_lengths[i]);
    i++;
  } attribute_hash_iterate_end;

  fc_assert(!dout.too_short);
  fc_assert_msg(dio_output_used(&dout) == total_length,
                "serialize_hash() total_length = %lu, actual = %lu",
                (long unsigned)total_length,
                (long unsigned)dio_output_used(&dout));

  /*
   * Step 5: return.
   */
  *pdata = result;
  *pdata_length = total_length;
  log_attribute("attribute.c serialize_hash() "
                "serialized %lu entries in %lu bytes",
                (long unsigned) entries, (long unsigned) total_length);
  return A_SERIAL_OK;
}

/************************************************************************//**
  This data was serialized (above), sent as an opaque data packet to the
  server, stored in a savegame, retrieved from the savegame, sent as an
  opaque data packet back to the client, and now is ready to be restored.
  Check everything!
****************************************************************************/
static enum attribute_serial unserialize_hash(struct attribute_hash *hash,
                                              const void *data,
                                              size_t data_length)
{
  int entries, i, dummy;
  struct data_in din;

  attribute_hash_clear(hash);

  dio_input_init(&din, data, data_length);

  dio_get_uint32_raw(&din, &dummy);
  if (dummy != 0) {
    log_verbose("attribute.c unserialize_hash() preamble, uint32 %lu != 0",
                (long unsigned) dummy);
    return A_SERIAL_OLD;
  }
  dio_get_uint8_raw(&din, &dummy);
  if (dummy != 2) {
    log_verbose("attribute.c unserialize_hash() preamble, "
                "uint8 %lu != 2 version", (long unsigned) dummy);
    return A_SERIAL_OLD;
  }
  dio_get_uint32_raw(&din, &entries);
  dio_get_uint32_raw(&din, &dummy);
  if (dummy != data_length) {
    log_verbose("attribute.c unserialize_hash() preamble, "
                "uint32 %lu != %lu data_length",
                (long unsigned) dummy, (long unsigned) data_length);
    return A_SERIAL_FAIL;
  }

  log_attribute("attribute.c unserialize_hash() "
                "uint32 %lu entries, %lu data_length",
                (long unsigned) entries, (long unsigned) data_length);

  for (i = 0; i < entries; i++) {
    struct attr_key key;
    void *pvalue;
    int value_length;
    struct raw_data_out dout;

    if (!dio_get_uint32_raw(&din, &value_length)) {
      log_verbose("attribute.c unserialize_hash() "
                  "uint32 value_length dio_input_too_short");
      return A_SERIAL_FAIL;
    }
    log_attribute("attribute.c unserialize_hash() "
                  "uint32 %lu value_length", (long unsigned) value_length);

    /* next 12 bytes */
    if (!dio_get_uint32_raw(&din, &key.key)
        || !dio_get_uint32_raw(&din, &key.id)
        || !dio_get_sint16_raw(&din, &key.x)
        || !dio_get_sint16_raw(&din, &key.y)) {
      log_verbose("attribute.c unserialize_hash() "
                  "uint32 key dio_input_too_short");
      return A_SERIAL_FAIL;
    }
    pvalue = fc_malloc(value_length + 4);

    dio_output_init(&dout, pvalue, value_length + 4);
    dio_put_uint32_raw(&dout, value_length);
    if (!dio_get_memory_raw(&din, ADD_TO_POINTER(pvalue, 4), value_length)) {
      log_verbose("attribute.c unserialize_hash() "
                  "memory dio_input_too_short");
      return A_SERIAL_FAIL;
    }

    if (!attribute_hash_insert(hash, &key, pvalue)) {
      /* There are some untraceable attribute bugs caused by the CMA that
       * can cause this to happen. I think the only safe thing to do is
       * to delete all attributes. Another symptom of the bug is the
       * value_length (above) is set to a random value, which can also
       * cause a bug. */
      free(pvalue);
      attribute_hash_clear(hash);
      return A_SERIAL_FAIL;
    }
  }

  if (dio_input_remaining(&din) > 0) {
    /* This is not an error, as old clients sent overlong serialized
     * attributes pre gna bug #21295, and these will be hanging around
     * in savefiles forever. */
    log_attribute("attribute.c unserialize_hash() "
                  "ignored %lu trailing octets",
                  (long unsigned) dio_input_remaining(&din));
  }

  return A_SERIAL_OK;
}

/************************************************************************//**
  Send current state to the server. Note that the current
  implementation will send all attributes to the server.
****************************************************************************/
void attribute_flush(void)
{
  struct player *pplayer = client_player();

  if (!pplayer || client_is_observer() || !pplayer->is_alive) {
    return;
  }

  fc_assert_ret(NULL != attribute_hash);

  if (0 == attribute_hash_size(attribute_hash))
    return;

  if (pplayer->attribute_block.data) {
    free(pplayer->attribute_block.data);
    pplayer->attribute_block.data = NULL;
  }

  serialize_hash(attribute_hash, &pplayer->attribute_block.data,
                 &pplayer->attribute_block.length);
  send_attribute_block(pplayer, &client.conn);
}

/************************************************************************//**
  Recreate the attribute set from the player's
  attribute_block. Shouldn't be used by normal code.
****************************************************************************/
void attribute_restore(void)
{
  struct player *pplayer = client_player();

  if (!pplayer) {
    return;
  }

  fc_assert_ret(attribute_hash != NULL);

  switch (unserialize_hash(attribute_hash,
                           pplayer->attribute_block.data,
                           pplayer->attribute_block.length)) {
  case A_SERIAL_FAIL:
    log_error(_("There has been a CMA error. "
                "Your citizen governor settings may be broken."));
    break;
  case A_SERIAL_OLD:
    log_normal(_("Old attributes detected and removed."));
    break;
  default:
    break;
  };
}

/************************************************************************//**
  Low-level function to set an attribute.  If data_length is zero the
  attribute is removed.
****************************************************************************/
void attribute_set(int key, int id, int x, int y, size_t data_length,
                   const void *const data)
{
  struct attr_key akey = { .key = key, .id = id, .x = x, .y = y };

  log_attribute("attribute_set(key = %d, id = %d, x = %d, y = %d, "
                "data_length = %lu, data = %p)", key, id, x, y,
                (long unsigned) data_length, data);

  fc_assert_ret(NULL != attribute_hash);

  if (0 != data_length) {
    void *pvalue = fc_malloc(data_length + 4);
    struct raw_data_out dout;

    dio_output_init(&dout, pvalue, data_length + 4);
    dio_put_uint32_raw(&dout, data_length);
    dio_put_memory_raw(&dout, data, data_length);

    attribute_hash_replace(attribute_hash, &akey, pvalue);
  } else {
    attribute_hash_remove(attribute_hash, &akey);
  }
}

/************************************************************************//**
  Low-level function to get an attribute. If data hasn't enough space
  to hold the attribute data isn't set to the attribute. Returns the
  actual size of the attribute. Can be zero if the attribute is
  unset. To get the size of an attribute use 
    size = attribute_get(key, id, x, y, 0, NULL)
*****************************************************************************/
size_t attribute_get(int key, int id, int x, int y, size_t max_data_length,
                     void *data)
{
  struct attr_key akey = { .key = key, .id = id, .x = x, .y = y };
  void *pvalue;
  int length;
  struct data_in din;

  log_attribute("attribute_get(key = %d, id = %d, x = %d, y = %d, "
                "max_data_length = %lu, data = %p)", key, id, x, y,
                (long unsigned) max_data_length, data);

  fc_assert_ret_val(NULL != attribute_hash, 0);

  if (!attribute_hash_lookup(attribute_hash, &akey, &pvalue)) {
    log_attribute("  not found");
    return 0;
  }

  dio_input_init(&din, pvalue, 0xffffffff);
  dio_get_uint32_raw(&din, &length);

  if (length <= max_data_length) {
    dio_get_memory_raw(&din, data, length);
  }

  log_attribute("  found length = %d", length);
  return length;
}

/************************************************************************//**
  Set unit related attribute
****************************************************************************/
void attr_unit_set(enum attr_unit what, int unit_id, size_t data_length,
		   const void *const data)
{
  attribute_set(what, unit_id, -1, -2, data_length, data);
}

/************************************************************************//**
  Get unit related attribute
****************************************************************************/
size_t attr_unit_get(enum attr_unit what, int unit_id, size_t max_data_length,
		  void *data)
{
  return attribute_get(what, unit_id, -1, -2, max_data_length, data);
}

/************************************************************************//**
  Set unit related integer attribute
****************************************************************************/
void attr_unit_set_int(enum attr_unit what, int unit_id, int data)
{
  attr_unit_set(what, unit_id, sizeof(int), &data);
}

/************************************************************************//**
  Get unit related integer attribute
****************************************************************************/
size_t attr_unit_get_int(enum attr_unit what, int unit_id, int *data)
{
  return attr_unit_get(what, unit_id, sizeof(int), data);
}

/************************************************************************//**
  Set city related attribute
****************************************************************************/
void attr_city_set(enum attr_city what, int city_id, size_t data_length,
		   const void *const data)
{
  attribute_set(what, city_id, -1, -1, data_length, data);
}

/************************************************************************//**
  Get city related attribute
****************************************************************************/
size_t attr_city_get(enum attr_city what, int city_id, size_t max_data_length,
		  void *data)
{
  return attribute_get(what, city_id, -1, -1, max_data_length, data);
}

/************************************************************************//**
  Set city related integer attribute
****************************************************************************/
void attr_city_set_int(enum attr_city what, int city_id, int data)
{
  attr_city_set(what, city_id, sizeof(int), &data);
}

/************************************************************************//**
  Get city related integer attribute
****************************************************************************/
size_t attr_city_get_int(enum attr_city what, int city_id, int *data)
{
  return attr_city_get(what, city_id, sizeof(int), data);
}

/************************************************************************//**
  Set player related attribute
****************************************************************************/
void attr_player_set(enum attr_player what, int player_id, size_t data_length,
		     const void *const data)
{
  attribute_set(what, player_id, -1, -1, data_length, data);
}

/************************************************************************//**
  Get player related attribute
****************************************************************************/
size_t attr_player_get(enum attr_player what, int player_id,
		    size_t max_data_length, void *data)
{
  return attribute_get(what, player_id, -1, -1, max_data_length, data);
}

/************************************************************************//**
  Set tile related attribute
****************************************************************************/
void attr_tile_set(enum attr_tile what, int x, int y, size_t data_length,
		   const void *const data)
{
  attribute_set(what, -1, x, y, data_length, data);
}

/************************************************************************//**
  Get tile related attribute
****************************************************************************/
size_t attr_tile_get(enum attr_tile what, int x, int y, size_t max_data_length,
		  void *data)
{
  return attribute_get(what, -1, x, y, max_data_length, data);
}
