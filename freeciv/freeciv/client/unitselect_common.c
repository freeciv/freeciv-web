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

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

/* common */
#include "fc_types.h"
#include "game.h"
#include "player.h"
#include "unit.h"
#include "unitlist.h"
#include "unittype.h"

/* client */
#include "client_main.h"
#include "control.h"

#include "unitselect_common.h"

static void usdlg_data_add_unit(struct usdata_hash *ushash,
                                const struct tile *ptile,
                                struct unit *punit);

static bool usdlg_check_unit_activity(const struct unit *punit,
                                      enum unit_activity act);
static bool usdlg_check_unit_location(const struct unit *punit,
                                      const struct tile *ptile,
                                      enum unit_select_location_mode loc);

static struct usdata *usdata_new(void);
static void usdata_destroy(struct usdata *data);

/*************************************************************************//**
  Create a unit selection data struct.
*****************************************************************************/
static struct usdata *usdata_new(void)
{
  enum unit_select_location_mode loc;
  enum unit_activity act;

  struct usdata *data = fc_calloc(1, sizeof(*data));

  data->utype = NULL;

  for (loc = unit_select_location_mode_begin();
       loc != unit_select_location_mode_end();
       loc = unit_select_location_mode_next(loc)) {
    for (act = 0; act < ACTIVITY_LAST; act++) {
      /* Create a unit list for all activities - even invalid once. */
      data->units[loc][act] = unit_list_new();
    }
  }

  return data;
}

/*************************************************************************//**
  Destroy a unit selection struct.
*****************************************************************************/
static void usdata_destroy(struct usdata *data)
{
  if (data != NULL) {
    enum unit_select_location_mode loc;
    enum unit_activity act;

    for (loc = unit_select_location_mode_begin();
         loc != unit_select_location_mode_end();
         loc = unit_select_location_mode_next(loc)) {
      for (act = 0; act < ACTIVITY_LAST; act++) {
        if (data->units[loc][act] != NULL) {
          unit_list_destroy(data->units[loc][act]);
        }
      }
    }
    free(data);
  }
}

/*************************************************************************//**
  Create a unit selection data set.
*****************************************************************************/
struct usdata_hash *usdlg_data_new(const struct tile *ptile)
{
  struct usdata_hash *ushash = usdata_hash_new();

  /* Add units of the player(s). */
  if (client_is_global_observer()) {
    players_iterate(pplayer) {
      unit_list_iterate(pplayer->units, punit) {
        usdlg_data_add_unit(ushash, ptile, punit);
      } unit_list_iterate_end;
    } players_iterate_end;
  } else {
    struct player *pplayer = client_player();
    unit_list_iterate(pplayer->units, punit) {
      usdlg_data_add_unit(ushash, ptile, punit);
    } unit_list_iterate_end;
  }

  /* Add units on the tile (SELLOC_UNITS). */
  unit_list_iterate(ptile->units, punit) {
    /* Save all top level transporters on the tile (SELLOC_UNITS) as 'idle'
     * units (ACTIVITY_IDLE). */
    if (!unit_transported(punit)) {
      struct usdata *data;
      struct unit_type *ptype = unit_type_get(punit);

      usdata_hash_lookup(ushash, utype_index(ptype), &data);

      if (!data) {
        data = usdata_new();
        data->utype = ptype;
        usdata_hash_insert(ushash, utype_index(ptype), data);
      }

      unit_list_append(data->units[SELLOC_UNITS][ACTIVITY_IDLE], punit);
    }
  } unit_list_iterate_end;

  return ushash;
}

/*************************************************************************//**
  Destroy a unit selection data set.
*****************************************************************************/
void usdlg_data_destroy(struct usdata_hash *ushash)
{
  usdata_hash_data_iterate(ushash, data) {
    usdata_destroy(data);
  } usdata_hash_data_iterate_end;
  usdata_hash_destroy(ushash);
}

/*************************************************************************//**
  Add a unit into the unit selection data hash.
*****************************************************************************/
static void usdlg_data_add_unit(struct usdata_hash *ushash,
                                const struct tile *ptile,
                                struct unit *punit)
{
  struct usdata *data;
  enum unit_activity act;
  struct unit_type *ptype = unit_type_get(punit);

  fc_assert_ret(ushash);
  fc_assert_ret(punit);

  usdata_hash_lookup(ushash, utype_index(ptype), &data);

  if (!data) {
    data = usdata_new();
    data->utype = ptype;
    usdata_hash_insert(ushash, utype_index(ptype), data);
  }

  for (act = 0; act < ACTIVITY_LAST; act++) {
    if (usdlg_check_unit_activity(punit, act)) {
      enum unit_select_location_mode loc;

      for (loc = unit_select_location_mode_begin();
           loc != unit_select_location_mode_end();
           loc = unit_select_location_mode_next(loc)) {
        if (usdlg_check_unit_location(punit, ptile, loc)) {
          unit_list_append(data->units[loc][act], punit);
        }
      }
    }
  }
}

/*************************************************************************//**
  Returns TRUE if the unit activity is equal to 'act'.
*****************************************************************************/
static bool usdlg_check_unit_activity(const struct unit *punit,
                                      enum unit_activity act)
{
  fc_assert_ret_val(punit != NULL, FALSE);

  return (punit->activity == act);
}

/*************************************************************************//**
  Returns TRUE if the unit locations corresponds to 'loc'.
*****************************************************************************/
static bool usdlg_check_unit_location(const struct unit *punit,
                                      const struct tile *ptile,
                                      enum unit_select_location_mode loc)
{
  struct tile *utile;

  fc_assert_ret_val(punit != NULL, FALSE);

  utile = unit_tile(punit);

  fc_assert_ret_val(utile != NULL, FALSE);

  switch (loc) {
  case SELLOC_UNITS:
    /* Special case - never TRUE. See usdlg_data_new(). */
    return FALSE;
    break;
  case SELLOC_TILE:
    if (utile == ptile) {
      return TRUE;
    }
    break;
  case SELLOC_CONT:
    if (!is_ocean(tile_terrain(utile))
        && utype_move_type(unit_type_get(punit)) == UMT_LAND
        && tile_continent(utile) == tile_continent(ptile)) {
      return TRUE;
    }
    break;
  case SELLOC_WORLD:
    return TRUE;
    break;
  case SELLOC_LAND:
    if (utype_move_type(unit_type_get(punit)) == UMT_LAND
        && (!is_ocean(tile_terrain(utile)) || tile_city(utile) != NULL)) {
      return TRUE;
    }
    break;
  case SELLOC_SEA:
    if (utype_move_type(unit_type_get(punit)) == UMT_SEA
        && (is_ocean(tile_terrain(utile)) || tile_city(utile) != NULL)) {
      return TRUE;
    }
    break;
  case SELLOC_BOTH:
    if (utype_move_type(unit_type_get(punit)) == UMT_BOTH) {
      return TRUE;
    }
    break;
  case SELLOC_COUNT:
    /* Invalid. */
    fc_assert_ret_val(loc != SELLOC_COUNT, FALSE);
    break;
  }

  return FALSE;
}
