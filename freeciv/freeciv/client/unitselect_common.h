/*****************************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/
#ifndef FC__UNITSELECT_COMMON_H
#define FC__UNITSELECT_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct unit_type;
struct unit_list;
struct tile;

struct usdata {
  struct unit_type *utype;
  struct unit_list *units[SELLOC_COUNT][ACTIVITY_LAST];
};

#define SPECHASH_TAG usdata
#define SPECHASH_INT_KEY_TYPE
#define SPECHASH_IDATA_TYPE struct usdata *
#include "spechash.h"

#define usdata_hash_data_iterate(phash, data)                                \
  TYPED_HASH_DATA_ITERATE(struct usdata *, phash, data)
#define usdata_hash_data_iterate_end                                         \
  HASH_DATA_ITERATE_END

struct usdata_hash *usdlg_data_new(const struct tile *ptile);
void usdlg_data_destroy(struct usdata_hash *ushash);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* FC__UNITSELECT_COMMON_H */
