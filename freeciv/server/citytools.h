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
#ifndef FC__CITYTOOLS_H
#define FC__CITYTOOLS_H

#include "events.h"		/* enum event_type */
#include "packets.h"
#include "unitlist.h"

#define FOOD_WEIGHTING 19
#define SHIELD_WEIGHTING 17
#define TRADE_WEIGHTING 12
/* The Trade Weighting has to about as large as the Shield Weighting,
   otherwise the AI will build Barracks to create veterans in cities 
   with only 1 shields production.
    8 is too low
   18 is too high
 */
#define POLLUTION_WEIGHTING 14 /* tentative */
#define WARMING_FACTOR 50
#define COOLING_FACTOR WARMING_FACTOR

int build_points_left(struct city *pcity);
int do_make_unit_veteran(struct city *pcity,
			 const struct unit_type *punittype);

void transfer_city_units(struct player *pplayer, struct player *pvictim, 
			 struct unit_list *units, struct city *pcity,
			 struct city *exclude_city,
			 int kill_outside, bool verbose);
void transfer_city(struct player *ptaker, struct city *pcity,
		   int kill_outside, bool transfer_unit_verbose,
		   bool resolve_stack, bool raze);
struct city *find_closest_owned_city(const struct player *pplayer,
                                     const struct tile *ptile,
                                     bool sea_required,
                                     const struct city *pexclcity);
void unit_enter_city(struct unit *punit, struct city *pcity, bool passenger);

bool send_city_suppression(bool now);
void send_city_info(struct player *dest, struct city *pcity);
void send_city_info_at_tile(struct player *pviewer, struct conn_list *dest,
			    struct city *pcity, struct tile *ptile);
void send_all_known_cities(struct conn_list *dest);
void send_player_cities(struct player *pplayer);
void package_city(struct city *pcity, struct packet_city_info *packet,
		  bool dipl_invest);

void reality_check_city(struct player *pplayer, struct tile *ptile);
bool update_dumb_city(struct player *pplayer, struct city *pcity);
void refresh_dumb_city(struct city *pcity);
void remove_dumb_city(struct player *pplayer, struct tile *ptile);

void create_city(struct player *pplayer, struct tile *ptile,
		 const char *name);
void remove_city(struct city *pcity);

void establish_trade_route(struct city *pc1, struct city *pc2);
void remove_trade_route(struct city *pc1, struct city *pc2);

void do_sell_building(struct player *pplayer, struct city *pcity,
		      struct impr_type *pimprove);
void building_lost(struct city *pcity, const struct impr_type *pimprove);
void city_units_upkeep(const struct city *pcity);

bool is_production_equal(const struct universal *one,
			 const struct universal *two);
void change_build_target(struct player *pplayer, struct city *pcity,
			 struct universal target,
			 enum event_type event);

bool is_allowed_city_name(struct player *pplayer, const char *cityname,
			  char *error_buf, size_t bufsz);
char *city_name_suggestion(struct player *pplayer, struct tile *ptile);

void city_freeze_workers(struct city *pcity);
void city_thaw_workers(struct city *pcity);
void city_freeze_workers_queue(struct city *pcity);
void city_thaw_workers_queue(void);

/* city map functions */
enum city_tile_type city_map_status(const struct city *pcity, int city_x,
				    int city_y);
void city_map_update_empty(struct city *pcity, struct tile *ptile,
			   int city_x, int city_y);
void city_map_update_worker(struct city *pcity, struct tile *ptile,
			    int city_x, int city_y);

bool city_map_update_tile_frozen(struct tile *ptile);
bool city_map_update_tile_now(struct tile *ptile);

void city_map_update_all(struct city *pcity);
void city_map_update_all_cities_for_player(struct player *pplayer);

void city_landlocked_sell_coastal_improvements(struct tile *ptile);
void city_refresh_vision(struct city *pcity);

void sync_cities(void);

#endif  /* FC__CITYTOOLS_H */
