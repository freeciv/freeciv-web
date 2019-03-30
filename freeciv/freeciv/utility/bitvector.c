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

#include "fc_prehdrs.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "mem.h"
#include "rand.h"
#include "string_vector.h"

#include "bitvector.h"

/* There are two types of bitvectors defined in this file:
   (1) bv_*  - static bitvectors; used for data which where the length is
               fixed (number of players; flags for enums; ...). They are
               named bv_* and the macros BV_* are defined.
   (2) dbv_* - dynamic bitvectors; its size is not known a priori but defined
               by the player (map known bitvectors). This bitvectors are
               given as 'struct dbv' and the information can be accessed
               using the functions dbv_*(). They uses the BV_* macros. */

/***********************************************************************//**
  Initialize a dynamic bitvector of size 'bits'. 'bits' must be greater
  than 0 and lower than the maximal size given by MAX_DBV_LENGTH. The
  bitvector is set to all clear.
***************************************************************************/
void dbv_init(struct dbv *pdbv, int bits)
{
  /* Here used to be asserts checking if pdbv->vec is NULL
   * and pdbv->bits is 0, but that's just broken. One would
   * assume that _init() function can be called when the thing
   * is currently uninitialized, i.e., can have any values.
   * Those fc_assert_ret()s caused this function to return
   * without actually initializing the structure, leading to
   * crash later. */

  fc_assert_ret(bits > 0 && bits < MAX_DBV_LENGTH);

  pdbv->bits = bits;
  pdbv->vec = fc_calloc(1, _BV_BYTES(pdbv->bits) * sizeof(*pdbv->vec));

  dbv_clr_all(pdbv);
}

/***********************************************************************//**
  Resize a dynamic bitvector. Create it if needed.
***************************************************************************/
void dbv_resize(struct dbv *pdbv, int bits)
{
  fc_assert_ret(bits > 0 && bits < MAX_DBV_LENGTH);

  if (pdbv->vec == NULL) {
    /* Initialise a new dbv. */
    dbv_init(pdbv, bits);
  } else {
    /* Resize an existing dbv. */
    fc_assert_ret(pdbv->bits != 0);

    if (bits != pdbv->bits) {
      pdbv->bits = bits;
      pdbv->vec = fc_realloc(pdbv->vec,
                             _BV_BYTES(pdbv->bits) * sizeof(*pdbv->vec));
    }

    dbv_clr_all(pdbv);
  }
}

/***********************************************************************//**
  Destroy a dynamic bitvector.
***************************************************************************/
void dbv_free(struct dbv *pdbv)
{
  if (pdbv != NULL) {
    free(pdbv->vec);
    pdbv->vec = NULL;

    pdbv->bits = 0;
  }
}

/***********************************************************************//**
  Returns the number of bits defined in a dynamic bitvector.
***************************************************************************/
int dbv_bits(struct dbv *pdbv)
{
  if (pdbv != NULL) {
    return pdbv->bits;
  }

  return -1;
}

/***********************************************************************//**
  Check if the bit 'bit' is set.
***************************************************************************/
bool dbv_isset(const struct dbv *pdbv, int bit)
{
  fc_assert_ret_val(pdbv != NULL, FALSE);
  fc_assert_ret_val(pdbv->vec != NULL, FALSE);
  fc_assert_ret_val(bit < pdbv->bits, FALSE);

  return ((pdbv->vec[_BV_BYTE_INDEX(bit)] & _BV_BITMASK(bit)) != 0);
}

/***********************************************************************//**
  Test if any bit is set.
***************************************************************************/
bool dbv_isset_any(const struct dbv *pdbv)
{
  fc_assert_ret_val(pdbv != NULL, FALSE);
  fc_assert_ret_val(pdbv->vec != NULL, FALSE);

  return bv_check_mask(pdbv->vec, pdbv->vec, _BV_BYTES(pdbv->bits),
                       _BV_BYTES(pdbv->bits));
}

/***********************************************************************//**
  Set the bit given by 'bit'.
***************************************************************************/
void dbv_set(struct dbv *pdbv, int bit)
{
  fc_assert_ret(pdbv != NULL);
  fc_assert_ret(pdbv->vec != NULL);
  fc_assert_ret(bit < pdbv->bits);

  pdbv->vec[_BV_BYTE_INDEX(bit)] |= _BV_BITMASK(bit);
}

/***********************************************************************//**
  Set all bits.
***************************************************************************/
void dbv_set_all(struct dbv *pdbv)
{
  fc_assert_ret(pdbv != NULL);
  fc_assert_ret(pdbv->vec != NULL);

  memset(pdbv->vec, 0xff, _BV_BYTES(pdbv->bits));
}

/***********************************************************************//**
  Clear the bit given by 'bit'.
***************************************************************************/
void dbv_clr(struct dbv *pdbv, int bit)
{
  fc_assert_ret(pdbv != NULL);
  fc_assert_ret(pdbv->vec != NULL);
  fc_assert_ret(bit < pdbv->bits);

  pdbv->vec[_BV_BYTE_INDEX(bit)] &= ~_BV_BITMASK(bit);
}

/***********************************************************************//**
  Clear all bits.
***************************************************************************/
void dbv_clr_all(struct dbv *pdbv)
{
  fc_assert_ret(pdbv != NULL);
  fc_assert_ret(pdbv->vec != NULL);

  memset(pdbv->vec, 0, _BV_BYTES(pdbv->bits));
}

/***********************************************************************//**
  Check if the two dynamic bitvectors are equal.
***************************************************************************/
bool dbv_are_equal(const struct dbv *pdbv1, const struct dbv *pdbv2)
{
  fc_assert_ret_val(pdbv1 != NULL, FALSE);
  fc_assert_ret_val(pdbv1->vec != NULL, FALSE);
  fc_assert_ret_val(pdbv2 != NULL, FALSE);
  fc_assert_ret_val(pdbv2->vec != NULL, FALSE);

  return bv_are_equal(pdbv1->vec, pdbv2->vec, _BV_BYTES(pdbv1->bits),
                      _BV_BYTES(pdbv2->bits));
}

/***********************************************************************//**
  Debug a dynamic bitvector.
***************************************************************************/
void dbv_debug(struct dbv *pdbv)
{
  char test_str[51];
  int i, j, bit;

  fc_assert_ret(pdbv != NULL);
  fc_assert_ret(pdbv->vec != NULL);

  for (i = 0; i < (pdbv->bits - 1) / 50 + 1; i++) {
    for (j = 0; j < 50; j++) {
      bit = i * 50 + j;
      if (bit >= pdbv->bits) {
        break;
      }
      test_str[j] = dbv_isset(pdbv, bit) ? '1' : '0';
    }
    test_str[j] = '\0';
    log_error("[%5d] %s", i, test_str);
  }
}

/***********************************************************************//**
  Return whether two vectors: vec1 and vec2 have common
  bits. I.e. (vec1 & vec2) != 0.

  Don't call this function directly, use BV_CHECK_MASK macro
  instead. Don't call this function with two different bitvectors.
***************************************************************************/
bool bv_check_mask(const unsigned char *vec1, const unsigned char *vec2,
                   size_t size1, size_t size2)
{
  size_t i;
  fc_assert_ret_val(size1 == size2, FALSE);

  for (i = 0; i < size1; i++) {
    if ((vec1[0] & vec2[0]) != 0) {
      return TRUE;
    }
    vec1++;
    vec2++;
  }
  return FALSE;
}

/***********************************************************************//**
  Compares elements of two bitvectors. Both vectors are expected to have
  same number of elements, i.e. , size1 must be equal to size2.
***************************************************************************/
bool bv_are_equal(const unsigned char *vec1, const unsigned char *vec2,
                  size_t size1, size_t size2)
{
  size_t i;
  fc_assert_ret_val(size1 == size2, FALSE);

  for (i = 0; i < size1; i++) {
    if (vec1[0] != vec2[0]) {
      return FALSE;
    }
    vec1++;
    vec2++;
  }
  return TRUE;
}

/***********************************************************************//**
  Set everything that is true in vec_from in vec_to. Stuff that already is
  true in vec_to aren't touched. (Bitwise inclusive OR assignment)

  Both vectors are expected to have same number of elements,
  i.e. , size1 must be equal to size2.

  Don't call this function directly, use BV_SET_ALL_FROM macro
  instead.
***************************************************************************/
void bv_set_all_from(unsigned char *vec_to,
                     const unsigned char *vec_from,
                     size_t size_to, size_t size_from)
{
  size_t i;

  fc_assert_ret(size_to == size_from);

  for (i = 0; i < size_to; i++) {
    vec_to[i] |= vec_from[i];
  }
}

/***********************************************************************//**
  Clear everything that is true in vec_from in vec_to. Stuff that already
  is false in vec_to aren't touched.

  Both vectors are expected to have same number of elements,
  i.e. , size1 must be equal to size2.

  Don't call this function directly, use BV_CLR_ALL_FROM macro
  instead.
***************************************************************************/
void bv_clr_all_from(unsigned char *vec_to,
                     const unsigned char *vec_from,
                     size_t size_to, size_t size_from)
{
  size_t i;

  fc_assert_ret(size_to == size_from);

  for (i = 0; i < size_to; i++) {
    vec_to[i] &= ~vec_from[i];
  }
}
