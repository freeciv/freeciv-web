/**********************************************************************
 Freeciv - Copyright (C) 2003 - The Freeciv Project
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
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#include "log.h"
#include "mem.h"
#include "shared.h"

#include "game.h"
#include "improvement.h"
#include "map.h"
#include "player.h"
#include "specialist.h"
#include "unit.h"
#include "unitlist.h"

#include "score.h"
#include "srv_main.h"

static int get_civ_score(const struct player *pplayer);
static int get_spaceship_score(const struct player *pplayer);

/**************************************************************************
  Allocates, fills and returns a land area claim map.
  Call free_landarea_map(&cmap) to free allocated memory.
**************************************************************************/

#define USER_AREA_MULT 1000

struct claim_map {
  struct {
    int landarea, settledarea;
  } player[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
};

/**************************************************************************
Land Area Debug...
**************************************************************************/

#define LAND_AREA_DEBUG 0

#if LAND_AREA_DEBUG >= 2

static char when_char(int when)
{
  static char list[] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z'
  };

  return (when >= 0 && when < sizeof(list)) ? list[when] : '?';
}

/* 
 * Writes the map_char_expr expression for each position on the map.
 * map_char_expr is provided with the variables x,y to evaluate the
 * position. The 'type' argument is used for formatting by printf; for
 * instance it should be "%c" for characters.  The data is printed in a
 * native orientation to make it easier to read.  
 */
#define WRITE_MAP_DATA(type, map_char_expr)        \
{                                                  \
  int nat_x, nat_y;                                \
  for (nat_x = 0; nat_x < map.xsize; nat_x++) {    \
    printf("%d", nat_x % 10);                      \
  }                                                \
  putchar('\n');                                   \
  for (nat_y = 0; nat_y < map.ysize; nat_y++) {    \
    printf("%d ", nat_y % 10);                     \
    for (nat_x = 0; nat_x < map.xsize; nat_x++) {  \
      int x, y;                                    \
      NATIVE_TO_MAP_POS(&x, &y, nat_x,nat_y);      \
      printf(type, map_char_expr);                 \
    }                                              \
    printf(" %d\n", nat_y % 10);                   \
  }                                                \
}

/**************************************************************************
  Prints the landarea map to stdout (a debugging tool).
*******************************************o*******************************/
static void print_landarea_map(struct claim_map *pcmap, int turn)
{
  int p;

  if (turn == 0) {
    putchar('\n');
  }

  if (turn == 0) {
    printf("Player Info...\n");

    for (p = 0; p < player_count(); p++) {
      printf(".know (%d)\n  ", p);
      WRITE_MAP_DATA("%c",
		     TEST_BIT(pcmap->claims[map_pos_to_index(x, y)].know,
			      p) ? 'X' : '-');
      printf(".cities (%d)\n  ", p);
      WRITE_MAP_DATA("%c",
		     TEST_BIT(pcmap->
			      claims[map_pos_to_index(x, y)].cities,
			      p) ? 'O' : '-');
    }
  }

  printf("Turn %d (%c)...\n", turn, when_char (turn));

  printf(".whom\n  ");
  WRITE_MAP_DATA((pcmap->claims[map_pos_to_index(x, y)].whom ==
		  32) ? "%c" : "%X",
		 (pcmap->claims[map_pos_to_index(x, y)].whom ==
		  32) ? '-' : pcmap->claims[map_pos_to_index(x, y)].whom);

  printf(".when\n  ");
  WRITE_MAP_DATA("%c", when_char(pcmap->claims[map_pos_to_index(x, y)].when));
}

#endif

/****************************************************************************
  Count landarea, settled area, and claims map for all players.
****************************************************************************/
static void build_landarea_map(struct claim_map *pcmap)
{
  bv_player claims[MAP_INDEX_SIZE];

  memset(claims, 0, sizeof(claims));
  memset(pcmap, 0, sizeof(*pcmap));

  /* First calculate claims: which tiles are owned by each player. */
  players_iterate(pplayer) {
    city_list_iterate(pplayer->cities, pcity) {
      struct tile *pcenter = city_tile(pcity);

      city_tile_iterate(pcenter, tile1) {
	BV_SET(claims[tile_index(tile1)], player_index(city_owner(pcity)));
      } city_tile_iterate_end;
    } city_list_iterate_end;
  } players_iterate_end;

  whole_map_iterate(ptile) {
    struct player *owner = NULL;
    bv_player *pclaim = &claims[tile_index(ptile)];

    if (is_ocean_tile(ptile)) {
      /* Nothing. */
    } else if (NULL != tile_city(ptile)) {
      owner = city_owner(tile_city(ptile));
      pcmap->player[player_index(owner)].settledarea++;
    } else if (NULL != tile_worked(ptile)) {
      owner = city_owner(tile_worked(ptile));
      pcmap->player[player_index(owner)].settledarea++;
    } else if (unit_list_size(ptile->units) > 0) {
      /* Because of allied stacking these calculations are a bit off. */
      owner = unit_owner(unit_list_get(ptile->units, 0));
      if (BV_ISSET(*pclaim, player_index(owner))) {
	pcmap->player[player_index(owner)].settledarea++;
      }
    }

    if (game.info.borders > 0) {
      /* If borders are enabled, use owner information directly from the
       * map.  Otherwise use the calculations above. */
      owner = tile_owner(ptile);
    }
    if (owner) {
      pcmap->player[player_index(owner)].landarea++;
    }
  } whole_map_iterate_end;

#if LAND_AREA_DEBUG >= 2
  print_landarea_map(pcmap, turn);
#endif
}

/**************************************************************************
  Returns the given player's land and settled areas from a claim map.
**************************************************************************/
static void get_player_landarea(struct claim_map *pcmap,
				struct player *pplayer,
				int *return_landarea,
				int *return_settledarea)
{
  if (pcmap && pplayer) {
#if LAND_AREA_DEBUG >= 1
    printf("%-14s", player_name(pplayer));
#endif
    if (return_landarea) {
      *return_landarea
	= USER_AREA_MULT * pcmap->player[player_index(pplayer)].landarea;
#if LAND_AREA_DEBUG >= 1
      printf(" l=%d", *return_landarea / USER_AREA_MULT);
#endif
    }
    if (return_settledarea) {
      *return_settledarea
	= USER_AREA_MULT * pcmap->player[player_index(pplayer)].settledarea;
#if LAND_AREA_DEBUG >= 1
      printf(" s=%d", *return_settledarea / USER_AREA_MULT);
#endif
    }
#if LAND_AREA_DEBUG >= 1
    printf("\n");
#endif
  }
}

/**************************************************************************
  Calculates the civilization score for the player.
**************************************************************************/
void calc_civ_score(struct player *pplayer)
{
  struct city *pcity;
  int landarea = 0, settledarea = 0;
  static struct claim_map cmap;

  pplayer->score.happy = 0;
  pplayer->score.content = 0;
  pplayer->score.unhappy = 0;
  pplayer->score.angry = 0;
  specialist_type_iterate(sp) {
    pplayer->score.specialists[sp] = 0;
  } specialist_type_iterate_end;
  pplayer->score.wonders = 0;
  pplayer->score.techs = 0;
  pplayer->score.techout = 0;
  pplayer->score.landarea = 0;
  pplayer->score.settledarea = 0;
  pplayer->score.population = 0;
  pplayer->score.cities = 0;
  pplayer->score.units = 0;
  pplayer->score.pollution = 0;
  pplayer->score.bnp = 0;
  pplayer->score.mfg = 0;
  pplayer->score.literacy = 0;
  pplayer->score.spaceship = 0;

  if (is_barbarian(pplayer)) {
    return;
  }

  city_list_iterate(pplayer->cities, pcity) {
    int bonus;

    pplayer->score.happy += pcity->feel[CITIZEN_HAPPY][FEELING_FINAL];
    pplayer->score.content += pcity->feel[CITIZEN_CONTENT][FEELING_FINAL];
    pplayer->score.unhappy += pcity->feel[CITIZEN_UNHAPPY][FEELING_FINAL];
    pplayer->score.angry += pcity->feel[CITIZEN_ANGRY][FEELING_FINAL];
    specialist_type_iterate(sp) {
      pplayer->score.specialists[sp] += pcity->specialists[sp];
    } specialist_type_iterate_end;
    pplayer->score.population += city_population(pcity);
    pplayer->score.cities++;
    pplayer->score.pollution += pcity->pollution;
    pplayer->score.techout += pcity->prod[O_SCIENCE];
    pplayer->score.bnp += pcity->surplus[O_TRADE];
    pplayer->score.mfg += pcity->surplus[O_SHIELD];

    bonus = get_final_city_output_bonus(pcity, O_SCIENCE) - 100;
    bonus = CLIP(0, bonus, 100);
    pplayer->score.literacy += (city_population(pcity) * bonus) / 100;
  } city_list_iterate_end;

  build_landarea_map(&cmap);

  get_player_landarea(&cmap, pplayer, &landarea, &settledarea);
  pplayer->score.landarea = landarea;
  pplayer->score.settledarea = settledarea;

  advance_index_iterate(A_FIRST, i) {
    if (player_invention_state(pplayer, i) == TECH_KNOWN) {
      pplayer->score.techs++;
    }
  } advance_index_iterate_end;
  pplayer->score.techs += get_player_research(pplayer)->future_tech * 5 / 2;
  
  unit_list_iterate(pplayer->units, punit) {
    if (is_military_unit(punit)) {
      pplayer->score.units++;
    }
  } unit_list_iterate_end

  improvement_iterate(i) {
    if (is_great_wonder(i)
	&& (pcity = find_city_from_great_wonder(i))
	&& player_owns_city(pplayer, pcity)) {
      pplayer->score.wonders++;
    }
  } improvement_iterate_end;

  pplayer->score.spaceship = pplayer->spaceship.state;

  pplayer->score.game = get_civ_score(pplayer);
}

/**************************************************************************
  Return the civilization score (a numerical value) for the player.
**************************************************************************/
static int get_civ_score(const struct player *pplayer)
{
  /* We used to count pplayer->score.happy here too, but this is too easily
   * manipulated by players at the endrturn. */
  return (total_player_citizens(pplayer)
	  + pplayer->score.techs * 2
	  + pplayer->score.wonders * 5
	  + get_spaceship_score(pplayer));
}

/**************************************************************************
  Return the spaceship score
**************************************************************************/
static int get_spaceship_score(const struct player *pplayer)
{
  if (pplayer->score.spaceship == SSHIP_ARRIVED) {
    /* How much should a spaceship be worth?
     * This gives 100 points per 10,000 citizens. */
    return (int)(100 * pplayer->spaceship.habitation
		 * pplayer->spaceship.success_rate);
  } else {
    return 0;
  }
}

/**************************************************************************
  Return the total number of citizens in the player's nation.
**************************************************************************/
int total_player_citizens(const struct player *pplayer)
{
  int count = (pplayer->score.happy
	       + pplayer->score.content
	       + pplayer->score.unhappy
	       + pplayer->score.angry);

  specialist_type_iterate(sp) {
    count += pplayer->score.specialists[sp];
  } specialist_type_iterate_end;

  return count;
}

/**************************************************************************
 save a ppm file which is a representation of the map of the current turn.
 this can later be turned into a animated gif.

 terrain type, units, and cities are saved.
**************************************************************************/
void save_ppm(void)
{
  char filename[600];
  char tmpname[600];
  FILE *fp;
  int i, j;

  /* the colors for each player. these were selected to give
   * the most differentiation between all players. YMMV. */
  int col[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS][3] = {
    {255,   0,   0}, {  0, 128,   0}, {255, 255, 255}, {255, 255,   0},
    {138,  43, 226}, {255, 140,   0}, {  0, 255, 255}, {139,  69,  19},
    {211, 211, 211}, {255, 215,   0}, {255,  20, 147}, {124, 252,   0},
    {218, 112, 214}, { 30, 144, 255}, {250, 128, 114}, {154, 205,  50},
    { 25,  25, 112}, {  0, 255, 127}, {139,   0,   0}, {100, 149, 237},
    {  0, 128, 128}, {255, 192, 203}, {255, 250, 205}, {119, 136, 153},
    {255, 127,  80}, {255,   0, 255}, {128, 128,   0}, {245, 222, 179},
    {184, 134,  11}, {173, 216, 230}, {102, 205, 170}, {255, 165,   0},
  };
  int watercol[3] = {0,0,255}; /* blue */
  int landcol[3] =  {0,0,0};   /* black */

  if (!srvarg.save_ppm) {
    return;
  }

  /* put this file in the same place we put savegames */
  my_snprintf(filename, sizeof(filename),
              "%s%+05d.int.ppm", game.server.save_name, game.info.year);

  /* Ensure the saves directory exists. */
  make_dir(srvarg.saves_pathname);

  sz_strlcpy(tmpname, srvarg.saves_pathname);
  if (tmpname[0] != '\0') {
    sz_strlcat(tmpname, "/");
  }
  sz_strlcat(tmpname, filename);
  sz_strlcpy(filename, tmpname);

  fp = fopen(filename, "w");

  if (!fp) {
    freelog(LOG_ERROR, "couldn't open file ppm save: %s\n", filename);
    return;
  }

  fprintf(fp, "P3\n# version:2\n# gameid: %s\n", server.game_identifier);
  fprintf(fp, "# An intermediate map from saved Freeciv game %s%+05d\n",
          game.server.save_name, game.info.year);

  players_iterate(pplayer) {
    fprintf(fp, "# playerno:%d:color:#%02x%02x%02x:name:\"%s\"\n", 
            player_number(pplayer),
            col[player_index(pplayer)][0],
            col[player_index(pplayer)][1],
            col[player_index(pplayer)][2],
            player_name(pplayer));
  } players_iterate_end;

  fprintf(fp, "%d %d\n", map.xsize, map.ysize);
  fprintf(fp, "255\n");

  for (j = 0; j < map.ysize; j++) {
    for (i = 0; i < map.xsize; i++) {
       struct tile *ptile = native_pos_to_tile(i, j);
       int *color;

       /* color for cities first, then units, then land */
       if (tile_city(ptile)) {
         color = col[player_index(tile_owner(ptile))];
       } else if (unit_list_size(ptile->units) > 0) {
         color = col[player_index(unit_owner(unit_list_get(ptile->units, 0)))];
       } else if (is_ocean_tile(ptile)) {
         color = watercol;
       } else {
         color = landcol;
       }

       fprintf(fp, "%d %d %d\n", color[0], color[1], color[2]);
    }
  }

  fclose(fp);
}

/**************************************************************************
  At the end of a game, figure the winners and losers of the game and
  output to a suitable place.

  The definition of winners and losers: a winner is one who is alive at the
  end of the game and has not surrendered, or in the case of a team game, 
  is alive or a teammate is alive and has not surrendered. A loser is
  surrendered or dead. Exception: the winner of the spacerace and his 
  teammates will win of course.

  Barbarians do not count as winners or losers.

  Games ending by endturn results in a draw, we don't rank in that case.
**************************************************************************/
void rank_users(void)
{
  FILE *fp;
  int i;
  enum victory_state { VS_NONE, VS_LOSER, VS_WINNER };
  enum victory_state plr_state[MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS];
  struct player *spacerace_winner = NULL;

  /* game ending via endturn results in a draw. We don't rank. */
  if (game.info.turn > game.info.end_turn) {
    return;
  }

  /* don't output ranking info if we haven't enabled it via cmdline */
  if (!srvarg.ranklog_filename) {
    return;
  }

  fp = fopen(srvarg.ranklog_filename,"w");

  /* don't fail silently, at least print an error */
  if (!fp) {
    freelog(LOG_ERROR, "couldn't open ranking log file: \"%s\"",
            srvarg.ranklog_filename);
    return;
  }

  /* initialize plr_state */
  for (i = 0; i < MAX_NUM_PLAYERS + MAX_NUM_BARBARIANS; i++) {
    plr_state[i] = VS_NONE;
  }

  /* do we have a spacerace winner? */
  players_iterate(pplayer) {
    if (pplayer->spaceship.state == SSHIP_ARRIVED) {
      spacerace_winner = pplayer;
      break;
    }
  } players_iterate_end;

  /* make this easy: if we have a spacerace winner, then treat all others
   * who are still alive as surrendered */
  if (spacerace_winner) {
    players_iterate(pplayer) {
      if (pplayer != spacerace_winner) {
        pplayer->surrendered = TRUE;
      }
    } players_iterate_end;
  }

  /* first pass: locate those alive who haven't surrendered, set them to win;
   * barbarians won't count, and everybody else is a loser for now. */
  players_iterate(pplayer) {
    if (is_barbarian(pplayer)) {
      plr_state[player_index(pplayer)] = VS_NONE;
    } else if (pplayer->is_alive && !pplayer->surrendered) {
      plr_state[player_index(pplayer)] = VS_WINNER;
    } else {
      plr_state[player_index(pplayer)] = VS_LOSER;
    }
  } players_iterate_end;

  /* second pass: find the teammates of those winners, they win too. */
  players_iterate(pplayer) {
    if (plr_state[player_index(pplayer)] == VS_WINNER) {
      players_iterate(aplayer) {
        if (aplayer->team == pplayer->team) {
          plr_state[player_index(aplayer)] = VS_WINNER;
        }
      } players_iterate_end;
    }
  } players_iterate_end;

  /* write out ranking information to file */
  fprintf(fp, "turns: %d\n", game.info.turn);
  fprintf(fp, "winners: ");
  players_iterate(pplayer) {
    if (plr_state[player_index(pplayer)] == VS_WINNER) {
      fprintf(fp, "%s (%s,%s), ", pplayer->ranked_username,
                                  player_name(pplayer),
                                  pplayer->username);
    }
  } players_iterate_end;
  fprintf(fp, "\nlosers: ");
  players_iterate(pplayer) {
    if (plr_state[player_index(pplayer)] == VS_LOSER) {
      fprintf(fp, "%s (%s,%s), ", pplayer->ranked_username,
                                  player_name(pplayer),
                                  pplayer->username);
    }
  } players_iterate_end;
  fprintf(fp, "\n");

  fclose(fp);
}
