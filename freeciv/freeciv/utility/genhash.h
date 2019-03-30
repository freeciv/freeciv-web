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
#ifndef FC__GENHASH_H
#define FC__GENHASH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/****************************************************************************
   A general-purpose generic hash table implementation.
   See comments in "genhash.c".
****************************************************************************/

/* utility */
#include "iterator.h"
#include "support.h"            /* bool type */

struct genhash;                 /* opaque */

/* Hash value type. */
typedef unsigned int genhash_val_t;

/* Function typedefs: */
typedef genhash_val_t (*genhash_val_fn_t) (const void *);
typedef bool (*genhash_comp_fn_t) (const void *, const void *);
typedef void * (*genhash_copy_fn_t) (const void *);
typedef void (*genhash_free_fn_t) (void *);


/* Supplied functions (matching above typedefs) appropriate for
 * keys being normal nul-terminated strings: */
genhash_val_t genhash_str_val_func(const char *vkey);
bool genhash_str_comp_func(const char *vkey1, const char *vkey2);
/* and malloc'ed strings: */
char *genhash_str_copy_func(const char *vkey);
void genhash_str_free_func(char *vkey);


/* General functions: */
struct genhash *genhash_new(genhash_val_fn_t key_val_func,
                            genhash_comp_fn_t key_comp_func)
                fc__warn_unused_result;
struct genhash *genhash_new_full(genhash_val_fn_t key_val_func,
                                 genhash_comp_fn_t key_comp_func,
                                 genhash_copy_fn_t key_copy_func,
                                 genhash_free_fn_t key_free_func,
                                 genhash_copy_fn_t data_copy_func,
                                 genhash_free_fn_t data_free_func)
                fc__warn_unused_result;
struct genhash *genhash_new_nentries(genhash_val_fn_t key_val_func,
                                     genhash_comp_fn_t key_comp_func,
                                     size_t nentries)
                fc__warn_unused_result;
struct genhash *
genhash_new_nentries_full(genhash_val_fn_t key_val_func,
                          genhash_comp_fn_t key_comp_func,
                          genhash_copy_fn_t key_copy_func,
                          genhash_free_fn_t key_free_func,
                          genhash_copy_fn_t data_copy_func,
                          genhash_free_fn_t data_free_func,
                          size_t nentries)
fc__warn_unused_result;
void genhash_destroy(struct genhash *pgenhash);

bool genhash_set_no_shrink(struct genhash *pgenhash, bool no_shrink);
size_t genhash_size(const struct genhash *pgenhash);
size_t genhash_capacity(const struct genhash *pgenhash);

struct genhash *genhash_copy(const struct genhash *pgenhash)
                fc__warn_unused_result;
void genhash_clear(struct genhash *pgenhash);

bool genhash_insert(struct genhash *pgenhash, const void *key,
                    const void *data);
bool genhash_replace(struct genhash *pgenhash, const void *key,
                     const void *data);
bool genhash_replace_full(struct genhash *pgenhash, const void *key,
                          const void *data, void **old_pkey,
                          void **old_pdata);

bool genhash_lookup(const struct genhash *pgenhash, const void *key,
                    void **pdata);

bool genhash_remove(struct genhash *pgenhash, const void *key);
bool genhash_remove_full(struct genhash *pgenhash, const void *key,
                         void **deleted_pkey, void **deleted_pdata);

bool genhashs_are_equal(const struct genhash *pgenhash1,
                        const struct genhash *pgenhash2);
bool genhashs_are_equal_full(const struct genhash *pgenhash1,
                             const struct genhash *pgenhash2,
                             genhash_comp_fn_t data_comp_func);


/* Iteration. */
struct genhash_iter;
size_t genhash_iter_sizeof(void);

struct iterator *genhash_key_iter_init(struct genhash_iter *iter,
                                       const struct genhash *hash);
#define genhash_keys_iterate(ARG_ht, NAME_key)                              \
  generic_iterate(struct genhash_iter, const void *, NAME_key,              \
                  genhash_iter_sizeof, genhash_key_iter_init, (ARG_ht))
#define genhash_keys_iterate_end generic_iterate_end

struct iterator *genhash_value_iter_init(struct genhash_iter *iter,
                                         const struct genhash *hash);
#define genhash_values_iterate(ARG_ht, NAME_value)                          \
  generic_iterate(struct genhash_iter, void *, NAME_value,                  \
                  genhash_iter_sizeof, genhash_value_iter_init, (ARG_ht))
#define genhash_values_iterate_end generic_iterate_end

struct iterator *genhash_iter_init(struct genhash_iter *iter,
                                   const struct genhash *hash);
void *genhash_iter_key(const struct iterator *genhash_iter);
void *genhash_iter_value(const struct iterator *genhash_iter);
#define genhash_iterate(ARG_ht, NAME_iter)                                  \
  generic_iterate(struct genhash_iter, struct iterator *, NAME_iter,        \
                  genhash_iter_sizeof, genhash_iter_init, (ARG_ht))
#define genhash_iterate_end generic_iterate_end

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__GENHASH_H */
