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

/****************************************************************************
   A general-purpose generic hash table implementation.

   Based on implementation previous included in registry.c, but separated
   out so that can be used more generally.  Maybe we should just use glib?

      Original author:  David Pfitzner  dwp@mso.anu.edu.au

   A hash table maps keys to user data values, using a user-supplied hash
   function to do this efficiently. Here both keys and values are general
   data represented by (void*) pointers. Memory management of both keys
   and data is the responsibility of the caller: that is, the caller must
   ensure that the memory (especially for keys) remains valid (allocated)
   for as long as required (typically, the life of the genhash table).
   (Otherwise, to allocate keys internally would either have to restrict
   key type (e.g., strings), or have user-supplied function to duplicate
   a key type.  See further comments below.)

   User-supplied functions required are:
   key_val_func: map key to bucket number given number of buckets; should
                 map keys fairly evenly to range 0 to (num_buckets - 1)
                 inclusive.

   key_comp_func: compare keys for equality, necessary for lookups for keys
                  which map to the same genhash value. Keys which compare
                  equal should map to the same hash value. Returns 0 for
                  equality, so can use qsort-type comparison function (but
                  the hash table does not make use of the ordering
                  information if the return value is non-zero).

   Some constructors also accept following functions to be registered:
   key_copy_func: This is called when assigning a new value to a bucket.
   key_free_func: This is called when genhash no longer needs key construct.
                  Note that one key construct gets freed even when it is
                  replaced with another that is considered identical by
                  key_comp_func().
   data_copy_func: same as 'key_copy_func', but for data.
   data_free_func: same as 'key_free_func', but for data.

   Implementation uses open hashing. Collision resolution is done by
   separate chaining with linked lists. Resize hash table when deemed
   necessary by making and populating a new table.
****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <string.h>

/* utility */
#include "log.h"
#include "mem.h"
#include "shared.h"     /* ARRAY_SIZE */
#include "support.h"

#include "genhash.h"

#define FULL_RATIO 0.75         /* consider expanding when above this */
#define MIN_RATIO 0.24          /* shrink when below this */

struct genhash_entry {
  void *key;
  void *data;
  genhash_val_t hash_val;
  struct genhash_entry *next;
};

/* Contents of the opaque type: */
struct genhash {
  struct genhash_entry **buckets;
  genhash_val_fn_t key_val_func;
  genhash_comp_fn_t key_comp_func;
  genhash_copy_fn_t key_copy_func;
  genhash_free_fn_t key_free_func;
  genhash_copy_fn_t data_copy_func;
  genhash_free_fn_t data_free_func;
  size_t num_buckets;
  size_t num_entries;
  bool no_shrink;               /* Do not auto-shrink when set. */
};

struct genhash_iter {
  struct iterator vtable;
  struct genhash_entry *const *bucket, *const *end;
  const struct genhash_entry *iterator;
};

#define GENHASH_ITER(p) ((struct genhash_iter *) (p))


/************************************************************************//**
  A supplied genhash function appropriate to nul-terminated strings.
  Prefers table sizes that are prime numbers.
****************************************************************************/
genhash_val_t genhash_str_val_func(const char *vkey)
{
  unsigned long result = 0;

  for (; *vkey != '\0'; vkey++) {
    result *= 5; 
    result += *vkey;
  }
  result &= 0xFFFFFFFF; /* To make results independent of sizeof(long) */
  return result;
}

/************************************************************************//**
  A supplied function for comparison of nul-terminated strings:
****************************************************************************/
bool genhash_str_comp_func(const char *vkey1, const char *vkey2)
{
  return 0 == strcmp(vkey1, vkey2);
}

/************************************************************************//**
  Copy function for string allocation.
****************************************************************************/
char *genhash_str_copy_func(const char *vkey)
{
  return fc_strdup(NULL != vkey ? vkey : "");
}

/************************************************************************//**
  Free function for string allocation.
****************************************************************************/
void genhash_str_free_func(char *vkey)
{
#ifdef FREECIV_DEBUG
  fc_assert_ret(NULL != vkey);
#endif
  free(vkey);
}

/************************************************************************//**
  Calculate a "reasonable" number of buckets for a given number of entries.
  Gives a prime number far from powers of 2, allowing at least a factor of
  2 from the given number of entries for breathing room.

  Generalized restrictions on the behavior of this function:
  * MIN_BUCKETS <= genhash_calc_num_buckets(x)
  * genhash_calc_num_buckets(x) * MIN_RATIO < x  whenever
    x > MIN_BUCKETS * MIN_RATIO.
  * genhash_calc_num_buckets(x) * FULL_RATIO > x.
  This one is more of a recommendation, to ensure enough free space:
  * genhash_calc_num_buckets(x) >= 2 * x.
****************************************************************************/
#define MIN_BUCKETS 29  /* Historical purposes. */
static size_t genhash_calc_num_buckets(size_t num_entries)
{
  /* A bunch of prime numbers close to successive elements of the sequence
   * A_n = 3 * 2 ^ n; to be used for table sizes. */
  static const size_t sizes[] = {
    MIN_BUCKETS,          53,         97,           193,
    389,       769,       1543,       3079,         6151,
    12289,     24593,     49157,      98317,        196613,
    393241,    786433,    1572869,    3145739,      6291469,
    12582917,  25165843,  50331653,   100663319,    201326611,
    402653189, 805306457, 1610612741, 3221225473ul, 4294967291ul
  };
  const size_t *pframe = sizes, *pmid;
  int fsize = ARRAY_SIZE(sizes) - 1, lpart;

  num_entries <<= 1; /* breathing room */

  while (fsize > 0) {
    lpart = fsize >> 1;
    pmid = pframe + lpart;
    if (*pmid < num_entries) {
      pframe = pmid + 1;
      fsize = fsize - lpart - 1;
    } else {
      fsize = lpart;
    }
  }
  return *pframe;
}

/************************************************************************//**
  Internal constructor, specifying exact number of buckets.
  Allows to specify functions to free the memory allocated for the key and
  user-data that get called when removing the bucket from the hash table or
  changing key/user-data values.

  NB: Be sure to check the "copy constructor" genhash_copy() if you change
  this function significantly.
****************************************************************************/
static struct genhash *
genhash_new_nbuckets(genhash_val_fn_t key_val_func,
                     genhash_comp_fn_t key_comp_func,
                     genhash_copy_fn_t key_copy_func,
                     genhash_free_fn_t key_free_func,
                     genhash_copy_fn_t data_copy_func,
                     genhash_free_fn_t data_free_func,
                     size_t num_buckets)
{
  struct genhash *pgenhash = fc_malloc(sizeof(*pgenhash));

  log_debug("New genhash table with %lu buckets",
            (long unsigned) num_buckets);

  pgenhash->buckets = fc_calloc(num_buckets, sizeof(*pgenhash->buckets));
  pgenhash->key_val_func = key_val_func;
  pgenhash->key_comp_func = key_comp_func;
  pgenhash->key_copy_func = key_copy_func;
  pgenhash->key_free_func = key_free_func;
  pgenhash->data_copy_func = data_copy_func;
  pgenhash->data_free_func = data_free_func;
  pgenhash->num_buckets = num_buckets;
  pgenhash->num_entries = 0;
  pgenhash->no_shrink = FALSE;

  return pgenhash;
}

/************************************************************************//**
  Constructor specifying number of entries.
  Allows to specify functions to free the memory allocated for the key and
  user-data that get called when removing the bucket from the hash table or
  changing key/user-data values.
****************************************************************************/
struct genhash *
genhash_new_nentries_full(genhash_val_fn_t key_val_func,
                          genhash_comp_fn_t key_comp_func,
                          genhash_copy_fn_t key_copy_func,
                          genhash_free_fn_t key_free_func,
                          genhash_copy_fn_t data_copy_func,
                          genhash_free_fn_t data_free_func,
                          size_t nentries)
{
  return genhash_new_nbuckets(key_val_func, key_comp_func,
                              key_copy_func, key_free_func,
                              data_copy_func, data_free_func,
                              genhash_calc_num_buckets(nentries));
}

/************************************************************************//**
  Constructor specifying number of entries.
****************************************************************************/
struct genhash *genhash_new_nentries(genhash_val_fn_t key_val_func,
                                     genhash_comp_fn_t key_comp_func,
                                     size_t nentries)
{
  return genhash_new_nbuckets(key_val_func, key_comp_func,
                              NULL, NULL, NULL, NULL,
                              genhash_calc_num_buckets(nentries));
}

/************************************************************************//**
  Constructor with unspecified number of entries.
  Allows to specify functions to free the memory allocated for the key and
  user-data that get called when removing the bucket from the hash table or
  changing key/user-data values.
****************************************************************************/
struct genhash *genhash_new_full(genhash_val_fn_t key_val_func,
                                 genhash_comp_fn_t key_comp_func,
                                 genhash_copy_fn_t key_copy_func,
                                 genhash_free_fn_t key_free_func,
                                 genhash_copy_fn_t data_copy_func,
                                 genhash_free_fn_t data_free_func)
{
  return genhash_new_nbuckets(key_val_func, key_comp_func,
                              key_copy_func, key_free_func,
                              data_copy_func, data_free_func, MIN_BUCKETS);
}

/************************************************************************//**
  Constructor with unspecified number of entries.
****************************************************************************/
struct genhash *genhash_new(genhash_val_fn_t key_val_func,
                            genhash_comp_fn_t key_comp_func)
{
  return genhash_new_nbuckets(key_val_func, key_comp_func,
                              NULL, NULL, NULL, NULL, MIN_BUCKETS);
}

/************************************************************************//**
  Destructor: free internal memory.
****************************************************************************/
void genhash_destroy(struct genhash *pgenhash)
{
  fc_assert_ret(NULL != pgenhash);
  pgenhash->no_shrink = TRUE;
  genhash_clear(pgenhash);
  free(pgenhash->buckets);
  free(pgenhash);
}

/************************************************************************//**
  Resize the genhash table: relink entries.
****************************************************************************/
static void genhash_resize_table(struct genhash *pgenhash,
                                 size_t new_nbuckets)
{
  struct genhash_entry **new_buckets, **bucket, **end, **slot;
  struct genhash_entry *iter, *next;

  fc_assert(new_nbuckets >= pgenhash->num_entries);

  new_buckets = fc_calloc(new_nbuckets, sizeof(*pgenhash->buckets));

  bucket = pgenhash->buckets;
  end = bucket + pgenhash->num_buckets;
  for (; bucket < end; bucket++) {
    for (iter = *bucket; NULL != iter; iter = next) {
      slot = new_buckets + (iter->hash_val % new_nbuckets);
      next = iter->next;
      iter->next = *slot;
      *slot = iter;
    }
  }

  free(pgenhash->buckets);
  pgenhash->buckets = new_buckets;
  pgenhash->num_buckets = new_nbuckets;
}

/************************************************************************//**
  Call this when an entry might be added or deleted: resizes the genhash
  table if seems like a good idea.  Count deleted entries in check
  because efficiency may be degraded if there are too many deleted
  entries.  But for determining new size, ignore deleted entries,
  since they'll be removed by rehashing.
****************************************************************************/
#define genhash_maybe_expand(htab) genhash_maybe_resize((htab), TRUE)
#define genhash_maybe_shrink(htab) genhash_maybe_resize((htab), FALSE)
static bool genhash_maybe_resize(struct genhash *pgenhash, bool expandingp)
{
  size_t limit, new_nbuckets;

  if (!expandingp && pgenhash->no_shrink) {
    return FALSE;
  }
  if (expandingp) {
    limit = FULL_RATIO * pgenhash->num_buckets;
    if (pgenhash->num_entries < limit) {
      return FALSE;
    }
  } else {
    if (pgenhash->num_buckets <= MIN_BUCKETS) {
      return FALSE;
    }
    limit = MIN_RATIO * pgenhash->num_buckets;
    if (pgenhash->num_entries > limit) {
      return FALSE;
    }
  }

  new_nbuckets = genhash_calc_num_buckets(pgenhash->num_entries);

  log_debug("%s genhash (entries = %lu, buckets =  %lu, new = %lu, "
            "%s limit = %lu)",
            (new_nbuckets < pgenhash->num_buckets ? "Shrinking"
             : (new_nbuckets > pgenhash->num_buckets
                ? "Expanding" : "Rehashing")),
            (long unsigned) pgenhash->num_entries,
            (long unsigned) pgenhash->num_buckets,
            (long unsigned) new_nbuckets,
            expandingp ? "up": "down", (long unsigned) limit);
  genhash_resize_table(pgenhash, new_nbuckets);
  return TRUE;
}


/************************************************************************//**
  Calculate genhash value given hash table and key.
****************************************************************************/
static inline genhash_val_t genhash_val_calc(const struct genhash *pgenhash,
                                             const void *key)
{
  if (NULL != pgenhash->key_val_func) {
    return pgenhash->key_val_func(key);
  } else {
    return ((intptr_t) key);
  }
}

/************************************************************************//**
  Return slot (entry pointer) in genhash table where key resides, or where
  it should go if it is to be a new key.
****************************************************************************/
static inline struct genhash_entry **
genhash_slot_lookup(const struct genhash *pgenhash,
                    const void *key,
                    genhash_val_t hash_val)
{
  struct genhash_entry **slot;
  genhash_comp_fn_t key_comp_func = pgenhash->key_comp_func;

  slot = pgenhash->buckets + (hash_val % pgenhash->num_buckets);
  if (NULL != key_comp_func) {
    for (; NULL != *slot; slot = &(*slot)->next) {
      if (hash_val == (*slot)->hash_val
          && key_comp_func((*slot)->key, key)) {
        return slot;
      }
    }
  } else {
    for (; NULL != *slot; slot = &(*slot)->next) {
      if (key == (*slot)->key) {
        return slot;
      }
    }
  }
  return slot;
}

/************************************************************************//**
  Function to store from invalid data.
****************************************************************************/
static inline void genhash_default_get(void **pkey, void **data)
{
  if (NULL != pkey) {
    *pkey = NULL;
  }
  if (NULL != data) {
    *data = NULL;
  }
}

/************************************************************************//**
  Function to store data.
****************************************************************************/
static inline void genhash_slot_get(struct genhash_entry *const *slot,
                                    void **pkey, void **data)
{
  const struct genhash_entry *entry = *slot;

  if (NULL != pkey) {
    *pkey = entry->key;
  }
  if (NULL != data) {
    *data = entry->data;
  }
}

/************************************************************************//**
  Create the entry and call the copy callbacks.
****************************************************************************/
static inline void genhash_slot_create(struct genhash *pgenhash,
                                       struct genhash_entry **slot,
                                       const void *key, const void *data,
                                       genhash_val_t hash_val)
{
  struct genhash_entry *entry = fc_malloc(sizeof(*entry));

  entry->key = (NULL != pgenhash->key_copy_func
                ? pgenhash->key_copy_func(key) : (void *) key);
  entry->data = (NULL != pgenhash->data_copy_func
                 ? pgenhash->data_copy_func(data) : (void *) data);
  entry->hash_val = hash_val;
  entry->next = *slot;
  *slot = entry;
}

/************************************************************************//**
  Free the entry slot and call the free callbacks.
****************************************************************************/
static inline void genhash_slot_free(struct genhash *pgenhash,
                                     struct genhash_entry **slot)
{
  struct genhash_entry *entry = *slot;

  if (NULL != pgenhash->key_free_func) {
    pgenhash->key_free_func(entry->key);
  }
  if (NULL != pgenhash->data_free_func) {
    pgenhash->data_free_func(entry->data);
  }
  *slot = entry->next;
  free(entry);
}

/************************************************************************//**
  Clear previous values (with free callback) and call the copy callbacks.
****************************************************************************/
static inline void genhash_slot_set(struct genhash *pgenhash,
                                    struct genhash_entry **slot,
                                    const void *key, const void *data)
{
  struct genhash_entry *entry = *slot;

  if (NULL != pgenhash->key_free_func) {
    pgenhash->key_free_func(entry->key);
  }
  if (NULL != pgenhash->data_free_func) {
    pgenhash->data_free_func(entry->data);
  }
  entry->key = (NULL != pgenhash->key_copy_func
                ? pgenhash->key_copy_func(key) : (void *) key);
  entry->data = (NULL != pgenhash->data_copy_func
                 ? pgenhash->data_copy_func(data) : (void *) data);
}

/************************************************************************//**
  Prevent or allow the genhash table automatically shrinking. Returns the
  old value of the setting.
****************************************************************************/
bool genhash_set_no_shrink(struct genhash *pgenhash, bool no_shrink)
{
  bool old;

  fc_assert_ret_val(NULL != pgenhash, FALSE);
  old = pgenhash->no_shrink;
  pgenhash->no_shrink = no_shrink;
  return old;
}

/************************************************************************//**
  Returns the number of entries in the genhash table.
****************************************************************************/
size_t genhash_size(const struct genhash *pgenhash)
{
  fc_assert_ret_val(NULL != pgenhash, 0);
  return pgenhash->num_entries;
}

/************************************************************************//**
  Returns the number of buckets in the genhash table.
****************************************************************************/
size_t genhash_capacity(const struct genhash *pgenhash)
{
  fc_assert_ret_val(NULL != pgenhash, 0);
  return pgenhash->num_buckets;
}

/************************************************************************//**
  Returns a newly allocated mostly deep copy of the given genhash table.
****************************************************************************/
struct genhash *genhash_copy(const struct genhash *pgenhash)
{
  struct genhash *new_genhash;
  struct genhash_entry *const *src_bucket, *const *end;
  const struct genhash_entry *src_iter;
  struct genhash_entry **dest_slot, **dest_bucket;

  fc_assert_ret_val(NULL != pgenhash, NULL);

  new_genhash = fc_malloc(sizeof(*new_genhash));

  /* Copy fields. */
  *new_genhash = *pgenhash;

  /* But make fresh buckets. */
  new_genhash->buckets = fc_calloc(new_genhash->num_buckets,
                                   sizeof(*new_genhash->buckets));

  /* Let's re-insert all data */
  src_bucket = pgenhash->buckets;
  end = src_bucket + pgenhash->num_buckets;
  dest_bucket = new_genhash->buckets;

  for (; src_bucket < end; src_bucket++, dest_bucket++) {
    dest_slot = dest_bucket;
    for (src_iter = *src_bucket; NULL != src_iter;
         src_iter = src_iter->next) {
      genhash_slot_create(new_genhash, dest_slot, src_iter->key,
                          src_iter->data, src_iter->hash_val);
      dest_slot = &(*dest_slot)->next;
    }
  }

  return new_genhash;
}

/************************************************************************//**
  Remove all entries of the genhash table.
****************************************************************************/
void genhash_clear(struct genhash *pgenhash)
{
  struct genhash_entry **bucket, **end;

  fc_assert_ret(NULL != pgenhash);

  bucket = pgenhash->buckets;
  end = bucket + pgenhash->num_buckets;
  for (; bucket < end; bucket++) {
    while (NULL != *bucket) {
      genhash_slot_free(pgenhash, bucket);
    }
  }

  pgenhash->num_entries = 0;
  genhash_maybe_shrink(pgenhash);
}

/************************************************************************//**
  Insert entry: returns TRUE if inserted, or FALSE if there was already an
  entry with the same key, in which case the entry was not inserted.
****************************************************************************/
bool genhash_insert(struct genhash *pgenhash, const void *key,
                    const void *data)
{
  struct genhash_entry **slot;
  genhash_val_t hash_val;

  fc_assert_ret_val(NULL != pgenhash, FALSE);

  hash_val = genhash_val_calc(pgenhash, key);
  slot = genhash_slot_lookup(pgenhash, key, hash_val);
  if (NULL != *slot) {
    return FALSE;
  } else {
    if (genhash_maybe_expand(pgenhash)) {
      /* Recalculate slot. */
      slot = pgenhash->buckets + (hash_val % pgenhash->num_buckets);
    }
    genhash_slot_create(pgenhash, slot, key, data, hash_val);
    pgenhash->num_entries++;
    return TRUE;
  }
}

/************************************************************************//**
  Insert entry, replacing any existing entry which has the same key.
  Returns TRUE if a data have been replaced, FALSE if it was a simple
  insertion.
****************************************************************************/
bool genhash_replace(struct genhash *pgenhash, const void *key,
                     const void *data)
{
  return genhash_replace_full(pgenhash, key, data, NULL, NULL);
}

/************************************************************************//**
  Insert entry, replacing any existing entry which has the same key.
  Returns TRUE if a data have been replaced, FALSE if it was a simple
  insertion.

  Returns in 'old_pkey' and 'old_pdata' the old content of the bucket if
  they are not NULL. NB: It can returns freed pointers if free functions
  were supplied to the genhash table.
****************************************************************************/
bool genhash_replace_full(struct genhash *pgenhash, const void *key,
                          const void *data, void **old_pkey,
                          void **old_pdata)
{
  struct genhash_entry **slot;
  genhash_val_t hash_val;

  fc_assert_action(NULL != pgenhash,
                   genhash_default_get(old_pkey, old_pdata); return FALSE);

  hash_val = genhash_val_calc(pgenhash, key);
  slot = genhash_slot_lookup(pgenhash, key, hash_val);
  if (NULL != *slot) {
    /* Replace. */
    genhash_slot_get(slot, old_pkey, old_pdata);
    genhash_slot_set(pgenhash, slot, key, data);
    return TRUE;
  } else {
    /* Insert. */
    if (genhash_maybe_expand(pgenhash)) {
      /* Recalculate slot. */
      slot = pgenhash->buckets + (hash_val % pgenhash->num_buckets);
    }
    genhash_default_get(old_pkey, old_pdata);
    genhash_slot_create(pgenhash, slot, key, data, hash_val);
    pgenhash->num_entries++;
    return FALSE;
  }
}

/************************************************************************//**
  Lookup data. Return TRUE on success, then pdata - if not NULL will be set
  to the data value.
****************************************************************************/
bool genhash_lookup(const struct genhash *pgenhash, const void *key,
                    void **pdata)
{
  struct genhash_entry **slot;

  fc_assert_action(NULL != pgenhash,
                   genhash_default_get(NULL, pdata); return FALSE);

  slot = genhash_slot_lookup(pgenhash, key, genhash_val_calc(pgenhash, key));
  if (NULL != *slot) {
    genhash_slot_get(slot, NULL, pdata);
    return TRUE;
  } else {
    genhash_default_get(NULL, pdata);
    return FALSE;
  }
}

/************************************************************************//**
  Delete an entry from the genhash table. Returns TRUE on success.
****************************************************************************/
bool genhash_remove(struct genhash *pgenhash, const void *key)
{
  return genhash_remove_full(pgenhash, key, NULL, NULL);
}

/************************************************************************//**
  Delete an entry from the genhash table. Returns TRUE on success.

  Returns in 'deleted_pkey' and 'deleted_pdata' the old contents of the
  deleted entry if not NULL. NB: It can returns freed pointers if free
  functions were supplied to the genhash table.
****************************************************************************/
bool genhash_remove_full(struct genhash *pgenhash, const void *key,
                         void **deleted_pkey, void **deleted_pdata)
{
  struct genhash_entry **slot;

  fc_assert_action(NULL != pgenhash,
                   genhash_default_get(deleted_pkey, deleted_pdata);
                   return FALSE);

  slot = genhash_slot_lookup(pgenhash, key, genhash_val_calc(pgenhash, key));
  if (NULL != *slot) {
    genhash_slot_get(slot, deleted_pkey, deleted_pdata);
    genhash_slot_free(pgenhash, slot);
    genhash_maybe_shrink(pgenhash);
    fc_assert(0 < pgenhash->num_entries);
    pgenhash->num_entries--;
    return TRUE;
  } else {
    genhash_default_get(deleted_pkey, deleted_pdata);
    return FALSE;
  }
}


/************************************************************************//**
  Returns TRUE iff the hash tables contains the same pairs of key/data.
****************************************************************************/
bool genhashs_are_equal(const struct genhash *pgenhash1,
                        const struct genhash *pgenhash2)
{
  return genhashs_are_equal_full(pgenhash1, pgenhash2, NULL);
}

/************************************************************************//**
  Returns TRUE iff the hash tables contains the same pairs of key/data.
****************************************************************************/
bool genhashs_are_equal_full(const struct genhash *pgenhash1,
                             const struct genhash *pgenhash2,
                             genhash_comp_fn_t data_comp_func)
{
  struct genhash_entry *const *bucket1, *const *max1, *const *slot2;
  const struct genhash_entry *iter1;

  /* Check pointers. */
  if (pgenhash1 == pgenhash2) {
    return TRUE;
  } else if (NULL == pgenhash1 || NULL == pgenhash2) {
    return FALSE;
  }

  /* General check. */
  if (pgenhash1->num_entries != pgenhash2->num_entries
      /* If the key functions is not the same, we cannot know if the
       * keys are equals. */
      || pgenhash1->key_val_func != pgenhash2->key_val_func
      || pgenhash1->key_comp_func != pgenhash2->key_comp_func) {
    return FALSE;
  }

  /* Compare buckets. */
  bucket1 = pgenhash1->buckets;
  max1 = bucket1 + pgenhash1->num_buckets;
  for (; bucket1 < max1; bucket1++) {
    for (iter1 = *bucket1; NULL != iter1; iter1 = iter1->next) {
      slot2 = genhash_slot_lookup(pgenhash2, iter1->key, iter1->hash_val);
      if (NULL == *slot2
          || (iter1->data != (*slot2)->data
              && (NULL == data_comp_func
                  || !data_comp_func(iter1->data, (*slot2)->data)))) {
        return FALSE;
      }
    }
  }

  return TRUE;
}


/************************************************************************//**
  "Sizeof" function implementation for generic_iterate genhash iterators.
****************************************************************************/
size_t genhash_iter_sizeof(void)
{
  return sizeof(struct genhash_iter);
}

/************************************************************************//**
  Helper function for genhash (key, value) pair iteration.
****************************************************************************/
void *genhash_iter_key(const struct iterator *genhash_iter)
{
  struct genhash_iter *iter = GENHASH_ITER(genhash_iter);
  return (void *) iter->iterator->key;
}

/************************************************************************//**
  Helper function for genhash (key, value) pair iteration.
****************************************************************************/
void *genhash_iter_value(const struct iterator *genhash_iter)
{
  struct genhash_iter *iter = GENHASH_ITER(genhash_iter);
  return (void *) iter->iterator->data;
}

/************************************************************************//**
  Iterator interface 'next' function implementation.
****************************************************************************/
static void genhash_iter_next(struct iterator *genhash_iter)
{
  struct genhash_iter *iter = GENHASH_ITER(genhash_iter);

  iter->iterator = iter->iterator->next;
  if (NULL != iter->iterator) {
    return;
  }

  for (iter->bucket++; iter->bucket < iter->end; iter->bucket++) {
    if (NULL != *iter->bucket) {
      iter->iterator = *iter->bucket;
      return;
    }
  }
}

/************************************************************************//**
  Iterator interface 'get' function implementation. This just returns the
  iterator itself, so you would need to use genhash_iter_get_key/value to
  get the actual keys and values.
****************************************************************************/
static void *genhash_iter_get(const struct iterator *genhash_iter)
{
  return (void *) genhash_iter;
}

/************************************************************************//**
  Iterator interface 'valid' function implementation.
****************************************************************************/
static bool genhash_iter_valid(const struct iterator *genhash_iter)
{
  struct genhash_iter *iter = GENHASH_ITER(genhash_iter);
  return iter->bucket < iter->end;
}

/************************************************************************//**
  Common genhash iterator initializer.
****************************************************************************/
static inline struct iterator *
genhash_iter_init_common(struct genhash_iter *iter,
                         const struct genhash *pgenhash,
                         void * (*get) (const struct iterator *))
{
  if (NULL == pgenhash) {
    return invalid_iter_init(ITERATOR(iter));
  }

  iter->vtable.next = genhash_iter_next;
  iter->vtable.get = get;
  iter->vtable.valid = genhash_iter_valid;
  iter->bucket = pgenhash->buckets;
  iter->end = pgenhash->buckets + pgenhash->num_buckets;

  /* Seek to the first used bucket. */
  for (; iter->bucket < iter->end; iter->bucket++) {
    if (NULL != *iter->bucket) {
      iter->iterator = *iter->bucket;
      break;
    }
  }

  return ITERATOR(iter);
}

/************************************************************************//**
  Returns an iterator that iterates over both keys and values of the genhash
  table. NB: iterator_get() returns an iterator pointer, so use the helper
  functions genhash_iter_get_{key,value} to access the key and value.
****************************************************************************/
struct iterator *genhash_iter_init(struct genhash_iter *iter,
                                   const struct genhash *pgenhash)
{
  return genhash_iter_init_common(iter, pgenhash, genhash_iter_get);
}

/************************************************************************//**
  Returns an iterator over the genhash table's k genhashgenhashenhashys.
****************************************************************************/
struct iterator *genhash_key_iter_init(struct genhash_iter *iter,
                                       const struct genhash *pgenhash)
{
  return genhash_iter_init_common(iter, pgenhash, genhash_iter_key);
}

/************************************************************************//**
  Returns an iterator over the hash table's values.
****************************************************************************/
struct iterator *genhash_value_iter_init(struct genhash_iter *iter,
                                         const struct genhash *pgenhash)
{
  return genhash_iter_init_common(iter, pgenhash, genhash_iter_value);
}
