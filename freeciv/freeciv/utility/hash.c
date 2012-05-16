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

/**************************************************************************
   A general-purpose hash table implementation.

   Based on implementation previous included in registry.c, but separated
   out so that can be used more generally.  Maybe we should just use glib?

      Original author:  David Pfitzner  dwp@mso.anu.edu.au

   A hash table maps keys to user data values, using a user-supplied hash
   function to do this efficiently.  Here both keys and values are general
   data represented by (void*) pointers.  Memory management of both keys
   and data is the responsibility of the caller: that is, the caller must
   ensure that the memory (especially for keys) remains valid (allocated)
   for as long as required (typically, the life of the hash table).
   (Otherwise, to allocate keys internally would either have to restrict
   key type (e.g., strings), or have user-supplied function to duplicate
   a key type.  See further comments below.)

   User-supplied functions required are:
   
     fval: map key to bucket number given number of buckets; should map
   keys fairly evenly to range 0 to (num_buckets-1) inclusive.
   
     fcmp: compare keys for equality, necessary for lookups for keys
   which map to the same hash value.  Keys which compare equal should
   map to the same hash value.  Returns 0 for equality, so can use
   qsort-type comparison function (but the hash table does not make
   use of the ordering information if the return value is non-zero).

   Some constructors also accept following functions to be registered:

     free_key_func: This is called when hash no longer needs key construct.
   Note that one key construct gets freed even when it is replaced with
   another that is considered identical by fcmp().

     free_data_func: This is called when hash no longer needs data construct. 


   Implementation uses closed hashing with simple collision resolution.
   Deleted elements are marked as DELETED rather than UNUSED so that
   lookups on previously added entries work properly.
   Resize hash table when deemed necessary by making and populating
   a new table.


   * More on memory management of keys and user-data:
   
   The above scheme of key memory management may cause some difficulties
   in ensuring that keys are freed appropriately, since the caller may
   need to keep another copy of the key pointer somewhere to be able to
   free it.  Possible approaches within the current hash implementation:

   - Allocate keys using sbuffer, and free them all together when done;
     this is the approach taken in registry and tilespec.
   - Store the keys within the "object" being stored as user-data in the
     hash.  Then when the data is returned by hash_delete or hash_replace,
     the key is available to be freed (or can just be freed as part of
     deleting the "object").  (This is done in registry hsec, but the
     allocation is still via sbuffer.)
   - Specify key free function pointers upon hash table creation which
     handle the freeing.
   - Keep another hash from object pointers to key pointers?!  Seems a
     bit too perverse...

   There are some potential problems with memory management of the
   user-data pointers too:
   
   - If may have multiple keys pointing to the same data, cannot just
     free the data when deleted/replaced from hash.  (Eg, tilespec;
     eg, could reference count.)
   - When hash table as whole is deleted, need other access to user-data
     pointers if they need to be freed.

   Approaches:

   - Don't have hash table as only access to user-data.
   - Allocate user-data using sbuffer.
   - Specify user-data free function pointers upon hash table creation.

   
***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include "log.h"
#include "mem.h"
#include "shared.h" /* ARRAY_SIZE */

#include "hash.h"

#define FULL_RATIO 0.75         /* consider expanding when above this */
#define MIN_RATIO 0.24          /* shrink when below this */

enum Bucket_State { BUCKET_UNUSED=0, BUCKET_USED, BUCKET_DELETED };

struct hash_bucket {
  enum Bucket_State used;
  const void *key;
  const void *data;
  unsigned int hash_val;	/* to avoid recalculating, or an extra fcmp,
				   in lookup */
};

/* Contents of the opaque type: */
struct hash_table {
  struct hash_bucket *buckets;
  hash_val_fn_t fval;
  hash_cmp_fn_t fcmp;
  hash_free_fn_t free_key_func;
  hash_free_fn_t free_data_func;
  unsigned int num_buckets;
  unsigned int num_entries;	/* does not included deleted entries */
  unsigned int num_deleted;
  bool frozen;			/* do not auto-resize when set */
  bool no_shrink;		/* do not auto-shrink when set */
};

struct hash_iter {
  struct iterator vtable;
  const struct hash_bucket *b, *end;
};

#define HASH_ITER(p) ((struct hash_iter *)(p))

/* Calculate hash value given hash_table ptr and key: */
#define HASH_VAL(h,k) (((h)->fval)((k), ((h)->num_buckets)))

/**************************************************************************
  Initialize a hash bucket to "zero" data:
**************************************************************************/
static void zero_hbucket(struct hash_bucket *bucket)
{
  bucket->used = BUCKET_UNUSED;
  bucket->key = NULL;
  bucket->data = NULL;
  bucket->hash_val = 0;
}

/**************************************************************************
  Initialize a hash table to "zero" data:
**************************************************************************/
static void zero_htable(struct hash_table *h)
{
  h->buckets = NULL;
  h->fval = NULL;
  h->fcmp = NULL;
  h->free_key_func = NULL;
  h->free_data_func = NULL;
  h->num_buckets = h->num_entries = h->num_deleted = 0;
  h->frozen = FALSE;
  h->no_shrink = FALSE;
}


/**************************************************************************
  A supplied hash function where key is pointer to int.  
  Prefers table sizes that are prime numbers.
**************************************************************************/
unsigned int hash_fval_int(const void *vkey, unsigned int num_buckets)
{
  const unsigned int key = (unsigned int) *(const int*)vkey;
  return (key % num_buckets);
}

/**************************************************************************
  A supplied function for comparison of pointers to int:
**************************************************************************/
int hash_fcmp_int(const void *vkey1, const void *vkey2)
{
  const int *key1 = vkey1, *key2 = vkey2;

  /* avoid overflow issues: */
  return (*key1 < *key2) ? -1 : (*key1 > *key2) ? 1 : 0;
}


/**************************************************************************
  A supplied hash function appropriate to nul-terminated strings.
  Prefers table sizes that are prime numbers.
**************************************************************************/
unsigned int hash_fval_string(const void *vkey, unsigned int num_buckets)
{
  const char *key = (const char*)vkey;
  unsigned long result = 0;

  for (; *key != '\0'; key++) {
    result *= 5; 
    result += *key;
  }
  result &= 0xFFFFFFFF; /* To make results independent of sizeof(long) */
  return (result % num_buckets);
}

/**************************************************************************
  A supplied function for comparison of nul-terminated strings:
**************************************************************************/
int hash_fcmp_string(const void *vkey1, const void *vkey2)
{
  return strcmp((const char*)vkey1, (const char*)vkey2);
}


/**************************************************************************
  A supplied hash function which operates on the key pointer values
  themselves; this way a void* (or, with casting, a long) can be used
  as a key, and also without having allocated space for it.
**************************************************************************/
unsigned int hash_fval_keyval(const void *vkey, unsigned int num_buckets)
{
  unsigned long result = ((unsigned long)vkey);
  return (result % num_buckets);
}

/**************************************************************************
  A supplied function for comparison of the raw void pointers (or,
  with casting, longs)
**************************************************************************/
int hash_fcmp_keyval(const void *vkey1, const void *vkey2)
{
  /* Simplicity itself. */
  return (vkey1 < vkey2) ? -1 : (vkey1 > vkey2) ? 1 : 0;
}


/**************************************************************************
  A bunch of prime numbers close to successive elements of the
  sequence A_n=3*2^n; to be used for table sizes.
**************************************************************************/
#define MIN_BUCKETS 29 /* historical purposes */
static const unsigned long ht_sizes[] =
{
  MIN_BUCKETS,          53,         97,           193, 
  389,       769,       1543,       3079,         6151,     
  12289,     24593,     49157,      98317,        196613,    
  393241,    786433,    1572869,    3145739,      6291469,   
  12582917,  25165843,  50331653,   100663319,    201326611, 
  402653189, 805306457, 1610612741, 3221225473ul, 4294967291ul
};
#define NSIZES ARRAY_SIZE(ht_sizes)

/**************************************************************************
  Calculate a "reasonable" number of buckets for a given number of
  entries.  Gives a prime number far from powers of 2 (see ht_sizes
  above; this allows for simpler hash-functions), allowing at least a
  factor of 2 from the given number of entries for breathing room.
***************************************************************************
  Generalized restrictions on the behavior of this function:
  * calc_appropriate_nbuckets(x) >= MIN_BUCKETS
  * calc_appropriate_nbuckets(x) * MIN_RATIO < x  whenever 
    x > MIN_BUCKETS * MIN_RATIO
  * calc_appropriate_nbuckets(x) * FULL_RATIO > x
  This one is more of a recommendation, to ensure enough free space:
  * calc_appropriate_nbuckets(x) >= 2 * x
**************************************************************************/
static unsigned int calc_appropriate_nbuckets(unsigned int num_entries)
{
  const unsigned long *pframe = &ht_sizes[0], *pmid;
  int fsize = NSIZES - 1, lpart;

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

/**************************************************************************
  Internal constructor, specifying exact number of buckets.
  Allows to specify functions to free the memory allocated for the key and
  user-data that get called when removing the bucket from the hash table or
  changing key/user-data values.

  NB: Be sure to check the "copy constructor" hash_copy() if you change
  this function significantly.
**************************************************************************/
static struct hash_table *hash_new_nbuckets(hash_val_fn_t fval,
					    hash_cmp_fn_t fcmp,
					    hash_free_fn_t free_key_func,
					    hash_free_fn_t free_data_func,
					    unsigned int nbuckets)
{
  struct hash_table *h;
  unsigned i;

  freelog(LOG_DEBUG, "New hash table with %u buckets", nbuckets);
  
  h = (struct hash_table *)fc_malloc(sizeof(struct hash_table));
  zero_htable(h);

  h->num_buckets = nbuckets;
  h->num_entries = 0;
  h->fval = fval;
  h->fcmp = fcmp;
  h->free_key_func = free_key_func;
  h->free_data_func = free_data_func;

  h->buckets = (struct hash_bucket *)
      	       fc_malloc(h->num_buckets*sizeof(struct hash_bucket));

  for(i = 0; i < h->num_buckets; i++) {
    zero_hbucket(&h->buckets[i]);
  }
  return h;
}

/**************************************************************************
  Constructor specifying number of entries.
  Allows to specify functions to free the memory allocated for the key and
  user-data that get called when removing the bucket from the hash table or
  changing key/user-data values.
**************************************************************************/
struct hash_table *hash_new_nentries_full(hash_val_fn_t fval,
					  hash_cmp_fn_t fcmp,
					  hash_free_fn_t free_key_func,
					  hash_free_fn_t free_data_func,
					  unsigned int nentries)
{
  return hash_new_nbuckets(fval, fcmp, free_key_func, free_data_func,
			   calc_appropriate_nbuckets(nentries));
}

/**************************************************************************
  Constructor specifying number of entries:
**************************************************************************/
struct hash_table *hash_new_nentries(hash_val_fn_t fval, hash_cmp_fn_t fcmp,
				     unsigned int nentries)
{
  return hash_new_nentries_full(fval, fcmp, NULL, NULL, nentries);
}

/**************************************************************************
  Constructor with unspecified number of entries.
  Allows to specify functions to free the memory allocated for the key and
  user-data that get called when removing the bucket from the hash table or
  changing key/user-data values.
**************************************************************************/
struct hash_table *hash_new_full(hash_val_fn_t fval,
			         hash_cmp_fn_t fcmp,
			         hash_free_fn_t free_key_func,
			         hash_free_fn_t free_data_func)
{
  return hash_new_nentries_full(fval, fcmp, free_key_func, free_data_func, 0);
}

/**************************************************************************
  Constructor with unspecified number of entries:
**************************************************************************/
struct hash_table *hash_new(hash_val_fn_t fval, hash_cmp_fn_t fcmp)
{
  return hash_new_full(fval, fcmp, NULL, NULL);
}

/**************************************************************************
  Free the contents of the hash table, _except_ leave the struct itself
  intact.  (also zeros memory, ... may be useful?)
**************************************************************************/
static void hash_free_contents(struct hash_table *h)
{
  unsigned i;

  for(i = 0; i < h->num_buckets; i++) {
    zero_hbucket(&h->buckets[i]);
  }
  free(h->buckets);
  zero_htable(h);
}

/**************************************************************************
  Destructor: free internal memory (not user-data memory)
  (also zeros memory, ... may be useful?)
**************************************************************************/
void hash_free(struct hash_table *h)
{
  hash_delete_all_entries(h);
  hash_free_contents(h);
  free(h);
}

/**************************************************************************
  Resize the hash table: create a new table, insert, then remove
  the old and assign.
**************************************************************************/
static void hash_resize_table(struct hash_table *h, unsigned int new_nbuckets)
{
  struct hash_table *h_new;
  unsigned i;

  assert(new_nbuckets >= h->num_entries);

  h_new = hash_new_nbuckets(h->fval, h->fcmp,
			    h->free_key_func, h->free_data_func,
			    new_nbuckets);
  h_new->frozen = TRUE;
  
  for(i=0; i<h->num_buckets; i++) {
    struct hash_bucket *bucket = &h->buckets[i];

    if (bucket->used == BUCKET_USED) {
      if (!hash_insert(h_new, bucket->key, bucket->data)) {
	assert(0);
      }
    }
  }
  h_new->frozen = FALSE;

  hash_free_contents(h);
  *h = *h_new;
  free(h_new);
}

/**************************************************************************
  Call this when an entry might be added or deleted: resizes the hash
  table if seems like a good idea.  Count deleted entries in check
  because efficiency may be degraded if there are too many deleted
  entries.  But for determining new size, ignore deleted entries,
  since they'll be removed by rehashing.
**************************************************************************/
#define hash_maybe_expand(htab) hash_maybe_resize((htab), TRUE)
#define hash_maybe_shrink(htab) hash_maybe_resize((htab), FALSE)
static void hash_maybe_resize(struct hash_table *h, bool expandingp)
{
  unsigned int num_used, limit, new_nbuckets;

  if (h->frozen) {
    return;
  }
  if (!expandingp && h->no_shrink) {
    return;
  }
  num_used = h->num_entries + h->num_deleted;
  if (expandingp) {
    limit = FULL_RATIO * h->num_buckets;
    if (num_used < limit) {
      return;
    }
  } else {
    if (h->num_buckets <= MIN_BUCKETS) {
      return;
    }
    limit = MIN_RATIO * h->num_buckets;
    if (h->num_entries > limit) {
      return;
    }
  }
  
  new_nbuckets = calc_appropriate_nbuckets(h->num_entries);
  
  freelog(LOG_DEBUG, "%s hash table "
	  "(entry %u del %u used %u nbuck %u new %u %slimit %u)",
	  (new_nbuckets<h->num_buckets) ? "Shrinking" :
	  (new_nbuckets>h->num_buckets) ? "Expanding" : "Rehashing",
	  h->num_entries, h->num_deleted, num_used, 
	  h->num_buckets, new_nbuckets, expandingp?"up":"dn", limit);
  hash_resize_table(h, new_nbuckets);
}

/**************************************************************************
  Return pointer to bucket in hash table where key resides, or where it
  should go if it is to be a new key.  Note caller needs to provide
  pre-calculated hash_val (this is to avoid re-calculations).
  Return any bucket marked "deleted" in preference to using an
  unused bucket.  (But have to get to an unused bucket first, to
  know that the key is not in the table.  Use first such deleted
  to speed subsequent lookups on that key.)
**************************************************************************/
static struct hash_bucket *internal_lookup(const struct hash_table *h,
					   const void *key,
					   unsigned int hash_val)
{
  struct hash_bucket *bucket;
  struct hash_bucket *deleted = NULL;
  unsigned int i = hash_val;

  do {
    bucket = &h->buckets[i];
    switch (bucket->used) {
    case BUCKET_UNUSED:
      return deleted ? deleted : bucket;
    case BUCKET_USED:
      if (bucket->hash_val == hash_val
	  && h->fcmp(bucket->key, key) == 0) { /* match */
	return bucket;
      }
      break;
    case BUCKET_DELETED:
      if (!deleted) {
	deleted = bucket;
      }
      break;
    default:
      die("Bad value %d in switch(bucket->used)", (int) bucket->used);
    }
    i++;
    if (i == h->num_buckets) {
      i = 0;
    }
  } while (i != hash_val);	/* catch loop all the way round  */

  if (deleted) {
    return deleted;
  }
  die("Full hash table -- and somehow did not resize!!");
  return NULL;
}

/**************************************************************************
  Insert entry: returns 1 if inserted, or 0 if there was already an entry
  with the same key, in which case the entry was not inserted.
**************************************************************************/
bool hash_insert(struct hash_table *h, const void *key, const void *data)
{
  struct hash_bucket *bucket;
  int hash_val;

  hash_maybe_expand(h);
  hash_val = HASH_VAL(h, key);
  bucket = internal_lookup(h, key, hash_val);
  if (bucket->used == BUCKET_USED) {
    return FALSE;
  }
  if (bucket->used == BUCKET_DELETED) {
    assert(h->num_deleted>0);
    h->num_deleted--;
  }

  if (h->free_key_func) {
    h->free_key_func((void*)bucket->key);
  }
  bucket->key = key;
  if (h->free_data_func) {
    h->free_data_func((void*)bucket->data);
  }
  bucket->data = data;

  bucket->used = BUCKET_USED;
  bucket->hash_val = hash_val;
  h->num_entries++;
  return TRUE;
}

/**************************************************************************
  Insert entry, replacing any existing entry which has the same key.
  Returns user-data of replaced entry if there was one, or NULL.
  (E.g. this allows caller to free or adjust data being replaced.)
**************************************************************************/
void *hash_replace(struct hash_table *h, const void *key, const void *data)
{
  struct hash_bucket *bucket;
  int hash_val;
  const void *ret;
    
  hash_maybe_expand(h);
  hash_val = HASH_VAL(h, key);
  bucket = internal_lookup(h, key, hash_val);
  if (bucket->used == BUCKET_USED) {
    ret = bucket->data;
  } else {
    ret = NULL;
    if (bucket->used == BUCKET_DELETED) {
      assert(h->num_deleted>0);
      h->num_deleted--;
    }
    h->num_entries++;
    bucket->used = BUCKET_USED;
  }

  if (h->free_key_func) {
    h->free_key_func((void*)bucket->key);
  }
  bucket->key = key;
  if (h->free_data_func) {
    h->free_data_func((void*)bucket->data);
  }
  bucket->data = data;

  bucket->hash_val = hash_val;
  return (void*)ret;
}

/**************************************************************************
  Deletes a single bucket from the hash.  You may call this on an unused
  bucket.  old_key and old_value, if non-NULL, will be set to contain the
  pointers to the key and value that are being deleted.  free_key_func
  and free_data_func will (if set) be called on the key and data so these
  return values may already have been freed.
**************************************************************************/
static void hash_delete_bucket(struct hash_table *h,
			       struct hash_bucket *bucket,
			       void **old_key,
			       void **old_value)
{
  if (bucket->used == BUCKET_USED) {
    if (old_key) {
      *old_key = (void*)bucket->key;
    }
    if (old_value) {
      *old_value = (void*)bucket->data;
    }
    if (h->free_key_func) {
      h->free_key_func((void*)bucket->key);
    }
    if (h->free_data_func) {
      h->free_data_func((void*)bucket->data);
    }
    zero_hbucket(bucket);
    bucket->used = BUCKET_DELETED;
    h->num_deleted++;
    assert(h->num_entries > 0);
    h->num_entries--;
  } else {
    if (old_key) {
      *old_key = NULL;
    }
    if (old_value) {
      *old_value = NULL;
    }
  }

}

/**************************************************************************
  Delete an entry with specified key.  Returns user-data of deleted
  entry, or NULL if not found.
**************************************************************************/
void *hash_delete_entry(struct hash_table *h, const void *key)
{
  return hash_delete_entry_full(h, key, NULL);
}

/**************************************************************************
  Delete an entry with specified key.  Returns user-data of deleted
  entry, or NULL if not found.  old_key, if non-NULL, will be set to the
  key that was used for the bucket (the caller may need to free this
  value).
**************************************************************************/
void *hash_delete_entry_full(struct hash_table *h, const void *key,
			     void **old_key)
{
  struct hash_bucket *bucket;
  void *old_value;

  hash_maybe_shrink(h);  
  bucket = internal_lookup(h, key, HASH_VAL(h,key));
  hash_delete_bucket(h, bucket, old_key, &old_value);
  return old_value;
}

/**************************************************************************
  Delete all entries of the hash.
**************************************************************************/
void hash_delete_all_entries(struct hash_table *h)
{
  unsigned int bucket_nr;

  if (h->free_key_func == NULL && h->free_data_func == NULL) {
    memset(h->buckets, 0, sizeof(struct hash_bucket) * h->num_buckets);
    h->num_entries = 0;
    h->num_deleted = 0;
    h->frozen = FALSE;
  } else {
    /* Modeled after hash_key_by_number and hash_delete_entry. */
    for (bucket_nr = 0; bucket_nr < h->num_buckets; bucket_nr++) {
      hash_delete_bucket(h, &h->buckets[bucket_nr], NULL, NULL);
    }
  }
  hash_maybe_shrink(h);
}

/**************************************************************************
  Lookup: return existence:
**************************************************************************/
bool hash_key_exists(const struct hash_table *h, const void *key)
{
  struct hash_bucket *bucket = internal_lookup(h, key, HASH_VAL(h,key));
  return (bucket->used == BUCKET_USED);
}

/**************************************************************************
  Lookup: looks up a key in the hash table returning the original key, and
  the associated user-data, and a boolean which is TRUE if the key was
  found.
  (This is useful if you need to free the memory allocated for the
  original key, for example before calling hash_delete_entry.)
**************************************************************************/
bool hash_lookup(const struct hash_table *h, const void *key,
                 const void **pkey, const void **pdata)
{
  struct hash_bucket *bucket = internal_lookup(h, key, HASH_VAL(h,key));

  if (bucket->used == BUCKET_USED) {
    if (pkey) {
      *pkey = bucket->key;
    }
    if (pdata) {
      *pdata = bucket->data;
    }
    return TRUE;
  } else {
    if (pkey) {
      *pkey = NULL;
    }
    if (pdata) {
      *pdata = NULL;
    }
    return FALSE;
  }
}

/**************************************************************************
  Lookup data: return user-data, or NULL.
  (Note that in other respects NULL is treated as a valid value, this is
  merely intended as a convenience when caller never uses NULL as value.)
**************************************************************************/
void *hash_lookup_data(const struct hash_table *h, const void *key)
{
  const void *data;

  hash_lookup(h, key, NULL, &data);
  return (void*)data;
}

/**************************************************************************
  Accessor functions:
**************************************************************************/
unsigned int hash_num_entries(const struct hash_table *h)
{
  return h->num_entries;
}
unsigned int hash_num_buckets(const struct hash_table *h)
{
  return h->num_buckets;
}
unsigned int hash_num_deleted(const struct hash_table *h)
{
  return h->num_deleted;
}

/**************************************************************************
  Enumeration: returns the pointer to a key. The keys are returned in
  random order.
**************************************************************************/
const void *hash_key_by_number(const struct hash_table *h,
			       unsigned int entry_number)
{
  unsigned int bucket_nr, counter = 0;
  assert(entry_number < h->num_entries);

  for (bucket_nr = 0; bucket_nr < h->num_buckets; bucket_nr++) {
    struct hash_bucket *bucket = &h->buckets[bucket_nr];

    if (bucket->used != BUCKET_USED)
      continue;

    if (entry_number == counter)
      return bucket->key;
    counter++;
  }
  die("never reached");
  return NULL;
}

/**************************************************************************
  Enumeration: returns the pointer to a value. 
**************************************************************************/
const void *hash_value_by_number(const struct hash_table *h,
				 unsigned int entry_number)
{
  return hash_lookup_data(h, hash_key_by_number(h, entry_number));
}

/**************************************************************************
  Prevent or allow the hash table automatically shrinking. Returns
  the old value of the setting.
**************************************************************************/
bool hash_set_no_shrink(struct hash_table *h, bool no_shrink)
{
  bool old = h->no_shrink;
  h->no_shrink = no_shrink;
  return old;
}

/**************************************************************************
  "Sizeof" function implementation for generic_iterate hash iterators.
**************************************************************************/
size_t hash_iter_sizeof(void)
{
  return sizeof(struct hash_iter);
}

/**************************************************************************
  Helper function for hash (key, value) pair iteration.
**************************************************************************/
void *hash_iter_get_key(const struct iterator *hash_iter)
{
  struct hash_iter *it = HASH_ITER(hash_iter);
  return (void *) it->b->key;
}

/**************************************************************************
  Helper function for hash (key, value) pair iteration.
**************************************************************************/
void *hash_iter_get_value(const struct iterator *hash_iter)
{
  struct hash_iter *it = HASH_ITER(hash_iter);
  return (void *) it->b->data;
}

/**************************************************************************
  Iterator interface 'next' function implementation.
**************************************************************************/
static void hash_iter_next(struct iterator *iter)
{
  struct hash_iter *it = HASH_ITER(iter);
  do {
    it->b++;
  } while (it->b < it->end && it->b->used != BUCKET_USED);
}

/**************************************************************************
  Iterator interface 'get' function implementation. This just returns the
  iterator itself, so you would need to use hash_iter_get_key/value to
  get the actual keys and values.
**************************************************************************/
static void *hash_iter_get(const struct iterator *iter)
{
  return (void *) iter;
}

/**************************************************************************
  Iterator interface 'valid' function implementation.
**************************************************************************/
static bool hash_iter_valid(const struct iterator *iter)
{
  struct hash_iter *it = HASH_ITER(iter);
  return it->b < it->end;
}

/**************************************************************************
  Returns an iterator that iterates over both keys and values of the hash
  table. NB: iterator_get() returns an iterator pointer, so use the helper
  functions hash_iter_get_{key,value} to access the key and value.
**************************************************************************/
struct iterator *hash_iter_init(struct hash_iter *it,
                                const struct hash_table *h)
{
  if (!h) {
    return invalid_iter_init(ITERATOR(it));
  }

  it->vtable.next = hash_iter_next;
  it->vtable.get = hash_iter_get;
  it->vtable.valid = hash_iter_valid;
  it->b = h->buckets - 1;
  it->end = h->buckets + h->num_buckets;

  /* Seek to the first used bucket. */
  hash_iter_next(ITERATOR(it));

  return ITERATOR(it);
}

/**************************************************************************
  Returns an iterator over the hash table's keys.
**************************************************************************/
struct iterator *hash_key_iter_init(struct hash_iter *it,
                                    const struct hash_table *h)
{
  struct iterator *ret;

  if (!h) {
    return invalid_iter_init(ITERATOR(it));
  }

  ret = hash_iter_init(it, h);
  it->vtable.get = hash_iter_get_key;
  return ret;
}

/**************************************************************************
  Returns an iterator over the hash table's values.
**************************************************************************/
struct iterator *hash_value_iter_init(struct hash_iter *it,
                                      const struct hash_table *h)
{
  struct iterator *ret;

  if (!h) {
    return invalid_iter_init(ITERATOR(it));
  }

  ret = hash_iter_init(it, h);
  it->vtable.get = hash_iter_get_value;
  return ret;
}

/**************************************************************************
  Returns a newly allocated mostly deep copy of the given hash table.

  NB: This just copies the key and value pointers (it does NOT make a deep
  copy of those). So using this function with allocated keys/values or with
  a hash_free_fn_t should be avoided, unless you know what you are doing.
**************************************************************************/
struct hash_table *hash_copy(const struct hash_table *other)
{
  struct hash_table *h;
  size_t size;

  if (!other) {
    return NULL;
  }

  h = fc_malloc(sizeof(*h));

  /* Copy fields. */
  *h = *other;

  /* But make fresh buckets. */
  size = h->num_buckets * sizeof(struct hash_bucket);
  h->buckets = fc_malloc(size);

  /* NB: Shallow copy. */
  memcpy(h->buckets, other->buckets, size);

  return h;
}

/**************************************************************************
  Returns TRUE if the two hash tables are equal. If the hash tables do
  not have the same number of elements, or have different hashing, compare
  or free functions, they will be "unequal". Otherwise this function will
  compare key and value pointers.
**************************************************************************/
bool hash_equal(const struct hash_table *a, const struct hash_table *b)
{
  struct hash_iter hita, hitb;
  struct iterator *ita, *itb;

  if (a == b) {
    return TRUE;
  }
  if (!a || !b) {
    return FALSE;
  }
  if (a->fval != b->fval || a->fcmp != b->fcmp
      || a->free_key_func != b->free_key_func
      || a->free_data_func != b->free_data_func
      || hash_num_entries(a) != hash_num_entries(b)) {
    return FALSE;
  }

  ita = hash_iter_init(&hita, a);
  itb = hash_iter_init(&hitb, b);
  while (iterator_valid(ita) && iterator_valid(itb)) {
    if (hash_iter_get_key(ita) != hash_iter_get_key(itb)
        || hash_iter_get_value(ita) != hash_iter_get_value(itb)) {
      return FALSE;
    }
    iterator_next(ita);
    iterator_next(itb);
  }

  return TRUE;
}
