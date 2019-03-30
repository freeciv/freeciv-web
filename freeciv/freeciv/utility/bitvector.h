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
#ifndef FC__BITVECTOR_H
#define FC__BITVECTOR_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdlib.h> /* size_t */
#include <string.h> /* memset */

/* utility */
#include "log.h"
#include "support.h" /* bool, fc__attribute */

/* Yields TRUE iff the bit bit_no is set in val. */
#define TEST_BIT(val, bit_no)                                               \
  (((val) & (1u << (bit_no))) == (1u << (bit_no)))

/* Dynamic bitvectors */
struct dbv {
  int bits;
  unsigned char *vec;
};

void dbv_init(struct dbv *pdbv, int bits);
void dbv_resize(struct dbv *pdbv, int bits);
void dbv_free(struct dbv *pdbv);

int dbv_bits(struct dbv *pdbv);

bool dbv_isset(const struct dbv *pdbv, int bit);
bool dbv_isset_any(const struct dbv *pdbv);

void dbv_set(struct dbv *pdbv, int bit);
void dbv_set_all(struct dbv *pdbv);

void dbv_clr(struct dbv *pdbv, int bit);
void dbv_clr_all(struct dbv *pdbv);

bool dbv_are_equal(const struct dbv *pdbv1, const struct dbv *pdbv2);

void dbv_debug(struct dbv *pdbv);

/* Maximal size of a dynamic bitvector.
   Use a large value to be on the safe side (4Mbits = 512kbytes). */
#define MAX_DBV_LENGTH (4 * 1024 * 1024)

/* Static bitvectors. */
#define _BV_BYTES(bits)        ((((bits) - 1) / 8) + 1)
#define _BV_BYTE_INDEX(bits)   ((bits) / 8)
#define _BV_BITMASK(bit)       (1u << ((bit) & 0x7))
#ifdef FREECIV_DEBUG
#  define _BV_ASSERT(bv, bit)  fc_assert((bit) >= 0                         \
                                         && (bit) < (signed int) sizeof((bv).vec) * 8)
#else
#  define _BV_ASSERT(bv, bit)  (void)0
#endif
#define BV_ISSET(bv, bit)                                                   \
  (_BV_ASSERT(bv, bit),                                                     \
   ((bv).vec[_BV_BYTE_INDEX(bit)] & _BV_BITMASK(bit)) != 0)
#define BV_SET(bv, bit)                                                     \
  do {                                                                      \
    _BV_ASSERT(bv, bit);                                                    \
    (bv).vec[_BV_BYTE_INDEX(bit)] |= _BV_BITMASK(bit);                      \
  } while (FALSE)
#define BV_CLR(bv, bit)                                                     \
  do {                                                                      \
    _BV_ASSERT(bv, bit);                                                    \
    (bv).vec[_BV_BYTE_INDEX(bit)] &= ~_BV_BITMASK(bit);                     \
  } while (FALSE)
#define BV_SET_VAL(bv, bit, val)                                            \
  do {                                                                      \
    if (val) { BV_SET(bv, bit); } else { BV_CLR(bv, bit); }                 \
  } while (FALSE);
#define BV_CLR_ALL(bv)                                                      \
  do {                                                                      \
     memset((bv).vec, 0, sizeof((bv).vec));                                 \
  } while (FALSE)
#define BV_SET_ALL(bv)                                                      \
  do {                                                                      \
    memset((bv).vec, 0xff, sizeof((bv).vec));                               \
  } while (FALSE)

bool bv_check_mask(const unsigned char *vec1, const unsigned char *vec2,
                   size_t size1, size_t size2);
#define BV_CHECK_MASK(vec1, vec2)                                           \
  bv_check_mask((vec1).vec, (vec2).vec, sizeof((vec1).vec),                 \
                sizeof((vec2).vec))
#define BV_ISSET_ANY(vec) BV_CHECK_MASK(vec, vec)

bool bv_are_equal(const unsigned char *vec1, const unsigned char *vec2,
                  size_t size1, size_t size2);
#define BV_ARE_EQUAL(vec1, vec2)                                            \
  bv_are_equal((vec1).vec, (vec2).vec, sizeof((vec1).vec),                  \
               sizeof((vec2).vec))

void bv_set_all_from(unsigned char *vec_to,
                     const unsigned char *vec_from,
                     size_t size_to, size_t size_from);
#define BV_SET_ALL_FROM(vec_to, vec_from)                                 \
  bv_set_all_from((vec_to).vec, (vec_from).vec,                           \
                  sizeof((vec_to).vec), sizeof((vec_from).vec))

void bv_clr_all_from(unsigned char *vec_to,
                     const unsigned char *vec_from,
                     size_t size_to, size_t size_from);
#define BV_CLR_ALL_FROM(vec_to, vec_from)                                 \
  bv_clr_all_from((vec_to).vec, (vec_from).vec,                           \
                  sizeof((vec_to).vec), sizeof((vec_from).vec))

/* Used to make a BV typedef. Such types are usually called "bv_foo". */
#define BV_DEFINE(name, bits)                                               \
  typedef struct { unsigned char vec[_BV_BYTES(bits)]; } name

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* FC__BITVECTOR_H */
