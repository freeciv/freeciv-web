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
#ifndef FC__MAPHAND_H
#define FC__MAPHAND_H

#include "fc_types.h"

#include "map.h"
#include "packets.h"
#include "terrain.h"
#include "vision.h"

#include "hand_gen.h"

struct section_file;
struct conn_list;


struct player_tile {
  struct vision_site *site;		/* NULL for no vision site */
  struct resource *resource;		/* NULL for no resource */
  struct terrain *terrain;		/* NULL for unknown tiles */
  struct player *owner; 		/* NULL for unowned */
  bv_special special;
  bv_bases bases;

  /* If you build a city with an unknown square within city radius
     the square stays unknown. However, we still have to keep count
     of the seen points, so they are kept in here. When the tile
     then becomes known they are moved to seen. */
  unsigned short own_seen[V_COUNT];
  unsigned short seen_count[V_COUNT];
  short last_updated;
};

void global_warming(int effect);
void nuclear_winter(int effect);
void upgrade_city_rails(struct player *pplayer, bool discovery);

void give_map_from_player_to_player(struct player *pfrom, struct player *pdest);
void give_seamap_from_player_to_player(struct player *pfrom, struct player *pdest);
void give_citymap_from_player_to_player(struct city *pcity,
					struct player *pfrom, struct player *pdest);
void send_all_known_tiles(struct conn_list *dest, bool force);

bool send_tile_suppression(bool now);
void send_tile_info(struct conn_list *dest, struct tile *ptile,
                    bool send_unknown, bool force);

void send_map_info(struct conn_list *dest);

void map_show_tile(struct player *pplayer, struct tile *ptile);
void map_show_circle(struct player *pplayer,
		     struct tile *ptile, int radius_sq);
void map_refog_circle(struct player *pplayer, struct tile *ptile,
                      int old_radius_sq, int new_radius_sq,
                      bool can_reveal_tiles,
                      enum vision_layer vlayer);
void map_show_all(struct player *pplayer);

bool map_is_known_and_seen(const struct tile *ptile, struct player *pplayer,
			   enum vision_layer vlayer);
void map_change_seen(struct tile *ptile, struct player *pplayer, int change,
		     enum vision_layer vlayer);
bool map_is_known(const struct tile *ptile, const struct player *pplayer);
void map_set_known(struct tile *ptile, struct player *pplayer);
void map_clear_known(struct tile *ptile, struct player *pplayer);
void map_know_and_see_all(struct player *pplayer);
void show_map_to_all(void);

void player_map_allocate(struct player *pplayer);
void player_map_free(struct player *pplayer);

struct vision_site *map_get_player_city(const struct tile *ptile,
					const struct player *pplayer);
struct vision_site *map_get_player_site(const struct tile *ptile,
					const struct player *pplayer);
struct player_tile *map_get_player_tile(const struct tile *ptile,
					const struct player *pplayer);
bool update_player_tile_knowledge(struct player *pplayer,struct tile *ptile);
void update_tile_knowledge(struct tile *ptile);
void update_player_tile_last_seen(struct player *pplayer, struct tile *ptile);

void give_shared_vision(struct player *pfrom, struct player *pto);
void remove_shared_vision(struct player *pfrom, struct player *pto);
bool really_gives_vision(struct player *me, struct player *them);

void enable_fog_of_war(void);
void disable_fog_of_war(void);
void enable_fog_of_war_player(struct player *pplayer);
void disable_fog_of_war_player(struct player *pplayer);

void map_calculate_borders(void);
void map_claim_border(struct tile *ptile, struct player *powner);
void map_claim_ownership(struct tile *ptile, struct player *powner,
                         struct tile *psource);
void map_clear_border(struct tile *ptile);

void check_terrain_change(struct tile *ptile, struct terrain *oldter);
bool need_to_fix_terrain_change(const struct terrain *oldter,
                                const struct terrain *newter);
void fix_tile_on_terrain_change(struct tile *ptile,
                                bool extend_rivers);

void vision_change_sight(struct vision *vision, enum vision_layer vlayer,
			 int radius_sq);
void vision_clear_sight(struct vision *vision);

void change_playertile_site(struct player_tile *ptile,
                            struct vision_site *new_site);

void create_base(struct tile *ptile, struct base_type *pbase,
                 struct player *pplayer);

#endif  /* FC__MAPHAND_H */
