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

/* speclists: "specific genlists", by dwp.
 * (A generalisation of previous city_list and unit_list stuff.)
 *
 * This file is used to implement a "specific" genlist.
 * That is, a (sometimes) type-checked genlist. (Or at least a
 * genlist with related functions with distinctly typed parameters.)
 * (Or, maybe, what you end up doing when you don't use C++ ?)
 * 
 * Before including this file, you must define the following:
 *   SPECLIST_TAG - this tag will be used to form names for functions etc.
 * You may also define:
 *   SPECLIST_TYPE - the typed genlist will contain pointers to this type;
 * If SPECLIST_TYPE is not defined, then 'struct SPECLIST_TAG' is used.
 * At the end of this file, these (and other defines) are undef-ed.
 *
 * Assuming SPECLIST_TAG were 'foo', and SPECLIST_TYPE were 'foo_t',
 * including this file would provide a struct definition for:
 *    struct foo_list;
 *    struct foo_list_link;
 *
 * function typedefs:
 *    typedef void (*foo_list_free_fn_t) (foo_t *);
 *    typedef foo_t * (*foo_list_copy_fn_t) (const foo_t *);
 *    typedef bool (*foo_list_comp_fn_t) (const foo_t *, const foo_t *);
 *    typedef bool (*foo_list_cond_fn_t) (const foo_t *);
 *
 * and prototypes for the following functions:
 *    struct foo_list *foo_list_new(void);
 *    struct foo_list *foo_list_new_full(foo_list_free_fn_t free_data_func);
 *    void foo_list_destroy(struct foo_list *plist);
 *    struct foo_list *foo_list_copy(const struct foolist *plist);
 *    struct foo_list *foo_list_copy_full(const struct foolist *plist,
 *                                        foo_list_copy_fn_t copy_data_func,
 *                                        foo_list_free_fn_t free_data_func);
 *    void foo_list_clear(struct foo_list *plist);
 *    void foo_list_unique(struct foo_list *plist);
 *    void foo_list_unique_full(struct foo_list *plist,
 *                              foo_list_comp_fn_t comp_data_func);
 *    void foo_list_append(struct foo_list *plist, foo_t *pfoo);
 *    void foo_list_prepend(struct foo_list *plist, foo_t *pfoo);
 *    void foo_list_insert(struct foo_list *plist, foo_t *pfoo, int idx);
 *    void foo_list_insert_after(struct foo_list *plist, foo_t *pfoo,
 *                               struct foo_list_link *plink);
 *    void foo_list_insert_before(struct foo_list *plist, foo_t *pfoo,
 *                                struct foo_list_link *plink);
 *    bool foo_list_remove(struct foo_list *plist, const foo_t *pfoo);
 *    bool foo_list_remove_if(struct foo_list *plist,
 *                            foo_list_cond_fn_t cond_data_func);
 *    int foo_list_remove_all(struct foo_list *plist, const foo_t *pfoo);
 *    int foo_list_remove_all_if(struct foo_list *plist,
 *                               foo_list_cond_fn_t cond_data_func);
 *    void foo_list_erase(struct foo_list *plist,
 *                        struct foo_list_link *plink);
 *    void foo_list_pop_front(struct foo_list *plist);
 *    void foo_list_pop_back(struct foo_list *plist);
 *    int foo_list_size(const struct foo_list *plist);
 *    foo_t *foo_list_get(const struct foo_list *plist, int idx);
 *    foo_t *foo_list_front(const struct foo_list *plist);
 *    foo_t *foo_list_back(const struct foo_list *plist);
 *    struct foo_list_link *foo_list_link_get(const struct foo_list *plist,
 *                                            int idx);
 *    struct foo_list_link *foo_list_head(const struct foo_list *plist);
 *    struct foo_list_link *foo_list_tail(const struct foo_list *plist);
 *    struct foo_list_link *foo_list_search(const struct foo_list *plist,
 *                                          const void *data);
 *    struct foo_list_link *foo_list_search_if(const struct foo_list *plist,
 *                                             foo_list_cond_fn_t
 *                                             cond_data_func);
 *    void foo_list_sort(struct foo_list *plist,
 *       int (*compar) (const foo_t *const *, const foo_t *const *));
 *    void foo_list_shuffle(struct foo_list *plist);
 *    void foo_list_reverse(struct foo_list *plist);
 *    void foo_list_allocate_mutex(struct foo_list *plist);
 *    void foo_list_release_mutex(struct foo_list *plist);
 *    foo_t *foo_list_link_data(const struct foo_list_link *plink);
 *    struct foo_list_link *
 *        foo_list_link_prev(const struct foo_list_link *plink);
 *    struct foo_list_link *
 *        foo_list_link_next(const struct foo_list_link *plink);
 *
 * You should also define yourself (this file cannot do this for you):
 * #define foo_list_iterate(foolist, pfoo)                                  \
 *   TYPED_LIST_ITERATE(foo_t, foolist, pfoo)
 * #define foo_list_iterate_end LIST_ITERATE_END
 *
 * #define foo_list_iterate_rev(foolist, pfoo)                              \
 *   TYPED_LIST_ITERATE_REV(foo_t, foolist, pfoo)
 * #define foo_list_iterate_rev_end LIST_ITERATE_REV_END
 *
 * #define foo_list_link_iterate(foolist, plink)                            \
 *   TYPED_LIST_LINK_ITERATE(struct foo_list_link, foolist, plink)
 * #define foo_list_link_iterate_end LIST_LINK_ITERATE_END
 *
 * #define foo_list_link_iterate_rev(foolist, plink)                        \
 *   TYPED_LIST_LINK_ITERATE_REV(struct foo_list_link, foolist, plink)
 * #define foo_list_link_iterate_rev_end LIST_LINK_ITERATE_REV_END
 *
 * #define foo_list_both_iterate(foolist, plink, pfoo)                      \
 *   TYPED_LIST_BOTH_ITERATE(struct foo_list_link, foo_t,                   \
                             foolist, plink, pfoo)
 * #define foo_list_both_iterate_end LIST_BOTH_ITERATE_END
 *
 * #define foo_list_both_iterate_rev(foolist, pfoo)                         \
 *   TYPED_LIST_BOTH_ITERATE_REV(struct foo_list_link, foo_t,               \
                                 foolist, plink, pfoo)
 * #define foo_list_both_iterate_rev_end LIST_BOTH_ITERATE_REV_END
 *
 * Note this is not protected against multiple inclusions; this is so that
 * you can have multiple different speclists. For each speclist, this file
 * should be included _once_, inside a .h file which _is_ itself protected
 * against multiple inclusions. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "genlist.h"

#ifndef SPECLIST_TAG
#error Must define a SPECLIST_TAG to use this header
#endif

#ifndef SPECLIST_TYPE
#define SPECLIST_TYPE struct SPECLIST_TAG
#endif

#define SPECLIST_PASTE_(x, y) x ## y
#define SPECLIST_PASTE(x, y) SPECLIST_PASTE_(x,y)

#define SPECLIST_LIST struct SPECLIST_PASTE(SPECLIST_TAG, _list)
#define SPECLIST_LINK struct SPECLIST_PASTE(SPECLIST_TAG, _list_link)
#define SPECLIST_FOO(suffix) SPECLIST_PASTE(SPECLIST_TAG, suffix)

/* Dummy type. Actually a genlist, and not defined anywhere. */
SPECLIST_LIST;

/* Dummy type. Actually a genlist_link, and not defined anywhere. */
SPECLIST_LINK;

/* Function related typedefs. */
typedef void (*SPECLIST_FOO(_list_free_fn_t)) (SPECLIST_TYPE *);
typedef SPECLIST_TYPE *
(*SPECLIST_FOO(_list_copy_fn_t)) (const SPECLIST_TYPE *);
typedef bool (*SPECLIST_FOO(_list_comp_fn_t)) (const SPECLIST_TYPE *,
                                               const SPECLIST_TYPE *);
typedef bool (*SPECLIST_FOO(_list_cond_fn_t)) (const SPECLIST_TYPE *);


/****************************************************************************
  Create a new speclist.
****************************************************************************/
static inline SPECLIST_LIST *SPECLIST_FOO(_list_new) (void)
fc__warn_unused_result;

static inline SPECLIST_LIST *SPECLIST_FOO(_list_new) (void)
{
  return (SPECLIST_LIST *) genlist_new();
}

/****************************************************************************
  Create a new speclist with a free callback.
****************************************************************************/
static inline SPECLIST_LIST *
SPECLIST_FOO(_list_new_full) (SPECLIST_FOO(_list_free_fn_t) free_data_func)
fc__warn_unused_result;

static inline SPECLIST_LIST *
SPECLIST_FOO(_list_new_full) (SPECLIST_FOO(_list_free_fn_t) free_data_func)
{
  return ((SPECLIST_LIST *)
          genlist_new_full((genlist_free_fn_t) free_data_func));
}

/****************************************************************************
  Free a speclist.
****************************************************************************/
static inline void SPECLIST_FOO(_list_destroy) (SPECLIST_LIST *tthis)
{
  genlist_destroy((struct genlist *) tthis);
}

/****************************************************************************
  Duplicate a speclist.
****************************************************************************/
static inline SPECLIST_LIST *
SPECLIST_FOO(_list_copy) (const SPECLIST_LIST *tthis)
fc__warn_unused_result;

static inline SPECLIST_LIST *
SPECLIST_FOO(_list_copy) (const SPECLIST_LIST *tthis)
{
  return (SPECLIST_LIST *) genlist_copy((const struct genlist *) tthis);
}

/****************************************************************************
  Duplicate a speclist with a free callback and a function to copy each
  element.
****************************************************************************/
static inline SPECLIST_LIST *
SPECLIST_FOO(_list_copy_full) (const SPECLIST_LIST *tthis,
                               SPECLIST_FOO(_list_copy_fn_t) copy_data_func,
                               SPECLIST_FOO(_list_free_fn_t) free_data_func)
fc__warn_unused_result;

static inline SPECLIST_LIST *
SPECLIST_FOO(_list_copy_full) (const SPECLIST_LIST *tthis,
                               SPECLIST_FOO(_list_copy_fn_t) copy_data_func,
                               SPECLIST_FOO(_list_free_fn_t) free_data_func)
{
  return ((SPECLIST_LIST *)
          genlist_copy_full((const struct genlist *) tthis,
                            (genlist_copy_fn_t) copy_data_func,
                            (genlist_free_fn_t) free_data_func));
}

/****************************************************************************
  Remove all elements from the speclist.
****************************************************************************/
static inline void SPECLIST_FOO(_list_clear) (SPECLIST_LIST *tthis)
{
  genlist_clear((struct genlist *) tthis);
}

/****************************************************************************
  Remove all element duplicates (the speclist must be sorted before).
****************************************************************************/
static inline void SPECLIST_FOO(_list_unique) (SPECLIST_LIST *tthis)
{
  genlist_unique((struct genlist *) tthis);
}

/****************************************************************************
  Remove all element duplicates (the speclist must be sorted before), using
  'comp_data_func' to determine if the elements are equivalents.
****************************************************************************/
static inline void
SPECLIST_FOO(_list_unique_full) (SPECLIST_LIST *tthis,
                                 SPECLIST_FOO(_list_comp_fn_t) comp_data_func)
{
  genlist_unique_full((struct genlist *) tthis,
                      (genlist_comp_fn_t) comp_data_func);
}

/****************************************************************************
  Push back an element into the speclist.
****************************************************************************/
static inline void SPECLIST_FOO(_list_append) (SPECLIST_LIST *tthis,
                                               SPECLIST_TYPE *pfoo)
{
  genlist_append((struct genlist *) tthis, pfoo);
}

/****************************************************************************
  Push front an element into the speclist.
****************************************************************************/
static inline void SPECLIST_FOO(_list_prepend) (SPECLIST_LIST *tthis,
                                                SPECLIST_TYPE *pfoo)
{
  genlist_prepend((struct genlist *) tthis, pfoo);
}

/****************************************************************************
  Insert an element into the speclist at the given position.
****************************************************************************/
static inline void SPECLIST_FOO(_list_insert) (SPECLIST_LIST *tthis,
                                               SPECLIST_TYPE *pfoo, int idx)
{
  genlist_insert((struct genlist *) tthis, pfoo, idx);
}

/****************************************************************************
  Insert an element after the specified link.
****************************************************************************/
static inline void SPECLIST_FOO(_list_insert_after) (SPECLIST_LIST *tthis,
                                                     SPECLIST_TYPE *pfoo,
                                                     SPECLIST_LINK *plink)
{
  genlist_insert_after((struct genlist *) tthis, pfoo,
                       (struct genlist_link *) plink);
}

/****************************************************************************
  Insert an element before the specified link.
****************************************************************************/
static inline void SPECLIST_FOO(_list_insert_before) (SPECLIST_LIST *tthis,
                                                     SPECLIST_TYPE *pfoo,
                                                     SPECLIST_LINK *plink)
{
  genlist_insert_before((struct genlist *) tthis, pfoo,
                        (struct genlist_link *) plink);
}

/****************************************************************************
  Search 'pfoo' in the speclist, and remove it. Returns TRUE on success.
****************************************************************************/
static inline bool SPECLIST_FOO(_list_remove) (SPECLIST_LIST *tthis,
                                               const SPECLIST_TYPE *pfoo)
{
  return genlist_remove((struct genlist *) tthis, pfoo);
}

/****************************************************************************
  Remove the first element which fit the conditional function. Returns
  TRUE on success.
****************************************************************************/
static inline bool
SPECLIST_FOO(_list_remove_if) (SPECLIST_LIST *tthis,
                               SPECLIST_FOO(_list_cond_fn_t) cond_data_func)
{
  return genlist_remove_if((struct genlist *) tthis,
                           (genlist_cond_fn_t) cond_data_func);
}

/****************************************************************************
  Remove 'pfoo' of the whole list. Returns the number of removed elements.
****************************************************************************/
static inline int SPECLIST_FOO(_list_remove_all) (SPECLIST_LIST *tthis,
                                                  const SPECLIST_TYPE *pfoo)
{
  return genlist_remove_all((struct genlist *) tthis, pfoo);
}

/****************************************************************************
  Remove all elements which fit the conditional function. Returns the
  number of removed elements.
****************************************************************************/
static inline bool
SPECLIST_FOO(_list_remove_all_if) (SPECLIST_LIST *tthis,
                                   SPECLIST_FOO(_list_cond_fn_t)
                                   cond_data_func)
{
  return genlist_remove_all_if((struct genlist *) tthis,
                               (genlist_cond_fn_t) cond_data_func);
}

/****************************************************************************
  Remove the elements pointed by 'plink'. Returns the next element of the
  speclist.

  NB: After calling this function 'plink' is no more usable. You should
  have saved the next or previous link before.
****************************************************************************/
static inline void SPECLIST_FOO(_list_erase) (SPECLIST_LIST *tthis,
                                              SPECLIST_LINK *plink)
{
  genlist_erase((struct genlist *) tthis, (struct genlist_link *) plink);
}

/****************************************************************************
  Remove the first element of the speclist.
****************************************************************************/
static inline void SPECLIST_FOO(_list_pop_front) (SPECLIST_LIST *tthis)
{
  genlist_pop_front((struct genlist *) tthis);
}

/****************************************************************************
  Remove the last element of the speclist.
****************************************************************************/
static inline void SPECLIST_FOO(_list_pop_back) (SPECLIST_LIST *tthis)
{
  genlist_pop_back((struct genlist *) tthis);
}

/****************************************************************************
  Return the number of elements inside the speclist.
****************************************************************************/
static inline int SPECLIST_FOO(_list_size) (const SPECLIST_LIST *tthis)
{
  return genlist_size((const struct genlist *) tthis);
}

/****************************************************************************
  Return the element at position in the speclist.
****************************************************************************/
static inline SPECLIST_TYPE *
SPECLIST_FOO(_list_get) (const SPECLIST_LIST *tthis, int slindex)
{
  return ((SPECLIST_TYPE *)
          genlist_get((const struct genlist *) tthis, slindex));
}

/****************************************************************************
  Return the first element of the speclist.
****************************************************************************/
static inline SPECLIST_TYPE *
SPECLIST_FOO(_list_front) (const SPECLIST_LIST *tthis)
{
  return (SPECLIST_TYPE *) genlist_front((const struct genlist *) tthis);
}

/****************************************************************************
  Return the last element of the speclist.
****************************************************************************/
static inline SPECLIST_TYPE *
SPECLIST_FOO(_list_back) (const SPECLIST_LIST *tthis)
{
  return (SPECLIST_TYPE *) genlist_back((const struct genlist *) tthis);
}

/****************************************************************************
  Return the element at position in the speclist.
****************************************************************************/
static inline SPECLIST_LINK *
SPECLIST_FOO(_list_link_get) (const SPECLIST_LIST *tthis, int slindex)
{
  return ((SPECLIST_LINK *)
          genlist_link_get((const struct genlist *) tthis, slindex));
}

/****************************************************************************
  Return the head link of the speclist.
****************************************************************************/
static inline SPECLIST_LINK *
SPECLIST_FOO(_list_head) (const SPECLIST_LIST *tthis)
{
  return (SPECLIST_LINK *) genlist_head((const struct genlist *) tthis);
}

/****************************************************************************
  Return the tail link of the speclist.
****************************************************************************/
static inline SPECLIST_LINK *
SPECLIST_FOO(_list_tail) (const SPECLIST_LIST *tthis)
{
  return (SPECLIST_LINK *) genlist_tail((const struct genlist *) tthis);
}

/****************************************************************************
  Return the link of the first element which match the data 'pfoo'.
****************************************************************************/
static inline SPECLIST_LINK *
SPECLIST_FOO(_list_search) (const SPECLIST_LIST *tthis,
                            const SPECLIST_TYPE *pfoo)
{
  return ((SPECLIST_LINK *)
          genlist_search((const struct genlist *) tthis, pfoo));
}

/****************************************************************************
  Return the link of the first element which match the conditional function.
****************************************************************************/
static inline SPECLIST_LINK *
SPECLIST_FOO(_list_search_if) (const SPECLIST_LIST *tthis,
                               SPECLIST_FOO(_list_cond_fn_t) cond_data_func)
{
  return ((SPECLIST_LINK *)
          genlist_search_if((const struct genlist *) tthis,
                            (genlist_cond_fn_t) cond_data_func));
}

/****************************************************************************
  Sort the speclist.
****************************************************************************/
static inline void
SPECLIST_FOO(_list_sort) (SPECLIST_LIST * tthis,
                          int (*compar) (const SPECLIST_TYPE *const *,
                                         const SPECLIST_TYPE *const *))
{
  genlist_sort((struct genlist *) tthis,
               (int (*)(const void *, const void *)) compar);
}

/****************************************************************************
  Shuffle the speclist.
****************************************************************************/
static inline void SPECLIST_FOO(_list_shuffle) (SPECLIST_LIST *tthis)
{
  genlist_shuffle((struct genlist *) tthis);
}

/****************************************************************************
  Reverse the order of the elements of the speclist.
****************************************************************************/
static inline void SPECLIST_FOO(_list_reverse) (SPECLIST_LIST *tthis)
{
  genlist_reverse((struct genlist *) tthis);
}

/****************************************************************************
  Allocate speclist mutex
****************************************************************************/
static inline void SPECLIST_FOO(_list_allocate_mutex) (SPECLIST_LIST *tthis)
{
  genlist_allocate_mutex((struct genlist *) tthis);
}

/****************************************************************************
  Release speclist mutex
****************************************************************************/
static inline void SPECLIST_FOO(_list_release_mutex) (SPECLIST_LIST *tthis)
{
  genlist_release_mutex((struct genlist *) tthis);
}

/****************************************************************************
  Return the data of the link.
****************************************************************************/
static inline SPECLIST_TYPE *
SPECLIST_FOO(_list_link_data) (const SPECLIST_LINK *plink)
{
  return ((SPECLIST_TYPE *)
          genlist_link_data((const struct genlist_link *) plink));
}

/****************************************************************************
  Return the previous link.
****************************************************************************/
static inline SPECLIST_LINK *
SPECLIST_FOO(_list_link_prev) (const SPECLIST_LINK *plink)
fc__warn_unused_result;

static inline SPECLIST_LINK *
SPECLIST_FOO(_list_link_prev) (const SPECLIST_LINK *plink)
{
  return ((SPECLIST_LINK *)
          genlist_link_prev((const struct genlist_link *) plink));
}

/****************************************************************************
  Return the next link.
****************************************************************************/
static inline SPECLIST_LINK *
SPECLIST_FOO(_list_link_next) (const SPECLIST_LINK *plink)
fc__warn_unused_result;

static inline SPECLIST_LINK *
SPECLIST_FOO(_list_link_next) (const SPECLIST_LINK *plink)
{
  return ((SPECLIST_LINK *)
          genlist_link_next((const struct genlist_link *) plink));
}

#undef SPECLIST_TAG
#undef SPECLIST_TYPE
#undef SPECLIST_PASTE_
#undef SPECLIST_PASTE
#undef SPECLIST_LIST
#undef SPECLIST_FOO

/* Base macros that the users can specialize. */
#ifndef FC__SPECLIST_H  /* Defines this only once, no multiple inclusions. */
#define FC__SPECLIST_H

#ifdef FREECIV_DEBUG
#  define TYPED_LIST_CHECK(ARG_list)                                       \
  fc_assert_action(NULL != ARG_list, break)
#else
#  define TYPED_LIST_CHECK(ARG_list) /* Nothing. */
#endif /* FREECIV_DEBUG */

/* Speclist data iterator.
 * 
 * Using *_list_remove(NAME_data) is safe in this loop (but it may be
 * inefficient due to the linear research of the data, see also
 * *_list_erase()).
 * Using *_list_clear() will result to use freed data. It must be avoided!
 *
 * TYPE_data - The real type of the data in the genlist/speclist.
 * ARG_list - The speclist to iterate.
 * NAME_data - The name of the data iterator (defined inside the macro). */
#define TYPED_LIST_ITERATE(TYPE_data, ARG_list, NAME_data)                  \
do {                                                                        \
  const struct genlist_link *NAME_data##_iter;                              \
  TYPE_data *NAME_data;                                                     \
  TYPED_LIST_CHECK(ARG_list);                                               \
  NAME_data##_iter = genlist_head((const struct genlist *) ARG_list);       \
  while (NULL != NAME_data##_iter) {                                        \
    NAME_data = (TYPE_data *) genlist_link_data(NAME_data##_iter);          \
    NAME_data##_iter = genlist_link_next(NAME_data##_iter);

/* Balance for above: */ 
#define LIST_ITERATE_END                                                    \
  }                                                                         \
} while (FALSE);

/* Mutex protected speclist data iterator.
 * 
 * Using *_list_remove(NAME_data) is safe in this loop (but it may be
 * inefficient due to the linear research of the data, see also
 * *_list_erase()).
 * Using *_list_clear() will result to use freed data. It must be avoided!
 *
 * TYPE_data - The real type of the data in the genlist/speclist.
 * LIST_tag - Tag of the speclist
 * ARG_list - The speclist to iterate.
 * NAME_data - The name of the data iterator (defined inside the macro). */
#define MUTEXED_LIST_ITERATE(TYPE_data, LIST_tag, ARG_list, NAME_data)      \
do {                                                                        \
  const struct genlist_link *NAME_data##_iter;                              \
  TYPE_data *NAME_data;                                                     \
  LIST_tag##_list_allocate_mutex(ARG_list);                                 \
  TYPED_LIST_CHECK(ARG_list);                                               \
  NAME_data##_iter = genlist_head((const struct genlist *) ARG_list);       \
  while (NULL != NAME_data##_iter) {                                        \
    NAME_data = (TYPE_data *) genlist_link_data(NAME_data##_iter);          \
    NAME_data##_iter = genlist_link_next(NAME_data##_iter);

/* Balance for above: */ 
#define MUTEXED_ITERATE_END(LIST_tag, ARG_list)                             \
  }                                                                         \
    LIST_tag##_list_release_mutex(ARG_list);                                \
} while (FALSE);

#define MUTEXED_ITERATE_BREAK(LIST_tag, ARG_list)                           \
do {                                                                        \
  LIST_tag##_list_release_mutex(ARG_list);                                  \
} while (FALSE);

/* Same, but iterate backwards:
 *
 * TYPE_data - The real type of the data in the genlist/speclist.
 * ARG_list - The speclist to iterate.
 * NAME_data - The name of the data iterator (defined inside the macro). */
#define TYPED_LIST_ITERATE_REV(TYPE_data, ARG_list, NAME_data)              \
do {                                                                        \
  const struct genlist_link *NAME_data##_iter;                              \
  TYPE_data *NAME_data;                                                     \
  TYPED_LIST_CHECK(ARG_list);                                               \
  NAME_data##_iter = genlist_tail((const struct genlist *) ARG_list);       \
  while (NULL != NAME_data##_iter) {                                        \
    NAME_data = (TYPE_data *) genlist_link_data(NAME_data##_iter);          \
    NAME_data##_iter = genlist_link_prev(NAME_data##_iter);

/* Balance for above: */ 
#define LIST_ITERATE_REV_END                                                \
  }                                                                         \
} while (FALSE);

/* Speclist link iterator.
 *
 * Using *_list_erase(NAME_link) is safe in this loop.
 * Using *_list_clear() will result to use freed data. It must be avoided!
 *
 * TYPE_link - The real type of the link.
 * ARG_list - The speclist to iterate.
 * NAME_link - The name of the link iterator (defined inside the macro). */
#define TYPED_LIST_LINK_ITERATE(TYPE_link, ARG_list, NAME_link)             \
do {                                                                        \
  TYPE_link *NAME_link = ((TYPE_link *)                                     \
                          genlist_head((const struct genlist *) ARG_list)); \
  TYPE_link *NAME_link##_next;                                              \
  TYPED_LIST_CHECK(ARG_list);                                               \
  for (; NULL != NAME_link; NAME_link = NAME_link##_next) {                 \
    NAME_link##_next = ((TYPE_link *)                                       \
                        genlist_link_next((struct genlist_link *)           \
                                          NAME_link));

/* Balance for above: */ 
#define LIST_LINK_ITERATE_END                                               \
  }                                                                         \
} while (FALSE);

/* Same, but iterate backwards:
 *
 * TYPE_link - The real type of the link.
 * ARG_list - The speclist to iterate.
 * NAME_link - The name of the link iterator (defined inside the macro). */
#define TYPED_LIST_LINK_ITERATE_REV(TYPE_link, ARG_list, NAME_link)         \
do {                                                                        \
  TYPE_link *NAME_link = ((TYPE_link *)                                     \
                          genlist_tail((const struct genlist *) ARG_list)); \
  TYPE_link *NAME_link##_prev;                                              \
  TYPED_LIST_CHECK(ARG_list);                                               \
  for (; NULL != NAME_link; NAME_link = NAME_link##_prev) {                 \
    NAME_link##_prev = ((TYPE_link *)                                       \
                        genlist_link_prev((struct genlist_link *)           \
                                          NAME_link));

/* Balance for above: */ 
#define LIST_LINK_ITERATE_REV_END                                           \
  }                                                                         \
} while (FALSE);

/* Speclist link and data iterator.
 *
 * Using *_list_erase(NAME_link) is safe in this loop.
 * Using *_list_clear() will result to use freed data. It must be avoided!
 *
 * TYPE_link - The real type of the link.
 * TYPE_data - The real type of the data in the genlist/speclist.
 * ARG_list - The speclist to iterate.
 * NAME_link - The name of the link iterator (defined inside the macro).
 * NAME_data - The name of the data iterator (defined inside the macro). */
#define TYPED_LIST_BOTH_ITERATE(TYPE_link, TYPE_data,                       \
                                ARG_list, NAME_link, NAME_data)             \
do {                                                                        \
  TYPE_link *NAME_link = ((TYPE_link *)                                     \
                          genlist_head((const struct genlist *) ARG_list)); \
  TYPE_link *NAME_link##_next;                                              \
  TYPE_data *NAME_data;                                                     \
  TYPED_LIST_CHECK(ARG_list);                                               \
  for (; NULL != NAME_link; NAME_link = NAME_link##_next) {                 \
    NAME_link##_next = ((TYPE_link *)                                       \
                        genlist_link_next((struct genlist_link *)           \
                                          NAME_link));                      \
    NAME_data = ((TYPE_data *)                                              \
                 genlist_link_data((struct genlist_link *) NAME_link));

/* Balance for above: */ 
#define LIST_BOTH_ITERATE_END                                               \
  }                                                                         \
} while (FALSE);

/* Same, but iterate backwards:
 *
 * TYPE_link - The real type of the link.
 * TYPE_data - The real type of the data in the genlist/speclist.
 * ARG_list - The speclist to iterate.
 * NAME_link - The name of the link iterator (defined inside the macro).
 * NAME_data - The name of the data iterator (defined inside the macro). */
#define TYPED_LIST_BOTH_ITERATE_REV(TYPE_link, TYPE_data,                   \
                                ARG_list, NAME_link, NAME_data)             \
do {                                                                        \
  TYPE_link *NAME_link = ((TYPE_link *)                                     \
                          genlist_tail((const struct genlist *) ARG_list)); \
  TYPE_link *NAME_link##_prev;                                              \
  TYPE_data *NAME_data;                                                     \
  TYPED_LIST_CHECK(ARG_list);                                               \
  for (; NULL != NAME_link; NAME_link = NAME_link##_prev) {                 \
    NAME_link##_prev = ((TYPE_link *)                                       \
                        genlist_link_prev((struct genlist_link *)           \
                                          NAME_link));                      \
    NAME_data = ((TYPE_data *)                                              \
                 genlist_link_data((struct genlist_link *) NAME_link));

/* Balance for above: */ 
#define LIST_BOTH_ITERATE_REV_END                                           \
  }                                                                         \
} while (FALSE);

#endif /* FC__SPECLIST_H */

/* This is after #endif FC__SPECLIST_H on purpose.
   extern "C" portion begins well before latter part of the header
   is guarded against multiple inclusions. */
#ifdef __cplusplus
}
#endif /* __cplusplus */
