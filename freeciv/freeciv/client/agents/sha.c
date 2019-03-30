/***********************************************************************
 Freeciv - Copyright (C) 2004 - A. Gorshenev
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

/* utility */
#include "log.h"
#include "support.h"

/* common */
#include "game.h"
#include "map.h"
#include "unitlist.h"

/* client/agents */
#include "agents.h"

#include "sha.h"


/**************************************************************************
  This is the simple historian agent.
  It just saves the last states of all tiles and units.
  The trick is just to call this agent the last of all
  so it still keeps old values whereas all other agents
  already got the new ones.
**************************************************************************/

static struct tile *previous_tiles = NULL;
static struct unit_list *previous_units;

/**********************************************************************//**
  Tile changed callback
**************************************************************************/
static void sha_tile_update(struct tile *ptile)
{
  log_debug("sha got tile: %d ~= (%d, %d)",
            tile_index(ptile), TILE_XY(ptile));

#if 0
  previous_tiles[tile_index(ptile)] = *ptile;
#endif
}

/**********************************************************************//**
  Unit changed callback
**************************************************************************/
static void sha_unit_change(int id)
{
  struct unit *punit = game_unit_by_number(id);
  struct unit *pold_unit = unit_list_find(previous_units, id);

  log_debug("sha got unit: %d", id);

  fc_assert_ret(NULL != pold_unit);
  *pold_unit = *punit;
}

/**********************************************************************//**
  New unit callback
**************************************************************************/
static void sha_unit_new(int id)
{
  struct unit *punit = game_unit_by_number(id);
  struct unit *pold_unit = unit_virtual_create(unit_owner(punit), NULL, 0, 0);

  log_debug("sha got unit: %d", id);

  *pold_unit = *punit;
  unit_list_prepend(previous_units, pold_unit);
}

/**********************************************************************//**
  Unit removed callback
**************************************************************************/
static void sha_unit_remove(int id)
{
  struct unit *pold_unit = unit_list_find(previous_units, id);;

  log_debug("sha got unit: %d", id);

  fc_assert_ret(NULL != pold_unit);
  unit_list_remove(previous_units, pold_unit);
  /* list pointers were struct copied, cannot unit_virtual_destroy() */
  memset(pold_unit, 0, sizeof(*pold_unit)); /* ensure no pointers remain */
  free(pold_unit);
}

/**********************************************************************//**
  Initialize simple historian agent
**************************************************************************/
void simple_historian_init(void)
{
  struct agent self;

  previous_tiles = fc_malloc(MAP_INDEX_SIZE * sizeof(*previous_tiles));
  memset(previous_tiles, 0, MAP_INDEX_SIZE * sizeof(*previous_tiles));

  previous_units = unit_list_new();

  memset(&self, 0, sizeof(self));
  sz_strlcpy(self.name, "Simple Historian");

  self.level = LAST_AGENT_LEVEL;

  self.unit_callbacks[CB_REMOVE] = sha_unit_remove;
  self.unit_callbacks[CB_CHANGE] = sha_unit_change;
  self.unit_callbacks[CB_NEW] = sha_unit_new;
  self.tile_callbacks[CB_REMOVE] = sha_tile_update;
  self.tile_callbacks[CB_CHANGE] = sha_tile_update;
  self.tile_callbacks[CB_NEW] = sha_tile_update;
  register_agent(&self);
}

/**********************************************************************//**
  Free resources allocated for simple historian agent.
**************************************************************************/
void simple_historian_done(void)
{
  unit_list_destroy(previous_units);
}

/**************************************************************************
  Public interface
**************************************************************************/

/**********************************************************************//**
  Return pointer to tile as it was last reported to us.
**************************************************************************/
struct tile *sha_tile_recall(struct tile *ptile)
{
  return &previous_tiles[tile_index(ptile)];
}

/**********************************************************************//**
  Report pointer to unit as it was last reported to us.
**************************************************************************/
struct unit *sha_unit_recall(int id)
{
  return unit_list_find(previous_units, id);
}
