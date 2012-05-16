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
   (A generalisation of previous city_list and unit_list stuff.)
   
   This file is used to implement a "specific" genlist.
   That is, a (sometimes) type-checked genlist.  (Or at least a
   genlist with related functions with distinctly typed parameters.)
   (Or, maybe, what you end up doing when you don't use C++ ?)
   
   Before including this file, you must define the following:
     SPECLIST_TAG - this tag will be used to form names for functions etc.
   You may also define:  
     SPECLIST_TYPE - the typed genlist will contain pointers to this type;
   If SPECLIST_TYPE is not defined, then 'struct SPECLIST_TAG' is used.
   At the end of this file, these (and other defines) are undef-ed.

   Assuming SPECLIST_TAG were 'foo', and SPECLIST_TYPE were 'foo_t',
   including this file would provide a struct definition for:
      struct foo_list;
   and prototypes for the following functions:
      struct foolist *foo_list_new();
      void foo_list_free(struct foo_list *This);
      int  foo_list_size(struct foo_list *This);
      foo_t *foo_list_get(struct foo_list *This, int index);
      void foo_list_prepend(struct foo_list *This, foo_t *pfoo);
      void foo_list_append(struct foo_list *This, foo_t *pfoo);
      void foo_list_unlink(struct foo_list *This, foo_t *pfoo);
      void foo_list_clear(struct foo_list *This);
      bool foo_list_search(struct foo_list *this, foo_t *pfoo);
      void foo_list_sort(struct foo_list *This, 
         int (*compar)(const void *, const void *));
      void foo_list_shuffle(struct foo_list *This);
      const struct genlist *foo_list_base(const struct foo_list *This);

   You should also define yourself:  (this file cannot do this for you)
   
   #define foo_list_iterate(foolist, pfoo) \
       TYPED_LIST_ITERATE(foo_t, foolist, pfoo)
   #define foo_list_iterate_end  LIST_ITERATE_END

   Also, in a single .c file, you should include speclist_c.h,
   with the same SPECLIST_TAG and SPECLIST_TYPE defined, to
   provide the function implementations.

   Note this is not protected against multiple inclusions; this is
   so that you can have multiple different speclists.  For each
   speclist, this file should be included _once_, inside a .h file
   which _is_ itself protected against multiple inclusions.
*/

#include "genlist.h"
#include "mem.h"

#ifndef SPECLIST_TAG
#error Must define a SPECLIST_TAG to use this header
#endif

#ifndef SPECLIST_TYPE
#define SPECLIST_TYPE struct SPECLIST_TAG
#endif

#define SPECLIST_PASTE_(x, y) x ## y
#define SPECLIST_PASTE(x, y) SPECLIST_PASTE_(x,y)

#define SPECLIST_LIST struct SPECLIST_PASTE(SPECLIST_TAG, _list)
#define SPECLIST_FOO(suffix) SPECLIST_PASTE(SPECLIST_TAG, suffix)

/* Opaque type.  Actually a genlist, but not defined anywhere. */
SPECLIST_LIST;

#define GENLIST(speclist) ((struct genlist *) (speclist))
#define SPECLIST(genlist) ((SPECLIST_LIST *) (genlist))

static inline SPECLIST_LIST *SPECLIST_FOO(_list_new) (void)
{
  return SPECLIST(genlist_new());
}

static inline SPECLIST_LIST *SPECLIST_FOO(_list_copy) (const SPECLIST_LIST *plist)
{
  return SPECLIST(genlist_copy(GENLIST(plist)));
}

static inline void SPECLIST_FOO(_list_prepend) (SPECLIST_LIST *tthis, SPECLIST_TYPE *pfoo)
{
  genlist_prepend(GENLIST(tthis), pfoo);
}

static inline void SPECLIST_FOO(_list_unlink) (SPECLIST_LIST *tthis, SPECLIST_TYPE *pfoo)
{
  genlist_unlink(GENLIST(tthis), pfoo);
}

static inline int SPECLIST_FOO(_list_size) (const SPECLIST_LIST *tthis)
{
  return genlist_size(GENLIST(tthis));
}

static inline SPECLIST_TYPE *SPECLIST_FOO(_list_get) (const SPECLIST_LIST *tthis, int index)
{
  return (SPECLIST_TYPE *) genlist_get(GENLIST(tthis), index);
}

static inline void SPECLIST_FOO(_list_append) (SPECLIST_LIST *tthis, SPECLIST_TYPE *pfoo)
{
  genlist_append(GENLIST(tthis), pfoo);
}

static inline void SPECLIST_FOO(_list_clear) (SPECLIST_LIST *tthis)
{
  genlist_clear(GENLIST(tthis));
}

static inline void SPECLIST_FOO(_list_free) (SPECLIST_LIST *tthis)
{
  genlist_free(GENLIST(tthis));
}

/****************************************************************************
  Return TRUE iff this element is in the list.

  This is an O(n) operation.  Hence, "search".
****************************************************************************/
static inline bool SPECLIST_FOO(_list_search) (SPECLIST_LIST *tthis,
                                               const SPECLIST_TYPE *pfoo)
{
  return genlist_search(GENLIST(tthis), pfoo);
}

static inline void SPECLIST_FOO(_list_sort) (SPECLIST_LIST * tthis,
                                             int (*compar) (const SPECLIST_TYPE *const *,
                                                            const SPECLIST_TYPE *const *))
{
  genlist_sort(GENLIST(tthis), (int (*)(const void *, const void *)) compar);
}

static inline void SPECLIST_FOO(_list_shuffle) (SPECLIST_LIST *tthis)
{
  genlist_shuffle(GENLIST(tthis));
}

static inline const struct genlist *SPECLIST_FOO(_list_base) (const SPECLIST_LIST *tthis)
{
  return GENLIST(tthis);
}

#undef SPECLIST
#undef GENLIST

#undef SPECLIST_TAG
#undef SPECLIST_TYPE
#undef SPECLIST_PASTE_
#undef SPECLIST_PASTE
#undef SPECLIST_LIST
#undef SPECLIST_FOO
