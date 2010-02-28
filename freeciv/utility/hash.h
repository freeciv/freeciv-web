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
#ifndef FC__HASH_H
#define FC__HASH_H

/**************************************************************************
   A general-purpose hash table implementation.
   See comments in hash.c.
***************************************************************************/

#include "shared.h"		/* bool type */

struct hash_table;		/* opaque */

/* Function typedefs: */
typedef unsigned int (*hash_val_fn_t)(const void *, unsigned int);
typedef int (*hash_cmp_fn_t)(const void *, const void *);
typedef void (*hash_free_fn_t)(void *);

/* Supplied functions (matching above typedefs) appropriate for
   keys being normal nul-terminated strings: */
unsigned int hash_fval_string(const void *vkey, unsigned int num_buckets);
int hash_fcmp_string(const void *vkey1, const void *vkey2);

/* Appropriate for int values: */
unsigned int hash_fval_int(const void *vkey, unsigned int num_buckets);
int hash_fcmp_int(const void *vkey1, const void *vkey2);

/* Appropriate for void pointers or casted longs, used as keys
   directly instead of by reference. */
unsigned int hash_fval_keyval(const void *vkey, unsigned int num_buckets);
int hash_fcmp_keyval(const void *vkey1, const void *vkey2);

/* General functions: */
struct hash_table *hash_new(hash_val_fn_t fval, hash_cmp_fn_t fcmp);
struct hash_table *hash_new_full(hash_val_fn_t fval,
			         hash_cmp_fn_t fcmp,
			         hash_free_fn_t free_key_func,
			         hash_free_fn_t free_data_func);
struct hash_table *hash_new_nentries(hash_val_fn_t fval, hash_cmp_fn_t fcmp,
				     unsigned int nentries);
struct hash_table *hash_new_nentries_full(hash_val_fn_t fval,
					  hash_cmp_fn_t fcmp,
					  hash_free_fn_t free_key_func,
					  hash_free_fn_t free_data_func,
					  unsigned int nentries);
struct hash_table *hash_copy(const struct hash_table *h);
bool hash_equal(const struct hash_table *a, const struct hash_table *b);

void hash_free(struct hash_table *h);

bool hash_insert(struct hash_table *h, const void *key, const void *data);
void *hash_replace(struct hash_table *h, const void *key, const void *data);

bool hash_key_exists(const struct hash_table *h, const void *key);
bool hash_lookup(const struct hash_table *h, const void *key,
                 const void **pkey, const void **pdata);
void *hash_lookup_data(const struct hash_table *h, const void *key);

void *hash_delete_entry(struct hash_table *h, const void *key);
void *hash_delete_entry_full(struct hash_table *h, const void *key,
			     void **deleted_key);
void hash_delete_all_entries(struct hash_table *h);

const void *hash_key_by_number(const struct hash_table *h,
			       unsigned int entry_number);
const void *hash_value_by_number(const struct hash_table *h,
				 unsigned int entry_number);

unsigned int hash_num_entries(const struct hash_table *h);
unsigned int hash_num_buckets(const struct hash_table *h);
unsigned int hash_num_deleted(const struct hash_table *h);

bool hash_set_no_shrink(struct hash_table *h,
                        bool no_shrink);

#include "iterator.h"

struct hash_iter;
size_t hash_iter_sizeof(void);

struct iterator *hash_key_iter_init(struct hash_iter *it,
                                    const struct hash_table *h);
#define hash_keys_iterate(ARG_ht, NAME_key)\
  generic_iterate(struct hash_iter, void *, NAME_key,\
                  hash_iter_sizeof, hash_key_iter_init, (ARG_ht))
#define hash_keys_iterate_end generic_iterate_end

struct iterator *hash_value_iter_init(struct hash_iter *it,
                                      const struct hash_table *h);
#define hash_values_iterate(ARG_ht, NAME_value)\
  generic_iterate(struct hash_iter, void *, NAME_value,\
                  hash_iter_sizeof, hash_value_iter_init, (ARG_ht))
#define hash_values_iterate_end generic_iterate_end

struct iterator *hash_iter_init(struct hash_iter *it,
                                const struct hash_table *h);
void *hash_iter_get_key(const struct iterator *hash_iter);
void *hash_iter_get_value(const struct iterator *hash_iter);
#define hash_iterate(ARG_ht, NAME_iter)\
  generic_iterate(struct hash_iter, struct iterator *, NAME_iter,\
                  hash_iter_sizeof, hash_iter_init, (ARG_ht))
#define hash_iterate_end generic_iterate_end

#endif  /* FC__HASH_H */
